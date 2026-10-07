#include <iostream>
#include <filesystem>
#include <opencv2/opencv.hpp>

namespace fs = std::filesystem;

int main() {
    std::string girdi_klasoru = "resimler";
    std::string cikti_klasoru = "cikti";

    const int HEDEF_GENISLIK = 1024;
    const int HEDEF_YUKSEKLIK = 768;

    if (!fs::exists(girdi_klasoru)) {
        std::cerr << "Hata: " << girdi_klasoru << " klasoru bulunamadi!" << std::endl;
        return -1;
    }

    std::cout << "--- C++ OpenCV Goruntu Boyutlandirma Basladi ---" << std::endl;

    for (const auto& dosya : fs::directory_iterator(girdi_klasoru)) {
        if (dosya.is_regular_file()) {
            std::string uzanti = dosya.path().extension().string();

            if (uzanti == ".png" || uzanti == ".jpg" || uzanti == ".jpeg" ||
                uzanti == ".PNG" || uzanti == ".JPG" || uzanti == ".JPEG") {

                std::string dosya_yolu = dosya.path().string();
                std::string dosya_adi = dosya.path().filename().string();

                cv::Mat resim = cv::imread(dosya_yolu);
                if (resim.empty()) {
                    std::cerr << "Dosya okunamadi: " << dosya_adi << std::endl;
                    continue;
                }

                std::cout << "\nIslem: " << dosya_adi << std::endl;
                std::cout << "Orijinal Boyut : " << resim.cols << "x" << resim.rows << std::endl;

                // 1024x768 Boyutlandırma
                cv::Mat boyutlu_resim;
                cv::resize(resim, boyutlu_resim, cv::Size(HEDEF_GENISLIK, HEDEF_YUKSEKLIK));

                std::cout << "Yeni Boyut     : " << boyutlu_resim.cols << "x" << boyutlu_resim.rows << std::endl;

                // Diske kaydet
                std::string cikti_yolu = cikti_klasoru + "/resized_" + dosya_adi;
                cv::imwrite(cikti_yolu, boyutlu_resim);
                std::cout << "Kaydedildi     : " << cikti_yolu << std::endl;
            }
        }
    }

    std::cout << "\n--- Islem Basariyla Tamamlandi ---" << std::endl;
    return 0;
}
