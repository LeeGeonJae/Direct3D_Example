#include <cassert>
#include <fstream>

#include "D3DSample.h"
#include "D3DUtil.h"

namespace lighting
{
	D3DSample::D3DSample(HINSTANCE hInstance, UINT width, UINT height, std::wstring name)
		: D3DProcessor(hInstance, width, height, name)
		, mVertexShader(nullptr)
		, mVertexShaderBuffer(nullptr)
		, mPixelShader(nullptr)
		, mInputLayout(nullptr)
		, mView(DirectX::SimpleMath::Matrix::Identity)
		, mProj(DirectX::SimpleMath::Matrix::Identity)
		, mTheta(1.5f * DirectX::XM_PI)
		, mPhi(0.25f * DirectX::XM_PI)
		, mRadius(200.f)
		, mLastMousePos({ 0, 0 })
	{
		using namespace DirectX::SimpleMath;

		mWaveWorld = Matrix::CreateTranslation(0.f, 3.f, 0.f);

		// Directional light.
		mDirLight.Ambient = { 0.2f, 0.2f, 0.2f, 1.0f };
		mDirLight.Diffuse = { 0.5f, 0.5f, 0.5f, 1.0f };
		mDirLight.Specular = { 0.5f, 0.5f, 0.5f, 1.0f };
		mDirLight.Direction = { 0.57735f, -0.57735f, 0.57735f };

		// Point light--position is changed every frame to animate in UpdateScene function.
		mPointLight.Ambient = { 0.3f, 0.3f, 0.3f, 1.0f };
		mPointLight.Diffuse = { 0.7f, 0.7f, 0.7f, 1.0f };
		mPointLight.Specular = { 0.7f, 0.7f, 0.7f, 1.0f };
		mPointLight.AttenuationParam = { 0.0f, 0.1f, 0.0f };
		mPointLight.Range = 100.0f;

		// Spot light--position and direction changed every frame to animate in UpdateScene function.
		mSpotLight.Ambient = { 0.0f, 0.0f, 0.0f, 1.0f };
		mSpotLight.Diffuse = { 1.0f, 1.0f, 0.0f, 1.0f };
		mSpotLight.Specular = { 1.0f, 1.0f, 1.0f, 1.0f };
		mSpotLight.AttenuationParam = { 1.0f, 0.0f, 0.0f };
		mSpotLight.Spot = 96.0f;
		mSpotLight.Range = 10000.0f;

		mLandModel.Material.Ambient = { 0.48f, 0.77f, 0.46f, 1.0f };
		mLandModel.Material.Diffuse = { 0.48f, 0.77f, 0.46f, 1.0f };
		mLandModel.Material.Specular = { 0.2f, 0.2f, 0.2f, 16.0f };

		mWavesModel.Material.Ambient = { 0.137f, 0.42f, 0.556f, 1.0f };
		mWavesModel.Material.Diffuse = { 0.137f, 0.42f, 0.556f, 1.0f };
		mWavesModel.Material.Specular = { 0.8f, 0.8f, 0.8f, 96.0f };

		Material material;
		material.Ambient = { 0.651f, 0.5f, 0.392f, 1.0f };
		material.Diffuse = { 0.651f, 0.5f, 0.392f, 1.0f };
		material.Specular = { 0.2f, 0.2f, 0.2f, 16.0f };
		mShapeMaterials.insert({ eShapeType::Box, material });

		material.Ambient = { 0.7f, 0.85f, 0.7f, 1.0f };
		material.Diffuse = { 0.7f, 0.85f, 0.7f, 1.0f };
		material.Specular = { 0.8f, 0.8f, 0.8f, 16.0f };
		mShapeMaterials.insert({ eShapeType::Cyliner, material });

		material.Ambient = { 0.48f, 0.77f, 0.46f, 1.0f };
		material.Diffuse = { 0.48f, 0.77f, 0.46f, 1.0f };
		material.Specular = { 0.2f, 0.2f, 0.2f, 16.0f };
		mShapeMaterials.insert({ eShapeType::Grid, material });

		material.Ambient = { 0.1f, 0.2f, 0.3f, 1.0f };
		material.Diffuse = { 0.2f, 0.4f, 0.6f, 1.0f };
		material.Specular = { 0.9f, 0.9f, 0.9f, 16.0f };
		mShapeMaterials.insert({ eShapeType::Sphere, material });

		mSkullModel.Material.Ambient = { 0.8f, 0.8f, 0.8f, 1.0f };
		mSkullModel.Material.Diffuse = { 0.8f, 0.8f, 0.8f, 1.0f };
		mSkullModel.Material.Specular = { 0.8f, 0.8f, 0.8f, 16.0f };

	}

	D3DSample::~D3DSample()
	{
		ReleaseCOM(mPerObjectCB);
		ReleaseCOM(mPerFrameCB);

		releaseModel(&mLandModel);
		releaseModel(&mWavesModel);
		releaseModel(&mSkullModel);

		ReleaseCOM(mShapeVB);
		ReleaseCOM(mShapeIB);

		ReleaseCOM(mVertexShader);
		ReleaseCOM(mVertexShaderBuffer);
		ReleaseCOM(mPixelShader);
		ReleaseCOM(mInputLayout);
	}

	void D3DSample::releaseModel(Model* model)
	{
		assert(model != nullptr);

		ReleaseCOM(model->VertexBuffer);
		ReleaseCOM(model->IndexBuffer);
	}

	bool D3DSample::Init()
	{
		if (!D3DProcessor::Init())
		{
			return false;
		}

		buildGeometryBuffers();
		buildShader();
		buildVertexLayout();

		return true;
	}

	void D3DSample::OnResize()
	{
		D3DProcessor::OnResize();

		mProj = DirectX::XMMatrixPerspectiveFovLH(0.25f * MathHelper::Pi, GetAspectRatio(), 1.f, 1000.f);
	}

	void D3DSample::Update(float deltaTime)
	{
		float x = mRadius * sinf(mPhi) * cosf(mTheta);
		float z = mRadius * sinf(mPhi) * sinf(mTheta);
		float y = mRadius * cosf(mPhi);

		mEyePosW = { x, y, z };

		DirectX::SimpleMath::Vector4 pos = { x, y, z, 1.f };
		DirectX::SimpleMath::Vector4 target = { 0.f, 0.f, 0.f, 0.f };
		DirectX::SimpleMath::Vector4 up = { 0.f, 1.f, 0.f, 0.f };

		mView = DirectX::XMMatrixLookAtLH(pos, target, up);

		static float t_base = 0.0f;
		if ((mTimer.GetTotalTime() - t_base) >= 0.25f)
		{
			t_base += 0.25f;

			DWORD i = 5 + rand() % (mWaves.GetRowCount() - 10);
			DWORD j = 5 + rand() % (mWaves.GetColumnCount() - 10);

			float r = MathHelper::RandF(1.f, 2.f);

			mWaves.Disturb(i, j, r);
		}

		mWaves.Update(deltaTime);

		D3D11_MAPPED_SUBRESOURCE mappedData;
		HR(md3dContext->Map(mWavesModel.VertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedData));

		Vertex* vertices = reinterpret_cast<Vertex*>(mappedData.pData);
		for (UINT i = 0; i < mWaves.GetVertexCount(); ++i)
		{
			vertices[i].Position = mWaves[i];
			vertices[i].Normal = mWaves.GetNormal(i);
		}

		md3dContext->Unmap(mWavesModel.VertexBuffer, 0);

		// Circle light over the land surface.
		//mPointLight.Position.x = 70.0f * cosf(0.2f * mTimer.GetTotalTime());
		//mPointLight.Position.z = 70.0f * sinf(0.2f * mTimer.GetTotalTime());
		//mPointLight.Position.y = 25; // MathHelper::Max(getHillHeight(mPointLight.Position.x, mPointLight.Position.z), -3.0f) + 10.0f;

		mSpotLight.Position = mEyePosW;
		mSpotLight.Direction = DirectX::SimpleMath::Vector3(target - pos);
		mSpotLight.Direction.Normalize();

		if (GetAsyncKeyState('1') & 0x8000)
		{
			mPointLight.Range += deltaTime * 1000;
		}
		else if (GetAsyncKeyState('2') & 0x8000)
		{
			mPointLight.Range -= deltaTime * 1000;
		}
	}

	void D3DSample::Render()
	{
		assert(md3dContext);
		assert(mSwapChain);

		float color[] = { 1.0f, 1.0f, 0.0f, 1.0f };

		md3dContext->ClearRenderTargetView(mRenderTargetView, color);
		md3dContext->ClearDepthStencilView(mDepthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

		md3dContext->IASetInputLayout(mInputLayout);
		md3dContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		md3dContext->VSSetShader(mVertexShader, NULL, 0);

		md3dContext->PSSetShader(mPixelShader, NULL, 0);

		{
			mCBPerFrame.DirLight = mDirLight;
			mCBPerFrame.PointLight = mPointLight;
			mCBPerFrame.SpotLight = mSpotLight;
			mCBPerFrame.EyePosW = mEyePosW;

			md3dContext->UpdateSubresource(mPerFrameCB, 0, NULL, &mCBPerFrame, 0, 0);

			md3dContext->PSSetConstantBuffers(0, 1, &mPerFrameCB);
		}

		drawModel(mSkullModel, mSkullWorld);
		drawModel(mWavesModel, mWaveWorld);
		drawModel(mLandModel, mLandWorld);

		for (const std::pair<eShapeType, DirectX::SimpleMath::Matrix>& pair : mWorldMatrixs)
		{
			drawShape(pair.first, pair.second);
		}

		mSwapChain->Present(0, 0);
	}

	void D3DSample::OnMouseDown(WPARAM btnState, int x, int y)
	{
		mLastMousePos.x = x;
		mLastMousePos.y = y;

		SetCapture(mhWnd);
	}

	void D3DSample::OnMouseUp(WPARAM btnState, int x, int y)
	{
		ReleaseCapture();
	}

	void D3DSample::OnMouseMove(WPARAM btnState, int x, int y)
	{
		if ((btnState & MK_LBUTTON) != 0)
		{
			// Make each pixel correspond to a quarter of a degree.
			float dx = DirectX::XMConvertToRadians(0.25f * static_cast<float>(x - mLastMousePos.x));
			float dy = DirectX::XMConvertToRadians(0.25f * static_cast<float>(y - mLastMousePos.y));

			// Update angles based on input to orbit camera around box.
			mTheta += dx;
			mPhi += dy;

			// Restrict the angle mPhi.
			mPhi = common::MathHelper::Clamp(mPhi, 0.1f, DirectX::XM_PI - 0.1f);
		}
		else if ((btnState & MK_RBUTTON) != 0)
		{
			// Make each pixel correspond to 0.005 unit in the scene.
			float dx = 0.05f * static_cast<float>(x - mLastMousePos.x);
			float dy = 0.05f * static_cast<float>(y - mLastMousePos.y);

			// Update the camera radius based on input.
			mRadius += dx - dy;

			// Restrict the radius.
			mRadius = common::MathHelper::Clamp(mRadius, 3.0f, 200.f);
		}

		mLastMousePos.x = x;
		mLastMousePos.y = y;
	}

	void D3DSample::drawModel(const Model& model, const Matrix& worldMatrix)
	{
		UINT stride = sizeof(Vertex);
		UINT offset = 0;
		md3dContext->IASetVertexBuffers(0, 1, &model.VertexBuffer, &stride, &offset);
		md3dContext->IASetIndexBuffer(model.IndexBuffer, DXGI_FORMAT_R32_UINT, 0);

		mCBPerObject.World = worldMatrix;
		mCBPerObject.WorldInvTranspose = MathHelper::InverseTranspose(worldMatrix);
		mCBPerObject.WorldViewProj = worldMatrix * mView * mProj;
		mCBPerObject.Material = model.Material;

		mCBPerObject.World = mCBPerObject.World.Transpose();
		mCBPerObject.WorldInvTranspose = mCBPerObject.WorldInvTranspose.Transpose();
		mCBPerObject.WorldViewProj = mCBPerObject.WorldViewProj.Transpose();

		md3dContext->UpdateSubresource(mPerObjectCB, 0, NULL, &mCBPerObject, 0, 0);

		md3dContext->VSSetConstantBuffers(0, 1, &mPerObjectCB);
		md3dContext->PSSetConstantBuffers(1, 1, &mPerObjectCB);

		md3dContext->DrawIndexed(model.IndexCount, 0, 0);
	}

	void D3DSample::drawObject(ID3D11Buffer* vertexBuffer, ID3D11Buffer* indexBuffer, UINT indexSize, const Material& material, const DirectX::SimpleMath::Matrix& worldMatrix)
	{
		UINT stride = sizeof(Vertex);
		UINT offset = 0;
		md3dContext->IASetVertexBuffers(0, 1, &vertexBuffer, &stride, &offset);
		md3dContext->IASetIndexBuffer(indexBuffer, DXGI_FORMAT_R32_UINT, 0);

		mCBPerObject.World = worldMatrix;
		mCBPerObject.WorldInvTranspose = MathHelper::InverseTranspose(worldMatrix);
		mCBPerObject.WorldViewProj = worldMatrix * mView * mProj;
		mCBPerObject.Material = material;

		mCBPerObject.World = mCBPerObject.World.Transpose();
		mCBPerObject.WorldInvTranspose = mCBPerObject.WorldInvTranspose.Transpose();
		mCBPerObject.WorldViewProj = mCBPerObject.WorldViewProj.Transpose();

		md3dContext->UpdateSubresource(mPerObjectCB, 0, NULL, &mCBPerObject, 0, 0);

		md3dContext->VSSetConstantBuffers(0, 1, &mPerObjectCB);
		md3dContext->PSSetConstantBuffers(1, 1, &mPerObjectCB);

		md3dContext->DrawIndexed(indexSize, 0, 0);
	}

	void D3DSample::drawShape(eShapeType shapeType, const DirectX::SimpleMath::Matrix& worldMatrix)
	{
		UINT stride = sizeof(Vertex);
		UINT offset = 0;
		md3dContext->IASetVertexBuffers(0, 1, &mShapeVB, &stride, &offset);
		md3dContext->IASetIndexBuffer(mShapeIB, DXGI_FORMAT_R32_UINT, 0);

		mCBPerObject.World = worldMatrix;
		mCBPerObject.WorldInvTranspose = MathHelper::InverseTranspose(worldMatrix);
		mCBPerObject.WorldViewProj = worldMatrix * mView * mProj;

		auto find = mShapeMaterials.find(shapeType);
		assert(find != mShapeMaterials.end());
		mCBPerObject.Material = find->second;

		mCBPerObject.World = mCBPerObject.World.Transpose();
		mCBPerObject.WorldInvTranspose = mCBPerObject.WorldInvTranspose.Transpose();
		mCBPerObject.WorldViewProj = mCBPerObject.WorldViewProj.Transpose();

		md3dContext->UpdateSubresource(mPerObjectCB, 0, NULL, &mCBPerObject, 0, 0);

		md3dContext->VSSetConstantBuffers(0, 1, &mPerObjectCB);
		md3dContext->PSSetConstantBuffers(1, 1, &mPerObjectCB);


		md3dContext->UpdateSubresource(mPerObjectCB, 0, NULL, &mCBPerObject, 0, 0);

		unsigned int shapeIndex = static_cast<unsigned int>(shapeType);
		md3dContext->DrawIndexed(mIndexCounts[shapeIndex], mIndexOffsets[shapeIndex], mVertexOffsets[shapeIndex]);
	}

	float D3DSample::getHillHeight(float x, float z) const
	{
		return 0.3f * (z * sinf(0.1f * x) + x * cosf(0.1f * z));
	}

	DirectX::SimpleMath::Vector3 D3DSample::getHillNormal(float x, float z) const
	{
		DirectX::SimpleMath::Vector3 n(
			-0.03f * z * cosf(0.1f * x) - 0.3f * cosf(0.1f * z),
			1.0f,
			-0.3f * sinf(0.1f * x) + 0.03f * x * sinf(0.1f * z));
		n.Normalize();

		return n;
	}

	void D3DSample::buildGeometryBuffers()
	{
		buildLand();
		buildWaves();
		buildShape();
		buildSkull();
		buildConstantBuffer();
	}

	void D3DSample::buildLand()
	{
		GeometryGenerator::MeshData grid;
		GeometryGenerator::CreateGrid(160.f, 160.f, 50, 50, &grid);

		mLandModel.IndexCount = grid.Indices.size();

		std::vector<Vertex> vertices(grid.Vertices.size());
		for (size_t i = 0; i < grid.Vertices.size(); ++i)
		{
			vertices[i].Position = grid.Vertices[i].Position;
			vertices[i].Position.y = getHillHeight(vertices[i].Position.x, vertices[i].Position.z);

			vertices[i].Normal = getHillNormal(vertices[i].Position.x, vertices[i].Position.z);
		}

		D3D11_BUFFER_DESC vbd;
		vbd.Usage = D3D11_USAGE_IMMUTABLE;
		vbd.ByteWidth = sizeof(Vertex) * grid.Vertices.size();
		vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		vbd.CPUAccessFlags = 0;
		vbd.MiscFlags = 0;
		D3D11_SUBRESOURCE_DATA vInitData;
		vInitData.pSysMem = &vertices[0];
		HR(md3dDevice->CreateBuffer(&vbd, &vInitData, &mLandModel.VertexBuffer));

		D3D11_BUFFER_DESC ibd;
		ibd.Usage = D3D11_USAGE_IMMUTABLE;
		ibd.ByteWidth = sizeof(UINT) * mLandModel.IndexCount;
		ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
		ibd.CPUAccessFlags = 0;
		ibd.MiscFlags = 0;
		D3D11_SUBRESOURCE_DATA iinitData;
		iinitData.pSysMem = &grid.Indices[0];
		HR(md3dDevice->CreateBuffer(&ibd, &iinitData, &mLandModel.IndexBuffer));

	}
	void D3DSample::buildWaves()
	{
		mWaves.Init(160, 160, 1.0f, 0.03f, 3.25f, 0.4f);

		D3D11_BUFFER_DESC vbd;
		vbd.Usage = D3D11_USAGE_DYNAMIC;
		vbd.ByteWidth = sizeof(Vertex) * mWaves.GetVertexCount();
		vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		vbd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		vbd.MiscFlags = 0;
		HR(md3dDevice->CreateBuffer(&vbd, 0, &mWavesModel.VertexBuffer));
		std::vector<UINT> indices(3 * mWaves.GetTriangleCount());

		UINT m = mWaves.GetRowCount();
		UINT n = mWaves.GetColumnCount();
		int k = 0;
		for (UINT i = 0; i < m - 1; ++i)
		{
			for (DWORD j = 0; j < n - 1; ++j)
			{
				indices[k] = i * n + j;
				indices[k + 1] = i * n + j + 1;
				indices[k + 2] = (i + 1) * n + j;

				indices[k + 3] = (i + 1) * n + j;
				indices[k + 4] = i * n + j + 1;
				indices[k + 5] = (i + 1) * n + j + 1;

				k += 6; // next quad
			}
		}

		D3D11_BUFFER_DESC ibd;
		ibd.Usage = D3D11_USAGE_IMMUTABLE;
		ibd.ByteWidth = sizeof(UINT) * indices.size();
		ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
		ibd.CPUAccessFlags = 0;
		ibd.MiscFlags = 0;
		D3D11_SUBRESOURCE_DATA iinitData;
		iinitData.pSysMem = &indices[0];
		HR(md3dDevice->CreateBuffer(&ibd, &iinitData, &mWavesModel.IndexBuffer));

		mWavesModel.IndexCount = indices.size();
	}

	void D3DSample::buildShape()
	{
		GeometryGenerator::MeshData box;
		GeometryGenerator::MeshData grid;
		GeometryGenerator::MeshData sphere;
		GeometryGenerator::MeshData cylinder;

		GeometryGenerator::CreateBox(1.f, 1.f, 1.f, &box);
		GeometryGenerator::CreateGrid(1000.0f, 1000.0f, 60, 40, &grid);
		GeometryGenerator::CreateSphere(0.5f, 20, 20, &sphere);
		//geoGen.CreateGeosphere(0.5f, 2, sphere);
		GeometryGenerator::CreateCylinder(0.5f, 0.3f, 3.0f, 20, 20, &cylinder);

		mVertexOffsets.push_back(0);
		mVertexOffsets.push_back(box.Vertices.size());
		mVertexOffsets.push_back(mVertexOffsets.back() + grid.Vertices.size());
		mVertexOffsets.push_back(mVertexOffsets.back() + sphere.Vertices.size());

		mIndexCounts.push_back(box.Indices.size());
		mIndexCounts.push_back(grid.Indices.size());
		mIndexCounts.push_back(sphere.Indices.size());
		mIndexCounts.push_back(cylinder.Indices.size());

		mIndexOffsets.push_back(0);
		mIndexOffsets.push_back(box.Indices.size());
		mIndexOffsets.push_back(mIndexOffsets.back() + grid.Indices.size());
		mIndexOffsets.push_back(mIndexOffsets.back() + sphere.Indices.size());

		UINT totalVertexCount =
			box.Vertices.size() +
			grid.Vertices.size() +
			sphere.Vertices.size() +
			cylinder.Vertices.size();

		UINT totalIndexCount =
			box.Indices.size() +
			grid.Indices.size() +
			sphere.Indices.size() +
			cylinder.Indices.size();

		std::vector<Vertex> vertices;
		vertices.reserve(totalVertexCount);

		auto pushBack = [&vertices](const GeometryGenerator::MeshData& meshData)
			{
				for (const GeometryGenerator::Vertex& geoVertex : meshData.Vertices)
				{
					Vertex vertex;
					vertex.Position = geoVertex.Position;
					vertex.Normal = geoVertex.Normal;

					vertices.push_back(vertex);
				}
			};

		pushBack(box);
		pushBack(grid);
		pushBack(sphere);
		pushBack(cylinder);

		D3D11_BUFFER_DESC vbd;
		vbd.Usage = D3D11_USAGE_IMMUTABLE;
		vbd.ByteWidth = sizeof(Vertex) * totalVertexCount;
		vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		vbd.CPUAccessFlags = 0;
		vbd.MiscFlags = 0;
		D3D11_SUBRESOURCE_DATA vinitData;
		vinitData.pSysMem = &vertices[0];
		HR(md3dDevice->CreateBuffer(&vbd, &vinitData, &mShapeVB));

		std::vector<UINT> indices;
		indices.insert(indices.end(), box.Indices.begin(), box.Indices.end());
		indices.insert(indices.end(), grid.Indices.begin(), grid.Indices.end());
		indices.insert(indices.end(), sphere.Indices.begin(), sphere.Indices.end());
		indices.insert(indices.end(), cylinder.Indices.begin(), cylinder.Indices.end());

		D3D11_BUFFER_DESC ibd;
		ibd.Usage = D3D11_USAGE_IMMUTABLE;
		ibd.ByteWidth = sizeof(UINT) * totalIndexCount;
		ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
		ibd.CPUAccessFlags = 0;
		ibd.MiscFlags = 0;
		D3D11_SUBRESOURCE_DATA iinitData;
		iinitData.pSysMem = &indices[0];
		HR(md3dDevice->CreateBuffer(&ibd, &iinitData, &mShapeIB));

		using namespace DirectX::SimpleMath;
		Matrix worldMat = Matrix::CreateScale(3.f, 1.f, 3.f) * Matrix::CreateTranslation(0.f, 0.5f, 0.f);
		mWorldMatrixs.push_back({ eShapeType::Box, worldMat });
		worldMat = Matrix::Identity;
		mWorldMatrixs.push_back({ eShapeType::Grid, worldMat });

		for (int i = 0; i < 5; ++i)
		{
			mWorldMatrixs.push_back({ eShapeType::Cyliner, Matrix::CreateTranslation(-5.0f, 1.5f, -10.0f + i * 5.0f) });
			mWorldMatrixs.push_back({ eShapeType::Cyliner, Matrix::CreateTranslation(+5.0f, 1.5f, -10.0f + i * 5.0f) });

			mWorldMatrixs.push_back({ eShapeType::Sphere, Matrix::CreateTranslation(-5.0f, 3.5f, -10.0f + i * 5.0f) });
			mWorldMatrixs.push_back({ eShapeType::Sphere, Matrix::CreateTranslation(+5.0f, 3.5f, -10.0f + i * 5.0f) });
		}
	}

	void D3DSample::buildSkull()
	{
		std::ifstream fin("../Resource/Models/skull.txt");

		if (!fin)
		{
			MessageBox(0, L"Models/skull.txt not found.", 0, 0);
			return;
		}

		UINT vcount = 0;
		UINT tcount = 0;
		std::string ignore;

		fin >> ignore >> vcount;
		fin >> ignore >> tcount;
		fin >> ignore >> ignore >> ignore >> ignore;

		float nx, ny, nz;

		std::vector<Vertex> vertices(vcount);
		for (UINT i = 0; i < vcount; ++i)
		{
			fin >> vertices[i].Position.x >> vertices[i].Position.y >> vertices[i].Position.z;
			fin >> vertices[i].Normal.x >> vertices[i].Normal.y >> vertices[i].Normal.z;
		}

		fin >> ignore;
		fin >> ignore;
		fin >> ignore;

		mSkullModel.IndexCount = 3 * tcount;
		std::vector<UINT> indices(mSkullModel.IndexCount);
		for (UINT i = 0; i < tcount; ++i)
		{
			fin >> indices[i * 3 + 0] >> indices[i * 3 + 1] >> indices[i * 3 + 2];
		}

		fin.close();

		D3D11_BUFFER_DESC vbd;
		vbd.Usage = D3D11_USAGE_IMMUTABLE;
		vbd.ByteWidth = sizeof(Vertex) * vcount;
		vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		vbd.CPUAccessFlags = 0;
		vbd.MiscFlags = 0;
		D3D11_SUBRESOURCE_DATA vinitData;
		vinitData.pSysMem = &vertices[0];
		HR(md3dDevice->CreateBuffer(&vbd, &vinitData, &mSkullModel.VertexBuffer));

		//
		// Pack the indices of all the meshes into one index buffer.
		//

		D3D11_BUFFER_DESC ibd;
		ibd.Usage = D3D11_USAGE_IMMUTABLE;
		ibd.ByteWidth = sizeof(UINT) * mSkullModel.IndexCount;
		ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
		ibd.CPUAccessFlags = 0;
		ibd.MiscFlags = 0;
		D3D11_SUBRESOURCE_DATA iinitData;
		iinitData.pSysMem = &indices[0];
		HR(md3dDevice->CreateBuffer(&ibd, &iinitData, &mSkullModel.IndexBuffer));

		using namespace DirectX::SimpleMath;

		mSkullWorld = Matrix::CreateScale(0.5f, 0.5f, 0.5f) * Matrix::CreateTranslation(0.f, 1.f, 0.f);
	}

	void D3DSample::buildConstantBuffer()
	{
		D3D11_BUFFER_DESC cbd;
		cbd.Usage = D3D11_USAGE_DEFAULT;
		static_assert(sizeof(CBPerObject) % 16 == 0, "must be align");
		cbd.ByteWidth = sizeof(CBPerObject);
		cbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		cbd.CPUAccessFlags = 0;
		cbd.MiscFlags = 0;
		HR(md3dDevice->CreateBuffer(&cbd, NULL, &mPerObjectCB));

		static_assert(sizeof(CBPerFrame) % 16 == 0, "must be align");
		cbd.ByteWidth = sizeof(CBPerFrame);
		HR(md3dDevice->CreateBuffer(&cbd, NULL, &mPerFrameCB));
	}

	void D3DSample::buildShader()
	{
		HR(D3DHelper::CompileShaderFromFile(L"../Resource/Shader/LightingVS.hlsl", "main", "vs_5_0", &mVertexShaderBuffer));

		HR(md3dDevice->CreateVertexShader(
			mVertexShaderBuffer->GetBufferPointer(),
			mVertexShaderBuffer->GetBufferSize(),
			NULL,
			&mVertexShader));

		ID3DBlob* pixelShaderBuffer = nullptr;
		HR(D3DHelper::CompileShaderFromFile(L"../Resource/Shader/LightingPS.hlsl", "main", "ps_5_0", &pixelShaderBuffer));

		HR(md3dDevice->CreatePixelShader(
			pixelShaderBuffer->GetBufferPointer(),
			pixelShaderBuffer->GetBufferSize(),
			NULL,
			&mPixelShader));

		ReleaseCOM(pixelShaderBuffer);
	}

	void D3DSample::buildVertexLayout()
	{
		D3D11_INPUT_ELEMENT_DESC vertexDesc[] =
		{
			{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
			{"NORMAL",    0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0}
		};

		HR(md3dDevice->CreateInputLayout(
			vertexDesc,
			ARRAYSIZE(vertexDesc),
			mVertexShaderBuffer->GetBufferPointer(),
			mVertexShaderBuffer->GetBufferSize(),
			&mInputLayout));
	}
}