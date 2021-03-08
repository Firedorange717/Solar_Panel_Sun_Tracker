/*
  Solar Panel Sun Tracker
  By: Joshua Kantarges
  Rev: 1.2

  Description: An Arduino MKR WiFi 1010 based Sun Tracker Using NPT Time Servers.
                 Additional Control of the Solar Panel will be avaliable through
                  The Devices Web Page Located at its IP Address. An Oled Sceen will
                    also show relavent data at a quick glance on the device itself.

                   Examples of html Buttons https://forum.arduino.cc/index.php?topic=165982.0
                   Arduino WiFi Library Documentation https://www.arduino.cc/en/Reference/WiFi

*/
//Firmware Revision
String rev = "1.2";

#include <NTPClient.h>
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

//===================== NPT Client Setup ==========================================
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, -25200); //offset 25200 seconds(7 hours) for Arizona Time
int seasonOffset = 0;
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

  // Setup Relay Digital Pins
  pinMode(0, OUTPUT);      // Right Relay Pin
  pinMode(1, OUTPUT);      // Left Relay Pin
}

void loop() {
  //Check Wifi Connection Status
  //if (WiFi.????) {
  // connectWiFi(); //Reconnect to wifi run WiFi method.
  //}

  //==================== Update Time and Oled ================================================
  wifiStatusInformation();
  //==========================================================================================

  //==================== Panel Movement Check ================================================
  panelMotorControl(((timeClient.getHours() * 100) + seasonOffset) + timeClient.getMinutes());
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
            client.print(timeClient.getFormattedTime());
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
  //=====================================END Web Page Code ========================================

}

void panelMotorControl(int currTime) {
  //Every hour from 5am - 5pm moves the panel for 2.84 seconds (25s/12hr)=2.84 sec
  switch (currTime) {
    case 500: //5:00 am Sunrise
      moveStep();
      break;
    case 600: //6:00 am
      moveStep();
      break;
    case 700: //7:00 am
      moveStep();
      break;
    case 800: //8:00 am
      moveStep();
      break;
    case 900: //9:00 am
      moveStep();
      break;
    case 1000: //10:00 am
      moveStep();
      break;
    case 1100: //11:00 am
      moveStep();
      break;
    case 1200: // 12:00 pm Afternoon
      moveStep();
      break;
    case 1300 : //1:00 pm
      moveStep();
      break;
    case 1400 : //2:00 pm
      moveStep();
      break;
    case 1500 : //3:00 pm
      moveStep();
      break;
    case 1600 : //4:00 pm
      moveStep();
      break;
    case 1700 : //5:00 pm Sunset
      moveStep();
      break;
    case 2000: //8:00 pm Panel Reset
      panelReset();
      break;

    default:
      // No Movement Needed
      break;
  }
}

void panelReset() {
  moveEast(); // Move panel all the way east ~25 Seconds +2 sec added for variation in motor speed
  delay(27000);
  moveStop();
  delay(60000); //Delay 1 minute to allow for time to change and prevent repeated movement
}

void moveStep() {
  moveWest(); // Move panel to mid-point between morning and afternoon
  delay(2084);
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

void moveStop() { // Stops all Panel Movment
  digitalWrite(0, LOW);
  digitalWrite(1, LOW);
}

void connectWiFi() {

  //Display WiFi Connection Status
  wifiConnecting(String(ssid));

  // attempt to connect to Wifi network:
  while (status != WL_CONNECTED) {
    Serial.print("Attempting to connect to Network named: ");
    Serial.println(ssid);                   // print the network name (SSID);

    // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
    status = WiFi.begin(ssid, pass);
    // wait 10 seconds for connection:
    delay(10000);
  }

  //Display Connected WiFi Information
  display.clearDisplay();
  display.setCursor(30, 10);
  display.print(F("Connected!"));
  display.display();
  delay(1000);
  wifiStatusInformation();
  server.begin();                           // start the web server on port 80
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
  display.setCursor(20, 10);
  timeClient.update();
  display.print(timeClient.getFormattedTime());
  display.print(" UTC -7");

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

  //Connection Status
  display.setCursor(20, 55);
  display.print("STATUS: ");
  display.print(status);

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
