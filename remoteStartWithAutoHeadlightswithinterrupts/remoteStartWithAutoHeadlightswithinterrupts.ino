#include <EnableInterrupt.h>
/*Pins D10-13 - reserved for RFID
Pin 2 - RPM in
pin 3 - RFID interrupt
Pin 4 - Hi light
pin 5 - lo light
pin 6- starter relay
pin 7 - acc
pin 8 - on
pin 9 - RF out/in
pins 10-13 SPI RFID
A1, A2 - SoftwareSerial HC-12
A3 - Lo Light override
A4- High light interrupt
A5 - clutch input
Pin A6 - Brightness input*/

int RPMpin = 2;
int hiBeampin = 4;
int loBeampin = 5;
int starterPin = 6;
int accPin = 7;
int onPin = 8;
int rfPin = 9;
int brightnessPin = A6;
unsigned int reportCounter;
bool firstBeam;
unsigned long beamCounter;
unsigned long buttonCounter;


long RPM;
long RPMold;
unsigned int trigCounter;
unsigned long times;
unsigned long curTime;
unsigned long timeOld;

bool onState;
bool hiLightState;
bool loLightState;
bool starterState;
bool accState;
bool starterReq;
bool engineRunning;
bool firstStart;
unsigned int brightnessValue;
char serialCmd;
byte buttonCount;
byte beamByte;

void setup() {
  // put your setup code here, to run once:
  //Begin Serial comms
  Serial.begin(115200);

  //Set I/O
  pinMode(hiBeampin, OUTPUT);
  pinMode(loBeampin, OUTPUT);
  pinMode(starterPin, OUTPUT);
  pinMode(accPin, OUTPUT);
  pinMode(onPin, OUTPUT);
  pinMode(rfPin, INPUT);
  pinMode(RPMpin, INPUT);
  pinMode(brightnessPin, INPUT);

    Serial.println("Box Ready");
   
    //Initialize variables
  onState = false;
  hiLightState = false;
  loLightState = false;
  starterState = false;
  accState = false;
  //starterReq = false;
  engineRunning = false;
  firstBeam = true;
  reportCounter = 0;
  buttonCounter = 0;
  serialCmd = 's';

  RPM = 0;
  RPMold = 0;
  curTime = 0;
  times = 0;
  timeOld = 0;
  trigCounter = 0;
  beamCounter = 0;
  beamByte = 0;
  
   //Attach RPM input interrupt
    enableInterrupt(RPMpin, RPMcounter, RISING);
    enableInterrupt(rfPin, buttonToChar, RISING);
}

void loop() {
  //Local variables
  bool startFlag = false,
  
  //Poll al inputs
  curTime = millis();
  brightnessValue = analogRead(brightnessPin);

  //Check for serial
  serialCheck();

  //switch for starting algorithm
  ignitionInterpreter();

  //Set running flag if RPM < 10
 if (RPM < 10){
    engineRunning = false;
  }
  else if (RPM > 800){
    engineRunning = true;
  }
  if (millis() - timeOld > 1000){
        RPM = 0;
        engineRunning = false;
  }
  
  //check automatic headlights thresholds and do the same
  beamCheck();
  
  //Write Light States
  digitalWrite(loBeampin, loLightState);
  digitalWrite(hiBeampin, hiLightState);

  //Report all states every 2000 counts
  reportCounter++;
  if (reportCounter > 4000){
    serialReport();
    reportCounter = 0;
  }

  //enable button interrupt if last press was more than 50ms ago
  if ((millis() - buttonCounter) > 50){
    enableInterrupt(rfPin, buttonToChar, RISING);
  }

}

void RPMcounter(){
  //Serial.println("*****INTERRUPT***");
  times = curTime-timeOld;        //finds the time 
  RPM = (30108/times);         //calculates rpm
    //RPM = RPM*2;
  if (((RPM - RPMold) > 2000) || (RPM < 2)){
    RPM = RPMold;
  }
  timeOld = millis();
  if ((RPM > 100) && (RPM < 7000)){
    RPMold = RPM;
  }
}

void serialCheck(){
  if (Serial.available()){
    int tempChar = Serial.read();
    if (isAlpha(tempChar)){
      serialCmd = tempChar;
    }
   // Serial.print("State changed, now serialCmd = ");Serial.println(serialCmd);
  }
}

void ignitionInterpreter(){
  switch (serialCmd){
    case 'r':
      firstStart = true;
      if (starterReq && !engineRunning){
        bool startOk = true; 
        unsigned long timeForTimeout = millis(); // cranking timeout
        while((RPM < 200) && (serialCmd == 'r')){          
          //Serial.println("STARTING");
          digitalWrite(onPin, HIGH); //activates ON relay
          digitalWrite(accPin, LOW); // deactivates Acc relay
          
          if (firstStart){ // this variable should be true on first start
            unsigned long startDelay = millis(); // creates a delay from switching power to cranking
            while (millis() - startDelay < 500){} // waits for .5 sec
            firstStart = false; //makes this not happen while starting
          }// end if
          
          digitalWrite(starterPin, HIGH); // begin cranking
          if ((millis() - timeForTimeout) > 2500){ // times out cranking after x ms
            serialCmd = 'o';
          }
        }// end while
        
        if (RPM > 400){
          engineRunning = true;
        }
       }
        digitalWrite(onPin, HIGH);
        digitalWrite(accPin, HIGH);
        digitalWrite(starterPin, LOW);
        onState = true;
        accState = true;
        starterState = false;
        serialCmd = 'o';
      // ***End R case (r stands for run)
      break;
      
     case 's':
      digitalWrite(onPin, LOW);
      digitalWrite(accPin, LOW);
      digitalWrite(starterPin, LOW);
      onState = false;
      accState = false;
      starterState = false;
      engineRunning = false;
      RPM = 0;
      break; //end S case (s stands for stop)
      
     case 'a':
      digitalWrite(onPin, LOW);
      digitalWrite(accPin, HIGH);
      digitalWrite(starterPin, LOW);
      onState = false;
      accState = true;
      starterState = false;
      engineRunning = false;
      break; //end A case (a stands for accesorry)
      
     case 'o':
      digitalWrite(onPin, HIGH);
      digitalWrite(accPin, HIGH);
      digitalWrite(starterPin, LOW);
      onState = true;
      accState = true;
      starterState = false;
      break;// end o case (o stands for on)
      
     default:
      digitalWrite(onPin, LOW);
      digitalWrite(accPin, LOW);
      digitalWrite(starterPin, LOW);
      onState = false;
      accState = false;
      starterState = false;
      engineRunning = false;
      break;
  }
}

void beamCheck(){
  if ((brightnessValue > 600) && (brightnessValue < 800)){
    if (firstBeam || (beamByte != 1)){
      beamCounter = millis();
      firstBeam = false;
      beamByte = 1;
    }
    if (millis() - beamCounter > 5000){
      loLightState = HIGH;
      hiLightState = LOW;
      firstBeam = true;
    }
  }
  else if (brightnessValue > 800){
    if (firstBeam || (beamByte != 2)){
      beamCounter = millis();
      firstBeam = false;
      beamByte = 2;
    }
    if (millis() - beamCounter > 5000){
      loLightState = LOW;
      hiLightState = HIGH;
      firstBeam = true;
    }
  }
  else{
    if (firstBeam || (beamByte != 0)){
      beamCounter = millis();
      firstBeam = false;
      beamByte = 0;
    }
    if (millis() - beamCounter > 5000){
      loLightState = LOW;
      hiLightState = LOW;
      firstBeam = true;
    }
  }
}

void serialReport(){
    Serial.print("AccState = ");Serial.println(accState);
    Serial.print("OnState = ");Serial.println(onState);
    Serial.print("StartState = ");Serial.println(starterState);
    Serial.print("EngineRunning = ");Serial.println(engineRunning);
    Serial.print("Hi Lights = ");Serial.println(hiLightState);
    Serial.print("Lo Lights = ");Serial.println(loLightState);
    Serial.print("brightnessValue = ");Serial.println(brightnessValue);
    Serial.print("starterReq = ");Serial.println(starterReq);
    Serial.print("RPM = ");Serial.println(RPM);
    Serial.print("serialCmd = ");Serial.println(serialCmd);
    Serial.println("");
}


void buttonToChar(){
  buttonCount++;
    //switch for button interface
  switch (buttonCount){
    case 0:
      serialCmd = 's';
      buttonCounter = millis();
      disableInterrupt(rfPin);
      break;
    case 1:
      serialCmd = 'a';
      buttonCounter = millis();
      disableInterrupt(rfPin);
      break;
     case 2:
      serialCmd = 'o';
      buttonCounter = millis();
      disableInterrupt(rfPin);
      break;
     case 3:
      serialCmd = 'r';
      buttonCounter = millis();
      disableInterrupt(rfPin);
      break;
     case 4:
      buttonCount = 0;
      serialCmd = 's';
      buttonCounter = millis();
      disableInterrupt(rfPin);
      break;
     default:
      break;
  }
}
