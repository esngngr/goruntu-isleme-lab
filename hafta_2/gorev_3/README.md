# Hafta 2 - Görev 3: Görüntü Dilimleme (ROI) ve Çoklu İş Parçacığı ile Parlaklık Analizi

Bu çalışmada, tek kanallı gri seviye \lena.png\ görüntüsü üzerinde uzamsal dilimleme yapılarak her bölgeye farklı parlaklık değerleri uygulanmış; tek iş parçacıklı (single-thread) ve 4 iş parçacıklı (\std::thread\) çalışma süreleri deneysel olarak ölçülmüştür.

---

## 1. Yöntem ve Uygulama Adımları

1. **Görüntü Okuma ve Çözünürlük:**
   - Test görüntüsü olarak \lena.png\ kullanıldı.
   - Görüntü matris boyutu: **447 x 447** piksel (\.809\$ piksel).

2. **Bölgesel Dilimleme (ROI - Region of Interest):**
   - Bellekte gereksiz kopyalamayı önlemek adına \cv::Rect\ ile matris pencereleri açıldı ve görüntü 4 yatay parçaya bölündü:
     - **Parça 1 [Y: 0, Yükseklik: 111]:** -60 ofset (Koyulaştırma)
     - **Parça 2 [Y: 111, Yükseklik: 111]:** -20 ofset
     - **Parça 3 [Y: 222, Yükseklik: 111]:** +20 ofset
     - **Parça 4 [Y: 333, Yükseklik: 114]:** +60 ofset (Aydınlatma - Kalan satırlar dahil edildi)

3. **Taşma Kontrolü (Saturate Cast):**
   - Piksel değerleri 8-bit işaretsiz tamsayı (\uchar\, 0-255) olduğundan \\$ altı ve \\$ üstü değerler \cv::saturate_cast<uchar>()\ ile sınırlandırıldı.

4. **İş Parçacığı Mimarisi:**
   - 4 bağımsız ROI dilimi için 4 ayrı \std::thread\ oluşturuldu. Her dilim kendi bellek bölgesine yazdığı için herhangi bir veri çakışması (race condition) yaşanmadı.

---

## 2. Performans ve Çalışma Süresi Ölçümü

Ölçümler \std::chrono::high_resolution_clock\ ile 100 iterasyonun ortalaması alınarak yapılmıştır:

| Yöntem | Çalışma Süresi (ms) | Hızlanma (Speedup) |
| :--- | :--- | :--- |
| **1 Thread (Tek İş Parçacığı)** | **0.2443 ms** | 1.00x (Referans) |
| **4 Thread (Çoklu İş Parçacığı)** | **0.3263 ms** | **~0.75x** |

---

## 3. Deneysel Sonuçların Değerlendirilmesi

Elde edilen sonuçlarda 4 iş parçacıklı yapının tek iş parçacığından daha yavaş kaldığı (\.75\times\$) gözlemlenmiştir. Bu durum paralel programlama literatüründeki temel kurallarla örtüşmektedir:

- **İş Parçacığı Oluşturma Maliyeti (Thread Overhead):** \std::thread\ başlatma, işletim sistemi seviyesinde bağlam değiştirme (context switching) ve \join()\ senkronizasyonu belirli bir zaman maliyeti getirir.
- **İş Yükünün Hafif Olması (Granularity):** \ \times 447\$ çözünürlüğündeki basit bir toplama işlemi tek çekirdekte mikrosaniyeler mertebesinde (0.24 ms) tamamlanabilmektedir. İşlem çok hafif olduğu için thread yönetim maliyeti, paralel çalışmanın kazandırdığı süreden daha baskın hale gelmiştir.
- **Sonuç:** Thread paralelleştirmesi, hesaplama yoğunluğunun (computational complexity) thread yönetim maliyetini aştığı daha büyük görüntülerde veya karmaşık filtreleme işlemlerinde avantaj sağlayacaktır.

---

## 4. Görsel Çıktılar

| Orijinal Görüntü | 4 Parçalı Parlaklık Çıktısı |
| :---: | :---: |
| ![Orijinal](cikti/gorev3_orijinal.png) | ![4 Parçalı Çıktı](cikti/gorev3_parlaklik_4parca.png) |
