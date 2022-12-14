#pragma once

#define WIN32_LEAN_AND_MEAN             // Исключите редко используемые компоненты из заголовков Windows
// Файлы заголовков Windows
#include <windows.h>
#include <WinSock2.h>
#include <ws2tcpip.h>

#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <list>
#include <forward_list>
#include <future>

using woss_t = std::wostringstream;

#include "../include/wchar_error.h"