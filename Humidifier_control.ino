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
#include <EncButton.h> //библиотека енкодера v3.x
#include <Wire.h>      //библиотека i2c     
#include <GyverOLED.h> //библиотека дисплея 
#include "ClosedCube_HDC1080.h" //библиотека датчика температуры и влажности HDC1080

// Объекты и переменные
EncButton enc(encA, encB, encKEY);                    //создание объекта енкодера enc и инициализация пинов с кнопкой для чипа Arduino NANO, EncButton v3.x
// EncButton<EB_TICK, encA, encB, encKEY> enc(INPUT); //создание объекта енкодера enc и инициализация пинов с кнопкой для чипа LGT8F328P, EncButton v2.0
// GyverOLED<SSH1106_128x64> oled;                    //создание объекта экрана oled для экрана SSH1106 1,3''
GyverOLED<SSD1306_128x64, OLED_BUFFER> oled;          //создание объекта экрана oled для экрана SSH1106 1,3''
ClosedCube_HDC1080 hdc1080;        // создание объекта датчика температуры и влажности HDC1080
float temperature;                 // температура измеренная
float humGet;                      // влажность измеренная
uint32_t tmr;                      // переменная таймера, используется при периодичном опросе датчика
int8_t humSet = 50;                // начальное значение заданной влажности
int8_t humDelta = 3;               // начальное значение отклонения от заданной влажности
bool screen = 0;                   // 0 на экран выводим заданную влажность, 1 отклонение от заданного
bool relayState = 0;               // состояние реле, 0 открыто, 1 замкнуто
int8_t humMin = humSet - humDelta; // начальное значение минимальной влажности
int8_t humMax = humSet + humDelta; // начальное значение максимальной влажности

void setup() {
  Serial.begin(9600);          // инициализация порта вывода  
  hdc1080.begin(0x40);         // default settings: Heater off, 14 bit Temperature and Humidity Measurement Resolutions
  oled.textMode(BUF_REPLACE);  // вывод текста на экран с заменой символов
  oled.init();                 // инициализация дисплея
  oled.setScale(2);            // масштаб текста (1..4)
  pinMode(relayPin, OUTPUT);   // инициализация пина реле как выход 
  digitalWrite(relayPin, LOW); // начальный сигнал на реле
  // описываем прерывания соответствующие пинам енкодера и процедуру обработчик прерываний  
  attachInterrupt(0, encChange, CHANGE);  // прерывание на пин D2 с крутилки енкодера
  attachInterrupt(1, encChange, CHANGE);  // прерывание на пин D3 с крутилки енкодера
  enc.setEncISR(true);                    // для работы с обработчиком прерываний EncButton v3.0
}

void loop() {  
  if enc.tick() {                             // функция обработки сигнала (тикер) с энкодера 
      if(enc.turn()) screen = !screen;        // если енкодер повернут без нажатия обращаем выводимый экран       
      if (screen && enc.rightH()) humDelta++; // если нажатый поворот направо на первом экране увеличиваем значение отклонения влажности       
      if (screen && enc.leftH())  humDelta--; // если нажатый поворот налево на первом экране уменьшаем значение отклонения влажности         
      if (!screen && enc.rightH()) humSet++;  // если нажатый поворот направо на нулевом экране увеличиваем значение заданной влажности       
      if (!screen && enc.leftH())  humSet--;  // если нажатый поворот налево на нулевом экране уменьшаем значение заданной влажности        
      humMin = humSet - humDelta;             // вычисляем новое значение humMin
      humMах = humSet + humDelta;             // вычисляем новое значение humMах
      showScreen();                           // выводим переменные на дисплей 
  }  // end if
    
  if (millis() - tmr >= period){                // если пришло время считать данные с датчика
      tmr = millis();                           // сброс таймера на текущее значение времени
      temperature = hdc1080.readTemperature();  // сохраняем текущее значение температуры
      humGet = hdc1080.readHumidity();          // сохраняем текущее значение влажности
      showScreen();                             // выводим переменные на дисплей     
  }  // end if 

  if (!relayState && (humGet < humMin)) {        // проверка условий для включения реле
    digitalWrite(relayPin, LOW);                 // включаем реле (реле обращенное, LOW включает)
    relayState = 1;                              // поднимаем флаг, что реле включено
    showScreen();                                // выводим переменные на дисплей 
  } // end if

  if (relayState && (humGet > humMax)) {         // проверка условий для отключения реле
    digitalWrite(relayPin, HIGH);                // выключаем реле (реле обращенное, HIGH выключает)
    relayState = 0;                              // опускаем флаг, что реле включено
    showScreen();                                // выводим переменные на дисплей 
  } // end if
} // end LOOP

// Процедура для вывода экрана с четырьмя переменными
// температура, влажность измеренная, влажность заданная или отклонение влажности, состояние реле
// Если screen = 0 на экран выводим заданную влажность, screen = 1 отклонение влажности
void showScreen() {
    oled.clear();                // очищаем дисплей
    oled.setCursor(0, 0);        // курсор на начало 1 строки
    if(!screen){                 // если screen = 0, выводим значение заданной влажности
        oled.print("ВлУст ");    // вывод на экран "ВлУст" 
        oled.print(humSet);      // вывод значения установленной влажности     
    } // end if
    if(screen){                  // если screen = 1, выводим значение допустимого отклонения влажности
        oled.print("Delta ");    // вывод на экран "Delta" 
        oled.print(humDelta);    // вывод значения допустимого отклонения влажности   
    } // end if   
    oled.setCursor(0, 2);        // курсор на начало 2 строки
    oled.print("ВлИзм ");        // вывод на экран "ВлИзм" 
    oled.print(humGet);          // вывод значения ВлИзм
    oled.setCursor(0, 4);        // курсор на начало 3 строки
    oled.print("Темп  ");        // вывод на экран "Темп"
    oled.print(temperature);     // вывод значения Т.
    oled.setCursor(0, 6);        // курсор на начало 4 строки
    oled.print("Реле  ");        // вывод на экран "Реле" 
    if (!relayState) oled.print("Выкл");  // вывод значения реле
    if (relayState) oled.print("Вкл ");   // вывод значения реле
    oled.update();    // Вывод содержимого буфера на дисплей. Только при работе с буфером.
} // end showScreen

// Процедура обработчик прерываний с енкодера
void encChange() {  
     enc.tickISR();                 
}  // end encChange
