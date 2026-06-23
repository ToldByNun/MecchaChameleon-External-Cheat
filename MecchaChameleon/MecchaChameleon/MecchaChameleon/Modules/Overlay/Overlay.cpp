#include "Overlay.hpp"

#include "../../Engine/ImGui/imgui.h"
#include "../../Engine/ImGui/imgui_impl_win32.h"
#include "../../Engine/ImGui/imgui_impl_dx11.h"

#include <dwmapi.h>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "dwmapi.lib")

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace {
	Overlay* g_overlay = nullptr;

	constexpr wchar_t kWindowClassName[] = L"MecchaOverlayClass";
}

bool Overlay::init(HWND targetWindow) {
	if (!targetWindow || !IsWindow(targetWindow))
		return false;

	targetHwnd = targetWindow;
	g_overlay = this;

	RECT rect{};
	if (!GetWindowRect(targetHwnd, &rect))
		return false;

	width = rect.right - rect.left;
	height = rect.bottom - rect.top;
	if (width <= 0 || height <= 0)
		return false;

	lastX = rect.left;
	lastY = rect.top;
	lastWidth = -1;
	lastHeight = -1;

	ImGui_ImplWin32_EnableDpiAwareness();

	if (!createOverlayWindow(rect.left, rect.top, width, height))
		return false;

	if (!createDeviceD3D(width, height)) {
		shutdown();
		return false;
	}

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();

	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	io.IniFilename = nullptr;

	ImGui::StyleColorsDark();
	ImGuiStyle& style = ImGui::GetStyle();
	style.WindowRounding = 6.f;
	style.FrameRounding = 4.f;
	style.Colors[ImGuiCol_WindowBg].w = 0.94f;

	ImGui_ImplWin32_Init(overlayHwnd);
	ImGui_ImplDX11_Init(device, context);
	ImGui_ImplWin32_EnableAlphaCompositing(overlayHwnd);

	setClickThrough(!settings::menuOpen);
	syncPosition();

	return true;
}

void Overlay::shutdown() {
	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();

	cleanupDeviceD3D();

	if (overlayHwnd) {
		DestroyWindow(overlayHwnd);
		overlayHwnd = nullptr;
	}

	UnregisterClassW(kWindowClassName, GetModuleHandleW(nullptr));
	g_overlay = nullptr;
}

bool Overlay::createOverlayWindow(int x, int y, int w, int h) {
	windowClass.cbSize = sizeof(WNDCLASSEXW);
	windowClass.style = CS_HREDRAW | CS_VREDRAW;
	windowClass.lpfnWndProc = wndProc;
	windowClass.hInstance = GetModuleHandleW(nullptr);
	windowClass.lpszClassName = kWindowClassName;

	RegisterClassExW(&windowClass);

	overlayHwnd = CreateWindowExW(
		WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_TRANSPARENT,
		kWindowClassName,
		L"MecchaOverlay",
		WS_POPUP,
		x, y, w, h,
		nullptr, nullptr, windowClass.hInstance, nullptr
	);

	if (!overlayHwnd)
		return false;

	SetLayeredWindowAttributes(overlayHwnd, RGB(0, 0, 0), 255, LWA_ALPHA);

	MARGINS margins = { -1, -1, -1, -1 };
	DwmExtendFrameIntoClientArea(overlayHwnd, &margins);

	ShowWindow(overlayHwnd, SW_SHOWDEFAULT);
	UpdateWindow(overlayHwnd);
	SetWindowPos(overlayHwnd, HWND_TOPMOST, x, y, w, h, SWP_NOACTIVATE | SWP_SHOWWINDOW);

	return true;
}

bool Overlay::createDeviceD3D(int w, int h) {
	DXGI_SWAP_CHAIN_DESC swapDesc = {};
	swapDesc.BufferCount = 2;
	swapDesc.BufferDesc.Width = static_cast<UINT>(w);
	swapDesc.BufferDesc.Height = static_cast<UINT>(h);
	swapDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapDesc.OutputWindow = overlayHwnd;
	swapDesc.SampleDesc.Count = 1;
	swapDesc.Windowed = TRUE;
	swapDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

	D3D_FEATURE_LEVEL featureLevel;
	const D3D_FEATURE_LEVEL featureLevels[] = { D3D_FEATURE_LEVEL_11_0 };
	const UINT createFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;

	if (FAILED(D3D11CreateDeviceAndSwapChain(
		nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createFlags,
		featureLevels, 1, D3D11_SDK_VERSION,
		&swapDesc, &swapChain, &device, &featureLevel, &context)))
		return false;

	createRenderTarget();
	return true;
}

void Overlay::createRenderTarget() {
	ID3D11Texture2D* backBuffer = nullptr;
	swapChain->GetBuffer(0, IID_PPV_ARGS(&backBuffer));
	if (backBuffer) {
		device->CreateRenderTargetView(backBuffer, nullptr, &renderTargetView);
		backBuffer->Release();
	}
}

void Overlay::cleanupRenderTarget() {
	if (renderTargetView) {
		renderTargetView->Release();
		renderTargetView = nullptr;
	}
}

void Overlay::cleanupDeviceD3D() {
	cleanupRenderTarget();

	if (swapChain) {
		swapChain->Release();
		swapChain = nullptr;
	}
	if (context) {
		context->Release();
		context = nullptr;
	}
	if (device) {
		device->Release();
		device = nullptr;
	}
}

void Overlay::syncPosition() {
	if (!targetHwnd || !overlayHwnd)
		return;

	RECT rect{};
	if (!GetWindowRect(targetHwnd, &rect))
		return;

	width = rect.right - rect.left;
	height = rect.bottom - rect.top;

	if (width <= 0 || height <= 0)
		return;

	const bool moved = rect.left != lastX || rect.top != lastY;
	const bool resized = width != lastWidth || height != lastHeight;

	SetWindowPos(
		overlayHwnd, HWND_TOPMOST,
		rect.left, rect.top, width, height,
		SWP_NOACTIVATE | SWP_SHOWWINDOW
	);

	if (moved) {
		lastX = rect.left;
		lastY = rect.top;
	}

	if (resized && swapChain) {
		cleanupRenderTarget();
		swapChain->ResizeBuffers(0, width, height, DXGI_FORMAT_UNKNOWN, 0);
		createRenderTarget();
		lastWidth = width;
		lastHeight = height;
	}
}

void Overlay::setClickThrough(bool enabled) {
	if (!overlayHwnd || clickThrough == enabled)
		return;

	clickThrough = enabled;

	LONG style = GetWindowLong(overlayHwnd, GWL_EXSTYLE);
	if (enabled)
		style |= WS_EX_TRANSPARENT;
	else
		style &= ~WS_EX_TRANSPARENT;

	SetWindowLong(overlayHwnd, GWL_EXSTYLE, style);
}

bool Overlay::processMessages() {
	MSG msg{};
	while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
		if (msg.message == WM_QUIT) {
			running = false;
			return false;
		}
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	if (!IsWindow(targetHwnd)) {
		running = false;
		return false;
	}

	return running;
}

void Overlay::beginFrame() {
	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();
}

void Overlay::endFrame() {
	ImGui::Render();

	const float clearColor[4] = { 0.f, 0.f, 0.f, 0.f };
	context->OMSetRenderTargets(1, &renderTargetView, nullptr);
	context->ClearRenderTargetView(renderTargetView, clearColor);

	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
	swapChain->Present(1, 0);
}

LRESULT CALLBACK Overlay::wndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
		return true;

	switch (msg) {
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	case WM_SIZE:
		if (g_overlay && g_overlay->device && wParam != SIZE_MINIMIZED) {
			g_overlay->cleanupRenderTarget();
			g_overlay->swapChain->ResizeBuffers(0, LOWORD(lParam), HIWORD(lParam), DXGI_FORMAT_UNKNOWN, 0);
			g_overlay->createRenderTarget();
		}
		return 0;
	}

	return DefWindowProcW(hWnd, msg, wParam, lParam);
}
