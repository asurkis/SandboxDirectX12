#include "RootSignature1.hlsl"

struct VertexPosColor
{
    float3 Position : POSITION;
    float3 Color : COLOR;
};

struct ModelViewProjection
{
    matrix MVP;
};

ConstantBuffer<ModelViewProjection> ModelViewProjectionCB : register(b0);

struct VertexShaderOutput
{
    float4 Position : SV_Position;
    float4 Color : COLOR;
};

[RootSignature(ROOT_SIGNATURE_1)]
VertexShaderOutput main(VertexPosColor IN)
{
    VertexShaderOutput OUT;
    OUT.Position = mul(ModelViewProjectionCB.MVP, float4(IN.Position, 1.0f));
    // OUT.Position = float4(0.5 * IN.Position, 1.0f);
    OUT.Color = float4(IN.Color, 1.0f);
    return OUT;
}
