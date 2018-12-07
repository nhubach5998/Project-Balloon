#include <TinyGPS.h>
TinyGPS gps; // create GPS instance
unsigned long SMS_Timer;
float Alt_Track[] = {0.0,0.0}
unsigned long sec;
byte month, day, hour, minutes, second, hundredths;
char Time[11];
char Location[19];
char Temp[6];
char Gyr[20];
char Other[25];
char* message[] = {Time," ",Location," ",Other," ",Temp," ",Gyr,"\n"};
                //   time          Latitude         Longtitude       altitude      temp         x        y        z as degree from origin
void setup(){
    Serial.begin(9600);
    Serial1.begin(115200);
    Serial2.begin(9600);
    //Hello world
    SMS_Timer = millis()/1000;
}

void loop(){
    //read accelerometer
    
    //read gps
    ReadGPS(); // read GPS
    for(int i = 0;i<10; i++){
      Serial.print(message[i]);
    }
    //send gps location every 10min if altitude < 1km
    //or altitude has not changes much since the last 2 minutes
    //and SMS service is available (check by AT command for NETWORK)
    if ((millis()/1000)-SMS_Timer >  36000 && millis() - Alt_Track[1] > 120000 || Alt_Track[0] < 1000.0 ){
//--->/ send SMS
    }
    //read temp and humidity
    
    //combine data and write to SD card
    delay(1000);
}
void ReadGPS(){
  float FLat,FLong;
  unsigned long age; // old data? or new data? but not use
  if(Serial2.available()){
    while(Serial2.available())  gps.encode(Serial2.read()); // read every character received from GPS to GPS instance
    sec = millis();
    gps.f_get_position(&FLat,&FLong,&age); // get GPS info from GPS instance to variable
    ProcessInfo(FLat,FLong); // send data to char array
    
  }else{
    Serial.println("not available");
    IncreaseTime();
  }
}
void IncreaseTime(){
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
void ProcessInfo(float Lat, float Long){
  sprintf(Location,"%.6f %.6f",Lat,Long);
  
  //code from library to get date and time
  int year;
  unsigned long fix_age;
  gps.crack_datetime(&year, &month, &day,
  &hour, &minutes, &second, &hundredths, &fix_age);
  sprintf(Time,"%02d:%02d:%02d.%02d",hour-5, minutes, second, hundredths);

  float alt = gps.f_altitude();
  float Course = gps.f_course();
  float mps = gps.f_speed_mps();
  // heading to (degree) at speed of (mps) at height of (altitude)
  sprintf(Other,"%.2f %.2f %.2f",Course,mps,alt);
  if(abs(alt-Alt_Track[0])>100){
    Alt_Track[1]= millis();
    Alt_Track[0]= alt;
  }
}
void GPSSMS(String &text){
  for(int i = 0;i<sizeof(Location);i++){
    text +=Location[i];
  }
}
