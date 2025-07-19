# Raspberry Pi C++ クロスコンパイル & OpenGL ES プロジェクト 🚀

## 概要

このプロジェクトは、**Windows 11**上の**VS Code**を使用して、**Raspberry Pi**向けのハードウェアアクセラレーションが有効な**C++/OpenGL ES**ネイティブアプリケーションを開発するためのテンプレートです。

開発環境は**WSL2**と**Docker**コンテナによって完全に分離されており、クリーンで再現性の高いビルドが可能です。

---

## ✨ 主な特徴

* **クロスコンパイル:** Windows (x86_64) から Raspberry Pi (aarch64) 向けの実行ファイルをビルドします。
* **コンテナ化された開発環境:** VS Code Dev Containers を使用し、必要なツールやライブラリがすべて含まれたDockerコンテナ内で開発を行います。
* **Sysroot同期:** ターゲットのRaspberry Piから直接ライブラリを同期し、OSバージョンやハードウェアの差異を吸収します。
* **画面直接描画:** デスクトップ環境（X11）を必要としない、**DRM/KMS/GBM**スタックを使用した低レベルな画面描画を行います。
* **リモートデバッグ:** VS CodeからRaspberry Pi上で実行されているプログラムにアタッチし、ブレークポイントやステップ実行が可能です。
* **自動化ワークフロー:** `Makefile`によって、ビルド、デプロイ、実行、デバッグ準備などの一般的なタスクが自動化されています。
* **環境の分離:** `.env`ファイルによって、個人の環境設定（IPアドレス、パスワード等）をプロジェクト本体から分離できます。

---

## 🔧 開発環境のセットアップ

### 1. ホストPCの要件

* Windows 11
* [WSL2](https://learn.microsoft.com/ja-jp/windows/wsl/install) (DebianやUbuntuなどのディストリビューション)
* [Docker Desktop](https://www.docker.com/products/docker-desktop/) (WSL2 Integrationが有効になっていること)
* [Visual Studio Code](https://code.visualstudio.com/)
    * 必須の拡張機能: **WSL**, **Dev Containers**

### 2. ターゲット (Raspberry Pi) の準備

1.  **OSのインストール:**
    * **Raspberry Pi OS Lite (64-bit)** の使用を強く推奨します。
    * Raspberry Pi Imagerを使い、SSHを有効にしてSDカードを作成します。

2.  **必須ライブラリのインストール:**
    * 新しいOSで起動したRaspberry PiにSSHでログインし、以下のコマンドを実行します。
        ```bash
        sudo apt-get update
        sudo apt-get install -y rsync libgles2-mesa-dev libegl1-mesa-dev libdrm-dev mesa-utils
        ```

### 3. プロジェクトのセットアップ

1.  **リポジトリをクローン:**
    ```bash
    git clone <リポジトリのURL>
    cd <プロジェクトフォルダ>
    ```

2.  **環境設定ファイルを作成:**
    * プロジェクトのルートにある`.env.example`をコピーして`.env`という名前のファイルを作成します。
    * `.env`ファイルを開き、あなたのRaspberry Piの接続情報（`RASPI_HOST`, `RASPI_USER`, `RASPI_PW`）を正しく編集します。

3.  **VS Codeで開く:**
    * プロジェクトフォルダをVS Codeで開きます。
    * 右下に表示される「**Reopen in Container**」（コンテナーで再度開く）ボタンをクリックします。
    * 初回はDockerイメージのビルドとSysrootの同期が実行されるため、数分かかります。

---

## ⚙️ 日々の運用方法 (Makefile)

開発作業は、コンテナ内のVS Codeターミナルで`make`コマンドを実行することで行います。

* **ビルド:**
    プロジェクトをコンパイルします。
    ```bash
    make
    # または make build
    ```

* **実行:**
    ビルド、Raspberry Piへの転送、そしてリモートでの実行を一度に行います。
    ```bash
    make run
    ```

* **デプロイ:**
    ビルドして、実行ファイルのみをRaspberry Piに転送します。
    ```bash
    make deploy
    ```

* **リモートデバッグ:**
    1.  以下のコマンドを実行して、Raspberry Pi上でデバッグサーバーを起動し、待機状態にします。
        ```bash
        make debug-server
        ```
    2.  VS Codeの「実行とデバッグ」ビューを開き (`Ctrl+Shift+D`)、`F5`キーを押して「**Remote Debug Raspberry Pi**」を開始します。

* **クリーン:**
    ビルド成果物（`build`ディレクトリ）を削除します。
    ```bash
    make clean
    ```

---

## 📂 プロジェクト構成

```
.
├── .devcontainer/         # Dev Container設定
│   ├── devcontainer.json  # VS Codeのコンテナ統合設定
│   ├── Dockerfile         # 開発環境の設計図
│   └── sync_sysroot.sh    # RasPiからライブラリを同期するスクリプト
├── .vscode/               # VS Codeのプロジェクト設定
│   ├── extensions.json    # 推奨拡張機能
│   ├── launch.json        # デバッグ構成
│   └── settings.json      # ワークスペース設定
├── include/               # C++ヘッダーファイル (.h, .hpp)
├── src/                   # C++ソースファイル (.cpp)
├── build/                 # ビルド成果物 (Git管理外)
├── .env                   # 個人環境設定 (Git管理外)
├── .env.example           # .envファイルのテンプレート
├── .gitignore             # Gitの無視ファイル設定
├── CMakeLists.txt         # CMakeのビルド定義ファイル
├── Makefile               # 開発タスク自動化ファイル
└── README.md              # このファイル
```
