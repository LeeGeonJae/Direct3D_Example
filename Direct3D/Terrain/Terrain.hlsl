#include "../Resource/Shader/LightHelper.hlsl"

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

	// 높이맵을 샘플링해서 반영해준다.
	vout.PosW = vin.PosL;
	vout.PosW.y = gHeightMap.SampleLevel(gSamHeightmap, vin.Tex, 0).r;
	vout.Tex = vin.Tex;
	vout.BoundsY = vin.BoundsY;

	return vout;
}

float CalcTessFactor(float3 p)
{
	float d = distance(p, gEyePosW);
	float ratio = saturate((d - gMinDist) / (gMaxDist - gMinDist));

	// 세부수준의 증가에 따라 두 배씩 계수를 상승시킨다.
	// 거리에 따른 분포가 더 잘된다고 한다.

	return pow(2, (lerp(gMaxTess, gMinTess, ratio)));
}

bool AabbBehindPlaneTest(float3 center, float3 extents, float4 plane)
{
	float3 n = abs(plane.xyz);
	float r = dot(extents, n); // 대각선 거리
	float s = dot(float4(center, 1.0f), plane); // 중점에서 평면까지의 거리

	return (s + r) < 0.0f; // 사각형의 가장 긴 거리와 차이값의 합이 0보다 작다면 외부이다.
}

bool AabbOutsideFrustumTest(float3 center, float3 extents, float4 frustumPlanes[6])
{
	for (int i = 0; i < 6; ++i)
	{
		// 한 면이라도 바깥에 있으면 상자는 절두체 바깥 쪽이다.
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

PatchTess ConstantHS(InputPatch<VertexOut, 4> patch, uint patchID : SV_PrimitiveID)
{
	PatchTess pt;

	// boundY로부터 최소/최대값을 가져온다.
	float minY = patch[0].BoundsY.x;
	float maxY = patch[0].BoundsY.y;

	// 패치에 대한 aabb 정보를 생성한다.
	float3 vMin = float3(patch[2].PosW.x, minY, patch[2].PosW.z);
	float3 vMax = float3(patch[1].PosW.x, maxY, patch[1].PosW.z);
	float3 boxCenter = 0.5f * (vMin + vMax);
	float3 boxExtents = 0.5f * (vMax - vMin);

	// 절두체 선별 실패 시 테셀레이션 계수를 0으로 하여 더 이상 처리되지 않게 한다.
	// 모든 테셀레이션 계수가 0이 되면 단계가 더 이상 진행되지 않는다.
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

	// 보간으로 선분의 중앙과 패치의 중앙점을 찾는다.
	float3 e0 = 0.5f * (patch[0].PosW + patch[2].PosW); 
	float3 e1 = 0.5f * (patch[0].PosW + patch[1].PosW);
	float3 e2 = 0.5f * (patch[1].PosW + patch[3].PosW);
	float3 e3 = 0.5f * (patch[2].PosW + patch[3].PosW);
	float3  c = 0.25f * (patch[0].PosW + patch[1].PosW + patch[2].PosW + patch[3].PosW);

	// 거리에 따른 테셀레이션 계수 할당
	pt.EdgeTess[0] = CalcTessFactor(e0);
	pt.EdgeTess[1] = CalcTessFactor(e1);
	pt.EdgeTess[2] = CalcTessFactor(e2);
	pt.EdgeTess[3] = CalcTessFactor(e3);
	pt.InsideTess[0] = CalcTessFactor(c);
	pt.InsideTess[1] = pt.InsideTess[0];

	return pt;
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
[patchconstantfunc("ConstantHS")]
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

	// 겹선형 보간으로 정점 데이터 생성
	dout.PosW = lerp(lerp(quad[0].PosW, quad[1].PosW, uv.x),
					lerp(quad[2].PosW, quad[3].PosW, uv.x),
					uv.y);
	dout.Tex = lerp(lerp(quad[0].Tex, quad[1].Tex, uv.x),
					lerp(quad[2].Tex, quad[3].Tex, uv.x),
					uv.y);

	// 샘플링 비율, wrap이니 높으면 타일링이 촘촘해진다.
	dout.TiledTex = dout.Tex * gTexScale;

	// 최상위 밉맵 수준으로 높이맵을 샘플링한다.
	dout.PosW.y = gHeightMap.SampleLevel(gSamHeightmap, dout.Tex, 0).r;

	// 투영 변환
	dout.PosH = mul(float4(dout.PosW, 1.0f), gViewProj);

	return dout;
}

float4 PS(DomainOut pin) : SV_Target
{
	// 현재 픽셀에 인접한 uv값을 가져온다. 
	float2 leftTex = pin.Tex + float2(-gTexelCellSpaceU, 0.0f);
	float2 rightTex = pin.Tex + float2(gTexelCellSpaceU, 0.0f);
	float2 bottomTex = pin.Tex + float2(0.0f, gTexelCellSpaceV);
	float2 topTex = pin.Tex + float2(0.0f, -gTexelCellSpaceV);

	// 현재 픽셀에서 인접한 높이값 4개를 읽어온다.
	float leftY = gHeightMap.SampleLevel(gSamHeightmap, leftTex, 0).r;
	float rightY = gHeightMap.SampleLevel(gSamHeightmap, rightTex, 0).r;
	float bottomY = gHeightMap.SampleLevel(gSamHeightmap, bottomTex, 0).r;
	float topY = gHeightMap.SampleLevel(gSamHeightmap, topTex, 0).r;

	// 세계 공간에서 낱칸들 사이의 간격으로 중심 차분법을 이용해 법선 벡터를 구한다.
	float3 tangent = normalize(float3(2.0f * gWorldCellSpace, rightY - leftY, 0.0f));
	float3 bitan = normalize(float3(0.0f, bottomY - topY, -2.0f * gWorldCellSpace));
	float3 normalW = cross(tangent, bitan);

	float3 toEye = gEyePosW - pin.PosW;

	float distToEye = length(toEye);

	toEye /= distToEye;

	// 멀티 텍스처를 로딩해서 블렌드맵에 가중치에 따라 선형보간
	float4 c0 = gLayerMapArray.Sample(gSamLinear, float3(pin.TiledTex, 0.0f));
	float4 c1 = gLayerMapArray.Sample(gSamLinear, float3(pin.TiledTex, 1.0f));
	float4 c2 = gLayerMapArray.Sample(gSamLinear, float3(pin.TiledTex, 2.0f));
	float4 c3 = gLayerMapArray.Sample(gSamLinear, float3(pin.TiledTex, 3.0f));
	float4 c4 = gLayerMapArray.Sample(gSamLinear, float3(pin.TiledTex, 4.0f));

	// 블랜드맵은 지형 전체를 덮는 대략적인 형태의 맵이므로 확대되지 않은 uv값을 사용한다.
	float4 t = gBlendMap.Sample(gSamLinear, pin.Tex);

	float4 texColor = c0;
	texColor = lerp(texColor, c1, t.r);
	texColor = lerp(texColor, c2, t.g);
	texColor = lerp(texColor, c3, t.b);
	texColor = lerp(texColor, c4, t.a);

	// 조명처리 + fog 처리

	float4 litColor = texColor;
	float4 ambient = float4(0.0f, 0.0f, 0.0f, 0.0f);
	float4 diffuse = float4(0.0f, 0.0f, 0.0f, 0.0f);
	float4 spec = float4(0.0f, 0.0f, 0.0f, 0.0f);

	[unroll]
	for (int i = 0; i < 1; ++i)
	{
		float4 A, D, S;

		ComputeDirectionLight(gMaterial,
			gDirLights[i],
			normalW, 
			toEye,
			A, D, S);

		ambient += A;
		diffuse += D;
		spec += S;
	}

	litColor = texColor * (ambient + diffuse) + spec;
	// litColor = (ambient + diffuse) + spec;

	float fogLerp = saturate((distToEye - gFogStart) / gFogRange);

	litColor = lerp(litColor, gFogColor, fogLerp);

	return litColor;
}
