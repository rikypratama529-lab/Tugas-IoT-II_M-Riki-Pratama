/*
  Nama : M Riki Pratama
  NPM  : 25782072
  Tugas: Sistem Sakelar Toggle (Latching)
*/

const int buttonPin = 4;
const int ledPin = 5;

int buttonState = 0;
bool ledState = false;  // Menyimpan status LED, awalnya OFF

void setup() {

  Serial.begin(115200);

  pinMode(buttonPin, INPUT);
  pinMode(ledPin, OUTPUT);

  // Saat sistem pertama dinyalakan, LED dalam keadaan mati
  digitalWrite(ledPin, LOW);
}

void loop() {

  buttonState = digitalRead(buttonPin);

  // Jika tombol ditekan
  if (buttonState == HIGH) {

    // Mengubah status LED setiap satu kali tekanan
    ledState = !ledState;

    // Menyalakan atau mematikan LED sesuai status
    digitalWrite(ledPin, ledState ? HIGH : LOW);

    if (ledState) {
      Serial.println("Tombol ditekan! -> LED ON");
    } else {
      Serial.println("Tombol ditekan! -> LED OFF");
    }

    // Debounce untuk mencegah satu tekanan terbaca berkali-kali
    delay(200);

    // Menunggu sampai tombol dilepas
    while (digitalRead(buttonPin) == HIGH) {
      delay(10);
    }
  }
}