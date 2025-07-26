#!/bin/bash
# '/home/h.itosu/raspi_gl_hello2' のための依存ライブラリ自動インストールスクリプト (自動生成日時: 2025年  7月 25日 金曜日 00:14:10 JST)
set -eu
echo "--- パッケージリストを更新します ---"
sudo apt-get update
echo "--- 必要な開発ライブラリをインストールします ---"
sudo apt-get install -y --no-install-recommends gdbserver \
    libexpat-dev liblzma-dev libtinfo-dev
echo ""
echo "✅ セットアップが正常に完了しました！"
