#include <iostream>
#include <vector>
#include <thread>
#include <chrono>
#include <filesystem>
#include <opencv2/opencv.hpp>

namespace fs = std::filesystem;

// Tek bir ROI dilimini işleyen thread fonksiyonu
void processSlice(const cv::Mat& src, cv::Mat& dst, cv::Rect roi, int brightness_offset) {
    cv::Mat src_slice = src(roi);
    cv::Mat dst_slice = dst(roi);

    int rows = src_slice.rows;
    int cols = src_slice.cols;

    for (int r = 0; r < rows; ++r) {
        const uchar* p_src = src_slice.ptr<uchar>(r);
        uchar* p_dst = dst_slice.ptr<uchar>(r);
        for (int c = 0; c < cols; ++c) {
            p_dst[c] = cv::saturate_cast<uchar>(p_src[c] + brightness_offset);
        }
    }
}

// 1. Tek Thread (Sıralı çalıştırma)
void runSingleThread(const cv::Mat& src, cv::Mat& dst, const std::vector<cv::Rect>& rois, const std::vector<int>& offsets) {
    for (size_t i = 0; i < rois.size(); ++i) {
        processSlice(src, dst, rois[i], offsets[i]);
    }
}

// 2. 4 Thread (Eşzamanlı std::thread çalıştırma)
void runMultiThread(const cv::Mat& src, cv::Mat& dst, const std::vector<cv::Rect>& rois, const std::vector<int>& offsets) {
    std::vector<std::thread> workers;
    workers.reserve(rois.size());

    for (size_t i = 0; i < rois.size(); ++i) {
        workers.emplace_back(processSlice, std::cref(src), std::ref(dst), rois[i], offsets[i]);
    }

    for (auto& th : workers) {
        if (th.joinable()) {
            th.join();
        }
    }
}

int main() {
    std::cout << "==========================================================" << std::endl;
    std::cout << "  HAFTA 2 - GOREV 3: 4 ROI PARCASI & 4 THREAD PARLAKLIK    " << std::endl;
    std::cout << "            1 THREAD vs 4 THREAD KIYASLAMASI              " << std::endl;
    std::cout << "==========================================================" << std::endl;

    std::string img_path = "lena.png";
    cv::Mat img = cv::imread(img_path, cv::IMREAD_GRAYSCALE);

    if (img.empty()) {
        std::cerr << "Hata: " << img_path << " bulunamadi! Colab ana dizinine lena.png yuklediginizden emin olun." << std::endl;
        return -1;
    }

    int total_h = img.rows;
    int total_w = img.cols;
    const int num_parts = 4;
    int slice_h = total_h / num_parts;

    // 4 parça için koordinatlar ve parlaklık ofsetleri (-60, -20, +20, +60)
    std::vector<cv::Rect> rois;
    std::vector<int> offsets = {-60, -20, 20, 60};

    for (int i = 0; i < num_parts; ++i) {
        int y_start = i * slice_h;
        int h = (i == num_parts - 1) ? (total_h - y_start) : slice_h;
        rois.emplace_back(0, y_start, total_w, h);
    }

    cv::Mat out_single = cv::Mat::zeros(total_h, total_w, CV_8UC1);
    cv::Mat out_multi  = cv::Mat::zeros(total_h, total_w, CV_8UC1);

    int test_runs = 100; // Hassas ölçüm ortalaması

    // 1 Thread testi
    auto t1 = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < test_runs; ++i) {
        runSingleThread(img, out_single, rois, offsets);
    }
    auto t2 = std::chrono::high_resolution_clock::now();
    double time_single = std::chrono::duration<double, std::milli>(t2 - t1).count() / test_runs;

    // 4 Thread testi
    auto t3 = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < test_runs; ++i) {
        runMultiThread(img, out_multi, rois, offsets);
    }
    auto t4 = std::chrono::high_resolution_clock::now();
    double time_multi = std::chrono::duration<double, std::milli>(t4 - t3).count() / test_runs;

    fs::create_directories("cikti");
    fs::create_directories("resimler");

    cv::imwrite("resimler/lena.png", img);
    cv::imwrite("cikti/gorev3_orijinal.png", img);
    cv::imwrite("cikti/gorev3_parlaklik_4parca.png", out_multi);

    std::cout << "Goruntu Boyutu       : " << total_w << "x" << total_h << std::endl;
    std::cout << "Bolunen Parca Sayisi : " << num_parts << std::endl;
    for (int i = 0; i < num_parts; ++i) {
        std::cout << "  -> Parca " << i + 1 << " [Y: " << rois[i].y 
                  << ", H: " << rois[i].height << "] Ofset: " << offsets[i] << std::endl;
    }

    std::cout << "\n------------------ SURE KIYASLAMASI ------------------" << std::endl;
    std::cout << "1 Thread (Tek Is Parcacigi)  : " << time_single << " ms" << std::endl;
    std::cout << "4 Thread (Coklu Is Parcacigi): " << time_multi << " ms" << std::endl;
    if (time_multi > 0) {
        std::cout << "Hizlanma (Speedup)          : " << (time_single / time_multi) << "x" << std::endl;
    }
    std::cout << "------------------------------------------------------" << std::endl;
    std::cout << "Ciktilar cikti/ klasorune basariyla kaydedildi." << std::endl;

    return 0;
}
