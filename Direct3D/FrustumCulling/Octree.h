#pragma once

#include <DirectXCollision.h>
#include <vector>
#include <set>

#include "D3DUtil.h"
#include "Object.h"

namespace frustumCulling
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
		std::vector<Object*> Objects;
		OctreeNode* Children[8];
		size_t Depth;
		bool IsLeaf;
	};

	class Basic32;
	class Octree
	{
	public:
		Octree();
		~Octree();

		void Build(BoundingBox initBox, size_t maxDepth, const std::vector<Object*>& objects, const BoundingBox& objectBox);
		void FindIntersectObject(const BoundingFrustum& worldfrustum, std::set<Object*>* outObjects, const BoundingBox& objectBox);
		void DebugRender(ID3D11DeviceContext* dc, Basic32* basic32, const Matrix& VP);

	private:
		void buildRecursive(OctreeNode* node, const Octree& octree, const std::vector<Object*>& objects, const BoundingBox& objectBox);
		void findIntersectObjectRecursive(OctreeNode* node, const BoundingFrustum& worldfrustum, std::set<Object*>* outObjects, const BoundingBox& objectBox);
		void debugRenderRecursive(OctreeNode* node, ID3D11DeviceContext* dc, Basic32* basic32, const Matrix& VP);

	private:
		OctreeNode* mRoot;
		size_t mMaxDepth;
	};
};