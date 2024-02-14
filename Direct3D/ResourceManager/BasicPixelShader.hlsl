struct PS_INPUT
{
	float4 position : SV_POSITION;
	float2 UV : TEXCOORD0;
	float3 viewDir : TEXCOORD1;
	float3 T : TEXCOORD2;
	float3 B : TEXCOORD3;
	float3 N : TEXCOORD4;
};

cbuffer ConstantBuffer : register(b0)
{
	float4 lightDirection;
	float4 lightColor;
};

cbuffer cbTexture : register(b1)
{
	bool gUseDiffuse;
	bool gUseNormal;
	bool gUseSpecular;
	bool gUseAlpha;
}

Texture2D txDiffuse : register(t0);
Texture2D txNormal : register(t1);
Texture2D txSpecular : register(t2);
Texture2D txAlpha : register(t3);
SamplerState samLinear : register(s0);

float4 main(PS_INPUT Input) : SV_TARGET
{
	float3 normal;

	if (gUseNormal)
	{
		normal = txNormal.Sample(samLinear, Input.UV).xyz;
		normal = normalize(normal * 2 - 1);
		float3x3 TBN = float3x3(normalize(Input.T), normalize(Input.B), normalize(Input.N));
		TBN = transpose(TBN);
		normal = mul(TBN, normal);
	}
	{
		normal = Input.N;
	}

	float4 albedo;
	float3 diffuse = saturate(dot(normal, -lightDirection));
	
	if (gUseDiffuse)
	{
		albedo = txDiffuse.Sample(samLinear, Input.UV);
		diffuse = lightColor * albedo.rgb * diffuse;
	}
	else
	{
		diffuse = lightColor * diffuse;
	}

	float3 specular = 0;
	if (diffuse.x > 0 && gUseSpecular)
	{
		float3 reflection = reflect(lightDirection, normal);
		float3 viewDir = normalize(Input.viewDir);
		
		specular = saturate(dot(reflection, -viewDir));
		specular = pow(specular, 10) * lightColor.xyz;

		float4 specularIntensity = txSpecular.Sample(samLinear, Input.UV);
		specular *= specularIntensity.xyz;
	}


	if (gUseAlpha)
	{
		albedo.a = txAlpha.Sample(samLinear, Input.UV).x; 
	}

	return float4(diffuse + specular, 1);
}
