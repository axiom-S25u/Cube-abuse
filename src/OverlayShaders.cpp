#include "OverlayShaders.h"

namespace overlay_rendering::shaders {

char const kMotionBlurVert[] = R"OVLSH(attribute vec4 a_position;
attribute vec4 a_color;
attribute vec2 a_texCoord;
#ifdef GL_ES
varying lowp vec4 v_fragmentColor;
varying mediump vec2 v_texCoord;
#else
varying vec4 v_fragmentColor;
varying vec2 v_texCoord;
#endif
void main() {
    gl_Position = CC_MVPMatrix * a_position;
    v_fragmentColor = a_color;
    v_texCoord = a_texCoord;
}
)OVLSH";
char const kMotionBlurFrag[] = R"OVLSH(#ifdef GL_ES
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
)OVLSH";
char const kWhiteFlashFrag[] = R"OVLSH(#ifdef GL_ES
precision lowp float;
#endif
// Premultiplied RGB from alpha (silhouette). Pair blend with GL_ONE, GL_ONE_MINUS_SRC_ALPHA on the sprite
varying vec2 v_texCoord;
uniform sampler2D CC_Texture0;
void main() {
    float a = texture2D(CC_Texture0, v_texCoord).a;
    gl_FragColor = vec4(a, a, a, a);
}
)OVLSH";
char const kColorInvertFrag[] = R"OVLSH(#ifdef GL_ES
precision lowp float;
#endif
// Premultiplied-style silhouette (inverted RGB from alpha). Pair blend with GL_ONE, GL_ONE_MINUS_SRC_ALPHA
varying vec2 v_texCoord;
uniform sampler2D CC_Texture0;
void main() {
    float a = texture2D(CC_Texture0, v_texCoord).a;
    gl_FragColor = vec4(1.0 - a, 1.0 - a, 1.0 - a, a);
}
)OVLSH";
char const kFireAuraFrag[] = R"OVLSH(#ifdef GL_ES
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
)OVLSH";
char const kImpactNoiseFrag[] = R"OVLSH(#ifdef GL_ES
precision mediump float;
#endif
varying vec2 v_texCoord;
uniform float u_time;
uniform float u_alpha;

// Simplex-3D FBM + domain warp (z = time)
const float noiseScale = 2.0;
const float noiseTimeScale = 0.1;

const int noiseSwirlSteps = 2;
const float noiseSwirlValue = 1.0;
const float noiseSwirlStepValue = noiseSwirlValue / float(noiseSwirlSteps);

// Ashima simplex 3D https://github.com/ashima/webgl-noise
vec3 mod289(vec3 x) { return x - floor(x * (1.0 / 289.0)) * 289.0; }
vec4 mod289(vec4 x) { return x - floor(x * (1.0 / 289.0)) * 289.0; }
vec4 permute(vec4 x) { return mod289(((x * 34.0) + 1.0) * x); }
vec4 taylorInvSqrt(vec4 r) { return 1.79284291400159 - 0.85373472095314 * r; }

float simplex(vec3 v) {
    const vec2 C = vec2(1.0 / 6.0, 1.0 / 3.0);
    const vec4 D = vec4(0.0, 0.5, 1.0, 2.0);

    vec3 i  = floor(v + dot(v, C.yyy));
    vec3 x0 = v - i + dot(i, C.xxx);

    vec3 g = step(x0.yzx, x0.xyz);
    vec3 l = 1.0 - g;
    vec3 i1 = min(g.xyz, l.zxy);
    vec3 i2 = max(g.xyz, l.zxy);

    vec3 x1 = x0 - i1 + C.xxx;
    vec3 x2 = x0 - i2 + C.yyy;
    vec3 x3 = x0 - D.yyy;

    i = mod289(i);
    vec4 p = permute(permute(permute(
                 i.z + vec4(0.0, i1.z, i2.z, 1.0))
               + i.y + vec4(0.0, i1.y, i2.y, 1.0))
               + i.x + vec4(0.0, i1.x, i2.x, 1.0));

    float n_ = 0.142857142857;
    vec3 ns = n_ * D.wyz - D.xzx;

    vec4 j = p - 49.0 * floor(p * ns.z * ns.z);

    vec4 x_ = floor(j * ns.z);
    vec4 y_ = floor(j - 7.0 * x_);

    vec4 x = x_ * ns.x + ns.yyyy;
    vec4 y = y_ * ns.x + ns.yyyy;
    vec4 h = 1.0 - abs(x) - abs(y);

    vec4 b0 = vec4(x.xy, y.xy);
    vec4 b1 = vec4(x.zw, y.zw);

    vec4 s0 = floor(b0) * 2.0 + 1.0;
    vec4 s1 = floor(b1) * 2.0 + 1.0;
    vec4 sh = -step(h, vec4(0.0));

    vec4 a0 = b0.xzyw + s0.xzyw * sh.xxyy;
    vec4 a1 = b1.xzyw + s1.xzyw * sh.zzww;

    vec3 p0 = vec3(a0.xy, h.x);
    vec3 p1 = vec3(a0.zw, h.y);
    vec3 p2 = vec3(a1.xy, h.z);
    vec3 p3 = vec3(a1.zw, h.w);

    vec4 norm = taylorInvSqrt(vec4(dot(p0, p0), dot(p1, p1), dot(p2, p2), dot(p3, p3)));
    p0 *= norm.x;
    p1 *= norm.y;
    p2 *= norm.z;
    p3 *= norm.w;

    vec4 m = max(0.6 - vec4(dot(x0, x0), dot(x1, x1), dot(x2, x2), dot(x3, x3)), 0.0);
    m = m * m;
    return 42.0 * dot(m * m, vec4(dot(p0, x0), dot(p1, x1),
                                  dot(p2, x2), dot(p3, x3)));
}

float fbm3(vec3 v) {
    float r = simplex(v);
    r += simplex(v * 2.0) / 2.0;
    r += simplex(v * 4.0) / 4.0;
    return r / (1.0 + 1.0 / 2.0 + 1.0 / 4.0);
}

float fbm5(vec3 v) {
    float r = simplex(v);
    r += simplex(v * 2.0) / 2.0;
    r += simplex(v * 4.0) / 4.0;
    r += simplex(v * 8.0) / 8.0;
    r += simplex(v * 16.0) / 16.0;
    return r / (1.0 + 1.0 / 2.0 + 1.0 / 4.0 + 1.0 / 8.0 + 1.0 / 16.0);
}

// Warp p in-place, return warped vec3 for reuse
vec3 warpDomain(vec3 v) {
    for (int i = 0; i < noiseSwirlSteps; i++) {
        v.xy += vec2(fbm3(v), fbm3(vec3(v.xy, v.z + 1000.0))) * noiseSwirlStepValue;
    }
    return v;
}

void main() {
    vec2 uv = v_texCoord;
    float t = u_time * noiseTimeScale;

    vec3 warped = warpDomain(vec3(uv * noiseScale, t));

    // Chromatic offset in noise-space
    float strength = 0.03;
    vec2 d = (uv - 0.5) * strength * noiseScale;

    float r = fbm5(warped + vec3( d, 0.0)) / 2.0 + 0.5;
    float g = fbm5(warped)                 / 2.0 + 0.5;
    float b = fbm5(warped + vec3(-d, 0.0)) / 2.0 + 0.5;

    r = r * r * r * r * 2.0;
    g = g * g * g * g * 2.0;
    b = b * b * b * b * 2.0;

    vec3 nc = clamp(vec3(r, g, b), 0.0, 1.0);
    float a = u_alpha * max(max(nc.r, nc.g), nc.b);
    gl_FragColor = vec4(nc * a, a);
}
)OVLSH";

} // namespace overlay_rendering::shaders
