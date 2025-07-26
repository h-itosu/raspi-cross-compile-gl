// 中精度の浮動小数点演算（組込みGPUに適した設定）
precision mediump float;

// 描画対象のテクスチャ（glyphや文字のα画像）
uniform sampler2D u_texture;

// テクスチャサイズ（ピクセル単位）
uniform vec2 u_textureSize;

// テキスト本体とアウトラインの色
uniform vec4 u_textColor;
uniform vec4 u_outlineColor;

// アウトラインの太さ（ピクセル単位指定）
uniform float u_outlinePixelWidth;

// アウトラインが有効化？
uniform int u_enableOutline;

// 周囲の余白（未使用ですが将来的に透明領域確保などで活用可能）
uniform float u_marginPixelSize;

// 頂点シェーダから受け取るUV座標（0.0〜1.0範囲）
varying vec2 v_texCoord;

void main() {

    // 中心ピクセルのアルファ値を取得（本体かどうか判定）
    float centerAlpha = texture2D(u_texture, v_texCoord).a;

    if( u_enableOutline == 0)
    {
        gl_FragColor = vec4(u_textColor.rgb, centerAlpha);
    }
    else
    {
        // 1ピクセルあたりのUVサイズ（UV空間での画素サイズ）
        vec2 texelSize = 1.0 / u_textureSize;

        // アウトラインのUV単位のオフセット距離
        float outlineUV = u_outlinePixelWidth * texelSize.x;

        // 周囲の最大アルファ値を保持（アウトライン判定用）
        float maxAlpha = 0.0;

        // 近傍3x3ピクセル（±1）を走査して最大アルファ値を取得
        float sumAlpha = 0.0;
        int count = 0;
        for (int x = -1; x <= 1; ++x) {
            for (int y = -1; y <= 1; ++y) {
                vec2 offset = vec2(float(x), float(y)) * outlineUV;
                sumAlpha += texture2D(u_texture, v_texCoord + offset).a;
                count ++;
            }
        }
        maxAlpha = sumAlpha / float(count);

        // 描画条件によって色を決定
        if (centerAlpha > 0.0) {
            vec3 blendedRGB = u_textColor.rgb * centerAlpha + u_outlineColor.rgb * (1.0 - centerAlpha);
            gl_FragColor = vec4(blendedRGB, 1.0);
        } else if (maxAlpha >= 0.0) {
            // 近傍が不透明 → アウトライン領域
            gl_FragColor = vec4(u_outlineColor.rgb, maxAlpha * u_outlineColor.a);
        } else {
            // 完全透明 → マージン領域として描画しない（背景が見える）
            gl_FragColor = vec4(0.0);
        }
    }
}
