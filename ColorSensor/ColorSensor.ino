/*********
 Color Sensor Analyzer
*********/

// Load Wi-Fi library
#include <ESP8266WiFi.h>

// Include I2C Library
#include <Wire.h>

// Include Sparkfun ISL29125 Library
#include "SparkFunISL29125.h"

// Replace with your network credentials
const char* ssid     = "Tomato96";
const char* password = "Mango!321";

// Set web server port number to 80
WiFiServer server(80);

// Variable to store the HTTP request
String header;

// Auxiliar variables to store the current output state
String mixerState = "off";
String ledState = "off";
String ledValue = "20"; //default value 20

// Assign output variables to GPIO pins
const int mixer = 14;
const int led = 0;

// Current time
unsigned long currentTime = millis();
// Previous time
unsigned long previousTime = 0; 
// Define timeout time in milliseconds (example: 2000ms = 2s)
const long timeoutTime = 2000;

/////

// Declare sensor object
SFE_ISL29125 RGB_sensor;

// Calibration values
unsigned int redlow = 589;
unsigned int redhigh = 3870;
unsigned int greenlow = 961;
unsigned int greenhigh = 5181;
unsigned int bluelow = 773;
unsigned int bluehigh = 4109;

// Declare RGB Values
int redVal = 0;
int greenVal = 0;
int blueVal = 0;

void setup() {
  Serial.begin(115200);

  // Initialize the output variables as outputs
  pinMode(mixer, OUTPUT);
  pinMode(led, OUTPUT);
  // Set outputs to LOW
  digitalWrite(mixer, LOW);
  digitalWrite(led, LOW);

  // Initialize the ISL29125 with simple configuration so it starts sampling
  if (RGB_sensor.init())
  {
    Serial.println("Sensor Initialization Successful");
  }
  
  // Connect to Wi-Fi network with SSID and password
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  // Print local IP address and start web server
  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  server.begin();
}

// method for turning Mixer on/off
void writeMixer(int Mixeron){
  if (Mixeron == 1){
    mixerState = "on";
    Serial.println("Mixer on");
    digitalWrite(mixer, HIGH);
  } else if (Mixeron == 0){
    mixerState = "off";
    Serial.println("Mixer off");
    digitalWrite(mixer, LOW);
  }
}

// method for turning LED on/off
void writeLED(int LEDon){
  if (LEDon ==1){
    ledState = "on";
    int ledValueP = (255*(ledValue.toFloat()/100));
    Serial.print("LED on at value:");
    Serial.print(ledValueP);
    Serial.print(" of ");
    Serial.print(ledValue);
    Serial.println("%");
    analogWrite(led, ledValueP);
  } else if (LEDon == 0){
    ledState = "off";
    Serial.println("LED off");
    digitalWrite(led, LOW);
  }
}

// method update led value from GET header response
void updateLedValueFromHeader(String header){
  int idx = header.indexOf("GET");
  String res = header.substring(idx+9,idx+12);
  int hasValue = header.indexOf("H");
  if (hasValue>0){
    res = res.substring(0,2);
  }
  res.trim();
  ledValue = res;
}

void readSensorValues(){
  ///////////////// COLOR
  // Read sensor values (16 bit integers)
  unsigned int red = RGB_sensor.readRed();
  unsigned int green = RGB_sensor.readGreen();
  unsigned int blue = RGB_sensor.readBlue();

  // Convert to RGB values
  int redV = map(red, redlow, redhigh, 0, 255);
  int greenV = map(green, greenlow, greenhigh, 0, 255);
  int blueV = map(blue, bluelow, bluehigh, 0, 255);

  // Constrain to values of 0-255
  redVal = constrain(redV, 0, 255);
  greenVal = constrain(greenV, 0, 255);
  blueVal = constrain(blueV, 0, 255);

  Serial.print("Red: ");
  Serial.print(redVal);
  Serial.print(" - Green: ");
  Serial.print(greenVal);
  Serial.print(" - Blue: ");
  Serial.println(blueVal);

  // Delay for sensor to stabilize
  //delay(2000);
}

void loop(){
  ///////////////// WIFI
  WiFiClient client = server.available();   // Listen for incoming clients

  if (client) {                             // If a new client connects,
    Serial.println("New Client.");          // print a message out in the serial port
    String currentLine = "";                // make a String to hold incoming data from the client
    currentTime = millis();
    previousTime = currentTime;
    while (client.connected() && currentTime - previousTime <= timeoutTime) { // loop while the client's connected
      currentTime = millis();         
      if (client.available()) {             // if there's bytes to read from the client,
        char c = client.read();             // read a byte, then
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
            if (header.indexOf("GET /5/1") >= 0) {
              writeMixer(1);
            } else if (header.indexOf("GET /5/0") >= 0) {
              writeMixer(0);
            } else if (header.indexOf("GET /4/1") >= 0) {
              updateLedValueFromHeader(header);
              writeLED(1);
            } else if (header.indexOf("GET /4/0") >= 0) {
              updateLedValueFromHeader(header);
              writeLED(0);
            }
            
            // Display the HTML web page
            client.println("<!DOCTYPE html><html>");
            client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
            client.println("<link rel=\"icon\" href=\"data:,\">");
            // CSS to style
            client.println("<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: left;}");
            client.println(".button { background-color: #195B6A; border: none; color: white; padding: 16px 20px;");
            client.println("text-decoration: none; font-size: 30px; margin: 2px; cursor: pointer;}");
            client.println("input[type=text] {width: 3%;padding: 20px 20px;display: inline-block;box-sizing: border-box");
            client.println(".button2 {background-color: #77878A;}</style></head>");
            
            // Web Page Heading
            client.println("<body><h1>Wacky Color Analyzer</h1>");

            client.println("<div style=\"width: 100%;\">");
            // Display sample test kit chart
            client.println("<div id=\"color_sample\" style=\"width: 50%; float: right;\"></div>");
            
            // Display dropdown options for Test kits
            client.println("<div style=\"margin-right: 50%;\">");
            client.println("<p>Select Test Kit:</p>");
            client.println("<select id=\"myTestKit\" onchange=\"setTestKitValues(this.value)\"><option></option><option value=\"no3\">NO3</option>\r\n<option value=\"po3\">PO3</option>\r\n<option value=\"ph\">PH</option>\r\n<option value=\"nh3\">NH3</option></select>");
            // Display current state, and ON/OFF buttons for Mixer  
            client.println("<p>Mixer - State " + mixerState + "</p>");
            // If the mixerState is off, it displays the ON button       
            if (mixerState=="off") {
              client.println("<p><a href=\"/5/1\"><button class=\"button\">ON</button></a></p>");
            } else {
              client.println("<p><a href=\"/5/0\"><button class=\"button button2\">OFF</button></a></p>");
            } 
               
            // Display current state, and ON/OFF buttons for LED  
            client.println("<p>LED - State " + ledState + "</p>");
            // If the ledState is off, it displays the ON button       
            if (ledState=="off") {
              client.println("<a href=\"#\" onClick=\"return goToLed(this,1)\"><button class=\"button\">ON</button></a>");
            } else {
              client.println("<a href=\"#\" onClick=\"return goToLed(this,0)\"><button class=\"button button2\">OFF</button></a>");
            }
            client.println("<input type=\"text\" id=\"ledValueId\" maxlength=\"3\" value=\"" + ledValue + "\">%");
            client.println("<div>");

            client.println("<div style=\"margin-bottom: 50%; height: 100px;\">");
            client.println("<p><button style=\"float: left;\" onClick=\"window.location.reload();\">Refresh Results</button></p><br>");
            // Test result div
            client.println("<div id=\"testResultDiv\">\n<h3 style=\"text-align:left\">Test Result:</h3>\n<h1 style=\"background-color:rgb("+(String)redVal+","+(String)greenVal+","+(String)blueVal+");\">Red:"+redVal+" Green:"+greenVal+" Blue:"+blueVal+"</h1></div>");
            client.println("<div>");
            client.println("<div>");
            
            client.println("<script>");
            client.println("function goToLed(link,swi){var e=document.getElementById(\"ledValueId\").value;var t=\"/4/\"+swi+\"/\"+e;link.setAttribute(\"href\",t);link.click();return true};");
            client.println("function setTestKitValues(sel) {\r\n var obj, dbParam, xmlhttp, myObj, x, txt = \"\";\r\n obj = { table: sel, limit: 20 };\r\n dbParam = JSON.stringify(obj);\r\n xmlhttp = new XMLHttpRequest();\r\n xmlhttp.onreadystatechange = function() {\r\n if (this.readyState == 4 && this.status == 200) {\r\n myObj = JSON.parse(this.responseText);\r\n txt += \"<table border='1'>\"\r\n for (i in myObj) {\r\n if(myObj[i].param == sel){\r\n txt += \"<tr>\"\r\n txt += \"<td>Test Kit: \" + myObj[i].name + \"</td>\";\r\n txt += \"<tr><td>Value</td><td>Red</td><td>Green</td><td>Blue</td><td>Color</td></tr>\";\r\n document.getElementById(\"ledValueId\").value = myObj[i].led;\r\n for (j in myObj[i].sample) {\r\n txt += \"<tr><td>\" + myObj[i].sample[j].value + \"</td><td>\" + myObj[i].sample[j].r + \"</td><td>\" + myObj[i].sample[j].g + \"</td><td>\" + myObj[i].sample[j].b + \"</td>\"\r\n txt += \"<td style='background-color:rgb(\"+myObj[i].sample[j].r+\",\"+myObj[i].sample[j].g+\",\"+myObj[i].sample[j].b+\"')></td></tr>\";\r\n }\r\n txt += \"</tr>\"\r\n }\r\n }\r\n txt += \"</table>\" \r\n document.getElementById(\"color_sample\").innerHTML = txt;\r\n }\r\n };\r\n xmlhttp.open(\"GET\", \"https://api.mocki.io/v1/86bada30\", true);\r\n xmlhttp.setRequestHeader(\"Content-type\", \"application/x-www-form-urlencoded\");\r\n xmlhttp.send(\"x=\" + dbParam);\r\n}");
            client.println("</script>");
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
    
      // read sensor values
      if (ledState == "on"){
        readSensorValues();
      }
    
    // Clear the header variable
    header = "";
    // Close the connection
    client.stop();
    Serial.println("Client disconnected.");
    Serial.println("");
  }
}
