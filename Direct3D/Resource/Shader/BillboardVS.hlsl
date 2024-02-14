#include "LightHelper.hlsl"

cbuffer cbPerObject : register(b0)
{
	float4x4 gViewProj;
	Material gMaterial;
};

struct VS_INPUT
{
	float3 PosW  : POSITION;
	float2 SizeW : SIZE;
};

struct VS_OUTPUT
{
	float3 CenterW : POSITION;
	float2 SizeW   : SIZE;
};

VS_OUTPUT main(VS_INPUT vin)
{
	VS_OUTPUT vout;

	vout.CenterW = vin.PosW;
	vout.SizeW   = vin.SizeW;

	return vout;
}