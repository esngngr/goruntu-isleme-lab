#include <iostream>
#include <vector>
#include <array>
#include <thread>
#include <chrono>
#include <filesystem>
#include <opencv2/opencv.hpp>

namespace fs = std::filesystem;

// Bit maskeleme yontemiyle hizli kuantalama: p[j] &= mask
void bitwiseQuantizeBlock(cv::Mat block, int target_bits) {
    if (target_bits < 1 || target_bits > 8) return;
    const uint8_t bit_mask = static_cast<uint8_t>(0xFF << (8 - target_bits));

    const int rows = block.rows;
    const int cols = block.cols;

    for (int r = 0; r < rows; ++r) {
        uint8_t* row_data = block.ptr<uint8_t>(r);
        for (int c = 0; c < cols; ++c) {
            row_data[c] &= bit_mask;
        }
    }
}

// 1. Yontem: Tek Cekirdek (Sirali Kuantalama)
void runSequentialQuantization(cv::Mat& image, int bits) {
    bitwiseQuantizeBlock(image, bits);
}

// 2. Yontem: 4 Kadran (2x2 Grid) uzerinde 4 Thread ile Paralel Kuantalama
void runParallelQuadrantQuantization(cv::Mat& image, int bits) {
    const int mid_x = image.cols / 2;
    const int mid_y = image.rows / 2;
    const int right_w = image.cols - mid_x;
    const int bottom_h = image.rows - mid_y;

    // 4 Kadrani (Quadrants) tanimla: Sol-Ust, Sag-Ust, Sol-Alt, Sag-Alt
    const std::array<cv::Rect, 4> quadrants = {
        cv::Rect(0, 0, mid_x, mid_y),
        cv::Rect(mid_x, 0, right_w, mid_y),
        cv::Rect(0, mid_y, mid_x, bottom_h),
        cv::Rect(mid_x, mid_y, right_w, bottom_h)
    };

    std::vector<std::thread> workers;
    workers.reserve(4);

    for (int i = 0; i < 4; ++i) {
        workers.emplace_back(bitwiseQuantizeBlock, image(quadrants[i]), bits);
    }

    for (auto& t : workers) {
        if (t.joinable()) {
            t.join();
        }
    }
}

int main() {
    std::cout << "==========================================================" << std::endl;
    std::cout << "  HAFTA 2 - GOREV 6: 4 KADRAN PARALEL BITSEL KUANTALAMA   " << std::endl;
    std::cout << "         TEK CEKIRDEK vs 4 THREAD SURE KIYASLAMASI        " << std::endl;
    std::cout << "==========================================================" << std::endl;

    std::string img_path = "gorsel1.png";
    cv::Mat original = cv::imread(img_path, cv::IMREAD_GRAYSCALE);

    if (original.empty()) {
        img_path = "gorsel1.jpg";
        original = cv::imread(img_path, cv::IMREAD_GRAYSCALE);
    }

    if (original.empty()) {
        std::cerr << "Hata: Gorsel bulunamadi! 'gorsel1.png' yuklendiginden emin olun." << std::endl;
        return -1;
    }

    const int total_pixels = original.cols * original.rows;
    std::cout << "Girdi Gorseli     : " << img_path << " (" << original.cols << "x" << original.rows << ")" << std::endl;
    std::cout << "Toplam Piksel     : " << total_pixels << std::endl;
    std::cout << "Hedef Kuantalama  : 4-bit (16 Gri Seviyesi, Maske: 0xF0)\n" << std::endl;

    const int target_bits = 4;
    const int test_iterations = 30; // Hassas olcum icin ortalama

    // 1. Tek Cekirdek Testi
    cv::Mat seq_img = original.clone();
    auto t1 = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < test_iterations; ++i) {
        seq_img = original.clone();
        runSequentialQuantization(seq_img, target_bits);
    }
    auto t2 = std::chrono::high_resolution_clock::now();
    double seq_duration_us = std::chrono::duration<double, std::micro>(t2 - t1).count() / test_iterations;

    // 2. 4 Thread Paralel Testi
    cv::Mat par_img = original.clone();
    auto t3 = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < test_iterations; ++i) {
        par_img = original.clone();
        runParallelQuadrantQuantization(par_img, target_bits);
    }
    auto t4 = std::chrono::high_resolution_clock::now();
    double par_duration_us = std::chrono::duration<double, std::micro>(t4 - t3).count() / test_iterations;

    // Ciktilari diske kaydet
    fs::create_directories("cikti");
    fs::create_directories("resimler");

    cv::imwrite("resimler/" + img_path, original);
    cv::imwrite("cikti/gorev6_orijinal.png", original);
    cv::imwrite("cikti/gorev6_kuantalanmis_4bit.png", par_img);

    std::cout << "------------------ SURE KIYASLAMASI ------------------" << std::endl;
    std::cout << "Tek Cekirdek (Sequential) : " << seq_duration_us << " microsecond (" << (seq_duration_us / 1000.0) << " ms)" << std::endl;
    std::cout << "4 Thread (Parallel 2x2)   : " << par_duration_us << " microsecond (" << (par_duration_us / 1000.0) << " ms)" << std::endl;
    if (par_duration_us > 0) {
        std::cout << "Hizlanma (Speedup)        : " << (seq_duration_us / par_duration_us) << "x" << std::endl;
    }
    std::cout << "------------------------------------------------------" << std::endl;
    std::cout << "Kuantalanmis cikti 'cikti/gorev6_kuantalanmis_4bit.png' olarak kaydedildi." << std::endl;

    return 0;
}
