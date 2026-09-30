# 为什么需要这些配置

本页区分本次实际观察和实现解释。验收依据是原生 Wine 11.18 + 企业微信 5.0.11.6018，不将单个修复推广成所有版本的必要条件。

## 文档页面有两层故障

首先，安装包自带的 x86 CEF/Chromium 107 在 V8 初始化中崩溃。其次，即使 CEF renderer 已能存活，文档列表的 Qt 容器仍不能嵌入主窗口。必须分别处理内核崩溃和窗口嵌入。

| 对照组合 | 实际结果 |
|---|---|
| 原始 x86 CEF | V8 检查失败、renderer 崩溃 |
| 配套 x86 CEF 加兼容补丁 | renderer 存活，文档列表仍空白 |
| 原始 x64 CEF | renderer 存活，工作台能打开，文档列表仍空白 |
| 原始 x64 CEF + 微软 VC/UCRT | 文档列表仍空白 |
| 上述组合 + Qt `dpiawareness=1` | 列表恢复，用户确认文档正文正常 |

### 完整 x64 CEF：解决 renderer 崩溃

原 x86 `libcef.dll` 在 `VirtualProtect` 成功后只接受旧页面属性 `PAGE_READWRITE (0x4)`，而 Wine 返回 `PAGE_WRITECOPY (0x8)`，触发 V8 检查。本次转储在 `libcef + 0x16a59ac` 处一致。

最终采用企业微信自行下载的原始 x64 Web 组件 `5.0.79.5483`，Chromium `129.0.6668.101`。清单允许企业微信最低版本 `5.0.8.8000`，本次版本符合；89 个文件与已保存的腾讯清单一致，未打二进制补丁。

这不是从系统 Chrome 拿一个 DLL，也不是把任意 CEF 包塞进安装目录。`WXWorkWeb.exe`、CEF、资源和配套库必须成套匹配。x86 主程序可以启动独立的 x64 Web 子进程，所以主程序不必换成 64 位，但 prefix 必须支持 64 位。

企业微信为何自动选择旧组件尚未确定；不能声称已修复其自动选择逻辑。版本更新可能覆盖 `compatible_web`。

证据：[验证摘要](evidence/verification-summary.json)、[CEF 清单](materials/cef64-vendor-manifest.json)。

### 微软 VC/UCRT：补齐已观察到的缺失导入

加载日志发现 Wine 内置 `MSVCP140.dll` 有 8 个缺失导入；企业微信配套库包含相应导出。配置 `WINEDLLOVERRIDES` 为原生优先后，这些错误消失。

完整覆盖列表保存在 [dll-overrides.txt](configs/dll-overrides.txt)，包括 MSVCP140 系列、ConCRT、VCRuntime 和 UCRT。`n,b` 保留内置回退；这只是设置加载优先级，前提是相应微软 DLL 确实存在。该项单独测试没有恢复文档列表，保留它是基于依赖修复证据，并非把它当成列表空白的唯一原因。

### Qt DPI：恢复文档列表的子窗口嵌入

主窗口 DPI awareness 为 System（1），WeMail 的 Qt 窗口为 Per-Monitor（2）；两边 DPI 数值都是 96，但感知模式不同。窗口跟踪显示 Qt 窗口留在隐藏父窗口中，企业微信文档容器尺寸为 0×0。

[Wine 的窗口实现](https://github.com/wine-mirror/wine/blob/master/server/window.c)对不兼容的 DPI 感知模式执行挂载限制；[Qt 5.15 文档](https://doc.qt.io/archives/qt-5.15/highdpi.html)提供 `dpiawareness` 平台参数。仅设置 `wemail/qt.conf` 后，两边模式一致，Qt 子窗口进入主容器，列表恢复。

此修复改变的是 WeMail 组件的 DPI 感知模式，不是显示器缩放或 DPI 数值。将来换版本目录，要检查新目录的 `wemail/qt.conf`。

证据：[验证摘要](evidence/verification-summary.json)。

## 字体不是只有一层配置

| 层次 | 配置 | 实际作用与验证范围 |
|---|---|---|
| 字体文件 | 4 个思源 CN OTF + 27 个西文 TTF，复制到 prefix 并注册 | 字体本体可用；不是仅在 Linux 字体列表中出现 |
| GDI 名称替代 | `HKLM\Software\Microsoft\Windows NT\CurrentVersion\FontSubstitutes` | 微软雅黑、黑体等名称映射到思源黑体；宋体映射到思源宋体 |
| GDI 字形回退 | 同一分支下 `FontLink\SystemLink` | 西文字体缺中文时，链接到安装的中文字体文件 |
| Wine 字体族映射 | `HKCU\Software\Wine\Fonts\Replacements` | 让 DirectWrite 也能找到应用请求的替代字体族 |
| 编辑控件 | x86 微软 RichEdit | 字体映射已通过后，编辑框仍异常，需要单独处理控件实现 |
| 系统提示字体 | `HKCU\Control Panel\Desktop\WindowMetrics\StatusFont` | 标准 tooltip 使用此字体；不完全跟随主界面的字体选择 |

首次 GDI 检查中，中文字体名称回退到 Arial，12 个样本全部缺字。增加 `FontSubstitutes` 后，GDI 样本通过；但 DirectWrite 字体集合仍找不到被请求的中文族名。

[Wine DirectWrite 的字体实现](https://github.com/wine-mirror/wine/blob/master/dlls/dwrite/font.c)读取 `Fonts/Replacements` 添加字体族替代。补齐该键后，32/64 位 DirectWrite 探针中的 7 个中文别名及常规/粗体样本均通过，中文水印恢复。

字体安装配置是 [fonts-install.reg](configs/fonts-install.reg)，字体族配置是 [fonts-replacements.reg](configs/fonts-replacements.reg)。后者额外覆盖 `Arial Unicode MS`、`DengXian`、`等线`。这些是思源替代映射，不等于安装了微软原版微软雅黑、宋体。

31 个文件是这次沿用的可用集合；没有证据说明每个西文字体都是启动所必需的。更换字体时，需要核对内部字体族名称，不能只把文件改名成 `simsun.ttc`。

## RichEdit：为什么不是继续添加字体

字体和字体族探针均通过后，输入框仍显示方框。旧环境使用微软 x86 `riched20.dll`，新环境使用 Wine 内置实现。复制同一份微软 DLL 并设置 `riched20=n,b` 后，加载跟踪确认使用 native 版本，用户确认输入框正常。

这给出了有效的兼容措施，但没有进一步定位 Wine RichEdit 内部的具体缺陷。不能将所有输入法问题都归因于此 DLL。本次没有更换 Fcitx 或系统输入法设置。

文件版本为 `5.30.23.1200`，SHA256：

```text
1c2508fb55ddc459d0327f2017471545c87420443391567094e768fb34032da1
```

只放进新 prefix 的 `windows/syswow64/riched20.dll`。不要用这份 x86 DLL 覆盖 `system32` 的 x64 文件。

证据：[验证摘要](evidence/verification-summary.json)。

## StatusFont：为什么其他中文正常，悬浮提示仍缺字

[Wine 标准 tooltip 实现](https://github.com/wine-mirror/wine/blob/master/dlls/comctl32/tooltips.c)通过 `SPI_GETNONCLIENTMETRICS` 的 `lfStatusFont` 创建提示字体。本次读取为 Tahoma；它本身没有中文样本字形，依赖回退。实际提示中，“表情”可以显示，“截图”的“图”及“查找聊天记录”的部分字却仍显示方框。

重写 Tahoma 和 Microsoft Sans Serif 的 FontLink 没有得到稳定结果，启动及区域设置变化后又出现指向缺失 Windows 字体文件的默认项。最后将 `StatusFont` 直接设为已经安装的思源黑体 CN，使提示不再依赖该西文字体的中文回退。重启企业微信后，“截图”“图片”“文档”“文件”“查找聊天记录”抓图均正常，用户确认修复。

字号、字重沿用原值，提示框因字体行高从 19 像素变成 22 像素。设置工具只改变取回的非客户区参数中的 StatusFont 字体选择，不改其他字体选择；保存操作由 Windows API 完成。不能只看设置 API 返回成功，必须重启后检查实际提示。

证据：[验证摘要](evidence/verification-summary.json)。

## 从零安装不必重做的历史操作

- **旧 Deepin Wine 的 IntxLNK 占位 DLL 替换**：只在旧 prefix 观察到，新的原生 Wine prefix 未复现。不要复制整套 Deepin 系统 DLL。
- **x86 CEF V8 二进制补丁**：曾有效，但最终已用原始 x64 组件；重装基线不使用该补丁。
- **Deepin PE 影子缓存处理**：属于旧环境补丁生效问题，不能套到此次原生 Wine 重装流程。
- **反复删除 Web/用户缓存**：没有证据表明这是新环境文档列表故障的必要修复。
- **更改桌面缩放、输入法或关闭全部 Wine 进程**：本次不需要。所有停止/重启均限定在目标 prefix。

## 独立标题栏的层级修复

用户另行反馈：其他窗口激活时，企业微信顶部横条仍显示在最前。检查确认它是单独的 `TitleBarWindow`，未设置 TOPMOST，但 Wine 未将其纳入正常窗口管理。仅增加使该 popup 受管的样式后，窗口管理器接管了标题栏，用户确认遮挡、拖动、最小化/恢复正常。

新环境的启动器现在配套一个只维护该主进程所属标题栏的辅助工具。它与 CEF、字体和 Qt DPI 修复无关，具体机制、验证范围和回退方法见 [标题栏层级修复](titlebar-layer-fix.md)。
