#pragma once
#include <windows.h>
#include <vector>

namespace Scanner {
    // Klasik Array of Bytes (AOB) tarayıcı algoritması
    inline uintptr_t FindPattern(HMODULE module, const char* pattern, const char* mask) {
        MODULEINFO modInfo = { 0 };
        GetModuleInformation(GetCurrentProcess(), module, &modInfo, sizeof(modInfo));
        
        uintptr_t startAddress = reinterpret_cast<uintptr_t>(modInfo.lpBaseOfDll);
        size_t size = modInfo.SizeOfImage;
        size_t patternLength = strlen(mask);

        for (size_t i = 0; i < size - patternLength; i++) {
            bool found = true;
            for (size_t j = 0; j < patternLength; j++) {
                if (mask[j] != '?' && pattern[j] != *reinterpret_cast<char*>(startAddress + i + j)) {
                    found = false;
                    break;
                }
            }
            if (found) {
                return startAddress + i;
            }
        }
        return 0;
    }
}

