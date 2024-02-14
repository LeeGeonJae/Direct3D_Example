#pragma once

#include <DirectXCollision.h>

#include "D3DUtil.h"

namespace ambientOcclusion
{
	using namespace common;
	using namespace DirectX;
	using namespace SimpleMath;

	struct OctreeNode
	{
	public:
		OctreeNode();
		~OctreeNode();
		void Subdivide(BoundingBox box[8]);

	public:
		BoundingBox Bounds;
		std::vector<UINT> Indices;
		OctreeNode* Children[8];
		bool IsLeaf;
	};

	class Octree
	{
	public:
		Octree();
		~Octree();

		void Build(const std::vector<Vector3>& vertices, const std::vector<UINT>& indices);
		bool RayOctreeIntersect(Vector4 rayPos, Vector4 rayDir);

	private:
		BoundingBox buildAABB();
		void buildOctreeRecursive(OctreeNode* parent, const std::vector<UINT>& indices);
		bool rayOctreeIntersectRecursive(OctreeNode* parent, Vector4 rayPos, Vector4 rayDir);

	private:
		OctreeNode* mRoot;
		std::vector<Vector3> mVertices;
	};
};