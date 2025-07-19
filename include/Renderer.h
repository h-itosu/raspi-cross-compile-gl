/**
 * @file Renderer.h
 * @brief OpenGL ESによる描画処理を行うクラスの宣言
 */
#ifndef RENDERER_H
#define RENDERER_H

#include <GLES2/gl2.h>
#include <cstdint>

/**
 * @class Renderer
 * @brief シェーダー管理、オブジェクトの描画など、OpenGL ES APIを直接呼び出す処理を担当する。
 */
class Renderer {
public:
    /** @brief コンストラクタ */
    Renderer();
    /** @brief デストラクタ */
    ~Renderer();

    /**
     * @brief シェーダーのコンパイルなど、描画に必要なリソースを初期化する。
     * @return 初期化に成功した場合はtrue、失敗した場合はfalse。
     */
    bool initialize();

    /**
     * @brief 確保したOpenGLリソースを解放する。
     */
    void shutdown();

    /**
     * @brief 1フレーム分の描画処理を行う。
     * @param angle 現在の回転角度。
     * @param width 描画領域の幅。
     * @param height 描画領域の高さ。
     */
    void render(float angle, uint32_t width, uint32_t height);

private:
    /// @brief シェーダープログラムのID
    GLuint program_ = 0;
    /// @brief 頂点位置属性のロケーション
    GLuint position_loc_ = 0;
    /// @brief MVP行列uniform変数のロケーション
    GLuint matrix_loc_ = 0;

    /**
     * @brief シェーダーをコンパイルし、リンクして、シェーダープログラムを作成する。
     * @param vShaderSrc 頂点シェーダーのソースコード。
     * @param fShaderSrc フラグメントシェーダーのソースコード。
     * @return 作成されたシェーダープログラムのID。失敗した場合は0。
     */
    GLuint createProgram(const char* vShaderSrc, const char* fShaderSrc);
    
    /**
     * @brief 単一のシェーダーをコンパイルする。
     * @param type シェーダーの種別 (GL_VERTEX_SHADER or GL_FRAGMENT_SHADER)。
     * @param shaderSrc シェーダーのソースコード。
     * @return コンパイルされたシェーダーのID。失敗した場合は0。
     */
    GLuint loadShader(GLenum type, const char* shaderSrc);
};

#endif // RENDERER_H