//
//
// Sensorix
//
// Description of the project
// Developed with [embedXcode](http://embedXcode.weebly.com)
//
// Author 		DocSystems
// 				DocDevs
//
// Date			02/01/2018 21:28
// Version		0.0.3
//
// Copyright	© 2018
// Licence		licence
//
// See         ReadMe.txt for references
//


// Core library for code-sense - IDE-based
// !!! Help: http://bit.ly/2AdU7cu
#if defined(ENERGIA) // LaunchPad specific
#include "Energia.h"
#elif defined(TEENSYDUINO) // Teensy specific
#include "Arduino.h"
#elif defined(ESP8266) // ESP8266 specific
#include "Arduino.h"
#elif defined(ARDUINO) // Arduino 1.8 specific
#include "Arduino.h"
#else // error
#error Platform not defined
#endif // end IDE

#include "LiquidCrystal.h"
#include "OneWire.h"
#include "DallasTemperature.h"
#include "GPRS_Shield_Arduino.h"
#include "SoftwareSerial.h"
#include "Wire.h"
#define VT_PIN A0 // voltage read-out

//GSM
#define PIN_TX    7
#define PIN_RX    8
#define BAUDRATE  9600
#define PHONE_NUMBER "XXXXXXXXX"
#define MESSAGE_LENGTH 160

//flags:
bool tempMessageSent = false;
bool powerMessageSent = false;
bool hasPower = true;

//main configuration:
float tolerance1 = 13.0; //set minimum temperature here
float tolerance2 = 20.0; //not used
int delayTime = 5000; //delay of the main loop in miliseconds, 300k - 5 minutes
unsigned int powerDelay = 10000; // delay for the voltage checking sub-loop, 180k = 3 minutes
int defaultCycles = 360; //number of cycles
int resetCycles = defaultCycles;
float tempLog[30];

//sms handling:
char message[MESSAGE_LENGTH];
int messageIndex = 0;
char phone[13];
char datetime[24];

//messages
const String messageLowTemp = "TEMPERATURA ZA NISKA.";
const String messageNoPower = "BRAK ZASILANIA.";
const String messagePowerBack = "ZASILANIE WROCILO.";

//lcd
const int rs = 12, en = 11, d4 = 5, d5 = 4, d6 = 3, d7 = 2;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

//char msg = "";
// char messages[] = {"log", "status", "temp 25"};

GPRS gprsTest(PIN_TX,PIN_RX,BAUDRATE);//RX,TX,BaudRate

//Temperature sensor
#define ONE_WIRE_BUS 13

// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWire(ONE_WIRE_BUS);

// Pass our oneWire reference to Dallas Temperature.
DallasTemperature sensors(&oneWire);

// arrays to hold device address
DeviceAddress insideThermometer;

void setup() {
    
    //Temperature sensor
    //start serial port
    Serial.begin(9600);
    
    //locate devices on the bus
    sensors.begin();
    if (!sensors.getAddress(insideThermometer, 0)) Serial.println("Unable to find address for Device 0");
    
    // show the addresses we found on the bus
    
    // set the resolution to 9 bit (Each Dallas/Maxim device is capable of several different resolutions)
    sensors.setResolution(insideThermometer, 9);
    
    //lcd
    Serial.println("Setting up lcd...");
    lcd.begin(16, 2);
}

// Function used to obtain temperature reading from sensor.

float outputTemperature(DeviceAddress deviceAddress)
{
    float tempC = sensors.getTempC(deviceAddress);
    return tempC;
}

// Function used to obtain voltage reading from sensor.
float getVoltage() {
    int vt_read = analogRead(VT_PIN);
    float voltage = vt_read * (5.0 / 1024.0) * 5.0;
    return voltage;
}


// Functions used to print messages on LCD.

void printTemperature(float tempC) {
    Serial.println("Temp: ");
    Serial.print(tempC);
    lcd.setCursor(0, 0);
    lcd.print("Temp: ");
    lcd.print(tempC);
    lcd.print(" ");
    lcd.print((char)223);
    lcd.print("C");
}

void printNoPower() {
    lcd.setCursor(0, 1);
    lcd.print("BRAK ZASILANIA!");
}

void printPowerOk(){
    lcd.setCursor(0, 1);
    lcd.print("Zasilanie OK.  ");
}


// Functions used by GSM Module.

void powerUp()
// software equivalent of pressing the GSM shield "power" button
{
    if(!gprsTest.init() == true)
    {
        pinMode(9, OUTPUT);
        digitalWrite(9,LOW);
        delay(1000);
        digitalWrite(9,HIGH);
        delay(2000);
        digitalWrite(9,LOW);
        delay(3000);
    }
}

void sendMessage(String message)
{
    //GPS module
    while(!gprsTest.init()) {
        delay(powerDelay);
        Serial.print("init error\r\n");
    }
    Serial.println("gprs init success");
    Serial.println("sending message ...");
    char cMessage[50];
    message.toCharArray(cMessage, sizeof(cMessage));
    gprsTest.sendSMS(PHONE_NUMBER, cMessage); //define phone number and text
}

/*
 Functions for future use
 
 void executeCommand(char message)
 {
 
 }
 
 void getLog(void)
 {
 
 }
 void getStatus(void)
 {
 
 }
 void changeTemp1(float temp)
 {
 tolerance1 = temp;
 }
 void changeTemp2(float temp)
 {
 tolerance2 = temp;
 }
 void changeDelay(int dTime)
 {
 delayTime = dTime;
 }
 */

/*
 * Main function.
 * Requests the tempC from the sensors and display on Serial.
 * Checks voltage reading
 * Prints relevant info to LCD and sends Text messages using GSM.
 */

void loop(void)
{
    Serial.println(resetCycles);
    if (resetCycles == 0){
        resetCycles = defaultCycles;
        tempMessageSent = false;
    } else {
        resetCycles--;
    }
    
    //Turn on GPS shield if not turned on
    powerUp();
    
    // Voltage check sub-loop
    float voltage = getVoltage();
    Serial.println("Current voltage is: "  + String(voltage));
    if (voltage == 0)
    {
        Serial.println("No voltage detected. Waiting for delay.");
        printNoPower();
        delay(powerDelay);
        Serial.println("Delay finished");
        voltage = getVoltage();
        if (voltage == 0)
        {
            hasPower = false;
            Serial.println("Current voltage is: " + String(voltage));
            if(powerMessageSent == false){
                sendMessage(messageNoPower);
                powerMessageSent = true;
            }
        }
        else
        {
            hasPower = true;
        }
    }
    else
    {
        printPowerOk();
        if(powerMessageSent == true){
            Serial.println("Voltage is back");
            sendMessage(messagePowerBack);
            powerMessageSent = false;
        }
    }
    
    // Check received messages, delete messages
    messageIndex = gprsTest.isSMSunread();
    if (messageIndex > 0) { //At least, there is one UNREAD SMS
        gprsTest.readSMS(messageIndex, message, MESSAGE_LENGTH, phone, datetime);
        //In order not to full SIM Memory, is better to delete it
        gprsTest.deleteSMS(messageIndex);
        Serial.print("From number: ");
        Serial.println(phone);
        Serial.print("Recieved Message: ");
        Serial.println(message);
    }
    
    // Check temperature
    sensors.requestTemperatures();
    float tempC = outputTemperature(insideThermometer); // Use a simple function to print out the data
    
    Serial.println("Checking temperature.");
    Serial.println(tempC);
    Serial.println("Printing temperature...");
    printTemperature(tempC);
    
    if (tempC < tolerance1 && tempMessageSent == false)
    {
        sendMessage(messageLowTemp);
        tempMessageSent = true;
    }
    
    delay(delayTime);
}
