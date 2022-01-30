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

const DWORD RET_ERROR = -1;
const DWORD RET_SUCCESS = 0;

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



#define bzero(a) memset(a,0,sizeof(a)) //easier -- shortcut

bool IsWinNT()  //check if we're running NT
{
  OSVERSIONINFO osv;
  osv.dwOSVersionInfoSize = sizeof(osv);
  GetVersionEx(&osv);
  return (osv.dwPlatformId == VER_PLATFORM_WIN32_NT);
}

void ErrorMessage(char *str)  //display detailed error info
{
  LPVOID msg;
  FormatMessage(
    FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
    NULL,
    GetLastError(),
    MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
    (LPTSTR) &msg,
    0,
    NULL
  );
  LOG_ERROR("svcmain::%s","%s",(char*)str,(char*)msg);
  //printf("%s: %s\n",(char*)str,(char*)msg);
  LocalFree(msg);
}



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


//----------------------------EOF--------------------------------------------
//---------------------------------------------------------------------------
DWORD WINAPI HandleScks(LPVOID lpParam)
{
  char buf[1024];           //i/o buffer

  SOCKET theSck = (SOCKET)lpParam;

  int receivedBytes, sentBytes = 0;
  STARTUPINFO si;
  SECURITY_ATTRIBUTES sa;
  SECURITY_DESCRIPTOR sd;               //security information for pipes
  PROCESS_INFORMATION pi;
  HANDLE newstdin,newstdout,read_stdout,write_stdin;  //pipe handles

  //Send socket some info
  if(send(theSck, "Remote Shell\r\n\r\n", sizeof("Remote Shell\r\n\r\n"), 0) == SOCKET_ERROR) goto closeSck;
    

  if (IsWinNT())        //initialize security descriptor (Windows NT)
  {
    InitializeSecurityDescriptor(&sd,SECURITY_DESCRIPTOR_REVISION);
    SetSecurityDescriptorDacl(&sd, true, NULL, false);
    sa.lpSecurityDescriptor = &sd;
  }
  else sa.lpSecurityDescriptor = NULL;
  sa.nLength = sizeof(SECURITY_ATTRIBUTES);
  sa.bInheritHandle = true;         //allow inheritable handles

  if (!CreatePipe(&newstdin,&write_stdin,&sa,0))   //create stdin pipe
  {
    ErrorMessage("CreatePipe");
    return RET_ERROR;
  }
  if (!CreatePipe(&read_stdout,&newstdout,&sa,0))  //create stdout pipe
  {
    ErrorMessage("CreatePipe");
    
    CloseHandle(newstdin);
    CloseHandle(write_stdin);
    return RET_ERROR;
  }

  GetStartupInfo(&si);      //set startupinfo for the spawned process
  /*
  The dwFlags member tells CreateProcess how to make the process.
  STARTF_USESTDHANDLES validates the hStd* members. STARTF_USESHOWWINDOW
  validates the wShowWindow member.
  */
  si.dwFlags = STARTF_USESTDHANDLES|STARTF_USESHOWWINDOW;
  si.wShowWindow = SW_HIDE;
  si.hStdOutput = newstdout;
  si.hStdError = newstdout;     //set the new handles for the child process
  si.hStdInput = newstdin;
  char app_spawn[] = "c:\\Windows\\System32\\cmd.exe"; //sample, modify for your
                                                     //system

  //spawn the child process
  if (!CreateProcess(app_spawn,NULL,NULL,NULL,TRUE,CREATE_NEW_CONSOLE,
                     NULL,NULL,&si,&pi))
  {
    ErrorMessage("CreateProcess");
    getch();
    CloseHandle(newstdin);
    CloseHandle(newstdout);
    CloseHandle(read_stdout);
    CloseHandle(write_stdin);
    return RET_ERROR;
  }

  unsigned long exit=0;  //process exit code
  unsigned long bread;   //bytes read
  unsigned long avail;   //bytes available

  bzero(buf);
  for(;;)      //main program loop
  {
    Sleep(100);
    GetExitCodeProcess(pi.hProcess,&exit);      //while the process is running
    if (exit != STILL_ACTIVE)
      break;
    PeekNamedPipe(read_stdout,buf,1023,&bread,&avail,NULL);
    //check to see if there is any data to read from stdout
    if (bread != 0)
    {
      bzero(buf);
      if (avail > 1023)
      {
        while (bread >= 1023)
        {
          ReadFile(read_stdout,buf,1023,&bread,NULL);  //read the stdout pipe
          LOG_TRACE("HandleSocks2::1","%s",(char*)buf);
          sentBytes = send(theSck, buf, strlen(buf), 0);
          LOG_TRACE("HandleSocks2::send1","send %d bytes",sentBytes);
          bzero(buf);
        }

      }
      else {
        ReadFile(read_stdout,buf,1023,&bread,NULL);
        LOG_TRACE("HandleSocks2::2","%s",(char*)buf);
        sentBytes = send(theSck, buf, strlen(buf), 0);
        LOG_TRACE("HandleSocks2::send2","send %d bytes",sentBytes);
      }
    }
    receivedBytes = recv(theSck, buf, sizeof(buf), 0);
    if((receivedBytes == 0) || (receivedBytes == SOCKET_ERROR)) {
      LOG_TRACE("HandleSocks2","receivedBytes %d break",receivedBytes);
      break;
    }
    if (receivedBytes)      //check for user input.
    {

      LOG_TRACE("HandleSocks2","%c",*buf);
      WriteFile(write_stdin,buf,receivedBytes,&bread,NULL); //send it to stdin
      if (*buf == '\r') {
        *buf = '\n';
        LOG_TRACE("HandleSocks2","%c",(char)*buf);
        WriteFile(write_stdin,buf,receivedBytes,&bread,NULL); //send an extra newline char,
                                                  //if necessary
      }
    }
  }

  //Cleaning up functions
  closeSck:
    TerminateProcess(pi.hProcess, 0);
    closesocket(theSck);

    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    CloseHandle(newstdin);            //clean stuff up
    CloseHandle(newstdout);
    CloseHandle(read_stdout);
    CloseHandle(write_stdin);

  return RET_SUCCESS;
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
       /* for (i = 0; i < max_clients; i++)
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
        }*/
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
