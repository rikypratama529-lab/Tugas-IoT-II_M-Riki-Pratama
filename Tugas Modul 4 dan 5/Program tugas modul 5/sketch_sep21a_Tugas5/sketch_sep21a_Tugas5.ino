#include <painlessMesh.h>
#include <ArduinoJson.h>

#define MESH_PREFIX     "LabIoTMesh"
#define MESH_PASSWORD   "iotmeshpassword"
#define MESH_PORT       5555

#define LED_PIN         12      // Pin D6 (GPIO 12) pada NodeMCU

Scheduler userScheduler;
painlessMesh mesh;

// Variabel untuk menyimpan status pembacaan terakhir
float suhuTerakhir = 0.0;
int adcTerakhir = 1023; // Default kondisi terang

void evaluateRuleEngine() {
  // Rule Engine: 
  // Nyalakan LED jika Suhu > 31.0°C ATAU nilai ADC LDR < 300 (kondisi gelap)
  if (suhuTerakhir > 31.0 || adcTerakhir < 300) {
    digitalWrite(LED_PIN, HIGH);
    Serial.println("-> [RULE ENGINE] Kondisi Kritis/Gelap! LED NYALA (HIGH)");
  } else {
    digitalWrite(LED_PIN, LOW);
    Serial.println("-> [RULE ENGINE] Kondisi Aman/Normal. LED MATI (LOW)");
  }
}

void receivedCallback(uint32_t from, String &msg) {
  StaticJsonDocument<200> doc;
  DeserializationError error = deserializeJson(doc, msg);

  if (!error) {
    const char* tipe = doc["tipe"];

    if (strcmp(tipe, "suhu_node") == 0) {
      suhuTerakhir = doc["suhu"];
      float kelembapan = doc["kelembapan"];
      Serial.printf("[TERIMA SUHU] Dari Node %u -> Suhu: %.2f °C, Kelembapan: %.2f %%\n", from, suhuTerakhir, kelembapan);
    } 
    else if (strcmp(tipe, "cahaya_node") == 0) {
      adcTerakhir = doc["adc"];
      Serial.printf("[TERIMA CAHAYA] Dari Node %u -> Nilai ADC: %d\n", from, adcTerakhir);
    }

    // Evaluasi aturan setiap kali ada data baru yang masuk
    evaluateRuleEngine();
    Serial.println("----------------------------------------");
  } else {
    Serial.printf("[TERIMA MENTAH DARI %u]: %s\n", from, msg.c_str());
  }
}

void newConnectionCallback(uint32_t nodeId) {
  Serial.printf("--> Koneksi Baru Terdeteksi! Node ID: %u\n", nodeId);
}

void changedConnectionCallback() {
  Serial.println("--> Topologi rantai mesh telah diperbarui");
}

void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW); // Pastikan awal mati

  mesh.setDebugMsgTypes(ERROR | STARTUP);
  mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT);
  mesh.onReceive(&receivedCallback);
  mesh.onNewConnection(&newConnectionCallback);
  mesh.onChangedConnections(&changedConnectionCallback);

  Serial.println("Node 3 (Hub Kontrol Aktuator) Siap Menunggu Data...");
}

void loop() {
  mesh.update();
}