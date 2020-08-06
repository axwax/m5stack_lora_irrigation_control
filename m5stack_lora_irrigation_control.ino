
#include <M5Stack.h>
#include <LoRaWan.h>
#include <ArduinoJson.h>
//#include <Metro.h>

unsigned char buffer[128] = {0,};


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

void setup() {
  M5.begin();
  SerialUSB.begin(9600);
  lora.init();
  delay(2000); // must delay for lorawan power on
  lora.initP2PMode(868, SF7, BW125, 8, 8, 20);
  delay(300);
  Serial2.print("AT+TEST=RXLRPKT\r\n");  
  M5.Lcd.setTextFont(2);
  M5.Lcd.println("Irrigation Control");
  //M5.Lcd.setTextColor(WHITE);
  int core = xPortGetCoreID();
  xTaskCreatePinnedToCore(key_scan, "key_scan", 3096, NULL, 5, NULL, 0);
}

void loop() {
  // put your main code here, to run repeatedly:

}

void onReceive()
{
  short length = 0;
  short rssi = 0;

  memset(buffer, 0, 128);
  length = lora.receivePacketP2PMode(buffer, 128, &rssi, 1);

  if (length)
  {
    SerialUSB.print("Length is: ");
    SerialUSB.println(length);
    SerialUSB.print("RSSI is: ");
    SerialUSB.println(rssi);
    SerialUSB.print("Data is: ");



    StaticJsonBuffer<500> jsonBuffer;
    char json[500];
    json[0] = '\0';
    

    for (unsigned char i = 0; i < length; i++)
    {
      SerialUSB.print((char)buffer[i]);

      byte hi = strlen(json);
      json[hi] = (char)buffer[i];
      json[hi + 1] = '\0';      
    }

    JsonObject& recMessage = jsonBuffer.parse(json);
    if (recMessage.success()) {
      recMessage.printTo(SerialUSB);
      int activeValve = (int) recMessage["activeValve"];
      M5.Lcd.setCursor(0, 18);
      M5.Lcd.fillRect(0, 18, 320, 222, BLACK);
      if(activeValve > 0){
        //SerialUSB.print(String(activeValve));
        M5.Lcd.println("RSSI: " + String(rssi));     
        M5.Lcd.println("valve " + String(activeValve) + " open");
      }
      else {
        M5.Lcd.println("RSSI: " + String(rssi));     
        M5.Lcd.println("irrigation stopped");      
      }
    }
    SerialUSB.println();

  }
}
