/*
  Solar Panel Sun Tracker
  By: Joshua Kantarges
  Rev: 1.4

  Description: An Arduino MKR WiFi 1010 based Sun Tracker Using NPT Time Servers and the Built in RTC.
                 Additional Control of the Solar Panel will be avaliable through
                  The Devices Web Page Located at its IP Address. An Oled Sceen will 
                    shows relavent data at a quick glance on the device itself.

                   Examples of html Buttons https://forum.arduino.cc/index.php?topic=165982.0

*/
//Firmware Revision
String rev = "1.4";

#include <RTCZero.h>
#include <SPI.h>
#include <WiFiNINA.h>
#include <Wire.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h>

#include "arduino_secrets.h"
///////Enter your sensitive data in the Secret tab/arduino_secrets.h

//======================= SSD 1306 Oled Setup =====================================
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
#define OLED_RESET     4 // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
//=================================================================================

//====================== WiFi Network Information =================================
char ssid[] = SECRET_SSID;        // your network SSID (name)
char pass[] = SECRET_PASS;        // your network password (use for WPA, or use as key for WEP)
int keyIndex = 0;                 // your network key Index number (needed only for WEP)

int status = WL_IDLE_STATUS;
WiFiServer server(80);
//=================================================================================

//===================== RTCZero Client Setup ==================================
RTCZero rtc;
const int UTC = -7; //change this to adapt it to your time zone
//=================================================================================

void setup() {
  Serial.begin(9600);      // initialize serial communication

  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3c)) { // Address 0x3c for 128x64
    Serial.println(F("SSD1306 allocation failed"));
    for (;;); // Don't proceed, loop forever
  }

  //Display Boot-UP Informaation On OLED
  bootScreen(rev);

  // check for the WiFi module:
  if (WiFi.status() == WL_NO_MODULE) {
    Serial.println("Communication with WiFi module failed!");
    // don't continue
    while (true);
  }

  //Connect to wifi
  connectWiFi();

  // you're connected now, so print out the status
  printWifiStatus();

  //Begin RTCZero Library
  rtc.begin();
  rtc.setEpoch(WiFi.getTime()); // set the time for MKRWiFi1010 RTC

  // Setup Relay Digital Pins
  pinMode(0, OUTPUT);      // Right Relay Pin
  pinMode(1, OUTPUT);      // Left Relay Pin
}

void loop() {
  //Connect to wifi if disconnected
  connectWiFi();

  //==================== Update Time on Oled =================================================
  wifiStatusInformation();
  //==========================================================================================

  //==================== Move panel ==========================================================
  panelMove(rtc.getHours() + UTC, rtc.getMinutes(), rtc.getMonth());
  //==========================================================================================

  //==================== Web Page Code =======================================================
  WiFiClient client = server.available();   // listen for incoming clients
  if (client) {                             // if you get a client,
    Serial.println("new client");           // print a message out the serial port
    String currentLine = "";                // make a String to hold incoming data from the client
    while (client.connected()) {            // loop while the client's connected
      if (client.available()) {             // if there's bytes to read from the client,
        char c = client.read();             // read a byte, then
        Serial.write(c);                    // print it out the serial monitor
        if (c == '\n') {                    // if the byte is a newline character

          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request, so send a response:
          if (currentLine.length() == 0) {
            // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
            // and a content-type so the client knows what's coming, then a blank line:
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println();

            // the content of the HTTP response follows the header:
            client.print("<h1>");
            char formattedTime[25];
            sprintf(formattedTime, "Time: %d:%d:%d", (rtc.getHours() + UTC), rtc.getMinutes(), rtc.getSeconds());
            client.print(formattedTime);
            client.println("  UTC -7 </h1>");
            client.print("Click <a href=\"/E\">here</a> to move Panel Eastward<br>");
            client.print("Click <a href=\"/W\">here</a> to move Panel Westward<br>");
            client.print("Click <a href=\"/S\">here</a> to Stop Panel movement<br>");

            // The HTTP response ends with another blank line:
            client.println();
            // break out of the while loop:
            break;
          } else {    // if you got a newline, then clear currentLine:
            currentLine = "";
          }
        } else if (c != '\r') {  // if you got anything else but a carriage return character,
          currentLine += c;      // add it to the end of the currentLine
        }

        // Check to see if the client request was East,West or Stop:
        if (currentLine.endsWith("GET /E")) { // GET /E Moves Panel East
          moveEast();
        }
        if (currentLine.endsWith("GET /W")) { // GET /W Moves Panel West
          moveWest();
        }
        if (currentLine.endsWith("GET /S")) { // GET /S Stops Panel Movement
          moveStop();
        }
      }
    }
    // close the connection:
    client.stop();
    Serial.println("client disonnected");
  }
  //===================================== END Web Page Code ========================================

}

void connectWiFi() {

  // attempt to connect to Wifi network:
  while (status != WL_CONNECTED) {

    //Display WiFi Connection Status
    wifiConnecting(String(ssid));
    Serial.print("Attempting to connect to Network named: ");
    Serial.println(ssid);                   // print the network name (SSID);

    // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
    status = WiFi.begin(ssid, pass);
    // wait 10 seconds for connection:
    delay(10000);

    //Display Connected WiFi Information
    display.clearDisplay();
    display.setCursor(30, 10);
    display.print(F("Connected!"));
    display.display();
    delay(1000);
    wifiStatusInformation();
    server.begin();                          // start the web server on port 80
  }
}

void panelMove(int hour, int minute, int month) {
  //Summer Months (march to august)
  if (month >= 3 && month <= 8) {
    //Sunrise ~5:00am
    //Sunset ~8:00pm
    //~15hrs total sunlight
    if (hour >= 5 && hour <= 19 && minute == 0 ) {
      moveStep(1300);
    } else if (hour == 20 && minute == 0) {
      panelReset();
    }
  }
  // Winter Months (september to february )
  else if (month >= 9 || month <= 2) {
    //Sunrise ~8:00am
    //Sunset ~5:00pm
    //~9hrs total sunlight
    if (hour >= 8 && hour <= 16 && minute == 0 ) {
      moveStep(2000);
    } else if (hour == 17 && minute == 0) {
      panelReset();
    }
  }
}

void panelReset() {
  moveEast(); // Move panel all the way east ~25 Seconds +2 sec added for variation in motor speed
  delay(27000);
  moveStop();
  delay(60000); //Delay 1 minute to allow for time to change and prevent repeated movement
}

void moveEast() { //Triggers Eastward Movement of the Panel
  digitalWrite(1, HIGH);
  delay(100);
  digitalWrite(0, LOW);
}

void moveWest() { //Triggers Westward Movement of the Panel
  digitalWrite(1, LOW);
  delay(100);
  digitalWrite(0, HIGH);
}
void moveStep(int ms) {
  moveWest(); // Move panel to mid-point between morning and afternoon
  delay(ms);
  moveStop();
  delay(60000); //Delay 1 minute to allow for time to change and prevent repeated movement
}

void moveStop() { // Stops all Panel Movment
  digitalWrite(0, LOW);
  digitalWrite(1, LOW);
}

void bootScreen(String rev) {
  display.clearDisplay();
  display.setTextSize(1);             // Normal 1:1 pixel scale
  display.setTextColor(SSD1306_WHITE);        // Draw white text
  display.setCursor(20, 20);
  display.println(F("Solar Panel Sun"));
  display.setCursor(45, 30);
  display.println(F("Tracker"));
  display.setCursor(30, 55);
  display.print(F("FW Rev: "));
  display.println(rev);
  display.display();
  delay(2000);
}

void wifiConnecting(String ssid) {
  display.clearDisplay();
  display.setCursor(20, 20);
  display.println(F("Attempting to"));
  display.setCursor(30, 30);
  display.println(F("Connect..."));
  display.setCursor(20, 45);
  display.print("SSID: ");
  display.println(ssid);
  display.display();
}

void wifiStatusInformation() {
  display.clearDisplay();
  display.setCursor(20, 2);
  char formattedTime[25];
  char formattedDate[25];
  sprintf(formattedTime, "Time: %d:%d:%d", (rtc.getHours() + UTC), rtc.getMinutes(), rtc.getSeconds());
  sprintf(formattedDate, "Date: %d/%d/%d", rtc.getDay(), rtc.getMonth(), rtc.getYear());
  display.print(formattedDate);
  display.setCursor(20, 12);
  display.print(formattedTime);

  //WiFi Network Name
  display.setCursor(20, 25);
  display.print("SSID: ");
  display.println(WiFi.SSID());

  //Signal Strength
  display.setCursor(20, 35);
  display.print("RSSI: ");
  display.print(WiFi.RSSI());
  display.println(" dB");

  //IP Address
  display.setCursor(20, 45);
  display.print("IP: ");
  display.print(WiFi.localIP());

  display.setCursor(105, 2);
  //Summer Months (march to august)
  if (rtc.getMonth() >= 3 && rtc.getMonth() <= 8) {
    display.println("SUM");
    // Winter Months (september to february )
  } else if (rtc.getMonth() >= 9 || rtc.getMonth() <= 2) {
    display.println("WINT");
  }

  display.display();
}

void printWifiStatus() {
  // print the SSID of the network you're attached to:
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print your board's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  // print the received signal strength:
  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");
  // print where to go in a browser:
  Serial.print("To see this page in action, open a browser to http://");
  Serial.println(ip);
}
