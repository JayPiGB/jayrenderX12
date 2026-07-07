struct VSInput
{
    float3 position : POSITION;
};

struct PSInput
{
    float4 position : SV_Position;
};

PSInput VSMain(VSInput input)
{
    PSInput result;
    result.position = float4(input.position, 1.0);
    return result;
}

float4 PSMain(PSInput input) : SV_Target
{
    return float4(1.0, 1.0, 1.0, 1.0);
}
