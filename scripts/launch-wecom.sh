#!/usr/bin/env bash
set -euo pipefail
: "${WECOM_PREFIX:?Set WECOM_PREFIX to the intended WeCom prefix}"
script_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
app="$WECOM_PREFIX/drive_c/Program Files (x86)/WXWork/WXWork.exe"
test -f "$WECOM_PREFIX/system.reg"
test -f "$app"
overrides="$(cat "$script_dir/../configs/dll-overrides.txt")"
cd -- "$(dirname -- "$app")"
wine_env=(env -u LC_ALL -u WINEARCH -u WINESERVER -u WINELOADER -u WINEDLLPATH
  LANG=zh_CN.UTF-8 LC_CTYPE=zh_CN.UTF-8 LC_MESSAGES=zh_CN.UTF-8
  WINEPREFIX="$WECOM_PREFIX" WINEDLLOVERRIDES="$overrides" WINEDEBUG=-all)
if [[ "${WECOM_TITLEBAR_FIX:-1}" != 0 ]]; then
  test -f "$script_dir/titlebar-managed.exe"
  "${wine_env[@]}" /usr/bin/wine "$script_dir/titlebar-managed.exe" -watch \
    </dev/null >>"$WECOM_PREFIX/wecom-titlebar.log" 2>&1 &
fi
exec "${wine_env[@]}" /usr/bin/wine "$app"
