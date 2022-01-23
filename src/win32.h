
//==============================================================================
//
//   windows-api-ex.h 
//
//==============================================================================
//  Copyright (C) Guilaume Plante 2020 <cybercastor@icloud.com>
//==============================================================================


#ifndef WINAPIEX_H
#define WINAPIEX_H

#ifdef UNICODE
std::wstring GetLastMsg();
#else
std::string GetLastMsg();
#endif
std::wstring stringToWstring(const char* utf8Bytes);

#endif
LPWSTR StringToString(LPCSTR str);
LPSTR StringToString(LPCWSTR str);
