#include <DHT.h>

#define DHTPIN D7
#define DHTTYPE DHT11

#define LDR_PIN A0
#define RELAY_PIN D6
#define LED_PIN D1

DHT dht(DHTPIN, DHTTYPE);

void setup() {
  Serial.begin(115200);

  pinMode(RELAY_PIN, OUTPUT);
  pinMode(LED_PIN, OUTPUT);

  digitalWrite(RELAY_PIN, HIGH);
  digitalWrite(LED_PIN, LOW);

  dht.begin();

  Serial.println("=== SMART WAREHOUSE ===");
  Serial.println("Sistem Otomasi Kontrol Lokal Terpadu");
  Serial.println();
}

void loop() {

  float suhu = dht.readTemperature();
  int ldr = analogRead(LDR_PIN);

  if (isnan(suhu)) {
    Serial.println("Gagal membaca sensor DHT!");
    delay(2000);
    return;
  }

  Serial.println("------------ KONDISI GUDANG ------------");
  Serial.print("Suhu       : ");
  Serial.print(suhu);
  Serial.println(" C");

  Serial.print("LDR        : ");
  Serial.println(ldr);

  if (suhu > 34 || ldr < 300) {

    digitalWrite(RELAY_PIN, LOW);
    digitalWrite(LED_PIN, HIGH);

    Serial.println("Peringatan: Aktuator Aktif!");

    if (suhu > 34) {
      Serial.println("Alasan     : Suhu terlalu panas!");
    }

    if (ldr < 300) {
      Serial.println("Alasan     : Kondisi terlalu gelap!");
    }

  } else {

    digitalWrite(RELAY_PIN, HIGH);
    digitalWrite(LED_PIN, LOW);

    Serial.println("Kondisi Aman");
    Serial.println("Aktuator    : MATI");
  }

  Serial.println("-----------------------------------------");
  Serial.println();

  delay(2000);
}