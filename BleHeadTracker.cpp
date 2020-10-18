#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include "BLE2902.h"
#include "BLEHIDDevice.h"
#include "HIDTypes.h"
#include "HIDKeyboardTypes.h"
#include <driver/adc.h>
#include "sdkconfig.h"

#include "BleConnectionStatus.h"
#include "BleHeadTracker.h"

#if defined(CONFIG_ARDUHAL_ESP_LOG)
  #include "esp32-hal-log.h"
  #define LOG_TAG ""
#else
  #include "esp_log.h"
  static const char* LOG_TAG = "BLEDevice";
#endif

static const uint8_t _hidReportDescriptor[] = {
  USAGE_PAGE(1),       0x01, // USAGE_PAGE (Generic Desktop)
  USAGE(1),            0x05, // USAGE (Gamepad)
  COLLECTION(1),       0x01, // COLLECTION (Application)
  USAGE(1),            0x01, //   USAGE (Pointer)
  COLLECTION(1),       0x00, //   COLLECTION (Physical)
  REPORT_ID(1),        0x01, //     REPORT_ID (1)
  // ------------------------------------------------- Buttons (1 to 14)
  /*USAGE_PAGE(1),       0x09, //     USAGE_PAGE (Button)
  USAGE_MINIMUM(1),    0x01, //     USAGE_MINIMUM (Button 1)
  USAGE_MAXIMUM(1),    0x0e, //     USAGE_MAXIMUM (Button 14)
  LOGICAL_MINIMUM(1),  0x00, //     LOGICAL_MINIMUM (0)
  LOGICAL_MAXIMUM(1),  0x01, //     LOGICAL_MAXIMUM (1)
  REPORT_SIZE(1),      0x01, //     REPORT_SIZE (1)
  REPORT_COUNT(1),     0x0e, //     REPORT_COUNT (14)
  HIDINPUT(1),         0x02, //     INPUT (Data, Variable, Absolute) ;14 button bits*/
  // ------------------------------------------------- Padding
  /*REPORT_SIZE(1),      0x01, //     REPORT_SIZE (1)
  REPORT_COUNT(1),     0x02, //     REPORT_COUNT (2)
  HIDINPUT(1),         0x03, //     INPUT (Constant, Variable, Absolute) ;2 bit padding*/
  // ------------------------------------------------- X/Y position, Z/rZ position
  USAGE_PAGE(1),       0x01, //     USAGE_PAGE (Generic Desktop)
  USAGE(1),            0x36, //     USAGE (Slider Roll)
  USAGE(1),            0x37, //     USAGE (Dial Pitch)
  USAGE(1),            0x38, //     USAGE (Wheel Yaw)
  //USAGE(1),            0x35, //     USAGE (rZ)
  LOGICAL_MINIMUM(1),  0x81, //     LOGICAL_MINIMUM (-127)
  LOGICAL_MAXIMUM(1),  0x7f, //     LOGICAL_MAXIMUM (127)
  REPORT_SIZE(1),      0x08, //     REPORT_SIZE (8)
  REPORT_COUNT(1),     0x03, //     REPORT_COUNT (3)
  HIDINPUT(1),         0x02, //     INPUT (Data, Variable, Absolute) ;4 bytes (X,Y,Z,rZ)

  /*USAGE_PAGE(1),       0x01, //     USAGE_PAGE (Generic Desktop)
  USAGE(1),            0x33, //     USAGE (rX) Left Trigger
  USAGE(1),            0x34, //     USAGE (rY) Right Trigger
  LOGICAL_MINIMUM(1),  0x81, //     LOGICAL_MINIMUM (-127)
  LOGICAL_MAXIMUM(1),  0x7f, //     LOGICAL_MAXIMUM (127)
  REPORT_SIZE(1),      0x08, //     REPORT_SIZE (8)
  REPORT_COUNT(1),     0x02, //     REPORT_COUNT (2)
  HIDINPUT(1),         0x02, //     INPUT (Data, Variable, Absolute) ;2 bytes rX, rY*/

  /*USAGE_PAGE(1),       0x01, //     USAGE_PAGE (Generic Desktop)
  USAGE(1),            0x39, //     USAGE (Hat switch)
  USAGE(1),            0x39, //     USAGE (Hat switch)
  LOGICAL_MINIMUM(1),  0x01, //     LOGICAL_MINIMUM (1)
  LOGICAL_MAXIMUM(1),  0x08, //     LOGICAL_MAXIMUM (8)
  REPORT_SIZE(1),      0x04, //     REPORT_SIZE (4)
  REPORT_COUNT(1),     0x02, //     REPORT_COUNT (2)
  HIDINPUT(1),         0x02, //     INPUT (Data, Variable, Absolute) ;1 byte Hat1, Hat2*/

  END_COLLECTION(0),         //     END_COLLECTION
  END_COLLECTION(0)          //     END_COLLECTION
};

BleHeadTracker::BleHeadTracker(std::string deviceName, std::string deviceManufacturer, uint8_t batteryLevel) :
  _buttons(0),
  hid(0)
{
  this->deviceName = deviceName;
  this->deviceManufacturer = deviceManufacturer;
  this->batteryLevel = batteryLevel;
  this->connectionStatus = new BleConnectionStatus();
}

void BleHeadTracker::begin(void)
{
  xTaskCreate(this->taskServer, "server", 20000, (void *)this, 5, NULL);
}

void BleHeadTracker::end(void)
{
}

void BleHeadTracker::setAxes(signed char roll, signed char pitch, signed char yaw)
{
  if (this->isConnected())
  {
    uint8_t m[3];
    m[0] = roll;
    m[1] = pitch;
    m[2] = yaw;
    this->inputGamepad->setValue(m, sizeof(m));
    this->inputGamepad->notify();
  }
}

/*void BleHeadTracker::buttons(uint16_t b)
{
  if (b != _buttons)
  {
    _buttons = b;
    setAxes(0, 0, 0, 0, 0, 0, 0);
  }
}

void BleHeadTracker::press(uint16_t b)
{
  buttons(_buttons | b);
}

void BleHeadTracker::release(uint16_t b)
{
  buttons(_buttons & ~b);
}

bool BleHeadTracker::isPressed(uint16_t b)
{
  if ((b & _buttons) > 0)
    return true;
  return false;
}*/

bool BleHeadTracker::isConnected(void) {
  return this->connectionStatus->connected;
}

void BleHeadTracker::setBatteryLevel(uint8_t level) {
  this->batteryLevel = level;
  if (hid != 0)
    this->hid->setBatteryLevel(this->batteryLevel);
}

void BleHeadTracker::taskServer(void* pvParameter) {
  BleHeadTracker* BleHeadTrackerInstance = (BleHeadTracker *) pvParameter; //static_cast<BleHeadTracker *>(pvParameter);
  BLEDevice::init(BleHeadTrackerInstance->deviceName);
  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(BleHeadTrackerInstance->connectionStatus);

  BleHeadTrackerInstance->hid = new BLEHIDDevice(pServer);
  BleHeadTrackerInstance->inputGamepad = BleHeadTrackerInstance->hid->inputReport(1); // <-- input REPORTID from report map
  BleHeadTrackerInstance->connectionStatus->inputGamepad = BleHeadTrackerInstance->inputGamepad;

  BleHeadTrackerInstance->hid->manufacturer()->setValue(BleHeadTrackerInstance->deviceManufacturer);

  BleHeadTrackerInstance->hid->pnp(0x01,0x02e5,0xabcd,0x0110);
  BleHeadTrackerInstance->hid->hidInfo(0x00,0x01);

  BLESecurity *pSecurity = new BLESecurity();

  pSecurity->setAuthenticationMode(ESP_LE_AUTH_BOND);

  BleHeadTrackerInstance->hid->reportMap((uint8_t*)_hidReportDescriptor, sizeof(_hidReportDescriptor));
  BleHeadTrackerInstance->hid->startServices();

  BleHeadTrackerInstance->onStarted(pServer);

  BLEAdvertising *pAdvertising = pServer->getAdvertising();
  pAdvertising->setAppearance(HID_GAMEPAD);
  pAdvertising->addServiceUUID(BleHeadTrackerInstance->hid->hidService()->getUUID());
  pAdvertising->start();
  BleHeadTrackerInstance->hid->setBatteryLevel(BleHeadTrackerInstance->batteryLevel);

  ESP_LOGD(LOG_TAG, "Advertising started!");
  vTaskDelay(portMAX_DELAY); //delay(portMAX_DELAY);
}
