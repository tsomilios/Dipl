/* BLYNK SET UP
#define BLYNK_TEMPLATE_ID "TMPLlLyPj_pi"
#define BLYNK_DEVICE_NAME "New"
#define BLYNK_AUTH_TOKEN "G1uWmLDik_O6ZeKJFbqNFXC8n9M7i0mj"
Comment this out to disable prints and save space 
#define BLYNK_PRINT Serial*/
#include "Wire.h"
#include "RTClib.h"
#include <ESP8266_Lib.h> 
//#include <BlynkSimpleShieldEsp8266.h>
// Include Libraries
#include "Arduino.h"
#include "DHT.h"
#include "DS18B20.h"
#include <LiquidCrystal.h>
#include "SolenoidValve.h"
#include "DFRobot_PH.h"
#include "DFRobot_EC.h"
#include "Pump.h"
#include <EEPROM.h>
#include <TimeLib.h>
#include <SoftwareSerial.h>       //Software Serial library


SoftwareSerial espSerial(24, 22);   //Pin 2 and 3 act as RX and TX. Connect them to TX and RX of ESP8266      
// Pin Definitions
#define PH_PIN A1
#define EC_PIN A2
#define WATERLEVELSENSOR_5V_PIN_SIG	A3
#define DHT_PIN_DATA	9
#define DS18B20WP_PIN_DQ	8 
#define LCD_PIN_RS	53
#define LCD_PIN_E	51
#define LCD_PIN_DB4 43
#define LCD_PIN_DB5	45
#define LCD_PIN_DB6	47
#define LCD_PIN_DB7	49
#define SOLENOIDVALVE1_1_PIN_COIL1	2//ACID
#define SOLENOIDVALVE2_2_PIN_COIL1	3//BASE
#define SOLENOIDVALVE3_3_PIN_COIL1	4//MICRO 1
#define SOLENOIDVALVE4_4_PIN_COIL1	5//MICRO 2
#define SOLENOIDVALVE5_5_PIN_COIL1	6//MICRO 3
#define Rele_1 10
#define Rele_2 11
#define Rele_3 12//RELE GIA TA FWTA
#define Rele_4 13//RELE GIA TO PUMP

#define DEBUG true
String mySSID = "COSMOTE-702277";       // WiFi SSID
String myPWD = "2381024532"; // WiFi Password

String myAPI = "BB61Y26J07Y1TAP0";   // API Key
String myHOST = "api.thingspeak.com";
String myPORT = "80";
String myFIELD = "field1"; 
float sendPh;
float sendEc;
float sendWaterTemp;
float sendRoomTemp;
float sendHumid;

// object initialization
DHT dht(DHT_PIN_DATA);
DS18B20 ds18b20wp(DS18B20WP_PIN_DQ);
LiquidCrystal lcd(LCD_PIN_RS,LCD_PIN_E,LCD_PIN_DB4,LCD_PIN_DB5,LCD_PIN_DB6,LCD_PIN_DB7);
SolenoidValve solenoidValve1_1(SOLENOIDVALVE1_1_PIN_COIL1);
SolenoidValve solenoidValve2_2(SOLENOIDVALVE2_2_PIN_COIL1);
SolenoidValve solenoidValve3_3(SOLENOIDVALVE3_3_PIN_COIL1);
SolenoidValve solenoidValve4_4(SOLENOIDVALVE4_4_PIN_COIL1);
SolenoidValve solenoidValve5_5(SOLENOIDVALVE5_5_PIN_COIL1);
int RelayModule4chPins[] = {Rele_1,Rele_2,Rele_3,Rele_4 };
float voltage,phValue,temperature,ecValue,voltageEC;
DFRobot_PH ph;
DFRobot_EC ec;
RTC_PCF8523 rtcPCF;
int AlarmPH;         // alarm Ph
int AlarmTempWater;   // alarm Temp
int AlarmEC;         // alarm Ec
// define vars for testing menu
unsigned long previousMillis = 0;
unsigned long currentMillis = millis();
unsigned long period = 600000;       //define timeout of 10 sec.....1000 = 1 sec \\\ 600.000 = 10 min
float PhMax= 7;       // Max value for pH
float PhMin= 4;       // Min value for pH
float EcMax= 1.4;   // Max value for EC
float EcMin= 1;    // Min value for EC
float WaterTempMax= 40;    // Max value for Water Temp
float WaterTempMin= 5;    // Min value for Water Temp
long time0;
float phAverage = 0;
int timeStampOn = 6;//set the time of led to turned on in 24h format
int timeStampOff = 19;//set the time of led to turned off in 24h format
int TimeFlag=0;
int ledFlagOn = 0;
int timeStampPumpOn = 0;//time to activate the pump in 24h format
int timeStampPumpOff = 30;//time to deactivate the pump in 24h format
int pumpFlagOn=0;



void setup() 
{
    // Setup Serial which is useful for debugging
    // Use the Serial Monitor to view printed messages
    Serial.begin(9600); 
    espSerial.begin(115200); 
    while (!Serial) ; // wait for serial port to connect. Needed for native USB
    Serial.println("start");
    ph.begin();
    dht.begin();
    ec.begin();
    pinMode(Rele_1,OUTPUT);
    pinMode(Rele_2,OUTPUT);
    pinMode(Rele_3,OUTPUT);
    pinMode(Rele_4,OUTPUT);
    espData("AT+RST", 1000, DEBUG);                      //Reset the ESP8266 module
    espData("AT+CWMODE=1", 1000, DEBUG);                 //Set the ESP mode as station mode
    espData("AT+CWJAP=\""+ mySSID +"\",\""+ myPWD +"\"", 1000, DEBUG);   //Connect to WiFi network
    if (! rtcPCF.begin()) {
        Serial.println("Couldn't find RTC");
        while (1);
    }
    if (!rtcPCF.initialized()) {
        Serial.println("RTC lost power, lets set the time!");
        // following line sets the RTC to the date & time this sketch was compiled
        rtcPCF.adjust(DateTime(F(__DATE__), F(__TIME__)));
        // This line sets the RTC with an explicit date & time, for example to set
        // January 21, 2014 at 3am you would call:
        // rtcPCF.adjust(DateTime(2014, 1, 21, 3, 0, 0));
    }
    
    // set up the LCD's number of columns and rows
    lcd.begin(16, 2);
    // Setup a function to be called every second
    //timer.setInterval(1000L, myTimerEvent);
}

void loop()
{
    currentMillis = millis();
    Serial.println(currentMillis);
    DateTime now = rtcPCF.now();
        
    Serial.print(now.day(), DEC);
    Serial.print('/');
    Serial.print(now.month(), DEC);
    Serial.print('/');
        
    Serial.print(now.year(), DEC);
    Serial.print("  ");
    Serial.print(now.hour(), DEC);
    Serial.print(':');
    Serial.print(now.minute(), DEC);
    Serial.print(':');
    Serial.print(now.second(), DEC);
    Serial.println();
    delay(1000);
    ///////////////////////////////////////
    //Code that operates the pump        //
    //From 0 min to 30 min the pump is ON//
    ///////////////////////////////////////
    if(now.minute()<timeStampPumpOff && pumpFlagOn == 0){
        digitalWrite(RelayModule4chPins[2],HIGH);
        Serial.print("the pump is on for the next 30 min ");
        Serial.println();
        pumpFlagOn = 1;
        delay(1000);
        lcd.begin(16,2);
        lcd.clear();
        lcd.setCursor(0, 0); 
    }
    /////////////////////////////////////////
    //Frop 30 min to 59 min the pump is OFF//
    /////////////////////////////////////////
    else if(now.minute()>=timeStampPumpOff  && pumpFlagOn == 1){
        digitalWrite(RelayModule4chPins[2],LOW);
        Serial.print("the pump is off for the net 30 min ");
        Serial.println();
        pumpFlagOn = 0;
        delay(1000);
        lcd.begin(16,2);
        lcd.clear();
        lcd.setCursor(0, 0); 
    }
    //////////////////////////////////////////
    //Code that operates the Led grow lights//
    //From 6:00 to 18:00 the Led are ON     //
    //////////////////////////////////////////
    if(now.hour()>=timeStampOn && now.hour()<timeStampOff && ledFlagOn == 0){
        digitalWrite(RelayModule4chPins[0],HIGH);
        Serial.print("Time is 6:00 ,Led is on ");
        Serial.println();
        ledFlagOn = 1;
    }
    ///////////////////////////////////////////////////
    //From 18:00 to 6:00 the next day the LED are OFF//
    ///////////////////////////////////////////////////
    else if((now.hour()>=timeStampOff || now.hour()<timeStampOn) && ledFlagOn == 1 ){
        
        digitalWrite(RelayModule4chPins[0],LOW);
        Serial.print("Time is 19:00 ,Led is off ");
        Serial.println();
        ledFlagOn = 0;
        
    }
    float temperature=ds18b20wp.readTempC();
    float dhtHumidity = dht.readHumidity(); //read room humidity
    float dhtTempC = dht.readTempC();   //read room temp
    phValue = readPh() ; //calculate pH value
    ecValue = readEc ();//calculate ec value
    //////////////
    //LCD update//
    //////////////
    lcdUpdate(dhtHumidity,dhtTempC,phValue,ecValue);//LCD data display refresh
    /////////////////////
    //Thingspeak Update//
    /////////////////////
    updateThingspeak(phValue,ecValue,temperature,dhtHumidity,dhtTempC,pumpFlagOn,ledFlagOn);
    ////////////////////////////////
    //Code to adjust the pH values//
    ////////////////////////////////
    if(pumpFlagOn==0){
        if(now.minute() == 40){
            for (int i= 0 ;i <=6;i++ ){
                phAverage+=readPh();
                Serial.println(phAverage);
                delay(1500);
            }
            phAverage=phAverage/7;
            Serial.println(phAverage);

            if(phAverage>=PhMax || phAverage <= PhMin){
                adjustPh(phAverage,PhMax,PhMin);

            }
        }
    }


   



    
    // testing code
    char c = Serial.read();
    if(c == '1'){
        float dhtHumidity = dht.readHumidity(); //read room humidity
        float dhtTempC = dht.readTempC();   //read room temp
        phValue = readPh() ; //calculate pH value
        ecValue = readEc ();//calculate ec value
        Serial.print(F("Humidity: ")); Serial.print(dhtHumidity); Serial.print(F(" [%]\t"));
        Serial.print(F("Temp: ")); Serial.print(dhtTempC); Serial.println(F(" [C]\t"));

    }
    else if(c == '2'){        
        float temperature = ds18b20wp.readTempC();
        Serial.print(F("Water Temp: ")); Serial.print(temperature); Serial.println(F(" [C]\t"));
    } 
    else if(c == '3'){
        Serial.print("Water temp:");
        Serial.print(temperature);
        Serial.print(", EC:");
        Serial.print(ecValue,2);
        Serial.print(" ms/cm");
        Serial.print(" , pH: ");
        Serial.print(phValue,2);
        Serial.println(" \t");
    }
    else if(c == '4'){
        solenoidValve1_1.on(); // 1. turns on
        Serial.println("solonoid 1 open contains ph down");
        delay(36000);       // 2. waits 500 milliseconds (0.5 sec). Change the value in the brackets (500) for a longer or shorter delay in milliseconds.
        solenoidValve1_1.off();// 3. turns off
        Serial.println("solonoid 1 close");
    }
    else if(c == '5'){
        solenoidValve2_2.on(); // 1. turns on
        Serial.println("solonoid 2 open contains ec");
        delay(1000);       // 2. waits 500 milliseconds (0.5 sec). Change the value in the brackets (500) for a longer or shorter delay in milliseconds.
        solenoidValve2_2.off();// 3. turns off 
        Serial.println("solonoid 2 close");
    }
    else if(c == '6'){
        solenoidValve3_3.on(); // 1. turns on
        Serial.println("solonoid 3 open contains ph up ");
        delay(1000);       // 2. waits 500 milliseconds (0.5 sec). Change the value in the brackets (500) for a longer or shorter delay in milliseconds.
        solenoidValve3_3.off();// 3. turns off
        Serial.println("solonoid 3 close");
        
    }
    else if (c == '7'){
        // Adafruit PCF8523 Real Time Clock Assembled Breakout Board - Test Code
    //This will display the time and date of the RTC. see RTC.h for more functions such as rtcPCF.hour(), rtcPCF.month() etc.
        DateTime now = rtcPCF.now();
        
        Serial.print(now.day(), DEC);
        Serial.print('/');
        Serial.print(now.month(), DEC);
        Serial.print('/');
        
        Serial.print(now.year(), DEC);
        Serial.print("  ");
        Serial.print(now.hour(), DEC);
        Serial.print(':');
        Serial.print(now.minute(), DEC);
        Serial.print(':');
        Serial.print(now.second(), DEC);
        Serial.println();
        delay(1000);
    } 
   else if (c == '8'){
       for (int i = 0; i < 4; i++) { 
        digitalWrite(RelayModule4chPins[i],HIGH);
        Serial.print("Rele ");
        Serial.print(i);
        Serial.print(" is ON");
        Serial.println();
        delay(1000);
        digitalWrite(RelayModule4chPins[i],LOW);
        Serial.print("Rele ");
        Serial.print(i);
        Serial.print(" is OFF");
        Serial.println();
       }
   }
    
}

void lcdUpdate(float dhtHumidity,float dhtTempC,float phValue,float ecValue){
    lcd.setCursor(0, 0);  
    lcd.print(F("T:")); lcd.print(dhtTempC); lcd.print(F("C"));lcd.print(F("H:")); lcd.print(dhtHumidity); lcd.print(F("%"));
    lcd.setCursor(0 , 1 );
    lcd.print(F("pH:")); lcd.print(phValue);lcd.print(F(" EC: ")); lcd.print(ecValue);
       
}
float readPh(){
    float temperature = ds18b20wp.readTempC();  //read water temp
    float voltage = analogRead(PH_PIN)/1024.0*5000;  // read the voltage from pH sensor
    float phValue = ph.readPH(voltage,temperature);  //calculate pH value
    return phValue;
}
float readEc(){
    float temperature = ds18b20wp.readTempC();  //read water temp
    float voltageEC = analogRead(EC_PIN)/1024.0*5000; //read the voltage from EC sensor
    float ecValue    = ec.readEC(voltageEC,temperature); //calculate ec value
    return ecValue;
}
void adjustPh (float phValue,float PhMax,float PhMin){
    AlarmPH = 0;  // reset of the pH alarm
    float average;
    average = phValue;
    //branch if the ph value is greater than 6.5
    if(average >= PhMax){
        while (average >= PhMax)
        {
            AlarmPH = 1 ;
            lcd.begin(16,2);
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("pH is above 7.5 "); 
            lcd.setCursor(1,0);
            lcd.print ("We will add some acid");
            solenoidValve3_3.on(); //open the solonoid valve that contains the pH UP sol 
            Serial.println("solonoid 3 open");
            delay(36000);       //wait for standar time.may depent on how litre of water we have
            solenoidValve3_3.off();//close the solonoid valve that contains the pH UP sol 
            Serial.println("solonoid 3 close");
            digitalWrite(RelayModule4chPins[2],HIGH);
            Serial.print("now we mix the water");
            Serial.println();
            lcd.begin(16,2);
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("Water is mixing for 10min");
            delay(200000);//start the pump for 10min to mix the water
            digitalWrite(RelayModule4chPins[2],LOW);
            Serial.println("we wait for water to calm");
            delay(200000);
            
            for (int i= 0 ;i <=6;i++ ){
                average+=readPh();
                
                delay(500);
            }
            average=average/7;
            Serial.println(average);
            lcd.begin(16,2);
            lcd.clear();
            lcd.setCursor(0, 0);  
            lcd.print(F("pH:")); lcd.print(average);
            Serial.print("Ph value is  ");
            Serial.print(average);
        }
    }
    //branch if the ph value is lower than 4
    if(phValue <= PhMin){
        while (phValue <= PhMin)
        {
            
            AlarmPH = 1 ;
            lcd.begin(16,2);
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("pH is below 5.5 "); 
            lcd.setCursor(1,0);
            lcd.print ("We will add some base");
            solenoidValve1_1.on(); //open the solonoid valve that contains the pH DOWN sol 
            Serial.println("solonoid 1 open");
            delay(36000);       //wait for standar time.may depent on how litre of water we have
            solenoidValve1_1.off();//close the solonoid valve that contains the pH UP sol 
            Serial.println("solonoid 3 close");
            digitalWrite(RelayModule4chPins[2],HIGH);
            Serial.print("now we mix the water");
            Serial.println();
            lcd.begin(16,2);
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("Water is mixing for 10min");
            delay(200000);//start the pump for 10min to mix the water
            digitalWrite(RelayModule4chPins[2],LOW);
            Serial.println("we wait for water to calm");
            delay(200000);
            
            for (int i= 0 ;i <=6;i++ ){
                average+=readPh();
                
                delay(500);
            }
            average=average/7;
            Serial.println(average);
            lcd.begin(16,2);
            lcd.clear();
            lcd.setCursor(0, 0);  
            lcd.print(F("pH:")); lcd.print(average);
            Serial.print("Ph value is  ");
            Serial.print(average);
        }
    }
    
    
    
    
}
void adjustEc(float ecValue,float EcMax,float EcMin){
    AlarmEC = 0;  // reset of the pH alarm
    while (ecValue <= EcMin)
    {
        AlarmEC = 1 ;
        solenoidValve2_2.on(); //open the solonoid valve that contains the ec UP sol 
        Serial.println("solonoid 2 open");
        delay(1000);       //wait for standar time.may depent on how litre of water we have
        solenoidValve2_2.off();//close the solonoid valve that contains the ec UP sol 
        Serial.println("solonoid 2 close");
        delay(2000);
        ecValue = readEc();
    }
    /*while (ecValue =< EcMin)
    {
        AlarmEC = 1 ;
        solenoidValve2_2.on(); //open the solonoid valve that contains the ec UP sol 
        Serial.println("solonoid 2 open");
        delay(1000);       //wait for standar time.may depent on how litre of water we have
        solenoidValve2_2.off();//close the solonoid valve that contains the ec UP sol 
        Serial.println("solonoid 2 close");
        delay(2000);
        ecValue = readEc;
    }*/
    
}

void updateThingspeak(float phValue,float ecValue,float temperature,float dhtHumidity,float dhtTempC,int pumpFlagOn, int ledFlagOn){
    String myAPI = "BB61Y26J07Y1TAP0";   // API Key
    String myHOST = "api.thingspeak.com";
    String myPORT = "80";
    //Values to be send//
    String sendData = "GET /update?api_key="+ myAPI +"&"+ "field1" +"="+String(phValue)+
    "&"+ "field2" +"="+String(ecValue)+"&"+ "field3" +"="+String(temperature)+
    "&"+ "field4" +"="+String(dhtHumidity)+"&"+ "field5" +"="+String(dhtTempC)+
    "&"+ "field6" +"="+String(pumpFlagOn)+"&"+ "field7" +"="+String(ledFlagOn);
    //..................//
    espData("AT+CIPMUX=1", 1000, DEBUG);       //Allow multiple connections
    espData("AT+CIPSTART=0,\"TCP\",\""+ myHOST +"\","+ myPORT, 1000, DEBUG);
    espData("AT+CIPSEND=0," +String(sendData.length()+4),1000,DEBUG);  
    espSerial.find(">"); 
    espSerial.println(sendData);
    Serial.print("Value to be sent: pH:");
    Serial.print(phValue);
    Serial.print(" Ec: ");
    Serial.print(ecValue);
    Serial.print(" Water Temp:");    
    Serial.print(temperature);
    Serial.print(" Humidity:");
    Serial.print(dhtHumidity);
    Serial.print(" Room Temp:");
    Serial.print(dhtTempC);
    Serial.println();
    espData("AT+CIPCLOSE=0",1000,DEBUG);
    delay(10000);
    

    //to add light indecator and pump indecator , maybe and relaey indecator
}


String espData(String command, const int timeout, boolean debug)
{
  Serial.print("AT Command ==> ");
  Serial.print(command);
  Serial.println("     ");
  
  String response = "";
  espSerial.println(command);
  long int time = millis();
  while ( (time + timeout) > millis())
  {
    while (espSerial.available())
    {
      char c = espSerial.read();
      response += c;
    }
  }
  if (debug)
  {
    Serial.print(response);
  }
  return response;
}
