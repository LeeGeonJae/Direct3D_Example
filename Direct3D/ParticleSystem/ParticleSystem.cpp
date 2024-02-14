#include "ParticleSystem.h"
#include "Camera.h"
#include "RenderStates.h"

namespace particleSystem
{

	ParticleSystem::ParticleSystem()
		: mInitVB(0), mDrawVB(0), mStreamOutVB(0), mTexArraySRV(0), mRandomTexSRV(0)
	{
		mFirstRun = true;
		mGameTime = 0.0f;
		mTimeStep = 0.0f;
		mAge = 0.0f;

		mEyePosW = Vector3(0.0f, 0.0f, 0.0f);
		mEmitPosW = Vector3(0.0f, 0.0f, 0.0f);
		mEmitDirW = Vector3(0.0f, 1.0f, 0.0f);
	}

	ParticleSystem::~ParticleSystem()
	{
		ReleaseCOM(mInputLayout);
		ReleaseCOM(mStreamOutVS);
		ReleaseCOM(mDrawVS);
		ReleaseCOM(mStreamOutGS);
		ReleaseCOM(mDrawGS);
		ReleaseCOM(mDrawPS);
		ReleaseCOM(mFrameCB);
		ReleaseCOM(mSamLinear);

		ReleaseCOM(mInitVB);
		ReleaseCOM(mDrawVB);
		ReleaseCOM(mStreamOutVB);
	}

	float ParticleSystem::GetAge()const
	{
		return mAge;
	}

	void ParticleSystem::SetEyePos(const Vector3& eyePosW)
	{
		mEyePosW = eyePosW;
	}

	void ParticleSystem::SetEmitPos(const Vector3& emitPosW)
	{
		mEmitPosW = emitPosW;
	}

	void ParticleSystem::SetEmitDir(const Vector3& emitDirW)
	{
		mEmitDirW = emitDirW;
	}

	void ParticleSystem::Init(ID3D11Device* device
		, const std::wstring& filename
		, ID3D11ShaderResourceView* texArraySRV
		, ID3D11ShaderResourceView* randomTexSRV
		, UINT maxParticles)
	{
		mTexArraySRV = texArraySRV;
		mRandomTexSRV = randomTexSRV;
		mMaxParticles = maxParticles;

		const D3D11_INPUT_ELEMENT_DESC particleDesc[5] =
		{
			{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,  0, D3D11_INPUT_PER_VERTEX_DATA, 0},
			{"VELOCITY", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
			{"SIZE",     0, DXGI_FORMAT_R32G32_FLOAT,    0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0},
			{"AGE",      0, DXGI_FORMAT_R32_FLOAT,       0, 32, D3D11_INPUT_PER_VERTEX_DATA, 0},
			{"TYPE",     0, DXGI_FORMAT_R32_UINT,        0, 36, D3D11_INPUT_PER_VERTEX_DATA, 0},
		};

		ID3DBlob* blob = nullptr;
		HR(D3DHelper::CompileShaderFromFile(filename.c_str(), "StreamOutVS", "vs_5_0", &blob));
		HR(device->CreateVertexShader(
			blob->GetBufferPointer(),
			blob->GetBufferSize(),
			NULL,
			&mStreamOutVS));
		ReleaseCOM(blob);
		HR(D3DHelper::CompileShaderFromFile(filename.c_str(), "DrawVS", "vs_5_0", &blob));
		HR(device->CreateVertexShader(
			blob->GetBufferPointer(),
			blob->GetBufferSize(),
			NULL,
			&mDrawVS));
		HR(device->CreateInputLayout(
			particleDesc,
			ARRAYSIZE(particleDesc),
			blob->GetBufferPointer(),
			blob->GetBufferSize(),
			&mInputLayout
		));
		ReleaseCOM(blob);

		// 스트림 출력용 기하 셰이더
		D3D11_SO_DECLARATION_ENTRY desc[] =
		{
			{ 0, "POSITION", 0, 0, 3, 0},
			{ 0, "VELOCITY", 0, 0, 3, 0},
			{ 0, "SIZE", 0, 0, 2, 0},
			{ 0, "AGE", 0, 0, 1, 0},
			{ 0, "TYPE", 0, 0, 1, 0},
		};
		UINT bufferStrides[] = { sizeof(Vertex) };
		HR(D3DHelper::CompileShaderFromFile(filename.c_str(), "StreamOutGS", "gs_5_0", &blob));
		HR(device->CreateGeometryShaderWithStreamOutput(
			blob->GetBufferPointer(), // 컴파일된 진입점
			blob->GetBufferSize(), // 컴파일된 사이즈
			desc, // 정점 형식
			ARRAYSIZE(desc), // 정점 형식 개수
			bufferStrides, // 정점 사이즈
			ARRAYSIZE(bufferStrides),
			0, // 래스터라이저 단계로 보낼 스트림 인덱스 번호
			NULL,
			&mStreamOutGS)); // out 매개변수
		ReleaseCOM(blob);

		HR(D3DHelper::CompileShaderFromFile(filename.c_str(), "DrawGS", "gs_5_0", &blob));
		HR(device->CreateGeometryShader(
			blob->GetBufferPointer(),
			blob->GetBufferSize(),
			NULL,
			&mDrawGS));
		ReleaseCOM(blob);

		HR(D3DHelper::CompileShaderFromFile(filename.c_str(), "DrawPS", "ps_5_0", &blob));
		HR(device->CreatePixelShader(
			blob->GetBufferPointer(),
			blob->GetBufferSize(),
			NULL,
			&mDrawPS));
		ReleaseCOM(blob);

		D3D11_BUFFER_DESC cbd;
		cbd.Usage = D3D11_USAGE_DEFAULT;
		static_assert(sizeof(PerFrame) % 16 == 0, "must be align");
		cbd.ByteWidth = sizeof(PerFrame);
		cbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		cbd.CPUAccessFlags = 0;
		cbd.MiscFlags = 0;
		HR(device->CreateBuffer(&cbd, NULL, &mFrameCB));
		Vertex p;
		ZeroMemory(&p, sizeof(Vertex));
		p.Age = 0.0f;
		p.Type = 0;

		D3D11_SAMPLER_DESC samplerDesc = {};
		samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
		samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
		samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
		samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
		samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER; // 비교를 통과하지 않는다?
		samplerDesc.MinLOD = 0;
		samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

		device->CreateSamplerState(&samplerDesc, &mSamLinear);

		// 스트림 출력용 정점 버퍼 만들기
		D3D11_BUFFER_DESC vbd;
		vbd.Usage = D3D11_USAGE_DEFAULT;
		vbd.ByteWidth = sizeof(Vertex) * 1;
		vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		vbd.CPUAccessFlags = 0;
		vbd.MiscFlags = 0;
		vbd.StructureByteStride = 0;
		D3D11_SUBRESOURCE_DATA vinitData;
		vinitData.pSysMem = &p;
		HR(device->CreateBuffer(&vbd, &vinitData, &mInitVB)); // 초기화용 방출기 입자

		vbd.ByteWidth = sizeof(Vertex) * mMaxParticles; // 최대로 다룰 버퍼 크기, 넘어가면 안댐
		vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER | D3D11_BIND_STREAM_OUTPUT; // 플래그 중요!
		HR(device->CreateBuffer(&vbd, 0, &mDrawVB));
		HR(device->CreateBuffer(&vbd, 0, &mStreamOutVB));
	}

	void ParticleSystem::Reset()
	{
		mFirstRun = true;
		mAge = 0.0f;
	}

	void ParticleSystem::Update(float dt, float gameTime)
	{
		mGameTime = gameTime;
		mTimeStep = dt;

		mAge += dt;
	}

	void ParticleSystem::Draw(ID3D11DeviceContext* dc, const Camera& cam)
	{
		UINT stride = sizeof(Vertex);
		UINT offset = 0;
		Matrix VP = cam.GetViewProj();

		// 갱신
		mPerFrame.ViewProj = VP.Transpose();
		mPerFrame.GameTime = mGameTime;
		mPerFrame.TimeStep = mTimeStep;
		mPerFrame.EyePosW = mEyePosW;
		mPerFrame.EmitPosW = mEmitPosW;
		mPerFrame.EmitDirW = mEmitDirW;

		dc->UpdateSubresource(mFrameCB, 0, 0, &mPerFrame, 0, 0);

		// 바인딩
		dc->IASetInputLayout(mInputLayout);
		dc->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);
		if (mFirstRun) // 처음 그릴 땐 초기화 버퍼로 그린다.
			dc->IASetVertexBuffers(0, 1, &mInitVB, &stride, &offset);
		else // 아니면 그냥 그리기용 버퍼 사용
			dc->IASetVertexBuffers(0, 1, &mDrawVB, &stride, &offset);

		dc->VSSetSamplers(0, 1, &mSamLinear);
		dc->VSSetShaderResources(0, 1, &mTexArraySRV);
		dc->VSSetShaderResources(1, 1, &mRandomTexSRV);

		dc->GSSetSamplers(0, 1, &mSamLinear);
		dc->GSSetShaderResources(0, 1, &mTexArraySRV);
		dc->GSSetShaderResources(1, 1, &mRandomTexSRV);

		dc->PSSetSamplers(0, 1, &mSamLinear);
		dc->PSSetShaderResources(0, 1, &mTexArraySRV);
		dc->PSSetShaderResources(1, 1, &mRandomTexSRV);

		// 스트림 아웃으로 정점 생성/삭제
		dc->VSSetShader(mStreamOutVS, 0, 0);
		dc->GSSetShader(mStreamOutGS, 0, 0);
		dc->PSSetShader(NULL, 0, 0); // 픽셀 셰이더 바인딩 안하기
		dc->OMSetDepthStencilState(RenderStates::DisableDepthDSS, 0); // 깊이 쓰기 끄기
		dc->SOSetTargets(1, // 묶을 정점 개수, 최대 4개
			&mStreamOutVB, // 출력용 버퍼
			&offset); // 정점 버퍼마다 기록하기 시작할 위치를 나타냄, 현재 0

		if (mFirstRun)
		{
			dc->VSSetConstantBuffers(0, 1, &mFrameCB);
			dc->GSSetConstantBuffers(0, 1, &mFrameCB);
			dc->PSSetConstantBuffers(0, 1, &mFrameCB);
			dc->Draw(1, 0);
			mFirstRun = false;
		}
		else
		{
			dc->DrawAuto(); // 정점 개수만큼 알아서 draw 해준다.
		}
		ID3D11Buffer* bufferArray[1] = { NULL};
		dc->OMSetDepthStencilState(0, 0);

		dc->SOSetTargets(1, bufferArray, &offset); // 출력 단계를 비워준다.
		std::swap(mDrawVB, mStreamOutVB); // 핑퐁을 위해 swap 한다.

		// 화면 렌더링
		dc->VSSetShader(mDrawVS, 0, 0);
		dc->GSSetShader(mDrawGS, 0, 0);
		dc->PSSetShader(mDrawPS, 0, 0);

		float factor[] = { 0.f, 0.f, 0.f, 0.f };
		dc->OMSetBlendState(RenderStates::AdditiveBlending, factor, 0xffffffff); // 가산 혼합
		dc->OMSetDepthStencilState(RenderStates::NoDepthWrites, 0); // 깊이 쓰기는 하지 않음
		dc->IASetVertexBuffers(0, 1, &mDrawVB, &stride, &offset);
		dc->DrawAuto();
		dc->OMSetBlendState(0, factor, 0xffffffff);
		dc->OMSetDepthStencilState(0, 0);

		dc->VSSetShader(0, 0, 0);
		dc->GSSetShader(0, 0, 0);
		dc->PSSetShader(0, 0, 0);
	}

}