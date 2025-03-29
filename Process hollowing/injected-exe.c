#include <windows.h>
#include <shellapi.h>

// UTF16("str") == L"str" # Wide Character String / UTF-16
#define _UTF16(x)   L##x
#define UTF16(x)    _UTF16(x)

DWORD WINAPI Notify(LPVOID param) {
    (void)param;
    NOTIFYICONDATAW nid = {0};
    nid.cbSize = sizeof(nid);

    // dummy msg-only window
    HWND hWnd = CreateWindowW(L"Static", L"Dummy", WS_OVERLAPPEDWINDOW,
                              0, 0, 0, 0, HWND_MESSAGE, NULL, 0, NULL);
    nid.hWnd = hWnd;
    nid.uID = 1;
    nid.uFlags = NIF_INFO;
    nid.hIcon = LoadIconW(0, MAKEINTRESOURCEW(IDI_APPLICATION));
    wsprintfW(nid.szInfoTitle, UTF16(__FILE__));
    nid.dwInfoFlags = NIIF_INFO | NIIF_NOSOUND;

    while (1) {
        SYSTEMTIME st;
        GetLocalTime(&st);
        wsprintfW(nid.szInfo,
                L"[%04d-%02d-%02d %02d:%02d:%02d] Running on PID %lu",
                st.wYear, st.wMonth, st.wDay,
                st.wHour, st.wMinute, st.wSecond,
                GetCurrentProcessId());
        Shell_NotifyIconW(NIM_DELETE, &nid);
        Shell_NotifyIconW(NIM_ADD, &nid);
        Sleep(5000);
    }
}

int WINAPI WinMain(HINSTANCE a, HINSTANCE b, LPSTR c, int d) {
    (void)a; (void)b; (void)c; (void)d;
    Notify(0);
    return 0;
}
 