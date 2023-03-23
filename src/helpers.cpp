
#include "stdafx.h"
#include <string>
using namespace std;
#include <algorithm> // for transform() in get_exe_path()
#define WIN32_LEAN_AND_MEAN
#define VC_EXTRALEAN
#include <Windows.h>


string replace(const string& s, const string& from, const string& to) {
    string r = s;
    int p = 0;
    while ((p = (int)r.find(from, p)) != string::npos) {
        r.replace(p, from.length(), to);
        p += (int)to.length();
    }
    return r;
}
string get_exe_path() { // returns path where executable is located
    string path = "";
#if defined(_WIN32)
    wchar_t wc[260] = { 0 };
    GetModuleFileNameW(NULL, wc, 260);
    wstring ws(wc);
    transform(ws.begin(), ws.end(), back_inserter(path), [](wchar_t c) { return (char)c; });
    path = replace(path, "\\", "/");
#elif defined(__linux__)
    char c[260];
    int length = (int)readlink("/proc/self/exe", c, 260);
    path = string(c, length > 0 ? length : 0);
#endif // Windows/Linux
    return path.substr(0, path.rfind('/') + 1);
}

const char* get_ini_path()
{
    string strExePath = get_exe_path();
    strExePath += "service.ini";
    char* iniPath = new char[MAX_PATH];
    strncpy(iniPath, strExePath.c_str(), MAX_PATH - 1);
    return iniPath;
}