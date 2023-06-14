#include "RootSignatureSponza.hlsl"

Texture2D BaseColorTexture : register(t0);
SamplerState DefaultSampler : register(s0);

struct VertexShaderOutput
{
    float4 Position : SV_Position;
    float3 Normal : NORMAL;
    float2 uv : UV;
};

[RootSignature(ROOT_SIGNATURE_SPONZA)]
float4 main(VertexShaderOutput IN)
    : SV_Target
{
    // return float4(IN.uv, 0.0f, 1.0f);
    return float4(0.5f * (IN.Normal + float3(1.0f, 1.0f, 1.0f)), 1.0f);
    // return BaseColorTexture.Sample(DefaultSampler, IN.uv);
}
