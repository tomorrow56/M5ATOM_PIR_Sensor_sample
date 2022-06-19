/*
NOTE:
======
Only RTC IO can be used as a source for external wake
source. They are pins: 0,2,4,12-15,25-27,32-39.
*/
#include "M5Atom.h"
#include <WiFi.h>
#include <WiFiClientSecure.h>

RTC_DATA_ATTR int bootCount = 0; // RTCスローメモリに変数を確保
//const gpio_num_t wakeIO = GPIO_NUM_39;  // GPIO39(M5ATOMのボタン)をwakeup I/Oに設定する
const gpio_num_t wakeIO = GPIO_NUM_32;  // GPIO32(Grove pin1)をwakeup I/Oに設定する

const char* ssid       = "xxxxxxxxxxxxxxxx";  // 無線LANのアクセスポイント名
const char* password   = "xxxxxxxxxxxxxxxx";  // 無線LANのパスワード

WiFiClientSecure client;

// LINE Notify設定
const char* host = "notify-api.line.me";  // LINE NotifyのURL
const char* token = "xxxxxxxxxxxxxxxxx";  // LINE NotifyのTOKEN

void print_wakeup_reason(){
  esp_sleep_wakeup_cause_t wakeup_reason;
  wakeup_reason = esp_sleep_get_wakeup_cause();

  switch(wakeup_reason){
    case ESP_SLEEP_WAKEUP_EXT0      : Serial.printf("外部割り込み(RTC_IO)で起動\n"); break;
    case ESP_SLEEP_WAKEUP_EXT1      : Serial.printf("外部割り込み(RTC_CNTL)で起動 IO=%llX\n", esp_sleep_get_ext1_wakeup_status()); break;
    case ESP_SLEEP_WAKEUP_TIMER     : Serial.printf("タイマー割り込みで起動\n"); break;
    case ESP_SLEEP_WAKEUP_TOUCHPAD  : Serial.printf("タッチ割り込みで起動 PAD=%d\n", esp_sleep_get_touchpad_wakeup_status()); break;
    case ESP_SLEEP_WAKEUP_ULP       : Serial.printf("ULPプログラムで起動\n"); break;
    case ESP_SLEEP_WAKEUP_GPIO      : Serial.printf("ライトスリープからGPIO割り込みで起動\n"); break;
    case ESP_SLEEP_WAKEUP_UART      : Serial.printf("ライトスリープからUART割り込みで起動\n"); break;
    default                         : Serial.printf("スリープ以外からの起動\n"); break;
  }
}

//Line Notifyへ通知
void LINE_Notify(String message){
  if(!client.connect(host, 443)){
    Serial.println("Connection failed!");
    return;
  }else{
    Serial.println("Connected to " + String(host));
    String query = String("message=") + message;
    String request = String("") +
                 "POST /api/notify HTTP/1.1\r\n" +
                 "Host: " + host + "\r\n" +
                 "Authorization: Bearer " + token + "\r\n" +
                 "Content-Length: " + String(query.length()) +  "\r\n" + 
                 "Content-Type: application/x-www-form-urlencoded\r\n\r\n" +
                  query + "\r\n";
    client.print(request);
    client.println("Connection: close");
    client.println();

    while (client.connected()) {
      String line = client.readStringUntil('\n');
      if (line == "\r") {
        Serial.println("headers received");
        break;
      }
    }
    // if there are incoming bytes available
    // from the server, read them and print them:
    while (client.available()) {
      char c = client.read();
      Serial.write(c);
    }

    client.stop();
    Serial.println("closing connection");
    Serial.println("");
  }
}

void setup(){
  client.setInsecure();  // Line Notifyのアクセスエラー対応
   
  // M5.begin(SerialEnable, I2CEnable, DisplayEnable);
  M5.begin(true, false, true);

  M5.dis.drawpix(0, 0x0000FF);  //LED BLUE
  delay(100);

  // 初回起動だけシリアル初期化待ち
  if( bootCount == 0 ){
    delay(1000);
  }

  //無線LANに接続
  WiFi.begin(ssid, password);
  int i = 0;
  int timeOut = 60;
  while (WiFi.status() != WL_CONNECTED && i < timeOut){
      delay(500);
      Serial.print(".");
      i++;
  }

  if(WiFi.status() == WL_CONNECTED){
    M5.dis.drawpix(0, 0x00FF00);  //LED GREEN
    delay(100);
    Serial.println("connected to router(^^)");
    Serial.print("SSID: ");
    Serial.println(WiFi.SSID());
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());  //IP address assigned to ATOM
    Serial.println("");
    if(bootCount == 0 ){
      LINE_Notify("監視を開始しました。");
      delay(1000);
    }else{
      LINE_Notify("PIRセンサーが反応しました。確認してください。");
      delay(1000);
    }
  }else{
    M5.dis.drawpix(0, 0xFF0000);  //LED RED
    delay(100);
    Serial.println(" Failed to connect to WiFi");
  }

  // 起動回数カウントアップ
  bootCount++;
  Serial.printf("起動回数: %d ", bootCount);

  //Print the wakeup reason for ESP32
  print_wakeup_reason();

  //wakeup I/Oを入力に設定する
  pinMode(wakeIO, INPUT_PULLUP);

  //wakeup I/OがLowの間はループする
  boolean button_status;
  do{
    M5.dis.drawpix(0, 0x00FFFF);  //LED CYAN
    delay(100);
    button_status = digitalRead(wakeIO);
    delay(100);
  } while(button_status == false);

  //wakeup I/OがLOWになったらDeep Sleepから抜けるように設定
  esp_sleep_enable_ext0_wakeup(wakeIO, LOW);

  M5.dis.drawpix(0, 0x000000);  //LED OFF
  delay(100);

  // deep sleep
  Serial.println("Going to sleep now");
  esp_deep_sleep_start();
  Serial.println("This will never be printed");
}

void loop(){
  //This is not going to be called
}
