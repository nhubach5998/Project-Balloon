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

//functions
void ReadGPS();
void WriteToSD();
void checkSMSValid();
void SendLocation(); // send time + location + altitude
void Airplane_Mode(bool on);
void Process_Command();
void EnableGPS();

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
        Data_File = SD.open("GPSDATA.txt", FILE_WRITE);
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
    ReadGPS(250);
    //write to SD card 50 lines and save
    WriteToSD();

    //2 flags:
    //flag_1 : altitude valid for transmisting -> airplane mode off
    //flag_2 : CREG = 5 or = 1
    //send location every 10 min when 2 flag meets.
    if(millis()/1000 - SMS_Timer > 600 && Flag_1 && Flag_2)
        SendLocation();

    //DEBUG message + command
    Process_Command();

}
void Process_Command(){
    if(Serial.available()>0){
        int i = 0;
        while(Serial.available()){
            a = Serial.read();
            if(a == 10){
                Serial1.println(str);
                str = "";
            }else{
                if (a == 33){
                    SendLocation();
                }else{
                    str += (char)a;
                }
            }
        }
    }
    ReadGPS(250);
    if(Serial1.available)
}
void SendLocation(){
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
    if(count>=49){
        Data_File.close();
        count = 0;
        Data_File = SD.open("GPSDATA.txt", FILE_WRITE);
    }else{
        count++;
    }
}
void readGPS(int ms){
    long start = millis();
    do{
        while(Serial2.available()){
            GPS.encode(Serial2.read());
        }
        sprintf(Time,"%2d:%2d:%2d.%2d ",gps.time.hour(),gps.time.minute(),gps.time.second(),gps.time.centisecond());
        if(GPS.location.isUpdated()){
            Location[0]=GPS.location.lat();
            Location[1]=GPS.location.lng();
        }
        Altitude = GPS.altitude.meters();
        Course = GPS.course.deg();
        Speed_mps = GPS.speed.mps();
    }while(millis()-start < ms);
}
void EnableGPS(){
    String str="";
    bool stop = false;
    do{
        Serial1.print("ATE0");
        Serial1.flush();
        Serial1.print("AT");
        delay(100);
        if(ReadGSM("OK")){
            stop=true;
        }
    }while(!stop);
    Serial1.print("AT+GPS=1");
    delay(100);
    Serial.print("AT+GPSUPGRADE=1");
    delay(100);
    while(GPS.charsProcessed()<10)  ReadGPS(200);
    Serial2.print("$PMTK220,1000*1F"); //frequency data feed is 1 sec
    Serial2.print("$PMTK314,0,1,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0*28"); // only get RMC and GGA
    Serial2.print("$PGCMD,33,0*6D"); // no attena status
}
bool ReadGSM(String str){
    String tmp="";
    bool flag = false;
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
    Serial.print(tmp);
    if(tmp == str){
        return true;
    }else{
        return false;
    }
}
void checkSMSValid(){
    readGPS(700);
    if(GPS.altitude.meters()<1000){
        Flag_1 = true;
    }
    Serial1.flush();
    Serial1.print("AT+CREG?");
    if(ReadGSM("+CREG: 1,1")){
        Flag_2=true;
    }
}

