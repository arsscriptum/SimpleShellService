//------------Sample using CreateProcess and Anonymous Pipes-----------------
//---------------------childspawn.cpp----------------------------------------
//---------------------use freely--------------------------------------------
#include <winsock2.h>
#include <windows.h>
#include <stdio.h>

//#include <winsock2.h>

#include <iostream>
#include <codecvt>
#include <locale> 
#include <stdio.h>
#include <conio.h>
#include <string.h>

#pragma comment(lib, "advapi32.lib")
//#pragma comment(lib, "ws2_32.lib") //Winsock Library

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
  printf("%s: %s\n",(char*)str,(char*)msg);
  LocalFree(msg);
}

//---------------------------------------------------------------------------
void main()
{
  char buf[1024];           //i/o buffer

  STARTUPINFO si;
  SECURITY_ATTRIBUTES sa;
  SECURITY_DESCRIPTOR sd;               //security information for pipes
  PROCESS_INFORMATION pi;
  HANDLE newstdin,newstdout,read_stdout,write_stdin;  //pipe handles

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
    getch();
    return;
  }
  if (!CreatePipe(&read_stdout,&newstdout,&sa,0))  //create stdout pipe
  {
    ErrorMessage("CreatePipe");
    getch();
    CloseHandle(newstdin);
    CloseHandle(write_stdin);
    return;
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
    return;
  }

  unsigned long exit=0;  //process exit code
  unsigned long bread;   //bytes read
  unsigned long avail;   //bytes available

  bzero(buf);
  for(;;)      //main program loop
  {
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
          printf("%s",(char*)buf);
          bzero(buf);
        }
      }
      else {
        ReadFile(read_stdout,buf,1023,&bread,NULL);
        printf("%s",(char*)buf);
      }
    }
    if (kbhit())      //check for user input.
    {
      bzero(buf);
      *buf = (char)getche();
      //printf("%c",*buf);
      WriteFile(write_stdin,buf,1,&bread,NULL); //send it to stdin
      if (*buf == '\r') {
        *buf = '\n';
        printf("%c",(char)*buf);
        WriteFile(write_stdin,buf,1,&bread,NULL); //send an extra newline char,
                                                  //if necessary
      }
    }
  }
  CloseHandle(pi.hThread);
  CloseHandle(pi.hProcess);
  CloseHandle(newstdin);            //clean stuff up
  CloseHandle(newstdout);
  CloseHandle(read_stdout);
  CloseHandle(write_stdin);
}
//----------------------------EOF--------------------------------------------
//---------------------------------------------------------------------------