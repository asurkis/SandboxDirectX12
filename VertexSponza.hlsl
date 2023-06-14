#include "RootSignatureSponza.hlsl"

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
    matrix MVP;
};

ConstantBuffer<ModelViewProjection> ModelViewProjectionCB : register(b0);

struct VertexShaderOutput
{
    float4 Position : SV_Position;
    float3 Normal : NORMAL;
    float2 uv : UV;
};

[RootSignature(ROOT_SIGNATURE_SPONZA)]
VertexShaderOutput main(VertexPosColor IN)
{
    VertexShaderOutput OUT;
    OUT.Position = mul(ModelViewProjectionCB.MVP, float4(IN.Position, 1.0f));
    float4 transNorm = mul(ModelViewProjectionCB.MVP, float4(IN.Normal, 0.0f));
    // if (norm.z < 0.0f)
    //     norm = -norm;
    OUT.Normal = normalize(transNorm);
    OUT.uv = IN.UV;
    return OUT;
}
