#include <SoftwareSerial.h>
#include <ESP8266.h>
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_PN532.h>
#include <Servo.h>
 
#define BT_RXD 2 
#define BT_TXD 3
#define SERVO_PIN 5
#define BUZZER_PIN 12

#define SSID        "KT_GiGA_2G_Wave2_DD93"
#define PASSWORD    "baf30bd739"
#define HOST_NAME   "172.30.1.42"
#define HOST_PORT   (3000)
#define PN532_IRQ   (9)
#define PN532_RESET (8)

SoftwareSerial ESP_wifi(BT_RXD, BT_TXD);
Servo servo;

ESP8266 wifi(ESP_wifi);
Adafruit_PN532 nfc(PN532_IRQ, PN532_RESET);

int melody_open[] = {3951,3136,3520,4699};
int melody_close[] = {3136, 3951, 4699};
int melody_wrong = 6644;

String password = "";
bool success = false;
uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0 }; 
uint8_t uidLength = 0;

void setup() {
  // WiFi setup
  wifi.leaveAP();
  
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
      password += aa;
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
  String stl = "GET /judge?userName=donghyunna&token=";
  stl += password;
  stl += "\r\n\r\n";
  char* cmd = new char[stl.length()+1];
  strcpy(cmd, stl.c_str());
  Serial.print("cmd : ");
  Serial.println(cmd);
  Serial.println(strlen(cmd));

  // communicate with Server
  if(wifi.send(cmd, strlen(cmd))){
    judge = wifi.recvStringMP("_forbidden", "_permitted", 2000);
    Serial.println(judge);
      if(judge == "_forbidden"){
        Serial.println("Access is forbbiden!");
        for(int Note = 0; Note < 8; Note++){
          tone(BUZZER_PIN, melody_wrong, 200);
          delay(250);
          noTone(BUZZER_PIN);
        }          
      } else if(judge == "_permitted"){
        Serial.println("Access is permitted!");
        servo.attach(SERVO_PIN);
        servo.write(0);
        for(int Note=0; Note < 4; Note++){
          tone(BUZZER_PIN, melody_open[Note], 200);
          delay(250);
          noTone(BUZZER_PIN); 
        }
        servo.detach();
        delay(4000);
        servo.attach(SERVO_PIN);
        servo.write(90);
        for(int Note = 0; Note < 3; Note++){
          tone(BUZZER_PIN, melody_close[Note], 200);
          delay(250);
          noTone(BUZZER_PIN);
        }
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
  
  // release TCP
  wifi.releaseTCP();
}
