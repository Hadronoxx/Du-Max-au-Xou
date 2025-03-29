#include <windows.h>

// On définit nous-même le point d'entrée du programme : WinMainCRTStartup
#pragma comment(linker, "/ENTRY:WinMainCRTStartup")

// WinMainCRTStartup appellera cette fonction, qu'on appelle "WinMain" 
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    // Petit test pour valider que le code est bien injecté et exécuté 
    MessageBoxA(NULL,
                "Hello from injected PE!",
                "Process Hollowing Test",
                MB_OK | MB_ICONINFORMATION);

    // Quitter le process une fois le message affiché
    ExitProcess(0);
    return 0; // pas forcément appelé
}
