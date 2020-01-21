#include <SoftwareSerial.h>
#include <ESP8266.h>
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_PN532.h>
#include <Servo.h>
 
#define BT_RXD      (2) 
#define BT_TXD      (3)
#define SERVO_PIN   (8)
#define BUZZER_PIN  (7)
#define PN532_SCK  (13)
#define PN532_MOSI (11)
#define PN532_SS   (10)
#define PN532_MISO (12)
#define MAIN_BUTTON (4)

//#define DL_NFC_AID  "3ADF2C8692"
//#define SELECT_APDU "00A40400"

#define SSID        "KT_GiGA_2G_Wave2_DD93"
#define PASSWORD    "baf30bd739"
#define HOST_NAME   "172.30.1.42"
#define HOST_PORT   3000
#define PN532_IRQ   9
#define PN532_RESET 8

SoftwareSerial ESP_wifi(BT_RXD, BT_TXD);
Servo servo;

ESP8266 wifi(ESP_wifi);
//Adafruit_PN532 nfc(PN532_SCK, PN532_MISO, PN532_MOSI, PN532_SS);
//Adafruit_PN532 nfc(PN532_IRQ, PN532_RESET);
Adafruit_PN532 nfc(PN532_SS);

const int melody_open[] = {3951,3136,3520,4699};
const int melody_close[] = {3136, 3951, 4699};
const int melody_wrong = 3322;

const uint8_t selectCommand[] = {0x00, 0xa4, 0x04, 0x00, 0x05, 0x3a, 0xdf, 0x2c, 0x86, 0x92};

String userName = "";
String keyValue = "";
bool success = false;
bool successTag = false;
uint8_t responseLength = 52;
uint8_t responseAPDU[52];
bool isOpened = false;
bool isCmdBuilt = false;


void afterDoorOpened();
void openDoorLock();

void setup() {
  // WiFi setup
  
  bool success = false;
  Serial.begin(9600);
  Serial.print("Configuring ESP8266..");  
  do {
    wifi.leaveAP();
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
  pinMode(MAIN_BUTTON, INPUT);
}

void loop() {
  //check door status
  Serial.print(servo.read());
  if(servo.read() < 89){
    afterDoorOpened();
  }
  if((isOpened == false) && (digitalRead(MAIN_BUTTON) == HIGH)){
    openDoorLock();
  }
  //NFC communication
  char instChar = "";  
  Serial.println("\r\nWait for NFC tag");
  success = nfc.inListPassiveTarget();
  if(success){
    success = nfc.inDataExchange(selectCommand, sizeof(selectCommand), responseAPDU, &responseLength);
    if(success){
      tone(BUZZER_PIN, 3951, 200);
      delay(200);
      noTone(BUZZER_PIN);
      Serial.print("responseLength: "); Serial.println(responseLength);
      nfc.PrintHexChar(responseAPDU, responseLength); 
    }
  
    Serial.println("NFC tag completed");
    delay(200);
  
    // Distinguish response
    if (!(responseAPDU[50] == 0x90 && responseAPDU[51] == 0x00)){
      Serial.println("APDU failed.");
      success = false;
      instChar = "";
      for(int Note = 0; Note < 8; Note++){
        tone(BUZZER_PIN, melody_wrong, 200);
        delay(250);
        noTone(BUZZER_PIN);
      }
      return;
    }
  
    // Make userName and keyValue String
    for(int i = 0 ; i < 50 ; i++){
      if((i < 30)){
        if(responseAPDU[i] != 0x00){
          instChar = responseAPDU[i];
          userName += instChar;
        }
      } else {
        instChar = responseAPDU[i];
        keyValue += instChar;
      }
    }
    
    Serial.print("Username is ");
    Serial.println(userName);
    Serial.print("Keyvalue is ");
    Serial.println(keyValue);
  
    // create TCP
    wifi.createTCP(HOST_NAME, HOST_PORT);
    
    // create command
    String judge = "";
    String cmdStr = "GET /judge?userName=" + userName + "&token=" + keyValue + "\r\n\r\n";
    char* cmd = new char[cmdStr.length()+1];
    strcpy(cmd, cmdStr.c_str());
    Serial.print("cmd : ");
    Serial.println(cmd);
    Serial.print("cmdLength : ");
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
          servo.detach();          
        } else if(judge == "_permitted"){
          Serial.println("Access is permitted!");
          openDoorLock();
        } else {
          Serial.println("An error occured!");
          wifi.releaseTCP();
          delete[] cmd;
          cmd = NULL;     
          return;
        }
    } else {
      Serial.println("send ERR.. retrying");
      wifi.releaseTCP();
      delete[] cmd;
      cmd = NULL;
      return;
    }
    
    // variables refresh
    delete[] cmd;
    cmd = NULL;
    userName = "";
    keyValue ="";
    success = false;
    for (uint8_t i = 0; i < sizeof(responseAPDU); i++)
    {
        responseAPDU[i]=0;
    }
    responseLength = 52;
    
    // release TCP
    wifi.releaseTCP();
  }
}

// custom functions
void openDoorLock(){
  Serial.println("Open the door.");
  servo.attach(SERVO_PIN);
  servo.write(0);
  for(int Note=0; Note < 4; Note++){
    tone(BUZZER_PIN, melody_open[Note], 200);
    delay(250);
    noTone(BUZZER_PIN); 
  }
  servo.detach();
  afterDoorOpened();
}

void afterDoorOpened(){
  isOpened = true;
  Serial.println("Door is opened.");
  while(isOpened){
    if(digitalRead(MAIN_BUTTON) == HIGH){
      isOpened = false;
      Serial.println("DoorLock is unlocking.");
      servo.attach(SERVO_PIN);
      servo.write(90);
      for(int Note = 0; Note < 3; Note++){
        tone(BUZZER_PIN, melody_close[Note], 200);
        delay(250);
        noTone(BUZZER_PIN);
      }
      servo.detach();
    }
  }
}
