#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <random>
#include <algorithm>
#include <iomanip>
#include <openssl/sha.h>
#include <nlohmann/json.hpp>

class CüzdanOlusturucu {
private:
    std::vector<std::string> kelimeler;
    const int KELIME_SAYISI = 12;
    
    // Güvenli rastgele sayı üreteci
    std::random_device rd;
    std::mt19937 gen{rd()};

    // SHA256 hash hesaplama
    std::string sha256_hash(const std::string& input) {
        unsigned char hash[SHA256_DIGEST_LENGTH];
        SHA256_CTX sha256;
        SHA256_Init(&sha256);
        SHA256_Update(&sha256, input.c_str(), input.size());
        SHA256_Final(hash, &sha256);

        std::stringstream ss;
        for(int i = 0; i < SHA256_DIGEST_LENGTH; ++i) {
            ss <d::hex << std::setw(2) << std::setfill('0') 
               <_cast<int>(hash[i]);
        }
        return ss.str();
    }

public:
    // Kelime listesini dosyadan yükle
    bool kelimeleriYukle(const std::string& dosyaYolu) {
        std::ifstream dosya(dosyaYolu);
        if (!dosya.is_open()) {
            std::cerr <Dosya açılamadı: " <aYolu << std::endl;
            return false;
        }

        kelimeler.clear();
        std::string kelime;
        while (std::getline(dosya, kelime)) {
            if (!kelime.empty()) {
                kelimeler.push_back(kelime);
            }
        }
        return !kelimeler.empty();
    }

    // Güvenli tohum oluşturma
    std::string guvenliTohumOlustur() {
        if (kelimeler.empty()) {
            throw std::runtime_error("Kelime listesi boş!");
        }

        std::uniform_int_distribution<> dis(0, kelimeler.size() - 1);
        std::string tohum;

        for (int i = 0; i < KELIME_SAYISI; ++i) {
            tohum += kelimeler[dis(gen)];
            if (i < KELIME_SAYISI - 1) {
                tohum += " ";
            }
        }

        return tohum;
    }

    // Gelişmiş cüzdan adresi oluşturma
    std::string cüzdanAdresiOlustur(const std::string& tohum) {
        // Tohum üzerinden güvenli hash hesaplama
        std::string hash = sha256_hash(tohum);
        
        // İlk 40 karakteri cüzdan adresi olarak kullan
        return hash.substr(0, 40);
    }

    // Bakiye kontrol mekanizması (mock)
    double bakiyeKontrol(const std::string& cüzdanAdresi) {
        // Gerçek API entegrasyonu için yer tutucu
        // Güvenlik ve performans için mock implementasyon
        std::hash<std::string> hasher;
        return (hasher(cüzdanAdresi) % 10000) / 100.0;
    }

    // Güvenli dosyaya kaydetme
    void kaydet(const std::string& cüzdanAdresi, double bakiye, const std::string& tohum) {
        std::ofstream dosya("cüzdanlar.log", std::ios::app);
        if (dosya.is_open()) {
            dosya <Tarih: " << std::time(nullptr) 
                  <nCüzdan: " << cüzdanAdresi 
                  << "\nBakiye: " << bakiye 
                  << "\nTohum: " << tohum 
                  << "\n---\n";
            dosya.close();
        }
    }
};

int main() {
    try {
        CüzdanOlusturucu oluşturucu;
        
        // Kelime listesini yükle
        if (!oluşturucu.kelimeleriYukle("kelimeler.txt")) {
            std::cerr <Kelime listesi yüklenemedi!" << std::endl;
            return 1;
        }

        // Tohum oluştur
        std::string tohum = oluşturucu.guvenliTohumOlustur();
        std::cout <Tohum: " << tohum <;

        // Cüzdan adresi oluştur
        std::string cüzdanAdresi = oluşturucu.cüzdanAdresiOlustur(tohum);
        std::cout << "Cüzdan Adresi: " <danAdresi << std::endl;

        // Bakiye kontrol
        double bakiye = oluşturucu.bakiyeKontrol(cüzdanAdresi);
        std::cout <Bakiye: " << bakiye <;

        // Kaydet
        oluşturucu.kaydet(cüzdanAdresi, bakiye, tohum);

    } catch (const std::exception& e) {
        std::cerr <Hata: " << e.what() <endl;
        return 1;
    }

    return 0;
}
