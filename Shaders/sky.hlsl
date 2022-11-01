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
    float3 PosL : POSITION;
};

VertexOut VSMain(VertexIn vin)
{
    VertexOut vout;
    
    float4x4 viewProj = mul(gView, gProj);
    
    float4 posW = mul(float4(vin.PosL, 1.0), gWorld);
    // posW.xyz += cameraPosition;
    
    vout.PosH = mul(posW, viewProj).xyww;
    
    vout.PosL = vin.PosL;
    
    return vout;
}

float4 PSMain(VertexOut pin) : SV_Target
{
    float4 color = gSkyBox.Sample(gsamLinearWrap, pin.PosL);
    
    return color;
}