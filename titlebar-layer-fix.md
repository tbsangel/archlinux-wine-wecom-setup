# 顶部横条盖在其他窗口之上的修复

2026-09-30，用户报告：企业微信未最小化时，切换到其他应用后，顶部拖动/最小化/关闭横条仍露在最前。修复后，用户确认切换遮挡、拖动、最小化和恢复均正常。

## 原因与证据

企业微信将顶部 28 像素横条单独实现为 `TitleBarWindow`，它与主窗口 `WeWorkWindow` 是不同的顶层窗口。它的 owner 是主窗口，原始样式为：

```text
STYLE   = 0x96000000
EXSTYLE = 0x08080080
```

扩展样式包含 `WS_EX_LAYERED | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE`，不包含 `WS_EX_TOPMOST`。因此这个现象不是企业微信普通“置顶”设置。

[Wine X11 驱动的 `is_window_managed()`](https://github.com/wine-mirror/wine/blob/master/dlls/winex11.drv/window.c)根据样式和激活状态判断是否交给桌面窗口管理器。这个不激活、没有标准标题栏/系统菜单的 popup 未命中托管条件；修复前，X11 标题栏不在正常受管窗口列表中，没有 `WM_STATE`，与绕过窗口管理的行为一致。主窗口退到后台或原生 Wayland 窗口激活时，这条辅助横栏仍可能显示在上层。

## 采用的调整

只给当前主进程中、owner 为主窗口的 `TitleBarWindow` 增加 `WS_SYSMENU (0x00080000)`，随后调用 `SetWindowPos` 刷新样式：

```text
STYLE   : 0x96000000 → 0x96080000
EXSTYLE : 保持 0x08080080
```

Wine 会将带系统菜单样式的 popup 纳入窗口管理。现有 layered/toolwindow 属性继续抑制额外系统装饰。刷新时使用 `NOMOVE | NOSIZE | NOZORDER | NOACTIVATE | FRAMECHANGED`，不移动、缩放或主动激活窗口。

调整后读取到 `WM_STATE=Normal`，标题栏进入 `_NET_CLIENT_LIST_STACKING`，`WM_TRANSIENT_FOR` 仍为企业微信主窗口；保留 `SKIP_TASKBAR/SKIP_PAGER/SKIP_SWITCHER`，没有多出任务栏项。宽高位置未变，用户复测通过。

这是针对当前 Wine X11/XWayland 路径与企业微信窗口实现的兼容处理，没有修改系统 Wine、企业微信 DLL、CEF、Qt DPI 或旧稳定 prefix。

## 启动时自动应用

辅助工具源码与二进制在 [scripts/titlebar-managed.c](scripts/titlebar-managed.c)、[scripts/titlebar-managed.exe](scripts/titlebar-managed.exe)。

| 参数 | 行为 |
|---|---|
| 无参数 | 只打印匹配标题栏的样式 |
| `-apply` | 对当前匹配标题栏调整一次 |
| `-watch` | 启动时最多等待主窗口 120 秒，随后每秒检查一次；主进程退出时结束 |

检查同时限定窗口类、owner 和进程 ID，不处理 tooltip、阴影或其他应用。互斥量防止同一 prefix 启动多个维护实例。该工具是随企业微信运行的小型辅助进程，不是全系统服务。

当前已将启动脚本、工具和覆盖列表复制到：

```text
/home/<user>/Works/Softwares/wine/WeCom/wecom-compat/
├── scripts/launch-wecom.sh
├── scripts/titlebar-managed.exe
├── scripts/titlebar-managed.c
└── configs/dll-overrides.txt
```

`WeCom-Native-Wine.desktop` 已改用这份 prefix 内部启动脚本，不依赖工作区文档目录。标题栏工具的少量输出写入 `<prefix>/wecom-titlebar.log`。直接绕过启动脚本调用 `wine WXWork.exe` 不会自动启动新维护实例；请使用桌面入口。

本次在正在运行的应用中验证了样式调整和用户交互，并安装了启动接入、为当前主进程启动辅助程序；没有为此再次重启正在使用的企业微信。完整进程重启后的持久化行为尚未另行人工复测。

## 回退与升级

先从企业微信菜单正常退出，使辅助程序随主进程结束，再以 `WECOM_TITLEBAR_FIX=0` 调用同一启动脚本，即可停用此项，保留 CEF、运行库和字体配置。例如：

```bash
env WECOM_PREFIX="$HOME/Works/Softwares/wine/WeCom" WECOM_TITLEBAR_FIX=0 \
  /bin/bash "$HOME/Works/Softwares/wine/WeCom/wecom-compat/scripts/launch-wecom.sh"
```

仅最小化或设置环境变量不会还原已经创建的标题栏；应退出并重新启动。也可以恢复自己修改前备份的桌面入口；本次调查的原入口备份 `desktop-before-titlebar.desktop` 仅保留在调查主机，未上传。

升级 Wine 或企业微信后重新验证遮挡、拖动、最小化/恢复、任务栏项及多显示器行为。若应用更改窗口类名或 owner 结构，辅助工具会因匹配不到而不修改，需要重新检查。不要在未验证时将该样式修改扩大到全部窗口。

记录：[验证摘要](evidence/verification-summary.json)。原始 `titlebar-fix.json`、`titlebar-inspect.txt`、`titlebar-apply.txt` 仅保留在调查主机。
