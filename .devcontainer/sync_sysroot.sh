#!/bin/bash
#
# sync_sysroot.sh: Raspberry PiからSysrootを同期するスクリプト
#
set -e # エラーが発生した時点でスクリプトを終了する

# --- 1. .env ファイルの読み込み ---
# コンテナ内ではプロジェクトルートが /workspace にマウントされるため、
# .env ファイルの絶対パスは /workspace/.env となる
ENV_FILE="/workspace/.env"
if [ -f "$ENV_FILE" ]; then
    echo "Loading environment variables from $ENV_FILE"
    # export を使い、読み込んだ変数をこのスクリプト内で有効にする
    export $(grep -v '^#' "$ENV_FILE" | xargs)
else
    echo "Warning: .env file not found. Using default values."
fi

# --- 2. 変数設定 ---
# .envから読み込んだ値があればそれを使用し、なければデフォルト値を使う
RASPI_USER=${RASPI_USER:-pi}
RASPI_HOST=${RASPI_HOST:-raspberrypi.local}
RASPI_PW=${RASPI_PW:-raspberry}

# コンテナ内のSysroot(擬似的なルートファイルシステム)のパス
SYSROOT_DIR="/opt/raspi_sysroot"
# SSH接続時のオプション (初回接続時のホストキー確認をスキップ)
SSH_OPTS="-o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null"

# 同期対象のディレクトリ
SYNC_DIRS=(
    "/lib/"
    "/usr/include/"
    "/usr/lib/"
)

# --- 3. スクリプト本体 ---
echo "--- Start Sysroot Sync from Raspberry Pi (${RASPI_HOST}) ---"

# Sysrootのルートディレクトリを作成
mkdir -p ${SYSROOT_DIR}

# 各ディレクトリをrsyncで同期
for dir in "${SYNC_DIRS[@]}"; do
    echo "Syncing ${dir}..."
    mkdir -p "${SYSROOT_DIR}${dir}"
    
    # sshpassでパスワードを渡し、rsyncを実行
    # --rsync-path="sudo rsync": RasPi側でシステムディレクトリを読むためにsudo権限でrsyncを実行させる
    sshpass -p "${RASPI_PW}" rsync -avz --delete --rsync-path="sudo rsync" \
        -e "ssh ${SSH_OPTS}" \
        "${RASPI_USER}@${RASPI_HOST}:${dir}" "${SYSROOT_DIR}${dir}"
done

echo "--- Sysroot Sync Complete. Location: ${SYSROOT_DIR} ---"