/*
*/

#include <SoftwareSerial.h>
#include <Wire.h>
#include <Time.h>
#include <DS3232RTC.h>    //http://github.com/JChristensen/DS3232RTC
#include <SPI.h>
#include <Ethernet.h>
#include <EthernetUdp.h>

/*-----( Declare Constants and Pin Numbers )-----*/
#define SSerialRX        4  //Serial Receive pin
#define SSerialTX        5  //Serial Transmit pin
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
const unsigned int cmp = baseFreq / primaryTrigFreq;
const unsigned int halfcmp = cmp / 2;
volatile unsigned long ctr = 0;
volatile unsigned long ctr2 = 0;
volatile bool tick;
bool stuffToSend = false;
byte addr = 0x01;

// Enter a MAC address for your controller below.
// Newer Ethernet shields have a MAC address printed on a sticker on the shield
byte mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};
unsigned int localPort = 8888;       // local port to listen for UDP packets
char timeServer[] = "time.nist.gov"; // time.nist.gov NTP server
const int NTP_PACKET_SIZE = 48; // NTP time stamp is in the first 48 bytes
byte packetBuffer[ NTP_PACKET_SIZE]; //buffer to hold incoming/outgoing packets
EthernetUDP Udp; // A UDP instance to let us send and receive packets over UDP
const long ntpInterval = 60000;
unsigned long prevNTPupdate = 0;

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
     SQWAVE_NONE
     SQWAVE_1_HZ
     SQWAVE_1024_HZ
     SQWAVE_4096_HZ
     SQWAVE_8192_HZ */
  RTC.squareWave(SQWAVE_8192_HZ);

  // start Ethernet and UDP
  if (Ethernet.begin(mac) == 0) {
    Serial.println("Failed to configure Ethernet using DHCP");
    // no point in carrying on, so do nothing forevermore:
    for (;;)
      ;
  }
  Udp.begin(localPort);

  pinMode(ledPin, OUTPUT);
  pinMode(trigPin, OUTPUT);
  pinMode(interruptPin, INPUT_PULLUP);
  attachInterrupt(0, trigFunc, RISING);
}//--(end setup )---

void loop() {
  unsigned long currentTime = millis();
  if (currentTime - prevNTPupdate == ntpInterval) {
    prevNTPupdate = currentTime;
    sendNTPpacket(timeServer); // send an NTP packet to a time server

    // wait to see if a reply is available
    while (!Udp.parsePacket()) {}
    {
      // We've received a packet, read the data from it
      Udp.read(packetBuffer, NTP_PACKET_SIZE); // read the packet into the buffer

      // the timestamp starts at byte 40 of the received packet and is four bytes,
      // or two words, long. First, extract the two words:

      unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
      unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);
      // combine the four bytes (two words) into a long integer
      // this is NTP time (seconds since Jan 1 1900):
      unsigned long secsSince1900 = highWord << 16 | lowWord;
      Serial.print("Seconds since Jan 1 1900 = ");
      Serial.println(secsSince1900);

      // now convert NTP time into everyday time:
      Serial.print("Unix time = ");
      // Unix time starts on Jan 1 1970. In seconds, that's 2208988800:
      const unsigned long seventyYears = 2208988800UL;
      // subtract seventy years:
      unsigned long epoch = secsSince1900 - seventyYears;
      // print Unix time:
      Serial.println(epoch);

      // print the hour, minute and second:
      Serial.print("The UTC time is ");       // UTC is the time at Greenwich Meridian (GMT)
      Serial.print((epoch  % 86400L) / 3600); // print the hour (86400 equals secs per day)
      Serial.print(':');
      if (((epoch % 3600) / 60) < 10) {
        // In the first 10 minutes of each hour, we'll want a leading '0'
        Serial.print('0');
      }
      Serial.print((epoch  % 3600) / 60); // print the minute (3600 equals secs per minute)
      Serial.print(':');
      if ((epoch % 60) < 10) {
        // In the first 10 seconds of each minute, we'll want a leading '0'
        Serial.print('0');
      }
      Serial.println(epoch % 60); // print the second
    }
  }
  while (Serial.available()) frameBuilder();
  if (tick and stuffToSend) frameSender();

  while (RS485Serial.available()) {
    byteReceived = RS485Serial.read(); // Read received byte
    if (byteReceived == 0x06) {
      byteReceived = '~';
    } else if (byteReceived == 0x01 || byteReceived == 0x02) {
      byteReceived += '0';
    } else if (byteReceived == 0x11) {
      byteReceived = 'D';
    } else if (byteReceived == 0x15) {
      byteReceived = 'N';
    }
    Serial.write(byteReceived); // Show on Serial Monitor
    //Serial.println();
  }
  Ethernet.maintain();
}//--(end main loop )---

void frameBuilder() {
  byteSend1 = Serial.read();
  switch (byteSend1) {
    case 'A':
      while (!Serial.available()) {}
      addr = (byte)(Serial.read() - '0');
      Serial.print("\n\rNew addr = ");
      Serial.println(addr);
      while (Serial.available()) {
        Serial.read();
      }
      break;
    case 'E':
      byteSend1 = 0x05;
      byteSend2 = 0x00;
      stuffToSend = true;
      break;
    case 'S':
      while (!Serial.available()) {}
      byteSend1 = 0x11;
      byteSend2 = Serial.read();
      stuffToSend = true;
      break;
    default:
      while (!Serial.available()) {}
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
  byteSend1 = 0x00;
  byteSend2 = 0x00;
}

void trigFunc() {
  ctr++;
  if (ctr == halfcmp) {
    PORTB ^= 1 << (ledPin - 8);
    tick = false;
  } else if (ctr == cmp) {
    ctr = 0;
    if (ctr2 < 7) ctr2++;
    else ctr2 = 0;
    PORTB ^= 1 << (ledPin - 8);
    tick = true;
  }
  if (ctr2 == 0) PORTB |= B00000001;
  else PORTB &= B11111110;
}

// send an NTP request to the time server at the given address
void sendNTPpacket(char* address) {
  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12]  = 49;
  packetBuffer[13]  = 0x4E;
  packetBuffer[14]  = 49;
  packetBuffer[15]  = 52;

  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:
  Udp.beginPacket(address, 123); //NTP requests are to port 123
  Udp.write(packetBuffer, NTP_PACKET_SIZE);
  Udp.endPacket();
}

