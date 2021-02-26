#include "GSM_MQTT.h"
unsigned long ahora;unsigned long lastPing=0;
String MQTT_HOST = "broker.hivemq.com";
String MQTT_PORT = "1883";
char TOPIC[]={"SampleToso1"};
GSM_MQTT MQTT(20);
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////AUTOCONNECT
void GSM_MQTT::AutoConnect(void){connect(random(99999), 0, 0, "", "", 1, 0, 0, 0, "", "");}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////ONCONNECT
void GSM_MQTT::OnConnect(void){MQTT.subscribe(0,random(99999),TOPIC,1);}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////ONMESSAGE
void GSM_MQTT::OnMessage(char *Topic,int TopicLength,char *Message,int MessageLength){/*Serial.println(String(Topic)+"/"+String(Message));*/}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////SETUP
void setup(){
  Serial.begin(115200);Serial3.begin(115200);for(int n=0;n<=50;n++){Serial.println("");}Serial.print("Inicio Sketch");
  Serial3.println("AT+CIPSHUT\r\n");Serial3.println("AT+CFUN=0");delay(2000);Serial3.println("AT+CFUN=1");delay(2000);//SIM RESET
  MQTT.begin();
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////LOOP
int sms=0;
void loop(){
  ahora=millis();
  if (MQTT.available()){
    if(ahora-lastPing>60000){
      lastPing=ahora;sms++;char mensajeStr[500];String(sms).toCharArray(mensajeStr,500);
      MQTT.publish(0,0,0,random(99999),TOPIC,mensajeStr);
    }
  }
  MQTT.processing();
}
