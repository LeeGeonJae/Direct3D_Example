#pragma once

#include <directxtk/SimpleMath.h>

#include "LightHelper.h"

namespace ssao
{
	using namespace common;
	using namespace DirectX;
	using namespace SimpleMath;

	struct ObjectSsaoNormalDepth
	{
		Matrix WorldView;
		Matrix WorldInvTransposeView;
		Matrix WorldViewProj;
		Matrix TexTransform;
	};

	struct ObjectBasic
	{
		Matrix World;
		Matrix WorldInvTranspose;
		Matrix WorldViewProj;
		Matrix WorldViewProjTex;
		Matrix TexTransform;
		Material Material;
	};

	struct FrameBasic
	{
		DirectionLight DirLights[3];

		Vector3 EyePosW;
		int LightCount;

		Vector4 FogColor;

		float  FogStart;
		float  FogRange;
		int UseTexure;
		int unused;
	};

	struct FrameSsao
	{
		Matrix ViewToTexSpace; // Proj*Texture
		Vector4 OffsetVectors[14];
		Vector4 FrustumCorners[4];
	};

	struct FrameSsaoBlur
	{
		float TexelWidth;
		float TexelHeight;
		int bHorizontalBlur;
		float unused[1];
	};

}