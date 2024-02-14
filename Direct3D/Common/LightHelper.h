#pragma once

#include <directxtk/SimpleMath.h>

namespace common
{
	struct DirectionLight
	{
		DirectX::SimpleMath::Vector4 Ambient;
		DirectX::SimpleMath::Vector4 Diffuse;
		DirectX::SimpleMath::Vector4 Specular;
		DirectX::SimpleMath::Vector3 Direction;
		float pad;
	};

	struct PointLight
	{
		DirectX::SimpleMath::Vector4 Ambient;
		DirectX::SimpleMath::Vector4 Diffuse;
		DirectX::SimpleMath::Vector4 Specular;
		DirectX::SimpleMath::Vector3 Position;
		float Range;
		DirectX::SimpleMath::Vector3 AttenuationParam; // 감쇠 매개변수 a0, a1, a2
		float pad;
	};

	struct SpotLight
	{
		DirectX::SimpleMath::Vector4 Ambient;
		DirectX::SimpleMath::Vector4 Diffuse;
		DirectX::SimpleMath::Vector4 Specular;
		DirectX::SimpleMath::Vector3 Direction;
		float Spot; // 원뿔에 사용될 지수
		DirectX::SimpleMath::Vector3 Position;
		float Range;
		DirectX::SimpleMath::Vector3 AttenuationParam; // 감쇠 매개변수 a0, a1, a2
		float pad;
	};

	struct Material
	{
		DirectX::SimpleMath::Vector4 Ambient;
		DirectX::SimpleMath::Vector4 Diffuse;
		DirectX::SimpleMath::Vector4 Specular; // specular의 마지막 성분을 지수로 사용함
		DirectX::SimpleMath::Vector4 Reflect;
	};
}