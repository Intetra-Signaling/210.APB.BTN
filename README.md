# Engelli Yaya Butonu Cihazı Yazılımı

Bu projede, Engelli Yaya Butonu Cihazı için geliştirilen yazılım bulunmaktadır. Yazılımda, ESP32 mikrodenetleyicisi üzerinde **Mongoose Web Server** kütüphanesi kullanılmıştır. Cihaz, Ethernet veya Wi-Fi bağlantısı üzerinden kendi IP adresiyle erişilebilen bir web arayüzü sunar.

Web arayüzü üzerinden aşağıdaki işlemler kolaylıkla gerçekleştirilebilir:

- Cihazın yapılandırılması (konfigürasyonu)
- Ses dosyası yükleme
- 4 farklı plan oluşturma ve yönetme
- Takvim ve özel gün tanımlamaları
- Dashboard üzerinden genel durum takibi
- IP ve saat ayarlarını güncelleme

Geliştirilen bu arayüz sayesinde, cihazın tüm gerekli ayarları kullanıcı dostu bir şekilde yapılabilmektedir.


## Kullanıcı Hesapları

|---------------|-----------------|-----------------------------------------------|
| Kullanıcı Adı | Şifre           | Açıklama                                      |
|---------------|-----------------|-----------------------------------------------|
| user          | user            | Varsayılan kullanıcı (şifre değiştirilebilir) |
|---------------|-----------------|-----------------------------------------------|
| admin         | !ntetrAPB_5     | Sabit yönetici hesabı (şifre değiştirilemez)  |
|---------------|-----------------|-----------------------------------------------|

> **Not:** `user` hesabının şifresi web arayüzünden değiştirilebilir, ancak `admin` hesabı ve şifresi sabittir ve değiştirilemez.