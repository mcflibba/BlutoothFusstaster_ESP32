/*
   Sample program for ESP32 acting as a Bluetooth keyboard

   Copyright (c) 2019 Manuel Bl

   Licensed under MIT License
   https://opensource.org/licenses/MIT
*/

//
// This program lets an ESP32 act as a keyboard connected via Bluetooth.
// When a button attached to the ESP32 is pressed, it will generate the key strokes for a message.
//
// For the setup, a momentary button should be connected to pin 2 and to ground.
// Pin 2 will be configured as an input with pull-up.
//
// In order to receive the message, add the ESP32 as a Bluetooth keyboard of your computer
// or mobile phone:
//
// 1. Go to your computers/phones settings
// 2. Ensure Bluetooth is turned on
// 3. Scan for Bluetooth devices
// 4. Connect to the device called "ESP32 Keyboard"
// 5. Open an empty document in a text editor
// 6. Press the button attached to the ESP32


//==========================================
/*Bluetooth Fusstaster_ESP32
--------------------------------------------
- Fusstaster fuer das umblaettern von Seiten am IPAD/Iphone
- Programm emuliert eine Bluetooth-Tastatur

1. Go to your computers/phones settings
2. Ensure Bluetooth is turned on
3. Scan for Bluetooth devices
4. Connect to the device called "ESP32 Keyboard"
5. Open an empty document in a text editor
6. Press the button attached to the ESP32


*/

//==========================================


/* R02_4_Dummy; 03.06.2024, fr
    - Status LED fuer Bluetooth hinzugefuegt
    - Automatisches verbinden nach Verlust der Verbindung   
*/
/* R02_3; 01.06.2024, fr
    - Tastendelay als Variable fuer Haupttaster und Switch-taster eingefuehrt
    - Tastendelay fuer Haupttaster verlaengert 100 -> 200
*/
/* R02_2; 05.05.2024, fr
    - Einfuhren von Mousclick
    - Umstrukturierung fuer Mausklick-Option
    - Information ueber Programmversion wird per Seriell uebertragen
*/

#define US_KEYBOARD 1
#define REVISION_STAND "BlutoothTastatur_ESP32_R02_4Dummy" //hier den Namen des Programms mit dem aktuellen Revisionsstand angeben

#include <Arduino.h>
#include "BLEDevice.h"
#include "BLEHIDDevice.h"
#include "HIDTypes.h"
#include "HIDKeyboardTypes.h"

//BleMouse bleMouse;


// Change the below values if desired
#define BUTTON_PIN_DOWN 2
#define BUTTON_PIN_UP 4
#define BUTTON_PIN_LEFT 5
#define BUTTON_PIN_RIGHT 19
#define BUTTON_PIN_SPACEBAR 12


#define MOUSE_BUTTON_LEFT   0x01
#define MOUSE_BUTTON_RIGHT  0x02
#define MOUSE_BUTTON_MIDDLE 0x04


#define MAIN_BUTTON_RIGHT 4
#define MAIN_BUTTON_LEFT 16
#define MAIN_BUTTON_SWITCH 17

#define DELAY_MAIN_BUTTON 200
#define DELAY_MAIN_BUTTON_SWITCH 200


#define LED_STATUS_1 32
#define LED_STATUS_2 33
#define LED_STATUS_3 25
#define LED_STATUS_4 26
#define LED_STATUS_5 27

int LED_STATUS[] = {LED_STATUS_1, LED_STATUS_2, LED_STATUS_3, LED_STATUS_4, LED_STATUS_5};
int Status = 0;

#define LED_BLUETOOTH 14


uint8_t COMMAND_LEFT[] = {
  0x52, // Keyboard Up Arrow,
  0x50, // Keyboard Left Arrow,
  0x4b, // Keyboard Page Up,
  0x2c // Keyboard Space Bar
};

uint8_t COMMAND_RIGHT[] = {
  0x51, // Keyboard Down Arrow,
  0x4f, // Keyboard Right Arrow,
  0x4e, // Keyboard Page Down,
  0x28 // Keyboard Enter
};


#define DEVICE_NAME "ESP32 Noten"



// Forward declarations
void bluetoothTask(void*);
void typeText(const char* text);


bool isBleConnected = false;


void setup() {
  Serial.begin(115200);
 // bleMouse.begin(); //Initialisieren der Maus

  // configure pin for button
  pinMode(MAIN_BUTTON_RIGHT, INPUT_PULLUP);
  pinMode(MAIN_BUTTON_LEFT, INPUT_PULLUP);
  pinMode(MAIN_BUTTON_SWITCH, INPUT_PULLUP);


  //for (int i=0; i <= LED_STATUS[]

  pinMode(LED_STATUS_1, OUTPUT);
  pinMode(LED_STATUS_2, OUTPUT);
  pinMode(LED_STATUS_3, OUTPUT);
  pinMode(LED_STATUS_4, OUTPUT);
  pinMode(LED_STATUS_5, OUTPUT);

  pinMode(LED_BLUETOOTH, OUTPUT);
  digitalWrite(LED_BLUETOOTH, LOW);





  // start Bluetooth task
  xTaskCreate(bluetoothTask, "bluetooth", 20000, NULL, 5, NULL);


  // Initialisierung Status-LEDs
  for (int Zaehler = 0; Zaehler <= 4; Zaehler++) {  //Reset LEDs
    digitalWrite(LED_STATUS[Zaehler], LOW);
  }
  digitalWrite(LED_STATUS[Status], HIGH);
}


void loop() {

// Abfrage mit Neue Verbindung
  if (isBleConnected){
    digitalWrite(LED_BLUETOOTH, HIGH);     
  }
  else{
    digitalWrite(LED_BLUETOOTH, LOW);  
  }


  if (digitalRead(MAIN_BUTTON_SWITCH) == LOW) {
    for (int Zaehler = 0; Zaehler <= 4; Zaehler++) { //Reset LEDs
      digitalWrite(LED_STATUS[Zaehler], LOW);
    }

  Status++;
  if (Status > 4){
    Status = 0;
  }
  
  Serial.print("Das ist der Status: ");
  Serial.println(Status);

  digitalWrite(LED_STATUS[Status], HIGH);
  delay(DELAY_MAIN_BUTTON_SWITCH);
  }


  if (Status == 4) { //Mausklick-Option
    if (isBleConnected && digitalRead(MAIN_BUTTON_RIGHT) == LOW) {
      Serial.print("Button rechts MAusschleife: ");
      Serial.println(Status);
     // bleMouse.click(MOUSE_RIGHT);
     sendMouseClick(MOUSE_BUTTON_RIGHT);
      delay(DELAY_MAIN_BUTTON);
    }

    if (isBleConnected && digitalRead(MAIN_BUTTON_LEFT) == LOW) {
      Serial.print("Button links MAusschleife: ");
      Serial.println(Status);
     // bleMouse.click(MOUSE_LEFT);
     sendMouseClick(MOUSE_BUTTON_LEFT);
      delay(DELAY_MAIN_BUTTON);
    }

  }
  else {

    if (isBleConnected && digitalRead(MAIN_BUTTON_RIGHT) == LOW) {
      Serial.print("COMMAND_RIGHT: ");
      Serial.println(COMMAND_RIGHT[Status]);
      sendKeyCode(COMMAND_RIGHT[Status]);
      delay(DELAY_MAIN_BUTTON);
    }

    if (isBleConnected && digitalRead(MAIN_BUTTON_LEFT) == LOW) {
      Serial.print("COMMAND_LEFT: ");
      Serial.println(COMMAND_LEFT[Status]);
      sendKeyCode(COMMAND_LEFT[Status]);
      delay(DELAY_MAIN_BUTTON);
    }
  }

}


// Message (report) sent when a key is pressed or released
struct InputReport {
  uint8_t modifiers;       // bitmask: CTRL = 1, SHIFT = 2, ALT = 4
  uint8_t reserved;        // must be 0
  uint8_t pressedKeys[6];  // up to six concurrenlty pressed keys
};

// Message (report) received when an LED's state changed
struct OutputReport {
  uint8_t leds;            // bitmask: num lock = 1, caps lock = 2, scroll lock = 4, compose = 8, kana = 16
};


// The report map describes the HID device (a keyboard in this case) and
// the messages (reports in HID terms) sent and received.
static const uint8_t REPORT_MAP[] = {
  USAGE_PAGE(1),      0x01,       // Generic Desktop Controls
  USAGE(1),           0x06,       // Keyboard
  COLLECTION(1),      0x01,       // Application
  REPORT_ID(1),       0x01,       //   Report ID (1)
  USAGE_PAGE(1),      0x07,       //   Keyboard/Keypad
  USAGE_MINIMUM(1),   0xE0,       //   Keyboard Left Control
  USAGE_MAXIMUM(1),   0xE7,       //   Keyboard Right Control
  LOGICAL_MINIMUM(1), 0x00,       //   Each bit is either 0 or 1
  LOGICAL_MAXIMUM(1), 0x01,
  REPORT_COUNT(1),    0x08,       //   8 bits for the modifier keys
  REPORT_SIZE(1),     0x01,
  HIDINPUT(1),        0x02,       //   Data, Var, Abs
  REPORT_COUNT(1),    0x01,       //   1 byte (unused)
  REPORT_SIZE(1),     0x08,
  HIDINPUT(1),        0x01,       //   Const, Array, Abs
  REPORT_COUNT(1),    0x06,       //   6 bytes (for up to 6 concurrently pressed keys)
  REPORT_SIZE(1),     0x08,
  LOGICAL_MINIMUM(1), 0x00,
  LOGICAL_MAXIMUM(1), 0x65,       //   101 keys
  USAGE_MINIMUM(1),   0x00,
  USAGE_MAXIMUM(1),   0x65,
  HIDINPUT(1),        0x00,       //   Data, Array, Abs
  REPORT_COUNT(1),    0x05,       //   5 bits (Num lock, Caps lock, Scroll lock, Compose, Kana)
  REPORT_SIZE(1),     0x01,
  USAGE_PAGE(1),      0x08,       //   LEDs
  USAGE_MINIMUM(1),   0x01,       //   Num Lock
  USAGE_MAXIMUM(1),   0x05,       //   Kana
  LOGICAL_MINIMUM(1), 0x00,
  LOGICAL_MAXIMUM(1), 0x01,
  HIDOUTPUT(1),       0x02,       //   Data, Var, Abs
  REPORT_COUNT(1),    0x01,       //   3 bits (Padding)
  REPORT_SIZE(1),     0x03,
  HIDOUTPUT(1),       0x01,       //   Const, Array, Abs
  END_COLLECTION(0)               // End application collection
};


BLEHIDDevice* hid;
BLECharacteristic* input;
BLECharacteristic* output;

const InputReport NO_KEY_PRESSED = { };


/*
   Callbacks related to BLE connection
*/
class BleKeyboardCallbacks : public BLEServerCallbacks {

    void onConnect(BLEServer* server) {
      isBleConnected = true;

      // Allow notifications for characteristics
      BLE2902* cccDesc = (BLE2902*)input->getDescriptorByUUID(BLEUUID((uint16_t)0x2902));
      cccDesc->setNotifications(true);

      Serial.println("Client has connected");
      Serial.println("REVISION:");
      Serial.println(REVISION_STAND);
    }

    void onDisconnect(BLEServer* server) {
      isBleConnected = false;

      // Disallow notifications for characteristics
      BLE2902* cccDesc = (BLE2902*)input->getDescriptorByUUID(BLEUUID((uint16_t)0x2902));
      cccDesc->setNotifications(false);

      Serial.println("Client has disconnected");
    }
};


/*
   Called when the client (computer, smart phone) wants to turn on or off
   the LEDs in the keyboard.

   bit 0 - NUM LOCK
   bit 1 - CAPS LOCK
   bit 2 - SCROLL LOCK
*/
class OutputCallbacks : public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic* characteristic) {
      OutputReport* report = (OutputReport*) characteristic->getData();
      Serial.print("LED state: ");
      Serial.print((int) report->leds);
      Serial.println();
    }
};


void bluetoothTask(void*) {

  // initialize the device
  BLEDevice::init(DEVICE_NAME);
  BLEServer* server = BLEDevice::createServer();
  server->setCallbacks(new BleKeyboardCallbacks());

  // create an HID device
  hid = new BLEHIDDevice(server);
  input = hid->inputReport(1); // report ID
  output = hid->outputReport(1); // report ID
  output->setCallbacks(new OutputCallbacks());

  // set manufacturer name
  hid->manufacturer()->setValue("Maker Community");
  // set USB vendor and product ID
  hid->pnp(0x02, 0xe502, 0xa111, 0x0210);
  // information about HID device: device is not localized, device can be connected
  hid->hidInfo(0x00, 0x02);

  // Security: device requires bonding
  BLESecurity* security = new BLESecurity();
  security->setAuthenticationMode(ESP_LE_AUTH_BOND);

  // set report map
  hid->reportMap((uint8_t*)REPORT_MAP, sizeof(REPORT_MAP));
  hid->startServices();

  // set battery level to 100%
  hid->setBatteryLevel(100);

  // advertise the services
  BLEAdvertising* advertising = server->getAdvertising();
  advertising->setAppearance(HID_KEYBOARD);
  advertising->addServiceUUID(hid->hidService()->getUUID());
  advertising->addServiceUUID(hid->deviceInfo()->getUUID());
  advertising->addServiceUUID(hid->batteryService()->getUUID());
  advertising->start();

  Serial.println("BLE ready");
  delay(portMAX_DELAY);
};


void typeText(const char* text) {
  int len = strlen(text);
  for (int i = 0; i < len; i++) {

    // translate character to key combination
    uint8_t val = (uint8_t)text[i];
    if (val > KEYMAP_SIZE)
      continue; // character not available on keyboard - skip
    KEYMAP map = keymap[val];

    // create input report
    InputReport report = {
      .modifiers = map.modifier,
      .reserved = 0,
      .pressedKeys = {
        map.usage,
        0, 0, 0, 0, 0
      }
    };

    // send the input report
    input->setValue((uint8_t*)&report, sizeof(report));
    input->notify();

    delay(5);

    // release all keys between two characters; otherwise two identical
    // consecutive characters are treated as just one key press
    input->setValue((uint8_t*)&NO_KEY_PRESSED, sizeof(NO_KEY_PRESSED));
    input->notify();

    delay(5);
  }
}


void sendKeyCode(uint8_t keyCode) {
  // create input report
  InputReport report = {
    .modifiers = 0,
    .reserved = 0,
    .pressedKeys = { keyCode, 0, 0, 0, 0, 0 }
  };

  // send the input report
  input->setValue((uint8_t*)&report, sizeof(report));
  input->notify();
  delay(5);

  // release all keys
  input->setValue((uint8_t*)&NO_KEY_PRESSED, sizeof(NO_KEY_PRESSED));
  input->notify();
  delay(5);
}


void sendMouseClick(uint8_t button) {
  // Erstelle ein Report-Objekt für Mausaktionen
  uint8_t report[] = {0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

  // Setze den entsprechenden Maus-Button im Report
  switch (button) {
    case MOUSE_BUTTON_LEFT:
      report[0] |= 0x01; // Bit 0 für linke Maustaste setzen
      break;
    case MOUSE_BUTTON_RIGHT:
      report[0] |= 0x02; // Bit 1 für rechte Maustaste setzen
      break;
    case MOUSE_BUTTON_MIDDLE:
      report[0] |= 0x04; // Bit 2 für mittlere Maustaste setzen
      break;
    default:
      break;
  }

  // Sende den Report über das HID-Protokoll
  input->setValue(report, sizeof(report));
  input->notify();
}
