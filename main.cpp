//#include "haxgui_impl_dx11.h"

#include <d3d11.h>
#include <dxgi.h>
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")

#include <atomic>
#include <thread>

//#include "imgui/imgui.h"
//#include "imgui/imgui_impl_dx11.h"
//#include "imgui/imgui_impl_win32.h"

#include "hax_gui_dx11.h"

#include "resource.h"

std::atomic<bool>               g_Running = true;

static ID3D11Device*            g_pd3dDevice;
static ID3D11DeviceContext*     g_pd3dDeviceContext;
static IDXGISwapChain*          g_pSwapChain;
static bool                     g_SwapChainOccluded;
static UINT                     g_ResizeWidth, g_ResizeHeight;
static ID3D11RenderTargetView*  g_mainRenderTargetView;

bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void CreateRenderTarget();
void CleanupRenderTarget();
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

using namespace Hax;

//extern void ShowCheatWindow(Handle hModule);

constexpr int kTests = 100;
static void RenderThread(HWND hwnd)
{
    static float s_FPS = 0.f;

    CreateDeviceD3D(hwnd);

    Gui::Initialize((Hax::Handle)hwnd, g_pd3dDevice);

    while (g_Running)
    {
        if (g_ResizeWidth != 0 && g_ResizeHeight != 0)
        {
            CleanupRenderTarget();
            g_pSwapChain->ResizeBuffers(0, g_ResizeWidth, g_ResizeHeight, DXGI_FORMAT_UNKNOWN, 0);
            g_ResizeWidth = g_ResizeHeight = 0;
            CreateRenderTarget();
        }

        Gui::LinearColor bg(0xF5F5F5FF);
        g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);
        g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, &bg.R);

        Hax::Gui::BeginFrame();
        Hax::Gui::ShowDemoWindow();
        Hax::Gui::EndFrame();

        g_pSwapChain->Present(1, 0);
    }

    printf("FPS: %f\n", Hax::Gui::GetFramerate());

    void* iter = nullptr;
    while (Hax::Allocator* alloc = Hax::Gui::IterAllocators(iter))
    {
        printf("%s | total: %zu, max: %zu\n", alloc->Name, alloc->TotalAllocated, alloc->MaxAllocated);
    }

    Hax::Gui::Shutdown();

    CleanupDeviceD3D();
}

//static void RenderThreadImGui(HWND hwnd)
//{
//    CreateDeviceD3D(hwnd);
//
//    IMGUI_CHECKVERSION();
//    ImGui::CreateContext();
//    ImGuiIO& io = ImGui::GetIO(); (void)io;
//    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
//    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
//
//    ImGui_ImplWin32_Init(hwnd);
//    ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);
//    ImGui::GetStyle().AntiAliasedFill = false;
//    ImGui::GetStyle().AntiAliasedLines = false;
//
//    while (g_Running)
//    {
//        if (g_ResizeWidth != 0 && g_ResizeHeight != 0)
//        {
//            CleanupRenderTarget();
//            g_pSwapChain->ResizeBuffers(0, g_ResizeWidth, g_ResizeHeight, DXGI_FORMAT_UNKNOWN, 0);
//            g_ResizeWidth = g_ResizeHeight = 0;
//            CreateRenderTarget();
//        }
//
//        ImGui_ImplDX11_NewFrame();
//        ImGui_ImplWin32_NewFrame();
//        ImGui::NewFrame();
//
//        //g_FPS = ImGui::GetIO().Framerate;
//
//        //for (int i = 0; i < 10000; ++i) { ImGui::GetForegroundDrawList()->AddTriangleFilled({50.f, 50.f}, {20.f, 100.f}, {80.f, 70.f}, 0xff0000ff); }
//
//        ImGui::ShowDemoWindow();
//
//        ImGui::Render();
//
//        float clear_color[4] = { 0.95f, 0.95f, 0.95f, 1.00f };
//        g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);
//        g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color);
//        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
//
//        g_pSwapChain->Present(0, 0);
//    }
//
//    printf("%f\n", ImGui::GetIO().Framerate);
//
//    ImGui_ImplDX11_Shutdown();
//    ImGui_ImplWin32_Shutdown();
//    ImGui::DestroyContext();
//
//
//    CleanupDeviceD3D();
//}

extern size_t g_TotalAllocated;
extern size_t g_MaxAllocated;

int main()
{
    ::SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

    WNDCLASSEXW wc = { sizeof(wc), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr, L"ImGui Example", nullptr };
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);

    DEVMODE dm;
    dm.dmSize = sizeof(dm);
    EnumDisplaySettings(NULL, ENUM_CURRENT_SETTINGS, &dm);

    ::RegisterClassExW(&wc);
    HWND hwnd = ::CreateWindowW(wc.lpszClassName, L"Example", WS_OVERLAPPEDWINDOW | WS_MAXIMIZE | WS_VISIBLE, 100, 100, 1600, 900, nullptr, nullptr, wc.hInstance, nullptr);

    ::ShowWindow(hwnd, SW_MAXIMIZE);
    ::UpdateWindow(hwnd);

    std::thread renderThread(RenderThread, hwnd);

    while (g_Running)
    {
        MSG msg;
        while (::PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE))
        {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
            if (msg.message == WM_QUIT)
                g_Running = false;
        }
    }

    renderThread.join();

    ::DestroyWindow(hwnd);
    ::UnregisterClassW(wc.lpszClassName, wc.hInstance);

    /*if (g_MaxAllocated > 1024LL * 1024 * 1024) printf("Max memory used: %f gb\n", g_MaxAllocated / (1024.f * 1024.f * 1024.f));
    else if (g_MaxAllocated > 1024LL * 1024) printf("Max memory used: %f mb\n", g_MaxAllocated / (1024.f * 1024.f));
    else if (g_MaxAllocated > 1024LL) printf("Max memory used: %f kb\n", g_MaxAllocated / 1024.f);
    else printf("Max memory used: %zu b\n", g_MaxAllocated);

    printf("Memory leaked: %zu bytes\n", g_TotalAllocated);*/

    return 0;
}

bool CreateDeviceD3D(HWND hWnd)
{
    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory(&sd, sizeof(sd));
    sd.BufferCount = 2;
    sd.BufferDesc.Width = 0;
    sd.BufferDesc.Height = 0;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    UINT createDeviceFlags = 0;
    //createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0, };
    HRESULT res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
    if (res == DXGI_ERROR_UNSUPPORTED)
        res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
    if (res != S_OK)
        return false;

    CreateRenderTarget();
    return true;
}

void CleanupDeviceD3D()
{
    CleanupRenderTarget();
    if (g_pSwapChain) { g_pSwapChain->Release(); g_pSwapChain = nullptr; }
    if (g_pd3dDeviceContext) { g_pd3dDeviceContext->Release(); g_pd3dDeviceContext = nullptr; }
    if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = nullptr; }
}

void CreateRenderTarget()
{
    ID3D11Texture2D* pBackBuffer;
    g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_mainRenderTargetView);
    pBackBuffer->Release();
}

void CleanupRenderTarget()
{
    if (g_mainRenderTargetView) { g_mainRenderTargetView->Release(); g_mainRenderTargetView = nullptr; }
}

//extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (Hax::Gui::HandleWndMsg((void*)hWnd, msg, wParam, lParam))
        return 1;

    /*if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return TRUE;*/

    switch (msg)
    {
        case WM_SIZE:
            if (wParam == SIZE_MINIMIZED)
                return 0;
            g_ResizeWidth = (UINT)LOWORD(lParam);
            g_ResizeHeight = (UINT)HIWORD(lParam);
            return 0;
        case WM_SYSCOMMAND:
            if ((wParam & 0xfff0) == SC_KEYMENU)
                return 0;
            break;
        case WM_DESTROY:
            ::PostQuitMessage(0);
            return 0;
    }
    return ::DefWindowProcW(hWnd, msg, wParam, lParam);
}