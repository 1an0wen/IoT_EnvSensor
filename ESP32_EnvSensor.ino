//setup comms
#include <WiFi.h>
#include <ESPmDNS.h>
#include <WiFiClient.h>
//#include <WiFiServer.h>
#include <ArduinoOTA.h>
#include <ESP8266FtpServer.h>

//setup filing system
#include <SPIFFS.h>
#include <FS.h>

//setup sensors
#include <UBX_Parser.h>
#include <HardwareSerial.h>
#include <DHT.h>

//--- DEVICE VARIABLES ---
//config wifi
//const char* ssid = "**********"; 
//const char* password = "**********";

//config http post
const char site[] = "**********"; //setup website root
const int port = 80;

//define software version
const char softVer[] = "2.5.5";

//define ArduinoOTA parameters
const char* OTAHost = "**********"; //setup over the air programming host
const char* OTAPass = "**********"; //setup over the air programming password

//configure the number of readings collected before transmission
const int stringloop = 10;
int publoop = 1;

//GPS UBX Configuration strings
//Enabled UBX: POSLLH, POSECEF, SOL and TIMEUTC
byte POSLLH_[] = {0xB5,0x62,0x06,0x01,0x03,0x00,0x01,0x02,0x01,0x0E,0x47};
byte POSECEF_[] = {0xB5,0x62,0x06,0x01,0x03,0x00,0x01,0x01,0x01,0x0D,0x45};
byte SOL_[] = {0xB5,0x62,0x06,0x01,0x03,0x00,0x01,0x06,0x01,0x12,0x4F};
byte TIMEUTC_[] = {0xB5,0x62,0x06,0x01,0x03,0x00,0x01,0x21,0x01,0x2D,0x85};
byte SAVE_CONFIG_[] = {0xB5,0x62,0x06,0x09,0x0D,0x00,0x58,0x2D,0xC7,0x06,0x03,0x00,0x00,0x00,0x68,0x2D,0xC7,0x06,0x17,0xEA,0x65};
//Other messages: For future reference 
//byte DISABLE_NMEA_[] = {0xB5,0x62,0x06,0x01,0x03,0x00,0xF0,0x00,0x00,0xFA,0x0F,0xB5,0x62,0x06,0x01,0x03,0x00,0xF0,0x01,0x00,0xFB,0x11,0xB5,0x62,0x06,0x01,0x03,0x00,0xF0,0x02,0x00,0xFC,0x13,0xB5,0x62,0x06,0x01,0x03,0x00,0xF0,0x03,0x00,0xFD,0x15,0xB5,0x62,0x06,0x01,0x03,0x00,0xF0,0x04,0x00,0xFE,0x17,0xB5,0x62,0x06,0x01,0x03,0x00,0xF0,0x05,0x00,0xFF,0x19};
//byte SET_REFRESH_[] = {0xB5,0x62,0x06,0x08,0x06,0x00,0x64,0x00,0x01,0x00,0x01,0x00,0x7A,0x12,0xB5,0x62,0x06,0x08,0x00,0x00,0x0E,0x30};
//byte SET_BAUD_[] = {0xB5,0x62,0x06,0x00,0x14,0x00,0x01,0x00,0x00,0x00,0xD0,0x08,0x00,0x00,0x00,0xC2,0x01,0x00,0x01,0x00,0x01,0x00,0x00,0x00,0x00,0x00,0xB8,0x42,0xB5,0x62,0x06,0x00,0x01,0x00,0x01,0x08,0x22};

//device variables
float humidity, temp;
unsigned char gpsFix;
double latitude;
double longitude;
double alt;
uint32_t gpsSats;
String gpsFT;
uint16_t gpsYear;
uint8_t gpsMonth;
uint8_t gpsDay;
uint8_t gpsHour;
uint8_t gpsMin;
uint8_t gpsSec;
String gpsDate;
String deviceID;
String senA;
String senH;
String senT;
String laT;
String lnG;
String alT;
String upT;
int i;
byte mac[6];

//HTTP Post variables
String response = "";
char cn;
int repSize;
String serSata;
String repLena;
String serverTime;
String repDBStatus;
String repIP;
int serSatLoc;
int serSat;
int serLenLocA;
int serLenLocB;

//define UBX_Parser data structure
class MyParser : public UBX_Parser {
    void handle_NAV_POSLLH(unsigned long iTOW, 
        long lon, 
        long lat, 
        long height, 
        long hMSL, 
        unsigned long hAcc, 
        unsigned long vAcc) {
          latitude = lat/1e7;
          longitude = lon/1e7;
          alt = hMSL/1000;
        }  
      void handle_NAV_TIMEUTC(unsigned long iTOW,
        unsigned long tAcc,
        long nano,
        unsigned short year,
        unsigned char month,
        unsigned char day,
        unsigned char hour,
        unsigned char min,
        unsigned char sec){
          gpsYear = year;
          gpsMonth = month;
          gpsDay = day;
          gpsHour = hour;
          gpsMin = min;
          gpsSec = sec;
        }
      void handle_NAV_SOL(unsigned long iTOW, 
        long fTOW, 
        short week, 
        unsigned char gpsFix,
        char flags,
        long ecefX,
        long ecefY,
        long ecefZ,
        unsigned long pAcc,
        long ecefVX,
        long ecefVY,
        long ecefVZ,
        unsigned long sAcc,
        unsigned short pDOP,
        unsigned char numSV) {
         switch (gpsFix) {
            case 0x00:
              gpsFT = "No Fix";
            break;
            case 0x01:
              gpsFT = "Dead Reckoning";
            break;
            case 0x02:
              gpsFT = "2D-fix";
            break;
            case 0x03:
              gpsFT = "3D-fix";
            break;
            case 0x04:
              gpsFT = "GPS & DR";
            break;
            case 0x05:
              gpsFT = "Time only";
            break;      
            default:
              gpsFT = "Unknown"; 
         }
         gpsSats = numSV;
      }
};

//--- SETUP DEVICE ---
//setup GPS hardware serial port
HardwareSerial SerialGPS(2);

//setup FTP server
FtpServer ftpSrv;

//setup DHT22 sensor
DHT dht(18, DHT22, 15); // (pin number, dhttype, cyclecount) :cyclecount faster uP value higher. 6=Arduino. 11=ESP8266. 15=ESP32. (Seems best)

//setup UBX_Parser
MyParser parser;

//setup wifi client
WiFiClient client;
int WiFiTimeout = 1000; //client response timeout in millis

//setup TCP server at port 80
WiFiServer server(80);

//setup SPIFFS files
File dataAir;
File dataTemp;
File dataHum;
File dataLat;
File dataLng;
File dataAlt;
File dataDate;
File dataUPT;

//--- INITIALISE PROGRAM ---
void setup(){
  dht.begin();
  Serial.begin(115200);
  
  //wifi setup
  WiFi.begin(ssid,password);
  IPAddress port(80);
  WiFi.macAddress(mac);
  for(i=0;i<5;i++){
    deviceID += mac[i];
  }
  deviceID += mac[5];
  i = 0;
  Serial.print("deviceID: ");
  Serial.print(deviceID); 
  Serial.print(" | software version: ");
  Serial.println(softVer);

  // setting up ublox GPS M8N to switch to from NMEA to UBX and run on 115200baud 
  Serial.print("Setting up GPS for UBX...");
  SerialGPS.begin(9600); //GPIO16 = RX and GPIO17 = TX
  SerialGPS.write(POSLLH_,HEX);
  delay(200);
  SerialGPS.write(POSECEF_,HEX);
  delay(200);
  SerialGPS.write(SOL_,HEX);
  delay(200);
  SerialGPS.write(TIMEUTC_,HEX);
  delay(200);
  SerialGPS.write(SAVE_CONFIG_,HEX);
  Serial.println(" OK.");

  //connecting to WiFi
  Serial.print("Connecting to WiFi");
  while(WiFi.status() != WL_CONNECTED){
    Serial.print(".");
    delay(500);
  } 
  if (WiFi.status() == WL_CONNECTED){
    Serial.print(" OK.");
  }
  
  //Enable OTA Programming
  // ArduinoOTA.setPort(3232); // Port defaults to 3232
  ArduinoOTA.setHostname(OTAHost);
  ArduinoOTA.setPassword(OTAPass);
  // Password can be set with it's md5 value as well
  // MD5(admin) = 21232f297a57a5a743894a0e4a801fc3
  // ArduinoOTA.setPasswordHash("21232f297a57a5a743894a0e4a801fc3");

  ArduinoOTA
    .onStart([]() {
      String type;
      if (ArduinoOTA.getCommand() == U_FLASH)
        type = "sketch";
      else // U_SPIFFS
        type = "filesystem";

      // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
      Serial.println("Start updating " + type);
    })
    .onEnd([]() {
      Serial.println("\nEnd");
    })
    .onProgress([](unsigned int progress, unsigned int total) {
      Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    })
    .onError([](ota_error_t error) {
      Serial.printf("Error[%u]: ", error);
      if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
      else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
      else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
      else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
      else if (error == OTA_END_ERROR) Serial.println("End Failed");
    });

  ArduinoOTA.begin();

  Serial.println();
  Serial.print("Connected to: ");
  Serial.print(ssid);
  Serial.print(" | IP Address: ");
  Serial.println(WiFi.localIP());

  //setup mDNS
  if (!MDNS.begin("espSensor")) {
      Serial.println("Error setting up MDNS responder!");
      while(1) {
          delay(1000);
      }
  }
  
  // Start Web Server server
  server.begin();
  Serial.println("Web server started");
  //Add HTTP service to MDNS-SD
  MDNS.addService("http", "tcp", 80);
  Serial.println("mDNS responder started");

  //Configuring SPIFFS
  SPIFFS.begin(true);
    
  if(!SPIFFS.begin()){
      Serial.println("SPIFFS Mount Failed");
      return;
  } else {
    Serial.print("SPIFFS Mounted | ");
  }

  Serial.print("Checking data files... ");
  dataAir = SPIFFS.open("/dataAir.txt", "a+");
  dataTemp = SPIFFS.open("/dataTemp.txt", "a+");
  dataHum = SPIFFS.open("/dataHum.txt", "a+");
  dataLat = SPIFFS.open("/dataLat.txt", "a+");
  dataLng = SPIFFS.open("/dataLng.txt", "a+");
  dataAlt = SPIFFS.open("/dataAlt.txt", "a+");
  dataDate = SPIFFS.open("/dataDate.txt", "a+");
  dataUPT = SPIFFS.open("/dataUPT.txt", "a+");
  if (!dataAir || !dataTemp || !dataHum || !dataLat || !dataLng || !dataAlt || !dataDate || !dataUPT) {
      Serial.println("Failed");
  } else {
    Serial.println("OK");
  }

  //start ftp - use same login details as OTA
  ftpSrv.begin(OTAHost,OTAPass);
 
  //Configuring GPS
  smartDelay(1000); // wait 1000ms to gather the GPS string data
  Serial.print("Configuring GPS >> Satellites locked: ");
  Serial.print(gpsSats);
  Serial.print(" | GPSfixType: ");
  Serial.print(gpsFT);
  Serial.print(" | GPS time: ");
  Serial.print(gpsHour);
  Serial.print(":");
  Serial.print(gpsMin);
  Serial.print(":");
  Serial.println(gpsSec);
   
  pinMode(2,OUTPUT);
  Serial.println("Configuration completed successfully. Programme started."); 
}

int time_ticker;
int time_delay = 10000;
int ddT = 3000;

String softDate;
String softUPT;
String softAir;
String softTemp;
String softHum;
String softLatLng;
    
//--- RUN THE PROGRAM ---
void loop(){

  //admin function to access device - monitor every cycle - take time_delay out of loop delay
  time_ticker = millis();
  while ( millis() <= (time_ticker + time_delay) ){
    digitalWrite(2,HIGH);

    // listen for OTA Firware updates
    ArduinoOTA.handle();
      
    // listen for FTP - need to put in an event handler to drop out of the main code 
    ftpSrv.handleFTP();
    
    WiFiClient webclient = server.available();   // listen for incoming clients
    //check if a web client has connected
    if (webclient) {                            
      Serial.println("new client"); 
      while (webclient.connected()) {
        if (webclient.available()) {
          String req = webclient.readStringUntil('\r');
          int addr_start = req.indexOf(' ');
          int addr_end = req.indexOf(' ', addr_start + 1);
          if (addr_start == -1 || addr_end == -1) {
              Serial.print("Invalid request: ");
              Serial.println(req);
              return;
          }
          req = req.substring(addr_start + 1, addr_end);
          Serial.print("Request: ");
          Serial.println(req);
          webclient.flush();
          String s;
          String sdata;
          if (req == "/") {
          //Use SPIFFS to host webpage  
            //Serial.print("Checking data files... ");
            html_index = SPIFFS.open("/index.html", "r");
            if (html_index) {
              while (html_index.available()) {
                s += char(html_index.read());     
              }
              Serial.println("HTML published OK.");
            } else {
              //Use code to push webpage
              //test content insert root webpage in here
              IPAddress ip = WiFi.localIP();
              String ipStr = String(ip[0]) + '.' + String(ip[1]) + '.' + String(ip[2]) + '.' + String(ip[3]);
  
              // HTML string
              sdata = "<!DOCTYPE HTML>\r\n<html><meta http-equiv=Content-Type content=text/html; charset=utf-8/><title>espSensor</title>";
              sdata += "</head><body><div><h2>Hello from espSensor at ";
              sdata += ipStr;
              sdata += "</h2></div><div>";
              sdata += softDate;
              sdata += "</div><div>Software version: ";
              sdata += softVer;
              sdata += "</div><div>Device uptime since last reset: ";
              sdata += softUPT;
              sdata += "<form>ssid:<br><input type=text name=ssid value=";
              sdata += ssid;
              sdata += "><br>password:<br><input type=text name=password value=";
              sdata += password;
              sdata += "><br><br><input type=submit value=Submit></form>";
              sdata += "</div><div>Device data:</div><div>Air Quality: ";
              sdata += softAir;
              sdata += " #No AQ Sensor currently connected.</div><div>Temperature: ";
              sdata += softTemp;
              sdata += "</div><div>Humidity: ";
              sdata += softHum;
              sdata += "</div><div>Location: ";
              sdata += softLatLng;
              sdata += "</div></body></html>\r\n\r\n";
             
              // HTML response
              s = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nContent-Length: ";
              s += sdata.length();
              s += "\r\n\r\n";
              s += sdata;
              //Serial.println(" | Sending 200");
            }
          } else {
              s = "HTTP/1.1 404 Not Found\r\n\r\n";
              Serial.println("Sending 404");
          }
          /* send response back to client and then close connect since HTTP do not keep connection*/
          webclient.print(s);
          Serial.println(s);
          webclient.flush();
        }
      }
      webclient.stop();
      Serial.println("Done with client");          
    }
    digitalWrite(2,LOW);      
  } // end wait for external interrupts
   
  //get gps data
  smartDelay(1000);

  if(gpsFix == 0x00){ //re-insert when GPS connected to device!!! 
    
    //Code here for NOX Sensor
    //
    //

    //get humidity data
    humidity = dht.readHumidity();
    if ( isnan(humidity)){
      delay (ddT);
      humidity = dht.readHumidity();
      if ( isnan(humidity) ){
        delay (ddT);
        humidity = dht.readHumidity();
        if ( isnan(humidity) ){
          humidity = 999.99;
          Serial.println("No Hum reading");
        }      
      } 
    } 
    delay (ddT);
    
    //get temp data
    temp = dht.readTemperature();
    if ( isnan(temp) ){
      delay (ddT);
      temp = dht.readTemperature();
      if ( isnan(temp) ){
        delay (ddT);
        temp = dht.readTemperature();
        if ( isnan(temp) ){
          temp = 999.99;
          Serial.println("No Temp reading");
        }      
      } 
    } 
    delay (ddT);
    
    //Serial.print(publoop);
    Serial.print(".");

    //create the publishing string for each variable
    
    if (publoop < stringloop){
      senA += String("0.00"); // No NOX Sensor connected
      senA += ",";
      senH += String(humidity,2);
      senH += ",";
      senT += String(temp,2);
      senT += ",";
      laT += String(latitude,7);
      laT += ",";
      lnG += String(longitude,7);
      lnG += ",";
      alT += String(alt,2);
      alT += ",";
      gpsDate += String(gpsYear);
      if (gpsMonth < 10)
        gpsDate += "0";
      gpsDate += String(gpsMonth);
      if (gpsDay < 10)
        gpsDate += "0";
      gpsDate += String(gpsDay);
      if (gpsHour < 10)
        gpsDate += "0";
      gpsDate += String(gpsHour);
      if (gpsMin < 10)
        gpsDate += "0";
      gpsDate += String(gpsMin);   
      if (gpsSec < 10)
        gpsDate += "0";
      gpsDate += String(gpsSec);
      gpsDate += ",";
      upT += String(millis()/1000);
      upT += ",";
    } 
    
    if (publoop == stringloop){
      senA += String("0.00"); //No NOX Sensor connected
      senH += String(humidity,2);
      senT += String(temp,2);
      laT += String(latitude,7);
      lnG += String(longitude,7);
      alT += String(alt,2);
      gpsDate += String(gpsYear);
      if (gpsMonth < 10)
        gpsDate += "0";
      gpsDate += String(gpsMonth);
      if (gpsDay < 10)
        gpsDate += "0";
      gpsDate += String(gpsDay);
      if (gpsHour < 10)
        gpsDate += "0";
      gpsDate += String(gpsHour);
      if (gpsMin < 10)
        gpsDate += "0";
      gpsDate += String(gpsMin);   
      if (gpsSec < 10)
        gpsDate += "0";
      gpsDate += String(gpsSec);
      upT += String(millis()/1000);
    }     
    
    //Heartbeat to confirm successful data collection
    digitalWrite(2,!digitalRead(2));
    delay(200);
    digitalWrite(2,!digitalRead(2));

    //Push data to webserver
    //softDate = "Date/Time : " + gpsDate;
    softDate = "Date : " + String(gpsDay) + "/" + String(gpsMonth) + "/" + String(gpsYear) + " Time: " + String(gpsHour) + ":" + String(gpsMin) + ":" + String(gpsSec);
    softAir = "NaN"; //change when NOX sensor included.
    softHum = humidity;
    softTemp = temp;
    softLatLng = String(latitude,7) + ", " + String(longitude,7) + ", " + String(alt,2) + "m";
    softUPT = String(millis()/1000);

    //next cycle
    publoop++;
  } else {
      Serial.println("No Satellite lock. Data invalid.");
  }

  if (publoop > stringloop){

    //Need to include code here for encrypting string data
    //either before storing to file or on read TBC
    
    dataAir.print(senA);
    dataTemp.print(senT);
    dataHum.print(senH);
    dataLat.print(laT);
    dataLng.print(lnG);
    dataAlt.print(alT);
    dataDate.print(gpsDate);
    dataUPT.print(upT);

    //clear the variables now the are uploaded into the file
    senA = ""; //NOX sensor not connected.
    senH = "";
    senT = "";
    laT = "";
    lnG = "";
    alT = "";
    gpsDate = "";
    upT = "";

    //reset the sensor publishing loop
    publoop = 1;
    
    //http post data
    if ( WiFi.status() == WL_CONNECTED ){
      //check wifi - if not connected, connect
      if (!client.connect(site,port) || !client.connected()){             
        client.connect(site,port);
        //debug
        Serial.print("Client connecting...");
        if (client.connect(site,port)){
          Serial.println("connected.");
        } else {
          Serial.println("failed.");
        }
      }
      // read sensor data from SPIFFS 
      // Use same variables - Purpose: if network goes down will keep storing data in files if not easier to clear
      dataAir.close();
      dataAir = SPIFFS.open("/dataAir.txt", "r");
      while (dataAir.available()) {
        senA += char(dataAir.read());     
      }
      dataTemp.close();
      dataTemp = SPIFFS.open("/dataTemp.txt", "r");
      while (dataTemp.available()) {
        senT += char(dataTemp.read());     
      }
      dataHum.close();
      dataHum = SPIFFS.open("/dataHum.txt", "r");
      while (dataHum.available()) {
        senH += char(dataHum.read());     
      }
      dataLat.close();
      dataLat = SPIFFS.open("/dataLat.txt", "r");
      while (dataLat.available()) {
        laT += char(dataLat.read());     
      }
      dataLng.close();
      dataLng = SPIFFS.open("/dataLng.txt", "r");
      while (dataLng.available()) {
        lnG += char(dataLng.read());     
      }
      dataAlt.close();
      dataAlt = SPIFFS.open("/dataAlt.txt", "r");
      while (dataAlt.available()) {
        alT += char(dataAlt.read());     
      }
      dataDate.close();
      dataDate = SPIFFS.open("/dataDate.txt", "r");
      while (dataDate.available()) {
        gpsDate += char(dataDate.read());     
      }
      dataUPT.close();
      dataUPT = SPIFFS.open("/dataUPT.txt", "r");
      while (dataUPT.available()) {
        upT += char(dataUPT.read());     
      }

      //create HTTP Post string
      String postie = "POST /sensor/sensorpush.php?id=";
      postie += deviceID;
      postie += "&Air=";
      postie += senA;
      postie += "&Hum=";
      postie += senH;
      postie += "&Temp=";
      postie += senT;
      postie += "&Lat=";
      postie += laT;
      postie += "&Lng=";
      postie += lnG;
      postie += "&Alt=";
      postie += alT;
      postie += "&gpsDate=";
      postie += gpsDate;    
      postie += "&UPT=";
      postie += upT;
      postie += " HTTP/1.1\r\n";
      postie += "Host: ";
      postie += site;
      postie += "\r\nContent-Type: application/x-www-form-urlencoded\r\nConnection: close\r\n\r\n";
      response = "";

      //Test postie
      //Serial.println(postie);
            
      //Print network connection status
      i++;
      Serial.println();
      Serial.print(i);
      Serial.print(". WiFi >> IP: ");
      Serial.print(WiFi.localIP());
      Serial.print(">");
      Serial.print(WiFi.isConnected());
      Serial.print(" | R.IP: ");
      Serial.print(client.remoteIP());
      Serial.print(":");
      Serial.print(client.remotePort());
      Serial.print(">");
      Serial.println(client.connected());
      
      //send HTTP Post string to server
      client.print(postie);
      delay(WiFiTimeout); // timeout delay while server recovers
      while (client.available()) {
        cn = client.read();
        response += cn;     
      }
      client.stop();

      //Parse HTTP Response
      repSize = response.length();
      serSatLoc = response.indexOf('HTTP');
      serSata = response.substring((serSatLoc+6),(serSatLoc+9));
      serSat = serSata.toInt(); 
      if (serSat == 200){
        serLenLocA = response.indexOf("SOR#");
        serLenLocB = response.indexOf("#EOR");
        repDBStatus = response.substring((serLenLocA+4), serLenLocB); 
        Serial.print("Server >> Status: ");
        Serial.print(serSat);
        Serial.print(" | Response: ");
        Serial.println(repDBStatus);

        //if repDBStatus is OK then - successful upload.
        
        //clear data files and re-open for use on next cycle
        dataAir.close();
        dataAir = SPIFFS.open("/dataAir.txt", "w");
        dataAir.print("");
        dataAir.close();
        dataAir = SPIFFS.open("/dataAir.txt", "a+");
         
        dataTemp.close();
        dataTemp = SPIFFS.open("/dataTemp.txt", "w");
        dataTemp.print("");
        dataTemp.close();
        dataTemp = SPIFFS.open("/dataTemp.txt", "a+");
         
        dataHum.close();
        dataHum = SPIFFS.open("/dataHum.txt", "w");
        dataHum.print("");
        dataHum.close();
        dataHum = SPIFFS.open("/dataHum.txt", "a+");
         
        dataLat.close();
        dataLat = SPIFFS.open("/dataLat.txt", "w");
        dataLat.print("");
        dataLat.close();
        dataLat = SPIFFS.open("/dataLat.txt", "a+");
         
        dataLng.close();
        dataLng = SPIFFS.open("/dataLng.txt", "w");
        dataLng.print("");
        dataLng.close();
        dataLng = SPIFFS.open("/dataLng.txt", "a+");
         
        dataAlt.close();
        dataAlt = SPIFFS.open("/dataAlt.txt", "w");
        dataAlt.print("");
        dataAlt.close();
        dataAlt = SPIFFS.open("/dataAlt.txt", "a+");
         
        dataDate.close();
        dataDate = SPIFFS.open("/dataDate.txt", "w");
        dataDate.print("");
        dataDate = SPIFFS.open("/dataDate.txt", "a+");
         
        dataUPT.close();
        dataUPT = SPIFFS.open("/dataUPT.txt", "w");
        dataUPT.print("");
        dataUPT.close();
        dataUPT = SPIFFS.open("/dataUPT.txt", "a+");
        
        if (!dataAir || !dataTemp || !dataHum || !dataLat || !dataLng || !dataAlt || !dataDate || !dataUPT) {
            Serial.println("Failed");
        } else {
          Serial.println("OK");
        } 

        // clear variable for next cycle
        humidity = NULL;
        temp = NULL;
        senA = "0"; //NOX sensor not connected.
        senH = "";
        senT = "";
        laT = "";
        lnG = "";
        alT = "";
        gpsDate = "";
        upT = "";

        //Double heartbeat - data to server was OK
        digitalWrite(2,!digitalRead(2));
        delay(200);
        digitalWrite(2,!digitalRead(2));
        delay(200);
        digitalWrite(2,!digitalRead(2));
        delay(200);
        digitalWrite(2,!digitalRead(2));
        
      } else {
        Serial.print("Website failed. Response >> Server status: ");
        Serial.println(serSat);
      }
    } else {
      WiFi.disconnect();
      //wifi setup
      WiFi.begin(ssid,password);
      IPAddress port(80);
      Serial.print("Reconnecting to WiFi");
      while(WiFi.status() != WL_CONNECTED){
        Serial.print(".");
        delay(500);
      }
      Serial.println();
      Serial.print("Connected to: ");
      Serial.print(ssid);
      Serial.print(" | IP Address: ");
      Serial.println(WiFi.localIP());
      smartDelay(1000); // wait ms to gather the GPS string data
      Serial.print("Configuring GPS >> Satellites locked: ");
      Serial.print(gpsSats);
      Serial.print(" | GPSfixType: ");
      Serial.print(gpsFT);
      Serial.print(" | GPS time: ");
      Serial.print(gpsHour);
      Serial.print(":");
      Serial.print(gpsMin);
      Serial.print(":");
      Serial.println(gpsSec);
    }
  }
}

static void smartDelay(unsigned long ms){ 
  unsigned long start = millis();
  do{
    while (SerialGPS.available())
      parser.parse(SerialGPS.read());
  } while (millis() - start < ms);
}
