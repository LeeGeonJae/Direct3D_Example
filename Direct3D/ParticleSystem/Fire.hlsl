cbuffer cbPerFrame : register(b0)
{
	float4x4 gViewProj;
	float3 gEyePosW;
	float gGameTime;
	float3 gEmitPosW;
	float gTimeStep;
	float3 gEmitDirW;
};

Texture2DArray gTexArray : register(t0);
Texture1D gRandomTex : register(t1);
SamplerState samLinear : register(s0);

// 게임 시간과 직접 전달한 offset을 기반으로 랜덤 벡터를 샘플링해준다.
float3 RandUnitVec3(float offset)
{
	float u = (gGameTime + offset);
	float3 v = gRandomTex.SampleLevel(samLinear, u, 0).xyz;

	return normalize(v);
}

#define PT_EMITTER 0
#define PT_FLARE 1

struct Particle
{
	float3 InitialPosW	: POSITION;
	float3 InitialVelW	: VELOCITY;
	float2 SizeW		: SIZE;
	float Age			: AGE;
	uint Type			: TYPE;
};

Particle StreamOutVS(Particle vin)
{
	return vin;
}

// 스트림 출력 전용 기하 셰이더는 새 입자의 방출과 기존 입자의
// 파괴만 담당한다. 입자 시스템마다 입자의 생성.파괴 규칙이 다를 것이므로,
// 이 부분은 구현 역시 입자 시스템마다 다를 필요가 있다.
[maxvertexcount(2)]
void StreamOutGS(point Particle gin[1],
    inout PointStream<Particle> ptStream)
{
    // 현재 파티클의 나이를 업데이트
    gin[0].Age += gTimeStep;

    // 에미터 파티클인 경우
    if (gin[0].Type == PT_EMITTER)
    {
        // 파티클이 특정 시간(0.005초) 이상 된 경우
        if (gin[0].Age > 0.005f)
        {
            // 3D 공간에서 무작위 방향 벡터를 생성
            float3 vRandom = RandUnitVec3(0.0f);
            // x 및 z 성분을 축소하여 범위를 제한
            vRandom.x *= 0.5f;
            vRandom.z *= 0.5f;

            // 새로운 파티클을 생성하여 스트림에 추가
            Particle p;
            p.InitialPosW = gEmitPosW.xyz; // 초기 위치 설정
            p.InitialVelW = 3.0f * vRandom; // 초기 속도 설정
            p.SizeW = float2(3.0f, 3.0f); // 크기 설정
            p.Age = 0.0f; // 나이 초기화
            p.Type = PT_FLARE; // 파티클 유형 설정

            ptStream.Append(p); // 생성된 파티클을 스트림에 추가

            gin[0].Age = 0.0f; // 에미터 파티클의 나이를 재설정
        }

        // 에미터 파티클을 현재 스트림에 추가
        ptStream.Append(gin[0]);
    }
    else
    {
        // 에미터가 아닌 경우, 파티클의 나이가 1.0보다 작거나 같은 경우에만 현재 스트림에 추가
        if (gin[0].Age <= 1.0f)
            ptStream.Append(gin[0]);
    }
}

struct VertexOut
{
	float3 PosW  : POSITION;
	float2 SizeW : SIZE;
	float4 Color : COLOR;
	uint   Type  : TYPE;
};

VertexOut DrawVS(Particle vin)
{
	VertexOut vout;

	// 입자의 가속을 위한 알짜 상수 가속도
    float3 gAccelW = { 0.0f, 17.8f, 0.0f };

	float t = vin.Age;
	vout.PosW = 0.5f * t * t * gAccelW + t * vin.InitialVelW + vin.InitialPosW;
	float opacity = 1.0f - smoothstep(0.0f, 1.0f, t / 1.0f);
	vout.Color = float4(1.0f, 1.0f, 1.0f, opacity);
	vout.SizeW = vin.SizeW;
	vout.Type = vin.Type;

	return vout;
}

struct GeoOut
{
	float4 PosH  : SV_Position;
	float4 Color : COLOR;
	float2 Tex   : TEXCOORD;
};

// 렌더링용 기하 셰이더는 그냥 점을 카메라를 향한 사각형으로 확장한다.
[maxvertexcount(4)]
void DrawGS(point VertexOut gin[1],
	inout TriangleStream<GeoOut> triStream)
{
	float2 gQuadTexC[4] =
	{
		float2(0.0f, 1.0f),
		float2(1.0f, 1.0f),
		float2(0.0f, 0.0f),
		float2(1.0f, 0.0f)
	};
	
	if (gin[0].Type != PT_EMITTER)
	{
		float3 look = normalize(gEyePosW.xyz - gin[0].PosW);
		float3 right = normalize(cross(float3(0, 1, 0), look));
		float3 up = cross(look, right);
		
		float halfWidth = 0.5f * gin[0].SizeW.x;
		float halfHeight = 0.5f * gin[0].SizeW.y;

		float4 v[4];
		v[0] = float4(gin[0].PosW + halfWidth * right - halfHeight * up, 1.0f);
		v[1] = float4(gin[0].PosW + halfWidth * right + halfHeight * up, 1.0f);
		v[2] = float4(gin[0].PosW - halfWidth * right - halfHeight * up, 1.0f);
		v[3] = float4(gin[0].PosW - halfWidth * right + halfHeight * up, 1.0f);
		
		GeoOut gout;
		[unroll]
		for (int i = 0; i < 4; ++i)
		{
			gout.PosH = mul(v[i], gViewProj);
			gout.Tex = gQuadTexC[i];
			gout.Color = gin[0].Color;
			triStream.Append(gout);
		}
	}
}

float4 DrawPS(GeoOut pin) : SV_TARGET
{
	return gTexArray.Sample(samLinear, float3(pin.Tex, 0)) * pin.Color;
}
