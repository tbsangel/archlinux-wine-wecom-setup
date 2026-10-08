# 使用与维护

从空 prefix 安装见[安装步骤](install-from-scratch.md)，配置原因见[配置说明](configuration-rationale.md)。以下命令在普通用户的 Bash 终端执行，路径按实际环境调整。

## 日常启动与开关

打开 `WeCom-Native-Wine.desktop`，或从终端运行：

```bash
export WECOM_PREFIX="$HOME/Works/Softwares/wine/WeCom"
test -f "$WECOM_PREFIX/system.reg"
bash "$WECOM_PREFIX/wecom-compat/scripts/launch-wecom.sh"
```

启动器从 prefix 内加载配套文件，日常运行不依赖仓库。直接执行 `wine WXWork.exe` 会跳过这些检查与窗口修复。

| 环境变量 | 设置为 `0` 时 |
|---|---|
| `WECOM_TITLEBAR_FIX` | 停用标题栏、边缘缩放、托盘坐标、菜单关闭和点击激活修复；保留启动前锁检查与图标修复 |
| `WECOM_TRAY_FIX` | 只停用托盘菜单和悬浮提示的坐标修正 |
| `WECOM_TRAY_ICON_FIX` | 只停用托盘黑块的空白占位修正 |

开关在启动时读取；先完整退出企业微信，再设置开关启动。例如：

```bash
env WECOM_TRAY_FIX=0 bash "$WECOM_PREFIX/wecom-compat/scripts/launch-wecom.sh"
```

改变屏幕排列或缩放后，也需退出再启动以重新计算。仅最小化或重复点击桌面入口不会刷新已运行的辅助程序。

## 更新启动辅助文件

先保存工作并退出目标企业微信。以下命令备份原辅助文件，再同步仓库版本，不重装应用、字体或 CEF：

```bash
set -euo pipefail
export WECOM_SETUP="/path/to/archlinux-wine-wecom-setup"
export WECOM_PREFIX="$HOME/Works/Softwares/wine/WeCom"
test -f "$WECOM_PREFIX/system.reg"
test -d "$WECOM_PREFIX/wecom-compat/scripts"
test -f "$WECOM_SETUP/scripts/titlebar-managed.exe"

env WINEPREFIX="$WECOM_PREFIX" /usr/bin/wineserver -k
env WINEPREFIX="$WECOM_PREFIX" /usr/bin/wineserver -w
mkdir -p -- "$WECOM_PREFIX/setup-backups"
compat_backup="$(mktemp -d "$WECOM_PREFIX/setup-backups/compat-update-XXXXXXXX")"
cp -a -- "$WECOM_PREFIX/wecom-compat" "$compat_backup/"
cp -- "$WECOM_SETUP/scripts/launch-wecom.sh" "$WECOM_SETUP/scripts/prepare-startup.py" \
  "$WECOM_SETUP/scripts/tray-geometry.py" "$WECOM_SETUP/scripts/tray-icon.py" \
  "$WECOM_SETUP/scripts/titlebar-managed.c" \
  "$WECOM_SETUP/scripts/titlebar-managed.exe" "$WECOM_PREFIX/wecom-compat/scripts/"
cp -- "$WECOM_SETUP/configs/dll-overrides.txt" "$WECOM_PREFIX/wecom-compat/configs/"
bash "$WECOM_PREFIX/wecom-compat/scripts/launch-wecom.sh"
```

源码、EXE、脚本和覆盖列表配套更新；不要覆盖仍由 Wine 映射的 EXE。更新后按下面清单验收。恢复旧辅助版本时，停止目标 prefix，再从 `setup-backups/compat-update-*/wecom-compat/` 还原。

## 验证范围

截至 **2026-10-08**，用户已确认文档列表及正文、中文输入及提示、启动速度、标题栏遮挡与激活、连续拖动和边缘缩放正常。启动修复后的一次主界面显示耗时约 4.4 秒。

托盘交互已确认：两块屏幕的右键菜单和悬浮提示位置正常；点击外部后菜单关闭，菜单内部操作正常；消息提醒在正常图标与空白占位之间闪烁；左键可以置前被遮挡的窗口，也能从最小化恢复。菜单关闭同时经过焦点切换探针验证，覆盖 Wine 未发出前台切换事件的情况。

标题栏间歇拖动失败在跟踪和最终回归中未再次出现，尚未定位独立原因。

邮件、长期稳定性及整理后的完整从零安装未单独验收。最后一轮窗口交互修复通过重启辅助程序在运行中的应用上验证，尚未验证最终组合的完整应用重启或宿主重启；其他屏幕排列与旋转也未覆盖。

`materials/versions.json` 保留首次安装时的 Wine 11.18 及材料版本，不代表当前运行状态；后续启动和窗口修复使用 Wine 11.19。材料一致性、脚本静态检查和用户实际操作是不同层次的验证。

## 验收清单

| 检查 | 通过条件 |
|---|---|
| 启动和登录 | 中文界面正常，登录后及时出现主界面 |
| 文档 | 能看到列表，点击正文后可以加载并操作 |
| 中文输入 | 编辑、删除中文和标点正常，无需发送测试消息 |
| 悬浮提示 | “截图”“图片”“文档”“文件”“查找聊天记录”无方框 |
| 标题栏 | 单击能置前，切换其他应用时整体退后，拖动和最小化/恢复正常，无重复任务栏项 |
| 边缘缩放 | 左右和底部向外拖动可放大，向内缩小遵守最小尺寸限制 |
| 托盘菜单 | 在各屏幕分别关闭、重开，菜单靠近图标；点击其他应用、桌面或主窗口空白处后关闭，菜单内部悬停和点击正常；应用内部菜单正常 |
| 托盘悬浮提示 | 在各屏幕悬停图标，提示靠近图标且留在屏幕内；移开鼠标后正常消失 |
| 托盘消息提醒 | 来消息后正常图标与空白占位交替，无黑块；左键点击能激活主窗口 |
| 其他功能 | 按实际用途检查工作台和邮件 |

材料/提示字体诊断可使用下面的只读检查；`status-font-probe.exe` 不带参数只检查，加 `-set` 才保存设置：

```bash
export WECOM_SETUP="/path/to/archlinux-wine-wecom-setup"
export WECOM_PREFIX="$HOME/Works/Softwares/wine/WeCom"
export WECOM_VERSION_DIR="$WECOM_PREFIX/drive_c/Program Files (x86)/WXWork/5.0.11.6018"
test -f "$WECOM_PREFIX/system.reg"
env -u LC_ALL WINEPREFIX="$WECOM_PREFIX" WINEDEBUG=-all \
  LANG=zh_CN.UTF-8 LC_CTYPE=zh_CN.UTF-8 LC_MESSAGES=zh_CN.UTF-8 \
  WINEDLLOVERRIDES=winemenubuilder.exe=d \
  /usr/bin/wine "$WECOM_SETUP/scripts/status-font-probe.exe"
python3 "$WECOM_SETUP/scripts/verify-materials.py" \
  --installer "$WECOM_SETUP/assets/WeCom_5.0.11.6018.exe" \
  --cef-dir "$WECOM_VERSION_DIR/compatible_web" \
  --richedit "$WECOM_PREFIX/drive_c/windows/syswow64/riched20.dll" \
  --fonts-dir "$WECOM_PREFIX/drive_c/windows/Fonts"
```

## 按现象定位

| 现象 | 优先检查 |
|---|---|
| 登录后恰好等待约五分钟 | [Boost 初始化锁](configuration-rationale.md#启动等待)及 `wecom-startup.log`；不要自动删除 `CUR_LOGIN_UIN_KEY` |
| 正文白屏、renderer 退出 | [实际加载的 CEF 版本和架构](configuration-rationale.md#文档页面)、新增 Crashpad 转储 |
| 工作台可用，但文档列表空白 | WeMail 的 `wemail/qt.conf`、Qt 感知模式和父窗口；renderer 存活不等于嵌入正常 |
| MSVCP140 缺失导入 | 配套 DLL 是否存在，启动器的覆盖列表和实际加载路径 |
| 水印或正文方框 | [字体文件、GDI 与 DirectWrite 映射](configuration-rationale.md#字体与输入)；仅看 Linux `fc-match` 不够 |
| 只有编辑框方框 | x86 RichEdit 校验及 native 加载情况 |
| 只有悬浮提示方框 | StatusFont 是否生效，设置后是否完整重启 |
| 标题栏盖在最前或点击不激活 | [窗口辅助程序](configuration-rationale.md#标题栏与边缘缩放)是否运行、标题栏样式是否已调整 |
| 边缘不能拖动 | 阴影鼠标穿透是否生效；先向外放大，当前布局最小尺寸为 `986×650` |
| 托盘菜单或悬浮提示偏移 | [逐屏坐标映射](configuration-rationale.md#多屏托盘菜单与悬浮提示)与 `wecom-titlebar.log`；分别查看 `tray menu` 或 `tray tooltip coordinate correction`，布局改变后重新启动 |
| 托盘菜单点击外部不消失 | [菜单关闭处理](configuration-rationale.md#托盘菜单关闭)及日志中的 `tray menu dismiss`；核对触发原因和 `posted` 结果 |
| 托盘消息提醒闪黑块 | [空白占位辅助程序](configuration-rationale.md#托盘消息提醒)及 `wecom-tray-icon.log`；确认图标尺寸和像素仍匹配已验证情况 |
| 托盘左键只恢复、不置前 | [激活权限处理](configuration-rationale.md#托盘点击激活)及 `wecom-titlebar.log` 中的 `tray left-click activation` 结果 |

日志路径均相对于目标 prefix；`<用户名>` 按实际环境替换：

| 路径 | 用途 |
|---|---|
| `wecom-startup.log` | 失效锁检查、跳过或备份结果 |
| `wecom-titlebar.log` | 窗口样式、托盘监听及坐标修正 |
| `wecom-tray-icon.log` | 托盘图标匹配与黑块替换 |
| `drive_c/users/<用户名>/AppData/Roaming/Tencent/WXWork/Log/` | 应用日志，部分加密；`CEF/` 下为网页组件日志 |
| `drive_c/users/<用户名>/AppData/Local/wxworkweb/User Data/Crashpad/reports/` | CEF 崩溃转储 |

比较操作前后的新增日志/转储，避免把历史故障当成本次结果。普通启动保持 `WINEDEBUG=-all`，原始诊断文件留在本机。

## 重启与升级

保存未完成工作后，只停止目标 prefix，不使用 `killall wine` 或 `pkill wineserver`：

```bash
export WECOM_PREFIX="$HOME/Works/Softwares/wine/WeCom"
test -f "$WECOM_PREFIX/system.reg"
env WINEPREFIX="$WECOM_PREFIX" /usr/bin/wineserver -k
env WINEPREFIX="$WECOM_PREFIX" /usr/bin/wineserver -w
bash "$WECOM_PREFIX/wecom-compat/scripts/launch-wecom.sh"
```

升级前停止目标 prefix 并备份；组件备份不等于聊天数据和账号状态的完整备份。升级后检查新版本目录的 `compatible_web`、`wemail/qt.conf`、VC/UCRT DLL，以及可能被 Wine 重建的 RichEdit、字体注册和 StatusFont，再执行完整验收清单。

未来版本是否仍需要各项修复须重新验证；不要强制沿用旧 CEF，或修改校验清单来迎合来源不明的文件。启动和停止目标 prefix 均使用系统 `/usr/bin/wine`、`/usr/bin/wineserver`。

## 单项回退

先停止目标 prefix，再按安装时的备份恢复：

| 配置 | 方法 |
|---|---|
| CEF | 改名保存当前 `compatible_web`，将 `compatible_web.x86-original` 恢复原名；原 x86 白屏可能复现 |
| Qt DPI | 保存并移走新增 `wemail/qt.conf` |
| VC/UCRT | 备份后只移除对应 DLL 覆盖项，保留其他配置 |
| RichEdit | 将 `setup-backups/riched20.dll` 放回 `windows/syswow64/`，移除 `riched20=n,b` |
| StatusFont | 从 `setup-backups/window-metrics.reg` 恢复原 `StatusFont` 二进制值，避免覆盖其他窗口参数 |
| 字体映射 | 根据安装前导出值逐项还原；`.reg` 导入只合并，不会自动删除新增键值 |
| 窗口/托盘修复 | 使用[启动开关](#日常启动与开关)停用，或还原更新前的整套辅助文件；图标占位可单独用 `WECOM_TRAY_ICON_FIX=0` 停用 |

本机已有备份的命名可能与新装步骤不同，恢复前需核对备份内容与目标路径。

### 本机验证环境备份

以下目录位于本机 prefix 的 `setup-backups/`，不随仓库分发。先停止目标 prefix，再将对应文件还原到 `wecom-compat/scripts/`，通过桌面入口启动。恢复某次备份会同时撤销该辅助程序之后的改动。

| 回退到的状态 | 备份目录 | 恢复文件 |
|---|---|---|
| 托盘闪烁与点击激活修复前 | `desktop-interaction-20261008-142927/` | `launch-wecom.sh`、`titlebar-managed.c`、`titlebar-managed.exe` |
| 托盘悬浮提示坐标修复前 | `tray-tooltip-20261008-152219/` | `titlebar-managed.c`、`titlebar-managed.exe` |
| 菜单关闭修复前，保留悬浮提示修正 | `tray-menu-dismiss-20261008-155206/` | `titlebar-managed.c`、`titlebar-managed.exe` |

首份备份的启动器不会启动 `tray-icon.py`。同类目录中较晚的备份可能是调试中间版本，按表中时间和恢复目标选择。

## 辅助文件维护

| 文件 | 职责 |
|---|---|
| `launch-wecom.sh` | 固定启动环境并按顺序运行辅助检查 |
| `prepare-startup.py` | 仅在空闲 prefix 中备份已识别的失效共享锁 |
| `tray-geometry.py` | 启动时计算 KDE Wayland/XWayland 逐屏映射 |
| `tray-icon.py` | 监听当前 prefix 托盘的重绘，将已识别的黑块帧改为空白占位 |
| `titlebar-managed.c` / `.exe` | 常驻维护标题栏、阴影输入、托盘菜单/提示坐标、菜单关闭和左键激活 |
| `status-font-probe.c` / `.exe` | 安装或诊断时读取/设置提示字体，不常驻 |
| `verify-materials.py` | 只读校验安装包、组件和字体材料 |

启动器、启动检查、两个托盘脚本和窗口辅助程序安装到 prefix 的 `wecom-compat/scripts/`，DLL 覆盖列表在 `wecom-compat/configs/`。其余配置/清单属于已验证材料快照；注册表文件使用 UTF-16，交给 Wine `reg.exe import`。

两个 EXE 均为附源码的小工具。重建需要 clang、lld 和 Wine 开发导入库，在仓库根目录执行：

```bash
clang --target=i686-pc-windows-msvc -ffreestanding -fno-stack-protector \
  -c scripts/status-font-probe.c -o /tmp/wecom-status-font-probe.obj
lld-link /entry:mainCRTStartup /subsystem:console /nodefaultlib /safeseh:no \
  /out:scripts/status-font-probe.exe /tmp/wecom-status-font-probe.obj \
  /usr/lib/wine/i386-windows/libkernel32.a /usr/lib/wine/i386-windows/libuser32.a \
  /usr/lib/wine/i386-windows/libgdi32.a

clang --target=i686-pc-windows-msvc -ffreestanding -fno-stack-protector \
  -Wall -Wextra -Wno-unused-parameter \
  -c scripts/titlebar-managed.c -o /tmp/wecom-titlebar-managed.obj
lld-link /entry:mainCRTStartup /subsystem:console /nodefaultlib /safeseh:no \
  /out:scripts/titlebar-managed.exe /tmp/wecom-titlebar-managed.obj \
  /usr/lib/wine/i386-windows/libkernel32.a /usr/lib/wine/i386-windows/libuser32.a
```

重建后按[更新步骤](#更新启动辅助文件)同步到已停止的环境，再验收实际行为。
