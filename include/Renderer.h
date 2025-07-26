#pragma once

#include <GLES2/gl2.h>
#include <vector>
#include <cstdint>

class Renderer
{
public:
    Renderer();
    ~Renderer();

    bool initialize(int width, int height);
    void shutdown();

    void uploadYUVTextures(const uint8_t *data, int width, int height, int ySize, int uSize);
    void renderYUV(int screenWidth, int screenHeight);

    void renderToFBO();                                                                   // FBO に描画開始
    void endOffscreenRender();                                                            // FBO描画終了
    void renderFBOToScreen(int screenWidth, int screenHeight);                            // FBOを画面に描画
    bool readPixelsFromFBO(std::vector<unsigned char> &outPixels, int width, int height); // PNG保存用

private:
    int fboWidth_ = 0;
    int fboHeight_ = 0;

    GLuint yTex_ = 0;
    GLuint uTex_ = 0;
    GLuint vTex_ = 0;
    GLuint yuvProgram_ = 0;

    GLuint fbo_ = 0;
    GLuint fboTexture_ = 0;
    GLuint fboRenderProgram_ = 0;
    GLuint fboRenderTextureLoc_ = 0;

    GLuint fullScreenQuadVBO_ = 0;
};
