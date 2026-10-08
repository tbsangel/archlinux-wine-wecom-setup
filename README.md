# Arch Linux 原生 Wine 企业微信

复现已验证的 **企业微信 5.0.11.6018 + 原生 Wine + x64 CEF 5.0.79.5483**，并保留安装理由与维护方法。更新于 2026-10-08。

| 需要做什么 | 文档 |
|---|---|
| 从空 Wine prefix 安装 | [从零安装](install-from-scratch.md)：材料校验、配置、桌面入口 |
| 了解配置的作用与原因 | [配置说明](configuration-rationale.md)：页面、字体、启动和窗口修复 |
| 启动、排查、更新或回退 | [使用与维护](verification-and-recovery.md)：验收范围、日志、辅助工具 |

```bash
git clone https://github.com/tbsangel/archlinux-wine-wecom-setup.git
cd archlinux-wine-wecom-setup
```

## 当前环境

- 企业微信 **5.0.11.6018**；首次安装使用 Wine **11.18**，后续启动和窗口修复在 **11.19** 下验证。
- Arch Linux，Plasma/KWin **6.7.5**，Wayland/XWayland；双屏分别为 **135% / 100%** 缩放。
- 原生环境：`~/Works/Softwares/wine/WeCom`，桌面入口 `WeCom-Native-Wine.desktop`。

文档、中文输入及提示、启动速度、标题栏和边缘缩放已确认正常。托盘已验证双屏菜单及悬浮提示定位、点击外部关闭菜单、空白占位闪烁，以及左键置前和最小化恢复。

最终组合的完整重启、邮件、长期使用及整理后的从零安装尚未完整验收，详见[验证范围](verification-and-recovery.md#验证范围)。已有安装更新配套脚本时，直接使用[维护步骤](verification-and-recovery.md#更新启动辅助文件)。

## 仓库内容

| 目录 | 内容 |
|---|---|
| [configs/](configs/) | Qt、字体注册表、DLL 加载配置 |
| [materials/](materials/) | 安装包、CEF、RichEdit、字体的版本与校验清单 |
| [scripts/](scripts/) | 材料校验、启动及兼容辅助工具；两个 EXE 均附源码 |

安装程序、厂商 DLL 和字体需按[材料准备](install-from-scratch.md#1-准备材料)自行备齐，默认放入被 Git 忽略的 `assets/`。账号数据、原始诊断材料及历史文档归档不随仓库分发。文档中的 `/home/<user>` 须替换为实际路径。
