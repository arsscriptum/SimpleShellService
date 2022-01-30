
#ifndef __NETWORKCLIENT_H__
#define __NETWORKCLIENT_H__


const DWORD RET_ERROR = -1;
const DWORD RET_SUCCESS = 0;

#ifdef UNICODE
const LPWSTR SVC_NAME = L"_socketservice";
#else    
const LPSTR SVC_NAME = "_socketservice";
#endif

DWORD WINAPI NetworkClient(LPVOID lpParam);

#endif