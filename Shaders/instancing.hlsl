#include "common.hlsl"

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
    
    nointerpolation uint materialIndex : MATINDEX;
};

VertexOut VSMain(VertexIn vin, uint instanceID : SV_InstanceID)
{
    VertexOut vout = (VertexOut) 0.0f;
    
    InstanceData instanceData = gInstanceDatas[instanceID];
    
    float4x4 gViewProj = mul(gView, gProj);
    
    vout.PosW = (float3) (mul(float4(vin.PosL, 1.0f), instanceData.world));
    
    vout.PosH = mul(float4(vout.PosW, 1.0), gViewProj);
    vout.NormalW = mul(vin.NormalL, (float3x3) instanceData.world);
    vout.TexC = vin.TexC;
    
    vout.materialIndex = instanceData.materialIndex;
    
    return vout;
}

float4 PSMain(VertexOut pin) : SV_Target
{    
    MaterialData matData = gMaterialDatas[pin.materialIndex];
    
    pin.NormalW = normalize(pin.NormalW);
    
    float3 toEyeW = normalize(cameraPosition - pin.PosW);
    
    float4 diffuseAlbedo = gDiffuseTexture.Sample(gsamAnisotropicWrap, pin.TexC);
    diffuseAlbedo *= matData.diffuseAlbedo;
    
    float4 ambient = gAmbientLight * diffuseAlbedo;
    
    const float shininess = 1.0f - matData.roughness;
    Material mat = { diffuseAlbedo, matData.fresnelR0, shininess };
    float3 shadowFactor = 1.0f;
    float4 directLight = ComputeLighting(gLights, mat, pin.PosW,
        pin.NormalW, toEyeW, shadowFactor);
    
    float4 litColor = ambient + directLight;
    
    litColor.a = diffuseAlbedo.a;
    
    return litColor;
}