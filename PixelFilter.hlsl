#include "RootSignature2.hlsl"

Texture2D<float4> Frame : register(s0);

struct ScreenSize
{
    int2 Size;
};

ConstantBuffer<ScreenSize> ScreenSizeCB : register(b0);

struct VertexShaderOutput
{
    float4 Position : SV_Position;
    float2 UV : UV;
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
    // return Frame.Load(IN.UV);
    int2 pos = ScreenSizeCB.Size * IN.UV;

    float4 gx = 1 * Frame.Load(int3(pos + int2(1, -1), 0)) - 1 * Frame.Load(int3(pos + int2(-1, -1), 0))
              + 2 * Frame.Load(int3(pos + int2(1, +0), 0)) - 2 * Frame.Load(int3(pos + int2(-1, +0), 0))
              + 1 * Frame.Load(int3(pos + int2(1, +1), 0)) - 1 * Frame.Load(int3(pos + int2(-1, +1), 0));

    float4 gy = 1 * Frame.Load(int3(pos + int2(-1, 1), 0)) - 1 * Frame.Load(int3(pos + int2(-1, -1), 0))
              + 2 * Frame.Load(int3(pos + int2(+0, 1), 0)) - 2 * Frame.Load(int3(pos + int2(+0, -1), 0))
              + 1 * Frame.Load(int3(pos + int2(+1, 1), 0)) - 1 * Frame.Load(int3(pos + int2(+1, -1), 0));

    // return float4(sqrt(gx.rgb * gx.rgb + gy.rgb * gy.rgb), 1.0f);
    return Frame.Load(int3(pos, 0));
}
