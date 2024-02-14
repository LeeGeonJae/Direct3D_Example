#pragma once

namespace resourceManager
{
	struct Subset
	{
		Subset() :
			Id(-1),
			VertexStart(0), VertexCount(0),
			FaceStart(0), FaceCount(0)
		{
		}

		unsigned int Id;
		unsigned int VertexStart;
		unsigned int VertexCount;
		unsigned int FaceStart;
		unsigned int FaceCount;
	};
}