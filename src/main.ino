#include <SomeSerial.h>

#include <BluetoothSerial.h>
#include<HardwareSerial.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLE2902.h>

#include <ArduinoJson.h>
#include <string.h>

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <WiFiClient.h>
#include <ESPmDNS.h>
#include <Update.h>
#include <PubSubClient.h>
#include <HTTPClient.h>
#include "esp_ota_ops.h"
#include <ArduinoOTA.h>
#include <Preferences.h>
#include <ArduinoUniqueID.h>
#include "time.h"
#include "Helpers.h"
#include "id.h"
#include "info.h"




#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
#endif

BluetoothSerial SerialBT;
HardwareSerial Heater_Serial(1);
HardwareSerial Light_Serial(2);










WiFiClient espClient;
PubSubClient client(espClient);
const char *mqtt_server = "wss://dev-mqtt.api.sunlighten.com:8083/mqtt";








// NTP server to request epoch time
const char* ntpServer = "pool.ntp.org";
unsigned long epochTime;
const char* channel="";




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
bool UPDATE_OTA_OVER_CLASSIC=false;
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
esp_ota_handle_t otaHandler = 0;
bool updateFlag = false;
bool readyFlag = false;
int bytesReceived = 0;
int timesWritten = 0;

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
String getOutTopic(void){
  return String("saunabridge/"  + device_id + "/out");
}

String getInTopic(){
  return String("saunabridge/"  + device_id + "/in");
}

String getStatsTopic(){
  return String("saunabridge/"  + device_id + "/stats");
}



Preferences preferences;







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
    /*setup_mqtt_connection();*/
      }

}

void saveWifiDetailsLocally(String ssid, String wifi_password)
{

  preferences.putString("wi_ssid", ssid);
  preferences.putString("wi_pass", wifi_password);
  Serial.println("Wifi Details saved locally");

}



void fn(int t, int t2){

  Serial.print(t);
  Serial.print(t2);
  Serial.println("");
}

class WIFI_SSID_Callbacks : public BLECharacteristicCallbacks
{
  void onWrite(BLECharacteristic *pCharacteristic)
  {
    std::string rxValue = pCharacteristic->getValue();

    if (rxValue.length() > 0)
    {
      wifi_ssid = rxValue.c_str();
      Serial.println(wifi_ssid);
    }
  }
};
class WIFI_PASS_Callbacks : public BLECharacteristicCallbacks
{
  void onWrite(BLECharacteristic *pCharacteristic)
  {
    std::string rxValue = pCharacteristic->getValue();

    if (rxValue.length() > 0)
    {
      wifi_password = rxValue.c_str();
      connect_to_wifi(wifi_ssid, wifi_password);


      Serial.println(wifi_password);
    }
  }
};

class WIFI_STATUS_Callbacks : public BLECharacteristicCallbacks
{

  void onRead(BLECharacteristic *pCharacteristic)
  {
    //wifiSetupOnStart();
    if (WiFi.status() == WL_CONNECTED)
    {
      wifi_status = 1;
    }
    else
    {
      wifi_status = 0;
    }
      Serial.print(wifi_status);
      pWIFI_STATUS_Characteristic->setValue(wifi_status);
    
  }
};




class UPDATE_FIRMWARE_CALLBACK : public BLECharacteristicCallbacks
{
  void onWrite(BLECharacteristic *pCharacteristic)
  {
    std::   string rxValue = pCharacteristic->getValue();
    Serial.println(rxValue.c_str());
    Serial.print("----------------------update the firmaware--------------");
    if (rxValue.length() > 0)
    {

       UPDATE_OTA=true;
      Serial.println("update write");
      Serial.print(FIRMWARE_VERSION);

    }
  }
};


std::string rxData;
void write_ota(uint8_t *data,int bufferLength);
class OTA_OVER_BLE_CALLBACK : public BLECharacteristicCallbacks{
 void onWrite(BLECharacteristic *pCharacteristic)
  {
    Serial.print("--------------------------write method of onWrite is called------------------");

   rxData = pCharacteristic->getValue();
    uint8_t data[10];
   write_ota(data,2);
  UPDATE_OTA_OVER_BLE=true;
  
   return;
  };

};


void BLE_Services()
{
  
  Serial.println("Starting BLE Services!");
  BLEDevice::init("SunlightenSauna-Signature");
  BLEServer *pServer = BLEDevice::createServer();
  BLEService *pWIFI_Service = pServer->createService(WIFI_SERVICE_UUID);
  BLECharacteristic *pWIFI_SSID_Characteristic = pWIFI_Service->createCharacteristic(
      WIFI_SSID_CHARACTERISTIC_UUID,
      BLECharacteristic::PROPERTY_READ |
          BLECharacteristic::PROPERTY_WRITE |
          BLECharacteristic::PROPERTY_NOTIFY |
          BLECharacteristic::PROPERTY_INDICATE |
          BLECharacteristic::PROPERTY_BROADCAST);
  pWIFI_SSID_Characteristic->setValue(wifi_ssid.c_str());
  pWIFI_SSID_Characteristic->addDescriptor(new BLE2902());
  pWIFI_SSID_Characteristic->setCallbacks(new WIFI_SSID_Callbacks());
  
  BLECharacteristic *pWIFI_PASS_Characteristic = pWIFI_Service->createCharacteristic(
      WIFI_PASS_CHARACTERISTIC_UUID,
      BLECharacteristic::PROPERTY_READ |
          BLECharacteristic::PROPERTY_WRITE |
          BLECharacteristic::PROPERTY_NOTIFY |
          BLECharacteristic::PROPERTY_INDICATE |
          BLECharacteristic::PROPERTY_BROADCAST);
  pWIFI_PASS_Characteristic->setValue(wifi_password.c_str());
  pWIFI_PASS_Characteristic->addDescriptor(new BLE2902());
  pWIFI_PASS_Characteristic->setCallbacks(new WIFI_PASS_Callbacks());

 BLEService *pFirmware_Update_Over_WIFI = pServer->createService(FIRMWARE_UPDATE_OVER_WIFI_UUID);
 BLECharacteristic *pFIRMWARE_UPDATE_OVER_WIFI_Characteristic = pFirmware_Update_Over_WIFI->createCharacteristic(
      FIRMWARE_UPDATE_OVER_WIFI_Characteristic,
       BLECharacteristic::PROPERTY_READ |
          BLECharacteristic::PROPERTY_WRITE |
          BLECharacteristic::PROPERTY_NOTIFY |
          BLECharacteristic::PROPERTY_INDICATE |
          BLECharacteristic::PROPERTY_BROADCAST);
 pFIRMWARE_UPDATE_OVER_WIFI_Characteristic->addDescriptor(new BLE2902());
 pFIRMWARE_UPDATE_OVER_WIFI_Characteristic->setCallbacks(new UPDATE_FIRMWARE_CALLBACK());
 pFirmware_Update_Over_WIFI->start();

  //WIFI STATUS
  pWIFI_STATUS_Characteristic = pWIFI_Service->createCharacteristic(
      WIFI_STATUS_CHARACTERISTIC_UUID,
      BLECharacteristic::PROPERTY_READ |
          BLECharacteristic::PROPERTY_WRITE |
          BLECharacteristic::PROPERTY_NOTIFY |
          BLECharacteristic::PROPERTY_INDICATE |
          BLECharacteristic::PROPERTY_BROADCAST);
  pWIFI_STATUS_Characteristic->addDescriptor(new BLE2902());
  pWIFI_STATUS_Characteristic->setCallbacks(new WIFI_STATUS_Callbacks());
  pWIFI_Service->start();
  //END


  BLEService *pfirmwareStatusService = pServer->createService(FIRMWARE_STATUS_SERVICE_UUID);
  BLECharacteristic *pFIRMWARE_STATUS_Characteristic = pfirmwareStatusService->createCharacteristic(
      FIRMWARE_STATUS_CHARACTERISTIC_UUID,
      BLECharacteristic::PROPERTY_READ |
          BLECharacteristic::PROPERTY_NOTIFY |
          BLECharacteristic::PROPERTY_INDICATE |
          BLECharacteristic::PROPERTY_BROADCAST);
  pFIRMWARE_STATUS_Characteristic->setValue(String(FIRMWARE_VERSION).c_str());
  pFIRMWARE_STATUS_Characteristic->addDescriptor(new BLE2902());
  pfirmwareStatusService->start();

  BLEService *deviceIdService = pServer->createService(DEVICE_INFORMATION_SERVICE_UUID);
  deviceIdService->start();
  BLECharacteristic *deviceIdSerialNumberCharacteristic = deviceIdService->createCharacteristic(
      SERIAL_NUMBER_STRING_CHARACTERISTIC_UUID,
      BLECharacteristic::PROPERTY_READ |
          BLECharacteristic::PROPERTY_NOTIFY);
  deviceIdSerialNumberCharacteristic->setValue(device_id.c_str());

  BLECharacteristic *firmwareRevisionCharacteristic = deviceIdService->createCharacteristic(
      FIRMWARE_REVISION_STRING_CHARACTERISTIC_UUID,
      BLECharacteristic::PROPERTY_READ |
          BLECharacteristic::PROPERTY_NOTIFY);
  firmwareRevisionCharacteristic->setValue(String(FIRMWARE_VERSION).c_str());

  BLECharacteristic *hardwareRevisionCharacteristic = deviceIdService->createCharacteristic(
      HARDWARE_REVISION_STRING_CHARACTERISTIC_UUID,
      BLECharacteristic::PROPERTY_READ |
          BLECharacteristic::PROPERTY_NOTIFY);
  hardwareRevisionCharacteristic->setValue(String(HARDWARE_VERSION).c_str());
  deviceIdService->start();

  environmentService = pServer->createService(ENVIRONMENTAL_SENSING_SERVICE_UUID);
  temperatureCharacteristic = environmentService->createCharacteristic(
      TEMPERATURE_CHARACTERISTIC_UUID,
      BLECharacteristic::PROPERTY_READ |
          BLECharacteristic::PROPERTY_NOTIFY |
          BLECharacteristic::PROPERTY_BROADCAST);
  environmentService->start();

  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(WIFI_SERVICE_UUID);
  pAdvertising->addServiceUUID(DEVICE_INFORMATION_SERVICE_UUID);
  pAdvertising->addServiceUUID(ENVIRONMENTAL_SENSING_SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06);
  pAdvertising->setMinPreferred(0x12);
  
  BLEDevice::startAdvertising();

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
   Heater_Serial.begin(9600, SERIAL_8N1, RXD2, TXD2);
   Light_Serial.begin(9600, SERIAL_8N1,RXD1,TXD1);
   Serial.println("Device ID: " + device_id);
   preferences.begin("prefs", false);
   Serial.print("---------Firmware Version----------- ");
   Serial.print(FIRMWARE_VERSION);
   Serial.println();
   disableCore0WDT();
   wifiSetupOnStart();
   SerialBT.begin("S_Bluetooth"); 
   BLE_Services(); 
   Serial.println("--------Bluetooth Device is Ready to Pair-------");
   
   configTime(0, 0, ntpServer);
   String f_name="/main";
   pinMode(Intlight,OUTPUT);
   pinMode(Extlight,OUTPUT);
   pinMode(Wifi_on,OUTPUT);
if (WiFi.status() == WL_CONNECTED)
 {
 setup_mqtt_connection();
  }

   
}
void setup_mqtt_connection()
{
  if (WiFi.status() == WL_CONNECTED)
  {
    client.setServer(mqtt_server, 443);
    client.setCallback(mqtt_subscribe_callback);
  }
}


void updateWifiStatus()
{

  if (WiFi.status() == WL_CONNECTED)
  {
    wifi_status = 1;
    digitalWrite(Wifi_on,HIGH);
  }
  else
  {
    wifi_status = 0;
    digitalWrite(Wifi_on,LOW);
  }
}
uint8_t msg[500];
bool updatingFirmware_Bluetooth=false;
void loop(){
if (WiFi.status() == WL_CONNECTED)
  {
    if (!client.connected())
    {
      mqtt_reconnect();
    }
    client.loop();
  }

 
  epochTime = getTime();
  
  Serial_received();

  if(updatingFirmware_Bluetooth)return;
  
  delay(10000);
  
  updateWifiStatus();
  
  send_raw_data();  
  
   

     }

void mqtt_reconnect()
{
  // Loop until we're reconnected
  while (!client.connected())
  {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP32Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str(),mqtt_username.c_str(),mqtt_password.c_str()))
    {
      Serial.print(clientId.c_str());
      Serial.print(" connected");
      // Once connected, publish an announcement...
      client.publish(getStatsTopic().c_str(), String("[" + device_id + "] BootedUp").c_str());
      // ... and resubscribe
     Serial.print(String("MQTT Subcribed to " + getInTopic()));
     Serial.print(String("MQTT Publishing to " + getOutTopic()));
     Serial.print(String("MQTT Publishing to " + getStatsTopic()));
      client.subscribe(getInTopic().c_str());
    }
    else
    {
      Serial.print("clientid=");
      Serial.print(clientId.c_str());
      Serial.print(" failed, rc=");
      Serial.print(client.state());
      Serial.print(" try again in 5 seconds");
      // Wait 5 seconds before
      delay(5000);
    }
  }
}
void mqtt_subscribe_callback(char *topic, byte *payload, unsigned int length)
{
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  String jsonString = String((char *)payload);
  for (int i = 0; i < length; i++)
  {
    Serial.print((char)payload[i]);
  }

  Serial.print("mqtt command length>>" + String(jsonString.length()));
  process_cmd(jsonString);
}








void progress(int t, int t2){
  Serial.print("----onProgram---");
  Serial.print(t);
  Serial.print(t2);
  Serial.println("");

}

void Serial_received(void){
  if(Heater_Serial.available()){
   while(Heater_Serial.available()) {
    c = Heater_Serial.read();
    readString += c;
  } 
  }
  else if(SerialBT.available()){
      if(UPDATE_OTA_OVER_CLASSIC){
        bluetooth_data();    
      }

  else if(Light_Serial.available()){
    while(Light_Serial.available()){
      c= Light_Serial.read();
      readString +=c;     
    }
  }
   else{   
    while(SerialBT.available()) {
    c = SerialBT.read();
    readString += c;
  }  
  }
  
  }

  
   if (readString.length() > 10) {
    Serial.print(readString);
    process_cmd(readString);
    readString="";
    
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
  protocol["cmd"]["cmd_metadata"]["Device_ID"]=device_id;
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
if(String(channel)=="<RS232>"){
    protocol["metadata"]["ch"]="<RS232>";
    serializeJson(protocol,Heater_Serial);
    Heater_Serial.println();
  }
else if(String(channel)=="<BT>"){
  protocol["metadata"]["ch"]="<BT>";
    String data_bt="";
    serializeJson(protocol,data_bt);
    SerialBT.print(data_bt);   
    SerialBT.println();
    }
else if(String(channel)=="<WIFI>"){
  protocol["metadata"]["ch"]="<WIFI>";
  serializeJson(protocol,my_data);
  my_data=my_data+"\n";
  client.publish(getOutTopic().c_str(), my_data.c_str());
}
 protocol.clear();
 received.clear();  
 my_data="";
}

void turnOnLight(boolean isIntLight,boolean isExtLight)
{

  if(isIntLight)
  {
    digitalWrite(Intlight, HIGH);

    Serial.print("Interior Light On");
  }
  else
  {

    digitalWrite(Intlight, LOW);
    
    Serial.print("Interior Light is OFF");
  }
  if(isExtLight)
  {
     digitalWrite(Extlight, HIGH);
     
     Serial.print("Exterior Light is On");
  }
  else{
     digitalWrite(Extlight, LOW);
   
     Serial.print("Exterior Light is  OFF");
    
  }
}
void process_cmd(String command){
  DeserializationError error = deserializeJson(received, command);
  command="";
  if (error)
  {
    Serial.print(String("deserializeJson() failed: " + String(error.c_str())));
  }
  
  if(received["v"]==1 && received["cmd"]["product_code"]==1)
  {
      channel=received["metadata"]["ch"];
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
        
        temp=received["cmd"]["cmd_metadata"]["set_ssid"];
        wifi_ssid=temp;        
        temp=received["cmd"]["cmd_metadata"]["set_password"];
        wifi_password=temp;
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
        turnOnLight(isIntLight,isExtLight);
      }
      else if(received["cmd"]["cmd_code"]=="TURN_OFF_HEATERS"){
            third_com=second_com;
            second_com=first_com;
            first_com="TOH";            
            Heaters_on=0;
      }
     else if(received["cmd"]["cmd_code"]=="UPDATE_FIRMWARE"){
            third_com=second_com;
            second_com=first_com;
            first_com="UF"; 
            UPDATE_OTA=received["cmd"]["cmd_metadata"]["Update_OTA"];
            UPDATE_OTA_OVER_BLE=received["cmd"]["cmd_metadata"]["Update_OTA_OVER_BLE"];
            UPDATE_OTA_OVER_CLASSIC=received["cmd"]["cmd_metadata"]["Update_OTA_OVER_CLASSIC"];  
            if(UPDATE_OTA){
                  ota_over_web();
                  }
           if(UPDATE_OTA_OVER_BLE){
                  UPDATE_OTA_OVER_BLE=false;
                  uint8_t data[12];

    }
            
   }
  } 
}



//////////////////////UPDATE OVER THE WEB/////////////////////////////////////

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



///////////////////UPDATE OVER BLUETOOTH  CLASSIC/////////////////

long int count=0;
bool fileOpened=false;
int written=0;
int updateStarted=0;
char testSend[]={'O','K',';'};
uint8_t buffer2[]={'O','K',';'};
int writtenCount=0;
int pos=0;
int size;
void bluetooth_data(){

UPDATE_OTA_OVER_CLASSIC=false;
   if(SerialBT.available()){

   updatingFirmware_Bluetooth=true;

   Serial.print("Data available()");
   uint8_t buffer[5000];
   int bufferSize=5000;
   byte byte=SerialBT.readBytes(buffer,bufferSize);


     if(updateStarted==0){
        size=getFirmwareSize(buffer);
        Serial.print("---size--");
        Serial.println(size);
       bool canBegin = Update.begin(size);
       Serial.print("canBegine");

       Serial.print(canBegin);

    if(!canBegin)
  {
    FIRMWARE_UPDATE_STATUS = UPDATE_FAILED;
    Serial.println("Not enough space to begin OTA");
    return;
  }

      updateStarted=1;
      int sentData=SerialBT.write(buffer2,3);
      Serial.print("sent data");

     return;

  }

      int byteRemaining= size-written;
          Serial.print("byteRemaining ");
          Serial.println(byteRemaining);


        if(byteRemaining<bufferSize){
        Serial.print("writing last segment");
        Serial.print(byteRemaining);
        int t=Update.write(buffer,byteRemaining);
        written+=t;
         writtenCount=writtenCount+byteRemaining;
      }else{

         int t1=Update.write(buffer,bufferSize);
            written+=t1;
            Serial.print("---written in else---");
             Serial.print(written);
             writtenCount=writtenCount+bufferSize;
       }

      if(Update.isFinished()){
        Serial.print(" Finnished");
       }

    if(byteRemaining<bufferSize){

        if(Update.isFinished()){
          Serial.println("--------Updation Fineshed sucessfully............");
          Serial.println(written);
         int res= Update.end();
         Serial.print("Updated.end Response");
         Serial.print(res);
          ESP.restart();
        }

        return;
      }

      int sentData=SerialBT.write(buffer2,3);

}

}
int getFirmwareSize(byte buffer[]){
 int sizeOfByte=0;
        unsigned char bytes[10];
        for(int i=0;i<10;i++){
          if(buffer[i]!=47 && buffer[i]!=165){
            bytes[i]=buffer[i];
            sizeOfByte++;
          }
          if(buffer[i]==47)break;
        }
         char chars[sizeOfByte + 1];
         memcpy(chars, bytes, sizeOfByte);
         chars[sizeOfByte] = '\0';
         String s(chars);
       int size= s.toInt();

 return size;
}



//////////////////////////////////UPDATE OVER BLUETOOTH BLE//////////////////////////////////////////


void write_ota(uint8_t *data,int bufferLength){


  UPDATE_OTA_OVER_BLE=false;
   if (!updateFlag)
    {
      Serial.println("BeginOTA");
      esp_ota_begin(esp_ota_get_next_update_partition(NULL), OTA_SIZE_UNKNOWN, &otaHandler);

      Serial.print("esp_ota_begin()");
      updateFlag = true;
    }
      if (bufferLength > 0)
      {

       esp_err_t res= esp_ota_write(otaHandler,data,bufferLength);
       String temp=esp_err_to_name(res);
       Serial.print("-------response-----");
       Serial.println(temp);

        if (bufferLength<10)
        {
          esp_ota_end(otaHandler);
          Serial.println("EndOTA");
          if (ESP_OK == esp_ota_set_boot_partition(esp_ota_get_next_update_partition(NULL)))
          {
            delay(2000);
            esp_restart();
          }
          else
          {
            Serial.println("Upload Error");
          }
        }
      }
}
