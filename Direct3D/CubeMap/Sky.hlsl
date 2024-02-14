
cbuffer cbPerFrame : register(b0)
{
	float4x4 gWorldViewProj; // 카메라의 회전이랑 투영변환만 반영해준다.
};

TextureCube gCubeMap : register(t0); 
SamplerState gSamLinear : register(s0);

struct VertexIn
{
	float3 PosL : POSITION;
};

struct VertexOut
{
	float4 PosH : SV_POSITION;
	float3 PosL : POSITION;
};

VertexOut VS(VertexIn vin)
{
	VertexOut vout;

	vout.PosH = mul(float4(vin.PosL, 1.0f), gWorldViewProj).xyww;

	vout.PosL = vin.PosL; 

	return vout;
}

float4 PS(VertexOut pin) : SV_Target
{
	// 조회벡터로 샘플링
	float4 color = gCubeMap.Sample(gSamLinear, pin.PosL); 
	
	return color; 
}
