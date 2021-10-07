//ESPboy Joystick
//for www.espboy.com project
//by RomanS

#include "ADS1X15.h" //https://github.com/RobTillaart/ADS1X15
#include "ESP_EEPROM.h" //https://github.com/jwrw/ESP_EEPROM

#include "lib/ESPboyInit.h"
#include "lib/ESPboyInit.cpp"

#define SAVE_MARKER 0xAAEE

#define REFRESH_RATE 30
#define CIRCLE_ELEMENTS 20
#define BOARDER_SPACE 5

#define MAX_CIRCLE_RADIUS 20
#define MIN_CIRCLE_RADIUS 5
#define MAX_CIRCLE_CHANGE_SPEED 4
#define MIN_CIRCLE_CHANGE_SPEED 2

#define SPRITE_WIDTH  128
#define SPRITE_HEIGHT 128

ADS1115 ADS(0x48);
ESPboyInit myESPboy;
TFT_eSprite sprite = TFT_eSprite(&myESPboy.tft); 

uint32_t colors[5]={TFT_GREEN, TFT_BLUE, TFT_RED, TFT_YELLOW, TFT_MAGENTA};

struct JOY{
  int32_t maxAdsX=0, minAdsX=0;
  int32_t maxAdsY=0, minAdsY=0;
  int32_t centerX=0, centerY=0;
  int32_t lastX, lastY;
}joyParam;


struct SAVE{ 
  uint32_t saveMarker;
  int32_t maxAdsX=0, minAdsX=0;
  int32_t maxAdsY=0, minAdsY=0;
  int32_t centerX=0, centerY=0;
}saveData;


struct CIRCLE{
  int16_t x, y, r, rt, color;
  int8_t delta;
}circleArray[CIRCLE_ELEMENTS+1];


struct CROSS{
  int16_t x=64,y=64;
}cross;



void joystickCalibration(){
 uint32_t tmr;
 uint32_t getX, getY;
  myESPboy.tft.setTextColor(TFT_GREEN, TFT_BLACK);
  myESPboy.tft.setTextSize(2);
  myESPboy.tft.drawString("CALIBRATE", (128-9*12)/2, 0);
  myESPboy.tft.setTextSize(1);
  myESPboy.tft.drawString("move joyst all sides", 0, 20);

  tmr = millis();
  while (millis()-tmr < 5000){
    getX = ADS.readADC(0);
    getY = ADS.readADC(1);
    if (joyParam.maxAdsX < getX) joyParam.maxAdsX = getX;
    if (joyParam.minAdsX > getX) joyParam.minAdsX = getX;
    if (joyParam.maxAdsY < getY) joyParam.maxAdsY = getY;
    if (joyParam.minAdsY > getY) joyParam.minAdsY = getY;
   delay(10);
   }
   myESPboy.tft.drawString("OK", 0, 30);
   myESPboy.tft.drawString("don't touch joystick", 0, 40);
   delay(3000);
   joyParam.centerX = ADS.readADC(0);
   joyParam.centerY = ADS.readADC(1);
   myESPboy.tft.drawString("OK", 0, 50);
   myESPboy.tft.drawString("SAVE...", 0, 60);

   saveData.maxAdsX = joyParam.maxAdsX;
   saveData.minAdsX = joyParam.minAdsX;
   saveData.maxAdsY = joyParam.maxAdsY;
   saveData.minAdsY = joyParam.minAdsY;
   saveData.centerX = joyParam.centerX;
   saveData.centerY = joyParam.centerY;
   saveData.saveMarker = SAVE_MARKER;
   EEPROM.put(0, saveData);
   EEPROM.commit();

   myESPboy.tft.drawString("OK", 0, 70);
   delay(1000);
   myESPboy.tft.fillScreen(TFT_BLACK);
}


void joyUpdate(){
   joyParam.lastX = ADS.readADC(0);
   joyParam.lastY = ADS.readADC(1);
}


void redrawScreen(){
  sprite.fillScreen(TFT_BLACK);
  for (uint16_t i=0; i<CIRCLE_ELEMENTS; i++)
    if(circleArray[i].r>0) 
      sprite.fillCircle(circleArray[i].x, circleArray[i].y, circleArray[i].r, circleArray[i].color);
  sprite.drawFastVLine(cross.x, cross.y-5, 11, TFT_WHITE);
  sprite.drawFastHLine(cross.x-5, cross.y, 11, TFT_WHITE);
}


void processData(){
  if (myESPboy.getKeys()){
    myESPboy.playTone(100,40);
    for (uint16_t i=0; i<CIRCLE_ELEMENTS; i++)
      if(circleArray[i].r == 0){
        circleArray[i].x = cross.x;
        circleArray[i].y = cross.y;  
        circleArray[i].r ++;
        circleArray[i].rt = random(MAX_CIRCLE_RADIUS) + MIN_CIRCLE_RADIUS;
        circleArray[i].delta = random(MAX_CIRCLE_CHANGE_SPEED) + MIN_CIRCLE_CHANGE_SPEED;
        circleArray[i].color = colors[random(sizeof(colors)/sizeof(uint32_t))];
        break;
      }
  }

  for (uint16_t i=0; i<CIRCLE_ELEMENTS; i++)
   if (circleArray[i].r != 0){
     circleArray[i].r += circleArray[i].delta;
     if(circleArray[i].r < 0) circleArray[i].r = 0;
     if(circleArray[i].r > circleArray[i].rt) circleArray[i].delta = -circleArray[i].delta;
   }

   int32_t deltaMoveX, deltaMoveY;
   deltaMoveX = ((joyParam.lastX-joyParam.minAdsX)-(joyParam.maxAdsX-joyParam.minAdsX)/2)/500;
   deltaMoveY = ((joyParam.lastY-joyParam.minAdsY)-(joyParam.maxAdsY-joyParam.minAdsY)/2)/500;
   if (abs(deltaMoveX)>4){
     cross.x += deltaMoveX;
     if(cross.x>127) cross.x = 127;
     if(cross.x<0) cross.x = 0;}     
   if (abs(deltaMoveY)>4){
     cross.y += deltaMoveY;
     if(cross.y>127) cross.y = 127;
     if(cross.y<0) cross.y = 0;}
}


void loadParameters(){
  EEPROM.get(0, saveData);
  if (saveData.saveMarker != SAVE_MARKER)
    joystickCalibration();
  else{
    joyParam.maxAdsX = saveData.maxAdsX;
    joyParam.minAdsX = saveData.minAdsX;
    joyParam.maxAdsY = saveData.maxAdsY;
    joyParam.minAdsY = saveData.minAdsY;
    joyParam.centerX = saveData.centerX;
    joyParam.centerY = saveData.centerY;    
  }
}



void setup() {
  Serial.begin(115200);
  EEPROM.begin(sizeof(saveData));
  myESPboy.begin("Joystick"); //Init ESPboy
  sprite.createSprite(SPRITE_WIDTH, SPRITE_HEIGHT);
  if (myESPboy.getKeys()) joystickCalibration();
  ADS.begin();
  ADS.setDataRate(7); 
  ADS.setMode(0); 
  loadParameters();
}


void loop(){
 static uint32_t tmrFPS=0;
  if(millis()-tmrFPS > 1000/REFRESH_RATE){
    tmrFPS = millis();
    joyUpdate();
    processData();
    redrawScreen();
    sprite.pushSprite(0, 0);
  }
}
