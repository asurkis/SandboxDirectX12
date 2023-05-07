#include "RootSignature1.hlsl"

struct VertexShaderOutput
{
    float4 Position : SV_Position;
    float2 UV : UV;
};

[RootSignature(ROOT_SIGNATURE_1)] float4 main(VertexShaderOutput IN)
    : SV_Target
{
    return float4(IN.UV, 1.0f, 1.0f);
}
