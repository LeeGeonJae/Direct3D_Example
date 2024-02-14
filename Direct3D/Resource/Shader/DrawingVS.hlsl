
struct VS_INPUT
{
	float3 position : POSITION;
	float4 color : COLOR;
};

struct VS_OUTPUT
{
	float4 position : SV_POSITION;
	float4 color : TEXCOORD0;
};

cbuffer cbPerObject : register(b0)
{
	float4x4 gWVP;
}

VS_OUTPUT main(VS_INPUT vin)
{
	VS_OUTPUT vout;

	vout.position = float4(vin.position, 1);

	vout.position = mul(vout.position, gWVP);

	vout.color = vin.color;

	return vout;
}