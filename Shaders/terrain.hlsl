#include "common.hlsl"

struct VertexIn
{
    float3 PosL : POSITION;
    float3 NormalL : NORMAL;
    float2 TexC : TEXCOORD;
};

struct VertexOut
{
    float3 PosL : POSITION;
    float2 TexC : TEXCOORD;
};

struct PatchTess
{
    float EdgeTess[4] : SV_TessFactor;
    float InsideTess[2] : SV_InsideTessFactor;
};

struct HullOut
{
    float3 PosL : POSITION;
    float2 TexC : TEXCOORD;
};

struct DomainOut
{
    float4 PosH : SV_POSITION;
    float3 NormalL : NORMAL;
    nointerpolation float Height : HEIGHT;
};

VertexOut VSMain(VertexIn vin)
{
    VertexOut vout;
    
    vout.PosL = vin.PosL;
    vout.TexC = vin.TexC;
    
    return vout;
}

PatchTess ConstantHSMain(InputPatch<VertexOut, 4> patch,
                         uint patchID : SV_PrimitiveID)
{
    PatchTess pt;
    
    pt.EdgeTess[0] = 16;
    pt.EdgeTess[1] = 16;
    pt.EdgeTess[2] = 16;
    pt.EdgeTess[3] = 16;
    
    pt.InsideTess[0] = 16;
    pt.InsideTess[1] = 16;
    
    return pt;
}

[domain("quad")]
[partitioning("fractional_odd")]
[outputtopology("triangle_ccw")]
[outputcontrolpoints(4)]
[patchconstantfunc("ConstantHSMain")]
[maxtessfactor(64.0f)]
HullOut HSMain(InputPatch<VertexOut, 4> patch,
               uint controlPointID : SV_OutputControlPointID,
               uint patchID : SV_PrimitiveID)
{
    HullOut hout;
    
    hout.PosL = patch[controlPointID].PosL;
    hout.TexC = patch[controlPointID].TexC;
    
    return hout;
}

[domain("quad")]
DomainOut DSMain(PatchTess patchTess,
                 float2 uv : SV_DomainLocation,
                 const OutputPatch<HullOut, 4> quad)
{
    DomainOut dout;
    
    // bilinearly interpolate texture coordinate across patch
    float2 tex0 = lerp(quad[0].TexC, quad[1].TexC, uv.x);
    float2 tex1 = lerp(quad[2].TexC, quad[3].TexC, uv.x);
    float2 texC = lerp(tex0, tex1, uv.y);
    
    // lookup texel at patch coordinate for height and scale + shift as desired
    dout.Height = gDiffuseTexture.SampleLevel(gsamLinearWrap, texC, 0).r * 64.0f - 16.0f;
    
    // compute patch surface normal
    float3 uVec = quad[1].PosL - quad[0].PosL;
    float3 vVec = quad[2].PosL - quad[0].PosL;
    dout.NormalL = normalize(cross(uVec, vVec));
    
    // bilinearly interpolate position coordinate across patch
    float3 p0 = lerp(quad[0].PosL, quad[1].PosL, uv.x);
    float3 p1 = lerp(quad[2].PosL, quad[3].PosL, uv.x);
    float3 PosL = lerp(p0, p1, uv.y);
    
    // displace point along normal
    PosL += dout.NormalL * dout.Height;
    
    float4x4 viewProj = mul(gView, gProj);
    float4x4 worldViewProj = mul(gWorld, viewProj);
    
    // output patch point position in clip space
    dout.PosH = mul(float4(PosL, 1.0), worldViewProj);
    
    return dout;
}


float4 PSMain(DomainOut pin) : SV_Target
{
    float h = (pin.Height + 16.0f) / 64.0f;
    return float4(h, h, h, 1.0f);
}