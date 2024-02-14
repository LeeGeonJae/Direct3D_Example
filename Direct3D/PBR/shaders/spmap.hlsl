static const float PI = 3.141592;
static const float TwoPI = 2 * PI;
static const float Epsilon = 0.00001;

static const uint NumSamples = 1024;
static const float InvNumSamples = 1.0 / float(NumSamples);

cbuffer SpecularMapFilterSettings : register(b0)
{
	float roughness;
};

TextureCube inputTexture : register(t0);
RWTexture2DArray<float4> outputTexture : register(u0);

SamplerState defaultSampler : register(s0);

float radicalInverse_VdC(uint bits)
{
	bits = (bits << 16u) | (bits >> 16u);
	bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
	bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
	bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
	bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
	return float(bits) * 2.3283064365386963e-10; // / 0x100000000
}

float2 sampleHammersley(uint i)
{
	return float2(i * InvNumSamples, radicalInverse_VdC(i));
}

float3 sampleGGX(float u1, float u2, float roughness)
{
	float alpha = roughness * roughness;

	float cosTheta = sqrt((1.0 - u2) / (1.0 + (alpha*alpha - 1.0) * u2));
	float sinTheta = sqrt(1.0 - cosTheta*cosTheta); // Trig. identity
	float phi = TwoPI * u1;

	return float3(sinTheta * cos(phi), sinTheta * sin(phi), cosTheta);
}

float ndfGGX(float cosLh, float roughness)
{
	float alpha   = roughness * roughness;
	float alphaSq = alpha * alpha;

	float denom = (cosLh * cosLh) * (alphaSq - 1.0) + 1.0;
	return alphaSq / (PI * denom * denom);
}

float3 getSamplingVector(uint3 ThreadID)
{
	float outputWidth, outputHeight, outputDepth;
	outputTexture.GetDimensions(outputWidth, outputHeight, outputDepth);

    float2 st = ThreadID.xy/float2(outputWidth, outputHeight);
    float2 uv = 2.0 * float2(st.x, 1.0-st.y) - 1.0;

	float3 ret;
	switch(ThreadID.z)
	{
		case 0: ret = float3(1.0,  uv.y, -uv.x); break;
		case 1: ret = float3(-1.0, uv.y,  uv.x); break;
		case 2: ret = float3(uv.x, 1.0, -uv.y); break;
		case 3: ret = float3(uv.x, -1.0, uv.y); break;
		case 4: ret = float3(uv.x, uv.y, 1.0); break;
		case 5: ret = float3(-uv.x, uv.y, -1.0); break;
	}
    return normalize(ret);
}

void computeBasisVectors(const float3 N, out float3 S, out float3 T)
{
	T = cross(N, float3(0.0, 1.0, 0.0));
	T = lerp(cross(N, float3(1.0, 0.0, 0.0)), T, step(Epsilon, dot(T, T)));

	T = normalize(T);
	S = normalize(cross(N, T));
}

// 탄젠트/쉐이딩 공간에서 월드 공간으로 변환
float3 tangentToWorld(const float3 v, const float3 N, const float3 S, const float3 T)
{
	return S * v.x + T * v.y + N * v.z;
}

// GGX NDF importance sampling으로 환경 맵을 사전 필터링 한다.
[numthreads(32, 32, 1)]
void main(uint3 ThreadID : SV_DispatchThreadID)
{
	uint outputWidth, outputHeight, outputDepth;
	outputTexture.GetDimensions(outputWidth, outputHeight, outputDepth);
	if(ThreadID.x >= outputWidth || ThreadID.y >= outputHeight) {
		return;
	}
	
	float inputWidth, inputHeight, inputLevels;
	inputTexture.GetDimensions(0, inputWidth, inputHeight, inputLevels);

	float wt = 4.0 * PI / (6 * inputWidth * inputHeight);
	
	float3 N = getSamplingVector(ThreadID);
	float3 Lo = N;
	
	float3 S, T;
	computeBasisVectors(N, S, T);

	float3 color = 0;
	float weight = 0;

	for(uint i=0; i<NumSamples; ++i) {
		float2 u = sampleHammersley(i);
		float3 Lh = tangentToWorld(sampleGGX(u.x, u.y, roughness), N, S, T);

		float3 Li = 2.0 * dot(Lo, Lh) * Lh - Lo;

		float cosLi = dot(N, Li);
		if(cosLi > 0.0) {
			float cosLh = max(dot(N, Lh), 0.0);

			float pdf = ndfGGX(cosLh, roughness) * 0.25;

			float ws = 1.0 / (NumSamples * pdf);
			
			float mipLevel = max(0.5 * log2(ws / wt) + 1.0, 0.0);

			color  += inputTexture.SampleLevel(defaultSampler, Li, mipLevel).rgb * cosLi;
			weight += cosLi;
		}
	}
	color /= weight;

	outputTexture[ThreadID] = float4(color, 1.0);
}
