cbuffer TransformConstants : register(b0)
{
	float4x4 viewProjectionMatrix;
	float4x4 gSkyProjMatrix;
	float4x4 WorldMatrix;
};

struct PixelShaderInput
{
	float3 localPosition : POSITION;
	float4 pixelPosition : SV_POSITION;
};

TextureCube envTexture : register(t0);
SamplerState defaultSampler : register(s0);

PixelShaderInput VS(float3 position : POSITION)
{
	PixelShaderInput vout;
	vout.localPosition = position;
	vout.pixelPosition = mul(float4(position, 1.0), gSkyProjMatrix);
	return vout;
}

float4 PS(PixelShaderInput pin) : SV_Target
{
	float3 envVector = normalize(pin.localPosition);
	return envTexture.SampleLevel(defaultSampler, envVector, 0);
}
