#pragma once

namespace common
{
	using namespace DirectX::SimpleMath;

	class MathHelper
	{
	public:
		static float AngleFromXY(float x, float y);
		static float RandF();
		static float RandF(float a, float b);
		static Matrix InverseTranspose(const Matrix& M);
		static Vector4 RandHemisphereUnitVec3(Vector4 n);

		template<typename T>
		static T Max(const T& a, const T& b);
		template <typename T>
		static T Clamp(const T& x, const T& low, const T& high);
		template<typename T>
		static T Min(const T& a, const T& b);

	public:
		static constexpr float Pi = 3.1415926535f;
		static constexpr float Infinity = FLT_MAX;
	};

	template<typename T>
	T MathHelper::Max(const T& a, const T& b)
	{
		return a > b ? a : b;
	}

	template<typename T>
	T MathHelper::Min(const T& a, const T& b)
	{
		return a < b ? a : b;
	}

	template <typename T>
	T MathHelper::Clamp(const T& x, const T& low, const T& high)
	{
		if (x < low)
		{
			return low;
		}
		else if (x > high)
		{
			return high;
		}

		return x;
	}
}