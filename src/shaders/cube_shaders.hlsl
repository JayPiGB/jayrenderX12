Texture2D g_texture : register(t0);
SamplerState g_sampler : register(s0);

cbuffer SceneCB : register(b0)
{
    float4x4 view;
    float4x4 proj;
}

cbuffer ModelCB : register(b1)
{
    float4x4 model;
}

struct VSInput
{
    float3 position : POSITION;
    float3 color: COLOR;
    float2 uv : UV;
};

struct PSInput
{
    float4 position : SV_Position;
    float3 color : COLOR;
    float2 uv : UV;
};

PSInput VSMain(VSInput input)
{
    PSInput result;
    
    float4x4 mvp = mul(mul(model, view), proj);
    result.position = mul(float4(input.position, 1.0), mvp);
    
    result.color = input.color;
    result.uv = input.uv;
    
    return result;
}

float4 PSMain(PSInput input) : SV_Target
{
    //return float4(input.color, 1.0);
    float3 lightColor = float3(1.0, 1.0, 1.0);
    
    float ambientStr = 0.1;
    float3 ambient = ambientStr * lightColor;
    
    float4 objectColor = g_texture.Sample(g_sampler, input.uv);
    
    return float4(ambient, 1.0) * objectColor;
}