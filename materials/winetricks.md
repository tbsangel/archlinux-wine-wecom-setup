# 当前修复与 winetricks 组件对照

2026-10-12 核对当前安装文件、[DLL 覆盖配置](../configs/dll-overrides.txt)、本机 winetricks `20260125-next` 配方和下载缓存。源码核对及缓存文件比对不等于重新安装后的功能验收；本次未对日常 prefix 执行 winetricks。

本机 winetricks 脚本 SHA256：`672a1ff4442e8691a3ffc0e6860137e201a1d1227e9b3044245f1731e9e9837e`。配方可在 [Winetricks 上游源码](https://github.com/Winetricks/winetricks/blob/master/src/winetricks)中按表内名称查找；后续配方和下载版本可能变化。逐文件核对结果见 [winetricks-comparison.json](winetricks-comparison.json)。

## 微软 DLL

| 当前文件/用途 | 对应 winetricks 包 | 核对结论 |
|---|---|---|
| x86 `riched20.dll` 5.30.23.1200，修复聊天编辑框方框 | **`riched30`** | 从该配方的 `InstMsiA.exe` 提取出的 DLL 与当前文件 SHA256 完全相同；不要按 DLL 名误选 `riched20`，后者从 Windows 2000 SP4 取另一版本 |
| `msls31.dll`，RichEdit 的 Line Services | 随 `riched30` 安装；另有 `msls31` / `msls31_nt4` | `riched30` 已复制配套文件，不必再叠装其他版本；本项目原修复只替换 RichEdit，未独立要求增加其覆盖项 |
| `msvcp140`、`msvcp140_1`、`msvcp140_2`、`msvcp140_atomic_wait`、`msvcp140_codecvt_ids`、`concrt140`、`vcruntime140`、x64 `vcruntime140_1`、`vcruntime140_threads` | **`vcrun2022`** | 属于同一 VC14 运行库系列；当前主程序自带 x86 14.38.33135.0，CEF 自带 x64 14.38.33130.0；本机该配方缓存为 14.44.35211.0，文件并不相同。缓存包也包含 `vcruntime140_threads`，但配方未单列其覆盖项 |
| CEF 中的 `vccorlib140.dll` | `vcrun2022` | 同属 VC14；当前为 14.38.33130.0，已经随 CEF 提供，不需要单独收集 |
| `ucrtbase.dll` | **`ucrtbase2019`** | 当前主程序是 10.0.10240.16384，CEF 是 10.0.22621.2428；该配方缓存的 x86/x64 均为 10.0.10586.15，不能当作相同版本。当前 `vcrun2019` 配方会调用它，`vcrun2022` 配方不会 |
| CEF 的 `api-ms-win-crt-*` | VC/UCRT 的配套 API-set 转发文件 | 早期 `vcrun2015/2017` 配方包含此类覆盖；不应把它们当作独立应用插件逐个安装，也不要为凑文件叠装多个 vcrun 年份 |
| CEF 的 `api-ms-win-core-*`、`api-ms-win-downlevel-*` | 没有与这套厂商目录一一等价的单个包 | 保留随组件配套的文件，不能仅凭相似名称声称已由 vcrun 完整替代 |
| CEF 的 x64 `d3dcompiler_47.dll` 10.0.22621.3233 | **`d3dcompiler_47`** | 有同名配方，但从 Windows SDK CAB 获取，不能保证与 CEF 的版本/哈希相同；当前无须额外安装 |
| CEF 的 x64 `dbghelp.dll` 10.0.22621.1778 | `dbghelp`（不等价） | 所核对配方提取 XP SP3 的 x86 文件，不能替代本组件的 x64 文件 |
| 邮件/文档/审批的输入提交 | `msctf`（不等价） | 配方提供 XP x86 TSF 组件，未修复已定位的 Wine XIM 跨进程焦点问题；继续使用已验证的输入桥接 |
| `WXWorkWeb.exe` / `libcef.dll` | 没有对应包；`webview2` 不等价 | 企业微信此处使用 CEF；Edge WebView2 运行时不能替代这套组件 |

当前 VC/UCRT 修复实际是**使用企业微信已有 DLL 并设置原生优先**，没有额外安装 vcrun。保留这一已验证方案，通常不必重复下载微软运行库。确需在独立试验 prefix 比较微软安装包时，候选命令为：

```bash
env WINEPREFIX='/path/to/test-prefix' WINE=/usr/bin/wine WINESERVER=/usr/bin/wineserver \
  winetricks -q vcrun2022 ucrtbase2019
```

这条命令是替代方案的试验入口，不是日常环境的必需步骤。winetricks 向 Windows 系统目录安装文件，应用目录内仍存在的原生 DLL 可能继续优先加载；要证明替代成功，需检查实际加载路径并重新验收，不能只看安装器退出成功。`vcrun2015/2017/2019/2022/2026` 是替代关系，配方声明冲突，不应全部叠装。运行库包也不能代替 Qt DPI、窗口/托盘或输入桥接修复。

## RichEdit 安装

在新装步骤已经初始化、停止的目标 prefix 中，先保留原 x86 DLL，再执行：

```bash
env WINEPREFIX="$WECOM_PREFIX" WINE=/usr/bin/wine WINESERVER=/usr/bin/wineserver \
  winetricks -q riched30
python3 "$WECOM_SETUP/scripts/verify-materials.py" --only richedit \
  --richedit "$WECOM_PREFIX/drive_c/windows/syswow64/riched20.dll"
```

当前匹配的 DLL SHA256 为 `1c2508fb55ddc459d0327f2017471545c87420443391567094e768fb34032da1`；上游安装包 SHA256 为 `536e4c8385d7d250fd5702a6868d1ed004692136eefad22252d0dac15f02563a`。启动器继续使用 `riched20=n,b`。本项目和 Release 均不分发这两个文件。

## 字体安装

西文字体可用 winetricks 准备：

```bash
env WINEPREFIX="$WECOM_PREFIX" WINE=/usr/bin/wine WINESERVER=/usr/bin/wineserver \
  winetricks -q corefonts calibri consolas tahoma micross
```

只想安装对照表中的字体族，可将 `corefonts` 换为 `arial courier times verdana`；`corefonts` 会额外安装其他西文字体。字体文件不随本项目或 Release 上传。

| 当前字体 | 对应包 | 缓存提取文件与当前清单的比对 |
|---|---|---|
| Arial / Times New Roman / Verdana，各 4 款 | `arial` / `times` / `verdana`，均含于 `corefonts` | 12 个文件的 SHA256 不同；是同名字体的另一版本 |
| Courier New，4 款 | `courier`，含于 `corefonts` | 4 个文件完全相同 |
| Calibri，4 款 | `calibri` | 4 个文件完全相同 |
| Consolas，4 款 | `consolas` | 4 个文件完全相同 |
| Tahoma，2 款 | `tahoma` | 2 个文件完全相同 |
| Microsoft Sans Serif | `micross` | 文件完全相同 |
| Source Han Sans CN Regular / Bold | `sourcehansans` 仅为相关替代 | 配方提供 2.004 TTC / `Source Han Sans SC`；当前配置要求 2.005 CN OTF / `Source Han Sans CN`，文件名、字体族不同 |
| Source Han Serif CN Regular / Bold | 所核对配方没有 `sourcehanserif` | 当前使用 2.003 CN OTF，从 Adobe 官方版本取得 |

精确复现目前中文字体映射时，从 Adobe 的 [Source Han Sans 2.005R](https://github.com/adobe-fonts/source-han-sans/releases/tag/2.005R) 和 [Source Han Serif 2.003R](https://github.com/adobe-fonts/source-han-serif/releases/tag/2.003R) 取得 CN 子集 Regular/Bold OTF，共四个文件，文件名与 SHA256 见 [fonts.json](fonts.json)。不要把 TTC 改名为 OTF；若采用 `winetricks sourcehansans`，必须同时调整映射和 StatusFont 的字体族，再验收中文显示。

`fonts.json` 继续记录原已验证环境，不能为了让校验通过而将不同版本的哈希直接改掉。采用 winetricks 的西文字体版本后，12 个文件会与历史清单不同，须按[验收清单](../verification-and-recovery.md#验收清单)检查显示、输入和提示；本次仅确认上述文件差异，没有宣称替代字体组合已经完成 UI 验收。

从零安装的第 6 节保留精确材料复制路径。采用本节 winetricks 路径时，跳过那段要求字体尚不存在的批量复制代码；把四个 CN OTF 另行复制到目标 `windows/Fonts`，随后仍需导入字体注册与替换配置，并执行第 8 节的 StatusFont 设置。
