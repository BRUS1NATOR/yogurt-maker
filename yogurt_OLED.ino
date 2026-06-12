// Include the DHT11 library for interfacing with the sensor.
#include <DHT11.h>
#include <GyverOLED.h>
#include <KY040.h>
#include <Pushbutton.h>
// Реле
#define REL_PIN 8                         // Указываем к какому выводу подключено реле 220
// DHT11
#define DHT_PIN 6                         // Указываем к какому выводу CLK энкодер подключен к Arduino
// Кнопки
#define CLK_PIN 2                       // Указываем к какому выводу CLK энкодер подключен к Arduino
#define DT_PIN 3                          // Указываем к какому выводу DT энкодер подключен к Arduino
#define SW_PIN 4                         // Указываем к какому выводу SW энкодер подключен к Arduino (BUTTON)
KY040 g_rotaryEncoder(CLK_PIN, DT_PIN);
Pushbutton button(SW_PIN);
 
// Create an instance of the DHT11 class.
DHT11 dht11(DHT_PIN);
// LCD address to 0x27 for a 16 chars and 2 line display
GyverOLED<SSD1306_128x64, OLED_NO_BUFFER> oled;
bool update_lcd_up = true;
bool heater_enabled = false;
// 0 - reset, 1 - set temp, 2 - set time, 3 - in progress, 4 - finish
int current_mode = 1;
//
int target_temperature = 42;
int current_temperature = 0;
//
unsigned long time_left_seconds = 25200;  // timer in minutes
unsigned long previousMillis = 0;     // will store last time LED was updated
const long interval = 30;           // interval 30 seconds
// 
char data[32];

const static uint8_t icons_7x7[][8] PROGMEM = {
    {0x02, 0x05, 0x02, 0x38, 0x44, 0x44, 0x28}, // 0 celcius
    {0x3e, 0x63, 0x41, 0x4d, 0x49, 0x63, 0x3e}, // 1 clock
};
const static uint8_t icons_8x8[][8] PROGMEM = {
    {0x7e, 0x81, 0x95, 0xa1, 0xa1, 0x95, 0x81, 0x7e}, // 0 137 smile 1
    {0x00, 0x98, 0xdc, 0xfe, 0x7f, 0x3b, 0x19, 0x18}, // 1 flash, lighting
    {0x1e, 0x3f, 0x7f, 0xfe, 0xfe, 0x7f, 0x3f, 0x1e}, // 2 heart
    {0x1e, 0x21, 0x41, 0x82, 0x82, 0x41, 0x21, 0x1e}  // 3 heart outline
};

void setup() {
    // Initialize serial communication to allow debugging and data readout.
    // Using a baud rate of 9600 bps.
    Serial.begin(9600);
    //Wire.setClock(400000L);   // макс. 800'000
    oled.init();
    oled.clear();
    oled.home();

    //
    pinMode(REL_PIN, OUTPUT);
    //
    pinMode(CLK_PIN, INPUT);         // Указываем вывод CLK как вход
    pinMode(DT_PIN, INPUT);          // Указываем вывод DT как вход
    pinMode(SW_PIN, INPUT);          // Указываем вывод SW как вход и включаем подтягивающий резистор

    // Бывает двойное считывание?
    // attachInterrupt(digitalPinToInterrupt(CLK_PIN), ISR_rotaryEncoder, CHANGE);
    // attachInterrupt(digitalPinToInterrupt(DT_PIN), ISR_rotaryEncoder, CHANGE);
    getCurrentTemperature();
}

void loop() {
  if (!ISR_rotaryEncoder() && button.getSingleDebouncedPress())
  {
    current_mode += 1;
    if(current_mode > 3){
      current_mode = 0;
      oled.clear();
      writeTemperature();
    }
    update_lcd_up = true;
  }
  
  timer();
  heater();
  writeUpLCD();
}

bool ISR_rotaryEncoder(){
  switch (g_rotaryEncoder.getRotation()) {
    case KY040::CLOCKWISE:
      onScroll(true);
      return true;
    case KY040::COUNTERCLOCKWISE:
      onScroll(false);
      return true;
  }
  return false;
}

void writeUpLCD(){
  if(!update_lcd_up){
    return;
  }
  update_lcd_up = false;
  Serial.println("Write lcd up");
  switch(current_mode){
    case 1:
      oled.setScale(1);
      sprintf(data, "SET TEMPERATURE: %02d", target_temperature);
      oled.setCursor(0, 0);
      oled.print(String(data));
      oled.setCursor(114, 0);
      drawIcon7x7(0);
      break;
    case 2:
      oled.setScale(1);
      sprintf(data, "SET TIMER:    %02lu:%02lu ", time_left_seconds / 3600, (time_left_seconds % 3600) / 60);
      oled.setCursor(0, 1);
      oled.print(String(data));
      oled.setCursor(114, 1);
      drawIcon7x7(1);
      break;
    case 3:
      oled.setScale(3);
      oled.setCursor(0, 3);
      sprintf(data, "%02lu:%02lu   ", time_left_seconds / 3600, (time_left_seconds % 3600) / 60);
      oled.print(String(data));
      oled.setScale(1);
      oled.setCursor(114, 3);
      drawIcon8x8(heater_enabled ? 1 : 0);
      oled.setCursor(100, 3);
      drawIcon8x8(heater_enabled ? 1 : 0);
      oled.setCursor(114, 5);
      drawIcon8x8(heater_enabled ? 1 : 0);
      oled.setCursor(100, 5);
      drawIcon8x8(heater_enabled ? 1 : 0);
      break;
    case 4:
      oled.setScale(3);
      oled.setCursor(0, 3);
      sprintf(data, "%02lu:%02lu   ", time_left_seconds / 3600, (time_left_seconds % 3600) / 60);
      oled.print(String(data));
      oled.setScale(1);
      oled.setCursor(114, 3);
      drawIcon8x8(2);
      oled.setCursor(100, 3);
      drawIcon8x8(3);
      oled.setCursor(114, 5);
      drawIcon8x8(3);
      oled.setCursor(100, 5);
      drawIcon8x8(2);
      break;
  }
}


void drawIcon7x7(byte index) {
  for(unsigned int i = 0; i < 7; i++) {
    oled.drawByte(pgm_read_byte(&(icons_7x7[index][i])));
  }
}
void drawIcon8x8(byte index) {
  for(unsigned int i = 0; i < 8; i++) {
    oled.drawByte(pgm_read_byte(&(icons_8x8[index][i])));
  }
}

void onScroll(bool isScrolledPositive){
  if(current_mode == 1) {
    update_lcd_up = true;
    if(isScrolledPositive) {
      target_temperature += 1;
      if(target_temperature > 60){
        target_temperature = 60;
      }
    }
    else {
      target_temperature -= 1;
      if(target_temperature < 24){
        target_temperature = 24;
      }
    }
    return;
  }
  if(current_mode == 2){
    update_lcd_up = true;
    if(isScrolledPositive) {
      time_left_seconds += 1800;
      if(time_left_seconds < 1800){
        time_left_seconds = 1800;
      }
    }
    else {
      time_left_seconds -= 1800;
      // 16 часов
      if(time_left_seconds > 57600){
        time_left_seconds = 57600;
      }
    }
  }
}

void timer(){
  if (millis() - previousMillis >= interval * 1000) {
    // save the last time
    previousMillis = millis();
    // Получаем температуру раз в минуту - что бы не блокировать поток инпута
    getCurrentTemperature();
    if(current_mode < 3){
      return;
    }
    Serial.println(time_left_seconds);
    if(current_mode == 3) {
      time_left_seconds -= interval;
      if(time_left_seconds <= 0){
        current_mode = 4;
      }
    }
    else if (current_mode == 4) {
      time_left_seconds += interval;
    }
    update_lcd_up = true;
  }
}

void heater() {
  // Если режим в процессе
  if(current_mode == 3){
  // Если температура ниже и работает таймер
    if(time_left_seconds > 0 && current_temperature < target_temperature){
      if(!heater_enabled){
        heater_enabled = true;
        update_lcd_up = true;
      }
      digitalWrite(REL_PIN, HIGH);
      return;
    }
  }
  if(heater_enabled){
    heater_enabled = false;
    update_lcd_up = true;
  }
  //disable
  digitalWrite(REL_PIN, LOW);
}

void getCurrentTemperature(){
    // Attempt to read the temperature and humidity values from the DHT11 sensor.
    int temperature = dht11.readTemperature();
    // If the reading is successful, print the temperature and humidity values.
    if (current_temperature != temperature) {
        if(temperature > 99){
          temperature = 99;
        }
        current_temperature = temperature;
        writeTemperature();
    }
}

void writeTemperature(){
    sprintf(data, "TEMPERATURE: %02d", current_temperature);
    oled.setCursor(0, 7);
    oled.print(String(data));
    oled.setCursor(91, 7);
    drawIcon7x7(0);
}
