#include "../Resource/Shader/LightHelper.hlsl"

cbuffer cbPerObject : register(b0)
{
	float4x4 gWorld;
	float4x4 gWorldInvTranspose;
	float4x4 gViewProj;
	float4x4 gWorldViewProj;
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
	bool gUseLight;
	bool gUseFog;

	float gHeightScale; // 높이맵 가중치
	float gMaxTessDistance; // 테셀레이션 최대 거리
	float gMinTessDistance; // 테셀레이션 최소 거리
	float gMaxTessFactor; // 테셀레이션 최대 계수
	float gMinTessFactor; // 테셀레이션 최소 계수
}

Texture2D gDiffuseMap : register(t0);
Texture2D gNormalMap : register(t1);
TextureCube gCubeMap : register(t2);
SamplerState gSamLinear : register(s0);

struct VertexIn
{
	float3 PosL : POSITION;
	float3 NormalL : NORMAL;
	float2 Tex : TEXCOORD;
	float3 TangentL : TANGENT;
};

struct VertexOut
{
	float3 PosW : POSITION;
	float3 NormalW : NORMAL;
	float3 TangentW : TANGENT;
	float2 Tex : TEXCOORD;
	float TessFactor : TESS;
};

VertexOut VS(VertexIn vin)
{
	VertexOut vout;

	vout.PosW = mul(float4(vin.PosL, 1.f), gWorld).xyz;
	vout.NormalW = mul(vin.NormalL, (float3x3)gWorldInvTranspose);
	vout.TangentW = mul(vin.TangentL, (float3x3)gWorld);

	vout.Tex = mul(float4(vin.Tex, 0.f, 1.f), gTexTransform).xy;

	float d = distance(vout.PosW, gEyePosW);

	// 거리 비율 구한 후 계수에 가중치로 사용
	float tessRatio = saturate((gMinTessDistance - d) / (gMinTessDistance - gMaxTessDistance));
	vout.TessFactor = gMinTessFactor + tessRatio * (gMaxTessFactor - gMinTessFactor);

	return vout;
}

struct PatchTess
{
	float EdgeTess[3] : SV_TessFactor;
	float InsideTess : SV_InsideTessFactor;
};

PatchTess PatchHS(InputPatch<VertexOut, 3> patch, uint patchID : SV_PrimitiveID)
{
	PatchTess pt;

	// 정점 정보로부터 변 테셀레이션 계수를 정하므로
	// 메시로 인해 공유되는 변은 항상 같은 테셀레이션 계수를 갖음
	pt.EdgeTess[0] = 0.5f * (patch[1].TessFactor + patch[2].TessFactor);
	pt.EdgeTess[1] = 0.5f * (patch[2].TessFactor + patch[0].TessFactor);
	pt.EdgeTess[2] = 0.5f * (patch[0].TessFactor + patch[1].TessFactor);

	// 내부 테셀레이션 계수는 첫 계수로 처리했네?
	pt.InsideTess = pt.EdgeTess[0];

	return pt;
}

struct HullOut
{
	float3 PosW : POSITION;
	float3 NormalW : NORMAL;
	float3 TangentW : TANGENT;
	float2 Tex : TEXCOORD;
};

[domain("tri")] // 패치의 종류
[partitioning("fractional_odd")] // 테셀레이션 계수가 매끄럽게 처리
[outputtopology("triangle_cw")] // 감김 순서
[outputcontrolpoints(3)]
[patchconstantfunc("PatchHS")]
HullOut HS(InputPatch<VertexOut, 3> p, uint i : SV_OutputControlPointID, uint patchId : SV_PrimitiveID)
{
	HullOut hout;

	// 기하구조 자체에 변경이 있는 것은 아니므로 그냥 단순 패스하도록 처리
	hout.PosW = p[i].PosW;
	hout.NormalW = p[i].NormalW;
	hout.TangentW = p[i].TangentW;
	hout.Tex = p[i].Tex;

	return hout;
}

struct DomainOut
{
	float4 PosH : SV_POSITION;
	float3 PosW : POSITION;
	float3 NormalW : NORMAL;
	float3 TangentW : TANGENT;
	float2 Tex : TEXCOORD;
};

[domain("tri")]
DomainOut DS(PatchTess patchTess, float3 bary : SV_DomainLocation, const OutputPatch<HullOut, 3> tri)
{
	DomainOut dout;

	// 제어점으로부터 보간해서 정점 생성
	dout.PosW = bary.x * tri[0].PosW + bary.y * tri[1].PosW + bary.z * tri[2].PosW;
	dout.NormalW = bary.x * tri[0].NormalW + bary.y * tri[1].NormalW + bary.z * tri[2].NormalW;
	dout.TangentW = bary.x * tri[0].TangentW + bary.y * tri[1].TangentW + bary.z * tri[2].TangentW;
	dout.Tex = bary.x * tri[0].Tex + bary.y * tri[1].Tex + bary.z * tri[2].Tex;

	dout.NormalW = normalize(dout.NormalW);

	// 월드 위치와 시야 위치 사이의 거리를 MipInterval로 나눠 밉레벨을 선정
	// 인터벌을 한 번 빼줘서 처리하므로 거리 40미만은 0레벨이다.
	const float MipInterval = 20.f;
	float mipLevel = clamp((distance(dout.PosW, gEyePosW) - MipInterval) / MipInterval, 0.f, 6.f);

	float h = gNormalMap.SampleLevel(gSamLinear, dout.Tex, mipLevel).a;

	// 안쪽 방향으로 높이값만큼 파고듬
	// h는 1에서 뺴주므로 범위가 -1 ~ 0임
	dout.PosW += (gHeightScale * (h - 1.f)) * dout.NormalW;

	dout.PosH = mul(float4(dout.PosW, 1.f), gViewProj);

	return dout;
}

float4 PS(DomainOut pin) : SV_Target
{
	pin.NormalW = normalize(pin.NormalW);

	float3 toEye = gEyePosW - pin.PosW;
	float distanceToEye = length(toEye);
	toEye /= distanceToEye;

	float4 texColor = float4(1, 1, 1, 1);
	if (gUseTexure)
	{
		texColor = gDiffuseMap.Sample(gSamLinear, pin.Tex);

		clip(texColor.a - 0.1f);
	}

	float3 normalMapSample = gNormalMap.Sample(gSamLinear, pin.Tex).rgb;

	// 정규화된 월드 노말과 탄젠트만 전달한다
	float3 bumpedNormalW = NormalSampleToWorldSpace(normalMapSample, pin.NormalW, pin.TangentW);

	float4 litColor = texColor;
	if (gUseLight && gLightCount > 0)
	{
		float4 ambient = float4(0.f, 0.f, 0.f, 0.f);
		float4 diffuse = float4(0.f, 0.f, 0.f, 0.f);
		float4 spec = float4(0.f, 0.f, 0.f, 0.f);

		[unroll]
		for (int i = 0; i < gLightCount; ++i)
		{
			float4 A, D, S;
			ComputeDirectionLight(gMaterial, gDirLights[i], bumpedNormalW, toEye, A, D, S);

			ambient += A;
			diffuse += D;
			spec += S;
		}

		litColor = texColor * (ambient + diffuse) + spec;

		float3 incident = -toEye;
		float3 reflectionVector = reflect(incident, bumpedNormalW);
		float4 reflectionColor = gCubeMap.Sample(gSamLinear, reflectionVector);

		litColor += gMaterial.Reflect * reflectionColor;
	}

	if (gUseFog)
	{
		// 표면과 카메라까지의 거리와 포그 시작 거리를 뺌
		// 거리가 멀어지면 포그 시작거리를 빼도 엄청 큰 값임
		// 이걸 range로 나눠서 0 ~ 1값으로 정규화 시킬 거임
		float fogLerp = saturate((distanceToEye - gFogStart) / gFogRange);

		// 거리가 멀면 포그 색상으로 픽셀 색상이 결정 나므로 안개 같아 보임
		litColor = lerp(litColor, gFogColor, fogLerp);
	}

	litColor.a = gMaterial.Diffuse.a * texColor.a;

	return litColor;
}