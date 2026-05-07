// Include the DHT11 library for interfacing with the sensor.
#include <DHT11.h>
#include <LiquidCrystal_I2C.h>
#include <KY040.h>
// Реле
#define REL_PIN 8                         // Указываем к какому выводу CLK энкодер подключен к Arduino
// DHT11
#define DHT_PIN 6                         // Указываем к какому выводу CLK энкодер подключен к Arduino
// Кнопки
#define CLK_PIN 2                       // Указываем к какому выводу CLK энкодер подключен к Arduino
#define DT_PIN 3                          // Указываем к какому выводу DT энкодер подключен к Arduino
#define SW_PIN 4                         // Указываем к какому выводу SW энкодер подключен к Arduino (BUTTON)
KY040 g_rotaryEncoder(CLK_PIN, DT_PIN);
 

// Create an instance of the DHT11 class.
// - For Arduino: Connect the sensor to Digital I/O Pin 2.
DHT11 dht11(DHT_PIN);

LiquidCrystal_I2C lcd(0x27,16,2);  // set the LCD address to 0x27 for a 16 chars and 2 line display
bool update_lcd_up = true;
bool update_lcd_down = true;
// 0 - wait, 1 - выбор температуры, 2 - выбор времени, 3 - в процессе, 4 - завершено
byte current_mode = 4;
//
int target_temperature = 42;
int current_temperature = 0;
//
int time_left = 420;  // время в минутах
unsigned long previousMillis = 0;     // will store last time LED was updated
const long interval = 60000;           // interval 60 seconds

char data[20];
bool button_pressed = false;

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
int surprise = 10;  // пасхалка
bool is_surprise = false;  // пасхалка
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
    lcd.createChar(0, Celsius);
    lcd.createChar(1, Heart);
    lcd.init();
    lcd.backlight(); 
    //
    pinMode(REL_PIN, OUTPUT);                // Указываем вывод CLK как вход
    //
    pinMode(CLK_PIN, INPUT);                // Указываем вывод CLK как вход
    pinMode(DT_PIN, INPUT);                 // Указываем вывод DT как вход
    pinMode(SW_PIN, INPUT);          // Указываем вывод SW как вход и включаем подтягивающий резистор

    // Бывает двойное считывание?
    // attachInterrupt(digitalPinToInterrupt(CLK_PIN), ISR_rotaryEncoder, CHANGE);
    // attachInterrupt(digitalPinToInterrupt(DT_PIN), ISR_rotaryEncoder, CHANGE);

    // Uncomment the line below to set a custom delay between sensor readings (in milliseconds).
    //dht11.setDelay(1000); // Set this to the desired delay. Default is 500ms.
    getCurrentTemperature();
}

void loop() {
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

  if(is_surprise){
    lcd.setCursor(0, 1);
    lcd.print("I LOVE YOU!     ");
    lcd.setCursor(11, 1);
    lcd.write(byte(1));
    lcd.setCursor(13, 1);
    lcd.write(byte(1));
    lcd.setCursor(15, 1);
    lcd.write(byte(1));
    is_surprise = false;
  }
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
      sprintf(data, "Set TEMP:   %02d.C", target_temperature);
      lcd.setCursor(0, 0);
      lcd.print(String(data));
      lcd.setCursor(14, 0);
      lcd.write(byte(0));
      return;
    case 2:
      sprintf(data, "Set TIME: %02d:%02d ", time_left / 60, time_left % 60);
      lcd.setCursor(0, 0);
      lcd.print(String(data));
      return;
    case 3:
      sprintf(data, "%02d.C / %02d:%02d   ", target_temperature, time_left / 60, time_left % 60);
      lcd.setCursor(0, 0);
      lcd.print(String(data));
      lcd.setCursor(2, 0);
      lcd.write(byte(0));
      return;
    case 4:
      sprintf(data, "^__^ \ %02d:%02d  ", time_left / 60, time_left % 60);
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
  sprintf(data, "Temperature:%02d.C", current_temperature);
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
  button_pressed = true;
  update_lcd_up = true;
  Serial.println("Кнопка нажата");
  current_mode += 1;
  if(current_mode > 3){
    current_mode = 0;
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
      time_left -= 30;
      if(time_left < 30){
        time_left = 30;
        surprise -= 1;
        if(surprise <= 0){
          is_surprise = true;
          surprise = 10;
        }
      }
    }
    else {
      time_left += 30;
      if(time_left > 900){
        time_left = 900;
      }
    }
  }
}

void timer(){
  unsigned long currentMillis = millis();

  if (currentMillis - previousMillis >= interval) {
    // save the last time
    previousMillis = currentMillis;
    // Получаем температуру раз в минуту - что бы не блокировать поток инпута
    Serial.println("GET TEMPERATURE");
    getCurrentTemperature();
    if(current_mode < 3){
      return;
    }
    Serial.println(time_left);
    if(current_mode == 3) {
      time_left--;
      if(time_left <= 0){
        current_mode = 4;
      }
    }
    else if (current_mode == 4) {
      time_left++;
    }
    update_lcd_up = true;
  }
}

void heater() {
  // Если режим в процессе
  if(current_mode == 3){
  // Если температура ниже и работает таймер
    if(time_left > 0 && current_temperature < target_temperature){
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
        current_temperature = temperature;
        update_lcd_down = true;
    }
}