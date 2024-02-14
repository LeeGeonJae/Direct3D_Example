#include "LightHelper.hlsl"

cbuffer cbPerFrame : register(b0)
{
	DirectionLight gDirLight;
	PointLight gPointLight;
	SpotLight gSpotLight;
	float3 gEyePosW;
}

cbuffer cbPerObject : register(b1)
{
	float4x4 gWorld;
	float4x4 gWorldInvTranspose;
	float4x4 gWorldViewProj;
	Material gMaterial;
}

struct PS_INPUT
{
	float4 PosH    : SV_POSITION;
	float3 PosW    : POSITION;
	float3 NormalW : NORMAL;
};

float4 main(PS_INPUT pin) : SV_Target
{
	pin.NormalW = normalize(pin.NormalW);

	float3 toEyeW = normalize(gEyePosW - pin.PosW); // object -> eye vec

	float4 ambient = float4(0.0f, 0.0f, 0.0f, 0.0f);
	float4 diffuse = float4(0.0f, 0.0f, 0.0f, 0.0f);
	float4 specular = float4(0.0f, 0.0f, 0.0f, 0.0f);

	float4 outAmbient;
	float4 outDiffuse;
	float4 outSpecular;

	ComputeDirectionLight(gMaterial, gDirLight, pin.NormalW, toEyeW, outAmbient, outDiffuse, outSpecular);
	ambient += outAmbient;
	diffuse += outDiffuse;
	specular += outSpecular;

	ComputePointLight(gMaterial, gPointLight, pin.PosW, pin.NormalW, toEyeW, outAmbient, outDiffuse, outSpecular);
	ambient += outAmbient;
	diffuse += outDiffuse;
	specular += outSpecular;

	ComputeSpotLight(gMaterial, gSpotLight, pin.PosW, pin.NormalW, toEyeW, outAmbient, outDiffuse, outSpecular);
	ambient += outAmbient;
	diffuse += outDiffuse;
	specular += outSpecular;

	// 일반적으로 diffuse 재질에서 알파를 가져온다고 함
	float4 litColor = ambient + diffuse + specular;
	litColor.a = gMaterial.Diffuse.a;

	return litColor;
}