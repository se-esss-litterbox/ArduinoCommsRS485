/* 
*/

#include <SoftwareSerial.h>
#include <Wire.h>
#include <Time.h>
#include <DS3232RTC.h>    //http://github.com/JChristensen/DS3232RTC

/*-----( Declare Constants and Pin Numbers )-----*/
#define SSerialRX        10  //Serial Receive pin
#define SSerialTX        11  //Serial Transmit pin
#define SSerialTxControl 3   //RS485 Direction control pin
#define RS485Transmit    HIGH
#define RS485Receive     LOW
#define ledPin           9
#define trigPin          8
#define interruptPin     2
#define baseFreq         8192
#define primaryTrigFreq  14

/*-----( Declare objects )-----*/
SoftwareSerial RS485Serial(SSerialRX, SSerialTX); // RX, TX
byte byteReceived, byteSend1, byteSend2;
const unsigned int cmp = baseFreq/primaryTrigFreq;
const unsigned int halfcmp = cmp/2;
volatile unsigned long ctr = 0;
volatile unsigned long ctr2 = 0;
volatile bool tick;
bool stuffToSend = false;
byte addr = 0x01;

void setup() {
  Serial.begin(9600);
  Serial.println("\nYourDuino.com SoftwareSerial remote loop example");
  Serial.println("Use Serial Monitor, type in upper window, then press ENTER");
  
  setSyncProvider(RTC.get);   // the function to get the time from the RTC
    
  pinMode(SSerialTxControl, OUTPUT);    
  digitalWrite(SSerialTxControl, RS485Receive);  // Init Transceiver   
  
  // Start the software serial port, to another device
  RS485Serial.begin(9600);   // set the data rate 
  
  /* Possible values for the following function:
  *  SQWAVE_NONE
  *  SQWAVE_1_HZ
  *  SQWAVE_1024_HZ
  *  SQWAVE_4096_HZ
  *  SQWAVE_8192_HZ */
  RTC.squareWave(SQWAVE_8192_HZ);
  
  pinMode(ledPin, OUTPUT);
  pinMode(trigPin, OUTPUT);
  pinMode(interruptPin, INPUT_PULLUP);
  attachInterrupt(0, trigFunc, RISING);
}//--(end setup )---

void loop() {
  while (Serial.available()) frameBuilder();
  if (tick and stuffToSend) frameSender();
  
  while (RS485Serial.available()) {
    byteReceived = RS485Serial.read(); // Read received byte
    if (byteReceived == 0x06) {
      byteReceived = 0x7E;
    }
    Serial.write(byteReceived); // Show on Serial Monitor
   }  
}//--(end main loop )---

void frameBuilder() {
  byteSend1 = Serial.read();
  while (!Serial.available()) {}
  switch (byteSend1) {
    case 'A':
      while (!Serial.available());
      addr = (byte)(Serial.read()-'0');
      Serial.print("\nNew addr = ");
      Serial.println(addr);
      break;
    /*case 'E':
      byteSend = 0x05;
      stuffToSend = true;
      break;
    case 'S':
      byteSend = 0x11;
      stuffToSend = true;
      break;*/
    default:
      byteSend2 = Serial.read();
      stuffToSend = true;
      break;
  }
}

void frameSender() {
  stuffToSend = false;
  digitalWrite(SSerialTxControl, RS485Transmit);  // Enable RS485 Transmit
  RS485Serial.write(addr);   // Send byte to Remote Arduino
  RS485Serial.write(byteSend1); // Send byte to Remote Arduino
  RS485Serial.write(byteSend2); // Send byte to Remote Arduino
  digitalWrite(SSerialTxControl, RS485Receive);  // Disable RS485 Transmit
}

void trigFunc() {
  ctr++;
  if (ctr == halfcmp) {
    PORTB ^= 1 << (ledPin-8);
    tick = false;
  } else if (ctr == cmp) {
    ctr = 0;
    if (ctr2<7) ctr2++;
    else ctr2 = 0;
    PORTB ^= 1 << (ledPin-8);
    tick = true;
  }
  if (ctr2==0) PORTB |= B00000001;
  else PORTB &= B11111110;
}

