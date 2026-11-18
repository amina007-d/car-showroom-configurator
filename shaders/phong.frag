#version 330 core

#define MAX_LIGHTS 4
#define MAX_SPOTS 4

struct PointLight {
    vec3 position;
    vec3 color;
    float intensity;
};

struct SpotLight {
    vec3 position;
    vec3 direction;
    vec3 color;
    float intensity;
    float cosInner;
    float cosOuter;
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
uniform SpotLight uSpots[MAX_SPOTS];
uniform int uNumSpots;
uniform float uAmbient;
uniform vec3 uViewPos;
uniform Material uMaterial;
uniform sampler2D uTexture;
uniform samplerCube uEnvMap;

out vec4 FragColor;

vec3 blinnPhong(vec3 N, vec3 V, vec3 L, vec3 albedo, vec3 radiance) {
    vec3 H = normalize(L + V);
    float diff = max(dot(N, L), 0.0);
    float spec = (diff > 0.0) ? pow(max(dot(N, H), 0.0), uMaterial.shininess) : 0.0;
    return (diff * albedo + spec * uMaterial.specular) * radiance;
}

float attenuation(float d) {
    return 1.0 / (1.0 + 0.045 * d + 0.0075 * d * d);
}

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
        vec3 radiance = uLights[i].color * uLights[i].intensity * attenuation(d);
        color += blinnPhong(N, V, toLight / d, albedo, radiance);
    }
    for (int i = 0; i < uNumSpots; ++i) {
        vec3 toLight = uSpots[i].position - vWorldPos;
        float d = length(toLight);
        vec3 L = toLight / d;
        float cone = smoothstep(uSpots[i].cosOuter, uSpots[i].cosInner, dot(-L, normalize(uSpots[i].direction)));
        vec3 radiance = uSpots[i].color * uSpots[i].intensity * attenuation(d) * cone;
        color += blinnPhong(N, V, L, albedo, radiance);
    }
    if (uMaterial.reflectivity > 0.0) {
        float fresnel = pow(1.0 - max(dot(N, V), 0.0), 5.0);
        float k = mix(uMaterial.reflectivity, min(1.0, uMaterial.reflectivity * 5.0), fresnel);
        vec3 reflection = texture(uEnvMap, reflect(-V, N)).rgb;
        color = mix(color, reflection, k);
    }

    color += uMaterial.emissive;

    color = pow(clamp(color, 0.0, 1.0), vec3(1.0 / 2.2));
    FragColor = vec4(color, alpha);
}
