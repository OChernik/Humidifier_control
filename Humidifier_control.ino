// Подключение периферии
// Экран SDA - A4, SCL - A5
// Датчики температуры и влажности HDC1080, BME280, SHT3x, SHT4x
// SDA - A4, SCL - A5
// Энкодер A - D2, B - D3, Key - D4
// Реле - 8. При использовании реле логика включения обычно обратная, HIGH выключает
// Лучше использовать модуль Mosfet. В этом случае логика прямая, HIGH включает

// Настройки
#define relayPin 8       // пин реле
#define encodA 2         // пин А энкодера
#define encodB 3         // пин В энкодера
#define encodKEY 4       // пин KEY энкодера
#define period 2000      // период между опросом датчика в мс
#define EB_FAST_TIME 120 // таймаут быстрого поворота (энкодер)

// Библиотеки
#include <EncButton.h>             // библиотека енкодера v3.x
#include <Wire.h>                  // библиотека i2c     
#include <GyverOLED.h>             // библиотека дисплея 
#include "SHTSensor.h"             // библиотека датчиков температуры и влажности SHT3х и SHT4х
// #include "ClosedCube_HDC1080.h" // библиотека датчика температуры и влажности HDC1080
// #include <GyverBME280.h>        // библиотека датчика температуры и влажности BMP280 или BME280
// #include "Adafruit_SHT31.h"     // библиотека датчика температуры и влажности SHT31

// Объекты и переменные
EncButton enc(encodA, encodB, encodKEY);      //создание объекта енкодера enc и инициализация пинов с кнопкой EncButton v3.x
// GyverOLED<SSH1106_128x64> oled;            //создание объекта экрана oled для экрана SSH1106 1,3''
GyverOLED<SSD1306_128x64, OLED_BUFFER> oled;  //создание объекта экрана oled для экрана SSH1306 0,9''
SHTSensor sensor;                             // создание объекта датчика SHT3х или SHT4х библиотеки SHTSensor.h
// ClosedCube_HDC1080 sensor;                 // создание объекта датчика HDC1080 
// GyverBME280 sensor;                        // создание объекта датчика BME280 или BMP280 
// Adafruit_SHT31 sensor = Adafruit_SHT31();  // создание объекта датчика SHT31 

float temperature;                  // температура измеренная
float humGet;                       // влажность измеренная
uint32_t tmr;                       // переменная таймера, используется при периодичном опросе датчика
int8_t humSet = 60;                 // начальное значение заданной влажности
int8_t humDelta = 2;                // начальное значение отклонения от заданной влажности
bool screen = 0;                    // 0 на экран выводим заданную влажность, 1 отклонение от заданного
bool relayState = 0;                // состояние реле, 0 открыто, 1 замкнуто
uint8_t humMin = humSet - humDelta; // начальное значение минимальной влажности
uint8_t humMax = humSet + humDelta; // начальное значение максимальной влажности

void setup() {
  Serial.begin(9600);          // инициализация порта вывода 
  sensor.init();               // инициализируем датчик SHT3х или SHT4х библиотеки SHTSensor.h
  // sensor.begin();           // инициализируем датчики HDC1080, BME280/BMP280, SHT31 остальных библиотек 
  oled.textMode(BUF_REPLACE);  // вывод текста на экран с заменой символов
  oled.init();                 // инициализация дисплея
  oled.setScale(2);            // масштаб текста (1..4)
  oled.flipH(true);            // true/false - отзеркалить по горизонтали (для переворота экрана)
  oled.flipV(true);            // true/false - отзеркалить по вертикали (для переворота экрана)
  pinMode(relayPin, OUTPUT);   // инициализация пина реле как выход 
  digitalWrite(relayPin, LOW); // начальный сигнал на реле или мосфет
  // описываем прерывания соответствующие пинам енкодера и процедуру обработчик прерываний  
  attachInterrupt(0, encChange, CHANGE);  // прерывание на пин D2 с крутилки енкодера
  attachInterrupt(1, encChange, CHANGE);  // прерывание на пин D3 с крутилки енкодера
  enc.setEncISR(true);                    // требуется для работы с обработчиком прерываний EncButton v3.0
}

void loop() {  
  if (enc.tick()) {                                   // функция обработки сигнала (тикер) с энкодера 
    if (enc.turn() && !enc.turnH()) screen = !screen; // если енкодер повернут без нажатия обращаем выводимый экран     
    if (screen && enc.rightH()) humDelta++;           // если нажатый поворот направо на первом экране увеличиваем значение отклонения влажности       
    if (screen && enc.leftH())  humDelta--;           // если нажатый поворот налево на первом экране уменьшаем значение отклонения влажности         
    if (!screen && enc.rightH() && !enc.fast()) humSet++;   // если нажатый небыстрый поворот направо на нулевом экране увеличиваем заданную влажность на 1     
    if (!screen && enc.leftH() && !enc.fast()) humSet--;    // если нажатый небыстрый поворот налево на нулевом экране уменьшаем заданную влажность на 1
    if (!screen && enc.rightH() && enc.fast()) humSet += 5; // если нажатый быстрый поворот направо на нулевом экране увеличиваем заданную влажность на 5     
    if (!screen && enc.leftH() && enc.fast()) humSet -= 5;  // если нажатый быстрый поворот налево на нулевом экране уменьшаем заданную влажность на 5
    if (humDelta < 0) humDelta = 0;                   // проверка допустимости диапазона значений humDelta (от 0 до 5)
    if (humDelta > 5) humDelta = 5;                   // проверка допустимости диапазона значений humDelta (от 0 до 5)
    if (humSet < 5) humSet = 5;                       // проверка допустимости диапазона значений humSet (от 5 до 95)
    if (humSet > 95) humSet = 95;                     // проверка допустимости диапазона значений humSet (от 5 до 95)
    humMin = humSet - humDelta;                       // вычисляем новое значение humMin
    humMax = humSet + humDelta;                       // вычисляем новое значение humMах
    showScreen();                                     // выводим переменные на дисплей 
  }  // end if (enc.tick())
    
  if (millis() - tmr >= period){               // если пришло время считать данные с датчика
      tmr = millis();                          // сброс таймера на текущее значение времени
      sensor.readSample();                     // при использовании библиотеки SHTSensor.h для SHT3x, SHT4x
      temperature = sensor.getTemperature();   // при использовании библиотеки SHTSensor.h для SHT3x, SHT4x
      humGet = sensor.getHumidity();           // при использовании библиотеки SHTSensor.h для SHT3x, SHT4x
      //temperature = sensor.readTemperature();// сохраняем текущее значение температуры для остальных библиотек
      //humGet = sensor.readHumidity();        // сохраняем текущее значение влажности для остальных библиотек
      showScreen();                            // выводим переменные на дисплей     
  }  // end if 

  if (!relayState && (humGet < humMin)) {      // проверка условий для включения реле
    digitalWrite(relayPin, HIGH);              // включаем реле или мосфет (HIGH включает)
    relayState = 1;                            // поднимаем флаг, что реле включено
    showScreen();                              // выводим переменные на дисплей 
  } // end if

  if (relayState && (humGet > humMax)) {       // проверка условий для отключения реле
    digitalWrite(relayPin, LOW);               // выключаем реле или мосфет (LOW выключает)
    relayState = 0;                            // опускаем флаг, что реле включено
    showScreen();                              // выводим переменные на дисплей 
  } // end if
} // end LOOP

// Процедура для вывода экрана с четырьмя переменными
// температура, влажность измеренная, влажность заданная или отклонение влажности, состояние реле
// Если screen = 0 на экран выводим заданную влажность, screen = 1 отклонение влажности
void showScreen() {
    oled.clear();             // очищаем дисплей
    oled.setCursor(0, 0);     // курсор на начало 1 строки
    if(!screen){              // если screen = 0, выводим значение заданной влажности
        oled.print("ВлУст "); // вывод на экран "ВлУст" 
        oled.print(humSet);   // вывод значения установленной влажности     
    } // end if
    if(screen){               // если screen = 1, выводим значение допустимого отклонения влажности
        oled.print("Delta "); // вывод на экран "Delta" 
        oled.print(humDelta); // вывод значения допустимого отклонения влажности   
    } // end if   
    oled.setCursor(0, 2);     // курсор на начало 2 строки
    oled.print("ВлИзм ");     // вывод на экран "ВлИзм" 
    oled.print(humGet);       // вывод значения ВлИзм
    oled.setCursor(0, 4);     // курсор на начало 3 строки
    oled.print("Темп  ");     // вывод на экран "Темп"
    oled.print(temperature);  // вывод значения Т.
    oled.setCursor(0, 6);     // курсор на начало 4 строки
    oled.print("Реле  ");     // вывод на экран "Реле" 
    if (!relayState) oled.print("Выкл"); // вывод значения реле
    if (relayState) oled.print("Вкл ");  // вывод значения реле
    oled.update();            // Вывод содержимого буфера на дисплей
} // end showScreen

// Процедура обработчик прерываний с енкодера
void encChange() {  
     enc.tickISR();                 
} // end encChange
