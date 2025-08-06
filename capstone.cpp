/* 
 * Project capstone
 * Author: Aarav Patel
 * Date: 8/1/2025
 * For comprehensive documentation and examples, please visit:
 * https://docs.particle.io/firmware/best-practices/firmware-template/
 */

// Include Particle Device OS APIs
#include "Particle.h"
#include "Adafruit_GFX.h" 
#include "Adafruit_SSD1306.h" 
#include "Encoder.h"
#include "Button.h"
#include "iotclassroom_cnm.h"
#include "neopixel.h"
#include "Adafruit_BME280.h"
#include "colors.h"
#include <Adafruit_MQTT.h>
#include "Adafruit_MQTT/Adafruit_MQTT_SPARK.h"
#include "Adafruit_MQTT/Adafruit_MQTT.h"
#include "credentials.h"
// Let Device OS manage the connection to the Particle Cloud
//adafruit 
/************ Global State (you don't need to change this!) ***   ***************/ 
TCPClient TheClient; 

// Setup the MQTT client class by passing in the WiFi client and MQTT server and login details. 
Adafruit_MQTT_SPARK mqtt(&TheClient,AIO_SERVER,AIO_SERVERPORT,AIO_USERNAME,AIO_KEY); 

/****************************** Feeds ***************************************/ 
// Setup Feeds to publish or subscribe 
// Notice MQTT paths for AIO follow the form: <username>/feeds/<feedname> 

Adafruit_MQTT_Publish pubFeed = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/mode");


unsigned int last, lastTime;
float subValue,pubValue;


void MQTT_connect();
bool MQTT_ping();
//bluefruit 
const size_t UART_TX_BUF_SIZE = 20;
uint8_t txBuf[UART_TX_BUF_SIZE];
uint8_t i;
const int BUFSIZE = 50;
byte buf[BUFSIZE];
int bleMode;
//oled
const int OLED_RESET=-1;
Adafruit_SSD1306 display(OLED_RESET);
SYSTEM_MODE(SEMI_AUTOMATIC);
//encoder
Encoder myEnc(D8,D9);
Button modeButton(D4);
//timer
unsigned int lastTime2;
unsigned int currentTime;
int blinkState;

// pong variables
unsigned int lastMoveTime;
int ballXStartPosition= 64;
int ballYStartPosition = 32;
int ballSpeedVertical = 3;
int ballSpeedhorizontal = 3;
int paddleXStartPosition = 55;
int paddleYStartPosition = 60;
int paddleWidth = 20;
int paddleHeight = 3;
int lives = 3;
int score;
//bme
char number = 164;


Adafruit_BME280 bme;
int hexAddress = 0x76;
float temp, pres, humid, Fahrenheit, inHg;
bool status;

//modes variables
int selectedMode;
int totalModes = 4;
int lastPosition = -1;
int encoderPosition;
int Mode;
String modeNames[] = {"Game","Chill","Party","DND"};
//functions
void menuUpdate();
void enterMode(int mode);
void chillMode();
void dndMOde();
void partyMode();
void gameMode();
void runMode(int mode);
void pixelFill(int startpixel, int endpixel, int pixelColor);
SYSTEM_THREAD(ENABLED);
//neopixels
const int PIXELCOUNT  = 12;
Adafruit_NeoPixel pixel(PIXELCOUNT, SPI1, WS2812B);
int n;
//bluefruit
const BleUuid serviceUuid("6E400001-B5A3-F393-E0A9-E50E24DCCA9E");
const BleUuid rxUuid("6E400002-B5A3-F393-E0A9-E50E24DCCA9E");
const BleUuid txUuid("6E400003-B5A3-F393-E0A9-E50E24DCCA9E");

void onDataReceived(const uint8_t* data, size_t len, const BlePeerDevice& peer, void* context);

BleCharacteristic txCharacteristic("tx", BleCharacteristicProperty::NOTIFY, txUuid, serviceUuid);
BleCharacteristic rxCharacteristic("rx", BleCharacteristicProperty::WRITE_WO_RSP, rxUuid, serviceUuid, onDataReceived, NULL);
BleAdvertisingData data;


void setup() {
  Serial.begin(9600);
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.display();
  pixel.begin();
  pixel.setBrightness(100);
  pixel.show();
  selectedMode = 0;
  menuUpdate();
  // Connect to Internet but not Particle Cloud
  WiFi.on();
  WiFi.connect();
  while(WiFi.connecting()) {
    Serial.printf(".");
  }
  Serial.printf("\n\n");
  //bme
    status = bme.begin(hexAddress);
  if (status == TRUE) {
    Serial.printf("BME280 at address 0x%02X started successfully!\n",hexAddress);
}
//bluefruit
  BLE.on();
    BLE.addCharacteristic(txCharacteristic);
    BLE.addCharacteristic(rxCharacteristic);
    data.appendServiceUUID(serviceUuid);
    BLE.advertise(&data);

    Serial.printf("Photon2 BLE Address: %s\n", BLE.address().toString().c_str());
}

void loop() {
  //adafruit io
    MQTT_connect();
  MQTT_ping();
   if((millis()-lastTime2 > 6000)) {
    if(mqtt.Update()) {
      pubValue = selectedMode;
      pubFeed.publish(pubValue);
      Serial.printf("Publishing %0.2f \n",pubValue); 
      } 
    lastTime2 = millis();
  }
  //bluefruit
  sprintf((char *)buf, "the mode is %i\n", selectedMode);
  buf[BUFSIZE - 1] = 0x0A;
  Serial.printf("Sending 'buf' contains the string: %s", (char*)buf);
 txCharacteristic.setValue(buf, BUFSIZE);
//bme
temp = bme.readTemperature();
Fahrenheit = (temp * 9/5) + 32;
  encoderPosition = myEnc.read() /4;
 if(encoderPosition != lastPosition && Mode == 0){
    lastPosition = encoderPosition;
     selectedMode = encoderPosition ;

if(selectedMode >= totalModes){
 selectedMode = 0;
  }
if(selectedMode < 0){
selectedMode = 0;
  }
  menuUpdate();
}
if (modeButton.isClicked()) {
  if (Mode == 0) {
    Mode = 1;
    enterMode(selectedMode);
  } else {
    Mode = 0;
    menuUpdate();
  }
}
 if (Mode == 1) {
    runMode(selectedMode);
  }
Serial.printf("encoder position = %i\n",encoderPosition);
}
void menuUpdate() {
  display.clearDisplay();
  display.setTextSize(1);
 display.setTextColor(WHITE);
  display.setCursor(10,0);
  display.printf("Select Mode");
  display.setTextSize(1);
  display.setCursor(0, 30);
  display.printf("> ");
  display.printf(modeNames[selectedMode].c_str());
  display.display();
}
void enterMode(int mode) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(10, 0);
  display.printf("Mode %s", modeNames[mode].c_str());
  display.display();
}

void runMode(int selectedMode){
  if(selectedMode == 0){
   gameMode();
  }
  if(selectedMode == 1){
    chillMode();
  }
  if(selectedMode == 2){
    partyMode();
  }
  if(selectedMode == 3){
    dndMOde();
  }
}
void chillMode() {
 display.clearDisplay();
 display.setTextSize(1);
 display.setTextColor(WHITE);
 display.setCursor(0, 10);
 display.printf("Chill Mode");
 display.setTextSize(1);
 display.setCursor(0, 30);
 display.printf("Relax and enjoy!");
  display.setTextSize(1);
 display.setCursor(0,50);
  display.printf("Temperature %0.1fF\n",Fahrenheit);
 display.display();


  currentTime = millis();
   if (( currentTime - lastTime ) >1000) {
 lastTime = millis () ;
 Serial . printf (".") ;
blinkState =!blinkState;
 if(blinkState == 0){
  pixelFill(0,12,cyan);
 }
 else{
  pixelFill(0,12,0);
 }
 }
 
 pixel.show();
 delay(100);
}
void dndMOde(){
   display.clearDisplay();
 display.setTextSize(1);
 display.setTextColor(WHITE);
 display.setCursor(0, 10);
 display.printf("Do not distrub");
 display.setTextSize(1);
 display.setCursor(0, 30);
 display.printf("focus on work");
 display.display();

 pixelFill(0,12,red);
}
void pixelFill(int startpixel, int endpixel, int pixelColor) {
  for (int n = startpixel; n <= endpixel; n++) {
    pixel.setPixelColor(n, pixelColor);
  }
  pixel.show();
}

 void partyMode(){
    display.clearDisplay();
 display.setTextSize(1);
 display.setTextColor(WHITE);
 display.setCursor(0, 10);
 display.printf("PARTY MODE!");
 display.setTextSize(1);
 display.setCursor(0, 30);
 display.printf("go have funnn");
 display.display();

 
  currentTime = millis();
   if (( currentTime - lastTime ) >1000) {
 lastTime = millis () ;
 Serial . printf (".") ;
blinkState =!blinkState;
 if(blinkState == 0){
 pixelFill(0,2,rainbow[0]);
  pixelFill(2,4,rainbow[2]);
  pixelFill(4,6,rainbow[3]);
  pixelFill(6,8,rainbow[4]);
  pixelFill(8,10,rainbow[5]);
  pixelFill(10,12,rainbow[6]);
 }
 else{
  pixelFill(0,12,0);
 }
 }
}

void gameMode(){
  
    currentTime = millis();
   if (( currentTime - lastMoveTime ) >50) {
 lastMoveTime = millis () ;
 Serial . printf (".") ;
 ballXStartPosition =ballXStartPosition + ballSpeedhorizontal;
 ballYStartPosition = ballYStartPosition + ballSpeedVertical;

 if(ballXStartPosition <= 0){
  ballXStartPosition = 0;
  ballSpeedhorizontal = - ballSpeedhorizontal;
 
}
if(ballXStartPosition > 127){
  ballXStartPosition = 127;
  ballSpeedhorizontal = -ballSpeedhorizontal;
}
if(ballYStartPosition <=0){
  ballYStartPosition = 0;
  ballSpeedVertical = -ballSpeedVertical;
}
if(ballYStartPosition > paddleYStartPosition + paddleHeight){
 lives--;
 
  if( lives < 3 && lives > 0){
    pixel.setPixelColor(lives,black);
  }
 
 pixel.show();
 ballXStartPosition = 64;
 ballYStartPosition = 32;
 ballSpeedhorizontal = 3;
 ballSpeedVertical = 3;
}

 if(ballYStartPosition >= paddleYStartPosition){
  if(ballXStartPosition > paddleXStartPosition && ballXStartPosition <= paddleXStartPosition + paddleWidth){
    ballSpeedVertical = -ballSpeedVertical;
    score++;
  }
 }
}
  encoderPosition = myEnc.read() / 4;
 if (encoderPosition < 0) {
  encoderPosition = 0;
}
if (encoderPosition > 96) {
  encoderPosition = 96;
}
paddleXStartPosition = encoderPosition;
display.clearDisplay();
display.fillCircle(ballXStartPosition,ballYStartPosition,2,WHITE);
display.fillRect(paddleXStartPosition,paddleYStartPosition,paddleWidth,3,WHITE);


 display.setTextSize(1);
 display.setTextColor(WHITE);
 display.setCursor(0,0);
 display.printf("Score = %i lives = %i",score,lives);
 display.display();

 if(lives ==0){
   display.clearDisplay();
 display.setTextSize(2);
 display.setTextColor(WHITE);
 display.setCursor(0,10);
 display.printf("Game Over you lose");
 display.display();
 delay(3000);

 lives = 3;
 score = 0;
 ballXStartPosition = 64;
 ballYStartPosition = 32;
 ballSpeedhorizontal = 3;
 ballSpeedVertical = 3;
 paddleXStartPosition = 55;
  for(n = 0;n < lives;n++){
  if( n < lives){
    pixel.setPixelColor(n,red);
  }
  pixel.show();
 }
}
}
//onDataReceived is used to receive data from Bluefruit Connect App
void onDataReceived(const uint8_t* data, size_t len, const BlePeerDevice& peer, void* context) {
  uint8_t i;

  Serial.printf("Received data from: %02X:%02X:%02X:%02X:%02X:%02X \n", peer.address()[0], peer.address()[1],peer.address()[2], peer.address()[3], peer.address()[4], peer.address()[5]);
    Serial.printf("Bytes: ");
  for (i = 0; i < len; i++) {
    Serial.printf(" %02X ", data[i]);
  }
  Serial.printf("\n");

  Serial.printf("Message: %s\n", (char*)data);
  bleMode = atoi((const char*)data);
  if(bleMode >=0 && bleMode < totalModes){
    selectedMode = bleMode;
    menuUpdate();
  }

 
  
}
// Function to connect and reconnect as necessary to the MQTT server.
// Should be called in the loop function and it will take care if connecting.
void MQTT_connect() {
  int8_t ret;
 
  // Return if already connected.
  if (mqtt.connected()) {
    return;
  }
 
  Serial.print("Connecting to MQTT... ");
 
  while ((ret = mqtt.connect()) != 0) { // connect will return 0 for connected
       Serial.printf("Error Code %s\n",mqtt.connectErrorString(ret));
       Serial.printf("Retrying MQTT connection in 5 seconds...\n");
       mqtt.disconnect();
       delay(5000);  // wait 5 seconds and try again
  }
  Serial.printf("MQTT Connected!\n");
}

bool MQTT_ping() {
  static unsigned int last;
  bool pingStatus;

  if ((millis()-last)>120000) {
      Serial.printf("Pinging MQTT \n");
      pingStatus = mqtt.ping();
      if(!pingStatus) {
        Serial.printf("Disconnecting \n");
        mqtt.disconnect();
      }
      last = millis();
  }
  return pingStatus;
}