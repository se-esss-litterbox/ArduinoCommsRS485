/* 
*/

/*-----( Import needed libraries )-----*/
#include <SoftwareSerial.h>
/*-----( Declare Constants and Pin Numbers )-----*/
#define SSerialRX        10  //Serial Receive pin
#define SSerialTX        11  //Serial Transmit pin
#define SSerialTxControl 3   //RS485 Direction control
#define RS485Transmit    HIGH
#define RS485Receive     LOW
#define intPin           2
#define statePin         5

/*-----( Declare objects )-----*/
SoftwareSerial RS485Serial(SSerialRX, SSerialTX); // RX, TX

/*-----( Declare Variables )-----*/
byte byteReceived;
int byteSend, reply, frameAddr, reply1, reply2;
unsigned int state = 1;
const byte addr = 0x02;

void setup() {  /****** SETUP: RUNS ONCE ******/
  //pinMode(Pin13LED, OUTPUT);   
  pinMode(SSerialTxControl, OUTPUT);
  pinMode(statePin, OUTPUT);
  digitalWrite(statePin, LOW);
  digitalWrite(SSerialTxControl, RS485Receive);  // Init Transceiver
  // Start the software serial port, to another device
  RS485Serial.begin(9600);   // set the data rate
  attachInterrupt(digitalPinToInterrupt(intPin), trigFunc, CHANGE);
}//--(end setup )---

void loop() {  /****** LOOP: RUNS CONSTANTLY ******/
  if (RS485Serial.available()) {
    frameConsumer();
    if (frameAddr == addr) { // I should reply
      delay(5);
      frameSender();
    }
  }// End If RS485SerialAvailable
}//--(end main loop )---

void frameConsumer() {
  int byte1, byte2;
  frameAddr = RS485Serial.read();
  if (frameAddr == addr) {
    while (!RS485Serial.available()) {}
    byte1 = RS485Serial.read();
    while (!RS485Serial.available()) {}
    byte2 = RS485Serial.read();

    switch (byte1) {
      case 0x05:
        reply1 = 0x06;
        reply2 = state;
        break;
      case 0x11:
        reply1 = 0x11;
        if (byte2=='0'+1 || byte2=='0'+2) {
          state = byte2-'0';
          reply2 = state;
        } else {
          reply2 = 0x15;
        }
        break;
      default:
        reply1 = byte1;
        reply2 = byte2;
        break;
    }
  }
}

void frameSender() {
  digitalWrite(SSerialTxControl, RS485Transmit);  // Enable RS485 Transmit
  RS485Serial.write(reply1);
  RS485Serial.write(reply2);
  digitalWrite(SSerialTxControl, RS485Receive);  // Disable RS485 Transmit
  reply1 = 0x00;
  reply2 = 0x00;
}

void trigFunc() {
  switch (state) {
    case 1:
      digitalWrite(statePin, !digitalRead(statePin));
      break;
    case 2:
      digitalWrite(statePin, LOW);
      break;
    default:
      digitalWrite(statePin, LOW);
      break;
  }
}

