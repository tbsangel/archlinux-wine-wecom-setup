# Arch Linux 原生 Wine 企业微信安装与配置记录

这套文档记录本机已验证的 **原生 Wine 11.18 + 企业微信 5.0.11.6018** 配置，整理于 2026-09-30。文档列表、文档内容、聊天输入框和按钮中文悬浮提示均已得到用户确认。新环境的邮件功能未单独验收。

从零安装时先读 [安装步骤](install-from-scratch.md)，了解原因看 [配置说明](configuration-rationale.md)，遇到问题看 [验证、升级与回退](verification-and-recovery.md)。完整排查过程保留在 [问题处理总结](troubleshooting-summary.md)。

```bash
git clone https://github.com/tbsangel/archlinux-wine-wecom-setup.git
cd archlinux-wine-wecom-setup
```

仓库只包含配置、文档、校验清单和附源码的小型辅助工具。安装程序、腾讯/微软 DLL、字体及账号数据均不随仓库分发；准备好材料后再执行安装步骤。

## 最终需要配置什么

| 项目 | 本次采用的配置 | 为什么需要 |
|---|---|---|
| 独立环境 | `/usr/bin/wine`，`WINEARCH=win64`，独立 prefix | 主程序是 32 位，Web 子进程是 64 位；保留旧环境可随时使用 |
| 中文区域设置 | `LANG/LC_CTYPE/LC_MESSAGES=zh_CN.UTF-8`，清除覆盖它们的 `LC_ALL` | 避免诊断终端启动成英文、反复生成不同代码页的字体默认值 |
| Web 组件 | 整套原始 x64 CEF `5.0.79.5483` / Chromium `129.0.6668.101` | 绕过安装包内 x86 CEF 的 V8 初始化崩溃 |
| VC/UCRT | 企业微信配套微软 DLL 优先，`n,b` 回退 | Wine 内置 MSVCP140 有 8 个缺失导入；该项单独不足以修复文档列表 |
| Qt DPI | `wemail/qt.conf`：`WindowsArguments = dpiawareness=1` | 让 Qt 与主窗口采用相同感知模式，文档列表窗口才能正常嵌入 |
| 字体 | 31 个已记录字体；GDI 替代/回退；Wine 字体族 Replacements | 补齐字体名称与中文字形，覆盖 GDI 和 DirectWrite 两条路径 |
| 输入控件 | x86 Microsoft `riched20.dll`，`riched20=n,b` | 字体补齐后输入框仍异常；切到此组件后用户确认正常 |
| 悬浮提示 | 系统 `StatusFont=Source Han Sans CN`，重启应用 | 提示控件使用独立的系统字体；单纯改字体回退未解决所有方框 |
| 标题栏层级 | 启动时维护 `TitleBarWindow` 的受管窗口样式 | 防止切换到其他应用时，顶部横条仍盖在最前；见 [标题栏修复](titlebar-layer-fix.md) |

这些是本次版本组合的复现基线，不是对任意 Wine/企业微信版本都必需的通用结论。31 个字体文件是已验证集合，尚未做逐个移除的最小化试验。

## 当前环境

| 项目 | 原稳定环境 | 原生 Wine 环境 |
|---|---|---|
| Prefix | `/home/<user>/Works/Softwares/wine/WeChatCom` | `/home/<user>/Works/Softwares/wine/WeCom` |
| Wine | Deepin Wine 10.14deepin11-1 | 系统 Wine 11.18 |
| 企业微信 | 5.0.9.6039 | 5.0.11.6018 |
| 桌面入口 | `WeCom.desktop` | `WeCom-Native-Wine.desktop` |
| 角色 | 保留的稳定环境 | 本文重装步骤的依据 |

宿主为 Arch Linux，当前桌面会话为 Wayland，所检查的 Wine 窗口通过 XWayland 显示。本文不要求更改桌面缩放、输入法或系统字体。

## 目录内容与复用边界

```text
wecom-setup/
├── README.md                         配置总览
├── install-from-scratch.md           从零安装步骤
├── configuration-rationale.md        每项配置的原因及证据
├── verification-and-recovery.md      验证、升级、回退
├── titlebar-layer-fix.md             独立标题栏的窗口层级修复
├── troubleshooting-summary.md        原完整问题处理总结
├── configs/                         已验证的 Qt、字体注册表和 DLL 配置
├── materials/                       版本、来源、字体 SHA256、CEF 文件清单
├── evidence/                        去除本机路径和用户数据的验证摘要
└── scripts/                         材料校验、启动、提示字体与标题栏工具
```

这个目录包含文档、配置和小型辅助工具，**不是完整离线安装包**。企业微信安装程序、整套 CEF、微软 RichEdit 和 31 个字体文件仍需按 [材料准备](install-from-scratch.md#1-准备材料) 留存。不要在未保存材料时删除旧环境。

公开版本中的 `/home/<user>` 是历史路径示例，请替换为自己的用户名。材料默认使用仓库根目录下的 `assets/`（已被 Git 忽略）；清单里的相对材料路径均相对于仓库根目录。大型日志、转储、截图、账号数据、原 DLL 和历史诊断/回退脚本未上传；可查阅 [验证摘要](evidence/verification-summary.json)。历史总结中的原始文件名仅用于追溯，不代表仓库包含这些文件。

本次整理核对了配置、材料校验值、脚本语法与文件链接，没有再次创建 prefix 或重装应用。安装文档将已执行过的操作重新编排；新启动脚本也已做语法检查，但整套重装流程没有作为一次全新安装重跑。
