#include "RenderState.h"

ID3D11RasterizerState* RenderStates::WireFrameRS = nullptr;
ID3D11RasterizerState* RenderStates::NoCullRS = nullptr;

ID3D11BlendState* RenderStates::AlphaToCoverageBS = nullptr;
ID3D11BlendState* RenderStates::TransparentBS = nullptr;