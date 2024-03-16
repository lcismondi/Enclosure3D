
#include <U8g2lib.h>
#include <DHT.h>
#include <EEPROM.h>

#define SW 13
#define DT 12
#define CLK 16

#define LUZ 15
#define VT1 0
#define VT2 14  //Pin 14 comparte con eeprom? Posible error

#define DHTPIN 2

//#define DHTTYPE DHT11  // DHT 11S
#define DHTTYPE DHT22  // DHT 22  (AM2302), AM2321


//DHT11   0 < Temperatura < 50°C | 20 < humedad <  90 %
//DHT22 -40 < Temperatura < 80°C |  0 < humedad < 100 %

/*  Utiliza el valor de GPIO

  A0   A0   ADC
  NC   Reservado
  NC   Reservado
  S3   10   GPIO
  S2   09   Reservado
  S1   08   Reservado
  SC   11   Reservado
  SO   07   Reservado
  SK   06   Reservado
  GND
  3.3V
  EN   Enable
  RST  Reset
  GND
  VIN


  D0   16   GPIO*
  D1   05   SCL(I2C)
  D2   04   SDA (I2C)
  D3   00   GPIO (pull up)*
  D4   02   GPIO (pull up)[LED]
  3.3V
  GND
  D5  14    SLCK (SPI)
  D6  12    MISO (SPI)
  D7  13    MOSI (SPI)
  D8  15    CS (SPI)
  RX  03
  TX  01
  GND
  3.3V


  Caracterítica especiales
*/

//NO USA EL RESET, VER COMO SACARLO!
U8G2_SSD1306_128X32_UNIVISION_F_HW_I2C display(U8G2_R0, /* reset=*/16, /* clock=*/5, /* data=*/4);
/*
  U8G2_R0  No rotation, landscape
  U8G2_R1 90 degree clockwise rotation
  U8G2_R2 180 degree clockwise rotation
  U8G2_R3 270 degree clockwise rotation
  U8G2_MIRROR No rotation, landscape, display content is mirrored (v2.6.x)
*/

DHT dht(DHTPIN, DHTTYPE);

int counter = 0;
int currentStateCLK;
int currentStateDT;
int lastStateCLK;
int lastStateDT;

//String currentDir = "";
unsigned long lastButtonPress = 0;    //Antirebote botón al presionar
unsigned long lastButtonRelease = 0;  //Antirebote botón al soltar
unsigned long lastRotaryEncoder = 0;  //Antirebote rotary
unsigned long displayTime = 0;        //Tiempo de rotador de pantalla
unsigned long tiempoInicio = 0;       //Base de tiempo del reloj
unsigned long tiempoEncendido = 0;    //Sleep mode
unsigned long countLongPress = 0;     //Tiempo botón presionado
//unsigned long tiempoAlarma = 0;       //Duración de la alarma
//unsigned long simuPower = 0;              //Reloj simulación potencia

bool flagCLK;
bool upClk = LOW;      //Rotación encoder
bool downClk = LOW;    //Rotación encoder
bool newBtn = LOW;     //Flag de presión de botón
bool oldBtn = HIGH;    //Flag de liberación de botón
bool flagTimer = LOW;  //Suma 1 segundo o 30 segundos
bool alarma = LOW;     //Código de alarma


int cursorMenu = 0;     //Indicador menú o carrusel
int cursorDisplay = 0;  //Indicador de pantalla rotativa
int cursorLvl1 = 0;     //Indicador menú general
int cursorLvl2 = 1;     //Indicador submenú siempre inicia en 1



//**********************************************************************
//                          Personalizaciones
//**********************************************************************
const int submenuLength = 5;
const String menu[][submenuLength] = {
  //CONVERTIR EN UN VECTOR DE VECTORES PARA ENTENDER EL TAMAÑO
  { "Temperatura", "Set point", "Ripple", "Unidad", "Volver" },
  { "Reloj", "Modo", "Tiempo", "Reset", "Volver" },
  { "Ventilacion", "Modo", "Funcion", "Velocidad", "Volver" },
  { "Iluminacion", "Modo", "Alarma", "Brillo", "Volver" },
  { "Wifi", "Modo", "SSIS", "Nivel", "Volver" },
  { "Sistema", "Reiniciar", "Encoder", "Contraste", "Volver" },
  { "Salir" }
};

const int menuLength = (sizeof(menu) / sizeof(menu[0][0]) / submenuLength);

float menuVal[menuLength][submenuLength] = {};  //SE PUEDE COMPLETAR CON LAS VARIABLES?
/*
   menuVal[0][0] = Temperatura medida
   menuVal[0][1] = Temperatura seleccionada
   menuVal[0][2] = Ripple
   menuVal[0][3] = Unidad temperatura
   menuVal[0][4] = Humedad medida
   menuVal[1][0] = -
   menuVal[1][1] = Modo reloj (Off/Cronometro/Temporizador)
   menuVal[1][2] = Tiempo en milisegundos
   menuVal[1][3] = Reset
   menuVal[1][4] = -
   menuVal[2][0] = -
   menuVal[2][1] = Modo ventilacion (Manual/Automatico)
   menuVal[2][2] = Función ventilación (Progresivo/Simultáneo)
   menuVal[2][3] = Velocidad PWM
   menuVal[2][4] = -
   menuVal[3][0] = -
   menuVal[3][1] = Modo iluminación (Off/On)
   menuVal[3][2] = Alarma iluminación (Off/On)
   menuVal[3][3] = Brillo PWM
   menuVal[3][4] = -
   menuVal[4][0] = Modo WIFI (Scan/AP)
   menuVal[4][1] = SSID
   menuVal[4][2] = Potencia dBm
   menuVal[4][3] = -
   menuVal[4][4] = -
   menuVal[5][0] = -
   menuVal[5][1] = Reinicio
   menuVal[5][2] = Sensibilidad rotary encoder
   menuVal[5][3] = Contraste pantalla
   menuVal[5][4] = -
*/

const String modoReloj[] = { "OFF", "Cronometro", "Temporizador" };
const String modoVenti[] = { "Manual", "Automatico", "Ripple" };
const String funcVenti[] = { "Progresivo", "Simultaneo" };
const String unidTempe[] = { "C", "F" };
const String modoSINO[] = { "NO", "SI" };
const String modoOFFON[] = { "OFF", "ON" };


byte lightPWM = 0;   //Brillo de luz "on live"
int lightStep = 10;  //Pasos del dial de luz


//**********************************************************************
//                          Configuraciones
//**********************************************************************
int line0 = 10;                   //Primer linea de texto display
int line1 = 21;                   //Segunda linea de texto display
int line2 = 32;                   //Tercera linea de texto display
int displayRow = 121;             //Flechas de navegación
int eeAddress = 100;              //Location we want the data to be put.
const int eeOffset = eeAddress;   //Dirección desde donde comienza la memoria
unsigned int sleepTime = 600000;  //Tiempo para que entre en suspención

//**********************************************************************
//                          Prototipos
//**********************************************************************
void display2(void);
void renglones(String, String, String, int);
void displayLvl1(int);
void displayLvl2(int, int);
void displayLvl3(int, int);
void printHorario(void);
void printAntihorario(void);
void printBoton(void);
void progresivo(byte);
void power(void);
void softPWM(int);



void setup() {
  Serial.begin(115200);

  pinMode(CLK, INPUT_PULLUP);
  pinMode(DT, INPUT_PULLUP);
  pinMode(SW, INPUT_PULLUP);
  pinMode(LUZ, OUTPUT);
  pinMode(VT1, OUTPUT);
  pinMode(VT2, OUTPUT);

  lastStateCLK = digitalRead(CLK);

  //pinMode(LED_BUILTIN, OUTPUT); //Pin D0 (16)

  display.begin();
  display.setPowerSave(0);
  display.setDrawColor(2);
  display.setFontMode(1);
  display.setFont(u8g2_font_8x13_mf);
  display.setContrast(255);
  //display.setFont(u8g2_font_7x13B_mr);
  //display.setFont(u8g2_font_unifont_t_cyrillic);

  dht.begin();

  display.clearBuffer();
  display.setCursor(20, line1);
  display.print("Iniciando...");
  //El display tiene 16 posiciones horizontales
  //display.print("1234567891123456");
  //display.print("ABCDEFGHIJKLMNOP");
  display.sendBuffer();
  Serial.println("");

  //**********************************************************************
  //                Cargar de memoria las configuraciones
  //**********************************************************************
  EEPROM.begin(4096);

  delay(100);

  Serial.println("");
  //Cálculo tamaño de memoria
  Serial.println("Tamaño memoria: ");
  Serial.print("Menu: ");
  Serial.println(sizeof(menu));
  Serial.print("Menu[0][0]: ");
  Serial.println(sizeof(menu[0][0]));
  Serial.print("submenuLength: ");
  Serial.println(submenuLength);
  Serial.print("menuLength: ");
  Serial.println(menuLength);
  Serial.println("");
  Serial.println("Cargando memoria: ");
  for (int i = 0; i < menuLength - 1; i++) {
    for (int j = 0; j < submenuLength; j++) {
      EEPROM.get(eeAddress, menuVal[i][j]);
      Serial.print(eeAddress);
      Serial.print(" ");
      Serial.print(i);
      Serial.print(" ");
      Serial.print(j);
      Serial.print(" ");
      Serial.println(menuVal[i][j]);
      eeAddress += sizeof(menuVal[i][j]);
    }
  }

  if (eeAddress >= 4096) {
    Serial.println("Advertencia error overflow memoria EEPROM");
  }

  /*
    if( isnan(menuVal[i][j]) || menuVal[i][j]))
    {
    //Terminar de definir inicialización
    }
  */

  display.setContrast(menuVal[5][3]);
  lightPWM = menuVal[3][3];
  analogWrite(LUZ, lightPWM);
  //analogWrite(VT1, menuVal[2][3]);

  menuVal[0][4] = dht.readHumidity();
  menuVal[0][0] = dht.readTemperature();

  //Puede ser bloqueante!
  while (menuVal[0][0] <= -50) {
    delay(1000);
    menuVal[0][4] = dht.readHumidity();
    menuVal[0][0] = dht.readTemperature();
  }


  //Prueba de potencia
  //menuVal[0][0] = 0;
  //Prueba reconocimiento de sensor humedad

  Serial.println(DHTTYPE);
}

void loop() {

  //**********************************************************************
  //                          Control del cursor
  //**********************************************************************
  //Rotación

  currentStateCLK = digitalRead(CLK);
  currentStateDT = digitalRead(DT);

  if (currentStateCLK == LOW && currentStateCLK != lastStateCLK) {

    flagCLK = HIGH;
    lastStateCLK = currentStateCLK;

  } else if (currentStateCLK == HIGH && currentStateCLK != lastStateCLK) {

    flagCLK = LOW;
    lastStateCLK = currentStateCLK;

  }

  if (flagCLK == HIGH) {

    if (currentStateDT == LOW && currentStateDT != lastStateDT) {

      if (millis() - lastRotaryEncoder > menuVal[5][2] * 2) {
        counter++;
        flagCLK = LOW;
        upClk = HIGH;
        downClk = LOW;
        tiempoEncendido = millis();
      }

    } else if (currentStateDT == HIGH && currentStateDT != lastStateDT) {

      if (millis() - lastRotaryEncoder > menuVal[5][2]) {
        counter--;
        flagCLK = LOW;
        upClk = LOW;
        downClk = HIGH;
        tiempoEncendido = millis();
      }
    }

  }
  else{
    lastRotaryEncoder = millis();
    lastStateDT = currentStateDT;
  }




  //**********************************************************************
  //                          Botón
  //**********************************************************************

  // Read the button state
  int btnState = digitalRead(SW);

  //If we detect LOW signal, button is pressed
  if (btnState == LOW && newBtn == LOW) {
    //if 50ms have passed since last LOW pulse, it means that the
    //button has been pressed, released and pressed again
    if (millis() - lastButtonPress > 50) {

      //Suspención 10 minutos 600000 milisegundos
      if (millis() - tiempoEncendido >= sleepTime) {
        //Enciende
        //display.setPowerSave(LOW);
        tiempoEncendido = millis();
      } else {
        newBtn = HIGH;
        oldBtn = LOW;
        countLongPress = millis();
        tiempoEncendido = millis();
      }
    }
    // Remember last button press event
    lastButtonPress = millis();
  }

  //Si detecta señal HIGH, el botón se liberó
  if (btnState == HIGH && oldBtn == LOW) {
    //if 50ms have passed since last LOW pulse, it means that the
    //button has been pressed, released and pressed again
    if (millis() - lastButtonRelease > 50) {

      oldBtn = HIGH;
    }
    // Remember last button press event
    lastButtonRelease = millis();
  }


  //**********************************************************************
  //                          Display
  //**********************************************************************
  //Pantalla principal

  switch (cursorMenu) {
    case 0:
      //Rota la pantalla cada 3 segundos
      if (millis() - displayTime >= 3000) {
        displayTime = millis();
        if (cursorDisplay == 0) {
          cursorDisplay++;

          menuVal[0][4] = dht.readHumidity();
          //Serial.println(menuVal[0][4]);
          menuVal[0][0] = dht.readTemperature();
          //Serial.println(t);

          String uno = String("Temperat. ") + String((int)menuVal[0][0]) + String("/") + String((int)menuVal[0][1]) + String(unidTempe[(int)menuVal[0][3]]);
          String dos = String("Humedad   ") + String((int)menuVal[0][4]) + String("%");
          String tres = String("Velocidad ") + String(map(menuVal[2][3], 0, 255, 0, 100)) + String("%");

          renglones(uno, dos, tres, 3);

        } else if (cursorDisplay == 1) {
          cursorDisplay = 0;
          display2();
        }
      } else if (cursorDisplay == 0) {

        display2();

      } else if (cursorDisplay == 2) {

        display.clearBuffer();
        display.setCursor(20, line1);
        display.print("ALARMA!!!!");
        display.sendBuffer();
      }

      //Al rotar el dial cambia la luz
      if (upClk == HIGH && cursorDisplay <= 2) {
        cursorDisplay = 1;
        displayTime = millis();
        upClk = LOW;
        if (lightPWM <= 255 - lightStep) {
          lightPWM = lightPWM + lightStep;
        } else if (lightPWM > 255 - lightStep) {
          lightPWM = 255;
        }
        display2();
        analogWrite(LUZ, lightPWM);
      } else if (downClk == HIGH && cursorDisplay <= 2) {
        cursorDisplay = 1;
        displayTime = millis();
        downClk = LOW;
        if (lightPWM >= 0 + lightStep) {
          lightPWM = lightPWM - lightStep;
        } else if (lightPWM < 0 + lightStep) {
          lightPWM = 0;
        }
        display2();
        analogWrite(LUZ, lightPWM);
      }

      //Al presionar el botón actúa la primer etapa
      //Cambia la lógica de control en nivel tres actúa al soltar por long press
      if (newBtn == HIGH && oldBtn == HIGH) {

        newBtn = LOW;
        oldBtn = LOW;
        //Serial.print("Short press ");
        //Serial.println(millis() - countLongPress);
        //countLongPress = 0;

        //Apagado de pantalla y puertos
        if (millis() - countLongPress >= 1500) {
          if (cursorDisplay <= 2) {
            cursorDisplay = 3;
            display.clearBuffer();
            display.setCursor(20, line1);
            display.print("Apagando...");
            display.sendBuffer();
            Serial.println("");
            delay(500);
            analogWrite(VT1, 0);
            delay(500);
            analogWrite(VT2, 0);
            delay(500);
            analogWrite(LUZ, 0);
            delay(500);
            display.setPowerSave(HIGH);
          } else {

            cursorDisplay = 0;
            display.setPowerSave(LOW);
            display.clearBuffer();
            display.setCursor(20, line1);
            display.print("Iniciando...");
            display.sendBuffer();
            Serial.println(" ");
            Serial.println(" ");
            Serial.println("Iniciando...");
            delay(500);
            tiempoInicio = millis();
          }
        }
        //Al presionar el botón entra al menú
        else if (cursorDisplay <= 2) {
          newBtn = LOW;
          cursorMenu++;
          //Falta condición de volver
          String uno = menu[cursorLvl1][0];
          String dos = menu[cursorLvl1 + 1][0];
          String tres = menu[cursorLvl1 + 2][0];
          renglones(uno, dos, tres, 0);
          printBoton();

        } else if (cursorDisplay == 2) {
          newBtn = LOW;
          cursorDisplay = 0;
        }
      }

      break;

    //Primer menú general
    case 1:

      //Al rotar el dial cambia el renglon
      if (upClk == HIGH && downClk == LOW) {
        upClk = LOW;
        if (cursorLvl1 < menuLength - 1) {
          cursorLvl1++;
          displayLvl1(cursorLvl1);
          printHorario();
        }

      } else if (downClk == HIGH && upClk == LOW) {
        downClk = LOW;
        if (cursorLvl1 > 0) {
          cursorLvl1--;
          displayLvl1(cursorLvl1);
          printAntihorario();
        }
      }
      //Al presionar el botón entra al submenú
      if (newBtn == HIGH) {
        newBtn = LOW;

        if (menu[cursorLvl1][0] == "Salir") {
          cursorMenu--;
          cursorLvl1 = 0;
          displayLvl1(cursorLvl1);
          //String uno = menu[cursorLvl1][0];
          //String dos = menu[cursorLvl1][1];
          //String tres = menu[cursorLvl1][2];
          //renglones(uno, dos, tres, 0);
          printBoton();
        } else {
          cursorMenu++;
          displayLvl2(cursorLvl1, cursorLvl2);
          //String uno = menu[cursorLvl1][1];
          //String dos = menu[cursorLvl1][2];
          //String tres = menu[cursorLvl1][3];
          //renglones(uno, dos, tres, 0);
          printBoton();
        }
      }

      break;

    //Opción submenú
    case 2:
      //Al rotar el dial cambia el renglon
      if (upClk == HIGH) {
        upClk = LOW;
        if (cursorLvl2 < submenuLength - 1) {
          if (menu[cursorLvl1][cursorLvl2] != "Volver") {
            cursorLvl2++;
            printHorario();
          }
          displayLvl2(cursorLvl1, cursorLvl2);
        }
      } else if (downClk == HIGH) {
        downClk = LOW;
        if (cursorLvl2 > 1) {
          cursorLvl2--;
          displayLvl2(cursorLvl1, cursorLvl2);
          printAntihorario();
        }
      }
      //Al presionar el botón entra al submenú
      if (newBtn == HIGH) {
        newBtn = LOW;

        if (menu[cursorLvl1][cursorLvl2] == "Volver") {
          cursorMenu--;
          cursorLvl2 = 1;  //El cursor de segundo nivel no puede ser menor a 1
          displayLvl1(cursorLvl1);
          printBoton();
        } else {
          cursorMenu++;
          displayLvl3(cursorLvl1, cursorLvl2);
          printBoton();
        }
      }


      break;


    //Configuración de valores del submenú
    case 3:

      /*
         menuVal[0][0] = Temperatura medida
         menuVal[0][1] = Temperatura seleccionada
         menuVal[0][2] = Ripple
         menuVal[0][3] = Unidad temperatura
         menuVal[0][4] = Humedad medida
         menuVal[1][0] = -
         menuVal[1][1] = Modo reloj (Off/Cronometro/Temporizador)
         menuVal[1][2] = Tiempo en milisegundos
         menuVal[1][3] = Reset
         menuVal[1][4] = -
         menuVal[2][0] = -
         menuVal[2][1] = Modo ventilacion (Manual/Automatico)
         menuVal[2][2] = Función ventilación (Progresivo/Simultáneo)
         menuVal[2][3] = Velocidad PWM
         menuVal[2][4] = -
         menuVal[3][0] = -
         menuVal[3][1] = Modo iluminación (Off/On)
         menuVal[3][2] = Alarma iluminación (Off/On)
         menuVal[3][3] = Brillo PWM
         menuVal[3][4] = -
         menuVal[4][0] = Modo WIFI (Scan/AP)
         menuVal[4][1] = SSID
         menuVal[4][2] = Porencia dBm
         menuVal[4][3] = -
         menuVal[4][4] = -
         menuVal[5][0] = -
         menuVal[5][1] = Reinicio
         menuVal[5][2] = Sensibilidad rotary encoder
         menuVal[5][3] = Contraste pantalla
         menuVal[5][4] = -
      */

      //Al rotar el dial cambia el valor de configuración
      if (upClk == HIGH) {
        upClk = LOW;

        //Temperatura seleccionada
        if (cursorLvl1 == 0 && cursorLvl2 == 1) {
          //Para °C el límite es 99 y para °F es 210
          if (menuVal[cursorLvl1][cursorLvl2] < ((210 - 99) * menuVal[0][3] + 99)) {
            menuVal[cursorLvl1][cursorLvl2]++;
          }
        }
        //Riple
        else if (cursorLvl1 == 0 && cursorLvl2 == 2) {
          //Para °C el límite es 10 y para °F es 18
          if (menuVal[cursorLvl1][cursorLvl2] < ((18 - 10) * menuVal[0][3] + 10)) {
            menuVal[cursorLvl1][cursorLvl2]++;
          }
        }
        //Unidad temperatura
        else if (cursorLvl1 == 0 && cursorLvl2 == 3) {
          //Pasó a °F
          if (menuVal[cursorLvl1][cursorLvl2] < sizeof(unidTempe) / sizeof(unidTempe[0]) - 1) {
            menuVal[cursorLvl1][cursorLvl2]++;
            //Conversión de °C a °F: °F = (°C x 1.8) + 32
            //Conversión de °F a °C: °C = (°F - 32) / 1.8

            //menuVal[0][1] = round(menuVal[0][1] * 1.8 + 32);
            //menuVal[0][2] = round(menuVal[0][2] * 1.8 + 32);
          }
        }
        //Modo reloj
        else if (cursorLvl1 == 1 && cursorLvl2 == 1) {
          if (menuVal[cursorLvl1][cursorLvl2] < sizeof(modoReloj) / sizeof(modoReloj[0]) - 1) {
            menuVal[cursorLvl1][cursorLvl2]++;
          }
        }
        //Tiempo reloj
        else if (cursorLvl1 == 1 && cursorLvl2 == 2) {

          //Si flagTimer es 0 suma 1 minuto si flagTimer es 1 suma 30 minutos
          menuVal[cursorLvl1][cursorLvl2] = menuVal[cursorLvl1][cursorLvl2] + 60000 + (flagTimer * 29 * 60000);

        }
        //Reset tiempo
        else if (cursorLvl1 == 1 && cursorLvl2 == 3) {
          if (menuVal[cursorLvl1][cursorLvl2] < sizeof(modoSINO) / sizeof(modoSINO[0]) - 1) {
            menuVal[cursorLvl1][cursorLvl2]++;
          }
        }
        //Modo ventilación
        else if (cursorLvl1 == 2 && cursorLvl2 == 1) {
          if (menuVal[cursorLvl1][cursorLvl2] < sizeof(modoVenti) / sizeof(modoVenti[0]) - 1) {
            menuVal[cursorLvl1][cursorLvl2]++;
          }
        }
        //Funcion ventilación
        else if (cursorLvl1 == 2 && cursorLvl2 == 2) {
          if (menuVal[cursorLvl1][cursorLvl2] < sizeof(funcVenti) / sizeof(funcVenti[0]) - 1) {
            menuVal[cursorLvl1][cursorLvl2]++;
          }
        }
        //Velocidad ventilación
        else if (cursorLvl1 == 2 && cursorLvl2 == 3) {
          if (menuVal[cursorLvl1][cursorLvl2] < 255 - lightStep) {
            menuVal[cursorLvl1][cursorLvl2] = menuVal[cursorLvl1][cursorLvl2] + lightStep;
          } else {
            menuVal[cursorLvl1][cursorLvl2] = 255;
          }
          analogWrite(VT1, menuVal[cursorLvl1][cursorLvl2]);
        }
        //Modo iluminación
        else if (cursorLvl1 == 3 && cursorLvl2 == 1) {
          if (menuVal[cursorLvl1][cursorLvl2] < sizeof(modoOFFON) / sizeof(modoOFFON[0]) - 1) {
            menuVal[cursorLvl1][cursorLvl2]++;
          }
        }
        //Modo alarma
        else if (cursorLvl1 == 3 && cursorLvl2 == 2) {
          if (menuVal[cursorLvl1][cursorLvl2] < sizeof(modoOFFON) / sizeof(modoOFFON[0]) - 1) {
            menuVal[cursorLvl1][cursorLvl2]++;
          }
        }
        //Brillo
        else if (cursorLvl1 == 3 && cursorLvl2 == 3) {
          if (menuVal[cursorLvl1][cursorLvl2] < 255 - lightStep) {
            menuVal[cursorLvl1][cursorLvl2] = menuVal[cursorLvl1][cursorLvl2] + lightStep;
          } else {
            menuVal[cursorLvl1][cursorLvl2] = 255;
          }
          lightPWM = menuVal[cursorLvl1][cursorLvl2];
          analogWrite(LUZ, lightPWM);
        }
        //Reinicio
        else if (cursorLvl1 == 5 && cursorLvl2 == 1) {
          if (menuVal[cursorLvl1][cursorLvl2] < sizeof(modoSINO) / sizeof(modoSINO[0]) - 1) {
            menuVal[cursorLvl1][cursorLvl2]++;
          }
        }
        //Contraste
        else if (cursorLvl1 == 5 && cursorLvl2 == 3) {
          if (menuVal[cursorLvl1][cursorLvl2] < 255 - lightStep) {
            menuVal[cursorLvl1][cursorLvl2] = menuVal[cursorLvl1][cursorLvl2] + lightStep;
          } else {
            menuVal[cursorLvl1][cursorLvl2] = 255;
          }

          display.setContrast(menuVal[5][3]);

        } else {
          menuVal[cursorLvl1][cursorLvl2]++;
        }

        displayLvl3(cursorLvl1, cursorLvl2);

      } else if (downClk == HIGH) {
        downClk = LOW;

        //Seleccionada
        if (  //Ripple
          cursorLvl1 == 0 && cursorLvl2 == 2 ||
          //Modo reloj
          cursorLvl1 == 1 && cursorLvl2 == 1 ||
          //Modo ventilación
          cursorLvl1 == 2 && cursorLvl2 == 1 ||
          //Funcion ventilación
          cursorLvl1 == 2 && cursorLvl2 == 2 ||
          //Modo iluminación
          cursorLvl1 == 3 && cursorLvl2 == 1 ||
          //modo alarma
          cursorLvl1 == 3 && cursorLvl2 == 2 ||
          //Sensibilidad rotary encoder
          cursorLvl1 == 5 && cursorLvl2 == 2) {
          if (menuVal[cursorLvl1][cursorLvl2] > 0) {
            menuVal[cursorLvl1][cursorLvl2]--;
          }
        }

        //Temperatura seleccionada
        else if (cursorLvl1 == 0 && cursorLvl2 == 1) {
          //No permite temperaturas negativas
          if (menuVal[cursorLvl1][cursorLvl2] > (32 - 0) * menuVal[0][3] + 0) {
            menuVal[cursorLvl1][cursorLvl2]--;
          }
        }
        //Unidad temperatura
        else if (cursorLvl1 == 0 && cursorLvl2 == 3) {
          //Pasó a °C
          if (menuVal[cursorLvl1][cursorLvl2] > 0) {
            menuVal[cursorLvl1][cursorLvl2]--;
            //Conversión de °C a °F: °F = (°C x 1.8) + 32
            //Conversión de °F a °C: °C = (°F - 32) / 1.8

            //menuVal[0][1] = round((menuVal[0][1] - 32 ) / 1.8);
            //menuVal[0][2] = round((menuVal[0][2] - 32 ) / 1.8);
          }
        }
        //Tiempo reloj
        else if (cursorLvl1 == 1 && cursorLvl2 == 2) {
          //Si falgstimer es 0 resta de a 1 minuto si flagTimer es 1 resta de a 30 minutos
          if (menuVal[cursorLvl1][cursorLvl2] >= 60000 + (flagTimer * 29 * 60000)) {
            menuVal[cursorLvl1][cursorLvl2] = menuVal[cursorLvl1][cursorLvl2] - 60000 - (flagTimer * 29 * 60000);
          }
        }
        //Reset tiempo
        else if (cursorLvl1 == 1 && cursorLvl2 == 3) {
          if (menuVal[cursorLvl1][cursorLvl2] >= sizeof(modoSINO) / sizeof(modoSINO[0]) - 1) {
            menuVal[cursorLvl1][cursorLvl2]--;
          }
        }
        //Velocidad ventilación
        else if (cursorLvl1 == 2 && cursorLvl2 == 3) {
          if (menuVal[cursorLvl1][cursorLvl2] >= lightStep) {
            menuVal[cursorLvl1][cursorLvl2] = menuVal[cursorLvl1][cursorLvl2] - lightStep;
          } else {
            menuVal[cursorLvl1][cursorLvl2] = 0;
          }
          analogWrite(VT1, menuVal[cursorLvl1][cursorLvl2]);
        }
        //Brillo
        else if (cursorLvl1 == 3 && cursorLvl2 == 3) {
          if (menuVal[cursorLvl1][cursorLvl2] >= lightStep) {
            menuVal[cursorLvl1][cursorLvl2] = menuVal[cursorLvl1][cursorLvl2] - lightStep;
          } else {
            menuVal[cursorLvl1][cursorLvl2] = 0;
          }
          lightPWM = menuVal[cursorLvl1][cursorLvl2];
          analogWrite(LUZ, lightPWM);
        }
        //Reinicio
        else if (cursorLvl1 == 5 && cursorLvl2 == 1) {
          if (menuVal[cursorLvl1][cursorLvl2] >= sizeof(modoSINO) / sizeof(modoSINO[0]) - 1) {
            menuVal[cursorLvl1][cursorLvl2]--;
          }
        }
        //Contraste pantalla
        else if (cursorLvl1 == 5 && cursorLvl2 == 3) {
          if (menuVal[cursorLvl1][cursorLvl2] >= lightStep) {
            menuVal[cursorLvl1][cursorLvl2] = menuVal[cursorLvl1][cursorLvl2] - lightStep;
          } else {
            menuVal[cursorLvl1][cursorLvl2] = 0;
          }

          display.setContrast(menuVal[5][3]);

        } else {
          menuVal[cursorLvl1][cursorLvl2]--;
        }

        displayLvl3(cursorLvl1, cursorLvl2);
      }

      //Al presionar el botón actúa la primer etapa
      //Cambia la lógica de control en nivel tres actúa al soltar por long press
      if (newBtn == HIGH && oldBtn == HIGH) {

        newBtn = LOW;
        oldBtn = LOW;
        Serial.print("Short press ");
        Serial.println(millis() - countLongPress);
        //countLongPress = 0;

        //Reset tiempo
        if (cursorLvl1 == 1 && cursorLvl2 == 3 && modoSINO[(int)menuVal[cursorLvl1][cursorLvl2]] == "SI") {
          tiempoInicio = millis();

          cursorMenu--;
          printBoton();
          displayLvl2(cursorLvl1, cursorLvl2);
        }
        //Reinicio a modo de fábrica
        else if (cursorLvl1 == 5 && cursorLvl2 == 1 && modoSINO[(int)menuVal[cursorLvl1][cursorLvl2]] == "SI") {

          display.clearBuffer();
          display.setCursor(2, line1);
          display.print("Modo fabrica...");
          display.sendBuffer();

          delay(1000);

          //Inicialización de variables
          //menuVal[0][0] = Temperatura medida
          menuVal[0][0] = 0;
          //menuVal[0][1] = Temperatura seleccionada
          menuVal[0][1] = 35;
          //menuVal[0][2] = Ripple
          menuVal[0][2] = 3;
          //menuVal[0][3] = Unidad tempertura
          menuVal[0][3] = 0;
          //menuVal[0][4] = Humedad medida
          menuVal[0][4] = 0;
          //menuVal[1][0] = -
          menuVal[1][0] = 0;
          //menuVal[1][1] = Modo reloj (Off/Cronometro/Temporizador)
          menuVal[1][1] = 1;
          //menuVal[1][2] = Tiempo en milisegundos
          menuVal[1][2] = 0;
          //menuVal[1][3] = Reinicio
          menuVal[1][3] = 0;
          //menuVal[1][4] = -
          menuVal[1][4] = 0;
          //menuVal[2][0] = -
          menuVal[2][0] = 0;
          //menuVal[2][1] = Modo ventilacion (Manual/Automatico)
          menuVal[2][1] = 1;
          //menuVal[2][2] = Función ventilación (Progresivo/Simultáneo)
          menuVal[2][2] = 0;
          //menuVal[2][3] = Velocidad PWM
          menuVal[2][3] = 0;
          //menuVal[2][4] = -
          menuVal[2][4] = 0;
          //menuVal[3][0] = -
          menuVal[3][0] = 0;
          //menuVal[3][1] = Alarma iluminación (Off/On)
          menuVal[3][1] = 1;
          //menuVal[3][2] = Modo alarma
          menuVal[3][2] = 0;
          //menuVal[3][3] = Brillo PWM
          menuVal[3][3] = 255;
          //menuVal[3][4] = -
          menuVal[3][4] = 0;
          //menuVal[4][0] = Modo WIFI (Scan/AP)
          menuVal[4][0] = 0;
          //menuVal[4][1] = SSID
          menuVal[4][1] = 0;
          //menuVal[4][2] = Potencia dBm
          menuVal[4][2] = 0;
          //menuVal[4][3] = -
          menuVal[4][3] = 0;
          //menuVal[4][4] = -
          menuVal[4][4] = 0;
          //menuVal[5][0] = -
          menuVal[5][0] = 0;
          //menuVal[5][1] = Reinicio
          menuVal[5][1] = 0;
          //menuVal[5][2] = Sensibilidad rotary encoder
          menuVal[5][2] = 12;
          //menuVal[5][3] = Contraste pantalla
          menuVal[5][3] = 255;
          //menuVal[5][4] = -
          menuVal[5][4] = 0;

          eeAddress = eeOffset;

          Serial.println("Reinicio a modo de fábrica");
          for (int i = 0; i < menuLength - 1; i++) {
            for (int j = 0; j < submenuLength; j++) {
              EEPROM.put(eeAddress, menuVal[i][j]);
              Serial.print(eeAddress);
              Serial.print(" ");
              Serial.print(i);
              Serial.print(" ");
              Serial.print(j);
              Serial.print(" ");
              Serial.println(menuVal[i][j]);
              eeAddress += sizeof(menuVal[i][j]);
            }
          }

          if (eeAddress >= 4096) {
            Serial.println("Advertencia error overflow memoria EEPROM");
          }

          if (EEPROM.commit()) {
            Serial.println(" ");
            Serial.println("EEPROM successfully committed");
            Serial.println(" ");
          } else {
            Serial.println(" ");
            Serial.println("ERROR! EEPROM commit failed");
            Serial.println(" ");
          }


          //Reinicia el menú
          cursorMenu = 0;     //Indicador menú o carrusel
          cursorDisplay = 0;  //Indicador de pantalla rotativa
          cursorLvl1 = 0;     //Indicador menú general
          cursorLvl2 = 1;     //El cursor de segundo nivel no puede ser menor a 1


          displayLvl1(cursorLvl1);
          printBoton();

        }
        //Tiempo reloj Long press 1.5 segundos prácticos 3 segundos de usuario
        else if (cursorLvl1 == 1 && cursorLvl2 == 2 && (millis() - countLongPress) >= 1500) {

          flagTimer = !flagTimer;

          Serial.print("Long Press ");
          Serial.println(millis() - countLongPress);
          displayLvl3(cursorLvl1, cursorLvl2);

        }
        //Para el resto de las variables solo guarda su valor
        else {

          cursorMenu--;
          printBoton();


          //Guardar en memoria el nuevo valor de la variable solo si no sufrio cambios

          eeAddress = eeOffset + sizeof(float) * (cursorLvl1 * submenuLength + cursorLvl2);

          //Compara por float
          float anterior = 0;
          EEPROM.get(eeAddress, anterior);
          if (anterior != menuVal[cursorLvl1][cursorLvl2]) {
            Serial.print(eeAddress);
            Serial.print(" ");
            Serial.print(cursorLvl1);
            Serial.print(" ");
            Serial.print(cursorLvl2);
            Serial.print(" ");
            Serial.println(menuVal[cursorLvl1][cursorLvl2]);
            EEPROM.put(eeAddress, menuVal[cursorLvl1][cursorLvl2]);

            //Si cambió la unidad de medida debe corregir y guardar las temperaturas
            if (cursorLvl1 == 0 && cursorLvl2 == 3) {

              //Conversión de °C a °F: °F = (°C x 1.8) + 32
              //Conversión de °F a °C: °C = (°F - 32) / 1.8

              //°C
              if (menuVal[0][3] == 0) {
                //Temperatura setpoint
                menuVal[0][1] = round((menuVal[0][1] - 32) / 1.8);
                eeAddress = eeOffset + sizeof(float) * (0 * submenuLength + 1);
                EEPROM.put(eeAddress, menuVal[0][1]);

                //Riple
                menuVal[0][2] = round(menuVal[0][2] / 1.8);
                eeAddress = eeOffset + sizeof(float) * (0 * submenuLength + 2);
                EEPROM.put(eeAddress, menuVal[0][2]);
              }
              //°F
              else if (menuVal[0][3] == 1) {
                //Temperatura setpoint
                menuVal[0][1] = round(menuVal[0][1] * 1.8 + 32);
                eeAddress = eeOffset + sizeof(float) * (0 * submenuLength + 1);
                EEPROM.put(eeAddress, menuVal[0][1]);

                //Riple
                menuVal[0][2] = round(menuVal[0][2] * 1.8);
                eeAddress = eeOffset + sizeof(float) * (0 * submenuLength + 2);
                EEPROM.put(eeAddress, menuVal[0][2]);
              }
            }

            if (EEPROM.commit()) {
              Serial.println("EEPROM successfully committed");
            } else {
              Serial.println("ERROR! EEPROM commit failed");
            }

            //El pin 14 del switch comparte con SPI (eeprom)
            while (digitalRead(SW) == LOW) {
              //Nada
            }
          }

          displayLvl2(cursorLvl1, cursorLvl2);

          //Compara por bytes FALTA CORREGIR E IMPLEMENTAR
          //Serial.println(sizeof(menuVal[cursorLvl1][cursorLvl2]));
          /*
            Serial.println("Cargando memoria: ");
            eeAddress = eeOffset;
            for (int i = 0; i < menuLength; i++) {
            for (int j = 0; j < submenuLength; j++)
            {
              EEPROM.get(eeAddress, menuVal[i][j]);
              Serial.print(eeAddress);
              Serial.print(" ");
              Serial.print(i);
              Serial.print(" ");
              Serial.print(j);
              Serial.print(" ");
              Serial.println(menuVal[i][j]);
              eeAddress += sizeof(menuVal[i][j]);
            }
            }
                    /*
            for (int i = 0; i < sizeof(menuVal[cursorLvl1][cursorLvl2]); i++) {

              if (EEPROM.read(eeAddress + i) != menuVal[cursorLvl1][cursorLvl2][i])
              {
              //EEPROM.write(eeAddress + i, menuVal[cursorLvl1][cursorLvl2][i]);
              Serial.print(menuVal[cursorLvl1][cursorLvl2]);
              Serial.print(" ");
              //Serial.println(menuVal[cursorLvl1][cursorLvl2][i]);
              }
              }
          */
        }
      }
      break;
  }

  //Funciona solo cuando no está apagado cursorDisplay == 3
  if (cursorDisplay <= 2) {

    //**********************************************************************
    //**********************************************************************
    //                          Lógica de control
    //**********************************************************************
    //**********************************************************************

    //Suspención 10 minutos 600000 milisegundos
    if (millis() - tiempoEncendido >= sleepTime) {
      //Apagado
      display.setPowerSave(HIGH);
      //Todas las acciones de activacación deben resetear la base de tiempo
      //tiempoEncendido = millis(); para reanudar
    } else {
      display.setPowerSave(LOW);
    }


    //**********************************************************************
    //                          Control ventiladores
    //**********************************************************************

    //Modo manual
    if (menuVal[2][1] == 0) {
      //Función progresivo
      if (menuVal[2][2] == 0) {

        progresivo((byte)menuVal[2][3]);

      }
      //Función simultaneo
      else if (menuVal[2][2] == 1) {
        analogWrite(VT1, menuVal[2][3]);
        analogWrite(VT2, menuVal[2][3]);
      }

    }
    //Modo automático
    else if (menuVal[2][1] == 1) {
      //Función progresivo
      if (menuVal[2][2] == 0) {
        power();
        progresivo((byte)menuVal[2][3]);
        //Simulación de potencia
        /*if (millis() - simuPower  >= 1000)
          {
          //Valores simulación
          Serial.print("Temperatura: ");
          Serial.println(menuVal[0][0]);
          Serial.print("Potencia: ");
          Serial.println((byte)menuVal[2][3]);
          //Serial.print("Temperatura: ");
          //Serial.println();
          Serial.println("");

          simuPower = millis();
          menuVal[0][0]++;
          if (menuVal[0][0] > 60)
          {
            menuVal[0][0] = 0;
          }
          }*/
      }
      //Función simultaneo
      else if (menuVal[2][2] == 1) {

        power();
        analogWrite(VT1, menuVal[2][3]);
        analogWrite(VT2, menuVal[2][3]);
      }
    }
    //Modo riple
    else if (menuVal[2][1] == 2) {

      //El error va de 0 a 255
      //0 = setpoint - riple
      //255 = setpont + riple
      //menuVal[0][0] = Temperatura medida
      //menuVal[0][1] = Temperatura seleccionada
      //menuVal[0][2] = riple

      //Por debajo del riple
      //Temperatura medida <= Temperatura seleccionada - Ripple
      if ((menuVal[0][0] <= menuVal[0][1] - menuVal[0][2])) {
        menuVal[2][3] = 0;
        analogWrite(VT1, menuVal[2][3]);
        analogWrite(VT2, menuVal[2][3]);
      }
      //Por debajo del setpoint
      //Temperatura medida <= Temperatura seleccionada
      else if (menuVal[0][0] <= menuVal[0][1]) {
        //Función progresivo
        if (menuVal[2][2] == 0) {
          menuVal[2][3] = 255;
          analogWrite(VT1, menuVal[2][3]);
          analogWrite(VT2, 0);
          tiempoEncendido = millis();
        }
        //Función simultaneo
        else if (menuVal[2][2] == 1) {
          menuVal[2][3] = 0;
          analogWrite(VT1, menuVal[2][3]);
          analogWrite(VT2, menuVal[2][3]);
        }
      }
      //Por sobre el setpoint
      //Temperatura medida <= (Temperatura seleccionada + Ripple)
      else if (menuVal[0][0] <= (menuVal[0][1] + menuVal[0][2])) {
        //Función progresivo
        if (menuVal[2][2] == 0) {
          menuVal[2][3] = 255;
          analogWrite(VT1, 0);
          analogWrite(VT2, menuVal[2][3]);
          tiempoEncendido = millis();
        }
        //Función simultaneo
        else if (menuVal[2][2] == 1) {
          menuVal[2][3] = 255;
          analogWrite(VT1, menuVal[2][3]);
          analogWrite(VT2, menuVal[2][3]);
          tiempoEncendido = millis();
        }
      }
      //Por sobre el riple
      //Temperatura seleccionada + Ripple <= Temperatura medida
      else {
        menuVal[2][3] = 255;
        analogWrite(VT1, menuVal[2][3]);
        analogWrite(VT2, menuVal[2][3]);
        tiempoEncendido = millis();
      }
    }
    //**********************************************************************
    //                            Control alarmas
    //**********************************************************************

    if (alarma == HIGH) {
      cursorDisplay = 2;

      //Modo reloj
      if (menuVal[1][1] == 0) {
      }
      //Modo cronometro
      else if (menuVal[1][1] == 1) {
      }
      //Modo temporizador
      else if (menuVal[1][1] == 2) {
      }
    }
    //analogWrite(LUZ,menuVal[3][3]); //Debería ser lightPWM
  }
}

//**********************************************************************
//**********************************************************************
//                            Funciones
//**********************************************************************
//**********************************************************************


//**********************************************************************
//                Actualiza la segunda pantalla rotativa
//**********************************************************************
void display2(void) {

  //menuVal[1][1] = Modo reloj (Off/Cronometro/Temporizador)
  String uno = "";
  unsigned long reloj = 0;  //Tiempo actual transcurrido

  //Modo reloj
  if (menuVal[1][1] == 0) {
    //*********************************************************************
    //                               OFF
    //*********************************************************************

    uno = String("Hora ") + String("00:00:00") + String("hs");

  } else if (menuVal[1][1] == 1) {
    //*********************************************************************
    //                            Cronometro
    //*********************************************************************

    reloj = millis() - tiempoInicio;

    int horas = (int)(reloj / 1000 / 60 / 60);

    if (horas > 99) {
      //ALARMA!!!!
      uno = String("Crono ") + String("99:00:00") + String("hs");
      alarma = 1;
    } else {
      uno = String("Crono ");

      if (horas < 10) {
        uno = uno + String("0");
      }

      uno = uno + String(horas) + String(":");

      int minutos = reloj / 1000;

      if (minutos >= 3600) {
        minutos = minutos - (int)(minutos / 3600) * 3600;
      }

      minutos = minutos / 60;

      if (minutos < 10) {
        uno = uno + String("0");
      }

      uno = uno + String(minutos) + String(":");

      int segundos = reloj / 1000;

      if (segundos >= 60) {
        segundos = segundos - (int)(segundos / 60) * 60;
      }

      if (segundos < 10) {
        uno = uno + String("0");
      }

      uno = uno + String(segundos) + String("hs");
    }
  } else if (menuVal[1][1] == 2) {
    //*********************************************************************
    //                           Temporizador
    //*********************************************************************

    if (millis() - tiempoInicio >= menuVal[1][2]) {
      //ALARMA!!!!
      uno = String("Tempo ") + String("00:00:00") + String("hs");
      alarma = 1;
    } else {

      reloj = menuVal[1][2] - (millis() - tiempoInicio);

      uno = String("Tempo ");

      int horas = (int)(reloj / 1000 / 60 / 60);

      if (horas < 10) {
        uno = uno + String("0");
      }

      uno = uno + String(horas) + String(":");

      int minutos = reloj / 1000;

      if (minutos >= 3600) {
        minutos = minutos - (int)(minutos / 3600) * 3600;
      }

      minutos = minutos / 60;

      if (minutos < 10) {
        uno = uno + String("0");
      }

      uno = uno + String(minutos) + String(":");

      int segundos = reloj / 1000;

      if (segundos >= 60) {
        segundos = segundos - (int)(segundos / 60) * 60;
      }

      if (segundos < 10) {
        uno = uno + String("0");
      }

      uno = uno + String(segundos) + String("hs");
    }
  }

  String dos = String("WIFI  ") + String("    --    ");
  String tres = String("Iluminacion ") + map(lightPWM, 0, 255, 0, 100) + String("%");

  renglones(uno, dos, tres, 3);
}

//**********************************************************************
//                   Imprime renglones menú display
//**********************************************************************
void renglones(String uno, String dos, String tres, int renglon) {

  //Sobrea primera línea
  if (renglon == 0) {
    display.clearBuffer();
    display.setFont(u8g2_font_7x13B_mr);
    display.drawBox(0, line0 - 10, displayRow + 7, 11);
    display.setCursor(0, line0);
    display.print(uno);
    display.setCursor(displayRow, line0);
    display.print(">");
    display.setFont(u8g2_font_8x13_mf);

    display.setCursor(0, line1);
    display.print(dos);

    display.setCursor(0, line2);
    display.print(tres);
    display.setCursor(displayRow, line2);
    display.print("v");
    display.sendBuffer();
  }
  //Sombrea última línea
  else if (renglon == 2) {
    display.clearBuffer();
    display.setCursor(0, line0);
    display.print(uno);
    display.setCursor(displayRow, line0 + 2);
    display.print("^");

    display.setCursor(0, line1);
    display.print(dos);

    display.setFont(u8g2_font_7x13B_mr);
    display.drawBox(0, line2 - 10, displayRow + 7, 11);
    display.setCursor(0, line2);
    display.print(tres);
    display.setCursor(displayRow, line2);
    display.print(">");
    display.setFont(u8g2_font_8x13_mf);
    display.sendBuffer();
  }
  //Sombrea línea intermedia
  else if (renglon == 1) {
    display.clearBuffer();
    display.setCursor(0, line0);
    display.print(uno);
    display.setCursor(displayRow, line0 + 2);
    display.print("^");

    display.setFont(u8g2_font_7x13B_mr);
    display.drawBox(0, line1 - 10, displayRow + 7, 11);
    display.setCursor(0, line1);
    display.print(dos);
    display.setCursor(displayRow, line1);
    display.print(">");
    display.setFont(u8g2_font_8x13_mf);

    display.setCursor(0, line2);
    display.print(tres);
    display.setCursor(displayRow, line2);
    display.print("v");
    display.sendBuffer();
  }
  //Sin sombreado
  else {
    display.clearBuffer();
    display.setCursor(0, line0);
    display.print(uno);

    display.setCursor(0, line1);
    display.print(dos);

    display.setCursor(0, line2);
    display.print(tres);
    display.sendBuffer();
  }
}
//**********************************************************************
//                   Imprime menú display primer nivel
//**********************************************************************
void displayLvl1(int cursorLvl1) {
  if (cursorLvl1 == 0) {
    String uno = menu[cursorLvl1][0];
    String dos = menu[cursorLvl1 + 1][0];
    String tres = menu[cursorLvl1 + 2][0];
    renglones(uno, dos, tres, 0);
  } else if (cursorLvl1 == menuLength - 1) {
    String uno = menu[cursorLvl1 - 2][0];
    String dos = menu[cursorLvl1 - 1][0];
    String tres = menu[cursorLvl1][0];
    renglones(uno, dos, tres, 2);
  } else {
    String uno = menu[cursorLvl1 - 1][0];
    String dos = menu[cursorLvl1][0];
    String tres = menu[cursorLvl1 + 1][0];
    renglones(uno, dos, tres, 1);
  }
}

//**********************************************************************
//                   Imprime menú display segundo nivel
//**********************************************************************

void displayLvl2(int cursorLvl1, int cursorLvl2) {
  if (cursorLvl2 == 1) {
    String uno = menu[cursorLvl1][cursorLvl2];
    String dos = menu[cursorLvl1][cursorLvl2 + 1];
    String tres = menu[cursorLvl1][cursorLvl2 + 2];
    renglones(uno, dos, tres, 0);
  } else if (cursorLvl2 == menuLength - 1 || menu[cursorLvl1][cursorLvl2] == "Volver") {
    String uno = menu[cursorLvl1][cursorLvl2 - 2];
    String dos = menu[cursorLvl1][cursorLvl2 - 1];
    String tres = menu[cursorLvl1][cursorLvl2];
    renglones(uno, dos, tres, 2);
  } else {
    String uno = menu[cursorLvl1][cursorLvl2 - 1];
    String dos = menu[cursorLvl1][cursorLvl2];
    String tres = menu[cursorLvl1][cursorLvl2 + 1];
    renglones(uno, dos, tres, 1);
  }
}

//**********************************************************************
//                   Imprime menú display tercer nivel
//**********************************************************************

void displayLvl3(int cursorLvl1, int cursorLvl2) {

  display.clearBuffer();
  display.setCursor(0, line0);
  display.print(menu[cursorLvl1][cursorLvl2]);
  display.setCursor(0, line1 + line0 / 2);
  display.print("<");
  display.setCursor(10, line1 + line0 / 2);


  //Temperatura seleccionada
  if (cursorLvl1 == 0 && cursorLvl2 == 1) {
    display.setCursor(50, line1 + line0 / 2);
    display.print((int)menuVal[cursorLvl1][cursorLvl2]);
    display.print("\xB0");  //Símbolo ° "grados" (ISO-8859-1)
    display.print(unidTempe[(int)menuVal[0][3]]);
  }
  //Riple
  else if (cursorLvl1 == 0 && cursorLvl2 == 2) {
    display.setCursor(50, line1 + line0 / 2);
    display.print("\xB1");  //Símbolo más menos (ISO-8859-1)
    display.print((int)menuVal[cursorLvl1][cursorLvl2]);
    display.print("\xB0");  //Símbolo ° "grados" (ISO-8859-1)
    display.print(unidTempe[(int)menuVal[0][3]]);
  }
  //Unidad temperatura
  else if (cursorLvl1 == 0 && cursorLvl2 == 3) {
    display.setCursor(50, line1 + line0 / 2);
    display.print("\xB0");  //Símbolo ° "grados"
    display.print(unidTempe[(int)menuVal[cursorLvl1][cursorLvl2]]);
  }
  //Modo reloj
  else if (cursorLvl1 == 1 && cursorLvl2 == 1) {
    display.print(modoReloj[(int)menuVal[cursorLvl1][cursorLvl2]]);

  }
  //Tiempo reloj
  else if (cursorLvl1 == 1 && cursorLvl2 == 2) {
    display.setCursor(50, line1 + line0 / 2);
    //milisegundos / 1000 = segundos
    //segundos / 60 = minutos
    //minutos / 60 = horas

    int horas = (int)(menuVal[cursorLvl1][cursorLvl2] / 1000 / 60 / 60);
    int minutos = menuVal[cursorLvl1][cursorLvl2] / 1000;

    if (minutos >= 3600) {
      minutos = minutos - (int)(minutos / 3600) * 3600;
    }

    minutos = minutos / 60;

    display.print(horas);
    display.print(":");
    if (minutos < 10) {
      display.print(0);
    }
    display.print(minutos);
    display.print(" Hs");

    display.setCursor(50, line0);
    if (flagTimer == HIGH) {
      display.print("+");
    }

  }
  //Reset
  else if (cursorLvl1 == 1 && cursorLvl2 == 3) {
    display.setCursor(50, line1 + line0 / 2);
    display.print(modoSINO[(int)menuVal[cursorLvl1][cursorLvl2]]);
  }
  //Modo ventilación
  else if (cursorLvl1 == 2 && cursorLvl2 == 1) {
    display.setCursor(20, line1 + line0 / 2);
    display.print(modoVenti[(int)menuVal[cursorLvl1][cursorLvl2]]);
  }
  //Funcion ventilación
  else if (cursorLvl1 == 2 && cursorLvl2 == 2) {
    display.setCursor(20, line1 + line0 / 2);
    display.print(funcVenti[(int)menuVal[cursorLvl1][cursorLvl2]]);
  }
  //Velocidad ventilación
  else if (cursorLvl1 == 2 && cursorLvl2 == 3) {
    display.setCursor(50, line1 + line0 / 2);
    display.print(map((int)menuVal[cursorLvl1][cursorLvl2], 0, 255, 0, 100));
    display.print("%");
  }
  //Modo iluminación
  else if (cursorLvl1 == 3 && cursorLvl2 == 1) {
    display.setCursor(50, line1 + line0 / 2);
    display.print(modoOFFON[(int)menuVal[cursorLvl1][cursorLvl2]]);
  }
  //Modo alarma
  else if (cursorLvl1 == 3 && cursorLvl2 == 2) {
    display.setCursor(50, line1 + line0 / 2);
    display.print(modoOFFON[(int)menuVal[cursorLvl1][cursorLvl2]]);
  }
  //Brillo
  else if (cursorLvl1 == 3 && cursorLvl2 == 3) {
    display.setCursor(50, line1 + line0 / 2);
    display.print(map((int)menuVal[cursorLvl1][cursorLvl2], 0, 255, 0, 100));
    display.print("%");
  }
  //Reinicio
  else if (cursorLvl1 == 5 && cursorLvl2 == 1) {
    display.setCursor(50, line1 + line0 / 2);
    display.print(modoSINO[(int)menuVal[cursorLvl1][cursorLvl2]]);
  }
  //Contraste
  else if (cursorLvl1 == 5 && cursorLvl2 == 3) {
    display.setCursor(50, line1 + line0 / 2);
    display.print(map((int)menuVal[cursorLvl1][cursorLvl2], 0, 255, 0, 100));
    display.print("%");
  } else {
    display.setCursor(50, line1 + line0 / 2);
    display.print((int)menuVal[cursorLvl1][cursorLvl2]);
  }

  display.setCursor(displayRow, line1 + line0 / 2);
  display.print(">");
  display.sendBuffer();
}

//**********************************************************************
//                              Coolers
//**********************************************************************


void progresivo(byte powerVal) {

  //Hasta un 30%
  if (powerVal < 76) {
    analogWrite(VT1, powerVal);
    analogWrite(VT2, 0);
  }
  //Hasta un 70%
  else if (powerVal < 187) {
    analogWrite(VT1, 0);
    analogWrite(VT2, powerVal);
  } else {
    analogWrite(VT1, powerVal);
    analogWrite(VT2, powerVal);
  }
}

void power(void) {

  //El error va de 0 a 255
  //0 = setpoint - riple
  //255 = setpont + riple
  //menuVal[0][0] = Temperatura medida
  //menuVal[0][1] = Temperatura seleccionada
  //menuVal[0][2] = riple

  byte error = 0;

  //Por debajo del riple
  //Temperatura medida <= Temperatura seleccionada - Ripple
  if ((menuVal[0][0] <= menuVal[0][1] - menuVal[0][2])) {
    error = 0;
  }
  //Por debajo del setpoint
  //Temperatura medida <= Temperatura seleccionada
  else if (menuVal[0][0] <= menuVal[0][1]) {
    //De 0 a 50% ( 0 a 127)
    //(riple - (Temperatura seleccionada - Temperatura medida))/riple)
    error = ((menuVal[0][2] - (menuVal[0][1] - menuVal[0][0])) / menuVal[0][2]) * 127;
    tiempoEncendido = millis();
  }
  //Por sobre el setpoint
  //Temperatura medida <= (Temperatura seleccionada + Ripple)
  else if (menuVal[0][0] <= (menuVal[0][1] + menuVal[0][2])) {
    //De 51 a 100% ( 128 a 255)
    error = ((menuVal[0][2] - (menuVal[0][1] - menuVal[0][0])) / menuVal[0][2]) * 127;
    tiempoEncendido = millis();
  }
  //Por sobre el riple
  //Temperatura seleccionada + Ripple <= Temperatura medida
  else {
    error = 255;
    tiempoEncendido = millis();
  }

  menuVal[2][3] = error;
}

void softPWM(int) {

  //se puede hacer uno general o por cada puerto?
}


//**********************************************************************
//                   Salida por terminal
//**********************************************************************
void printHorario(void) {
  Serial.print("⏩ ");
  Serial.print(cursorMenu);
  Serial.print(" ");
  Serial.print(cursorLvl1);
  Serial.print(" ");
  Serial.println(cursorLvl2);
}

void printAntihorario(void) {
  Serial.print("⏪ ");
  Serial.print(cursorMenu);
  Serial.print(" ");
  Serial.print(cursorLvl1);
  Serial.print(" ");
  Serial.println(cursorLvl2);
}

void printBoton(void) {
  Serial.print("■ ");
  Serial.print(cursorMenu);
  Serial.print(" ");
  Serial.print(cursorLvl1);
  Serial.print(" ");
  Serial.print(cursorLvl2);
  Serial.println(" ");
}
