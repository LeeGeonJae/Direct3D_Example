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

// ���� �ð��� ���� ������ offset�� ������� ���� ���͸� ���ø����ش�.
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

// ��Ʈ�� ��� ���� ���� ���̴��� �� ������ ����� ���� ������
// �ı��� ����Ѵ�. ���� �ý��۸��� ������ ����.�ı� ��Ģ�� �ٸ� ���̹Ƿ�,
// �� �κ��� ���� ���� ���� �ý��۸��� �ٸ� �ʿ䰡 �ִ�.
[maxvertexcount(2)]
void StreamOutGS(point Particle gin[1],
    inout PointStream<Particle> ptStream)
{
    // ���� ��ƼŬ�� ���̸� ������Ʈ
    gin[0].Age += gTimeStep;

    // ������ ��ƼŬ�� ���
    if (gin[0].Type == PT_EMITTER)
    {
        // ��ƼŬ�� Ư�� �ð�(0.005��) �̻� �� ���
        if (gin[0].Age > 0.005f)
        {
            // 3D �������� ������ ���� ���͸� ����
            float3 vRandom = RandUnitVec3(0.0f);
            // x �� z ������ ����Ͽ� ������ ����
            vRandom.x *= 0.5f;
            vRandom.z *= 0.5f;

            // ���ο� ��ƼŬ�� �����Ͽ� ��Ʈ���� �߰�
            Particle p;
            p.InitialPosW = gEmitPosW.xyz; // �ʱ� ��ġ ����
            p.InitialVelW = 3.0f * vRandom; // �ʱ� �ӵ� ����
            p.SizeW = float2(3.0f, 3.0f); // ũ�� ����
            p.Age = 0.0f; // ���� �ʱ�ȭ
            p.Type = PT_FLARE; // ��ƼŬ ���� ����

            ptStream.Append(p); // ������ ��ƼŬ�� ��Ʈ���� �߰�

            gin[0].Age = 0.0f; // ������ ��ƼŬ�� ���̸� �缳��
        }

        // ������ ��ƼŬ�� ���� ��Ʈ���� �߰�
        ptStream.Append(gin[0]);
    }
    else
    {
        // �����Ͱ� �ƴ� ���, ��ƼŬ�� ���̰� 1.0���� �۰ų� ���� ��쿡�� ���� ��Ʈ���� �߰�
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

	// ������ ������ ���� ��¥ ��� ���ӵ�
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

// �������� ���� ���̴��� �׳� ���� ī�޶� ���� �簢������ Ȯ���Ѵ�.
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
