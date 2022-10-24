cbuffer cbPerObject : register(b0)
{
    float4x4 gWorld;
    float4x4 gView;
    float4x4 gProj;
    float4x4 gWorldViewProj;
};

struct VertexIn
{
    float3 PosL : POSITION;
    float3 NormalL : NORMAL;
    float2 TexC : TEXCOORD;
};

struct VertexOut
{
    float4 PosH : SV_POSITION;
    float3 NormalW : NORMAL;
    float2 TexC : TEXCOORD;
};

VertexOut VSMain(VertexIn vin)
{
    VertexOut vout;
    
    // float4x4 gViewProj = mul(gView, gProj);
    // float4x4 gWorldViewProj = mul(gWorld, gViewProj); 
    
    vout.PosH = mul(float4(vin.PosL, 1.0), gWorldViewProj);
    vout.NormalW = mul(vin.NormalL, (float3x3) gWorld);
    vout.TexC = vin.TexC;
    
    return vout;
}

float4 PSMain(VertexOut pin) : SV_Target
{
    return float4(1.0, 0.0, 0.0, 1.0);
}