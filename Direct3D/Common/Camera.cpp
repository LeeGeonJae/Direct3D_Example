#include "pch.h"

#include "Camera.h"
#include "MathHelper.h"

namespace common
{
	Camera::Camera()
		: mPosition(0.0f, 0.0f, 0.0f)
		, mRight(1.0f, 0.0f, 0.0f)
		, mUp(0.0f, 1.0f, 0.0f)
		, mLook(0.0f, 0.0f, 1.0f)
	{
		SetLens(0.25f * MathHelper::Pi, 1.0f, 1.0f, 1000.0f);
	}

	void Camera::UpdateViewMatrix()
	{
		using namespace DirectX;

		XMVECTOR R = XMLoadFloat3(&mRight);
		XMVECTOR U = XMLoadFloat3(&mUp);
		XMVECTOR L = XMLoadFloat3(&mLook);
		XMVECTOR P = XMLoadFloat3(&mPosition);

		// Keep camera's axes orthogonal to each other and of unit length.
		L = XMVector3Normalize(L);
		U = XMVector3Normalize(XMVector3Cross(L, R));

		// U, L already ortho-normal, so no need to normalize cross product.
		R = XMVector3Cross(U, L);

		// Fill in the view matrix entries.
		float x = -XMVectorGetX(XMVector3Dot(P, R));
		float y = -XMVectorGetX(XMVector3Dot(P, U));
		float z = -XMVectorGetX(XMVector3Dot(P, L));

		XMStoreFloat3(&mRight, R);
		XMStoreFloat3(&mUp, U);
		XMStoreFloat3(&mLook, L);

		mView(0, 0) = mRight.x;
		mView(1, 0) = mRight.y;
		mView(2, 0) = mRight.z;
		mView(3, 0) = x;

		mView(0, 1) = mUp.x;
		mView(1, 1) = mUp.y;
		mView(2, 1) = mUp.z;
		mView(3, 1) = y;

		mView(0, 2) = mLook.x;
		mView(1, 2) = mLook.y;
		mView(2, 2) = mLook.z;
		mView(3, 2) = z;

		mView(0, 3) = 0.0f;
		mView(1, 3) = 0.0f;
		mView(2, 3) = 0.0f;
		mView(3, 3) = 1.0f;
	}

	void Camera::LookAt(const Vector3& pos, const Vector3& target, const Vector3& worldUp)
	{
		using namespace DirectX;

		XMVECTOR L = XMVector3Normalize(XMVectorSubtract(target, pos));
		XMVECTOR R = XMVector3Normalize(XMVector3Cross(worldUp, L));
		XMVECTOR U = XMVector3Cross(L, R);

		XMStoreFloat3(&mPosition, pos);
		XMStoreFloat3(&mLook, L);
		XMStoreFloat3(&mRight, R);
		XMStoreFloat3(&mUp, U);
	}

	void Camera::RotatePitch(float angle)
	{
		using namespace DirectX::SimpleMath;

		Matrix rotationMat = Matrix::CreateFromAxisAngle(mRight, angle);

		mUp = Vector3::TransformNormal(mUp, rotationMat);
		mLook = Vector3::TransformNormal(mLook, rotationMat);
	}
	void Camera::RotateYaw(float angle)
	{
		using namespace DirectX::SimpleMath;

		Matrix rotationMat = Matrix::CreateFromAxisAngle(mUp, angle);

		mRight = Vector3::TransformNormal(mRight, rotationMat);
		mLook = Vector3::TransformNormal(mLook, rotationMat);
	}
	void Camera::RotateRoll(float angle)
	{
		using namespace DirectX::SimpleMath;

		Matrix rotationMat = Matrix::CreateFromAxisAngle(mLook, angle);

		mUp = Vector3::TransformNormal(mUp, rotationMat);
		mRight = Vector3::TransformNormal(mRight, rotationMat);
	}
	void Camera::RotateAxis(const Vector3& axis, float angle)
	{
		using namespace DirectX::SimpleMath;

		assert(axis.LengthSquared() <= 1.001f);
		Matrix rotationMat = Matrix::CreateFromAxisAngle(axis, angle);

		mUp = Vector3::TransformNormal(mUp, rotationMat);
		mRight = Vector3::TransformNormal(mRight, rotationMat);
		mLook = Vector3::TransformNormal(mLook, rotationMat);
	}

	void Camera::TranslateLook(float d)
	{
		using namespace DirectX;
		Vector3 movedPos = mLook * d;
		mPosition += movedPos;
	}
	void Camera::TranslateRight(float d)
	{
		Vector3 movedPos = mRight * d;
		mPosition += movedPos;
	}
	void Camera::TranslateUp(float d)
	{
		Vector3 movedPos = mUp * d;
		mPosition += movedPos;
	}
}