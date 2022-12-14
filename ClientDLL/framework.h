#pragma once

#define WIN32_LEAN_AND_MEAN             // Исключите редко используемые компоненты из заголовков Windows
#define GUI_APP
// Файлы заголовков Windows
#include <windows.h>
#include <WinSock2.h>
#include <ws2tcpip.h>

#include <string>
#include <vector>
#include <algorithm>
#include <sstream>

using woss_t = std::wostringstream;

#include "../include/wchar_error.h"