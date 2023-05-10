#include "RootSignature2.hlsl"

struct VertexShaderOutput
{
    float4 Position : SV_Position;
    float2 uv : UV;
};

[RootSignature(ROOT_SIGNATURE_2)] VertexShaderOutput main(float2 uv : UV)
{
    VertexShaderOutput OUT;
    OUT.Position = float4(2.0f * uv - float2(1.0f, 1.0f), 0.0f, 1.0f);
    OUT.uv       = uv;
    return OUT;
}
