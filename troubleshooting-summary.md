# 企业微信 Wine 文档白屏问题处理总结

记录日期：2026-09-30。稳定环境验证截止时间：15:26（Asia/Shanghai）；随后新增的原生 Wine 测试环境见后半部分。从零安装请先阅读 [安装步骤](install-from-scratch.md)。

这是原排查记录的公开整理版。路径中的 `<user>` 是占位符，日志、转储、缓存、原 DLL 及历史诊断/回退脚本未随仓库上传。下文涉及旧环境的操作记录不能直接作为本仓库安装命令执行。

## 处理结果

企业微信已切换到其自行下载的原始 **64 位 Web 组件 5.0.79.5483（Chromium 129.0.6668.101）**。用户确认：“文档正常，其他已试页面也正常”。

组件已放入正式安装目录，继续使用原桌面图标启动即可。64 位组件未修改二进制文件，主程序、桌面快捷方式和 Wine 版本均未修改。已验证可用的 32 位修复版保留作回退。

## 环境

| 项目 | 本次环境 |
|---|---|
| 操作系统 | Arch Linux，Wayland 会话；Wine 窗口经 XWayland，Intel 核显 |
| Wine | deepin-wine10-stable，包版本 10.14deepin11-1 |
| 企业微信主程序 | 5.0.9.6039，32 位 WXWork.exe |
| 原 Web 内核 | 32 位 CEF，Chromium 107 |
| 当前 Web 内核 | 64 位组件 5.0.79.5483，Chromium 129.0.6668.101 |
| 启动入口 | /home/<user>/Desktop/WeCom.desktop |
| Wine prefix | /home/<user>/Works/Softwares/wine/WeChatCom |

下文 `<prefix>` 指表中的 Wine prefix。

## 问题与已确认原因

### 早期依赖加载失败

根据前一轮调查记录，prefix 中 `system32`、`syswow64` 下的 `sys_msvcrt.dll`、`capi2032.dll`、`inkobj.dll`、`slwga.dll` 共 8 个文件为旧格式 IntxLNK 占位符，Wine 10 无法加载，最终导致 Web 进程启动失败。前一轮已使用当前 Deepin Wine 对应架构的真实 PE 文件修复此问题；修复后仍存在文档白屏。

### 32 位 V8 初始化检查失败

后续白屏由旧版 32 位 CEF 的渲染进程崩溃导致。V8 调用 `VirtualProtect(..., PAGE_READONLY, &old_protection)` 成功后，仅接受旧属性 `PAGE_READWRITE (0x4)`；当前 Wine 返回 `PAGE_WRITECOPY (0x8)`，触发检查失败并退出。

转储显示异常位于 `libcef.dll + 0x16a59ac`，检查位于 RVA `0x16a55e4`，被检查的 `[EBP-0xd4]` 实际值为 `0x8`。相关证据和补丁校验值保存在诊断目录的 `manifest.json`、`verification.json` 中。

本轮先恢复此前针对窗口创建的诊断补丁，再增加接受 `0x8` 的兼容分支，保留内存只读保护及调用成功检查。32 位修复后用户确认文档正常，渲染进程持续存活。此时日志仍有 `CreateWindowEx Error = 1400`，说明它不是本次白屏的直接原因。

### 磁盘补丁未生效的原因

Deepin Wine 使用持久化、重定位后的 PE 影子缓存。本次旧 CEF 的实际代码来自 `<prefix>/.cache/1CA31EDDEC577702786DE2CB3714191F`；磁盘 DLL 已改变时，缓存仍包含原始代码。单独重启 wineserver 未使该缓存失效。

将对应缓存移入持久备份后重新启动，已确认新缓存包含修复后的分支。这解释了此前磁盘补丁与运行结果不一致的现象。

## 最终采用的 64 位方案

企业微信已自行下载完整的 64 位组件，来源为：

```text
<prefix>/drive_c/users/<user>/AppData/Roaming/Tencent/WXWork/cef/5.0.79.5483/cef/
```

本次校验下载清单后，使用整套组件替换主程序实际调用的 Web 组件目录。原下载目录仍保留。安装目录布局为：

```text
<prefix>/drive_c/Program Files (x86)/WXWork/5.0.9.6039/
├── compatible_web/                         当前使用的原始 64 位组件
└── compatible_web.x86-fixed-before-cef64/   已验证可用的 32 位修复版备份
```

测试时曾使用符号链接；验证通过后，已将相同文件移入正式目录并重启。当前 `compatible_web` 是实际目录，日常启动不依赖诊断目录中的测试组件。该目录名称和 32 位主程序不影响其启动独立的 64 位 Web 进程。

## 验证结果

| 验证项 | 结果 |
|---|---|
| 用户操作 | 文档及其他已试页面正常 |
| 组件完整性 | 89/89 文件与企业微信下载清单 MD5 一致 |
| 二进制架构 | WXWorkWeb.exe、libcef.dll 的 PE Machine 均为 0x8664 |
| 实际加载路径 | 指向正式安装目录的 compatible_web/libcef.dll |
| 实际内核版本 | 子进程参数报告 Chrome/129.0.6668.101 |
| 渲染进程 | 正式目录重启后检查到 4 个 renderer |
| 崩溃记录 | 15:12 切换至 15:26 检查期间，已知 Crashpad reports 目录无新增转储 |
| 致命错误检查 | 重启后的两份 CEF 日志未发现 FATAL、Check failed 或 CreateWindowEx 1400 |

用户页面验证发生在测试阶段；正式落盘后复核了文件一致性和运行状态。上述结果覆盖已试页面和检查时段，不代表所有功能或长期运行均已验证。新原生 Wine 环境的公开验证记录见 [验证摘要](evidence/verification-summary.json)。

## 适用范围

本次通过完整组件目录替换启用 64 位 CEF，未修改主程序的自动选择逻辑。原程序为何选择旧组件仍未确定。

带 Win10 兼容声明的 32 位探针实测：`GetNativeSystemInfo` 返回 AMD64（9），`IsWow64Process` 为真，`GetVersionExW` 返回 10.0.19045。下载组件声明的最低企业微信版本为 5.0.8.8000，当前主程序符合要求；现有证据不足以将自动选择问题归因于 Wine 误报系统架构。

此方案针对企业微信 5.0.9.6039 和组件 5.0.79.5483。应用升级可能覆盖目录安排，升级后应复核实际内核与页面行为。旧 32 位二进制补丁也只适用于记录中的文件版本。

## 回退方法

如需恢复已修复的 32 位组件，先保存企业微信中未完成的工作，再运行：

```bash
python3 "$HOME/Works/Work/Projects/wecom-wine-diagnostics/2026-09-30/switch-cef64-test.py" restore
```

脚本校验当前组件版本，关闭此 Wine prefix，留存当前 64 位目录，恢复备份的 32 位目录，并重启企业微信。它还会恢复测试前备份的 `Data/Global/CefCache` 与 `AppData/Local/CEF/User Data` 两个缓存目录；这些备份不是全部用户数据的快照。

当前 64 位状态下不适用 `apply-fix.py`。该脚本仅处理指定版本的 32 位 DLL，版本校验不符时会拒绝覆盖；需先执行上述回退，才适用旧补丁操作。

## 留存材料

诊断和回退材料位于：

```text
/home/<user>/Works/Work/Projects/wecom-wine-diagnostics/2026-09-30/
```

| 文件 | 用途 |
|---|---|
| verification-cef64.json | 最终 64 位方案的验证记录 |
| switch-cef64-test.py、cef64-test-state.json | 组件切换、回退脚本及状态 |
| cef64-profile-before-0、cef64-profile-before-1 | 测试前备份的两个缓存目录 |
| manifest.json、verification.json | 32 位故障证据、补丁校验值和验证记录 |
| libcef.fixed.dll | 已验证的 32 位修复版 DLL |
| libcef.restored-original.dll | 恢复早期窗口诊断补丁后的 32 位 DLL |
| libcef.before-this-session.dll | 本轮开始时的 32 位 DLL 基线 |
| shadow-cache.1790751503164993992.bak | 替换前的 Wine 影子缓存 |
| prepare-fix.py、apply-fix.py | 指定版本的 32 位补丁准备和应用脚本 |
| probe-platform.c、probe-platform.exe、probe-platform.manifest | Wine 系统架构与版本探针 |
| wine-cef64-*.log | 64 位切换与最终重启的运行日志 |

CEF 日志位于 `<prefix>/drive_c/users/<user>/AppData/Roaming/Tencent/WXWork/Log/CEF/`；本次检查的崩溃转储位于 `<prefix>/drive_c/users/<user>/AppData/Local/wxworkweb/User Data/Crashpad/reports/`。

## 新增独立原生 Wine 测试环境

2026-09-30 按用户要求，在独立 prefix 中安装指定的企业微信 5.0.11.6018，使用系统原生 Wine 11.18。已成功登录，文档列表恢复显示，用户确认文档内容也能正常显示。邮件功能本轮未单独确认。旧 Deepin Wine 环境继续作为稳定环境保留。

| 项目 | 稳定环境 | 新测试环境 |
|---|---|---|
| Prefix | `/home/<user>/Works/Softwares/wine/WeChatCom` | `/home/<user>/Works/Softwares/wine/WeCom` |
| Wine | deepin-wine10-stable 10.14deepin11-1 | 系统 `/usr/bin/wine`，11.18 |
| 企业微信 | 5.0.9.6039 | 5.0.11.6018 |
| 桌面入口 | `WeCom.desktop` | `WeCom-Native-Wine.desktop`，显示名“WeCom（原生 Wine 测试）” |
| 当前 Web 组件 | 原始 64 位 CEF 5.0.79.5483 | 原始 64 位 CEF 5.0.79.5483，Chromium 129.0.6668.101 |

安装来源：`/home/<user>/Works/Download/WeCom_5.0.11.6018.exe`。这是用户指定的版本，本次未比较线上其他版本。新环境以 `WINEARCH=win64` 初始化，安装程序退出码为 0。

### 文档列表空白的原因与修复

文档列表所在的 Qt 窗口与企业微信主窗口采用不同的 DPI 感知模式。主程序为 System DPI Aware（值 1），`wemail/WeMail.exe` 的 Qt 窗口为 Per-Monitor DPI Aware（值 2）。窗口跟踪显示主程序尝试将 Qt 窗口挂入文档容器，但 Qt 窗口实际仍留在隐藏的原父窗口下，主程序中的 `WXWorkQtHostWindow_NOT_BACKGROUND` 容器尺寸为 0×0。

Wine 的 [set_parent 实现](https://github.com/wine-mirror/wine/blob/master/server/window.c)会拒绝 DPI 感知模式不兼容的窗口挂载；[Qt 5.15 文档](https://doc.qt.io/archives/qt-5.15/highdpi.html)提供 `dpiawareness` 平台参数。两边实际 DPI 数值均为 96，冲突的是感知模式，并非缩放数值。

仅在新环境添加：

```text
/home/<user>/Works/Softwares/wine/WeCom/drive_c/Program Files (x86)/WXWork/5.0.11.6018/wemail/qt.conf
```

内容为：

```ini
[Platforms]
WindowsArguments = dpiawareness=1
```

重启后，两边均为模式 1；Qt 子窗口成功位于主窗口的文档容器下，容器恢复为 925×648，CEF 子窗口可见。截图已确认文档列表出现，随后用户打开文档并确认内容正常显示。本次仅增加该 Qt 配置，没有修改 Qt DLL、系统 Wine 或桌面整体缩放。配置位于版本目录，后续升级若改用新版本目录，需要重新检查。

### 同时保留的兼容处理

1. **64 位 CEF。** 安装包自带 32 位 Chromium 107 的 `libcef.dll` 与旧环境原始文件相同，3 份转储均在 `libcef + 0x16a59ac` 触发同一 V8 `PAGE_WRITECOPY` 检查异常。换入完整的原始 64 位组件后，渲染进程恢复存活。组件从旧环境保留的企业微信下载缓存复制，89/89 文件与腾讯组件清单 MD5 一致，无二进制补丁；新环境运行不依赖旧 prefix。新环境组件清单允许该组件用于 5.0.11.6018。
2. **微软 VC/UCRT 运行库优先。** 加载跟踪发现 Wine 内置 `MSVCP140.dll` 有 8 个未实现导入，而企业微信自带 DLL 包含相应导出。测试桌面入口通过 `WINEDLLOVERRIDES` 优先加载配套微软运行库，保留 Wine 内置回退；跟踪确认缺失导入消失。该项单独应用并未恢复文档列表，不能将其视为列表空白的根因。

曾对照测试安装包配套的 32 位 Web 组件加 V8 修复，渲染进程同样恢复，但列表仍为空；原始 64 位 CEF 加微软运行库也仍为空。最后在后者基础上统一 Qt DPI 模式，列表才恢复。

旧环境曾有的 `sys_msvcrt.dll` 等 IntxLNK 占位符故障未在新 prefix 复现，因此没有向原生 Wine 复制 Deepin Wine 的系统 DLL。

新环境组件备份：

```text
5.0.11.6018/
├── compatible_web/                当前原始 64 位组件
├── compatible_web.x86-original/   安装包原始 32 位组件
├── compatible_web.x86-fixed/      配套 32 位组件，加 V8 修复
└── wemail/qt.conf                 Qt DPI 兼容配置
```

### 隔离与验证

新环境未复制旧账号数据。安装和启动禁用 `winemenubuilder.exe` 自动创建菜单入口，Windows Desktop 使用新 prefix 内部目录，Linux 桌面另建明确标记的测试入口。新 prefix 的 Windows Documents 保留 Wine 默认映射，指向 `/home/<user>/Documents`；稳定环境的 Documents 为旧 prefix 内部目录。

最后检查：64 位组件 89/89 文件校验通过，5 个 CEF renderer 存活，Qt 修复后检查时段内 CEF Crashpad reports 无新增转储；临时详细窗口跟踪已关闭。旧桌面入口、主程序和工作中的 CEF 三项 SHA256 均与创建新环境前一致。此验证不代表所有功能或长期运行均已覆盖。

诊断、安装包校验值、加载配置和验证记录位于：

```text
/home/<user>/Works/Work/Projects/wecom-wine-diagnostics/2026-09-30/native-wine-5.0.11.6018/
```

关键记录为 `verification.json`、`final-runtime-evidence.json`、`dpi-before.json`、`windows-after-dpi.txt`、`qt-dpi-fix.json`、`dll-overrides.txt`。`setup.py restart` 只重启新 prefix；`setup.py verify-current` 核对当前组件、进程及旧环境关键文件。

日常使用各自桌面图标。需要稳定环境时直接打开原 `WeCom.desktop`，无需回退旧 prefix。若只撤销 Qt 调整，关闭新环境，将新版本 `wemail/qt.conf` 改名保存后再启动即可；原来不存在此配置文件。

### 新环境字体补齐

新 prefix 的 `C:/windows/Fonts` 起初为空。Wine 能使用部分宿主字体，但 GDI 探针发现 `Microsoft YaHei`、`SimSun`、`SimHei` 等常见中文字体请求实际退到 Arial，所测 12 个中文及标点字符均不在该字体中。

已仅向新环境复制并注册 31 个本机字体文件：思源黑体 CN、思源宋体 CN 的常规和粗体共 4 个文件，来自系统字体目录；Arial、Tahoma、Calibri、Consolas、Times New Roman、Courier New、Verdana、Microsoft Sans Serif 等共 27 个文件，复制自旧环境。没有下载字体，旧环境源文件校验值未变。

在新 prefix 中配置 `FontSubstitutes`：微软雅黑及黑体相关名称替代为思源黑体 CN，宋体及新宋体相关名称替代为思源宋体 CN。这是替代映射，并非安装了微软原版微软雅黑或宋体。另为 9 个常见西文字体设置 `FontLink/SystemLink` 中文回退，保留原有回退项；未调整桌面缩放或字体平滑参数。

安装后 32 位和 64 位 GDI 探针均确认中文名称映射可用，所测 12 个字符无缺字；31 个目标文件与源文件 SHA256 一致。此检查覆盖字体解析及样本文字，不代表所有字体排版均已逐页验证。首次安装时未重启企业微信；随后针对输入框和悬浮提示方框问题补充了下述配置，并已重启新环境。

字体记录为 `fonts-installed.json`，注册表修改前备份位于 `fonts-before/`，注册内容为 `fonts-install.reg`，安装前及安装后的探针结果分别为 `font-probe-before.txt` 和 `font-probe-x86-result.txt`、`font-probe-x64-result.txt`。

### 输入框及悬浮提示的字体族映射补充

用户随后反馈：输入框编辑文字、功能按钮悬浮提示仍出现方框。核对发现前述 `FontSubstitutes` 能通过 GDI 检查，但新环境缺少旧稳定环境已有的 `HKCU/Software/Wine/Fonts/Replacements` 配置。[Wine DirectWrite 实现](https://github.com/wine-mirror/wine/blob/master/dlls/dwrite/font.c)从此键读取系统字体族映射。

安装前探针确认，在 32 位和 64 位 DirectWrite 系统字体集合中，微软雅黑、Microsoft YaHei UI、SimSun、SimHei 等 7 个被测中文字体名称均不存在，而思源黑体 CN 和思源宋体 CN 存在。这解释了为什么只通过 GDI 字形检查还不足以确认字体配置完整。

仅在新 prefix 增加 `Fonts/Replacements`：沿用已有的 10 项中文替代映射，并为 `Arial Unicode MS`、`DengXian`、`等线` 提供思源黑体 CN 回退。配置后，32/64 位 DirectWrite 都能找到上述字体族，常规与粗体的 12 个中文及标点样本均无缺字。已重启新环境刷新缓存，截图确认此前为方框的中文水印恢复显示；用户复测输入框仍不正常，表明字体族映射尚不足以解决编辑控件的问题；当次英文启动无法验证中文悬浮提示。

配置与证据：`fonts-replacements.json`、`fonts-replacements.reg`、`dwrite-font-x86-before.txt`、`dwrite-font-x64-before.txt`、`dwrite-font-x86-result.txt`、`dwrite-font-x64-result.txt`。这次没有替换 DLL、修改输入法或调整已验证的 Qt DPI 配置。

### RichEdit 输入控件对照修复

旧稳定环境除字体配置外，还使用微软原生 32 位 `riched20.dll`（RichEdit 3.0，文件版本 5.30.23.1200），新环境原本使用 Wine 内置版。仅在新 prefix 中将 `C:/windows/syswow64/riched20.dll` 替换为这份本机已有的微软组件，并为测试桌面启动器增加 `riched20=n,b`。原 DLL 与启动器均已备份；64 位 RichEdit、系统 `/usr/bin/wine` 和稳定环境均未修改。

加载跟踪已确认主程序的 `riched20.dll` 以 native 方式加载，中文主界面与搜索框占位文字正常显示。用户确认聊天输入框已正常，悬浮提示仍有方框，继续单独处理。

诊断脚本也已按实际 KDE/Fcitx 会话设置 `LANG=zh_CN.UTF-8`，去除诊断工具注入的 `LC_ALL=C.UTF-8`，使其启动语言与桌面图标一致。

记录与备份为 `richedit-native.json`、`riched20.before-native.dll`、`desktop-before-richedit.desktop`；该变更的原始覆盖字符串保存在记录中的 `overrides_before` 字段。

### 中文悬浮提示修复：系统 StatusFont

**最终结果：用户确认截图按钮的悬浮提示已正常。** 实际抓图也确认“截图 Alt+Shift+A”“图片”“文档”“文件”“查找聊天记录”均显示完整。聊天输入框继续使用前述微软 RichEdit，文档相关的 CEF、运行库和 Qt DPI 配置保持原样。

悬浮提示与主界面的字体选择并不相同。[Wine 标准 tooltip 实现](https://github.com/wine-mirror/wine/blob/master/dlls/comctl32/tooltips.c)从 `SPI_GETNONCLIENTMETRICS` 的 `lfStatusFont` 创建提示字体。新环境实际返回 Tahoma；该基础字体没有“截图表情图片”六个样本字形，需要回退。此前字体族映射虽修复了水印等位置，仍不足以修复全部提示文字：直接复现时，“截图”中的“图”以及“查找聊天记录”的部分文字显示方框。

仅在新 Wine prefix 中将系统提示字体改为已安装的 `Source Han Sans CN`（思源黑体 CN），通过 `SPI_SETNONCLIENTMETRICS` 保存，沿用原来的字号与字重。对应持久化位置为：

```text
HKEY_CURRENT_USER\Control Panel\Desktop\WindowMetrics\StatusFont
```

字体探针确认所选思源黑体包含全部样本字形。运行中的控件刷新仍不完整；重启新企业微信后，提示窗口重新创建，抓图及用户复测均通过。提示框高度从 19 像素变为 22 像素，符合所用字体的行高；没有修改系统桌面字体或稳定环境。

此前还尝试重新添加 Tahoma、Microsoft Sans Serif 的 `FontLink/SystemLink` 思源中文回退。切换区域设置及启动过程中，这两项又被 Wine 覆盖为 `SIMSUN.TTC` 等默认文件名；用户复测未解决悬浮提示。因此这项尝试不能视为最终修复。诊断启动和探针统一使用桌面的中文区域设置，当前应用已关闭临时详细跟踪。

证据与回退：`status-font-fix.json`、`status-font-probe.txt`、`status-font-fix.txt`，以及 `tooltip-screenshot.png`、`tooltip-image.png`、`tooltip-document.png`、`tooltip-file.png`、`tooltip-history.png`。修改前系统字体设置备份为 `window-metrics-before-status-font.reg`；如需撤销，只还原其中的 `StatusFont` 值并重启新企业微信即可。早期 FontLink 尝试的备份和记录仍保留在 `font-links-before-chinese-refresh.reg`、`font-links-chinese-refresh.json`。

最终运行核对：原始 64 位 CEF 的 89 个文件校验通过，Qt 修复后的 CEF Crashpad reports 无新增转储，普通日志模式运行；旧桌面入口、主程序和稳定 CEF 三项 SHA256 与创建新环境前一致。

### 独立顶部标题栏覆盖其他窗口

用户反馈未最小化时，切换其他应用后企业微信顶部横条仍露在最前。该横条是独立 `TitleBarWindow`，原样式 `0x96000000`、扩展样式 `0x08080080`，owner 为主窗口；无 TOPMOST 标记。其 popup 样式没有触发 Wine 的受管窗口条件。

仅增加 `WS_SYSMENU` 并在不移动、不缩放、不激活的条件下刷新样式，标题栏随后出现 `WM_STATE=Normal` 并进入窗口管理器的层级列表；owner 与跳过任务栏/切换器的属性仍保留。用户确认切换遮挡、拖动、最小化及恢复正常。

已将单实例维护工具和启动脚本安装到新 prefix 的 `wecom-compat/`，并备份、更新新环境桌面入口。当前维护实例已运行；主进程退出时工具结束。没有修改旧稳定环境，也没有为本次修复再次重启企业微信，因此重启后的自动应用尚未单独复测。详情见 [标题栏修复文档](titlebar-layer-fix.md)，诊断记录为 `titlebar-fix.json`。
