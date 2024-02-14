#include "../Resource/Shader/LightHelper.hlsl"

cbuffer cbPerObject : register(b0)
{
	float4x4 gWorld;
	float4x4 gWorldInvTranspose;
	float4x4 gWorldViewProj;
	float4x4 gWorldViewProjTex;
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
    bool gUseTexure;
};

Texture2D gDiffuseMap : register(t0);
Texture2D gNormalMap : register(t1);
Texture2D gSsaoMap : register(t2);

SamplerState samLinear : register(s0);

struct VertexIn
{
	float3 PosL     : POSITION;
	float3 NormalL  : NORMAL;
	float2 Tex      : TEXCOORD;
	float3 TangentL : TANGENT;
};

struct VertexOut
{
	float4 PosH       : SV_POSITION;
    float3 PosW       : POSITION;
    float3 NormalW    : NORMAL;
	float3 TangentW   : TANGENT;
	float2 Tex        : TEXCOORD0;
	float4 SsaoPosH   : TEXCOORD1;
};

VertexOut VS(VertexIn vin)
{
	VertexOut vout;
	
	vout.PosW     = mul(float4(vin.PosL, 1.0f), gWorld).xyz;
	vout.NormalW  = mul(vin.NormalL, (float3x3)gWorldInvTranspose);
	vout.TangentW = mul(vin.TangentL, (float3x3)gWorld);
	vout.PosH = mul(float4(vin.PosL, 1.0f), gWorldViewProj);
	vout.Tex = mul(float4(vin.Tex, 0.0f, 1.0f), gTexTransform).xy;
	vout.SsaoPosH = mul(float4(vin.PosL, 1.0f), gWorldViewProjTex);

	return vout;
}
 
float4 PS(VertexOut pin) : SV_Target
{
	pin.NormalW = normalize(pin.NormalW);
	float3 toEye = gEyePosW - pin.PosW;
	float distToEye = length(toEye);
	toEye /= distToEye;
	
    float4 texColor = float4(1, 1, 1, 1);
    if(gUseTexure)
	{
		texColor = gDiffuseMap.Sample( samLinear, pin.Tex );

        clip(texColor.a - 0.1f);
	}

    // 노말 매핑하지 않아도 되지 않을까? 하는 게 이쁠 거 같긴하지만
	float3 normalMapSample = gNormalMap.Sample(samLinear, pin.Tex).rgb;
	float3 bumpedNormalW = NormalSampleToWorldSpace(normalMapSample, pin.NormalW, pin.TangentW);
	 
	float4 litColor = texColor;
	if( gLightCount > 0  )
	{   
		float4 ambient = float4(0.0f, 0.0f, 0.0f, 0.0f);
		float4 diffuse = float4(0.0f, 0.0f, 0.0f, 0.0f);
		float4 spec    = float4(0.0f, 0.0f, 0.0f, 0.0f);
		  
		pin.SsaoPosH /= pin.SsaoPosH.w;
		float ambientAccess = gSsaoMap.Sample(samLinear, pin.SsaoPosH.xy, 0.0f).r;
		
		[unroll] 
		for(int i = 0; i < gLightCount; ++i)
		{
			float4 A, D, S;
			ComputeDirectionLight(gMaterial, gDirLights[i], bumpedNormalW, toEye, A, D, S);

			ambient += ambientAccess * A;    
			diffuse += D;
			spec    += S;
		}
		   
		litColor = texColor*(ambient + diffuse) + spec;
	}
 
	float fogLerp = saturate( (distToEye - gFogStart) / gFogRange ); 
	litColor = lerp(litColor, gFogColor, fogLerp);
	litColor.a = gMaterial.Diffuse.a * texColor.a;

    return litColor;
}
