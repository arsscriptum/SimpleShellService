@echo off

:: reg query "HKLM\SYSTEM\CurrentControlSet\Control\Session Manager\Environment" | findstr /r ^VS[0-9][0-9][0-9]COMNTOOLS
:: put this:
:: @SET VS_BUILD_TOOLS_ENVIRONMENT_CONFIGURED="BRAVO"
:: at the end of batch file of environment script

if defined VS_BUILD_TOOLS_ENVIRONMENT_CONFIGURED ( 
	goto :clean 
) 

set defines=-D_USING_V110_SDK71_ -DSUBSYSTEM_CONSOLE -DDEBUG_OUTPUT
set link_options=/link /FILEALIGN:512 /OPT:REF /OPT:ICF /INCREMENTAL:NO /subsystem:console,5.01
set libs=gdiplus.lib user32.lib Gdi32.lib ws2_32.lib Wininet.lib

:clean
del /F /Q *.obj 2> NUL
del /F /Q *.exe 2> NUL

:build
if not exist bin ( mkdir bin )
echo starting build...

echo "building main.exe..."
cl /Ox main.cpp %defines% %link_options% %libs% /out:pipe.exe 
del /F /Q *.obj 2> NUL
