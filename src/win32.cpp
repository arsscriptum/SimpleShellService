
//==============================================================================
//
//   windows-api-ex.cpp
//
//==============================================================================
//  Copyright (C) Guilaume Plante 2020 <cybercastor@icloud.com>
//==============================================================================


#include "stdafx.h"
#include "win32.h"
#include <iostream>
#include <codecvt>
#include <locale> 


LPWSTR StringToString(LPCSTR str)
{
    int size = MultiByteToWideChar(CP_UTF8, 0, str, -1, NULL, 0);
    PWCHAR result = new WCHAR[size];
    MultiByteToWideChar(CP_UTF8, 0, str, -1, result, size);

    return result;
}
LPSTR StringToString(LPCWSTR str)
{
    int size = WideCharToMultiByte(CP_UTF8, 0, str, -1, NULL, 0, NULL, NULL);
    PCHAR result = new CHAR[size];
    WideCharToMultiByte(CP_UTF8, 0, str, -1, result, size, NULL, NULL);

    return result;
}


std::wstring stringToWstring(const char* utf8Bytes)
{
	//setup converter
	using convert_type = std::codecvt_utf8<typename std::wstring::value_type>;
	std::wstring_convert<convert_type, typename std::wstring::value_type> converter;

	//use converter (.to_bytes: wstr->str, .from_bytes: str->wstr)
	return converter.from_bytes(utf8Bytes);

}
#ifdef UNICODE
std::wstring GetLastMsg()
{
    std::wstring _ret = stringToWstring("no error");
    DWORD errorCode = GetLastError();
    if (errorCode == 0)
        return _ret;

    // map errorCode into a system defined error string

    DWORD dwFlags =
        FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER;
    LPCVOID lpSource = NULL;
    DWORD dwMessageID = errorCode;
    DWORD dwLanguageId = MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US);
    LPWSTR lpBuffer;
    DWORD nSize = 0;
    va_list* Arguments = NULL;

    FormatMessage(
        dwFlags, lpSource, dwMessageID, dwLanguageId,
        (LPWSTR)&lpBuffer, nSize, Arguments
    );
    printf("%ls", lpBuffer);
    std::wstring _msg = lpBuffer;
    LocalFree(lpBuffer);
    return _msg;
}
#else
std::string GetLastMsg()
{
    std::string _ret = "no error";
    DWORD errorCode = GetLastError();
    if (errorCode == 0)
        return _ret;

    // map errorCode into a system defined error string

    DWORD dwFlags =
        FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER;
    LPCVOID lpSource = NULL;
    DWORD dwMessageID = errorCode;
    DWORD dwLanguageId = MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US);
    LPSTR lpBuffer;
    DWORD nSize = 0;
    va_list* Arguments = NULL;

    FormatMessage(
        dwFlags, lpSource, dwMessageID, dwLanguageId,
        (LPSTR)&lpBuffer, nSize, Arguments
    );
    printf("%s", lpBuffer);
    std::string _msg = lpBuffer;
    LocalFree(lpBuffer);
    return _msg;
}
#endif