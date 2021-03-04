#include "GSM_MQTT.h"
unsigned long ahora;unsigned long lastPing=0;
String MQTT_HOST = "broker.hivemq.com";
String APN="igprs.claro.com.ar";//wap.gprs.unifon.com.ar
long waitPeriod=0;
#define SIMSerial Serial3
#define DEBUGSerial Serial

String MQTT_PORT = "1883";
char TOPIC[]={"SampleToso1"};
GSM_MQTT MQTT(30);
int sketch=0;
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////AUTOCONNECT
void GSM_MQTT::AutoConnect(void){MQTT.connect(random(99999), 0, 0, "", "", 1, 0, 0, 0, "", "");}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////ONCONNECT
void GSM_MQTT::OnConnect(void){MQTT.subscribe(0,random(99999),TOPIC,1);}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////ONMESSAGE
void GSM_MQTT::OnMessage(char *Topic,int TopicLength,char *Message,int MessageLength){/*Serial.println(String(Topic)+"/"+String(Message));*/}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////SETUP
void setup(){
  DEBUGSerial.begin(115200);SIMSerial.begin(115200);for(int n=0;n<=50;n++){DEBUGSerial.println("");}DEBUGSerial.print("Inicio Sketch");
  SIMSerial.println("AT+CIPSHUT\r\n");SIMSerial.println("AT+CFUN=0");delay(2000);SIMSerial.println("AT+CFUN=1");delay(2000);//SIM RESET
  MQTT.begin();
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////LOOP
int sms=0;
void loop(){
  ahora=millis();
  if(DEBUGSerial.available()){
    char tempchar=DEBUGSerial.read();
    if(tempchar=='0'){sketch=0;DEBUGSerial.println("Sketch Stopped!!");MQTT.disconnect();}
    if(tempchar=='1'){sketch=1;DEBUGSerial.println("Sketch Start!!");}
  }
  
  if(ahora-lastPing>60000){
    lastPing=ahora;
    if(sketch==1){if(MQTT.available()){char temp1[500];String(lastPing).toCharArray(temp1,500);MQTT.publish(0,0,0,random(99999),TOPIC,temp1);}}
    if(sketch==0){MQTT.SmsSend(lastPing);}
  }
  if(ahora-waitPeriod>1000L){
    waitPeriod=ahora;
    if(sketch==1){MQTT.processing();}
    if(sketch==0){MQTT.next();}
  }
}
