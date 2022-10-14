#pragma once
#include "pch.h"
#include "ClientInfo.h"

inline void WFSOINF(HANDLE Obj)
{
	if (WaitForSingleObject(Obj, INFINITE) != WAIT_OBJECT_0)
	{
		throw std::runtime_error(std::string("WaitForSingleObject failed. Code: ") + std::to_string(GetLastError()));
	}
}

void PrintMessage(const ClientInfo& Sender, const std::vector<WSABUF>& V);
void ValidatePacket(const ClientInfo& Sender, const std::vector<WSABUF>& V);
void AppendSenderAddr(const ClientInfo& Sender, std::vector<WSABUF>& V, char* buf);

//std::string Join(const std::vector<WSABUF>& IOBuf, const char* const delimiter) {
//    std::wostringstream os;
//    auto b = begin(elements), e = end(elements);
//
//    if (b != e) {
//        std::copy(b, prev(e), std::ostream_iterator<Value>(os, delimiter));
//        b = prev(e);
//    }
//    if (b != e) {
//        os << *b;
//    }
//
//    return os.str();
//}