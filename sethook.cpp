#include <windows.h>

#include <iostream>

#include <AnalogueKeyboard.hpp>
#include <Thread.hpp>
#include <time.hpp>

using EnableHook_t = void(*)(HMODULE);
using DisableHook_t = void(*)();
using BlockKey_t = void(*)(int vk);
using UnblockKey_t = void(*)(int vk);
using EnsureKeyIsDown_t = void(*)(int vk);

static bool priority_algoritm = false;

static BlockKey_t BlockKey;
static UnblockKey_t UnblockKey;
static EnsureKeyIsDown_t EnsureKeyIsDown;

int main(int argc, const char** argv)
{
	if (argc > 1 && strcmp(argv[1], "priority") == 0)
	{
		std::cout << "Using \"last input priority\" algorithm." << std::endl;
		priority_algoritm = true;
	}
	else
	{
		std::cout << "Using \"furthest down\" algorithm for SOCD resolution. To use \"last input priority\" algorithm, pass 'priority' as an argument." << std::endl;
	}

	HMODULE hHookDLL = LoadLibraryA("hookdll.dll");
	if (hHookDLL)
	{
		auto EnableHook = (EnableHook_t)GetProcAddress(hHookDLL, "EnableHook");
		auto DisableHook = (DisableHook_t)GetProcAddress(hHookDLL, "DisableHook");
		BlockKey = (BlockKey_t)GetProcAddress(hHookDLL, "BlockKey");
		UnblockKey = (UnblockKey_t)GetProcAddress(hHookDLL, "UnblockKey");
		EnsureKeyIsDown = (EnsureKeyIsDown_t)GetProcAddress(hHookDLL, "EnsureKeyIsDown");

		if (EnableHook && DisableHook && BlockKey && UnblockKey)
		{
			EnableHook(hHookDLL);

			soup::Thread t([](soup::Capture&&)
			{
				time_t time_a = 0;
				time_t time_d = 0;
				float last_value_a = 0.0f;
				float last_value_d = 0.0f;
				while (true)
				{
					for (auto& kbd : soup::AnalogueKeyboard::getAll())
					{
						std::cout << "Detected " << kbd.name << std::endl;
						while (!kbd.disconnected)
						{
							float value_a = 0.0f;
							float value_d = 0.0f;
							for (auto& key : kbd.getActiveKeys())
							{
								switch (key.getSoupKey())
								{
								case soup::KEY_A: value_a = key.getFValue(); break;
								case soup::KEY_D: value_d = key.getFValue(); break;
								default:;
								}
							}
							if (value_a > last_value_a)
							{
								time_a = soup::time::millis();
							}
							last_value_a = value_a;
							if (value_d > last_value_d)
							{
								time_d = soup::time::millis();
							}
							last_value_d = value_d;
							if (value_a >= 0.25f && value_d >= 0.25f) // Both keys at least 1.0mm down?
							{
								if (priority_algoritm ? (time_a > time_d) : (value_a > value_d))
								{
									BlockKey('D');
									UnblockKey('A');
									EnsureKeyIsDown('A');
								}
								else if (priority_algoritm ? (time_a < time_d) : (value_a < value_d))
								{
									BlockKey('A');
									UnblockKey('D');
									EnsureKeyIsDown('D');
								}
								else
								{
									UnblockKey('A');
									UnblockKey('D');
									EnsureKeyIsDown('A');
									EnsureKeyIsDown('D');
								}
							}
							else
							{
								UnblockKey('A');
								UnblockKey('D');
							}
						}
					}
				}
			});

			MSG msg;
			while (GetMessage(&msg, nullptr, 0, 0) /*&& t.isRunning()*/)
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}

			DisableHook();
		}

		FreeLibrary(hHookDLL);
	}

	return 0;
}
