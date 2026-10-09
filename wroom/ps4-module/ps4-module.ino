#include <Arduino.h>
#include <Bluepad32.h>
#include <Preferences.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <esp_partition.h>
#include <esp_ota_ops.h>
#include <esp_system.h>
#include <atomic>
#include <driver/ledc.h>

// Full independent Bluetooth firmware: no Wi-Fi, HTTP, TLS or OTA writer.
static constexpr uint8_t ledPin=26,backPin=32;
static constexpr const char *moduleVersion="0.0.28";
static Adafruit_SSD1306 oled(128,64,&Wire,-1,400000,400000);
static ControllerPtr controller=nullptr;
static bool returnReady=false,oledReady=false,ledOn=false;
static uint8_t ledButton=1;
static std::atomic<bool> returnPressed{false};
static volatile uint32_t lastBack=0;
static uint32_t lastPacket=0,nextScreen=0,loopMaxUs=0,loopAt=0;
static uint16_t buttons=0;static int lx=0,ly=0,rx=0,ry=0;
static String returnError="NOT_CHECKED";
// Servo outputs stay off until the PS4 dead-man L1 is held.
static constexpr uint8_t servoPin=14,okPin=25,upPin=27,downPin=33;
static bool servoReady=false,servoOn=false;
static int servoAngle=90;
struct ServoButton {uint8_t pin;bool raw,stable;uint32_t changed;
 ServoButton(uint8_t p):pin(p),raw(true),stable(true),changed(0){}
};
static ServoButton servoButtons[]={{okPin},{upPin},{downPin}};
static bool buttonPressed(ServoButton &b) {
 bool raw=digitalRead(b.pin);uint32_t now=millis();
 if(raw!=b.raw){b.raw=raw;b.changed=now;}
 if(raw!=b.stable&&(uint32_t)(now-b.changed)>=30){b.stable=raw;return !raw;}
 return false;
}
static void stopServo() {
 servoOn=false;if(servoReady)ledc_stop(LEDC_LOW_SPEED_MODE,LEDC_CHANNEL_0,0);
 digitalWrite(servoPin,LOW);
}
static bool initServo() {
 ledc_timer_config_t timer={};timer.speed_mode=LEDC_LOW_SPEED_MODE;
 timer.duty_resolution=LEDC_TIMER_16_BIT;timer.timer_num=LEDC_TIMER_0;
 timer.freq_hz=50;timer.clk_cfg=LEDC_AUTO_CLK;
 if(ledc_timer_config(&timer)!=ESP_OK)return false;
 ledc_channel_config_t channel={};channel.gpio_num=servoPin;
 channel.speed_mode=LEDC_LOW_SPEED_MODE;channel.channel=LEDC_CHANNEL_0;
 channel.intr_type=LEDC_INTR_DISABLE;channel.timer_sel=LEDC_TIMER_0;
 channel.duty=0;channel.hpoint=0;
 return ledc_channel_config(&channel)==ESP_OK;
}
static bool writeServo(int angle) {
 servoAngle=constrain(angle,0,180);
 // Expanded nominal travel; actual endpoints require unloaded calibration.
 uint32_t pulseUs=600+servoAngle*10UL;
 uint32_t duty=(pulseUs*65536UL+10000)/20000;
 if(ledc_set_duty(LEDC_LOW_SPEED_MODE,LEDC_CHANNEL_0,duty)!=ESP_OK ||
    ledc_update_duty(LEDC_LOW_SPEED_MODE,LEDC_CHANNEL_0)!=ESP_OK){stopServo();return false;}
 servoOn=true;return true;
}
// PS4 dead-man control: release L1 or stale packets => no PWM.
static bool servoL1=false,servoInhibited=false;
static uint32_t nextServoStep=0;
static int servoTarget(int axis) {
 axis=constrain(axis,-512,512);
 if(axis>=-35&&axis<=35)return 90;
 return constrain(90+(axis*90)/512,0,180);
}
static void pollPS4Servo() {
 bool ok=buttonPressed(servoButtons[0]);
 // Consume the local navigation buttons without moving the servo in this mode.
 buttonPressed(servoButtons[1]);buttonPressed(servoButtons[2]);
 bool fresh=controller&&controller->isConnected()&&(uint32_t)(millis()-lastPacket)<250;
 if(ok){stopServo();servoInhibited=true;}
 if(!servoL1||!fresh||!returnReady||!servoReady){
  if(servoOn)stopServo();
  if(!servoL1)servoInhibited=false;
  return;
 }
 if(servoInhibited)return;
 uint32_t now=millis();
 if(!servoOn){
  if(!writeServo(servoAngle)){servoInhibited=true;return;}
  nextServoStep=now+20;
 }
 if((int32_t)(now-nextServoStep)<0)return;
 nextServoStep=now+20;
 int target=servoTarget(rx),delta=constrain(target-servoAngle,-2,2);
 if(delta&&!writeServo(servoAngle+delta))servoInhibited=true;
}

static String digestHex(const unsigned char *bytes) {
 char text[65];for(unsigned i=0;i<32;++i)snprintf(text+i*2,3,"%02x",bytes[i]);text[64]=0;return String(text);
}
static bool armMenuReturn() {
 const esp_partition_t *run=esp_ota_get_running_partition(),*menu=esp_ota_get_next_update_partition(nullptr);
 if(!run||!menu||run->size!=0x140000||menu->size!=0x140000 ||
   !((run->address==0x10000&&menu->address==0x150000)||(run->address==0x150000&&menu->address==0x10000))) {returnError="PARTITIONS";return false;}
 Preferences p;if(!p.begin("module-return",true)){returnError="NO_MENU_RECORD";return false;}
 String record=p.getString("menu","");p.end();int first=record.indexOf('|'),second=record.indexOf('|',first+1);
 if(first<1||second!=first+65||(record.substring(second+1)!="0.0.22"&&record.substring(second+1)!="0.0.27")||record.substring(0,first).toInt()!=(long)menu->address){returnError="WRONG_MENU_RECORD";return false;}
 unsigned char digest[32];
 if(esp_partition_get_sha256(menu,digest)!=ESP_OK||digestHex(digest)!=record.substring(first+1,second)){returnError="MENU_HASH";return false;}
 if(esp_ota_set_boot_partition(menu)!=ESP_OK){returnError="BOOT_SELECTION";return false;}
 const esp_partition_t *boot=esp_ota_get_boot_partition();
 if(!boot||boot->address!=menu->address){returnError="BOOT_READBACK";return false;}
 returnError="READY";return true;
}
static void ARDUINO_ISR_ATTR backISR() {
 uint32_t now=millis();if((uint32_t)(now-lastBack)>=150){lastBack=now;returnPressed.store(true);}
}
static void onConnected(ControllerPtr ctl) {if(!controller&&ctl->isGamepad()) controller=ctl;}
static void onDisconnected(ControllerPtr ctl) {if(controller==ctl){controller=nullptr;servoL1=false;stopServo();}}
static void setLed(bool on) {ledOn=returnReady&&on;digitalWrite(ledPin,ledOn?HIGH:LOW);}
static void line(uint8_t row,const String &text) {
 oled.setCursor(0,row<2?row*8:18+(row-2)*9);oled.print(text.substring(0,21));
}
static void drawScreen() {
 if(!oledReady)return;
 oled.clearDisplay();line(0,"BOTIZIN PS4 "+String(moduleVersion));line(1,returnReady?"Wi-Fi OFF | BT ON":"SAIDA BLOQUEADA");
 if(!returnReady){line(2,"Retorno nao validado");line(3,returnError);line(5,"LED permanece apagado");}
 else {
  line(2,controller&&controller->isConnected()?"Controle conectado":"Parear: SHARE + PS");
  line(3,"Botoes: "+String(buttons,HEX)+" LED:"+(ledOn?"ON":"OFF"));
  line(4,"Servo D14: "+String(!servoReady?"ERRO":servoOn?String(servoAngle):"OFF"));
  line(5,"L1 + analogico dir.");
  line(6,"Esquerda: voltar OTA");
 }
 oled.display();
}
void setup() {
 pinMode(ledPin,OUTPUT);digitalWrite(ledPin,LOW);Serial.begin(115200);
 pinMode(servoPin,OUTPUT);digitalWrite(servoPin,LOW);
 for(auto &b:servoButtons){pinMode(b.pin,INPUT_PULLUP);b.raw=b.stable=digitalRead(b.pin);b.changed=millis();}
 pinMode(backPin,INPUT_PULLUP);attachInterrupt(backPin,backISR,FALLING);
 Wire.begin(21,22);Wire.setTimeOut(25);oledReady=oled.begin(SSD1306_SWITCHCAPVCC,0x3c);
 if(oledReady){oled.setTextSize(1);oled.setTextColor(SSD1306_WHITE);oled.setTextWrap(false);}
 returnReady=armMenuReturn();
 if(returnReady){servoReady=initServo();stopServo();}
 Preferences p;if(p.begin("ps4-config",true)){uint8_t b=p.getUChar("led-button",1);p.end();if(b==1||b==2||b==4||b==8)ledButton=b;}
 Serial.println("BOTIZIN PS4 SERVO V0.0.28");
 Serial.println(servoReady?"SERVO_D14: READY_OFF":"SERVO_D14: DISABLED");Serial.println("MENU_RETURN: "+returnError);
 if(returnReady) BP32.setup(&onConnected,&onDisconnected);
 drawScreen();loopAt=micros();
}
void loop() {
 uint32_t nowUs=micros(),gap=nowUs-loopAt;loopAt=nowUs;if(gap>loopMaxUs)loopMaxUs=gap;
 if(returnPressed.exchange(false)) {
  setLed(false);stopServo();
  if(returnReady){if(oledReady){oled.clearDisplay();line(0,"RETORNO CONFIRMADO");line(2,"Voltando ao OTA...");oled.display();}delay(750);esp_restart();}
 }
 if(returnReady) {
  bool updated=BP32.update();
  if(updated&&controller&&controller->isConnected()&&controller->hasData()) {
   lastPacket=millis();servoL1=controller->l1();buttons=controller->buttons();lx=controller->axisX();ly=controller->axisY();rx=controller->axisRX();ry=controller->axisRY();
   setLed((buttons&ledButton)!=0);
  }
  if(!controller||!controller->isConnected()||(uint32_t)(millis()-lastPacket)>1000){buttons=0;servoL1=false;setLed(false);}
 }
 pollPS4Servo();
 if((int32_t)(millis()-nextScreen)>=0){nextScreen=millis()+250;drawScreen();}
 delay(1);
}
