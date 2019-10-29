#include <SoftwareSerial.h>
#include <IRremote.h>
#include <ESP8266.h>
#define BT_RXD 2 
#define BT_TXD 3
#define LED_RED 8
#define LED_GREEN 9
#define RECV_PIN  12
#define SSID        "SK_WiFiGIGA4A0F"
#define PASSWORD    "ques1234@"
#define HOST_NAME   "192.168.35.190"
#define HOST_PORT   (3000)
SoftwareSerial ESP_wifi(BT_RXD, BT_TXD);
IRrecv irrecv(RECV_PIN);
ESP8266 wifi(ESP_wifi);

decode_results results;
String password = "";

void setup() {
  bool success = false;
  Serial.begin(9600);
  irrecv.enableIRIn();
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
  pinMode(LED_RED, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);
} 
void loop() {
  digitalWrite(LED_RED, LOW);
  digitalWrite(LED_GREEN, LOW);

  Serial.println("Input password.");
  while(results.value!=0x00FF906F){
    if(irrecv.decode(&results)){
      if(results.decode_type == NEC){
        switch(results.value){
          case 0x00FF6897:
            Serial.println("Press '0'");
            password +='0';
            break;
          case 0x00FF30CF:
            Serial.println("Press '1'");
            password +='1';
            break;
          case 0x00FF18E7:
            Serial.println("Press '2'");
            password +='2';
            break;
          case 0x00FF7A85:
            Serial.println("Press '3'");
            password +='3';
            break;
          case 0x00FF10EF:
            Serial.println("Press '4'");
            password +='4';
            break;
          case 0x00FF38C7:
            Serial.println("Press '5'");
            password +='5';
            break;
          case 0x00FF5AA5:
            Serial.println("Press '6'");
            password +='6';
            break;
          case 0x00FF42BD:
            Serial.println("Press '7'");
            password +='7';
            break;
          case 0x00FF4AB5:
            Serial.println("Press '8'");
            password +='8';
            break;                          
          case 0x00FF52AD:
            Serial.println("Press '9'");
            password +='9';
            break;                          
          case 0x00FF906F:
            Serial.println("Complete.");
            break;
          default:
            break;
        }
      }
    irrecv.resume();
    }   
  }
  Serial.println("password : " + password);


  uint8_t buffer[7]={0};
  String judge = "";
  String stl = "GET /";
  stl += password;
  stl += "\r\n\r\n";
  char* cmd = new char[stl.length()+1];
  strcpy(cmd, stl.c_str());
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
  results.value = "";
  Serial.println(judge);
  if(judge.equals("_passed")){
    digitalWrite(LED_GREEN, HIGH);
    Serial.println("Access alowed!");
  } else if(judge.equals("_banned")){
    digitalWrite(LED_RED, HIGH);
    Serial.println("Access banned!");
  } else {
  Serial.println("An error occured!");
  }
  delay(300);
  wifi.releaseTCP();
  

}
