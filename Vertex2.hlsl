#include "RootSignature2.hlsl"

struct VertexShaderInput
{
    float2 UV : UV;
};

struct VertexShaderOutput
{
    float4 Position : SV_Position;
    float2 UV : UV;
};

[RootSignature(ROOT_SIGNATURE_2)] VertexShaderOutput main(VertexShaderInput IN)
{
    VertexShaderOutput OUT;
    OUT.Position = float4(2.0f * IN.UV - float2(1.0f, 1.0f), 0.0f, 1.0f);
    OUT.UV       = IN.UV;
    return OUT;
}
