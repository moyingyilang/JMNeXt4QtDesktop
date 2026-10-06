#!/bin/sh
# 从主项目（JMComic_Next）的 theme 文件生成 Qt 侧的配色表，**数值原样搬运**，不做任何转换。
#
# 用法：tools/gen-theme-tokens.sh <主项目 theme 目录> [输出文件]
#
# 为什么要生成：五套风格 × 深浅 × 20 多个 token 是两百多个数值，手抄必然出错。
# 生成结果**提交进仓库**，所以本仓库构建时不依赖主项目。
set -e
SRC=${1:?需要主项目 theme 目录}
OUT=${2:-src/qt/ThemeTokens.h}
TOK=$SRC/Tokens.kt
PAL=$SRC/Palettes.kt
[ -f "$TOK" ] || { echo "找不到 $TOK"; exit 3; }

pairs() {   # pairs <文件> <起始行> <结束行> -> "字段 #AARRGGBB"
  sed -n "$2,$3p" "$1" | grep -oE '^[[:space:]]+[a-zA-Z]+ = (Color\(0x[0-9A-Fa-f]{8}\)|Color\.White|Color\.Black)' | \
    sed -E 's/^[[:space:]]+([a-zA-Z]+) = Color\(0x([0-9A-Fa-f]{8})\)/\1 #\2/;
            s/^[[:space:]]+([a-zA-Z]+) = Color\.White/\1 #FFFFFFFF/;
            s/^[[:space:]]+([a-zA-Z]+) = Color\.Black/\1 #FF000000/'
}
block_range() {   # block_range <文件> <val 名> -> "起始 结束"
  s=$(grep -n "^val $2" "$1" | head -1 | cut -d: -f1)
  [ -n "$s" ] || { echo "找不到 $2 于 $1" >&2; return 3; }
  e=$(awk -v s="$s" 'NR>s && /^\)/ {print NR; exit}' "$1")
  echo "$s $e"
}
R=$(block_range "$TOK" LightPalette); LS=${R% *}; LE=${R#* }
R=$(block_range "$TOK" DarkPalette);  DS=${R% *}; DE=${R#* }
pairs "$TOK" "$LS" "$LE" > .light.base
pairs "$TOK" "$DS" "$DE" > .dark.base
FIELDS=$(awk '{print $1}' .light.base)

# 把"基色板 + 改写"合并成一份完整表（改写里没有的字段沿用基色板）
merge() {   # merge <base 文件> <val 名> <输出>
  cp "$1" "$3"
  R=$(block_range "$PAL" "$2"); s=${R% *}; e=${R#* }
  pairs "$PAL" "$s" "$e" | while read -r f v; do
    if grep -q "^$f " "$3"; then
      awk -v f="$f" -v v="$v" '$1==f {print f" "v; next} {print}' "$3" > "$3.tmp" && mv "$3.tmp" "$3"
    else
      echo "  [跳过] $2 里的字段 $f 不在基色板字段表中（未纳入）" >&2
    fi
  done
}
merge .light.base TranslucentLight .light.translucent
merge .dark.base  TranslucentDark  .dark.translucent
merge .light.base FlatBlurLight    .light.flatblur
merge .dark.base  FlatBlurDark     .dark.flatblur
merge .light.base MiuixLight       .light.miuix
merge .dark.base  MiuixDark        .dark.miuix

{
  echo "// 本文件由 tools/gen-theme-tokens.sh 生成，请勿手改。"
  echo "// 数值来源：主项目 JMComic_Next 的 Tokens.kt / Palettes.kt（原样搬运，未做转换）。"
  echo "// Material 风格不在此表内：它直接用 Material You 3 的颜色角色（主项目也是这么做的）。"
  echo "#pragma once"
  echo
  echo "namespace jmnext::qt::tokens {"
  echo
  echo "struct Palette {"
  for f in $FIELDS; do echo "    const char* $f;"; done
  echo "};"
  echo
  emit() {   # emit <函数名> <light 表> <dark 表>
    echo "inline Palette $1(bool dark) {"
    for pair in "light $2" "dark $3"; do
      set -- $pair
      echo "    static const Palette ${1}Pal = [] {"
      echo "        Palette p{};"
      for f in $FIELDS; do
        v=$(awk -v f="$f" '$1==f {print $2}' "$2")
        echo "        p.$f = \"$v\";"
      done
      echo "        return p;"
      echo "    }();"
    done
    echo "    return dark ? darkPal : lightPal;"
    echo "}"
    echo
  }
  emit windowGlass .light.base       .dark.base
  emit translucent .light.translucent .dark.translucent
  emit flatBlur    .light.flatblur    .dark.flatblur
  emit miuix       .light.miuix       .dark.miuix
  echo "}  // namespace jmnext::qt::tokens"
} > "$OUT"
rm -f .light.* .dark.*
echo "已生成 $OUT"
echo "  token 字段数：$(awk '/^struct Palette/{f=1;next} /^};/{f=0} f&&/const char\*/{n++} END{print n}' "$OUT")"
echo "  赋值行数：$(grep -c '^        p\.' "$OUT")"
