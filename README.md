# Engelli Yaya Butonu Cihazı Yazılımı

Bu proje, engelli bireyler için tasarlanmış, hoparlörlü ve titreşimli bir yaya butonu cihazının gelişmiş yazılımını içerir. ESP32 mikrodenetleyicisi üzerinde geliştirilen cihaz, **Mongoose Web Server** kütüphanesi sayesinde Ethernet veya Wi-Fi ile web tabanlı uzaktan erişim ve yönetim sunar. Sistem; trafik sinyalizasyonu, yaya talepleri ve geri bildirimleri güvenli biçimde yöneterek şehir içi yaya geçitlerinde erişilebilirliği ve güvenliği artırmayı hedefler.

---

## Temel Özellikler

- **Hoparlörlü Sesli Geri Bildirim:** Cihaz, dahili hoparlör üzerinden yönlendirme ve geri bildirim sesleri sunar.
- **Titreşimli Bildirim:** Butona basıldığında, işitme engelli bireyler için titreşim motoru ile fiziksel uyarı sağlar.
- **Dinamik Ortam Sesi Analizi:** Ortamın gürültü seviyesini analiz ederek hoparlör ses seviyesini otomatik olarak ayarlar.
- **Web Arayüzü ile Konfigürasyon:** Ethernet veya Wi-Fi üzerinden erişilen web paneli ile cihazın tüm ayarları kolayca yönetilebilir.
- **Çoklu Planlama ve Takvim:** Haftanın günlerine veya özel günlere uygun 4 farklı çalışma planı oluşturulabilir ve yönetilebilir.
- **Ses Dosyası Yönetimi:** Farklı durumlar için (bekleme, istek, yeşil ışık, vb.) ayrı ses dosyaları yüklenebilir ve atanabilir.
- **Dashboard ile Durum Takibi:** Web arayüzü üzerinden cihazın anlık durumu, IP ve saat bilgileri görüntülenip güncellenebilir.
- **Kullanıcı Hesapları:** Güvenli erişim için admin ve kullanıcı hesapları desteklenir.

---

## Web Arayüzü Özellikleri

Web arayüzü üzerinden aşağıdaki işlemler kolaylıkla gerçekleştirilebilir:

- Cihazın yapılandırılması (konfigürasyonu)
- Ses dosyası yükleme
- 4 farklı plan oluşturma ve yönetme
- Takvim ve özel gün tanımlamaları
- Dashboard üzerinden genel durum takibi
- IP ve saat ayarlarını güncelleme

Kullanıcı dostu arayüz ile tüm gerekli ayarlar hızlı ve kolay şekilde yapılabilir.

---

## Kullanıcı Hesapları

| Kullanıcı Adı | Şifre           | Açıklama                                      |
|---------------|-----------------|-----------------------------------------------|
| user          | user            | Varsayılan kullanıcı (şifre değiştirilebilir) |
| admin         | !ntetrAPB_5     | Sabit yönetici hesabı (şifre değiştirilemez)  |

> **Not:** `user` hesabının şifresi web arayüzünden değiştirilebilir, ancak `admin` hesabı ve şifresi sabittir ve değiştirilemez.

---

## Sistem Mimarisi
<img width="1075" height="657" alt="SystemArchitecture" src="https://github.com/user-attachments/assets/ad8a9715-8d56-4886-9eca-62176043882c" />

---

## Teknik Detaylar & Dosya Yapısı

- **Donanım:** ESP32 tabanlı mikrodenetleyici, hoparlör, titreşim motoru, SD kart.
- **Yazılım:** C/C++ ile ESP-IDF üzerinde geliştirilmiştir. Mongoose Web Server ile web arayüzü sunar.
- **Ana dosyalar:**
  - `DetectTraffic.c/h`: Giriş algılama, debounce ve durum takibi
  - `SpeakerDriver.c/h`: Ses dosyası oynatma, dinamik ses seviyesi
  - `Plan.h`: Çalışma planı ve takvim yönetimi
  - `mongoose_glue.h`: Web sunucu ve konfigürasyon arayüzleri

---

## Nasıl Kullanılır

1. **Kurulum:** Donanımı bağlayıp cihazı başlatın.
2. **Web Arayüzüne Bağlanın:** Cihazın IP adresini öğrenerek web paneline erişin.
3. **Konfigürasyon:** Web arayüzünden cihaz ayarlarını, planları ve ses dosyalarını yükleyin.
4. **Çalıştırma:** Sistem, trafik sinyalleri ve yaya taleplerini otomatik olarak yönetir; sesli ve titreşimli geri bildirim sağlar.

---

## Bağımlılıklar

- **ESP-IDF:** GPIO ve zamanlayıcı işlemleri
- **Mongoose Web Server:** Web arayüzü
- **SD Kart:** Ses dosyası depolama
- **C Standart Kütüphaneleri:** Temel işlevler için

---

## Yazar

- **Mete SEPETCIOGLU**

---

## Lisans

Bu proje **INTETRA** tarafından geliştirilmiştir. İzinsiz kopyalanamaz, dağıtılamaz veya değiştirilemez.

---

Daha fazla bilgi ve teknik dokümantasyon için kaynak kodu ve `docs` klasörünü inceleyiniz.
