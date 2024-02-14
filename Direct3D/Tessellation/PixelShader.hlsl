struct DomainOut
{
	float4 PosH : SV_POSITION;
};

float4 main(DomainOut pin) : SV_TARGET
{
	return float4(1.0f, 1.0f, 1.0f, 1.0f);
}