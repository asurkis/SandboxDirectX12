#include "RootSignatureSponza.inc"

struct VertexPosColor
{
    float3 Position : POSITION;
    float3 Normal : NORMAL;
    float3 Tangent : TANGENT;
    float3 Bitangent : BITANGENT;
    float2 UV : UV;
};

struct ModelViewProjection
{
    matrix MV;
    matrix Projection;
};

ConstantBuffer<ModelViewProjection> MVP_CB : register(b0);

struct VertexShaderOutput
{
    float4 ScreenPos : SV_Position;
    float3 ViewPos : VIEW;
    float3 Normal : NORMAL;
    float2 uv : UV;
};

[RootSignature(ROOT_SIGNATURE_SPONZA)]
VertexShaderOutput main(VertexPosColor IN)
{
    VertexShaderOutput OUT;
    matrix MVP = mul(MVP_CB.Projection, MVP_CB.MV);
    float4 view = mul(MVP_CB.MV, float4(IN.Position, 1.0f));
    OUT.ScreenPos = mul(MVP_CB.Projection, view);
    OUT.ViewPos = view.xyz;
    OUT.Normal = mul(MVP_CB.MV, float4(IN.Normal, 0.0f)).xyz;
    OUT.uv = IN.UV;
    return OUT;
}
