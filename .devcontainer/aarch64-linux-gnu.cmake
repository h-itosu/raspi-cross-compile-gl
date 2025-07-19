# .devcontainer/aarch64-linux-gnu.cmake

# ターゲットシステムの定義
set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR aarch64)

# Sysrootのパスを指定
set(CMAKE_SYSROOT /opt/raspi_sysroot)

# クロスコンパイラの指定
set(CMAKE_C_COMPILER aarch64-linux-gnu-gcc)
set(CMAKE_CXX_COMPILER aarch64-linux-gnu-g++)

# 検索パスの設定
# ライブラリやヘッダはSysroot内のみを検索し、ホスト(コンテナ)のものは検索しない
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)