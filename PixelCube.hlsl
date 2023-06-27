#include "RootSignatureCube.inc"

struct VertexShaderOutput
{
    float4 Position : SV_Position;
    float4 Color : COLOR;
};

[RootSignature(ROOT_SIGNATURE_CUBE)]
float4 main(VertexShaderOutput IN) : SV_Target
{
    return IN.Color;
}
