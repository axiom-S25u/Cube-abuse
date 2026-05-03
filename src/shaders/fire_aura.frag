#ifdef GL_ES
precision mediump float;
#endif
varying vec4 v_fragmentColor;
varying vec2 v_texCoord;
uniform sampler2D CC_Texture0;
uniform vec2 u_velocity;
uniform float u_time;
uniform float u_intensity;
uniform vec3 u_colorPrimary;
uniform vec3 u_colorSecondary;

float hash21(vec2 p) {
    return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453);
}

float noise(vec2 p) {
    vec2 i = floor(p);
    vec2 f = fract(p);
    vec2 u = f * f * (3.0 - 2.0 * f);
    float a = hash21(i);
    float b = hash21(i + vec2(1.0, 0.0));
    float c = hash21(i + vec2(0.0, 1.0));
    float d = hash21(i + vec2(1.0, 1.0));
    return mix(mix(a, b, u.x), mix(c, d, u.x), u.y);
}

float fbm(vec2 p) {
    float v = 0.0;
    float a = 0.5;
    v += a * noise(p);
    p *= 2.02;
    a *= 0.5;
    v += a * noise(p);
    p *= 2.03;
    a *= 0.5;
    v += a * noise(p);
    p *= 2.01;
    a *= 0.5;
    v += a * noise(p);
    return v;
}

void main() {
    vec2 uv = v_texCoord - 0.5;
    float dist = length(uv);
    float mask = smoothstep(0.52, 0.1, dist);

    vec2 vel = u_velocity;
    float vlen = length(vel);
    vec2 dir = vlen > 1e-5 ? vel / vlen : vec2(0.0, 1.0);

    vec2 flowUv = v_texCoord;
    flowUv -= vel * u_time * 0.18;
    flowUv += vec2(0.0, u_time * 0.07);

    float shearAmt = 0.14 * u_intensity;
    flowUv += dir * dist * shearAmt;

    vec2 q = flowUv * 5.5;
    float n = fbm(q + 0.5 * fbm(q * 0.73 + vec2(0.0, u_time * 0.4)));

    vec3 darkC = u_colorSecondary * 0.52;
    vec3 midC = mix(u_colorSecondary, u_colorPrimary, 0.58);
    vec3 hotC = u_colorPrimary;
    vec3 col = mix(darkC, midC, smoothstep(0.15, 0.5, n));
    col = mix(col, hotC, smoothstep(0.5, 0.92, n));

    float glow = exp(-dist * 3.8) * u_intensity * 0.4;
    col += glow * mix(u_colorSecondary, u_colorPrimary, 0.72);

    vec4 tex = texture2D(CC_Texture0, v_texCoord);
    float alpha = mask * u_intensity * tex.a;
    vec3 rgb = col * v_fragmentColor.rgb * tex.rgb;
    // Premultiplied alpha: required for blend GL_ONE, GL_ONE_MINUS_SRC_ALPHA
    gl_FragColor = vec4(rgb * alpha, alpha);
}
