#define MaxLights 16

struct Light
{
    float3 strength;
    float falloffStart; // point/spot light only
    float3 direction; // directional/spot light only
    float falloffEnd; // point/spot light only
    float3 position; // point light only
    float spotPower; // spot light only
};

struct Material
{
    float4 diffuseAlbedo;
    float3 fresnelR0;
    float shininess;
};

float CalcAttenuation(float d, float falloffEnd, float falloffStart)
{
    return saturate((falloffEnd - d) / (falloffEnd - falloffStart));
}

float3 SchlickFresnel(float3 R0, float3 normal, float3 lightVec)
{
    float cosIncidentAngle = max(dot(normal, lightVec), 0.0f);
    
    float f0 = 1.0f - cosIncidentAngle;
    float3 reflectPercent = R0 + (1.0f - R0) * (f0 * f0 * f0 * f0 * f0);
    
    return reflectPercent;
}

float3 BlinnPhong(float3 lightStrength, float3 lightVec, float3 normal, float3 toEye, Material mat)
{
    const float m = mat.shininess * 256.0f;
    float3 halfVec = normalize(lightVec + toEye);
    
    float roughnessFactor = 
        (m + 8.0f) * pow(max(dot(halfVec, normal), 0.0f), m) / 8.0f;
    float3 fresnelFactor =
        SchlickFresnel(mat.fresnelR0, normal, lightVec);
    
    float3 specAlbedo = roughnessFactor * fresnelFactor;
    
    specAlbedo = specAlbedo / (specAlbedo + 1.0f);
    
    return (mat.diffuseAlbedo.rgb + specAlbedo) * lightStrength;
}

float3 ComputeDirectionalLight(Light light, Material mat, float3 normal, float3 toEye)
{
    // The light vector aims to opposite the direction the light rays travel.
    float3 lightVec = normalize(-light.direction);
    
    // Scale light down by Lambert's cosine law.
    float ndotl = max(dot(lightVec, normal), 0.0f);
    float3 lightStrength = light.strength * ndotl;
    
    return BlinnPhong(lightStrength, lightVec, normal, toEye, mat);
}

float3 ComputePointLight(Light light, Material mat, float3 pos, 
    float3 normal, float3 toEye)
{
    // The vector from the surface to the light.
    float3 lightVec = normalize(light.position - pos);
    
    // The distance from surface to light.
    float d = length(lightVec);
    
    // Range test.
    if (d > light.falloffEnd)
        return 0.0f;
    
    // Normalize the light vector.
    lightVec /= d;
    
    // Scale light down by Lambert's cosine law.
    float ndotl = max(dot(lightVec, normal), 0.0f);
    float3 lightStrength = light.strength * ndotl;
    
    // Attenuate light by distance.
    float att = CalcAttenuation(d, light.falloffStart, light.falloffEnd);
    lightStrength *= att;
    
    return BlinnPhong(lightStrength, lightVec, normal, toEye, mat);
}

float3 ComputeSpotLight(Light light, Material mat, float3 pos, float3 normal, float3 toEye)
{
    // The vector from the surface to the light.
    float3 lightVec = normalize(light.position - pos);

    // The distance from surface to light.
    float d = length(lightVec);

    // Range test.
    if (d > light.falloffEnd)
        return 0.0f;

    // Normalize the light vector.
    lightVec /= d;

    // Scale light down by Lambert's cosine law.
    float ndotl = max(dot(lightVec, normal), 0.0f);
    float3 lightStrength = light.strength * ndotl;

    // Attenuate light by distance.
    float att = CalcAttenuation(d, light.falloffStart, light.falloffEnd);
    lightStrength *= att;

    // Scale by spotlight
    float spotFactor = pow(max(dot(-lightVec, light.direction), 0.0f), light.spotPower);
    lightStrength *= spotFactor;

    return BlinnPhong(lightStrength, lightVec, normal, toEye, mat);
}

float4 ComputeLighting(Light gLights[MaxLights], Material mat,
                       float3 pos, float3 normal, float3 toEye,
                       float3 shadowFactor)
{
    float3 result = 0.0f;
    
    int i = 0;
    
#if (NUM_DIR_LIGHTS > 0)
    for (i = 0; i < NUM_DIR_LIGHTS; ++i)
    {
        result += shadowFactor[i] * ComputeDirectionalLight(gLights[i], mat, normal, toEye);
    }
#endif
    
#if (NUM_POINT_LIGHTS > 0)
    for (i = NUM_DIR_LIGHTS; i < NUM_DIR_LIGHTS + NUM_POINT_LIGHTS; ++i)
    {
        result += ComputePointLight(gLights[i], mat, pos, normal, toEye);
    }
#endif
    
#if (NUM_SPOT_LIGHTS > 0)
    for (i = NUM_DIR_LIGHTS + NUM_POINT_LIGHTS; i < NUM_DIR_LIGHTS + NUM_POINT_LIGHTS + NUM_SPOT_LIGHTS; ++i)
    {
        result += ComputeSpotLight(gLights[i], mat, pos, normal, toEye);
    }
#endif
    
    return float4(result, 0.0f);
}