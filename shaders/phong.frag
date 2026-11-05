#version 330 core
// Per-fragment Blinn-Phong lighting (R4) with several point lights.
// result = ambient + sum over lights( diffuse + specular ) * attenuation + emissive

#define MAX_LIGHTS 4

struct PointLight {
    vec3 position;
    vec3 color;
    float intensity;   // 0 when the light is switched off
};

struct Material {
    vec3 color;
    float shininess;   // exponent: small = wide highlight (matte), large = sharp (glossy)
    float specular;    // strength of the highlight
    vec3 emissive;
    float alpha;
    bool useTexture;
};

in vec3 vWorldPos;
in vec3 vNormal;
in vec2 vUV;

uniform PointLight uLights[MAX_LIGHTS];
uniform int uNumLights;
uniform float uAmbient;
uniform vec3 uViewPos;
uniform Material uMaterial;
uniform sampler2D uTexture;

out vec4 FragColor;

void main() {
    vec3 albedo = uMaterial.color;
    float alpha = uMaterial.alpha;
    if (uMaterial.useTexture) {
        vec4 texel = texture(uTexture, vUV);   // sRGB textures are already linear here
        albedo *= texel.rgb;
        alpha *= texel.a;
    }

    vec3 N = normalize(vNormal);
    if (!gl_FrontFacing) N = -N;               // two-sided materials: light the back face too
    vec3 V = normalize(uViewPos - vWorldPos);

    vec3 color = uAmbient * albedo;
    for (int i = 0; i < uNumLights; ++i) {
        vec3 toLight = uLights[i].position - vWorldPos;
        float d = length(toLight);
        vec3 L = toLight / d;
        vec3 H = normalize(L + V);                       // Blinn-Phong half vector

        float diff = max(dot(N, L), 0.0);
        float spec = (diff > 0.0) ? pow(max(dot(N, H), 0.0), uMaterial.shininess) : 0.0;
        float attenuation = 1.0 / (1.0 + 0.045 * d + 0.0075 * d * d);

        vec3 radiance = uLights[i].color * uLights[i].intensity * attenuation;
        color += (diff * albedo + spec * uMaterial.specular) * radiance;
    }
    color += uMaterial.emissive;

    // Colours are computed in linear space; convert to sRGB for the screen.
    color = pow(clamp(color, 0.0, 1.0), vec3(1.0 / 2.2));
    FragColor = vec4(color, alpha);
}
