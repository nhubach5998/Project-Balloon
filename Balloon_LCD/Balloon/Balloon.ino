//Libraries
#include <TinyGPS++.h>
#include <SD.h>
#include <Keypad.h>
#include <LiquidCrystal.h>

//constant
const int SD_CS = 53;
const byte ROWS = 4; //four rows
const byte COLS = 4; //four columns
//variables
char Time[12];
float Location[2], Altitude, Speed_mps;
double Course;
float Temperature;
char Gyroscope[20];
unsigned long SMS_Timer;
bool Flag_1 = false, //Altitude check
     Flag_2 = false; //SMS function check
int count;
bool Airplane = false;

char keys[ROWS][COLS] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};

byte rowPins[ROWS] = {9, 8, 7, 6}; //connect to the row pinouts of the keypad
byte colPins[COLS] = {5, 4, 3, 2}; //connect to the column pinouts of the keypad


TinyGPSPlus GPS;
File Data_File;
LiquidCrystal lcd(27, 26, 25, 24, 23, 22);
Keypad keypad = Keypad( makeKeymap(keys), rowPins, colPins, ROWS, COLS );


//functions
static void GetGPSData(unsigned long ms);
void WriteToSD();
void checkSMSValid();
void SendLocation(); // send time + location + altitude
void Airplane_Mode(bool on);
void Process_Command();
void EnableGPS();
void Clear_Buffer();
void Process_Command_LCD();
void LCD_printat(String msg,int x, int y);

//setup Arduino
void setup(){
    //turn DEBUG serial port
    //turn GPS port (Serial2)
    //turn GSM port for SMS (Serial1)
    //Serial.begin(9600);     // debugging Terminal
    Serial1.begin(115200);  //Communication with GPS
    Serial2.begin(9600);    //Read GPS
    lcd.begin(16,2);
    //Open SD card to file called "GPSDATA.txt"
    if (!SD.begin(SD_CS)) {
    //Serial.println("Failed to open sd card... Please try again!");
    lcd.print("SD CARD FAILED!");
    delay(100);
    while(1);
    }else{
        //Serial.println("Initialized SD card...");
        lcd.print("SD CARD SUCCESS!");
    }

    EnableGPS();        //Enable GPS
    checkSMSValid();    //check SMS function
    //Serial.println("System ready to go");
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
    //Process_Command();
    Process_Command_LCD();

}
void Process_Command_LCD(){
    char key = keypad.getKey();
    switch(key){
        case '1':
        SendLocation();
        break;
        case '2':
        Airplane_Mode(true);
        break;
        case '3':
        Airplane_Mode(false);
        break;
        case '4':
        //check network
        LCD_printat("Check GSM: ",0,1);
        Serial1.println("AT+CREG?");
        if(ReadGSM("+CREG: 1,1OK")){
            LCD_printat("OK!",11,1);
        }else{
            LCD_printat("N/A",11,1);
        }
        break;
        case 'C':
        lcd.clear();
        break;
        case '*':
        //scroll r
        break;
        case '#':
        break;
        default:
        break;
    }

    LCD_printat(Flag_1==1?"1":"0",0,0);
    LCD_printat(Flag_2==1?"1":"0",2,0);
    lcd.setCursor(4,0);
    for(int i=0;i<5;i++) lcd.print(Time[i]);
    lcd.setCursor(13,0);
    lcd.print(500-(millis()/1000-SMS_Timer));
    GetGPSData(250);
    if(Serial1.available()){
        lcd.setCursor(0,1);
        while(Serial1.available()){
            int a = Serial1.read();
            if(a == 13 || a == 10){
                lcd.print(" ");
            }else{
                lcd.print((char)a);
            }
            //Serial.write(Serial1.read());
        }
      //Serial.print("\n");
    }
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
void LCD_printat(String msg, int x,int y){
    lcd.setCursor(x,y);
    lcd.print(msg);
}
void SendLocation(){
    //Serial.println("Sending location...");
    Serial1.println("AT+CMGF=1");
    LCD_printat("SENDING LOCATION",0,1);
    Serial1.print("AT+CMGS=\"+16147475298\"\r");
    for(int i = 0;i<sizeof(Time);i++)
        Serial1.print(Time[i]);
    Serial1.print(Location[0],6);
    Serial1.print(" ");
    Serial1.print(Location[1],6);
    Serial1.print(" ");
    Serial1.print(Altitude,2);
    GetGPSData(100);
    Serial1.println((char)26);// ASCII code of CTRL+Z
    GetGPSData(100);
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
    //Serial.println("Enabling GPS...");
    LCD_printat("Enabling GPS...",0,0);
    String str="";
    bool stop = false;
    do{
        //Serial.println("Turn off echo...");
        LCD_printat("Turn off echo...",0,0);
        Serial1.println("ATE0");
        delay(100);
        Clear_Buffer();
        Serial1.println("AT");
        delay(100);
        //Serial.println("Test AT OK...");
        LCD_printat("AT OK               ",0,0);
        if(ReadGSM("OK")){
            stop=true;
        }
    }while(!stop);
    stop = false;
    //Serial.println("Turn on GPS...");
    LCD_printat("TURN ON GPS....",0,0);
    while(!stop){
      Serial1.println("AT+GPS=1");
      if(ReadGSM("OK")){
        stop = true;
      }
    }
    delay(100);
    stop = false;
    //Serial.println("Push GPS to UART2...");
    //LCD_printat("TURN ON GPS....",0,0);
    while(!stop){
      Serial1.println("AT+GPSUPGRADE=1");
      if(ReadGSM("OK")){
        stop = true;
      }
    }
    delay(100);
    while(GPS.charsProcessed()<10)  GetGPSData(200);
    //Serial.println("GPS configure settings...");
    Serial2.print("$PMTK220,1000*1F"); //frequency data feed is 1 sec
    Serial2.print("$PMTK314,0,1,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0*28"); // only get RMC and GGA
    Serial2.print("$PGCMD,33,0*6D"); // no attena status
}
bool ReadGSM(String str){
    String tmp="";
    bool flag = false;
    //Serial.println("Checking response...");
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
    //Serial.println();
    //Serial.print("String collected from GSM: "); Serial.println(tmp);
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
    LCD_printat("CHECK VALIDITY..",0,0);
    //GetGPSData(250);
    if(Altitude<1000){
        if(!Flag_1){
            Airplane_Mode(false);
            Flag_1 = true;
            //Serial.println("Altitude ok for SMS...");
            LCD_printat("Altitude OK",0,0);
        }
    }else{
        if(Flag_1){
            Airplane_Mode(true);
            Flag_1 = false;
            //Serial.println("Altitude NOT ok for SMS...");
        }
    }

    if(!Flag_2 && Flag_1 || Flag_2){
      //Clear_Buffer();
      Serial1.println("AT+CREG?");
      if(ReadGSM("+CREG: 1,1OK")){
          Flag_2=true;
          //Serial.println("Network registered...");
          LCD_printat("GSM Network OK!",0,1);
      }else{
        Flag_2 = false;
        //Serial.println("no network connected... looking for one...");
        LCD_printat("NO GSM Network!",0,1);
      }
    }
}
void Clear_Buffer(){
    //Serial.println();
    lcd.setCursor(0,1);
    if(Serial1.available()){
        while(Serial1.available()){
            //Serial.write(Serial1.read());
            lcd.print((char)Serial1.read());
        }
    }else{
    }
    //Serial.println();
}
