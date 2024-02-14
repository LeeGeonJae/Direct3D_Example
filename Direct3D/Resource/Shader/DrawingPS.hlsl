
struct PS_INPUT
{
	float4 position : SV_POSITION;
	float4 color : TEXCOORD0;
};

float4 main(PS_INPUT pin) : SV_Target
{
	return pin.color;
}