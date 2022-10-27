#include <DS18B20.h> //Call temperature sensor DS18B20 library
DS18B20 ds(14);      //Define the feet of the temperature sensor
#include "Arduino.h"
#include "esp_ota_ops.h"


#include <WiFi.h>
// BLE DECLERATIONS

#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLE2902.h>

// BLE DECLERATIONS END
#include <ArduinoUniqueID.h>
//******************************
#define DATA_COMMAND 0X40 //We use an IC TM1638 to drive the digital diode.
#define DISP_COMMAND 0x80 //Defines TM1638-related commands.
#define ADDR_COMMAND 0XC0 //

void init_TM1638(void);                                 //TM1638 initialization program
void Write_DATA(unsigned char add, unsigned char DATA); //TM1638 writes data subroutines.
unsigned char Read_key(void);                           //TM1638 reads data subroutines.
//**************************************

//***FIRMWARE_INFO_START
const String FIRMWARE_VERSION = "v0.0.3";
//***FIRMWARE_INFO_END
const String HARDWARE_VERSION = "v1.0.0";

void pro_funtion(void); //Read the status of the switch and act accordingly.

void pro_time(void);    //Time-based subroutine, 10 milliseconds
void pro_display(void); //Display digital diode subroutine.
void pro_pwm(void);     //Heating output, internal lamp and external lamp output subroutine.Control relay.

void sauna_temperature(void); //Sample the temperature of the sauna

//***********************************
int DIO = 21; //Defines three pins for ESP32 to communicate with TM1638.
int CLK = 22;
int STB = 19; //
//***********************************

int RELAY1_LED = 25; // Define the I/O port for heat indicator 1
int RELAY2_LED = 26; // Define the I/O port for heat indicator 2,red,100%
int RELAY1_OUT = 33; // Define the I/O port for heating 1
int RELAY2_OUT = 16; // Define the I/O port for heating 2

int LED_1 = 13; // Define the I/O port for led1,Can be used as a Bluetooth or WIFI indicator
int LED_2 = 02; // Define the I/O port for led2,Can be used as a Bluetooth or WIFI indicator

int RELAY2_BLUE = 04;  // Define the I/O port for heat indicator 2,blue,75%
int RELAY2_GREEN = 23; // Define the I/O port for heat indicator 2,green,50%
int RELAY2_N = 0;      // Define a variable , the gear of heating 2

//***********************************

int IN_LED = 17;  // Define the I/O port for IN led output,Control relay.
int OUT_LED = 32; // Define the I/O port for OUT led output,Control relay.

int temp_c_led = 18; // Define the I/O port for Centigrade's indicator light
int temp_f_led = 05; // Define the I/O port for Fahrenheit's indicator light

int appointment_led = 27; // Define the I/O port for  appointment led

int POWER_FLAG;      // Define a variable ,the status of the heating 1 switch
int POWER_FLAG_BACK; // Define a variable ,the status of the last heating 1 switch

int TURN_ON_FLAG; // Define a variable ,Heating on or not

int GRADE_UP;   // Define a variable ,Display temperature BCD code
int GRADE_DOWN; // Define a variable ,Display temperature BCD code
int TIME_COUNT; // Define a variable ,Heating time timer,1-60MIN

int GRADE_MAX = 0; // Define a variable ,Display temperature BCD code,A one hundred - bit,0 or 1

int TEMP_INC_FLAG;    // Define a variable ,the status of the temperature + switch
int TEMP_DEC_FLAG;    // Define a variable ,the status of the temperature - switch
int POWER_FLAG2;      // Define a variable ,the status of the heating 2 switch
int IN_LED_FLAG;      // Define a variable ,the status of the in led switch
int OUT_LED_FLAG = 0; // Define a variable ,the status of the out led switch
int TIME_INC_FLAG;    // Define a variable ,the status of the time + switch
int TIME_DEC_FLAG;    // Define a variable ,the status of the time - switch

int POWER_FLAG_on_time = 5;  // Define a variable ,the heating 1 switch's anti-jamming timer
int POWER_FLAG_off_time = 0; // Define a variable ,the heating 1 switch's anti-jamming timer

//*********************
int TIME_ALL_FLAG = 0;      // Define a variable ,the status of holding the Time + Switch and the time-switch simultaneously
int TEMP_ALL_FLAG = 0;      // Define a variable ,the status of holding the temperature + Switch and the temperature - switch simultaneously
int TIME_ALL_FLAG_BACK = 0; // Define a variable ,the last status of holding the Time + Switch and the time-switch simultaneously
int TEMP_ALL_FLAG_BACK = 0; // Define a variable ,the last status of holding the temperature + Switch and the temperature - switch simultaneously
int TEMP_FC_FLAG = 1;       // Define a variable ,One is Fahrenheit, zero is Celsius.

//***************************

int TEMP_INC_FLAG_BACK; // Define a variable ,the last status of the temperature + switch
int TEMP_DEC_FLAG_BACK; // Define a variable ,the last status of the temperature - switch
int POWER_FLAG2_BACK;   // Define a variable ,the last status of the heating 2 switch
int IN_LED_FLAG_BACK;   // Define a variable ,the last status of the in led switch
int TIME_INC_FLAG_BACK; // Define a variable ,the last status of the time + switch
int TIME_DEC_FLAG_BACK; // Define a variable ,the last status of the time - switch

int TIME_100ms;           // Define a variable ,100ms
int TIME_1000ms;          // Define a variable ,1000ms
int TIME_INC_SWTICH_LONG; // Define a variable ,the time + switch Long press counter
int TIME_DEC_SWTICH_LONG; // Define a variable ,the time - switch Long press counter

int TIME_LONG; // Define a variable ,100ms

int RELAY1_flag; // Define a variable,RELAY1 flag
int RELAY2_flag; // Define a variable,RELAY2 flag

int RELAY1_flag_back = 0; // Define a variable,the last status of RELAY1 flag

int LED_1_FLAG = 0; // Define a variable,LED_1 FLAG
int LED_2_FLAG = 0; // Define a variable,LED_2 FLAG

//***************************
byte set_temp = 75;                    //Define a variable,Set the temperature in degrees Celsius
byte set_f_temp = 170;                 //Define a variable,Set the temperature in degrees Fahrenheit
unsigned int settemp_display_time = 0; //Define a variable,Display set temperature within 30 seconds
unsigned int TEMP_INC_SWTICH_LONG = 0; //Define a variable,the counter of long press temperature + switch
unsigned int TEMP_DEC_SWTICH_LONG = 0; //Define a variable,the counter of long press temperature - switch

unsigned char read_c0 = 0; //Define a variable,Read data from TM1638
unsigned char read_c1 = 0; //Define a variable,Read data from TM1638
unsigned char read_c2 = 0; //Define a variable,Read data from TM1638
unsigned char read_c3 = 0; //Define a variable,Read data from TM1638

//*******************************************
float float_currenttemp = 0;         //Define a variable,Read the current temperature, floating point number
unsigned int single_currenttemp = 0; //Define a variable,the current temperature,the integer
unsigned int all_currenttemp = 0;    //Define a variable,the sum of the current temperature

int sample_tem_n = 0;        //Define a variable,the number of times the temperature is sampled
byte actual_temperature = 0; //Define a variable,the actual temperature sampled in degrees Celsius

byte actual_f_temperature = 0; //Define a variable,the actual temperature sampled in degrees Fahrenheit

unsigned int sample_n = 0; //Define a variable,the temperature was sampled every 25 times
//********************************

#define appointment_com 4136
//#define  appointment_com  247198  //Define a constant, the number of 10 milliseconds required per hour.The time error that has been considered.

unsigned int appointment_h_time = 24;                      //Define a variable,Number of hours booked
unsigned long int appointment_10ms_time = appointment_com; //Define a variable,the number of 10 milliseconds required per hour.The time error that has been considered.

unsigned int appointment_flag = 0; //Define a variable,Flag bit of appointment mode
//*************************************
unsigned int on_4h_key_time = 0;                     //Define a variable,Represents the time that the heating 1 switch is held down
unsigned int on_4h_h_time = 4;                       //Define a variable,Number of hours of heating function，0-4H
unsigned long int on_4h_10ms_time = appointment_com; //Define a variable,the number of 10 milliseconds required per hour.The time error that has been considered.

int TIME_WORK_FLAG = 0; //0--60min    1--4h，Define a variable,Indicates whether the heat is being applied

unsigned int on_just_flag = 0; //Define a variable,Indicates whether the heating 1 switch is long pressed

//共阴数码管显示代码    Common Yin digital tube display code
unsigned char tab[] = {
    0x3F, //0
    0x06, //1
    0x5B, //2
    0x4F, //3
    0x66, //4
    0x6D, //5
    0x7D, //6
    0x07, //7
    0x7F, //8
    0x6F, //9
    0x77, //A
    0x7C, //B
    0x39, //C
    0x5E, //D
    0x79, //E
    0x71  //F
};
unsigned char num[8];      //各个数码管显示的值    Values displayed on each nixie tube
static unsigned char c[4]; //Define a variable, Read data from TM1638

//************************************************
void pro_uart(void);
unsigned int from_acp_heater_n = 0;
unsigned int from_acp_heater = 0;
byte command_heater_acp[13] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

byte command_tx_acp[13] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

byte command_sw = 0;
byte command_time_60 = 0;
byte command_time_4h = 0;
byte command_time_fc = 0;

byte command_temp_en = 0;
byte command_temp_set = 0;

byte command_time_reservation = 0;
//unsigned char  nnn=0;
static unsigned char nnn = 0;

unsigned int tx_time = 0;
void Write_COM(unsigned char cmd) //发送命令字  Send command word
{
  digitalWrite(STB, LOW);
  TM1638_Write(cmd);
  digitalWrite(STB, HIGH);
}

String device_id = String();

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

#define WIFI_SERVICE_UUID "0c40e5e8-5fed-4747-9fb0-877e78e78947"
#define WIFI_SSID_CHARACTERISTIC_UUID "13e0021f-c715-4abf-9422-96d62b05c87e"
#define WIFI_PASS_CHARACTERISTIC_UUID "13e0021f-c715-4abf-9422-96d62b05c87f"
#define WIFI_STATUS_CHARACTERISTIC_UUID "13e0021f-c715-4abf-9422-96d62b05c87a"
#define DEVICE_INFORMATION_SERVICE_UUID "0000180a-0000-1000-8000-00805f9b34fb"
#define SERIAL_NUMBER_STRING_CHARACTERISTIC_UUID "00002a25-0000-1000-8000-00805f9b34fb"
#define FIRMWARE_REVISION_STRING_CHARACTERISTIC_UUID "00002a26-0000-1000-8000-00805f9b34fb"
#define HARDWARE_REVISION_STRING_CHARACTERISTIC_UUID "00002a27-0000-1000-8000-00805f9b34fb"
/*
https://programmaticponderings.com/2020/08/04/getting-started-with-bluetooth-low-energy-ble-and-generic-attribute-profile-gatt-specification-for-iot/

The sensor telemetry will be advertised by the Sense, over BLE, as a GATT Environmental Sensing Service (GATT Assigned Number 0x181A) with multiple GATT Characteristics. 
Each Characteristic represents a sensor reading and contains the most current sensor value(s), 
for example, Temperature (0x2A6E) or Humidity (0x2A6F).

Each GATT Characteristic defines how the data should be represented. To represent the data accurately, the sensor readings need to be modified. 
For example, using ArduinoHTS221 library, the temperature is captured with two decimal points of precision (e.g., 22.21 °C). 
However, the Temperature GATT Characteristic (0x2A6E) requires a signed 16-bit value (-32,768–32,767). 
To maintain precision, the captured value (e.g., 22.21 °C) is multiplied by 100 to convert it to an integer (e.g., 2221). 
The Raspberry Pi will then handle converting the value back to the original value with the correct precision.
*/

#define ENVIRONMENTAL_SENSING_SERVICE_UUID "0000181a-0000-1000-8000-00805f9b34fb"
#define TEMPERATURE_CHARACTERISTIC_UUID "00002a6e-0000-1000-8000-00805f9b34fb"

BLEService *environmentService;
BLECharacteristic *temperatureCharacteristic;
BLECharacteristic *pWIFI_STATUS_Characteristic;

String wifi_ssid = "";
String wifi_pass = "";
int wifi_status = 0;

void connect_to_wifi(String _ssid, String _wifi_password)
{
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
  delay(2000);
  Serial.println(WiFi.status());
  if (WiFi.waitForConnectResult() != WL_CONNECTED)
  {
    Serial.println((String) "Unable to connect to " + _ssid + " with pass " + _wifi_password + " , maybe because of incorrect credentials.");
    
  }
  else
  {
    Serial.println((String) "Connected to Wifi at ssid:  " + _ssid);
    wifi_status =1;
    // setup_mqtt_connection();
  }
  pWIFI_STATUS_Characteristic->setValue(wifi_status);
}


class WIFI_SSID_Callbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
      std::string rxValue = pCharacteristic->getValue();

      if (rxValue.length() > 0) {
        wifi_ssid = rxValue.c_str();
        Serial.println(wifi_ssid);
      }
    }
};
class WIFI_PASS_Callbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
      std::string rxValue = pCharacteristic->getValue();

      if (rxValue.length() > 0) {
        wifi_pass = rxValue.c_str();
        connect_to_wifi(wifi_ssid,wifi_pass);
        Serial.println(wifi_pass);
      }
    }
};

void BLE_Services()
{

  Serial.println("Starting BLE Services!");

  // TODO: ADD SAUNA ID TO THE BT BROADCAST NAME
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
  pWIFI_PASS_Characteristic->setValue(wifi_pass.c_str());
  pWIFI_PASS_Characteristic->addDescriptor(new BLE2902());
  pWIFI_PASS_Characteristic->setCallbacks(new WIFI_PASS_Callbacks());

  pWIFI_STATUS_Characteristic = pWIFI_Service->createCharacteristic(
      WIFI_STATUS_CHARACTERISTIC_UUID,
      BLECharacteristic::PROPERTY_READ |
          BLECharacteristic::PROPERTY_WRITE |
          BLECharacteristic::PROPERTY_NOTIFY |
          BLECharacteristic::PROPERTY_INDICATE |
          BLECharacteristic::PROPERTY_BROADCAST);
  pWIFI_STATUS_Characteristic->setValue(wifi_status);
  pWIFI_STATUS_Characteristic->addDescriptor(new BLE2902());

  pWIFI_Service->start();

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

  environmentService = pServer->createService(ENVIRONMENTAL_SENSING_SERVICE_UUID);
  temperatureCharacteristic = environmentService->createCharacteristic(
      TEMPERATURE_CHARACTERISTIC_UUID,
          BLECharacteristic::PROPERTY_READ |
          BLECharacteristic::PROPERTY_INDICATE |
          BLECharacteristic::PROPERTY_NOTIFY );
  temperatureCharacteristic->addDescriptor(new BLE2902()); //this will allow the clients to subscribe to the notification and indications
  environmentService->start();

  // BLEAdvertising *pAdvertising = pServer->getAdvertising();  // this still is working for backward compatibility
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(WIFI_SERVICE_UUID);
  pAdvertising->addServiceUUID(DEVICE_INFORMATION_SERVICE_UUID);
  pAdvertising->addServiceUUID(ENVIRONMENTAL_SENSING_SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06); // functions that help with iPhone connections issue
  pAdvertising->setMinPreferred(0x12);
  // https://specificationrefs.bluetooth.com/assigned-values/Appearance%20Values.pdf
  // Set device type to Radiant Panel Heater
  // pAdvertising->setAppearance(0x06C5); // Uncommenting this line is putting the arduino in reboot loop
  BLEDevice::startAdvertising();
}

#define FULL_PACKET 512
esp_ota_handle_t otaHandler = 0;
bool updateFlag = false;
bool readyFlag = false;
int bytesReceived = 0;
int timesWritten = 0;

// class MyServerCallbacks : public BLECharacteristicCallbacks
// {
//   // void onConnect(BLEServer* pServer) {
//   //   deviceConnected = true;
//   // };

//   // void onDisconnect(BLEServer* pServer) {
//   //   deviceConnected = false;
//   // }

//   void onWrite(BLECharacteristic *pCharacteristic)
//   {
//     std::string rxData = pCharacteristic->getValue();
//     if (!updateFlag)
//     { //If it's the first packet of OTA since bootup, begin OTA
//       Serial.println("BeginOTA");
//       esp_ota_begin(esp_ota_get_next_update_partition(NULL), OTA_SIZE_UNKNOWN, &otaHandler);
//       updateFlag = true;
//     }
//     if (_p_ble != NULL)
//     {
//       if (rxData.length() > 0)
//       {
//         esp_ota_write(otaHandler, rxData.c_str(), rxData.length());
//         if (rxData.length() != FULL_PACKET)
//         {
//           esp_ota_end(otaHandler);
//           Serial.println("EndOTA");
//           if (ESP_OK == esp_ota_set_boot_partition(esp_ota_get_next_update_partition(NULL)))
//           {
//             delay(2000);
//             esp_restart();
//           }
//           else
//           {
//             Serial.println("Upload Error");
//           }
//         }
//       }
//     }

//     uint8_t txData[5] = {1, 2, 3, 4, 5};
//     //delay(1000);
//     // pCharacteristic->setValue((uint8_t*)txData, 5);
//     // pCharacteristic->notify();
//   }
// };
// void otaCallback::onWrite(BLECharacteristic *pCharacteristic)
// {
//   std::string rxData = pCharacteristic->getValue();
//   if (!updateFlag)
//   { //If it's the first packet of OTA since bootup, begin OTA
//     Serial.println("BeginOTA");
//     esp_ota_begin(esp_ota_get_next_update_partition(NULL), OTA_SIZE_UNKNOWN, &otaHandler);
//     updateFlag = true;
//   }
//   if (_p_ble != NULL)
//   {
//     if (rxData.length() > 0)
//     {
//       esp_ota_write(otaHandler, rxData.c_str(), rxData.length());
//       if (rxData.length() != FULL_PACKET)
//       {
//         esp_ota_end(otaHandler);
//         Serial.println("EndOTA");
//         if (ESP_OK == esp_ota_set_boot_partition(esp_ota_get_next_update_partition(NULL)))
//         {
//           delay(2000);
//           esp_restart();
//         }
//         else
//         {
//           Serial.println("Upload Error");
//         }
//       }
//     }
//   }

//   uint8_t txData[5] = {1, 2, 3, 4, 5};
//   //delay(1000);
//   // pCharacteristic->setValue((uint8_t*)txData, 5);
//   // pCharacteristic->notify();
// }

int connection_code = 0;
void setup()
{
  generate_serial_id();
  unsigned char i;
  pinMode(STB, OUTPUT); //Let all three pins be output states，These three pins connect to TM1638
  pinMode(CLK, OUTPUT);
  pinMode(DIO, OUTPUT); //让三个脚都是输出状态

  pinMode(RELAY1_LED, OUTPUT); //Set I/O input and output
  pinMode(RELAY2_LED, OUTPUT);
  pinMode(RELAY1_OUT, OUTPUT);
  pinMode(RELAY2_OUT, OUTPUT);
  pinMode(IN_LED, OUTPUT);
  pinMode(OUT_LED, OUTPUT);

  pinMode(RELAY2_BLUE, OUTPUT);
  pinMode(RELAY2_GREEN, OUTPUT);

  pinMode(temp_c_led, OUTPUT);
  pinMode(temp_f_led, OUTPUT);

  pinMode(LED_1, OUTPUT);
  pinMode(LED_2, OUTPUT);

  pinMode(appointment_led, OUTPUT);

  // analogWriteFreq(500);    //2KHZ,最少100HZ
  //analogWriteRange(new_range)语句来更改

  //  analogWrite(PWM1, 128);    //1023----100%
  //  analogWrite(PWM2, 128);    //1023----100%

  digitalWrite(RELAY1_LED, HIGH);
  digitalWrite(RELAY2_LED, HIGH);

  digitalWrite(RELAY2_BLUE, HIGH);
  digitalWrite(RELAY2_GREEN, HIGH);

  digitalWrite(IN_LED, LOW);
  digitalWrite(OUT_LED, LOW);

  digitalWrite(temp_c_led, HIGH);
  digitalWrite(temp_f_led, HIGH);
  digitalWrite(appointment_led, HIGH);

  digitalWrite(RELAY1_OUT, LOW);
  digitalWrite(RELAY2_OUT, LOW);

  //  Serial.begin(9600);
  //  wdt_enable(WDTO_2S); //开启看门狗，并设置溢出时间为两秒
  //  init_TM1638();                             //初始化TM1638
  //
  ////  for(i=0;i<4;i++)
  ////  Write_DATA(i<<1,tab[0]);                 //初始化寄存器
  //  Write_DATA(0<<1,tab[6]);
  //  Write_DATA(1<<1,tab[0]);
  //
  //  Write_DATA(2<<1,tab[5]);
  //  Write_DATA(3<<1,tab[5]);

  Write_COM(0x80); //关闭显示 Turn off display all 8

  POWER_FLAG = 0; //Initialize register
  POWER_FLAG_BACK = 0;
  TURN_ON_FLAG = 0;

  TEMP_INC_FLAG = 0;
  TEMP_DEC_FLAG = 0;
  POWER_FLAG2 = 0;
  IN_LED_FLAG = 0;
  TIME_INC_FLAG = 0;
  TIME_DEC_FLAG = 0;

  TEMP_INC_FLAG_BACK = 0;
  TEMP_DEC_FLAG_BACK = 0;
  POWER_FLAG2_BACK = 0;
  IN_LED_FLAG_BACK = 0;
  TIME_INC_FLAG_BACK = 0;
  TIME_DEC_FLAG_BACK = 0;

  TIME_100ms = 0;
  TIME_1000ms = 0;

  TIME_INC_SWTICH_LONG = 0;
  TIME_DEC_SWTICH_LONG = 0;

  TIME_LONG = 0;

  Serial.begin(115200);

  Serial.println("Device ID: " + device_id);

  BLE_Services();
}
void loop()
{
  pro_funtion(); //Read the status of the switch and act accordingly.
                 //    wdt_reset();
  delay(10);
  pro_time();    //Time-based subroutine, 10 milliseconds
  pro_display(); //Display digital diode subroutine.
  pro_pwm();     //Heating output, internal lamp and external lamp output subroutine.Control relay.

  sauna_temperature(); //Sample the temperature of the sauna

  pro_uart();

  //********************

  if (LED_1_FLAG == 0)
    LED_1_FLAG = 1; //led1 and led2 can be used as a Bluetooth or WIFI indicator
  else
    LED_1_FLAG = 0;

  if (LED_2_FLAG == 0)
    LED_2_FLAG = 1;
  else
    LED_2_FLAG = 0;

  if (LED_1_FLAG == 0)
    digitalWrite(LED_1, HIGH);
  else
    digitalWrite(LED_1, LOW);

  if (LED_2_FLAG == 0)
    digitalWrite(LED_2, HIGH);
  else
    digitalWrite(LED_2, LOW);

  temperatureCharacteristic->setValue(String(actual_temperature).c_str());
  // Serial.print("Actual Temperature in C:");
  // Serial.println(String(actual_temperature).c_str());
}

//*********************************************
void sauna_temperature(void) //Sample the temperature of the sauna
{
  unsigned int i;
  unsigned int i_z;

  sample_n++;
  if (sample_n > 25)
  {
    sample_n = 0;

    ds.setResolution(9);
    float_currenttemp = ds.getTempC() + 6; //Read the temperature in degrees Celsius and add 6 degrees for temperature compensation
    single_currenttemp = (unsigned int)float_currenttemp;
    all_currenttemp = all_currenttemp + single_currenttemp;

    sample_tem_n++;

    if (sample_tem_n > 3)
    {
      sample_tem_n = 0;

      all_currenttemp = all_currenttemp / 4;
      actual_temperature = (byte)all_currenttemp; //Sample 4 times and take the average value
      all_currenttemp = 0;
    }

    //   Serial.println("TEMP:");                       //for test
    //   Serial.println(float_currenttemp);
    //   Serial.println(single_currenttemp);
    //   Serial.println(actual_temperature);
    //   Serial.println();

    if (TEMP_FC_FLAG == 0) //Calculate the BCD code for the temperature display
    {                      //c
      i = actual_temperature / 10 * 0x10 + actual_temperature % 10;

      GRADE_MAX = 0;
    }
    else
    { //f
      actual_f_temperature = actual_temperature * 1.8 + 32;
      if (actual_f_temperature > 166)
        actual_f_temperature = 170;

      if (actual_f_temperature < 100)
      {
        GRADE_MAX = 0;
        i = actual_f_temperature / 10 * 0x10 + actual_f_temperature % 10;
      }
      else
      {
        GRADE_MAX = 1;
        i_z = actual_f_temperature - 100;
        i = i_z / 10 * 0x10 + i_z % 10;
      }
    }
    GRADE_DOWN = i & 0x0f;
    GRADE_UP = (i >> 4) & 0x0f;
  }
}

void TM1638_Write(unsigned char DATA) //写数据函数 Write the data function
{
  unsigned char i;
  pinMode(DIO, OUTPUT_OPEN_DRAIN);
  for (i = 0; i < 8; i++)
  {
    digitalWrite(CLK, LOW);
    if (DATA & 0X01)
      digitalWrite(DIO, HIGH);
    else
      digitalWrite(DIO, LOW);
    DATA >>= 1;
    digitalWrite(CLK, HIGH);
  }
}
unsigned char TM1638_Read(void) //读数据函数  Read data function
{
  unsigned char i;
  unsigned char temp = 0;
  ;

  pinMode(DIO, INPUT); //设置为输入
  for (i = 0; i < 8; i++)
  {
    temp >>= 1;
    digitalWrite(CLK, LOW);

    if (digitalRead(DIO) == HIGH)
      temp |= 0x80;
    digitalWrite(CLK, HIGH);
  }
  return temp;
}
// void Write_COM(unsigned char cmd)		//发送命令字  Send command word
// {
//         digitalWrite(STB,LOW);
// 	TM1638_Write(cmd);
//         digitalWrite(STB,HIGH);
// }
unsigned char Read_key(void) //Read the state of each key
{
  unsigned char i, key_value = 0;
  // unsigned char  nnn=0;
  unsigned char mmm = 0;
  digitalWrite(STB, LOW);
  TM1638_Write(0x42); //读键扫数据 命令
  for (i = 0; i < 4; i++)
  {
    c[i] = TM1638_Read();
  }

  //        read_c0=TM1638_Read();
  //        read_c1=TM1638_Read();
  //        read_c2=TM1638_Read();
  //        read_c3=TM1638_Read();
  //
  digitalWrite(STB, HIGH); //4个字节数据合成一个字节
                           //	for(i=0;i<4;i++)
                           //	{
                           //            key_value|=c[i]<<i;
                           //        }

  //        Serial.println("IASY");
  //        Serial.println(c[0]);
  //        Serial.println(c[1]);
  //        Serial.println(c[2]);
  //        Serial.println(c[3]);
  //        Serial.println(key_value);

  //********************************************
  read_c0 = c[0];
  read_c1 = c[1];
  read_c2 = c[2];
  read_c3 = c[3];

  //      Serial.println("c[0-3]");
  //
  //
  // Serial.println(c[0]);
  delayMicroseconds(1);
  //      Serial.println(c[1]);
  //      Serial.println(c[2]);
  //      Serial.println(c[3]);

  //      Serial.println("read_c0-3");
  //      Serial.println(read_c0);
  //      Serial.println(read_c1);
  //      Serial.println(read_c2);
  //      Serial.println(read_c3);

  nnn = c[2];
  nnn = nnn & 0x10;
  if ((nnn == 16) || (command_sw & 0x04))
    TIME_INC_FLAG = 1; //time + key
  else
    TIME_INC_FLAG = 0;

  nnn = c[0];
  nnn = nnn & 0x04;
  if ((nnn == 4) || (command_sw & 0x08))
    TIME_DEC_FLAG = 1; //time - key
  else
    TIME_DEC_FLAG = 0;

  nnn = c[3];
  nnn = nnn & 0x10;
  if ((nnn == 16) || (command_sw & 0x01))
    POWER_FLAG = 1; //Heat 1 key
  else
    POWER_FLAG = 0;

  nnn = c[3];
  nnn = nnn & 0x01;
  if (nnn == 1)
    POWER_FLAG_off_time = 2;
  else
    POWER_FLAG_on_time = 5;

  if ((POWER_FLAG_on_time == 0) || (command_sw & 0x02))
    POWER_FLAG2 = 1; //Heat 2 key
  if ((POWER_FLAG_off_time == 0) && ((command_sw & 0x02) == 0))
    POWER_FLAG2 = 0;

  nnn = c[2];
  nnn = nnn & 0x01;
  if ((nnn == 1) || (command_sw & 0x10))
    TEMP_INC_FLAG = 1; //temp + key
  else
    TEMP_INC_FLAG = 0;

  nnn = c[0];
  nnn = nnn & 0x20;
  if ((nnn == 32) || (command_sw & 0x20))
    TEMP_DEC_FLAG = 1; //temp - key
  else
    TEMP_DEC_FLAG = 0;

  nnn = c[1];
  nnn = nnn & 0x10;
  if ((nnn == 16) || (command_sw & 0x40))
    IN_LED_FLAG = 1; //in led key
  else
    IN_LED_FLAG = 0;

  nnn = c[1];
  nnn = nnn & 0x01;
  if ((nnn == 1) || (command_sw & 0x80))
    OUT_LED_FLAG = 1; //out  led key
  else
    OUT_LED_FLAG = 0;

  nnn = read_c0;
  nnn = nnn & 0x04;

  mmm = read_c2;
  mmm = mmm & 0x10;
  if (((nnn == 4) && (mmm == 16)) || ((command_sw & 0x04) && (command_sw & 0x08)))
    TIME_ALL_FLAG = 1; //Time + and Time - key,The switch is pressed together
  else
    TIME_ALL_FLAG = 0;

  nnn = read_c0;
  nnn = nnn & 0x20;
  mmm = read_c2;
  mmm = mmm & 0x01;
  if (((nnn == 32) && (mmm == 1)) || ((command_sw & 0x10) && (command_sw & 0x20)))
    TEMP_ALL_FLAG = 1; //TEMP + and TEMP - key,The switch is pressed together
  else
    TEMP_ALL_FLAG = 0;

  //      Serial.println("TIME_ALL_FLAG:");
  //      Serial.println(TIME_ALL_FLAG);
  //
  //      Serial.println("c[0-3]");
  //      Serial.println(c[0]);
  //      Serial.println(c[1]);
  //      Serial.println(c[2]);
  //      Serial.println(c[3]);
  //
  //      Serial.println("read_c0-3");
  //      Serial.println(read_c0);
  //      Serial.println(read_c1);
  //      Serial.println(read_c2);
  //      Serial.println(read_c3);

  command_sw = 0;

  //********************************************

  //        for(i=0;i<8;i++)
  //        {
  //            if((0x01<<i)==key_value)
  //            break;
  //        }
  return i;
}
void Write_DATA(unsigned char add, unsigned char DATA) //指定地址写入数据Write data at the specified address
{
  Write_COM(0x44);
  digitalWrite(STB, LOW);
  TM1638_Write(0xc0 | add);
  TM1638_Write(DATA);
  digitalWrite(STB, HIGH);
}
void Write_allLED(unsigned char LED_flag) //控制全部LED函数，LED_flag表示各个LED状态Control all LED functions, LED_flag represents each LED state
{
  unsigned char i;
  for (i = 0; i < 8; i++)
  {
    if (LED_flag == 0)
    {
      Write_DATA(2 * 0 + 1, 1);
    }
    else
    {
      if (LED_flag & (1 << i))
        Write_DATA(2 * i + 1, 1);
      else
        Write_DATA(2 * i + 1, 0);
    }
  }
}

//TM1638初始化函数
void init_TM1638(void) //TM1638 initialization function
{
  unsigned char i;
  Write_COM(0x8b);         //亮度 (0x88-0x8f)8级亮度可调
  Write_COM(0x40);         //采用地址自动加1
  digitalWrite(STB, LOW);  //
  TM1638_Write(0xc0);      //设置起始地址
  for (i = 0; i < 16; i++) //传送16个字节的数据
    TM1638_Write(0x00);
  digitalWrite(STB, HIGH);
}

//**************************************
void pro_funtion(void) //Read the status of the switch and act accordingly.
{
  unsigned char switch_flag;

  switch_flag = Read_key(); //读按键值  // Read the key value

  //  Serial.println(switch_flag);
  //*********************************************************
  //      if(switch_flag==8)   POWER_FLAG=1;
  //      else       POWER_FLAG=0;

  //    Serial.println("POWER_FLAG88:");
  //    Serial.println(POWER_FLAG_BACK);
  //    Serial.println(POWER_FLAG);

  if (appointment_flag == 0)
  {                                                  //Absent reservation function
                                                     //      if((POWER_FLAG_BACK==0)&&(POWER_FLAG==1))
    if ((POWER_FLAG_BACK == 1) && (POWER_FLAG == 0)) //Heat 1 switch from OFF to ON
    {
      if (on_just_flag == 1)
      {
        on_just_flag = 0; //Long press the heater switch 1
      }
      else
      {
        if (TURN_ON_FLAG == 0)
        {
          TURN_ON_FLAG = 1; //If the sauna does not heat, it will start to heat.
          init_TM1638();    //初始化TM1638  Initialize TM1638

          //  for(i=0;i<4;i++)
          //  Write_DATA(i<<1,tab[0]);                 //初始化寄存器 Initialize register
          GRADE_UP = 0;
          GRADE_DOWN = 0;
          GRADE_MAX = 0;

          TIME_COUNT = 60;

          TIME_1000ms = 0;

          Write_DATA(0 << 1, tab[0]);
          Write_DATA(1 << 1, tab[0]);

          //Write_DATA(2<<1,tab[0]);
          Write_DATA(3 << 1, tab[0]);

          Write_DATA(4 << 1, tab[0]);

          RELAY1_flag = 1;
          RELAY2_flag = 1;
          //**************************
          on_4h_h_time = 4;
          on_4h_10ms_time = appointment_com;

          //**************************
        }
        else
        {
          TURN_ON_FLAG = 0; //If the sauna is heating, it will stop heating.
          //Write_COM(0x80);                 //关闭显示
          Write_DATA(0 << 1, 0); // Close the display
          Write_DATA(1 << 1, 0);

          Write_DATA(2 << 1, 0);
          Write_DATA(3 << 1, 0);

          Write_DATA(4 << 1, 0);

          RELAY1_flag = 0;
          RELAY2_flag = 0;

          digitalWrite(temp_c_led, HIGH); //Turn off the Celsius temperature indicator
          digitalWrite(temp_f_led, HIGH); //Turn off the Fahrenheit temperature indicator
        }
      }
    }
  }
  //************************************************************
  if (command_time_60 & 0x80)
  {
    command_time_60 = (command_time_60 & 0x7f);
    if ((command_time_60 < 61) && (command_time_60 > 0))
    {
      if (TIME_WORK_FLAG == 0)
      {
        TIME_COUNT = command_time_60;
        TIME_1000ms = 0;
      }
    }
  }

  if (command_time_4h & 0x80)
  {
    command_time_4h = (command_time_4h & 0x7f);

    if ((command_time_4h < 5) && (command_time_4h > 0))
    {
      if (TIME_WORK_FLAG == 1)
      {
        on_4h_h_time = command_time_4h;
        on_4h_10ms_time = appointment_com;
      }
    }
  }

  if (command_time_reservation & 0x80)
  {
    command_time_reservation = (command_time_reservation & 0x7f);

    if ((command_time_reservation < 37) && (command_time_reservation > 0))
    {
      if (appointment_flag == 1)
      {
        appointment_h_time = command_time_reservation;
        appointment_10ms_time = appointment_com;
      }
    }
  }

  if (command_time_fc & 0x80)
  {
    command_time_fc = (command_time_fc & 0x7f);
    if (command_time_fc == 0)
      TEMP_FC_FLAG = 0;
    if (command_time_fc == 1)
      TEMP_FC_FLAG = 1;
  }

  if (command_temp_en == 1)
  {
    command_temp_en = 0;
    if (TEMP_FC_FLAG == 1)
    {
      if ((command_temp_set < 171) && (command_temp_set > 99))
      {
        set_f_temp = command_temp_set;
        settemp_display_time = 300; //Display delay time of set temperature,3S
      }
    }

    if (TEMP_FC_FLAG == 0)
    {
      if ((command_temp_set < 76) && (command_temp_set > 39))
      {
        set_temp = command_temp_set;
        settemp_display_time = 300; //Display delay time of set temperature,3S
      }
    }
  }

  //**********************************************
  if (TURN_ON_FLAG == 1)
  { //The sauna is heating up

    if ((POWER_FLAG2_BACK == 0) && (POWER_FLAG2 == 1)) //Heat 2 switch from OFF to ON.
    {
      if (RELAY2_flag == 0)
        RELAY2_flag = 1; //Switch heating 2 function
      else
        RELAY2_flag = 0;
    }

    //RELAY2_flag=~RELAY2_flag;

    //      Serial.println("TIME_INC_FLAG:");
    //      Serial.println(TIME_INC_FLAG_BACK);
    //      Serial.println(TIME_INC_FLAG);

    if ((TIME_INC_FLAG_BACK == 0) && (TIME_INC_FLAG == 1)) //Time + switch from OFF to ON.
    {
      if (TIME_WORK_FLAG == 0)
      {
        if (TIME_COUNT < 60)
        {
          TIME_COUNT++;
          TIME_1000ms = 0;
        } //60 minutes heating mode, time + switch timing.
      }
      else
      {
        if (on_4h_h_time < 4)
        {
          on_4h_h_time++;
          on_4h_10ms_time = appointment_com;
        } //4 hours heating mode, time + switch timing.
      }
    }

    if ((TIME_DEC_FLAG_BACK == 0) && (TIME_DEC_FLAG == 1))
    {
      if (TIME_WORK_FLAG == 0)
      {
        if (TIME_COUNT > 1)
        {
          TIME_COUNT--;
          TIME_1000ms = 0;
        } //60 minutes heating mode, time - switch timing.
      }
      else
      {
        if (on_4h_h_time > 1)
        {
          on_4h_h_time--;
          on_4h_10ms_time = appointment_com;
        } //4 hours heating mode, time - switch timing.
      }
    }
    //*********************************************
    if ((TEMP_INC_FLAG_BACK == 0) && (TEMP_INC_FLAG == 1)) //TEMP + switch from OFF to ON.
    {
      if (TEMP_FC_FLAG == 0)
      {
        if (set_temp < 75)
        {
          set_temp++;
        } //In Celsius, increase the setting temperature to 75 degrees.
      }
      else
      {
        if (set_f_temp < 170)
        {
          set_f_temp++;
        } //In degrees Fahrenheit, increase the setting temperature to 170 degrees.
      }

      settemp_display_time = 300; //Display delay time of set temperature,3S
    }

    if ((TEMP_DEC_FLAG_BACK == 0) && (TEMP_DEC_FLAG == 1)) //TEMP + switch from OFF to ON.
    {
      if (TEMP_FC_FLAG == 0)
      {
        if (set_temp > 40)
        {
          set_temp--;
        } //In Celsius, reduce the temperature setting temperature, the minimum value is 40 degrees.
      }
      else
      {
        if (set_f_temp > 100)
        {
          set_f_temp--;
        } //In degrees Fahrenheit, reduce the temperature setting temperature, the minimum value is 100 degrees.
      }
      settemp_display_time = 300; //Display delay time of set temperature,3S
    }
    //*************************************************
    if ((TEMP_ALL_FLAG_BACK == 0) && (TEMP_ALL_FLAG == 1)) //TEMP + switch and TEMP - switch all from OFF to ON.
    {
      if (TEMP_FC_FLAG == 0)
      { //The display switches to Fahrenheit
        TEMP_FC_FLAG = 1;
        set_f_temp = set_temp * 1.8 + 32;
        actual_f_temperature = actual_temperature * 1.8 + 32;

        if (set_f_temp > 166)
          set_f_temp = 170;
        //if(actual_f_temperature>166)  actual_f_temperature=170;
        if (set_f_temp < 100)
          set_f_temp = 100;
      }
      else
      {
        TEMP_FC_FLAG = 0; //Display switches to Celsius temperature

        set_temp = (set_f_temp - 32) / 1.8;
        actual_temperature = (actual_f_temperature - 32) / 1.8;

        if (set_temp > 74)
          set_temp = 75;
        //if(actual_temperature>74)   actual_temperature=75;
        if (set_temp < 40)
          set_temp = 40;
      }
    }
  }
  else
  {
    if (appointment_flag == 1) //In reservation mode
    {
      if ((TIME_INC_FLAG_BACK == 0) && (TIME_INC_FLAG == 1)) //Time + switch from OFF to ON.
      {
        if (appointment_h_time < 36)
        {
          appointment_h_time++;
          appointment_10ms_time = appointment_com;
        }
      }

      if ((TIME_DEC_FLAG_BACK == 0) && (TIME_DEC_FLAG == 1)) //Time - switch from OFF to ON.
      {
        if (appointment_h_time > 1)
        {
          appointment_h_time--;
          appointment_10ms_time = appointment_com;
        }
      }
    }

    if ((TIME_ALL_FLAG_BACK == 0) && (TIME_ALL_FLAG == 1)) //Time + switch and Time - switch all from OFF to ON.
    {
      if (appointment_flag == 0)
      {
        appointment_flag = 1; //Switch to the reservation mode
        init_TM1638();        //Initialize TM1638
      }
      else
        appointment_flag = 0; //Exit the Reservation Mode
    }
  }
  //******************************
  if ((appointment_flag == 0) && (TURN_ON_FLAG == 0)) //Not in reservation mode, not in heating mode
  {
    if ((POWER_FLAG_BACK == 0) && (POWER_FLAG == 1)) //Heat 1 switch from OFF to ON
    {
      on_4h_key_time = 582; //Heat 1 Switch counter
    }

    if (on_4h_key_time == 1)
    {
      on_4h_key_time = 0; //Hold Heat 1 switch for a while
      //TURN_ON_FLAG=1;
      on_just_flag = 1;

      if (TIME_WORK_FLAG == 0)
      {
        TIME_WORK_FLAG = 1; //Switch to 4-hour heating mode
        on_4h_10ms_time = appointment_com;
        on_4h_h_time = 4;
      }
      else
        TIME_WORK_FLAG = 0; //Switch to 60-minute heating mode

      TURN_ON_FLAG = 1; //Start the heating
      init_TM1638();    //Initialize TM1638

      //  for(i=0;i<4;i++)
      //  Write_DATA(i<<1,tab[0]);                 //初始化寄存器 Initialize register
      GRADE_UP = 0;
      GRADE_DOWN = 0;
      GRADE_MAX = 0;

      TIME_COUNT = 60;

      TIME_1000ms = 0;

      Write_DATA(0 << 1, tab[0]);
      Write_DATA(1 << 1, tab[0]);

      //Write_DATA(2<<1,tab[0]);
      Write_DATA(3 << 1, tab[0]);

      Write_DATA(4 << 1, tab[0]);

      RELAY1_flag = 1;
      RELAY2_flag = 1;
    }
  }
  else
  {
    on_4h_key_time = 0;
  }

  if (POWER_FLAG == 0)
  {
    on_4h_key_time = 0;
  }

  //************************************************************

  //    wdt_reset();
  //    i=Read_key();                          //读按键值
  //    Serial.println(i);
  //    if(i<8)
  //    {
  //      num[i]++;
  //      while(i==Read_key());          //等待按键释放
  //      if(num[i]>15)
  //      num[i]=0;
  //      Write_DATA(i*2,tab[num[i]]);

  //    }

  //************************************************
  POWER_FLAG_BACK = POWER_FLAG; //Each key value backup

  TEMP_INC_FLAG_BACK = TEMP_INC_FLAG;
  TEMP_DEC_FLAG_BACK = TEMP_DEC_FLAG;
  //     POWER_FLAG2_BACK=POWER_FLAG2;
  IN_LED_FLAG_BACK = IN_LED_FLAG;

  TIME_INC_FLAG_BACK = TIME_INC_FLAG;
  TIME_DEC_FLAG_BACK = TIME_DEC_FLAG;

  TIME_ALL_FLAG_BACK = TIME_ALL_FLAG;
  TEMP_ALL_FLAG_BACK = TEMP_ALL_FLAG;
}

void pro_time(void) //Time-based subroutine, 10 milliseconds
{
  //******************************
  if (POWER_FLAG_on_time > 0)
    POWER_FLAG_on_time--;
  if (POWER_FLAG_off_time > 0)
    POWER_FLAG_off_time--;

  //******************************
  if (appointment_flag == 1)
  { //In Reservation mode
    if (appointment_10ms_time > 0)
      appointment_10ms_time--;
    else
    {
      appointment_10ms_time = appointment_com;
      if (appointment_h_time > 0)
        appointment_h_time--;
      if (appointment_h_time == 0)
      {
        appointment_flag = 0; //It's appointment time. It's heating up.

        TURN_ON_FLAG = 1;
        init_TM1638(); //初始化TM1638  Initialize TM1638

        //  for(i=0;i<4;i++)
        //  Write_DATA(i<<1,tab[0]);                 //初始化寄存器 Initialize register
        GRADE_UP = 0;
        GRADE_DOWN = 0;
        GRADE_MAX = 0;

        TIME_COUNT = 60;

        TIME_1000ms = 0;

        Write_DATA(0 << 1, tab[0]);
        Write_DATA(1 << 1, tab[0]);

        //Write_DATA(2<<1,tab[0]);
        Write_DATA(3 << 1, tab[0]);

        Write_DATA(4 << 1, tab[0]);

        RELAY1_flag = 1;
        RELAY2_flag = 1;
      }
    }
  }
  else
    appointment_10ms_time = appointment_com;

  //*****************************************************
  if ((TURN_ON_FLAG == 1) && (TIME_WORK_FLAG == 1))
  {
    if (on_4h_10ms_time > 0)
      on_4h_10ms_time--; //In 4-hour heating mode
    else
    {
      on_4h_10ms_time = appointment_com;
      if (on_4h_h_time > 0)
        on_4h_h_time--;
      if (on_4h_h_time == 0)
      {
        TURN_ON_FLAG = 0; //The heating time is up. Stop the heating.
      }
    }
  }

  //*****************************************************

  if (settemp_display_time > 0)
    settemp_display_time--; //Display delay time of set temperature --

  if (on_4h_key_time > 1)
    on_4h_key_time--; //Heating 1 switch hold time

  if (TIME_100ms < 10)
    TIME_100ms++;
  if (TIME_1000ms < 4136)
    TIME_1000ms++;
  else
  { //60000ms=60s=1min
    TIME_1000ms = 0;
    if (TURN_ON_FLAG == 1)
    {
      if (TIME_COUNT > 0)
        TIME_COUNT--; //Display minus 1 minute
    }
  }

  //**************************************

  //**************************************

  if (TIME_LONG < 10)
    TIME_LONG++;
  else
  {
    TIME_LONG = 0;
    if (TURN_ON_FLAG == 1)
    {
      if (TIME_INC_SWTICH_LONG == 50) //In heating mode, long press the time + switch
      {
        if (TIME_WORK_FLAG == 0)
        {
          if (TIME_COUNT < 60)
          {
            TIME_COUNT++;
            TIME_1000ms = 0;
          } //In 60-minute mode,Add time quickly, up to 60 minutes.
        }
        else
        {
          if (on_4h_h_time < 4)
          {
            on_4h_h_time++;
            on_4h_10ms_time = appointment_com;
          } //In 4-hour mode,Quickly add up to 4 hours.
        }
      }

      if (TIME_DEC_SWTICH_LONG == 50) //In heating mode, long press the time - switch
      {
        if (TIME_WORK_FLAG == 0)
        {
          if (TIME_COUNT > 1)
          {
            TIME_COUNT--;
            TIME_1000ms = 0;
          } //In 60-minute mode,Reduce time quickly, by at least 1 minute.
        }
        else
        {
          if (on_4h_h_time > 1)
          {
            on_4h_h_time--;
            on_4h_10ms_time = appointment_com;
          } //In 4-hour mode,Quickly reduce the time by at least 1 hour.
        }
      }
      //*********************************
      if (TEMP_INC_SWTICH_LONG == 50) //In heating mode, long press the TEMP + switch
      {
        if (TEMP_FC_FLAG == 0)
        {
          if (set_temp < 75)
          {
            set_temp++;
          } //Rapidly increase setting temperature to 75 ° C Max
        }
        else
        {
          if (set_f_temp < 170)
          {
            set_f_temp++;
          } //Quickly increase setting temperature to 170 degrees F
        }

        settemp_display_time = 300; //Display delay time of set temperature,3S
      }

      if (TEMP_DEC_SWTICH_LONG == 50) //In heating mode, long press the TEMP - switch
      {
        if (TEMP_FC_FLAG == 0)
        {
          if (set_temp > 40)
          {
            set_temp--;
          } //Quickly reduce setting temperature to 40 ° C minimum
        }
        else
        {
          if (set_f_temp > 100)
          {
            set_f_temp--;
          } //Quickly reduce setting temperature to 100 ° F minimum
        }

        settemp_display_time = 300; //Display delay time of set temperature,3S
      }
    }
    else
    {
      if (appointment_flag == 1) //In Reservation mode
      {
        if (TIME_INC_SWTICH_LONG == 50)
        {
          if (appointment_h_time < 36)
          {
            appointment_h_time++;
            appointment_10ms_time = appointment_com;
          } //Increase appointment time quickly, up to 36 hours.
        }

        if (TIME_DEC_SWTICH_LONG == 50)
        {
          if (appointment_h_time > 1)
          {
            appointment_h_time--;
            appointment_10ms_time = appointment_com;
          } //Quickly reduce your appointment time by at least 1 hour.
        }
      }
    }
  }

  if (TURN_ON_FLAG == 1) //While it's heating up
  {
    if (TIME_INC_FLAG == 1)
    {
      if (TIME_INC_SWTICH_LONG < 50)
        TIME_INC_SWTICH_LONG++; //time + key Long press
    }
    else
    {
      TIME_INC_SWTICH_LONG = 0;
    }

    if (TIME_DEC_FLAG == 1)
    {
      if (TIME_DEC_SWTICH_LONG < 50)
        TIME_DEC_SWTICH_LONG++; //time - key Long press
    }
    else
    {
      TIME_DEC_SWTICH_LONG = 0;
    }
    //*************************************
    if (TEMP_INC_FLAG == 1)
    {
      if (TEMP_INC_SWTICH_LONG < 50)
        TEMP_INC_SWTICH_LONG++; //temp + key Long press
    }
    else
    {
      TEMP_INC_SWTICH_LONG = 0;
    }

    if (TEMP_DEC_FLAG == 1)
    {
      if (TEMP_DEC_SWTICH_LONG < 50)
        TEMP_DEC_SWTICH_LONG++; //temp - key Long press
    }
    else
    {
      TEMP_DEC_SWTICH_LONG = 0;
    }
  }
  else
  {
    //    TIME_INC_SWTICH_LONG=0;
    //    TIME_DEC_SWTICH_LONG=0;

    TEMP_INC_SWTICH_LONG = 0;
    TEMP_DEC_SWTICH_LONG = 0;
    if (appointment_flag == 1) //In Reservation mode
    {
      if (TIME_INC_FLAG == 1)
      {
        if (TIME_INC_SWTICH_LONG < 50)
          TIME_INC_SWTICH_LONG++; //time + key Long press
      }
      else
      {
        TIME_INC_SWTICH_LONG = 0;
      }

      if (TIME_DEC_FLAG == 1)
      {
        if (TIME_DEC_SWTICH_LONG < 50)
          TIME_DEC_SWTICH_LONG++; //time - key Long press
      }
      else
      {
        TIME_DEC_SWTICH_LONG = 0;
      }
      //*************************************
    }
  }
}

void pro_display(void) //Display digital diode subroutine.
{

  if (connection_code != 0)
  {
    String code_string = String(989);
    // Serial.println(code_string[0]);
    Write_DATA(2 << 1, tab[(code_string[0] - '0')]);
    Write_DATA(3 << 1, tab[(code_string[1] - '0')]);
    Write_DATA(4 << 1, tab[(code_string[2] - '0')]);
  }
  else
  {
    unsigned int i;
    unsigned char i_l;
    unsigned char i_h;
    unsigned int i_z;

    unsigned char i_max = 0;
    if (TIME_100ms == 10)
    {
      TIME_100ms = 0; //Rinse it every 100 milliseconds
      //TURN_ON_FLAG=1;

      if (TURN_ON_FLAG == 1) //While it's heating up
      {
        if (TIME_WORK_FLAG == 0)
        {
          i = TIME_COUNT / 10 * 0x10 + TIME_COUNT % 10; //In 60-minute mode
          i_l = i & 0x0f;
          i_h = (i >> 4) & 0x0f;
          Write_DATA(1 << 1, tab[i_l]); //Display the first 8
        }
        else
        { //In 4-hour mode,

          i_l = 0x76; //"h"
          i_h = on_4h_h_time;
          Write_DATA(1 << 1, i_l); //Display the first 8
        }

        Write_DATA(0 << 1, tab[i_h]); //Show the second 8
                                      //          Write_DATA(1<<1,tab[i_l]);

        if (settemp_display_time > 0)
        { //Display setting temperature
          if (TEMP_FC_FLAG == 0)
          {
            i = set_temp / 10 * 0x10 + set_temp % 10; //Celsius temperature display
            i_max = 0;
          }
          else
          {
            //i=set_f_temp/100*0x100+set_f_temp/10*0x10+set_f_temp%10;
            //              i=set_f_temp/100*0x100+set_f_temp%100+set_f_temp%10;
            //              i_max=(i>>8)&0x0f;

            if (set_f_temp < 100) //Shows the temperature in Fahrenheit
            {
              i_max = 0;
              i = set_f_temp / 10 * 0x10 + set_f_temp % 10;
            }
            else
            {
              i_max = 1;
              i_z = set_f_temp - 100;
              i = i_z / 10 * 0x10 + i_z % 10;
            }
          }
          i_l = i & 0x0f;
          i_h = (i >> 4) & 0x0f;

          if (i_max == 0)
            Write_DATA(2 << 1, 0);
          else
            Write_DATA(2 << 1, tab[1]);
          Write_DATA(3 << 1, tab[i_h]);
          Write_DATA(4 << 1, tab[i_l]);
        }
        else
        { //Display actual temperature
          if (GRADE_MAX == 0)
            Write_DATA(2 << 1, 0);
          else
            Write_DATA(2 << 1, tab[1]);

          Write_DATA(3 << 1, tab[GRADE_UP]);
          Write_DATA(4 << 1, tab[GRADE_DOWN]);
        }

        if (TIME_WORK_FLAG == 0) //In 60 minutes heating mode
        {
          if (TIME_COUNT == 0)
          {
            RELAY1_flag = 0;
            RELAY2_flag = 0;
            TURN_ON_FLAG = 0;
          } //The heating time is up. Stop the heating
          else
            RELAY1_flag = 1;
        }

        if (TEMP_FC_FLAG == 0)
        {
          digitalWrite(temp_c_led, LOW); //The Centigrade temperature indicator is on
          digitalWrite(temp_f_led, HIGH);
        }
        else
        {
          digitalWrite(temp_c_led, HIGH);
          digitalWrite(temp_f_led, LOW); //The Fahrenheit temperature indicator is on
        }
      }
      else
      { //Not in heated mode

        digitalWrite(temp_c_led, HIGH); //Turn off the Celsius temperature indicator
        digitalWrite(temp_f_led, HIGH); //Turn off the light indicating the Fahrenheit temperature
        RELAY1_flag = 0;
        RELAY2_flag = 0; //Turn off heat 1 and heat 2

        if (appointment_flag == 1)
        { //In Reservation mode
          i = appointment_h_time / 10 * 0x10 + appointment_h_time % 10;
          i_l = i & 0x0f;
          i_h = (i >> 4) & 0x0f;
          Write_DATA(0 << 1, tab[i_h]); //Displays the time of the appointment
          Write_DATA(1 << 1, tab[i_l]);

          Write_DATA(2 << 1, 0);
          Write_DATA(3 << 1, 0);
          Write_DATA(4 << 1, 0);

          digitalWrite(appointment_led, LOW); //The reservation light came on
        }
        else
        {
          //Write_COM(0x80);                 //关闭显示

          Write_DATA(0 << 1, 0); // Close the display
          Write_DATA(1 << 1, 0);

          Write_DATA(2 << 1, 0);
          Write_DATA(3 << 1, 0);
          Write_DATA(4 << 1, 0);

          digitalWrite(appointment_led, HIGH); // Turn off the reservation indicator
        }
      }
    }
  }
}

//*********************************************
void pro_pwm(void) //Heating output, internal lamp and external lamp output subroutine.Control relay.
{
  unsigned char i;

  if (RELAY1_flag == 0) //Heat 1
  {
    digitalWrite(RELAY1_LED, HIGH);
    digitalWrite(RELAY1_OUT, LOW);
  }
  else
  {
    if (TEMP_FC_FLAG == 0)
    {
      if (actual_temperature > set_temp)
      {
        digitalWrite(RELAY1_LED, HIGH);
        digitalWrite(RELAY1_OUT, LOW);
      }
      else
      {
        digitalWrite(RELAY1_LED, LOW);
        digitalWrite(RELAY1_OUT, HIGH);
      }
    }
    else
    {
      if (actual_f_temperature > set_f_temp)
      {
        digitalWrite(RELAY1_LED, HIGH);
        digitalWrite(RELAY1_OUT, LOW);
      }
      else
      {
        digitalWrite(RELAY1_LED, LOW);
        digitalWrite(RELAY1_OUT, HIGH);
      }
    }
  }

  //  Serial.println("RELAY2_flag:");
  //      Serial.println(RELAY2_flag);

  //****************************************
  //  if(RELAY2_flag==0)
  //  {
  //    digitalWrite(RELAY2_LED,HIGH);
  //    digitalWrite(RELAY2_OUT,LOW);
  //  }
  //  else
  //  {
  //    if(TEMP_FC_FLAG==0)
  //    {
  //      if(actual_temperature>set_temp)
  //      {
  //        digitalWrite(RELAY2_LED,HIGH);
  //        digitalWrite(RELAY2_OUT,LOW);
  //      }
  //      else
  //      {
  //        digitalWrite(RELAY2_LED,LOW);
  //        digitalWrite(RELAY2_OUT,HIGH);
  //      }
  //    }
  //    else
  //    {
  //      if(actual_f_temperature>set_f_temp)
  //      {
  //        digitalWrite(RELAY2_LED,HIGH);
  //        digitalWrite(RELAY2_OUT,LOW);
  //      }
  //      else
  //      {
  //        digitalWrite(RELAY2_LED,LOW);
  //        digitalWrite(RELAY2_OUT,HIGH);
  //      }
  //    }
  //  }
  //*********************************
  if (POWER_FLAG2 == 1)
    digitalWrite(RELAY2_OUT, HIGH);
  else
    digitalWrite(RELAY2_OUT, LOW);
  //*********************************
  if (RELAY1_flag == 1)
  {
    if (RELAY1_flag_back == 0)
    {
      RELAY2_N = 1;
    }
    else
    {
      if ((POWER_FLAG2_BACK == 0) && (POWER_FLAG2 == 1))
      {
        RELAY2_N++;
        if (RELAY2_N > 3)
          RELAY2_N = 0;
      }
    }
  }
  else
  {
    RELAY2_N = 0;
  }

  RELAY1_flag_back = RELAY1_flag;
  POWER_FLAG2_BACK = POWER_FLAG2;

  //*********************************
  if (RELAY2_N == 0)
  {
    digitalWrite(RELAY2_LED, HIGH); //Heat 2 indicator light, 50% green, 75% blue, 100% red.
    digitalWrite(RELAY2_BLUE, HIGH);
    digitalWrite(RELAY2_GREEN, HIGH);
  }

  if (RELAY2_N == 1)
  {
    digitalWrite(RELAY2_LED, HIGH);
    digitalWrite(RELAY2_BLUE, HIGH);
    digitalWrite(RELAY2_GREEN, LOW);
  }

  if (RELAY2_N == 2)
  {
    digitalWrite(RELAY2_LED, HIGH);
    digitalWrite(RELAY2_BLUE, LOW);
    digitalWrite(RELAY2_GREEN, HIGH);
  }

  if (RELAY2_N == 3)
  {
    digitalWrite(RELAY2_LED, LOW);
    digitalWrite(RELAY2_BLUE, HIGH);
    digitalWrite(RELAY2_GREEN, HIGH);
  }

  //************************************

  if (IN_LED_FLAG == 0)
    digitalWrite(IN_LED, LOW); //in led
  else
    digitalWrite(IN_LED, HIGH);

  if (OUT_LED_FLAG == 0)
    digitalWrite(OUT_LED, LOW); //out led
  else
    digitalWrite(OUT_LED, HIGH);
}

//**************************
void pro_uart(void)
{
  int i;
  byte y;

  int x;

  i = Serial.available();
  if (i > 0)
  {
    if (i != from_acp_heater)
    {
      from_acp_heater_n = 0;
      from_acp_heater = i;
    }
    else
    {
      if (from_acp_heater_n < 10000)
        from_acp_heater_n++; //
      else
      {
        while (Serial.available() > 0)
        {
          Serial.read();
        }

        from_acp_heater = 0;
        from_acp_heater_n = 0;
      }
    }
  }
  else
  {
    from_acp_heater = 0;
    from_acp_heater_n = 0;
  }
  //***********************************
  if (Serial.available() > 13)
  {
    while (Serial.available() > 0)
    {
      Serial.read();
    }
  }

  //**********************************
  if (Serial.available() == 13)
  {
    //    Serial.println(Serial.available());
    //    Serial.println("888");
    Serial.setTimeout(20);
    i = Serial.readBytes(command_heater_acp, 13);
    //    Serial.println(i);
    //
    //    Serial.println(Serial.available());

    //    Serial.println(command_heater_acp[0]);
    //    Serial.println(command_heater_acp[1]);

    y = command_heater_acp[0] + command_heater_acp[1] + command_heater_acp[2] + command_heater_acp[3] + command_heater_acp[4] + command_heater_acp[5] + command_heater_acp[6] + command_heater_acp[7] + command_heater_acp[8] + command_heater_acp[9] + command_heater_acp[10] + command_heater_acp[11];

    if ((i == 13) && (command_heater_acp[0] == 0x46) && (command_heater_acp[1] == 0x53) && (command_heater_acp[2] == 0x44) && (y == command_heater_acp[12]))
    {
      command_sw = command_heater_acp[3];
      command_time_60 = command_heater_acp[4];
      command_time_4h = command_heater_acp[5];
      command_time_reservation = command_heater_acp[6];
      command_time_fc = command_heater_acp[7];

      command_temp_en = command_heater_acp[8];
      command_temp_set = command_heater_acp[9];
    }
  }

  //*********************************************
  if (tx_time > 0)
    tx_time--;
  if (tx_time == 0)
  {
    tx_time = 50; //500ms
    command_tx_acp[0] = 0X46;
    command_tx_acp[1] = 0X53;
    command_tx_acp[2] = 0X44;

    if (appointment_flag == 1)
      command_tx_acp[3] = 0X03;
    else
    {
      if (TURN_ON_FLAG == 1)
      {
        if (TIME_WORK_FLAG == 0)
          command_tx_acp[3] = 0X01;
        else
          command_tx_acp[3] = 0X02;
      }
      else
      {
        command_tx_acp[3] = 0X00;
      }
    }

    command_tx_acp[4] = 0;
    command_tx_acp[5] = 0;
    command_tx_acp[6] = 0;

    if (command_tx_acp[3] == 1)
      command_tx_acp[4] = TIME_COUNT;
    if (command_tx_acp[3] == 2)
      command_tx_acp[5] = on_4h_h_time;
    if (command_tx_acp[3] == 3)
      command_tx_acp[6] = appointment_h_time;

    //    if(TURN_ON_FLAG==1)
    //    {
    //    }
    //    else
    //    {
    //    }
    command_tx_acp[7] = TEMP_FC_FLAG;

    if (TEMP_FC_FLAG == 1)
    {
      command_tx_acp[8] = actual_f_temperature;
      command_tx_acp[9] = set_f_temp;
    }
    else
    {
      command_tx_acp[8] = actual_temperature;
      command_tx_acp[9] = set_temp;
    }

    command_tx_acp[10] = 0;
    command_tx_acp[11] = 0;

    if (IN_LED_FLAG == 1)
      command_tx_acp[10] = (command_tx_acp[10] | 0x01);
    if (OUT_LED_FLAG == 1)
      command_tx_acp[10] = (command_tx_acp[10] | 0x02);

    command_tx_acp[12] = command_tx_acp[0] + command_tx_acp[1] + command_tx_acp[2] + command_tx_acp[3] + command_tx_acp[4] + command_tx_acp[5] + command_tx_acp[6] + command_tx_acp[7] + command_tx_acp[8] + command_tx_acp[9] + command_tx_acp[10] + command_tx_acp[11];

    Serial.write(command_tx_acp, 13);
  }
}
