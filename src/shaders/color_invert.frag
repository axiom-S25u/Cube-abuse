#ifdef GL_ES
precision lowp float;
#endif
// Premultiplied-style silhouette (inverted RGB from alpha). Pair blend with GL_ONE, GL_ONE_MINUS_SRC_ALPHA
varying vec2 v_texCoord;
uniform sampler2D CC_Texture0;
void main() {
    float a = texture2D(CC_Texture0, v_texCoord).a;
    gl_FragColor = vec4(1.0 - a, 1.0 - a, 1.0 - a, a);
}
