#ifndef OVERLAY_HPP
#define OVERLAY_HPP

#include "../Menu/Menu.hpp"
#include <Windows.h>
#include <d3d11.h>

class Overlay {
public:
	bool init(HWND targetWindow);
	void shutdown();

	bool processMessages();
	void syncPosition();
	void beginFrame();
	void endFrame();

	void setClickThrough(bool enabled);

	HWND getWindow() const { return overlayHwnd; }
	bool isRunning() const { return running; }

private:
	Menu menu;

	bool createOverlayWindow(int x, int y, int w, int h);
	bool createDeviceD3D(int width, int height);
	void cleanupDeviceD3D();
	void createRenderTarget();
	void cleanupRenderTarget();

	static LRESULT CALLBACK wndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

	HWND targetHwnd = nullptr;
	HWND overlayHwnd = nullptr;
	WNDCLASSEXW windowClass = {};

	ID3D11Device* device = nullptr;
	ID3D11DeviceContext* context = nullptr;
	IDXGISwapChain* swapChain = nullptr;
	ID3D11RenderTargetView* renderTargetView = nullptr;

	bool running = true;
	bool clickThrough = true;
	int width = 0;
	int height = 0;
	int lastX = 0;
	int lastY = 0;
	int lastWidth = 0;
	int lastHeight = 0;
};

#endif // OVERLAY_HPP
