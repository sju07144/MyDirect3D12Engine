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

Texture2D gDiffuseTexture : register(t0);

SamplerState gsamPointWrap : register(s0);
SamplerState gsamPointClamp : register(s1);
SamplerState gsamLinearWrap : register(s2);
SamplerState gsamLinearClamp : register(s3);
SamplerState gsamAnisotropicWrap : register(s4);
SamplerState gsamAnisotropicClamp : register(s5);
SamplerComparisonState gsamShadow : register(s6);

struct VertexIn
{
    float3 PosL : POSITION;
    float3 NormalL : NORMAL;
    float2 TexC : TEXCOORD;
};

struct VertexOut
{
    float4 PosH : SV_POSITION;
    float3 PosW : POSITION;
    float3 NormalW : NORMAL;
    float2 TexC : TEXCOORD;
};

VertexOut VSMain(VertexIn vin)
{
    VertexOut vout;
    
    float4x4 gViewProj = mul(gView, gProj); 
    
    vout.PosW = (float3) (mul(float4(vin.PosL, 1.0f), gWorld));
    
    vout.PosH = mul(float4(vout.PosW, 1.0), gViewProj);
    vout.NormalW = mul(vin.NormalL, (float3x3) gWorld);
    vout.TexC = vin.TexC;
    
    return vout;
}

float4 PSMain(VertexOut pin) : SV_Target
{
    pin.NormalW = normalize(pin.NormalW);
    
    float3 toEyeW = normalize(cameraPosition - pin.PosW);
    
    float4 diffuseAlbedo = gDiffuseTexture.Sample(gsamAnisotropicWrap, pin.TexC);
    
    float4 ambient = gAmbientLight * diffuseAlbedo;
    
    const float shininess = 1.0f - gRoughness;
    Material mat = { diffuseAlbedo, gFresnelR0, shininess };
    float3 shadowFactor = 1.0f;
    float4 directLight = ComputeLighting(gLights, mat, pin.PosW,
        pin.NormalW, toEyeW, shadowFactor);
    
    float4 litColor = ambient + directLight;
    
    litColor.a = diffuseAlbedo.a;
    
    return litColor;
}