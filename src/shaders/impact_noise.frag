#ifdef GL_ES
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
