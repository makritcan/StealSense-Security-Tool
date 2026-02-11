#pragma once
#include <d3d11.h>

namespace Gui {
    void Initialize(ID3D11Device* device, ID3D11DeviceContext* deviceContext);
    void Render();
    void Shutdown();
}
