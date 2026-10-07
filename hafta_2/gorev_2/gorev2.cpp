#include <iostream>
#include <opencv2/opencv.hpp>

int main() {
    std::cout << "=====================================================" << std::endl;
    std::cout << "  HAFTA 2 - GOREV 2: GENLIK COZUNURLUGU & KUANTALAMA " << std::endl;
    std::cout << "=====================================================" << std::endl;

    std::string input_path = "resimler/saltpepper.png";
    cv::Mat color_img = cv::imread(input_path, cv::IMREAD_COLOR);

    if (color_img.empty()) {
        std::cerr << "Hata: " << input_path << " bulunamadi!" << std::endl;
        return -1;
    }

    system("mkdir -p cikti");

    // 1. Kanal Donusumu: BGR -> Grayscale (8-bit: [0, 255])
    cv::Mat gray_img;
    cv::cvtColor(color_img, gray_img, cv::COLOR_BGR2GRAY);
    cv::imwrite("cikti/gray_original_8bit.png", gray_img);

    int rows = gray_img.rows;
    int cols = gray_img.cols;

    // Cikti matrisleri (8-bit tek kanal olarak ilklendirilir)
    cv::Mat raw_6bit = cv::Mat::zeros(rows, cols, CV_8UC1);
    cv::Mat posterized_6bit = cv::Mat::zeros(rows, cols, CV_8UC1);

    // 2. Ic ice donguler ile dogrudan piksel taramasi (Noktasal Islem)
    for (int y = 0; y < rows; ++y) {
        for (int x = 0; x < cols; ++x) {
            // Orijinal 8-bit piksel degeri [0, 255]
            uchar original_pixel = gray_img.at<uchar>(y, x);

            // 6-bit kuantalama: [0, 63] araligina indirgeme (2^6 = 64 seviye)
            uchar quantized_val = original_pixel / 4;
            raw_6bit.at<uchar>(y, x) = quantized_val;

            // Yeniden olcekleme: [0, 252] araligina genisleterek posterizasyon olusturma
            posterized_6bit.at<uchar>(y, x) = quantized_val * 4;
        }
    }

    // 3. Dosyalari diske kaydetme
    cv::imwrite("cikti/quantized_raw_6bit.png", raw_6bit);
    cv::imwrite("cikti/quantized_posterized_6bit.png", posterized_6bit);

    std::cout << "Girdi Gorseli     : " << input_path << std::endl;
    std::cout << "Cozunurluk        : " << cols << "x" << rows << std::endl;
    std::cout << "Islenen Piksel    : " << rows * cols << " adet piksel gezildi." << std::endl;
    std::cout << "\n[Uretilen Ciktilar]" << std::endl;
    std::cout << "-> cikti/gray_original_8bit.png       (Orijinal Gri, 8-bit)" << std::endl;
    std::cout << "-> cikti/quantized_raw_6bit.png        (Saf 6-bit: [0, 63], karanlik)" << std::endl;
    std::cout << "-> cikti/quantized_posterized_6bit.png (Posterize: [0, 252], basamaklanma)" << std::endl;
    std::cout << "=====================================================" << std::endl;

    return 0;
}
