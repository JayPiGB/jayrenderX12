cbuffer SceneCB : register(b0)
{
    float4x4 view;
    float4x4 proj;
}

cbuffer QuadCB : register(b1)
{
    float4x4 model;
}

struct VSInput
{
    float3 position : POSITION;
    float3 color : COLOR;
};

struct PSInput
{
    float4 position : SV_Position;
    float3 color : COLOR;
};

PSInput VSMain(VSInput input)
{
    PSInput result;
    
    float4x4 mvp = mul(mul(model, view), proj);
    result.position = mul(float4(input.position, 1.0), mvp);
    
    result.color = input.color;
    return result;
}

float4 PSMain(PSInput input) : SV_Target
{
    return float4(input.color, 1.0);
}
