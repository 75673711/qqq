cbuffer ConstantBuffer : register(b0)
{
        float4x4 modelview;
        float4x4 projection;
        float opacity;
};

struct PSInput
{
        float4 position : SV_POSITION;
        float2 color : COLOR;
};

Texture2D g_texture : register(t0);
SamplerState g_sampler : register(s0);

// 顶点大小保持一致
PSInput VS_Simple(float2 position : POSITION, float3 color : COLOR)
{
        PSInput result;

		float4 temp_position = float4(position, 0.0, 1.0);
        float4x4 mvp = mul(projection, modelview);
        result.position = mul(mvp, temp_position);
        result.color = color;

        return result;
}

float4 PS_Simple(PSInput input) : SV_TARGET
{
	return g_texture.Sample(g_sampler, input.color);
    //return float4(input.color, 0.0, 1.0) * opacity;
}
