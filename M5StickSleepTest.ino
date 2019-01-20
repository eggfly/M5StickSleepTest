#include<Wire.h>

/*
 * Stick休眠和电源状态的的例子(参考m5stack官方代码并增加了isCharging()函数)
 * setPowerBoostKeepOn(true) 可以开启IP5306芯片的boost状态，避免低电流(小于45mA)持续32秒电源芯片待机断电
 * isChargeFull() 读取IP5306的寄存器获取是否充满电的状态，经过测试仅在插入外接供电+电池达到设置电压后返回true
 * isCharging() 读取IP5306的寄存器获取是否正在充电的状态，经过测试仅在插入外接供电+电池还没达到设置电压后返回true
 * a. isChargeFull() == true 的条件下 可以显示电池界面是“USB插入且充满状态”
 * b. isChargeFull() == false && isCharging() == true 的条件下 可以显示电池界面是“USB插入且正在充电的状态”
 * c. isChargeFull() == false && isCharging() == false 的条件下 可以显示电池界面是“USB未插入，正在用锂电池供电状态”
 * 未插电的时候现有的电路无法获取剩余电量状态，计划：
 * TODO: 用ADC获取+分压电阻获取电池电压然后换算成电量
 * 
 * 经过测试不同场景下锂电池的电流和在80mAh电池的时候的理论待机时间（不是很准，仅供参考）
 * 1. ESP32@240MHz开机状态：80mA左右 - 1小时
 * 2. 开启boost后，deep_sleep状态：7mA左右 - 11小时
 * 3. 关闭boost并等待32秒待机后（此时无法唤醒ESP32）: 1mA左右 - 80小时，3天多
 * 
 */
 
#define LedPin 19
#define BuzzerPin 26
#define BtnPin 35

// ================ Power IC IP5306 ===================
#define IP5306_ADDR           117
#define IP5306_REG_SYS_CTL0   0x00
#define IP5306_REG_READ0      0x70
#define IP5306_REG_READ1      0x71
#define CHARGE_FULL_BIT       3
#define CHARGING_BIT          3

// enable boost to keep power ic not going to standby sleep
void setPowerBoostKeepOn(bool en) {
  Wire.beginTransmission(IP5306_ADDR);
  Wire.write(IP5306_REG_SYS_CTL0);
  if (en) Wire.write(0x37); // Set bit1: 1 enable 0 disable boost keep on
  else Wire.write(0x35);    // 0x37 is default reg value
  int error = Wire.endTransmission();
  Serial.print("Wire.endTransmission: ");
  Serial.println(error);
}

// charge full state: input power was plugged in and battery voltage reached high level
bool isChargeFull() {
  uint8_t data;
  Wire.beginTransmission(IP5306_ADDR);
  Wire.write(IP5306_REG_READ1);
  Wire.endTransmission(false);
  Wire.requestFrom(IP5306_ADDR, 1);
  data = Wire.read();
  if (data & (1 << CHARGE_FULL_BIT)) return true;
  else return false;
}

// charging state: input power was plugged in and battery not reached the high level
bool isCharging() {
  uint8_t data;
  Wire.beginTransmission(IP5306_ADDR);
  Wire.write(IP5306_REG_READ0);
  Wire.endTransmission(false);
  Wire.requestFrom(IP5306_ADDR, 1);
  data = Wire.read();
  if (data & (1 << CHARGING_BIT)) return true;
  else return false;
}

void print_wakeup_reason() {
  esp_sleep_wakeup_cause_t wakeup_reason;

  wakeup_reason = esp_sleep_get_wakeup_cause();

  switch (wakeup_reason)
  {
    case ESP_SLEEP_WAKEUP_EXT0 : Serial.println("Wakeup caused by external signal using RTC_IO"); break;
    case ESP_SLEEP_WAKEUP_EXT1 : Serial.println("Wakeup caused by external signal using RTC_CNTL"); break;
    case ESP_SLEEP_WAKEUP_TIMER : Serial.println("Wakeup caused by timer"); break;
    case ESP_SLEEP_WAKEUP_TOUCHPAD : Serial.println("Wakeup caused by touchpad"); break;
    case ESP_SLEEP_WAKEUP_ULP : Serial.println("Wakeup caused by ULP program"); break;
    default : Serial.printf("Wakeup was not caused by deep sleep: %d\n", wakeup_reason); break;
  }
}

void setup() {
  pinMode(LedPin, OUTPUT);
  pinMode(BuzzerPin, OUTPUT);
  pinMode(BtnPin, INPUT);
  Serial.begin(115200);
  Wire.begin(21, 22, 100000);
  setPowerBoostKeepOn(true);

  //Print the wakeup reason for ESP32
  print_wakeup_reason();
  Serial.print("isChargeFull? ");
  bool isFull = isChargeFull();
  Serial.println(isFull);

  if (isFull) {
    // beep
    for (int i = 0; i < 200; i++) {
      digitalWrite(BuzzerPin, HIGH);
      delayMicroseconds(2);
      digitalWrite(BuzzerPin, LOW);
      delayMicroseconds(1998);
    }
  }
  
  // led flash
  for (int i = 0; i < 20; i++) {
    digitalWrite(LedPin, 1 - digitalRead(LedPin));
    delay(500);
  }

  esp_sleep_enable_ext0_wakeup(GPIO_NUM_35, LOW); //1 = High, 0 = Low
  //Go to sleep now
  Serial.println("Going to sleep now");
  esp_deep_sleep_start();
  Serial.println("This will never be printed");
}

void loop() {
  //This is not going to be called
}
