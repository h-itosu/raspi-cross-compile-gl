#version 100
precision mediump float;
varying vec2 v_texCoord;
uniform sampler2D texY;
uniform sampler2D texUV;

void main() {
    float y = texture2D(texY, v_texCoord).r;
    vec2 uv = texture2D(texUV, v_texCoord).ra;
    float u = uv.x - 0.5;
    float v = uv.y - 0.5;

    float r = y + 1.402 * v;
    float g = y - 0.344136 * u - 0.714136 * v;
    float b = y + 1.772 * u;

    gl_FragColor = vec4(r, g, b, 1.0);
}
