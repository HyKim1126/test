#include <SoftwareSerial.h>
#include <ESP8266.h>
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_PN532.h>
 
#define BT_RXD 2 
#define BT_TXD 3
#define RECV_PIN  12
#define SSID        "SK_WiFiGIGA4A0F"
#define PASSWORD    "ques1234@"
#define HOST_NAME   "192.168.35.190"
#define HOST_PORT   (3000)
#define PN532_IRQ   (9)
#define PN532_RESET (8)

SoftwareSerial ESP_wifi(BT_RXD, BT_TXD);

ESP8266 wifi(ESP_wifi);
Adafruit_PN532 nfc(PN532_IRQ, PN532_RESET);

String password = "";
int OMC = 0;

void setup() {
  // WiFi setup
  bool success = false;
  Serial.begin(9600);
  Serial.print("Configuring ESP8266..");
  do {
    success = wifi.joinAP(SSID, PASSWORD);
    if (success) {
      Serial.print(".");
    } else {
      Serial.println("..ERR: Joining AP");
      Serial.print(" -- Retrying..");
    }
  } while(!success);
  if (wifi.disableMUX()) {
     Serial.println("OK");
  } else {
     Serial.println("ERR: Disabling MUX");
     while (1);
  }
  Serial.println("[ Setup end ]");
  
  // NFC setup
  Serial.print("\r\n\r\nPN532 NFC Start..");
  nfc.begin();
  
  uint32_t versiondata = nfc.getFirmwareVersion();
  if (! versiondata)
  {
      Serial.print("\r\nDidn't find PN53x board");
      while (1); // halt
  }
  Serial.print("\r\nFound chip PN5");
  Serial.print((versiondata>>24) & 0xFF, HEX);
  Serial.print("\r\nFirmware ver. ");
  Serial.print((versiondata>>16) & 0xFF, DEC);
  Serial.print('.');
  Serial.print((versiondata>>8) & 0xFF, DEC);

  nfc.setPassiveActivationRetries(0xFF);
  nfc.SAMConfig(); 
}

void loop() {
  bool success = false;
  uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0 }; 
  uint8_t uidLength = 0;
  
  Serial.print("\r\nWaiting for an ISO14443A card.");

  while(!success){
    success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, &uid[0], &uidLength); 
    Serial.print(".");
    delay(1000);
    if(OMC){
      break;
    }
  }

  OMC = 1;
  
  Serial.print("\r\nFound a card!");
  Serial.print("\r\nUID Length: ");
  Serial.print(uidLength, DEC);
  Serial.print(" bytes");
  Serial.print("\r\nUID Value: ");

    for (uint8_t i = 0; i < uidLength; i++)
    {
        Serial.print(" 0x");
        Serial.print(uid[i], HEX);
        password += (int)uid[i];
    }
    Serial.println("\r\n password : "+ password);

  uint8_t buffer[7]={0};
  String judge = "";
  String stl = "GET /";
  stl += password;
  stl += "\r\n\r\n";
  char* cmd = new char[stl.length()+1];
  strcpy(cmd, stl.c_str());
  Serial.println(cmd);
  if(wifi.send(cmd, strlen(cmd))){
    int len = wifi.recvMP(buffer,300,10000);
    if(len > 0){
      Serial.println("recv OK");
    } else {
      Serial.println("recv ERR.. retrying");
      delete[] cmd;
      cmd = NULL;
      return;
    }
  } else {
    Serial.println("send ERR.. retrying");
    delete[] cmd;
    cmd = NULL;
    return;
  }
  for(int i=0; i<sizeof(buffer);i++){
    judge += (char)buffer[i];
    Serial.print((char)buffer[i]);
  }
  delete[] cmd;
  cmd = NULL;
  password = "";
  OMC = 0;
  Serial.println(judge);

  wifi.releaseTCP();
}
