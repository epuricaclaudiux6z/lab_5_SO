#include <iostream>
#include <windows.h>
#include <string>
#include <vector>

bool is_prime(int n) {
    if (n <= 1) return false;
    if (n <= 3) return true;
    if (n % 2 == 0 || n % 3 == 0) return false;
    for (int i = 5; i * i <= n; i += 6) {
        if (n % i == 0 || n % (i + 2) == 0) return false;
    }
    return true;
}

void run_child(HANDLE hRead, HANDLE hWrite) {
    int start, end;
    DWORD bytesOp;
    
    ReadFile(hRead, &start, sizeof(start), &bytesOp, NULL);
    ReadFile(hRead, &end, sizeof(end), &bytesOp, NULL);
    CloseHandle(hRead);

    for (int num = start; num <= end; ++num) {
        if (is_prime(num)) {
            WriteFile(hWrite, &num, sizeof(num), &bytesOp, NULL);
        }
    }
    CloseHandle(hWrite);
}

bool spawn_worker(int index, int chunk_size, const std::string& exe_name, HANDLE& outProcess, HANDLE& outReadPipe) {
    HANDLE hReadP2C, hWriteP2C, hReadC2P, hWriteC2P;
    SECURITY_ATTRIBUTES sa = { sizeof(SECURITY_ATTRIBUTES), NULL, TRUE };

    CreatePipe(&hReadP2C, &hWriteP2C, &sa, 0);
    CreatePipe(&hReadC2P, &hWriteC2P, &sa, 0);

    SetHandleInformation(hWriteP2C, HANDLE_FLAG_INHERIT, 0);
    SetHandleInformation(hReadC2P, HANDLE_FLAG_INHERIT, 0);

    std::string cmd = exe_name + " child " + 
                      std::to_string((unsigned long long)hReadP2C) + " " + 
                      std::to_string((unsigned long long)hWriteC2P);

    STARTUPINFOA si; 
    ZeroMemory(&si, sizeof(si)); 
    si.cb = sizeof(si);
    PROCESS_INFORMATION pi; 
    ZeroMemory(&pi, sizeof(pi));

    if (!CreateProcessA(NULL, &cmd[0], NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi)) {
        return false;
    }

    CloseHandle(hReadP2C);
    CloseHandle(hWriteC2P);
    CloseHandle(pi.hThread);

    outProcess = pi.hProcess;
    outReadPipe = hReadC2P;

    int start = index * chunk_size + 1;
    int end = (index + 1) * chunk_size;
    DWORD bytesWritten;

    WriteFile(hWriteP2C, &start, sizeof(start), &bytesWritten, NULL);
    WriteFile(hWriteP2C, &end, sizeof(end), &bytesWritten, NULL);
    CloseHandle(hWriteP2C);

    return true;
}

void run_parent(const std::string& exe_name) {
    const int num_processes = 10;
    const int chunk_size = 1000;
    
    std::vector<HANDLE> childProcesses(num_processes);
    std::vector<HANDLE> readHandles(num_processes);

    for (int i = 0; i < num_processes; ++i) {
        spawn_worker(i, chunk_size, exe_name, childProcesses[i], readHandles[i]);
    }

    for (int i = 0; i < num_processes; ++i) {
        int prime;
        DWORD bytesRead;
        while (ReadFile(readHandles[i], &prime, sizeof(prime), &bytesRead, NULL) && bytesRead > 0) {
            std::cout << prime << " ";
        }
        CloseHandle(readHandles[i]);
    }
    std::cout << "\n";

    WaitForMultipleObjects(num_processes, childProcesses.data(), TRUE, INFINITE);
    for (int i = 0; i < num_processes; ++i) {
        CloseHandle(childProcesses[i]);
    }
}

int main(int argc, char* argv[]) {
    if (argc == 4 && std::string(argv[1]) == "child") {
        HANDLE hRead = (HANDLE)std::stoull(argv[2]);
        HANDLE hWrite = (HANDLE)std::stoull(argv[3]);
        run_child(hRead, hWrite);
        return 0;
    }
    
    run_parent(argv[0]);
    return 0;
}
