#include "BLEDevice.h"
//#include "BLEScan.h"

int count = 0;

int RELAY = 21;
int on_time = 2000;
unsigned long current_time = 0;
unsigned long start_time = 0;
int detect = 0;

int BTNS[] = {33,32}; //latched/momentary, ble
unsigned btn_delay[] = {0,0};
int debounce = 500;

int BLE_DALAY = 1500;
int MAX_BLE_DALAY = 5000;

#define BLE_LED 13
#define BLE_STATUS 25
#define STATE 35


static BLEUUID serviceUUID("4fafc201-1fb5-459e-8fcc-c5c9c331914b");
static BLEUUID    charUUID("beb5483e-36e1-4688-b7f5-ea07361b26a8");

bool doConnect = false;
bool doScan = false;

bool mom = false;
bool latched = false;
bool isON = false;

static BLERemoteCharacteristic* pRemoteCharacteristic;
static BLEAdvertisedDevice* myDevice;

static void notifyCallback(BLERemoteCharacteristic* pBLERemoteCharacteristic, uint8_t* pData, size_t length, bool isNotify) {
    String payload = "";
    Serial.print("data: ");
    for(int i=0; i<length; i++)
      payload = payload + pData[i];

    Serial.println(payload);
    if(payload == "1")
      detect = 1;
}

class MyClientCallback : public BLEClientCallbacks {
  void onConnect(BLEClient* pclient) {
    ledcWrite(0, 255);
    delay(1000);
    ledcWrite(0, 0);
    count++;
    if(count>0)
      digitalWrite(BLE_STATUS, HIGH);
    Serial.println("Device Connected: " + String(count));
  }

  void onDisconnect(BLEClient* pclient) {
    count--;
    if(count == 0 && latched){
      digitalWrite(RELAY, LOW);
    if(count<1)
      digitalWrite(BLE_STATUS, LOW);
    }
  }
};

bool connectToServer() {
    Serial.print("Forming a connection to ");
    Serial.println(myDevice->getAddress().toString().c_str());
    
    BLEClient*  pClient  = BLEDevice::createClient();
    Serial.println(" - Created client");

    pClient->setClientCallbacks(new MyClientCallback());


    pClient->connect(myDevice);
    Serial.println(" - Connected to server");

    BLERemoteService* pRemoteService = pClient->getService(serviceUUID);
    if (pRemoteService == nullptr) {
      Serial.print("Failed to find our service UUID: ");
      Serial.println(serviceUUID.toString().c_str());
      pClient->disconnect();
      return false;
    }
    Serial.println(" - Found our service");

    pRemoteCharacteristic = pRemoteService->getCharacteristic(charUUID);
    if (pRemoteCharacteristic == nullptr) {
      Serial.print("Failed to find our characteristic UUID: ");
      Serial.println(charUUID.toString().c_str());
      pClient->disconnect();
      return false;
    }
    Serial.println(" - Found our characteristic");

    if(pRemoteCharacteristic->canNotify())
      pRemoteCharacteristic->registerForNotify(notifyCallback);

    return true;
}

class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {

  void onResult(BLEAdvertisedDevice advertisedDevice) {
    Serial.print("BLE Advertised Device found: ");
    Serial.println(advertisedDevice.toString().c_str());
    Serial.println(advertisedDevice.haveServiceUUID());
    Serial.println(advertisedDevice.isAdvertisingService(serviceUUID));

    if (advertisedDevice.haveServiceUUID() && advertisedDevice.isAdvertisingService(serviceUUID)){
      Serial.println("FOUND");

      BLEDevice::getScan()->stop();
      myDevice = new BLEAdvertisedDevice(advertisedDevice);
      doConnect = true;
      doScan = false;
    }
  }
};


void setup() {
  Serial.begin(115200);

  ledcSetup(0, 10, 8);
  ledcAttachPin(BLE_LED, 0);

  pinMode(STATE, OUTPUT);
  pinMode(BLE_STATUS, OUTPUT);
  
  Serial.println("Starting Arduino BLE Client application...");
  BLEDevice::init("Receiver");

  pinMode(RELAY, OUTPUT);
  pinMode(BTNS[0], INPUT);
  pinMode(BTNS[1], INPUT);

  BLEScan* pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setInterval(1349);
  pBLEScan->setWindow(449);
  pBLEScan->setActiveScan(true);
  pBLEScan->start(5, false);
  Serial.println("Setup Complete");
}


void loop() {
  Read_btns();
  if(count > 0){
    if(detect == 1 && !isON){
      Serial.println("TURNING ON RELAY");
      isON = true;
      digitalWrite(RELAY, HIGH);
      start_time = millis();
      detect = 0;
    }
  }
  if(isON && mom){
    current_time = millis();
    if(current_time - start_time >= on_time){
      digitalWrite(RELAY, LOW);
      isON = false;
    }
  }
  else if(isON && latched && detect == 1){
    digitalWrite(RELAY, LOW);
    isON = false;
    detect = 0;
  }
  
  if (doConnect == true) {
    if (connectToServer()) {
      Serial.println("We are now connected to the BLE Server.");
    } else {
      Serial.println("We have failed to connect to the server; there is nothin more we will do.");
    }
    doConnect = false;
  }

  if(doScan){
    BLEDevice::getScan()->start(5);
    ledcWrite(0, 0);
    doScan = false;
  }
}

void Read_btns(){
  current_time = millis();
  if(digitalRead(BTNS[0]) && current_time-btn_delay[0]>debounce){
    latched = true;
    mom = false;
    digitalWrite(STATE, HIGH);
    btn_delay[0] = current_time;
  }
  if(!digitalRead(BTNS[0]) && current_time-btn_delay[0]>debounce){
    latched = false;
    mom = true;
    digitalWrite(STATE, LOW);
    btn_delay[0] = current_time;
  }
  if(digitalRead(BTNS[1]) && current_time-btn_delay[1]>debounce){
    btn_delay[1] = current_time;
    while(digitalRead(BTNS[1]))
      current_time = millis();
    if(current_time-btn_delay[1]>BLE_DALAY && current_time-btn_delay[1]<MAX_BLE_DALAY){
      doScan = true;
      ledcWrite(0, 125);
    }
  }
}
