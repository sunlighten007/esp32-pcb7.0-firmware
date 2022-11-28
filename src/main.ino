#include <BluetoothSerial.h>
#include <ArduinoJson.h>
#include <string.h>
#include <WiFi.h>
#include "esp_ota_ops.h"
#include <HTTPClient.h>
#include <ArduinoOTA.h>
#include <Preferences.h>
#include <ArduinoUniqueID.h>
#include "time.h"
void generate_serial_id();
void connect_to_wifi(String _ssid, String _wifi_password);
void saveWifiDetailsLocally(String ssid, String wifi_password);
void wifiSetupOnStart();
void setup();
void process_cmdR1();
void send_data();
void send_info();
void updateWifiStatus();
void datareceived();
void ota_over_web();
void fn(int t, int t2);

#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
#endif

BluetoothSerial SerialBT;

// NTP server to request epoch time
const char* ntpServer = "pool.ntp.org";
unsigned long epochTime;

unsigned long getTime() {
  time_t now;
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    //Serial.println("Failed to obtain time");
    return(0);
  }
  time(&now);
  return now;
}
 
//protocol variables
int version_no=1;
String cmd_code="";
int pdt_code=1;
String my_data;
String readString;
int i=0;
char c;
const char* first_com;
const char* second_com;
const char* third_com;
//*******************************
//Dip switch values
int way0=1;
int way1=0;
int way2=0;
int acvol=1;
//*******************************

const String FIRMWARE_VERSION = "v0.0.1";
const String HARDWARE_VERSION = "v7.0.0";
boolean isExtLight;
boolean isIntLight;
//firmware update status
int UPDATE_NOT_INITIALIZE = 0;
int UPDATE_FOUND = 1;
int UPDATE_IN_PROCESS = 2;
int UPDATE_SUCCESSFULLY_DONE = 3;
int UPDATE_FAILED = 4;
int FIRMWARE_UPDATE_STATUS = UPDATE_NOT_INITIALIZE;
//**************************************

//update ota
bool UPDATE_OTA=false;
bool UPDATE_OTA_OVER_BLE=false;
//***************************************

//heaters intensity(0-10) 
int all_mir_inten=0;
int all_fir_inten=0;
int LF_inten=0;
int LM_inten=0;
int F_inten=0;
int FM_inten=0;
int FF_inten=0;
int BF_inten=0;
int BM_inten=0;
int RF_inten=0;
int RM_inten=0;
//nir flag (0/1)
int left_nir_flag=0;
int right_nir_flag=0;
int back_nir_flag=0;
//heaters current sensors value
int LF_current=0;
int LM_current=0;
int F_current=0;
int FM_current=0;
int FF_current=0;
int BF_current=0;
int BM_current=0;
int RF_current=0;
int RM_current=0;
int Ac_Volt=0;
// temperature  
int cabin_temp_in=0;
int set_temp=0;
int ntc_avg=0;
int Heaters_on=0;
DynamicJsonDocument received(1024);
DynamicJsonDocument protocol(1024);
#define RXD2 21
#define TXD2 22



String device_id = "";

void generate_serial_id()
{
  for (size_t i = 0; i < UniqueIDsize; i++)
  {
    if (UniqueID[i] < 0x10)
      device_id = device_id + 0;

    device_id = device_id + String(UniqueID[i], HEX);

    if (i != UniqueIDsize - 1)
    {
      device_id = device_id;
    }
  }
  device_id.toUpperCase();
}
Preferences preferences;

//set wifi ssid and password
const char* wifi_ssid = "";
const char* wifi_password = "";
int wifi_status = 0;
//******************************
void connect_to_wifi(String _ssid, String _wifi_password)
{

  Serial.println("Attempting wifi connection");
  wifi_status = 0;
  if (WiFi.status() == WL_CONNECTED)
  {
    Serial.println("Wifi was already connected. Disconnecting...");
    WiFi.disconnect();
    delay(2000);
    Serial.println("Wifi disconnected");
  }
  WiFi.mode(WIFI_STA);
  WiFi.begin(_ssid.c_str(), _wifi_password.c_str());
  delay(3000);
  Serial.println(WiFi.status());
  if (WiFi.waitForConnectResult() != WL_CONNECTED)
  {
    Serial.println((String) "Unable to connect to " + _ssid + " with pass " + _wifi_password + " , maybe because of incorrect credentials.");

  }
  else
  {
    saveWifiDetailsLocally(_ssid, _wifi_password);
    Serial.println((String) "Connected to Wifi at ssid:  " + _ssid);
    wifi_status = 1;

  }

}

void saveWifiDetailsLocally(String ssid, String wifi_password)
{

  preferences.putString("wi_ssid", ssid);
  preferences.putString("wi_pass", wifi_password);
  Serial.println("Wifi Details saved locally");

}

long contentLength = 0;
bool isValidContentType = false;

String url_host = String("http://bridge.otauserver.sunlightsaunas.com:2300/signature_bridge/update?current_version=" + FIRMWARE_VERSION + "&uuid=" + device_id + "&country=US&language=en");
int port = 2300;
String bin = "/update.bin";

String getHeaderValue(String header, String headerName)
{
  return header.substring(strlen(headerName.c_str()));
}

void ota_over_web()
{
  UPDATE_OTA=false;

  Serial.println(String("Firmware update URL: ") + url_host);

  FIRMWARE_UPDATE_STATUS = UPDATE_IN_PROCESS;

  HTTPClient http;
  http.begin(url_host);

  int httpCode = http.GET();
  if (httpCode != HTTP_CODE_OK)
  {

    Serial.println(String("response code: " + String(httpCode)));
    FIRMWARE_UPDATE_STATUS = UPDATE_FAILED;
    return;
  }
  int contentLength = http.getSize();
  if (contentLength <= 0)
  {
    FIRMWARE_UPDATE_STATUS = UPDATE_FAILED;
    Serial.println("Content-Length not defined");
    return;
  }

  bool canBegin = Update.begin(contentLength);
  if (!canBegin)
  {
    FIRMWARE_UPDATE_STATUS = UPDATE_FAILED;
    Serial.println("Not enough space to begin OTA");
    return;
  }

   FIRMWARE_UPDATE_STATUS = UPDATE_FOUND;

   Client &wifi_client = http.getStream();

  Serial.print("----22---update.begin()----------- COntent len->  ");
  Serial.print(contentLength);
  Update.onProgress(fn);
  Serial.println("onPress added now writing");
  int written = Update.writeStream(wifi_client);

   Serial.print("------11---update.begin()-----------");


  if (written != contentLength)
  {
    Serial.println(String("OTA written ") + written + " / " + contentLength + " bytes");
    return;
  }

  if (!Update.end())
  {
    Serial.println("Error #" + String(Update.getError()));
    FIRMWARE_UPDATE_STATUS = UPDATE_FAILED;
    return;
  }

  if (!Update.isFinished())
  {
    Serial.println("Update failed.");
    FIRMWARE_UPDATE_STATUS = UPDATE_FAILED;
    return;
  }

  FIRMWARE_UPDATE_STATUS = UPDATE_SUCCESSFULLY_DONE;
  Serial.println("Update successfully completed. Rebooting.");
  ESP.restart();
}

void fn(int t, int t2){

  Serial.print(t);
  Serial.print(t2);
  Serial.println("");
}

void wifiSetupOnStart()
{ 
  Serial.println("Trying to setup wifi with saved wifi details");
  String DEFAULT_WIFI_CREDS = "---";
  String wifi_ssid = preferences.getString("wi_ssid", DEFAULT_WIFI_CREDS);
  String wifi_password = preferences.getString("wi_pass", DEFAULT_WIFI_CREDS);
  Serial.println("ssid retreived from storage->" + wifi_ssid);
  Serial.println("password retreived from storage->" + wifi_password);
  if (wifi_ssid == DEFAULT_WIFI_CREDS && wifi_password == DEFAULT_WIFI_CREDS)
  {
    Serial.println("Unable to find saved wifi details.");
    Serial.println("Please setup wifi connection.");
  }
  else
  {
    connect_to_wifi(wifi_ssid, wifi_password);
    delay(1000);
  }
}

void setup(){
   generate_serial_id();
   unsigned char i;
   Serial.begin(115200);
   Serial2.begin(115200, SERIAL_8N1, RXD2, TXD2);
   Serial.println("Device ID: " + device_id);
   preferences.begin("prefs", false);
   Serial.print("---------Firmware Version----------- ");
   Serial.print(FIRMWARE_VERSION);
   Serial.println();
   disableCore0WDT();
   wifiSetupOnStart();
   SerialBT.begin("S_Bluetooth"); 
   configTime(0, 0, ntpServer);
   String f_name="/main";
}
void updateWifiStatus()
{

  if (WiFi.status() == WL_CONNECTED)
  {
    wifi_status = 1;
  }
  else
  {
    wifi_status = 0;
  }
}
uint8_t msg[500];

void loop(){
  
  epochTime = getTime();
  
  datareceived();
  
  process_cmdR1();

  delay(10000);
  
  updateWifiStatus();
  
  send_raw_data();   

  ota_over_web();
     }

void datareceived(void){
   while(Serial2.available()) {
    c = Serial2.read();
    readString += c;
  }
   if (readString.length() > 0) {
    deserializeJson(received,readString);
    readString ="";
      }  
}

void process_cmdR1(void){
  if(received["v"]==1 && received["cmd"]["product_code"]==1)
  {
     if(received["cmd"]["cmd_code"]=="REQUEST_PCB_INFO"){
        third_com=second_com;
        second_com=first_com;
        first_com="RPI";
        send_info();
      }
      else if(received["cmd"]["cmd_code"]=="START_CUSTOM_PROGRAM"){
              third_com=second_com;
              second_com=first_com;
              first_com="SCP";
              cabin_temp_in=received["cmd"]["cmd_metadata"]["ACP_TEMP"];
              
              set_temp=received["cmd"]["cmd_metadata"]["SET_TEMP"];
              
              LF_inten=received["cmd"]["cmd_metadata"]["LF_INTEN"];
             
              LM_inten=received["cmd"]["cmd_metadata"]["LM_INTEN"];
              
              F_inten=received["cmd"]["cmd_metadata"]["F_INTEN"];
              
              FM_inten=received["cmd"]["cmd_metadata"]["FM_INTEN"];
                           
              FF_inten=received["cmd"]["cmd_metadata"]["FF_INTEN"];
              
              BF_inten=received["cmd"]["cmd_metadata"]["BF_INTEN"];
              
              BM_inten=received["cmd"]["cmd_metadata"]["BM_INTEN"];
              
              RF_inten=received["cmd"]["cmd_metadata"]["RF_INTEN"];
              
              RM_inten=received["cmd"]["cmd_metadata"]["RM_INTEN"];
              
              left_nir_flag=received["cmd"]["cmd_metadata"]["LEFT_NIR_FLAG"];
              
              right_nir_flag=received["cmd"]["cmd_metadata"]["RIGHT_NIR_FLAG"];
              
              back_nir_flag=received["cmd"]["cmd_metadata"]["BACK_NIR_FLAG"];
              
      }
      else if(received["cmd"]["cmd_code"]=="START_PREDEFINED_PROGRAM"){
        third_com=second_com;
        second_com=first_com;
        first_com="SPP";
        cabin_temp_in=received["cmd"]["cmd_metadata"]["ACP_TEMP"];
        
        set_temp=received["cmd"]["cmd_metadata"]["SET_TEMP"];
        
        all_fir_inten=received["cmd"]["cmd_metadata"]["ALL_FIR_INTEN"];
        
        all_mir_inten=received["cmd"]["cmd_metadata"]["ALL_FIR_INTEN"];
        
        left_nir_flag=received["cmd"]["cmd_metadata"]["LEFT_NIR_FLAG"];
        
        right_nir_flag=received["cmd"]["cmd_metadata"]["RIGHT_NIR_FLAG"];
        
        back_nir_flag=received["cmd"]["cmd_metadata"]["BACK_NIR_FLAG"];
        
      }
      else if(received["cmd"]["cmd_code"]=="SET_WIFI_CREDENTIAL"){
        third_com=second_com;
        second_com=first_com;
        first_com="SWC";
        
        wifi_ssid=received["cmd"]["cmd_metadata"]["set_ssid"];        
        wifi_password=received["cmd"]["cmd_metadata"]["set_password"];
        saveWifiDetailsLocally(wifi_ssid,wifi_password);
        wifiSetupOnStart(); 
        configTime(0, 0, ntpServer);        
             }
      else if(received["cmd"]["cmd_code"]=="SET_LIGHT"){
        third_com=second_com;
        second_com=first_com;
        first_com="SL";
    
        isIntLight=received["cmd"]["cmd_metadata"]["Set_Interior_light"];
        isExtLight=received["cmd"]["cmd_metadata"]["Set_Exterior_light"]; 
      }
      else if(received["cmd"]["cmd_code"]=="TURN_OFF_HEATERS"){
            third_com=second_com;
            second_com=first_com;
            first_com="TOH";            
            Heaters_on=0;
      }
     
  }
  
}

void send_info(void){
  protocol["v"]=version_no;
  protocol["cmd"]["product_code"]=pdt_code;
  protocol["cmd"]["cmd_code"]="SEND_PCB_INFO";
  protocol["cmd"]["cmd_metadata"]["WAY0"]=way0;
  protocol["cmd"]["cmd_metadata"]["WAY1"]=way1;
  protocol["cmd"]["cmd_metadata"]["WAY2"]=way2;
  protocol["cmd"]["cmd_metadata"]["AC_VOL"]=acvol;
  protocol["cmd"]["cmd_metadata"]["FIRMWARE_V"]=FIRMWARE_VERSION;
  protocol["cmd"]["cmd_metadata"]["HARDWARE_V"]=HARDWARE_VERSION;
  protocol["cmd"]["cmd_metadata"]["WIFI_STATUS"]=wifi_status;
  send_data();
}
void send_raw_data(void){
  protocol["v"]=version_no;
  protocol["cmd"]["product_code"]=pdt_code;
  protocol["cmd"]["cmd_code"]="SEND_RAW_DATA";
  protocol["cmd"]["cmd_metadata"]["NTC"]=ntc_avg;
  protocol["cmd"]["cmd_metadata"]["LF_CURRENT"]=LF_current;
  protocol["cmd"]["cmd_metadata"]["LM_CURRENT"]=LM_current;
  protocol["cmd"]["cmd_metadata"]["RF_CURRENT"]=RF_current;
  protocol["cmd"]["cmd_metadata"]["RM_CURRENT"]=RM_current;
  protocol["cmd"]["cmd_metadata"]["F_CURRENT"]=F_current;
  protocol["cmd"]["cmd_metadata"]["FF_CURRENT"]=FF_current;
  protocol["cmd"]["cmd_metadata"]["FM_CURRENT"]=FM_current;
  protocol["cmd"]["cmd_metadata"]["BF_CURRENT"]=BF_current;
  protocol["cmd"]["cmd_metadata"]["BM_CURRENT"]=BM_current;
  protocol["cmd"]["cmd_metadata"]["AC_VOLT"]=Ac_Volt;
  protocol["cmd"]["cmd_metadata"]["WIFI_STATUS"]=wifi_status;
  send_data();
}

void send_data(void){
  protocol["ack"]["1"]=first_com;
  protocol["ack"]["2"]=second_com;
  protocol["ack"]["3"]=third_com;
  protocol["metadata"]["timestamp"]=epochTime;
 if(Serial2.availableForWrite()>0){
    protocol["metadata"]["ch"]="<RS232>";
    serializeJson(protocol,my_data);
    for(i=0;i<=my_data.length();i++){
          Serial2.write(my_data[i]);
              }
          Serial2.println(); 
  }
 my_data=""; 
 protocol.clear();
 received.clear();
  
}