cbuffer cbPerFrame : register(b0)
{
	float4x4 gViewToTexSpace; // Proj*Texture
	float4   gOffsetVectors[14];
	float4   gFrustumCorners[4];
};
 
Texture2D gNormalDepthMap : register(t0);
Texture2D gRandomVecMap : register(t1);
 
SamplerState samNormalDepth : register(s0);
SamplerState samRandomVec : register(s1);

struct VertexIn
{
	float3 PosL            : POSITION;
	float3 ToFarPlaneIndex : NORMAL;
	float2 Tex             : TEXCOORD;
};

struct VertexOut
{
    float4 PosH       : SV_POSITION;
    float3 ToFarPlane : TEXCOORD0;
	float2 Tex        : TEXCOORD1;
};

VertexOut VS(VertexIn vin)
{
	VertexOut vout;
	
	// 정규화된 값을 그대로 전달하고, 인덱스를 통해 먼 평면 정보를 같이 전달한다.
	vout.PosH = float4(vin.PosL, 1.0f);
	vout.ToFarPlane = gFrustumCorners[vin.ToFarPlaneIndex.x].xyz;
	vout.Tex = vin.Tex;
	
    return vout;
}

int gSampleCount = 14;
float    gSurfaceEpsilon     = 0.05f;
float    gOcclusionRadius    = 0.5f;
float    gOcclusionFadeStart = 1.0f;
float    gOcclusionFadeEnd   = 2.0f;

float OcclusionFunction(float distZ)
{
	float occlusion = 0.0f;

	// 차폐도가 판정되려면 최소한 앱실론보다 앞에 존재해야 한다.
	if(distZ > gSurfaceEpsilon)
	{
		// 거리가 멀수록 차폐도가 작도록 하고, 정규화해준다.
		float fadeLength = gOcclusionFadeEnd - gOcclusionFadeStart; 
		occlusion = saturate( (gOcclusionFadeEnd-distZ) / fadeLength ); 
	}
	
	return occlusion;	
}

float4 PS(VertexOut pin) : SV_Target
{
	// 화면에 보이는 픽셀의 노말과 깊이값을 읽는다.
	float4 normalDepth = gNormalDepthMap.SampleLevel(samNormalDepth, pin.Tex, 0.0f);
	float3 n = normalDepth.xyz;
	float pz = normalDepth.w;
	
	// 보간된 먼 평면 거리로 현재 위치를 읽어온다.
	float depthRatio = pz / pin.ToFarPlane.z;
	float3 p = depthRatio * pin.ToFarPlane;
	
	// 랜덤한 벡터를 읽어와서 -1, 1로 매핑한다.
	float3 randVec = 2.0f * gRandomVecMap.SampleLevel(samRandomVec, 4.0f * pin.Tex, 0.0f).rgb - 1.0f;

	float occlusionSum = 0.0f;

	[unroll]
	for(int i = 0; i < gSampleCount; ++i)
	{
		// 랜덤 벡터에 반사시켜 고르게 분포된 무작위 벡터를 얻는다.
		float3 offset = reflect(gOffsetVectors[i].xyz, randVec);
	
		// 오프셋 벡터가 평면의 반대를 향하고 있다면 뒤집는다.
		float flip = sign( dot(offset, n) );
		float3 q = p + flip * gOcclusionRadius * offset;
		
		// 임의로 만든 표본을 텍스처 공간으로 변환한다.
		float4 projQ = mul(float4(q, 1.0f), gViewToTexSpace);
		projQ /= projQ.w;

		// 깊이맵을 샘플링하여 실제 잠재적인 차폐점을 구한다.
		float rz = gNormalDepthMap.SampleLevel(samNormalDepth, projQ.xy, 0.0f).a;
		float3 r = (rz / q.z) * q; 

		// 깊이 차이를 계산한다.
		float distZ = p.z - r.z;

		// 같은 평면에 있는지 검사한다. 두 벡터가 같은 평면에 있으면 차폐점이 아님!
		float dp = max(dot(n, normalize(r - p)), 0.0f);
		
		// 차폐도를 누적시킨다.
		float occlusion = dp * OcclusionFunction(distZ);
		occlusionSum += occlusion;
	}
	
	occlusionSum /= gSampleCount;
	
	// 도달도를 구해준다.
	float access = 1.0f - occlusionSum;

	// 거듭 제곱 수를 올리면 더 가파른 모양을 보여 대비가 강해진다.
	return saturate(pow(access,4));
}
