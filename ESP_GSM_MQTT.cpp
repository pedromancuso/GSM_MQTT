#define SIMSerial Serial
#define DEBUGSerial Serial
#include "ESP_GSM_MQTT.h"
void Monitor(String str){/*DEBUGSerial.println("");DEBUGSerial.print(str);*/}
#include "Arduino.h"
#include <avr/pgmspace.h>
char RcvdMsg[200] = "";int RcvdConf = 0;int count = 0;int RcvdEnd = 0;char MsgMob[15];char MsgTxt[50];int MsgLength = 0;
const char CONNECTMessage[] PROGMEM={"Client request to connect to Server\r\n"};
const char CONNACKMessage[] PROGMEM={"Connect Acknowledgment\r\n"};
const char PUBLISHMessage[] PROGMEM={"Publish message\r\n"};
const char PUBACKMessage[] PROGMEM={"Publish Acknowledgment\r\n"};
const char PUBRECMessage[] PROGMEM={"Publish Received (assured delivery part 1)\r\n"};
const char PUBRELMessage[] PROGMEM={"Publish Release (assured delivery part 2)\r\n"};
const char PUBCOMPMessage[] PROGMEM={"Publish Complete (assured delivery part 3)\r\n"};
const char SUBSCRIBEMessage[] PROGMEM={"Client Subscribe request\r\n"};
const char SUBACKMessage[] PROGMEM={"Subscribe Acknowledgment\r\n"};
const char UNSUBSCRIBEMessage[] PROGMEM={"Client Unsubscribe request\r\n"};
const char UNSUBACKMessage[] PROGMEM={"Unsubscribe Acknowledgment\r\n"};
const char PINGREQMessage[] PROGMEM={"PING Request\r\n"};
const char PINGRESPMessage[] PROGMEM={"PING Response\r\n"};
const char DISCONNECTMessage[] PROGMEM={"Client is Disconnecting\r\n"};
const char ConnectAck0[] PROGMEM={"Connection Accepted\r\n"};
const char ConnectAck1[] PROGMEM={"Connection Refused: unacceptable protocol version\r\n"};
const char ConnectAck2[] PROGMEM={"Connection Refused: identifier rejected\r\n"};
const char ConnectAck3[] PROGMEM={"Connection Refused: server unavailable\r\n"};
const char ConnectAck4[] PROGMEM={"Connection Refused: bad user name or password\r\n"};
const char ConnectAck5[] PROGMEM={"Connection Refused: not authorized\r\n"};
extern uint8_t GSM_Response;uint8_t GSM_Response = 0;unsigned long previousMillis = 0;boolean stringComplete = false;
extern String mqttbroker;extern int mqttport;extern GSM_MQTT MQTT;extern String APN;int HangUp=0;
void serialEvent();
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////GSM_MQTT
GSM_MQTT::GSM_MQTT(unsigned long KeepAlive){_KeepAliveTimeOut = KeepAlive;}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////BEGIN
void GSM_MQTT::begin(void){SIMSerial.write("AT\r\n");delay(1000);_tcpInit();}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////SENDAT
char GSM_MQTT::_sendAT(char *command, unsigned long waitms){
  unsigned long PrevMillis = millis();
  strcpy(reply, "none");
  GSM_Response = 0;
  SIMSerial.write(command);
  unsigned long currentMillis=millis();
  while((GSM_Response==0)&&((currentMillis-PrevMillis)<waitms)){serialEvent();currentMillis=millis();}
  return GSM_Response;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////SENDATREPLY
char GSM_MQTT::sendATreply(char *command, char *replystr, unsigned long waitms){
  strcpy(reply,replystr);
  unsigned long PrevMillis=millis();
  GSM_ReplyFlag=0;
  SIMSerial.write(command);
  unsigned long currentMillis=millis();
  while((GSM_ReplyFlag==0)&&((currentMillis-PrevMillis)<waitms)){serialEvent();currentMillis=millis();}
  return GSM_ReplyFlag;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////TCPINIT
void GSM_MQTT::_tcpInit(void){
  switch(modemStatus){
    case 0:{delay(1000);SIMSerial.print("+++");delay(500);
         if(_sendAT("AT\r\n",5000)==1){_sendAT("AT+CMGF=1\n",2000);_sendAT("AT+CNMI=1,2,0,0,0\n",2000);modemStatus=1;}else{modemStatus=0;break;}}
    case 1:{if(_sendAT("ATE0\r\n",2000)==1){modemStatus=2;}else{modemStatus=1;break;}}
    case 2:{
      if(sendATreply("AT+CREG?\r\n","0,1",5000)==1){
        delay(1000);
        _sendAT("AT+CIPMUX=0\r\n",2000);
        delay(1000);
        _sendAT("AT+CIPMODE=1\r\n",2000);
        if (sendATreply("AT+CGATT?\r\n", ": 1", 4000)!= 1){_sendAT("AT+CGATT=1\r\n", 2000);}
        modemStatus = 3;_tcpStatus = 2;
      }else{modemStatus = 2;break;}
    }
    case 3:{
      if (GSM_ReplyFlag != 7){_tcpStatus = sendATreply("AT+CIPSTATUS\r\n", "STATE", 4000);
        if(_tcpStatusPrev==_tcpStatus){tcpATerrorcount++;if (tcpATerrorcount >= 10){tcpATerrorcount = 0;_tcpStatus = 7;}}
        else{_tcpStatusPrev = _tcpStatus;tcpATerrorcount = 0;}
      }
      _tcpStatusPrev = _tcpStatus;
      Monitor(String(_tcpStatus));
      switch (_tcpStatus){
        case 2:{String str="AT+CSTT=\""+APN+"\"\r\n";char temp[50];str.toCharArray(temp,50);_sendAT(temp, 5000);break;}
        case 3:{_sendAT("AT+CIICR\r\n", 5000);break;}
        case 4:{sendATreply("AT+CIFSR\r\n", ".", 4000);break;}
        case 5:{SIMSerial.print("AT+CIPSTART=\"TCP\",\"");SIMSerial.print(mqttbroker);SIMSerial.print("\",\"");SIMSerial.print(String(mqttport));
          if(_sendAT("\"\r\n", 5000)==1){unsigned long PrevMillis = millis();unsigned long currentMillis = millis();
            while((GSM_Response!=4)&&((currentMillis-PrevMillis)<20000)){serialEvent();currentMillis = millis();}}break;}
        case 6:{unsigned long PrevMillis = millis();unsigned long currentMillis = millis();
          while((GSM_Response!=4)&&((currentMillis-PrevMillis)<20000)){serialEvent();currentMillis=millis();}break;}
        case 7:{sendATreply("AT+CIPSHUT\r\n", "OK", 4000);modemStatus = 0;_tcpStatus = 2;break;}
      }
    }
  }
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////_PING
void GSM_MQTT::_ping(void){if(pingFlag==true){unsigned long currentMillis=millis();if((currentMillis-_PingPrevMillis)>=_KeepAliveTimeOut*1000){
      _PingPrevMillis=currentMillis;SIMSerial.print(char(PINGREQ*16));_sendLength(0);}}}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////SEND
void GSM_MQTT::_sendUTFString(char *string){
  int localLength=strlen(string);SIMSerial.print(char(localLength/256));SIMSerial.print(char(localLength%256));SIMSerial.print(string);}
void GSM_MQTT::_sendLength(int len){bool length_flag=false;while(length_flag==false){
    if((len/128)>0){SIMSerial.print(char(len%128+128));len/=128;}else{length_flag=true;SIMSerial.print(char(len));}}}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////CONNECT
void GSM_MQTT::connect(char *ClientIdentifier,char UserNameFlag,char PasswordFlag,char *UserName,char *Password,char CleanSession,char WillFlag,char WillQoS,char WillRetain,char *WillTopic,char *WillMessage){
  ConnectionAcknowledgement = NO_ACKNOWLEDGEMENT;
  SIMSerial.print(char(CONNECT*16));
  char ProtocolName[7]="MQIsdp";
  int localLength=(2+strlen(ProtocolName))+1+3+(2+strlen(ClientIdentifier));
  if(WillFlag!=0){localLength=localLength+2+strlen(WillTopic)+2+strlen(WillMessage);}
  if(UserNameFlag!=0){localLength=localLength+2+strlen(UserName);if(PasswordFlag!=0){localLength=localLength+2+strlen(Password);}}
  _sendLength(localLength);
  _sendUTFString(ProtocolName);
  SIMSerial.print(char(_ProtocolVersion));
  SIMSerial.print(char(UserNameFlag*User_Name_Flag_Mask+PasswordFlag*Password_Flag_Mask+WillRetain*Will_Retain_Mask+WillQoS*Will_QoS_Scale+WillFlag*Will_Flag_Mask+CleanSession*Clean_Session_Mask));
  SIMSerial.print(char(_KeepAliveTimeOut/256));
  SIMSerial.print(char(_KeepAliveTimeOut%256));
  _sendUTFString(ClientIdentifier);
  if (WillFlag!=0){_sendUTFString(WillTopic);_sendUTFString(WillMessage);}
  if (UserNameFlag!=0){_sendUTFString(UserName);if(PasswordFlag!= 0){_sendUTFString(Password);}}
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////PUBLISH
void GSM_MQTT::publish(char DUP,char Qos,char RETAIN,unsigned int MessageID,char *Topic,char *Message){
  SIMSerial.print(char(PUBLISH*16+DUP*DUP_Mask+Qos*QoS_Scale+RETAIN));
  int localLength=(2+strlen(Topic));
  if(Qos>0){localLength+=2;}
  localLength+=strlen(Message);_sendLength(localLength);_sendUTFString(Topic);
  if(Qos>0){SIMSerial.print(char(MessageID/256));SIMSerial.print(char(MessageID%256));}
  SIMSerial.print(Message);
  Monitor("Publish: "+String(Topic)+"/"+String(Message));
}
void GSM_MQTT::publishACK(unsigned int MessageID){SIMSerial.print(char(PUBACK*16));
  _sendLength(2);SIMSerial.print(char(MessageID/256));SIMSerial.print(char(MessageID%256));}
void GSM_MQTT::publishREC(unsigned int MessageID){SIMSerial.print(char(PUBREC*16));
  _sendLength(2);SIMSerial.print(char(MessageID/256));SIMSerial.print(char(MessageID%256));}
void GSM_MQTT::publishREL(char DUP, unsigned int MessageID){SIMSerial.print(char(PUBREL*16+DUP*DUP_Mask+1*QoS_Scale));
  _sendLength(2);SIMSerial.print(char(MessageID/256));SIMSerial.print(char(MessageID%256));}
void GSM_MQTT::publishCOMP(unsigned int MessageID){SIMSerial.print(char(PUBCOMP*16));
  _sendLength(2);SIMSerial.print(char(MessageID/256));SIMSerial.print(char(MessageID%256));}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////SUBSCRIBE
void GSM_MQTT::subscribe(char DUP,unsigned int MessageID,char *SubTopic,char SubQoS){
  SIMSerial.print(char(SUBSCRIBE * 16 + DUP * DUP_Mask + 1 * QoS_Scale));int localLength = 2 + (2 + strlen(SubTopic)) + 1;
  _sendLength(localLength);SIMSerial.print(char(MessageID / 256));SIMSerial.print(char(MessageID % 256));_sendUTFString(SubTopic);
  SIMSerial.print(SubQoS);/*Serial.println("Subscribe: "+String(SubTopic));*/}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////UNSUBSCRIBE
void GSM_MQTT::unsubscribe(char DUP, unsigned int MessageID,char *SubTopic){
  SIMSerial.print(char(UNSUBSCRIBE*16+DUP*DUP_Mask+1*QoS_Scale));int localLength = (2 + strlen(SubTopic)) + 2;
  _sendLength(localLength);SIMSerial.print(char(MessageID / 256));SIMSerial.print(char(MessageID % 256));_sendUTFString(SubTopic);}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////DISCONNECT
void GSM_MQTT::disconnect(void){SIMSerial.print(char(DISCONNECT*16));_sendLength(0);pingFlag=false;/*Serial.println("MQTT.pingFlag=false (ln119)");*/}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////PRINTMESSAGETYPE
void GSM_MQTT::printMessageType(uint8_t Message){
  switch (Message){
    case CONNECT:{int k,len=strlen_P(CONNECTMessage);char myChar;
      for(k=0;k<len;k++){myChar=pgm_read_byte_near(CONNECTMessage+k);/*Serial.print(myChar);*/}break;}
    case CONNACK:{int k,len=strlen_P(CONNACKMessage);char myChar;
      for(k=0;k<len;k++){myChar=pgm_read_byte_near(CONNACKMessage+k);/*Serial.print(myChar);*/}break;}
    case PUBLISH:{int k,len=strlen_P(PUBLISHMessage);char myChar;
      for(k=0;k<len;k++){myChar=pgm_read_byte_near(PUBLISHMessage+k);/*Serial.print(myChar);*/}break;}
    case PUBACK:{int k,len=strlen_P(PUBACKMessage);char myChar;
      for(k=0;k<len;k++){myChar=pgm_read_byte_near(PUBACKMessage+k);/*Serial.print(myChar);*/}break;}
    case  PUBREC:{int k,len=strlen_P(PUBRECMessage);char myChar;
      for(k=0;k<len;k++){myChar=pgm_read_byte_near(PUBRECMessage+k);/*Serial.print(myChar);*/}break;}
    case PUBREL:{int k,len=strlen_P(PUBRELMessage);char myChar;
      for(k=0;k<len;k++){myChar=pgm_read_byte_near(PUBRELMessage+k);/*Serial.print(myChar);*/}break;}
    case PUBCOMP:{int k,len=strlen_P(PUBCOMPMessage);char myChar;
      for(k=0;k<len;k++){myChar=pgm_read_byte_near(PUBCOMPMessage+k);/*Serial.print(myChar);*/}break;}
    case SUBSCRIBE:{int k,len=strlen_P(SUBSCRIBEMessage);char myChar;
      for(k=0;k<len;k++){myChar=pgm_read_byte_near(SUBSCRIBEMessage+k);/*Serial.print(myChar);*/}break;}
    case SUBACK:{int k,len=strlen_P(SUBACKMessage);char myChar;
      for(k=0;k<len;k++){myChar=pgm_read_byte_near(SUBACKMessage+k);/*Serial.print(myChar);*/}break;}
    case UNSUBSCRIBE:{int k,len=strlen_P(UNSUBSCRIBEMessage);char myChar;
      for(k=0;k<len;k++){myChar=pgm_read_byte_near(UNSUBSCRIBEMessage+k);/*Serial.print(myChar);*/}break;}
    case UNSUBACK:{int k,len=strlen_P(UNSUBACKMessage );char myChar;
      for(k=0;k<len;k++){myChar=pgm_read_byte_near(UNSUBACKMessage+k);/*Serial.print(myChar);*/}break;}
    case PINGREQ:{int k,len=strlen_P(PINGREQMessage);char myChar;
      for(k=0;k<len;k++){myChar=pgm_read_byte_near(PINGREQMessage+k);/*Serial.print(myChar);*/}break;}
    case PINGRESP:{int k,len=strlen_P(PINGRESPMessage);char myChar;
      for(k=0;k<len;k++){myChar=pgm_read_byte_near(PINGRESPMessage+k);/*Serial.print(myChar);*/}break;}
    case DISCONNECT:{int k,len=strlen_P(DISCONNECTMessage);char myChar;
      for(k=0;k<len;k++){myChar=pgm_read_byte_near(DISCONNECTMessage+k);/*Serial.print(myChar);*/}break;}
  }
}
void GSM_MQTT::printConnectAck(uint8_t Ack){
  switch (Ack){
    case 0:{int k,len=strlen_P(ConnectAck0);char myChar;for(k=0;k<len;k++){myChar=pgm_read_byte_near(ConnectAck0+k);/*Serial.print(myChar);*/}break;}
    case 1:{int k,len=strlen_P(ConnectAck1);char myChar;for(k=0;k<len;k++){myChar=pgm_read_byte_near(ConnectAck1+k);/*Serial.print(myChar);*/}break;}
    case 2:{int k,len=strlen_P(ConnectAck2);char myChar;for(k=0;k<len;k++){myChar=pgm_read_byte_near(ConnectAck2+k);/*Serial.print(myChar);*/}break;}
    case 3:{int k,len=strlen_P(ConnectAck3);char myChar;for(k=0;k<len;k++){myChar=pgm_read_byte_near(ConnectAck3+k);/*Serial.print(myChar);*/}break;}
    case 4:{int k,len=strlen_P(ConnectAck4);char myChar;for(k=0;k<len;k++){myChar=pgm_read_byte_near(ConnectAck4+k);/*Serial.print(myChar);*/}break;}
    case 5:{int k,len=strlen_P(ConnectAck5);char myChar;for(k=0;k<len;k++){myChar=pgm_read_byte_near(ConnectAck5+k);/*Serial.print(myChar);*/}break;}
  }
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////_GENERATEMESSAGEID
unsigned int GSM_MQTT::_generateMessageID(void){if(_LastMessaseID<65535){return ++_LastMessaseID;}else{_LastMessaseID=0;return _LastMessaseID;}}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////PROCESSING
void GSM_MQTT::processing(void){
  if(TCP_Flag==true){serialEvent();}//new code
  else if(TCP_Flag==false){MQTT_Flag=false;_tcpInit();/*Serial.println("MQTT_Flag=false (ln 192)");*/}
  _ping();
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////AVAILABLE
bool GSM_MQTT::available(void){return MQTT_Flag;}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////RECSMS
char inchar4;char inchar3;char inchar2;char inchar1;int times=0;
extern char RESULTADO[500];
void RecSMS(char data){
  if(times==3){inchar4=data;times++;}
  if(times==2){inchar3=data;times++;}
  if(times==1){inchar2=data;times++;}
  if(times==0){inchar1=data;times++;}
  if(times>=4){
    inchar1=inchar2;inchar2=inchar3;inchar3=inchar4;inchar4=data;
    if((inchar1=='+')&&(inchar2=='C')&&(inchar3=='M')&&(inchar4=='T')){RcvdConf=1;}
    if(RcvdConf==1){
      MQTT.OnSmsArrived(String(data)+String(SIMSerial.readStringUntil('\n')));
    //   if(data == '\n'){RcvdEnd++;}
    //   if(RcvdEnd == 3){RcvdEnd = 0;}
    //   RcvdMsg[count] = data;
    //   count++;
    //   if(RcvdEnd == 2){RcvdConf = 0;MsgLength = count-2;count= 0;}
    //   if(RcvdConf == 0){
    //     Monitor("Mobile Number is: ");
    //     for(int x = 4;x < 18;x++){MsgMob[x-4] = RcvdMsg[x];Monitor(String(MsgMob[x-4]));}
    //     Monitor("Message Text: ");
    //     for(int x = 47; x < MsgLength; x++){MsgTxt[x-47] = RcvdMsg[x];Monitor(String(MsgTxt[x-47]));}
    //     char RcvdMsg[200] = "";int RcvdConf = 0;int count = 0;int RcvdEnd = 0;char MsgMob[15];char MsgTxt[50];int MsgLength = 0;
    //   }
    }
  }
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////ATENDER
void GSM_MQTT::receivecall(){
  HangUp=1;
  if(MQTT.TCP_Flag==true){disconnect();}
  delay(1000);
  SIMSerial.write("AT+CGATT=0\n");
  delay(1000);
  //SIMSerial.write("AT+CIPSHUT\n");
  modemStatus=0;
  SIMSerial.write("ATA\n");SIMSerial.write(26);Monitor("Llamada Entrante");
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////COLGAR
void GSM_MQTT::endcall(){if(HangUp==1){HangUp=0;SIMSerial.write("ATH\n");SIMSerial.write(26);Monitor("Llamada Finalizada");}}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////SERIALEVENT
void serialEvent(){
    while(SIMSerial.available()){
      char inChar=(char)SIMSerial.read();
      RecSMS(inChar);//<////////////////////////sms reading
      if(MQTT.TCP_Flag==false){
        if(MQTT.index<200){MQTT.inputString[MQTT.index++]=inChar;}
        if(inChar=='\n'){
          MQTT.inputString[MQTT.index]=0;stringComplete=true;Monitor(MQTT.inputString);
          if(strstr(MQTT.inputString,MQTT.reply)!=NULL){
            MQTT.GSM_ReplyFlag=1;
            if(strstr(MQTT.inputString," INITIAL")!=0){MQTT.GSM_ReplyFlag=2;}
            else if(strstr(MQTT.inputString," START")!=0){MQTT.GSM_ReplyFlag=3;}
            else if(strstr(MQTT.inputString," IP CONFIG")!=0){delay(10);MQTT.GSM_ReplyFlag=4;}
            else if(strstr(MQTT.inputString," GPRSACT")!=0){MQTT.GSM_ReplyFlag=4;}
            else if((strstr(MQTT.inputString," STATUS")!=0)||(strstr(MQTT.inputString,"TCP CLOSED")!=0)){MQTT.GSM_ReplyFlag=5;}
            else if(strstr(MQTT.inputString," TCP CONNECTING")!=0){MQTT.GSM_ReplyFlag=6;}
            else if((strstr(MQTT.inputString," CONNECT OK")!=0)||(strstr(MQTT.inputString,"CONNECT FAIL")!=NULL)||(strstr(MQTT.inputString,"PDP DEACT")!=0)){MQTT.GSM_ReplyFlag=7;}}
          else if(strstr(MQTT.inputString,"OK")!=NULL){GSM_Response=1;}
          else if(strstr(MQTT.inputString,"ERROR")!=NULL){GSM_Response=2;}
          else if(strstr(MQTT.inputString,".")!= NULL){GSM_Response=3;}
          else if(strstr(MQTT.inputString,"CONNECT FAIL")!=NULL){GSM_Response=5;}
          else if(strstr(MQTT.inputString,"CONNECT")!=NULL){GSM_Response=4;MQTT.TCP_Flag=true;MQTT.AutoConnect();MQTT.pingFlag=true;MQTT.tcpATerrorcount=0;Monitor("MQTT.TCP_Flag=True");}
          else if(strstr(MQTT.inputString,"CLOSED")!=NULL){GSM_Response=4;MQTT.TCP_Flag=false;MQTT.MQTT_Flag=false;Monitor("TCP_Flag=False");}
          MQTT.index=0;MQTT.inputString[0]=0;
        }
      }else{
        uint8_t ReceivedMessageType=(inChar/16) & 0x0F;uint8_t DUP=(inChar & DUP_Mask)/DUP_Mask;
        uint8_t QoS=(inChar & QoS_Mask)/QoS_Scale;uint8_t RETAIN=(inChar & RETAIN_Mask);
        if((ReceivedMessageType>=CONNECT)&&(ReceivedMessageType<=DISCONNECT)){
          bool NextLengthByte=true;MQTT.length=0;MQTT.lengthLocal=0;uint32_t multiplier=1;delay(2);char Cchar=inChar;
          while((NextLengthByte==true)&&(MQTT.TCP_Flag==true)){
            if(SIMSerial.available()){
              inChar=(char)SIMSerial.read();/*Serial.println(inChar, DEC);*/
              if(((((Cchar & 0xFF)=='C')&&((inChar & 0xFF)=='L'))||(((Cchar & 0xFF)=='+')&&((inChar & 0xFF)=='P')))&&(MQTT.length==0)){
                MQTT.index=0;MQTT.inputString[MQTT.index++]=Cchar;MQTT.inputString[MQTT.index++]=inChar;
                MQTT.TCP_Flag=false;MQTT.MQTT_Flag=false;MQTT.pingFlag=false;Monitor("Disconnecting");
                Monitor("MQTT.TCP_Flag=False (ln223)");
              }else{
                if((inChar&128)==128){MQTT.length+=(inChar&127)*multiplier;multiplier*=128;Monitor("More");}
                else{NextLengthByte=false;MQTT.length+=(inChar&127)*multiplier;multiplier*=128;}
              }
            }
          }
          MQTT.lengthLocal=MQTT.length;/*Serial.println(MQTT.length);*/ 
          if(MQTT.TCP_Flag==true){
            MQTT.printMessageType(ReceivedMessageType);MQTT.index=0L;uint32_t a=0;
            while((MQTT.length-- > 0)&&(SIMSerial.available())){MQTT.inputString[uint32_t(MQTT.index++)]=(char)SIMSerial.read();delay(1);}
            /*Serial.println(" ");*/  
            if (ReceivedMessageType==CONNACK){
              MQTT.ConnectionAcknowledgement=MQTT.inputString[0]*256+MQTT.inputString[1];
              if(MQTT.ConnectionAcknowledgement==0){MQTT.MQTT_Flag=true;MQTT.OnConnect();}
              MQTT.printConnectAck(MQTT.ConnectionAcknowledgement); /*MQTT.OnConnect();*/
            }else if(ReceivedMessageType==PUBLISH){
              uint32_t TopicLength=(MQTT.inputString[0])*256+(MQTT.inputString[1]);Monitor("Topic: '");MQTT.PublishIndex = 0;
              for(uint32_t iter=2;iter<TopicLength+2;iter++){Monitor(String(MQTT.inputString[iter]));MQTT.Topic[MQTT.PublishIndex++]=MQTT.inputString[iter];}
              MQTT.Topic[MQTT.PublishIndex]=0;Monitor("' Message: '");
              MQTT.TopicLength=MQTT.PublishIndex;MQTT.PublishIndex=0;uint32_t MessageSTART=TopicLength+2UL;int MessageID=0;
              if(QoS!=0){MessageSTART+=2;MessageID=MQTT.inputString[TopicLength+2UL]*256+MQTT.inputString[TopicLength+3UL];}
              for(uint32_t iter=(MessageSTART);iter<(MQTT.lengthLocal);iter++){Monitor(String(MQTT.inputString[iter]));MQTT.Message[MQTT.PublishIndex++]=MQTT.inputString[iter];}
              MQTT.Message[MQTT.PublishIndex]=0;Monitor("'");MQTT.MessageLength=MQTT.PublishIndex;
              if(QoS==1){MQTT.publishACK(MessageID);}else if(QoS==2){MQTT.publishREC(MessageID);}
              MQTT.OnMessage(MQTT.Topic,MQTT.TopicLength,MQTT.Message,MQTT.MessageLength);MQTT.MessageFlag=true;
            }else if(ReceivedMessageType==PUBREC){MQTT.publishREL(0,MQTT.inputString[0]*256+MQTT.inputString[1]);
              int MessageID=MQTT.inputString[0]*256+MQTT.inputString[1];
            }else if(ReceivedMessageType==PUBREL){MQTT.publishCOMP(MQTT.inputString[0]*256+MQTT.inputString[1]);
              int MessageID=MQTT.inputString[0]*256+MQTT.inputString[1];
            }else if((ReceivedMessageType==PUBACK)||(ReceivedMessageType==PUBCOMP)||(ReceivedMessageType==SUBACK)||(ReceivedMessageType==UNSUBACK)){
              int MessageID=MQTT.inputString[0]*256+MQTT.inputString[1];
            }else if(ReceivedMessageType==PINGREQ){
              MQTT.TCP_Flag=false;MQTT.pingFlag=false;MQTT.sendATreply("AT+CIPSHUT\r\n",".",4000);MQTT.modemStatus=0;
              Monitor("Disconnecting");Monitor("MQTT.TCP_Flag=False (ln261)");
            }
          }
        }else if((inChar==13)||(inChar==10)){Monitor("inChar=13||10");}else{Monitor("Received: Unknown Message Type: ");Monitor(String(inChar));}
      }
    }
  //}
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////NEXT
void GSM_MQTT::next(void){serialEvent();}
void GSM_MQTT::SmsSend(String sms){
    String msg ="La temperatura actual es de " + String(sms) + " C.";
    String TEL="+542615131175";
    SIMSerial.write(("AT+CMGS=\""+TEL+"\"\n").c_str());
    delay(500);
    SIMSerial.write(msg.c_str());
    delay(500);
    SIMSerial.write(26);
    Monitor("SmsSend");
}