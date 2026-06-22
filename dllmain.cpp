#include <windows.h>
#include <iostream>
#include <fstream>
#include <psapi.h>
#include "UnrealStructures.hpp"
#include "Scanner.hpp"

// Terminali renklendirmek için küçük bir yardımcı fonksiyon
void SetTerminalColor(WORD color) {
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), color);
}

DWORD WINAPI DumperThread(LPVOID lpParam) {
    // Hata ayıklama terminalini başlat
    AllocConsole();
    FILE* fDummy;
    freopen_s(&fDummy, "CONOUT$", "w", stdout);

    SetTerminalColor(11); // Turkuaz
    std::cout << "==================================================\n";
    std::cout << "          NOVA SUITE - ADVANCED UE4/UE5 DUMPER    \n";
    std::cout << "==================================================\n";
    
    SetTerminalColor(7); // Varsayılan Beyaz
    std::cout << "[STATUS] -> Terminal aktif edildi.\n";
    std::cout << "[STATUS] -> Hedef oyun modulu haritasi okunuyor...\n";

    HMODULE hGameModule = GetModuleHandleA(nullptr); 
    uintptr_t baseAddress = reinterpret_cast<uintptr_t>(hGameModule);
    
    SetTerminalColor(14); // Sarı
    std::cout << "[INFO]   -> Oyun Base Adresi: 0x" << std::hex << baseAddress << "\n";

    SetTerminalColor(7);
    std::cout << "[STATUS] -> GObjects icin bellek taramasi (AOB Scan) baslatildi...\n";

    // UE4.21 - UE5.x arası oyunlar için endüstri standardı kararlı GObjects deseni
    const char* pattern = "\x48\x8B\x05\x00\x00\x00\x00\x48\x8B\x0C\xC8\x48\x8D\x04\xD1";
    const char* mask = "xxx????xxxxxxxx";

    uintptr_t signatureAddress = Scanner::FindPattern(hGameModule, pattern, mask);
    
    if (!signatureAddress) {
        SetTerminalColor(12); // Kırmızı
        std::cout << "[FATAL]  -> HATA: GObjects imza deseni bellek taramasinda eslesmedi!\n";
        std::cout << "[FATAL]  -> Oyun guncellenmis veya anti-cheat bellek korumasi devreye girmis olabilir.\n";
        return 0;
    }

    // Relative RIP Adres Çözümleme (Adres hesaplaması güçlendirildi)
    int32_t relativeOffset = *reinterpret_cast<int32_t*>(signatureAddress + 3);
    FUObjectArray* GObjects = reinterpret_cast<FUObjectArray*>(signatureAddress + 7 + relativeOffset);
    
    // RVA (Relative Virtual Address) Hesaplama - Cheat Engine'de direkt aratabilmen için
    uintptr_t gObjectsOffset = reinterpret_cast<uintptr_t>(GObjects) - baseAddress;

    if (IsBadReadPtr(GObjects, sizeof(FUObjectArray)) || GObjects->NumElements <= 0) {
        SetTerminalColor(12);
        std::cout << "[FATAL]  -> HATA: Bulunan GObjects adresi gecersiz veya isaret ettigi bellek bolgesi korumali!\n";
        return 0;
    }

    // --- ÖNEMLİ OFFSETLERİN TERMİNALE YAZDIRILMASI ---
    SetTerminalColor(10); // Yeşil
    std::cout << "\n==================================================\n";
    std::cout << "[SUCCESS]-> KRITIK MOTOR OFFSETLERI BULUNDU:\n";
    std::cout << " -> GObjects VA (Bellek Adresi): 0x" << std::hex << GObjects << "\n";
    std::cout << " -> GObjects RVA (Oyun Offseti): 0x" << std::hex << gObjectsOffset << "\n";
    std::cout << " -> Toplam Nesne (Count): " << std::dec << GObjects->NumElements << " adet nesne aktif.\n";
    std::cout << "==================================================\n\n";

    SetTerminalColor(7);
    std::cout << "[STATUS] -> C:\\Dumper-Output\\UObject_Dump.txt dosyasina aktarim basliyor...\n";

    CreateDirectoryA("C:\\Dumper-Output", NULL);
    std::ofstream dumpFile("C:\\Dumper-Output\\UObject_Dump.txt");

    if (dumpFile.is_open()) {
        dumpFile << "=== NOVA SUITE ENGINE DUMPER OUTPUT ===\n";
        dumpFile << "Base Address: 0x" << std::hex << baseAddress << "\n";
        dumpFile << "GObjects Offset (RVA): 0x" << std::hex << gObjectsOffset << "\n";
        dumpFile << "Total Elements: " << std::dec << GObjects->NumElements << "\n\n";

        int32_t successfullyDumped = 0;

        for (int32_t i = 0; i < GObjects->NumElements; ++i) {
            // Güçlendirilmiş güvenli fonksiyonu çağırıyoruz
            void* rawObj = GObjects->GetByIndexSafe(i);
            if (!rawObj) continue;

            UObject* object = reinterpret_cast<UObject*>(rawObj);
            
            // Ekstra güvenlik: Nesnenin sanal tablo (vtable) adresini doğrula
            if (IsBadReadPtr(object, sizeof(UObject)) || IsBadReadPtr(object->VTable, sizeof(void*))) continue;

            dumpFile << "[" << i << "] Address: " << object->GetAddressStr() 
                     << " | NameIndex: " << object->NameIndex 
                     << " | Outer: " << (object->OuterPrivate ? object->OuterPrivate->GetAddressStr() : "nullptr") 
                     << "\n";

            successfullyDumped++;
            
            // Her 10.000 nesnede bir terminale ilerleme durumunu raporla
            if (successfullyDumped % 10000 == 0) {
                SetTerminalColor(14);
                std::cout << "[PROGRESS]-> " << std::dec << successfullyDumped << " nesne basariyla dosyaya yazildi...\n";
            }
        }
        
        dumpFile.close();
        
        SetTerminalColor(10);
        std::cout << "\n[SUCCESS]-> Dump islemi harika bir sekilde tamamlandi!\n";
        std::cout << "[SUCCESS]-> Toplam " << std::dec << successfullyDumped << " gecerli nesne kurtarildi.\n";
    } else {
        SetTerminalColor(12);
        std::cout << "[FATAL]  -> HATA: Disk uzerinde dosya olusturulamadi! Yönetici izinlerini kontrol edin.\n";
    }

    SetTerminalColor(11);
    std::cout << "\n==================================================\n";
    std::cout << " NovaDumper gorevini tamamladi. Kapatabilirsiniz.\n";
    std::cout << "==================================================\n";

    return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    switch (ul_reason_for_call) {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hModule);
        // Arka planda güvenli thread oluşturma
        HANDLE hThread = CreateThread(nullptr, 0, DumperThread, hModule, 0, nullptr);
        if (hThread) CloseHandle(hThread);
        break;
    }
    return TRUE;
}

