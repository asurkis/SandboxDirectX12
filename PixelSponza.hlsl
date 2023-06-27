#include "RootSignatureSponza.inc"

Texture2D BaseColorTexture : register(t0);
SamplerState DefaultSampler : register(s0);

struct VertexShaderOutput
{
    float4 ScreenPos : SV_Position;
    float3 View : VIEW;
    float3 Normal : NORMAL;
    float2 uv : UV;
};

[RootSignature(ROOT_SIGNATURE_SPONZA)]
float4 main(VertexShaderOutput IN) : SV_Target
{
    // return float4(IN.View.xyz, 1.0f);
    
    float3 norm = normalize(IN.Normal);
    // return float4(IN.uv, 0.0f, 1.0f);
    // return float4(0.5f * (norm + float3(1.0f, 1.0f, 1.0f)), 1.0f);
    float3 fall = IN.View - float3(0.125f, 0.125f, 0.5f);
    float dist2 = dot(fall, fall);
    float diffuse1 = max(0, dot(normalize(-fall), norm)) / dist2;
    float3 diffuse = float3(1.0f, 2.0f, 4.0f) * diffuse1;
    
    float3 ambient = float3(0.25f, 0.25f, 0.25f);
    float3 lighting = ambient + diffuse;
    float4 base = BaseColorTexture.Sample(DefaultSampler, IN.uv);
    /*
    if ((int(IN.ScreenPos.x) + int(IN.ScreenPos.y)) % 2 == 0)
    {
        base = BaseColorTexture.Sample(DefaultSampler, IN.uv);
    }
    else
    {
        base = BaseColorTexture.Sample(DefaultSampler, IN.uv / 0.0f);
        base = float4(0.0f, 0.0f, 0.0f, 1.0f);
    }
    */
    // return base.aaaa;
    if (base.w < 0.875)
        discard;
    return float4(base.xyz * lighting, base.w);
}
