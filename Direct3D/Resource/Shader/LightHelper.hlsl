
struct DirectionLight
{
	float4 Ambient;
	float4 Diffuse;
	float4 Specular;
	float3 Direction;
	float pad;
};

struct PointLight
{
	float4 Ambient;
	float4 Diffuse;
	float4 Specular;
	float3 Position;
	float Range;
	float3 AttenuationParam;
	float pad;
};

struct SpotLight
{
	float4 Ambient;
	float4 Diffuse;
	float4 Specular;
	float3 Direction;
	float Spot;
	float3 Position;
	float Range;
	float3 AttenuationParam;
	float pad;
};

struct Material
{
	float4 Ambient;
	float4 Diffuse;
	float4 Specular; // specular w = specular power
	float4 Reflect;
};

void ComputeDirectionLight(Material material
	, DirectionLight directionLight
	, float3 normal
	, float3 toEye
	, out float4 ambient
	, out float4 diffuse
	, out float4 specular)
{
	ambient = float4(0.0f, 0.0f, 0.0f, 0.0f);
	diffuse = float4(0.0f, 0.0f, 0.0f, 0.0f);
	specular = float4(0.0f, 0.0f, 0.0f, 0.0f);

	// 간접광
	ambient = material.Ambient * directionLight.Ambient;

	// 난반사광
	float ndotl = dot(normal, -directionLight.Direction);

	if (ndotl < 0)
	{
		return;
	}

	diffuse = material.Diffuse * directionLight.Diffuse * ndotl;

	// 정반사광
	float3 r = reflect(directionLight.Direction, normal);
	float rdotv = dot(r, normalize(toEye));
	specular = material.Specular * directionLight.Specular * pow(max(rdotv, 0.0f), directionLight.Specular.w);
}

void ComputePointLight(Material material
	, PointLight pointLight
	, float3 position
	, float3 normal
	, float3 toEye
	, out float4 ambient
	, out float4 diffuse
	, out float4 specular)
{
	ambient = float4(0.0f, 0.0f, 0.0f, 0.0f);
	diffuse = float4(0.0f, 0.0f, 0.0f, 0.0f);
	specular = float4(0.0f, 0.0f, 0.0f, 0.0f);

	float3 lightDirection = position - pointLight.Position;
	float distance = length(lightDirection);

	if (distance > pointLight.Range)
	{
		return;
	}

	// 주변광
	ambient = material.Ambient * pointLight.Ambient;

	// 난반사광
	lightDirection /= distance;
	float ndotl = dot(normal, -lightDirection);

	if (ndotl < 0)
	{
		return;
	}

	diffuse = material.Diffuse * pointLight.Diffuse * ndotl;

	// 정반사광
	float3 r = reflect(lightDirection, normal);
	float rdotv = dot(r, normalize(toEye));
	specular = material.Specular * pointLight.Specular * pow(max(rdotv, 0.0f), pointLight.Specular.w);

	// 감쇠율
	float invAttenuationRate = 1 / dot(pointLight.AttenuationParam, float3(1.0f, distance, distance * distance));
	diffuse *= invAttenuationRate;
	specular *= invAttenuationRate;
}

void ComputeSpotLight(Material material
	, SpotLight spotLight
	, float3 position
	, float3 normal
	, float3 toEye
	, out float4 ambient
	, out float4 diffuse
	, out float4 specular)
{
	ambient = float4(0.0f, 0.0f, 0.0f, 0.0f);
	diffuse = float4(0.0f, 0.0f, 0.0f, 0.0f);
	specular = float4(0.0f, 0.0f, 0.0f, 0.0f);

	float3 lightDirection = position - spotLight.Position;
	float distance = length(lightDirection);

	if (distance > spotLight.Range)
	{
		return;
	}

	// 주변광
	ambient = material.Ambient * spotLight.Ambient;

	// 난반사광
	lightDirection /= distance;
	float ndotl = dot(normal, -lightDirection);

	if (ndotl < 0)
	{
		return;
	}

	diffuse = material.Diffuse * spotLight.Diffuse * ndotl;

	// 정반사광
	float3 r = reflect(lightDirection, normal);
	float rdotv = dot(r, normalize(toEye));
	specular = material.Specular * spotLight.Specular * pow(max(rdotv, 0.0f), spotLight.Specular.w); // max를 취해줘야 올바른 값이 나온다.

	// 중심의 거리와 지수에 따른 범위 제어
	float spot = pow(max(dot(lightDirection, spotLight.Direction), 0.0f), spotLight.Spot);

	// 감쇠율
	float invAttenuationRate = spot / dot(spotLight.AttenuationParam, float3(1.0f, distance, distance * distance));
	diffuse *= invAttenuationRate;
	specular *= invAttenuationRate;
}

float3 NormalSampleToWorldSpace(float3 normalMapSample, float3 unitNormalW, float3 tangentW)
{
	// Uncompress each component from [0,1] to [-1,1].
	float3 normalT = 2.0f * normalMapSample - 1.0f;

	// Build orthonormal basis.
	float3 N = unitNormalW;
	float3 T = normalize(tangentW - dot(tangentW, N) * N);
	float3 B = cross(N, T);

	float3x3 TBN = float3x3(T, B, N);

	// Transform from tangent space to world space.
	float3 bumpedNormalW = mul(normalT, TBN);

	return bumpedNormalW;
}

static const float SMAP_SIZE = 2048.0f;
static const float SMAP_DX = 1.0f / SMAP_SIZE;

float CalcShadowFactor(SamplerComparisonState samShadow,
	Texture2D shadowMap,
	float4 shadowPosH)
{
	// 광원 기준으로 한 텍스처 공간 좌표
	shadowPosH.xyz /= shadowPosH.w;

	// 깊이는 텍스처 공간과 NDC가 동일하다.
	float depth = shadowPosH.z;

	// 텍셀 사이즈로 커널 구축
	const float dx = SMAP_DX;
	float percentLit = 0.0f;
	const float2 offsets[9] =
	{
		float2(-dx,  -dx), float2(0.0f,  -dx), float2(dx,  -dx),
		float2(-dx, 0.0f), float2(0.0f, 0.0f), float2(dx, 0.0f),
		float2(-dx,  +dx), float2(0.0f,  +dx), float2(dx,  +dx)
	};

	[unroll]
	for (int i = 0; i < 9; ++i)
	{
		// 해당함수는 PCF를 기반하여 0 ~ 1 사이의 결과값을 반환한다.
		percentLit += shadowMap.SampleCmpLevelZero(samShadow, // 샘플러
			shadowPosH.xy + offsets[i], // 텍스처 좌표
			depth).r; // 비교값
	}

	return percentLit /= 9.0f;
}