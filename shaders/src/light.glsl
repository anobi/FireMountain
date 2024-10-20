struct Light {
    vec4 colorIntensity;
    float intensity;
    float attenuation;
    highp vec3 position;
    highp vec3 direction;
    uint type;
};