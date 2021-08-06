#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

#define BLE_LED 25
#define SENSOR_LED 26

uint32_t detected = 48;
int count = 0;

bool BLE_STATE = false;
bool HAND_STATE = false;

#define Sensor_Pin_1 32
#define Sensor_Pin_2 34
#define Sensor_Pin_3 35

#define BUZZER 12
#define BUZZER_BTN_PIN 4
bool isActive = true;
int BTN_DELAY = 1500; //1.5 sec

unsigned long current_time = 0;
unsigned long start_time = 0;

int BLE_DELAY = 5000; //5 sec delay in terms on milli sec.
int SPIKE_DELAY = 30; //30 ms
int HAND_DELAY = 1500; //1.5 sec
unsigned long BLE_ON_TIME = 0;
int BLE_ON_DELAY = 5000; //30 sec

BLEServer* pServer = NULL;
BLECharacteristic* pCharacteristic = NULL;

#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

class MyServerCallbacks: public BLEServerCallbacks {
  void onConnect(BLEServer* pServer){
    BLEDevice::stopAdvertising();
    ledcWrite(0, 255);
    delay(1000);
    ledcWrite(0, 0);
    BLE_STATE = false;
    count++;
  };

  void onDisconnect(BLEServer* pServer){
    count--;
  }
};


void setup() {
  Serial.begin(115200);

  ledcSetup(0, 10, 8);
  ledcAttachPin(BLE_LED, 0);

  pinMode(SENSOR_LED, OUTPUT);
  pinMode(Sensor_Pin_1, INPUT);
  pinMode(Sensor_Pin_2, INPUT);
  pinMode(Sensor_Pin_3, INPUT);
  pinMode(BUZZER_BTN_PIN, INPUT);
  pinMode(BUZZER, OUTPUT);

  BLEDevice::init("Transmitter");
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());
  BLEService *pService = pServer->createService(SERVICE_UUID);
  pCharacteristic = pService->createCharacteristic(
                      CHARACTERISTIC_UUID,
                      BLECharacteristic::PROPERTY_READ   |
                      BLECharacteristic::PROPERTY_WRITE  |
                      BLECharacteristic::PROPERTY_NOTIFY
                    );

  pCharacteristic->addDescriptor(new BLE2902());
  pService->start();

  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x0);
  Serial.println("BLE Initialization Complete");
}

void loop() {
  current_time = millis();
  Read_Sensor();
  if(HAND_STATE && count > 0){
    detected = 1;
    pCharacteristic->setValue((uint8_t*)&detected, 1);
    pCharacteristic->notify();
    detected = 0;
    HAND_STATE = false;
    delay(500);
    digitalWrite(SENSOR_LED, LOW);
    digitalWrite(BUZZER, LOW);
  }
  if(BLE_STATE && current_time-BLE_ON_TIME > BLE_ON_DELAY){
    ledcWrite(0, 0);
    BLEDevice::stopAdvertising();
    BLE_STATE = false;
  }
}

void Read_Sensor(){
  if(digitalRead(Sensor_Pin_1) || digitalRead(Sensor_Pin_2) || digitalRead(Sensor_Pin_3)){
    start_time = millis();
    while(digitalRead(Sensor_Pin_1) || digitalRead(Sensor_Pin_2) || digitalRead(Sensor_Pin_3));
    current_time = millis();
    int duration = current_time - start_time;
    
    if(duration > SPIKE_DELAY && duration < HAND_DELAY){
      HAND_STATE = true;
      if(count > 0){
        if(isActive)
          digitalWrite(BUZZER, HIGH);
        digitalWrite(SENSOR_LED, HIGH);
      }
    }
    else if(!BLE_STATE && duration > HAND_DELAY && duration < BLE_DELAY){
      BLE_ON_TIME = current_time;
      BLEDevice::startAdvertising();
      BLE_STATE = true;
      ledcWrite(0, 125);
    }
    Serial.println(duration);
  }
  if(digitalRead(BUZZER_BTN_PIN)){
    start_time = millis();
    while(digitalRead(BUZZER_BTN_PIN));
    current_time = millis();
    int d = current_time - start_time;
    if(d > SPIKE_DELAY && d < BTN_DELAY){
      if(isActive)
        isActive = false;
      else
        isActive = true;
    }
  }
}
