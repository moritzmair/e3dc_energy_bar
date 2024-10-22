#include <ESP8266WiFi.h>
#include <ModbusIP_ESP8266.h>
#include <Adafruit_NeoPixel.h>
#include <math.h>

#define LED_PIN   5
#define NUMPIXELS 72
#define ANIMATION_INTERVAL 20  // Zeit in Millisekunden zwischen LED-Schritten

Adafruit_NeoPixel pixels(NUMPIXELS, LED_PIN, NEO_GRB + NEO_KHZ800);

union DoubleRegister {
    int32_t  dr;    // occupies 4 bytes
    uint16_t sr[2]; // occupies 4 bytes
};

DoubleRegister batGrid;
DoubleRegister batBat;
DoubleRegister batSolar;
DoubleRegister batHome;

#define REG_OFFSET -1
#define REG_GRID   40074
#define REG_BAT    40070
#define REG_SOLAR  40068
#define REG_HOME   40072

IPAddress remote(192, 168, 178, 87);  // Modbus Slave IP

ModbusIP mb;  // ModbusIP object

int production_grid = 0;
int production_bat = 0;
int production_solar = 0;

int consumption_grid = 0;
int consumption_bat = 0;
int consumption_house = 0;

int divider = 2;

uint32_t leds[NUMPIXELS*100];


void setup() {
  Serial.begin(9600);
  pixels.begin();
  pixels.setBrightness(15);
  
  WiFi.begin("SSID", "Password");
  
  Serial.println("Connecting to WiFi...");
  while (WiFi.status() != WL_CONNECTED) {
    pixels.setPixelColor(0, pixels.Color(255, 0, 0));  // Rote LED während der Verbindung
    pixels.show();
    delay(500);
    pixels.clear();
    pixels.show();
    delay(500);
  }
  
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  mb.client();
}

void loop() {
  if (mb.isConnected(remote)) {  // Check if Modbus Slave is connected
    mb.readHreg(remote, REG_GRID + REG_OFFSET, &batGrid.sr[0], 2);
    mb.readHreg(remote, REG_BAT + REG_OFFSET, &batBat.sr[0], 2);
    mb.readHreg(remote, REG_SOLAR + REG_OFFSET, &batSolar.sr[0], 2);
    mb.readHreg(remote, REG_HOME + REG_OFFSET, &batHome.sr[0], 2);
  } else {
    mb.connect(remote);  // Try to reconnect
  }

  delay(50);  // Short delay between Modbus tasks
  mb.task();
  delay(50);

  updateValues();
  animateLEDs();

  delay(500);
}

void updateValues() {
  // Update the production and consumption values based on Modbus readings
  production_grid = (batGrid.dr < 0) ? 0 : batGrid.dr;
  production_bat = (batBat.dr < 0) ? -batBat.dr : 0;
  production_solar = batSolar.dr;

  consumption_grid = (batGrid.dr < 0) ? -batGrid.dr : 0;
  consumption_bat = (batBat.dr > 0) ? batBat.dr : 0;
  consumption_house = batHome.dr;
}

void animateLEDs() {
  float led_production_grid = production_grid / divider;
  float led_production_bat = production_bat / divider;
  float led_production_solar = production_solar / divider;

  float led_consumption_grid = consumption_grid / divider;
  float led_consumption_bat = consumption_bat / divider;
  float led_consumption_house = consumption_house / divider;

  // Start point for production LEDs (right side)
  int startpoint = NUMPIXELS * 100 / 2;

  // Animate production values (right side)
  setSection(startpoint, led_production_grid, pixels.Color(0, 0, 255), true);
  setSection(startpoint + led_production_grid, led_production_bat, pixels.Color(0, 255, 0), true);
  setSection(startpoint + led_production_grid + led_production_bat, led_production_solar, pixels.Color(255, 255, 0), true);

  // Start point for consumption LEDs (left side)
  setSection(startpoint - 1, led_consumption_house, pixels.Color(255, 0, 0), false);
  setSection(startpoint - 1 - led_consumption_house, led_consumption_bat, pixels.Color(255, 255, 255), false);
  setSection(startpoint - 1 - led_consumption_house - led_consumption_bat, led_consumption_grid, pixels.Color(255, 0, 255), false);
  

  displayPixels();
}

void setSection(int start, int length, uint32_t color, bool forward) {
  for (int i = 0; i < length; i++) {
    int index = forward ? start + i : start - i;
    if (index >= 0 && index < NUMPIXELS * 100) {
      leds[index] = color;
    }
  }
}

void displayPixels() {
  pixels.clear();
  for (int i = 0; i < NUMPIXELS; i++) {
    pixels.setPixelColor(i, averageColor(leds, i*100, 100));
  }
  pixels.show();
}

uint32_t averageColor(uint32_t colors[], int startIndex, int numColors) {
    uint32_t sumR = 0;
    uint32_t sumG = 0;
    uint32_t sumB = 0;

    // Durchlaufe die Anzahl der Farben ab dem angegebenen Startindex
    for (int i = startIndex; i < startIndex + numColors; i++) {
        sumR += (colors[i] >> 16) & 0xFF;  // Extrahiere den Rot-Wert
        sumG += (colors[i] >> 8) & 0xFF;   // Extrahiere den Grün-Wert
        sumB += colors[i] & 0xFF;          // Extrahiere den Blau-Wert
    }

    // Berechne den Durchschnitt
    uint32_t avgR = sumR / numColors;
    uint32_t avgG = sumG / numColors;
    uint32_t avgB = sumB / numColors;

    // Setze den Durchschnitt in einen uint32_t zurück
    return pixels.Color(avgR, avgG, avgB);
}