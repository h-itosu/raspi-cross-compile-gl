# -----------------------------------------------------------------------------
# CMakeLists.txt: C++プロジェクトのビルド定義
# -----------------------------------------------------------------------------

# --- プロジェクトの基本設定 ---
cmake_minimum_required(VERSION 3.10)
project(RaspiGLExample CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- プロジェクト内インクルードパス ---
# `include`フォルダをヘッダーファイルの検索パスに追加する
include_directories(include)

# --- 依存ライブラリ検索 ---
# クロスコンパイル用のSysroot内から、必要なライブラリとヘッダを探す。
# HINTS: 検索を効率化・確実化するためのヒントパス
find_path(GLES2_INCLUDE_DIR NAMES GLES2/gl2.h)
find_path(EGL_INCLUDE_DIR NAMES EGL/egl.h)
find_path(DRM_INCLUDE_DIR NAMES drm.h HINTS /usr/include/libdrm /usr/include)
find_path(FRTP_INCLUDE_DIR NAMES ft2build.h HINTS /opt/raspi_sysroot/usr/include/freetype2)

find_library(GLESV2_LIBRARY NAMES GLESv2 HINTS /usr/lib/aarch64-linux-gnu)
find_library(EGL_LIBRARY NAMES EGL HINTS /usr/lib/aarch64-linux-gnu)
find_library(DRM_LIBRARY NAMES drm HINTS /usr/lib/aarch64-linux-gnu)
find_library(GBM_LIBRARY NAMES gbm HINTS /usr/lib/aarch64-linux-gnu)

# --- GStreamerとGLibのヘッダ検索 ---
find_path(GSTREAMER_INCLUDE_DIR gst/gst.h
    PATHS /opt/raspi_sysroot/usr/include/gstreamer-1.0
)
find_path(GLIB_INCLUDE_DIR glib.h
    PATHS /opt/raspi_sysroot/usr/include/glib-2.0
)
find_path(GLIBCONFIG_INCLUDE_DIR glibconfig.h
    PATHS /opt/raspi_sysroot/usr/lib/aarch64-linux-gnu/glib-2.0/include
)
# --- GStreamer libraries ---
find_library(GSTREAMER_LIBRARY gstreamer-1.0
    PATHS /opt/raspi_sysroot/usr/lib/aarch64-linux-gnu
)
find_library(GST_BASE_LIBRARY gstbase-1.0
    PATHS /opt/raspi_sysroot/usr/lib/aarch64-linux-gnu
)

find_library(GOBJECT_LIB
    NAMES gobject-2.0
    PATHS ${CMAKE_SYSROOT}/usr/lib/aarch64-linux-gnu
    NO_DEFAULT_PATH
)
find_library(GLIB_LIB
    NAMES glib-2.0
    PATHS ${CMAKE_SYSROOT}/usr/lib/aarch64-linux-gnu
    NO_DEFAULT_PATH
)

find_library(GST_APP_LIBRARY gstapp-1.0
    PATHS /opt/raspi_sysroot/usr/lib/aarch64-linux-gnu
)

find_library(FRTP_LIBRARY
    NAMES libfreetype.so
    PATHS /opt/raspi_sysroot/usr/lib/aarch64-linux-gnu
)

# 検出結果をログに表示
message(STATUS "Found GLib: ${GLIB_LIB}")
message(STATUS "Found GObject: ${GOBJECT_LIB}")

# 見つけたヘッダパスをコンパイラに教える
include_directories(
    include
    src
    ${GLES2_INCLUDE_DIR}
    ${EGL_INCLUDE_DIR}
    ${DRM_INCLUDE_DIR}
    ${FRTP_INCLUDE_DIR}
    ${GSTREAMER_INCLUDE_DIR}
    ${GLIB_INCLUDE_DIR}
    ${GLIBCONFIG_INCLUDE_DIR}
)


# 見つけたライブラリをリンカで使うためにリスト化する
set(REQUIRED_LIBRARIES
    ${GLESV2_LIBRARY}
    ${EGL_LIBRARY}
    ${DRM_LIBRARY}
    ${GBM_LIBRARY}
    ${GSTREAMER_LIBRARY}
    ${GST_BASE_LIBRARY}
    ${GST_APP_LIBRARY}
    ${GOBJECT_LIB}
    ${GLIB_LIB}
    ${FRTP_LIBRARY}
    m # 数学ライブラリ
)

# 必須ライブラリが見つからなかった場合はエラーで停止
if(NOT DRM_INCLUDE_DIR OR NOT DRM_LIBRARY OR NOT GBM_LIBRARY)
    message(FATAL_ERROR "DRM or GBM library/headers not found.")
endif()

# デフォルト値 "raspi_gl_hello" を設定可能に
set(TARGET_EXEC "raspi_gl_hello" CACHE STRING "Executable name")

# --- 実行ファイルの定義 ---
# 実行ファイル名と、それに必要なソースファイル群を指定する
add_executable(${TARGET_EXEC}
    src/main.cpp
    src/Application.cpp
    src/GraphicsPlatform.cpp
    src/Renderer.cpp
    src/GStreamerSupport.cpp
    src/TelopRenderer.cpp
    src/ShaderUtils.cpp
    src/Util.cpp
)

# 実行ファイルに必要なライブラリをリンクする
target_link_libraries(${TARGET_EXEC} PRIVATE ${REQUIRED_LIBRARIES})
