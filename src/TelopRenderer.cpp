// TelopRenderer.cpp
#include "TelopRenderer.h"
#include "ShaderUtils.h"
#include <GLES2/gl2.h>
#include <iostream>
#include <locale>
#include <codecvt>
#include <fstream>
#include <sstream>
#include <vector>

TelopRenderer::TelopRenderer()
    : library_(nullptr), face_(nullptr), telopProgram_(0), attrPosition_(-1), attrTexCoord_(-1),
      uniformResolution_(-1), uniformTexture_(-1), uniformOutlineColor_(-1), uniformOutlineWidth_(-1), uniformTextureSize_(-1),
      vbo_(0), scrollX_(0.0f), startTime_(std::chrono::steady_clock::now()), screenWidth_(1280), screenHeight_(720),
      outlineEnabled_(false), outlineWidth_(1.0f)
{
    outlineColor_[0] = 0.0f;
    outlineColor_[1] = 0.0f;
    outlineColor_[2] = 0.0f;
    outlineColor_[3] = 1.0f;
}

TelopRenderer::~TelopRenderer()
{
    for (auto &entry : glyphCache_)
    {
        glDeleteTextures(1, &entry.second.textureID);
    }
    glyphCache_.clear();

    if (face_)
        FT_Done_Face(face_);
    if (library_)
        FT_Done_FreeType(library_);
    if (vbo_)
        glDeleteBuffers(1, &vbo_);
}

bool TelopRenderer::initialize(const char *fontPath)
{
    if (FT_Init_FreeType(&library_) != 0)
    {
        std::cerr << "Failed to initialize FreeType library." << std::endl;
        return false;
    }

    if (FT_New_Face(library_, fontPath, 0, &face_) != 0)
    {
        std::cerr << "Failed to load font: " << fontPath << std::endl;
        return false;
    }
    std::cout << "Font loaded: " << fontPath << std::endl;

    FT_Set_Pixel_Sizes(face_, 0, 64);

    const char *vertexPath = "/home/h.itosu/shaders/telop.vert";
    const char *fragmentPath = "/home/h.itosu/shaders/telop.frag";
    telopProgram_ = createProgramFromFiles(vertexPath, fragmentPath);
    if (!telopProgram_)
    {
        std::cerr << "Failed to create shader program from " << vertexPath << " and " << fragmentPath << std::endl;
        return false;
    }

    attrPosition_ = glGetAttribLocation(telopProgram_, "a_position");
    attrTexCoord_ = glGetAttribLocation(telopProgram_, "a_texcoord");
    uniformResolution_ = glGetUniformLocation(telopProgram_, "u_resolution");
    uniformTexture_ = glGetUniformLocation(telopProgram_, "u_texture");
    uniformOutlineColor_ = glGetUniformLocation(telopProgram_, "u_outlineColor");
    uniformOutlineWidth_ = glGetUniformLocation(telopProgram_, "u_outlineWidth");
    uniformTextureSize_ = glGetUniformLocation(telopProgram_, "u_textureSize");

    glGenBuffers(1, &vbo_);

    updateText("こんにちは、世界！テロップのテスト中です・・・・・いかがでしょうか？〇(^^♪〇");

    return true;
}

void TelopRenderer::setOutline(bool enabled)
{
    outlineEnabled_ = enabled;
}

void TelopRenderer::setOutlineColor(float r, float g, float b, float a)
{
    outlineColor_[0] = r;
    outlineColor_[1] = g;
    outlineColor_[2] = b;
    outlineColor_[3] = a;
}

void TelopRenderer::setOutlineWidth(float width)
{
    outlineWidth_ = width;
}

void TelopRenderer::updateText(const std::string &text)
{
    currentText_ = text;
    std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
    text_ = converter.from_bytes(text);

    for (wchar_t c : text_)
    {
        loadGlyph(c);
    }
}

void TelopRenderer::update()
{
    auto now = std::chrono::steady_clock::now();
    float elapsed = std::chrono::duration<float>(now - startTime_).count();
    float totalWidth = 0.0f;
    for (wchar_t c : text_)
    {
        auto it = glyphCache_.find(c);
        if (it != glyphCache_.end())
            totalWidth += it->second.advance;
    }

    scrollX_ = screenWidth_ - elapsed * 100.0f;
    if (scrollX_ < -totalWidth)
    {
        startTime_ = now;
        scrollX_ = screenWidth_;
    }
}

void TelopRenderer::render()
{
    float x = scrollX_;
    float y = screenHeight_ - 64.0f;

    for (wchar_t c : text_)
    {
        auto it = glyphCache_.find(c);
        if (it != glyphCache_.end())
        {
            renderGlyph(it->second, x, y, 1.0f);
            x += it->second.advance;
        }
    }

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glUseProgram(0);
}

bool TelopRenderer::loadGlyph(wchar_t c)
{
    if (glyphCache_.count(c))
        return true;

    if (FT_Load_Char(face_, c, FT_LOAD_RENDER))
        return false;

    // ★★★★★ デバッグコードを追加 ★★★★★
    std::cout << "Loading char '" << (char)c << "'"
              << " -> Bitmap Size: "
              << face_->glyph->bitmap.width << "x" << face_->glyph->bitmap.rows
              << std::endl;
    // ★★★★★ここまで★★★★★

    FT_GlyphSlot g = face_->glyph;
    int originalWidth = g->bitmap.width;
    int originalHeight = g->bitmap.rows;

    // アウトライン幅の分だけパディングを追加する
    int padding = static_cast<int>(outlineWidth_);
    int paddingX = padding;
    int paddingY = padding;

    int expandedWidth = originalWidth + paddingX * 2;
    int expandedHeight = originalHeight + paddingY * 2;

    // 拡張バッファの作成（α値のみ）
    std::vector<unsigned char> expandedBuffer(expandedWidth * expandedHeight, 0);

    // 中央にビットマップを配置（行単位でコピー）
    for (int row = 0; row < originalHeight; ++row)
    {
        memcpy(&expandedBuffer[(row + paddingY) * expandedWidth + paddingX],
               &g->bitmap.buffer[row * originalWidth],
               originalWidth);
    }

    // OpenGL テクスチャ生成
    GLuint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, expandedWidth, expandedHeight, 0,
                 GL_ALPHA, GL_UNSIGNED_BYTE, expandedBuffer.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // Glyph をキャッシュ（描画オフセットも調整）
    Glyph glyph = {
        tex,
        expandedWidth,
        expandedHeight,
        g->bitmap_left - paddingX,
        g->bitmap_top + paddingY,
        static_cast<int>(g->advance.x >> 6)};

    glyphCache_[c] = glyph;
    return true;
}

void TelopRenderer::renderGlyph(const Glyph &glyph, float x, float y, float alpha)
{
    float xpos = x + glyph.bearingX;
    float ypos = y - glyph.bearingY;
    float w = glyph.width;
    float h = glyph.height;

    float vertices[24] = {
        xpos,
        ypos + h,
        0.0f,
        0.0f,
        xpos,
        ypos,
        0.0f,
        1.0f,
        xpos + w,
        ypos,
        1.0f,
        1.0f,

        xpos,
        ypos + h,
        0.0f,
        0.0f,
        xpos + w,
        ypos,
        1.0f,
        1.0f,
        xpos + w,
        ypos + h,
        1.0f,
        0.0f,
    };

    glUseProgram(telopProgram_);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW);

    glEnableVertexAttribArray(attrPosition_);
    glVertexAttribPointer(attrPosition_, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 4, (void *)0);

    glEnableVertexAttribArray(attrTexCoord_);
    glVertexAttribPointer(attrTexCoord_, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 4, (void *)(sizeof(float) * 2));

    glUniform2f(uniformResolution_, (float)screenWidth_, (float)screenHeight_);
    glUniform4fv(uniformOutlineColor_, 1, outlineColor_);
    glUniform1f(uniformOutlineWidth_, outlineEnabled_ ? outlineWidth_ : 0.0f);
    glUniform2f(uniformTextureSize_, (float)glyph.width, (float)glyph.height);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, glyph.textureID);
    glUniform1i(uniformTexture_, 0);

    glDrawArrays(GL_TRIANGLES, 0, 6);

    glDisableVertexAttribArray(attrTexCoord_);
    glDisableVertexAttribArray(attrPosition_);
}

void TelopRenderer::checkGLError(const char *label)
{
    GLenum err;
    while ((err = glGetError()) != GL_NO_ERROR)
    {
        std::cerr << "[OpenGL Error] " << label << ": 0x" << std::hex << err << std::dec << std::endl;
    }
}
