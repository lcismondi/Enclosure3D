
#include <U8g2lib.h>
#include <DHT.h>

#define ENCODER_BTN 14
#define ENCODER_DT  13
#define ENCODER_CLK 12


#define DHTPIN 0


/*  Utiliza el valor de GPIO
   D2   04  SDA (oled)
   D3   00  CS2
   D8   15  CS
   D7   13  MOSI
   D6   12  MISO
   SCL  14  CLK
   D0   16  RST (oled)
   A0       ADC

   CTS
   DTR
   D1   05
   TX   01
   RX   03
   RST
   SDA  02
   EN

*/


//U8X8_SSD1306_128X64_NONAME_HW_I2C display(U8X8_PIN_NONE, A5, A4);


U8G2_SSD1306_128X32_UNIVISION_F_HW_I2C display(U8G2_R0, /* reset=*/ 16, /* clock=*/ 5, /* data=*/ 4);
/*
  U8G2_R0  No rotation, landscape
  U8G2_R1 90 degree clockwise rotation
  U8G2_R2 180 degree clockwise rotation
  U8G2_R3 270 degree clockwise rotation
  U8G2_MIRROR No rotation, landscape, display content is mirrored (v2.6.x)
*/

// Uncomment whatever type you're using!
//#define DHTTYPE DHT11   // DHT 11
#define DHTTYPE DHT22   // DHT 22  (AM2302), AM2321
//#define DHTTYPE DHT21   // DHT 21 (AM2301)
DHT dht(DHTPIN, DHTTYPE);

int lastClk = HIGH;
int newClk = LOW;
bool upClk = LOW;
bool downClk = LOW;
int lastBtn = LOW;
bool newBtn = LOW;

unsigned long displayTime = 0;          //Tiempo de rotador de pantalla
int cursorMenu = 0;                     //Indicador menú o carrusel
int cursorDisplay = 0;                  //Indicador de pantalla rotativa
int cursorLvl1 = 0;                     //Indicador menú general
int cursorLvl2 = 1;                     //Indicador submenú

// Gamma table from https://learn.adafruit.com/led-tricks-gamma-correction/the-quick-fix
const uint8_t PROGMEM gamma8[] = {
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  1,  1,  1,
  1,  1,  1,  1,  1,  1,  1,  1,  1,  2,  2,  2,  2,  2,  2,  2,
  2,  3,  3,  3,  3,  3,  3,  3,  4,  4,  4,  4,  4,  5,  5,  5,
  5,  6,  6,  6,  6,  7,  7,  7,  7,  8,  8,  8,  9,  9,  9, 10,
  10, 10, 11, 11, 11, 12, 12, 13, 13, 13, 14, 14, 15, 15, 16, 16,
  17, 17, 18, 18, 19, 19, 20, 20, 21, 21, 22, 22, 23, 24, 24, 25,
  25, 26, 27, 27, 28, 29, 29, 30, 31, 32, 32, 33, 34, 35, 35, 36,
  37, 38, 39, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 50,
  51, 52, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 66, 67, 68,
  69, 70, 72, 73, 74, 75, 77, 78, 79, 81, 82, 83, 85, 86, 87, 89,
  90, 92, 93, 95, 96, 98, 99, 101, 102, 104, 105, 107, 109, 110, 112, 114,
  115, 117, 119, 120, 122, 124, 126, 127, 129, 131, 133, 135, 137, 138, 140, 142,
  144, 146, 148, 150, 152, 154, 156, 158, 160, 162, 164, 167, 169, 171, 173, 175,
  177, 180, 182, 184, 186, 189, 191, 193, 196, 198, 200, 203, 205, 208, 210, 213,
  215, 218, 220, 223, 225, 228, 231, 233, 236, 239, 241, 244, 247, 249, 252, 255
};
//analogWrite(LED_PIN, pgm_read_byte(&gamma8[i]));

//**********************************************************************
//                          Personalizaciones
//**********************************************************************
const int submenuLength = 5;
const String menu [][submenuLength] = {
  {"Temperatura", "Set point", "Ripple", "Volver"},
  {"Reloj",       "Modo", "Tiempo", "Volver"},
  {"Ventilacion", "Modo", "Función", "Velocidad", "Volver"},
  {"Iluminacion", "Modo", "Alarma", "Brillo", "Volver"},
  {"Wifi",        "Modo", "SSIS", "Señal", "Volver"},
  {"Salir"}
};
const int menuLength = sizeof(menu) / sizeof(menu[0][0]) / submenuLength;

byte lightPWM = 0;
int lightStep = 10;
float h = 0;
float t = 0;


//**********************************************************************
//                          Configuraciones
//**********************************************************************
int line0 = 10;  //Primer linea de texto display
int line1 = 21;  //Segunda linea de texto display
int line2 = 32;  //Tercera linea de texto display
int displayRow = 121;    //Flechas de navegación

//**********************************************************************
//                          Prototipos
//**********************************************************************
void renglones (char, char, char, int);
void displayLvl1(int);
void displayLvl2(int, int);


void setup() {
  Serial.begin(115200);
  pinMode(ENCODER_CLK, INPUT);
  pinMode(ENCODER_DT, INPUT);
  pinMode(ENCODER_BTN, INPUT_PULLUP);
  pinMode(LED_BUILTIN, OUTPUT);


  display.begin();
  display.setPowerSave(0);
  display.setDrawColor(1);
  display.setFontMode(0);
  display.setFont(u8g2_font_8x13_mf);
  //display.setFont(u8g2_font_unifont_t_cyrillic);

  dht.begin();

  display.clearBuffer();
  display.setCursor(2, line1);
  display.print("Iniciando...");
  //El display tiene 16 posiciones horizontales
  //display.print("1234567891123456");
  //display.print("ABCDEFGHIJKLMNOP");
  display.sendBuffer();
  Serial.println("");

  //**********************************************************************
  //                Cargar de memoria las configuraciones
  //**********************************************************************


}

void loop() {

  //**********************************************************************
  //                          Control del cursor
  //**********************************************************************
  //Rotación
  newClk = digitalRead(ENCODER_CLK);
  if (newClk != lastClk) {
    // There was a change on the CLK pin
    lastClk = newClk;
    int dtValue = digitalRead(ENCODER_DT);
    if (newClk == LOW && dtValue == HIGH) {
      Serial.print("⏩ ");
      Serial.print(cursorMenu);
      Serial.print(" ");
      Serial.print(cursorLvl1);
      Serial.print(" ");
      Serial.println(cursorLvl2);
      upClk = HIGH;
      downClk = LOW;
    }
    else if (newClk == LOW && dtValue == LOW) {
      Serial.print("⏪ ");
      Serial.print(cursorMenu);
      Serial.print(" ");
      Serial.print(cursorLvl1);
      Serial.print(" ");
      Serial.println(cursorLvl2);
      upClk = LOW;
      downClk = HIGH;
    }
  }

  //Pulsador
  if (digitalRead(ENCODER_BTN) == LOW) {
    if (lastBtn == LOW)
    {
      digitalWrite(LED_BUILTIN, HIGH);
      Serial.print("■ ");
      Serial.print(cursorMenu);
      Serial.print(" ");
      Serial.print(cursorLvl1);
      Serial.print(" ");
      Serial.print(cursorLvl2);
      Serial.println(" ");

      newBtn = HIGH;
      lastBtn = HIGH;
    }
  } else {
    digitalWrite(LED_BUILTIN, LOW);
    lastBtn = LOW;
  }


  //**********************************************************************
  //                          Display
  //**********************************************************************
  //Pantalla principal

  switch (cursorMenu) {
    case 0:
      //Rota la pantalla cada 3 segundos
      if (millis() - displayTime >= 3000)
      {
        displayTime = millis();
        if (cursorDisplay == 0)
        {
          cursorDisplay++;

          h = dht.readHumidity();
          //Serial.println(h);
          t = dht.readTemperature();
          //Serial.println(t);

          String uno = String("Temperat. ") + String((int)t) + String("/") + String("35") + String("C");
          String dos = String("Humedad   ") + String((int)h) + String("%");
          String tres = String("Velocidad ") + String("100") + String("%");

          renglones(uno, dos, tres, 3);

        }
        else if (cursorDisplay == 1)
        {
          cursorDisplay = 0;

          display2();

        }
      }

      //Al rotar el dial cambia la luz
      if (upClk == HIGH)
      {
        cursorDisplay = 1;
        displayTime = millis();
        upClk = LOW;
        if (lightPWM <= 255 - lightStep)
        {
          lightPWM = lightPWM + lightStep;
        }
        else if (lightPWM > 255 - lightStep)
        {
          lightPWM = 255;
        }
        display2();
      }
      else if (downClk == HIGH)
      {
        cursorDisplay = 1;
        displayTime = millis();
        downClk = LOW;
        if (lightPWM >= 0 + lightStep)
        {
          lightPWM = lightPWM - lightStep;
        }
        else if (lightPWM < 0 + lightStep)
        {
          lightPWM = 0;
        }
        display2();
      }

      //Al presionar el botón entra al menú
      if (newBtn == HIGH)
      {
        newBtn = LOW;
        cursorMenu++;
        //Falta condición de volver
        String uno = menu[cursorLvl1][0];
        String dos = menu[cursorLvl1 + 1][0];
        String tres = menu[cursorLvl1 + 2][0];
        renglones(uno, dos, tres, 0);


      }

      break;

    //Primer menú general
    case 1:

      //Al rotar el dial cambia el renglon
      if (upClk == HIGH && downClk == LOW)
      {
        upClk = LOW;
        if (cursorLvl1 < menuLength - 1)
        {
          cursorLvl1++;
          displayLvl1(cursorLvl1);
        }

      }
      else if (downClk == HIGH && upClk == LOW)
      {
        downClk = LOW;
        if (cursorLvl1 > 0)
        {
          cursorLvl1--;
          displayLvl1(cursorLvl1);
        }

      }
      //Al presionar el botón entra al submenú
      if (newBtn == HIGH)
      {
        newBtn = LOW;

        if (menu[cursorLvl1][0] == "Salir")
        {
          cursorMenu--;
          cursorLvl1 = 0;
          String uno = menu[cursorLvl1][0];
          String dos = menu[cursorLvl1][1];
          String tres = menu[cursorLvl1][2];
          renglones(uno, dos, tres, 0);
        }
        else
        {
          cursorMenu++;
          String uno = menu[cursorLvl1][cursorLvl2];
          String dos = menu[cursorLvl1][cursorLvl2 + 1];
          String tres = menu[cursorLvl1][cursorLvl2 + 2];
          renglones(uno, dos, tres, 0);
        }
      }

      break;

    //Opción submenú
    case 2:
      //Al rotar el dial cambia el renglon
      if (upClk == HIGH)
      {
        upClk = LOW;
        if (cursorLvl2 < submenuLength - 1)
        {
          if (menu[cursorLvl1][cursorLvl2] != "Volver")
          {
            cursorLvl2++;
          }
          displayLvl2(cursorLvl1, cursorLvl2);
        }
      }
      else if (downClk == HIGH)
      {
        downClk = LOW;
        if (cursorLvl2 > 1)
        {
          cursorLvl2--;

          displayLvl2(cursorLvl1, cursorLvl2);
        }


      }
      //Al presionar el botón entra al submenú
      if (newBtn == HIGH)
      {
        newBtn = LOW;

        if (menu[cursorLvl1][cursorLvl2] == "Volver")
        {
          cursorMenu--;
          cursorLvl2 = 0;
        }
        else
        {
          cursorMenu++;

        }
      }


      break;


    //Configuración de valores del submenú
    case 3:
      //Edición del submenú queda hardcodeado

      //Al presionar el botón entra al submenú
      if (newBtn == HIGH)
      {
        newBtn = LOW;
        cursorMenu--;

        //cursorMenu = 0;
        //cursorLvl1 = 0;
        //cursorLvl2 = 0;
        //Falta condición de volver
      }

      break;

  }

  //Menú


}

//**********************************************************************
//                Actualiza la segunda pantalla rotativa
//**********************************************************************
void display2 (void)
{

  String uno  = String("Tempo ") + String("00:35:21") + String("hs");
  String dos  = String("WIFI  ") + String("    --    ");
  String tres = String("Iluminacion ") + map(lightPWM, 0, 255, 0, 100) + String("%");

  renglones(uno, dos, tres, 3);

}

//**********************************************************************
//                   Imprime renglones menú display
//**********************************************************************
void renglones (String uno, String dos, String tres, int renglon)
{

  //Sobrea primera línea
  if (renglon == 0)
  {
    display.clearBuffer();
    display.setCursor(0, line0);
    display.print(uno);
    display.setCursor(displayRow, line0);
    display.print(">");

    display.setCursor(0, line1);
    display.print(dos);

    display.setCursor(0, line2);
    display.print(tres);
    display.setCursor(displayRow, line2);
    display.print("v");
    display.sendBuffer();
  }
  //Sombrea última línea
  else if (renglon == 2)
  {
    display.clearBuffer();
    display.setCursor(0, line0);
    display.print(uno);
    display.setCursor(displayRow, line0 + 2);
    display.print("^");

    display.setCursor(0, line1);
    display.print(dos);

    display.setCursor(0, line2);
    display.print(tres);
    display.setCursor(displayRow, line2);
    display.print(">");
    display.sendBuffer();
  }
  //Sombrea línea intermedia
  else if (renglon == 1)
  {
    display.clearBuffer();
    display.setCursor(0, line0);
    display.print(uno);
    display.setCursor(displayRow, line0 + 2);
    display.print("^");

    display.setCursor(0, line1);
    display.print(dos);
    display.setCursor(displayRow, line1);
    display.print(">");

    display.setCursor(0, line2);
    display.print(tres);
    display.setCursor(displayRow, line2);
    display.print("v");
    display.sendBuffer();
  }
  //Sin sombreado
  else
  {
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
void displayLvl1(int cursorLvl1)
{
  if (cursorLvl1 == 0)
  {
    String uno = menu[cursorLvl1][0];
    String dos = menu[cursorLvl1 + 1][0];
    String tres = menu[cursorLvl1 + 2][0];
    renglones(uno, dos, tres, 0);
  }
  else if (cursorLvl1 == menuLength - 1)
  {
    String uno = menu[cursorLvl1 - 2][0];
    String dos = menu[cursorLvl1 - 1][0];
    String tres = menu[cursorLvl1][0];
    renglones(uno, dos, tres, 2);
  }
  else
  {
    String uno = menu[cursorLvl1 - 1][0];
    String dos = menu[cursorLvl1][0];
    String tres = menu[cursorLvl1 + 1][0];
    renglones(uno, dos, tres, 1);
  }
}

//**********************************************************************
//                   Imprime menú display segundo nivel
//**********************************************************************

void displayLvl2(int cursorLvl1, int cursorLvl2)
{
  if (cursorLvl2 == 1)
  {
    String uno = menu[cursorLvl1][cursorLvl2];
    String dos = menu[cursorLvl1][cursorLvl2 + 1];
    String tres = menu[cursorLvl1][cursorLvl2 + 2];
    renglones(uno, dos, tres, 0);
  }
  else if (cursorLvl2 == menuLength - 1)
  {
    String uno = menu[cursorLvl1][cursorLvl2 - 2];
    String dos = menu[cursorLvl1][cursorLvl2 - 1];
    String tres = menu[cursorLvl1][cursorLvl2];
    renglones(uno, dos, tres, 2);
  }
  else
  {
    String uno = menu[cursorLvl1][cursorLvl2 - 1];
    String dos = menu[cursorLvl1][cursorLvl2];
    String tres = menu[cursorLvl1][cursorLvl2 + 1];
    renglones(uno, dos, tres, 1);
  }
}
