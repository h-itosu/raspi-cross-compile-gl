#include "Renderer.h"
#include <iostream>
#include <vector>
#include <cstring>

Renderer::Renderer() {}
Renderer::~Renderer() { shutdown(); }

bool Renderer::initialize()
{
    const char *vShaderSrc =
        "attribute vec4 a_position;      \n"
        "attribute vec2 a_texCoord;      \n"
        "varying vec2 v_texCoord;        \n"
        "void main()                     \n"
        "{                               \n"
        "  gl_Position = a_position;     \n"
        "  v_texCoord = a_texCoord;      \n"
        "}";

    const char *fShaderSrc =
        "precision mediump float;                                \n"
        "varying vec2 v_texCoord;                                \n"
        "uniform sampler2D tex_y;                                \n"
        "uniform sampler2D tex_u;                                \n"
        "uniform sampler2D tex_v;                                \n"
        "void main()                                             \n"
        "{                                                       \n"
        "  float y = texture2D(tex_y, v_texCoord).r;             \n"
        "  float u = texture2D(tex_u, v_texCoord).r - 0.5;       \n"
        "  float v = texture2D(tex_v, v_texCoord).r - 0.5;       \n"
        "  float r = y + 1.402 * v;                              \n"
        "  float g = y - 0.344 * u - 0.714 * v;                  \n"
        "  float b = y + 1.772 * u;                              \n"
        "  gl_FragColor = vec4(r, g, b, 1.0);                    \n"
        "}";

    programYUV_ = createProgram(vShaderSrc, fShaderSrc);
    if (!programYUV_)
        return false;

    positionLoc_ = glGetAttribLocation(programYUV_, "a_position");
    texcoordLoc_ = glGetAttribLocation(programYUV_, "a_texCoord");
    samplerYLoc_ = glGetUniformLocation(programYUV_, "tex_y");
    samplerULoc_ = glGetUniformLocation(programYUV_, "tex_u");
    samplerVLoc_ = glGetUniformLocation(programYUV_, "tex_v");

    glGenTextures(1, &texY_);
    glGenTextures(1, &texU_);
    glGenTextures(1, &texV_);

    return true;
}

void Renderer::shutdown()
{
    if (texY_)
    {
        glDeleteTextures(1, &texY_);
        texY_ = 0;
    }
    if (texU_)
    {
        glDeleteTextures(1, &texU_);
        texU_ = 0;
    }
    if (texV_)
    {
        glDeleteTextures(1, &texV_);
        texV_ = 0;
    }
    if (programYUV_)
    {
        glDeleteProgram(programYUV_);
        programYUV_ = 0;
    }
}

void Renderer::uploadYUVTextures(const uint8_t *data, int width, int height, int ySize, int uSize)
{
    const uint8_t *yPlane = data;
    const uint8_t *uPlane = data + ySize;
    const uint8_t *vPlane = data + ySize + uSize;

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glBindTexture(GL_TEXTURE_2D, texY_);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, width, height,
                 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, yPlane);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glBindTexture(GL_TEXTURE_2D, texU_);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, width / 2, height / 2,
                 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, uPlane);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glBindTexture(GL_TEXTURE_2D, texV_);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, width / 2, height / 2,
                 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, vPlane);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
}

void Renderer::renderYUV(uint32_t width, uint32_t height)
{
    const GLfloat vertices[] = {
        // x, y,     u, v
        -1.0f, 1.0f, 0.0f, 0.0f,
        -1.0f, -1.0f, 0.0f, 1.0f,
        1.0f, -1.0f, 1.0f, 1.0f,
        1.0f, 1.0f, 1.0f, 0.0f};

    const GLushort indices[] = {0, 1, 2, 0, 2, 3};

    glViewport(0, 0, width, height);
    //    glClearColor(0, 0, 0, 1);
    //    glClear(GL_COLOR_BUFFER_BIT);

    glUseProgram(programYUV_);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texY_);
    glUniform1i(samplerYLoc_, 0);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, texU_);
    glUniform1i(samplerULoc_, 1);

    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, texV_);
    glUniform1i(samplerVLoc_, 2);

    glEnableVertexAttribArray(positionLoc_);
    glEnableVertexAttribArray(texcoordLoc_);
    glVertexAttribPointer(positionLoc_, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), vertices);
    glVertexAttribPointer(texcoordLoc_, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), vertices + 2);

    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, indices);
}

GLuint Renderer::loadShader(GLenum type, const char *shaderSrc)
{
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &shaderSrc, nullptr);
    glCompileShader(shader);

    GLint compiled;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
    if (!compiled)
    {
        char log[512];
        glGetShaderInfoLog(shader, sizeof(log), nullptr, log);
        std::cerr << "[Renderer] Shader compile error: " << log << std::endl;
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

GLuint Renderer::createProgram(const char *vShaderSrc, const char *fShaderSrc)
{
    GLuint vertexShader = loadShader(GL_VERTEX_SHADER, vShaderSrc);
    GLuint fragmentShader = loadShader(GL_FRAGMENT_SHADER, fShaderSrc);
    if (!vertexShader || !fragmentShader)
        return 0;

    GLuint program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);

    GLint linked;
    glGetProgramiv(program, GL_LINK_STATUS, &linked);
    if (!linked)
    {
        char log[512];
        glGetProgramInfoLog(program, sizeof(log), nullptr, log);
        std::cerr << "[Renderer] Program link error: " << log << std::endl;
        glDeleteProgram(program);
        return 0;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    return program;
}
