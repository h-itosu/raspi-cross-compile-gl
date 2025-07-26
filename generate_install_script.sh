#!/bin/bash
#
# [デバッグ版] 実行ファイルを解析し、その依存ライブラリをインストールするための
# シェルスクリプトを自動生成します。
#

# --- デバッグ設定: 実行するコマンドをすべて表示し、エラーで停止しない ---
set -x

# --- 初期設定 ---
EXECUTABLE=$1
OUTPUT_SCRIPT="install_deps.sh"

echo "--- [デバッグ] スクリプト開始 ---"
echo "--- [デバッグ] 解析対象の実行ファイル: $EXECUTABLE"

if [ -z "$EXECUTABLE" ]; then
    echo "エラー: 実行ファイルが指定されていません。"
    exit 1
fi
if [ ! -f "$EXECUTABLE" ]; then
    echo "エラー: 実行ファイルが見つかりません: $EXECUTABLE"
    exit 1
fi

echo "--- [デバッグ] ldd の出力を確認します ---"
ldd "$EXECUTABLE"

echo "--- [デバッグ] ldd からライブラリパスを抽出します ---"
LIB_PATHS=$(ldd "$EXECUTABLE" | grep "=>" | awk '{print $3}' | grep '^/' | sort -u)
echo "--- [デバッグ] 抽出されたライブラリパス一覧 ---"
echo "$LIB_PATHS"
echo "--- [デバッグ] -------------------------- ---"


echo "--- [デバッグ] 各ライブラリパスからパッケージ名を検索します (dpkg -S) ---"
# エラーが発生しても処理を続けるため、forループで1つずつ処理する
RUNTIME_PACKAGES_LIST=()
while read -r lib; do
    # dpkg -S の出力を変数に格納し、成功したかチェック
    pkg_line=$(dpkg -S "$lib" 2>/dev/null)
    if [ $? -eq 0 ] && [ -n "$pkg_line" ]; then
        pkg_name=$(echo "$pkg_line" | cut -d: -f1)
        echo "  [成功] $lib -> $pkg_name"
        RUNTIME_PACKAGES_LIST+=("$pkg_name")
    else
        echo "  [情報] $lib はどのパッケージにも属していません (またはエラー)。スキップします。"
    fi
done <<< "$LIB_PATHS"

echo "--- [デバッグ] 発見された実行時パッケージ (重複あり) ---"
echo "${RUNTIME_PACKAGES_LIST[@]}"

# 重複を除外
RUNTIME_PACKAGES=$(echo "${RUNTIME_PACKAGES_LIST[@]}" | tr ' ' '\n' | sort -u)

echo "--- [デバッグ] 発見された実行時パッケージ (重複なし) ---"
echo "$RUNTIME_PACKAGES"


# --- 開発用パッケージ(-dev)の推測 ---
echo "--- [本処理] 開発用パッケージを推測します ---"
DEV_PACKAGES=()
for pkg in $RUNTIME_PACKAGES; do
    if [[ "$pkg" == "libc6" || "$pkg" == "libgcc-s1" ]]; then continue; fi
    base_name=$(echo "$pkg" | sed -E 's/([a-zA-Z-]+[a-zA-Z])[0-9.-]*/\1/' | sed 's/:armhf$//')
    dev_pkg="${base_name}-dev"
    if [[ "$dev_pkg" == "libcurl-dev" ]]; then dev_pkg="libcurl4-openssl-dev"; fi
    if [[ "$dev_pkg" == "libglesv2-dev" ]]; then dev_pkg="libgles2-mesa-dev"; fi
    if [[ "$dev_pkg" == "libegl-dev" ]]; then dev_pkg="libegl1-mesa-dev"; fi
    if apt-cache show "$dev_pkg" &>/dev/null; then
        echo "  -> Found dev package: $dev_pkg"
        DEV_PACKAGES+=("$dev_pkg")
    else
        echo "  ⚠️  Warning: 推奨パッケージ '$dev_pkg' が見つかりません。手動での確認が必要です。"
    fi
done
UNIQUE_DEV_PACKAGES=($(echo "${DEV_PACKAGES[@]}" | tr ' ' '\n' | sort -u | tr '\n' ' '))

# --- インストールスクリプトの生成 ---
echo "--- [本処理] インストールスクリプト '$OUTPUT_SCRIPT' を生成します ---"
cat > "$OUTPUT_SCRIPT" << EOL
#!/bin/bash
# '$EXECUTABLE' のための依存ライブラリ自動インストールスクリプト (自動生成日時: $(date))
set -eu
echo "--- パッケージリストを更新します ---"
sudo apt-get update
echo "--- 必要な開発ライブラリをインストールします ---"
sudo apt-get install -y --no-install-recommends gdbserver \\
    ${UNIQUE_DEV_PACKAGES[@]}
echo ""
echo "✅ セットアップが正常に完了しました！"
EOL
chmod +x "$OUTPUT_SCRIPT"
echo "✅ 生成完了: $OUTPUT_SCRIPT"
