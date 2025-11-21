#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>

#pragma warning(push, 0)
#define DERPNET_STATIC
#include "vendor/derpnet.h"
#pragma warning(pop)

#include <windows.h>


typedef int8_t  i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef i64 imm;
typedef u64 umm;

HWND Window = 0;
bool IsSub = false;
DerpKey PrivateKey;
DerpKey PublicKey;
DerpKey ConsentKey;
DerpNet Net = {0};

#define DERP_SERVER_HOST "derp7e.tailscale.com"

void
InvalidArguments()
{
#define ERRSTRING L"Invalid Arguments.\nExpected:\nrct sub\nrct dom CONSENT_KEY\n"
	MessageBoxW(0, ERRSTRING, L"Remote Clicker Trainer", MB_OK | MB_ICONSTOP);
	wprintf(ERRSTRING);
#undef ERRSTRING
}

static LRESULT CALLBACK
WndProc(HWND window, UINT msg_code, WPARAM wparam, LPARAM lparam)
{
	if (msg_code == WM_HOTKEY)
	{
#define MESS "Good Girl"
		DerpNet_Send(&Net, &ConsentKey, MESS, sizeof(MESS)-1);
#undef MESS

		return 0;
	}

	return DefWindowProcW(window, msg_code, wparam, lparam);
}

int
wWinMain(HINSTANCE instance, HINSTANCE previous, LPWSTR cmdline, int cmdshow)
{
	int argc;
	LPWSTR* argv = CommandLineToArgvW(cmdline, &argc);

	if      (argc == 1 && wcscmp(argv[0], L"sub") == 0) IsSub = true;
	else if (argc == 2 && wcscmp(argv[0], L"dom") == 0)
	{
		IsSub = false;

		umm i = 0;
		for (; i < 32; ++i)
		{
			wchar_t c0 = argv[1][2*i + 0];
			wchar_t c1 = (c0 != 0 ? argv[1][2*i + 1] : 0);

			bool c0_invalid = false;
			bool c1_invalid = false;

			u8 c0_digit = 0;
			u8 c1_digit = 0;

			if      ((u8)(c0 - L'0') < (u8)10)                  c0_digit = (u8)(c0 - L'0');
			else if ((u8)((c0&0xDF) - L'A') <= (u8)('Z' - 'A')) c0_digit = (u8)((c0 - L'A') + 10);
			else c0_invalid = true;

			if      ((u8)(c1 - L'0') < (u8)10)                  c1_digit = (u8)(c1 - L'0');
			else if ((u8)((c1&0xDF) - L'A') <= (u8)('Z' - 'A')) c1_digit = (u8)((c1 - L'A') + 10);
			else c1_invalid = true;

			if (c0_invalid || c1_invalid)
			{
				MessageBoxW(0, L"Invalid consent key", L"Remote Clicker Trainer", MB_OK | MB_ICONERROR);
				exit(1);
			}

			ConsentKey.Bytes[i] = c0_digit | (c1_digit << 4);
		}
	}
	else
	{
		InvalidArguments();
		exit(1);
	}

	DerpNet_CreateNewKey(&PrivateKey);
	DerpNet_GetPublicKey(&PrivateKey, &PublicKey);

	if(IsSub)
	{
#define CONSENT_STR L"Your consent key is:\n"
#define CONSENT_STR_LEN (sizeof(CONSENT_STR)/sizeof(wchar_t) - 1)
		
		wchar_t message[CONSENT_STR_LEN + 64 + 1] = {0};

		for (umm i = 0; i < CONSENT_STR_LEN; ++i) message[i] = CONSENT_STR[i];

		for (umm i = 0; i < 32; ++i)
		{
			message[CONSENT_STR_LEN + 2*i + 0] = ((PublicKey.Bytes[i] & 0xF) < 10 ? L'0' : L'A' - 10) + (PublicKey.Bytes[i] & 0xF);
			message[CONSENT_STR_LEN + 2*i + 1] = ((PublicKey.Bytes[i] >>  4) < 10 ? L'0' : L'A' - 10) + (PublicKey.Bytes[i] >>  4);
		}


		MessageBoxW(0, message, L"Remote Clicker Trainer", MB_OK);

		if (!DerpNet_Open(&Net, DERP_SERVER_HOST, &PrivateKey))
		{
			MessageBoxW(0, L"Failed to connect to DERP relay", L"Remote Clicker Trainer", MB_OK | MB_ICONERROR);
			exit(1);
		}

		for (;;)
		{
			if (DerpNet_Recv(&Net, &(DerpKey){0}, &(u8*){0}, &(u32){0}, false))
			{
				PlaySound("M:\\clicker.wav", 0, SND_SYNC);
			}

			Sleep(33);
		}
	}
	else
	{
		if (!DerpNet_Open(&Net, DERP_SERVER_HOST, &PrivateKey))
		{
			MessageBoxW(0, L"Failed to connect to DERP relay", L"Remote Clicker Trainer", MB_OK | MB_ICONERROR);
			exit(1);
		}

		WNDCLASSEXW window_class = {
			.cbSize        = sizeof(window_class),
			.lpfnWndProc   = WndProc,
			.hInstance     = GetModuleHandleW(0),
			.lpszClassName = L"RCT_WINDOW",
		};

		ATOM atom = RegisterClassExW(&window_class);
		assert(atom);

		Window = CreateWindowExW(0, window_class.lpszClassName, L"RCT", WS_POPUP, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, 0, 0, window_class.hInstance, 0);
		assert(Window);

		BOOL registered_hotkey = RegisterHotKey(Window, 0, 0, VK_F7);
		assert(registered_hotkey);

		for (;;)
		{
			MSG msg;
			if (!GetMessageW(&msg, 0, 0, 0))
			{
				DerpNet_Close(&Net);
				exit(0);
			}

			TranslateMessage(&msg);
			DispatchMessageW(&msg);
		}
	}
}
