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
#define ledPin           8
#define trigPin          7
#define interruptPin     2

/*-----( Declare objects )-----*/
SoftwareSerial RS485Serial(SSerialRX, SSerialTX); // RX, TX
byte byteReceived;
byte byteSend;
const unsigned int baseFreq = 8192;
const unsigned int primaryTrigFreq = 14;
const unsigned int cmp = baseFreq/primaryTrigFreq;
const unsigned int halfcmp = cmp/2;
volatile unsigned long ctr = 0;
volatile unsigned long ctr2 = 0;
volatile bool tick;
bool lasttick;
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
  if (ctr2==0) digitalWrite(trigPin, HIGH);
  if (!tick == lasttick) {
    digitalWrite(trigPin, LOW);
    digitalWrite(ledPin, !digitalRead(ledPin));
    lasttick = tick;
    if (tick and stuffToSend) {
      stuffToSend = false;
      digitalWrite(SSerialTxControl, RS485Transmit);  // Enable RS485 Transmit
      RS485Serial.write(addr);   // Send byte to Remote Arduino
      RS485Serial.write(byteSend); // Send byte to Remote Arduino
      digitalWrite(SSerialTxControl, RS485Receive);  // Disable RS485 Transmit
    }
  }
  
  while (Serial.available()) {
    byteSend = Serial.read();
    switch (byteSend) {
      case 'A':
        while (!Serial.available());
        addr = (byte)(Serial.read()-'0');
        break;
      default:
        stuffToSend = true;
        break;
    }
  }
  
  while (RS485Serial.available()) {
    byteReceived = RS485Serial.read();    // Read received byte
    Serial.write(byteReceived);        // Show on Serial Monitor
   }  

}//--(end main loop )---

void trigFunc() {
  ctr++;
  if (ctr == halfcmp) {
    tick = false;
  } else if (ctr == cmp) {
    ctr = 0;
    if (ctr2<7) ctr2++;
    else ctr2 = 0;
    tick = true;
  }
}

