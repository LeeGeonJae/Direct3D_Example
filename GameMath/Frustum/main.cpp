#include <array>

struct Vector3
{
public:
	float Dot(const Vector3& other) const
	{
		return X * other.X + Y * other.Y + Z * other.Z;
	}
	float GetMagnitude() const
	{
		return X * X + Y * Y + Z * Z;
	}
	Vector3 operator*(float scalar) const
	{
		return { X * scalar, Y * scalar, Z * scalar };
	}

public:
	float X;
	float Y;
	float Z;
};

struct Plane
{
public:
	float CalculateDistance(const Vector3& point) const
	{
		return Normal.Dot(point) + InvDistance;
	}
	bool CheckIsOutSide(const Vector3& point) const
	{
		return CalculateDistance(point) > 0.f;
	}
	void Normalize()
	{
		float magnitude = Normal.GetMagnitude();

		if (magnitude <= 1.f)
		{
			return;
		}

		float invMagnitude = 1 / magnitude;

		Normal = Normal * invMagnitude;
		InvDistance = InvDistance * invMagnitude;
	}

public:
	Vector3 Normal;
	float InvDistance = 0.f;
};

enum PlaneType
{
	TOP,
	BOTTOM,
	RIGHT,
	LEFT,
	Far,
	Near
};

enum class eBoundCheckResult
{
	Outside,
	Intersect,
	Inside
};

struct Sphere
{

};

struct Box
{

};

struct Frustum
{
	eBoundCheckResult CheckBound(const Vector3& point) const
	{
		constexpr float EPSION = 0.01f;

		for (const Plane& plane : Planes)
		{
			if (plane.CheckIsOutSide(point))
			{
				return eBoundCheckResult::Outside;
			}
			else if (fabsf(plane.CalculateDistance(point)) < EPSION)
			{
				return eBoundCheckResult::Intersect;
			}
		}

		return eBoundCheckResult::Inside;
	}
	eBoundCheckResult CheckBound(const Sphere& sphere) const
	{

	}
	eBoundCheckResult CheckBound(const Box& box) const
	{

	}


public:
	std::array<Plane, 6> Planes;
};



int main()
{

	return 0;
}