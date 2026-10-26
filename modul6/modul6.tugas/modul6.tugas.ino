#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <DHT.h>

// Kredensial Jaringan Wi-Fi
const char* ssid = "oiiu";
const char* password = "22222229";

// Konfigurasi Broker MQTT Publik
const char* mqttServer = "broker.hivemq.com";
const int mqttPort = 1883;
const char* clientId = "ESP8266_LabIoT_25782072_Task";

// Definisi Topik Sesuai NIM 25782072
const char* topicPublish   = "kampus/labiot/25782072/telemetri";
const char* topicAlert     = "kampus/labiot/25782072/alert";
const char* topicStatus    = "kampus/labiot/25782072/status";
const char* topicSubscribe = "kampus/labiot/25782072/kendali";

// Alokasi Pin Perangkat Keras
const byte dhtPin = 2; // Pin D4 (GPIO 2)
const byte ledPin = 5; // Pin D1 (GPIO 5)
#define DHTTYPE DHT22  // Sesuaikan dengan sensor Anda (DHT11 / DHT22)

DHT dht(dhtPin, DHTTYPE);
WiFiClient espClient;
PubSubClient client(espClient);

unsigned long lastTelemetryTime = 0;
const unsigned long telemetryInterval = 4000; // Siklus kirim 4 detik
bool alertSentState = false; // Flag untuk mencegah pengiriman pesan alert berulang (spam)

void callback(char* topic, byte* payload, unsigned int length) {
  char incomingBuffer[length + 1];
  memcpy(incomingBuffer, payload, length);
  incomingBuffer[length] = '\0';

  Serial.println("----------------------------------------");
  Serial.printf("[KONTROL DITERIMA] Topik: %s | Payload: %s\n", topic, incomingBuffer);

  StaticJsonDocument<256> doc;
  DeserializationError err = deserializeJson(doc, incomingBuffer);
  if (err) {
    Serial.println("Gagal membedah payload JSON!");
    return;
  }

  // Kendali manual override LED oleh operator
  if (doc.containsKey("led")) {
    int val = doc["led"];
    digitalWrite(ledPin, val == 1 ? HIGH : LOW);
    Serial.printf("--> [INDIKATOR] Status LED diubah manual: %s\n", val == 1 ? "MENYALA" : "PADAM");
  }
  Serial.println("----------------------------------------");
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Menghubungkan kembali ke Broker...");
    
    // Konfigurasi LWT (Last Will and Testament)
    // Parameter: clientId, willTopic, willQos, willRetain, willMessage
    bool connected = client.connect(
      clientId, 
      topicStatus, 
      1, 
      true, 
      "{\"status\": \"OFFLINE\", \"keterangan\": \"Perangkat Terputus Tak Terduga\"}"
    );

    if (connected) {
      Serial.println(" Sukses Terhubung!");
      
      // Publikasi status ONLINE sesaat setelah berhasil terhubung
      client.publish(topicStatus, "{\"status\": \"ONLINE\", \"keterangan\": \"Perangkat Beroperasi Normal\"}", true);
      
      // Subskripsi topik kendali
      client.subscribe(topicSubscribe);
      Serial.printf("Subskripsi Topik: %s\n", topicSubscribe);
    } else {
      Serial.printf("Gagal (rc=%d). Coba lagi dalam 4 detik...\n", client.state());
      delay(4000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW);

  dht.begin();

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.print("Menghubungkan Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWi-Fi Terkoneksi!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  client.setServer(mqttServer, mqttPort);
  client.setCallback(callback);
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop(); // Memelihara sesi TCP dan memproses callback masuk

  unsigned long now = millis();
  if (now - lastTelemetryTime >= telemetryInterval) {
    lastTelemetryTime = now;

    float t = dht.readTemperature();
    float h = dht.readHumidity();

    if (!isnan(t) && !isnan(h)) {
      // 1. Klasifikasi Kondisi Suhu (Ambang batas 31.0°C)
      String kondisiSuhu = (t > 31.0) ? "PANAS" : "NORMAL";

      // 2. Publikasi Telemetri Rutin berformat JSON
      StaticJsonDocument<256> docOut;
      docOut["node_id"] = clientId;
      docOut["suhu"] = serialized(String(t, 2));
      docOut["kelembapan"] = serialized(String(h, 2));
      docOut["kondisi_suhu"] = kondisiSuhu;
      docOut["status_led"] = digitalRead(ledPin);
      docOut["uptime_detik"] = millis() / 1000;
      docOut["ip_lokal"] = WiFi.localIP().toString();

      char buffer[256];
      serializeJson(docOut, buffer);
      
      bool success = client.publish(topicPublish, buffer);
      if (success) {
        Serial.printf("[TELEMETRI TERKIRIM] %s\n", buffer);
      } else {
        Serial.println("[ERROR] Gagal memublikasikan telemetri!");
      }

      // 3. Logika Rule Engine Keselamatan (Closed-Loop Safety)
      if (t > 31.0) {
        // Otomatis menyalakan indikator LED Bahaya
        digitalWrite(ledPin, HIGH);

        // Publikasi peringatan bahaya ke dedicated alert topic (jika belum terkirim)
        if (!alertSentState) {
          StaticJsonDocument<256> docAlert;
          docAlert["status"] = "BAHAYA";
          docAlert["parameter"] = "OVERHEAT";
          docAlert["nilai_suhu"] = serialized(String(t, 2));

          char alertBuffer[256];
          serializeJson(docAlert, alertBuffer);

          if (client.publish(topicAlert, alertBuffer)) {
            Serial.println("[WARNING] Suhu melebihi 31°C! Paket ALERT terkirim.");
            alertSentState = true; // Latch diaktifkan agar tidak spam
          }
        }
      } else {
        // Reset status alert jika suhu kembali normal
        if (alertSentState) {
          alertSentState = false;
          Serial.println("[INFO] Suhu kembali aman (NORMAL).");
        }
      }
    } else {
      Serial.println("Peringatan: Gagal membaca data dari sensor DHT!");
    }
  }
}