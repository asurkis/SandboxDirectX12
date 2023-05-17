#include "RootSignature2.hlsl"

Texture2D Frame : register(t0);

struct ScreenSize
{
    int2 Size;
};

ConstantBuffer<ScreenSize> ScreenSizeCB : register(b0);

struct VertexShaderOutput
{
    float4 Position : SV_Position;
    float2 uv : UV;
};

bool IsPrime(int x)
{
    for (int i = 2; i * i <= x; ++i)
        if (x % i == 0)
            return false;
    return true;
}

[RootSignature(ROOT_SIGNATURE_2)] float4 main(VertexShaderOutput IN)
    : SV_Target
{
    int2 pos = ScreenSizeCB.Size * (IN.uv * float2(1.0f, -1.0f) + float2(0.0f, 1.0f));
    return Frame.Load(int3(pos, 0));

    float4 gx = 1 * Frame.Load(int3(pos + int2(1, -1), 0)) - 1 * Frame.Load(int3(pos + int2(-1, -1), 0))
              + 2 * Frame.Load(int3(pos + int2(1, +0), 0)) - 2 * Frame.Load(int3(pos + int2(-1, +0), 0))
              + 1 * Frame.Load(int3(pos + int2(1, +1), 0)) - 1 * Frame.Load(int3(pos + int2(-1, +1), 0));

    float4 gy = 1 * Frame.Load(int3(pos + int2(-1, 1), 0)) - 1 * Frame.Load(int3(pos + int2(-1, -1), 0))
              + 2 * Frame.Load(int3(pos + int2(+0, 1), 0)) - 2 * Frame.Load(int3(pos + int2(+0, -1), 0))
              + 1 * Frame.Load(int3(pos + int2(+1, 1), 0)) - 1 * Frame.Load(int3(pos + int2(+1, -1), 0));

    // return float4(sqrt(gx.rgb * gx.rgb + gy.rgb * gy.rgb), 1.0f);
}
