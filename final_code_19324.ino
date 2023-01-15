#include <ESP8266WiFi.h> // esp8266 library
#include "ThingSpeak.h" // always include thingspeak header file
#define SECRET_SSID "19324"    // your WiFi network name
#define SECRET_PASS "cap19324" //your WiFi password
#define SECRET_CH_ID 1978179     // channel number
#define SECRET_WRITE_APIKEY "FXO8COS70DAPM1RD"   //your channel write API Key
unsigned long myChannelNumber = SECRET_CH_ID;
const char * myWriteAPIKey = SECRET_WRITE_APIKEY;
char ssid[] = SECRET_SSID;    //  your network SSID (name)
char pass[] = SECRET_PASS;   // your network password
int keyIndex = 0;            // your network key Index number (needed only for WEP)
WiFiClient  client;
#define SENSOR  14 //define varaible for flow sesnor
#include <SFE_BMP180.h> //library for pressure sensor
#include <Wire.h> // library for esp8266 x
SFE_BMP180 pressure; 
#include <SPI.h>
#define ALTITUDE 12.0  // Altitude of the area we are in tanta
long currentMillis = 0; // assign varaible type long for curentmillis (current milli litres) and begin with zero
long previousMillis = 0; // assign varaible type long for previousmillis ( previous milli litres) and begin with zero
int interval = 1000; //
float calibrationFactor = 4.5; //
volatile byte pulseCount; // 
byte pulse1Sec = 0; //
float flowRate; // assign float values to the varaible flowrate
unsigned long flowMilliLitres;
unsigned int totalMilliLitres;
float flowLitres; // assign float values to the varaible flowlitres
float totalLitres; // assign float values to the varaible totallitres
float Pnew ;//assign avariable named Pnew for the pressure difference with float values
float flows[500]; //
float pressures[500]; //
int counter = 0; //
int sw = 0; //
double  p1, P, p2, T ; // assign p1 for sesnor 1,p2 for sesnor 2 , P for relative sea pressure , T for tempreture
void IRAM_ATTR pulseCounter() //
{
  pulseCount++;
}
void setup() //
{
//start flow sensor reading from zero
  Serial.begin(115200); 
  pinMode(SENSOR, INPUT_PULLUP); 
  pulseCount = 0; 
  flowRate = 0.0; 
  flowMilliLitres = 0; 
  totalMilliLitres = 0; 
  previousMillis = 0; 
  attachInterrupt(digitalPinToInterrupt(SENSOR), pulseCounter, FALLING); //
  pinMode(2, OUTPUT);// sensor 1 
  digitalWrite(2, HIGH); 
  pinMode(15, OUTPUT); // sensor 2
  digitalWrite(15, LOW);
  Serial.println("REBOOT"); 
  // Initialize the sensor (it is important to get calibration values stored on the device)
  if (pressure.begin())
    Serial.println("BMP180 init success");
  else {
    Serial.println("BMP180 init fail");
    while (1)
      ;
  }
  while (!Serial) {
    ; // wait for serial port to connect. 
  }
  WiFi.mode(WIFI_STA);
  ThingSpeak.begin(client);  // Initialize ThingSpeak
  pinMode(0, INPUT_PULLUP); //button when pressed, it sends the data from array to ThingSpeak
}
//start measring pressure difference and flow rate array 
void loop() {
  Serial.println("Measuring");
  pressureSensor();
  flowSensor();
  counter++;
  Serial.print("counter : ");
  Serial.println(counter);// counter from 1 to 500 readings 
  if (counter > 499)
  {
    Serial.println("stopped");// when more than 500 readings are measured
    while (1);
  }
  if (digitalRead(0) == 0)
  {
    Serial.print("Sending data amount of : ");
    Serial.println(counter);
    sendingData();
  }
  delay(1000);
}
void flowSensor()
{
  currentMillis = millis();
  if (currentMillis - previousMillis > interval)
  {
    pulse1Sec = pulseCount;
    pulseCount = 0;
    //calibration factor to convwert the time interval to seconds
    flowRate = ((1000.0 / (millis() - previousMillis)) * pulse1Sec) / calibrationFactor;
    previousMillis = millis();
    //divide by 60 to get mins and multiply by 1000 to get ml
    flowMilliLitres = (flowRate / 60) * 1000;
    flowLitres = (flowRate / 60);
    // Add the millilitres passed in this second to the cumulative total
    totalMilliLitres += flowMilliLitres;
    totalLitres += flowLitres;
    // Print the flow rate for this second in litres / minute
    Serial.print("Flow rate: ");
    Serial.print(float(flowRate)); // Print the integer part of the variable
    Serial.print("l/min ");
   // Serial.print("wind speed:"); // convert flowrate to windspeed.
    //Serial.print(float(flowRate * 0.0401123));
    //ASerial.print("m/s");
    // Print the cumulative total of litres flowed since starting
    Serial.print("Output air Quantity: ");
    Serial.print(totalMilliLitres);
    Serial.print("mL / ");
    Serial.print(totalLitres);
    Serial.println("L");
    flows[counter] = flowRate;
  }
}
void pressureSensor()
{
  Serial.println();
  Serial.print("provided altitude: ");
  Serial.print(ALTITUDE, 0);
  Serial.print(" meters, "); 
  //swiching between the two barometric sensors 
  if (sw) {
    digitalWrite(2, HIGH); // sensor 1 is on
    digitalWrite(15, LOW);
    Serial.println("sensor 1 ");
    p2= p1;
    sw = 0;
  } else {
    digitalWrite(2, LOW);
    digitalWrite(15, HIGH);// sensor 2 is on
    Serial.println("sensor 2 ");
    p2= p1;
    sw = 1;
  }
  int status = pressure.startTemperature();
  if (status != 0) {
    status = pressure.getTemperature(T);
    if (status != 0) {
      status = pressure.startPressure(3);
      if (status != 0) {
        status = pressure.getPressure(P, T);
        if (status != 0) {
          p1 = pressure.sealevel(P, ALTITUDE);  
          p1 = p1 * 0.000986923;
          Serial.print("relative (sea-level) pressure: ");
          Serial.print(p1, 2);
          Serial.println(" atm");
          Pnew = p1 - p2;
          if (Pnew < 0)
            Pnew = Pnew * -1;
          Serial.print("pressure difference: ");
          Serial.println(Pnew);
          pressures[counter] = Pnew;
        } else Serial.println("error retrieving pressure measurement");
      } else Serial.println("error starting pressure measurement");
    } else Serial.println("error retrieving temperature measurement");
  } else Serial.println("error starting temperature measurement");
}
void sendingData()
//connecting the esp to the wifi network
{
  if (WiFi.status() != WL_CONNECTED) {
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(SECRET_SSID);
    while (WiFi.status() != WL_CONNECTED) {
      WiFi.begin(ssid, pass);
      Serial.print(".");
      delay(5000);
    }
    Serial.println("Connected.");
  }

  // set the fields with the values
  //  int x[4] = {1, 2, 3, 4};
  for (int a = 0; a <= counter; a++)
  {
    ThingSpeak.setField(1, flows[a]);
    ThingSpeak.setField(2, pressures[a]);
    // write to the ThingSpeak channel
    int x = ThingSpeak.writeFields(myChannelNumber, myWriteAPIKey);
    if (x == 200) {
      Serial.println("Channel update successful.");
    }
    else {
      Serial.println("Problem updating channel. HTTP error code " + String(x));
    }
    delay(15000);
  }
      counter=0;
}
