#pragma once
#include <Windows.h>
#include <TlHelp32.h>
#include <cstdint>
#include <Psapi.h>
#include <iostream>
#include <string>

class Driver {
public:
    HANDLE hProcess = NULL;
    DWORD processId = 0;
    uintptr_t baseAddress = 0;

    bool Attach(const char* processName) {
        PROCESSENTRY32 entry;
        entry.dwSize = sizeof(PROCESSENTRY32);

        HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (snapshot == INVALID_HANDLE_VALUE)
            return false;

        if (Process32First(snapshot, &entry)) {
            do {
                if (!strcmp(processName, entry.szExeFile)) {
                    processId = entry.th32ProcessID;
                    hProcess = OpenProcess(PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_VM_OPERATION | PROCESS_QUERY_INFORMATION, FALSE, processId);
                    if (!hProcess) {
                        hProcess = OpenProcess(PROCESS_VM_READ | PROCESS_QUERY_INFORMATION, FALSE, processId);
                    }
                    CloseHandle(snapshot);
                    return hProcess != NULL;
                }
            } while (Process32Next(snapshot, &entry));
        }

        CloseHandle(snapshot);
        return false;
    }

    uintptr_t GetModuleBase() {
        MODULEENTRY32 modEntry;
        modEntry.dwSize = sizeof(modEntry);

        HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, processId);
        if (snapshot == INVALID_HANDLE_VALUE)
            return 0;

        if (Module32First(snapshot, &modEntry)) {
            baseAddress = reinterpret_cast<uintptr_t>(modEntry.modBaseAddr);
        }

        CloseHandle(snapshot);
        return baseAddress;
    }

    template <typename T>
    T rpm(uintptr_t address) {
        T buffer{};
        if (hProcess && address)
            ReadProcessMemory(hProcess, (LPCVOID)address, &buffer, sizeof(T), nullptr);
        return buffer;
    }

    template <typename T>
    bool wpm(uintptr_t address, T value) {
        if (!hProcess || !address) return false;
        return WriteProcessMemory(hProcess, (LPVOID)address, &value, sizeof(T), nullptr);
    }

    bool ReadRaw(uintptr_t address, void* buffer, size_t size) {
        if (!hProcess || !address || !buffer) return false;
        return ReadProcessMemory(hProcess, (LPCVOID)address, buffer, size, nullptr);
    }
};
