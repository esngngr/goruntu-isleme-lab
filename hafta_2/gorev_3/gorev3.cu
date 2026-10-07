#include <iostream>
#include <chrono>
#include <opencv2/opencv.hpp>
#include <cuda_runtime.h>

// CUDA Kernel: Her bir pikseli paralel olarak kuantalar
__global__ void quantizeKernel(const unsigned char* d_input, unsigned char* d_output, int width, int height, int levels) {
    int x = blockIdx.x * blockDim.x + threadIdx.x;
    int y = blockIdx.y * blockDim.y + threadIdx.y;

    if (x < width && y < height) {
        int idx = y * width + x;
        unsigned char val = d_input[idx];
        
        // Kuantalama adimi: [0, 255] araligini 'levels' adete bol ve yeniden olcekle
        int step = 256 / levels;
        unsigned char q_val = (val / step) * step;
        d_output[idx] = q_val;
    }
}

int main() {
    std::cout << "=====================================================" << std::endl;
    std::cout << "  HAFTA 2 - GOREV 3: STANDART KUANTIZASYON (CPU vs CUDA)" << std::endl;
    std::cout << "=====================================================" << std::endl;

    std::string input_path = "resimler/saltpepper.png";
    cv::Mat img = cv::imread(input_path, cv::IMREAD_GRAYSCALE);

    if (img.empty()) {
        std::cerr << "Hata: " << input_path << " bulunamadi!" << std::endl;
        return -1;
    }

    system("mkdir -p cikti");

    int width = img.cols;
    int height = img.rows;
    int total_pixels = width * height;
    int levels = 4; // 4 seviyeli kuantalama (2-bit karsiligi)

    std::cout << "Girdi: " << input_path << " (" << width << "x" << height << ")" << std::endl;
    std::cout << "Kuantalama Seviyesi: " << levels << " seviye" << std::endl;

    // -------------------------------------------------------------------------
    // 1. CPU UZERINDE STANDART KUANTIZASYON & SURE OLCUMU
    // -------------------------------------------------------------------------
    cv::Mat cpu_output = cv::Mat::zeros(height, width, CV_8UC1);
    int step = 256 / levels;

    auto start_cpu = std::chrono::high_resolution_clock::now();

    for (int y = 0; y < height; ++y) {
        const uchar* row_ptr = img.ptr<uchar>(y);
        uchar* out_ptr = cpu_output.ptr<uchar>(y);
        for (int x = 0; x < width; ++x) {
            out_ptr[x] = (row_ptr[x] / step) * step;
        }
    }

    auto end_cpu = std::chrono::high_resolution_clock::now();
    double cpu_time = std::chrono::duration<double, std::milli>(end_cpu - start_cpu).count();

    cv::imwrite("cikti/gorev3_cpu_quantized.png", cpu_output);

    // -------------------------------------------------------------------------
    // 2. GPU (CUDA) UZERINDE KUANTIZASYON & SURE OLCUMU
    // -------------------------------------------------------------------------
    unsigned char *d_input, *d_output;
    size_t bytes = total_pixels * sizeof(unsigned char);

    cudaMalloc(&d_input, bytes);
    cudaMalloc(&d_output, bytes);

    cudaMemcpy(d_input, img.data, bytes, cudaMemcpyHostToDevice);

    dim3 block(16, 16);
    dim3 grid((width + block.x - 1) / block.x, (height + block.y - 1) / block.y);

    cudaEvent_t start_gpu, stop_gpu;
    cudaEventCreate(&start_gpu);
    cudaEventCreate(&stop_gpu);

    cudaEventRecord(start_gpu);
    quantizeKernel<<<grid, block>>>(d_input, d_output, width, height, levels);
    cudaEventRecord(stop_gpu);
    cudaEventSynchronize(stop_gpu);

    float gpu_time = 0.0f;
    cudaEventElapsedTime(&gpu_time, start_gpu, stop_gpu);

    cv::Mat gpu_output = cv::Mat::zeros(height, width, CV_8UC1);
    cudaMemcpy(gpu_output.data, d_output, bytes, cudaMemcpyDeviceToHost);

    cv::imwrite("cikti/gorev3_cuda_quantized.png", gpu_output);

    // Temizlik
    cudaFree(d_input);
    cudaFree(d_output);
    cudaEventDestroy(start_gpu);
    cudaEventDestroy(stop_gpu);

    // -------------------------------------------------------------------------
    // 3. RAPOR VE KIYASLAMA
    // -------------------------------------------------------------------------
    std::cout << "\n------------------ SURE KIYASLAMASI ------------------" << std::endl;
    std::cout << "CPU Islem Suresi   : " << cpu_time << " ms" << std::endl;
    std::cout << "CUDA Kernel Suresi : " << gpu_time << " ms" << std::endl;
    if (gpu_time > 0) {
        std::cout << "Hizlanma (Speedup) : " << (cpu_time / gpu_time) << "x" << std::endl;
    }
    std::cout << "------------------------------------------------------" << std::endl;
    std::cout << "Ciktilar cikti/ klasorune kaydedildi." << std::endl;

    return 0;
}
