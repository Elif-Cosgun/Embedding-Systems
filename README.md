# Hareketli Duvarlı Labirent Oyunu

Zorluk seviyesine göre servo motorlarla hareket eden duvarları olan, süre sınırlı ve skor tablosu
tutan fiziksel bir Arduino labirent oyunu.

Gömülü Sistemler dersi dönem projesi olarak geliştirilmiştir.

## Oyun akışı

1. **Zorluk seçimi** — LCD ekranda 3 saniyede bir KOLAY → ORTA → ZOR döner, butonla seçilir
2. **Oyun** — 60 saniyelik süre başlar, duvarlar belirli aralıklarla hareket eder
3. **Bitiş** — top çıkışa ulaşıp sensörü tetiklerse ya da süre dolarsa oyun biter
4. **Skor tablosu** — en iyi 3 derece saat damgasıyla LCD'de sırayla gösterilir

## Zorluk seviyeleri

Seviye hem hareket **sıklığını** hem hareket eden **duvar sayısını** birlikte değiştirir:

| Seviye | Hareket aralığı | Hareket eden duvar | LED |
|--------|-----------------|--------------------|-----|
| Kolay  | 15 saniye       | 8 servo            | Yeşil |
| Orta   | 12 saniye       | 12 servo           | Turuncu |
| Zor    | 9 saniye        | 16 servo           | Kırmızı |

## Teknik detaylar

**Donanım kesmeleri.** Başlat ve çıkış butonları `attachInterrupt` ile kesmeye bağlıdır. Ana döngü
oyun sırasında LCD güncelleme ve servo kontrolüyle meşgul olduğundan, butonu döngü içinde
sorgulamak basışların kaçmasına yol açardı. Kesme fonksiyonunda yalnızca bayrak set edilir,
asıl iş ana döngüde yapılır.

**Timer kesmesi.** LED'in saniyede bir yanıp sönmesi `TimerOne` ile yürütülür; ana döngüyü bloklamaz.

**I2C hattı.** LCD ekran, gerçek zaman modülü ve servo sürücü kartı aynı iki hat üzerinde farklı
adreslerle çalışır. Arduino Uno'nun sınırlı pin sayısı bu sayede yeterli olur.

**Servo sürücü.** 16 servoyu doğrudan Arduino'dan sürmek mümkün değildir; PCA9685 kartı bunu
I2C üzerinden çözer.

## Donanım

`Arduino Uno` · `PCA9685 Servo Shield` · `DS3231 RTC` · `16x2 I2C LCD` · `RGB LED` · `Hareket sensörü` · `Butonlar`

## Kütüphaneler

`Wire` · `RTClib` · `LiquidCrystal_I2C` · `TimerOne` · `Adafruit_PWMServoDriver`

## Not

Skorlar oturum boyunca RAM'de tutulur; cihaz kapandığında sıfırlanır. Kalıcı saklama için
EEPROM'a yazma eklenebilir — projenin geliştirilmeye açık yanlarından biridir.
