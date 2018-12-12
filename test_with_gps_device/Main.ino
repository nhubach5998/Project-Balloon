
#include <TinyGPS++.h>
#include <SD.h>
TinyGPSPlus gps; // create GPS instance
unsigned long SMS_Timer;
float Alt_Track[] = {0.0,0.0};
unsigned long sec;
byte month, day, hour, minutes, second, hundredths;
char Time[11];
char Location[19];
char Temp[6];
char Gyr[20];
char Other[25];
int count=0;
char* message[] = {Time," ",Location," ",Other," ",Temp," ",Gyr,"\n"};
                //   time Latitude Longtitude altitude      temp         x        y        z as degree from origin
File myFile;

static void ReadGPS();
static void ProcessInfo();
static void SendLocation();
static void IncreaseTime();


void setup(){
    Serial.begin(9600);     // debugging Terminal
    Serial1.begin(115200);  //Communication with GPS
    Serial2.begin(9600);    //Read GPS
    //Hello world
    delay(1000);
    if (!SD.begin(53)) {
    Serial.println("failed to open sd card"); 
    delay(100);
    while (1);
    }
    myFile = SD.open("data.txt", FILE_WRITE);
    SMS_Timer = millis()/1000;
    Serial1.println("AT+GPS=1");
    delay(200);
    Serial1.println("AT+GPSUPGRADE=1");//open GPS port
}

void loop(){
    //read accelerometer


    //read gps
    ReadGPS(200); // read GPS
    //read temp and humidity
//--->do sth   

    //combine data and write to SD card
    if(myFile){
      for(int i = 0 ; i < 9 ; i ++){
        myFile.print(message[i]);
        Serial.print(message[i]);
      }
      myFile.println();
    }else{
        Serial.println("Can't open file in SD card");
      }
    
    if(count++ > 500){
      myFile.close();
      count = 0;
      myFile = SD.open("data.txt", FILE_WRITE);
    }
    if(Serial.available()){
      switch(Serial.read()){
        case 's':
        SendLocation();
        break;
      }
    }
    //send gps location every 10min if altitude < 1km
    //or altitude has not changes much since the last 2 minutes
    //and SMS service is available (check by AT command for NETWORK)

    if ((millis() - Alt_Track[1] > 180000 || Alt_Track[0] < 1000.0) && (millis()/1000)-SMS_Timer >  36000){
      SendLocation();
    }else{
      //Serial.print("Time: ");Serial.println(millis());
    }
    delay(200);
}
static void ReadGPS(long ms){
  float FLat,FLong;
  unsigned long age; // old data? or new data? but not use
  long p1= millis();
  do{
  if(Serial2.available()){
    //byte a = Serial2.read();
    while(Serial2.available()) gps.encode(Serial2.read()); // read every character received from GPS to GPS instance
    //Serial.write(a);
  }else{
    IncreaseTime();
  }
  }while(millis()-p1<ms);
  sec = millis();
//  gps.f_get_position(&FLat,&FLong,&age); // get GPS info from GPS instance to variable
  Serial.print(FLat);Serial.print(FLong);Serial.println();
  ProcessInfo(FLat,FLong); // send data to char array
}
static void IncreaseTime(){
  unsigned long change = millis() - sec;
  change /= 10;
  hundredths += change;
  if(hundredths>=100){
    second += (int)(hundredths/100);
    hundredths %= 100;
    if(second >=60){
      minutes += (int)(second/60);
      second %= 60;
      if(minutes >=60){
      hour += (int)(minutes/60);
      minutes %= 60;
      }
    }
  }
  sec = millis();
  sprintf(Time,"%02d:%02d:%02d.%02d",hour-5, minutes, second, hundredths);
}
static void ProcessInfo(float Lat, float Long){
  sprintf(Location,"%.6f %.6f",Lat,Long);
  
  //code from library to get date and time
  int year;
  unsigned long fix_age;
//  gps.crack_datetime(&year, &month, &day,
//  &hour, &minutes, &second, &hundredths, &fix_age);
  sprintf(Time,"%02d:%02d:%02d.%02d",gps.time.hour()-5, gps.time.minute(), gps.time.second(), gps.time.centisecond());

  float alt = gps.
  float Course = gps.f_course();
  float mps = gps.f_speed_mps();
  // heading to (degree) at speed of (mps) at height of (altitude)
  sprintf(Other,"%.2f %.2f %.2f",Course,mps,alt);
  if(abs(alt-Alt_Track[0])>100){
    Alt_Track[1]= millis();
    Alt_Track[0]= alt;
  }
}
static void SendLocation(){
  Serial1.println("AT+CMGF=1");
  delay(200);
  Serial1.print("AT+CMGS=\"+16147475298\"\r");
  delay(200);
  for(int i = 0;i<10; i++){
      Serial1.print(message[i]);
    }
  delay(200);
  Serial1.print((char)26);
  SMS_Timer = millis();
}
