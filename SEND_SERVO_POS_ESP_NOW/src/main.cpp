#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>

#define LED_BUILTIN 2

#define  READ_PIN 18
#define PULSE_PIN 19
const uint32_t  ANALOG_TIMEOUT  = 500;  // = 20ms 20000

uint8_t broadcastAddress[] = {0x40,0x22,0xD8,0x5F,0xF7,0xFC};// mac metalico
//uint8_t broadcastAddress[] = {0x58,0xBF,0x25,0x81,0x3E,0x98};//mac aberto 


// Definir a estrutura para o pacote de dados
typedef struct struct_message {
  int servoA, servoB;
  bool led;
} struct_message;

// Inicializar a estrutura do pacote de dados
struct_message myData;

uint8_t analogDigitalRead(const uint8_t &pin) {
  // Envia pulso
  digitalWrite(PULSE_PIN, HIGH);
  uint32_t us = micros();

  // Aguarda carga do capacitor
  while ((micros() - us) < ANALOG_TIMEOUT && digitalRead(READ_PIN) == LOW);

  // Obtem tempo de carga
  us = micros() - us;

  // Coloca pino em nível baixo para descarga do capacitor
  digitalWrite(PULSE_PIN, LOW);

  // Aguarda descarga (= Tau x 5)
  delay(ANALOG_TIMEOUT / 1000 * 5); // Tempo em ms

  // Verifica valor lido
  if (us > ANALOG_TIMEOUT) {
    // Timeout
    us = ANALOG_TIMEOUT;
  }
  
  // Calcula leitura (0% a 100%)
  return uint8_t(us * 100 / ANALOG_TIMEOUT);
}//end analogDigitalRead

void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t sendStatus) { 
  Serial.println(sendStatus == 0 ? "Entregue":"Não entregue");
  digitalWrite(LED_BUILTIN, sendStatus == 0 ? LOW : HIGH);

}//end OnDataSent

void setup() {
  Serial.begin(115200);
  pinMode(LED_BUILTIN,OUTPUT);
  digitalWrite(LED_BUILTIN,LOW);

  pinMode(PULSE_PIN,  OUTPUT);      // Envio do pulso
  pinMode(READ_PIN,   INPUT);       // Leitura

  WiFi.mode(WIFI_STA);
  Serial.print("RX WiFi.macAddress: ");
  Serial.println(WiFi.macAddress());
  /*
      Available ESP32 RF power parameters:
      WIFI_POWER_19_5dBm    // 19.5dBm (For 19.5dBm of output, highest. Supply current ~150mA)
      WIFI_POWER_19dBm      // 19dBm
      WIFI_POWER_18_5dBm    // 18.5dBm
      WIFI_POWER_17dBm      // 17dBm
      WIFI_POWER_15dBm      // 15dBm
      WIFI_POWER_13dBm      // 13dBm
      WIFI_POWER_11dBm      // 11dBm
      WIFI_POWER_8_5dBm     //  8dBm
      WIFI_POWER_7dBm       //  7dBm
      WIFI_POWER_5dBm       //  5dBm
      WIFI_POWER_2dBm       //  2dBm
      WIFI_POWER_MINUS_1dBm // -1dBm( For -1dBm of output, lowest. Supply current ~120mA)
      Available ESP8266 RF power parameters:
      0    (for lowest RF power output, supply current ~ 70mA
      20.5 (for highest RF power output, supply current ~ 80mA
  */ 
  WiFi.setTxPower(WIFI_POWER_19_5dBm);
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  esp_now_peer_info_t peerInfo;
  memset(&peerInfo, 0, sizeof(peerInfo));
  for (int ii = 0; ii < 6; ++ii )
  {
    peerInfo.peer_addr[ii] = (uint8_t) broadcastAddress[ii];
  }
  peerInfo.channel = 0;
  peerInfo.encrypt = false;
  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add peer");
    return;
  }
  esp_now_register_send_cb(OnDataSent);
}//end setup

void loop() {

  int adc = analogDigitalRead(READ_PIN);
  Serial.print(adc);
  Serial.println("");
  delay(1);

  digitalWrite(LED_BUILTIN, HIGH);
  // Construir o pacote de dados
  //strcpy(myData.data, "Hello world");

  //myData.servoA < 180 ? myData.servoA += 10 : myData.servoA = 0;
  //myData.servoB < 180 ? myData.servoB += 10 : myData.servoB = 0;

  myData.servoA = adc;

  myData.led = !myData.led;

  //Serial.println(myData.servoA);
  //Serial.println(myData.servoB);

  // Enviar o pacote de dados para o receptor
  esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *) &myData, sizeof(myData));
  //Serial.println(result == ESP_OK?"PKG_OK":"PKG_ERROR");
  //delay(1000);
}//end loop