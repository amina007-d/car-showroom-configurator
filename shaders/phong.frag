#version 330 core

#define MAX_LIGHTS 4

struct PointLight {
    vec3 position;
    vec3 color;
    float intensity;
};

struct Material {
    vec3 color;
    float shininess;
    float specular;
    vec3 emissive;
    float alpha;
    bool useTexture;
    float reflectivity;
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
uniform samplerCube uEnvMap;

out vec4 FragColor;

void main() {
    vec3 albedo = uMaterial.color;
    float alpha = uMaterial.alpha;
    if (uMaterial.useTexture) {
        vec4 texel = texture(uTexture, vUV);
        albedo *= texel.rgb;
        alpha *= texel.a;
    }

    vec3 N = normalize(vNormal);
    if (!gl_FrontFacing) N = -N;
    vec3 V = normalize(uViewPos - vWorldPos);

    vec3 color = uAmbient * albedo;
    for (int i = 0; i < uNumLights; ++i) {
        vec3 toLight = uLights[i].position - vWorldPos;
        float d = length(toLight);
        vec3 L = toLight / d;
        vec3 H = normalize(L + V);

        float diff = max(dot(N, L), 0.0);
        float spec = (diff > 0.0) ? pow(max(dot(N, H), 0.0), uMaterial.shininess) : 0.0;
        float attenuation = 1.0 / (1.0 + 0.045 * d + 0.0075 * d * d);

        vec3 radiance = uLights[i].color * uLights[i].intensity * attenuation;
        color += (diff * albedo + spec * uMaterial.specular) * radiance;
    }
    vec3 R = reflect(-V, N);
    vec3 reflection = texture(uEnvMap, R).rgb;
    color = mix(color, reflection, uMaterial.reflectivity);

    color += uMaterial.emissive;

    color = pow(clamp(color, 0.0, 1.0), vec3(1.0 / 2.2));
    FragColor = vec4(color, alpha);
}
