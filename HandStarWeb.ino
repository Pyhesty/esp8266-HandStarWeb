#include "EEPROM.h"
// Load Wi-Fi library
#include <ESP8266WiFi.h>
#include <Adafruit_GFX.h>
// Adafruit_SSD1306 wemos mini oled lib install!!!
#include <Adafruit_SSD1306.h> 

#define OLED_RESET 0  // GPIO0
Adafruit_SSD1306 display(OLED_RESET);

// Replace with your network credentials
//const char* ssid     = "Neva_VN_AP";
//const char* password = "neva_wifi";
//const char* ssid     = "Local.NET";
//const char* password = "23145hhh";
//const char* ssid     = "cnc_connect";
//const char* password = "local_wifi_450";


char newNET[21]; 
char newPASS[21]; 

// Set web server port number to 80
WiFiServer server(80);

// Variable to store the HTTP request
String header;

// Auxiliar variables to store the current output state
String outputLEDState = "off";
String clearPixel     = "yes";
bool clearAll = false;

// Assign output variables to GPIO pins
//const int output5 = 5;
//const int output4 = 4;

#if (SSD1306_LCDHEIGHT != 48)
#error("Height incorrect, please fix Adafruit_SSD1306.h!");
#endif

uint8_t posx = 32;
uint8_t posy = 24;
uint8_t newposx = 32;
uint8_t newposy = 24;
bool NeedNewConnect = false;
uint8_t cntshowDot = 0;
const uint16_t addrFlashNET  = 0x00;
const uint16_t addrFlashPASS = 0x20;
 
void setup() {
  Serial.begin(115200);
  EEPROM.begin(256);
  for (int n = 0; n<21; n++) {
      newNET[n] = 0x00; 
      newPASS[n] = 0x00;
  }
  for (int n = 0; n<20; n++) {
     char ch = EEPROM.read(addrFlashNET + n); 
     newNET[n] = ch;
  }
  Serial.println("Storage NET: " + String(newNET));
  for (int n = 0; n<20; n++) {
     char ch = EEPROM.read(addrFlashPASS + n); 
     newPASS[n] = ch;
  }
  Serial.println("Storage PASS: " + String(newPASS));  
  pinMode(LED_BUILTIN, OUTPUT);  
  digitalWrite(LED_BUILTIN, HIGH); // LED off

  // by default, we'll generate the high voltage from the 3.3v line internally! (neat!)
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);  // initialize with the I2C addr 0x3C (for the 64x48)
  // Clear the buffer. 
  display.clearDisplay();
  display.setRotation(2);
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0,0);  
  display.println("Star WEB");  
  display.println("(c)Pyhesty");    
  display.println("Connect to"); 
  display.println(newNET);      
  display.display(); 
  // Connect to Wi-Fi network with SSID and password
  Serial.print("Connecting to ");
  Serial.println(newNET);
  //WiFi.begin(ssid, password);
  WiFi.begin(newNET, newPASS);
  while ((WiFi.status() != WL_CONNECTED)and(!NeedNewConnect)) {
    cntshowDot++;
    if (cntshowDot>10) {
      cntshowDot = 0;
      display.clearDisplay(); 
      display.drawPixel(posx,posy, WHITE);
      display.display();      
    }
    delay(500);
    Serial.print(".");
    if (Serial.available() > 0) {       
        Serial.println();
        String str = Serial.readString(); // read the incoming byte:        
        Serial.println("received: "+ str);
        if (str[0] == '#') {
          str = str.substring(1);
          Serial.println("new NET :" + str);
          str.toCharArray(newNET, str.length());
          newNET[str.length()-2] = 0x00;
          str = String(newNET);
          Serial.println("new NET :" + str);
        }else
        if (str[0] == '$') {
          str = str.substring(1);
          Serial.println("new password :" + str);
          str.toCharArray(newPASS, str.length());
          newPASS[str.length()-2] = 0x00;
          str = String(newPASS);
          Serial.println("new password :" + str); 
          //for (int i = 1; i< 10; i++)
          //  Serial.println(String(byte(newPASS[i])));        
          NeedNewConnect = true;
            for (int n = 0; n<20; n++) 
               EEPROM.write(addrFlashNET + n, newNET[n]);             
            for (int n = 0; n<20; n++) 
               EEPROM.write(addrFlashPASS + n, newPASS[n]);                
              if (EEPROM.commit()) {
                Serial.println("NET Info EEPROM successfully committed");
              } else {
                Serial.println("ERROR! EEPROM commit failed");
              }  
        }else
          Serial.println("don't understand: need prefix # - new NET, $ - new password");               
    }
  }
  if (NeedNewConnect)
  {
    WiFi.disconnect();
    //WiFi.end();    
    display.clearDisplay();     
    display.setCursor(0,0);  
    display.println("New connect");  
    display.println("Connect to"); 
    display.println(newNET);      
    display.println(newPASS);          
    display.display();      
    Serial.print("Connecting to ");
    Serial.println(newNET);    
    WiFi.begin(newNET, newPASS);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }     
  }
  
  display.clearDisplay();
  display.setCursor(0,0);  
  display.println("IP address:");  
  display.println(WiFi.localIP());    
  display.display(); 
    
  // Print local IP address and start web server
  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");  
  Serial.println(WiFi.localIP());
  server.begin();

  delay(2000);
  display.clearDisplay(); 
  display.drawPixel(posx,posy, WHITE);
  display.display();
  delay(500);  
  
}

bool newcommand = false;
void cmd(){
    if (newcommand){
      newcommand = false;  
        if (clearAll) {
            clearAll = false;
            display.clearDisplay();            
        } else {    
              if (clearPixel == "yes")   
                display.drawPixel(posx,posy, BLACK);        
              display.drawPixel(newposx,newposy, WHITE);        
              posx = newposx;
              posy = newposy;              
        }
        display.display(); 
    }
}

void loop(){
  WiFiClient client = server.available();   // Listen for incoming clients
  cmd();
  if (client) {                             // If a new client connects,
    Serial.println("New Client.");          // print a message out in the serial port
    String currentLine = "";                // make a String to hold incoming data from the client
    while (client.connected()) {            // loop while the client's connected
      if (client.available()) {             // if there's bytes to read from the client,
        char c = client.read();             // read a byte, then
        Serial.write(c);                    // print it out the serial monitor
        header += c;
        if (c == '\n') {                    // if the byte is a newline character
          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request, so send a response:
          if (currentLine.length() == 0) {
            // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
            // and a content-type so the client knows what's coming, then a blank line:
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println("Connection: close");
            client.println();
            
            // turns the GPIOs on and off
            if (header.indexOf("GET /UP") >= 0) {              
              newposy = newposy - 1;
              Serial.print("input UP:");Serial.print(newposx);Serial.print(",");Serial.println(newposy);              
              newcommand = true;
            } else if (header.indexOf("GET /DOWN") >= 0) {
              newposy = newposy + 1;
              Serial.print("input DOWN:");Serial.print(newposx);Serial.print(",");Serial.println(newposy);              
              newcommand = true;
            } else if (header.indexOf("GET /LEFT") >= 0) {
              newposx = newposx - 1;              
              Serial.print("input LEFT:");Serial.print(newposx);Serial.print(",");Serial.println(newposy);              
              newcommand = true;
            } else if (header.indexOf("GET /RIGHT") >= 0) {
              newposx = newposx + 1;              
              Serial.print("input RIGHT:");Serial.print(newposx);Serial.print(",");Serial.println(newposy);              
              newcommand = true;
            } else if (header.indexOf("GET /CENTRE") >= 0) {
              newposx = 32;              
              newposy = 24;              
              Serial.print("input CENTRE:");Serial.print(newposx);Serial.print(",");Serial.println(newposy);              
              newcommand = true;              
            } else if (header.indexOf("GET /LED/on") >= 0) {
              Serial.println("GPIO LED on");
              outputLEDState = "on";
              digitalWrite(LED_BUILTIN, LOW); 
            } else if (header.indexOf("GET /LED/off") >= 0) {
              Serial.println("GPIO LED off");
              outputLEDState = "off";
              digitalWrite(LED_BUILTIN, HIGH); 
            } else if (header.indexOf("GET /ClearPixel/yes") >= 0) {
              Serial.println("Clear Pixel YES");
              clearPixel = "yes";
            } else if (header.indexOf("GET /ClearPixel/no") >= 0) {
              Serial.println("Clear Pixel NO");
              clearPixel = "no";
            } else if (header.indexOf("GET /ClearPixel/ALL") >= 0) {
              Serial.print("input CLEAR ALL:"); Serial.print(newposx);Serial.print(",");Serial.println(newposy);              
              clearAll = true;
              newcommand = true;   
            }
            
            
            
            // Display the HTML web page
            client.println("<!DOCTYPE html><html>");
            client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
            client.println("<link rel=\"icon\" href=\"data:,\">");
            // CSS to style the on/off buttons 
            // Feel free to change the background-color and font-size attributes to fit your preferences
            client.println("<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}");
            client.println(".button { background-color: #195B6A; border: none; color: white; padding: 16px 40px;");
            client.println("text-decoration: none; font-size: 30px; margin: 2px; cursor: pointer;}");
            client.println(".button2 {background-color: #77878A;}</style></head>");
            
            // Web Page Heading
            client.println("<body><h1>STAR ping-pong web server</h1>");
            client.println("<p>posiion pixel (x,y): " + String(posx) +","+ String(posy) + "</p>");            
            client.println("<p><a href=\"/UP\"><button class=\"button\">UP</button></a></p>");                    
            client.println("<p><a href=\"/LEFT\"><button class=\"button\">LEFT</button></a> <a href=\"/CENTRE\"><button class=\"button\">CENTRE</button></a> <a href=\"/RIGHT\"><button class=\"button\">RIGHT</button></a></p>");           
//            client.println("<p><a href=\"/RIGHT\"><button class=\"button\">RIGHT</button></a></p>");        
            client.println("<p><a href=\"/DOWN\"><button class=\"button\">DOWN</button></a></p>");       
                   
            // Display current state, and ON/OFF buttons for GPIO 4              
            client.println("<p>GPIO LED - State " + outputLEDState + "</p>");            
            if (outputLEDState=="on") {
              client.println("<p><a href=\"/LED/off\"><button class=\"button\">ON</button></a></p>");
            } else {
              client.println("<p><a href=\"/LED/on\"><button class=\"button button2\">OFF</button></a></p>");
            }                           
            client.println("<p>Clear old pixel : " + clearPixel + "</p>");            
            if (clearPixel=="yes") {
              client.println("<p><a href=\"/ClearPixel/no\"><button class=\"button\">YES</button></a>  <a href=\"/ClearPixel/ALL\"><button class=\"button\">CLEAR ALL</button></a></p>");
            } else {
              client.println("<p><a href=\"/ClearPixel/yes\"> <button class=\"button button2\">NO</button></a>  <a href=\"/ClearPixel/ALL\"><button class=\"button\">CLEAR ALL</button></a></p>");
            }                           
            client.println("<p>(c)Pyhesty/Alex Stars WEB Server, 2021  </p>");              
            client.println("</body></html>");            
            
            // The HTTP response ends with another blank line
            client.println();
            // Break out of the while loop
            break;
          } else { // if you got a newline, then clear currentLine
            currentLine = "";
          }
        } else if (c != '\r') {  // if you got anything else but a carriage return character,
          currentLine += c;      // add it to the end of the currentLine
        }
      }
    }
    // Clear the header variable
    header = "";
    // Close the connection
    client.stop();
    Serial.println("Client disconnected.");
    Serial.println("");
  }
}
