
#include "haxgui_internal.h"

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <windowsx.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

size_t g_TotalAllocated, g_MaxAllocated;

void* operator new(size_t size)
{
    g_TotalAllocated += size;
    g_MaxAllocated = std::max(g_MaxAllocated, g_TotalAllocated);

    void* ptr = std::malloc(size);
    if (!ptr) 
        throw std::bad_alloc();
    return ptr;
}

void operator delete(void* ptr, size_t size) noexcept 
{
    g_TotalAllocated -= size;
    std::free(ptr);
}

void operator delete(void* ptr) noexcept 
{
    printf("Dealloc: ?. Left %zu\n", g_TotalAllocated);
    std::free(ptr);
}

void* operator new[](size_t size)
{
    g_TotalAllocated += size;
    g_MaxAllocated = std::max(g_MaxAllocated, g_TotalAllocated);

    void* ptr = std::malloc(size);
    if (!ptr) 
        throw std::bad_alloc();
    return ptr;
}

void operator delete[](void* ptr) noexcept
{
    printf("Dealloc: ?. Left %zu\n", g_TotalAllocated);
    std::free(ptr);
}

void operator delete[](void* ptr, size_t size) noexcept
{
    g_TotalAllocated -= size;
    std::free(ptr);
}

namespace Hax
{
}

namespace Hax::Gui
{
    float g_DpiScale;

    struct ContainerState
    {
        Rect Bounds;
        struct
        {
            Vector2     Offset = { -1.f, -1.f };
            float       TargetOffsetY = -1.f;
            float       LastTimeHeldX, LastTimeHeldY;
            ScrollStyle Style;
        } Scroll;
    };

    struct InterRes
    {
        bool Hovered, Active, Focused, Pressed, Clicked;
    };

    struct FontDecoder
    {
        FontDecoder(ArrayView<const uint8> data) : m_Binary(data.data()), m_BinarySize(data.size()), m_ReadCursor(data.data()) {}

        Font& Decode(Context& ctx);

        void ReadBytes(uint8* target, uint64 nBytes);
        void ReadString(String* str, uint64 nBytes);
        void AlignCursor();

    private:
        const uint8* m_Binary;
        uint64 m_BinarySize;

        const uint8* m_ReadCursor;
        uint32 m_Checksum = ~0u;
    };

    static uint32 Crc32Update(uint32 crc, uint8 x)
    {
        static const uint32 s_Crc32Table[256] =
        {
            0x00000000u, 0x77073096u, 0xee0e612cu, 0x990951bau, 0x076dc419u, 0x706af48fu, 0xe963a535u, 0x9e6495a3u,
            0x0edb8832u, 0x79dcb8a4u, 0xe0d5e91eu, 0x97d2d988u, 0x09b64c2bu, 0x7eb17cbdu, 0xe7b82d07u, 0x90bf1d91u,
            0x1db71064u, 0x6ab020f2u, 0xf3b97148u, 0x84be41deu, 0x1adad47du, 0x6ddde4ebu, 0xf4d4b551u, 0x83d385c7u,
            0x136c9856u, 0x646ba8c0u, 0xfd62f97au, 0x8a65c9ecu, 0x14015c4fu, 0x63066cd9u, 0xfa0f3d63u, 0x8d080df5u,
            0x3b6e20c8u, 0x4c69105eu, 0xd56041e4u, 0xa2677172u, 0x3c03e4d1u, 0x4b04d447u, 0xd20d85fdu, 0xa50ab56bu,
            0x35b5a8fau, 0x42b2986cu, 0xdbbbc9d6u, 0xacbcf940u, 0x32d86ce3u, 0x45df5c75u, 0xdcd60dcfu, 0xabd13d59u,
            0x26d930acu, 0x51de003au, 0xc8d75180u, 0xbfd06116u, 0x21b4f4b5u, 0x56b3c423u, 0xcfba9599u, 0xb8bda50fu,
            0x2802b89eu, 0x5f058808u, 0xc60cd9b2u, 0xb10be924u, 0x2f6f7c87u, 0x58684c11u, 0xc1611dabu, 0xb6662d3du,
            0x76dc4190u, 0x01db7106u, 0x98d220bcu, 0xefd5102au, 0x71b18589u, 0x06b6b51fu, 0x9fbfe4a5u, 0xe8b8d433u,
            0x7807c9a2u, 0x0f00f934u, 0x9609a88eu, 0xe10e9818u, 0x7f6a0dbbu, 0x086d3d2du, 0x91646c97u, 0xe6635c01u,
            0x6b6b51f4u, 0x1c6c6162u, 0x856530d8u, 0xf262004eu, 0x6c0695edu, 0x1b01a57bu, 0x8208f4c1u, 0xf50fc457u,
            0x65b0d9c6u, 0x12b7e950u, 0x8bbeb8eau, 0xfcb9887cu, 0x62dd1ddfu, 0x15da2d49u, 0x8cd37cf3u, 0xfbd44c65u,
            0x4db26158u, 0x3ab551ceu, 0xa3bc0074u, 0xd4bb30e2u, 0x4adfa541u, 0x3dd895d7u, 0xa4d1c46du, 0xd3d6f4fbu,
            0x4369e96au, 0x346ed9fcu, 0xad678846u, 0xda60b8d0u, 0x44042d73u, 0x33031de5u, 0xaa0a4c5fu, 0xdd0d7cc9u,
            0x5005713cu, 0x270241aau, 0xbe0b1010u, 0xc90c2086u, 0x5768b525u, 0x206f85b3u, 0xb966d409u, 0xce61e49fu,
            0x5edef90eu, 0x29d9c998u, 0xb0d09822u, 0xc7d7a8b4u, 0x59b33d17u, 0x2eb40d81u, 0xb7bd5c3bu, 0xc0ba6cadu,
            0xedb88320u, 0x9abfb3b6u, 0x03b6e20cu, 0x74b1d29au, 0xead54739u, 0x9dd277afu, 0x04db2615u, 0x73dc1683u,
            0xe3630b12u, 0x94643b84u, 0x0d6d6a3eu, 0x7a6a5aa8u, 0xe40ecf0bu, 0x9309ff9du, 0x0a00ae27u, 0x7d079eb1u,
            0xf00f9344u, 0x8708a3d2u, 0x1e01f268u, 0x6906c2feu, 0xf762575du, 0x806567cbu, 0x196c3671u, 0x6e6b06e7u,
            0xfed41b76u, 0x89d32be0u, 0x10da7a5au, 0x67dd4accu, 0xf9b9df6fu, 0x8ebeeff9u, 0x17b7be43u, 0x60b08ed5u,
            0xd6d6a3e8u, 0xa1d1937eu, 0x38d8c2c4u, 0x4fdff252u, 0xd1bb67f1u, 0xa6bc5767u, 0x3fb506ddu, 0x48b2364bu,
            0xd80d2bdau, 0xaf0a1b4cu, 0x36034af6u, 0x41047a60u, 0xdf60efc3u, 0xa867df55u, 0x316e8eefu, 0x4669be79u,
            0xcb61b38cu, 0xbc66831au, 0x256fd2a0u, 0x5268e236u, 0xcc0c7795u, 0xbb0b4703u, 0x220216b9u, 0x5505262fu,
            0xc5ba3bbeu, 0xb2bd0b28u, 0x2bb45a92u, 0x5cb36a04u, 0xc2d7ffa7u, 0xb5d0cf31u, 0x2cd99e8bu, 0x5bdeae1du,
            0x9b64c2b0u, 0xec63f226u, 0x756aa39cu, 0x026d930au, 0x9c0906a9u, 0xeb0e363fu, 0x72076785u, 0x05005713u,
            0x95bf4a82u, 0xe2b87a14u, 0x7bb12baeu, 0x0cb61b38u, 0x92d28e9bu, 0xe5d5be0du, 0x7cdcefb7u, 0x0bdbdf21u,
            0x86d3d2d4u, 0xf1d4e242u, 0x68ddb3f8u, 0x1fda836eu, 0x81be16cdu, 0xf6b9265bu, 0x6fb077e1u, 0x18b74777u,
            0x88085ae6u, 0xff0f6a70u, 0x66063bcau, 0x11010b5cu, 0x8f659effu, 0xf862ae69u, 0x616bffd3u, 0x166ccf45u,
            0xa00ae278u, 0xd70dd2eeu, 0x4e048354u, 0x3903b3c2u, 0xa7672661u, 0xd06016f7u, 0x4969474du, 0x3e6e77dbu,
            0xaed16a4au, 0xd9d65adcu, 0x40df0b66u, 0x37d83bf0u, 0xa9bcae53u, 0xdebb9ec5u, 0x47b2cf7fu, 0x30b5ffe9u,
            0xbdbdf21cu, 0xcabac28au, 0x53b39330u, 0x24b4a3a6u, 0xbad03605u, 0xcdd70693u, 0x54de5729u, 0x23d967bfu,
            0xb3667a2eu, 0xc4614ab8u, 0x5d681b02u, 0x2a6f2b94u, 0xb40bbe37u, 0xc30c8ea1u, 0x5a05df1bu, 0x2d02ef8du,
        };
        return s_Crc32Table[uint8(x ^ crc)] ^ crc >> 8;
    }

    void FontDecoder::ReadBytes(uint8* target, uint64 nBytes)
    {
        HAX_ASSERT(m_ReadCursor + nBytes <= m_Binary + m_BinarySize);

        if (target != nullptr)
            memcpy(target, m_ReadCursor, nBytes);

        for (uint32 i = 0; i < nBytes; ++i)
        {
            m_Checksum = Crc32Update(m_Checksum, *m_ReadCursor);
            ++m_ReadCursor;
        }
    }

    void FontDecoder::AlignCursor()
    {
        uint64 nBytesRead = m_ReadCursor - m_Binary;
        if (nBytesRead % 4 > 0)
            this->ReadBytes(nullptr, 4 - nBytesRead % 4);
    }

    void FontDecoder::ReadString(String* str, uint64 nChars)
    {
        if (nChars > 0)
        {
            if (str != nullptr)
            {
                str->Clear();
                str->Reserve(nChars + 1);

                this->ReadBytes((uint8*)str->Data(), nChars + 1);
                str->Last() = '\0';
            }
            else
                this->ReadBytes(nullptr, nChars + 1);

            this->AlignCursor();
        }
    }

    struct FontHeader
    {
        char Tag[16];
        uint32 MagicNo;
        uint32 Version;
        uint32 Flags;
        uint32 RealType;
        uint32 Reserved[4];

        uint32 MetadataFormat;
        uint32 MetadataLength;
        uint32 VariantCount;
        uint32 VariantsLength;
        uint32 ImageCount;
        uint32 ImagesLength;
        uint32 AppendixCount;
        uint32 AppendicesLength;
        uint32 Reserved2[8];
    };

    struct FontVariant
    {
        struct Metrics
        {
            float fontSize;
            float distanceRange;

            float emSize;
            float ascender, descender;
            float lineHeight;
            float underlineY, underlineThickness;

            float distanceRangeMiddle;
            float reserved[23];
        };

        uint32 flags;
        uint32 weight;
        uint32 codepointType;
        uint32 imageType;
        uint32 fallbackVariant;
        uint32 fallbackGlyph;
        uint32 reserved[6];
        Metrics metrics;
        uint32 nameLength;
        uint32 metadataLength;
        uint32 glyphCount;
        uint32 kernPairCount;
    };

    enum Encoding
    {
        IMAGE_UNKNOWN_ENCODING = 0,
        IMAGE_RAW_BINARY = 1,
        IMAGE_BMP = 4,
        IMAGE_TIFF = 5,
        IMAGE_PNG = 8,
        IMAGE_TGA = 9
    };

    enum Orientation
    {
        ORIENTATION_TOP_DOWN = 1,
        ORIENTATION_BOTTOM_UP = -1
    };

    enum PixelFormat
    {
        PIXEL_UNKNOWN = 0,
        PIXEL_BOOLEAN1 = 1,
        PIXEL_UNSIGNED8 = 8,
        PIXEL_FLOAT32 = 32
    };

    enum Type
    {
        IMAGE_NONE = 0,
        IMAGE_SRGB_IMAGE = 1,
        IMAGE_LINEAR_MASK = 2,
        IMAGE_MASKED_SRGB_IMAGE = 3,
        IMAGE_SDF = 4,
        IMAGE_PSDF = 5,
        IMAGE_MSDF = 6,
        IMAGE_MTSDF = 7,
        IMAGE_MIXED_CONTENT = 255
    };

    struct ImageHeader
    {
        enum Type
        {
            IMAGE_NONE = 0,
            IMAGE_SRGB_IMAGE = 1,
            IMAGE_LINEAR_MASK = 2,
            IMAGE_MASKED_SRGB_IMAGE = 3,
            IMAGE_SDF = 4,
            IMAGE_PSDF = 5,
            IMAGE_MSDF = 6,
            IMAGE_MTSDF = 7,
            IMAGE_MIXED_CONTENT = 255
        };

        uint32 flags;
        uint32 encoding;
        uint32 width, height;
        uint32 channels;
        uint32 pixelFormat;
        uint32 imageType;
        uint32 rowLength;
        int orientation;
        uint32 childImages;
        uint32 textureFlags;
        uint32 reserved[3];
        uint32 metadataLength;
        uint32 dataLength;
    };

    struct AppendixHeader
    {
        uint32 metadataLength;
        uint32 dataLength;
    };

    struct FontFooter
    {
        uint32 salt;
        uint32 magicNo;
        uint32 reserved[4];
        uint32 totalLength;
        uint32 checksum;
    };

    enum CodepointType
    {
        CP_UNSPECIFIED = 0,
        CP_UNICODE = 1,
        CP_INDEXED = 2,
        CP_ICONOGRAPHIC = 14
    };

    struct KernPair
    {
        CodepointType Codepoint1, Codepoint2;
        struct { float h, v; } Advance;
    };

    Font& FontDecoder::Decode(Context& ctx)
    {
        Font* font = ctx.FontAlloc.New<Font>();
        ctx.Fonts.PushBack(font);

        uint32 variantLen = 0;
        uint32 imageLen = 0;
        uint32 appendixLen = 0;

        // Read header
        {
            FontHeader header{};

            this->ReadBytes((uint8*)&header, sizeof(header));

            HAX_ASSERT(memcmp(header.Tag, "ARTERY/FONT\0\0\0\0\0", sizeof(header.Tag)) == 0);
            HAX_ASSERT(header.MagicNo == 0x4d276a5cu);
            HAX_ASSERT(header.RealType == 0x14u);

            this->ReadString(nullptr, header.MetadataLength);

            HAX_ASSERT(header.VariantCount == 1);
            HAX_ASSERT(header.ImageCount == 1);
            HAX_ASSERT(header.AppendixCount == 0);

            variantLen = header.VariantsLength;
            imageLen = header.ImagesLength;
            appendixLen = header.AppendicesLength;
        }

        const uint8* savedPtr = m_ReadCursor;

        // Read variant
        {
            FontVariant variant{};
            this->ReadBytes((uint8*)&variant, sizeof(variant));

            HAX_ASSERT(variant.codepointType == CP_UNICODE);
            HAX_ASSERT(variant.imageType == IMAGE_MSDF);
            HAX_ASSERT(variant.kernPairCount == 0);

            font->Size = variant.metrics.fontSize;
            font->DistanceRange = variant.metrics.distanceRange;
            font->EmSize = variant.metrics.emSize;
            font->Ascender = variant.metrics.ascender;
            font->Descender = variant.metrics.descender;
            font->LineHeight = variant.metrics.lineHeight;

            this->ReadString(nullptr, variant.nameLength);
            this->ReadString(nullptr, variant.metadataLength);

            for (uint32 i = 0; i < variant.glyphCount; ++i)
            {
                Glyph* glyph = (Glyph*)m_ReadCursor;
                font->Glyphs.Insert(glyph->Codepoint, glyph);

                this->ReadBytes(nullptr, sizeof(Glyph));
            }

            this->ReadBytes(nullptr, variant.kernPairCount * sizeof(KernPair));
        }

        HAX_ASSERT((m_ReadCursor - savedPtr) == variantLen);
        savedPtr = m_ReadCursor;

        // Read image
        {
            ImageHeader header{};
            this->ReadBytes((uint8*)&header, sizeof(header));

            HAX_ASSERT(header.encoding == IMAGE_PNG);
            HAX_ASSERT(header.channels == 3);
            HAX_ASSERT(header.imageType == IMAGE_MSDF);

            this->ReadString(nullptr, header.metadataLength);
            font->Atlas = LoadImageFromMemory(ArrayView(m_ReadCursor, header.dataLength));

            HAX_ASSERT(font->Atlas.Width == header.width);
            HAX_ASSERT(font->Atlas.Height == header.height);

            this->ReadBytes(nullptr, header.dataLength);
            this->AlignCursor();
        }

        HAX_ASSERT((m_ReadCursor - savedPtr) == imageLen);
        savedPtr = m_ReadCursor;

        // Read footer
        {
            FontFooter footer{};
            this->ReadBytes((uint8*)&footer, sizeof(footer) - sizeof(footer.checksum));
            HAX_ASSERT(footer.magicNo == 0x55ccb363u);

            uint32 checksum = m_Checksum;
            this->ReadBytes((uint8*)&footer.checksum, sizeof(footer.checksum));
            HAX_ASSERT(footer.checksum == checksum);
            HAX_ASSERT((m_ReadCursor - m_Binary) == footer.totalLength);
        }

        return *font;
    }

    static Layer* CreateLayer(Context& ctx, StringView name, int zOrder);
    static Layer* FindLayerById(Context& ctx, uint64 id);
    static void Initialize(Context& ctx, Handle hwnd);
    static void InitKeyNames();
    static InterRes Interact(Context& ctx, uint64 id, const Rect& bounds);
    static Font& LoadFontFromMemory(Context& ctx, ArrayView<const uint8> data);
    static Texture2D LoadImageFromMemory(Context& ctx, ArrayView<const uint8> data);
    static void ProcessWndMsgs(Context& ctx);
    static void ResetLayer(Layer& layer);
    static void UpdateViewportSize(Viewport& viewport);
    static void UpdateViewportScale(Viewport& viewport);
    static void UpdateWheelScroll(Context& ctx, uint64 id, ContainerState& state, float maxHeight, float maxOffset);
    static bool UpdateMouseIcon();
    static void UpdateKeyboardInputs(Context& ctx);

    Context* g_Context;
    float g_ScaleFactor;
    static Hax::String g_KeyToName[256];

    void BeginFrame()
    {
        HAX_ASSERT(g_Context != nullptr && "Context not set");

        {
            Timer& timer = g_Context->Timer;

            int64 totalTicksThisFrame = 0;
            ::QueryPerformanceCounter((LARGE_INTEGER*)&totalTicksThisFrame);

            timer.DeltaTime = (float)(totalTicksThisFrame - timer.TotalTicksLastFrame) / timer.TicksPerSecond;
            timer.Time += timer.DeltaTime;
            timer.TotalTicksLastFrame = totalTicksThisFrame;

            timer.FramerateSecPerFrameAccum += timer.DeltaTime - timer.FramerateSecPerFrame[timer.FramerateSecPerFrameIdx];
            timer.FramerateSecPerFrame[timer.FramerateSecPerFrameIdx] = timer.DeltaTime;
            timer.FramerateSecPerFrameCount = std::max(timer.FramerateSecPerFrameCount, ++timer.FramerateSecPerFrameIdx);
            timer.FramerateSecPerFrameIdx %= (int)_countof(timer.FramerateSecPerFrame);
        }

        {
            Mouse& mouse = g_Context->Mouse;
            mouse.PrevLmbDown = mouse.LmbDown;
            mouse.DeltaPos = mouse.DeltaWheel = Vector2();
            mouse.PrevIcon = mouse.Icon;
            mouse.Icon = MouseIcon_Default;
        }

        {
            Viewport& viewport = g_Context->Viewport;
            UpdateViewportSize(viewport);

        }

        {
            g_Context->LayerStack.Clear();
            g_Context->CurrentLayer = g_Context->DefaultLayer;

            for (Layer* layer : g_Context->Layers)
                ResetLayer(*layer);
        }

        ProcessWndMsgs(*g_Context);

        {
            Interaction& inter = g_Context->Interaction;

            inter.ThisFrame.HoveredItem = inter.NextFrame.HoveredItem;
            inter.ThisFrame.HoveredContainer = inter.NextFrame.HoveredContainer;
            inter.ThisFrame.ActiveItemId = inter.NextFrame.ActiveItemId;
            inter.NextFrame.ActiveItemId = kInvalidId;
            inter.ClickedItemId = kInvalidId;

            if (g_Context->Mouse.IsLmbJustPressed())
            {
                inter.ThisFrame.ActiveItemId = inter.ThisFrame.HoveredItem.Id;
                inter.FocusedItemId = inter.ThisFrame.ActiveItemId;
            }

            if (g_Context->Mouse.IsLmbJustReleased())
            {
                if (inter.ThisFrame.ActiveItemId == inter.ThisFrame.HoveredItem.Id)
                    inter.ClickedItemId = inter.ThisFrame.ActiveItemId;
                inter.ThisFrame.ActiveItemId = kInvalidId;
            }

            inter.NextFrame.HoveredItem = {};
            inter.NextFrame.HoveredContainer = {};
        }

        UpdateKeyboardInputs(*g_Context);
    }

    void EndFrame()
    {
        for (Layer* layer : g_Context->Layers)
        {
            HAX_ASSERT(layer->ContainerStack.Empty() && "Begin / End container mismatch!");
            HAX_ASSERT(layer->ClipRectStack.Empty() && "Push / Pop cliprect mismatch!");
            HAX_ASSERT(layer->RotationStack.Empty() && "Begin / End rotation mismatch!");
            HAX_ASSERT(layer->LayoutStack.Empty() && "Begin / End layout mismatch!");
        }

        std::sort(g_Context->Layers.begin(), g_Context->Layers.end(), [](Layer* l1, Layer* l2) { return l1->ZOrder < l2->ZOrder; });
        g_Context->Backend->Render();
    }

    // Context

    Context* CreateContext(Handle hwnd)
    {
        Context* newContext = g_GlobalAlloc.New<Context>();
        Initialize(*newContext, hwnd);

        if (g_Context == nullptr)
        {
            g_Context = newContext;
            g_ScaleFactor = newContext->Viewport.ScaleFactor;
        }

        return newContext;
    }

    void DestroyContext(Context* context)
    {
        if (context == nullptr)
            context = g_Context;

        if (context != nullptr)
        {
            if (context == g_Context)
                g_Context = nullptr;

            g_GlobalAlloc.Delete(context);
        }
    }

    Context* GetCurrentContext()
    {
        return g_Context;
    }

    void SetCurrentContext(Context* context)
    {
        g_Context = context;
        g_ScaleFactor = context->Viewport.ScaleFactor;
    }

    // Timer

    double GetTime()
    {
        return g_Context->Timer.Time;
    }

    float GetDeltaTime()
    {
        return g_Context->Timer.DeltaTime;
    }

    float GetFramerate()
    {
        Timer& timer = g_Context->Timer;
        return (float)timer.FramerateSecPerFrameCount / timer.FramerateSecPerFrameAccum;
    }

    Vector2 GetMouseDeltaPos()
    {
        return g_Context->Mouse.DeltaPos;
    }

    void SetMouseIcon(MouseIcon icon)
    {
        g_Context->Mouse.Icon = icon;
    }

    //

    long HandleWndMsg(void* hwnd, uint32 msg, uintptr wParam, uint64 lParam)
    {
        if (!g_Context)
            return 0;

        if (msg == WM_SETCURSOR)
        {
            if (LOWORD(lParam) == HTCLIENT && UpdateMouseIcon())
                return 1;
            return 0;
        }

        WndMsg newMsg
        {
            .Hwnd = (HWND)hwnd,
            .Msg = msg,
            .WParam = wParam,
            .LParam = lParam
        };
        
        switch (msg)
        {
            case WM_MOUSEMOVE:
            case WM_LBUTTONDOWN:
            case WM_LBUTTONDBLCLK:
            case WM_LBUTTONUP:
            case WM_MOUSEWHEEL:
            case WM_MOUSEHWHEEL:
            case WM_KEYDOWN:
            case WM_KEYUP:
            case WM_SYSKEYDOWN:
            case WM_SYSKEYUP:
                g_Context->WndMsgQueue.Write(newMsg);
        }

        return 0;
    }

    // Layer

    void CreateLayer(StringView name, int zOrder)
    {
        CreateLayer(*g_Context, name, zOrder);
    }

    void SwitchLayer(StringView name)
    {
        uint64 id = Hash(name);
        Layer* layer = FindLayerById(*g_Context, id);

        HAX_ASSERT(layer != nullptr && "Layer not found");

        g_Context->LayerStack.PushBack(g_Context->CurrentLayer);
        g_Context->CurrentLayer = layer;
    }

    void RestoreLayer()
    {
        HAX_ASSERT(!g_Context->LayerStack.Empty() && "Layer stack empty");

        g_Context->CurrentLayer = g_Context->LayerStack.Last();
        g_Context->LayerStack.PopBack();
    }

    void DrawRect(const Vector2& a, const Vector2& b, const RectParams& params)
    {
        g_Context->CurrentLayer->AddRect(a, b, params);
    }

    void DrawEllipse(const Vector2& c, const Vector2& r, const EllipseParams& params)
    {
        g_Context->CurrentLayer->AddEllipse(c, r, params);
    }

    void DrawCircle(const Vector2& c, float r, const CircleParams& params)
    {
        g_Context->CurrentLayer->AddEllipse(c, { r, r }, params);
    }

    void DrawTriangle(const Vector2& a, const Vector2& b, const Vector2& c, const TriangleParams& params)
    {
        g_Context->CurrentLayer->AddTriangle(a, b, c, params);
    }

    void DrawLine(const Vector2& a, const Vector2& b, const LineParams& params)
    {
        g_Context->CurrentLayer->AddLine(a, b, params);
    }

    void DrawImage(Texture2D image, const Vector2& a, const Vector2& b, const ImageParams& params)
    {
        g_Context->CurrentLayer->AddImage(image, a, b, params);
    }

    void DrawGlyph(const Font& font, char16 sym, const Vector2& pos, float size, const GlyphParams& params)
    {
        g_Context->CurrentLayer->AddGlyph(font, sym, pos, size, params);
    }

    void DrawTexto(const Font& font, StringView text, const Vector2& pos, float size, const TextParams& params)
    {
        g_Context->CurrentLayer->AddText(font, text, pos, size, params);
    }

    // Skip

    void BeginSkip()
    {
        g_Context->CurrentLayer->SkipCounter++;
    }

    void EndSkip(uint32 count)
    {
        int& counter = g_Context->CurrentLayer->SkipCounter;
        HAX_ASSERT(counter > 0 && "Begin / End skip mismatch");
        counter--;
    }

    bool IsSkipping()
    {
        return g_Context->CurrentLayer->SkipCounter > 0;
    }

    void BeginRotation(float angleRad)
    {
        g_Context->CurrentLayer->BeginRotation(angleRad);
    }

    void EndRotation()
    {
        g_Context->CurrentLayer->EndRotation();
    }

    void EndRotation();

    void PushClipRect(const Rect& clipRect)
    {
        g_Context->CurrentLayer->PushClipRect(clipRect);
    }

    void PopClipRect()
    {
        g_Context->CurrentLayer->PopClipRect();
    }

    void BeginVertical(float spacing)
    {
        g_Context->CurrentLayer->BeginLayout(spacing, LayoutType::Vertical);
    }

    void EndVertical()
    {
        g_Context->CurrentLayer->EndLayout(LayoutType::Vertical);
    }

    void BeginHorizontal(float spacing)
    {
        g_Context->CurrentLayer->BeginLayout(spacing, LayoutType::Horizontal);
    }

    void EndHorizontal()
    {
        g_Context->CurrentLayer->EndLayout(LayoutType::Horizontal);
    }

    void PlaceItem(const Vector2& size)
    {
        g_Context->CurrentLayer->CurrentLayout.PlaceItem(size);
    }

    void Space(float pixels)
    {
        g_Context->CurrentLayer->CurrentLayout.Space(pixels);
    }

    Vector2 GetCursorPos()
    {
        return g_Context->CurrentLayer->CurrentLayout.CursorPos;
    }

    void SetCursorPos(const Vector2& pos)
    {
        Layout& layout = g_Context->CurrentLayer->CurrentLayout;
        HAX_ASSERT(layout.IsRoot() && "SetCursorPos must be used in root layers");
        layout.CursorPos = pos;
    }

    void VerticalLine(float th, const Color& color, float size)
    {
        if (size <= 0.f) size = GetContentRegionAvail().Y + size;

        Rect bounds;
        bounds.Min = GetCursorPos();
        bounds.Max = bounds.Min + Vector2(th, size);

        PlaceItem(bounds.GetSize());
        if (IsItemVisible(bounds))
            DrawRect(bounds.Min, bounds.Max, { .FillColor = color });
    }

    void HorizontalLine(float th, const Color& color, float size)
    {
        if (size <= 0.f) size = GetContentRegionAvail().X + size;

        Rect bounds;
        bounds.Min = GetCursorPos();
        bounds.Max = bounds.Min + Vector2(size, th);

        PlaceItem(bounds.GetSize());
        if (IsItemVisible(bounds))
            DrawRect(bounds.Min, bounds.Max, { .FillColor = color });
    }

    Rect GetLayoutBounds()
    {
        return g_Context->CurrentLayer->CurrentLayout.Bounds;
    }

    float GetLayoutSpacing()
    {
        return g_Context->CurrentLayer->CurrentLayout.Spacing;
    }

    void ResetCursor()
    {
        g_Context->CurrentLayer->CurrentLayout.Reset();
    }

    bool IsItemVisible(const Rect& bounds)
    {
        return g_Context->CurrentLayer->IsItemVisible(bounds);
    }

    void BeginContainer(uint64 id, const ContainerParams& params)
    {
        g_Context->CurrentLayer->BeginContainer(id, params);
    }

    Vector2 GetContentRegionAvail()
    {
        Layer& layer = *g_Context->CurrentLayer;
        return layer.CurrentContainer.Bounds.Max - layer.CurrentLayout.CursorPos;
    }

    void ScrollYTo(float posY)
    {
        uint64 id = g_Context->CurrentLayer->CurrentScrollId;
        if (id == kInvalidId)
            return;

        ContainerState& state = GetState<ContainerState>(id);

        float minY = state.Bounds.Min.Y;
        float maxY = state.Bounds.Max.Y;

        if (posY > minY && posY < maxY)
            state.Scroll.TargetOffsetY = posY - minY;
    }

    void EndContainer()
    {
        g_Context->CurrentLayer->EndContainer();
    }

    Rect GetContainerBounds()
    {
        return g_Context->CurrentLayer->CurrentContainer.Bounds;
    }

    void Interact(uint64 id, const Rect& bounds)
    {
        Interact(*g_Context, id, bounds);
    }

    bool IsItemHovered(uint64 id)
    {
        return g_Context->Interaction.ThisFrame.HoveredItem.Id == id;
    }

    bool IsItemFocused(uint64 id)
    {
        return g_Context->Interaction.FocusedItemId == id;
    }

    bool IsItemActive(uint64 id)
    {
        return g_Context->Interaction.ThisFrame.ActiveItemId == id;
    }

    bool IsItemClicked(uint64 id)
    {
        return g_Context->Interaction.ClickedItemId == id;
    }

    bool IsItemPressed(uint64 id)
    {
        return IsItemActive(id) && IsItemHovered(id);
    }

    ArrayView<const uint8> GetResourceData(Handle resModule, int resId, const char16* resType)
    {
        HRSRC resInfo = ::FindResource((HMODULE)resModule, MAKEINTRESOURCE(resId), resType);
        HAX_ASSERT(resInfo);

        HGLOBAL resMem = ::LoadResource((HMODULE)resModule, resInfo);
        HAX_ASSERT(resMem);

        uint8* resData = (uint8*)::LockResource(resMem);
        HAX_ASSERT(resData);

        DWORD resDataSize = ::SizeofResource((HMODULE)resModule, resInfo);
        return ArrayView(resData, resDataSize);
    }

    Font& LoadFontFromMemory(ArrayView<const uint8> data)
    {
        return LoadFontFromMemory(*g_Context, data);
    }

    Vector2 CalcTextSize(const Font& font, StringView text, float size, float spacing)
    {
        Vector2 textSize{ 0.f, font.LineHeight * size };

        float curLineWidth = 0.f;

        for (char16 wc : text)
        {
            if (wc == '\n')
            {
                textSize.X = std::max(textSize.X, curLineWidth);
                textSize.Y += font.LineHeight * size;
                curLineWidth = 0.f;
                continue;
            }

            Glyph* const* it = font.Glyphs.Get(wc);
            if (it == nullptr)
                continue;

            Glyph* glyph = *it;
            curLineWidth += glyph->Advance.X * size * spacing;
        }

        textSize.X = std::max(textSize.X, curLineWidth);
        return textSize;
    }

    Texture2D LoadImageFromMemory(ArrayView<const uint8> data)
    {
        return LoadImageFromMemory(*g_Context, data);
    }

    Vector2 GetViewportSize()
    {
        return g_Context->Viewport.Size;
    }

    bool IsKeyDown(uint8 vk)
    {
        return g_Context->KeysData[vk].Down;
    }

    bool IsKeyJustDown(uint8 vk)
    {
        return g_Context->KeysData[vk].DownDuration == 0.f;
    }

    bool IsKeyUp(uint8 vk)
    {
        return !g_Context->KeysData[vk].Down;
    }

    bool IsKeyJustUp(uint8 vk)
    {
        return g_Context->KeysData[vk].DownDurationPrev >= 0.f && !g_Context->KeysData[vk].Down;
    }

    StringView GetKeyName(uint8 vk)
    {
        return g_KeyToName[vk].CStr();
    }

    IStatePool** GetStatePool(size_t typeId)
    {
        return g_Context->StatePools.Get(typeId);
    }

    void AddStatePool(size_t typeId, IStatePool* pool)
    {
        g_Context->StatePools.Insert(typeId, pool);
    }

    Allocator* IterAllocators(void*& iter)
    {
        size_t& idx = (size_t&)iter;

        Allocator* allocators[] = {&g_GlobalAlloc, &g_StateAlloc, &g_Context->FontAlloc, &g_Context->GeneralAlloc};
        return idx < _countof(allocators) ? allocators[idx++] : nullptr;
    }
}

namespace Hax
{
    const LinearColor LinearColor::White(1.f,1.f,1.f);
    const LinearColor LinearColor::Gray(0.5f,0.5f,0.5f);
    const LinearColor LinearColor::Black(0,0,0);
    const LinearColor LinearColor::Transparent(0,0,0,0);
    const LinearColor LinearColor::Red(1.f,0,0);
    const LinearColor LinearColor::Green(0,1.f,0);
    const LinearColor LinearColor::Blue(0,0,1.f);
    const LinearColor LinearColor::Yellow(1.f,1.f,0);

    

    LinearColor LinearColor::Brighten(float factor) const
    {
        LinearColor target{1.f, 1.f, 1.f, A};
        return Lerp(*this, target, factor);
    }

    LinearColor LinearColor::Darken(float factor) const
    {
        LinearColor target{0.f, 0.f, 0.f, A};
        return Lerp(*this, target, factor);
    }
}

namespace Hax::Gui
{
    void Layer::AddRect(const Vector2& a, const Vector2& b, const RectParams& params)
    {
        if (this->IsSkipping() || (params.FillColor.IsTransparent() && params.BorderColor.IsTransparent()))
            return;
        
        RenderItem item{};
        item.Color1 = params.FillColor;
        item.Color2 = params.BorderColor;
        item.Type = RenderItemType::Rect;
        item.Sin = CurrentRotation.Sin;
        item.Cos = CurrentRotation.Cos;
        item.Rect.Min = Vector2(Floor(a.X), Floor(a.Y));
        item.Rect.Max = Vector2(Floor(b.X), Floor(b.Y));
        item.Rect.R = params.Rounding;
        item.Rect.BorderTh = params.BorderTh;
        
        this->AddItem(item);
    }

    void Layer::AddEllipse(const Vector2& c, const Vector2& r, const EllipseParams& params)
    {
        if (this->IsSkipping() || (params.FillColor.IsTransparent() && params.BorderColor.IsTransparent()))
            return;
    
        RenderItem item{};
        item.Type = RenderItemType::Ellipse;
        item.Color1 = params.FillColor;
        item.Color2 = params.BorderColor;
        item.Sin = CurrentRotation.Sin;
        item.Cos = CurrentRotation.Cos;
        item.Ellipse.Center = c;
        item.Ellipse.R = r;
        item.Ellipse.BorderTh = params.BorderTh;
    
        this->AddItem(item);
    }
    
    void Layer::AddTriangle(const Vector2& a, const Vector2& b, const Vector2& c, const TriangleParams& params)
    {
        if (this->IsSkipping() || (params.FillColor.IsTransparent() && params.BorderColor.IsTransparent()))
            return;
    
        RenderItem item{};
        item.Type = RenderItemType::Triangle;
        item.Color1 = params.FillColor;
        item.Color2 = params.BorderColor;
        item.Sin = CurrentRotation.Sin;
        item.Cos = CurrentRotation.Cos;
        item.Triangle.A = a;
        item.Triangle.B = b;
        item.Triangle.C = c;
        item.Triangle.BorderTh = params.BorderTh;
    
        this->AddItem(item);
    }
    
    void Layer::AddLine(const Vector2& a, const Vector2& b, const LineParams& params)
    {
        if (params.Th <= 0.f)
            return;
    
        RenderItem item{};
        item.Type = RenderItemType::Line;
        item.Color1 = params.FillColor;
        item.Sin = CurrentRotation.Sin;
        item.Cos = CurrentRotation.Cos;
        item.Line.A = a;
        item.Line.B = b;
        item.Line.Th = params.Th;
    
        this->AddItem(item);
    }
    
    void Layer::AddImage(Texture2D image, const Vector2& a, const Vector2& b, const ImageParams& params)
    {
        if (this->IsSkipping() || a.X >= b.X || a.Y >= b.Y)
            return;
    
        RenderItem item{};
        item.Type = RenderItemType::Image;
        item.Sin = CurrentRotation.Sin;
        item.Cos = CurrentRotation.Cos;
        item.Color1 = params.BgColor;
        item.Image.A = a;
        item.Image.B = b;
        item.Image.UVmin = params.UVmin;
        item.Image.UVmax = params.UVmax;
        item.Image.R = params.R;
    
        this->AddItem(item, {.Image = image.Handle});
    }
    
    Vector2 Layer::AddGlyph(const Font& font, char16 sym, const Vector2& pos, float size, const GlyphParams& params)
    {
        if (this->IsSkipping() || size < 1.f)
            return pos;
    
        auto it = font.Glyphs.Get(sym);
        if (it == nullptr)
            return pos;
    
        Glyph* glyph = *it;
    
        Vector2 imgSize = { (float)font.Atlas.Width, (float)font.Atlas.Height };
    
        Vector2 uvPos1 = Vector2(glyph->ImageBounds.L, imgSize.Y - glyph->ImageBounds.T) / imgSize;
        Vector2 uvPos2 = Vector2(glyph->ImageBounds.R, imgSize.Y - glyph->ImageBounds.B) / imgSize;
    
        float scale = size;
        Vector2 baseLine{pos.X, pos.Y + font.Ascender * size};
        Vector2 posTL = baseLine + Vector2(glyph->PlaneBounds.L, -glyph->PlaneBounds.T) * scale;
        Vector2 posBR = baseLine + Vector2(glyph->PlaneBounds.R, -glyph->PlaneBounds.B) * scale;
    
        RenderItem item{};
        item.Type = RenderItemType::Glyph;
        item.Sin = CurrentRotation.Sin;
        item.Cos = CurrentRotation.Cos;
        item.Color1 = params.Color;
        item.Image.A = posTL;
        item.Image.B = posBR;
        item.Image.UVmin = uvPos1;
        item.Image.UVmax = uvPos2;
    
        this->AddItem(item, {.Font = &font});
    
        return pos + glyph->Advance * scale * params.Spacing;
    }
    
    void Layer::AddText(const Font& font, StringView text, const Vector2& pos, float size, const TextParams& params)
    {
        if (this->IsSkipping() || size < 1.f)
            return;
    
        Vector2 glyphPos = pos;
        for (char16 wc : text)
        {
            if (wc == '\n')
            {
                glyphPos = Vector2(pos.X, glyphPos.Y + font.LineHeight * size);
                continue;
            }
    
            glyphPos = this->AddGlyph(font, wc, glyphPos, size, params);
        }
    }

    void Layer::AddItem(const RenderItem& item, const RenderItemRes& resources)
    {
        RenderItems.PushBack(item);
        
        RenderBatch newBatch{};
        
        if (!RenderBatches.Empty())
        {
            const RenderBatch& prevBatch = RenderBatches.Last();
        
            newBatch = prevBatch;
            newBatch.ActionMask = RenderBatchAction_None;
            newBatch.InstancesNum = 0;
        }
        
        if (resources.Font != nullptr && resources.Font != newBatch.Font)
        {
            newBatch.Font = resources.Font;
            newBatch.ActionMask |= RenderBatchAction_SetFont;
        }
        
        if (resources.Image != 0 && resources.Image != newBatch.Texture)
        {
            newBatch.Texture = resources.Image;
            newBatch.ActionMask |= RenderBatchAction_SetTexture;
        }
        
        if (newBatch.ClipRect != CurrentClipRect)
        {
            newBatch.ClipRect = CurrentClipRect;
            newBatch.ActionMask |= RenderBatchAction_SetClipRect;
        }
        
        if (newBatch.ActionMask != RenderBatchAction_None)
            RenderBatches.PushBack(newBatch);
        
        RenderBatches.Last().InstancesNum++;
    }

    void Layer::BeginRotation(float angle)
    {
        RotationStack.PushBack(CurrentRotation);

        CurrentRotation.Angle += angle;
        CurrentRotation.Sin = Sin(CurrentRotation.Angle);
        CurrentRotation.Cos = Cos(CurrentRotation.Angle);
    }

    void Layer::EndRotation()
    {
        HAX_ASSERT(!RotationStack.Empty() && "Begin / End rotation mismatch!");

        CurrentRotation = RotationStack.Last();
        RotationStack.PopBack();
    }

    void Layer::BeginSkip() 
    { 
        SkipCounter++;
    }

    void Layer::EndSkip() 
    { 
        HAX_ASSERT(SkipCounter > 0 && "Skip mismatch"); 
        SkipCounter--; 
    }

    bool Layer::IsSkipping() const
    { 
        return CurrentClipRect.IsInverted() || SkipCounter > 0; 
    }

    void Layer::PushClipRect(const Rect& clipRect)
    {
        ClipRectStack.PushBack(CurrentClipRect); 
        CurrentClipRect = CurrentClipRect.Intersect(clipRect); 
    }

    void Layer::PopClipRect()
    {
        HAX_ASSERT(!ClipRectStack.Empty()); 
        CurrentClipRect = ClipRectStack.Last(); 
        ClipRectStack.PopBack();
    }

    void Layer::BeginLayout(float spacing, LayoutType type)
    {            
        LayoutStack.PushBack(CurrentLayout);

        CurrentLayout.Bounds = Rect(CurrentLayout.CursorPos, CurrentLayout.CursorPos);
        CurrentLayout.Type = type;
        CurrentLayout.Spacing = spacing;
    }

    void Layer::EndLayout(LayoutType type)
    {
        HAX_ASSERT(CurrentLayout.Type == type);
        HAX_ASSERT(!LayoutStack.Empty() && "Begin / End layout mismatch!");

        Vector2 size = CurrentLayout.Bounds.GetSize();

        CurrentLayout = LayoutStack.Last();
        CurrentLayout.PlaceItem(size);
        LayoutStack.PopBack();
    }

    void Layer::BeginContainer(uint64 id, const ContainerParams& params)
    {
        Container& cont = CurrentContainer;

        // Calculate bounds
        Vector2 cursorPos = GetCursorPos();
        Vector2 spaceLeft = cont.Bounds.Max - cursorPos;

        Vector2 size{params.W, params.H};
        if (size.X <= 0.f) size.X = spaceLeft.X + size.X;
        if (size.Y <= 0.f) size.Y = spaceLeft.Y + size.Y;

        uint32 flags = ContainerFlag_None;
        if (params.Clip)            { flags |= ContainerFlag_Clip; }
        if (params.ScrollX)         { flags |= ContainerFlag_ScrollX; }
        if (params.ScrollY)         { flags |= ContainerFlag_ScrollY; }
        if (params.ScrollVisible)   { flags |= ContainerFlag_ScrollVisible; }
        if (params.FitX)            { flags |= ContainerFlag_FitX; flags &= ~ContainerFlag_ScrollX; }
        if (params.FitY)            { flags |= ContainerFlag_FitY; flags &= ~ContainerFlag_ScrollY; }

        Rect bounds = Rect(cursorPos, cursorPos + size);

        Vector2 layoutPos = cursorPos;

        if (flags > ContainerFlag_Clip)
        {
            HAX_ASSERT(id > 0);
            auto& state = GetState<ContainerState>(id);
            state.Scroll.Style = params.Style;

            if (flags & ContainerFlag_FitX)
                bounds.Max.X = bounds.Min.X + state.Bounds.GetSize().X;
            if (flags & ContainerFlag_FitY)
                bounds.Max.Y = bounds.Min.Y + state.Bounds.GetSize().Y;

            auto& scroll = state.Scroll;
            if ((flags & ContainerFlag_ScrollY) != 0 && scroll.Offset.Y >= 0.f)
            {
                CurrentScrollId = id; 

                Vector2 mousePos = Context->Mouse.Pos;
                if (CurrentClipRect.Contains(mousePos) && bounds.Contains(mousePos))
                {
                    auto& hoveredCont = Context->Interaction.NextFrame.HoveredContainer;

                    if (ZOrder >= hoveredCont.ZOrder)
                    {
                        hoveredCont.Id = id;
                        hoveredCont.ZOrder = ZOrder;
                    }
                }

                bounds.Max.X -= params.Style.TrackWidth; //!ScaleFactor
                layoutPos.Y -= Floor(scroll.Offset.Y);
            }

            if ((flags & ContainerFlag_ScrollX) != 0 && scroll.Offset.X >= 0.f)
            {
                CurrentScrollId = id; 

                bounds.Max.Y -= params.Style.TrackWidth; //!ScaleFactor
                layoutPos.X -= Floor(scroll.Offset.X);
                flags |= ContainerFlag_ScrollX;
            }
        }

        // Start new layout
        LayoutStack.PushBack(CurrentLayout);

        CurrentLayout.CursorPos = layoutPos;
        CurrentLayout.Bounds = Rect(layoutPos, layoutPos);
        CurrentLayout.Type = LayoutType::Container;

        // Start new container
        ContainerStack.PushBack(CurrentContainer);
        CurrentContainer.Bounds = bounds;
        CurrentContainer.Id = id;
        CurrentContainer.Flags = flags;

        if ((flags & ContainerFlag_Clip))
            this->PushClipRect(bounds);
    }

    Container Layer::EndContainer()
    {
        _ASSERTE(CurrentLayout.Type == LayoutType::Container);

        ContainerState* state = nullptr;
        if (CurrentContainer.RequiresState())
            state = &GetState<ContainerState>(CurrentContainer.Id);

        if (state)
            state->Bounds = CurrentLayout.Bounds;

        Rect contentBounds = CurrentLayout.Bounds;
        float maxHeight = CurrentContainer.Bounds.GetSize().Y;
        float maxWidth = CurrentContainer.Bounds.GetSize().X;
        float height = contentBounds.GetSize().Y;
        float width = contentBounds.GetSize().X;
        bool yScrollVisible = (height > maxHeight && CurrentContainer.ScrollY()) || CurrentContainer.ScrollVisible();
        bool xScrollVisible = (width > maxWidth && CurrentContainer.ScrollX()) || CurrentContainer.ScrollVisible();
        Vector2 realSize = CurrentContainer.Bounds.GetSize();
        if (yScrollVisible)
            realSize.X += state->Scroll.Style.TrackWidth;
        if (xScrollVisible)
            realSize.Y += state->Scroll.Style.TrackWidth;

        CurrentLayout = LayoutStack.Last();
        LayoutStack.PopBack();
        CurrentLayout.PlaceItem(realSize);

        if ((CurrentContainer.Flags & ContainerFlag_Clip))
            this->PopClipRect();

        // Scroll
        if (CurrentContainer.ScrollX() || CurrentContainer.ScrollY())
        {
            auto& scroll = state->Scroll;

            uint64 internalId = (uint64)&scroll;

            float trackWidth = std::max(3.f, state->Scroll.Style.TrackWidth);//!SCaleFactor
            float padding = std::max(0.f, std::min(state->Scroll.Style.ThumbPadding, (trackWidth - 1) / 2.f)); //!ScaleFactor
            float thumbWidth = trackWidth - padding * 2.f;

            // ScrollY
            if (CurrentContainer.ScrollY())
            {
                if (yScrollVisible)
                {
                    height = std::max(height, maxHeight);

                    bool firstFrameVisible = scroll.Offset.Y < 0.f;
                    scroll.Offset.Y = std::max(0.f, std::min(scroll.Offset.Y, height - maxHeight));

                    Rect trackBounds;
                    trackBounds.Min = CurrentContainer.Bounds.GetTR();
                    trackBounds.Max = CurrentContainer.Bounds.GetBR() + Vector2(trackWidth, 0.f);

                    Rect thumbBounds;
                    thumbBounds.Min = trackBounds.Min + Vector2(padding, scroll.Offset.Y / height * trackBounds.GetSize().Y + padding);
                    thumbBounds.Max = thumbBounds.Min + Vector2(thumbWidth, maxHeight / height * trackBounds.GetSize().Y - padding * 2.f);

                    InterRes trackHitTest = Interact(*Context, internalId, trackBounds);
                    InterRes thumbHitTest = Interact(*Context, internalId + 1, thumbBounds);

                    if (!firstFrameVisible)
                    {
                        Color trackColor = state->Scroll.Style.TrackCol;
                        this->AddRect(trackBounds.Min, trackBounds.Max, {.FillColor = trackColor, .BorderColor = trackColor});

                        Color thumbColorHeld = state->Scroll.Style.ThumbActiveCol;
                        Color thumbColorHovered = state->Scroll.Style.ThumbHovCol;
                        Color thumbColor = thumbHitTest.Active ? thumbColorHeld : thumbHitTest.Hovered ? thumbColorHovered : state->Scroll.Style.ThumbCol;
                        this->AddRect(thumbBounds.Min, thumbBounds.Max, {.FillColor = thumbColor, .Rounding = 5.f});//!Rounding
                    }

                    UpdateWheelScroll(*Context, CurrentContainer.Id, *state, maxHeight, height - maxHeight);

                    if (trackHitTest.Pressed && (float)Context->Timer.Time - scroll.LastTimeHeldY > 0.2f)
                    {
                        scroll.LastTimeHeldY = (float)Context->Timer.Time;

                        float delta = 0.f;
                        if (Context->Mouse.Pos.Y < thumbBounds.Min.Y) delta = -maxHeight;
                        if (Context->Mouse.Pos.Y > thumbBounds.Max.Y) delta = maxHeight;

                        scroll.Offset.Y += delta;
                        scroll.TargetOffsetY += delta;
                    }

                    if (thumbHitTest.Active)
                    {
                        float delta = Context->Mouse.DeltaPos.Y / trackBounds.GetSize().Y * height;
                        scroll.Offset.Y += delta;
                        scroll.TargetOffsetY += delta;
                    }

                    scroll.Offset.Y = std::min(scroll.Offset.Y, height - maxHeight);
                    scroll.Offset.Y = std::max(scroll.Offset.Y, 0.f);

                    scroll.TargetOffsetY = std::max(scroll.TargetOffsetY, 0.f);
                    scroll.TargetOffsetY = std::min(scroll.TargetOffsetY, height - maxHeight);
                }
                else
                {
                    scroll.Offset.Y = -1;
                    scroll.TargetOffsetY = -1;
                }
            }

            // ScrollX
            if (CurrentContainer.ScrollX())
            {
                if (xScrollVisible)
                {
                    width = std::max(width, maxWidth);

                    bool firstFrameVisible = scroll.Offset.X < 0.f;
                    scroll.Offset.X = std::max(0.f, std::min(scroll.Offset.X, width - maxWidth));

                    Rect trackBounds;
                    trackBounds.Min = CurrentContainer.Bounds.GetBL();
                    trackBounds.Max = CurrentContainer.Bounds.GetBR() + Vector2(0.f, trackWidth);

                    Rect thumbBounds;
                    thumbBounds.Min = trackBounds.Min + Vector2(scroll.Offset.X / width * trackBounds.GetSize().X + padding, padding);
                    thumbBounds.Max = thumbBounds.Min + Vector2(maxWidth / width * trackBounds.GetSize().X - padding * 2.f, thumbWidth);

                    InterRes trackHitTest = Interact(*Context, internalId + 2, trackBounds);
                    InterRes thumbHitTest = Interact(*Context, internalId + 3, thumbBounds);

                    if (!firstFrameVisible)
                    {
                        Color trackColor = state->Scroll.Style.TrackCol;
                        this->AddRect(trackBounds.Min, trackBounds.Max, {.FillColor = trackColor});

                        Color thumbColorHeld = state->Scroll.Style.ThumbActiveCol;
                        Color thumbColorHovered = state->Scroll.Style.ThumbHovCol;
                        Color thumbColor = thumbHitTest.Active ? thumbColorHeld : thumbHitTest.Hovered ? thumbColorHovered : state->Scroll.Style.ThumbCol;
                        this->AddRect(thumbBounds.Min, thumbBounds.Max, {.FillColor = thumbColor, .Rounding = 5.f});//!Rounding
                    }

                    if (trackHitTest.Pressed && (float)Context->Timer.Time - scroll.LastTimeHeldX > 0.2f)
                    {
                        scroll.LastTimeHeldX = (float)Context->Timer.Time;

                        if (Context->Mouse.Pos.X < thumbBounds.Min.X) scroll.Offset.X -= maxWidth;
                        if (Context->Mouse.Pos.X > thumbBounds.Max.X) scroll.Offset.X += maxWidth;
                    }

                    if (thumbHitTest.Active)
                        scroll.Offset.X += Context->Mouse.DeltaPos.X / trackBounds.GetSize().X * width;

                    scroll.Offset.X = std::max(scroll.Offset.X, 0.f);
                    scroll.Offset.X = std::min(scroll.Offset.X, width - maxWidth);
                }
                else
                    scroll.Offset.X = -1;
            }
        }

        // End container
        Container copy = CurrentContainer;
        CurrentContainer = ContainerStack.Last();
        ContainerStack.PopBack();
        return copy;
    }

    void Layout::PlaceItem(const Vector2& size)
    {
        Rect itemBounds{CursorPos, CursorPos + size};
        Bounds.Add(itemBounds);

        this->Space(Spacing + (this->IsHorizontal() ? size.X : size.Y));
    }

    void Layout::Space(float pixels)
    {
        if (this->IsHorizontal()) 
            CursorPos.X += pixels;

        if (this->IsVertical())
            CursorPos.Y += pixels;
    }
}

namespace Hax::Gui
{
    static Layer* CreateLayer(Context& ctx, StringView name, int zOrder)
    {
        uint64 id = Hash(name);

        for (Layer* layer : ctx.Layers)
        {
            HAX_ASSERT(layer->Id != id && "Layer name already occupied");
            HAX_ASSERT(layer->ZOrder != zOrder && "Layer zorder already occupied");
        }

        Layer* newLayer = ctx.GeneralAlloc.New<Layer>();
        newLayer->Context = &ctx;
        newLayer->Id = id;
        newLayer->ZOrder = zOrder;
        ResetLayer(*newLayer);

        ctx.Layers.PushBack(newLayer);

        if (ctx.CurrentLayer == nullptr)
            ctx.CurrentLayer = newLayer;

        return newLayer;
    }

    static Layer* FindLayerById(Context& ctx, uint64 id)
    {
        for (Layer* layer : ctx.Layers)
            if (layer->Id == id)
                return layer;

        return nullptr;
    }

    static void Initialize(Context& ctx, Handle hwnd)
    {        
        // Timer
        {
            Timer& timer = ctx.Timer;

            ::QueryPerformanceFrequency((LARGE_INTEGER*)&timer.TicksPerSecond);
            ::QueryPerformanceCounter((LARGE_INTEGER*)&timer.TotalTicksLastFrame);
        }

        // Viewport
        {
            Viewport& viewport = ctx.Viewport;
            viewport.Hwnd = hwnd;
            UpdateViewportSize(viewport);
            UpdateViewportScale(viewport);
        }

        // Layers
        {
            ctx.DefaultLayer = CreateLayer(ctx, L"Default", 0);
        }

        InitKeyNames();
    }

    static void InitKeyNames()
    {
        static bool s_Inited;
        if (s_Inited)
            return;

        s_Inited = true;

        for (uint32 vk = 0; vk < 256; ++vk)
        {
            wchar_t buffer[64];

            uint32 scanCode = ::MapVirtualKeyW(vk, MAPVK_VK_TO_VSC);
            long lParam = (scanCode << 16);

            if (vk >= VK_PRIOR && vk <= VK_HELP)                        lParam |= (1 << 24);
            if (vk >= VK_LWIN  && vk <= VK_APPS)                        lParam |= (1 << 24);
            if (vk == VK_DIVIDE || vk == VK_RCONTROL || vk == VK_RMENU) lParam |= (1 << 24);

            if (::GetKeyNameTextW(lParam, buffer, 64) < 1)
                wsprintf(buffer, L"Key%d", vk);

            g_KeyToName[vk].Append(buffer);
        }
    }

    static InterRes Interact(Context& ctx, uint64 id, const Rect& bounds)
    {
        if (id == kInvalidId)
            return InterRes();

        if (id == ctx.Interaction.ThisFrame.ActiveItemId)
            ctx.Interaction.NextFrame.ActiveItemId = id;

        bool isOtherItemActive = ctx.Interaction.ThisFrame.ActiveItemId != kInvalidId && ctx.Interaction.ThisFrame.ActiveItemId != id;
        bool isBoundsHovered = bounds.Contains(ctx.Mouse.Pos) && ctx.CurrentLayer->CurrentClipRect.Contains(ctx.Mouse.Pos);
        if (!isOtherItemActive && isBoundsHovered)
        {
            auto& hoveredItem = ctx.Interaction.NextFrame.HoveredItem;

            if (ctx.CurrentLayer->ZOrder >= hoveredItem.ZOrder)
            {
                hoveredItem.Id = id;
                hoveredItem.ZOrder = ctx.CurrentLayer->ZOrder;
            }
        }

        InterRes res{};
        res.Active = ctx.Interaction.ThisFrame.ActiveItemId == id;
        res.Focused = ctx.Interaction.FocusedItemId == id;
        res.Hovered = ctx.Interaction.ThisFrame.HoveredItem.Id == id;
        res.Pressed = res.Active && res.Hovered;
        res.Clicked = ctx.Interaction.ClickedItemId == id;
        return res;
    }

    static Font& LoadFontFromMemory(Context& ctx, ArrayView<const uint8> data)
    {
        return FontDecoder(data).Decode(ctx);
    }

    static Texture2D LoadImageFromMemory(Context& ctx, ArrayView<const uint8> data)
    {
        int w, h, nChannels;
        
        uint8* pixels = stbi_load_from_memory(data.data(), (int)data.size(), &w, &h, &nChannels, 4);
        HAX_ASSERT(pixels);
        
        Texture2D texture = ctx.Backend->CreateTexture(pixels, w, h);
        
        stbi_image_free(pixels);
        return texture;
    }

    static void ProcessWndMsgs(Context& ctx)
    {
        Mouse& mouse = ctx.Mouse;

        WndMsg message;
        while (ctx.WndMsgQueue.Read(message))
        {
            uint32 msg = message.Msg;
            uint64 lParam = message.LParam;
            uint64 vk = message.WParam;

            switch (msg)
            {
                case WM_MOUSEMOVE:
                {
                    Vector2 newPos = Vector2((float)GET_X_LPARAM(message.LParam), (float)GET_Y_LPARAM(message.LParam));
                    mouse.DeltaPos += (newPos - mouse.Pos);
                    mouse.Pos = newPos;
                    break;
                }
                case WM_LBUTTONDBLCLK:
                case WM_LBUTTONDOWN:
                    mouse.LmbDown = true;
                    break;
                case WM_LBUTTONUP:
                    mouse.LmbDown = false;
                    break;
                case WM_MOUSEWHEEL:
                    mouse.DeltaWheel.Y = (float)GET_WHEEL_DELTA_WPARAM(message.WParam) / (float)WHEEL_DELTA;
                    break;
                case WM_MOUSEHWHEEL:
                    mouse.DeltaWheel.X = -(float)GET_WHEEL_DELTA_WPARAM(message.WParam) / (float)WHEEL_DELTA;
                    break;
                case WM_KEYDOWN:
                case WM_KEYUP:
                case WM_SYSKEYDOWN:
                case WM_SYSKEYUP:
                {
                    const bool isDown = (message.Msg == WM_KEYDOWN || message.Msg == WM_SYSKEYDOWN);
                    if (vk < 256)
                    {
                        ctx.KeysData[vk].Down = isDown;

                        if (vk == VK_SHIFT)
                        {
                            ctx.KeysData[VK_LSHIFT].Down = (GetKeyState(VK_LSHIFT) & 0x8000) != 0;
                            ctx.KeysData[VK_RSHIFT].Down = (GetKeyState(VK_RSHIFT) & 0x8000) != 0;
                        }
                        else if (vk == VK_CONTROL)
                        {
                            ctx.KeysData[VK_LCONTROL].Down = (GetKeyState(VK_LCONTROL) & 0x8000) != 0;
                            ctx.KeysData[VK_RCONTROL].Down = (GetKeyState(VK_RCONTROL) & 0x8000) != 0;
                        }
                        else if (vk == VK_MENU)
                        {
                            ctx.KeysData[VK_LMENU].Down = (GetKeyState(VK_LMENU) & 0x8000) != 0;
                            ctx.KeysData[VK_RMENU].Down = (GetKeyState(VK_RMENU) & 0x8000) != 0;
                        }
                    }
                }
            }
        }
    }

    static void ResetLayer(Layer& layer)
    {
        layer.RenderItems.Clear();
        layer.RenderBatches.Clear();
        layer.CurrentLayout.Bounds = Rect();
        layer.CurrentLayout.CursorPos = Vector2();
        layer.CurrentContainer.Bounds = layer.CurrentClipRect = Rect{{}, layer.Context->Viewport.Size};
        layer.CurrentRotation = Rotation{.Angle = 0.f, .Sin = 0.f, .Cos = 1.f};
    }

    static void UpdateViewportSize(Viewport& viewport)
    {
        RECT rect = {};
        ::GetClientRect((HWND)viewport.Hwnd, &rect);
        viewport.Size = Vector2((float)(rect.right - rect.left), (float)(rect.bottom - rect.top));
    }

    static void UpdateViewportScale(Viewport& viewport)
    {
        HMONITOR handle = ::MonitorFromWindow((HWND)viewport.Hwnd, MONITOR_DEFAULTTONEAREST);
        MONITORINFO mi;
        mi.cbSize = sizeof(mi);
        if (::GetMonitorInfo(handle, &mi))
        {
            int screenHeight = mi.rcMonitor.bottom - mi.rcMonitor.top;
            viewport.ScaleFactor = screenHeight / SCALE_BASE_HEIGHT;
        }
    }

    static void UpdateWheelScroll(Context& ctx, uint64 id, ContainerState& state, float maxHeight, float maxOffset)
    {
        auto& scroll = state.Scroll;

        if (ctx.Interaction.ThisFrame.HoveredContainer.Id == id && ctx.Mouse.DeltaWheel.Y != 0.f)
        {
            scroll.TargetOffsetY -= ctx.Mouse.DeltaWheel.Y * maxHeight * 0.2f;
            scroll.TargetOffsetY = std::max(scroll.TargetOffsetY, 0.f);
            scroll.TargetOffsetY = std::min(scroll.TargetOffsetY, maxOffset);
        }

        const float stiffness = 15.0f; 

        float delta = scroll.TargetOffsetY - scroll.Offset.Y;

        if (std::abs(delta) < 0.1f) 
        {
            scroll.Offset.Y = scroll.TargetOffsetY;
            return;
        }

        scroll.Offset.Y += delta * (1.0f - expf(-stiffness * ctx.Timer.DeltaTime));
    }

    static bool UpdateMouseIcon()
    {
        MouseIcon icon = g_Context->Mouse.PrevIcon;

        if (icon == MouseIcon_Default)
            return false;

        if (icon == MouseIcon_None)
        {
            ::SetCursor(nullptr);
            return true;
        }

        LPTSTR winIcon = IDC_ARROW;
        switch (icon)
        {
            case MouseIcon_Arrow:        winIcon = IDC_ARROW; break;
            case MouseIcon_TextInput:    winIcon = IDC_IBEAM; break;
            case MouseIcon_ResizeAll:    winIcon = IDC_SIZEALL; break;
            case MouseIcon_ResizeEW:     winIcon = IDC_SIZEWE; break;
            case MouseIcon_ResizeNS:     winIcon = IDC_SIZENS; break;
            case MouseIcon_ResizeNESW:   winIcon = IDC_SIZENESW; break;
            case MouseIcon_ResizeNWSE:   winIcon = IDC_SIZENWSE; break;
            case MouseIcon_Hand:         winIcon = IDC_HAND; break;
            case MouseIcon_Wait:         winIcon = IDC_WAIT; break;
            case MouseIcon_Progress:     winIcon = IDC_APPSTARTING; break;
            case MouseIcon_NotAllowed:   winIcon = IDC_NO; break;
        }
        ::SetCursor(::LoadCursor(nullptr, winIcon));
        return true;
    }

    static void UpdateKeyboardInputs(Context& ctx)
    {
        for (uint32 vk = 0; vk < 256; ++vk)
        {
            float& downDuration = ctx.KeysData[vk].DownDuration;
            ctx.KeysData[vk].DownDurationPrev = downDuration;
            downDuration = ctx.KeysData[vk].Down ? (downDuration < 0.f ? 0.f : (downDuration + GetDeltaTime())) : -1.f;
        }
    }
}