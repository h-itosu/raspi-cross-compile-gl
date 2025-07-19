# =============================================================================
# Dockerfile for Raspberry Pi C++ Cross-Compilation
# =============================================================================
#
# ターゲットOS: Debian 12 (bookworm) for Raspberry Pi (aarch64)
# 開発環境:   Debian 12 (bookworm) on Docker (amd64)

# --- 1. ベースイメージ ---
# ターゲットであるRaspberry PiのOSバージョンに合わせることで、互換性の問題を最小限に抑える
FROM debian:bookworm

# --- 2. 環境変数 ---
# パッケージインストール(apt-get)実行時に、対話的なプロンプトを無効化する
ENV DEBIAN_FRONTEND=noninteractive

# --- 3. パッケージのインストール ---
# 必要なツールやライブラリを一度にインストールする
RUN apt-get update && apt-get install -y \
    \
    # [ビルドツール]
    # C++/CMakeプロジェクトをビルドするための基本的なツールセット
    build-essential \
    cmake \
    ninja-build \
    \
    # [クロスコンパイル用ツール]
    # aarch64 (ARM 64bit)アーキテクチャ向けのC++コンパイラ
    g++-aarch64-linux-gnu \
    # 複数のアーキテクチャに対応したデバッガ
    gdb-multiarch \
    \
    # [開発支援ツール]
    # Sysroot同期やリモート操作に使用
    rsync \
    sshpass \
    # ソースコードのバージョン管理
    git \
    # グラフィックスAPIの状態を確認するユーティリティ (eglinfoなど)
    mesa-utils \
    \
    # [依存ライブラリ (開発用)]
    # 低レベルグラフィックスAPI (DRM/KMS) のヘッダとライブラリ
    libdrm-dev \
    \
    # インストール後にaptキャッシュを削除し、Dockerイメージのサイズを削減
    && rm -rf /var/lib/apt/lists/*

# --- 4. デフォルトコマンド ---
# `docker run` や `docker exec` でコマンド指定がない場合に実行される
CMD [ "/bin/bash" ]
