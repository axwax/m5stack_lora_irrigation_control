
#include <M5Stack.h>
#include <LoRaWan.h>
#include <ArduinoJson.h>
//#include <Metro.h>

unsigned char buffer[128] = {0,};
bool loraSent = false;
bool irrigate = false;
short rssi = 0;

// Forward declarations
void key_scan(void *arg);
void onReceive();
void sendLora();

void setup() {
  M5.begin();
  SerialUSB.begin(115200);
  lora.init();
  delay(2000); // must delay for lorawan power on
  lora.initP2PMode(868, SF7, BW125, 8, 8, 20);
  delay(300);
  Serial2.print("AT+TEST=RXLRPKT\r\n");  
  M5.Lcd.setTextFont(2);
  M5.Lcd.println("Irrigation Control");
  //M5.Lcd.setTextColor(WHITE);

// power settings
  Wire.begin(); // required to access IP5306
  M5.Power.begin();
  M5.Power.setPowerBoostOnOff(false); //Change the power on / off method. The power does not turn off when connected via USB. true=Press and hold to turn on / off. false=Turn on / off with two short presses.
  M5.Power.setPowerBoostSet(true); //Change the power on / off method true=ON / OFF in one short press. false=same as above
  M5.Power.setPowerVin(true); //When the power supply from USB etc. is cut off, Decide whether to turn on the power again.
  M5.Power.setPowerBtnEn(true); //Set whether to accept the power button.
  M5.Power.setPowerBoostKeepOn(false); //Always output power. True= Always output power. False=not Always output power.
  M5.Power.setAutoBootOnLoad(false); //Set whether to automatically start when power consumption occurs
  
  if(!M5.Power.canControl())
  {
  M5.Lcd.setTextColor(RED);
  M5.Lcd.printf("No communication with IP5306 chip");
  while(1);
  }
  uint8_t bat = M5.Power.getBatteryLevel();
  if (M5.Power.isCharging()) M5.Lcd.printf("Battery is charging\r\n");
  else M5.Lcd.printf("Battery is not charging\r\n");
  M5.Lcd.printf("Battery Level %d", bat);


  int core = xPortGetCoreID();
  xTaskCreatePinnedToCore(key_scan, "key_scan", 3096, NULL, 5, NULL, 0);
  SerialUSB.println();
  SerialUSB.println("Irrigation Control is ready");
}

void loop() {

  if (M5.BtnC.wasPressed()) {
    irrigate = !irrigate;
    sendLoraMsg();
  }
  M5.update();
}



void key_scan(void *arg)
{
  while (1)
  {
    onReceive();
    delay(10);
    // lora.loraDebug();
  }
  vTaskDelete(NULL);
}

void onReceive()
{
  short length = 0;


  memset(buffer, 0, 128);
  length = lora.receivePacketP2PMode(buffer, 128, &rssi, 1);

  if (length)
  {
    SerialUSB.print("Length is: ");
    SerialUSB.println(length);
    SerialUSB.print("RSSI is: ");
    SerialUSB.println(rssi);
    SerialUSB.print("Data is: ");

    // download lora data into json variable
    char json[500];
    json[0] = '\0';
    for (int i = 0; i < length; i++) {
      byte hi = strlen(json);
      json[hi] = (char)buffer[i];
      json[hi + 1] = '\0';      
    }
    
    DynamicJsonDocument recMessage(1024);
    DeserializationError error = deserializeJson(recMessage, json);
    if (error) {
      SerialUSB.println("error:");
      SerialUSB.println(error.c_str());
      return;
    }
    serializeJson(recMessage, SerialUSB);

      int activeValve = (int) recMessage["activeValve"];
      irrigate = (bool) recMessage["irrigate"];
      M5.Lcd.setCursor(0, 18);
      M5.Lcd.fillRect(0, 18, 320, 222, BLACK);
      if(irrigate){
        //SerialUSB.print(String(activeValve));
        M5.Lcd.println("RSSI: " + String(rssi));     
        M5.Lcd.println("valve " + String(activeValve+1) + " open");
      }
      else {
        M5.Lcd.println("RSSI: " + String(rssi));     
        M5.Lcd.println("irrigation stopped");      
      }
 
    SerialUSB.println();
  }
}

void sendLoraMsg(){
      // send lora
      DynamicJsonDocument sendMessage(1024);
      sendMessage["rssi"] = rssi;
      sendMessage["irrigate"] = irrigate;
      sendMessage["enabledValves"] = serialized("[true, false, true, false]");
      String sMessage;
      serializeJson(sendMessage, sMessage);

      lora.transferPacketP2PMode(const_cast<char*>(sMessage.c_str()));
      loraSent = true;
      SerialUSB.println("irrigate:"+String(irrigate));
      SerialUSB.println(sMessage);
}
