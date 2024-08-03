//==========================================
/*Bluetooth Fusstaster_ESP32
--------------------------------------------
- Fusstaster fuer das umblaettern von Seiten am IPAD/Iphone
- Programm emuliert eine Bluetooth-Tastatur

Anleitung Verbinden mit Ipad
1. Go to your computers/phones settings
2. Ensure Bluetooth is turned on
3. Scan for Bluetooth devices
4. Connect to the device called "ESP32...."
5. Select Key-Combination with selection-switch
6. Press the button attached to the ESP32 to send signals

Anleitung Benutzung Key-Combination
1. Key Up / Down
2. Key Left / Right
3. Key Pg Down / Pg Up
4. Key Space / Enter
*/

//==========================================
/* R03_1_Release; 03.08.2024, fr
    - nicht genutzten Code bereinigt: void Mousclick entfernt 
    - nicht getesteter Release
*/
/* R03_0_Release; 03.08.2024, fr
    - #define durch const ersetzt
    - Aufbau fuer neue Schaltung
    - Intergration Tastenentprellen
    - Einzelener Tastendruck statt mehrfache Befehle
    - Funktion eingefuehrt fuer das Ruecksetzen der LEDs
    - Reduktion der verschiedenen Varianten auf 4, nur Tastaturbefehle
    - getester Release
*/

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

const int US_KEYBOARD = 1;
const char* REVISION_STAND = "BluetoothFusstaster_ESP32_R03_1_Release"; //hier den Namen des Programms mit dem aktuellen Revisionsstand angeben

#include <Arduino.h>
#include "BLEDevice.h"
#include "BLEHIDDevice.h"
#include "HIDTypes.h"
#include "HIDKeyboardTypes.h"


// Change the below values if desired
const int MAIN_BUTTON_RIGHT = 4;
const int MAIN_BUTTON_LEFT = 16;
const int MAIN_BUTTON_SWITCH = 17;

const int DELAY_MAIN_BUTTON = 200;
const int DELAY_MAIN_BUTTON_SWITCH = 200;

int toggleStatusRight = 0;  // ToogleStatus um nur eine Schaltung auszuloesen
int toggleStatusLeft = 0;
int toggleStatusSwitch = 0;

//Variablen fuer das Entprellen
int buttonStateRight = 0; 
int buttonStateLeft = 0; 
int buttonStateSwitch = 0;

int lastButtonStateRight = 0;
int lastButtonStateLeft = 0;
int lastButtonStateSwitch = 0;

bool buttonPressedRight = false;
bool buttonPressedLeft = false;
bool buttonPressedSwitch = false;

unsigned long lastDebounceTimeRight = 0;  // Die letzte Zeit, zu der der Tasterstatus geändert wurde
unsigned long lastDebounceTimeLeft = 0;  // Die letzte Zeit, zu der der Tasterstatus geändert wurde
unsigned long lastDebounceTimeSwitch = 0;  // Die letzte Zeit, zu der der Tasterstatus geändert wurde
unsigned long debounceDelay = 50;    // Zeit zum Entprellen in Millisekunden

// Definition LEDs
const int LED_STATUS_1 = 32;
const int LED_STATUS_2 = 33;
const int LED_STATUS_3 = 25;
const int LED_STATUS_4 = 26;

const int lengthArrayLedStatus = 4;
const int LED_STATUS[lengthArrayLedStatus] = {LED_STATUS_1, LED_STATUS_2, LED_STATUS_3, LED_STATUS_4};
int Status = 0;

const int LED_BLUETOOTH =14;

uint8_t COMMAND_LEFT[] = {
  0x52, // Keyboard Up Arrow,
  0x50, // Keyboard Left Arrow,
  0x4b, // Keyboard Page Up,
  0x2c // Keyboard Space Bar,
};

uint8_t COMMAND_RIGHT[] = {
  0x51, // Keyboard Down Arrow,
  0x4f, // Keyboard Right Arrow,
  0x4e, // Keyboard Page Down,
  0x28 // Keyboard Enter,
};

const char* DEVICE_NAME = "ESP32-Fusstaster"; //Name des Bluetooth Device, welcher am Ipad angezeigt wird

// Forward declarations
void bluetoothTask(void*);
void typeText(const char* text);

bool isBleConnected = false;
//-------------------------------------

void setup() {
  //Initialisierung Serial-Connection
  Serial.begin(115200);
  Serial.print("Device name: ");
  Serial.println(DEVICE_NAME);

  // configure pins for buttons
  pinMode(MAIN_BUTTON_RIGHT, INPUT_PULLUP);
  pinMode(MAIN_BUTTON_LEFT, INPUT_PULLUP);
  pinMode(MAIN_BUTTON_SWITCH, INPUT_PULLUP);

  // configure pins for Status-LEDs
  for (int i=0; i < lengthArrayLedStatus; i++) {
    pinMode (LED_STATUS[i], OUTPUT);
  }
  pinMode(LED_BLUETOOTH, OUTPUT);
  
// Initialisierung Status-LEDs
  resetLEDs(LED_STATUS,lengthArrayLedStatus);
  digitalWrite(LED_STATUS[Status], HIGH);
  digitalWrite(LED_BLUETOOTH, LOW);

  // start Bluetooth task
  xTaskCreate(bluetoothTask, "bluetooth", 20000, NULL, 5, NULL);
  
} //End Setup


void loop() {
  
//-----------------------------
// Abfrage der Bluetooth-Verbindung
//-----------------------------
  if (isBleConnected){
    digitalWrite(LED_BLUETOOTH, HIGH);     
  }
  else{
    digitalWrite(LED_BLUETOOTH, LOW);  
  }

//-----------------------------
// Taster Entprellen
//-----------------------------

// Lese den Zustand jedes Tasters
  int readingRight = digitalRead(MAIN_BUTTON_RIGHT);
  int readingLeft = digitalRead(MAIN_BUTTON_LEFT);
  int readingSwitch = digitalRead(MAIN_BUTTON_SWITCH);

//-------Button Switch
  if (readingSwitch != lastButtonStateSwitch) {
      lastDebounceTimeSwitch = millis();
    }
  if ((millis() - lastDebounceTimeSwitch) > debounceDelay) {
    if (readingSwitch != buttonStateSwitch) {
      buttonStateSwitch = readingSwitch;

      if (buttonStateSwitch == LOW) {
        buttonPressedSwitch = true;
      } else {
        buttonPressedSwitch = false;
        toggleStatusSwitch = 0;
      }
    }
  }
  
//-------Button Right
  if (readingRight != lastButtonStateRight) {
    lastDebounceTimeRight = millis();
  }
  if ((millis() - lastDebounceTimeRight) > debounceDelay) {
    if (readingRight != buttonStateRight) {
      buttonStateRight = readingRight;

      if (buttonStateRight == LOW) {
        buttonPressedRight = true;
      } else {
        buttonPressedRight = false;
        toggleStatusRight = 0;
      }
    }
  }
//-------Button Left
  if (readingLeft != lastButtonStateLeft) {
    lastDebounceTimeLeft = millis();
  }
  if ((millis() - lastDebounceTimeLeft) > debounceDelay) {
    if (readingLeft != buttonStateLeft) {
      buttonStateLeft = readingLeft;

      if (buttonStateLeft == LOW) {
        buttonPressedLeft = true;
      } else {
        buttonPressedLeft = false;
        toggleStatusLeft = 0;
      }
    }
  }
  
//-----------------------------
// Taster Ereignisse
//-----------------------------   
// Taster Switch 
  if (toggleStatusSwitch == 0 && buttonPressedSwitch == true) {
    resetLEDs(LED_STATUS,lengthArrayLedStatus);
    
    Status++;
    if (Status >= lengthArrayLedStatus){
      Status = 0;
    }
    Serial.print("Das ist der Status: ");
    Serial.println(Status);
    digitalWrite(LED_STATUS[Status], HIGH);
    toggleStatusSwitch = 1;
  }
    
// Taster Right
  if (isBleConnected && toggleStatusRight == 0 && buttonPressedRight == true) {
    //if (isBleConnected && buttonPressedRight == true) {
    Serial.print("COMMAND_RIGHT: ");
    Serial.println(COMMAND_RIGHT[Status]);
    sendKeyCode(COMMAND_RIGHT[Status]);
    toggleStatusRight = 1;
  }

// Taster Left
  if (isBleConnected && toggleStatusLeft == 0 && buttonPressedLeft == true) {
    Serial.print("COMMAND_LEFT: ");
    Serial.println(COMMAND_LEFT[Status]);
    sendKeyCode(COMMAND_LEFT[Status]);
    toggleStatusLeft = 1;
  }

// Aktualisiere den letzten Zustand jedes Tasters
  lastButtonStateRight = readingRight;
  lastButtonStateLeft = readingLeft;
  lastButtonStateSwitch = readingSwitch;

} //End Loop


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
   // advertising->setAppearance(HID_KEYBOARD);
  advertising->setAppearance(HID_MOUSE);
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

void resetLEDs(const int* leds, const int length) {
  for (int i = 0; i < length; i++) {
    digitalWrite(leds[i], LOW);
  }
}
