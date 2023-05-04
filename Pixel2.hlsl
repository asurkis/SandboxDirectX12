Texture2D<float4> Frame : register(t0);

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

float4 main(VertexShaderOutput IN)
    : SV_Target
{
    // return Frame.Load(IN.UV);
    return float4(1.0f, 0.0f, 0.0f, 1.0f);
}
