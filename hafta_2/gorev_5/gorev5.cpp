#include <iostream>
#include <vector>
#include <array>
#include <cmath>
#include <filesystem>
#include <opencv2/opencv.hpp>

namespace fs = std::filesystem;

// 256 elemanli Lookup Table (LUT) ile dogrusal donusum: s = saturate_cast(alpha * r + beta)
cv::Mat applyLinearTransformationLUT(const cv::Mat& input, float alpha, float beta) {
    std::array<uchar, 256> lut_table;
    for (int r = 0; r < 256; ++r) {
        lut_table[r] = cv::saturate_cast<uchar>(std::round(alpha * r + beta));
    }

    cv::Mat output = cv::Mat::zeros(input.size(), CV_8UC1);
    int total_pixels = input.rows * input.cols;

    const uchar* in_ptr = input.data;
    uchar* out_ptr = output.data;

    for (int i = 0; i < total_pixels; ++i) {
        out_ptr[i] = lut_table[in_ptr[i]];
    }

    return output;
}

// 256 seviyeli histogram hesaplama
std::array<int, 256> calculateHistogram(const cv::Mat& img) {
    std::array<int, 256> hist{};
    hist.fill(0);
    const uchar* ptr = img.data;
    int total = img.rows * img.cols;
    for (int i = 0; i < total; ++i) {
        hist[ptr[i]]++;
    }
    return hist;
}

// Goruntu istatistikleri (Min, Max, Ortalama Piksel Degeri)
void printStatistics(const std::string& label, const cv::Mat& img, const std::array<int, 256>& hist) {
    double min_val, max_val;
    cv::minMaxLoc(img, &min_val, &max_val);

    double sum = 0.0;
    int total_pixels = img.rows * img.cols;
    for (int i = 0; i < 256; ++i) {
        sum += i * hist[i];
    }
    double mean = sum / total_pixels;

    std::cout << "\n[" << label << "]" << std::endl;
    std::cout << "  - Minimum Piksel : " << static_cast<int>(min_val) << std::endl;
    std::cout << "  - Maksimum Piksel: " << static_cast<int>(max_val) << std::endl;
    std::cout << "  - Ortalama Ton   : " << mean << std::endl;
}

int main() {
    std::cout << "==========================================================" << std::endl;
    std::cout << "  HAFTA 2 - GOREV 5: DOGRUSAL PARLAKLIK VE KONTRAST       " << std::endl;
    std::cout << "          s = alpha * r + beta (0.75r + 20)              " << std::endl;
    std::cout << "==========================================================" << std::endl;

    // gorsel1.jpg veya gorsel1.png kontrolu
    std::string img_path = "gorsel1.jpg";
    cv::Mat src = cv::imread(img_path, cv::IMREAD_GRAYSCALE);

    if (src.empty()) {
        img_path = "gorsel1.png";
        src = cv::imread(img_path, cv::IMREAD_GRAYSCALE);
    }

    if (src.empty()) {
        std::cerr << "Hata: 'gorsel1.jpg' veya 'gorsel1.png' bulunamadi!" << std::endl;
        std::cerr << "Lutfen Colab sol panelindeki dosya alanina resminizi bu isimle yukleyin." << std::endl;
        return -1;
    }

    // 1. Sadece Kontrast Sikistirma (alpha = 0.75, beta = 0)
    cv::Mat img_scale = applyLinearTransformationLUT(src, 0.75f, 0.0f);

    // 2. Sadece Parlaklik Artirma (alpha = 1.0, beta = +20)
    cv::Mat img_shift = applyLinearTransformationLUT(src, 1.0f, 20.0f);

    // 3. Bilesik Dogrusal Donusum: s = 0.75 * r + 20
    cv::Mat img_combo = applyLinearTransformationLUT(src, 0.75f, 20.0f);

    // Dizinleri olustur ve kaydet
    fs::create_directories("cikti");
    fs::create_directories("resimler");

    cv::imwrite("resimler/" + img_path, src);
    cv::imwrite("cikti/gorev5_orijinal.png", src);
    cv::imwrite("cikti/gorev5_kontrast_075.png", img_scale);
    cv::imwrite("cikti/gorev5_parlaklik_arti20.png", img_shift);
    cv::imwrite("cikti/gorev5_bilesik_075_arti20.png", img_combo);

    auto hist_src   = calculateHistogram(src);
    auto hist_scale = calculateHistogram(img_scale);
    auto hist_shift = calculateHistogram(img_shift);
    auto hist_combo = calculateHistogram(img_combo);

    std::cout << "Kullanilan Gorsel : " << img_path << " (" << src.cols << "x" << src.rows << ")" << std::endl;

    printStatistics("1. Orijinal Gorsel", src, hist_src);
    printStatistics("2. Sadece 0.75 Kontrast (s = 0.75r)", img_scale, hist_scale);
    printStatistics("3. Sadece +20 Parlaklik (s = r + 20)", img_shift, hist_shift);
    printStatistics("4. Bilesik Donusum (s = 0.75r + 20)", img_combo, hist_combo);

    // Matematiksel sinir kontrolleri
    int count_scale_high = 0;
    for (int i = 192; i < 256; ++i) count_scale_high += hist_scale[i];

    int count_shift_low = 0;
    for (int i = 0; i < 20; ++i) count_shift_low += hist_shift[i];

    int count_combo_low = 0;
    for (int i = 0; i < 20; ++i) count_combo_low += hist_combo[i];
    int count_combo_high = 0;
    for (int i = 212; i < 256; ++i) count_combo_high += hist_combo[i];

    std::cout << "\n------------------ ARALIK KONTROL ANALIZI ------------------" << std::endl;
    std::cout << "[Sadece 0.75]: [192, 255] arasi piksel: " << count_scale_high 
              << " (Dinamik aralik daraldigi icin 0 olmali)" << std::endl;
    std::cout << "[Sadece +20] : [0, 19] arasi piksel   : " << count_shift_low 
              << " (Taban +20 otelendigi icin 0 olmali)" << std::endl;
    std::cout << "[Bilesik]    : [0, 19] arasi piksel   : " << count_combo_low 
              << " (Minimum taban degeri 20)" << std::endl;
    std::cout << "[Bilesik]    : [212, 255] arasi piksel: " << count_combo_high 
              << " (Maksimum tavan degeri ~211)" << std::endl;
    std::cout << "------------------------------------------------------------" << std::endl;
    std::cout << "Sonuclar cikti/ ve resimler/ klasorlerine kaydedildi." << std::endl;

    return 0;
}
