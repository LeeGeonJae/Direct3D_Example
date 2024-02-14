#pragma once

#include <map>
#include <string>
#include <Windows.h>
#include <d3d11.h>
#include <directxtk/SimpleMath.h>

namespace resourceManager
{
	class Model;
	class SkinnedModel;
	struct Node;

	class ResourceManager
	{
	public:
		static ResourceManager* GetInstance();
		static void DeleteInstance();

		void Init(ID3D11Device* d3dDevice, ID3D11DeviceContext* d3dContext);

		Model* LoadModel(const std::string& fileName);
		Model* LoadModel(const std::wstring& fileName);
		SkinnedModel* LoadSkinnedModel(const std::string& fileName);
		SkinnedModel* LoadSkinnedModel(const std::wstring& fileName);
		ID3D11ShaderResourceView* LoadTexture(const std::string& fileName);
		ID3D11ShaderResourceView* LoadTexture(const std::wstring& fileName);

	private:
		ResourceManager() = default;
		~ResourceManager();

	private:
		static ResourceManager* mInstance;

		ID3D11Device* md3dDevice;
		ID3D11DeviceContext* md3dContext;

		std::map<std::string, ID3D11ShaderResourceView*> mSRVs;
		std::map<std::string, Model*> mModels;
		std::map<std::string, SkinnedModel*> mSkinnedModels;
	};
}