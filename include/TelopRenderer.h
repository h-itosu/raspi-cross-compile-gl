#pragma once

#include <string>
#include <map>
#include <ft2build.h>
#include FT_FREETYPE_H
#include <GLES2/gl2.h>
#include <chrono>

class TelopRenderer
{
public:
    TelopRenderer();
    ~TelopRenderer();

    void setupDefaultUniforms(); // 初期設定関数
    bool initialize(const char *fontPath);
    void update();
    void render();

    // ★ テキスト更新用メソッド
    void SetText(const std::string &text);
    // ★ フォントサイズを設定
    void SetFontSize(int size);
    // ★ 追加：アウトライン設定用メソッド
    void setOutline(bool enabled);
    // アウトライン色を設定（RGBA 各値 0.0〜1.0）
    void setOutlineColor(float r, float g, float b, float a);
    // アウトラインの太さを設定（ピクセル単位）
    void setOutlinePixelWidth(float px);

    void setMarginSize(float size);

private:
    struct Glyph
    {
        GLuint textureID;
        int width;
        int height;
        int bearingX;
        int bearingY;
        int advance;
    };

    FT_Library library_;
    FT_Face face_;

    GLuint telopProgram_;
    GLint attrPosition_;
    GLint attrTexCoord_;
    GLint uniformResolution_;
    GLint uniformTexture_;
    GLint uniformOutlineColor_;
    GLint uniformTextureSize_;
    GLint uniformEnableOutline_; // アウトライン有効化フラグ
    GLint uniformMarginSize_;    // マージン制御用 uniform
    GLint uniformTextColor_;
    GLint uniformOutlinePixelWidth_;

    GLuint vbo_;

    std::wstring text_;
    std::string currentText_;
    std::map<wchar_t, Glyph> glyphCache_;

    float scrollX_;
    std::chrono::steady_clock::time_point startTime_;
    int screenWidth_;
    int screenHeight_;

    // アウトライン描画設定
    float outlineColor_[4];   // RGBA (0.0〜1.0)
    float outlinePixelWidth_; // ピクセル単位
    float textureWidth_[4];
    bool outlineEnabled_ = true;
    float marginSize_ = 0.0f; // マージンのピクセルサイズ

    bool loadGlyph(wchar_t c);
    void renderGlyph(const Glyph &glyph, float x, float y, float alpha);

    void checkGLError(const char *label);
};
