# Arch Linux 原生 Wine 企业微信

复现已验证的 **企业微信 5.0.11.6018 + 原生 Wine + x64 CEF 5.0.79.5483**，并保留安装理由与维护方法。更新于 2026-10-12。

| 需要做什么 | 文档 |
|---|---|
| 从空 Wine prefix 安装 | [从零安装](install-from-scratch.md)：材料校验、配置、桌面入口 |
| 了解配置的作用与原因 | [配置说明](configuration-rationale.md)：页面、字体、启动和窗口修复 |
| 启动、排查、更新或回退 | [使用与维护](verification-and-recovery.md)：验收范围、日志、辅助工具 |
| 下载修复素材与示例 | [examples/](examples/)：x64 CEF Release 素材、校验与桌面入口 |
| 对照 winetricks 可安装组件 | [组件对照](materials/winetricks.md)：RichEdit、字体、VC/UCRT 与版本差异 |

```bash
git clone https://github.com/tbsangel/archlinux-wine-wecom-setup.git
cd archlinux-wine-wecom-setup
```

## 当前环境

- 企业微信 **5.0.11.6018**；首次安装使用 Wine **11.18**，后续启动和窗口修复在 **11.19** 下验证。
- Arch Linux，Plasma/KWin **6.7.5**，Wayland/XWayland；双屏分别为 **135% / 100%** 缩放。
- 原生环境：`~/Works/Softwares/wine/WeCom`，桌面入口 `WeCom-Native-Wine.desktop`。

文档、中文输入及提示、启动速度、标题栏和边缘缩放已确认正常。托盘已验证双屏菜单及悬浮提示定位、点击外部关闭菜单、空白占位闪烁，以及左键置前和最小化恢复。

fcitx5 输入桥接已安装到日常环境，继续使用系统 Wine 11.19 和原桌面入口。用户确认邮件正文、文档正文及审批输入框均能提交中文；此前标题和聊天输入也已验证正常。邮件收发、长期使用及整理后的从零安装尚未完整验收，详见[验证范围](verification-and-recovery.md#验证范围)。已有安装更新配套脚本时，直接使用[维护步骤](verification-and-recovery.md#更新启动辅助文件)。

## 仓库内容

| 目录 | 内容 |
|---|---|
| [configs/](configs/) | Qt、字体注册表、DLL 加载配置 |
| [materials/](materials/) | 安装包、CEF、RichEdit、字体的版本与校验清单 |
| [scripts/](scripts/) | 材料校验、启动及兼容辅助工具；两个 EXE 和输入桥接 SO 均附源码 |
| [examples/](examples/) | 修复文件索引、桌面入口示例、CEF 下载和校验信息 |

完整 x64 CEF 作为同一仓库的 [Release 示例素材](https://github.com/tbsangel/archlinux-wine-wecom-setup/releases/tag/compat-2026-10-12)提供，下载后放入被 Git 忽略的 `assets/cef/`。RichEdit 使用 `winetricks riched30`，字体和其他微软包的获取方式及差异见[组件对照](materials/winetricks.md)，不另行上传这些材料。安装程序仍需自行准备。账号数据、原始诊断材料及历史文档归档不随仓库分发。文档中的 `/home/<user>` 须替换为实际路径。
