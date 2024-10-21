
#include <ESP8266WiFi.h>
#include <ModbusIP_ESP8266.h>
#include <Adafruit_NeoPixel.h>

#define LED_PIN   5
#define NUMPIXELS 72

Adafruit_NeoPixel pixels(NUMPIXELS, LED_PIN, NEO_GRB + NEO_KHZ800);

union DoubleRegister
{
    int32_t  dr;    // occupies 4 bytes
    uint16_t sr[2]; // occupies 4 bytes
};

DoubleRegister batGrid;
DoubleRegister batBat;
DoubleRegister batSolar;
DoubleRegister batHome;

#define REG_OFFSET                  -1

#define REG_GRID                    40074
#define REG_BAT                     40070
#define REG_SOLAR                   40068
#define REG_HOME                    40072

IPAddress remote(192, 168, 178, 87);  // Address of Modbus Slave device

ModbusIP mb;  //ModbusIP object

int production_grid = 0;
int production_bat = 0;
int production_solar = 0;

int consumption_grid = 0;
int consumption_bat = 0;
int consumption_house = 0;

int divider = 200;

void setup() {
  Serial.begin(9600);

  pixels.begin();
 
  WiFi.begin("SSID", "password");

  Serial.println("connecting to WiFi");
  
  while (WiFi.status() != WL_CONNECTED) {
    pixels.setPixelColor(0, pixels.Color(255, 0, 0));
    pixels.show();
    delay(250);
    pixels.clear();
    pixels.show();
    delay(250);
    Serial.print(".");
  }
 
  Serial.println("");
  Serial.println("WiFi connected");  
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  mb.client();
}

void loop() {
  
  if (mb.isConnected(remote)) {   // Check if connection to Modbus Slave is established
    mb.readHreg(remote, REG_GRID + REG_OFFSET, &batGrid.sr[0], 2);
    mb.readHreg(remote, REG_BAT + REG_OFFSET, &batBat.sr[0], 2);
    mb.readHreg(remote, REG_SOLAR + REG_OFFSET, &batSolar.sr[0], 2);
    mb.readHreg(remote, REG_HOME + REG_OFFSET, &batHome.sr[0], 2);
  } else {
    mb.connect(remote);           // Try to connect if no connection
  }
  
  delay(50);
  mb.task();                      // Common local Modbus task
  delay(50);
  
  Serial.print("GRID: ");
  Serial.println(batGrid.dr);
  
  Serial.print("battery: ");
  Serial.println(batBat.dr);
  
  Serial.print("solar: ");
  Serial.println(batSolar.dr);

  Serial.print("home: ");
  Serial.println(batHome.dr);

  Serial.println();

  if(batGrid.dr < 0){
    production_grid = 0;
    consumption_grid = batGrid.dr;
  }else{
    production_grid = batGrid.dr;
    consumption_grid = 0;
  }
  
  if(batBat.dr < 0){
    production_bat = batBat.dr*-1;
    consumption_bat = 0;
  }else{
    production_bat = 0;
    consumption_bat = batBat.dr;
  }
  production_solar = batSolar.dr;

  consumption_house = batHome.dr;

  production_grid = production_grid/divider;
  production_bat = production_bat/divider;
  production_solar = production_solar/divider;
  
  consumption_grid = consumption_grid/divider;
  consumption_bat = consumption_bat/divider;
  consumption_house = consumption_house/divider;

  pixels.clear();

  int startpoint = NUMPIXELS/2;

  for(int i=startpoint; i<startpoint+production_grid; i++){
    pixels.setPixelColor(i, pixels.Color(0, 0, 3));
  }
  for(int i=startpoint+production_grid; i< startpoint+production_grid+production_bat; i++){
    pixels.setPixelColor(i, pixels.Color(0, 3, 0));
  }
  for(int i=startpoint+production_grid+production_bat; i< startpoint+production_grid+production_bat+production_solar; i++){
    pixels.setPixelColor(i, pixels.Color(2, 2, 0));
  }

  startpoint--;

  for(int i=startpoint; i>startpoint-consumption_house; i--){
    pixels.setPixelColor(i, pixels.Color(2, 0, 0));
  }
  for(int i=startpoint-consumption_house; i> startpoint-consumption_house-consumption_bat; i--){
    pixels.setPixelColor(i, pixels.Color(2, 2, 2));
  }
  for(int i=startpoint-consumption_house-consumption_bat; i> startpoint-consumption_house-consumption_bat-consumption_grid; i--){
    pixels.setPixelColor(i, pixels.Color(2, 0, 2));
  }

  
  pixels.show();
  
  delay(1000);                     // Pulling interval
}
