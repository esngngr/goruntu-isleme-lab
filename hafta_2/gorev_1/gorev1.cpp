#include <iostream>
#include <chrono>
#include <opencv2/opencv.hpp>

int main() {
    std::cout << "=====================================================" << std::endl;
    std::cout << "  HAFTA 2 - GOREV 1: MEKANSAL COZUNURLUK & DONANIM   " << std::endl;
    std::cout << "=====================================================" << std::endl;

    std::string input_path = "resimler/saltpepper.png";
    cv::Mat img = cv::imread(input_path, cv::IMREAD_COLOR);

    if (img.empty()) {
        std::cerr << "Hata: " << input_path << " bulunamadi! resimler klasorunde saltpepper.png oldugundan emin olun." << std::endl;
        return -1;
    }

    system("mkdir -p cikti");

    std::cout << "Girdi Gorseli: " << input_path << std::endl;
    std::cout << "Orijinal Boyut (BGR): " << img.cols << "x" << img.rows 
              << " | Kanallar: " << img.channels() << std::endl;

    // 1. Kanal Donusumu: BGR -> Grayscale
    cv::Mat gray_img;
    cv::cvtColor(img, gray_img, cv::COLOR_BGR2GRAY);
    cv::imwrite("cikti/gray_img.png", gray_img);

    // 2. CPU Uzerinde Resampling ve Sure Olcumu
    auto start_cpu = std::chrono::high_resolution_clock::now();

    cv::Mat downsampled, upsampled;
    // 0.5x boyutlandirma
    cv::resize(gray_img, downsampled, cv::Size(), 0.5, 0.5, cv::INTER_LINEAR);
    // 2.0x boyutlandirma
    cv::resize(gray_img, upsampled, cv::Size(), 2.0, 2.0, cv::INTER_LINEAR);

    auto end_cpu = std::chrono::high_resolution_clock::now();
    double cpu_duration = std::chrono::duration<double, std::milli>(end_cpu - start_cpu).count();

    cv::imwrite("cikti/resampled_0.5x.png", downsampled);
    cv::imwrite("cikti/resampled_2.0x.png", upsampled);

    std::cout << "\n[CPU Sonuclari]" << std::endl;
    std::cout << "-> 0.5x Boyut: " << downsampled.cols << "x" << downsampled.rows << std::endl;
    std::cout << "-> 2.0x Boyut: " << upsampled.cols << "x" << upsampled.rows << std::endl;
    std::cout << "-> CPU Toplam Resample Suresi: " << cpu_duration << " ms" << std::endl;

    // 3. Donanim & CUDA Bilgisi
    std::cout << "\n[Donanim Hizlandirma Analizi]" << std::endl;
    int gpu_check = std::system("nvidia-smi > /dev/null 2>&1");
    if (gpu_check == 0) {
        std::cout << ">> Sistemde Nvidia CUDA GPU tespit edildi." << std::endl;
    } else {
        std::cout << ">> GPU bulunamadi, yalnizca CPU profil cikarildi." << std::endl;
    }

    return 0;
}
