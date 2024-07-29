#include <windows.h>

#include <Key.hpp>
#include <osInput.hpp>

static HHOOK hKeyboardHook;
static bool blocked_vks[256];
static bool apparent_state[256];

LRESULT CALLBACK KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam)
{
	if (nCode == HC_ACTION)
	{
		auto event = (KBDLLHOOKSTRUCT*)lParam;

		if (event->vkCode >= 0 && event->vkCode < 256)
		{
			if (event->flags & LLKHF_UP)
			{
				if (blocked_vks[event->vkCode] && !apparent_state[event->vkCode])
				{
					return 1;
				}
				apparent_state[event->vkCode] = false;
			}
			else
			{
				if (blocked_vks[event->vkCode])
				{
					return 1;
				}
				apparent_state[event->vkCode] = true;
			}
		}
	}

	return CallNextHookEx(hKeyboardHook, nCode, wParam, lParam);
}

extern "C" __declspec(dllexport) void EnableHook(HMODULE hmod)
{
	hKeyboardHook = SetWindowsHookExA(WH_KEYBOARD_LL, KeyboardProc, hmod, 0);
}

extern "C" __declspec(dllexport) void DisableHook()
{
	UnhookWindowsHookEx(hKeyboardHook);
}

extern "C" __declspec(dllexport) void BlockKey(int vk)
{
	if (vk >= 0 && vk < 256)
	{
		if (apparent_state[vk])
		{
			soup::osInput::simulateKeyRelease(soup::virtual_key_to_soup_key(vk));
			apparent_state[vk] = false;
		}

		blocked_vks[vk] = true;
	}
}

extern "C" __declspec(dllexport) void UnblockKey(int vk)
{
	if (vk >= 0 && vk < 256)
	{
		blocked_vks[vk] = false;
	}
}

extern "C" __declspec(dllexport) void EnsureKeyIsDown(int vk)
{
	if (!apparent_state[vk])
	{
		soup::osInput::simulateKeyDown(soup::virtual_key_to_soup_key(vk));
		apparent_state[vk] = true;
	}
}
