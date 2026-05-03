#ifdef GL_ES
precision lowp float;
#endif
varying vec4 v_fragmentColor;
varying vec2 v_texCoord;
uniform sampler2D CC_Texture0;
uniform vec2 u_blurDir;
void main() {
    vec4 s =
        texture2D(CC_Texture0, v_texCoord + u_blurDir * -4.0) * 0.05 +
        texture2D(CC_Texture0, v_texCoord + u_blurDir * -3.0) * 0.09 +
        texture2D(CC_Texture0, v_texCoord + u_blurDir * -2.0) * 0.12 +
        texture2D(CC_Texture0, v_texCoord + u_blurDir * -1.0) * 0.15 +
        texture2D(CC_Texture0, v_texCoord) * 0.18 +
        texture2D(CC_Texture0, v_texCoord + u_blurDir * 1.0) * 0.15 +
        texture2D(CC_Texture0, v_texCoord + u_blurDir * 2.0) * 0.12 +
        texture2D(CC_Texture0, v_texCoord + u_blurDir * 3.0) * 0.09 +
        texture2D(CC_Texture0, v_texCoord + u_blurDir * 4.0) * 0.05;
    gl_FragColor = s * v_fragmentColor;
}
