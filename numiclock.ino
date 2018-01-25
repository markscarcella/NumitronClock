// clock stuff
#include <SPI.h>
const int  cs=8; //chip select 

//Pin connected to latch pin (ST_CP) of 74HC595
const int latchPin = 2;
//Pin connected to clock pin (SH_CP) of 74HC595
const int clockPin = 3;
////Pin connected to Data in (DS) of 74HC595
const int dataPin = 4;

byte digit[11]   = {B11110110, B11000000, B01101110, B11101010, B11011000, B10111010, B10111110, B11100000, B11111111, B11111000};

// button stuffs
int setButton = 5;
int upButton = 6;
int dnButton = 7;
static boolean setMode;
int timePos = 0;

void setup() {
  Serial.begin(9600);
  // start cock and set time
  RTC_init();
  //day(1-31), month(1-12), year(0-99), hour  (0-23), minute(0-59), second(0-59)
  //SetTime(22,24,03); 
  
  // set numi pins
  pinMode(latchPin, OUTPUT);
  pinMode(dataPin, OUTPUT);  
  pinMode(clockPin, OUTPUT);
  pinMode(setButton, INPUT);
  pinMode(upButton, INPUT);
  pinMode(dnButton, INPUT);

  setMode = 0;
}

void loop() {
  int duration = pressTime(setButton);
  if (!setMode && duration > 3000) {
    Serial.println("set mode");
    setMode = 1;
    flash();
  }
  if (setMode) {
    if (!digitalRead(setButton)) {
      changeTime();
    }
  }
  else {
    writeTime(ReadTime());
    Serial.print(ReadTime());
    Serial.println();
    delay(1000);
  }
}
//=====================================
int RTC_init(){ 
	  pinMode(cs,OUTPUT); // chip select
	  // start the SPI library:
	  SPI.begin();
	  SPI.setBitOrder(MSBFIRST); 
	  SPI.setDataMode(SPI_MODE3); // both mode 1 & 3 should work 
	  //set control register 
	  digitalWrite(cs, LOW);  
	  SPI.transfer(0x8E);
	  SPI.transfer(0x60); //60= disable Osciallator and Battery SQ wave @1hz, temp compensation, Alarms disabled
	  digitalWrite(cs, HIGH);
	  delay(10);
}
//=====================================
int SetTime(int h, int m, int s){ 
	int TimeDate [3]={s,m,h};
	for(int i=0; i<=2;i++){
		int b= TimeDate[i]/10;
		int a= TimeDate[i]-b*10;
		if(i==2){
			if (b==2)
				b=B00000010;
			else if (b==1)
				b=B00000001;
		}	
		TimeDate[i]= a+(b<<4);

		digitalWrite(cs, LOW);
		SPI.transfer(i+0x80); 
		SPI.transfer(TimeDate[i]);        
		digitalWrite(cs, HIGH);
  }
}
//=====================================

String ReadTime(){
  int Time [3]; //second,minute,hour		
  for(int i=0; i<3; i++){
    digitalWrite(cs, LOW);
    SPI.transfer(i+0x00); 
    unsigned int n = SPI.transfer(0x00);        
    digitalWrite(cs, HIGH);
    int a=n & B00001111;    
    if(i==2){	
      int b=(n & B00110000)>>4; //24 hour mode
      if(b==B00000010)
        b=20;        
      else if(b==B00000001)
	b=10;
    Time[i]=a+b;
    }
    else{	
      int b=(n & B01110000)>>4;
      Time[i]=a+b*10;	
    }
  }
  String timeString;
  for (int i=2; i>-1; i--) {
    if (Time[i] < 10) timeString.concat("0");
      timeString.concat(Time[i]); 
  }
  return(timeString); 
}


void writeTime(String timeString) {
  for (int i=5; i>-1; i--) {
    int n = timeString.charAt(i)-'0';
    digitalWrite(latchPin, LOW);
    shiftOut(dataPin, clockPin, MSBFIRST, digit[n]);
    digitalWrite(latchPin, HIGH);
  }   
}


long pressTime(int button) {
  static unsigned long start = 0; 
  static boolean state; 
  if(digitalRead(button) != state)
  {
    state = ! state;
    start = millis();
  }
  if(state == HIGH)
    return millis() - start;
  else
    return 0;
}

void shiftTime(int n) {
  digitalWrite(latchPin, LOW);
  shiftOut(dataPin, clockPin, MSBFIRST, digit[n]);
  digitalWrite(latchPin, HIGH);
}

void flash() {
  String crntTime = ReadTime();
  if (timePos == 0) {
    for (int i=5;i>1;i--) {
      shiftTime(crntTime.charAt(i)-'0');
    }
    shiftTime(10);
    shiftTime(10);
  }
  else if (timePos == 1) {
    for (int i=5;i>3;i--) {
      shiftTime(crntTime.charAt(i)-'0');
    }
    shiftTime(10);
    shiftTime(10);
    for (int i=1;i>-1;i--) {
      shiftTime(crntTime.charAt(i)-'0');
    }
  }
  else if (timePos == 2) {
    shiftTime(10);
    shiftTime(10);
    for (int i=3;i>-1;i--) {
      shiftTime(crntTime.charAt(i)-'0');
    }
  }
  delay(50);
  writeTime(ReadTime());
}

void changeTime() {
  delay(200);
  flash();
  String crntTime = ReadTime();
  int h = (crntTime.charAt(0)-'0')*10+crntTime.charAt(1)-'0';
  int m = (crntTime.charAt(2)-'0')*10+crntTime.charAt(3)-'0';
  int s = (crntTime.charAt(4)-'0')*10+crntTime.charAt(5)-'0';
  int timeFields[3] = {h,m,s};
  
  if (digitalRead(upButton)) {
    if (timePos == 0) {
      h = (h+1)%24;
    }
    else if (timePos == 1) {
      m = (m+1)%60;
    }
    else if (timePos == 2) {
      s = 0;
    }
    SetTime(h,m,s);
    writeTime(ReadTime());
    Serial.println("UP!");
    delay(200);
  }
  if (digitalRead(dnButton)) {
    if (timePos == 0) {
      h = (h-1)%24;
    }
    else if (timePos == 1) {
      m = (m-1)%60;
    }
    else if (timePos == 2) {
      s = 0;
    }
    SetTime(h,m,s);
    writeTime(ReadTime());
    Serial.println("DN!");
    delay(200);
  } 
  if (digitalRead(setButton)) {
    timePos += 1;
    delay(200);
    if (timePos > 2) {
      setMode = 0;
      timePos = 0;
      Serial.println("time mode");
    }
    else {
      Serial.println("set next");
    }
  }
}
