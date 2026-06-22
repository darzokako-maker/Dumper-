#pragma once
#include <windows.h>
#include <string>
#include <vector>

struct FUObjectItem {
    void* Object; 
    int32_t Flags;
    int32_t ClusterRootIndex;
    int32_t SerialNumber;
    char Pad[0x4]; 
};

struct FUObjectArray {
    FUObjectItem** Objects;
    FUObjectItem* PreAllocatedObjects;
    int32_t MaxElements;
    int32_t NumElements;
    int32_t MaxChunks;
    int32_t NumChunks;

    // Bellek çökmesini engelleyen güvenli eleman getirme fonksiyonu
    void* GetByIndexSafe(int32_t Index) const {
        __try {
            if (Index < 0 || Index >= NumElements) return nullptr;
            
            if (Objects) {
                int32_t ChunkIndex = Index / 65536;
                int32_t WithinChunkIndex = Index % 65536;
                
                if (ChunkIndex >= NumChunks || !Objects[ChunkIndex]) return nullptr;
                
                // Chunk işaretçisinin geçerli bellek bölgesinde olup olmadığını kontrol et
                if (IsBadReadPtr(Objects[ChunkIndex], sizeof(FUObjectItem) * 65536)) return nullptr;

                return Objects[ChunkIndex][WithinChunkIndex].Object;
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            // Bellek okuma hatası alınırsa çökme yerine direkt nullptr döndürür
            return nullptr;
        }
        return nullptr;
    }
};

struct UObject {
    void** VTable;
    int32_t ObjectFlags;
    int32_t InternalIndex;
    void* ClassPrivate; 
    int32_t NameIndex;  
    int32_t NameNumber;
    UObject* OuterPrivate;

    std::string GetAddressStr() {
        char buffer[64];
        sprintf_s(buffer, "0x%p", this);
        return std::string(buffer);
    }
};
