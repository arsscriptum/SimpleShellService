//==============================================================================
//
//     helpers.h : helpers
//
//==============================================================================
//  Copyright (C) Guilaume Plante 2020 <cybercastor@icloud.com>
//==============================================================================

#ifndef __HELPERS_H__
#define __HELPERS_H__


#include <Windows.h>
#include <string>

const char* get_ini_path();
std::string get_exe_path();
std::string replace(const std::string& s, const std::string& from, const std::string& to);

#endif