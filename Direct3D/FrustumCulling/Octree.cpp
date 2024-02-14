#include <DirectXColors.h>
#include <DirectXCollision.h>

#include "Octree.h"
#include "Basic32.h"

namespace frustumCulling
{
	OctreeNode::OctreeNode()
	{
		for (int i = 0; i < 8; ++i)
			Children[i] = 0;

		Bounds.Center = Vector3(0.0f, 0.0f, 0.0f);
		Bounds.Extents = Vector3(0.0f, 0.0f, 0.0f);

		IsLeaf = false;
	}
	OctreeNode::~OctreeNode()
	{
		for (int i = 0; i < 8; ++i)
			SafeDelete(Children[i]);
	}
	void OctreeNode::Subdivide(BoundingBox box[8])
	{
		Vector3 halfExtent(
			0.5f * Bounds.Extents.x,
			0.5f * Bounds.Extents.y,
			0.5f * Bounds.Extents.z);

		box[0].Center = Vector3(
			Bounds.Center.x + halfExtent.x,
			Bounds.Center.y + halfExtent.y,
			Bounds.Center.z + halfExtent.z);
		box[0].Extents = halfExtent;

		box[1].Center = Vector3(
			Bounds.Center.x - halfExtent.x,
			Bounds.Center.y + halfExtent.y,
			Bounds.Center.z + halfExtent.z);
		box[1].Extents = halfExtent;

		box[2].Center = Vector3(
			Bounds.Center.x - halfExtent.x,
			Bounds.Center.y + halfExtent.y,
			Bounds.Center.z - halfExtent.z);
		box[2].Extents = halfExtent;

		box[3].Center = Vector3(
			Bounds.Center.x + halfExtent.x,
			Bounds.Center.y + halfExtent.y,
			Bounds.Center.z - halfExtent.z);
		box[3].Extents = halfExtent;

		// "Bottom" four quadrants.
		box[4].Center = Vector3(
			Bounds.Center.x + halfExtent.x,
			Bounds.Center.y - halfExtent.y,
			Bounds.Center.z + halfExtent.z);
		box[4].Extents = halfExtent;

		box[5].Center = Vector3(
			Bounds.Center.x - halfExtent.x,
			Bounds.Center.y - halfExtent.y,
			Bounds.Center.z + halfExtent.z);
		box[5].Extents = halfExtent;

		box[6].Center = Vector3(
			Bounds.Center.x - halfExtent.x,
			Bounds.Center.y - halfExtent.y,
			Bounds.Center.z - halfExtent.z);
		box[6].Extents = halfExtent;

		box[7].Center = Vector3(
			Bounds.Center.x + halfExtent.x,
			Bounds.Center.y - halfExtent.y,
			Bounds.Center.z - halfExtent.z);
		box[7].Extents = halfExtent;
	}

	Octree::Octree()
		: mRoot(nullptr)
	{
	}
	Octree::~Octree()
	{
		SafeDelete(mRoot);
	}

	void Octree::Build(BoundingBox initBox, size_t depth, const std::vector<Object*>& objects, const BoundingBox& objectBox)
	{
		mMaxDepth = depth;
		mRoot = new OctreeNode();
		mRoot->Bounds = initBox;
		mRoot->Depth = 0;
		mRoot->Objects = objects;

		buildRecursive(mRoot, *this, objects, objectBox);
	}
	void Octree::FindIntersectObject(const BoundingFrustum& worldfrustum, std::set<Object*>* outObjects, const BoundingBox& objectBox)
	{
		findIntersectObjectRecursive(mRoot, worldfrustum, outObjects, objectBox);
	}
	void Octree::DebugRender(ID3D11DeviceContext* dc, Basic32* basic32, const Matrix& VP)
	{
		debugRenderRecursive(mRoot, dc, basic32, VP);
	}
	void Octree::debugRenderRecursive(OctreeNode* node, ID3D11DeviceContext* dc, Basic32* basic32, const Matrix& VP)
	{
		if (node == nullptr)
		{
			return;
		}
		if (node->Objects.size() == 0)
		{
			return;
		}

		Matrix traslate = Matrix::CreateTranslation(node->Bounds.Center);
		Matrix scale = Matrix::CreateScale(abs(node->Bounds.Extents.x * 2));

		auto& perObject = basic32->GetPerObject();
		perObject.World = (scale * traslate);
		perObject.WorldViewProj = (perObject.World * VP);

		perObject.World = perObject.World.Transpose();
		perObject.WorldViewProj = perObject.WorldViewProj.Transpose();

		basic32->UpdateSubresource(dc);
		dc->DrawIndexed(36, 0, 0);

		for (int i = 0; i < 8; ++i)
		{
			debugRenderRecursive(node->Children[i], dc, basic32, VP);
		}
	}

	void Octree::buildRecursive(OctreeNode* parent, const Octree& octree, const std::vector<Object*>& objects, const BoundingBox& objectBox)
	{
		// 포함된 오브젝트가 100개 이하이면 더 이상 분할하지 않는다.
		if (objects.size() <= 100)
		{
			parent->IsLeaf = true;
			parent->Objects = objects;
			return;
		}

		parent->IsLeaf = false;

		BoundingBox subbox[8];
		parent->Subdivide(subbox);

		for (int i = 0; i < 8; ++i)
		{
			// 각 자식에게 바운딩 박스를 하나씩 할당해준다.
			parent->Children[i] = new OctreeNode();
			parent->Children[i]->Bounds = subbox[i];
			parent->Children[i]->Depth = parent->Depth + 1;
			parent->Children[i]->Objects.reserve(objects.size() / 2);

			for (const auto& object : objects)
			{
				BoundingBox localBoundingBox;
				subbox[i].Transform(localBoundingBox, object->World.Invert());

				// 겹침 판정이 날 경우 오브젝트를 공간에 추가해준다.
				if (localBoundingBox.Intersects(objectBox))
				{
					parent->Children[i]->Objects.push_back(object);
				}
			}
			// 재귀 호출
			buildRecursive(parent->Children[i], octree, parent->Children[i]->Objects, objectBox);
		}
	}
	void Octree::findIntersectObjectRecursive(OctreeNode* node, const BoundingFrustum& worldfrustum, std::set<Object*>* outObjects, const BoundingBox& objectBox)
	{
		switch (worldfrustum.Contains(node->Bounds))
		{
		case CONTAINS:
			outObjects->insert(node->Objects.begin(), node->Objects.end());
			return;
		case INTERSECTING:
			if (!node->IsLeaf)
			{
				break;
			}

			for (auto* object : node->Objects)
			{
				BoundingFrustum localFrustum;
				worldfrustum.Transform(localFrustum, object->World.Invert());

				if (localFrustum.Intersects(objectBox))
				{
					outObjects->insert(object);
				}
			}
			return;
		case DISJOINT:
			return;
		default:
			break;
		}

		for (int i = 0; i < 8; ++i)
		{
			findIntersectObjectRecursive(node->Children[i], worldfrustum, outObjects, objectBox);
		}
	}
}
