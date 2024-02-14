#include "LightHelper.hlsl"

cbuffer cbPerObject : register(b0)
{
	float4x4 gViewProj;
	Material gMaterial;
};

cbuffer cbPerFrame : register(b1)
{
	DirectionLight gDirLights[3];
	float3 gEyePosW;
	float  gFogStart;
	float4 gFogColor;
	float  gFogRange;
	float gMinDist;
	float gMaxDist;
	float gMinTess;
	float gMaxTess;
	float gTexelCellSpaceU;
	float gTexelCellSpaceV;
	float gWorldCellSpace;
	float4 gWorldFrustumPlanes[6];
	float2 gTexScale;// = 50.0f;
};

Texture2DArray gLayerMapArray : register(t0);
Texture2D gBlendMap : register(t1);
Texture2D gHeightMap : register(t2);
SamplerState gSamHeightmap : register(s0);
SamplerState gSamLinear : register(s1);

struct VertexIn
{
	float3 PosL     : POSITION;
	float2 Tex      : TEXCOORD0;
	float2 BoundsY  : TEXCOORD1;
};

struct VertexOut
{
	float3 PosW     : POSITION;
	float2 Tex      : TEXCOORD0;
	float2 BoundsY  : TEXCOORD1;
};

VertexOut VS(VertexIn vin)
{
	VertexOut vout;

	vout.PosW = vin.PosL;
	vout.PosW.y = gHeightMap.SampleLevel(gSamHeightmap, vin.Tex, 0).r;
	vout.Tex = vin.Tex;
	vout.BoundsY = vin.BoundsY;

	return vout;
}

float CalcTessFactor(float3 p)
{
	float d = distance(p, gEyePosW);
	float s = saturate((d - gMinDist) / (gMaxDist - gMinDist));

	return pow(2, (lerp(gMaxTess, gMinTess, s)));
}

bool AabbBehindPlaneTest(float3 center, float3 extents, float4 plane)
{
	float3 n = abs(plane.xyz);
	float r = dot(extents, n);
	float s = dot(float4(center, 1.0f), plane);

	return (s + r) < 0.0f;
}

bool AabbOutsideFrustumTest(float3 center, float3 extents, float4 frustumPlanes[6])
{
	for (int i = 0; i < 6; ++i)
	{
		if (AabbBehindPlaneTest(center, extents, frustumPlanes[i]))
		{
			return true;
		}
	}

	return false;
}

struct PatchTess
{
	float EdgeTess[4]   : SV_TessFactor;
	float InsideTess[2] : SV_InsideTessFactor;
};

PatchTess main(InputPatch<VertexOut, 4> patch, uint patchID : SV_PrimitiveID)
{
	PatchTess pt;

	float minY = patch[0].BoundsY.x;
	float maxY = patch[0].BoundsY.y;

	float3 vMin = float3(patch[2].PosW.x, minY, patch[2].PosW.z);
	float3 vMax = float3(patch[1].PosW.x, maxY, patch[1].PosW.z);

	float3 boxCenter = 0.5f * (vMin + vMax);
	float3 boxExtents = 0.5f * (vMax - vMin);
	if (AabbOutsideFrustumTest(boxCenter, boxExtents, gWorldFrustumPlanes))
	{
		pt.EdgeTess[0] = 0.0f;
		pt.EdgeTess[1] = 0.0f;
		pt.EdgeTess[2] = 0.0f;
		pt.EdgeTess[3] = 0.0f;

		pt.InsideTess[0] = 0.0f;
		pt.InsideTess[1] = 0.0f;

		return pt;
	}
	else
	{
		float3 e0 = 0.5f * (patch[0].PosW + patch[2].PosW);
		float3 e1 = 0.5f * (patch[0].PosW + patch[1].PosW);
		float3 e2 = 0.5f * (patch[1].PosW + patch[3].PosW);
		float3 e3 = 0.5f * (patch[2].PosW + patch[3].PosW);
		float3  c = 0.25f * (patch[0].PosW + patch[1].PosW + patch[2].PosW + patch[3].PosW);

		pt.EdgeTess[0] = CalcTessFactor(e0);
		pt.EdgeTess[1] = CalcTessFactor(e1);
		pt.EdgeTess[2] = CalcTessFactor(e2);
		pt.EdgeTess[3] = CalcTessFactor(e3);

		pt.InsideTess[0] = CalcTessFactor(c);
		pt.InsideTess[1] = pt.InsideTess[0];

		return pt;
	}
}

struct HullOut
{
	float3 PosW     : POSITION;
	float2 Tex      : TEXCOORD0;
};

[domain("quad")]
[partitioning("fractional_even")]
[outputtopology("triangle_cw")]
[outputcontrolpoints(4)]
[patchconstantfunc("main")]
[maxtessfactor(64.0f)]
HullOut HS(InputPatch<VertexOut, 4> p,
	uint i : SV_OutputControlPointID,
	uint patchId : SV_PrimitiveID)
{
	HullOut hout;

	hout.PosW = p[i].PosW;
	hout.Tex = p[i].Tex;

	return hout;
}

struct DomainOut
{
	float4 PosH     : SV_POSITION;
	float3 PosW     : POSITION;
	float2 Tex      : TEXCOORD0;
	float2 TiledTex : TEXCOORD1;
};

[domain("quad")]
DomainOut DS(PatchTess patchTess,
	float2 uv : SV_DomainLocation,
	const OutputPatch<HullOut, 4> quad)
{
	DomainOut dout;

	dout.PosW = lerp(
		lerp(quad[0].PosW, quad[1].PosW, uv.x),
		lerp(quad[2].PosW, quad[3].PosW, uv.x),
		uv.y);

	dout.Tex = lerp(
		lerp(quad[0].Tex, quad[1].Tex, uv.x),
		lerp(quad[2].Tex, quad[3].Tex, uv.x),
		uv.y);

	dout.TiledTex = dout.Tex * gTexScale;

	dout.PosW.y = gHeightMap.SampleLevel(gSamHeightmap, dout.Tex, 0).r;

	dout.PosH = mul(float4(dout.PosW, 1.0f), gViewProj);

	return dout;
}

float4 PS(DomainOut pin) : SV_Target
{
	float2 leftTex = pin.Tex + float2(-gTexelCellSpaceU, 0.0f);
	float2 rightTex = pin.Tex + float2(gTexelCellSpaceU, 0.0f);
	float2 bottomTex = pin.Tex + float2(0.0f, gTexelCellSpaceV);
	float2 topTex = pin.Tex + float2(0.0f, -gTexelCellSpaceV);

	float leftY = gHeightMap.SampleLevel(gSamHeightmap, leftTex, 0).r;
	float rightY = gHeightMap.SampleLevel(gSamHeightmap, rightTex, 0).r;
	float bottomY = gHeightMap.SampleLevel(gSamHeightmap, bottomTex, 0).r;
	float topY = gHeightMap.SampleLevel(gSamHeightmap, topTex, 0).r;

	float3 tangent = normalize(float3(2.0f * gWorldCellSpace, rightY - leftY, 0.0f));
	float3 bitan = normalize(float3(0.0f, bottomY - topY, -2.0f * gWorldCellSpace));
	float3 normalW = cross(tangent, bitan);

	float3 toEye = gEyePosW - pin.PosW;

	float distToEye = length(toEye);

	toEye /= distToEye;

	float4 c0 = gLayerMapArray.Sample(gSamLinear, float3(pin.TiledTex, 0.0f));
	float4 c1 = gLayerMapArray.Sample(gSamLinear, float3(pin.TiledTex, 1.0f));
	float4 c2 = gLayerMapArray.Sample(gSamLinear, float3(pin.TiledTex, 2.0f));
	float4 c3 = gLayerMapArray.Sample(gSamLinear, float3(pin.TiledTex, 3.0f));
	float4 c4 = gLayerMapArray.Sample(gSamLinear, float3(pin.TiledTex, 4.0f));

	float4 t = gBlendMap.Sample(gSamLinear, pin.Tex);

	float4 texColor = c0;
	texColor = lerp(texColor, c1, t.r);
	texColor = lerp(texColor, c2, t.g);
	texColor = lerp(texColor, c3, t.b);
	texColor = lerp(texColor, c4, t.a);


	float4 litColor = texColor;
	float4 ambient = float4(0.0f, 0.0f, 0.0f, 0.0f);
	float4 diffuse = float4(0.0f, 0.0f, 0.0f, 0.0f);
	float4 spec = float4(0.0f, 0.0f, 0.0f, 0.0f);

	[unroll]
	for (int i = 0; i < 1; ++i)
	{
		float4 A, D, S;
		ComputeDirectionLight(gMaterial, gDirLights[i], normalW, toEye,
			A, D, S);

		ambient += A;
		diffuse += D;
		spec += S;
	}

	// litColor = texColor * (ambient + diffuse) + spec;
	litColor = (ambient + diffuse) + spec;

	float fogLerp = saturate((distToEye - gFogStart) / gFogRange);

	litColor = lerp(litColor, gFogColor, fogLerp);

	return litColor;
}
