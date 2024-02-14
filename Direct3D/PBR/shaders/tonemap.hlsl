static const float gamma     = 2.2;
static const float exposure  = 1.0;
static const float pureWhite = 1.0;

struct PixelShaderInput
{
	float4 position : SV_POSITION;
	float2 texcoord : TEXCOORD;
};

Texture2D sceneColor: register(t0);
SamplerState defaultSampler : register(s0);

PixelShaderInput VS(uint vertexID : SV_VertexID)
{
	PixelShaderInput vout;

	if(vertexID == 0) {
		vout.texcoord = float2(1.0, -1.0);
		vout.position = float4(1.0, 3.0, 0.0, 1.0);
	}
	else if(vertexID == 1) {
		vout.texcoord = float2(-1.0, 1.0);
		vout.position = float4(-3.0, -1.0, 0.0, 1.0);
	}
	else /* if(vertexID == 2) */ {
		vout.texcoord = float2(1.0, 1.0);
		vout.position = float4(1.0, -1.0, 0.0, 1.0);
	}
	return vout;
}

float4 PS(PixelShaderInput pin) : SV_Target
{
	float3 color = sceneColor.Sample(defaultSampler, pin.texcoord).rgb * exposure;
	
	// 리인하드 톤매핑
	float luminance = dot(color, float3(0.2126, 0.7152, 0.0722));
	float mappedLuminance = (luminance * (1.0 + luminance/(pureWhite*pureWhite))) / (1.0 + luminance);
	float3 mappedColor = (mappedLuminance / luminance) * color;

	// 감마 보정
	return float4(pow(mappedColor, 1.0/gamma), 1.0);
}
