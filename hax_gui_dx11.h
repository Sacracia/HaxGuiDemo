#pragma once

#include "hax_gui.h"

struct ID3D11Device;

namespace Hax::Gui
{
    void Initialize(Handle hwnd, ID3D11Device* device);
}