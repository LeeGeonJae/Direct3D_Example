#pragma once

#include <string>
#include <vector>
#include <map>

#include <directxtk/SimpleMath.h>

namespace resourceManager
{
	struct KeyAnimation
	{
		float Time;
		DirectX::SimpleMath::Vector3 Position;
		DirectX::SimpleMath::Quaternion Rotation;
		DirectX::SimpleMath::Vector3 Scaling;
	};

	struct AnimationNode
	{
	public:
		DirectX::SimpleMath::Matrix Evaluate(float progressTime) const;

	public:
		std::string Name;
		std::vector<KeyAnimation> KeyAnimations;
	};

	struct AnimationClip
	{
		std::string Name;
		double Duration;
		std::map<std::string, AnimationNode> AnimationNodes; // 본이름과 애니메이션 노드 매핑
	};
}