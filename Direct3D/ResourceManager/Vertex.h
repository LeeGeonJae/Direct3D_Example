#pragma once

#include <directxtk/SimpleMath.h>

namespace vertex
{
	struct Basic32
	{
		DirectX::SimpleMath::Vector3 Pos;
		DirectX::SimpleMath::Vector3 Normal;
		DirectX::SimpleMath::Vector2 Tex;
	};

	struct PosNormalTexTan
	{
		DirectX::SimpleMath::Vector3 Pos;
		DirectX::SimpleMath::Vector3 Normal;
		DirectX::SimpleMath::Vector3 TangentU;
		DirectX::SimpleMath::Vector2 Tex;
	};

	struct PosNormalTexTanSkinned
	{
		enum { INVALID_INDEX = -1 };

		DirectX::SimpleMath::Vector3 Pos;
		DirectX::SimpleMath::Vector3 Normal;
		DirectX::SimpleMath::Vector3 TangentU;
		DirectX::SimpleMath::Vector2 Tex;
		int Indices[4] = { INVALID_INDEX, INVALID_INDEX, INVALID_INDEX, INVALID_INDEX };
		float Weights[4] = { 0.f };
	};
};