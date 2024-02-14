cbuffer cbPerObject : register(b0)
{
	float4x4 gWorldViewProj;
}; 

struct VertexIn
{
	float3 PosL      : POSITION;
	float3 NormalL   : NORMAL;
	float2 Tex       : TEXCOORD0;
	float AmbientOcc : AMBIENT;
};

struct VertexOut
{
	float4 PosH      : SV_POSITION;
	float AmbientOcc : AMBIENT;
};

VertexOut VS(VertexIn vin)
{
	VertexOut vout;
	
	vout.PosH = mul(float4(vin.PosL, 1.0f), gWorldViewProj);
	// 정점별 주변광 도달도
	vout.AmbientOcc = vin.AmbientOcc;

	return vout;
}
 
float4 PS(VertexOut pin) : SV_Target
{
	// 정점별 주변광 도달도 색상을 그냥 반환해버림.
	return float4(pin.AmbientOcc.xxx, 1.0f);
}