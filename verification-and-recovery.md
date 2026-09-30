# 验证、升级与回退

以下操作沿用 [安装步骤](install-from-scratch.md) 中的 Bash 变量。当前运行环境已确认正常，本页供重装或升级后复核，不需要为阅读文档再次修改它。

## 验收清单

| 检查 | 通过条件 |
|---|---|
| 主界面与登录 | 中文正常，能够自行登录 |
| 文档列表 | 能看到实际列表，不只是 renderer 进程存在 |
| 文档正文 | 点击一篇文档，内容能加载并操作 |
| 聊天输入框 | 输入中文、标点，编辑和删除均正常；无需发送测试消息 |
| 悬浮提示 | “截图”“图片”“文档”“文件”“查找聊天记录”没有方框 |
| 标题栏 | 切换其他窗口覆盖时顶部横条一起退到后台；拖动、最小化和恢复正常，无重复任务栏项 |
| 其他页面 | 按实际用途检查工作台、邮件；不能用文档成功代替邮件验证 |
| 旧环境 | 旧桌面入口与原 prefix 保持独立、可继续使用 |

本次用户确认了原生 Wine 环境的文档及字体问题修复；邮件未单独确认，也未进行长期稳定性测试。

只读检查当前配置与提示字体：

```bash
cat "$WECOM_VERSION_DIR/wemail/qt.conf"
cat "$WECOM_SETUP/configs/dll-overrides.txt"
/usr/bin/wine "$WECOM_SETUP/scripts/status-font-probe.exe"
python3 "$WECOM_SETUP/scripts/verify-materials.py" \
  --installer "$WECOM_INSTALLER" \
  --cef-dir "$WECOM_VERSION_DIR/compatible_web" \
  --richedit "$WECOM_PREFIX/drive_c/windows/syswow64/riched20.dll" \
  --fonts-dir "$WECOM_PREFIX/drive_c/windows/Fonts"
```

`status-font-probe.exe` 不带参数只读取/检查；加 `-set` 才保存字体。校验工具不会修改文件，也不会检查登录或真实页面行为。

## 按现象定位

| 现象 | 优先看什么 | 不应直接得出的结论 |
|---|---|---|
| 文档正文白屏、renderer 不断退出 | 实际加载的 CEF 目录、PE 架构、Crashpad 新转储 | 主界面能打开不代表 CEF 正常 |
| 工作台能打开，文档列表仍为空 | WeMail 的 Qt DPI 感知模式、文档容器和父窗口关系 | renderer 存活不代表窗口嵌入正常 |
| 日志出现 MSVCP140 缺失导入 | 配套 DLL 是否存在、启动入口的覆盖字符串是否完整 | 设置 `n,b` 不代表加载的一定是预期文件 |
| 水印或部分正文方框 | GDI 替代、DirectWrite 的 Replacements、字体文件内部名称 | 只看 Linux `fc-match` 不足以证明应用字体正确 |
| 只有输入框编辑异常 | x86 RichEdit 文件校验与 native 加载情况 | 不应直接归咎于 Fcitx |
| 只有按钮提示的部分字方框 | `StatusFont`，以及修改后是否完整重启 | 主界面换了字体不代表 tooltip 已换字体 |
| 顶部横条残留在最前 | 独立 `TitleBarWindow` 是否纳入受管窗口、启动脚本中的标题栏工具是否运行 | 没有 TOPMOST 标记也可能因绕过桌面管理而浮在最前 |

日志位置，用户名按实际 prefix 调整：

```text
drive_c/users/<用户名>/AppData/Roaming/Tencent/WXWork/Log/CEF/
drive_c/users/<用户名>/AppData/Local/wxworkweb/User Data/Crashpad/reports/
```

按本次操作前后时间比较新增转储，不要把历史转储当成新故障。正常使用保留 `WINEDEBUG=-all`；若临时开启 DLL/窗口跟踪，验证结束后恢复普通启动。

## 重启只作用于一个 prefix

先保存该企业微信中未完成的工作，再明确指定 prefix：

```bash
env WINEPREFIX="$WECOM_PREFIX" /usr/bin/wineserver -k
env WINEPREFIX="$WECOM_PREFIX" /usr/bin/wineserver -w
bash "$WECOM_PREFIX/wecom-compat/scripts/launch-wecom.sh"
```

不要用 `killall wine`、`pkill wineserver` 影响旧环境。原生 Wine prefix 配合 `/usr/bin/wineserver`，不要与 Deepin Wine 工具混用。

## 升级后检查什么

升级企业微信或 Wine 都可能改变兼容性。升级前停止目标 prefix 并保留其文件/配置备份；组件备份不等于聊天数据和账号状态的完整备份。

企业微信升级后优先确认新版本目录：

1. 新 `compatible_web` 是哪个版本/架构，旧的替换是否被安装器覆盖。
2. 新 `wemail/qt.conf` 是否存在且仍适用于该 Qt/主窗口组合。
3. 配套 VC/UCRT DLL 的文件位置及导出是否变化，覆盖字符串是否仍合适。
4. `riched20.dll` 是否被 Wine 更新重建，是否仍按 native 加载。
5. 字体注册、Replacements 和 StatusFont 是否保留；中文区域设置是否一致。
6. 重新执行完整页面/输入/提示验收。
7. 确认标题栏工具仍能匹配窗口，并复测切换焦点、拖动、最小化/恢复；细节见 [标题栏修复](titlebar-layer-fix.md)。

不要仅因当前配置可用，就强制未来版本沿用旧 CEF。固定版本清单校验失败时先核对材料/版本，而不是修改清单迎合未知文件。

## 单项回退

| 配置 | 回退方法 | 影响 |
|---|---|---|
| x64 CEF | 关闭目标 prefix，把当前 `compatible_web` 改名保存，再把 `compatible_web.x86-original` 恢复原名 | 新装备份是未打补丁的 x86 组件，已知可能恢复白屏；不是可用性保证 |
| Qt DPI | 保存并移走新增 `wemail/qt.conf`，重启 | 原有列表嵌入故障可能恢复 |
| VC/UCRT 覆盖 | 备份启动配置后移除对应覆盖项 | 内置库缺失导入可能恢复；保留其他已验证项 |
| RichEdit | 关闭 prefix，将 `setup-backups/riched20.dll` 复制回 x86 位置，移除 `riched20=n,b` | 输入框问题可能恢复 |
| StatusFont | 从 `setup-backups/window-metrics.reg` 提取并恢复原 `StatusFont` 二进制值，再重启 | 不需要覆盖其他窗口参数 |
| 标题栏维护 | 完整退出企业微信后，以 `WECOM_TITLEBAR_FIX=0` 调用启动脚本 | 保留其他配置；已创建的受管窗口要重启后才恢复原样式 |
| 字体配置 | 依据安装前导出值逐项还原，移除新增别名/回退及字体注册后再处理文件 | `.reg` 导入是合并操作，不会自动删除新加键值 |

历史环境中实际使用的备份保留在调查主机的诊断目录，未上传。其中备份命名与新重装步骤的 `setup-backups/` 不同，参见 [完整总结](troubleshooting-summary.md)。公开验证结果见 [验证摘要](evidence/verification-summary.json)。

需要稳定使用时，直接打开原 `WeCom.desktop` 即可。历史 `setup.py` 固定了本机路径并有一次性状态断言，它是诊断工具，不是可对任意 prefix 直接重跑的通用安装器。

## 辅助文件维护

`configs/` 和清单属于本次已验证版本快照。改动后同步维护版本说明和验证记录。注册表文件采用 UTF-16 编码，直接使用 Wine `reg.exe import`。

`scripts/status-font-probe.exe` 是本次自编译的 x86 小工具，不是微软或腾讯组件。源码相邻保存；本机重新编译命令如下（需要 clang、lld 及本机 Wine 开发导入库）：

```bash
clang --target=i686-pc-windows-msvc -ffreestanding -fno-stack-protector \
  -c "$WECOM_SETUP/scripts/status-font-probe.c" -o /tmp/wecom-status-font-probe.obj
lld-link /entry:mainCRTStartup /subsystem:console /nodefaultlib /safeseh:no \
  "/out:$WECOM_SETUP/scripts/status-font-probe.exe" /tmp/wecom-status-font-probe.obj \
  /usr/lib/wine/i386-windows/libkernel32.a \
  /usr/lib/wine/i386-windows/libuser32.a \
  /usr/lib/wine/i386-windows/libgdi32.a
```
