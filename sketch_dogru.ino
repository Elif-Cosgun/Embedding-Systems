// 2 interrupt butonlarda ve bir timer ledde
//Gerekli kütüphaneler
#include <Wire.h>
#include <RTClib.h>
#include <LiquidCrystal_I2C.h>
#include <TimerOne.h>
#include <Adafruit_PWMServoDriver.h>
//LCD i2c bağlantısı
LiquidCrystal_I2C lcd(0x27, 16, 2);
RTC_DS3231 rtc; //zaman modülü
Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver(); // Servo shield

//Pinler
const int buttonStartPin = 2;
const int buttonExitPin = 3;
const int sensorPin = 4;

const int redPin = 9;
const int greenPin = 10;
const int bluePin = 11;
//zorluk seviyeleri
const char* difficultyNames[3] = {"KOLAY", "ORTA", "ZOR"};
const unsigned long gameDuration = 60000; //oyun süresi 1 dakika

volatile bool startInterruptFlag = false; //Başlangıç interrupt flag
volatile bool exitInterruptFlag = false; //sıfırlama interrupt flag

//Zorluk seçimi için gerekenler
int difficulty = 0;
unsigned long lastDifficultyChange = 0;
unsigned long gameStartTime = 0;
unsigned long lastScoreShow = 0;

bool inDifficultySelection = true;
bool gameRunning = false;
bool gameOver = false;
bool showingScores = false;
//Skor tablosu
struct ScoreEntry {
  DateTime time;
  int score;
};

ScoreEntry scores[3];
int scoreIndex = 0;
int lastActivatedSecond = -1;

//Setup fonksiyonu
void setup() {
  Serial.begin(9600);
  pinMode(buttonStartPin, INPUT_PULLUP);
  pinMode(buttonExitPin, INPUT_PULLUP);
  pinMode(sensorPin, INPUT);
  pinMode(redPin, OUTPUT);
  pinMode(greenPin, OUTPUT);
  pinMode(bluePin, OUTPUT);

  lcd.init();
  lcd.backlight();

  pwm.begin();
  pwm.setPWMFreq(50);

//LCD bağlantısı kontrolü
  if (!rtc.begin()) {
    lcd.print("RTC bulunamadi!");
    while (1);
  }
  if (rtc.lostPower()) rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));

  for (int i = 0; i < 3; i++) scores[i].score = 9999;

//interruptlar
  attachInterrupt(digitalPinToInterrupt(buttonStartPin), onStartInterrupt, FALLING);
  attachInterrupt(digitalPinToInterrupt(buttonExitPin), onExitInterrupt, FALLING);

  showDifficulty();
  lastDifficultyChange = millis();
}

//3 saniyede bir zorluk ekranı döngüsü
void loop() {
  unsigned long now = millis();

  if (inDifficultySelection) {
    if (now - lastDifficultyChange >= 3000) {
      difficulty = (difficulty + 1) % 3;
      showDifficulty();
      lastDifficultyChange = now;
    }

    if (startInterruptFlag) {
      startInterruptFlag = false;
      startGame();
    }
  }
//oyun başlayınca ekranda zorluk ve süre gösterilsin
  else if (gameRunning) {
    unsigned long elapsed = now - gameStartTime;
    int seconds = elapsed / 1000;

    lcd.setCursor(0, 0);
    lcd.print("Zorluk:");
    lcd.print(difficultyNames[difficulty]);
    lcd.print("     ");

    lcd.setCursor(0, 1);
    lcd.print("Sure:");
    lcd.print(seconds);
    lcd.print("sn   ");

    checkServoActivation(seconds);

    if (digitalRead(sensorPin) == HIGH || elapsed >= gameDuration) {
      endGame(seconds);
    }
  }

//oyun bitince ekranda oyun bitti yazsın
  else if (gameOver) {
    if (!showingScores) {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("OYUN BITTI!");
      delay(1500);
      showingScores = true;  // Burada skor tablosunu açıyoruz
      scoreIndex = 0;
      lastScoreShow = millis();
      lcd.clear();
      //Skor tablosunu göster
    } else {
      // Skorları sırayla göster
      if (millis() - lastScoreShow >= 3000) {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Skor ");
        lcd.print(scoreIndex + 1);
        lcd.print(":");

        lcd.setCursor(0, 1);
        if (scores[scoreIndex].score == 9999) {
          lcd.print("---");
        } else {
          DateTime t = scores[scoreIndex].time;
          if (t.hour() < 10) lcd.print("0");
          lcd.print(t.hour());
          lcd.print(":");
          if (t.minute() < 10) lcd.print("0");
          lcd.print(t.minute());
          lcd.print(" ");
          lcd.print(scores[scoreIndex].score);
          lcd.print("sn");
        }

        scoreIndex = (scoreIndex + 1) % 3;
        lastScoreShow = millis();
      }

      // Exit butonuna basılırsa skor tablosunu kapatıp zorluk seçimine geç
      if (exitInterruptFlag) {
        exitInterruptFlag = false;
        showingScores = false;
        gameOver = false;
        inDifficultySelection = true;
        showDifficulty();
        lastDifficultyChange = millis();
        lcd.clear();
      }
    }
  }
}

//Zorrluk seçimi 
void showDifficulty() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Zorluk Secimi:");
  lcd.setCursor(0, 1);
  lcd.print(difficultyNames[difficulty]);
}

// Oyun başlasın
void startGame() {
  inDifficultySelection = false;
  gameRunning = true;
  gameOver = false;
  showingScores = false;

  gameStartTime = millis();
  Timer1.initialize(1000000); // 1 saniyede bir timerCallback çağrılır
  Timer1.attachInterrupt(timerCallback);

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Oyun Basliyor...");
  delay(1000);
  lcd.clear();
}

//Oyun bitti
void endGame(int score) {
  gameRunning = false;
  gameOver = true;

  Timer1.detachInterrupt();
  turnOffRGB();
  stopAllServos();
  addScore(score);
}

//Skor tablsona ekleme
void addScore(int newScore) {
  DateTime now = rtc.now();
  int worstIndex = 0;
  for (int i = 1; i < 3; i++) {
    if (scores[i].score > scores[worstIndex].score) worstIndex = i;
  }

  if (newScore < scores[worstIndex].score) {
    scores[worstIndex].score = newScore;
    scores[worstIndex].time = now;
  }
}

//interrupt0
void onStartInterrupt() {
  startInterruptFlag = true;
}

//interrupt1
void onExitInterrupt() {
  exitInterruptFlag = true;
}

//timer0
void timerCallback() {
  static bool ledOn = false;
  if (ledOn) turnOffRGB();
  else {
    if (difficulty == 0) setRGB(0, 255, 0);
    else if (difficulty == 1) setRGB(255, 165, 0);
    else setRGB(255, 0, 0);
  }
  ledOn = !ledOn;
}

//Led kontrol
void setRGB(int r, int g, int b) {
  analogWrite(redPin, 255 - r);
  analogWrite(greenPin, 255 - g);
  analogWrite(bluePin, 255 - b);
}

//Led kapama
void turnOffRGB() {
  setRGB(0, 0, 0);
}

//Servo hareketi
void moveServo(int ch) {
  pwm.setPWM(ch, 0, 120); // sola
  delay(150);
  pwm.setPWM(ch, 0, 520); //sağa
}

//bitince ilk (orta) konuma dön
void stopAllServos() {
  for (int i = 0; i < 16; i++) {
    pwm.setPWM(i, 0, 370);
  }
}

//Hareketi kontrol et
void checkServoActivation(int seconds) {
  int interval = (difficulty == 0) ? 15 : (difficulty == 1 ? 12 : 9);
  int count = (difficulty == 0) ? 8 : (difficulty == 1 ? 12 : 16);

  if (seconds > 0 && seconds % interval == 0 && seconds != lastActivatedSecond) {
    lastActivatedSecond = seconds;
    for (int i = 0; i < count; i++) {
      moveServo(i);
    }
  }
}   