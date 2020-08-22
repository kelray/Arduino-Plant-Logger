#include "FS.h"
#include "SD.h"
#include "SPI.h"
#include <WiFi.h>
#include <PubSubClient.h>
#include <Arduino.h>
#include "ESP32_MailClient.h"
#include <OneWire.h>
#include <DallasTemperature.h>
#include "DHT.h"

/*
 * Connect the SD card to the following pins:
 *
 * SD Card | ESP32
 *    D2       -
 *    D3       SS
 *    CMD      MOSI
 *    VSS      GND
 *    VDD      3.3V
 *    CLK      SCK
 *    VSS      GND
 *    D0       MISO
 *    D1       -
 */
 
//WiFi Network information
const char* ssid = "SSID-NETWORK-NAME";
const char* password = "NETWORK-PASSWORD";

int i = 0;
char tmp[24];
char timeStamp[64];
int value = 0;
char str[20];

//Mail
const char smpt_server[] = "smtp.gmail.com";
const int smpt_port = 465;
const char userid[] = "MyGmailUserID"; // gmail userid 
const char pswd[] = "**************";// gmail password 
const char mailTo[] = "John@Smith.com";
const char mailFrom[] = "MyGmailUserID@gmail.com";
 
WiFiClient espClient;

//The Email Sending data object contains config and data to send
SMTPData smtpData;

//Callback function to get the Email sending status
void sendCallback(SendStatus info);

//Sensors - DHT11 - ambient temperature and humidity
// for DHT11, 
//      VCC: 5V or 3V
//      GND: GND
//      DATA: 27
#define DHTTYPE DHT11   // DHT 11
#define DHTPIN 27 
DHT dht(DHTPIN, DHTTYPE);

//DS18B20 - soil sensor
#define ONE_WIRE_BUS_1 26
OneWire oneWire_in(ONE_WIRE_BUS_1);
DallasTemperature ds18b20_soilSensor(&oneWire_in);
float SoilTemp = 0;

//Soil moisture sensor
int soilMoistpin = 34;
int soilMoistLevel = 0;

void listDir(fs::FS &fs, const char * dirname, uint8_t levels){
    Serial.printf("Listing directory: %s\n", dirname);

    File root = fs.open(dirname);
    if(!root){
        Serial.println("Failed to open directory");
        return;
    }
    if(!root.isDirectory()){
        Serial.println("Not a directory");
        return;
    }

    File file = root.openNextFile();
    while(file){
        if(file.isDirectory()){
            Serial.print("  DIR : ");
            Serial.println(file.name());
            if(levels){
                listDir(fs, file.name(), levels -1);
            }
        } else {
            Serial.print("  FILE: ");
            Serial.print(file.name());
            Serial.print("  SIZE: ");
            Serial.println(file.size());
        }
        file = root.openNextFile();
    }
}

void createDir(fs::FS &fs, const char * path){
    Serial.printf("Creating Dir: %s\n", path);
    if(fs.mkdir(path)){
        Serial.println("Dir created");
    } else {
        Serial.println("mkdir failed");
    }
}

void removeDir(fs::FS &fs, const char * path){
    Serial.printf("Removing Dir: %s\n", path);
    if(fs.rmdir(path)){
        Serial.println("Dir removed");
    } else {
        Serial.println("rmdir failed");
    }
}

void readFile(fs::FS &fs, const char * path){
    Serial.printf("Reading file: %s\n", path);

    File file = fs.open(path);
    if(!file){
        Serial.println("Failed to open file for reading");
        return;
    }

    Serial.print("Read from file: ");
    while(file.available()){
        Serial.write(file.read());
    }
    file.close();
}

void writeFile(fs::FS &fs, const char * path, const char * message){
    Serial.printf("Writing file: %s\n", path);

    File file = fs.open(path, FILE_WRITE);
    if(!file){
        Serial.println("Failed to open file for writing");
        return;
    }
    if(file.print(message)){
        Serial.println("File written");
    } else {
        Serial.println("Write failed");
    }
    file.close();
}

void appendFile(fs::FS &fs, const char * path, const char * message){
    Serial.printf("Appending to file: %s\n", path);

    File file = fs.open(path, FILE_APPEND);
    if(!file){
        Serial.println("Failed to open file for appending");
        return;
    }
    if(file.print(message)){
        Serial.println("Message appended");
    } else {
        Serial.println("Append failed");
    }
    file.close();
}

void renameFile(fs::FS &fs, const char * path1, const char * path2){
    Serial.printf("Renaming file %s to %s\n", path1, path2);
    if (fs.rename(path1, path2)) {
        Serial.println("File renamed");
    } else {
        Serial.println("Rename failed");
    }
}

void deleteFile(fs::FS &fs, const char * path){
    Serial.printf("Deleting file: %s\n", path);
    if(fs.remove(path)){
        Serial.println("File deleted");
    } else {
        Serial.println("Delete failed");
    }
}

void testFileIO(fs::FS &fs, const char * path){
    File file = fs.open(path);
    static uint8_t buf[512];
    size_t len = 0;
    uint32_t start = millis();
    uint32_t end = start;
    if(file){
        len = file.size();
        size_t flen = len;
        start = millis();
        while(len){
            size_t toRead = len;
            if(toRead > 512){
                toRead = 512;
            }
            file.read(buf, toRead);
            len -= toRead;
        }
        end = millis() - start;
        Serial.printf("%u bytes read for %u ms\n", flen, end);
        file.close();
    } else {
        Serial.println("Failed to open file for reading");
    }


    file = fs.open(path, FILE_WRITE);
    if(!file){
        Serial.println("Failed to open file for writing");
        return;
    }

    size_t i;
    start = millis();
    for(i=0; i<2048; i++){
        file.write(buf, 512);
    }
    end = millis() - start;
    Serial.printf("%u bytes written for %u ms\n", 2048 * 512, end);
    file.close();
}

void setup(){
    Serial.begin(115200);
    if(!SD.begin(2)){
        Serial.println("Card Mount Failed");
        return;
    }
    uint8_t cardType = SD.cardType();

    if(cardType == CARD_NONE){
        Serial.println("No SD card attached");
        return;
    }

    Serial.print("SD Card Type: ");
    if(cardType == CARD_MMC){
        Serial.println("MMC");
    } else if(cardType == CARD_SD){
        Serial.println("SDSC");
    } else if(cardType == CARD_SDHC){
        Serial.println("SDHC");
    } else {
        Serial.println("UNKNOWN");
    }

    uint64_t cardSize = SD.cardSize() / (1024 * 1024);
    Serial.printf("SD Card Size: %lluMB\n", cardSize);
    
    listDir(SD, "/", 0);
    createDir(SD, "/mydir");
    listDir(SD, "/", 0);
    removeDir(SD, "/mydir");
    listDir(SD, "/", 2);
        
    Serial.printf("Total space: %lluMB\n", SD.totalBytes() / (1024 * 1024));
    Serial.printf("Used space: %lluMB\n", SD.usedBytes() / (1024 * 1024));

     //Set up WiFi
     WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED)
    {
      Serial.print(".");
      delay(200);
    }
  
    Serial.println("");
    Serial.println("WiFi connected.");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
    Serial.println();

    //Set Clock
    //GMT offset (-5 Hrs), Daylight offset (0 Hrs)
    MailClient.Time.setClock(-5, 0);
    
    //Print out current date and time
    int d = MailClient.Time.getDay();
    int m = MailClient.Time.getMonth();
    int y = MailClient.Time.getYear();
    int hr = MailClient.Time.getHour();
    int min = MailClient.Time.getMin();
    int sec = MailClient.Time.getSec();
    sprintf(timeStamp, "/%d-%d-%d-%d-%d-%d.csv", d, m, y, hr, min, sec);
    Serial.print("Time stamp is:");Serial.println(timeStamp);
    readFile(SD, timeStamp);
    testFileIO(SD, timeStamp);
    writeFile(SD, timeStamp, "Ambient Temperature, ");
    appendFile(SD, timeStamp, "Ambient Humidity, ");
    appendFile(SD, timeStamp, "Soil Temperaure, ");
    appendFile(SD, timeStamp, "Soil Moisture,\n");

    dht.begin();
    ds18b20_soilSensor.begin();
}

void loop()
{
    // Reading temperature or humidity takes about 250 milliseconds!
    // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
    float humid_A = dht.readHumidity();
	
    // Read temperature as Celsius (the default)
    float tempC_A = dht.readTemperature();
	
    // Read temperature as Fahrenheit (isFahrenheit = true)
    //float tempF_A = dht.readTemperature(true);
    
   //Write ambient temperature to file
   sprintf(tmp, "%.2f,", tempC_A);
   appendFile(SD, timeStamp, tmp);
   Serial.println(tmp);
   memset(tmp, '\0', sizeof(tmp));

   //Write ambient humidity to file
   sprintf(tmp, "%.2f,", humid_A);
   appendFile(SD, timeStamp, tmp);
   Serial.println(tmp);
   memset(tmp, '\0', sizeof(tmp));

   //Write soil temperature to file
   ds18b20_soilSensor.requestTemperatures();
   SoilTemp = ds18b20_soilSensor.getTempCByIndex(0);
   sprintf(tmp, "%.2f,", SoilTemp);
   appendFile(SD, timeStamp, tmp);
   Serial.println(tmp);
   memset(tmp, '\0', sizeof(tmp));

   //Write soil moisture level to file
   soilMoistLevel = analogRead(soilMoistpin);
   sprintf(tmp, "%d,\n", soilMoistLevel);
   appendFile(SD, timeStamp, tmp);
   Serial.println(tmp);
   memset(tmp, '\0', sizeof(tmp));
   
   i++;
   
    if(i == 150)
    {
      Serial.println("Sending log now...");
      //Set the Email host, port, account and password
      smtpData.setLogin(smpt_server, smpt_port, mailFrom, pswd);
    
      //For library version 1.2.0 and later which STARTTLS protocol was supported,the STARTTLS will be 
      //enabled automatically when port 587 was used, or enable it manually using setSTARTTLS function.
      //smtpData.setSTARTTLS(true);
    
      //Set the sender name and Email
      smtpData.setSender("ESP32", mailFrom);
    
      //Set Email priority or importance High, Normal, Low or 1 to 5 (1 is highest)
      smtpData.setPriority("High");
    
      //Set the subject
      char mailSubject[128];
      sprintf(mailSubject, "ESP32 SMTP Mail Sending Test %s", timeStamp);
      smtpData.setSubject(mailSubject);
    
      //Set the message - normal text or html format
      smtpData.setMessage("<div style=\"color:#ff0000;font-size:20px;\">Hello World! - From ESP32</div>", true);
    
      //Add recipients, can add more than one recipient
      smtpData.addRecipient(mailTo);
    
      //Add attach files from SD card
      //Comment these two lines, if no SD card connected
      //Two files that previousely created.
      smtpData.addAttachFile(timeStamp);
     
      smtpData.setSendCallback(sendCallback);
      
      //Start sending Email, can be set callback function to track the status
      if (!MailClient.sendMail(smtpData))
      {
        Serial.println("Error sending Email, " + MailClient.smtpErrorReason());
      }
    
      //Clear all data from Email object to free memory
      smtpData.empty();

      //Get time stamp and create a new log file
      Serial.println("Creating new log file");
      deleteFile(SD, timeStamp);   //delete old log file
      int d = MailClient.Time.getDay();
      int m = MailClient.Time.getMonth();
      int y = MailClient.Time.getYear();
      int hr = MailClient.Time.getHour();
      int min = MailClient.Time.getMin();
      int sec = MailClient.Time.getSec();
      sprintf(timeStamp, "/%d-%d-%d-%d-%d-%d.csv", d, m, y, hr, min, sec);
      Serial.print("New time stamp is:");Serial.println(timeStamp);
      readFile(SD, timeStamp);
      writeFile(SD, timeStamp, "Ambient Temperature, ");
      appendFile(SD, timeStamp, "Ambient Humidity, ");
      appendFile(SD, timeStamp, "Soil Temperaure, ");
      appendFile(SD, timeStamp, "Soil Moisture,\n");   
      i = 0;
    }        
    //Sampling rate is 2 HZ.
    delay(2000);
}

//Callback function to get the Email sending status
void sendCallback(SendStatus msg)
{
  //Print the current status
  Serial.println(msg.info());

  //Do something when complete
  if (msg.success())
  {
    Serial.println("----------------");
  }
}
