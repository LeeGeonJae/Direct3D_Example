
cbuffer cbPerFrame : register(b0)
{
	float4x4 gWorldViewProj;
};

TextureCube gCubeMap : register(t0);
SamplerState gSamLinear : register(s0);

struct VertexIn
{
	float3 PosL : POSITION;
};

struct VertexOut
{
	float4 PosH : SV_POSITION;
	float3 PosL : POSITION;
};

VertexOut VS(VertexIn vin)
{
	VertexOut vout;

	vout.PosH = mul(float4(vin.PosL, 1.0f), gWorldViewProj).xyww;

	vout.PosL = vin.PosL;

	return vout;
}

float4 PS(VertexOut pin) : SV_Target
{
	return gCubeMap.Sample(gSamLinear, pin.PosL);
}
