#include <windows.h>
#include <iostream>

#include <detours/detours.h>

static int (WINAPI * trueConnect)(SOCKET s, const sockaddr *name, int namelen) = connect;

int WINAPI hookedConnect(SOCKET s, const sockaddr *name, int namelen) {
    if(name->sa_family == AF_INET) {
        const sockaddr_in *name_in = reinterpret_cast<const sockaddr_in*>(name);
        std::cout << "Port: " << ntohs(name_in->sin_port) << "\n" << std::flush;
        if(name_in->sin_port == htons(1935)) {
            std::cout << "Attempting to redirect connect\n" << std::flush;
            sockaddr_in fake_name;
            //fake_name.sin_addr = name_in->sin_addr;
            fake_name.sin_addr.s_addr = inet_addr("127.0.0.1");
            fake_name.sin_family = name_in->sin_family;
            fake_name.sin_port = name_in->sin_port;
            return trueConnect(s, (const sockaddr*)(&fake_name), sizeof(fake_name)); 
        }
    }
    return trueConnect(s, name, namelen);
}

BOOL WINAPI DllMain(HINSTANCE hinst, DWORD dwReason, LPVOID reserved)
{
    LONG error;
    (void)hinst;
    (void)reserved;

    if (DetourIsHelperProcess()) {
        return TRUE;
    }

    if (dwReason == DLL_PROCESS_ATTACH) {
        DetourRestoreAfterWith();

        DetourTransactionBegin();
        DetourUpdateThread(GetCurrentThread());
        DetourAttach(&(PVOID&)trueConnect, hookedConnect);
        error = DetourTransactionCommit();

    }
    else if (dwReason == DLL_PROCESS_DETACH) {
        DetourTransactionBegin();
        DetourUpdateThread(GetCurrentThread());
        DetourDetach(&(PVOID&)trueConnect, hookedConnect);
        error = DetourTransactionCommit();
    }

    return TRUE;
}
