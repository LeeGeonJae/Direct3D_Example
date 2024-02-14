#pragma once

namespace common
{
	using DirectX::SimpleMath::Matrix;
	using DirectX::SimpleMath::Vector3;

	class Camera
	{
	public:
		Camera();
		~Camera() = default;
		Camera(const Camera&) = default;
		Camera& operator=(const Camera&) = default;

		void UpdateViewMatrix();
		void LookAt(const Vector3& pos, const Vector3& target, const Vector3& up);

		void RotatePitch(float angle);
		void RotateYaw(float angle);
		void RotateRoll(float angle);
		void RotateAxis(const Vector3& axis, float angle);

		void TranslateLook(float d);
		void TranslateRight(float d);
		void TranslateUp(float d);

		inline void SetLens(float fovY, float aspect, float zn, float zf);
		inline void SetPosition(float x, float y, float z);
		inline void SetPosition(const Vector3& position);

		inline Vector3 GetPosition() const;
		inline Vector3 GetRight() const;
		inline Vector3 GetUp() const;
		inline Vector3 GetLook() const;

		inline float GetNearZ() const;
		inline float GetFarZ() const;
		inline float GetAspect() const;
		inline float GetFovY() const;
		inline float GetFovX() const;

		inline float GetNearWindowWidth() const;
		inline float GetNearWindowHeight() const;
		inline float GetFarWindowWidth() const;
		inline float GetFarWindowHeight() const;

		inline Matrix GetView() const;
		inline Matrix GetProj() const;
		inline Matrix GetViewProj() const;

	private:
		Vector3 mPosition;
		Vector3 mRight;
		Vector3 mUp;
		Vector3 mLook;

		float mNearZ;
		float mFarZ;
		float mAspect;
		float mFovY;
		float mNearWindowHeight;
		float mFarWindowHeight;

		Matrix mView;
		Matrix mProj;
	};

	void Camera::SetLens(float fovY, float aspect, float zn, float zf)
	{
		mFovY = fovY;
		mAspect = aspect;
		mNearZ = zn;
		mFarZ = zf;

		mNearWindowHeight = 2.f * mNearZ * tanf(fovY * 0.5f);
		mFarWindowHeight = 2.f * mFarZ * tanf(fovY * 0.5f);

		mProj = DirectX::XMMatrixPerspectiveFovLH(mFovY, mAspect, mNearZ, mFarZ);
	}
	void Camera::SetPosition(float x, float y, float z)
	{
		mPosition = { x, y, z };
	}
	void Camera::SetPosition(const Vector3& v)
	{
		mPosition = v;
	}

	Vector3 Camera::GetPosition() const
	{
		return mPosition;
	}
	Vector3 Camera::GetRight() const
	{
		return mRight;
	}
	Vector3 Camera::GetUp() const
	{
		return mUp;
	}
	Vector3 Camera::GetLook() const
	{
		return mLook;
	}

	float Camera::GetNearZ() const
	{
		return mNearZ;
	}
	float Camera::GetFarZ() const
	{
		return mFarZ;
	}
	float Camera::GetAspect() const
	{
		return mAspect;
	}
	float Camera::GetFovY() const
	{
		return mFovY;
	}
	float Camera::GetFovX() const
	{
		float halfWidth = 0.5f * GetNearWindowWidth();

		return 2.f * atan(halfWidth / mNearZ);
	}
	float Camera::GetNearWindowWidth() const
	{
		return mAspect * mNearWindowHeight;
	}
	float Camera::GetNearWindowHeight() const
	{
		return mNearWindowHeight;
	}
	float Camera::GetFarWindowWidth() const
	{
		return mAspect * mFarWindowHeight;
	}
	float Camera::GetFarWindowHeight() const
	{
		return mFarWindowHeight;
	}

	Matrix Camera::GetView() const
	{
		return mView;
	}
	Matrix Camera::GetProj() const
	{
		return mProj;
	}
	Matrix Camera::GetViewProj() const
	{
		return mView * mProj;
	}
}