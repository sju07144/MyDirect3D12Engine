#ifndef NUM_DIR_LIGHTS
    #define NUM_DIR_LIGHTS 1
#endif

#ifndef NUM_POINT_LIGHTS
    #define NUM_POINT_LIGHTS 0
#endif

#ifndef NUM_SPOT_LIGHTS
    #define NUM_SPOT_LIGHTS 0
#endif

#include "LightingUtility.hlsl"

cbuffer cbPerObject : register(b0)
{
    float4x4 gWorld;
    uint materialIndex;
};

cbuffer cbScene : register(b1)
{
    float4x4 gView;
    float4x4 gProj;
    float3 cameraPosition;
    float pad0;
    float4 gAmbientLight;
    
    Light gLights[MaxLights];
};

cbuffer cbMaterial : register(b2)
{
    float4 gDiffuseAlbedo;
    float3 gFresnelR0;
    float gRoughness;
};

struct MaterialData
{
    float4 diffuseAlbedo;
    float3 fresnelR0;
    float roughness;
};

struct InstanceData
{
    float4x4 world;
    uint materialIndex;
};

StructuredBuffer<MaterialData> gMaterialDatas : register(t0, space1);
StructuredBuffer<InstanceData> gInstanceDatas : register(t1, space1);
Texture2D gDiffuseTexture : register(t0);
TextureCube gSkyBox : register(t1);

SamplerState gsamPointWrap : register(s0);
SamplerState gsamPointClamp : register(s1);
SamplerState gsamLinearWrap : register(s2);
SamplerState gsamLinearClamp : register(s3);
SamplerState gsamAnisotropicWrap : register(s4);
SamplerState gsamAnisotropicClamp : register(s5);
SamplerComparisonState gsamShadow : register(s6);