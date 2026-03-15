#pragma once

#include "haxgui.h"
#include "hax.h"

#include <atomic>

namespace Hax::Gui
{
    constexpr size_t kInvalidId = 0;

    struct WndMsg
    {
        void*       Hwnd;
        uint32      Msg;
        uintptr     WParam;
        uint64      LParam;
    };

    struct WndMsgBuffer
    {
        static constexpr size_t kCapacity = 64;
        static constexpr size_t kMask = kCapacity - 1;

        void Write(const WndMsg& msg)
        {
            size_t index = m_Write.load(std::memory_order_relaxed);
            m_Buffer[index] = msg;

            size_t next = (index + 1) & kMask;
            m_Write.store(next, std::memory_order_release);
        }

        bool Read(WndMsg& out)
        {
            size_t read = m_Read.load(std::memory_order_relaxed);
            size_t write = m_Write.load(std::memory_order_acquire);

            if (read == write)
                return false;

            out = m_Buffer[read];
            m_Read.store((read + 1) & kMask, std::memory_order_release);
            return true;
        }

        Array<WndMsg, kCapacity>            m_Buffer;
        alignas(64) std::atomic<size_t>     m_Read = 0;
        alignas(64) std::atomic<size_t>     m_Write = 0;
    };
}

namespace Hax::Gui
{
    // Predefine

    struct Font;

    // enums

    enum class RenderItemType 
    { 
        Ellipse, 
        Rect, 
        Triangle,
        Line,
        Image,
        Glyph
    };

    enum RenderBatchAction : uint32
    {
        RenderBatchAction_None = 0,
        RenderBatchAction_SetClipRect = 1 << 0,
        RenderBatchAction_SetTexture = 1 << 1,
        RenderBatchAction_SetFont = 1 << 2,
    };

    enum class LayoutType 
    { 
        Root, 
        Container, 
        Horizontal, 
        Vertical 
    };

    enum ContainerFlag
    {
        ContainerFlag_None = 0,
        ContainerFlag_Clip = 1 << 0,
        ContainerFlag_ScrollX = 1 << 1,
        ContainerFlag_ScrollY = 1 << 2,
        ContainerFlag_ScrollVisible = 1 << 3,
        ContainerFlag_FitX = 1 << 4,
        ContainerFlag_FitY = 1 << 5,
    };

    // classes

    struct Timer
    {
        int64   TicksPerSecond;
        int64   TotalTicksLastFrame;
        double  Time;
        float   DeltaTime;

        float   FramerateSecPerFrameAccum;
        float   FramerateSecPerFrame[60];
        int     FramerateSecPerFrameIdx;
        int     FramerateSecPerFrameCount = 1;
    };

    struct Mouse
    {
        bool        IsLmbJustPressed() const    { return LmbDown && !PrevLmbDown; }
        bool        IsLmbJustReleased() const   { return PrevLmbDown && !LmbDown; }

        Vector2     Pos;
        Vector2     DeltaPos;
        Vector2     DeltaWheel;

        bool        LmbDown;
        bool        PrevLmbDown;

        MouseIcon   PrevIcon = MouseIcon_Default;
        MouseIcon   Icon = MouseIcon_Default;
    };

    struct Viewport
    {
        Vector2     Size;
        Handle      Hwnd;
        float       ScaleFactor;
    };

    struct Layout
    {
        void        PlaceItem(const Vector2& size);
        void        Space(float pixels);
        void        Reset() { CursorPos = Bounds.Min; }

        bool        IsRoot() const          { return Type == LayoutType::Root; }
        bool        IsHorizontal() const    { return Type == LayoutType::Horizontal; }
        bool        IsVertical() const      { return Type == LayoutType::Vertical || Type == LayoutType::Container; }

        Vector2     CursorPos;
        Rect        Bounds;
        float       Spacing;
        LayoutType  Type;
    };

    struct Container
    {
        bool        ScrollX() const         { return Flags & ContainerFlag_ScrollX; }
        bool        ScrollY() const         { return Flags & ContainerFlag_ScrollY; }
        bool        ScrollVisible() const   { return Flags & ContainerFlag_ScrollVisible; }

        bool        FitsX() const           { return Flags & ContainerFlag_FitX; }
        bool        FitsY() const           { return Flags & ContainerFlag_FitY; }

        bool        Clipping() const        { return Flags & ContainerFlag_Clip; }
        bool        RequiresState() const   { return Flags > ContainerFlag_Clip; }

        uint64      Id;
        Rect        Bounds;
        uint32      Flags;
    };

    __declspec(align(16)) struct RenderItem
    {
        RenderItem() { memset(this, 0, sizeof(RenderItem)); }

        bool operator==(const RenderItem& other) const { return memcmp(this, &other, sizeof(RenderItem)) == 0; }

        union
        {
            struct
            {
                float Params14[4];
                float Params58[4];
                float Param9;
            };

            struct { Vector2 Center; Vector2 R; float BorderTh; } Ellipse;
            struct { Vector2 Min; Vector2 Max; Vector4 R; float BorderTh; } Rect;
            struct { Vector2 A; Vector2 B; float Th; } Line;
            struct { Vector2 A; Vector2 B; Vector2 C; float BorderTh; } Triangle;
            struct { Vector2 A; Vector2 B; Vector2 UVmin; Vector2 UVmax; float R; } Image;
        };

        RenderItemType  Type;
        Color           Color1, Color2, Color3, Color4;
        float           Sin, Cos;
    };

    struct RenderItemRes
{ 
        const Font* Font;
        TexHandle Image;
    };

    struct RenderBatch
    {
        Optional<Rect>  ClipRect;
        const Font*     Font;
        TexHandle       Texture;
        uint32          InstancesNum;
        uint32          ActionMask = RenderBatchAction_None;
    };

    struct Rotation
    {
        float Angle;
        float Sin;
        float Cos;
    };

    struct Layer
    {
        void                AddRect(const Vector2& a, const Vector2& b, const RectParams& params = {});
        void                AddEllipse(const Vector2& c, const Vector2& r, const EllipseParams& params = {});
        void                AddTriangle(const Vector2& a, const Vector2& b, const Vector2& c, const TriangleParams& params = {});
        void                AddLine(const Vector2& a, const Vector2& b, const LineParams& params = {});
        void                AddImage(Texture2D image, const Vector2& a, const Vector2& b, const ImageParams& params = {});
        Vector2             AddGlyph(const Font& font, char16 sym, const Vector2& pos, float size, const GlyphParams& params = {});
        void                AddText(const Font& font, StringView text, const Vector2& pos, float size, const TextParams& params = {});
        void                AddItem(const RenderItem& item, const RenderItemRes& requirements = {});

        void                BeginRotation(float angle);
        void                EndRotation();

        void                BeginSkip();
        void                EndSkip();
        bool                IsSkipping() const;

        void                PushClipRect(const Rect& clipRect);
        void                PopClipRect();

        void                BeginLayout(float spacing, LayoutType type);
        void                EndLayout(LayoutType type);

        void                BeginContainer(uint64 id, const ContainerParams& params);
        Container           EndContainer();

        bool                IsItemVisible(const Rect& bounds) const { return SkipCounter == 0 && !CurrentClipRect.IsInverted() && CurrentClipRect.Intersects(bounds); }

        Context*            Context;
        uint64              Id;
        int                 ZOrder;
        int                 SkipCounter;

        Vector<RenderItem>  RenderItems;
        Vector<RenderBatch> RenderBatches;

        Vector<Rotation>    RotationStack;
        Rotation            CurrentRotation;

        Vector<Rect>        ClipRectStack;
        Rect                CurrentClipRect;

        Vector<Layout>      LayoutStack;
        Layout              CurrentLayout;

        Vector<Container>   ContainerStack;
        Container           CurrentContainer;
        uint64              CurrentScrollId;
    };

    struct IBackend
    {
        virtual ~IBackend() = default;

        virtual void Render() = 0;

        virtual Texture2D CreateTexture(const uint8* pixels, int width, int height) = 0;
        virtual void DestroyTexture(TexHandle handle) = 0;
    };

    __declspec(align(16)) struct FontConstData
    {
        float PxRange;
        float TexWidth;
        float TexHeight;
    };

    struct Interaction
    {
        size_t FocusedItemId;
        size_t ClickedItemId;
        struct
        {
            struct { size_t Id = kInvalidId; int ZOrder = INT_MIN; } HoveredItem, HoveredContainer;
            size_t ActiveItemId;
        } ThisFrame, NextFrame;
    };

    struct KeyData
    {
        bool        Down = false;
        float       DownDuration = -1.f;
        float       DownDurationPrev = -1.f;
    };

    struct Context
    {
        Context() = default;
        ~Context() 
        {
            GeneralAlloc.Delete(Backend);

            for (auto& p : StatePools)
                g_StateAlloc.Delete(p.value);
            for (Layer* layer : Layers)
                GeneralAlloc.Delete(layer);
            for (Font* font : Fonts)
                FontAlloc.Delete(font);

            Layers.ClearFree();
            LayerStack.ClearFree();
            Fonts.ClearFree();
            StatePools.ClearFree();

            printf("Memory leaks:\nFonts - %zu\nGeneral - %zu\nState pools - %zu\n", 
                FontAlloc.TotalUsage(), GeneralAlloc.TotalUsage(), g_StateAlloc.TotalUsage());
        }

        Allocator           GeneralAlloc{"General"};
        Allocator           FontAlloc{"Font"};

        Timer               Timer;
        Mouse               Mouse;
        Viewport            Viewport;
        IBackend*           Backend;
        Interaction         Interaction;

        Vector<Layer*>      Layers{GeneralAlloc};
        Vector<Layer*>      LayerStack{GeneralAlloc};
        Layer*              CurrentLayer;
        Layer*              DefaultLayer;

        WndMsgBuffer        WndMsgQueue;

        Vector<Font*>       Fonts{GeneralAlloc};

        Map<size_t, IStatePool*>  StatePools{GeneralAlloc};

        KeyData             KeysData[256]{};
    };

    extern Context* g_Context;
}