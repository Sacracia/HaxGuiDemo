#pragma once

#include <cstdint>
#include <algorithm>
#include <type_traits>

#include <span>
#include <utility>

#include "hax.h"

#ifndef HAX_API
#define HAX_API
#endif

#define HAX_LINE static_cast<uint64>(__LINE__)

namespace Hax
{
                                        using Handle = uintptr_t;
                                        using TexHandle = Handle;
    template <typename T>               using ArrayView = std::span<T>;

    constexpr float SCALE_BASE_HEIGHT = 1080.f;

    constexpr float RadToDeg(float angleRad) { return angleRad * 180.f / kPi; }
    constexpr float DegToRad(float angleDeg) { return angleDeg * kPi / 180.f; }

    struct Vector2
    {
        constexpr               Vector2() = default;
        constexpr               Vector2(float x, float y) : X(x), Y(y) {}
        constexpr               Vector2(const Vector2& o) = default;

        constexpr Vector2&      operator=(const Vector2& o) = default;
        constexpr bool          operator==(const Vector2& o) const  { return X == o.X && Y == o.Y; }
        constexpr explicit      operator bool() const               { return X != 0 && Y != 0; }

        constexpr Vector2       operator*(const float v) const      { return Vector2(X * v, Y * v); }
        constexpr Vector2       operator*(const Vector2& o) const   { return Vector2(X * o.X, Y * o.Y); }
        constexpr Vector2       operator/(const float v) const      { return Vector2(X / v, Y / v); }
        constexpr Vector2       operator/(const Vector2& o) const   { return Vector2(X / o.X, Y / o.Y); }
        constexpr Vector2       operator+(const Vector2& o) const   { return Vector2(X + o.X, Y + o.Y); }
        constexpr Vector2       operator-(const Vector2& o) const   { return Vector2(X - o.X, Y - o.Y); }

        constexpr Vector2&      operator*=(const float rhs)         { X *= rhs; Y *= rhs; return *this; }
        constexpr Vector2&      operator/=(const float rhs)         { X /= rhs; Y /= rhs; return *this; }
        constexpr Vector2&      operator+=(const Vector2& rhs)      { X += rhs.X; Y += rhs.Y; return *this; }
        constexpr Vector2&      operator-=(const Vector2& rhs)      { X -= rhs.X; Y -= rhs.Y; return *this; }
        constexpr Vector2&      operator*=(const Vector2& rhs)      { X *= rhs.X; Y *= rhs.Y; return *this; }
        constexpr Vector2&      operator/=(const Vector2& rhs)      { X /= rhs.X; Y /= rhs.Y; return *this; }
        constexpr bool          operator==(const Vector2& rhs)      { return X == rhs.X && Y == rhs.Y; }
        constexpr bool          operator!=(const Vector2& rhs)      { return X != rhs.X || Y != rhs.Y; }

        float                   Length() const                      { return Sqrt(X * X + Y * Y); }

        float                   X = 0.f, Y = 0.f;
    };
    constexpr Vector2 Min(const Vector2& v1, const Vector2& v2) { return Vector2(Min(v1.X, v2.X), Min(v1.Y, v2.Y)); }
    constexpr Vector2 Max(const Vector2& v1, const Vector2& v2) { return Vector2(Max(v1.X, v2.X), Max(v1.Y, v2.Y)); }

    struct Vector3
    {
        constexpr Vector3() = default;
        constexpr Vector3(float v) : X(v), Y(v), Z(v) {}
        constexpr Vector3(float x, float y, float z) : X(x), Y(y), Z(z) {}
        constexpr Vector3(const Vector3& o) = default;

        constexpr Vector3 operator+(float rhs) const { return Vector3(X + rhs, Y + rhs, Z + rhs); }
        constexpr Vector3 operator-(float rhs) const { return Vector3(X - rhs, Y - rhs, Z - rhs); }

        float X = 0.f, Y = 0.f, Z = 0.f;
    };

    struct Vector4
    {
        constexpr Vector4() = default;
        constexpr Vector4(float v) : X(v), Y(v), Z(v), W(v) {}
        constexpr Vector4(float x, float y, float z, float w) : X(x), Y(y), Z(z), W(w) {}
        constexpr Vector4(const Vector4& o) = default;

        constexpr Vector4 operator+(float rhs) const { return Vector4(X + rhs, Y + rhs, Z + rhs, W + rhs); }
        constexpr Vector4 operator-(float rhs) const { return Vector4(X - rhs, Y - rhs, Z - rhs, W - rhs); }

        float X = 0.f, Y = 0.f, Z = 0.f, W = 0.f;
    };

    struct Rect
    {
        static Rect FromFloats(float xMin, float yMin, float xMax, float yMax) { return {{xMin, yMin}, {xMax, yMax}}; }
        static Rect FromMinMax(const Vector2& min, const Vector2& max) { return {min, max}; }
        static Rect FromPosSize(const Vector2& pos, const Vector2& size) { return {pos, pos + size}; }

        bool operator==(const Rect& o) const { return Min == o.Min && Max == o.Max; }

        Vector2 GetCenter() const { return Vector2((Min.X + Max.X) * 0.5f, (Min.Y + Max.Y) * 0.5f); }
        Vector2 GetSize() const { return Max - Min; }
        Vector2 GetTL() const { return Min; }
        Vector2 GetTR() const { return Vector2(Max.X, Min.Y); }
        Vector2 GetBL() const { return Vector2(Min.X, Max.Y); }
        Vector2 GetBR() const { return Max; }

        bool Contains(const Vector2& p) const { return p.X >= Min.X && p.Y >= Min.Y && p.X < Max.X && p.Y < Max.Y; }
        bool Contains(const Rect& r) const { return Contains(r.Min) && Contains(r.Max); }
        bool Intersects(const Rect& r) const { return r.Min.X < Max.X && r.Min.Y < Max.Y && r.Max.X > Min.X && r.Max.Y > Min.Y; }
        void ClipWith(const Rect& o) { Min = Hax::Max(o.Min, Min); Max = Hax::Min(o.Max, Max); }
        Rect Intersect(const Rect& o) const { return Rect(Hax::Max(o.Min, Min), Hax::Min(o.Max, Max)); }
        bool IsInverted() const { return Min.X > Max.X || Min.Y > Max.Y; }

        void Add(const Vector2& p) { if (Min.X > p.X) Min.X = p.X; if (Min.Y > p.Y) Min.Y = p.Y; if (Max.X < p.X) Max.X = p.X; if (Max.Y < p.Y) Max.Y = p.Y; }
        void Add(const Rect& r) { if (Min.X > r.Min.X) Min.X = r.Min.X; if (Min.Y > r.Min.Y) Min.Y = r.Min.Y; if (Max.X < r.Max.X) Max.X = r.Max.X; if (Max.Y < r.Max.Y) Max.Y = r.Max.Y; }

        Vector2 Min = {};
        Vector2 Max = {};
    };

    struct Color;

    struct LinearColor
    {
        constexpr LinearColor() = default;
        constexpr LinearColor(float r, float g, float b, float a = 1.f) : R(r), G(g), B(b), A(a) {}
        constexpr LinearColor(const Color& color);
        constexpr LinearColor(uint32 hex);

        constexpr LinearColor& operator*=(float rhs) { R *= rhs; G *= rhs; B *= rhs; return *this; }

        constexpr Color ToColor() const;

        constexpr Vector4 ToHSV() const;
        static constexpr LinearColor FromHSV(float h, float s, float v, float a = 1.f);

        LinearColor Brighten(float factor) const;
        LinearColor Darken(float factor) const;
        
        static const LinearColor White;
        static const LinearColor Gray;
        static const LinearColor Black;
        static const LinearColor Transparent;
        static const LinearColor Red;
        static const LinearColor Green;
        static const LinearColor Blue;
        static const LinearColor Yellow;

        union
        {
            struct { float R, G, B, A; };
            float RGBA[4]{1.f, 1.f, 1.f, 1.f};
        };
    };

    constexpr Vector4 LinearColor::ToHSV() const
    {
        float r = R;
        float g = G;
        float b = B;
        float a = A;

        float K = 0.f;
        if (g < b)
        {
            Swap(g, b);
            K = -1.f;
        }
        if (r < g)
        {
            Swap(r, g);
            K = -2.f / 6.f - K;
        }

        const float chroma = r - (g < b ? g : b);
        float h = Abs(K + (g - b) / (6.f * chroma + 1e-20f));
        float s = chroma / (r + 1e-20f);
        float v = r;

        return {h, s, v, a};
    }

    constexpr LinearColor LinearColor::FromHSV(float h, float s, float v, float a)
    {
        float r, g, b;

        if (s == 0.0f)
        {
            r = g = b = v;
            return LinearColor(r, g, b);
        }

        h = Mod(h, 1.0f) / (60.0f / 360.0f);
        int   i = (int)h;
        float f = h - (float)i;
        float p = v * (1.0f - s);
        float q = v * (1.0f - s * f);
        float t = v * (1.0f - s * (1.0f - f));

        switch (i)
        {
            case 0: r = v; g = t; b = p; break;
            case 1: r = q; g = v; b = p; break;
            case 2: r = p; g = v; b = t; break;
            case 3: r = p; g = q; b = v; break;
            case 4: r = t; g = p; b = v; break;
            case 5: 
            default: r = v; g = p; b = q; break;
        }

        return LinearColor(r, g, b);
    }

    struct Color
    {
        constexpr Color() = default;
        constexpr Color(const Color& c) = default;
        constexpr Color(uint8 r, uint8 g, uint8 b, uint8 a = 255) : R(r), G(g), B(b), A(a) {}

        constexpr Color(uint32 col) : A(col & 0xFF),
                                      B((col >> 8) & 0xFF),
                                      G((col >> 16) & 0xFF),
                                      R((col >> 24) & 0xFF) {}

        constexpr bool IsTransparent() const { return A == 0; }

        static const Color White;
        static const Color Black;
        static const Color Transparent;
        static const Color Red;
        static const Color Green;
        static const Color Blue;
        static const Color Yellow;
        static const Color Cyan;
        static const Color Magenta;
        static const Color Orange;
        static const Color Purple;
        static const Color Turquoise;
        static const Color Silver;
        static const Color Emerald;

        union
        {
            struct { uint8 R, G, B, A; };
            uint32 Bits = 0xFFFFFFFF;
        };
    };

    inline constexpr Color Color::White(255,255,255);
    inline constexpr Color Color::Black(0,0,0);
    inline constexpr Color Color::Transparent(0, 0, 0, 0);
    inline constexpr Color Color::Red(255,0,0);
    inline constexpr Color Color::Green(0,255,0);
    inline constexpr Color Color::Blue(0,0,255);
    inline constexpr Color Color::Yellow(255,255,0);
    inline constexpr Color Color::Cyan(0,255,255);
    inline constexpr Color Color::Magenta(255,0,255);
    inline constexpr Color Color::Orange(243, 156, 18);
    inline constexpr Color Color::Purple(169, 7, 228);
    inline constexpr Color Color::Turquoise(26, 188, 156);
    inline constexpr Color Color::Silver(189, 195, 199);
    inline constexpr Color Color::Emerald(46, 204, 113);

    constexpr float k1To255 = 1.f / 255.f;
    constexpr LinearColor::LinearColor(const Color& color) : R(static_cast<float>(color.R) * k1To255), 
                                                             G(static_cast<float>(color.G) * k1To255), 
                                                             B(static_cast<float>(color.B) * k1To255), 
                                                             A(static_cast<float>(color.A) * k1To255) {}

    constexpr LinearColor::LinearColor(uint32 hex) : LinearColor(Color(hex)) {}

    constexpr Color LinearColor::ToColor() const
    {
        return Color
        (
            (uint8)(0.5f + Clamp(R, 0.f, 1.f) * 255.f),
            (uint8)(0.5f + Clamp(G, 0.f, 1.f) * 255.f),
            (uint8)(0.5f + Clamp(B, 0.f, 1.f) * 255.f),
            (uint8)(0.5f + Clamp(A, 0.f, 1.f) * 255.f)
        );
    }

    constexpr LinearColor Lerp(const LinearColor& a, const LinearColor& b, float t)
    {
        return LinearColor
        (
            Lerp(a.R, b.R, t),
            Lerp(a.G, b.G, t),
            Lerp(a.B, b.B, t),
            Lerp(a.A, b.A, t)
        );
    }

    constexpr Color Lerp(const Color& a, const Color& b, float t)
    {
        return Lerp(LinearColor(a), LinearColor(b), t).ToColor();
    }
}

//-----------------------------------------------------------------------------
// [SECTION] API
//-----------------------------------------------------------------------------

namespace Hax::Gui
{
    enum MouseIcon
    {
        MouseIcon_Default = -2,
        MouseIcon_None = -1,
        MouseIcon_Arrow = 0,
        MouseIcon_TextInput,
        MouseIcon_ResizeAll,
        MouseIcon_ResizeNS,
        MouseIcon_ResizeEW,
        MouseIcon_ResizeNESW,
        MouseIcon_ResizeNWSE,
        MouseIcon_Hand,
        MouseIcon_Wait,
        MouseIcon_Progress,
        MouseIcon_NotAllowed,
        MouseIcon_COUNT
    };

    struct Context;

    struct Texture2D
    {
        TexHandle Handle;
        int Width;
        int Height;
    };

    struct Glyph
    {
        uint32 Codepoint;
        uint32 Image;
        struct { float L, B, R, T;  } PlaneBounds, ImageBounds;
        Vector2 Advance;
    };

    struct Font
    {
        Map<uint32, Glyph*> Glyphs;//! Test unordered_map vs Map
        Texture2D Atlas;

        float Size;
        float DistanceRange;
        float EmSize;
        float Ascender, Descender;
        float LineHeight;
    };

    struct LinearAnim
    {
        void Elapse(float time, float speed) { if (speed == 0.f) return; Progress = std::clamp(Progress + time / speed, 0.f, 1.f); }

        float Progress;
    };

    extern float g_ScaleFactor;

    inline float operator "" _dp(long double val) { return (float)val * g_ScaleFactor; }
    inline float operator "" _dp(unsigned long long val) { return (float)val * g_ScaleFactor; }

    void        BeginFrame();
    void        EndFrame();

    // Context

    Context*    CreateContext(Handle hwnd);
    void        DestroyContext(Context* context = nullptr);
    Context*    GetCurrentContext();
    void        SetCurrentContext(Context* context);

    // Timer

    double      GetTime();
    float       GetDeltaTime();
    float       GetFramerate();

    // Mouse

    Vector2 GetMouseDeltaPos();
    void SetMouseIcon(MouseIcon icon);

    // WndHandler

    long HandleWndMsg(void* hwnd, uint32 msg, uintptr wParam, uint64 lParam);

    // Layer

    void CreateLayer(StringView name, int zOrder);
    void SwitchLayer(StringView name);
    void RestoreLayer();

    // Render

    struct  RectParams      { Color FillColor; Vector4 Rounding; float BorderTh; Color BorderColor = 0x0; };
    struct  EllipseParams   { Color FillColor; float BorderTh; Color BorderColor; };
    struct  TriangleParams  { Color FillColor; float BorderTh; Color BorderColor; };
    struct  LineParams      { float Th = 1_dp; Color FillColor = Color::Black; };
    struct  ImageParams     { Color BgColor = Color::White; float R = 0.f; Vector2 UVmin = {}; Vector2 UVmax = { 1.f, 1.f }; };
    struct  GlyphParams     { Color Color = Color::Black; float Spacing = 1.f; };
    using   CircleParams    = EllipseParams;
    using   TextParams      = GlyphParams;

    void DrawRect(const Vector2& a, const Vector2& b, const RectParams& params = {});
    void DrawEllipse(const Vector2& c, const Vector2& r, const EllipseParams& params);
    void DrawCircle(const Vector2& c, float r, const CircleParams& params);
    void DrawTriangle(const Vector2& a, const Vector2& b, const Vector2& c, const TriangleParams& params = {});
    void DrawLine(const Vector2& a, const Vector2& b, const LineParams& params = {});
    void DrawImage(Texture2D image, const Vector2& a, const Vector2& b, const ImageParams& params = {});
    void DrawGlyph(const Font& font, char16 sym, const Vector2& pos, float size, const GlyphParams& params = {});
    void DrawTexto(const Font& font, StringView text, const Vector2& pos, float size, const TextParams& params = {});

    // Skip

    void BeginSkip();
    void EndSkip(uint32 count = 1);
    bool IsSkipping();

    //

    void BeginRotation(float angleRad);
    void EndRotation();

    // 

    void PushClipRect(const Rect& clipRect);
    void PopClipRect();

    //

    void BeginVertical(float spacing = 0.f);
    void EndVertical();
    void BeginHorizontal(float spacing = 0.f);
    void EndHorizontal();

    // Layout

    void PlaceItem(const Vector2& size);
    void Space(float pixels);
    inline void Dummy(const Vector2& size) { PlaceItem(size); }

    Vector2 GetCursorPos();
    void SetCursorPos(const Vector2& pos);
    void VerticalLine(float th, const Color& color, float size = 0.f);
    void HorizontalLine(float th, const Color& color, float size = 0.f);
    Rect GetLayoutBounds();
    float GetLayoutSpacing();
    void ResetCursor();

    bool IsItemVisible(const Rect& bounds);

    // Container

    struct ScrollStyle
    {
        float TrackWidth = 20.f;
        float ThumbPadding = 5.f;

        Color TrackCol = 0x404040FF;
        Color ThumbCol = 0xECECECFF;
        Color ThumbHovCol = 0x696969FF;
        Color ThumbActiveCol = 0x828282FF;
    };

    struct ContainerParams
    {
        float W = 0.f;
        float H = 0.f;
        
        bool  Clip : 1 = true;
        bool  ScrollX : 1 = false;
        bool  ScrollY : 1 = false;
        bool  ScrollVisible : 1 = false;
        bool  FitX : 1 = false;
        bool  FitY : 1 = false;

        ScrollStyle Style = {};
    };

    void BeginContainer(uint64 id = 0, const ContainerParams& params = {});
    void EndContainer();
    Rect GetContainerBounds();

    Vector2 GetContentRegionAvail();
    void ScrollYTo(float posY);

    // Interaction

    void Interact(uint64 id, const Rect& bounds);
    bool IsItemHovered(uint64 id);
    bool IsItemFocused(uint64 id);
    bool IsItemActive(uint64 id);
    bool IsItemClicked(uint64 id);
    bool IsItemPressed(uint64 id);

    // Resources

    ArrayView<const uint8> GetResourceData(Handle resModule, int resId, const char16* resType);

    // Font

    Font& LoadFontFromMemory(ArrayView<const uint8> data);
    Vector2 CalcTextSize(const Font& font, StringView text, float size, float spacing = 1.0);
    inline float GetFontLineHeight(const Font& font, float fontH) { return font.LineHeight * fontH; }

    // Textures

    Texture2D LoadImageFromMemory(ArrayView<const uint8> data);

    // Demo

    void ShowDemoWindow();

    //

    Vector2 GetViewportSize();

    // Keyboard

    bool IsKeyDown(uint8 vk);
    bool IsKeyJustDown(uint8 vk);
    bool IsKeyUp(uint8 vk);
    bool IsKeyJustUp(uint8 vk);
    StringView GetKeyName(uint8 vk);

    struct IStatePool
    {
        virtual ~IStatePool() = default;
    };

    IStatePool** GetStatePool(size_t typeId);
    void AddStatePool(size_t typeId, IStatePool* pool);

    template <typename T>
    struct StatePool : IStatePool
    {
        static_assert(std::is_trivially_copyable<T>::value);

        ~StatePool() override
        {
            m_Map.ClearFree();
        }

        Map<size_t, T> m_Map{g_StateAlloc};
    };

    template <typename T>
    inline T& GetState(size_t id) 
    {
        constexpr size_t typeId = GetTypeId<T>();

        StatePool<T>* pool = nullptr;

        if (IStatePool** ppPool = GetStatePool(typeId))
            pool = (StatePool<T>*)*ppPool;
        else
        {
            pool = g_StateAlloc.New<StatePool<T>>();
            AddStatePool(typeId, pool);
        }

        return pool->m_Map.FindOrAdd(id);
    }

    Allocator* IterAllocators(void*& iter);
}