

## API

### Life Cycle

The library follows the **Immediate Mode GUI** architecture. This means the interface is described and repainted "on the fly" every frame. All drawing calls must be placed strictly within the application loop between the frame setup and output commands.

| Function | Description |
| :--- | :--- |
| `Initialize(hwnd, device)` | Initializes the library. Must be called once before any other API calls. |
| `Shutdown()` | Shuts down the library and releases all allocated resources. |
| `BeginFrame()` | Synchronizes the library state (input, time) with the new frame. |
| `EndFrame()` | Finalizes the frame and outputs the image to the screen. |

```cpp
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
```

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
    Gui::Initialize((Hax::Handle)hwnd, g_pd3dDevice);
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

### Time

Time metrics are automatically updated during `BeginFrame()`.


| Function | Description |
| :--- | :--- |
| `GetTime()` | Time in seconds since the library initialization. |
| `GetDeltaTime()` | Time elapsed between the current and previous frames. |
| `GetFramerate()` | Current frames per second (FPS). |

### Mouse

Functions for retrieving cursor state and mouse button input.

| Function | Description |
| :--- | :--- |
| `GetMousePos()` | Current cursor coordinates within the viewport. |
| `GetMouseDeltaPos()` | Cursor movement offset relative to the previous frame. |
| `IsLmbJustPressed()` | Returns `true` if the left mouse button was pressed in the current frame. |
| `IsLmbJustReleased()` | Returns `true` if the left mouse button was released in the current frame. |
| `SetMouseIcon(icon)` | Sets the standard system cursor icon. |
| `SetMouseTexture(...)` | Sets a custom texture for the mouse cursor. |

### Keyboard


| Function | Description |
| :--- | :--- |
| `IsKeyDown(vk)` | Returns `true` while the key is being held down. |
| `IsKeyJustDown(vk)` | Returns `true` only in the frame the key was pressed. |
| `IsKeyDownRepeat(vk, delay, interval)` | Triggers repeatedly while the key is held, using specified delay and interval. |
| `IsKeyUp(vk)` | Returns `true` if the key is currently up. |
| `IsKeyJustUp(vk)` | Returns `true` only in the frame the key was released. |
| `GetKeyName(vk)` | Returns the string name of the key (e.g., "Enter"). |
| `GetJustPressedKeys()` | Returns a list of all keys pressed in the current frame. |

```cpp
Example: demo/VirtualKey
```

### WndProc

Integration with Windows system messages.

| Function | Description |
| :--- | :--- |
| `HandleWndMsg(...)` | Processes window events (input, resizing, etc.). Must be called within the system message loop. |

```cpp

WndProc (see main.cpp):

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (Hax::Gui::HandleWndMsg((void*)hWnd, msg, wParam, lParam))
        return 1;
    //...
}

or hooked PeekMessageA / PeekMessageW / GetMessageA / GetMessageW (see RepoHax/win_hooks):

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
| `DrawEllipse(...)` | Draws an ellipse with a given center and radii. |
| `DrawCircle(...)` | Draws a circle. |
| `DrawTriangle(...)` | Draws a filled or outlined triangle. |
| `DrawLine(...)` | Draws a line between two points. |
| `DrawImage(...)` | Renders a texture into the specified area. |
| `DrawString(MsdfFontHandle, ...)` | Renders MSDF text (remains sharp when scaled). |
| `DrawString(FontHandle, ...)` | Renders standard bitmap text. |

```cpp
See widgets from Demo / RepoHax
```

#### Drawing State Control

| Function | Description |
| :--- | :--- |
| `PushSkipDrawing()` | Pushes a "skip" state onto the stack. Subsequent draw calls will be ignored. |
| `PopSkipDrawing(count)` | Pops the specified number of skip states from the stack. |
| `IsDrawingSkipped()` | Checks if the drawing skip mode is currently active. |

```cpp
Example: Demo BeginPageSection
```

### Rotation & Clipping

| Function | Description |
| :--- | :--- |
| `BeginRotation(angleRad)` | Applies a rotation angle (in radians) to all subsequent draw calls. |
| `EndRotation()` | Resets the current rotation. |
| `PushClipRect(rect)` | Defines a clipping rectangle for the following commands. |
| `PopClipRect()` | Removes the last clipping rectangle. |

```cpp
Example: Demo Swap
```

### Clipping

Functions for defining and checking rendering boundaries.

| Function | Description |
| :--- | :--- |
| `PushClipRect(clipRect)` | Sets a clipping rectangle. All subsequent drawing will be restricted to this area. |
| `PopClipRect()` | Removes the last clipping rectangle from the stack. |
| `IsItemVisible(bounds)` | Checks if a given rectangle is within the current clipping area. |

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
| `Dummy(size)` | An invisible element that occupies space. Equivalent to `PlaceItem`. |
| `GetCursorPos()` | Returns the current layout cursor position. |
| `SetCursorPos(pos)` | Manually overrides the layout cursor position. |

```cpp
// Widget:
static void SimpleWidget(const Hax::Vector2& size, const Hax::Gui::Color& col)
{
    const Hax::Vector2 cursorPos = Hax::Gui::GetCursorPos();
    Hax::Gui::PlaceItem(size);
    Hax::Gui::DrawRect(cursorPos, cursorPos + size, {.FillColor = col});
}

// Render:
{
    Hax::Gui::BeginFrame();

    const float spacing = 3_px;
    const Vector2 size = { 50_px, 100_px };
    Hax::Gui::BeginHorizontal(spacing);
    {
        Hax::Gui::BeginVertical(spacing);
        {
            SimpleWidget(size, Hax::Gui::Color::Red);
            SimpleWidget(size, Hax::Gui::Color::Red);
            SimpleWidget(size, Hax::Gui::Color::Red);
        }
        Hax::Gui::EndVertical();

        Hax::Gui::BeginVertical(spacing);
        {
            SimpleWidget({size.Y, size.X}, Hax::Gui::Color::Green);
            Hax::Gui::BeginHorizontal(2_px);
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

### Container

Advanced layout elements with their own coordinate systems and scrolling.

| Function | Description |
| :--- | :--- |
| `BeginContainer(id, params)` | Starts a new container (e.g., a scrollable area or a window). |
| `EndContainer()` | Finalizes the current container. |
| `GetContainerBounds()` | Returns the absolute boundaries of the current container. |
| `GetContentRegionAvail()` | Returns the remaining available space within the current container. |
| `ScrollYTo(posY)` | Programmatically sets the vertical scroll position. |

### Interaction & Hit-testing

Functions for handling user input relative to UI elements.

| Function | Description |
| :--- | :--- |
| `Interact(id, bounds)` | Registers an interactive area and performs a hit-test for the current frame. |
| `IsItemHovered(id)` | Returns `true` if the mouse is over the element. |
| `IsItemFocused(id)` | Returns `true` if the element is currently focused (e.g., for keyboard input). |
| `IsItemActive(id)` | Returns `true` while the element is being interacted with (e.g., mouse button held). |
| `IsItemClicked(id)` | Returns `true` if the element was clicked (pressed and released) in this frame. |
| `IsItemPressed(id)` | Returns `true` if the element is currently being pressed down. |
| `IsItemPressedRepeat(...)` | Returns `true` for repeated click events while holding the element. |

```cpp
See widgets
```

### Scaling

Support for DPI-aware layouts and value scaling.

| Function | Description |
| :--- | :--- |
| `Scale(float val)` | Scales a scalar value based on the current global scale factor. |
| `Scale(Vector2 val)` | Scales a 2D vector. |
| `operator "" _px(...)` | Literal for convenient inline scaling (e.g., `100_px`). |

### Resources

| Function | Description |
| :--- | :--- |
| `GetResourceData(...)` | Loads raw binary data from a module resource (e.g., Win32 resources). |

### Text & Fonts

Comprehensive API for font management, text measurement, and editing.

| Function | Description |
| :--- | :--- |
| `LoadMsdfFonts(items)` | Batch loads MSDF fonts from memory. Should be called once. |
| `CalcTextSize(...)` | Calculates the width and height of a string for a given font and size. |
| `GetFontLineHeight(...)` | Returns the vertical distance between lines of text. |
| `GetFontAscent(...)` | Returns the distance from the baseline to the highest point of the font. |
| `GetFontCapHeight(...)` | Returns the height of capital letters. |
| `GetGlyphSize(...)` | Returns the dimensions of a specific character. |
| `GetFontMetrics(...)` | Returns a structure containing all key font metrics. |
| `LoadFont(data)` | Loads a standard font from a memory buffer. |
| `LoadSystemFont(...)` | Loads a font installed in the Windows system by its family name. |
| `UnloadFont(hFont)` | Releases font resources and removes it from memory. |
| `BakeGlyphsInRange(...)` | Pre-renders a range of characters into the font atlas for performance. |
| `GetFontAtlas()` | Returns the texture handle containing all baked font glyphs. |
| `StringEdit(...)` | A full-featured text input widget for editing strings. |

### Images

| Function | Description |
| :--- | :--- |
| `LoadImageFromMemory(data)` | Decodes an image from memory and creates a GPU texture. |

### Screen & Viewport

| Function | Description |
| :--- | :--- |
| `GetViewportSize()` | Returns the dimensions of the current drawing area. |
| `GetViewportCenter()` | Returns the coordinates of the viewport center. |
| `GetViewportBounds()` | Returns a Rect representing the full viewport area. |

### Demo

| Function | Description |
| :--- | :--- |
| `ShowDemoWindow()` | Opens a built-in window demonstrating all library features and widgets. |

### States

State management for persisting user data between frames.

| Function | Description |
| :--- | :--- |
| `GetState<T>(id)` | Retrieves or creates a persistent state object of type T associated with the given ID. |

### Lines (Utility)

| Function | Description |
| :--- | :--- |
| `VerticalLine(...)` | Draws a vertical separator line with specified thickness and color. |
| `HorizontalLine(...)` | Draws a horizontal separator line with specified thickness and color. |