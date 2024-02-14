#include "../Resource/Shader/LightHelper.hlsl"

cbuffer cbPerObject : register(b0)
{
	float4x4 gWorld;
	float4x4 gWorldInvTranspose;
	float4x4 gViewProj;
	float4x4 gTexTransform;
	Material gMaterial;
};

cbuffer cbPerFrame : register(b1)
{
	DirectionLight gDirLights[3];
	float3 gEyePosW;
	int gLightCount;
	float4 gFogColor;
	float  gFogStart;
	float  gFogRange;
	bool gbUseTexture;
};

struct VertexIn
{
	float3 PosL     : POSITION;
	float3 NormalL  : NORMAL;
	float2 Tex      : TEXCOORD;
	row_major float4x4 World  : WORLD; // 인스턴싱을 위한 데이터들
	float4 Color    : COLOR;		// 인스턴싱을 위한 데이터들
	uint InstanceId : SV_InstanceID; // 인스턴스별 ID
};

struct VertexOut
{
	float4 PosH    : SV_POSITION;
	float3 PosW    : POSITION;
	float3 NormalW : NORMAL;
	float2 Tex     : TEXCOORD;
	float4 Color   : COLOR;
};

VertexOut VS(VertexIn vin)
{
	VertexOut vout;

	vout.PosW = mul(float4(vin.PosL, 1.0f), vin.World).xyz;
	vout.NormalW = mul(vin.NormalL, (float3x3)vin.World);

	vout.PosH = mul(float4(vout.PosW, 1.0f), gViewProj);

	vout.Tex = mul(float4(vin.Tex, 0.0f, 1.0f), gTexTransform).xy;
	vout.Color = vin.Color;

	return vout;
}

float4 PS(VertexOut pin) : SV_Target
{
	pin.NormalW = normalize(pin.NormalW);

	float3 toEye = gEyePosW - pin.PosW;

	float distToEye = length(toEye);
	toEye /= distToEye;
	float4 texColor = float4(1, 1, 1, 1);

	float4 litColor = texColor;
	if (gLightCount > 0)
	{
		float4 ambient = float4(0.0f, 0.0f, 0.0f, 0.0f);
		float4 diffuse = float4(0.0f, 0.0f, 0.0f, 0.0f);
		float4 spec = float4(0.0f, 0.0f, 0.0f, 0.0f);

		[unroll]
		for (int i = 0; i < gLightCount; ++i)
		{
			float4 A, D, S;
			ComputeDirectionLight(gMaterial, gDirLights[i],	pin.NormalW, toEye,
				A, D, S);

			ambient += A * pin.Color;
			diffuse += D * pin.Color;
			spec += S;
		}

		litColor = texColor * (ambient + diffuse) + spec;
	}
	
	float fogLerp = saturate((distToEye - gFogStart) / gFogRange);

	litColor = lerp(litColor, gFogColor, fogLerp);
	litColor.a = gMaterial.Diffuse.a * texColor.a;
	
	return litColor;
}
