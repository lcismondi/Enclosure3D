
#include <U8g2lib.h>
#include <DHT.h>
#include <EEPROM.h>

#define SW 14
#define DT 13
#define CLK 12

#define DHTPIN 0

#define DHTTYPE DHT22   // DHT 22  (AM2302), AM2321

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

U8G2_SSD1306_128X32_UNIVISION_F_HW_I2C display(U8G2_R0, /* reset=*/ 16, /* clock=*/ 5, /* data=*/ 4);
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
String currentDir = "";
unsigned long lastButtonPress = 0;
bool upClk = LOW;
bool downClk = LOW;
bool newBtn = LOW;

unsigned long displayTime = 0;          //Tiempo de rotador de pantalla
int cursorMenu = 0;                     //Indicador menú o carrusel
int cursorDisplay = 0;                  //Indicador de pantalla rotativa
int cursorLvl1 = 0;                     //Indicador menú general
int cursorLvl2 = 1;                     //Indicador submenú



//**********************************************************************
//                          Personalizaciones
//**********************************************************************
const int submenuLength = 5;
const String menu [][submenuLength] = {
  //CONVERTIR EN UN VECTOR DE VECTORES PARA ENTENDER EL TAMAÑO
  {"Temperatura", "Set point", "Ripple", "Volver"},
  {"Reloj",       "Modo", "Tiempo", "Volver"},
  {"Ventilacion", "Modo", "Funcion", "Velocidad", "Volver"},
  {"Iluminacion", "Modo", "Alarma", "Brillo", "Volver"},
  {"Wifi",        "Modo", "SSIS", "Nivel", "Volver"},
  {"Salir"}
};

const int menuLength = sizeof(menu) / sizeof(menu[0][0]) / submenuLength;

float menuVal[menuLength] [submenuLength] = {}; //SE PUEDE COMPLETAR CON LAS VARIABLES?
/*
   menuVal[0][0] = Temperatura medida
   menuVal[0][1] = Temperatura seleccionada
   menuVal[0][2] = Ripple
   menuVal[0][3] = Humedad medida
   menuVal[0][4] = -
   menuVal[1][0] = -
   menuVal[1][1] = Modo reloj (Temporizador/Cronometro/Off)
   menuVal[1][2] = Tiempo en milisegundos
   menuVal[1][3] = -
   menuVal[1][4] = -
   menuVal[2][0] = -
   menuVal[2][1] = Modo ventilacion (Manual/Automatico)
   menuVal[2][2] = Función ventilación (Progresivo/Simultáneo)
   menuVal[2][3] = Velocidad PWM
   menuVal[2][4] = -
   menuVal[3][0] = Modo iluminación (On/Off)
   menuVal[3][1] = Alarma iluminación (On/Off)
   menuVal[3][2] = Brillo PWM
   menuVal[3][3] = -
   menuVal[3][4] = -
   menuVal[4][0] = Modo WIFI (Scan/AP)
   menuVal[4][1] = SSID
   menuVal[4][2] = Porencia dBm
   menuVal[4][3] = -
*/

const String modoReloj[] = {"OFF", "Temporizador", "Cronometro"};
const String modoVenti[] = {"Manual", "Automatico"};
const String modoIlumi[] = {"OFF", "ON"};
const String modoAlarm[] = {"OFF", "ON"};
const String funcVenti[] = {"Progresivo", "Simultaneo"};


byte lightPWM = 0;          //Brillo de luz
int lightStep = 10;         //Pasos del dial de luz


//**********************************************************************
//                          Configuraciones
//**********************************************************************
int line0 = 10;             //Primer linea de texto display
int line1 = 21;             //Segunda linea de texto display
int line2 = 32;             //Tercera linea de texto display
int displayRow = 121;       //Flechas de navegación
int eeAddress = 100;          //Location we want the data to be put.


//**********************************************************************
//                          Prototipos
//**********************************************************************
void renglones (char, char, char, int);
void displayLvl1(int);
void displayLvl2(int, int);
void displayLvl3(int, int);
void printHorario (void);
void printAntihorario (void);
void printBoton (void);




void setup() {
  Serial.begin(115200);

  pinMode(CLK, INPUT_PULLUP);
  pinMode(DT, INPUT_PULLUP);
  pinMode(SW, INPUT_PULLUP);
  lastStateCLK = digitalRead(CLK);

  //pinMode(LED_BUILTIN, OUTPUT); VER EN QUÉ PIN ESTÁ CONECTADO

  display.begin();
  display.setPowerSave(0);
  display.setDrawColor(2);
  display.setFontMode(1);
  display.setFont(u8g2_font_8x13_mf);
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
  EEPROM.begin(4096);  //VER SI SE PUEDE EXPANDIR!!!

  delay(100);
  /*
    Serial.println("Cargando memoria: ");
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

    if (eeAddress >= 4096) {
      Serial.println("Advertencia error overflow memoria EEPROM");
    }
  */

  menuVal[0][1] = 45;       //Temperatura seleccionada

}

void loop() {

  //**********************************************************************
  //                          Control del cursor
  //**********************************************************************
  //Rotación

  currentStateCLK = digitalRead(CLK);
  currentStateDT = digitalRead(DT);

  if (currentStateCLK != lastStateCLK)
  {
    if (currentStateCLK == HIGH && currentStateDT == HIGH)
    {
      counter --;
      upClk = LOW;
      downClk = HIGH;

    }
    else if (currentStateCLK == HIGH && currentStateDT == LOW)
    {
      counter ++;
      upClk = HIGH;
      downClk = LOW;

    }
    
    Serial.print("(");
    Serial.print(counter);
    Serial.print(")");
    lastStateCLK = currentStateCLK;
  }





  /*
    currentStateCLK = digitalRead(CLK);

    // If last and current state of CLK are different, then pulse occurred
    // React to only 1 state change to avoid double count
    if (currentStateCLK != lastStateCLK  && currentStateCLK == 1) {


    // If the DT state is different than the CLK state then
    // the encoder is rotating CCW so increment
    if (digitalRead(DT) != currentStateCLK) {

      counter ++;

      upClk = HIGH;
      downClk = LOW;

    } else {
      // Encoder is rotating CW so decrement

      counter --;

      upClk = LOW;
      downClk = HIGH;

    }

    Serial.print("(");
    Serial.print(counter);
    Serial.print(")");
    }

    // Remember last CLK state
    lastStateCLK = currentStateCLK;
  */

  //**********************************************************************
  //                          Botón
  //**********************************************************************

  // Read the button state
  int btnState = digitalRead(SW);

  //If we detect LOW signal, button is pressed
  if (btnState == LOW) {
    //if 50ms have passed since last LOW pulse, it means that the
    //button has been pressed, released and pressed again
    if (millis() - lastButtonPress > 50) {

      newBtn = HIGH;

    }

    // Remember last button press event
    lastButtonPress = millis();
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

          menuVal[0][3] = dht.readHumidity();
          //Serial.println(menuVal[0][3]);
          menuVal[0][0] = dht.readTemperature();
          //Serial.println(t);

          String uno = String("Temperat. ") + String((int)menuVal[0][0]) + String("/") + String((int)menuVal[0][1]) + String("C");
          String dos = String("Humedad   ") + String((int)menuVal[0][3]) + String("%");
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
        printBoton();

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
          printHorario();
        }

      }
      else if (downClk == HIGH && upClk == LOW)
      {
        downClk = LOW;
        if (cursorLvl1 > 0)
        {
          cursorLvl1--;
          displayLvl1(cursorLvl1);
          printAntihorario();
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
          displayLvl1(cursorLvl1);
          //String uno = menu[cursorLvl1][0];
          //String dos = menu[cursorLvl1][1];
          //String tres = menu[cursorLvl1][2];
          //renglones(uno, dos, tres, 0);
          printBoton();
        }
        else
        {
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
      if (upClk == HIGH)
      {
        upClk = LOW;
        if (cursorLvl2 < submenuLength - 1)
        {
          if (menu[cursorLvl1][cursorLvl2] != "Volver")
          {
            cursorLvl2++;
            printHorario();
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
          printAntihorario();
        }


      }
      //Al presionar el botón entra al submenú
      if (newBtn == HIGH)
      {
        newBtn = LOW;

        if (menu[cursorLvl1][cursorLvl2] == "Volver")
        {
          cursorMenu--;
          cursorLvl2 = 1;           //El cursor de segundo nivel no puede ser menor a 1
          displayLvl1(cursorLvl1);
          printBoton();
        }
        else
        {
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
         menuVal[0][3] = Humedad medida
         menuVal[0][4] = -
         menuVal[1][0] = -
         menuVal[1][1] = Modo reloj (Temporizador/Cronometro/Off)
         menuVal[1][2] = Tiempo en milisegundos
         menuVal[1][3] = -
         menuVal[1][4] = -
         menuVal[2][0] = -
         menuVal[2][1] = Modo ventilacion (Manual/Automatico)
         menuVal[2][2] = Función ventilación (Progresivo/Simultáneo)
         menuVal[2][3] = Velocidad PWM
         menuVal[2][4] = -
         menuVal[3][0] = Modo iluminación (On/Off)
         menuVal[3][1] = Alarma iluminación (On/Off)
         menuVal[3][2] = Brillo PWM
         menuVal[3][3] = -
         menuVal[3][4] = -
         menuVal[4][0] = Modo WIFI (Scan/AP)
         menuVal[4][1] = SSID
         menuVal[4][2] = Porencia dBm
         menuVal[4][3] = -
      */

      //Al rotar el dial cambia el valor de configuración
      if (upClk == HIGH)
      {
        upClk = LOW;

        //Temperatura seleccionada
        if (cursorLvl1 == 0 && cursorLvl2 == 1)
        {
          if (menuVal[cursorLvl1][cursorLvl2] < 100)
          {
            menuVal[cursorLvl1][cursorLvl2]++;
          }
        }
        //Riple
        else if (cursorLvl1 == 0 && cursorLvl2 == 2)
        {
          if (menuVal[cursorLvl1][cursorLvl2] < 10)
          {
            menuVal[cursorLvl1][cursorLvl2]++;
          }
        }
        //Modo reloj
        else if (cursorLvl1 == 1 && cursorLvl2 == 1)
        {
          if (menuVal[cursorLvl1][cursorLvl2] < sizeof(modoReloj) / sizeof(modoReloj[0]) - 1)
          {
            //Serial.println(sizeof(modoReloj) / sizeof(modoReloj[0]));
            menuVal[cursorLvl1][cursorLvl2]++;
            //Serial.println(menuVal[cursorLvl1][cursorLvl2]);
          }
        }
        //Modo ventilación
        else if (cursorLvl1 == 2 && cursorLvl2 == 1)
        {
          if (menuVal[cursorLvl1][cursorLvl2] < sizeof(modoVenti) / sizeof(modoVenti[0]) - 1)
          {
            //Serial.println(sizeof(modoVenti) / sizeof(modoVenti[0]));
            menuVal[cursorLvl1][cursorLvl2]++;
            //Serial.println(menuVal[cursorLvl1][cursorLvl2]);
          }
        }
        //Funcion ventilación
        else if (cursorLvl1 == 2 && cursorLvl2 == 2)
        {
          if (menuVal[cursorLvl1][cursorLvl2] < sizeof(funcVenti) / sizeof(funcVenti[0]) - 1)
          {
            Serial.println(sizeof(funcVenti) / sizeof(funcVenti[0]));
            menuVal[cursorLvl1][cursorLvl2]++;
            Serial.println(menuVal[cursorLvl1][cursorLvl2]);
          }
        }
        //Modo iluminación
        else if (cursorLvl1 == 3 && cursorLvl2 == 0)
        {
          if (menuVal[cursorLvl1][cursorLvl2] < sizeof(modoIlumi) / sizeof(modoIlumi[0]) - 1)
          {
            menuVal[cursorLvl1][cursorLvl2]++;
          }
        }
        //Modo alarma
        else if (cursorLvl1 == 3 && cursorLvl2 == 1)
        {
          if (menuVal[cursorLvl1][cursorLvl2] < sizeof(modoAlarm) / sizeof(modoAlarm[0]) - 1)
          {
            menuVal[cursorLvl1][cursorLvl2]++;
          }
        }
        else
        {
          menuVal[cursorLvl1][cursorLvl2]++;
        }

        displayLvl3(cursorLvl1, cursorLvl2);
      }
      else if (downClk == HIGH)
      {
        downClk = LOW;

        //Temperatura seleccionada
        if (cursorLvl1 == 0 && cursorLvl2 == 1 ||
            //Ripple
            cursorLvl1 == 0 && cursorLvl2 == 2 ||
            //Modo reloj
            cursorLvl1 == 1 && cursorLvl2 == 1 ||
            //Modo ventilación
            cursorLvl1 == 2 && cursorLvl2 == 1 ||
            //Funcion ventilación
            cursorLvl1 == 2 && cursorLvl2 == 2 ||
            //Modo iluminación
            cursorLvl1 == 3 && cursorLvl2 == 0 ||
            //modo alarma
            cursorLvl1 == 3 && cursorLvl2 == 1 )
        {
          if (menuVal[cursorLvl1][cursorLvl2] > 0)
          {
            menuVal[cursorLvl1][cursorLvl2]--;
          }
        }
        else
        {
          menuVal[cursorLvl1][cursorLvl2]--;
        }

        displayLvl3(cursorLvl1, cursorLvl2);

      }

      //Al presionar el botón entra al submenú
      if (newBtn == HIGH)
      {
        newBtn = LOW;
        cursorMenu--;
        printBoton();
        displayLvl2(cursorLvl1, cursorLvl2);
        //Guardar en memoria el nuevo valor de la variable
        /*
          eeAddress = 100 + sizeof(float) * (cursorLvl1 * submenuLength + cursorLvl2);

          EEPROM.put(eeAddress, menuVal[cursorLvl1][cursorLvl2]);

          Serial.print(eeAddress);
          Serial.print(" ");
          Serial.print(cursorLvl1);
          Serial.print(" ");
          Serial.print(cursorLvl2);
          Serial.print(" ");
          Serial.println(menuVal[cursorLvl1][cursorLvl2]);

          Serial.println("");

          Serial.println("Cargando memoria: ");
          eeAddress = 100;
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
        */

      }

      break;

  }

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
  else if (renglon == 2)
  {
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
  else if (renglon == 1)
  {
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
  else if (cursorLvl2 == menuLength - 1 || menu[cursorLvl1][cursorLvl2] == "Volver")
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

//**********************************************************************
//                   Imprime menú display segundo nivel
//**********************************************************************

void displayLvl3(int cursorLvl1, int cursorLvl2)
{

  display.clearBuffer();
  display.setCursor(0, line0);
  display.print(menu[cursorLvl1][cursorLvl2]);

  //Modo reloj
  if (cursorLvl1 == 1 && cursorLvl2 == 1)
  {
    display.setCursor(10, line1 + line0 / 2);
    display.print(modoReloj[(int)menuVal[cursorLvl1][cursorLvl2]]);
  }
  //Modo ventilación
  else  if (cursorLvl1 == 2 && cursorLvl2 == 1)
  {
    display.setCursor(10, line1 + line0 / 2);
    display.print(modoVenti[(int)menuVal[cursorLvl1][cursorLvl2]]);
  }
  //Funcion ventilación
  else  if (cursorLvl1 == 2 && cursorLvl2 == 2)
  {
    display.setCursor(10, line1 + line0 / 2);
    display.print(funcVenti[(int)menuVal[cursorLvl1][cursorLvl2]]);
  }
  //Modo iluminación
  else if (cursorLvl1 == 3 && cursorLvl2 == 0)
  {
    display.setCursor(10, line1 + line0 / 2);
    display.print(modoIlumi[(int)menuVal[cursorLvl1][cursorLvl2]]);
  }

  //Modo alarma
  else  if (cursorLvl1 == 3 && cursorLvl2 == 1)
  {
    display.setCursor(10, line1 + line0 / 2);
    display.print(modoAlarm[(int)menuVal[cursorLvl1][cursorLvl2]]);
  }
  else
  {
    display.setCursor(50, line1 + line0 / 2);
    display.print((int)menuVal[cursorLvl1][cursorLvl2]);
  }

  display.sendBuffer();
}



//**********************************************************************
//                   Salida por terminal
//**********************************************************************
void printHorario (void)
{
  Serial.print("⏩ ");
  Serial.print(cursorMenu);
  Serial.print(" ");
  Serial.print(cursorLvl1);
  Serial.print(" ");
  Serial.println(cursorLvl2);
}

void printAntihorario (void)
{
  Serial.print("⏪ ");
  Serial.print(cursorMenu);
  Serial.print(" ");
  Serial.print(cursorLvl1);
  Serial.print(" ");
  Serial.println(cursorLvl2);
}

void printBoton (void)
{
  Serial.print("■ ");
  Serial.print(cursorMenu);
  Serial.print(" ");
  Serial.print(cursorLvl1);
  Serial.print(" ");
  Serial.print(cursorLvl2);
  Serial.println(" ");

}
