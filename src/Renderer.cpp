/**
 * @file Renderer.cpp
 * @brief OpenGL ESによる描画処理を行うクラスの実装
 */
#include "Renderer.h"
#include <iostream>
#include <vector>
#include <cmath> // M_PI, cosf, sinf のために必要

Renderer::Renderer() {}

Renderer::~Renderer()
{
    shutdown();
}

bool Renderer::initialize()
{
    // 頂点シェーダーのソースコード
    // a_position: 頂点座標 (入力)
    // u_mvpMatrix: モデル・ビュー・プロジェクション行列 (外部から設定)
    // gl_Position: 最終的な頂点のスクリーン上の位置 (出力)
    const char vShaderStr[] =
        "attribute vec4 a_position;   \n"
        "uniform mat4 u_mvpMatrix;    \n"
        "void main()                  \n"
        "{                            \n"
        "   gl_Position = u_mvpMatrix * a_position; \n"
        "}                            \n";

    // フラグメントシェーダーのソースコード
    // gl_FragColor: ピクセルの最終的な色 (出力)
    const char fShaderStr[] =
        "void main()                               \n"
        "{                                         \n"
        "  gl_FragColor = vec4(1.0, 0.8, 0.0, 1.0);\n" // オレンジ色
        "}                                         \n";

    // シェーダーをコンパイルし、リンクしてプログラムを作成
    program_ = createProgram(vShaderStr, fShaderStr);
    if (program_ == 0)
        return false;

    // シェーダー内の変数の場所(ロケーション)を取得しておく
    position_loc_ = glGetAttribLocation(program_, "a_position");
    matrix_loc_ = glGetUniformLocation(program_, "u_mvpMatrix");

    return true;
}

void Renderer::shutdown()
{
    // 作成したシェーダープログラムを解放
    if (program_ != 0)
    {
        glDeleteProgram(program_);
        program_ = 0;
    }
}

void Renderer::render(float angle, uint32_t width, uint32_t height)
{
    // 描画する三角形の頂点データ (x, y, z)
    GLfloat vVertices[] = {
        0.0f, 0.5f, 0.0f,   // 上
        -0.5f, -0.5f, 0.0f, // 左下
        0.5f, -0.5f, 0.0f   // 右下
    };

    // --- 回転行列の計算 ---
    float aspect = (float)width / (float)height;
    float radians = angle * (M_PI / 180.0f);
    float cos_a = cosf(radians);
    float sin_a = sinf(radians);
    // Z軸回転行列を作成
    GLfloat matrix[16] = {
        cos_a, sin_a, 0.0f, 0.0f,
        -sin_a, cos_a, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f};
    // 画面のアスペクト比に合わせて行列を補正 (縦長/横長でも歪まないようにする)
    if (aspect > 1.0f)
    { // 横長の画面
        matrix[0] /= aspect;
        matrix[1] /= aspect;
    }
    else
    { // 縦長の画面
        matrix[4] *= aspect;
        matrix[5] *= aspect;
    }
    // --- 行列計算ここまで ---

    // 1. 描画領域(ビューポート)を画面全体に設定
    glViewport(0, 0, width, height);
    // 2. 画面をクリアする色を設定 (濃い青色)
    glClearColor(0.1f, 0.2f, 0.3f, 1.0f);
    // 3. 実際に画面をクリア
    glClear(GL_COLOR_BUFFER_BIT);

    // 4. 使用するシェーダープログラムを指定
    glUseProgram(program_);

    // 5. 頂点データをシェーダーの a_position 属性に結びつける
    glVertexAttribPointer(position_loc_, 3, GL_FLOAT, GL_FALSE, 0, vVertices);
    glEnableVertexAttribArray(position_loc_);

    // 6. 計算した回転行列をシェーダーの u_mvpMatrix uniform変数に送る
    glUniformMatrix4fv(matrix_loc_, 1, GL_FALSE, matrix);

    // 7. 描画コマンドを実行 (3頂点で1つの三角形を描画)
    glDrawArrays(GL_TRIANGLES, 0, 3);
}

// (以下、プライベートなヘルパー関数の実装)
GLuint Renderer::loadShader(GLenum type, const char *shaderSrc)
{
    GLuint shader = glCreateShader(type);
    if (shader == 0)
        return 0;
    glShaderSource(shader, 1, &shaderSrc, NULL);
    glCompileShader(shader);
    GLint compiled;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
    if (!compiled)
    {
        GLint infoLen = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLen);
        if (infoLen > 1)
        {
            std::vector<char> infoLog(infoLen);
            glGetShaderInfoLog(shader, infoLen, NULL, infoLog.data());
            std::cerr << "Error compiling shader:\n"
                      << infoLog.data() << std::endl;
        }
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

GLuint Renderer::createProgram(const char *vShaderSrc, const char *fShaderSrc)
{
    GLuint vertexShader = loadShader(GL_VERTEX_SHADER, vShaderSrc);
    GLuint fragmentShader = loadShader(GL_FRAGMENT_SHADER, fShaderSrc);
    if (vertexShader == 0 || fragmentShader == 0)
        return 0;
    GLuint program = glCreateProgram();
    if (program == 0)
        return 0;
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);
    GLint linked;
    glGetProgramiv(program, GL_LINK_STATUS, &linked);
    if (!linked)
    { /* ... エラーログ ... */
        glDeleteProgram(program);
        return 0;
    }
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    return program;
}
