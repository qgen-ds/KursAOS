#pragma once

#define WIN32_LEAN_AND_MEAN             // Исключите редко используемые компоненты из заголовков Windows
// Файлы заголовков Windows
#include <windows.h>
#include <WinSock2.h>
#include <ws2tcpip.h>

#include <iostream>
#include <vector>
#include <list>
#include <sstream>

constexpr size_t RECV_SIZE = 4096; // Размер буфера I/O
#include "../include/WCHARException.h"