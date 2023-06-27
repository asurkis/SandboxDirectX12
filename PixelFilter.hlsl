#include "RootSignatureFilter.inc"

Texture2D Frame : register(t0);

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

[RootSignature(ROOT_SIGNATURE_FILTER)]
float4 main(VertexShaderOutput IN) : SV_Target
{
    int2 pos = int2(IN.Position.xy);
    return Frame.Load(int3(pos, 0));

    float4 gx = 1 * Frame.Load(int3(pos + int2(1, -1), 0)) - 1 * Frame.Load(int3(pos + int2(-1, -1), 0))
              + 2 * Frame.Load(int3(pos + int2(1, +0), 0)) - 2 * Frame.Load(int3(pos + int2(-1, +0), 0))
              + 1 * Frame.Load(int3(pos + int2(1, +1), 0)) - 1 * Frame.Load(int3(pos + int2(-1, +1), 0));

    float4 gy = 1 * Frame.Load(int3(pos + int2(-1, 1), 0)) - 1 * Frame.Load(int3(pos + int2(-1, -1), 0))
              + 2 * Frame.Load(int3(pos + int2(+0, 1), 0)) - 2 * Frame.Load(int3(pos + int2(+0, -1), 0))
              + 1 * Frame.Load(int3(pos + int2(+1, 1), 0)) - 1 * Frame.Load(int3(pos + int2(+1, -1), 0));

    return float4(sqrt(gx.rgb * gx.rgb + gy.rgb * gy.rgb), 1.0f);
}
