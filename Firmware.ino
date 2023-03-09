//Libraries
#include "Wire.h"
#include "RTClib.h"
#include <ESP8266_Lib.h> 
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
#include <SoftwareSerial.h>


SoftwareSerial espSerial(22, 24);//Pin 24 and 22 act as RX and TX. Connect them to TX and RX of ESP8266      
// Pin Definitions
#define PH_PIN A1
#define EC_PIN A2
#define DHT_PIN_DATA	9 //DIGITAL TEMPERATURE AND HUMIDITY SENSOR
#define DS18B20WP_PIN_DQ	8 //DIGITAL WATER TEMPERATURE SENSOR
#define LCD_PIN_RS	53//LCD 
#define LCD_PIN_E	51//
#define LCD_PIN_DB4 43//
#define LCD_PIN_DB5	45//
#define LCD_PIN_DB6	47//
#define LCD_PIN_DB7	49//LCD
#define SOLENOIDVALVE1_1_PIN_COIL1	2//PERISTALTIC PUMP FOR ACID
#define SOLENOIDVALVE2_2_PIN_COIL1	3//BASE
#define SOLENOIDVALVE3_3_PIN_COIL1	4//PERISTALTIC PUMP FOR MICRO 1 NUTRITIONS
#define SOLENOIDVALVE4_4_PIN_COIL1	5//PERISTALTIC PUMP FOR MICRO 2 NUTRITIONS
#define SOLENOIDVALVE5_5_PIN_COIL1	6//PERISTALTIC PUMP FOR MICRO 3 NUTRITIONS
#define Rele_1 10//NO USE
#define Rele_2 12//RELE PINOUT FOR THE LED
#define Rele_3 11//
#define Rele_4 13//RELE PINOUT FOR THE PUMP
#define DEBUG true
//Wi-Fi SET UP
String mySSID = "COSMOTE-t9kcg4";       // WiFi SSID
String myPWD = "2381024532"; // WiFi Password
//thingspeak parameters
String myAPI = "BB61Y26J07Y1TAP0";   // API Key
String myHOST = "api.thingspeak.com";
String myPORT = "80";
String myFIELD = "field1"; 
float sendPh; // TEMP VAR FOR PH 
float sendEc;// TEMP VAR FOR EC
float sendWaterTemp; // TEMP VAR FOR WATER TEMP
float sendRoomTemp; // TEMP VAR FOR ROOM TEMP
float sendHumid; // TEMP VAR FOR HUMIDITY 

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
int AlarmPH;         // alarm for Ph levels
int AlarmTempWater;   // alarm for Temp levels
int AlarmEC;         // alarm for Ec levels
// define vars for testing menu
unsigned long previousMillis = 0;
unsigned long currentMillis = millis();
unsigned long period = 600000;       //define timeout of 10 min.....1000 = 1 sec \\\ 600.000 = 10 min
float PhMax= 6;       // Max value for pH
float PhMin= 5;       // Min value for pH
float EcMax= 1.5;   // Max value for EC
float EcMin= 1;    // Min value for EC
float WaterTempMax= 24;    // Max value for Water Temp
float WaterTempMin= 18;    // Min value for Water Temp
long time0;
float phAverage = 0; //the average value of ph 
float ecAverage = 0; //the average value of ec
int timeStampOn = 6;//set the hour that the leds will be turned on in 24h format 
int timeStampOff = 19;//set the hour that the leds will be turned off in 24h format  
int TimeFlag=0;
int ledFlagOn = 0; //flag that point if the leds are on or off
int timeStampPumpOn = 0;//set the minute that the pump will be turned on every hour  
int timeStampPumpOff = 30;//set the minute that the pump will be turned off every hour
int pumpFlagOn=0; //flag that point if the pump is on or off



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
    espData("AT+RST", 1000, DEBUG); //Reset the ESP8266 module
    espData("AT+CWMODE=1", 1000, DEBUG);//Set the ESP mode as station mode
    espData("AT+CWJAP=\""+ mySSID +"\",\""+ myPWD +"\"", 1000, DEBUG); //Connect to WiFi network
    
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
    solenoidValve1_1.off();
    DateTime now = rtcPCF.now();
    /*DateTime dt75 = now + TimeSpan(0, 0, 10, 0);
    if (now == dt75 ){
        Serial.print("d");
    }*/
    //Serial print for the current date/time
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
        digitalWrite(RelayModule4chPins[3],LOW);
        Serial.print("the pump is on for the next 30 min ");
        Serial.println();
        pumpFlagOn = 1;
        delay(1000);
        lcd.begin(16,2);
        lcd.clear();
        lcd.setCursor(0, 0); 
    }
     
    else if(now.minute()>=timeStampPumpOff  && pumpFlagOn == 1){
        digitalWrite(RelayModule4chPins[3],HIGH);
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
    if(now.hour()>=timeStampOn && now.hour()<timeStampOff){
        digitalWrite(RelayModule4chPins[1],LOW);
        ledFlagOn = 1;
        Serial.print("Led is on until 7pm");
        Serial.println();        
    }
    ///////////////////////////////////////////////////
    //From 18:00 to 6:00 the next day the LED are OFF//
    ///////////////////////////////////////////////////
    //(now.hour()>=timeStampOff || now.hour()<timeStampOn) && ledFlagOn == 1 
     else if((now.hour()>=timeStampOff || now.hour()<timeStampOn) && ledFlagOn == 1 ){     
        digitalWrite(RelayModule4chPins[1],HIGH);
        ledFlagOn = 0;
        Serial.print("Led is off until 6am");
        Serial.println();
       
        
    }
    float temperature=ds18b20wp.readTempC();//read water temp
    float dhtHumidity = dht.readHumidity(); //read room humidity sensor
    float dhtTempC = dht.readTempC();   //read room temp sensor
    phValue = readPh() ; //calculate pH value
    ecValue = readEc();//calculate ec value
    ///////////////////////////////////////////////////////
    //function thst operates the LCD data display refresh//
    ///////////////////////////////////////////////////////
    lcdUpdate(dhtHumidity,dhtTempC,phValue,ecValue);
    ///////////////////////////////////////////////
    //function that update the date on Thingspeak//
    ///////////////////////////////////////////////
    updateThingspeak(phValue,ecValue,temperature,dhtHumidity,dhtTempC,pumpFlagOn,ledFlagOn);
    ////////////////////////////////
    //Code to adjust the pH values//
    ////////////////////////////////
    if(pumpFlagOn==0){ //checks if the pump is off,so the measurment are more accurate
        //checks if 10 minutes have past since the pump went off. 
        //This gives time so the water can settle for more accurate measurment
        if(now.minute() == 40){
            for (int i= 0 ;i <=6;i++ ){ //we take the average value of the ph sensor
                phAverage+=readPh();    //every 2 seconds
                Serial.println(phAverage);
                delay(2000);
            }
            phAverage=phAverage/7; //calculates the average value
            Serial.println(phAverage);
            //checks if the average value of ph is on desired levels, if true exits the if
            if(phAverage>=PhMax || phAverage <= PhMin){
                //if false calls a function to adjust the ph value
                adjustPh(phAverage,PhMax,PhMin);
            }
        }
    }
    ////////////////////////////////
    //Code to adjust the Ec values//
    ////////////////////////////////
    if(pumpFlagOn==0){ //checks if the pump is off,so the measurment are more accurate
        //checks if 10 minutes have past since the pump went off. 
        //This gives time so the water can settle for more accurate measurment
        if(now.minute() == 40){
            for (int i= 0 ;i <=6;i++ ){ //we take the average value of the ph sensor
                ecAverage+=readEc();    //every 2 seconds
                Serial.println(ecAverage);
                delay(2000);
            }
            ecAverage=ecAverage/7; //calculates the average value
            Serial.println(ecAverage);
            //checks if the average value of ph is on desired levels, if true exits the if
            if(ecAverage>=EcMax || ecAverage <= EcMin){
                //if false calls a function to adjust the ph value
                adjustEc(ecAverage,EcMax,EcMin);
            }
        }
    }
   


    //
    
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
        delay(10000);       // 2. waits 500 milliseconds (0.5 sec). Change the value in the brackets (500) for a longer or shorter delay in milliseconds.
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
        delay(30000);       // 2. waits 500 milliseconds (0.5 sec). Change the value in the brackets (500) for a longer or shorter delay in milliseconds.
        solenoidValve3_3.off();// 3. turns off
        Serial.println("solonoid 3 close");
        
    }
    else if(c == '7'){
        digitalWrite(RelayModule4chPins[3],HIGH);
        Serial.print("the pump is off");
        Serial.println();
        
    }
    else if(c == '8'){
        digitalWrite(RelayModule4chPins[3],LOW);
        Serial.print("the pump is on");
        Serial.println();
        
    }
    
    else if(c == '9'){
        solenoidValve1_1.on(); // 1. turns on
        Serial.println("solonoid 1 open contains ph up ");
        delay(10000);       // 2. waits 500 milliseconds (0.5 sec). Change the value in the brackets (500) for a longer or shorter delay in milliseconds.
        solenoidValve1_1.off();// 3. turns off
        Serial.println("solonoid 1 close");

        solenoidValve2_2.on(); // 1. turns on
        Serial.println("solonoid 2 open contains ph up ");
        delay(10000);       // 2. waits 500 milliseconds (0.5 sec). Change the value in the brackets (500) for a longer or shorter delay in milliseconds.
        solenoidValve2_2.off();// 3. turns off
        Serial.println("solonoid 2 close");

        solenoidValve3_3.on(); // 1. turns on
        Serial.println("solonoid 3 open contains ph up ");
        delay(10000);       // 2. waits 500 milliseconds (0.5 sec). Change the value in the brackets (500) for a longer or shorter delay in milliseconds.
        solenoidValve3_3.off();// 3. turns off
        Serial.println("solonoid 3 close");

        solenoidValve4_4.on(); // 1. turns on
        Serial.println("solonoid 4 open contains ph up ");
        delay(10000);       // 2. waits 500 milliseconds (0.5 sec). Change the value in the brackets (500) for a longer or shorter delay in milliseconds.
        solenoidValve4_4.off();// 3. turns off
        Serial.println("solonoid 4 close");
        
        solenoidValve5_5.on(); // 1. turns on
        Serial.println("solonoid 5 open contains ph up ");
        delay(10000);       // 2. waits 500 milliseconds (0.5 sec). Change the value in the brackets (500) for a longer or shorter delay in milliseconds.
        solenoidValve5_5.off();// 3. turns off
        Serial.println("solonoid 5 close");
        
    }
    /*
    else if(c == '5'){
        digitalWrite(RelayModule4chPins[1],LOW);
        Serial.print("the pump is off");
        Serial.println();
        
    }*/
    
    
   
    
}
//A function that updates the values displayed in the lcd
void lcdUpdate(float dhtHumidity,float dhtTempC,float phValue,float ecValue){
    lcd.setCursor(0, 0);  // Sets the cursor on the upper left corner of the LCD
    lcd.print(F("T:")); lcd.print(dhtTempC); lcd.print(F("C"));lcd.print(F("H:")); lcd.print(dhtHumidity); lcd.print(F("%"));
    lcd.setCursor(0 , 1 );// Sets the cursor on the lower left corner of the LCD
    lcd.print(F("pH:")); lcd.print(phValue);lcd.print(F(" EC: ")); lcd.print(ecValue);
       
}
//A function that calculates and return the pH value of our solution
float readPh(){
    float temperature = ds18b20wp.readTempC();  //read water temp
    float voltage = analogRead(PH_PIN)/1024.0*5000;  // read the voltage from pH sensor
    float phValue = ph.readPH(voltage,temperature);  //calculate pH value
    return phValue;
}
//A function that calculates and return the EC value of our solution
float readEc(){
    float temperature = ds18b20wp.readTempC();  //read water temp
    float voltageEC = analogRead(EC_PIN)/1024.0*5000; //read the voltage from EC sensor
    float ecValue    = ec.readEC(voltageEC,temperature); //calculate ec value
    return ecValue;
}
//A function that adjust the pH value of our solution
void adjustPh (float phValue,float PhMax,float PhMin){
    AlarmPH = 0;  // reset of the pH alarm
    float average;
    average = phValue;
    //branch if the ph value is greater than 7
    if(average >= PhMax){
        while (average >= PhMax)
        {
            AlarmPH = 1 ; //resets the alarm flag
            //prints on LCD
            lcd.begin(16,2);
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("pH is above 6.5 "); 
            lcd.setCursor(1,0);
            lcd.print ("We will add some acid");
            if((average-PhMax)>=1){
                solenoidValve3_3.on(); //open the peristaltic pump that contains the pH UP solution 
                Serial.println("pH up peristaltic pump open");
                delay(30000); //wait for standar time.may depent on how many lt of water we have
                solenoidValve3_3.off();//close the peristaltic pump that contains the pH UP sol 
                Serial.println("pH up peristaltic pump closed");
            }
            else
            {
                solenoidValve3_3.on(); //open the peristaltic pump that contains the pH UP solution 
                Serial.println("pH up peristaltic pump open");
                delay(10000); //wait for standar time.may depent on how many lt of water we have
                solenoidValve3_3.off();//close the peristaltic pump that contains the pH UP sol 
                Serial.println("pH up peristaltic pump closed");
            }                                
            digitalWrite(RelayModule4chPins[3],HIGH);//opens the pump so the water can mix
            Serial.print("now we mix the water");
            Serial.println();
            lcd.begin(16,2);
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("Water is mixing for 10min");
            delay(600000);//start the pump for 10min to mix the water//1m = 60.000 ms
            digitalWrite(RelayModule4chPins[3],LOW);//turns the pump off
            Serial.println("we wait for water to calm");
            delay(300000);
            //reads and calculate the average ph value so we can check again if we are on desired levels
            for (int i= 0 ;i <=6;i++ ){
                average+=readPh();                
                delay(2000);
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
            AlarmPH = 1 ;//resets the alarm flag
            //prints on LCD
            lcd.begin(16,2);
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("pH is below 5.5 "); 
            lcd.setCursor(1,0);
            lcd.print ("We will add some base");
            if((PhMin - average)>=1){
                solenoidValve4_4.on(); //open the peristaltic pump that contains the pH DOWN sol 
                Serial.println("pH down peristaltic pump open");
                delay(30000); //wait for standar time.may depent on how many lt of water we have
                solenoidValve4_4.off();//close the peristaltic pump that contains the pH UP sol 
                Serial.println("pH up peristaltic pump closed");
            }   
            else{
                solenoidValve4_4.on(); //open the peristaltic pump that contains the pH DOWN sol 
                Serial.println("pH down peristaltic pump open");
                delay(10000); //wait for standar time.may depent on how many lt of water we have
                solenoidValve4_4.off();//close the peristaltic pump that contains the pH UP sol 
                Serial.println("pH up peristaltic pump closed");
            }
            digitalWrite(RelayModule4chPins[3],HIGH);//opens the pump so the water can mix
            Serial.print("now we mix the water");
            Serial.println();
            lcd.begin(16,2);
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("Water is mixing for 10min");
            delay(600000);//start the pump for 10min to mix the water
            digitalWrite(RelayModule4chPins[3],LOW);//turns the pump off
            Serial.println("we wait for water to calm");
            delay(300000);
            //reads and calculate the average ph value so we can check again if we are on desired levels
            for (int i= 0 ;i <=6;i++ ){
                average+=readPh();                
                delay(2000);
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

//A function that adjust the EC value of our solution
void adjustEc(float ecValue,float EcMax,float EcMin){
    AlarmEC = 0;  // reset of the ec alarm
    float average;
    average = ecValue;
    //branch if the ec value is greater than ....
    if(average >= EcMax){
        while (average >= EcMax)
        {
            AlarmEC = 1 ; //resets the alarm flag
            //prints on LCD
            lcd.begin(16,2);
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("ec is above ... "); 
            lcd.setCursor(1,0);
            lcd.print ("We will add some ...");
            solenoidValve3_3.on(); //open the peristaltic pump that contains the ... solution 
            Serial.println("ec up peristaltic pump open");
            delay(36000); //wait for standar time.may depent on how many lt of water we have
            solenoidValve3_3.off();//close the peristaltic pump that contains the ... solution 
            Serial.println("ec up peristaltic pump closed");
            digitalWrite(RelayModule4chPins[3],HIGH);//opens the pump so the water can mix
            Serial.print("now we mix the water");
            Serial.println();
            lcd.begin(16,2);
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("Water is mixing for 10min");
            delay(200000);//start the pump for 10min to mix the water
            digitalWrite(RelayModule4chPins[3],LOW);//turns the pump off
            Serial.println("we wait for water to calm");
            delay(200000);
            //reads and calculate the average ph value so we can check again if we are on desired levels
            for (int i= 0 ;i <=6;i++ ){
                average+=readEc();                
                delay(2000);
            }
            average=average/7;
            Serial.println(average);
            lcd.begin(16,2);
            lcd.clear();
            lcd.setCursor(0, 0);  
            lcd.print(F("ec:")); lcd.print(average);
            Serial.print("ec value is  ");
            Serial.print(average);
        }
    }
    //branch if the ec value is lower than ...
    if(ecValue <= EcMin){
        while (ecValue <= EcMin)
        {            
            AlarmEC = 1 ;//resets the alarm flag
            //prints on LCD
            lcd.begin(16,2);
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("ec is below ... "); 
            lcd.setCursor(1,0);
            lcd.print ("We will add some ...");
            solenoidValve1_1.on(); //open the peristaltic pump that contains the ... solution 
            Serial.println("ec down peristaltic pump open");
            delay(36000); //wait for standar time.may depent on how many lt of water we have
            solenoidValve1_1.off();//close the peristaltic pump that contains the ... solution 
            Serial.println("ec up peristaltic pump closed");
            digitalWrite(RelayModule4chPins[3],HIGH);//opens the pump so the water can mix
            Serial.print("now we mix the water");
            Serial.println();
            lcd.begin(16,2);
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("Water is mixing for 10min");
            delay(200000);//start the pump for 10min to mix the water
            digitalWrite(RelayModule4chPins[3],LOW);//turns the pump off
            Serial.println("we wait for water to calm");
            delay(200000);
            //reads and calculate the average ph value so we can check again if we are on desired levels
            for (int i= 0 ;i <=6;i++ ){
                average+=readEc();                
                delay(2000);
            }
            average=average/7;
            Serial.println(average);
            lcd.begin(16,2);
            lcd.clear();
            lcd.setCursor(0, 0);  
            lcd.print(F("ec:")); lcd.print(average);
            Serial.print("ec value is  ");
            Serial.print(average);
        }
    }
}
//A function that updates certain values on our database 
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
    espData("AT+CIPSTART=0,\"TCP\",\""+ myHOST +"\","+ myPORT, 1000, DEBUG);//starts a TCP connection.
    espData("AT+CIPSEND=0," +String(sendData.length()+4),1000,DEBUG); //command is used to send the data over the TCP connection.
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
    espData("AT+CIPCLOSE=0",1000,DEBUG);//At command closes the TCP connection. It can be configured for slow close or quick close.When there are multi-IP conneciton, a connection number is also required.
    delay(5000);
    

    //to add light indecator and pump indecator , maybe and relaey indecator
}

//A function that operates the esp module
String espData(String command, const int timeout, boolean debug)
{
  Serial.print("AT Command ==> ");
  Serial.print(command);
  Serial.println("     ");
  
  String response = "";//creates a blank value to store the response fron esp module
  espSerial.println(command);
  long int time = millis();
  while ( (time + timeout) > millis())
  {
    while (espSerial.available())//waits for the response from esp module
    {
      char c = espSerial.read();//stores the respone 
      response += c;
    }
  }
  if (debug)
  {
    Serial.print(response);//prints the response 
  }
  return response;
}
