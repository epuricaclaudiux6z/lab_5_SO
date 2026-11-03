#include <iostream>
#include <windows.h>
#include <string>

bool is_prime(int n) {
    if (n <= 1) return false;
    if (n <= 3) return true;
    if (n % 2 == 0 || n % 3 == 0) return false;
    for (int i = 5; i * i <= n; i += 6) {
        if (n % i == 0 || n % (i + 2) == 0) return false;
    }
    return true;
}

int main(int argc, char* argv[]) {
    if (argc == 4) {
        HANDLE hRead = (HANDLE)std::stoull(argv[2]);
        HANDLE hWrite = (HANDLE)std::stoull(argv[3]);

        int start, end;
        DWORD bytes;
        ReadFile(hRead, &start, sizeof(start), &bytes, NULL);
        ReadFile(hRead, &end, sizeof(end), &bytes, NULL);
        CloseHandle(hRead);

        for (int i = start; i <= end; i++) {
            if (is_prime(i)) {
                WriteFile(hWrite, &i, sizeof(i), &bytes, NULL);
            }
        }
        CloseHandle(hWrite);
        return 0;
    }

    HANDLE copii[10];
    HANDLE tevi_citire[10];
    SECURITY_ATTRIBUTES sa = { sizeof(SECURITY_ATTRIBUTES), NULL, TRUE };

    for (int i = 0; i < 10; i++) {
        HANDLE p2c_r, p2c_w, c2p_r, c2p_w;
        CreatePipe(&p2c_r, &p2c_w, &sa, 0);
        CreatePipe(&c2p_r, &c2p_w, &sa, 0);

        SetHandleInformation(p2c_w, HANDLE_FLAG_INHERIT, 0);
        SetHandleInformation(c2p_r, HANDLE_FLAG_INHERIT, 0);

        std::string cmd = std::string(argv[0]) + " child " + 
                          std::to_string((unsigned long long)p2c_r) + " " + 
                          std::to_string((unsigned long long)c2p_w);

        STARTUPINFOA si = { sizeof(STARTUPINFOA) };
        PROCESS_INFORMATION pi = { 0 };

        CreateProcessA(NULL, &cmd[0], NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi);

        CloseHandle(p2c_r);
        CloseHandle(c2p_w);
        CloseHandle(pi.hThread);

        copii[i] = pi.hProcess;
        tevi_citire[i] = c2p_r;

        int start = i * 1000 + 1;
        int end = (i + 1) * 1000;
        DWORD bytes;
        WriteFile(p2c_w, &start, sizeof(start), &bytes, NULL);
        WriteFile(p2c_w, &end, sizeof(end), &bytes, NULL);
        CloseHandle(p2c_w);
    }

    for (int i = 0; i < 10; i++) {
        int prim;
        DWORD bytes;
        while (ReadFile(tevi_citire[i], &prim, sizeof(prim), &bytes, NULL) && bytes > 0) {
            std::cout << prim << " ";
        }
        CloseHandle(tevi_citire[i]);
    }
    std::cout << "\n";

    WaitForMultipleObjects(10, copii, TRUE, INFINITE);
    for (int i = 0; i < 10; i++) {
        CloseHandle(copii[i]);
    }

    return 0;
}
