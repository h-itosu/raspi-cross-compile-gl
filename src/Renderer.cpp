#include "Renderer.h"
#include "ShaderUtils.h"
#include <GLES2/gl2.h>
#include <EGL/egl.h>
#include <iostream>
#include <vector>
#include <cstring>

Renderer::Renderer()
    : fbo_(0), fboTexture_(0), fullScreenQuadVBO_(0),
      fboRenderProgram_(0), fboRenderTextureLoc_(-1),
      yuvProgram_(0), yTex_(0), uTex_(0), vTex_(0),
      fboWidth_(0), fboHeight_(0)
{
}

Renderer::~Renderer()
{
    shutdown();
}

bool Renderer::initialize(int width, int height)
{
    fboWidth_ = width;
    fboHeight_ = height;

    // FBO 初期化
    glGenFramebuffers(1, &fbo_);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo_);

    glGenTextures(1, &fboTexture_);
    glBindTexture(GL_TEXTURE_2D, fboTexture_);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, fboWidth_, fboHeight_, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fboTexture_, 0);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
        std::cerr << "FBO not complete!" << std::endl;
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        shutdown();
        return false;
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // フルスクリーン用VBO初期化
    const GLfloat quadVertices[] = {
        -1.0f, -1.0f,
        1.0f, -1.0f,
        -1.0f, 1.0f,
        1.0f, 1.0f};
    glGenBuffers(1, &fullScreenQuadVBO_);
    glBindBuffer(GL_ARRAY_BUFFER, fullScreenQuadVBO_);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);

    // FBOを画面に描画する用のシェーダ作成
    const char *vs = R"(
        attribute vec2 aPos;
        varying vec2 vTexCoord;
        void main() {
            vTexCoord = (aPos + 1.0) * 0.5;
            gl_Position = vec4(aPos, 0.0, 1.0);
        }
    )";

    const char *fs = R"(
        precision mediump float;
        varying vec2 vTexCoord;
        uniform sampler2D uTexture;
        void main() {
            gl_FragColor = texture2D(uTexture, vTexCoord);
        }
    )";

    fboRenderProgram_ = createProgram(vs, fs);
    if (!fboRenderProgram_)
    {
        std::cerr << "Failed to create FBO render shader program." << std::endl;
        shutdown();
        return false;
    }
    fboRenderTextureLoc_ = glGetUniformLocation(fboRenderProgram_, "uTexture");
    GLint posLoc = glGetAttribLocation(fboRenderProgram_, "aPos");
    if (posLoc < 0)
    {
        std::cerr << "Failed to get attribute location for aPos." << std::endl;
        shutdown();
        return false;
    }
    glUseProgram(fboRenderProgram_);
    glBindBuffer(GL_ARRAY_BUFFER, fullScreenQuadVBO_);
    glEnableVertexAttribArray(posLoc);
    glVertexAttribPointer(posLoc, 2, GL_FLOAT, GL_FALSE, 0, 0);

    // YUV用テクスチャとシェーダの初期化
    glGenTextures(1, &yTex_);
    glGenTextures(1, &uTex_);
    glGenTextures(1, &vTex_);

    const char *yuvVs = vs; // 同じ頂点シェーダを使う
    const char *yuvFs = R"(
        precision mediump float;
        varying vec2 vTexCoord;
        uniform sampler2D texY;
        uniform sampler2D texU;
        uniform sampler2D texV;
        void main() {
            float y = texture2D(texY, vTexCoord).r;
            float u = texture2D(texU, vTexCoord).r - 0.5;
            float v = texture2D(texV, vTexCoord).r - 0.5;
            float r = y + 1.402 * v;
            float g = y - 0.344 * u - 0.714 * v;
            float b = y + 1.772 * u;
            gl_FragColor = vec4(r, g, b, 1.0);
        }
    )";

    yuvProgram_ = createProgram(yuvVs, yuvFs);
    if (!yuvProgram_)
    {
        std::cerr << "Failed to create YUV shader program." << std::endl;
        shutdown();
        return false;
    }

    return true;
}

void Renderer::shutdown()
{
    if (fboTexture_)
    {
        glDeleteTextures(1, &fboTexture_);
        fboTexture_ = 0;
    }
    if (fbo_)
    {
        glDeleteFramebuffers(1, &fbo_);
        fbo_ = 0;
    }
    if (fullScreenQuadVBO_)
    {
        glDeleteBuffers(1, &fullScreenQuadVBO_);
        fullScreenQuadVBO_ = 0;
    }
    if (fboRenderProgram_)
    {
        glDeleteProgram(fboRenderProgram_);
        fboRenderProgram_ = 0;
    }
    if (yuvProgram_)
    {
        glDeleteProgram(yuvProgram_);
        yuvProgram_ = 0;
    }
    if (yTex_)
    {
        glDeleteTextures(1, &yTex_);
        yTex_ = 0;
    }
    if (uTex_)
    {
        glDeleteTextures(1, &uTex_);
        uTex_ = 0;
    }
    if (vTex_)
    {
        glDeleteTextures(1, &vTex_);
        vTex_ = 0;
    }
}

void Renderer::renderToFBO()
{
    glBindFramebuffer(GL_FRAMEBUFFER, fbo_);
    glViewport(0, 0, fboWidth_, fboHeight_);
}

void Renderer::endOffscreenRender()
{
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Renderer::renderFBOToScreen(int screenWidth, int screenHeight)
{
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, screenWidth, screenHeight);

    glUseProgram(fboRenderProgram_);
    glBindTexture(GL_TEXTURE_2D, fboTexture_);

    glBindBuffer(GL_ARRAY_BUFFER, fullScreenQuadVBO_);
    GLint posLoc = glGetAttribLocation(fboRenderProgram_, "aPos");
    glEnableVertexAttribArray(posLoc);
    glVertexAttribPointer(posLoc, 2, GL_FLOAT, GL_FALSE, 0, 0);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

bool Renderer::readPixelsFromFBO(std::vector<unsigned char> &outPixels, int width, int height)
{
    glBindFramebuffer(GL_FRAMEBUFFER, fbo_);
    outPixels.resize(width * height * 4);
    glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, outPixels.data());
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    return true;
}

void Renderer::uploadYUVTextures(const uint8_t *data, int width, int height, int ySize, int uSize)
{
    const uint8_t *yPlane = data;
    const uint8_t *uPlane = data + ySize;
    const uint8_t *vPlane = data + ySize + uSize;

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    glBindTexture(GL_TEXTURE_2D, yTex_);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, width, height, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, yPlane);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glBindTexture(GL_TEXTURE_2D, uTex_);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, width / 2, height / 2, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, uPlane);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glBindTexture(GL_TEXTURE_2D, vTex_);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, width / 2, height / 2, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, vPlane);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
}

void Renderer::renderYUV(int screenWidth, int screenHeight)
{
    glViewport(0, 0, screenWidth, screenHeight);
    glUseProgram(yuvProgram_);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, yTex_);
    glUniform1i(glGetUniformLocation(yuvProgram_, "texY"), 0);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, uTex_);
    glUniform1i(glGetUniformLocation(yuvProgram_, "texU"), 1);

    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, vTex_);
    glUniform1i(glGetUniformLocation(yuvProgram_, "texV"), 2);

    glBindBuffer(GL_ARRAY_BUFFER, fullScreenQuadVBO_);
    GLint posLoc = glGetAttribLocation(yuvProgram_, "aPos");
    glEnableVertexAttribArray(posLoc);
    glVertexAttribPointer(posLoc, 2, GL_FLOAT, GL_FALSE, 0, 0);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}
