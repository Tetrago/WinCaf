#include <Windows.h>
#include <CommCtrl.h>

#include "resource.h"

#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#pragma comment(lib, "comctl32.lib")

UINT const WMAPP_NOTIFYCALLBACK = WM_APP + 1;
UINT const TRAY_ID = 0;

HINSTANCE instance_;

PCWSTR enabledIcon_;
PCWSTR disabledIcon_;

bool state_ = false;

void register_window(PCWSTR name, WNDPROC proc)
{
	WNDCLASSEX wndc = {};
	wndc.cbSize = sizeof(wndc);
	wndc.lpfnWndProc = proc;
	wndc.hInstance = instance_;
	wndc.lpszClassName = name;

	RegisterClassEx(&wndc);
}

BOOL add_icon(HWND wnd)
{
	NOTIFYICONDATA icon = {};
	icon.cbSize = sizeof(icon);
	icon.hWnd = wnd;
	icon.uFlags = NIF_ICON | NIF_TIP | NIF_MESSAGE | NIF_SHOWTIP;
	icon.uID = TRAY_ID;
	icon.uCallbackMessage = WMAPP_NOTIFYCALLBACK;

	LoadIconMetric(instance_, disabledIcon_, LIM_SMALL, &icon.hIcon);
	LoadString(instance_, IDS_TIP, icon.szTip, ARRAYSIZE(icon.szTip));
	Shell_NotifyIcon(NIM_ADD, &icon);

	DestroyIcon(icon.hIcon);

	icon.uVersion = NOTIFYICON_VERSION_4;
	return Shell_NotifyIcon(NIM_SETVERSION, &icon);
}

BOOL remove_icon(HWND wnd)
{
	NOTIFYICONDATA icon = {};
	icon.cbSize = sizeof(icon);
	icon.uFlags = 0;
	icon.uID = TRAY_ID;
	icon.hWnd = wnd;
	return Shell_NotifyIcon(NIM_DELETE, &icon);
}

BOOL switch_icon(HWND wnd, PCWSTR to)
{
	NOTIFYICONDATA icon = {};
	icon.cbSize = sizeof(icon);
	icon.uFlags = NIF_ICON;
	icon.uID = TRAY_ID;
	icon.hWnd = wnd;
	
	LoadIconMetric(instance_, to, LIM_SMALL, &icon.hIcon);

	BOOL res = Shell_NotifyIcon(NIM_MODIFY, &icon);
	DestroyIcon(icon.hIcon);
	return res;
}

void block_sleep()
{
	SetThreadExecutionState(ES_CONTINUOUS | ES_AWAYMODE_REQUIRED | ES_SYSTEM_REQUIRED);
}

void allow_sleep()
{
	SetThreadExecutionState(ES_CONTINUOUS);
}

void flip_state(HWND wnd)
{
	PCWSTR to;

	if(state_ = !state_)
	{
		block_sleep();
		to = enabledIcon_;
	}
	else
	{
		allow_sleep();
		to = disabledIcon_;
	}

	if(!switch_icon(wnd, to))
	{
		MessageBox(wnd, L"Failed to modify tray icon", L"Error", MB_OK);
	}
}

LRESULT CALLBACK window_proc(HWND wnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch(msg)
	{
	case WM_CREATE:
		if(!add_icon(wnd))
		{
			MessageBox(wnd, L"Failed to add tray icon", L"Fatal Error", MB_OK);
			abort();
		}
		break;
	case WMAPP_NOTIFYCALLBACK:
		switch(LOWORD(lParam))
		{
		case WM_LBUTTONDBLCLK:
			flip_state(wnd);
			break;
		case WM_RBUTTONUP:
			DestroyWindow(wnd);
			break;
		}
		break;
	case WM_DESTROY:
		if(!remove_icon(wnd))
		{
			MessageBox(wnd, L"Failed to destroy tray icon properly", L"Error", MB_OK);
		}
		PostQuitMessage(0);
		break;
	}

	return DefWindowProc(wnd, msg, wParam, lParam);
}

INT WINAPI WinMain(HINSTANCE instance, HINSTANCE prevInstance, LPSTR cmdLine, int showCmd)
{
	instance_ = instance;

	enabledIcon_ = MAKEINTRESOURCE(IDI_ENABLED);
	disabledIcon_ = MAKEINTRESOURCE(IDI_DISABLED);

	wchar_t const className[] = L"wincaf";

	register_window(className, window_proc);
	if(HWND wnd = CreateWindow(className, className, WS_MINIMIZE, CW_USEDEFAULT, CW_USEDEFAULT, 10, 10, NULL, NULL, instance, NULL))
	{
		MSG msg;
		while(GetMessage(&msg, NULL, 0, 0))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	return 0;
}