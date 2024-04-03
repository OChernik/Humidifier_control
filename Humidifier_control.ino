// Подключение периферии
// Экран SDA - A4, SCL - A5
// Датчик HDC1080 SDA - A4, SCL - A5
// Энкодер A - D2, B - D3, Key - D4
// Реле - 8

// Настройки
#define relayPin 8  // пин реле
#define encA 2      // пин А энкодера
#define encB 3      // пин В энкодера
#define encKEY 4    // пин KEY энкодера
#define period 2000 // период между опросом датчика в мс

// Библиотеки
#include <EncButton.h> //библиотека енкодера
#include <Wire.h> //библиотека i2c     
#include <GyverOLED.h> //библиотека дисплея 
#include "ClosedCube_HDC1080.h" //библиотека датчика температуры и влажности HDC1080

// Объекты и переменные
EncButton<EB_TICK, encA, encB, encKEY> enc;       //инициализация пинов енкодера с кнопкой <A, B, KEY> для чипа Arduino NANO
// EncButton<EB_TICK, encA, encB, encKEY> enc(INPUT);       //инициализация пинов енкодера с кнопкой <A, B, KEY> INPUT - для чипа LGT8F328P
// GyverOLED<SSH1106_128x64> oled; //инициализация экрана SSH1106 1,3''
GyverOLED<SSD1306_128x64, OLED_BUFFER> oled;  //инициализация экрана SSD1306 0,96''
ClosedCube_HDC1080 hdc1080;     // инициализация датчика температуры и влажности HDC1080
float temperature;              // температура измеренная
float humGet;                   // влажность измеренная
uint32_t tmr;                   // переменная таймера 
volatile int8_t humMin = 70;    // начальное значение минимальной влажности, изменяется в функции по прерыванию
volatile int8_t humMax = 75;    // начальное значение максимальной влажности, изменяется в функции по прерыванию
bool relayState = 0;            // состояние реле, 0 открыто, 1 замкнуто
volatile bool humMinChange = 0; // флаг изменения минимального значения влажности, изменяется в функции по прерыванию
volatile bool humMaxChange = 0; // флаг изменения максимального значения влажности, изменяется в функции по прерыванию
volatile bool Screen = 1;       // 0 на экран выводим минимальную влажность, 1 максимальную влажность

void setup() {
  Serial.begin(9600);          // инициализация порта вывода  
  hdc1080.begin(0x40);         // default settings: Heater off, 14 bit Temperature and Humidity Measurement Resolutions
  oled.textMode(BUF_REPLACE);  // вывод текста на экран с заменой символов
  oled.init();                 // инициализация дисплея
  oled.setScale(2);            // масштаб текста (1..4)
  pinMode(relayPin, OUTPUT);   // инициализация пина реле как выход 
  digitalWrite(relayPin, LOW); // начальный сигнал на реле
 // описываем прерывания, по которым будут считываться показания енкодера  
  attachInterrupt(0, encChange, CHANGE);  // прерывание на пин D2 с крутилки енкодера
  attachInterrupt(1, encChange, CHANGE);  // прерывание на пин D3 с крутилки енкодера
}

void loop() {
    
  // если енкодером было изменено минимальное или максимальное значение влажности
  if(humMinChange || humMaxChange){
    showScreen();                          // выводим переменные на дисплей 
    humMinChange = 0;                      // сбрасываем флаг изменения минимального значения влажности
    humMaxChange = 0;                      // сбрасываем флаг изменения максимального значения влажности
  } // end if

  if (millis() - tmr >= period){           // если пришло время считать данные с датчика
      tmr = millis();                      // сброс таймера

      if ((hdc1080.readTemperature() != temperature)) {   // если измеренная температура изменилась
       temperature = hdc1080.readTemperature();           // сохраняем измененное значение температуры
       showScreen();                                      // выводим переменные на дисплей 
      } // end if

      if ((hdc1080.readHumidity() != humGet)) {           // если измеренная влажность изменилась
       humGet = hdc1080.readHumidity();                   // сохраняем измененное значение влажности
       showScreen();                                      // выводим переменные на дисплей 
      } // end if
      
  }  // end if 

  if (!relayState && (humGet < humMin)) {                 // проверка условий для включения реле
    digitalWrite(relayPin, LOW);                          // включаем реле (реле обращенное, LOW включает)
    relayState = 1;                                       // поднимаем флаг, что реле включено
    showScreen();                                         // выводим переменные на дисплей 
  } // end if

  if (relayState && (humGet > humMax)) {                  // проверка условий для отключения реле
    digitalWrite(relayPin, HIGH);                         // выключаем реле (реле обращенное, HIGH выключает)
    relayState = 0;                                       // опускаем флаг, что реле включено
    showScreen();                                         // выводим переменные на дисплей 
  } // end if
} // end LOOP

// определяем функцию для вывода экрана с четырьмя переменными
// температура, влажность измеренная, влажность минимальная или максимальная, состояние реле
// Если Screen = 0 на экран выводим минимальную влажность, Screen = 1 максимальную влажность
void showScreen() {
    oled.clear();                // очищаем дисплей
    oled.setCursor(0, 0);        // курсор на начало 1 строки
    if(!Screen){                 // если Screen = 0, выводим значение минимальной влажности
        oled.print("ВлMin ");    // вывод ВлMin 
        oled.print(humMin);      // вывод значения минимальной влажности     
    } // end if
    if(Screen){                  // если Screen = 1, выводим значение максимальной влажности
        oled.print("ВлMax ");    // вывод ВлMax 
        oled.print(humMax);      // вывод значения максимальной влажности     
    } // end if   
    oled.setCursor(0, 2);        // курсор на начало 2 строки
    oled.print("ВлИзм ");        // вывод ВлИзм 
    oled.print(humGet);          // вывод значения ВлИзм
    oled.setCursor(0, 4);        // курсор на начало 3 строки
    oled.print("Темп  ");        // вывод Темп
    oled.print(temperature);     // вывод значения Т.
    oled.setCursor(0, 6);        // курсор на начало 4 строки
    oled.print("Реле  ");        // вывод Реле 
    if (!relayState) oled.print("Выкл");  // вывод значения реле
    if (relayState) oled.print("Вкл ");   // вывод значения реле
    oled.update();    // Вывод содержимого буфера на дисплей. Только при работе с буфером.
} // end showScreen

void encChange() {            // функция обработчик прерываний с енкодера
     enc.tick();
      if(enc.turn()){
       Screen = !Screen;              // если енкодер повернут без нажатия, обращаем выводимый экран     
      }  // end if
      if (Screen && enc.rightH()) {   // если нажатый поворот направо на первом экране
        humMax++;                     // увеличиваем значение максимальной влажности
        humMaxChange = 1;             // поднимаем флаг изменения максимальной значения влажности
      }  // end if
      if (Screen && enc.leftH()) {    // если нажатый поворот налево на первом экране
        humMax--;                     // уменьшаем значение максимальной влажности
        humMaxChange = 1;             // поднимаем флаг изменения максимальной значения влажности
      }  // end if       
      if (!Screen && enc.rightH()) {  // если нажатый поворот направо на нулевом экране
        humMin++;                     // увеличиваем значение минимальной влажности
        humMinChange = 1;             // поднимаем флаг изменения минимального значения влажности
      }  // end if
      if (!Screen && enc.leftH()) {   // если нажатый поворот налево на нулевом экране
        humMin--;                     // уменьшаем значение минимальной влажности
        humMinChange = 1;             // поднимаем флаг изменения минимального значения влажности
      }  // end if             
}  // end encChange