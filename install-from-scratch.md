# 从零安装原生 Wine 企业微信

目标组合：Wine 11.18、企业微信 5.0.11.6018、原始 x64 Web 组件 5.0.79.5483。先备齐材料，再初始化一个空 prefix。以下示例使用新路径 `WeCom-rebuild`，避免覆盖当前已经可用的两个环境。

先在仓库根目录打开 Bash。所有命令在普通用户的 **同一个 Bash 终端** 中分节执行；每节成功后再继续。不要使用 `sudo wine`。材料路径可修改，修改后重新运行材料校验。

## 1. 准备材料

本机已有系统 `/usr/bin/wine` 和 `/usr/bin/wineserver`，实测 `wine --version` 为 `wine-11.18`；另需 Python 3、Bash、已生成的 `zh_CN.UTF-8` locale。若在其他机器安装 Wine，先使用该发行版支持的安装方式准备能运行 32/64 位程序的 Wine，再核对版本。这里没有把滚动更新的软件包命令等同于固定的 11.18 安装方式。

| 材料 | 本机已验证来源 | 需要留存的内容 |
|---|---|---|
| 安装程序 | `/home/<user>/Works/Download/WeCom_5.0.11.6018.exe` | 完整 EXE；SHA256 见 `materials/versions.json` |
| x64 CEF | 旧 prefix 的 `drive_c/users/<user>/AppData/Roaming/Tencent/WXWork/cef/5.0.79.5483/cef/` | 完整目录，89 个清单文件，不只 `libcef.dll` |
| x86 RichEdit | 旧 prefix 的 `drive_c/windows/syswow64/riched20.dll` | Microsoft 5.30.23.1200；SHA256 必须匹配 |
| 27 个西文字体 | 旧 prefix 的 `drive_c/windows/Fonts/` | 具体文件名和校验值见 `materials/fonts.json` |
| 4 个中文字体 | 宿主 `/usr/share/fonts/adobe-source-han-sans/`、`adobe-source-han-serif/` | 思源黑体/宋体 CN 常规及粗体 OTF |

CEF 是企业微信自行下载的腾讯组件，RichEdit 是本机旧环境已有的微软组件。这里没有提供重新从网络取得同一版本 RichEdit/CEF 的已验证下载流程。全新机器上如果没有这些材料，需要先取得并核对材料；不能把“装好 Wine 后运行安装包”视作已经能复现本配置。材料复制完成后，新 prefix 运行不依赖旧 prefix。

将已取得的材料整理为以下结构，或在下方变量中填写自己的材料路径。`assets/` 已被 Git 忽略，不会随正常提交上传：

```text
assets/
├── WeCom_5.0.11.6018.exe
├── cef/                    完整 x64 Web 组件目录
├── riched20.dll             已核对的 x86 微软组件
└── fonts/                  清单中的 31 个字体文件
```

`WECOM_FONTS_SRC` 留空时同样按清单从仓库 `assets/fonts/` 读取。

```bash
bash
set -euo pipefail

export WECOM_SETUP="$PWD"
export WECOM_PREFIX="$HOME/Works/Softwares/wine/WeCom-rebuild"
export WECOM_INSTALLER="$WECOM_SETUP/assets/WeCom_5.0.11.6018.exe"
export WECOM_CEF_SRC="$WECOM_SETUP/assets/cef"
export WECOM_RICHEDIT_SRC="$WECOM_SETUP/assets/riched20.dll"
export WECOM_FONTS_SRC="$WECOM_SETUP/assets/fonts"

/usr/bin/wine --version
locale -a | rg -i '^zh_CN\.(utf8|UTF-8)$'

check_args=(--installer "$WECOM_INSTALLER" --cef-dir "$WECOM_CEF_SRC" --richedit "$WECOM_RICHEDIT_SRC")
if [[ -n "$WECOM_FONTS_SRC" ]]; then
  check_args+=(--fonts-dir "$WECOM_FONTS_SRC")
fi
python3 "$WECOM_SETUP/scripts/verify-materials.py" "${check_args[@]}"
```

预期输出 `PASS`，覆盖安装程序、RichEdit 的 SHA256，89 个 CEF 文件的清单 MD5 和 x64 PE 架构，以及 31 个字体的 SHA256。清单 MD5 用于复核已知材料的一致性，不能替代来源核验。

## 2. 初始化独立 64 位 prefix

初始化前固定中文区域设置，避免 Wine 先按英文代码页生成默认字体配置。32 位主程序与 64 位 CEF 可以在同一 `win64` prefix 运行；`win32` prefix 不适合这个方案。

```bash
unset LC_ALL WINESERVER WINELOADER WINEDLLPATH
export LANG=zh_CN.UTF-8 LC_CTYPE=zh_CN.UTF-8 LC_MESSAGES=zh_CN.UTF-8
export WINEPREFIX="$WECOM_PREFIX" WINEARCH=win64 WINEDEBUG=-all
export WINEDLLOVERRIDES='winemenubuilder.exe=d'

if [[ -e "$WECOM_PREFIX" ]]; then
  printf '目标已存在，请换一个空路径：%s\n' "$WECOM_PREFIX"
  exit 1
fi
mkdir -p -- "$WECOM_PREFIX"
/usr/bin/wineboot --init
export WECOM_BACKUP="$WECOM_PREFIX/setup-backups"
mkdir -- "$WECOM_BACKUP"
wecom_winpath() { /usr/bin/winepath -w "$1"; }
```

`winemenubuilder.exe=d` 用来避免安装时自动生成混淆新旧环境的桌面菜单，不是 CEF 修复。

让 Windows 安装器的 Desktop 留在新 prefix 内，单独创建 Linux 启动入口：

```bash
python3 - <<'PY'
import getpass, os
from pathlib import Path
desktop = Path(os.environ['WECOM_PREFIX']) / 'drive_c/users' / getpass.getuser() / 'Desktop'
if desktop.is_symlink():
    desktop.unlink()  # 只移除 prefix 内的映射，不删除真实桌面
    desktop.mkdir()
PY
```

Windows Documents 默认可能映射到宿主用户的 Documents；本次未复制旧聊天账号数据，也没有要求修改该映射。

## 3. 安装指定企业微信版本

```bash
/usr/bin/wine "$WECOM_INSTALLER" /S
export WECOM_APP_DIR="$WECOM_PREFIX/drive_c/Program Files (x86)/WXWork"
export WECOM_VERSION_DIR="$WECOM_APP_DIR/5.0.11.6018"
test -f "$WECOM_APP_DIR/WXWork.exe"
test -d "$WECOM_VERSION_DIR/compatible_web"

# 安装器可能已启动企业微信；只结束这个新 prefix。
/usr/bin/wineserver -k
/usr/bin/wineserver -w
```

此时先不要把能打开主界面当作安装完成。默认 Web 组件、Qt 嵌入和字体还没有处理。

## 4. 更换整套 64 位 Web 组件

```bash
test ! -e "$WECOM_VERSION_DIR/compatible_web.x86-original"
test ! -e "$WECOM_VERSION_DIR/compatible_web.x64-staged"
cp -a -- "$WECOM_CEF_SRC" "$WECOM_VERSION_DIR/compatible_web.x64-staged"
mv -- "$WECOM_VERSION_DIR/compatible_web" "$WECOM_VERSION_DIR/compatible_web.x86-original"
mv -- "$WECOM_VERSION_DIR/compatible_web.x64-staged" "$WECOM_VERSION_DIR/compatible_web"
```

替换的是整套 `WXWorkWeb.exe`、`libcef.dll`、配套 DLL、资源及 locales。主程序仍然是 x86；不需要替换主程序，不需要对这套 x64 DLL 打二进制补丁。目录名仍为 `compatible_web`。

## 5. 配置运行库及 Qt DPI

使用安装包自带微软 VC/UCRT DLL，配置完整列表在 [dll-overrides.txt](configs/dll-overrides.txt)：

```bash
export WINEDLLOVERRIDES="$(cat "$WECOM_SETUP/configs/dll-overrides.txt")"
test -f "$WECOM_VERSION_DIR/msvcp140.dll"
test -f "$WECOM_VERSION_DIR/ucrtbase.dll"

test ! -e "$WECOM_VERSION_DIR/wemail/qt.conf"
cp -- "$WECOM_SETUP/configs/qt.conf" "$WECOM_VERSION_DIR/wemail/qt.conf"
```

`qt.conf` 内容：

```ini
[Platforms]
WindowsArguments = dpiawareness=1
```

DLL 的 `n,b` 表示优先微软原生实现、保留 Wine 内置回退。本次未额外运行通用 `vcrun` 安装器。Qt 配置针对文档列表不能嵌入的问题，不需要改变 Linux 或 Wine 的整体 DPI 数值。

## 6. 安装字体及两种字体映射

先备份新环境原有键值，再复制材料并注册。以下 `.reg` 是这次已使用的固定字体集合，只适合配套的 31 个文件；如果换字体，需同步修改注册表中的内部字体族名和文件名。

```bash
/usr/bin/wine reg.exe export 'HKLM\Software\Microsoft\Windows NT\CurrentVersion\Fonts' "$(wecom_winpath "$WECOM_BACKUP/fonts.reg")" /y
/usr/bin/wine reg.exe export 'HKLM\Software\Microsoft\Windows\CurrentVersion\Fonts' "$(wecom_winpath "$WECOM_BACKUP/legacy-fonts.reg")" /y
/usr/bin/wine reg.exe export 'HKLM\Software\Microsoft\Windows NT\CurrentVersion\FontSubstitutes' "$(wecom_winpath "$WECOM_BACKUP/substitutes.reg")" /y
/usr/bin/wine reg.exe export 'HKLM\Software\Microsoft\Windows NT\CurrentVersion\FontLink\SystemLink' "$(wecom_winpath "$WECOM_BACKUP/links.reg")" /y

python3 - <<'PY'
import hashlib, json, os, shutil
from pathlib import Path
setup = Path(os.environ['WECOM_SETUP'])
entries = json.loads((setup / 'materials/fonts.json').read_text())
dest = Path(os.environ['WECOM_PREFIX']) / 'drive_c/windows/Fonts'
flat = os.environ['WECOM_FONTS_SRC']
pairs = [(Path(flat) / x['filename'] if flat else setup / x['source'], dest / x['filename'], x) for x in entries]
for source, target, info in pairs:
    assert not target.exists(), f'目标字体已存在，先检查：{target}'
    assert hashlib.sha256(source.read_bytes()).hexdigest() == info['sha256'], source
for source, target, info in pairs:
    shutil.copy2(source, target)
    target.chmod(0o644)
    assert hashlib.sha256(target.read_bytes()).hexdigest() == info['sha256']
print('31 个字体已复制并核对')
PY

/usr/bin/wine reg.exe import "$(wecom_winpath "$WECOM_SETUP/configs/fonts-install.reg")"
/usr/bin/wine reg.exe import "$(wecom_winpath "$WECOM_SETUP/configs/fonts-replacements.reg")"
```

在此全新 prefix 中，`Fonts/Replacements` 原本不存在。对已有环境应用时要先查询并备份它，不能假定为空。

字体分工见 [配置说明](configuration-rationale.md#字体不是只有一层配置)：字体文件、GDI `FontSubstitutes`、`FontLink` 和 Wine `Fonts/Replacements` 各有用途。Tahoma 等默认 FontLink 可能因区域设置变化被重写，不能仅凭导入成功就认定悬浮提示修复。

## 7. 配置 32 位微软 RichEdit

停止新 prefix 后再替换该 DLL，并保留原文件：

```bash
/usr/bin/wineserver -k
/usr/bin/wineserver -w
test ! -e "$WECOM_BACKUP/riched20.dll"
cp -p -- "$WECOM_PREFIX/drive_c/windows/syswow64/riched20.dll" "$WECOM_BACKUP/riched20.dll"
cp -p -- "$WECOM_RICHEDIT_SRC" "$WECOM_PREFIX/drive_c/windows/syswow64/riched20.native-staged.dll"
mv -- "$WECOM_PREFIX/drive_c/windows/syswow64/riched20.native-staged.dll" "$WECOM_PREFIX/drive_c/windows/syswow64/riched20.dll"
```

已加载的覆盖列表含 `riched20=n,b`。注意 `syswow64` 放的是 x86 DLL，`system32` 放的是 x64 DLL；此次不改 x64 RichEdit，不复制旧环境全部系统 DLL。

## 8. 设置系统提示字体

```bash
/usr/bin/wine reg.exe export 'HKCU\Control Panel\Desktop\WindowMetrics' "$(wecom_winpath "$WECOM_BACKUP/window-metrics.reg")" /y
/usr/bin/wine "$WECOM_SETUP/scripts/status-font-probe.exe" -set
```

这个小工具通过 `SPI_GET/SETNONCLIENTMETRICS` 将 `lfStatusFont` 字体名改为 `Source Han Sans CN`，沿用原字号和字重并保存设置。对应键为 `HKCU\Control Panel\Desktop\WindowMetrics\StatusFont`，不是一个简单的字符串注册表值；工具和源码一同留存，避免手写错误的 LOGFONT 二进制数据。

预期输出包含 `selected=思源黑体 CN`（或英文族名），`missing samples:` 后为空。仍需在下面完整停止并重启，让企业微信的提示控件重新创建。

## 9. 启动与独立桌面入口

复制启动配套文件到 prefix，使日常启动不依赖文档目录。标题栏维护工具会将独立顶部横条纳入正常窗口管理，修复其他窗口激活时横条仍盖在最前的问题，原因见 [标题栏修复](titlebar-layer-fix.md)。

```bash
test ! -e "$WECOM_PREFIX/wecom-compat"
mkdir -p -- "$WECOM_PREFIX/wecom-compat/scripts" "$WECOM_PREFIX/wecom-compat/configs"
cp -- "$WECOM_SETUP/scripts/launch-wecom.sh" "$WECOM_SETUP/scripts/titlebar-managed.exe" \
  "$WECOM_SETUP/scripts/titlebar-managed.c" "$WECOM_PREFIX/wecom-compat/scripts/"
cp -- "$WECOM_SETUP/configs/dll-overrides.txt" "$WECOM_PREFIX/wecom-compat/configs/"
/usr/bin/wineserver -k
/usr/bin/wineserver -w
bash "$WECOM_PREFIX/wecom-compat/scripts/launch-wecom.sh"
```

启动脚本使用 `WECOM_PREFIX`，显式固定原生 Wine、中文区域设置及完整 DLL 覆盖列表，并启动标题栏维护工具；工具随主进程退出。首次登录在企业微信窗口中自行完成。

需要桌面图标时，在另一个终端创建一个**新名字**的 `.desktop`，不要覆盖已有入口。将示例中的 `<user>` 替换为真实用户名；Desktop Entry 不展开 `$HOME`，必须使用实际绝对路径：

```ini
[Desktop Entry]
Name=企业微信（原生 Wine 重建）
Type=Application
Exec=env WECOM_PREFIX=/home/<user>/Works/Softwares/wine/WeCom-rebuild /bin/bash /home/<user>/Works/Softwares/wine/WeCom-rebuild/wecom-compat/scripts/launch-wecom.sh
Icon=applications-office
Terminal=false
StartupNotify=true
StartupWMClass=wxwork.exe
```

这个示例入口依赖 prefix 内 `wecom-compat/` 的文件；移动 prefix 后更新 `Exec`。当前已经存在的测试桌面入口也已采用同样布局。

最后执行 [验收清单](verification-and-recovery.md#验收清单)，包括文档列表、文档正文、编辑框、悬浮提示，以及标题栏遮挡/拖动/最小化行为。
