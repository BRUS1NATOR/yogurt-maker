// Include the DHT11 library for interfacing with the sensor.
#include <DHT11.h>
#include <LiquidCrystal_I2C.h>
#include <KY040.h>
// Реле
#define REL_PIN 8                         // Указываем к какому выводу подключено реле 220
// DHT11
#define DHT_PIN 6                         // Указываем к какому выводу CLK энкодер подключен к Arduino
// Кнопки
#define CLK_PIN 2                       // Указываем к какому выводу CLK энкодер подключен к Arduino
#define DT_PIN 3                          // Указываем к какому выводу DT энкодер подключен к Arduino
#define SW_PIN 4                         // Указываем к какому выводу SW энкодер подключен к Arduino (BUTTON)
KY040 g_rotaryEncoder(CLK_PIN, DT_PIN);
 
// Create an instance of the DHT11 class.
DHT11 dht11(DHT_PIN);
// LCD address to 0x27 for a 16 chars and 2 line display
LiquidCrystal_I2C lcd(0x27,16,2);
bool update_lcd_up = true;
bool update_lcd_down = true;

// 0 - reset, 1 - set temp, 2 - set time, 3 - in progress, 4 - finish
byte current_mode = 1;
//
int target_temperature = 42;
int current_temperature = 0;
//
unsigned long time_left_seconds = 25200;  // timer in minutes
unsigned long currentMillis;
unsigned long previousMillis = 0;     // will store last time LED was updated
const long interval = 30;           // interval 30 seconds

bool button_pressed = false;
unsigned long lastDebounceTime = 0;
unsigned long debounceDelay = 100;
// 
char data[20];

byte Celsius[] = {
  B00111,
  B00101,
  B00111,
  B00000,
  B00000,
  B00000,
  B00000,
  B00000,
};
byte Heart[] = {
  B00000,
  B01010,
  B11111,
  B11111,
  B01110,
  B00100,
  B00000,
  B00000
};


void setup() {
    // Initialize serial communication to allow debugging and data readout.
    // Using a baud rate of 9600 bps.
    Serial.begin(9600);
    lcd.init();
    lcd.backlight();
    
    lcd.createChar(0, Celsius);
    lcd.createChar(1, Heart);
    Serial.println("CHAR INITIALIZED");
    //
    pinMode(REL_PIN, OUTPUT);
    //
    pinMode(CLK_PIN, INPUT);         // Указываем вывод CLK как вход
    pinMode(DT_PIN, INPUT);          // Указываем вывод DT как вход
    pinMode(SW_PIN, INPUT);          // Указываем вывод SW как вход и включаем подтягивающий резистор

    Serial.println("FINISH");
    // Бывает двойное считывание?
    // attachInterrupt(digitalPinToInterrupt(CLK_PIN), ISR_rotaryEncoder, CHANGE);
    // attachInterrupt(digitalPinToInterrupt(DT_PIN), ISR_rotaryEncoder, CHANGE);
    getCurrentTemperature();
}

void loop() {
  currentMillis = millis();
  ISR_rotaryEncoder();
  if (digitalRead(SW_PIN) == LOW) {
    onButton();
  }
  else {
    button_pressed = false;
  }
  
  timer();
  heater();
  writeUpLCD();
  writeDownLCD();
}

void ISR_rotaryEncoder(){
  switch (g_rotaryEncoder.getRotation()) {
    case KY040::CLOCKWISE:
      onScroll(true);
      break;
    case KY040::COUNTERCLOCKWISE:
      onScroll(false);
      break;
  }
}

void writeUpLCD(){
  if(!update_lcd_up){
    return;
  }
    Serial.println("WRITE LCD UP");
  update_lcd_up = false;
  switch(current_mode){
    case 0:
      lcd.setCursor(0, 0);
      lcd.print("RESET           ");
      return;
    case 1:
      sprintf(data, "Set TEMP:   %02d*C", target_temperature);
      lcd.setCursor(0, 0);
      lcd.print(String(data));
      lcd.setCursor(14, 0);
      lcd.write(byte(0));
      return;
    case 2:
      Serial.println((time_left_seconds % 3600) / 60);
      sprintf(data, "Set TIME: %02lu:%02lu ", time_left_seconds / 3600, (time_left_seconds % 3600) / 60);
      lcd.setCursor(0, 0);
      lcd.print(String(data));
      return;
    case 3:
      sprintf(data, "%02d.C / %02lu:%02lu   ", target_temperature, time_left_seconds / 3600, (time_left_seconds % 3600) / 60);
      lcd.setCursor(0, 0);
      lcd.print(String(data));
      lcd.setCursor(2, 0);
      lcd.write(byte(0));
      return;
    case 4:
      sprintf(data, "^__^ \ %02lu:%02lu  ", time_left_seconds / 3600, (time_left_seconds % 3600) / 60);
      lcd.setCursor(0, 0);
      lcd.print(String(data));
      return;
  }
  return "ERROR :(";
}
void writeDownLCD(){
  if(!update_lcd_down){
    return;
  }
  Serial.println("WRITE LCD DoWN");
  sprintf(data, "Temperature:%02d*C", current_temperature);
  lcd.setCursor(0, 1);
  lcd.print(String(data));
  lcd.setCursor(14, 1);
  lcd.write(byte(0));
  update_lcd_down = false;
}

void onButton(){
  if(button_pressed){
    return;
  }
  lastDebounceTime = millis();
  if ((currentMillis - lastDebounceTime) > debounceDelay) {
    button_pressed = true;
    update_lcd_up = true;
    Serial.println("Кнопка нажата");
    current_mode += 1;
    if(current_mode > 3){
      current_mode = 0;
    }
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
      time_left_seconds -= 1800;
      if(time_left_seconds < 1800){
        time_left_seconds = 1800;
      }
    }
    else {
      time_left_seconds += 1800;
      // 16 часов
      if(time_left_seconds > 57600){
        time_left_seconds = 57600;
      }
    }
  }
}

void timer(){
  if (currentMillis - previousMillis >= interval * 1000) {
    // save the last time
    previousMillis = currentMillis;
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
      digitalWrite(REL_PIN, HIGH);
      return;
    }
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
        Serial.println(current_temperature);
        update_lcd_down = true;
    }
}
