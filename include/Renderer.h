/**
 * @file Renderer.h
 * @brief OpenGL ESによるYUV描画処理を行うクラスの宣言
 */
#ifndef RENDERER_H
#define RENDERER_H

#include <GLES2/gl2.h>
#include <cstdint>

class Renderer
{
public:
    Renderer();
    ~Renderer();

    bool initialize();
    void shutdown();

    // YUVテクスチャアップロード
    void uploadYUVTextures(const uint8_t *data, int width, int height, int ySize, int uSize);

    // 描画
    void renderYUV(uint32_t width, uint32_t height);

private:
    GLuint programYUV_ = 0;
    GLuint texY_ = 0, texU_ = 0, texV_ = 0;
    GLuint positionLoc_ = 0;
    GLuint texcoordLoc_ = 0;
    GLuint samplerYLoc_ = 0;
    GLuint samplerULoc_ = 0;
    GLuint samplerVLoc_ = 0;

    GLuint createProgram(const char *vShaderSrc, const char *fShaderSrc);
    GLuint loadShader(GLenum type, const char *shaderSrc);
};

#endif // RENDERER_H
