
#include "haxgui.h"

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include "resource.h"

namespace Hax::Gui
{
    struct WindowState
    {
        Vector2 Pos;
        bool FirstFrame = true;
    };

    struct NavItem
    {
        StringView Label;
        void (*PageFn)();
    };

    struct PageSectionState
    {
        float ContentWidth;
        Vector2 ContainerSize;
        bool FirstFrame = true;
    };

    struct ButtonState
    {
        LinearAnim Anim;
    };

    struct ButtonStyle
    {
        ButtonStyle LerpTo(const ButtonStyle& target, float t) const
        {
            ButtonStyle res{};
            res.Bg = Lerp(Bg, target.Bg, t);
            res.Fg = Lerp(Fg, target.Fg, t);
            res.Out = Lerp(Out, target.Out, t);

            return res;
        }

        LinearColor Bg;
        LinearColor Fg;
        LinearColor Out;
    };

    struct ButtonTheme
    {
        ButtonStyle GetMain() const { return ButtonStyle(Bg, Fg, Bg.Darken(0.04f)); }
        ButtonStyle GetOutline() const { return ButtonStyle(LinearColor::White, Bg, Bg); }
        ButtonStyle GetSoft() const { return ButtonStyle(Soft, Bg, Soft.Darken(0.03f)); }
        ButtonStyle GetHovered() const { return ButtonStyle(Bg.Darken(0.05f), Fg, Bg.Darken(0.06f)); }

        LinearColor Bg;
        LinearColor Fg;
        LinearColor Soft;
    };

    namespace Themes
    {
        const ButtonTheme White =    {.Bg = 0xFFFFFFFF, .Fg = LinearColor::Black, .Soft = 0xFFFFFFFF};
        const ButtonTheme Def =      {.Bg = 0xF8F8F8FF, .Fg = LinearColor::Black, .Soft = 0xF8F8F8FF};
        const ButtonTheme Neutral =  {.Bg = 0x1b1718ff, .Fg = LinearColor::White, .Soft = 0xEAEAEAFF};
        const ButtonTheme Prime =    {.Bg = 0x422ad5FF, .Fg = LinearColor::White, .Soft = 0xe0e7ffFF};
        const ButtonTheme Second =   {.Bg = 0xf43098ff, .Fg = LinearColor::White, .Soft = 0xf9e4f0ff};
        const ButtonTheme Accent =   {.Bg = 0x00d3bbff, .Fg = 0x084d49ff, .Soft = 0xf1fcf9ff};
        const ButtonTheme Info =     {.Bg = 0x00bafeff, .Fg = 0x042e49ff, .Soft = 0xf0faffff};
        const ButtonTheme Success =  {.Bg = 0x00d390ff, .Fg = 0x1b1718ff, .Soft = 0xf1fcf6ff};
        const ButtonTheme Warn =     {.Bg = 0xfcb700ff, .Fg = 0x793205ff, .Soft = 0xfffaefff};
        const ButtonTheme Error =    {.Bg = 0xff637dff, .Fg = 0x4d0218ff, .Soft = 0xfff3f4ff};
    }

    struct ButtonParams
    {
        bool        AlignH      = false;
        ButtonStyle DefStyle    = Themes::Def.GetMain();
        float       FontH       = 13_dp;
        ButtonStyle HovStyle    = Themes::Def.GetHovered();
        size_t      Id          = 0;
        Vector2     Padding     = {15_dp, 13_dp};
    };

    struct BadgeParams
    {
        bool AlignH = false;
        float FontH = 13_dp;
        StringView Icon = {};
        Optional<ButtonStyle> Style = {};
    };

    struct ModalState 
    { 
        bool Opened; 
        LinearAnim Anim; 
    };

    struct ModalParams
    {
        bool CloseBtn : 1 = false;
        bool CrossBtn : 1 = false;
        bool ClickClose : 1 = false;
        float Width = 500_dp;
    };

    struct CheckboxParams 
    { 
        ButtonStyle Def; 
        bool Disabled = false; 
        ButtonStyle Hov; 
        StringView Text = {}; 
    };

    enum class Presence
    {
        Off, 
        Offline, 
        Online
    };

    enum class CollapseType
    {
        Focus,
        Checkbox
    };

    enum class CollapseIcon
    {
        None,
        StartArrow,
        EndArrow
    };

    enum class StatusAnimation
    {
        None,
        Pulse,
        Bounce
    };

    static void BeginWindow(size_t id);
    static bool Button(StringView text, const ButtonParams& params = {});
    static bool CrossButton(size_t id);
    static void EndWindow();
    static void Label(Font& font, StringView text, const Color& color, float fontH = 13_dp, bool alignH = false);
    static bool NavMenuBtn(StringView text, bool selected, size_t id = 0);
    static void BeginNavMenuSection(StringView header);
    static void EndNavMenuSection();
    static void BeginPageSection(StringView header);
    static void EndPageSection();
    static bool PageSectionAnchor(StringView text);
    static bool Swap(const Font& font, StringView text1, StringView text2, float fontH, bool first, bool rotate = false);
    static void Image(Texture2D img, float size, float r, Presence presence = Presence::Off);
    static void Badge(StringView text, const BadgeParams& params = {});
    static bool Collapse(size_t id, CollapseType type = CollapseType::Focus, CollapseIcon icon = CollapseIcon::None);
    static float CalcKbdHeight(float rem = 0.875f, float heightMult = 6.f);
    static void VirtualKey(uint8 key, float rem = 0.875f, float heightMult = 6.f);
    static void StatusLabel(StringView text, const Color& color, float mult = 0.5f, StatusAnimation anim = StatusAnimation::None);
    static bool BeginDifView(size_t id, const Vector2& size, float& split);
    static void DifGoUp();
    static void EndDifView();
    static bool ShowModal(StringView name, StringView text, const ModalParams& params = {});
    static void OpenModal(StringView name);
    static void BeginBreadCrumbs();
    static bool BreadCrumb(size_t id, StringView icon, StringView text, bool hoverable);
    static void EndBreadCrumb();
    static bool HyperLink(size_t id, StringView text, bool on, const ButtonTheme& theme);
    static bool Checkbox(size_t id, bool& val, const CheckboxParams& params = {});

    static void EmptyPage() {}
    static void ButtonPage();
    static void DifPage();
    static void ModalPage();
    static void SwapPage();
    static void AvatarPage();
    static void BadgePage();
    static void CollapsePage();
    static void KbdPage();
    static void StatusPage();
    static void BreadCrumbsPage();
    static void LinkPage();
    static void CheckboxPage();

    static Font *g_InterMedium, *g_IconsRegular, *g_IconsSolid, *g_InterRegular, *g_InterSemibold;
    static Texture2D g_SectionBg, g_FlowersImg, g_BluredFlowersImg;
    static Texture2D g_BadPerson, g_SuperPerson, g_YellingWoman, g_YellingCat, g_Gordon, g_IdiotSandwich;

    constexpr ScrollStyle NavMenuScroll = { .TrackWidth = 8.f, .ThumbPadding = 1.f, .TrackCol = 0xF3F4F6FF, .ThumbCol = 0xD1D5DCFF, .ThumbHovCol = 0xD1D5DCFF, .ThumbActiveCol = 0xD1D5DCFF };
    constexpr ScrollStyle NavMenuScroll2 = { .TrackWidth = 8.f, .ThumbPadding = 1.f, .TrackCol = 0xFFFFFFFF, .ThumbCol = 0xD1D5DCFF, .ThumbHovCol = 0xD1D5DCFF, .ThumbActiveCol = 0xD1D5DCFF };

    void ShowDemoWindow()
    {
        static bool s_Inited;
        if (!s_Inited)
        {
            s_Inited = true;

            CreateLayer(L"Modal", 100);

            g_InterMedium   = &Hax::Gui::LoadFontFromMemory(Hax::Gui::GetResourceData((Hax::Handle)::GetModuleHandle(NULL), IDR_FNT1, L"FNT"));
            g_IconsRegular  = &Hax::Gui::LoadFontFromMemory(Hax::Gui::GetResourceData((Hax::Handle)::GetModuleHandle(NULL), IDR_FNT2, L"FNT"));
            g_IconsSolid    = &Hax::Gui::LoadFontFromMemory(Hax::Gui::GetResourceData((Hax::Handle)::GetModuleHandle(NULL), IDR_FNT3, L"FNT"));
            g_InterRegular  = &Hax::Gui::LoadFontFromMemory(Hax::Gui::GetResourceData((Hax::Handle)::GetModuleHandle(NULL), IDR_FNT4, L"FNT"));
            g_InterSemibold = &Hax::Gui::LoadFontFromMemory(Hax::Gui::GetResourceData((Hax::Handle)::GetModuleHandle(NULL), IDR_FNT5, L"FNT"));
        
            g_SectionBg = Hax::Gui::LoadImageFromMemory(Hax::Gui::GetResourceData((Hax::Handle)::GetModuleHandle(NULL), IDB_PNG2, L"PNG"));
            g_FlowersImg = Hax::Gui::LoadImageFromMemory(Hax::Gui::GetResourceData((Hax::Handle)::GetModuleHandle(NULL), IDR_JPG1, L"JPG"));
            g_BluredFlowersImg = Hax::Gui::LoadImageFromMemory(Hax::Gui::GetResourceData((Hax::Handle)::GetModuleHandle(NULL), IDR_JPG2, L"JPG"));
            
            g_BadPerson = Hax::Gui::LoadImageFromMemory(Hax::Gui::GetResourceData((Hax::Handle)::GetModuleHandle(NULL), IDR_JPG3, L"JPG"));
            g_SuperPerson = Hax::Gui::LoadImageFromMemory(Hax::Gui::GetResourceData((Hax::Handle)::GetModuleHandle(NULL), IDR_JPG4, L"JPG"));
            g_YellingWoman = Hax::Gui::LoadImageFromMemory(Hax::Gui::GetResourceData((Hax::Handle)::GetModuleHandle(NULL), IDR_JPG5, L"JPG"));
            g_YellingCat = Hax::Gui::LoadImageFromMemory(Hax::Gui::GetResourceData((Hax::Handle)::GetModuleHandle(NULL), IDR_JPG6, L"JPG"));
            g_Gordon = Hax::Gui::LoadImageFromMemory(Hax::Gui::GetResourceData((Hax::Handle)::GetModuleHandle(NULL), IDR_JPG7, L"JPG"));
            g_IdiotSandwich = Hax::Gui::LoadImageFromMemory(Hax::Gui::GetResourceData((Hax::Handle)::GetModuleHandle(NULL), IDR_JPG8, L"JPG"));
        
        }

        BeginWindow(Hash("DemoWindow"));

        BeginVertical();
        Space(55_dp);
        HorizontalLine(2_dp, 0xF3F4F6FF);
        EndVertical();

        BeginHorizontal();

        Space(30_dp);
        BeginContainer(__LINE__, {.W = 288_dp, .ScrollY = true, .ScrollVisible = true, .Style = NavMenuScroll});
        BeginVertical();
        Space(40_dp);
       
        //struct NavItem { StringView Name; void(*RenderPageFn)() = RenderEmptyPage; }
        static uint32 s_SelectedNavItem;
        static struct { StringView Name; void(*RenderPageFn)() = EmptyPage; } s_NavItems[] =
        {
            { L"Buttons", ButtonPage },
            { L"Modal", ModalPage},
            { L"Swap", SwapPage },

            { L"Diff", DifPage},
            { L"Avatar", AvatarPage},
            { L"Badge", BadgePage },
            { L"Collapse", CollapsePage },
            { L"Kbd", KbdPage },
            { L"Status", StatusPage },

            { L"BreadCrumbs", BreadCrumbsPage },
            { L"Link", LinkPage },

            { L"Checkbox", CheckboxPage}
        };

        BeginNavMenuSection(L"Actions");
        for (uint32 i = 0; i < 3; ++i) 
            if (NavMenuBtn(s_NavItems[i].Name, s_SelectedNavItem == i, __LINE__ + i)) 
                s_SelectedNavItem = i;
        EndNavMenuSection();

        Space(35_dp);

        BeginNavMenuSection(L"Data display");
        for (uint32 i = 3; i < 9; ++i) 
            if (NavMenuBtn(s_NavItems[i].Name, s_SelectedNavItem == i, __LINE__ + i)) 
                s_SelectedNavItem = i;
        EndNavMenuSection();

        Space(35_dp);

        BeginNavMenuSection(L"Navigation");
        for (uint32 i = 9; i < 11; ++i) 
            if (NavMenuBtn(s_NavItems[i].Name, s_SelectedNavItem == i, __LINE__ + i)) 
                s_SelectedNavItem = i;
        EndNavMenuSection();

        Space(35_dp);

        BeginNavMenuSection(L"Data input");
        for (uint32 i = 11; i < _countof(s_NavItems); ++i) 
            if (NavMenuBtn(s_NavItems[i].Name, s_SelectedNavItem == i, __LINE__ + i)) 
                s_SelectedNavItem = i;
        EndNavMenuSection();

        EndVertical();
        EndContainer();

        size_t hash = Hash(s_SelectedNavItem);
        BeginContainer(hash, {.H = -20_dp, .ScrollY = true, .ScrollVisible = true, .Style = NavMenuScroll2});
        BeginHorizontal();
        Space(40_dp);
        BeginContainer(hash + 1, {.W = -60_dp, .FitY = true});
        BeginVertical();
        Space(32_dp);
        s_NavItems[s_SelectedNavItem].RenderPageFn();
        Dummy({0.f, 20_dp});
        EndVertical();
        EndContainer();
        EndHorizontal();
        EndContainer();

        EndHorizontal();

        EndWindow();
    }

    static void BeginWindow(size_t id)
    {
        const Vector2 size = {1340_dp, 820_dp};

        WindowState& state = GetState<WindowState>(id);
        if (state.FirstFrame)
        {
            state.FirstFrame = false;
            state.Pos = ((GetViewportSize()) - size) / 2.f;
        }

        Rect bounds;
        bounds.Min = state.Pos;
        bounds.Max = bounds.Min + size;

        Interact(id, bounds);
        if (IsItemActive(id))
            state.Pos += GetMouseDeltaPos();

        SetCursorPos(bounds.Min);
        BeginContainer(id, {.W = size.X, .H = size.Y, .Clip = true});

        DrawRect(bounds.Min, bounds.Max, {.FillColor = 0xFFFFFFFF, .Rounding = 20_dp});
    }

    static bool Button(StringView text, const ButtonParams& params)
    {
        const Vector2 padding = params.Padding * (params.FontH / 13_dp);
        const Vector2 textSize = CalcTextSize(*g_InterSemibold, text, params.FontH, 1.1f);
        const Vector2 size = textSize + padding * 2.f;

        const float layoutHeight = std::max(GetLayoutBounds().GetSize().Y, size.Y);
        const Vector2 extraSize{0.f, params.AlignH ? (layoutHeight - size.Y) / 2.f : 0.f};

        Rect bounds = Rect::FromPosSize(GetCursorPos() + extraSize, size);

        const size_t id = params.Id == 0 ? Hash(text) : params.Id;
        ButtonState& state = GetState<ButtonState>(id);

        Interact(id, bounds);
        PlaceItem(size + extraSize);

        bool hovered = IsItemHovered(id);
        bool active = IsItemActive(id);

        float animDelta = -GetDeltaTime();
        if (hovered) SetMouseIcon(MouseIcon_Hand);
        if (hovered || active) animDelta = -animDelta;

        state.Anim.Elapse(animDelta, 0.15f);

        if (!IsItemVisible(bounds)) { return false; }

        ButtonStyle style = params.DefStyle;

        if (IsItemActive(id))
        {
            style = params.HovStyle;
            style.Bg = style.Bg.Brighten(0.05f);
            style.Out = style.Bg.Darken(0.02f);

            bounds.Min.Y += 1_dp;
        }
        else
        {
            style = style.LerpTo(params.HovStyle, state.Anim.Progress);
        }

        DrawRect(bounds.Min, bounds.Max, {.FillColor = style.Bg.ToColor(), .Rounding = 5_dp, .BorderTh = 1_dp, .BorderColor = style.Out.ToColor()});
        DrawTexto(*g_InterSemibold, text, bounds.Min + padding, params.FontH, {.Color = style.Fg.ToColor(), .Spacing = 1.1f});

        return IsItemClicked(id);
    }

    static bool CrossButton(size_t id)
    {
        const float dp1 = 1_dp;
        const Vector2 size = {32_dp, 32_dp};
        Rect bounds = Rect::FromPosSize(GetCursorPos(), size);

        Interact(id, bounds);
        PlaceItem(size);

        bool hovered = IsItemHovered(id);
        bool active = IsItemActive(id);

        float animDelta = -GetDeltaTime();
        if (hovered) SetMouseIcon(MouseIcon_Hand);
        if (hovered || active) animDelta = -animDelta;

        ButtonState& state = GetState<ButtonState>(id);
        state.Anim.Elapse(animDelta, 0.15f);

        if (!IsItemVisible(bounds)) { return false; }

        const ButtonStyle style = Themes::Def.GetHovered();
        LinearColor bg = style.Bg;
        LinearColor out = style.Out;
        bg.A = out.A = state.Anim.Progress;

        if (IsItemActive(id))
        {
            bg = bg.Brighten(0.05f);
            out = out.Darken(0.05f);

            bounds.Min.Y += 1_dp;
        }

        Vector2 center = bounds.GetCenter();
        Vector2 crossHalfSize = {5_dp, 5_dp};
        DrawEllipse(center, size / 2.f, {.FillColor = bg.ToColor(), .BorderTh = dp1, .BorderColor = out.ToColor()});
        DrawLine(center - crossHalfSize, center + crossHalfSize, {.Th = dp1, .FillColor = Color::Black});

        crossHalfSize.X = -crossHalfSize.X;
        DrawLine(center + crossHalfSize, center - crossHalfSize, {.Th = dp1, .FillColor = Color::Black});

        return IsItemClicked(id);
    }

    static void EndWindow()
    {
        EndContainer();
    }

    static void Label(Font& font, StringView text, const Color& color, float fontH, bool alignH)
    {
        const Vector2 size = CalcTextSize(font, text, fontH, 1.1f);

        const float layoutHeight = std::max(GetLayoutBounds().GetSize().Y, size.Y);
        const Vector2 extraSize = {0.f, alignH ? (layoutHeight - size.Y) / 2.f : 0.f};

        Rect bounds = Rect::FromPosSize(GetCursorPos() + extraSize, size);

        PlaceItem(bounds.GetSize() + extraSize);
        if (IsItemVisible(bounds))
            DrawTexto(font, text, bounds.Min, fontH, {.Color = color, .Spacing = 1.1f});
    }

    static bool NavMenuBtn(StringView text, bool selected, size_t id)
    {
        const float fontH = 13_dp;
        const float spacing = 1.1f;
        const Vector2 textSize = CalcTextSize(*g_InterRegular, text, fontH, spacing);

        const Vector2 padding = {20_dp, 5_dp};
        const Vector2 size = textSize + padding * 2.f;

        Rect bounds;
        bounds.Min = GetCursorPos();
        bounds.Max = bounds.Min + size;

        PlaceItem(size);
        if (!IsItemVisible(bounds))
            return false;

        if (id == 0)
            id = Hash(text);
        Interact(id, bounds);

        bool hovered = IsItemHovered(id);
        if (hovered && !selected)
            SetMouseIcon(MouseIcon_Hand);

        if (selected || hovered)
        {
            const float th = 2_dp;
            const Color cl = selected ? 0x155DFCFF : 0x99A1AFFF;
            DrawRect(bounds.Min - Vector2(th, 0.f), bounds.GetBL(), {.FillColor = cl});
        }

        const Color cl = selected ? Color::Black : 0x364153FF;
        DrawTexto(*g_InterRegular, text, bounds.Min + padding, fontH, {.Color = cl, .Spacing = spacing});

        return IsItemClicked(id);
    }

    static void BeginNavMenuSection(StringView header)
    {
        Label(*g_InterSemibold, header, Color::Black);
        Space(15_dp);

        BeginContainer(Hash(header), {.FitY = true});
        BeginHorizontal();
        VerticalLine(2_dp, 0xF3F4F6FF);
        BeginVertical(8_dp);
    }

    static void EndNavMenuSection()
    {
        EndVertical();
        EndHorizontal();
        EndContainer();
    }

    static PageSectionState* g_CurrentPageSectionState;
    static void BeginPageSection(StringView header)
    {
        float anchorPosY = GetCursorPos().Y;
        if (PageSectionAnchor(header)) 
            ScrollYTo(anchorPosY - 32_dp);
        Space(15_dp);

        size_t id = Hash(header);

        PageSectionState& state = GetState<PageSectionState>(id);
        g_CurrentPageSectionState = &state;
        
        if (!state.FirstFrame)
        {
            Rect bounds = Rect::FromPosSize(GetCursorPos(), state.ContainerSize);
            if (IsItemVisible(bounds))
            {
                float uvY = (std::min)(bounds.GetSize().Y / g_SectionBg.Height, 1.f);
                DrawImage(g_SectionBg, bounds.Min, bounds.Max, {.UVmax = {1.f, uvY}});
                DrawRect(bounds.Min, bounds.Max, {.FillColor = 0xE8E8E800, .Rounding = 10_dp, .BorderTh = 0.5_dp, .BorderColor = 0xE8E8E8FF});
            }
        }
        else
            BeginSkip();

        BeginContainer(id, {.FitY = true});
        float padding = (std::max)(0.f, (GetContentRegionAvail().X - state.ContentWidth) / 2.f);
        BeginVertical();
        Dummy({30_dp, 30_dp});
        BeginHorizontal();
        Space(padding);
        BeginHorizontal(8_dp);
    }

    static void EndPageSection()
    {
        HAX_ASSERT(g_CurrentPageSectionState != nullptr);
        PageSectionState& state = *g_CurrentPageSectionState;
        
        state.ContentWidth = GetLayoutBounds().GetSize().X;
        if (state.FirstFrame)
        {
            state.FirstFrame = false;
            EndSkip();
        }

        EndHorizontal();
        EndHorizontal();
        Dummy({30_dp, 30_dp});
        EndVertical();

        state.ContainerSize = GetContainerBounds().GetSize();
        EndContainer();
    }

    static bool PageSectionAnchor(StringView text)
    {
        const size_t id = Hash(text);
        const float dp13 = 13_dp;
        const float dp16 = 16_dp;

        // Anchor btn
        const Vector2 hashtagSize = CalcTextSize(*g_InterRegular, L"#", dp13);
        const Vector2 padding = {10_dp, 5_dp};
        const Vector2 btnSize = hashtagSize + padding * 2.f;
        const Rect btnBounds = Rect::FromPosSize(GetCursorPos(), btnSize);
        Interact(id, btnBounds);

        // Text
        const Vector2 textSize = CalcTextSize(*g_InterSemibold, text, dp16, 1.1f);
        const float textOffsetY = (btnBounds.GetSize().Y - textSize.Y) / 2.f;
        const Vector2 textPos = {btnBounds.Max.X + 8_dp, btnBounds.Min.Y + textOffsetY};
        const Rect textBounds = Rect::FromPosSize(textPos, textSize);

        const Rect totalBounds = Rect(btnBounds.Min, Vector2(textBounds.Max.X, btnBounds.Max.Y));

        PlaceItem(totalBounds.GetSize());
        if (!IsItemVisible(totalBounds))
            return false;

        const bool hovered = IsItemHovered(id);
        const bool active = IsItemActive(id);

        Color btnColor = Color::White;
        Color fgColor = 0x18181b80;
        if (hovered || active)
        {
            btnColor = 0x422ad51a;
            fgColor = Color::Black;
        }

        DrawRect(btnBounds.Min, btnBounds.Max, {.FillColor = btnColor, .Rounding = 5_dp, .BorderTh = 1_dp, .BorderColor = 0xE8E8E8FF});
        DrawTexto(*g_InterRegular, L"#", btnBounds.Min + padding, dp13, {.Color = fgColor});
        DrawTexto(*g_InterSemibold, text, textPos, dp16, {.Spacing = 1.1f});

        if (hovered) SetMouseIcon(MouseIcon_Hand);
        return IsItemClicked(id);
    }

    struct SwapState { LinearAnim Anim; };
    static bool Swap(const Font& font, StringView text1, StringView text2, float fontH, bool first, bool rotate)
    {
        const Vector2 size1 = CalcTextSize(font, text1, fontH);
        const Vector2 size2 = CalcTextSize(font, text2, fontH);

        const Vector2 totalSize = Max(size1, size2);
        const Rect bounds = Rect::FromPosSize(GetCursorPos(), totalSize);

        PlaceItem(totalSize);
        if (!IsItemVisible(bounds))
            return false;

        const size_t id = Hash(text1) + Hash(text2);
        Interact(id, bounds);
        if (IsItemHovered(id))
            SetMouseIcon(MouseIcon_Hand);

        LinearAnim& anim = GetState<SwapState>(id).Anim;
        float deltaTime = GetDeltaTime();
        anim.Elapse(first ? -deltaTime : deltaTime, 0.1f);

        if (rotate)
            BeginRotation(DegToRad(std::lerp(0.f, -90.f, anim.Progress)));

        LinearColor firstColor = LinearColor::Black;
        firstColor.A = 1 - anim.Progress;
        DrawTexto(font, text1, bounds.Min, fontH, {.Color = firstColor.ToColor()});

        firstColor.A = anim.Progress;
        DrawTexto(font, text2, bounds.Min, fontH, {.Color = firstColor.ToColor()});

        if (rotate)
            EndRotation();

        return IsItemClicked(id);
    }

    static void Image(Texture2D img, float size, float r, Presence presence)
    {
        const Vector2 size2 = {size, size};
        const Rect bounds = Rect::FromPosSize(GetCursorPos(), size2);

        PlaceItem(size2);
        if (!IsItemVisible(bounds))
            return;

        DrawImage(img, bounds.Min, bounds.Max, {.R = r});

        if (presence != Presence::Off)
        {
            Color statusColor = presence == Presence::Offline ? 0xEEEEEEFF : 0x00d390ff;
            Vector2 position = bounds.GetTR() + Vector2(-r, r) * 0.33f;
            
            r = 8_dp;
            DrawCircle(position, r + 2_dp, {.FillColor = Color::White});
            DrawCircle(position, r, {.FillColor = statusColor});
        }
    }

    static void Badge(StringView text, const BadgeParams& params)
    {
        const Vector2 textSize = CalcTextSize(*g_InterRegular, text, params.FontH);
        const Vector2 padding = {params.FontH - 2_dp, 4_dp};

        Vector2 iconSize{};
        Vector2 iconSizeWithPadding{};
        if (!params.Icon.Empty())
        {
            iconSize = iconSizeWithPadding = CalcTextSize(*g_IconsRegular, params.Icon, params.FontH);
            iconSizeWithPadding.X += padding.X * 0.7f;
        }
        
        const Vector2 size = textSize + padding * 2.f + Vector2(iconSizeWithPadding.X, 0.f);
        const float layoutHeight = std::max(GetLayoutBounds().GetSize().Y, size.Y);
        const Vector2 extraSize = {0.f, params.AlignH ? (layoutHeight - size.Y) / 2.f : 0.f};

        const Rect bounds = Rect::FromPosSize(GetCursorPos() + extraSize, size);
        PlaceItem(size + extraSize);
        
        if (IsItemVisible(bounds))
        {
            ButtonStyle style{.Bg = 0xFFFFFFFF,.Fg = 0x1b1718ff,.Out = 0xf8f8f8ff };
            if (params.Style.HasValue())
                style = params.Style.Value();

            DrawRect(bounds.Min, bounds.Max, { .FillColor = style.Bg.ToColor(), .Rounding = 7_dp, .BorderTh = 1_dp, .BorderColor = style.Out.ToColor()});
            
            Color color = style.Fg.ToColor();
            Vector2 pos = bounds.Min + padding;
            if (!params.Icon.Empty())
            {
                float align = (textSize.Y - iconSize.Y) / 2.f;;
                pos.Y += align;
                DrawTexto(*g_IconsRegular, params.Icon, pos, params.FontH, {.Color = color});
                pos.X += iconSizeWithPadding.X;
                pos.Y -= align;
            }
            DrawTexto(*g_InterRegular, text, pos, params.FontH, {.Color = color});
        }
    }

    struct CollapseState { LinearAnim Anim; bool Open; };
    static bool Collapse(size_t id, CollapseType type, CollapseIcon icon)
    {
        CollapseState& state = GetState<CollapseState>(id);

        const float minHeight = 60_dp;
        const float maxHeight = 100_dp;
        const float height = std::lerp(minHeight, maxHeight, state.Anim.Progress);

        const Vector2 size = {GetContainerBounds().GetSize().X - 20_dp, height};
        const Rect bounds = Rect::FromPosSize(GetCursorPos(), size);

        PlaceItem(size);
        Interact(id, bounds);
        const bool hovered = IsItemHovered(id);

        const bool wasOpen = state.Open;

        if (type == CollapseType::Focus)
        {
            state.Open = IsItemFocused(id);

            if (!state.Open && hovered)
                SetMouseIcon(MouseIcon_Hand);
        }

        if (type == CollapseType::Checkbox)
        {
            if (hovered)
                SetMouseIcon(MouseIcon_Hand);

            if (IsItemClicked(id))
                state.Open = !state.Open;
        }

        const float deltaTime = GetDeltaTime();
        state.Anim.Elapse(state.Open ? deltaTime : -deltaTime, 0.2f);

        if (IsItemVisible(bounds))
        {
            PushClipRect(bounds);

            DrawRect(bounds.Min, bounds.Max, {.FillColor = Color::White, .Rounding = 5_dp, .BorderTh = 1_dp, .BorderColor = 0xE8E8E8FF});

            const float dp15 = 15_dp;
            const float headerHeight = GetFontLineHeight(*g_InterRegular, dp15);
            Vector2 headerPadding = {20_dp, (minHeight - headerHeight) / 2.f };

            if (icon == CollapseIcon::StartArrow)
            {
                BeginRotation(DegToRad(std::lerp(0.f, -180.f, state.Anim.Progress)));
                DrawTexto(*g_IconsRegular, L"\uf107", bounds.Min + Vector2(headerPadding.X, headerPadding.Y + 1_dp), dp15);
                headerPadding.X *= 2.5f;
                EndRotation();
            }

            if (icon == CollapseIcon::EndArrow)
            {
                const Vector2 pos = {bounds.Max.X - 40_dp, bounds.Min.Y + headerPadding.Y + 1_dp};
                BeginRotation(DegToRad(std::lerp(0.f, -180.f, state.Anim.Progress)));
                DrawTexto(*g_IconsRegular, L"\uf107", pos, dp15);
                EndRotation();
            }

            Vector2 textPos = bounds.Min + headerPadding;
            DrawTexto(*g_InterRegular, L"How do I create an account?", textPos, dp15);

            textPos.Y = bounds.Min.Y + maxHeight * 0.64f;
            DrawTexto(*g_InterRegular, L"Click the \"Sign Up\" button in the top right corner and follow the registration process", textPos, 13_dp);

            PopClipRect();
        }

        return wasOpen != state.Open;
    }

    static float CalcKbdHeight(float rem, float heightMult)
    {
        return 16_dp * 0.25f * heightMult;
    }

    static void VirtualKey(uint8 key, float rem, float heightMult)
    {
        const float dp16 = 16_dp;
        const float fontH = 14_dp * rem;
        const float minSize = CalcKbdHeight(rem, heightMult);

        StringView keyName = GetKeyName(key);
        const Vector2 textSize = CalcTextSize(*g_InterRegular, keyName, fontH, 1.08f);

        const float width = std::max(minSize, textSize.X + minSize * 0.5f);
        const float height = minSize;
        const Vector2 size = {width, height};

        Rect bounds = Rect::FromPosSize(GetCursorPos(), size);

        PlaceItem(size);
        if (IsItemVisible(bounds))
        {
            const Vector2 padding = (size - textSize) / 2.f;

            const float dp15 = 2_dp;

            bounds.Min.Y += dp15;
            bounds.Max.Y += dp15;

            if (IsKeyUp(key))
            {
                DrawRect(bounds.Min, bounds.Max, { .FillColor = 0xCBCBCBFF, .Rounding = 3_dp });

                bounds.Min.Y -= dp15;
                bounds.Max.Y -= dp15;
            }

            DrawRect(bounds.Min, bounds.Max, {.FillColor = 0xf8f8f8ff, .Rounding = 3_dp, .BorderTh = 0.5_dp, .BorderColor = 0xCBCBCBFF});
            DrawTexto(*g_InterRegular, keyName, bounds.Min + padding, fontH, {.Spacing = 1.08f});
        }
    }

    static void StatusLabel(StringView text, const Color& color, float mult, StatusAnimation anim)
    {
        const float rem = 14_dp;
        const float r = rem * mult * 0.6f;
        
        Vector2 size = {rem * mult, rem};

        if (!text.Empty())
        {
            Vector2 textSize = CalcTextSize(*g_InterRegular, text, rem);
            size.X += 7_dp + textSize.X;
            size.Y = textSize.Y;
        }

        const Rect bounds = Rect::FromPosSize(GetCursorPos(), size);
        PlaceItem(size);
        if (!IsItemVisible(bounds))
            return;

        const float centerLine = bounds.GetCenter().Y;
        Vector2 circlePos = { bounds.Min.X + r, centerLine };

        if (anim == StatusAnimation::Pulse)
        {
            float t = fmodf((float)GetTime(), 1.0f);
            float progress = 1.0f - powf(1.0f - t, 4.0f);

            float finalScale = 1.0f + progress;       // Масштаб: 1.0 -> 2.0
            float finalAlpha = 1.0f - progress;       // Прозрачность: 1.0 -> 0.0

            float currentRadius = r * finalScale;
            uint8_t alphaByte = (uint8_t)(finalAlpha * 255);
            Color pulseColor = Color(color.R, color.G, color.B, alphaByte);

            DrawCircle(circlePos, currentRadius, { .FillColor = pulseColor, .BorderColor = pulseColor });
            DrawCircle(circlePos, r, {.FillColor = color});
        }
        else if (anim == StatusAnimation::Bounce)
        {
            Vector2 pos = circlePos;

            float duration = 1.0f;
            float jumpHeight = r * 0.5f; 
            float t = fmodf((float)GetTime(), duration) / duration;
            float offset;

            if (t < 0.5f) {
                float phase = t / 0.5f;
                offset = phase * phase * phase; // от 0 до 1 (быстро растет к концу)
            } else {
                float phase = (t - 0.5f) / 0.5f;
                offset = 1.0f - powf(phase, 0.33f); // возвращаемся к 0 (замедляемся к концу)
            }

            pos.Y += (jumpHeight * offset);

            DrawCircle(pos, r, {.FillColor = color});
        }
        else
            DrawCircle(circlePos, r, {.FillColor = color});

        if (!text.Empty())
            DrawTexto(*g_InterRegular, text, {bounds.Min.X + r + r + 7_dp, bounds.Min.Y}, rem);
    }

    static struct { size_t Id; Rect Bounds; float* Split; } g_CurrentDifView;
    static bool BeginDifView(size_t id, const Vector2& size, float& split)
    {
        HAX_ASSERT(g_CurrentDifView.Id == 0);

        BeginContainer(id, {.W = size.X, .H = size.Y});

        g_CurrentDifView.Id = id;
        g_CurrentDifView.Bounds = GetContainerBounds();
        g_CurrentDifView.Split = &split;

        if (!IsItemVisible(g_CurrentDifView.Bounds))
        {
            EndContainer();
            g_CurrentDifView.Id = 0;
            return false;
        }

        return true;
    }

    static void DifGoUp()
    {
        HAX_ASSERT(g_CurrentDifView.Id != 0);

        ResetCursor();
        *g_CurrentDifView.Split = std::max(0.02f, std::min(*g_CurrentDifView.Split, 0.98f));

        Vector2 size = g_CurrentDifView.Bounds.GetSize();
        size.X *= *g_CurrentDifView.Split;

        const Rect clipRect = Rect::FromPosSize(g_CurrentDifView.Bounds.Min, size);
        PushClipRect(clipRect);
    }

    static void EndDifView()
    {
        HAX_ASSERT(g_CurrentDifView.Id != 0);

        PopClipRect();

        const float dp1 = 1_dp;
        float& split = *g_CurrentDifView.Split;
        const Rect bounds = GetContainerBounds();
        const Vector2 size = bounds.GetSize();

        Vector2 center = bounds.GetCenter();
        center.X = bounds.Min.X + size.X * split;

        Rect splitLineBounds;
        splitLineBounds.Min = {center.X - dp1, bounds.Min.Y};
        splitLineBounds.Max = {center.X + dp1, bounds.Max.Y};
        DrawRect(splitLineBounds.Min, splitLineBounds.Max, {.FillColor = Color::White});

        Vector2 r = {8_dp, 13_dp};
        Rect thumbBounds = Rect(center - r, center + r);
        Interact(g_CurrentDifView.Id, thumbBounds);
        DrawRect(thumbBounds.Min, thumbBounds.Max, {.FillColor = Color::White, .Rounding = 5_dp});

        bool active = IsItemActive(g_CurrentDifView.Id);
        bool hovered = IsItemHovered(g_CurrentDifView.Id);

        if (hovered || active)
        {
            SetMouseIcon(MouseIcon_ResizeEW);

            if (active)
            {
                float delta = GetMouseDeltaPos().X / size.X;
                split = std::max(0.02f, std::min(split + delta, 0.98f));
            }
        }

        EndContainer();

        g_CurrentDifView.Id = 0;
    }

    static bool ShowModal(StringView name, StringView text, const ModalParams& params)
    {
        const size_t id = Hash(name);

        ModalState& state = GetState<ModalState>(id);
        if (!state.Opened && state.Anim.Progress == 0.f)
            return false;

        SwitchLayer(L"Modal");

        // Anim

        const float deltaTime = GetDeltaTime();
        state.Anim.Elapse(state.Opened ? deltaTime : -deltaTime, 0.1f);

        // Bg

        LinearColor bg = LinearColor::Black;
        bg.A = state.Anim.Progress * 0.4f;

        const Rect screenBounds = GetContainerBounds();
        DrawRect(screenBounds.Min, screenBounds.Max, {.FillColor = bg.ToColor()});

        const Vector2 modalSize = {params.Width, params.CloseBtn ? 200_dp : 130_dp};
        const Rect modalBounds = Rect::FromPosSize(screenBounds.GetCenter() - modalSize / 2.f, modalSize);
        DrawRect(modalBounds.Min, modalBounds.Max, {.FillColor = Color::White, .Rounding = 10_dp});

        // Text

        LinearColor fg = LinearColor::Black;
        fg.A = state.Anim.Progress;
        DrawTexto(*g_InterSemibold, L"Hello", modalBounds.Min + Vector2(25_dp, 30_dp), 17_dp, {.Color = fg.ToColor()});
        DrawTexto(*g_InterRegular, text, modalBounds.Min + Vector2(25_dp, 75_dp), 14_dp, {.Color = fg.ToColor()});

        if (state.Opened)
        {
            Interact(id, screenBounds);
            Interact(id + 1, modalBounds);

            if (params.ClickClose)
            {
                if (IsItemClicked(id)) state.Opened = false;
                if (IsItemHovered(id)) SetMouseIcon(MouseIcon_Hand);
            }
        }

        if (params.CloseBtn)
        {
            const Vector2 textSize = CalcTextSize(*g_InterRegular, L"Close", 13_dp);
            const Vector2 padding = {15_dp, 10_dp};
            const Vector2 btnSize = textSize + padding * 2.f;
            const Rect btnBounds = Rect::FromPosSize(modalBounds.Max - Vector2(25_dp, 25_dp) - btnSize, btnSize);

            SetCursorPos(btnBounds.Min);
            
            bool clicked = Button(L"Close", {.Id = id + 2});
            if (clicked)
                state.Opened = false;
        }

        if (params.CrossBtn)
        {
            const float dp10 = 10_dp;

            const Vector2 padding = {dp10, dp10};
            const Vector2 size = {32_dp, 32_dp};
            const Vector2 pos = {modalBounds.Max.X - padding.X - size.X, modalBounds.Min.Y + padding.Y};

            SetCursorPos(pos);
            if (CrossButton(id + 3))
                state.Opened = false;
        }

        RestoreLayer();

        return true;
    }

    static void OpenModal(StringView name)
    {
        const size_t id = Hash(name);

        ModalState& state = GetState<ModalState>(id);
        state.Opened = true;
    }

    static int s_nCrumbs = -1;
    static void BeginBreadCrumbs()
    {
        HAX_ASSERT(s_nCrumbs < 0);

        s_nCrumbs = 0;
        BeginHorizontal(10_dp);
    }

    static bool BreadCrumb(size_t id, StringView icon, StringView text, bool hoverable)
    {
        HAX_ASSERT(s_nCrumbs >= 0);

        if (s_nCrumbs > 0)
        {
            const Vector2 size = {5_dp, 8_dp};
            float x = GetCursorPos().X;
            float y = GetLayoutBounds().GetCenter().Y;

            const Vector2 a = {x, y - size.Y / 2.f};
            const Vector2 b = {x + size.X, y + 0.5_dp};
            const Vector2 c = {x + size.X, y - 0.5_dp};
            const Vector2 d = {x, y + size.Y / 2.f};

            DrawLine(a, b, {.FillColor = 0xC1C1C2FF});
            DrawLine(c, d, {.FillColor = 0xC1C1C2FF});

            Dummy(size);
        }

        const Vector2 textSize = CalcTextSize(*g_InterRegular, text, 13_dp);
        Vector2 extraSize = {};
        Vector2 iconSize = {};
        if (!icon.Empty())
        {
            iconSize = CalcTextSize(*g_IconsRegular, icon, 12_dp);
            extraSize = {iconSize.X + 10_dp, 0.f};
        }

        Rect bounds = Rect::FromPosSize(GetCursorPos(), textSize + extraSize);

        bool clicked = false;

        PlaceItem(bounds.GetSize());
        if (IsItemVisible(bounds))
        {
            if (!icon.Empty())
            {
                Vector2 pos = bounds.Min;
                pos.Y += (textSize.Y - iconSize.Y) / 2.f;
                DrawTexto(*g_IconsRegular, icon, pos, 12_dp);
            }

            DrawTexto(*g_InterRegular, text, bounds.Min + extraSize, 13_dp);

            if (hoverable)
            {
                Interact(id, bounds);
                clicked = IsItemClicked(id);
                if (IsItemHovered(id))
                {
                    bounds.Min.Y = bounds.Max.Y += 1_dp;
                    DrawLine(bounds.Min, bounds.GetTR(), {.Th = 1_dp});
                    SetMouseIcon(MouseIcon_Hand);
                }
            }
        }

        ++s_nCrumbs;
        return clicked;
    }

    static void EndBreadCrumb()
    {
        HAX_ASSERT(s_nCrumbs >= 0);

        EndHorizontal();
        s_nCrumbs = -1;
    }

    static bool HyperLink(size_t id, StringView text, bool on, const ButtonTheme& theme)
    {
        Vector2 textSize = CalcTextSize(*g_InterRegular, text, 14_dp, 1.1f);
        const Rect bounds = Rect::FromPosSize(GetCursorPos(), textSize);

        PlaceItem(textSize);
        if (IsItemVisible(bounds))
        {
            Interact(id, bounds);

            LinearColor col = theme.GetMain().Bg;
            bool hovered = IsItemHovered(id);
            if (IsItemHovered(id))
            {
                SetMouseIcon(MouseIcon_Hand);
                col = col.Darken(0.3f);
            }

            DrawTexto(*g_InterRegular, text, bounds.Min, 14_dp, {.Color = col.ToColor(), .Spacing = 1.1f});

            if (hovered || on)
                DrawLine(bounds.GetBL(), bounds.Max, {.Th = 1_dp, .FillColor = col.ToColor()});
        }

        return IsItemClicked(id);
    }

    struct CheckboxState { LinearAnim AlphaAnim; LinearAnim CrossAnim; };
    static bool Checkbox(size_t id, bool& val, const CheckboxParams& params)
    {
        const Vector2 boxSize = {24_dp, 24_dp};
        Vector2 totalSize = boxSize;
        Vector2 textOffset = {};

        StringView text = params.Text;

        const float fontH = 12_dp;

        if (!text.Empty())
        {
            const Vector2 textSize = CalcTextSize(*g_InterRegular, text, fontH, 1.1f);

            textOffset.X = boxSize.X + 10_dp;
            textOffset.Y = (boxSize.Y - textSize.Y) / 2.f;

            totalSize.X = textOffset.X + textSize.X;
        }

        const Rect bounds = Rect::FromPosSize(GetCursorPos(), totalSize);
        PlaceItem(totalSize);
        if (!IsItemVisible(bounds))
            return false;

        Interact(id, bounds);
        bool clicked = IsItemClicked(id) && !params.Disabled;
        bool hovered = IsItemHovered(id);

        if (clicked) val = !val;
        if (hovered) SetMouseIcon(params.Disabled ? MouseIcon_NotAllowed : MouseIcon_Hand);

        CheckboxState& state = GetState<CheckboxState>(id);
        if (params.Disabled)
        {
            state.AlphaAnim.Progress = state.CrossAnim.Progress = val ? 1.f : 0.f;
        }

        float deltaTime = GetDeltaTime();
        if (!val) deltaTime = -deltaTime;

        state.AlphaAnim.Elapse(deltaTime, 0.1f);
        state.CrossAnim.Elapse(deltaTime, 0.15f);
        float t = state.AlphaAnim.Progress;

        ButtonStyle style = params.Def.LerpTo(params.Hov, t);
        if (params.Disabled)
        {
            style.Bg.A = style.Fg.A = style.Out.A = 0.2f;
        }
        DrawRect(bounds.Min, bounds.Min + boxSize, {.FillColor = style.Bg.ToColor(), .Rounding = 5_dp, .BorderTh = 1_dp, .BorderColor = style.Out.ToColor()});

        const float th = 2.5_dp;
        float offset = th * 0.3536f;

        Vector2 a = {bounds.Min.X + boxSize.X * 0.22f, bounds.Min.Y + boxSize.Y * 0.51f};
        Vector2 cross = a; cross.X += 4_dp; cross.Y += 4_dp;
        Vector2 b = cross; b.X += offset; b.Y += offset;
        Vector2 c = cross; c.X -= offset; c.Y += offset;
        float off = val ? Lerp(0_dp, 9_dp, state.CrossAnim.Progress) : 9_dp;
        Vector2 d = cross; d.X += off; d.Y -= off;

        if (t > 0.f)
        {
            LinearColor lColor = params.Hov.Fg;
            lColor.A = t;
            Color color = lColor.ToColor();
            if (params.Disabled)
                color.A = 0x33;
            
            DrawLine(a, b, {.Th = th, .FillColor = color});
            DrawLine(c, d, {.Th = th, .FillColor = color});
        }

        if (!params.Text.Empty())
        {
            DrawTexto(*g_InterRegular, params.Text, bounds.Min + textOffset, fontH, {.Color = style.Fg.ToColor(), .Spacing = 1.1f});
        }

        return clicked;
    }

    static void ButtonPage()
    {
        BeginPageSection(L"Button");
        Button(L"Default", {.Id = HAX_LINE});
        EndPageSection();

        Space(55_dp);

        BeginPageSection(L"Button sizes");
        Dummy({0.f, 60_dp});
        Space(-GetLayoutSpacing());
        Button(L"Xsmall",   {.AlignH = true, .FontH = 11_dp, .Id = HAX_LINE});
        Button(L"Small",    {.AlignH = true, .FontH = 12_dp, .Id = HAX_LINE});
        Button(L"Medium",   {.AlignH = true, .FontH = 13_dp, .Id = HAX_LINE});
        Button(L"Large",    {.AlignH = true, .FontH = 15_dp, .Id = HAX_LINE});
        Button(L"Xlarge",   {.AlignH = true, .FontH = 18_dp, .Id = HAX_LINE});
        EndPageSection();

        Space(55_dp);

        BeginPageSection(L"Button colors");
        Button(L"Default", {.Id = HAX_LINE});
        Button(L"Primary",  {.DefStyle = Themes::Prime.GetMain(),  .HovStyle = Themes::Prime.GetHovered(), .Id = HAX_LINE});
        Button(L"Secondary",{.DefStyle = Themes::Second.GetMain(), .HovStyle = Themes::Second.GetHovered(), .Id = HAX_LINE});
        Button(L"Accent",   {.DefStyle = Themes::Accent.GetMain(), .HovStyle = Themes::Accent.GetHovered(), .Id = HAX_LINE});
        Button(L"Info",     {.DefStyle = Themes::Info.GetMain(),   .HovStyle = Themes::Info.GetHovered(), .Id = HAX_LINE});
        Button(L"Success",  {.DefStyle = Themes::Success.GetMain(),.HovStyle = Themes::Success.GetHovered(), .Id = HAX_LINE});
        Button(L"Warning",  {.DefStyle = Themes::Warn.GetMain(),   .HovStyle = Themes::Warn.GetHovered(), .Id = HAX_LINE});
        Button(L"Error",    {.DefStyle = Themes::Error.GetMain(),  .HovStyle = Themes::Error.GetHovered(), .Id = HAX_LINE});
        EndPageSection();

        Space(55_dp);

        BeginPageSection(L"Soft buttons");
        Button(L"Default", {.Id = HAX_LINE});
        Button(L"Primary",  {.DefStyle = Themes::Prime.GetSoft(), .HovStyle = Themes::Prime.GetHovered(), .Id = HAX_LINE});
        Button(L"Secondary",{.DefStyle = Themes::Second.GetSoft(), .HovStyle = Themes::Second.GetHovered(), .Id = HAX_LINE});
        Button(L"Accent",   {.DefStyle = Themes::Accent.GetSoft(), .HovStyle = Themes::Accent.GetHovered(), .Id = HAX_LINE});
        Button(L"Info",     {.DefStyle = Themes::Info.GetSoft(), .HovStyle = Themes::Info.GetHovered(), .Id = HAX_LINE});
        Button(L"Success",  {.DefStyle = Themes::Success.GetSoft(), .HovStyle = Themes::Success.GetHovered(), .Id = HAX_LINE});
        Button(L"Warning",  {.DefStyle = Themes::Warn.GetSoft(), .HovStyle = Themes::Warn.GetHovered(), .Id = HAX_LINE});
        Button(L"Error",    {.DefStyle = Themes::Error.GetSoft(), .HovStyle = Themes::Error.GetHovered(), .Id = HAX_LINE});
        EndPageSection();

        Space(55_dp);

        BeginPageSection(L"Outline buttons");
        Button(L"Default", {.DefStyle = Themes::Neutral.GetOutline(), .Id = HAX_LINE});
        Button(L"Primary", {.DefStyle = Themes::Prime.GetOutline(), .HovStyle = Themes::Prime.GetHovered(), .Id = HAX_LINE});
        Button(L"Secondary", {.DefStyle = Themes::Second.GetOutline(), .HovStyle = Themes::Second.GetHovered(), .Id = HAX_LINE});
        Button(L"Accent", {.DefStyle = Themes::Accent.GetOutline(), .HovStyle = Themes::Accent.GetHovered(), .Id = HAX_LINE});
        Button(L"Info", {.DefStyle = Themes::Info.GetOutline(), .HovStyle = Themes::Info.GetHovered(), .Id = HAX_LINE});
        Button(L"Success", {.DefStyle = Themes::Success.GetOutline(), .HovStyle = Themes::Success.GetHovered(), .Id = HAX_LINE});
        Button(L"Warning", {.DefStyle = Themes::Warn.GetOutline(), .HovStyle = Themes::Warn.GetHovered(), .Id = HAX_LINE});
        Button(L"Error", {.DefStyle = Themes::Error.GetOutline(), .HovStyle = Themes::Error.GetHovered(), .Id = HAX_LINE});
        EndPageSection();

        Space(55_dp);

        BeginPageSection(L"Wide button");
        Button(L"Wide", {.Padding = {105_dp, 11_dp}});
        EndPageSection();
    }

    static void DifPage()
    {
        BeginPageSection(L"Diff");
        {
            static float s_Split = 0.2f;
            if (BeginDifView(__LINE__, { GetContainerBounds().GetSize().X - 50_dp, 430_dp }, s_Split))
            {
                Rect bounds = GetContainerBounds();
                Vector2 size = bounds.GetSize();

                DrawImage(g_BluredFlowersImg, bounds.Min, bounds.Max, {.UVmax = {1.f, size.Y / g_BluredFlowersImg.Height}});

                DifGoUp();

                DrawImage(g_FlowersImg, bounds.Min, bounds.Max, {.UVmax = {1.f, size.Y / g_FlowersImg.Height}});

                EndDifView();
            }
        }
        EndPageSection();

        Space(55_dp);

        BeginPageSection(L"Diff text");
        {
            static float s_Split = 0.5f;
            if (BeginDifView(__LINE__, { GetContainerBounds().GetSize().X - 50_dp, 430_dp }, s_Split))
            {
                Rect bounds = GetContainerBounds();
                Vector2 textSize = CalcTextSize(*g_InterMedium, L"DAISY", 128_dp);
                Vector2 padding = (bounds.GetSize() - textSize) / 2.f;

                DrawRect(bounds.Min, bounds.Max, { .FillColor = 0xF8F8F8FF, .Rounding = 5_dp });
                DrawTexto(*g_InterMedium, L"DAISY", bounds.Min + padding, 128_dp, { .Color = Color::Black });

                DifGoUp();

                DrawRect(bounds.Min, bounds.Max, { .FillColor = 0x422AD5FF, .Rounding = 5_dp });
                DrawTexto(*g_InterMedium, L"DAISY", bounds.Min + padding, 128_dp, { .Color = 0xe7e3e4ff });

                EndDifView();
            }
        }
        EndPageSection();
    }

    static void ModalPage()
    {
        BeginPageSection(L"Dialog modal");
        if (Button(L"Open modal", {.Id = HAX_LINE})) OpenModal(L"Modal1");
        EndPageSection();

        Space(55_dp);

        BeginPageSection(L"Dialog modal, closes when clicked outside");
        if (Button(L"Open modal", {.Id = HAX_LINE})) OpenModal(L"Modal2");
        EndPageSection();

        Space(55_dp);

        BeginPageSection(L"Dialog modal with a close button at corner");
        if (Button(L"Open modal", {.Id = HAX_LINE})) OpenModal(L"Modal3");
        EndPageSection();

        Space(55_dp);

        BeginPageSection(L"Dialog modal with custom width");
        if (Button(L"Open modal", {.Id = HAX_LINE})) OpenModal(L"Modal4");
        EndPageSection();

        ShowModal(L"Modal1", L"Press ESC key or click the button below to close", {.CloseBtn = true}); //!
        ShowModal(L"Modal2", L"Press ESC key or click outside to close", {.ClickClose = true}); //!
        ShowModal(L"Modal3", L"Press ESC key or click on X button to close", {.CrossBtn = true}); //!
        ShowModal(L"Modal4", L"Click the button below to close", {.CloseBtn = true, .Width = 1000_dp}); //!
    }

    static void SwapPage()
    {
        BeginPageSection(L"Swap text");
        {
            static bool s_Flag = true;
            if (Swap(*g_InterRegular, L"OFF", L"ON", 15_dp, s_Flag))
                s_Flag = !s_Flag;
        }
        EndPageSection();

        Space(55_dp);

        BeginPageSection(L"Swap volume icons");
        {
            static bool s_Flag = true;
            if (Swap(*g_IconsSolid, L"\uf6a8", L"\uf6a9", 32_dp, s_Flag))
                s_Flag = !s_Flag;
        }
        EndPageSection();

        Space(55_dp);

        BeginPageSection(L"Swap icons with rotate effect");
        {
            static bool s_Flag = true;
            if (Swap(*g_IconsRegular, L"\uf186", L"\ue28f", 32_dp, s_Flag, true))
                s_Flag = !s_Flag;
        }
        EndPageSection();

        Space(55_dp);

        BeginPageSection(L"Hamburger button");
        {
            static bool s_Flag = true;
            if (Swap(*g_IconsRegular, L"\uf0c9", L"\uf00d", 32_dp, s_Flag, true))
                s_Flag = !s_Flag;
        }
        EndPageSection();
    }

    static void AvatarPage()
    {
        BeginPageSection(L"Avatar");
        {
            Image(g_BadPerson, 96_dp, 5_dp);
        }
        EndPageSection();

        Space(55_dp);

        BeginPageSection(L"Avatar in custom sizes");
        {
            int multipliers[] = {6, 4, 3, 2};
            float maxSize = 16_dp * (float)multipliers[0];

            BeginHorizontal(8_dp);
            for (int i : multipliers)
            {
                float size = 16_dp * (float)i;
                BeginVertical();
                Space((maxSize - size) / 2.f);
                Image(g_SuperPerson, size, 5_dp);
                EndVertical();
            }
            EndHorizontal();
        }
        EndPageSection();

        Space(55_dp);

        BeginPageSection(L"Avatar rounded");
        {
            float size = 96_dp;
            BeginHorizontal(8_dp);
            Image(g_YellingWoman, size, 5_dp);
            Image(g_YellingCat, size, size / 2.f);
            EndHorizontal();
        }
        EndPageSection();

        Space(55_dp);

        BeginPageSection(L"Avatar with presence indicator");
        {
            float size = 96_dp;
            BeginHorizontal(8_dp);
            Image(g_Gordon, size, size / 2.f, Presence::Online);
            Image(g_IdiotSandwich, size, size / 2.f, Presence::Offline);
            EndHorizontal();
        }
        EndPageSection();
    }

    static void BadgePage()
    {
        BeginPageSection(L"Badge");
        {
            Badge(L"Badge");
        }
        EndPageSection();

        Space(55_dp);

        BeginPageSection(L"Badge sizes");
        {
            Dummy({0.f, 32_dp});
            Space(-GetLayoutSpacing());
            Badge(L"Xsmall",   {.AlignH = true, .FontH = 10_dp});
            Badge(L"Small",    {.AlignH = true, .FontH = 12_dp});
            Badge(L"Medium",   {.AlignH = true, .FontH = 14_dp});
            Badge(L"Large",    {.AlignH = true, .FontH = 16_dp});
            Badge(L"Xlarge",   {.AlignH = true, .FontH = 18_dp});
        }
        EndPageSection();

        Space(55_dp);

        BeginPageSection(L"Badge with colors");
        Badge(L"Primary",  {.Style = Themes::Prime.GetMain()});
        Badge(L"Secondary",{.Style = Themes::Second.GetMain()});
        Badge(L"Accent",   {.Style = Themes::Accent.GetMain()});
        Badge(L"Neutral",  {.Style = Themes::Neutral.GetMain()});
        Badge(L"Info",     {.Style = Themes::Info.GetMain()});
        Badge(L"Success",  {.Style = Themes::Success.GetMain()});
        Badge(L"Warning",  {.Style = Themes::Warn.GetMain()});
        Badge(L"Error",    {.Style = Themes::Error.GetMain()});
        EndPageSection();

        Space(55_dp);

        BeginPageSection(L"Badge with soft style");
        Badge(L"Primary",  {.Style = Themes::Prime.GetSoft()});
        Badge(L"Secondary",{.Style = Themes::Second.GetSoft()});
        Badge(L"Accent",   {.Style = Themes::Accent.GetSoft()});
        Badge(L"Info",     {.Style = Themes::Info.GetSoft()});
        Badge(L"Success",  {.Style = Themes::Success.GetSoft()});
        Badge(L"Warning",  {.Style = Themes::Warn.GetSoft()});
        Badge(L"Error",    {.Style = Themes::Error.GetSoft()});
        EndPageSection();

        Space(55_dp);

        BeginPageSection(L"Badge with outline style");
        Badge(L"Primary",  {.Style = Themes::Prime.GetOutline()});
        Badge(L"Secondary",{.Style = Themes::Second.GetOutline()});
        Badge(L"Accent",   {.Style = Themes::Accent.GetOutline()});
        Badge(L"Info",     {.Style = Themes::Info.GetOutline()});
        Badge(L"Success",  {.Style = Themes::Success.GetOutline()});
        Badge(L"Warning",  {.Style = Themes::Warn.GetOutline()});
        Badge(L"Error",    {.Style = Themes::Error.GetOutline()});
        EndPageSection();

        Space(55_dp);

        BeginPageSection(L"Empty badge");
        Badge(L"", {.AlignH = true, .FontH = 16_dp, .Style = Themes::Prime.GetMain()});
        Badge(L"", {.AlignH = true, .FontH = 14_dp, .Style = Themes::Prime.GetMain()});
        Badge(L"", {.AlignH = true, .FontH = 12_dp, .Style = Themes::Prime.GetMain()});
        Badge(L"", {.AlignH = true, .FontH = 10_dp, .Style = Themes::Prime.GetMain()});
        EndPageSection();

        Space(55_dp);

        BeginPageSection(L"Badge with icon");
        Badge(L"Info", {.Icon = L"\uf05a", .Style = Themes::Info.GetMain()});
        Badge(L"Success", {.Icon = L"\uf058", .Style = Themes::Success.GetMain()});
        Badge(L"Warning", {.Icon = L"\uf071", .Style = Themes::Warn.GetMain()});
        Badge(L"Error", {.Icon = L"\uf05e", .Style = Themes::Error.GetMain()});
        EndPageSection();
    }

    static void CollapsePage()
    {
        BeginPageSection(L"Collapse with focus");
        Collapse(HAX_LINE);
        EndPageSection();

        Space(55_dp);

        BeginPageSection(L"Collapse with checkbox");
        Collapse(HAX_LINE, CollapseType::Checkbox);
        EndPageSection();

        Space(55_dp);

        BeginPageSection(L"With arrow icon");
        Collapse(HAX_LINE, CollapseType::Focus, CollapseIcon::EndArrow);
        EndPageSection();

        Space(55_dp);

        BeginPageSection(L"Moving collapse icon to the start");
        Collapse(HAX_LINE, CollapseType::Focus, CollapseIcon::StartArrow);
        EndPageSection();
    }

    static void KbdPage()
    {
        BeginPageSection(L"Kbd");
        VirtualKey('K');
        EndPageSection();

        Space(55_dp);

        BeginPageSection(L"In text");
        Label(*g_InterRegular, L"Press", Color::Black);
        Space(3_dp);
        VirtualKey('F', .75f, 5.f);
        Space(3_dp);
        Label(*g_InterRegular, L"to pay respects.", Color::Black);
        EndPageSection();

        Space(55_dp);

        BeginPageSection(L"Key combination");
        Dummy({0.f, CalcKbdHeight()});
        Space(-GetLayoutSpacing());
        VirtualKey(VK_CONTROL);
        Label(*g_InterRegular, L"+", Color::Black, 13_dp, true);
        VirtualKey(VK_LSHIFT);
        Label(*g_InterRegular, L"+", Color::Black, 13_dp, true);
        VirtualKey(VK_DELETE);
        EndPageSection();

        Space(55_dp);

        BeginPageSection(L"A full keyboard");
        {
            BeginVertical(5_dp);
            BeginHorizontal(5_dp);
            static const uint8 s_Layout1[] = { '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', VK_OEM_MINUS, VK_OEM_PLUS, VK_BACK };
            for (uint8 vk : s_Layout1) VirtualKey(vk);
            EndHorizontal();

            BeginHorizontal(5_dp);
            static const uint8 s_Layout2[] = { VK_TAB, 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', VK_OEM_4, VK_OEM_6, VK_OEM_5 };
            for (uint8 vk : s_Layout2) VirtualKey(vk);
            EndHorizontal();

            BeginHorizontal(5_dp);
            static const uint8 s_Layout3[] = { VK_CAPITAL, 'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', VK_OEM_1, VK_OEM_7, VK_RETURN };
            for (uint8 vk : s_Layout3) VirtualKey(vk);
            EndHorizontal();

            BeginHorizontal(5_dp);
            static const uint8 s_Layout4[] = { VK_LSHIFT, 'Z', 'X', 'C', 'V', 'B', 'N', 'M', VK_OEM_COMMA, VK_OEM_PERIOD, VK_OEM_2, VK_RSHIFT };
            for (uint8 vk : s_Layout4) VirtualKey(vk);
            EndHorizontal();

            EndVertical();
        }
        EndPageSection();
    }

    static void StatusPage()
    {
        BeginPageSection(L"Status");
        StatusLabel({}, 0xD9D9D9ff);
        EndPageSection();

        Space(55_dp);

        BeginPageSection(L"Status sizes");
        BeginHorizontal(5_dp);
        StatusLabel({}, 0xD9D9D9ff);
        StatusLabel({}, 0xD9D9D9ff, 0.75f);
        StatusLabel({}, 0xD9D9D9ff, 1.f);
        EndHorizontal();
        EndPageSection();

        Space(55_dp);

        BeginPageSection(L"Status with colors");
        StatusLabel({}, Themes::Prime.GetMain().Bg.ToColor());
        StatusLabel({}, Themes::Prime.GetMain().Bg.ToColor());
        StatusLabel({}, Themes::Second.GetMain().Bg.ToColor());
        StatusLabel({}, Themes::Accent.GetMain().Bg.ToColor());
        StatusLabel({}, Themes::Neutral.GetMain().Bg.ToColor());
        StatusLabel({}, Themes::Info.GetMain().Bg.ToColor());
        StatusLabel({}, Themes::Success.GetMain().Bg.ToColor());
        StatusLabel({}, Themes::Warn.GetMain().Bg.ToColor());
        StatusLabel({}, Themes::Error.GetMain().Bg.ToColor());
        EndPageSection();

        Space(55_dp);

        BeginPageSection(L"Status with ping animation");
        StatusLabel(L"Server is down", 0xff637dff, 0.5f, StatusAnimation::Pulse);
        EndPageSection();

        Space(55_dp);

        BeginPageSection(L"Status with bounce animation");
        StatusLabel(L"Unread messages", 0x00bafeff, 0.5f, StatusAnimation::Bounce);
        EndPageSection();
    }

    static void BreadCrumbsPage()
    {
        BeginPageSection(L"Breadcrumbs");
        BeginBreadCrumbs();
        BreadCrumb(HAX_LINE, {}, L"Home", true);
        BreadCrumb(HAX_LINE, {}, L"Documents", true);
        BreadCrumb(HAX_LINE, {}, L"Add Document", false);
        EndBreadCrumb();
        EndPageSection();

        Space(55_dp);

        BeginPageSection(L"Breadcrumbs with icons");
        BeginBreadCrumbs();
        BreadCrumb(HAX_LINE, L"\uf07b", L"Home", true);
        BreadCrumb(HAX_LINE, L"\uf07b", L"Documents", true);
        BreadCrumb(HAX_LINE, L"\uf56d", L"Add Document", false);
        EndBreadCrumb();
        EndPageSection();

        Space(55_dp);

        BeginPageSection(L"Breadcrumbs with max-width");
        BeginContainer(HAX_LINE, {.W = 320_dp, .H = 30_dp, .ScrollX = true, .Style = NavMenuScroll});
        BeginBreadCrumbs();
        BreadCrumb(HAX_LINE, {}, L"Long text 1", false);
        BreadCrumb(HAX_LINE, {}, L"Long text 2", false);
        BreadCrumb(HAX_LINE, {}, L"Long text 3", false);
        BreadCrumb(HAX_LINE, {}, L"Long text 4", false);
        BreadCrumb(HAX_LINE, {}, L"Long text 5", false);
        EndBreadCrumb();
        EndContainer();
        EndPageSection();
    }

    static void LinkPage()
    {
        BeginPageSection(L"Link");
        HyperLink(HAX_LINE, L"Click me", true, Themes::Neutral);
        EndPageSection();

        Space(55_dp);

        BeginPageSection(L"Embedded link");
        BeginVertical(5_dp);
        Label(*g_InterRegular, L"Tailwind CSS resets the style of links by default.", Color::Black, 14_dp);
        BeginHorizontal(5_dp);
        Label(*g_InterRegular, L"Add \"link\" class to make it look like a", Color::Black, 14_dp); 
        HyperLink(HAX_LINE, L"normal link", true, Themes::Neutral);
        Label(*g_InterRegular, L"again.", Color::Black, 14_dp); 
        EndHorizontal();
        EndVertical();
        EndPageSection();

        Space(55_dp);

        BeginPageSection(L"Primary color");
        HyperLink(HAX_LINE, L"Click me", true, Themes::Prime);
        EndPageSection();

        Space(55_dp);

        BeginPageSection(L"Secondary color");
        HyperLink(HAX_LINE, L"Click me", true, Themes::Second);
        EndPageSection();

        Space(55_dp);

        BeginPageSection(L"Accent color");
        HyperLink(HAX_LINE, L"Click me", true, Themes::Accent);
        EndPageSection();

        Space(55_dp);

        BeginPageSection(L"Success color");
        HyperLink(HAX_LINE, L"Click me", true, Themes::Success);
        EndPageSection();

        Space(55_dp);

        BeginPageSection(L"Info color");
        HyperLink(HAX_LINE, L"Click me", true, Themes::Info);
        EndPageSection();

        Space(55_dp);

        BeginPageSection(L"Warning color");
        HyperLink(HAX_LINE, L"Click me", true, Themes::Warn);
        EndPageSection();

        Space(55_dp);

        BeginPageSection(L"Error color");
        HyperLink(HAX_LINE, L"Click me", true, Themes::Error);
        EndPageSection();

        Space(55_dp);

        BeginPageSection(L"Show underline only on hover");
        HyperLink(HAX_LINE, L"Click me", false, Themes::Neutral);
        EndPageSection();
    }

    static void CheckboxPage()
    {
        static Array<bool, 12> s_Values{true};

        BeginPageSection(L"Checkbox");
        Checkbox(HAX_LINE, s_Values[0], {.Def = Themes::Neutral.GetOutline(), .Hov = Themes::Neutral.GetOutline()});
        EndPageSection();

        Space(55_dp);

        BeginPageSection(L"With label");
        Checkbox(HAX_LINE, s_Values[11], {.Def = Themes::Neutral.GetOutline(), .Hov = Themes::Neutral.GetOutline(), .Text = L"Remember me"});
        EndPageSection();

        Space(55_dp);

        BeginPageSection(L"Colors");
        BeginHorizontal(8_dp);
        Checkbox(HAX_LINE, s_Values[1], {.Def = Themes::Prime.GetOutline(), .Hov = Themes::Prime.GetMain()});
        Checkbox(HAX_LINE, s_Values[2], {.Def = Themes::Second.GetOutline(), .Hov = Themes::Second.GetMain()});
        Checkbox(HAX_LINE, s_Values[3], {.Def = Themes::Accent.GetOutline(), .Hov = Themes::Accent.GetMain()});
        Checkbox(HAX_LINE, s_Values[4], {.Def = Themes::Neutral.GetOutline(), .Hov = Themes::Neutral.GetMain()});
        Checkbox(HAX_LINE, s_Values[5], {.Def = Themes::Info.GetOutline(), .Hov = Themes::Info.GetMain()});
        Checkbox(HAX_LINE, s_Values[6], {.Def = Themes::Success.GetOutline(), .Hov = Themes::Success.GetMain()});
        Checkbox(HAX_LINE, s_Values[7], {.Def = Themes::Warn.GetOutline(), .Hov = Themes::Warn.GetMain()});
        Checkbox(HAX_LINE, s_Values[8], {.Def = Themes::Error.GetOutline(), .Hov = Themes::Error.GetMain()});
        EndHorizontal();
        EndPageSection();

        Space(55_dp);

        BeginPageSection(L"Disabled");
        static bool s_Val1 = false;
        Checkbox(HAX_LINE, s_Val1, {.Def = Themes::Neutral.GetOutline(), .Disabled = true, .Hov = Themes::Neutral.GetOutline()});
        Checkbox(HAX_LINE, s_Values[9], {.Def = Themes::Neutral.GetOutline(), .Disabled = true, .Hov = Themes::Neutral.GetOutline()});
        EndPageSection();

        Space(55_dp);

        BeginPageSection(L"Checkbox with custom colors");
        const ButtonStyle style1 = {.Bg = 0x615fffff, .Fg = 0x0, .Out = 0x4f39f6ff};
        const ButtonStyle style2 = {.Bg = 0xff8904ff, .Fg = 0x9f2d00ff, .Out = 0x9f2d00ff};
        Checkbox(HAX_LINE, s_Values[10], {.Def = style1, .Hov = style2});
        EndPageSection();
    }
}