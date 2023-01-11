#include <HardwareSerial.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLE2902.h>
#include <ArduinoJson.h>
#include <string.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <WiFiClient.h>
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
#include "config.h"
#include "FIRMIR.h"

Preferences preferences;
int heat_on;
String mqtt_username="";
String mqtt_password="eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJkYXRhIjp7ImVtYWlsSWQiOiJ5ZGhlZXJhanJhb0BnbWFpbC5jb20iLCJ1c2VySWQiOiIwOGFhY2IwNy02MGM1LTQ5ZjAtYjViZS00MTVjZjBjNzk4M2UiLCJmaXJzdE5hbWUiOiJEaGVlcmFqIiwibGFzdE5hbWUiOiJSYW8ifSwiaWF0IjoxNjY5MjgzOTE2LCJleHAiOjE4MjY5NjM5MTZ9.4JzSp6J6Bz5_s75nLGuveEsT6Na1BOr1AmsZspY_HCE";
const char *mqtt_server = "dev-mqtt.api.sunlighten.com";

void load_env(){
  String env_from_storage = preferences.getString("env", "prod");
  if(env_from_storage=="dev"){
    mqtt_username= dev_mqtt_password;
    mqtt_password= dev_mqtt_password;
    mqtt_server = dev_mqtt_server;
}
 else if(env_from_storage=="stg"){
    mqtt_username= stg_mqtt_password;
    mqtt_password= stg_mqtt_password;
    mqtt_server = stg_mqtt_server;
  }
  else if(env_from_storage=="prod"){
    mqtt_username= prod_mqtt_password;
    mqtt_password= prod_mqtt_password;
    mqtt_server = prod_mqtt_server;
  }
  
  else{
    mqtt_username= prod_mqtt_password;
    mqtt_password= prod_mqtt_password;
    mqtt_server = prod_mqtt_server;
    
  }

}




BLECharacteristic *pWIFI_STATUS_Characteristic;
BLECharacteristic *pPB_TO_ACP_Characteristic;
String data_bt;


WiFiClient espClient;
PubSubClient client(espClient);


// NTP server to request epoch time
const char *ntpServer = "pool.ntp.org";
unsigned long epochTime;
const char *channel = "";

unsigned long getTime()
{
  time_t now;
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo))
  {

    return (0);
  }
  time(&now);
  return now;
}

// protocol variables
int version_no = 1;
String cmd_code = "";
int pdt_code = 1;
String my_data;
String readString;
int i = 0;
char c;
const char *first_com;
const char *second_com;
const char *third_com;
//*******************************
// Dip switch values
int way0 = 0;
int way1 = 0;
int way2 = 0;
int acvol = 0;
//*******************************

boolean isExtLight;
boolean isIntLight;
// firmware update status
int UPDATE_NOT_INITIALIZE = 0;
int UPDATE_FOUND = 1;
int UPDATE_IN_PROCESS = 2;
int UPDATE_SUCCESSFULLY_DONE = 3;
int UPDATE_FAILED = 4;
int FIRMWARE_UPDATE_STATUS = UPDATE_NOT_INITIALIZE;
//**************************************

// update ota
bool UPDATE_OTA = false;
bool UPDATE_OTA_OVER_BLE = false;
bool UPDATE_OTA_OVER_CLASSIC = false;
//***************************************

// heaters intensity(0-10)
int all_mir_inten = 0;
int all_fir_inten = 0;
int all_nir_inten =0;
int LF_inten = 0;
int LM_inten = 0;
int F_inten = 0;
int FM_inten = 0;
int FF_inten = 0;
int BF_inten = 0;
int BM_inten = 0;
int RF_inten = 0;
int RM_inten = 0;
// nir flag (0/1)
int left_nir_flag = 0;
int right_nir_flag = 0;
int back_nir_flag = 0;

// heaters current sensors value

int Ac_Volt = 0;
// temperature
int cabin_temp_in = 0;
int set_temp = 0;
int ntc_avg = 0;
int Heaters_on = 0;
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
String getOutTopic(void)
{
  return String("saunabridge/" + device_id + "/out");
}

String getInTopic()
{
  return String("saunabridge/" + device_id + "/in");
}

String getStatsTopic()
{
  return String("saunabridge/" + device_id + "/stats");
}



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

void  changeEnv(String _env){
  if (_env == "dev" || _env == "stg" || _env == "prod" )
  {
    preferences.putString("env", _env);
    String env = _env;
    Serial.println("Rebooting to change the env");
    ESP.restart();

    
  }else{
    Serial.println("Invalid env value. Should be one of dev, stg or prod");
  }
  
  
}

void saveWifiDetailsLocally(String ssid, String wifi_password)
{

  preferences.putString("wi_ssid", ssid);
  preferences.putString("wi_pass", wifi_password);
  Serial.println("Wifi Details saved locally");
}

void print_values(int t, int t2)
{

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

class ACP_TO_PB_CALLBACK : public BLECharacteristicCallbacks
{
  void onWrite(BLECharacteristic *pCharacteristic)
  {
    std::string rxValue = pCharacteristic->getValue();
    if (rxValue.length() > 0)
    {
      process_cmd(rxValue.c_str());
    }
  }
};

class PB_TO_ACP_CALLBACK : public BLECharacteristicCallbacks
{

  void onRead(BLECharacteristic *pCharacteristic)
  {
    pPB_TO_ACP_Characteristic->setValue(data_bt.c_str());
  }
};

class UPDATE_FIRMWARE_CALLBACK : public BLECharacteristicCallbacks
{
  void onWrite(BLECharacteristic *pCharacteristic)
  {
    std::string rxValue = pCharacteristic->getValue();
    Serial.println(rxValue.c_str());
    Serial.print("----------------------update the firmaware--------------");
    if (rxValue.length() > 0)
    {

      UPDATE_OTA = true;
      Serial.println("update write");
      Serial.print(FIRMWARE_VERSION);
    }
  }
};

std::string rxData;
void write_ota(uint8_t *data, int bufferLength);
class OTA_OVER_BLE_CALLBACK : public BLECharacteristicCallbacks
{
  void onWrite(BLECharacteristic *pCharacteristic)
  {
    Serial.print("--------------------------write method of onWrite is called------------------");

    rxData = pCharacteristic->getValue();
    uint8_t data[10];
    write_ota(data, 2);
    UPDATE_OTA_OVER_BLE = true;

    return;
  };
};

void BLE_Services()
{
  Serial.println("Starting BLE Services!");
  // TODO: ADD SAUNA ID TO THE BT BROADCAST NAME
  BLEDevice::init("SunlightenSauna-Mpulse");
  BLEServer *pServer = BLEDevice::createServer();

  BLEService *deviceIdService = pServer->createService(DEVICE_INFORMATION_SERVICE_UUID);
  deviceIdService->start();
  BLECharacteristic *deviceIdSerialNumberCharacteristic = deviceIdService->createCharacteristic(
      SERIAL_NUMBER_STRING_CHARACTERISTIC_UUID,
      BLECharacteristic::PROPERTY_READ |
          BLECharacteristic::PROPERTY_NOTIFY);
  deviceIdSerialNumberCharacteristic->setValue(device_id.c_str());

  // Set firmware Revision Number
  BLECharacteristic *firmwareRevisionCharacteristic = deviceIdService->createCharacteristic(
      FIRMWARE_REVISION_STRING_CHARACTERISTIC_UUID,
      BLECharacteristic::PROPERTY_READ |
          BLECharacteristic::PROPERTY_NOTIFY);
  firmwareRevisionCharacteristic->setValue(String(FIRMWARE_VERSION).c_str());
  // Set the hardware revision number
  BLECharacteristic *hardwareRevisionCharacteristic = deviceIdService->createCharacteristic(
      HARDWARE_REVISION_STRING_CHARACTERISTIC_UUID,
      BLECharacteristic::PROPERTY_READ |
          BLECharacteristic::PROPERTY_NOTIFY);
  hardwareRevisionCharacteristic->setValue(String(HARDWARE_VERSION).c_str());
  deviceIdService->start();
  // update firmware through the web
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

  // update firmware through BLE
  BLEService *pFirmware_Update_Over_BLE = pServer->createService(FIRMWARE_UPDATE_OVER_BLE_UUID);
  BLECharacteristic *pFIRMWARE_UPDATE_OVER_BLE_Characteristic = pFirmware_Update_Over_BLE->createCharacteristic(
      FIRMWARE_UPDATE_OVER_BLE_Characteristic,
      BLECharacteristic::PROPERTY_READ |
          BLECharacteristic::PROPERTY_WRITE |
          BLECharacteristic::PROPERTY_NOTIFY);
  pFIRMWARE_UPDATE_OVER_BLE_Characteristic->addDescriptor(new BLE2902());
  pFIRMWARE_UPDATE_OVER_BLE_Characteristic->setCallbacks(new OTA_OVER_BLE_CALLBACK());
  pFirmware_Update_Over_BLE->start();

  // make communication between ACP and PB
  BLEService *pCommand_Service = pServer->createService(COMMAND_SERVICE_UUID);
  BLECharacteristic *pACP_TO_PB_Characteristic = pCommand_Service->createCharacteristic(
      ACP_TO_PB_CHARACTERISTIC_UUID,
      BLECharacteristic::PROPERTY_READ |
          BLECharacteristic::PROPERTY_WRITE |
          BLECharacteristic::PROPERTY_NOTIFY);
  pACP_TO_PB_Characteristic->addDescriptor(new BLE2902());
  pACP_TO_PB_Characteristic->setCallbacks(new ACP_TO_PB_CALLBACK());

  BLECharacteristic *pPB_TO_ACP_Characteristic = pCommand_Service->createCharacteristic(
      PB_TO_ACP_CHARACTERISTIC_UUID,
      BLECharacteristic::PROPERTY_READ |
          BLECharacteristic::PROPERTY_WRITE |
          BLECharacteristic::PROPERTY_NOTIFY);
  pPB_TO_ACP_Characteristic->addDescriptor(new BLE2902());
  pPB_TO_ACP_Characteristic->setCallbacks(new PB_TO_ACP_CALLBACK());

  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(DEVICE_INFORMATION_SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06);
  pAdvertising->setMinPreferred(0x12);
  BLEDevice::startAdvertising();
}
void wifiSetupOnStart()
{
  Serial.println("Trying to setup wifi with saved wifi details");
  String DEFAULT_WIFI_CREDS = "-----";
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

void setup()
{
  load_env();
  generate_serial_id();
  unsigned char i;
  Serial.begin(115200);
  Serial1.begin(115200, SERIAL_8N1, RXD2, TXD2);
  Serial2.begin(115200, SERIAL_8N1, RXD1, TXD1);
  Serial.println("Device ID: " + device_id);
  preferences.begin("prefs", false);
  Serial.print("---------Firmware Version----------- ");
  Serial.print(FIRMWARE_VERSION);
  Serial.println();
  disableCore0WDT();
  wifiSetupOnStart();

  BLE_Services();
  Serial.println("--------Bluetooth Device is Ready to Pair-------");

  pinMode(Intlight, OUTPUT);
  pinMode(Extlight, OUTPUT);
  pinMode(Wifi_on, OUTPUT);
  pinMode(BT_ON, OUTPUT);
  pinMode(s0, OUTPUT);
  pinMode(s1, OUTPUT);
  pinMode(s2, OUTPUT);
  pinMode(s3, OUTPUT);
  digitalWrite(s0, LOW);
  digitalWrite(s1, LOW);
  digitalWrite(s2, LOW);
  digitalWrite(s3, LOW);
  if (WiFi.status() == WL_CONNECTED)
  {
    configTime(0, 0, ntpServer);
    setup_mqtt_connection();
  }
}
void setup_mqtt_connection()
{
  if (WiFi.status() == WL_CONNECTED)
  {
    client.setServer(mqtt_server, 1883);
    client.setCallback(mqtt_subscribe_callback);
  }
}

void updateWifiStatus()
{

  if (WiFi.status() == WL_CONNECTED)
  {
    wifi_status = 1;
    digitalWrite(Wifi_on, HIGH);
    if (!client.connected())
    {
      mqtt_reconnect();
    }
    client.loop();
  }
  if (WiFi.status() == WL_DISCONNECTED)
  {
    wifi_status = 0;
    digitalWrite(Wifi_on, LOW);
    wifiSetupOnStart();
  }
}
uint8_t msg[500];
bool updatingFirmware_Bluetooth = false;

void loop()
{
  updateWifiStatus();
  Serial_received();
  if (updatingFirmware_Bluetooth)
    return;
  epochTime = getTime();
  send_raw_data();
}

void mqtt_reconnect()
{
  // Loop until we're reconnected
  while ((!client.connected() && WiFi.status() == WL_CONNECTED))
  {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "pbv7-";
    clientId += device_id;
    // Attempt to connect
    if (client.connect(clientId.c_str(), mqtt_username.c_str(), mqtt_password.c_str()))
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

void progress(int t, int t2)
{
  Serial.print("----onProgram---");
  Serial.print(t);
  Serial.print(t2);
  Serial.println("");
}

void Serial_received(void)
{
  if (Serial1.available())
  {
    while (Serial1.available())
    {
      c = Serial1.read();
      readString += c;
      delay(10);
    }
  }

  else if (Serial2.available())
  {
    while (Serial2.available())
    {
      c = Serial2.read();
      readString += c;
      delay(10);
    }
  }

  if (readString.length() > 1)
  {
    Serial.print(readString);
    process_cmd(readString);
    readString = "";
  }
}

int readMux(int channel, int sig_pin, int cmd)
{
  int controlPin[] = {s0, s1, s2, s3};

  int muxChannel[16][4] = {
      {0, 0, 0, 0}, // channel 0
      {1, 0, 0, 0}, // channel 1
      {0, 1, 0, 0}, // channel 2
      {1, 1, 0, 0}, // channel 3
      {0, 0, 1, 0}, // channel 4
      {1, 0, 1, 0}, // channel 5
      {0, 1, 1, 0}, // channel 6
      {1, 1, 1, 0}, // channel 7
      {0, 0, 0, 1}, // channel 8
      {1, 0, 0, 1}, // channel 9
      {0, 1, 0, 1}, // channel 10
      {1, 1, 0, 1}, // channel 11
      {0, 0, 1, 1}, // channel 12
      {1, 0, 1, 1}, // channel 13
      {0, 1, 1, 1}, // channel 14
      {1, 1, 1, 1}  // channel 15
  };

  // loop through the 4 sig
  for (int i = 0; i < 4; i++)
  {
    digitalWrite(controlPin[i], muxChannel[channel][i]);
  }
  if (cmd == 1)
  {
    int totalval = 0;
    for (int i = 0; i < 10; i++)
    {
      int val = analogRead(sig_pin);
      totalval = totalval + val;
    }
    int avg = totalval / 10;
    return avg;
  }
  else if (cmd == 2)
  {
    int totalval = 0;
    for (int i = 0; i < 16; i++)
    {
      int val = analogRead(sig_pin);
      totalval = totalval + val;
    }
    int avg = totalval / 16;
    return avg;
  }
  else if (cmd == 3)
  {
    int val = digitalRead(sig_pin);
    return val;
  }
}
void read_raw(void)
{
  for (i = 0; i < 16; i++)
  {

    NTC[i] = readMux(i, MUX[1], 1);
    NTC[i + 16] = readMux(i, MUX[2], 1);
  }
  for (i = 0; i < 10; i++)
  {
    CURRENT[i] = readMux(i, MUX[0], 2);
  }

  WAY2 = readMux(0, MUX[3], 3);
  WAY1 = readMux(1, MUX[3], 3);
  WAY0 = readMux(2, MUX[3], 3);
  AC_VOLT = readMux(4, MUX[3], 3);
  smoke_d = readMux(4, MUX[3], 3);
  power_d = readMux(11, MUX[3], 3);
}

void turn_on_Heaters(int Heater_pin,int Heater_on){
  if(Heater_on){
    digitalWrite(Heater_pin,HIGH);

  }
  else{
    digitalWrite(Heater_pin,LOW);
  }

}


void FIR_Heater_inten(int temp,int heater_int,int,int Heater_pin)
{

    switch(heater_int)
    {
            case   1:
            {
                heat_on=heat_temp_pro(temp,FIR_1_TH,FIR_1_TL);
                turn_on_Heaters(Heater_pin,heat_on);
                break;          
            }       
            case   2:
            {
                heat_on=heat_temp_pro(temp,FIR_2_TH,FIR_2_TL);
                turn_on_Heaters(Heater_pin,heat_on);
                break;          
            }    
            case   3:
            {
                heat_on=heat_temp_pro(temp,FIR_3_TH,FIR_3_TL);
                turn_on_Heaters(Heater_pin,heat_on);
                break;          
            } 
            
            case   4:
            {
                heat_on=heat_temp_pro(temp,FIR_4_TH,FIR_4_TL);
                turn_on_Heaters(Heater_pin,heat_on);
                break;          
            }    
            
            case   5:
            {
                heat_on=heat_temp_pro(temp,FIR_5_TH,FIR_5_TL);
                turn_on_Heaters(Heater_pin,heat_on);
                break;          
            } 
            
            case   6:
            {
                heat_on=heat_temp_pro(temp,FIR_6_TH,FIR_6_TL);
                turn_on_Heaters(Heater_pin,heat_on);
                break;          
            }       
            case   7:
            {
                heat_on=heat_temp_pro(temp,FIR_7_TH,FIR_7_TL);
                turn_on_Heaters(Heater_pin,heat_on);
                break;          
            }    
            case   8:
            {
                heat_on=heat_temp_pro(temp,FIR_8_TH,FIR_8_TL);
                turn_on_Heaters(Heater_pin,heat_on);
                break;          
            } 
            
            case   9:
            {
                heat_on=heat_temp_pro(temp,FIR_9_TH,FIR_9_TL);
                turn_on_Heaters(Heater_pin,heat_on);
                break;          
            }    
            
            case   10:
            {
                heat_on=heat_temp_pro(temp,FIR_10_TH,FIR_10_TL);
                turn_on_Heaters(Heater_pin,heat_on);
                break;          
            } 
            
            default:
            {
                heat_on=0;
                turn_on_Heaters(Heater_pin,heat_on);
                break; 
            }
    }

}

void heat_temp_pro(unsigned int temp ,unsigned  int  TEM_H,unsigned  int TEM_L)
{
    if(temp<TEM_H)   return 0;
    else
    {
        if(temp>TEM_L)  return 1;
    
    }

}
void MIR_Heater_inten(int temp,int heater_int,int,int Heater_pin)
{

    switch(heater_int)
    {
            case   1:
            {
                heat_on=heat_temp_pro(temp,MIR_1_TH,MIR_1_TL);
                turn_on_Heaters(Heater_pin,heat_on);
                break;          
            }       
            case   2:
            {
                heat_on=heat_temp_pro(temp,MIR_2_TH,MIR_2_TL);
                turn_on_Heaters(Heater_pin,heat_on);
                break;          
            }    
            case   3:
            {
                heat_on=heat_temp_pro(temp,MIR_3_TH,MIR_3_TL);
                turn_on_Heaters(Heater_pin,heat_on);
                break;          
            } 
            
            case   4:
            {
                heat_on=heat_temp_pro(temp,MIR_4_TH,MIR_4_TL);
                turn_on_Heaters(Heater_pin,heat_on);
                break;          
            }    
            
            case   5:
            {
                heat_on=heat_temp_pro(temp,MIR_5_TH,MIR_5_TL);
                turn_on_Heaters(Heater_pin,heat_on);
                break;          
            } 
            
            case   6:
            {
                heat_on=heat_temp_pro(temp,MIR_6_TH,MIR_6_TL);
                turn_on_Heaters(Heater_pin,heat_on);
                break;          
            }       
            case   7:
            {
                heat_on=heat_temp_pro(temp,MIR_7_TH,MIR_7_TL);
                turn_on_Heaters(Heater_pin,heat_on);
                break;          
            }    
            case   8:
            {
                heat_on=heat_temp_pro(temp,MIR_8_TH,MIR_8_TL);
                turn_on_Heaters(Heater_pin,heat_on);
                break;          
            } 
            
            case   9:
            {
                heat_on=heat_temp_pro(temp,FIR_9_TH,FIR_9_TL);
                turn_on_Heaters(Heater_pin,heat_on);
                break;          
            }    
            
            case   10:
            {
                heat_on=heat_temp_pro(temp,MIR_10_TH,MIR_10_TL);
                turn_on_Heaters(Heater_pin,heat_on);
                break;          
            } 
            
            default:
            {
                heat_on=0;
                turn_on_Heaters(Heater_pin,heat_on);
                break; 
            }
    }

}

void start_custom_program(){

}














void start_custom_program(){
FIR_Heater_inten(NTC[2],BF_inten,Heaters_pin[0]);
MIR_Heater_inten(NTC[1],BM_inten,Heaters_pin[1]);
FIR_Heater_inten(NTC[23],LF_inten,Heaters_pin[2]);
MIR_Heater_inten(NTC[22],LM_inten,Heaters_pin[3]);
FIR_Heater_inten(NTC[29],RF_inten,Heaters_pin[4]);
MIR_Heater_inten(NTC[28],RM_inten,Heaters_pin[5]);
FIR_Heater_inten(NTC[19],F_inten,Heaters_pin[6]);
FIR_Heater_inten(NTC[17],FF_inten,Heaters_pin[7]);
FIR_Heater_inten(NTC[16],FM_inten,Heaters_pin[8]);
turn_on_Heaters(Heaters_pin[9],left_nir_flag);
turn_on_Heaters(Heaters_pin[10],right_nir_flag);
turn_on_Heaters(Heaters_pin[11],back_nir_flag);

}

void start_predefined_program(){
FIR_Heater_inten(NTC[2],all_fir_inten,Heaters_pin[0]);
MIR_Heater_inten(NTC[1],all_mir_inten,Heaters_pin[1]);
FIR_Heater_inten(NTC[23],all_fir_inten,Heaters_pin[2]);
MIR_Heater_inten(NTC[22],all_mir_inten,Heaters_pin[3]);
FIR_Heater_inten(NTC[29],all_fir_inten,Heaters_pin[4]);
MIR_Heater_inten(NTC[28],all_mir_inten,Heaters_pin[5]);
FIR_Heater_inten(NTC[19],all_fir_inten,Heaters_pin[6]);
FIR_Heater_inten(NTC[17],all_mir_inten,Heaters_pin[8]);
turn_on_Heaters(Heaters_pin[9],all_nir_flag);
turn_on_Heaters(Heaters_pin[10],all_nir_flag);
turn_on_Heaters(Heaters_pin[11],all_nir_flag);

}









void send_info(void)
{
  protocol["v"] = version_no;
  protocol["cmd"]["product_code"] = pdt_code;
  protocol["cmd"]["cmd_code"] = "SEND_PCB_INFO";
  protocol["cmd"]["cmd_metadata"]["WAY0"] = way0;
  protocol["cmd"]["cmd_metadata"]["WAY1"] = way1;
  protocol["cmd"]["cmd_metadata"]["WAY2"] = way2;
  protocol["cmd"]["cmd_metadata"]["AC_VOL"] = acvol;
  protocol["cmd"]["cmd_metadata"]["FIRMWARE_V"] = FIRMWARE_VERSION;
  protocol["cmd"]["cmd_metadata"]["HARDWARE_V"] = HARDWARE_VERSION;
  protocol["cmd"]["cmd_metadata"]["Device_ID"] = device_id;
  protocol["cmd"]["cmd_metadata"]["WIFI_STATUS"] = wifi_status;
  send_data();
}
void send_raw_data(void)
{
  read_raw();
  protocol["v"] = version_no;
  protocol["cmd"]["product_code"] = pdt_code;
  protocol["cmd"]["cmd_code"] = "SEND_RAW_DATA";
  for (i = 0; i < 32; i++)
  {
    protocol["cmd"]["cmd_metadata"]["NTC"][i] = NTC[i];
  }
  protocol["cmd"]["cmd_metadata"]["LF_CURRENT"] = CURRENT[0];
  protocol["cmd"]["cmd_metadata"]["LM_CURRENT"] = CURRENT[1];
  protocol["cmd"]["cmd_metadata"]["RF_CURRENT"] = CURRENT[2];
  protocol["cmd"]["cmd_metadata"]["RM_CURRENT"] = CURRENT[3];
  protocol["cmd"]["cmd_metadata"]["F_CURRENT"] = CURRENT[4];
  protocol["cmd"]["cmd_metadata"]["FF_CURRENT"] = CURRENT[5];
  protocol["cmd"]["cmd_metadata"]["FM_CURRENT"] = CURRENT[6];
  protocol["cmd"]["cmd_metadata"]["BF_CURRENT"] = CURRENT[7];
  protocol["cmd"]["cmd_metadata"]["BM_CURRENT"] = CURRENT[8];
  protocol["cmd"]["cmd_metadata"]["AC_VOLT"] = Ac_Volt;
  protocol["cmd"]["cmd_metadata"]["WIFI_STATUS"] = wifi_status;
  send_data();
}

void send_data(void)
{
  protocol["ack"]["1"] = first_com;
  protocol["ack"]["2"] = second_com;
  protocol["ack"]["3"] = third_com;
  protocol["metadata"]["timestamp"] = epochTime;
  if (String(channel) == "<RS232>")
  {
    protocol["metadata"]["ch"] = "<RS232>";
    if (Serial1.availableForWrite())
    {
      serializeJson(protocol, Serial1);
      Serial1.println();
    }
  }
  else if (String(channel) == "<BT>")
  {
    protocol["metadata"]["ch"] = "<BT>";
    String data_bt = "";
    serializeJson(protocol, data_bt);
  }
  else if (String(channel) == "<WIFI>")
  {
    protocol["metadata"]["ch"] = "<WIFI>";
    serializeJson(protocol, my_data);
    my_data = my_data + "\n";
    client.publish(getOutTopic().c_str(), my_data.c_str());
  }
  protocol.clear();
  received.clear();
  my_data = "";
}

void turnOnLight(boolean isIntLight, boolean isExtLight)
{

  if (isIntLight)
  {
    digitalWrite(Intlight, HIGH);

    Serial.print("Interior Light On");
  }
  else
  {

    digitalWrite(Intlight, LOW);

    Serial.print("Interior Light is OFF");
  }
  if (isExtLight)
  {
    digitalWrite(Extlight, HIGH);

    Serial.print("Exterior Light is On");
  }
  else
  {
    digitalWrite(Extlight, LOW);

    Serial.print("Exterior Light is  OFF");
  }
}
void process_cmd(String command)
{
  DeserializationError error = deserializeJson(received, command);
  command = "";
  if (error)
  {
    Serial.print(String("deserializeJson() failed: " + String(error.c_str())));
  }

  if (received["v"] == 1 && received["cmd"]["product_code"] == 1)
  {
    channel = received["metadata"]["ch"];
    if (received["cmd"]["cmd_code"] == "REQUEST_PCB_INFO")
    {
      third_com = second_com;
      second_com = first_com;
      first_com = "RPI";
      send_info();
    }
    else if (received["cmd"]["cmd_code"] == "START_CUSTOM_PROGRAM")
    {
      third_com = second_com;
      second_com = first_com;
      first_com = "SCP";
      cabin_temp_in = received["cmd"]["cmd_metadata"]["ACP_TEMP"];

      set_temp = received["cmd"]["cmd_metadata"]["SET_TEMP"];

      LF_inten = received["cmd"]["cmd_metadata"]["LF_INTEN"];

      LM_inten = received["cmd"]["cmd_metadata"]["LM_INTEN"];

      F_inten = received["cmd"]["cmd_metadata"]["F_INTEN"];

      FM_inten = received["cmd"]["cmd_metadata"]["FM_INTEN"];

      FF_inten = received["cmd"]["cmd_metadata"]["FF_INTEN"];

      BF_inten = received["cmd"]["cmd_metadata"]["BF_INTEN"];

      BM_inten = received["cmd"]["cmd_metadata"]["BM_INTEN"];

      RF_inten = received["cmd"]["cmd_metadata"]["RF_INTEN"];

      RM_inten = received["cmd"]["cmd_metadata"]["RM_INTEN"];

      left_nir_flag = received["cmd"]["cmd_metadata"]["LEFT_NIR_FLAG"];

      right_nir_flag = received["cmd"]["cmd_metadata"]["RIGHT_NIR_FLAG"];

      back_nir_flag = received["cmd"]["cmd_metadata"]["BACK_NIR_FLAG"];
      
     start_custom_program();
    }
    else if (received["cmd"]["cmd_code"] == "START_PREDEFINED_PROGRAM")
    {
      third_com = second_com;
      second_com = first_com;
      first_com = "SPP";
      cabin_temp_in = received["cmd"]["cmd_metadata"]["ACP_TEMP"];

      set_temp = received["cmd"]["cmd_metadata"]["SET_TEMP"];

      all_fir_inten = received["cmd"]["cmd_metadata"]["ALL_FIR_INTEN"];

      all_mir_inten = received["cmd"]["cmd_metadata"]["ALL_FIR_INTEN"];

      all_nir_flag = received["cmd"]["cmd_metadata"]["ALL_NIR_FLAG"];
      
      start_predefined_program();
    }
    else if (received["cmd"]["cmd_code"] == "SET_WIFI_CREDENTIAL")
    {
      third_com = second_com;
      second_com = first_com;
      first_com = "SWC";

      temp = received["cmd"]["cmd_metadata"]["set_ssid"];
      wifi_ssid = temp;
      temp = received["cmd"]["cmd_metadata"]["set_password"];
      wifi_password = temp;
      saveWifiDetailsLocally(wifi_ssid, wifi_password);
      wifiSetupOnStart();
      configTime(0, 0, ntpServer);
    }
    else if (received["cmd"]["cmd_code"] == "SET_LIGHT")
    {
      third_com = second_com;
      second_com = first_com;
      first_com = "SL";

      isIntLight = received["cmd"]["cmd_metadata"]["Set_Interior_light"];
      isExtLight = received["cmd"]["cmd_metadata"]["Set_Exterior_light"];
      turnOnLight(isIntLight, isExtLight);
    }
    else if (received["cmd"]["cmd_code"] == "TURN_OFF_HEATERS")
    {
      third_com = second_com;
      second_com = first_com;
      first_com = "TOH";
      Heaters_on = 0;
    }
    else if (received["cmd"]["cmd_code"] == "UPDATE_FIRMWARE")
    {
      third_com = second_com;
      second_com = first_com;
      first_com = "UF";
      UPDATE_OTA = received["cmd"]["cmd_metadata"]["Update_OTA"];
      UPDATE_OTA_OVER_BLE = received["cmd"]["cmd_metadata"]["Update_OTA_OVER_BLE"];
      UPDATE_OTA_OVER_CLASSIC = received["cmd"]["cmd_metadata"]["Update_OTA_OVER_CLASSIC"];
      if (UPDATE_OTA)
      {
        ota_over_web();
      }
      if (UPDATE_OTA_OVER_BLE)
      {
        UPDATE_OTA_OVER_BLE = false;
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
  UPDATE_OTA = false;

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
  Update.onProgress(print_values);
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

//////////////////////////////////UPDATE OVER BLUETOOTH BLE//////////////////////////////////////////

void write_ota(uint8_t *data, int bufferLength)
{

  UPDATE_OTA_OVER_BLE = false;
  if (!updateFlag)
  {
    Serial.println("BeginOTA");
    esp_ota_begin(esp_ota_get_next_update_partition(NULL), OTA_SIZE_UNKNOWN, &otaHandler);

    Serial.print("esp_ota_begin()");
    updateFlag = true;
  }
  if (bufferLength > 0)
  {

    esp_err_t res = esp_ota_write(otaHandler, data, bufferLength);
    String temp = esp_err_to_name(res);
    Serial.print("-------response-----");
    Serial.println(temp);

    if (bufferLength < 10)
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
