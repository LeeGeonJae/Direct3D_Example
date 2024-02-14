#include "Animation.h"

namespace resourceManager
{
	DirectX::SimpleMath::Matrix AnimationNode::Evaluate(float progressTime) const
	{
		using namespace DirectX::SimpleMath;

		Vector3 position = KeyAnimations[0].Position;
		Quaternion rotation = KeyAnimations[0].Rotation;
		Vector3 scaling = KeyAnimations[0].Scaling;

		for (auto iter = KeyAnimations.begin(); iter != KeyAnimations.end(); ++iter)
		{
			if (iter->Time > progressTime)
			{
				auto begin = iter - 1;
				float delta = progressTime - begin->Time;
				float deltaRatio = delta / (iter->Time - begin->Time);

				position = Vector3::Lerp(begin->Position, iter->Position, deltaRatio);
				rotation = Quaternion::Slerp(begin->Rotation, iter->Rotation, deltaRatio);
				scaling = Vector3::Lerp(begin->Scaling, iter->Scaling, deltaRatio);

				break;
			}
		}

		return Matrix::CreateScale(scaling)
			* Matrix::CreateFromQuaternion(rotation)
			* Matrix::CreateTranslation(position);
	}
}