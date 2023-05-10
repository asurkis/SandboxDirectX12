#include "RootSignatureSponza.hlsl"

struct VertexPosColor
{
    float3 Position : POSITION;
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
    float2 UV : UV;
};

[RootSignature(ROOT_SIGNATURE_SPONZA)] VertexShaderOutput main(VertexPosColor IN) {
    VertexShaderOutput OUT;
    OUT.Position = mul(ModelViewProjectionCB.MVP, float4(0.005f * IN.Position, 1.0f));

    OUT.UV = IN.UV;
    return OUT;
}
