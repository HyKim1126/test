#include <SoftwareSerial.h>
#include <ESP8266.h>
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_PN532.h>
#include <Servo.h>
 
#define BT_RXD 2 
#define BT_TXD 3
#define SERVO_PIN 5

#define SSID        "SK_WiFiGIGA4A0F"
#define PASSWORD    "ques1234@"
#define HOST_NAME   "192.168.35.190"
#define HOST_PORT   (3000)
#define PN532_IRQ   (9)
#define PN532_RESET (8)

SoftwareSerial ESP_wifi(BT_RXD, BT_TXD);
Servo servo;

ESP8266 wifi(ESP_wifi);
Adafruit_PN532 nfc(PN532_IRQ, PN532_RESET);

String password = "";
bool success = false;
uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0 }; 
uint8_t uidLength = 0;

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
  Serial.println("[ WiFi Setup end ]");
  
  // NFC setup
  Serial.print("\r\nPN532 NFC Start..");
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

  //Servo moter setting
  servo.attach(SERVO_PIN);
  servo.write(90);
  delay(300);
  servo.detach();
}


void loop() {
  
  // NFC communication
  Serial.print("\r\nWaiting for an ISO14443A card.");
  while(!success){
    success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, &uid[0], &uidLength); 
    Serial.print(".");
    for (uint8_t i = 0; i < uidLength; i++)
    {
      password += (int)uid[i];
    }
    delay(1000);
  }
  
  Serial.print("\r\nFound a card!");
  Serial.print("\r\nUID Length: ");
  Serial.print(uidLength, DEC);
  Serial.print(" bytes");
  Serial.print("\r\nUID Value: ");
  for (uint8_t i = 0; i < uidLength; i++)
  {
    Serial.print(" 0x");
    Serial.print(uid[i], HEX);
  }
  Serial.println("\r\npassword : "+ password);

  // create TCP
  wifi.createTCP(HOST_NAME, HOST_PORT);

  // create command
  String judge = "";
  String stl = "GET /";
  stl += password;
  stl += "\r\n\r\n";
  char* cmd = new char[stl.length()+1];
  strcpy(cmd, stl.c_str());
  Serial.print("cmd : ");
  Serial.println(cmd);

  // communicate with Server
  if(wifi.send(cmd, strlen(cmd))){
    judge = wifi.recvStringMP("banned", "passed", 2000);
    Serial.println(judge);
      if(judge == "banned"){
        Serial.println("Access banned!");  
      } else if(judge == "passed"){
        Serial.println("Access allowed!");
        servo.attach(SERVO_PIN);
        servo.write(0);
        delay(300);
        servo.detach();
      } else {
        Serial.println("An error occured!");
        wifi.releaseTCP();
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
  
  // variables refresh
  delete[] cmd;
  cmd = NULL;
  password = "";
  success = false;
  for (uint8_t i = 0; i < uidLength; i++)
  {
      uid[i]=0;
  }
  uidLength = 0;
  delay(3000);
  
  // release TCP
  wifi.releaseTCP();
  servo.attach(SERVO_PIN);
  servo.write(90);
  delay(300);
  servo.detach();
}
