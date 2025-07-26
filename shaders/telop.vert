attribute vec2 a_position;
attribute vec2 a_texcoord;
varying vec2 v_texCoord;

uniform vec2 u_resolution;

void main() {
    vec2 zeroToOne = a_position / u_resolution;
    vec2 zeroToTwo = zeroToOne * 2.0;
    vec2 clipSpace = zeroToTwo - 1.0;
    gl_Position = vec4(clipSpace * vec2(1, -1), 0, 1);
    v_texCoord = vec2(a_texcoord.x, 1.0 - a_texcoord.y);  // ← ここでY軸反転
}
