#include "pch.h"

#include "MathHelper.h"

namespace common
{
	float MathHelper::AngleFromXY(float x, float y)
	{
		// atan2쓰면 되는 거 아닌가 이거..?
		float theta = 0.0f;

		// Quadrant I or IV
		if (x >= 0.0f)
		{
			// If x = 0, then atanf(y/x) = +pi/2 if y > 0
			//                atanf(y/x) = -pi/2 if y < 0
			theta = atanf(y / x); // in [-pi/2, +pi/2]

			if (theta < 0.0f)
			{
				theta += 2.0f * Pi; // in [0, 2*pi).
			}
		}

		// Quadrant II or III
		else
		{
			theta = atanf(y / x) + Pi; // in [0, 2*pi).
		}

		return theta;
	}

	float MathHelper::RandF()
	{
		return (float)(rand()) / (float)RAND_MAX;
	}

	float MathHelper::RandF(float a, float b)
	{
		return a + RandF() * (b - a);
	}

	DirectX::SimpleMath::Matrix MathHelper::InverseTranspose(const DirectX::SimpleMath::Matrix& matrix)
	{
		DirectX::SimpleMath::Matrix result = matrix;
		result._41 = 0.f;
		result._42 = 0.f;
		result._43 = 0.f;
		result._44 = 1.f;

		DirectX::SimpleMath::Matrix invResult;
		result.Invert(invResult);

		return invResult.Transpose();
	}

	Vector4 MathHelper::RandHemisphereUnitVec3(Vector4 n)
	{
		Vector4 One = { 1.0f, 1.0f, 1.0f, 1.0f };
		Vector4 Zero = Vector4::Zero;

		while (true)
		{
			Vector4 v = { RandF(-1.0f, 1.0f), RandF(-1.0f, 1.0f), RandF(-1.0f, 1.0f), 0.0f };

			if (XMVector3Greater(XMVector3LengthSq(v), One))
				continue;

			if (XMVector3Less(XMVector3Dot(n, v), Zero))
				continue;

			return XMVector3Normalize(v);
		}
	}
}