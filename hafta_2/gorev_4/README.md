# Hafta 2 - Görev 4: Görüntü Dilimleme ile Paralel Histogram Hesabı ve Thread Analizi

Bu çalışmada, gri seviye test görüntüsü 4 eşit uzamsal parçaya (ROI) bölünerek her bölgenin 256 seviyeli histogramı çoklu iş parçacığı (std::thread) mimarisiyle hesaplanmış, ardından bu alt histogramlar ana histogramda birleştirilerek (reduction) tek thread ile çalışma süreleri karşılaştırılmıştır.

---

## 1. Yöntem ve Algoritma Tasarımı

1. **Giriş Görüntüsü ve Dilimleme:**
   - Test görüntüsü boyutu: **447 x 447** (.809$ piksel).
   - Görüntü bellek kopyalaması yapılmadan cv::Rect pencereleriyle 4 eşit yatay banda bölünmüştür.

2. **Paralel Sayım Mimarisi ve Yarış Durumu (Race Condition) Önleme:**
   - Birden fazla thread'in tek bir global diziye aynı anda yazması durumunda veri kaybı ve kilitlenme (lock contention) yaşanmaması için her thread'e özel std::array<uint32_t, 256> yerel bellek havuzu atanmıştır.
   - 4 ayrı iş parçacığı (std::thread) kendi bölgesindeki piksellerin frekansını bağımsız olarak saymıştır.

3. **Birleştirme (Reduction Adımı):**
   - Tüm iş parçacıklarının çalışması join() ile tamamlandıktan sonra, 4 yerel histogram dizisi tek bir döngüde toplanarak görüntünün genel histogramı elde edilmiştir.

4. **Veri Bütünlüğü Doğrulaması:**
   - Elde edilen ana histogramdaki tüm frekansların toplamı hesaplanmış ve resmin toplam piksel sayısı ile karşılaştırılmıştır:
     - Toplam Piksel: .809$
     - Histogram Toplamı: .809$
     - **Doğrulama Sonucu:** Başarılı (Kayıpsız sayım).

---

## 2. Performans ve Çalışma Süresi Ölçümü

Ölçümler std::chrono::high_resolution_clock kullanılarak 150 tekrarın ortalaması üzerinden hesaplanmıştır:

| Yöntem | Çalışma Süresi (ms) | Hızlanma (Speedup) |
| :--- | :--- | :--- |
| **1 Thread (Sıralı Hesaplama)** | **0.1991 ms** | 1.00x (Referans) |
| **4 Thread (Paralel + Reduction)** | **0.2195 ms** | **~0.91x** |

---

## 3. Deneysel Sonuçların Değerlendirilmesi

- **Hesaplama Süresi ve Maliyet Dengesi:** Tek iş parçacığı yaklaşık 0.199 ms gibi çok kısa bir sürede 200 bin pikselin sayımını tamamlayabilmektedir.
- **Overhead Etkisi:** std::thread havuzunun kurulması, çekirdeklere dağıtılması, join() bekleme süresi ve sonrasındaki 256 indislik 4 dizinin toplanma (reduction) süresi; paralel sayımın sağladığı kazançtan biraz daha fazla bir süre harcamıştır. Bu nedenle hızlanma .91\times$ seviyesinde kalarak paralelleştirmenin küçük veri setlerinde yönetim maliyeti ürettiğini doğrulamıştır.
