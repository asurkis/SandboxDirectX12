#include "RootSignature1.hlsl"

struct VertexShaderOutput
{
    float4 Position : SV_Position;
    float4 Color : COLOR;
};

[RootSignature(ROOT_SIGNATURE_1)] float4 main(VertexShaderOutput IN)
    : SV_Target
{
    return IN.Color;
}
