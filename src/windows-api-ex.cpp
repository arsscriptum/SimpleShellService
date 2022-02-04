
//==============================================================================
//
//   windows-api-ex.cpp
//
//==============================================================================
//  Copyright (C) Guilaume Plante 2020 <cybercastor@icloud.com>
//==============================================================================


#include "stdafx.h"
#include "windows-api-ex.h"
#include "Shlwapi.h"
#include <winternl.h>
namespace C
{
	namespace Convert
	{



		int wstrlen(_TCHAR * wstr)
		{
			int l_idx = 0;
			while (((char *)wstr)[l_idx] != 0) l_idx += 2;
			return l_idx;
		}

		// Allocate char string and copy TCHAR->char->string 

		char * wstrdup(_TCHAR * wSrc)
		{
			int l_idx = 0;
			int l_len = wstrlen(wSrc);
			char * l_nstr = (char *)malloc(l_len);
			if (l_nstr)
			{
				do
				{
					l_nstr[l_idx] = (char)wSrc[l_idx];
					l_idx++;
				} while ((char)wSrc[l_idx] != 0);
			}
			l_nstr[l_idx] = 0;
			return l_nstr;
		}

		// allocate argn structure parallel to argv 
		// argn must be released 

		char ** allocate_argn(int argc, _TCHAR* argv[])
		{
			char ** l_argn = (char **)malloc(argc * sizeof(char *));
			for (int idx = 0; idx<argc; idx++)
			{
				l_argn[idx] = wstrdup(argv[idx]);
			}

			return l_argn;
		}

		// release argn and its content 

		void release_argn(int argc, char ** nargv)
		{
			for (int idx = 0; idx<argc; idx++)
			{
				free(nargv[idx]);
			}
			free(nargv);
		}

		void __TrimDecimalString(PWCHAR str)
		{
			int length = lstrlenW(str);
			for (; str[length - 1] == L'0'; length--)
			{
				str[length - 1] = L'\0';
			}

			if (str[length - 1] == L'.') str[length - 1] = L'\0';
		}

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

		LPWSTR Int32ToString(__int32 value, int base)
		{
			WCHAR buffer[20];
			_ltow_s(value, buffer, base);

			PWCHAR result = new WCHAR[lstrlenW(buffer) + 1];
			StrCpyW(result, buffer);

			return result;
		}
		LPWSTR UInt32ToString(unsigned __int32 value, int base)
		{
			WCHAR buffer[20];
			_ultow_s(value, buffer, base);

			PWCHAR result = new WCHAR[lstrlenW(buffer) + 1];
			StrCpyW(result, buffer);

			return result;
		}
		LPWSTR Int64ToString(__int64 value, int base)
		{
			WCHAR buffer[30];
			_i64tow_s(value, buffer, sizeof(buffer) / sizeof(WCHAR), base);

			PWCHAR result = new WCHAR[lstrlenW(buffer) + 1];
			StrCpyW(result, buffer);

			return result;
		}
		LPWSTR UInt64ToString(unsigned __int64 value, int base)
		{
			WCHAR buffer[30];
			_ui64tow_s(value, buffer, sizeof(buffer) / sizeof(WCHAR), base);

			PWCHAR result = new WCHAR[lstrlenW(buffer) + 1];
			StrCpyW(result, buffer);

			return result;
		}
		LPWSTR FloatToString(float value)
		{
			WCHAR buffer[50];
			swprintf_s(buffer, L"%f", value);

			__TrimDecimalString(buffer);
			PWCHAR result = new WCHAR[lstrlenW(buffer) + 1];
			StrCpyW(result, buffer);

			return result;
		}
		LPWSTR DoubleToString(double value)
		{
			WCHAR buffer[50];
			swprintf_s(buffer, L"%.20f", value);

			__TrimDecimalString(buffer);
			PWCHAR result = new WCHAR[lstrlenW(buffer) + 1];
			StrCpyW(result, buffer);

			return result;
		}

		__int32 StringToInt32(LPCWSTR str, int base)
		{
			return wcstol(str, NULL, base);
		}
		unsigned __int32 StringToUInt32(LPCWSTR str, int base)
		{
			return wcstoul(str, NULL, base);
		}
		__int64 StringToInt64(LPCWSTR str, int base)
		{
			return wcstoll(str, NULL, base);
		}
		unsigned __int64 StringToUInt64(LPCWSTR str, int base)
		{
			return wcstoull(str, NULL, base);
		}
		float StringToFloat(LPCWSTR str)
		{
			return wcstof(str, NULL);
		}
		double StringToDouble(LPCWSTR str)
		{
			return wcstod(str, NULL);
		}

		LPWSTR UInt32ToHexString(unsigned __int32 value)
		{
			LPWSTR buffer = UInt32ToString(value, 16);
			PWCHAR result = new WCHAR[11];

			StrCpyW(result, L"0x00000000");
			StrCpyW(&result[10 - lstrlenW(buffer)], buffer);
			delete[] buffer;

			return result;
		}
		LPWSTR UInt64ToHexString(unsigned __int64 value)
		{
			LPWSTR buffer = UInt64ToString(value, 16);
			PWCHAR result = new WCHAR[19];

			StrCpyW(result, L"0x0000000000000000");
			StrCpyW(&result[18 - lstrlenW(buffer)], buffer);
			delete[] buffer;

			return result;
		}
		LPWSTR BytesToHexView(LPBYTE data, DWORD length)
		{
			PWCHAR result = new WCHAR[((length - 1) / 16 + 1) * 79 + 1];

			for (DWORD i = 0, offset = 0; i < length; i += 16)
			{
				LPWSTR line = UInt32ToString(i, 16);
				StrCpyW(&result[offset], L"00000000");
				StrCpyW(&result[offset + 8 - lstrlenW(line)], line);
				StrCpyW(&result[offset + 8], L"h: ");

				delete[] line;
				offset += 11;

				for (DWORD j = 0; j < 16; j++)
				{
					if (i + j < length)
					{
						LPWSTR number = UInt32ToString(data[i + j], 16);
						StrCpyW(&result[offset], L"00");
						StrCpyW(&result[offset + 2 - lstrlenW(number)], number);
						result[offset + 2] = L' ';

						delete[] number;
					}
					else
					{
						result[offset] = L' ';
						result[offset + 1] = L' ';
						result[offset + 2] = L' ';
					}

					offset += 3;
				}

				result[offset++] = L';';
				result[offset++] = L' ';

				for (DWORD j = 0; j < 16; j++)
				{
					if (i + j < length)
					{
						result[offset++] = data[i + j] >= 32 && data[i + j] <= 126 ? (WCHAR)data[i + j] : L'.';
					}
					else
					{
						result[offset++] = L' ';
					}
				}

				StrCpyW(&result[offset], L"\r\n");
				offset += 2;
			}

			return result;
		}
	}

	namespace Random
	{
		LPBYTE Bytes(DWORD length)
		{
			HCRYPTPROV cryptProvider;
			if (!CryptAcquireContextW(&cryptProvider, NULL, MS_DEF_PROV_W, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT)) return NULL;

			LPBYTE result = new BYTE[length];
			if (!CryptGenRandom(cryptProvider, length, result))
			{
				delete[] result;
				result = NULL;
			}

			CryptReleaseContext(cryptProvider, 0);
			return result;
		}
		LPWSTR HexadecimalString(DWORD length)
		{
			LPWSTR result = NULL;

			LPBYTE data = Bytes(length);
			if (data)
			{
				result = new WCHAR[length + 1];
				result[length] = L'\0';

				for (DWORD i = 0; i < length; i++) result[i] = L"0123456789abcdef"[data[i] & 15];
				delete[] data;
			}

			return result;
		}
	}

	namespace Environment
	{
		LPWSTR GetCurrentUser()
		{
			WCHAR buffer[UNLEN + 1];
			DWORD size = sizeof(buffer) / sizeof(WCHAR);
			if (!GetUserNameW(buffer, &size)) return NULL;

			PWCHAR result = new WCHAR[size];
			StrCpyW(result, buffer);

			return result;
		}
		BOOL GetWindowsVersion(LPDWORD major, LPDWORD minor)
		{
			LPBYTE buffer = NULL;
			if (NetWkstaGetInfo(NULL, 100, &buffer) != NERR_Success) return FALSE;

			WKSTA_INFO_100* workstationInfo = (LPWKSTA_INFO_100)buffer;
			*major = workstationInfo->wki100_ver_major;
			*minor = workstationInfo->wki100_ver_minor;
			NetApiBufferFree(buffer);

			return TRUE;
		}
		Array<LPWSTR>* GetCommandLineArgs()
		{
			int count;
			LPWSTR *argv = CommandLineToArgvW(GetCommandLineW(), &count);

			Array<LPWSTR> *result = NULL;

			if (argv)
			{
				result = new Array<LPWSTR>(count);

				for (int i = 0; i < count; i++)
				{
					PWCHAR buffer = new WCHAR[lstrlenW(argv[i]) + 1];
					StrCpyW(buffer, argv[i]);
					result->Add(buffer);
				}

				LocalFree(argv);
			}

			return result;
		}
		LPWSTR GetTimeStamp(BOOL useFileFormat)
		{
			time_t currentTime = time(NULL);
			struct tm *timeInfo = localtime(&currentTime);

			WCHAR buffer[50];
			DWORD size = (DWORD)wcsftime(buffer, sizeof(buffer) / sizeof(WCHAR), useFileFormat ? L"%Y-%m-%d %H.%M.%S" : L"%Y-%m-%d %H:%M:%S", timeInfo);

			PWCHAR result = new WCHAR[size + 1];
			StrCpyW(result, buffer);

			return result;
		}
	}

	namespace Path
	{
		LPWSTR Combine(LPCWSTR path1, LPCWSTR path2)
		{
			WCHAR buffer[MAX_PATH + 1];
			if (!PathCombineW(buffer, path1, path2)) return FALSE;

			PWCHAR result = new WCHAR[lstrlenW(buffer) + 1];
			StrCpyW(result, buffer);

			return result;
		}
		LPWSTR GetDirectoryName(LPCWSTR path)
		{
			PWCHAR buffer = new WCHAR[lstrlenW(path) + 1];
			StrCpyW(buffer, path);

			LPWSTR result = NULL;

			if (PathRemoveFileSpecW(buffer))
			{
				result = new WCHAR[lstrlenW(buffer) + 1];
				StrCpyW(result, buffer);
			}
			delete[] buffer;

			return result;
		}
		LPWSTR GetFileName(LPCWSTR path)
		{
			LPWSTR buffer = PathFindFileNameW(path);

			PWCHAR result = new WCHAR[lstrlenW(buffer) + 1];
			StrCpyW(result, buffer);

			return result;
		}

		LPWSTR GetExecutablePath()
		{
			return GetModulePath(NULL);
		}
		LPWSTR GetStartupPath()
		{
			LPWSTR buffer = GetExecutablePath();
			if (!buffer) return NULL;

			wcsrchr(buffer, L'\\')[0] = L'\0';

			PWCHAR result = new WCHAR[lstrlenW(buffer) + 1];
			StrCpyW(result, buffer);
			delete[] buffer;

			return result;
		}
		LPWSTR GetModulePath(HMODULE module)
		{
			WCHAR buffer[MAX_PATH + 1];
			if (!SUCCEEDED(GetModuleFileNameW(module, buffer, MAX_PATH + 1))) return NULL;

			PWCHAR result = new WCHAR[lstrlenW(buffer) + 1];
			StrCpyW(result, buffer);

			return result;
		}
		LPWSTR GetTempFolderPath()
		{
			WCHAR buffer[MAX_PATH + 1];
			DWORD size = GetTempPathW(MAX_PATH + 1, buffer);

			if (buffer[size - 1] == L'\\') buffer[size - 1] = L'\0';

			PWCHAR result = new WCHAR[lstrlenW(buffer) + 1];
			StrCpyW(result, buffer);

			return result;
		}
		LPWSTR GetFolderPath(SpecialFolder folder)
		{
			WCHAR buffer[MAX_PATH + 1];
			if (!SUCCEEDED(SHGetFolderPathW(NULL, (int)folder, NULL, SHGFP_TYPE::SHGFP_TYPE_CURRENT, buffer))) return NULL;

			PWCHAR result = new WCHAR[lstrlenW(buffer) + 1];
			StrCpyW(result, buffer);

			return result;
		}
	}

	namespace Directory
	{
		Array<LPWSTR>* __GetFilesOrDirectories(LPCWSTR path, BOOL getFiles)
		{
			int length = lstrlenW(path);
			PWCHAR search = new WCHAR[length + 3];
			StrCpyW(search, path);

			if (path[length - 1] != L'\\') StrCatW(search, L"\\");
			StrCatW(search, L"*");

			WIN32_FIND_DATAW data;
			HANDLE find = FindFirstFileW(search, &data);
			delete[] search;

			if (find != INVALID_HANDLE_VALUE)
			{
				Array<LPWSTR> *files = new Array<LPWSTR>();

				do
				{
					if (StrCmpW(data.cFileName, L".") && StrCmpW(data.cFileName, L".."))
					{
						if (((data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY) ^ getFiles)
						{
							PWCHAR name = new WCHAR[lstrlenW(data.cFileName) + 1];
							StrCpyW(name, data.cFileName);

							files->Add(name);
						}
					}
				}
				while (FindNextFileW(find, &data));
				FindClose(find);

				return files;
			}

			return NULL;
		}

		BOOL Exists(LPCWSTR path)
		{
			DWORD attributes = GetFileAttributesW(path);
			return attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY;
		}
		Array<LPWSTR>* GetDirectories(LPCWSTR path)
		{
			return __GetFilesOrDirectories(path, FALSE);
		}
		Array<LPWSTR>* GetFiles(LPCWSTR path)
		{
			return __GetFilesOrDirectories(path, TRUE);
		}
	}

	namespace File
	{
		BOOL Exists(LPCWSTR path)
		{
			DWORD attributes = GetFileAttributesW(path);
			return attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_DIRECTORY) != FILE_ATTRIBUTE_DIRECTORY;
		}
		LPBYTE Read(LPCWSTR path, LPDWORD bytesRead)
		{
			HANDLE file = CreateFileW(path, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
			if (file == INVALID_HANDLE_VALUE) return NULL;

			LPBYTE result = NULL;

			DWORD size = GetFileSize(file, NULL);
			if (size != INVALID_FILE_SIZE)
			{
				result = new BYTE[size];

				if (ReadFile(file, result, size, NULL, NULL))
				{
					*bytesRead = size;
				}
				else
				{
					delete[] result;
					result = NULL;
				}
			}

			CloseHandle(file);
			return result;
		}
		BOOL Write(LPCWSTR path, LPBYTE data, DWORD length)
		{
			HANDLE file = CreateFileW(path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
			if (file == INVALID_HANDLE_VALUE) return NULL;

			BOOL result = WriteFile(file, data, length, NULL, NULL);
			CloseHandle(file);

			return result;
		}
	}

	namespace Registry
	{
		HKEY __OpenKey(HKEY hive, LPCWSTR subkey, REGSAM desiredAccess)
		{
			HKEY key;
			if (RegOpenKeyExW(hive, subkey, 0, desiredAccess, &key) != ERROR_SUCCESS) return NULL;

			return key;
		}
		BOOL __QueryKey(HKEY hive, LPDWORD subKeyCount, LPDWORD valueCount, LPDWORD maxKeyNameLength, LPDWORD maxValueNameLength)
		{
			WCHAR achClass[MAX_PATH] = L"";
			DWORD classNameSize = MAX_PATH;
			DWORD maxClassSize;
			DWORD maxValueLength;
			DWORD securityDescriptor;
			FILETIME lastWriteTime;

			return RegQueryInfoKeyW(hive, achClass, &classNameSize, NULL, subKeyCount, maxKeyNameLength, &maxClassSize, valueCount, maxValueNameLength, &maxValueLength, &securityDescriptor, &lastWriteTime) == ERROR_SUCCESS;
		}

		LPWSTR GetValueString(HKEY hive, LPCWSTR subkey, LPCWSTR name)
		{
			HKEY key = __OpenKey(hive, subkey, KEY_QUERY_VALUE);
			if (!key) return NULL;

			LPWSTR result = NULL;

			WCHAR buffer[1024];
			DWORD size = sizeof(buffer);
			LSTATUS status = RegQueryValueExW(key, name, 0, NULL, (LPBYTE)buffer, &size);

			if (status == ERROR_SUCCESS)
			{
				result = new WCHAR[(size - 1) / 2 + 1];
				StrCpyW(result, buffer);
			}

			RegCloseKey(key);
			return result;
		}
		BOOL GetValueDword(HKEY hive, LPCWSTR subkey, LPCWSTR name, PDWORD value)
		{
			HKEY key = __OpenKey(hive, subkey, KEY_QUERY_VALUE);
			if (!key) return FALSE;

			DWORD size = sizeof(DWORD);
			BOOL result = RegQueryValueExW(key, name, 0, NULL, (LPBYTE)value, &size) == ERROR_SUCCESS;

			RegCloseKey(key);
			return result;
		}
		BOOL SetValueString(HKEY hive, LPCWSTR subkey, LPCWSTR name, LPCWSTR value, BOOL isExpandedString)
		{
			HKEY key = __OpenKey(hive, subkey, KEY_ALL_ACCESS);
			if (!key) return FALSE;

			BOOL result = RegSetValueExW(key, name, 0, isExpandedString ? REG_EXPAND_SZ : REG_SZ, (LPBYTE)value, (lstrlenW(value) + 1) * sizeof(WCHAR)) == ERROR_SUCCESS;

			RegCloseKey(key);
			return result;
		}
		BOOL SetValueDword(HKEY hive, LPCWSTR subkey, LPCWSTR name, DWORD value)
		{
			HKEY key = __OpenKey(hive, subkey, KEY_ALL_ACCESS);
			if (!key) return FALSE;

			BOOL result = RegSetValueExW(key, name, 0, REG_DWORD, (LPBYTE)&value, sizeof(value)) == ERROR_SUCCESS;

			RegCloseKey(key);
			return result;
		}
		BOOL DeleteValue(HKEY hive, LPCWSTR subkey, LPCWSTR name)
		{
			HKEY key = __OpenKey(hive, subkey, KEY_ALL_ACCESS);
			if (!key) return FALSE;

			BOOL result = RegDeleteValueW(key, name) == ERROR_SUCCESS;

			RegCloseKey(key);
			return result;
		}

		BOOL CreateKey(HKEY hive, LPCWSTR subkey, LPCWSTR name)
		{
			HKEY key = __OpenKey(hive, subkey, KEY_ALL_ACCESS);
			if (!key) return FALSE;

			HKEY createdKey;
			RegCreateKeyW(key, name, &createdKey);
			BOOL result = createdKey != NULL;

			RegCloseKey(key);
			RegCloseKey(createdKey);
			return result;
		}
		BOOL DeleteKey(HKEY hive, LPCWSTR subkey, LPCWSTR name)
		{
			HKEY key = __OpenKey(hive, subkey, KEY_ALL_ACCESS);
			if (!key) return FALSE;

			BOOL result = RegDeleteKeyW(key, name) == ERROR_SUCCESS;

			RegCloseKey(key);
			return result;
		}
		Array<LPWSTR>* GetKeyNames(HKEY hive, LPCWSTR subkey)
		{
			Array<LPWSTR> *result = NULL;

			if (hive && subkey)
			{
				HKEY key = __OpenKey(hive, subkey, KEY_READ);
				if (key)
				{
					DWORD subKeyCount;
					DWORD valueCount;
					DWORD maxKeyNameLength;
					DWORD maxValueNameLength;

					if (__QueryKey(key, &subKeyCount, &valueCount, &maxKeyNameLength, &maxValueNameLength))
					{
						PWCHAR name = new WCHAR[maxKeyNameLength + 1];
						result = new Array<LPWSTR>(subKeyCount);
						int resultIndex = 0;

						for (DWORD i = 0; i < subKeyCount; i++)
						{
							DWORD nameSize = maxKeyNameLength + 1;
							name[0] = L'\0';

							if (RegEnumKeyExW(key, i, name, &nameSize, NULL, NULL, NULL, NULL) != ERROR_SUCCESS) continue;

							PWCHAR buffer = new WCHAR[nameSize + 1];
							StrCpyW(buffer, name);
							result->Add(buffer);
						}

						delete[] name;
					}

					RegCloseKey(key);
				}
			}

			return result;
		}
		Array<LPWSTR>* GetValueNames(HKEY hive, LPCWSTR subkey)
		{
			Array<LPWSTR> *result = NULL;

			if (hive && subkey)
			{
				HKEY key = __OpenKey(hive, subkey, KEY_READ);
				if (key)
				{
					DWORD subKeyCount;
					DWORD valueCount;
					DWORD maxKeyNameLength;
					DWORD maxValueNameLength;

					if (__QueryKey(key, &subKeyCount, &valueCount, &maxKeyNameLength, &maxValueNameLength))
					{
						PWCHAR name = new WCHAR[maxValueNameLength + 1];
						result = new Array<LPWSTR>(valueCount);
						int resultIndex = 0;

						for (DWORD i = 0; i < valueCount; i++)
						{
							DWORD nameSize = maxValueNameLength + 1;
							name[0] = L'\0';

							if (RegEnumValueW(key, i, name, &nameSize, NULL, NULL, NULL, NULL) != ERROR_SUCCESS) continue;
							if (RegQueryValueExW(key, name, 0, NULL, NULL, NULL) != ERROR_SUCCESS) continue;

							PWCHAR buffer = new WCHAR[nameSize + 1];
							StrCpyW(buffer, name);
							result->Add(buffer);
						}

						delete[] name;
					}

					RegCloseKey(key);
				}
			}

			return result;
		}
	}

	namespace Process
	{
		typedef NTSTATUS(NTAPI *__NtQueryInformationProcess)(HANDLE ProcessHandle, DWORD ProcessInformationClass, PVOID ProcessInformation, DWORD ProcessInformationLength, PDWORD ReturnLength);

		//FEATURE: Other privileges than SE_DEBUG_NAME
		//TODO: Test if it works
		//TODO: Bug: Returns true even if failed
		/*BOOL EnableDebugPrivilege()
		{
			BOOL result = FALSE;
			HANDLE token = NULL;
			HANDLE process = GetCurrentProcess();

			if (process && OpenProcessToken(process, TOKEN_ADJUST_PRIVILEGES, &token))
			{
				LUID luid;
				if (LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &luid))
				{
					TOKEN_PRIVILEGES tokenPrivileges;
					tokenPrivileges.PrivilegeCount = 1;
					tokenPrivileges.Privileges[0].Luid = luid;
					tokenPrivileges.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

					result = AdjustTokenPrivileges(token, FALSE, &tokenPrivileges, sizeof(tokenPrivileges), NULL, NULL);
				}
			}

			if (process) CloseHandle(process);
			if (token) CloseHandle(token);
			return result;
		}*/
		LPCWSTR GetIntegrityLevelName(DWORD integrityLevel)
		{
			if (integrityLevel >= SECURITY_MANDATORY_UNTRUSTED_RID && integrityLevel < SECURITY_MANDATORY_LOW_RID) return L"Untrusted";
			else if (integrityLevel >= SECURITY_MANDATORY_LOW_RID && integrityLevel < SECURITY_MANDATORY_MEDIUM_RID) return L"Low";
			else if (integrityLevel >= SECURITY_MANDATORY_MEDIUM_RID && integrityLevel < SECURITY_MANDATORY_HIGH_RID) return L"Medium";
			else if (integrityLevel >= SECURITY_MANDATORY_HIGH_RID && integrityLevel < SECURITY_MANDATORY_SYSTEM_RID) return L"High";
			else if (integrityLevel >= SECURITY_MANDATORY_SYSTEM_RID) return L"System";
			else return NULL;
		}

		DWORD GetProcessIdByName(LPCWSTR name)
		{
			HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
			if (snapshot == INVALID_HANDLE_VALUE) return 0;

			PROCESSENTRY32W process;
			ZeroMemory(&process, sizeof(process));
			process.dwSize = sizeof(process);

			DWORD processId = 0;
			if (Process32FirstW(snapshot, &process))
			{
				do
				{
					if (!StrCmpIW(process.szExeFile, name))
					{
						processId = process.th32ProcessID;
						break;
					}
				}
				while (Process32NextW(snapshot, &process));
			}

			CloseHandle(snapshot);
			return processId;
		}
		BOOL CreateProcessWithIntegrity(LPCWSTR commandLine, DWORD integrityLevel, LPDWORD processId)
		{
			BOOL result = FALSE;
			HANDLE process = GetCurrentProcess();
			HANDLE token = NULL;
			HANDLE newToken = NULL;
			SID_IDENTIFIER_AUTHORITY authority = SECURITY_MANDATORY_LABEL_AUTHORITY;
			PSID integritySid = NULL;
			TOKEN_MANDATORY_LABEL tokenMandatoryLabel = { 0 };
			STARTUPINFOW startupInfo = { sizeof(startupInfo) };
			PROCESS_INFORMATION processInformation = { 0 };

			if (OpenProcessToken(process, TOKEN_DUPLICATE | TOKEN_QUERY | TOKEN_ADJUST_DEFAULT | TOKEN_ASSIGN_PRIMARY, &token))
			{
				if (DuplicateTokenEx(token, 0, NULL, SecurityImpersonation, TokenPrimary, &newToken))
				{
					if (AllocateAndInitializeSid(&authority, 1, integrityLevel, 0, 0, 0, 0, 0, 0, 0, &integritySid))
					{
						tokenMandatoryLabel.Label.Attributes = SE_GROUP_INTEGRITY;
						tokenMandatoryLabel.Label.Sid = integritySid;

						if (SetTokenInformation(newToken, TokenIntegrityLevel, &tokenMandatoryLabel, (sizeof(tokenMandatoryLabel) + GetLengthSid(integritySid))))
						{
							WCHAR path[1024];
							StrCpyNW(path, commandLine, sizeof(path) / sizeof(WCHAR));

							result = CreateProcessAsUserW(newToken, NULL, path, NULL, NULL, FALSE, 0, NULL, NULL, &startupInfo, &processInformation);
						}
					}
				}
			}

			if (process) CloseHandle(process);
			if (token) CloseHandle(token);
			if (newToken) CloseHandle(newToken);
			if (integritySid) FreeSid(integritySid);
			if (processInformation.hProcess) CloseHandle(processInformation.hProcess);
			if (processInformation.hThread) CloseHandle(processInformation.hThread);

			*processId = processInformation.dwProcessId;
			return result;
		}

		LPWSTR GetProcessName(DWORD processId)
		{
			HANDLE process = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, processId);
			if (!process) return NULL;

			WCHAR buffer[MAX_PATH + 1];
			DWORD size = GetProcessImageFileNameW(process, buffer, sizeof(buffer) / sizeof(WCHAR));
			if (size == 0) return NULL;

			PWCHAR fileName = wcsrchr(buffer, L'\\');
			if (!fileName) return NULL;

			fileName++;
			PWCHAR result = new WCHAR[lstrlenW(fileName) + 1];
			StrCpyW(result, fileName);

			return result;
		}
		BOOL TerminateProcess(DWORD dwProcessId, UINT uExitCode, BOOL selfElevate)
		{
			if (selfElevate)
			{

				if (IsRunAsAdministrator())
				{
					std::cout << "The applicaiton is already running with admin privileges" << std::endl;
				}
				else
				{
					ElevateNow();
				}
			}
			DWORD dwDesiredAccess = PROCESS_TERMINATE;
			BOOL  bInheritHandle = FALSE;
			HANDLE hProcess = OpenProcess(dwDesiredAccess, bInheritHandle, dwProcessId);
			if (hProcess == NULL)
				return FALSE;

			BOOL result = ::TerminateProcess(hProcess, uExitCode);

			CloseHandle(hProcess);

			return result;
		}
		LPWSTR GetProcessCommandLine(DWORD processId)
		{
			HANDLE process = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, processId);
			LPWSTR result = NULL;

			HMODULE module = GetModuleHandleW(L"ntdll.dll");
			if (module)
			{
				LPVOID queryInformationProcess = (LPVOID)GetProcAddress(module, "NtQueryInformationProcess");
				if (queryInformationProcess)
				{
					PROCESS_BASIC_INFORMATION processBasicInformation;
					if (NT_SUCCESS(((__NtQueryInformationProcess)queryInformationProcess)(process, 0, &processBasicInformation, sizeof(processBasicInformation), NULL)))
					{
						PRTL_USER_PROCESS_PARAMETERS paramsAddress;
						if (ReadProcessMemory(process, &processBasicInformation.PebBaseAddress->ProcessParameters, &paramsAddress, sizeof(PVOID), NULL))
						{
							UNICODE_STRING commandLine;
							if (ReadProcessMemory(process, &(paramsAddress->CommandLine), &commandLine, sizeof(commandLine), NULL))
							{
								result = new WCHAR[commandLine.Length / 2 + 1];
								if (ReadProcessMemory(process, commandLine.Buffer, result, commandLine.Length, NULL))
								{
									result[commandLine.Length / 2] = L'\0';
								}
								else
								{
									delete[] result;
									result = NULL;
								}
							}
						}
					}
				}
			}

			CloseHandle(process);
			return result;
		}
		DWORD GetProcessIntegrityLevel(HANDLE process)
		{
			DWORD result = -1;
			HANDLE token;
			PTOKEN_MANDATORY_LABEL tokenMandatoryLabel = NULL;
			DWORD lengthNeeded;

			if (OpenProcessToken(process, TOKEN_QUERY, &token))
			{
				if (!GetTokenInformation(token, TOKEN_INFORMATION_CLASS::TokenIntegrityLevel, NULL, 0, &lengthNeeded))
				{
					if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
					{
						tokenMandatoryLabel = (PTOKEN_MANDATORY_LABEL)LocalAlloc(0, lengthNeeded);
						if (tokenMandatoryLabel)
						{
							if (GetTokenInformation(token, TOKEN_INFORMATION_CLASS::TokenIntegrityLevel, tokenMandatoryLabel, lengthNeeded, &lengthNeeded))
							{
								result = *GetSidSubAuthority(tokenMandatoryLabel->Label.Sid, (DWORD)(UCHAR)(*GetSidSubAuthorityCount(tokenMandatoryLabel->Label.Sid) - 1));
							}
						}
					}
				}
			}

			if (token) CloseHandle(token);
			if (tokenMandatoryLabel) LocalFree(tokenMandatoryLabel);
			return result;
		}
		DWORD GetParentProcessId(DWORD processId)
		{
			HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
			if (snapshot == INVALID_HANDLE_VALUE) return 0;

			PROCESSENTRY32 process;
			ZeroMemory(&process, sizeof(process));
			process.dwSize = sizeof(process);

			DWORD parentProcessId = 0;
			if (Process32First(snapshot, &process))
			{
				do
				{
					if (process.th32ProcessID == processId)
					{
						parentProcessId = process.th32ParentProcessID;
						break;
					}
				}
				while (Process32Next(snapshot, &process));
			}

			CloseHandle(snapshot);
			return parentProcessId;
		}
		Array<HWND>* GetProcessWindows(DWORD processID)
		{
			Array<HWND> *windows = new Array<HWND>();
			HWND hwnd = NULL;

			do
			{
				hwnd = FindWindowExW(NULL, hwnd, NULL, NULL);

				DWORD windowProcessId = 0;
				GetWindowThreadProcessId(hwnd, &windowProcessId);

				if (windowProcessId == processID) windows->Add(hwnd);
			}
			while (hwnd != NULL);

			return windows;
		}
		BOOL InjectDll(HANDLE process, LPCWSTR dllPath)
		{
			HMODULE module = GetModuleHandleW(L"kernel32.dll");
			if (!module) return FALSE;

			LPVOID loadLibraryAddress = (LPVOID)GetProcAddress(module, "LoadLibraryW");
			if (!loadLibraryAddress) return FALSE;

			LPVOID allocatedMemoryAddress = VirtualAllocEx(process, NULL, lstrlenW(dllPath) * sizeof(WCHAR), MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
			if (!allocatedMemoryAddress) return FALSE;

			if (!WriteProcessMemory(process, allocatedMemoryAddress, dllPath, lstrlenW(dllPath) * sizeof(WCHAR), NULL)) return FALSE;
			if (!CreateRemoteThread(process, NULL, NULL, (LPTHREAD_START_ROUTINE)loadLibraryAddress, allocatedMemoryAddress, 0, NULL)) return FALSE;

			return TRUE;
		}

		BOOL GetLogonSID(HANDLE hToken, PSID *ppsid)
		{
			BOOL bSuccess = FALSE;
			DWORD dwIndex;
			DWORD dwLength = 0;
			PTOKEN_GROUPS ptg = NULL;

			// Get required buffer size and allocate the TOKEN_GROUPS buffer.

			if (!GetTokenInformation(
				hToken,         // handle to the access token
				TokenGroups,    // get information about the token's groups 
				(LPVOID)ptg,   // pointer to TOKEN_GROUPS buffer
				0,              // size of buffer
				&dwLength       // receives required buffer size
			))
			{
				if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
					goto Cleanup;

				ptg = (PTOKEN_GROUPS)HeapAlloc(GetProcessHeap(),
					HEAP_ZERO_MEMORY, dwLength);

				if (ptg == NULL)
					goto Cleanup;
			}

			// Get the token group information from the access token.

			if (!GetTokenInformation(
				hToken,         // handle to the access token
				TokenGroups,    // get information about the token's groups 
				(LPVOID)ptg,   // pointer to TOKEN_GROUPS buffer
				dwLength,       // size of buffer
				&dwLength       // receives required buffer size
			))
			{
				goto Cleanup;
			}

			// Loop through the groups to find the logon SID.

			for (dwIndex = 0; dwIndex < ptg->GroupCount; dwIndex++)
				if ((ptg->Groups[dwIndex].Attributes & SE_GROUP_LOGON_ID)
					== SE_GROUP_LOGON_ID)
				{
					// Found the logon SID; make a copy of it.

					dwLength = GetLengthSid(ptg->Groups[dwIndex].Sid);
					*ppsid = (PSID)HeapAlloc(GetProcessHeap(),
						HEAP_ZERO_MEMORY, dwLength);
					if (*ppsid == NULL)
						goto Cleanup;
					if (!CopySid(dwLength, *ppsid, ptg->Groups[dwIndex].Sid))
					{
						HeapFree(GetProcessHeap(), 0, (LPVOID)*ppsid);
						goto Cleanup;
					}
					break;
				}

			bSuccess = TRUE;

		Cleanup:

			// Free the buffer for the token groups.

			if (ptg != NULL)
				HeapFree(GetProcessHeap(), 0, (LPVOID)ptg);

			return bSuccess;
		}

		VOID FreeLogonSID(PSID *ppsid)
		{
			HeapFree(GetProcessHeap(), 0, (LPVOID)*ppsid);
		}


		BOOL IsRunAsAdministrator()
		{
			BOOL fIsRunAsAdmin = FALSE;
			DWORD dwError = ERROR_SUCCESS;
			PSID pAdministratorsGroup = NULL;

			// Allocate and initialize a SID of the administrators group.
			SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;
			if (!AllocateAndInitializeSid(
				&NtAuthority,
				2,
				SECURITY_BUILTIN_DOMAIN_RID,
				DOMAIN_ALIAS_RID_ADMINS,
				0, 0, 0, 0, 0, 0,
				&pAdministratorsGroup))
			{
				dwError = GetLastError();
				goto Cleanup;
			}

			// Determine whether the SID of administrators group is enabled in 
			// the primary access token of the process.
			if (!CheckTokenMembership(NULL, pAdministratorsGroup, &fIsRunAsAdmin))
			{
				dwError = GetLastError();
				goto Cleanup;
			}

		Cleanup:
			// Centralized cleanup for all allocated resources.
			if (pAdministratorsGroup)
			{
				FreeSid(pAdministratorsGroup);
				pAdministratorsGroup = NULL;
			}

			// Throw the error if something failed in the function.
			if (ERROR_SUCCESS != dwError)
			{
				throw dwError;
			}

			return fIsRunAsAdmin;
		}

		BOOL AddAceToWindowStation(HWINSTA hwinsta, PSID psid)
		{
			ACCESS_ALLOWED_ACE   *pace = NULL;
			ACL_SIZE_INFORMATION aclSizeInfo;
			BOOL                 bDaclExist;
			BOOL                 bDaclPresent;
			BOOL                 bSuccess = FALSE;
			DWORD                dwNewAclSize;
			DWORD                dwSidSize = 0;
			DWORD                dwSdSizeNeeded;
			PACL                 pacl;
			PACL                 pNewAcl = NULL;
			PSECURITY_DESCRIPTOR psd = NULL;
			PSECURITY_DESCRIPTOR psdNew = NULL;
			PVOID                pTempAce;
			SECURITY_INFORMATION si = DACL_SECURITY_INFORMATION;
			unsigned int         i;

			__try
			{
				// Obtain the DACL for the window station.

				if (!GetUserObjectSecurity(
					hwinsta,
					&si,
					psd,
					dwSidSize,
					&dwSdSizeNeeded)
					)
					if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
					{
						psd = (PSECURITY_DESCRIPTOR)HeapAlloc(
							GetProcessHeap(),
							HEAP_ZERO_MEMORY,
							dwSdSizeNeeded);

						if (psd == NULL)
							__leave;

						psdNew = (PSECURITY_DESCRIPTOR)HeapAlloc(
							GetProcessHeap(),
							HEAP_ZERO_MEMORY,
							dwSdSizeNeeded);

						if (psdNew == NULL)
							__leave;

						dwSidSize = dwSdSizeNeeded;

						if (!GetUserObjectSecurity(
							hwinsta,
							&si,
							psd,
							dwSidSize,
							&dwSdSizeNeeded)
							)
							__leave;
					}
					else
						__leave;

				// Create a new DACL.

				if (!InitializeSecurityDescriptor(
					psdNew,
					SECURITY_DESCRIPTOR_REVISION)
					)
					__leave;

				// Get the DACL from the security descriptor.

				if (!GetSecurityDescriptorDacl(
					psd,
					&bDaclPresent,
					&pacl,
					&bDaclExist)
					)
					__leave;

				// Initialize the ACL.

				ZeroMemory(&aclSizeInfo, sizeof(ACL_SIZE_INFORMATION));
				aclSizeInfo.AclBytesInUse = sizeof(ACL);

				// Call only if the DACL is not NULL.

				if (pacl != NULL)
				{
					// get the file ACL size info
					if (!GetAclInformation(
						pacl,
						(LPVOID)&aclSizeInfo,
						sizeof(ACL_SIZE_INFORMATION),
						AclSizeInformation)
						)
						__leave;
				}

				// Compute the size of the new ACL.

				dwNewAclSize = aclSizeInfo.AclBytesInUse +
					(2 * sizeof(ACCESS_ALLOWED_ACE)) + (2 * GetLengthSid(psid)) -
					(2 * sizeof(DWORD));

				// Allocate memory for the new ACL.

				pNewAcl = (PACL)HeapAlloc(
					GetProcessHeap(),
					HEAP_ZERO_MEMORY,
					dwNewAclSize);

				if (pNewAcl == NULL)
					__leave;

				// Initialize the new DACL.

				if (!InitializeAcl(pNewAcl, dwNewAclSize, ACL_REVISION))
					__leave;

				// If DACL is present, copy it to a new DACL.

				if (bDaclPresent)
				{
					// Copy the ACEs to the new ACL.
					if (aclSizeInfo.AceCount)
					{
						for (i = 0; i < aclSizeInfo.AceCount; i++)
						{
							// Get an ACE.
							if (!GetAce(pacl, i, &pTempAce))
								__leave;

							// Add the ACE to the new ACL.
							if (!AddAce(
								pNewAcl,
								ACL_REVISION,
								MAXDWORD,
								pTempAce,
								((PACE_HEADER)pTempAce)->AceSize)
								)
								__leave;
						}
					}
				}

				// Add the first ACE to the window station.

				pace = (ACCESS_ALLOWED_ACE *)HeapAlloc(
					GetProcessHeap(),
					HEAP_ZERO_MEMORY,
					sizeof(ACCESS_ALLOWED_ACE) + GetLengthSid(psid) -
					sizeof(DWORD));

				if (pace == NULL)
					__leave;

				pace->Header.AceType = ACCESS_ALLOWED_ACE_TYPE;
				pace->Header.AceFlags = CONTAINER_INHERIT_ACE |
					INHERIT_ONLY_ACE | OBJECT_INHERIT_ACE;
				pace->Header.AceSize = LOWORD(sizeof(ACCESS_ALLOWED_ACE) +
					GetLengthSid(psid) - sizeof(DWORD));
				pace->Mask = GENERIC_ACCESS;

				if (!CopySid(GetLengthSid(psid), &pace->SidStart, psid))
					__leave;

				if (!AddAce(
					pNewAcl,
					ACL_REVISION,
					MAXDWORD,
					(LPVOID)pace,
					pace->Header.AceSize)
					)
					__leave;

				// Add the second ACE to the window station.

				pace->Header.AceFlags = NO_PROPAGATE_INHERIT_ACE;
				pace->Mask = WINSTA_ALL;

				if (!AddAce(
					pNewAcl,
					ACL_REVISION,
					MAXDWORD,
					(LPVOID)pace,
					pace->Header.AceSize)
					)
					__leave;

				// Set a new DACL for the security descriptor.

				if (!SetSecurityDescriptorDacl(
					psdNew,
					TRUE,
					pNewAcl,
					FALSE)
					)
					__leave;

				// Set the new security descriptor for the window station.

				if (!SetUserObjectSecurity(hwinsta, &si, psdNew))
					__leave;

				// Indicate success.

				bSuccess = TRUE;
			}
			__finally
			{
				// Free the allocated buffers.

				if (pace != NULL)
					HeapFree(GetProcessHeap(), 0, (LPVOID)pace);

				if (pNewAcl != NULL)
					HeapFree(GetProcessHeap(), 0, (LPVOID)pNewAcl);

				if (psd != NULL)
					HeapFree(GetProcessHeap(), 0, (LPVOID)psd);

				if (psdNew != NULL)
					HeapFree(GetProcessHeap(), 0, (LPVOID)psdNew);
			}

			return bSuccess;

		}

		BOOL AddAceToDesktop(HDESK hdesk, PSID psid)
		{
			ACL_SIZE_INFORMATION aclSizeInfo;
			BOOL                 bDaclExist;
			BOOL                 bDaclPresent;
			BOOL                 bSuccess = FALSE;
			DWORD                dwNewAclSize;
			DWORD                dwSidSize = 0;
			DWORD                dwSdSizeNeeded;
			PACL                 pacl;
			PACL                 pNewAcl = NULL;
			PSECURITY_DESCRIPTOR psd = NULL;
			PSECURITY_DESCRIPTOR psdNew = NULL;
			PVOID                pTempAce;
			SECURITY_INFORMATION si = DACL_SECURITY_INFORMATION;
			unsigned int         i;

			__try
			{
				// Obtain the security descriptor for the desktop object.

				if (!GetUserObjectSecurity(
					hdesk,
					&si,
					psd,
					dwSidSize,
					&dwSdSizeNeeded))
				{
					if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
					{
						psd = (PSECURITY_DESCRIPTOR)HeapAlloc(
							GetProcessHeap(),
							HEAP_ZERO_MEMORY,
							dwSdSizeNeeded);

						if (psd == NULL)
							__leave;

						psdNew = (PSECURITY_DESCRIPTOR)HeapAlloc(
							GetProcessHeap(),
							HEAP_ZERO_MEMORY,
							dwSdSizeNeeded);

						if (psdNew == NULL)
							__leave;

						dwSidSize = dwSdSizeNeeded;

						if (!GetUserObjectSecurity(
							hdesk,
							&si,
							psd,
							dwSidSize,
							&dwSdSizeNeeded)
							)
							__leave;
					}
					else
						__leave;
				}

				// Create a new security descriptor.

				if (!InitializeSecurityDescriptor(
					psdNew,
					SECURITY_DESCRIPTOR_REVISION)
					)
					__leave;

				// Obtain the DACL from the security descriptor.

				if (!GetSecurityDescriptorDacl(
					psd,
					&bDaclPresent,
					&pacl,
					&bDaclExist)
					)
					__leave;

				// Initialize.

				ZeroMemory(&aclSizeInfo, sizeof(ACL_SIZE_INFORMATION));
				aclSizeInfo.AclBytesInUse = sizeof(ACL);

				// Call only if NULL DACL.

				if (pacl != NULL)
				{
					// Determine the size of the ACL information.

					if (!GetAclInformation(
						pacl,
						(LPVOID)&aclSizeInfo,
						sizeof(ACL_SIZE_INFORMATION),
						AclSizeInformation)
						)
						__leave;
				}

				// Compute the size of the new ACL.

				dwNewAclSize = aclSizeInfo.AclBytesInUse +
					sizeof(ACCESS_ALLOWED_ACE) +
					GetLengthSid(psid) - sizeof(DWORD);

				// Allocate buffer for the new ACL.

				pNewAcl = (PACL)HeapAlloc(
					GetProcessHeap(),
					HEAP_ZERO_MEMORY,
					dwNewAclSize);

				if (pNewAcl == NULL)
					__leave;

				// Initialize the new ACL.

				if (!InitializeAcl(pNewAcl, dwNewAclSize, ACL_REVISION))
					__leave;

				// If DACL is present, copy it to a new DACL.

				if (bDaclPresent)
				{
					// Copy the ACEs to the new ACL.
					if (aclSizeInfo.AceCount)
					{
						for (i = 0; i < aclSizeInfo.AceCount; i++)
						{
							// Get an ACE.
							if (!GetAce(pacl, i, &pTempAce))
								__leave;

							// Add the ACE to the new ACL.
							if (!AddAce(
								pNewAcl,
								ACL_REVISION,
								MAXDWORD,
								pTempAce,
								((PACE_HEADER)pTempAce)->AceSize)
								)
								__leave;
						}
					}
				}

				// Add ACE to the DACL.

				if (!AddAccessAllowedAce(
					pNewAcl,
					ACL_REVISION,
					DESKTOP_ALL,
					psid)
					)
					__leave;

				// Set new DACL to the new security descriptor.

				if (!SetSecurityDescriptorDacl(
					psdNew,
					TRUE,
					pNewAcl,
					FALSE)
					)
					__leave;

				// Set the new security descriptor for the desktop object.

				if (!SetUserObjectSecurity(hdesk, &si, psdNew))
					__leave;

				// Indicate success.

				bSuccess = TRUE;
			}
			__finally
			{
				// Free buffers.

				if (pNewAcl != NULL)
					HeapFree(GetProcessHeap(), 0, (LPVOID)pNewAcl);

				if (psd != NULL)
					HeapFree(GetProcessHeap(), 0, (LPVOID)psd);

				if (psdNew != NULL)
					HeapFree(GetProcessHeap(), 0, (LPVOID)psdNew);
			}

			return bSuccess;
		}




		BOOL SetPrivilege(
			HANDLE hToken,          // access token handle
			LPCTSTR lpszPrivilege,  // name of privilege to enable/disable
			BOOL bEnablePrivilege   // to enable or disable privilege
		)
		{
			TOKEN_PRIVILEGES tp;
			LUID luid;

			if (!LookupPrivilegeValue(
				NULL,            // lookup privilege on local system
				lpszPrivilege,   // privilege to lookup 
				&luid))        // receives LUID of privilege
			{
				PRINT_OUT(_T("LookupPrivilegeValue error: %u\n"), GetLastError());
				return FALSE;
			}

			tp.PrivilegeCount = 1;
			tp.Privileges[0].Luid = luid;
			if (bEnablePrivilege)
				tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
			else
				tp.Privileges[0].Attributes = 0;

			// Enable the privilege or disable all privileges.

			if (!AdjustTokenPrivileges(
				hToken,
				FALSE,
				&tp,
				sizeof(TOKEN_PRIVILEGES),
				(PTOKEN_PRIVILEGES)NULL,
				(PDWORD)NULL))
			{
				PRINT_OUT(_T("AdjustTokenPrivileges error: %u\n"), GetLastError());
				return FALSE;
			}

			if (GetLastError() == ERROR_NOT_ALL_ASSIGNED)

			{
				PRINT_OUT(_T("The token does not have the specified privilege. \n"));
				return FALSE;
			}

			return TRUE;
		}

		DWORD GetCurrentSessionId()
		{
			WTS_SESSION_INFO *pSessionInfo;
			DWORD n_sessions = 0;
			BOOL ok = WTSEnumerateSessions(WTS_CURRENT_SERVER, 0, 1, &pSessionInfo, &n_sessions);
			if (!ok)
				return 0;

			DWORD SessionId = 0;

			for (DWORD i = 0; i < n_sessions; ++i)
			{
				if (pSessionInfo[i].State == WTSActive)
				{
					SessionId = pSessionInfo[i].SessionId;
					break;
				}
			}

			WTSFreeMemory(pSessionInfo);
			return SessionId;
		}

		BOOL CreateInteractiveProcess(TCHAR* pszCommandLine, bool selfElevate)
		{
			DWORD dwError = ERROR_SUCCESS;

			LPVOID lpvEnv = NULL;
			TCHAR szUserProfileDir[MAX_PATH];
			DWORD cchUserProfileDir = ARRAYSIZE(szUserProfileDir);
			STARTUPINFO si = { sizeof(si) };
			PROCESS_INFORMATION pi = { 0 };

			DWORD dwWaitResult;
			DWORD dwSessionId = GetCurrentSessionId();
			if (dwSessionId == 0)    // no-one logged in
			{
				PRINT_OUT(TEXT("GetCurrentSessionId failed (%d).\n"), GetLastError());
				return false;
			}

			RevertToSelf();
			HANDLE hToken = NULL;
			BOOL goodToken = WTSQueryUserToken(dwSessionId, &hToken);
			if (!goodToken)
			{
				DWORD err = GetLastError();
				PRINT_OUT(TEXT("WTSQueryUserToken failed (%d).\n"), GetLastError());

				if (!OpenThreadToken(GetCurrentThread(), TOKEN_ALL_ACCESS, TRUE, &hToken))
				{
					if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ALL_ACCESS, &hToken)) {
						return FALSE;
					}
				}
			}

			if (selfElevate)
			{

				if (IsRunAsAdministrator())
				{
					std::cout << "The applicaiton is already running with admin privileges" << std::endl;
				}
				else
				{
					ElevateNow();
				}
			}

			BOOL bSeAssignPrimaryTokenPrivilege = SetPrivilege(hToken, TEXT("SeAssignPrimaryTokenPrivilege"), TRUE);
			BOOL bSeImpersonatePrivilege = SetPrivilege(hToken, TEXT("SeImpersonatePrivilege"), TRUE);
			BOOL bSeCreateTokenPrivilege = SetPrivilege(hToken, TEXT("SeCreateTokenPrivilege"), TRUE);
			BOOL bSeTcbPrivilege = SetPrivilege(hToken, TEXT("SeTcbPrivilege"), TRUE);
			BOOL bSeIncreaseQuotaPrivilege = SetPrivilege(hToken, TEXT("SeIncreaseQuotaPrivilege"), TRUE);

			if (!bSeAssignPrimaryTokenPrivilege ||
				!bSeImpersonatePrivilege ||
				!bSeCreateTokenPrivilege ||
				!bSeTcbPrivilege ||
				!bSeIncreaseQuotaPrivilege)
			{
				DWORD err = GetLastError();
				PRINT_OUT(TEXT("User privilege problem (%d).\n"), err);
			}

			// Run the command line in the session that we found by using the default 
			// values for working directory and desktop.

			// This creates the default environment block for the user.
			if (!CreateEnvironmentBlock(&lpvEnv, hToken, TRUE))
			{
				dwError = GetLastError();
				goto Cleanup;
			}

			// Retrieve the path to the root directory of the user's profile.
			if (!GetUserProfileDirectory(hToken, szUserProfileDir,
				&cchUserProfileDir))
			{
				dwError = GetLastError();
				goto Cleanup;
			}

			// Specify that the process runs in the interactive desktop.
			si.lpDesktop = TEXT("winsta0\\default");
			
			// Launch the process.
			if (!CreateProcessAsUser(hToken, NULL, pszCommandLine, NULL, NULL, FALSE,
				CREATE_UNICODE_ENVIRONMENT, lpvEnv, szUserProfileDir, &si, &pi))
			{
				dwError = GetLastError();
				goto Cleanup;
			}

			// Wait for the exit of the process.
			WaitForSingleObject(pi.hProcess, INFINITE);


		Cleanup:

			// Centralized cleanup for all allocated resources.
			if (hToken)
			{
				CloseHandle(hToken);
				hToken = NULL;
			}
			if (lpvEnv)
			{
				DestroyEnvironmentBlock(lpvEnv);
				lpvEnv = NULL;
			}
			if (pi.hProcess)
			{
				CloseHandle(pi.hProcess);
				pi.hProcess = NULL;
			}
			if (pi.hThread)
			{
				CloseHandle(pi.hThread);
				pi.hThread = NULL;
			}

			// Set the last error if something failed in the function.
			if (dwError != ERROR_SUCCESS)
			{
				SetLastError(dwError);
				return FALSE;
			}
			else
			{
				return TRUE;
			}
		}

#define USE_NEW_INTERACTIVE_DESKTOP
		BOOL CreateInteractiveProcessForUser(
			LPTSTR lpszUsername,    // client to log on
			LPTSTR lpszDomain,      // domain of client's account
			LPTSTR lpszPassword,    // client's password
			LPTSTR lpCommandLine    // command line to execute
		)
		{
			HANDLE      hToken;
			HDESK       hdesk = NULL;
			HWINSTA     hwinsta = NULL, hwinstaSave = NULL;
			PSID pSid = NULL;
			BOOL bResult = FALSE;
			DWORD dwError = ERROR_SUCCESS;
			LPVOID lpvEnv = NULL;
			TCHAR szUserProfileDir[MAX_PATH];
			DWORD cchUserProfileDir = ARRAYSIZE(szUserProfileDir);
			STARTUPINFO si = { sizeof(si) };
			PROCESS_INFORMATION pi = { 0 };

			DWORD dwWaitResult;
			ZeroMemory(&si, sizeof(si));
			si.cb = sizeof(si);
			ZeroMemory(&pi, sizeof(pi));
			// Log the client on to the local computer.

			if (!LogonUser(lpszUsername, lpszDomain, lpszPassword, LOGON32_LOGON_INTERACTIVE, LOGON32_PROVIDER_DEFAULT, &hToken)) {
				return FALSE;
			}

			// Save a handle to the caller's current window station.

			if ((hwinstaSave = GetProcessWindowStation()) == NULL) {
				return FALSE;
			}

			// Run the command line in the session that we found by using the default 
			// values for working directory and desktop.

			// This creates the default environment block for the user.
			if (!CreateEnvironmentBlock(&lpvEnv, hToken, TRUE))
			{
				dwError = GetLastError();
				return FALSE;
			}

			// Retrieve the path to the root directory of the user's profile.
			if (!GetUserProfileDirectory(hToken, szUserProfileDir, &cchUserProfileDir))
			{
				dwError = GetLastError();
				return FALSE;
			}

			BOOL bSeAssignPrimaryTokenPrivilege = SetPrivilege(hToken, TEXT("SeAssignPrimaryTokenPrivilege"), TRUE);
			BOOL bSeImpersonatePrivilege = SetPrivilege(hToken, TEXT("SeImpersonatePrivilege"), TRUE);
			BOOL bSeCreateTokenPrivilege = SetPrivilege(hToken, TEXT("SeCreateTokenPrivilege"), TRUE);
			BOOL bSeTcbPrivilege = SetPrivilege(hToken, TEXT("SeTcbPrivilege"), TRUE);
			BOOL bSeIncreaseQuotaPrivilege = SetPrivilege(hToken, TEXT("SeIncreaseQuotaPrivilege"), TRUE);

			if (!bSeAssignPrimaryTokenPrivilege ||
				!bSeImpersonatePrivilege ||
				!bSeCreateTokenPrivilege ||
				!bSeTcbPrivilege ||
				!bSeIncreaseQuotaPrivilege)
			{
				DWORD err = GetLastError();
				PRINT_OUT(TEXT("User privilege problem (%d).\n"), err);
				return FALSE;
			}

#ifdef USE_NEW_INTERACTIVE_DESKTOP
			// Get a handle to the interactive window station.
			hwinsta = OpenWindowStation(
				_T("winsta0"),                   // the interactive window station 
				FALSE,                       // handle is not inheritable
				READ_CONTROL | WRITE_DAC);   // rights to read/write the DACL

			if (hwinsta == NULL)
				goto Cleanup;

			// To get the correct default desktop, set the caller's 
			// window station to the interactive window station.

			if (!SetProcessWindowStation(hwinsta))
				goto Cleanup;

			// Get a handle to the interactive desktop.

			hdesk = OpenDesktop(
				_T("default"),     // the interactive window station 
				0,             // no interaction with other desktop processes
				FALSE,         // handle is not inheritable
				READ_CONTROL | // request the rights to read and write the DACL
				WRITE_DAC |
				DESKTOP_WRITEOBJECTS |
				DESKTOP_READOBJECTS);

			// Restore the caller's window station.

			if (!SetProcessWindowStation(hwinstaSave))
				goto Cleanup;

			if (hdesk == NULL)
				goto Cleanup;

			// Get the SID for the client's logon session.

			if (!GetLogonSID(hToken, &pSid))
				goto Cleanup;

			// Allow logon SID full access to interactive window station.

			if (!AddAceToWindowStation(hwinsta, pSid))
				goto Cleanup;

			// Allow logon SID full access to interactive desktop.

			if (!AddAceToDesktop(hdesk, pSid))
				goto Cleanup;

			// Impersonate client to ensure access to executable file.

			//if (!ImpersonateLoggedOnUser(hToken))
			//	goto Cleanup;

			// Initialize the STARTUPINFO structure.
			// Specify that the process runs in the interactive desktop.
			ZeroMemory(&si, sizeof(STARTUPINFO));
			si.cb = sizeof(STARTUPINFO);
			si.lpDesktop = TEXT("winsta0\\default");
#else
			ZeroMemory(&si, sizeof(STARTUPINFO));
			si.cb = sizeof(STARTUPINFO);
			si.lpDesktop = TEXT("winsta0\\default");
#endif 

			// Launch the process in the client's logon session.

			bResult = CreateProcessAsUser(hToken, NULL, lpCommandLine, NULL, NULL, FALSE, CREATE_UNICODE_ENVIRONMENT, lpvEnv, szUserProfileDir, &si, &pi);

			if (!bResult)
			{
				DWORD err = GetLastError();
				PRINT_OUT(TEXT("CreateProcess failed (%d).\n"), err);
				return FALSE;
			}

			// Wait until child process exits.
			WaitForSingleObject(pi.hProcess, INFINITE);

			// Close process and thread handles. 
			CloseHandle(pi.hProcess);
			CloseHandle(pi.hThread);


#ifdef USE_NEW_INTERACTIVE_DESKTOP
			Cleanup :

					if (hwinstaSave != NULL)
						SetProcessWindowStation(hwinstaSave);

					// Free the buffer for the logon SID.

					if (pSid)
						FreeLogonSID(&pSid);

					// Close the handles to the interactive window station and desktop.

					if (hwinsta)
						CloseWindowStation(hwinsta);

					if (hdesk)
						CloseDesktop(hdesk);

					// Close the handle to the client's access token.

					if (hToken != INVALID_HANDLE_VALUE)
						CloseHandle(hToken);
#endif 

					return bResult;
		}


		void ElevateNow()
		{
			BOOL bAlreadyRunningAsAdministrator = FALSE;
			try
			{
				bAlreadyRunningAsAdministrator = IsRunAsAdministrator();
			}
			catch (...)
			{
				std::cout << "Failed to determine if application was running with admin rights" << std::endl;
				DWORD dwErrorCode = GetLastError();
				TCHAR szMessage[256];
				_stprintf_s(szMessage, ARRAYSIZE(szMessage), _T("Error code returned was 0x%08lx"), dwErrorCode);
				std::cout << szMessage << std::endl;
			}
			if (!bAlreadyRunningAsAdministrator)
			{
				TCHAR szPath[MAX_PATH];
				if (GetModuleFileName(NULL, szPath, ARRAYSIZE(szPath)))
				{
					// Launch itself as admin
					SHELLEXECUTEINFO sei = { sizeof(sei) };
					sei.lpVerb = TEXT("runas");
					sei.lpFile = szPath;
					sei.hwnd = NULL;
					sei.nShow = SW_NORMAL;

					if (!ShellExecuteEx(&sei))
					{
						DWORD dwError = GetLastError();
						if (dwError == ERROR_CANCELLED)
						{
							// The user refused to allow privileges elevation.
							std::cout << "End user did not allow elevation" << std::endl;
						}
					}
				}
			}
		}
		// Either returns true (for a retry) or false (success or failure)
		// Failure: pnbProcesses is 0 and there is no buffer to free
		// Success: pnbProcesses is greater than 0 and *pprocesses contains a pointer to be freed
		BOOL FillProcessesListWithAlloc(PDWORD *pprocesses, DWORD size, PDWORD pnbProcesses)
		{
			DWORD *processes, bytes = 0, result = 0;
			BOOL retry = FALSE, realloc = FALSE;

			// Attempt allocation or reallocation
			if (!(*pprocesses)) {
				processes = (PDWORD)HeapAlloc(GetProcessHeap(), 0, size);
			}
			else {
				processes = (PDWORD)HeapReAlloc(GetProcessHeap(), 0, *pprocesses, size);
				realloc = TRUE;
			}
			// If allocation for requested size succeeded
			if (processes) {
				if (EnumProcesses(processes, size, &bytes)) {
					// Success
					if (bytes<size) {
						result = bytes / sizeof(DWORD);
					}
					else {
						// Buffer too small to list all processIDs
						retry = TRUE;
					}
					// Writes the allocation pointer back in case of success or retry
					*pprocesses = processes;
				}
				else {
					HeapFree(GetProcessHeap(), 0, processes);
					_tprintf(_T("EnumProcesses() failure, error %#.8x\n"), GetLastError());
				}
			} // if processes
			else {
				// Allocation failure handling
				_tprintf(_T("Allocation failure (requested %#.8x bytes), aborting\n"), size);
				// If realloc failed, a free is necessary
				if (realloc) {
					HeapFree(GetProcessHeap(), 0, *pprocesses);
				}
			}
			// Write back nb of processe only if we are done
			if (!retry) {
				*pnbProcesses = result;
			}
			return retry;
		}

		// Attemps to fill the stack buffer if large enough, otherwise move on to allocations
		DWORD FillProcessesList(PDWORD *pprocesses, DWORD bufsize)
		{
			DWORD nb_processes = 0, bytes, size = bufsize;
			BOOL retry;

			// First attemps on stack buffer
			if (EnumProcesses(*pprocesses, size, &bytes)) {
				if (bytes >= size) {
					// Not large enough, allocating
					*pprocesses = NULL;
					do {
						size *= 2;    // doubling size of buffer for processIDs list
						retry = FillProcessesListWithAlloc(pprocesses, size, &nb_processes);
					} while (retry);
				}
				else {
					nb_processes = bytes / sizeof(DWORD);
				}
			} // if enumProcesses
			else {
				_tprintf(_T("EnumProcesses failed with error %#.8x\n"), GetLastError());
			}
			return nb_processes;
		}
		BOOL ProcessIdToName(DWORD processId, TCHAR* processName, DWORD buffSize)
		{
			BOOL ret = FALSE;
			HANDLE handle = OpenProcess(
				PROCESS_QUERY_LIMITED_INFORMATION,
				FALSE,
				processId 
			);
			if (handle){
				DWORD copied = QueryFullProcessImageName(handle, 0, processName, &buffSize);
				if (copied > 0 && copied <= buffSize){
					ret = TRUE;
				}
				else{
					printf("Error QueryFullProcessImageName : %lu", GetLastError());
				}
				CloseHandle(handle);
			}
			else{
				printf("Error OpenProcess : %lu", GetLastError());
			}
			return ret;
		}
		// Returns success boolean and outputs process handle with requested rights
		BOOL GetProcessbyNameOrId(LPTSTR searchstring, PHANDLE phProcess, DWORD rights)
		{
			BOOL found = FALSE;
			HMODULE hMod;
			DWORD *processes, lpProcesses[QUITE_LARGE_NB_PROCESSES], bytes, processId;
			SIZE_T nbProcesses, i;
			HANDLE hProcess;
			TCHAR processname[MAX_PATH + 1], *stop;

			processId = _tcstoul(searchstring, &stop, 0);
			if (processId && *stop == L'\0') {
				hProcess = OpenProcess(rights, FALSE, processId);
				if (hProcess) {
					*phProcess = hProcess;
					found = TRUE;
				}
			}
			else {
				processes = lpProcesses;
				nbProcesses = FillProcessesList(&processes, sizeof(lpProcesses));
				if (nbProcesses) {
					for (i = 0; i<nbProcesses && !found; i++) {
						hProcess = OpenProcess(IDENTIFICATION_RIGHTS, FALSE, processes[i]);
						if (hProcess) {
							if (EnumProcessModules(hProcess, &hMod, sizeof(hMod), &bytes)) {
								if (GetModuleBaseName(hProcess, hMod, processname, sizeof(processname) / sizeof(TCHAR))) {
									// Found the process by that name
									if (!_tcsicmp(searchstring, processname)) {
										// Close the handle and attempt reopenning with requested rights
										CloseHandle(hProcess);
										hProcess = OpenProcess(rights, FALSE, processes[i]);
										if (hProcess) {
											*phProcess = hProcess;
											found = TRUE;
										}
									} // if _tcsicmp
								} // if GetModuleBaseName
							} // if EnumProcessModules
							if (!found) {
								// Only close this process handle if it is not the one we are looking for
								CloseHandle(hProcess);
							}
						} // if hProcess
					} // for all processes
					if (processes != lpProcesses) {
						HeapFree(GetProcessHeap(), 0, processes);
					}
				} // if nbProcesses
			}
			return found;
		}

		// Just returns process session
		DWORD GetProcessSession(HANDLE hProcess)
		{
			DWORD sessionId = (DWORD)(-1);

			if (!ProcessIdToSessionId(GetProcessId(hProcess), &sessionId)) {
				_tprintf(_T("Getting target process session id failed with error %#.8x\n"), GetLastError());
			}
			return sessionId;
		}
	}

	namespace Service
	{
		SC_HANDLE GetServiceByName(LPCWSTR name)
		{
			SC_HANDLE serviceManager = OpenSCManagerW(NULL, NULL, SC_MANAGER_ALL_ACCESS);
			if (!serviceManager) return NULL;

			SC_HANDLE service = OpenServiceW(serviceManager, name, SC_MANAGER_ALL_ACCESS);

			CloseServiceHandle(serviceManager);
			return service;
		}
		DWORD GetServiceState(SC_HANDLE service)
		{
			SERVICE_STATUS_PROCESS status;
			DWORD bytesNeeded;
			if (!QueryServiceStatusEx(service, SC_STATUS_PROCESS_INFO, (LPBYTE)&status, sizeof(SERVICE_STATUS_PROCESS), &bytesNeeded)) return 0;

			return status.dwCurrentState;
		}
		DWORD GetServiceProcessId(SC_HANDLE service)
		{
			SERVICE_STATUS_PROCESS status;
			DWORD bytesNeeded;
			if (!QueryServiceStatusEx(service, SC_STATUS_PROCESS_INFO, (LPBYTE)&status, sizeof(SERVICE_STATUS_PROCESS), &bytesNeeded)) return 0;

			return status.dwProcessId;
		}
		BOOL StartServiceWait(SC_HANDLE service, DWORD expectedState, DWORD delayMilliseconds, DWORD timeoutMilliseconds)
		{
			BOOL result = FALSE;
			ULONGLONG startTime = GetTickCount64();

			while (!result && GetTickCount64() - startTime < timeoutMilliseconds)
			{
				StartServiceW(service, 0, NULL);
				Sleep(delayMilliseconds);

				result = GetServiceState(service) == expectedState;
			}

			return result;
		}
		BOOL ControlServiceWait(SC_HANDLE service, DWORD control, DWORD expectedState, DWORD delayMilliseconds, DWORD timeoutMilliseconds)
		{
			BOOL result = FALSE;
			ULONGLONG startTime = GetTickCount64();
			SERVICE_STATUS_PROCESS status;

			while (!result && GetTickCount64() - startTime < timeoutMilliseconds)
			{
				ControlService(service, control, (LPSERVICE_STATUS)&status);
				Sleep(delayMilliseconds);

				result = GetServiceState(service) == expectedState;
			}

			return result;
		}
	}

	namespace FileOperation
	{
		BOOL __FileOperation(LPCWSTR srcPath, LPCWSTR destPath, DWORD action)
		{
			BOOL result = FALSE;
			IFileOperation *fileOperation = NULL;
			IShellItem *shellItemFrom = NULL;
			IShellItem *shellItemTo = NULL;

			HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
			if (SUCCEEDED(hr))
			{
				hr = CoCreateInstance(CLSID_FileOperation, NULL, CLSCTX_ALL, IID_PPV_ARGS(&fileOperation));

				if (SUCCEEDED(hr))
				{
					hr = fileOperation->SetOperationFlags(FOF_NOCONFIRMATION | FOF_SILENT | FOFX_SHOWELEVATIONPROMPT | FOFX_NOCOPYHOOKS | FOFX_REQUIREELEVATION | FOF_NOERRORUI);
					if (SUCCEEDED(hr))
					{
						hr = SHCreateItemFromParsingName(srcPath, NULL, IID_PPV_ARGS(&shellItemFrom));
						if (SUCCEEDED(hr))
						{
							if (destPath)
							{
								LPWSTR destDirectory = Path::GetDirectoryName(destPath);
								hr = SHCreateItemFromParsingName(destDirectory, NULL, IID_PPV_ARGS(&shellItemTo));
								delete[] destDirectory;
							}

							if (SUCCEEDED(hr))
							{
								LPWSTR newFileName = destPath ? Path::GetFileName(destPath) : NULL;

								switch (action)
								{
									case 1:
										hr = fileOperation->CopyItem(shellItemFrom, shellItemTo, newFileName, NULL);
										if (SUCCEEDED(hr)) result = TRUE;
										break;
									case 2:
										hr = fileOperation->MoveItem(shellItemFrom, shellItemTo, newFileName, NULL);
										if (SUCCEEDED(hr)) result = TRUE;
										break;
									case 3:
										hr = fileOperation->DeleteItem(shellItemFrom, NULL);
										if (SUCCEEDED(hr)) result = TRUE;
										break;
								}

								if (newFileName) delete[] newFileName;
								if (shellItemTo) shellItemTo->Release();
							}

							shellItemFrom->Release();
						}

						if (SUCCEEDED(hr)) hr = fileOperation->PerformOperations();
					}

					fileOperation->Release();
				}
				CoUninitialize();
			}

			return result;
		}

		BOOL Copy(LPCWSTR srcPath, LPCWSTR destPath)
		{
			return __FileOperation(srcPath, destPath, 1);
		}
		BOOL Move(LPCWSTR srcPath, LPCWSTR destPath)
		{
			return __FileOperation(srcPath, destPath, 2);
		}
		BOOL Delete(LPCWSTR path)
		{
			return __FileOperation(path, NULL, 3);
		}
	}

	namespace Message
	{
		void Information(LPCWSTR message)
		{
			Information(L"Information", message);
		}
		void Information(LPCWSTR title, LPCWSTR message)
		{
			MessageBoxW(NULL, message, title, MB_OK | MB_ICONASTERISK);
		}
		void Warning(LPCWSTR message)
		{
			Warning(L"Warning", message);
		}
		void Warning(LPCWSTR title, LPCWSTR message)
		{
			MessageBoxW(NULL, message, title, MB_OK | MB_ICONWARNING);
		}
		void Error(LPCWSTR message, BOOL exitProcess)
		{
			Error(L"Error", message, exitProcess);
		}
		void Error(LPCWSTR title, LPCWSTR message, BOOL exitProcess)
		{
			MessageBoxW(NULL, message, title, MB_OK | MB_ICONERROR);
			if (exitProcess) ExitProcess(0);
		}
		BOOL Confirmation(LPCWSTR message, BOOL warning)
		{
			return Confirmation(L"Confirmation", message, false);
		}
		BOOL Confirmation(LPCWSTR title, LPCWSTR message, BOOL warning)
		{
			return MessageBoxW(NULL, message, title, MB_OKCANCEL | (warning ? MB_ICONWARNING : MB_ICONASTERISK)) == IDOK;
		}
	}
}