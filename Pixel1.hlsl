struct VertexShaderOutput
{
    float4 Position : SV_Position;
    float4 Color : COLOR;
};

float4 main(VertexShaderOutput IN)
    : SV_Target
{
    return IN.Color;
}
