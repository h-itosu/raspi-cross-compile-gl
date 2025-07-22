precision mediump float;

uniform sampler2D u_texture;
uniform vec4 u_outlineColor;
uniform float u_outlineWidth;

varying vec2 v_texcoord;

void main() {
    float alpha = texture2D(u_texture, v_texcoord).a;

    // アウトライン検出用
    float outlineAlpha = 0.0;
    float offset = u_outlineWidth;

    for (int y = -1; y <= 1; y++) {
        for (int x = -1; x <= 1; x++) {
            if (x == 0 && y == 0) continue;
            vec2 offsetUV = vec2(float(x), float(y)) * offset;
            outlineAlpha += texture2D(u_texture, clamp((v_texcoord + offsetUV), vec2(0,0), vec2(1,1))).a;
        }
    }

    // 描画判定
    if (alpha > 0.1) {
        // 中心の白文字
        gl_FragColor = vec4(1.0, 1.0, 1.0, alpha);
    } else if (outlineAlpha > 0.1) {
        // アウトライン
        gl_FragColor = vec4(u_outlineColor.rgb, u_outlineColor.a);
    } else {
        discard;
    }
}
