//Libraries
#include <TinyGPS++.h>
#include <SD.h>

//constant
const int SD_CS = 53;
//variables
char Time[12];
float Location[2], Altitude, Speed_mps;
double Course;
float Temperature;
char Gyroscope[20];
unsigned long SMS_Timer;
bool Flag_1 = false, //Altitude check
     Flag_2 = false; //SMS function check
TinyGPSPlus GPS;
File Data_File;
int count;
bool Airplane = false;

//functions
static void GetGPSData(unsigned long ms);
void WriteToSD();
void checkSMSValid();
void SendLocation(); // send time + location + altitude
void Airplane_Mode(bool on);
void Process_Command();
void EnableGPS();
void Clear_Buffer();

//setup Arduino
void setup(){
    //turn DEBUG serial port
    //turn GPS port (Serial2)
    //turn GSM port for SMS (Serial1)
    Serial.begin(9600);     // debugging Terminal
    Serial1.begin(115200);  //Communication with GPS
    Serial2.begin(9600);    //Read GPS
    
    //Open SD card to file called "GPSDATA.txt"
    if (!SD.begin(SD_CS)) {
    Serial.println("Failed to open sd card... Please try again!");
    delay(100);
    while(1);
    }else{
        Serial.println("Initialized SD card...");
    }

    EnableGPS();        //Enable GPS
    checkSMSValid();    //check SMS function
    Serial.println("System ready to go");
    SMS_Timer = millis()/1000;
    count = 0;
}

//loop
void loop(){

    //Read GPS for 400ms + process data
    GetGPSData(250);
     //write to sd card
//    Serial.print("Lat: ");
//    Serial.print(Location[0],6);
//    Serial.println("");
//    Serial.print("Long: ");
//    Serial.print(Location[1],6);
//    Serial.println("");
    for(int i=0;i<12;i++)
    Serial.print(Time[i]);
    WriteToSD();
    //Serial.println(millis()/1000-SMS_Timer);
    //2 flags:
    //flag_1 : altitude valid for transmisting -> airplane mode off
    //flag_2 : CREG = 5 or = 1
    //send location every 10 min when 2 flag meets.
    if(millis()/1000 - SMS_Timer > 500){
        checkSMSValid();
        if(Flag_1 && Flag_2)
            SendLocation();
    }
    
    //DEBUG message + command
    Process_Command();

}
void Process_Command(){
  String str="";
    if(Serial.available()>0){
        int i = 0;
        while(Serial.available()){
           int a = Serial.read();
            if(a == 10){
                Serial1.println(str);
                Serial.println(str);
            }else{
                if (a == 33){
                    SendLocation();
                }else{
                    str += (char)a;
                }
            }
        }
    }
    GetGPSData(250);
    if(Serial1.available()){
      while(Serial1.available()){
        Serial.write(Serial1.read());
      }
      Serial.print("\n");
    }
}
void SendLocation(){
    Serial.println("Sending location...");
    Serial1.println("AT+CMGF=1");
    delay(500); 

    Serial1.print("AT+CMGS=\"+16147475298\"\r");
    for(int i = 0;i<sizeof(Time);i++)
        Serial1.print(Time[i]);
    Serial1.print(Location[0],6);
    Serial1.print(" ");
    Serial1.print(Location[1],6);
    Serial1.print(" ");
    Serial1.print(Altitude,2);
    delay(200);

    Serial1.println((char)26);// ASCII code of CTRL+Z
    delay(500);
    SMS_Timer = millis()/1000;
}
void WriteToSD(){
    // Time Location Altitude Speed Course Temp Gyr x 6
    Data_File = SD.open("GPSDATA.txt",FILE_WRITE);
    for(int i = 0; i<sizeof(Time);i++)
        Data_File.print(Time[i]);
    Data_File.print(Location[0],6);
    Data_File.print(" ");
    Data_File.print(Location[1],6);
    Data_File.print(" ");
    Data_File.print(Altitude,2);
    Data_File.print(" ");
    Data_File.print(Speed_mps,2);
    Data_File.print(" ");
    Data_File.print(Course);
    Data_File.print(" ");
    Data_File.print(Temperature,2);
    Data_File.print(" ");
    for(int i = 0; i<sizeof(Gyroscope);i++)
        Data_File.print(Gyroscope[i]);
    Data_File.println();
    Data_File.close();
}
static void GetGPSData(unsigned long ms){
    unsigned long start = millis();
    do{
        while(Serial2.available()){
          //Serial.print(".");
          GPS.encode(Serial2.read());
        }
        sprintf(Time,"%02d:%02d:%02d",((GPS.time.hour()-5<0)?(GPS.time.hour()+19):(GPS.time.hour()-5)),GPS.time.minute(),GPS.time.second());
        if(GPS.location.isUpdated()){
            Location[0]=GPS.location.lat();
            Location[1]=GPS.location.lng();
            //Serial.println("Get new location:");
        }
        Altitude = GPS.altitude.meters();
        Course = GPS.course.deg();
        Speed_mps = GPS.speed.mps();
    }while(millis()-start < ms);
}
void EnableGPS(){
    Serial.println("Enabling GPS...");
    String str="";
    bool stop = false;
    do{
        Serial.println("Turn off echo...");
        Serial1.println("ATE0");
        delay(100);
        Clear_Buffer();
        Serial1.println("AT");
        delay(100);
        Serial.println("Test AT OK...");
        if(ReadGSM("OK")){
            stop=true;
        }
    }while(!stop);
    stop = false;
    Serial.println("Turn on GPS...");
    while(!stop){
      Serial1.println("AT+GPS=1");
      if(ReadGSM("OK")){
        stop = true;
      }
    }
    delay(100);
    stop = false;
    Serial.println("Push GPS to UART2...");
    while(!stop){
      Serial1.println("AT+GPSUPGRADE=1");
      if(ReadGSM("OK")){
        stop = true;
      }
    }
    delay(100);
    while(GPS.charsProcessed()<10)  GetGPSData(200);
    Serial.println("GPS configure settings...");
    Serial2.print("$PMTK220,1000*1F"); //frequency data feed is 1 sec
    Serial2.print("$PMTK314,0,1,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0*28"); // only get RMC and GGA
    Serial2.print("$PGCMD,33,0*6D"); // no attena status
}
bool ReadGSM(String str){
    String tmp="";
    bool flag = false;
    Serial.println("Checking response...");
    delay(1000);
    if(Serial1.available()){
        while(Serial1.available()){
            int a = Serial1.read();
            if(a == 13){
                flag = false;
            }else{
                if(a == 10){
                    flag= true;
                }else{
                    if(flag==true){
                        tmp += (char)a;
                    }
                }
            }
        }
    }
    Serial.println();
    Serial.print("String collected from GSM: "); Serial.println(tmp);
    if(tmp == str){
        return true;
    }else{
        return false;
    }
}
void Airplane_Mode(bool c){
    if(!Airplane && c){
        Serial1.println("AT+CREG=0");
        Serial1.println("AT+CFUN=0");
        Clear_Buffer();
        Airplane = true;
    }else{
        if(!c && Airplane){
            Serial1.println("AT+CREG=1");
            Serial1.println("AT+CFUN=1");
            Clear_Buffer();
            Airplane = false;
        }

    }
}
void checkSMSValid(){
    //Serial.println("Check for GSM ok...");
    //GetGPSData(250);
    if(Altitude<1000){
        if(!Flag_1){
            Airplane_Mode(false);
            Flag_1 = true;
            Serial.println("Altitude ok for SMS...");
        }
    }else{
        if(Flag_1){
            Airplane_Mode(true);
            Flag_1 = false;
            Serial.println("Altitude NOT ok for SMS..."); 
        }
    }

    if(!Flag_2 && Flag_1 || Flag_2){
      //Clear_Buffer();
      Serial1.println("AT+CREG?");
      if(ReadGSM("+CREG: 1,1OK")){
          Flag_2=true;
          Serial.println("Network registered...");
      }else{
        Flag_2 = false;
        Serial.println("no network connected... looking for one...");
      }
    }
}
void Clear_Buffer(){
    Serial.println();
    if(Serial1.available()){
        while(Serial1.available()){
            Serial.write(Serial1.read());
        }
    }else{
    }
    Serial.println();
}
