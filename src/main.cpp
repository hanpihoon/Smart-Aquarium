#include <Arduino.h>
#include <SPI.h>
#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>

// ESP32-2432S028R (CYD). Touch is on its own VSPI bus on common revisions.
static const int TOUCH_CS=33, TOUCH_IRQ=36, TOUCH_CLK=25, TOUCH_MISO=39, TOUCH_MOSI=32;
static const int TFT_BL=21;
TFT_eSPI tft;
SPIClass touchSPI(VSPI);
XPT2046_Touchscreen ts(TOUCH_CS, TOUCH_IRQ);

// Change these after touch calibration if necessary.
int TS_MINX=200, TS_MAXX=3900, TS_MINY=200, TS_MAXY=3900;

enum Page { HOME, WATER, TEMP, MANUAL, SETTINGS };
Page page=HOME;
float tempC=25.3, targetTemp=25.0;
int changePct=20;
bool autoWater=true, valveIn=false, valveOut=false, heater=false, chiller=false;
unsigned long lastDemo=0;

uint16_t C_BG=0x0841, C_PANEL=0x10A2, C_PANEL2=0x18E3, C_TEXT=TFT_WHITE, C_MUTED=0x9CF3, C_ACCENT=0x04FF, C_OK=0x07E0, C_WARN=0xFD20;

void text(const String&s,int x,int y,int size=1,uint16_t c=TFT_WHITE){tft.setTextColor(c,C_BG);tft.setTextSize(size);tft.setCursor(x,y);tft.print(s);} 
void panel(int x,int y,int w,int h,const String&title){tft.fillRoundRect(x,y,w,h,8,C_PANEL);tft.setTextColor(C_MUTED,C_PANEL);tft.setTextSize(1);tft.setCursor(x+8,y+7);tft.print(title);} 
void button(int x,int y,int w,int h,const String&s,bool on=false){uint16_t c=on?C_ACCENT:C_PANEL2;tft.fillRoundRect(x,y,w,h,7,c);tft.drawRoundRect(x,y,w,h,7,0x39E7);tft.setTextColor(TFT_WHITE,c);tft.setTextDatum(MC_DATUM);tft.drawString(s,x+w/2,y+h/2,2);tft.setTextDatum(TL_DATUM);} 
void header(const String&name){tft.fillScreen(C_BG);text("AQUARIUM CONTROL",10,7,2,C_TEXT);text(name,10,28,1,C_MUTED);tft.fillCircle(293,16,4,C_OK);text("WiFi",260,27,1,C_MUTED);} 
void nav(){const char* n[]={"HOME","WATER","TEMP","MANUAL","SET"};for(int i=0;i<5;i++)button(i*64,211,64,29,n[i],(int)page==i);} 

void drawHome(){header("Dashboard");panel(8,45,94,72,"TEMPERATURE");text(String(tempC,1)+" C",20,72,2,C_TEXT);text("Target 25.0 C",20,98,1,C_MUTED);
panel(108,45,99,72,"WATER CHANGE");text(String(changePct)+"%",121,70,3,C_ACCENT);text("Sun 09:00",121,99,1,C_MUTED);
panel(213,45,99,72,"WATER LEVEL");text("NORMAL",226,73,2,C_OK);text("All sensors OK",226,99,1,C_MUTED);
panel(8,124,304,76,"SYSTEM STATUS");text("Filter: RUN",20,149,1,C_OK);text("Heater: OFF",20,168,1,C_MUTED);text("Chiller: OFF",113,149,1,C_MUTED);text("Leak: DRY",113,168,1,C_OK);text("Next change: 4d 02h",205,149,1,C_TEXT);text("Last: 79.8 L",205,168,1,C_MUTED);nav();}
void drawWater(){header("Automatic Water Change");panel(8,45,145,151,"SCHEDULE & VOLUME");text("Every Sunday",20,68,1,C_TEXT);text("Start 09:00",20,86,1,C_MUTED);text("Replacement",20,109,1,C_MUTED);button(18,126,38,30,"5",changePct==5);button(60,126,38,30,"10",changePct==10);button(102,126,38,30,"20",changePct==20);text("Target: ~"+String(changePct*4)+" L / 400 L",20,170,1,C_TEXT);
panel(160,45,152,151,"PROCESS");text("1 Drain to target",172,69,1,C_TEXT);text("2 Pause / settle",172,88,1,C_TEXT);text("3 Refill measured volume",172,107,1,C_TEXT);text("4 Verify HIGH sensor",172,126,1,C_TEXT);button(172,151,128,34,autoWater?"AUTO ENABLED":"AUTO OFF",autoWater);nav();}
void drawTemp(){header("Temperature Management");panel(8,45,145,151,"CURRENT");text(String(tempC,1)+" C",24,78,3,C_TEXT);text("Target",24,119,1,C_MUTED);text(String(targetTemp,1)+" C",24,137,2,C_ACCENT);button(20,164,52,24,"-0.5");button(78,164,52,24,"+0.5");panel(160,45,152,151,"CONTROL");text("Heater",174,72,1,C_TEXT);button(236,62,62,27,heater?"ON":"OFF",heater);text("Chiller",174,108,1,C_TEXT);button(236,98,62,27,chiller?"ON":"OFF",chiller);text("Alarm 28.0 C",174,143,1,C_WARN);text("DS18B20: OK",174,166,1,C_OK);nav();}
void drawManual(){header("Manual Control");panel(8,45,304,151,"OUTPUTS - SERVICE MODE");button(20,70,130,38,valveOut?"STOP DRAIN":"DRAIN VALVE",valveOut);button(170,70,130,38,valveIn?"STOP FILL":"FILL VALVE",valveIn);button(20,120,130,38,heater?"HEATER ON":"HEATER OFF",heater);button(170,120,130,38,chiller?"CHILLER ON":"CHILLER OFF",chiller);text("Safety interlocks remain active in manual mode",22,176,1,C_WARN);nav();}
void drawSettings(){header("Settings");panel(8,45,304,151,"SYSTEM");text("Tank effective volume",20,68,1,C_MUTED);text("400 L",246,68,1,C_TEXT);text("I/O expander",20,91,1,C_MUTED);text("MCP23017",230,91,1,C_TEXT);text("Flow IN / OUT",20,114,1,C_MUTED);text("Calibration required",191,114,1,C_TEXT);text("Float sensors",20,137,1,C_MUTED);text("LOW / HIGH / EMG",188,137,1,C_OK);text("Firmware",20,160,1,C_MUTED);text("GitHub Actions build",189,160,1,C_ACCENT);text("Demo UI - hardware outputs disabled by default",20,182,1,C_WARN);nav();}
void draw(){switch(page){case HOME:drawHome();break;case WATER:drawWater();break;case TEMP:drawTemp();break;case MANUAL:drawManual();break;case SETTINGS:drawSettings();break;}}

bool touchXY(int &x,int &y){if(!ts.touched())return false;TS_Point p=ts.getPoint(); // landscape mapping may need inversion per revision
 x=map(p.y,TS_MINY,TS_MAXY,0,320); y=map(p.x,TS_MINX,TS_MAXX,240,0); x=constrain(x,0,319);y=constrain(y,0,239);return true;}
void tap(int x,int y){if(y>=205){page=(Page)constrain(x/64,0,4);draw();return;} if(page==WATER){if(y>=120&&y<=162){if(x<58)changePct=5;else if(x<100)changePct=10;else if(x<145)changePct=20;draw();}else if(x>165&&y>145&&y<193){autoWater=!autoWater;draw();}}
else if(page==TEMP){if(y>158&&y<195){if(x<75)targetTemp-=0.5;else if(x<140)targetTemp+=0.5;draw();}else if(x>230&&y>55&&y<95){heater=!heater;draw();}else if(x>230&&y>95&&y<135){chiller=!chiller;draw();}}
else if(page==MANUAL){if(y>65&&y<115){if(x<160)valveOut=!valveOut;else valveIn=!valveIn;draw();}else if(y>115&&y<165){if(x<160)heater=!heater;else chiller=!chiller;draw();}}}

void setup(){Serial.begin(115200);pinMode(TFT_BL,OUTPUT);digitalWrite(TFT_BL,HIGH);tft.init();tft.setRotation(1);touchSPI.begin(TOUCH_CLK,TOUCH_MISO,TOUCH_MOSI,TOUCH_CS);ts.begin(touchSPI);ts.setRotation(1);draw();}
void loop(){int x,y;if(touchXY(x,y)){tap(x,y);delay(220);} if(millis()-lastDemo>5000){lastDemo=millis();tempC += (random(-2,3))*0.1;if(page==HOME||page==TEMP)draw();}}
