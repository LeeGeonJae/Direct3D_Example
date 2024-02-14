#pragma once

#include <directxtk/SimpleMath.h>
#include <DirectXCollision.h>

namespace frustumCulling
{
	struct Object
	{
		DirectX::SimpleMath::Matrix World;
	};
}