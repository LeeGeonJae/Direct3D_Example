cbuffer cbPerFrame : register(b0)
{
	float gTexelWidth;
	float gTexelHeight;
	bool gHorizontalBlur;
};

Texture2D gNormalDepthMap : register(t0);
Texture2D gInputImage : register(t1);
SamplerState samPoint : register(s0);

struct VertexIn
{
	float3 PosL    : POSITION;
	float3 NormalL : NORMAL;
	float2 Tex     : TEXCOORD;
};

struct VertexOut
{
    float4 PosH  : SV_POSITION;
	float2 Tex   : TEXCOORD;
};

VertexOut VS(VertexIn vin)
{
	VertexOut vout;
	
	// 차폐맵을 화면을 모두 감싸는 사각형으로 다시 그린다.
	vout.PosH = float4(vin.PosL, 1.0f);
	vout.Tex = vin.Tex;
	
    return vout;
}

float4 PS(VertexOut pin) : SV_Target
{
	float gWeights[11] = 
	{	
		0.05f, 0.05f, 0.1f, 0.1f, 0.1f, 0.2f, 0.1f, 0.1f, 0.1f, 0.05f, 0.05f
	};
	const int gBlurRadius = 5;

	float2 texOffset;

	if (gHorizontalBlur)
	{
		texOffset = float2(gTexelWidth, 0.0f);
	}
	else
	{
		texOffset = float2(0.0f, gTexelWidth);
	}

	// 중앙 값은 항상 총합에 기여하도록 미리 처리한다.
	float4 color      = gWeights[5] * gInputImage.SampleLevel(samPoint, pin.Tex, 0.0);
	float totalWeight = gWeights[5];
	 
	// 법선/깊이 데이터 샘플링
	float4 centerNormalDepth = gNormalDepthMap.SampleLevel(samPoint, pin.Tex, 0.0f);

	for(float i = -gBlurRadius; i <= gBlurRadius; ++i)
	{
		// 중앙값은 다시 처리하지 않는다.
		if( i == 0 )
			continue;

		float2 tex = pin.Tex + i * texOffset;
		float4 neighborNormalDepth = gNormalDepthMap.SampleLevel(samPoint, tex, 0.0f);

		// 중앙과 이웃의 법선이 너무 차이나거나 깊이 차이가 너무 크면 해당 표본은 흐리기에서 제외한다.
		if( dot(neighborNormalDepth.xyz, centerNormalDepth.xyz) < 0.8f &&
		    abs(neighborNormalDepth.a - centerNormalDepth.a) > 0.2f )
		{
			continue;
		}

		float weight = gWeights[i + gBlurRadius];

		color += weight * gInputImage.SampleLevel(samPoint, tex, 0.0);
		totalWeight += weight;
	}

	// 적용된 가중치들과 합을 나눠준다.
	return color / totalWeight;
}