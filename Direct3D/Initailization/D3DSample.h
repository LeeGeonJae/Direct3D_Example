#pragma once

#include "D3dProcessor.h"

namespace initalization
{
	using namespace common;

	class D3DSample final : public D3DProcessor
	{
	public:
		D3DSample(HINSTANCE hInstance, UINT width, UINT height, std::wstring name);

		void Update(float deltaTime) override;
		void Render() override;
	};
}