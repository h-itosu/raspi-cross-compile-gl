#
# Makefile: 開発ワークフローの自動化
#
# 使い方:
#   make           - プロジェクトをビルド
#   make deploy    - ビルドしてRaspberry Piに転送
#   make run       - ビルド、転送、実行
#   make debug-server - デバッグサーバーを起動
#   make clean     - ビルド成果物を削除
#

# --- .envファイルの読み込み ---
# .envファイルが存在すれば、中の変数を読み込む
# このMakefile内のデフォルト値よりも優先される
-include .env
# 読み込んだ変数をシェルコマンド(sshpassなど)で使えるようにする
export


# --- 設定変数 (.envファイルで上書き可能) ---
# ?= 演算子は、変数が未定義の場合にのみデフォルト値を設定する
RASPI_USER    ?= pi
RASPI_HOST    ?= raspberrypi.local
RASPI_PW      ?= raspberry
TARGET_EXEC   ?= raspi_gl_hello
RASPI_DEBUG_PORT ?= 5000


# --- 固定変数 ---
BUILD_DIR     := build
EXECUTABLE    := $(BUILD_DIR)/$(TARGET_EXEC)
REMOTE_DIR    := /home/$(RASPI_USER)


# --- 色付け用の変数 ---
GREEN         := \033[0;32m
NC            := \033[0m # No Color


# --- コマンド定義 ---
SSHPASS_CMD   := sshpass -p '$(RASPI_PW)'
SCP_CMD       := $(SSHPASS_CMD) scp -o StrictHostKeyChecking=no
SSH_CMD       := $(SSHPASS_CMD) ssh -o StrictHostKeyChecking=no $(RASPI_USER)@$(RASPI_HOST)
CMAKE_CONFIGURE_CMD := cmake -B $(BUILD_DIR) -S . -G Ninja \
    -DCMAKE_TOOLCHAIN_FILE=.devcontainer/aarch64-linux-gnu.cmake \
    -DTARGET_EXEC=$(TARGET_EXEC)

# --- ターゲット定義 ---
# .PHONY: これらはファイル名ではなく、命令の別名(エイリアス)であることを示す
.PHONY: all build deploy run debug-server clean

# デフォルトターゲット: `make`とだけ打った時に実行される
all: build

# プロジェクトの構成とビルド
build:
	@echo "--- Configuring and Building Project ---"
	@$(CMAKE_CONFIGURE_CMD)
	@cmake --build $(BUILD_DIR)
	@echo -e "$(GREEN)--- Build Successful! ---$(NC)"

# Raspberry Piへのデプロイ
deploy: build
	@echo "--- Deploying to $(RASPI_HOST) ---"
	@$(SCP_CMD) $(EXECUTABLE) $(RASPI_USER)@$(RASPI_HOST):$(REMOTE_DIR)/
	@echo -e "$(GREEN)--- Deploy Successful! ---$(NC)"

# Raspberry Pi上での実行
run: deploy
	@echo "--- Running on $(RASPI_HOST) ---"
	@$(SSH_CMD) '$(REMOTE_DIR)/$(TARGET_EXEC)'

# Raspberry Pi上でデバッグサーバーを起動
debug-server: deploy
	@echo "--- Starting GDB Server on $(RASPI_HOST) ---"
	@echo "VS Codeのデバッガを起動してください (F5)..."
	@$(SSH_CMD) 'gdbserver :$(RASPI_DEBUG_PORT) $(REMOTE_DIR)/$(TARGET_EXEC)'

# ビルド成果物のクリーンアップ
clean:
	@echo "--- Cleaning build directory ---"
	@rm -rf $(BUILD_DIR)
