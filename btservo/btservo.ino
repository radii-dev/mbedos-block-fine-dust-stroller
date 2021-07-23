#include <SoftwareSerial.h>
#include <Servo.h> 
#include <stdlib.h>

SoftwareSerial bluetooth(2, 3); // Rx|Tx
int servoPin = 9; //서보 제어 핀
int sw = 4; //모터 원점 스위치 제어
Servo servo; 
float servosw; //서보 제어 변수 선언
char buffer[10];
int bufferPosition;
volatile byte mode = HIGH; //모드지정 H: auto L: manual
 
void setup()
{
  Serial.begin(9600);
  bluetooth.begin(9600);
  pinMode(sw, INPUT_PULLUP);
  servo.attach(servoPin);
  bufferPosition = 0;
  attachInterrupt(digitalPinToInterrupt(sw), chmod, CHANGE);
  servo.write(0);
}
 
void loop()
{
  while(1){
    if(bluetooth.available()){
      byte data = bluetooth.read();
      Serial.write(data); //시리얼 모니터
      buffer[bufferPosition++] = data;
      if(data == '\n'){
        servosw = atof(buffer);
        buffer[bufferPosition] = '\0';
        bufferPosition = 0;
        break;
      }
    }
  }
    if(mode == HIGH){
      if(servosw >= 30.3)  servo.write(180);
      else if(servosw >= 30 && servosw < 30.3)  servo.write(90);
      else if(servosw < 30)  servo.write(0);
    }
//    servosw = 0;
}

void chmod() {
  servo.write(0);
  mode = !mode;
}
