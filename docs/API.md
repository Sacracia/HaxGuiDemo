

## API

### Life Cycle

The library is built on the Immediate Mode GUI paradigm. Unlike traditional retained-mode frameworks, the UI is reconstructed and rendered from scratch every frame. All API calls must be executed within target application's render thread, strictly between the frame initialization and the final output command.

| Function | Description |
| :--- | :--- |
| `Initialize(hwnd, device)` | Initializes the library specifically for DirectX11. Must be called once before any other API calls. |
| `Shutdown()` | Shuts down the library and releases all allocated resources. |
| `BeginFrame()` | Synchronizes the library state (input, time) with the new frame. |
| `EndFrame()` | Finalizes the frame and outputs the graphics to the view |
| `HandleWndMsg(...)` | Processes window events (input, resizing, etc.). Must be called within the system message 

```cpp
// Rendering

LRESULT Hooked_Present(IDXGISwapChain* swapChain, UINT syncInterval, UINT flags)
{
    if (!Hax::Gui::Initialized())
    {
        // ...
        Hax::Gui::Initialize(...);
        // ...
    }

    Hax::Gui::BeginFrame();
    // ...
    Hax::Gui::EndFrame();

    return Original_Present(swapChain, syncInterval, flags);
}

// Window Proc

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (Hax::Gui::HandleWndMsg((void*)hWnd, msg, wParam, lParam))
        return 1;
    //...
}

or hooked PeekMessageA / PeekMessageW / GetMessageA / GetMessageW:

static BOOL WINAPI Hooked_GetMessageW(LPMSG lpMsg, HWND  hWnd, UINT wMsgFilterMin, UINT  wMsgFilterMax)
{
    if (!Orig_GetMessageW(lpMsg, hWnd, wMsgFilterMin, wMsgFilterMax))
        return FALSE;

    return TRUE;
}

```

### Render Primitives

Base functions for graphical output. Most functions take optional `Params` structures for styling.

| Function | Description |
| :--- | :--- |
| `DrawRect(...)` | Draws a rectangle between two points. |
| `DrawEllipse(...)` | Draws an ellipse with a given center and radius. |
| `DrawCircle(...)` | Draws a circle. |
| `DrawTriangle(...)` | Draws a triangle by 3 points. |
| `DrawLine(...)` | Draws a line between two points. |
| `DrawImage(...)` | Renders a texture into the specified area. |
| `DrawString(MsdfFontHandle, ...)` | Renders MSDF text. |
| `DrawString(FontHandle, ...)` | Renders standard bitmap text. |

```cpp
// Initialization:
{
    extern const Hax::uint8 Inter_Regular[65824]; // From demo
    extern const Hax::uint8 Batperson192[4626];   // From demo

    Hax::Gui::Initialize((Hax::Handle)hwnd, g_pd3dDevice);
    Hax::Gui::FontHandle hFont = Hax::Gui::LoadFont(Inter_Regular);
    Hax::Gui::TextureHandle hImage = Hax::Gui::LoadImageFromMemory(Batperson192);
}

// Rendering:
{
    Hax::Gui::BeginFrame();

    Hax::Vector2 pos = {100.f, 100.f};
    Hax::Gui::DrawLine(pos, pos + Hax::Vector2(50.f, 50.f), {.FillColor = 0xFF0000FF, .Th = 3.f}); 
    pos.X += 60.f;
    Hax::Gui::DrawRect
    (
        pos,
        {pos.X + 70.f, pos.Y + 50.f}, 
        {
            .BorderColor = 0x00FF00FF,
            .BorderTh = 5.f,
            .FillColor = 0xFF0000FF,
            .Rounding = {5.f, 5.f, 0.f, 0.f}
        }
    );
    pos.X += 150.f;
    Hax::Gui::DrawEllipse(pos, {50.f, 20.f}, {.FillColor = 0xFF0000FF});
    pos.X += 150.f;
    Hax::Gui::DrawCircle(pos, 50.f, {.FillColor = 0xFF0000FF});
    pos.X = 100.f;
    pos.Y += 100.f;
    Hax::Gui::DrawTriangle
    (
        {pos.X + 50.f / 2, pos.Y}, 
        {pos.X, pos.Y + 50.f}, 
        {pos.X + 50.f, pos.Y + 50.f},
        {.FillColor = 0xFF0000FF}
    );
    pos.X += 150.f;
    Hax::Gui::DrawImage(hImage, pos, {pos.X + 100.f, pos.Y + 100.f});
    pos.X += 150.f;
    Hax::Gui::DrawString(hFont, L"Text", pos, 15.f, {.Color = 0x00FF00FF});

    Hax::Gui::EndFrame();
}
```

<p align="center">
  <img src="https://i.imgur.com/lmeiqJN.png">
</p>

### Layers

Layers allow you to group elements and control their rendering order (Z-index) independently of the call sequence.


| Function | Description |
| :--- | :--- |
| `CreateLayer(name, zOrder)` | Registers a new layer. Higher `zOrder` values appear on top. |
| `SwitchLayer(name)` | Switches the current drawing context to the specified layer. |
| `RestoreLayer()` | Returns to the previous layer in the stack. |

```cpp
// Initialization:
{
    Gui::Initialize(...);
    Gui::CreateLayer(L"Background", -1);
    Gui::CreateLayer(L"Foreground", 1);
}

// Rendering:
{
    Hax::Gui::BeginFrame();

    Hax::Vector2 drawPos = {100.f, 100.f};
    const float r = 50_px;

    Hax::Gui::SwitchLayer(L"Foreground"); // Default -> Foreground
    Hax::Gui::DrawCircle(drawPos, r, {.FillColor = 0xFF0000FF});
    Hax::Gui::SwitchLayer(L"Background"); // Foreground -> Background
    drawPos.X += r * 0.8f;
    Hax::Gui::DrawCircle(drawPos, r, {.FillColor = 0x00FF00FF});
    Hax::Gui::SwitchLayer(L"Default");  // Background -> Default
    drawPos.X += r * 0.8f;
    Hax::Gui::DrawCircle(drawPos, r, {.FillColor = 0x0000FFFF}); 
    Hax::Gui::RestoreLayer(); // Default -> Background
    Hax::Gui::RestoreLayer(); // Background -> Foreground
    drawPos.X += r * 0.8f;
    Hax::Gui::DrawCircle(drawPos, r, {.FillColor = 0x000000FF}); 

    Hax::Gui::EndFrame();
}
```

<p align="center">
  <img src="https://i.imgur.com/raX9iWE.png" alt="Описание">
</p>

### Positioning (Layout)

Tools for automatic and manual placement of elements.

| Function | Description |
| :--- | :--- |
| `BeginVertical(spacing)` | Starts a vertical layout group. Elements will be placed one below another. |
| `EndVertical()` | Finalizes the vertical layout group. |
| `BeginHorizontal(spacing)` | Starts a horizontal layout group. Elements will be placed side by side. |
| `EndHorizontal()` | Finalizes the horizontal layout group. |
| `PlaceItem(size)` | Advances the cursor by the specified size, reserving space for an element. |
| `Space(pixels)` | Adds a fixed empty space (offset) along the current layout axis. |
| `Dummy(size)` | An invisible element that occupies space. |
| `GetCursorPos()` | Returns the current layout cursor position. |
| `SetCursorPos(pos)` | Manually overrides the layout cursor position. Not available inside a user layout |

```cpp

// Widgets:

static void SimpleWidget(Hax::Vector2 size, Hax::Gui::Color col)
{
    const Hax::Vector2 cursorPos = Hax::Gui::GetCursorPos();
    Hax::Gui::PlaceItem(size);
    Hax::Gui::DrawRect(cursorPos, cursorPos + size, {.FillColor = col});
}

// Initialization:
{
    Hax::Gui::Initialize((Hax::Handle)hwnd, g_pd3dDevice);
}

// Render:
{
    Hax::Gui::BeginFrame();

    const Hax::Vector2 size = {50.f, 100.f};
    Hax::Gui::SetCursorPos({100.f, 100.f});
    Hax::Gui::BeginHorizontal(3.f);
    {
        Hax::Gui::BeginVertical(3.f);
        {
            SimpleWidget(size, Hax::Gui::Color::Red);
            SimpleWidget(size, Hax::Gui::Color::Red);
            SimpleWidget(size, Hax::Gui::Color::Red);
        }
        Hax::Gui::EndVertical();

        Hax::Gui::BeginVertical(10.f);
        {
            SimpleWidget({size.Y, size.X}, Hax::Gui::Color::Green);
            Hax::Gui::BeginHorizontal(2.f);
            {
                SimpleWidget(size, Hax::Gui::Color::Green);
                SimpleWidget(size, Hax::Gui::Color::Green);
            }
            Hax::Gui::EndHorizontal();
        }
        Hax::Gui::EndVertical();

        Hax::Gui::Dummy(size);

        SimpleWidget(size, Hax::Gui::Color::Blue);
    }
    Hax::Gui::EndHorizontal();

    Hax::Gui::EndFrame();
}
```

<p align="center">
  <img src="https://i.imgur.com/MoCJ52Z.png">
</p>

### Mouse

Function for mouse state polling and cursor management.

| Function | Description |
| :--- | :--- |
| `GetMousePos()` | Current cursor coordinates within the viewport. |
| `GetMouseDeltaPos()` | Cursor movement offset relative to the previous frame. |
| `IsLmbPressed()` | Returns `true` if the left mouse button is pressed. |
| `IsLmbJustPressed()` | Returns `true` if the left mouse button was pressed in the current frame. |
| `IsLmbReleased()` | Returns `true` if the left mouse button is released. |
| `IsLmbJustReleased()` | Returns `true` if the left mouse button was released in the current frame. |
| `SetMouseIcon(icon)` | Sets the standard system cursor icon. |
| `SetMouseTexture(...)` | Sets a custom texture for the mouse cursor. |

```cpp
// Initialization:
{
    Hax::Gui::Initialize(...);
}

// Rendering:
{
    Hax::Gui::BeginFrame();

    const float r = 50_px;
    const Hax::Vector2 mousePos = Hax::Gui::GetMousePos();

    // Background circle
    {
        const Hax::Vector2 prevMousePos = mousePos - Hax::Gui::GetMouseDeltaPos();
        Hax::Gui::DrawCircle(prevMousePos, r, {.FillColor = 0x00FF00FF});
    }

    // Foreground circle
    {
        const Hax::Gui::Color color = Hax::Gui::IsLmbPressed() ? 0xFFFF00FF : 0xFF0000FF;
        Hax::Gui::DrawCircle(mousePos, r, {.FillColor = color});
    }

    Hax::Gui::EndFrame();
}
```

<p align="center">
  <img src="https://i.imgur.com/WIjqlr8.gif" alt="Описание гифки">
</p>

### Time

API for time tracking and framerate metrics.

| Function | Description |
| :--- | :--- |
| `GetTime()` | Time in seconds since the library initialization. |
| `GetDeltaTime()` | Time elapsed between the current and previous frames. |
| `GetFramerate()` | Current frames per second (FPS). |

```cpp
// Initialization:
{
    extern const Hax::uint8 Inter_Regular[65824]; // From demo
    Hax::Gui::Initialize(...);
    Hax::Gui::FontHandle hFont = Hax::Gui::LoadFont(Inter_Regular);
}

// Rendering:
{
    Hax::Gui::BeginFrame();

    // Circle
    {
        float t = fmod((float)Hax::Gui::GetTime() * 0.3f, 1.f);
        float oscillation = 1.f - fabsf(t * 2.0f - 1.f);
        float currentX = 60.f + (oscillation * 200_px);
        Hax::Gui::DrawCircle({currentX, 80_px}, 50_px, {.FillColor = 0xFF0000FF});
    }

    // FPS counter
    {
        Hax::WStringBuilder<8> sb;
        sb.AppendF(L"%.2f", Hax::Gui::GetFramerate());
        Hax::Gui::DrawString(hFont, sb.CStr(), {10.f, 10.f}, 13_px);
    }

    Hax::Gui::EndFrame();
}
```

<p align="center">
  <img src="https://i.imgur.com/WL8HqVq.gif" alt="Описание гифки">
</p>

### Drawing State Control

Manages global drawing state and visibility clipping.

| Function | Description |
| :--- | :--- |
| `PushSkipDrawing()` | Suspends all subsequent draw calls. |
| `PopSkipDrawing(count)` | Resumes drawing by popping states from the stack. |
| `IsDrawingSkipped()` | Returns true if drawing is currently suspended. |

```cpp
// Initialization:
{
    Hax::Gui::Initialize(...);
}

// Rendering:
{
    Hax::Gui::BeginFrame();

    const Hax::Rect bounds = {.Min = {100.f, 100.f}, .Max = {150.f, 150.f}};
    Hax::Gui::DrawRect(bounds.Min, bounds.Max, {.FillColor = 0x00FF00FF});

    Hax::Gui::PushSkipDrawing();
    if (bounds.Contains(Hax::Gui::GetMousePos()))
        Hax::Gui::PopSkipDrawing();

    for (int i = 0; i < 5; ++i)
    {
        const Hax::Vector2 a = {170.f + 70.f * (float)i, 100.f};
        const Hax::Vector2 b = a + Hax::Vector2(50.f, 50.f);
        Hax::Gui::DrawRect(a, b, {.FillColor = 0xFF0000FF});
    }

    if (Hax::Gui::IsDrawingSkipped())
        Hax::Gui::PopSkipDrawing();

    Hax::Gui::EndFrame();
}
```

<p align="center">
  <img src="https://i.imgur.com/cUpu66h.gif">
</p>

### Rotation & Clipping

API for coordinate transformation and visibility clipping.

| Function | Description |
| :--- | :--- |
| `BeginRotation(angle)` | Starts rotation for subsequent draw calls. |
| `EndRotation()` | Ends the current rotation block. |
| `PushClipRect(rect)` | Pushes a new clipping boundary. |
| `PopClipRect()` | Restores the previous clipping boundary. |
| `IsItemVisible(rect)` | Returns true if the area is within clip bounds. |

```cpp
// Initialization:
{
    Hax::Gui::Initialize(...);
}

// Rendering:
{
    Hax::Gui::BeginFrame();

    float t = (float)Hax::Gui::GetTime();

    const Hax::Rect viewBounds = {.Min = {100.f, 100.f}, .Max = {500.f, 200.f}};
    Hax::Gui::PushClipRect(viewBounds);

    Hax::Gui::BeginRotation(t * 2.f);
    const Hax::Vector2 pos = {300.f + 100.f * cosf(t * 2.f) - 50.f, 150.f + 100.f * sinf(t * 2.f) - 50.f};
    Hax::Gui::DrawRect(pos, pos + Hax::Vector2(100.f, 100.f), {.FillColor = 0x00FF00FF});
    Hax::Gui::EndRotation();

    Hax::Gui::PopClipRect();
    Hax::Gui::DrawRect(viewBounds.Min, viewBounds.Max, {.BorderColor = 0xFF0000FF, .BorderTh = 3.f, .FillColor = 0x0});

    Hax::Gui::EndFrame();
}
```

<p align="center">
  <img src="https://i.imgur.com/GsBZSVk.gif">
</p>

### Interaction & Hit-testing

API for input state tracking and area hit-testing.

| Function | Description |
| :--- | :--- |
| `Interact(id, rect)` | Processes interaction for a specific area. |
| `IsItemHovered(id)` | `true` if mouse is over the element. |
| `IsItemFocused(id)` | `true` if element has keyboard focus. |
| `IsItemActive(id)` | `true` while element is held down. |
| `IsItemClicked(id)` | `true` on mouse release over element. |
| `IsItemPressed(id)` | `true` on initial click of the element. |
| `IsItemPressedRepeat(id)` | `true` for repeated press events while held. |

```cpp
// Widgets:
static void DragSquare(size_t id, Hax::Vector2& pos)
{
    const Hax::Rect bounds = {pos, pos + Hax::Vector2(100.f, 100.f)};
    auto res = Hax::Gui::Interact(id, bounds);

    if (res.Active) // or Hax::Gui::IsItemActive(id)
        pos += Hax::Gui::GetMouseDeltaPos();

    Hax::Gui::Color color = res.Pressed ? 0xFFFF00FF : (res.Hovered ? 0x00FF00FF : 0xFF0000FF);
    Hax::Gui::DrawRect(bounds.Min, bounds.Max, {.BorderColor = 0x454500FF, .BorderTh = 5.f, .FillColor = color});
}

// Initialization:
{
    Hax::Gui::Initialize(...);
}

// Render:
{
    Hax::Gui::BeginFrame();

    static Hax::Vector2 pos1 = {300.f, 300.f};
    DragSquare(HAX_LINE, pos1);
    static Hax::Vector2 pos2 = {450.f, 300.f};
    DragSquare(HAX_LINE, pos2);

    Hax::Gui::EndFrame();
}
```

<p align="center">
  <img src="https://i.imgur.com/Lqn0ScW.gif">
</p>

### Container

API for advanced layouts with auto-size, scrolling, and clipping.


| Function | Description |
| :--- | :--- |
| `BeginContainer(id, params)` | Starts a new scrollable or isolated area. |
| `EndContainer()` | Finalizes the current container block. |
| `GetContainerBounds()` | Returns current container absolute boundaries. |
| `GetContentRegionAvail()` | Returns remaining space within the container. |
| `ScrollYTo(pos)` | Sets the vertical scroll offset. |

```cpp

// Widgets:

static void DummyWidget(Hax::Vector2 size, Hax::Gui::Color color)
{
    Hax::Rect bounds;  
    bounds.Min = Hax::Gui::GetCursorPos();
    bounds.Max = bounds.Min + size;

    Hax::Gui::PlaceItem(size);
    Hax::Gui::DrawRect(bounds.Min, bounds.Max, {.FillColor = color});
}

// Initialization:
{
    Hax::Gui::Initialize((Hax::Handle)hwnd, g_pd3dDevice);
}

// Render:
{
    Hax::Gui::BeginFrame();

    const Hax::Vector2 startPos = {150.f, 150.f};
    Hax::Gui::SetCursorPos(startPos);
    Hax::Gui::BeginContainer(HAX_LINE, {.H = 300.f, .Clip = true, .ScrollY = true, .FitX = true});
    {
        Hax::Gui::BeginVertical(20.f);

        for (int i = 0; i < 10; ++i)
        {
            Hax::Gui::BeginHorizontal(20.f);
            for (int j = 0; j < 10; ++j)
            {
                const Hax::Gui::LinearColor color = {i * 0.1f, j * 0.1f, 0.5f, 1.0f};
                DummyWidget({50.f, 50.f}, color.ToColor());
            }
            Hax::Gui::EndHorizontal();
        }

        const Hax::Vector2 size = {(Hax::Gui::GetContainerBounds().GetSize().X - 20.f * 2.f) / 3.f, 50.f};
        Hax::Gui::BeginHorizontal(20.f);
        DummyWidget(size, 0xFF0000FF);
        DummyWidget(size, 0xFF0000FF);
        DummyWidget(size, 0xFF0000FF);
        Hax::Gui::EndHorizontal();

        Hax::Gui::EndVertical();
    }
    Hax::Gui::EndContainer();

    Hax::Gui::EndFrame();
}
```

<p align="center">
  <img src="https://i.imgur.com/oW2lA2z.gif">
</p>

### Text & Fonts

API for font loading, text metrics, and string editing.

| Function | Description |
| :--- | :--- |
| `LoadMsdfFonts(items)` | Batch loads MSDF fonts from memory. |
| `CalcTextSize(...)` | Measures string dimensions for a specific font. |
| `GetFontLineHeight(...)` | Returns vertical distance between text lines. |
| `GetFontAscent(...)` | Returns font ascent from the baseline. |
| `GetFontCapHeight(...)` | Returns the height of capital characters. |
| `GetGlyphSize(...)` | Returns dimensions of a specific glyph. |
| `GetFontMetrics(...)` | Returns a structure of all font metrics. |
| `LoadFont(data)` | Loads a font from a memory buffer. |
| `LoadSystemFont(...)` | Loads a Windows font by family name. |
| `UnloadFont(hFont)` | Frees font resources from memory. |
| `GetFontAtlas()` | Returns the current font atlas texture. |
| `StringEdit(...)` | Interactive widget for text input and editing. |

```cpp
// Widgets:
static void MarkedText(Hax::Gui::FontHandle hFont, Hax::Vector2 pos, Hax::WStringView text)
{
    const float fontH = 23.f;
    Hax::Gui::FontMetrics metrics = Hax::Gui::GetFontMetrics(hFont, fontH);

    const Hax::Rect totalBounds = {pos, pos + Hax::Gui::CalcTextSize(hFont, text, fontH)};
    Hax::Gui::DrawRect(totalBounds.Min, totalBounds.Max, {.FillColor = 0xFF0000FF});

    Hax::Rect ascentBounds = totalBounds;
    ascentBounds.Max.Y = ascentBounds.Min.Y + (metrics.LineHeight - metrics.Ascent);
    Hax::Gui::DrawRect(ascentBounds.Min, ascentBounds.Max, {.FillColor = 0x00FF00FF});

    Hax::Rect descentBounds = totalBounds;
    descentBounds.Min.Y = descentBounds.Max.Y - metrics.Descent;
    Hax::Gui::DrawRect(descentBounds.Min, descentBounds.Max, {.FillColor = 0x0000FFFF});

    Hax::Gui::DrawString(hFont, text, totalBounds.Min, fontH);
}

// Initialization:
{
    extern const Hax::uint8 Inter_Regular[65824]; // From demo

    Hax::Gui::Initialize(...);
    Hax::Gui::FontHandle hArial = Hax::Gui::LoadSystemFont(L"Arial", Hax::Gui::FontWeight::Bold);
    Hax::Gui::FontHandle hInter = Hax::Gui::LoadFont(Inter_Regular);
}

// Render:
{
    Hax::Gui::BeginFrame();

    Hax::Gui::BeginFrame();
    MarkedText(hArial, {100.f, 100.f}, L"Привет мир!");
    MarkedText(hInter, {100.f, 150.f}, L"Привет мир!");
    Hax::Gui::EndFrame();

    Hax::Gui::EndFrame();
}
```

<p align="center">
  <img src="https://i.imgur.com/Z3ADhuo.png">
</p>

### Keyboard

API for keyboard state polling and key event tracking.

| Function | Description |
| :--- | :--- |
| `IsKeyDown(vk)` | `true` while the key is held. |
| `IsKeyClicked(vk)` | `true` only on the frame the key was pressed. |
| `IsKeyDownRepeat(vk, d, i)` | `true` repeatedly while the key is held. |
| `IsKeyUp(vk)` | `true` while the key is released. |
| `IsKeyReleased(vk)` | `true` only on the frame the key was released. |
| `GetKeyName(vk)` | Returns the string name of the key. |
| `GetJustPressedKeys()` | Returns a list of keys pressed this frame. |

```cpp

// Widgets:

static void VirtualKey(Hax::Gui::FontHandle hFont, Hax::uint8 key)
{
    const Hax::Vector2 padding = {10.f, 5.f};
    const Hax::Vector2 size = Hax::Gui::CalcTextSize(hFont, Hax::Gui::GetKeyName(key), 14.f) + padding * 2.f;

    Hax::Rect bounds;
    bounds.Min = Hax::Gui::GetCursorPos();
    bounds.Max = bounds.Min + size;

    Hax::Gui::PlaceItem(size);
    if (Hax::Gui::IsItemVisible(bounds))
    {
        bounds.Min.Y += 5.f;
        bounds.Max.Y += 5.f;

        Hax::Gui::Color color = 0xFFFF00FF;

        if (Hax::Gui::IsKeyUp(key))
        {
            Hax::Gui::DrawRect(bounds.Min, bounds.Max, {.FillColor = 0xCBCBCBFF, .Rounding = 3.f});

            bounds.Min.Y -= 5.f;
            bounds.Max.Y -= 5.f;

            color = 0xF8F8F8FF;
        }

        Hax::Gui::DrawRect(bounds.Min, bounds.Max, {.BorderColor = 0xCBCBCBFF, .BorderTh = 1.f, .FillColor = color, .Rounding = 3.f});
        Hax::Gui::DrawString(hFont, Hax::Gui::GetKeyName(key), bounds.Min + padding, 14.f);
    }
}

// Initialization:
{
    extern const Hax::uint8 Inter_Regular[65824]; // From demo
    Hax::Gui::Initialize(...);
    Hax::Gui::FontHandle hFont = Hax::Gui::LoadFont(Inter_Regular);
}

// Rendering:
{
    Hax::Gui::BeginFrame();

    Hax::Gui::SetCursorPos({500.f, 500.f});

    Hax::Gui::BeginVertical(5.f);
    {
        Hax::Gui::BeginHorizontal(5.f);
        {
            static const Hax::uint8 s_Layout1[] = {'1', '2', '3', '4', '5', '6', '7', '8', '9', '0', VK_OEM_MINUS, VK_OEM_PLUS, VK_BACK};
            for (Hax::uint8 vk : s_Layout1) 
                VirtualKey(hFont, vk);
        }
        Hax::Gui::EndHorizontal();

        Hax::Gui::BeginHorizontal(5.f);
        {
            static const Hax::uint8 s_Layout2[] = {VK_TAB, 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', VK_OEM_4, VK_OEM_6, VK_OEM_5};
            for (Hax::uint8 vk : s_Layout2) 
                VirtualKey(hFont, vk);
        }
        Hax::Gui::EndHorizontal();

        Hax::Gui::BeginHorizontal(5.f);
        {
            static const Hax::uint8 s_Layout3[] = {VK_CAPITAL, 'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', VK_OEM_1, VK_OEM_7, VK_RETURN};
            for (Hax::uint8 vk : s_Layout3) 
                VirtualKey(hFont, vk);
        }
        Hax::Gui::EndHorizontal();

        Hax::Gui::BeginHorizontal(5.f;
        {
            static const Hax::uint8 s_Layout4[] = {VK_LSHIFT, 'Z', 'X', 'C', 'V', 'B', 'N', 'M', VK_OEM_COMMA, VK_OEM_PERIOD, VK_OEM_2, VK_RSHIFT};
            for (Hax::uint8 vk : s_Layout4) 
                VirtualKey(hFont, vk);
        }
        Hax::Gui::EndHorizontal();
    }
    Hax::Gui::EndVertical();

    Hax::Gui::EndFrame();
}
```

<p align="center">
  <img src="https://i.imgur.com/wOc5k6G.gif" alt="Описание гифки">
</p>

### Screen & Viewport

API for information about the application area and its dimensions

| Function | Description |
| :--- | :--- |
| `GetViewportSize()` | Returns the dimensions of the current drawing area. |
| `GetViewportCenter()` | Returns the center coordinates of the viewport. |
| `GetViewportBounds()` | Returns the full bounding rectangle of the viewport. |

```cpp
// Initialization:
{
    Hax::Gui::Initialize(...);
}

// Render:
{
    Hax::Gui::BeginFrame();

    const Hax::Rect bounds = Hax::Gui::GetViewportBounds();
    Hax::Gui::DrawRect(bounds.Min, {bounds.Max.X / 2.f, bounds.Max.Y}, {.FillColor = 0xFF0000FF});
    Hax::Gui::DrawRect({bounds.Max.X / 2.f, bounds.Min.Y}, bounds.Max, {.FillColor = 0x0000FFFF});

    const Hax::Vector2 center = Hax::Gui::GetViewportCenter();
    const Hax::Vector2 halfSize = {50.f, 50.f};
    Hax::Gui::DrawRect(center - halfSize, center + halfSize, {.FillColor = 0x00FF00FF});

    Hax::Gui::EndFrame();
}
```

<p align="center">
  <img src="https://i.imgur.com/VBUlOGL.gif">
</p>

### States

State management for persisting user data between frames.

| Function | Description |
| :--- | :--- |
| `GetState<T>(id)` | Retrieves or creates a persistent state object of type T associated with the given ID. |

```cpp
// Widgets:
struct DummyWidgetState
{
    Hax::Gui::LinearAnim Animation;
};

static bool MyAnimatedBtn(size_t id, Hax::Vector2 pos, Hax::Gui::FontHandle hFont)
{
    const Hax::Vector2 size = {180.f, 60.f};
    const Hax::Rect bounds = {pos, pos + size};

    DummyWidgetState& state = Hax::Gui::GetState<DummyWidgetState>(id);

    const float deltaTime = Hax::Gui::GetDeltaTime();
    auto interRes = Hax::Gui::Interact(id, bounds);
    state.Animation.Elapse(interRes.Hovered ? deltaTime : -deltaTime, 0.2f);
    
    constexpr Hax::Gui::LinearColor mainColor = 0xE64B3CFF;
    Hax::Gui::LinearColor bgColor = mainColor; bgColor.A = state.Animation.Progress;
    Hax::Gui::DrawRect(bounds.Min, bounds.Max, {.BorderColor = mainColor.ToColor(), .BorderTh = 2.f, .FillColor = bgColor.ToColor(), .Rounding = 10.f});

    const Hax::Gui::LinearColor fgColor = Hax::Lerp(mainColor, 0xFFFFFFFF, state.Animation.Progress);
    const Hax::Vector2 textSize = Hax::Gui::CalcTextSize(hFont, L"BUTTON", 16.f);
    Hax::Gui::DrawString(hFont, L"BUTTON", bounds.GetCenter() - textSize / 2.f, 16.f, {.Color = fgColor.ToColor()});

    if (interRes.Hovered)
        Hax::Gui::SetMouseIcon(Hax::Gui::MouseIcon_Hand);

    return interRes.Clicked;
}

// Initialization:
{
    Hax::Gui::Initialize(...);
    Hax::Gui::FontHandle hFont = Hax::Gui::LoadSystemFont(L"Segoe UI", Hax::Gui::FontWeight::Bold);
}

// Render:
{
    Hax::Gui::BeginFrame();
    MyAnimatedBtn(HAX_LINE, {100.f, 100.f}, hFont);
    Hax::Gui::EndFrame();
}
```

<p align="center">
  <img src="https://i.imgur.com/jv0jE16.gif">
</p>


### Scaling

API to adapt UI for different resolutions. Basically, 1_px = 1 for 1080 screen height

| Function | Description |
| :--- | :--- |
| `Scale(float val)` | Scales a scalar value based on the current global scale factor. |
| `Scale(Vector2 val)` | Scales a 2D vector. |
| `operator "" _px(...)` | Literal for convenient inline scaling (e.g., `100_px`). |

### Resources

| Function | Description |
| :--- | :--- |
| `GetResourceData(...)` | Loads raw binary data from a module resource (e.g., Win32 resources). |

### Images

| Function | Description |
| :--- | :--- |
| `LoadImageFromMemory(data)` | Decodes an image from memory and creates a GPU texture. |

### Lines (Utility)

| Function | Description |
| :--- | :--- |
| `VerticalLine(...)` | Draws a vertical separator line with specified thickness and color. |
| `HorizontalLine(...)` | Draws a horizontal separator line with specified thickness and color. |

### Demo

| Function | Description |
| :--- | :--- |
| `ShowDemoWindow()` | Opens a built-in window demonstrating library features and widgets. |

```cpp
// Initialization:
{
    Hax::Gui::Initialize(...);
}

// Render:
{
    Hax::Gui::BeginFrame();
    Hax::Gui::ShowDemoWindow();
    Hax::Gui::EndFrame();
}
```

<p align="center">
  <img src="https://i.imgur.com/1rlpf9i.gif">
</p>