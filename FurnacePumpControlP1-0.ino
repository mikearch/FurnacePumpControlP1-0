#include <SPI.h>
#include <Ethernet.h>
#include <PubSubClient.h>               //For Mosquito
#include <Wire.h>
#include <TimeLib.h>
#include <LiquidCrystal_I2C.h>
#include <DS1307RTC.h>
#include "DHT.h"

#define DHTPIN 2                       // digital pin for temperature data
#define DHTTYPE DHT22                  // DHT 22  (AM2302), AM2321
DHT dht(DHTPIN, DHTTYPE);

byte mac[]    = {  0xDE, 0xED, 0xBA, 0xFE, 0xFE, 0xED };  //For Mosquito don't change

IPAddress ip(192, 168, 1, 109);       // Replace with an IP that is unused for network
IPAddress server(192, 168, 1, 103);   //Raspberry Pi IP Address

// Initializes the clients
EthernetClient ethClient;
PubSubClient client(ethClient);

float temp_in_celsius = 0;
float temp_in_kelvin=0;
float temp_sensor_adj = .9;
float T=0;                             //value of Temp Cent returned by temp function

const int buttonPin = 4;               // the number of the pushbutton pin for backlight
const int pumpPin =5;                  // pin controlling relay for irrigation pump
const int pumpOff = 9;
const int furnacePin =6;               // pin controlling relaz for furnace 
//const int ledPin7 = 7;                //part of Internet hookup test
const int internetPin = 8;              //pin for turning internet access on or off
const int furnaceModePin = 10;           //pin to receive Home or Away signal from Internet Switch

int internetButton;                      //variable for ON OFF state of internet button
int buttonState = 0;                   // variable for reading the pushbutton status-LOW turns on LCD
int prevbuttonState = 0;               //stores previous LCD button state to see if it has changed

unsigned long Basetime;                //variable for time of current bootup
unsigned long CurtimePump;             //time of pump at snapshot
unsigned long PumpOnTime;              //Target time for turning on pump
unsigned long ActPumpOnTime;           //Actual time when the pump was turned on
unsigned long PumpRunningTime;         //Length in seconds that pumpPin has been LOW
unsigned long Timediff;                //seconds remaining until PumpOnTime
unsigned long PumpInterval = 86400;    //One day cycle for pump time
//unsigned long PumpInterval = 200;    //for testing purposes only should be commented out
unsigned long StartOfDayTime;          //midnight(start of day) of current day
unsigned long PumpOnHour = 28800;      //Seconds from midnight to 8:00AM
//unsigned long PumpOnHour = 66720;    //Seconds from midnight to test time.should be commented out

int pumpStatus = HIGH;

//Heating Variables-------------------------------------------------------------------------------
int HeatingMode = 1;                    //Mode of Heating system 0 = Away, 1= Home
unsigned long CurtimeFurnace;           //time of furnace at execution of furnace control function
unsigned long FurnaceOnTime;            //time that the furnace was turned on
unsigned long FurnaceRunningTime;        //length of time in seconds that the furnance has been on
int furnaceStatus = LOW;                //sets status variable to LOW (furnance On)
float HeatOnTemp = 10;                  //minimum temperature allowed when system is set to AWAY
unsigned long HeatingOffDelay = 7200;    //furnace runs for 2hrs (7200 seconds) past satisfaction of minimum temp
int HoldFurnace = 0;                    //allows mainbody to test if Furnace is running within the HeatingDelay time
unsigned long FurnaceTimeRemaining = 0;  //the amount of seconds remaining from current time to furnace off for Heating Delay

unsigned long daystart(void);
void lcd_on_off(void);
void PumpControl(void);
float temp(void);

LiquidCrystal_I2C lcd(0x27,20,4);      // set the LCD address to 0x27 for a 20 chars and 4 line display

//----Publishing Logic Function---------------------------------------------------------------------------------
// This functions is executed when some device publishes a message to a topic that your Arduino is subscribed to
// Change the function below to add logic to your program, so when a device publishes a message to a topic that 
// your Arduino is subscribed you can actually do something

void callback(char* topic, byte* message, unsigned int length) {
  Serial.print("Message arrived on topic: ");
  Serial.print(topic);
  Serial.print(". Message: ");
  String messageTemp;
  
  for (int i = 0; i < length; i++) {
    Serial.print((char)message[i]);
    messageTemp += (char)message[i];
  }
  Serial.println();
  // Feel free to add more if statements to control more Pins with MQTT

  // If a message is received on the topic home/livingroom/arduino/ledPin6 
  // It's a value between 0 and 255 to adjust the LED brightness
  if(String(topic)=="home/bar/pin6"){
      Serial.print("Changing Digital Pin 6 Brithness to ");
      Serial.print(messageTemp);
      // analogWrite(furnacePin, messageTemp.toInt());
  }
  // If a message is received on the topic home/livingroom/arduino/ledPin7, 
  //you check if the message is either 1 or 0. Turns the Arduino Digital Pin according to the message
  if(String(topic)=="home/bar/pin7"){
      Serial.print("Changing Digital Pin 7 to ");
      if(messageTemp == "1"){
        //digitalWrite(ledPin7, HIGH);
        Serial.print("On");
      }
      else if(messageTemp == "0"){
        //digitalWrite(ledPin7, LOW);
        Serial.print("Off");
      }
  }
  Serial.println();
}
//----End of Publishing Logic Function---------------------------------------------------------------


//----Reconnect Function------------------------------------------------------------------------------
// This functions reconnects your Arduino to your MQTT broker
// Change the function below if you want to subscribe to more topics with your Arduino 
void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("arduinoClient")) {
      Serial.println("connected");
      // Subscribe or resubscribe to a topic
      // You can subscribe to more topics (to control more LEDs in this example)
      client.subscribe("home/bar/pin6");
      client.subscribe("home/bar/pin7");
      client.subscribe("inTopic");
      client.subscribe("outTopic");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

//---End of Reconnect Function---------------------------------------------------------

void setup() {
    
  //General Setup---------------------------------------
  pinMode(buttonPin, INPUT);          // initialize LCD backlight button 
  digitalWrite (buttonPin, LOW);      //set LCD control pin to LOW(ON)
  Serial.begin(9600);
  //NodeRed Ethernet Setup---------------------------------------
  
  //pinMode(furnacePin, OUTPUT);      //controls low voltate relay for furnace control redundant
  //pinMode(ledPin7, OUTPUT);
  pinMode(internetPin, INPUT);        //sets internet pin to INPUT

  client.setServer(server, 1883);
  client.setCallback(callback);

  Ethernet.begin(mac, ip);
  
  delay(1500);                        // Allow the hardware to sort itself out
  
 //LCD Setup------------------------------------------------------------ 
  lcd.init();                         // initialize the lcd 
  lcd.init();                         //2nd init per example sketches
  lcd.backlight();

//DHT Setup-------------------------------------------------------------  
  lcd.setCursor(0,2);
  lcd.print("T:");
  lcd.setCursor(11,2);
  lcd.print("H:");
  dht.begin();
 
//RTC Setup--------------------------------------------------------------
 setSyncProvider(RTC.get);                  // the function to get the time from the RTC

 //Pump Control Setup-----------------------------------------------------
  StartOfDayTime = daystart();              //Gets the UNIX time for midnight of current day
  
  Basetime = RTC.get();                     //gets current time in seconds when system has started
  PumpOnTime = StartOfDayTime + PumpOnHour; //sets initial pump operation for current time plus interval
  if (Basetime > PumpOnTime) {              //checks to see if the day's pump has been missed
    PumpOnTime = PumpOnTime + PumpInterval; //if missed set it for the next pump on time
  }
  pinMode(pumpPin, OUTPUT);                 //initiates pin for pump relay
  pinMode(pumpOff, INPUT),
  digitalWrite(pumpPin,HIGH);               //sets pin to release relay
  digitalWrite (pumpOff, HIGH);             //sets up pin for pump on-off button

 //Furnace Control Setup----------------------------------------------------
  pinMode(furnacePin, OUTPUT);               //initiates pin for furnace relay
  pinMode(furnaceModePin, INPUT);            //sets furnace mode pin to INPUT
  digitalWrite(furnacePin, HIGH);            //turns furnace on by default at startup--note relay wired Normally Closed
  lcd.setCursor(0,3);                        //Heating Mode Label
  lcd.print("H Mode:");
  lcd.setCursor(11,3);
  lcd.print("HS:");

//Debug Output------------------------------
//Serial.println("Setup Data");
//Serial.print("Basetime: "); Serial.print(Basetime); Serial.print("  PumpOnTime"); Serial.print(PumpOnTime); Serial.print("  StartOfDayTime");Serial.print(StartOfDayTime);Serial.print("  PumpOnHour"); Serial.print(PumpOnHour);
//Serial.println();

  lcd.setCursor(13,1);                      //Internet ON or OFF label
  lcd.print("Int ");                        //Prints Label for Internet Satus
}
//------------END OF SETUP--------------

void loop() {
  lcd.setCursor(17,1);                        //Sets cursor to Internet ON or OFF position
  
  internetButton = digitalRead(internetPin);
  if (internetButton == 1) {                  //Checks if Internet On button is depressed, if not skipt Internet Connection Routine
   lcd.print("ON "); 
   if (!client.connected()) {
    reconnect();
  }
  if(!client.loop()) {
    client.connect("arduinoClient");
  }
  }
  else {
    lcd.print("OFF");
  }
  lcd_on_off();                             //turns lcd backlight on or off based upon switch



//-----------------GETS AND DIPLAYS CURRENT TIME AND DATE  
  //Reads RTC
  tmElements_t tm;
    
  if (RTC.read(tm)) 
   {
      lcd.setCursor(15,0);
      if ((tm.Hour)<10) 
         {
              lcd.print("0");  
         }
      lcd.print(tm.Hour);
      lcd.print(":");
      
      if ((tm.Minute)<10) 
         {
              lcd.print("0");
         }
      lcd.print(tm.Minute);
      lcd.setCursor(0,0);
      lcd.print(tm.Day); 
      lcd.print("/");
      lcd.print(tm.Month);
      lcd.print("/");
      lcd.print(tmYearToCalendar(tm.Year));  
    } 
    
    else 
    {
      lcd.setCursor(0,0);
      if (RTC.chipPresent()) 
        {
              lcd.print("Run the SetTime");
        } 
      else 
        {
              lcd.print("Chk Circuits");
        }
      }
//------------------End of Time and Date Routine



  PumpControl();                        //calls function to control pump

  lcd.setCursor(0,1);
  lcd.print("Pump Sys: ");
  lcd.setCursor(10,1);
 if (digitalRead(pumpOff) == HIGH) {
    lcd.print("On ");
  }
  else {
    lcd.print("Off");
  }
/*
lcd.setCursor(0,2);
  lcd.print("Pump State: ");
  lcd.setCursor(13,2);
  if (digitalRead(pumpPin) == HIGH) {
    
    lcd.print("OFF");
  }
  else  {
    lcd.print("ON ");
  }
*/  
  T = temp();                             //calls temperature reading and display function
 Serial.println(T);
 Serial.println(HeatOnTemp);
 Serial.println(digitalRead(furnaceModePin));
//---Furnace Control----------------------------------------------------------------------

   
   if (digitalRead(furnaceModePin) == LOW) {      //Checks if power from remote switch is OFF (HOME)
      
      digitalWrite(furnacePin, HIGH);    //Turns Furnace ON by keeping relay low
      lcd.setCursor(8,3);
      lcd.print("H"); 
      }
     
    if (digitalRead(furnaceModePin) == HIGH){         //Controls furnace to maintain minimum temperature
    
        if (T < HeatOnTemp){                          //If less than target T, pass control to function that turns furn
          FurnaceControl();
        }
        if (HoldFurnace == HIGH){                     // If furnace is in HOLD mode (minimum 2hr run time) pass control to function
          FurnaceControl();
        }

        if (HoldFurnace == LOW){                      //  If furncace is not in HOLD mode and T >= to target, turn furnace off
          if (T >= HeatOnTemp){
          digitalWrite(furnacePin, LOW);
          }
        }
        
       lcd.setCursor(8,3);
       lcd.print("A");   
      }
  
   
   lcd.setCursor(15,3);
   lcd.print(HoldFurnace); 
}

//------------END OF VOID LOOP--------------



