x86_64-w64-mingw32-gcc -o origPe.exe origPe.c -mwindows -nostdlib -ffreestanding -lkernel32 -luser32 -lshell32 

x86_64-w64-mingw32-gcc -o process_hollowing.exe process_hollowing.c -mwindows -nostdlib -ffreestanding -lkernel32 -luser32 -lntdll