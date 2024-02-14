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

float3 RandUnitVec3(float offset)
{
	float u = (gGameTime + offset);
	float3 v = gRandomTex.SampleLevel(samLinear, u, 0).xyz;

	return normalize(v);
}

float3 RandVec3(float offset)
{
	float u = (gGameTime + offset);
	float3 v = gRandomTex.SampleLevel(samLinear, u, 0).xyz;

	return v;
}

#define PT_EMITTER 0
#define PT_FLARE 1

struct Particle
{
	float3 InitialPosW : POSITION;
	float3 InitialVelW : VELOCITY;
	float2 SizeW       : SIZE;
	float Age : AGE;
	uint Type          : TYPE;
};

Particle StreamOutVS(Particle vin)
{
	return vin;
}

[maxvertexcount(6)]
void StreamOutGS(point Particle gin[1],
	inout PointStream<Particle> ptStream)
{
	gin[0].Age += gTimeStep;

	if (gin[0].Type == PT_EMITTER)
	{
		if (gin[0].Age > 0.002f)
		{
			for (int i = 0; i < 5; ++i)
			{
				float3 vRandom = 35.0f * RandVec3((float)i / 5.0f);
				vRandom.y = 20.0f;

				Particle p;
				p.InitialPosW = gEmitPosW.xyz + vRandom;
				p.InitialVelW = float3(0.0f, 0.0f, 0.0f);
				p.SizeW = float2(1.0f, 1.0f);
				p.Age = 0.0f;
				p.Type = PT_FLARE;

				ptStream.Append(p);
			}

			gin[0].Age = 0.0f;
		}

		ptStream.Append(gin[0]);
	}
	else
	{
		if (gin[0].Age <= 5.0f)
			ptStream.Append(gin[0]);
	}
}

// 기하 셰이더를 출력 스트림에 쓰려면 쉐이더에 정점 구조를 정의해야 한다.
GeometryShader gsStreamOut = ConstructGSWithSO(
	CompileShader(gs_5_0, StreamOutGS()), // 컴파일된 기하셰이더 프로그램
	"POSITION.xyz; VELOCITY.xyz; SIZE.xy; AGE.x; TYPE.x"); // 정점 형식

struct VertexOut
{
	float3 PosW  : POSITION;
	uint   Type  : TYPE;
};

VertexOut DrawVS(Particle vin)
{
	VertexOut vout;

	float3 gAccelW = { -1.0f, -9.8f, 0.0f };

	float t = vin.Age;
	vout.PosW = 0.5f * t * t * gAccelW + t * vin.InitialVelW + vin.InitialPosW;
	vout.Type = vin.Type;

	return vout;
}

struct GeoOut
{
	float4 PosH  : SV_Position;
	float2 Tex   : TEXCOORD;
};

[maxvertexcount(2)]
void DrawGS(point VertexOut gin[1],
	inout LineStream<GeoOut> lineStream)
{
	float3 gAccelW = { -1.0f, -50.8f, 0.0f };

	if (gin[0].Type != PT_EMITTER)
	{
		float3 p0 = gin[0].PosW;
		float3 p1 = gin[0].PosW + 0.07f * gAccelW;

		GeoOut v0;
		v0.PosH = mul(float4(p0, 1.0f), gViewProj);
		v0.Tex = float2(0.0f, 0.0f);
		lineStream.Append(v0);

		GeoOut v1;
		v1.PosH = mul(float4(p1, 1.0f), gViewProj);
		v1.Tex = float2(1.0f, 1.0f);
		lineStream.Append(v1);
	}
}

float4 DrawPS(GeoOut pin) : SV_TARGET
{
	return gTexArray.Sample(samLinear, float3(pin.Tex, 0));
}
