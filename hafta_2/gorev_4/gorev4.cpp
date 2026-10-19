#include <iostream>
#include <vector>
#include <array>
#include <thread>
#include <chrono>
#include <numeric>
#include <filesystem>
#include <opencv2/opencv.hpp>

namespace fs = std::filesystem;

// 256 seviyeli histogram veri tipi
using HistogramBuffer = std::array<uint32_t, 256>;

// Tek bir ROI parcasinin histogramini cikaran cekirdek islev
void extractRoiHistogram(const cv::Mat& image, cv::Rect slice_area, HistogramBuffer& local_hist) {
    local_hist.fill(0);
    cv::Mat roi = image(slice_area);

    const int rows = roi.rows;
    const int cols = roi.cols;

    for (int y = 0; y < rows; ++y) {
        const uint8_t* row_ptr = roi.ptr<uint8_t>(y);
        for (int x = 0; x < cols; ++x) {
            local_hist[row_ptr[x]]++;
        }
    }
}

// 1. Yontem: Tek thread ile 4 parcanin sirayla histogramini hesaplama
HistogramBuffer computeSequential(const cv::Mat& img, const std::vector<cv::Rect>& slices) {
    HistogramBuffer total_hist{};
    total_hist.fill(0);

    HistogramBuffer temp_hist{};
    for (const auto& slice : slices) {
        extractRoiHistogram(img, slice, temp_hist);
        for (size_t b = 0; b < 256; ++b) {
            total_hist[b] += temp_hist[b];
        }
    }
    return total_hist;
}

// 2. Yontem: 4 ayri std::thread ile paralel histogram hesaplama ve birlestirme (Reduction)
HistogramBuffer computeParallel(const cv::Mat& img, const std::vector<cv::Rect>& slices) {
    const size_t num_threads = slices.size();
    std::vector<HistogramBuffer> thread_buffers(num_threads);
    std::vector<std::thread> thread_pool;
    thread_pool.reserve(num_threads);

    // Her thread yalnizca kendi buffer'ina yazar -> Race condition olusmaz
    for (size_t i = 0; i < num_threads; ++i) {
        thread_pool.emplace_back(extractRoiHistogram, std::cref(img), slices[i], std::ref(thread_buffers[i]));
    }

    for (auto& worker : thread_pool) {
        if (worker.joinable()) {
            worker.join();
        }
    }

    // Alt histogramlari tek ana histogramda topla (Reduction adimi)
    HistogramBuffer total_hist{};
    total_hist.fill(0);
    for (size_t i = 0; i < num_threads; ++i) {
        for (size_t b = 0; b < 256; ++b) {
            total_hist[b] += thread_buffers[i][b];
        }
    }
    return total_hist;
}

// Histogrami 512x400 boyutunda grafik olarak cizip diske kaydeden fonksiyon
void renderHistogramPlot(const HistogramBuffer& hist, const std::string& output_filename) {
    const int plot_w = 512;
    const int plot_h = 400;
    const int margin = 20;

    cv::Mat canvas(plot_h, plot_w, CV_8UC3, cv::Scalar(245, 245, 245));

    uint32_t peak = *std::max_element(hist.begin(), hist.end());
    if (peak == 0) peak = 1;

    const double bin_width = static_cast<double>(plot_w) / 256.0;

    for (int i = 0; i < 256; ++i) {
        int bar_height = static_cast<int>((static_cast<double>(hist[i]) / peak) * (plot_h - 2 * margin));
        cv::Point pt1(cvRound(i * bin_width), plot_h - margin);
        cv::Point pt2(cvRound((i + 1) * bin_width), plot_h - margin - bar_height);

        cv::rectangle(canvas, pt1, pt2, cv::Scalar(70, 70, 70), cv::FILLED);
    }

    // Taban cizgisi
    cv::line(canvas, cv::Point(0, plot_h - margin), cv::Point(plot_w, plot_h - margin), cv::Scalar(0, 0, 0), 1);
    cv::imwrite(output_filename, canvas);
}

int main() {
    std::cout << "==========================================================" << std::endl;
    std::cout << "  HAFTA 2 - GOREV 4: 4 PARCA PARALEL HISTOGRAM HESABI     " << std::endl;
    std::cout << "          1 THREAD vs 4 THREAD SURE KIYASLAMASI           " << std::endl;
    std::cout << "==========================================================" << std::endl;

    const std::string input_path = "lena.png";
    cv::Mat image = cv::imread(input_path, cv::IMREAD_GRAYSCALE);

    if (image.empty()) {
        std::cerr << "Hata: " << input_path << " acilamadi!" << std::endl;
        return -1;
    }

    const int width = image.cols;
    const int height = image.rows;
    const int total_pixels = width * height;
    const int num_parts = 4;
    const int base_part_h = height / num_parts;

    // Goruntuyu 4 yatay ROI dilimine ayirma
    std::vector<cv::Rect> rois;
    rois.reserve(num_parts);
    for (int i = 0; i < num_parts; ++i) {
        int y_start = i * base_part_h;
        int h = (i == num_parts - 1) ? (height - y_start) : base_part_h;
        rois.emplace_back(0, y_start, width, h);
    }

    const int iterations = 150; // Sure olcum hassasiyeti
    HistogramBuffer seq_result{}, par_result{};

    // 1 Thread test
    auto t1 = std::chrono::high_resolution_clock::now();
    for (int it = 0; it < iterations; ++it) {
        seq_result = computeSequential(image, rois);
    }
    auto t2 = std::chrono::high_resolution_clock::now();
    double seq_time = std::chrono::duration<double, std::milli>(t2 - t1).count() / iterations;

    // 4 Thread test
    auto t3 = std::chrono::high_resolution_clock::now();
    for (int it = 0; it < iterations; ++it) {
        par_result = computeParallel(image, rois);
    }
    auto t4 = std::chrono::high_resolution_clock::now();
    double par_time = std::chrono::duration<double, std::milli>(t4 - t3).count() / iterations;

    // Dogrulama: Histogramlar birbirine esit mi ve toplam piksel tutuyor mu?
    uint64_t total_counted = 0;
    bool is_identical = true;
    for (size_t b = 0; b < 256; ++b) {
        total_counted += par_result[b];
        if (seq_result[b] != par_result[b]) {
            is_identical = false;
        }
    }

    fs::create_directories("cikti");
    renderHistogramPlot(par_result, "cikti/gorev4_histogram_grafigi.png");

    std::cout << "Gorsel Boyutu          : " << width << "x" << height << " (" << total_pixels << " piksel)" << std::endl;
    std::cout << "Hesaplanan Piksel Topl.: " << total_counted << std::endl;
    std::cout << "Veri Butunlugu         : " 
              << ((total_counted == static_cast<uint64_t>(total_pixels) && is_identical) ? "DOGRULANDI (KUSURSUZ)" : "HATALI") 
              << std::endl;

    std::cout << "\n------------------ SURE KIYASLAMASI ------------------" << std::endl;
    std::cout << "1 Thread (Sirali)      : " << seq_time << " ms" << std::endl;
    std::cout << "4 Thread (Paralel)     : " << par_time << " ms" << std::endl;
    if (par_time > 0) {
        std::cout << "Hizlanma (Speedup)     : " << (seq_time / par_time) << "x" << std::endl;
    }
    std::cout << "------------------------------------------------------" << std::endl;
    std::cout << "Histogram grafigi cikti/gorev4_histogram_grafigi.png olarak kaydedildi." << std::endl;

    return 0;
}
