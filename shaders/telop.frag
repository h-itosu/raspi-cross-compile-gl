precision mediump float;

uniform sampler2D u_texture;
uniform vec4 u_outlineColor;
uniform float u_outlineWidth; // アウトラインの幅をピクセル単位で指定 (例: 1.0, 2.0)
uniform vec2 u_textureSize;   // テクスチャの解像度 (例: vec2(256.0, 256.0))

varying vec2 v_texcoord;

void main() {
    float alpha = texture2D(u_texture, v_texcoord).a;

    // アウトライン検出用
    float outlineAlpha = 0.0;
    // 1ピクセル分のUV座標オフセットを計算
    vec2 onePixel = vec2(1.0, 1.0) / u_textureSize;

    for (int y = -1; y <= 1; y++) {
        for (int x = -1; x <= 1; x++) {
            if (x == 0 && y == 0) continue;
            // ピクセル単位のオフセットをUV座標に変換してサンプリング
            vec2 offsetUV = vec2(float(x), float(y)) * onePixel * u_outlineWidth;
            outlineAlpha += texture2D(u_texture, v_texcoord + offsetUV).a;
        }
    }

    // 描画判定
    if (alpha > 0.1) {
        // 中心の白文字
        gl_FragColor = vec4(1.0, 1.0, 1.0, alpha);
    } else if (outlineAlpha > 0.1) {
        // アウトライン
        // アウトラインの濃さを少し調整したい場合は outlineAlpha を利用する
        // 例: gl_FragColor = vec4(u_outlineColor.rgb, u_outlineColor.a * min(outlineAlpha, 1.0));
        gl_FragColor = u_outlineColor;
    } else {
        discard;
    }
}
