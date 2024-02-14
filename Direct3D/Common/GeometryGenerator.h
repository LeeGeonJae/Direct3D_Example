#pragma once

namespace common
{
	using namespace DirectX::SimpleMath;

	class GeometryGenerator
	{
	public:
		struct Vertex
		{
			Vertex() = default;
			Vertex(const Vertex&) = default;
			Vertex& operator=(const Vertex&) = default;
			Vertex(Vector3 position, Vector3 normal, Vector3 tangent, Vector2 texture);
			Vertex(float px, float py, float pz, float nx, float ny, float nz, float tanx, float tany, float tanz, float tx, float ty);

			Vector3 Position;
			Vector3 Normal;
			Vector3 TangentU;
			Vector2 TexC;
		};

		struct MeshData
		{
			std::vector<Vertex> Vertices;
			std::vector<unsigned int> Indices;
		};

	public:
		static void CreateBox(float width, float height, float depth, MeshData* outMeshData);
		static void CreateSphere(float radius, UINT sliceCount, UINT stackCount, MeshData* outMeshData);
		static void CreateGeosphere(float radius, UINT numSubdivisions, MeshData* outMeshData);
		static void CreateCylinder(float bottomRadius, float topRadius, float height, UINT sliceCount, UINT stackCount, MeshData* outMeshData);
		static void CreateGrid(float width, float depth, UINT m, UINT n, MeshData* outMeshData);
		static void CreateFullscreenQuad(MeshData* outMeshData);

	private:
		static void subdivide(MeshData* meshData);
		static void buildCylinderTopCap(float bottomRadius, float topRadius, float height, UINT sliceCount, UINT stackCount, MeshData* outMeshData);
		static void buildCylinderBottomCap(float bottomRadius, float topRadius, float height, UINT sliceCount, UINT stackCount, MeshData* outMeshData);
	};
}