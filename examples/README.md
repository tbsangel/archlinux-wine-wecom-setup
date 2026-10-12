# 修复示例与素材

本目录对应企业微信 **5.0.11.6018 / 系统 Wine 11.19 / fcitx5 / KDE Wayland** 的已验证修复。源码和小型辅助工具随 Git 仓库提供，大型 CEF 素材放在同一项目的 [2026-10-12 Release](https://github.com/tbsangel/archlinux-wine-wecom-setup/releases/tag/compat-2026-10-12)。

## 各项修复需要什么

| 修复 | 配套文件或获取方式 |
|---|---|
| 文档、工作台的 CEF 页面崩溃 | Release 的完整 x64 CEF 5.0.79.5483，见下方下载步骤 |
| 文档列表无法嵌入 | [qt.conf](../configs/qt.conf) 和[加载顺序](../configs/dll-overrides.txt) |
| 中文编辑框、字体和提示 | RichEdit、字体通过 [winetricks 对照说明](../materials/winetricks.md)准备；[字体注册](../configs/fonts-install.reg)、[字体替换](../configs/fonts-replacements.reg)、[StatusFont 工具](../scripts/status-font-probe.c)随仓库提供 |
| 邮件、文档正文和审批选词不出字 | [ime-bridge.c / SO](../scripts/ime-bridge.c)，由[启动器](../scripts/launch-wecom.sh)加载 |
| 五分钟启动等待 | [prepare-startup.py](../scripts/prepare-startup.py) |
| 标题栏、边缘拖动、托盘菜单定位/关闭、点击激活 | [titlebar-managed.c / EXE](../scripts/titlebar-managed.c) 和 [tray-geometry.py](../scripts/tray-geometry.py) |
| 托盘提醒闪黑块 | [tray-icon.py](../scripts/tray-icon.py) |
| 桌面入口 | [WeCom-Native-Wine.desktop](WeCom-Native-Wine.desktop)，先替换两处 `<user>` 和 prefix 路径 |

辅助程序的构建、开关和更新方式见[使用与维护](../verification-and-recovery.md)。示例入口引用安装到 prefix 内的工具；先完成安装，再复制入口。

## 下载 CEF 示例素材

在仓库根目录的 Bash 中执行。需要 `curl`、`sha256sum`、支持 xz 的 `tar` 和 Python 3：

```bash
set -euo pipefail
mkdir -p dist assets
curl --fail --location --retry 3 \
  https://github.com/tbsangel/archlinux-wine-wecom-setup/releases/download/compat-2026-10-12/wecom-cef-x64-5.0.79.5483.tar.xz \
  --output dist/wecom-cef-x64-5.0.79.5483.tar.xz
(cd dist && sha256sum --check ../examples/cef/SHA256SUMS)
test ! -e assets/cef  # 已有材料时先核对，不直接覆盖
tar --extract --xz --no-same-owner --file dist/wecom-cef-x64-5.0.79.5483.tar.xz
python3 scripts/verify-materials.py --only cef
```

解压得到 `assets/cef/`，共 **89 个清单文件**。包的 SHA256、体积和来源说明见 [bundle.json](cef/bundle.json)，文件级校验见[厂商清单](../materials/cef64-vendor-manifest.json)。下载和解压不会修改已运行的企业微信；应用到新环境按[安装步骤](../install-from-scratch.md#4-更换整套-64-位-web-组件)操作。

素材包保留厂商组件的完整目录，包括随 CEF 配套的运行库。微软组件与 winetricks 的对应关系、版本差异见[对照表](../materials/winetricks.md)；尚未验证拆掉这些 DLL 后仅用系统目录中的 winetricks 运行库能保持相同行为。CEF、腾讯程序和微软 DLL 仍属于各自权利人，作为第三方组件记录来源，不由本项目重新授权。

**字体文件、单独的 RichEdit、微软运行库安装包、企业微信安装程序均不上传。** 素材不包含账号、聊天、邮件、文档、注册表导出、崩溃转储或原始日志。`assets/` 和 `dist/` 被 Git 忽略，压缩包仅作为 Release 附件。

## 从已验证组件重建素材包

```bash
python3 scripts/package-cef-example.py \
  --cef-dir '/path/to/verified/compatible_web' \
  --output dist/wecom-cef-x64-5.0.79.5483-rebuilt.tar.xz
```

脚本按清单逐个读取并验证文件，固定归档元数据，不递归复制整个 prefix。现有输出文件不会被覆盖。不同 Python/xz 版本的压缩结果可能不同，文件级清单是内容一致性的依据。
