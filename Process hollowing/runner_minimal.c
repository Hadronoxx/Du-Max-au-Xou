#include <windows.h>

// Forcera le linker à utiliser "mainCRTStartup" comme point d'entrée
#pragma comment(linker, "/ENTRY:mainCRTStartup")
// Évite que MSVC / mingw ne fasse des imports implicites
#pragma comment(linker, "/NODEFAULTLIB")

// On va définir nos propres prototypes de fonctions, pour l'appel dynamique :
typedef int   (WINAPI *MessageBoxAFunc)(HWND, LPCSTR, LPCSTR, UINT);
typedef void  (WINAPI *ExitProcessFunc)(UINT);

// On évite d'importer <stdio.h>, etc., pour ne pas avoir la CRT standard

// Équivalent minimal au main : "mainCRTStartup" sera le point d'entrée effectif
void mainCRTStartup(void)
{
    // Étape 1 : Récupérer des handles sur user32.dll et kernel32.dll
    HMODULE hUser32    = LoadLibraryA("user32.dll");
    HMODULE hKernel32  = LoadLibraryA("kernel32.dll");

    if (!hUser32 || !hKernel32) {
        // Si on est vraiment prudent, on peut faire un "exit" direct en raw :
        // 0x400000003 => opcode 'ret', ou on appelle un ExitProcess direct
        return;
    }

    // Étape 2 : Résoudre "MessageBoxA" et "ExitProcess"
    MessageBoxAFunc pMessageBoxA = 
        (MessageBoxAFunc)GetProcAddress(hUser32, "MessageBoxA");
    ExitProcessFunc pExitProcess = 
        (ExitProcessFunc)GetProcAddress(hKernel32, "ExitProcess");

    if (!pMessageBoxA || !pExitProcess) {
        return;
    }

    // Étape 3 : Afficher un message inoffensif
    pMessageBoxA(NULL, 
                 "Hello from minimal FUD runner!", 
                 "Runner Minimal", 
                 MB_OK);

    // Étape 4 : Quitter proprement l'exécutable
    pExitProcess(0);
}
