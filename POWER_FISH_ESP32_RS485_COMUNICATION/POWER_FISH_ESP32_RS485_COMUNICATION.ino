// #include <Servo.h> // incluye libreria de Servo
#include <EEPROM.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <ESP32_Servo.h>
#include <ESP.h>
#include <Wire.h>   // incluye libreria para interfaz I2C
#include <RTClib.h> // incluye libreria para el manejo del modulo RTC
#include <BluetoothSerial.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <HardwareSerial.h>
BluetoothSerial SerialBT;
RTC_DS3231 rtc;                     // crea objeto del tipo RTC_DS_3231
LiquidCrystal_I2C lcd(0x26, 16, 2); // set the LCD address to 0x27 for a 16 chars and 2 line displ
////////////////////*asignacion de servos*//////////////////////////////////////////////////////
Servo servo1, servo2, servo3, servo4;
///////////////////////**asignacion de nombres a los pines**//////////////////////
#define pinservo1 25
#define pinservo2 26
#define pinservo3 27
#define pinservo4 32
bool M1_available = true;
bool M2_available = true;
bool M3_available = true;
bool M4_available = true;
//////////////////////////////variables que nombraran los espacios de la eeprom///////////////
#define ultim_corrid 0
#define intervalos_H 2
#define TIPO_PEZ 6
#define suma_H 8
#define suma_M 10
#define M1_status 12
#define M2_status 14
#define M3_status 16
#define M4_status 18
#define battery_status 20
#define tiem_apertura 22
///////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////variables de los botones//////////
#define PIN_ABAJO 13
#define PIN_ENTER 12
#define PIN_ARRIBA 14
#define PIN_DISPENSE 4
#define BAT_PIN 34
#define buzzer 15
#define antirebote 100
const short VALOR_UP = 1;
const short VALOR_ENTER = 2;
const short VALOR_DOWN = 3;
unsigned short UP;
unsigned short IN;
unsigned short DOWN;
short resultado = 0;
short botones = 0;
///////////////////// variables de las posiciones de los diferentes menus////////
signed short posicion = 0;
/////////////////////variables del cambio de hora/////////////////////////
bool dispendio = false;
short milisegundos = 0;
short servo_enable = 1;
short aviso_cambio = 0;
String entradaSerial = ""; // String para almacenar entrada
String entrada_RS485_Serial = "";
String commandBT;
char buffer[20];
bool entradaCompleta = false; // Indicar si el String está completo
bool entrada_RS485_completa = false;
#define SERIAL_RS485_BAUDRATE 115200
#define RS485_DE_PIN 18
HardwareSerial RS485(2);  // Usamos el puerto serial 2 del ESP32 (GPIO17 - RX2, GPIO16 - TX2)
//////////////////////////////////////////////////////////////////
#define Pin_Sensor_Temp 5
OneWire oneWire(Pin_Sensor_Temp);
DallasTemperature TempSensor(&oneWire);
float T_MAX = 0.0;
float T_MIN = 0.0;
bool Temperature_alarm = false;
float Temperature=0.0;
int tiempo_apertura = 1000;
short intervalos_hora = 1;
short ultima_comida = 0;
short proxima_comida= 0;
short tipo_de_pez = 0;
short pos = 0;
short hora = 0;
short minutos = 0;
short suma_hora = 0;
short suma_minutos = 0;
const unsigned long BACKLIGHT_TIMEOUT = 1 * 60 * 1000; // 3 minutos en milisegundos
unsigned long lastActivityTime = 0;
float volt_bat = 0;
const float V_MIN = 3.3;
const float V_MAX = 4.2;
const float V_OPERATED = 3.6;
float voltage = 0;
#define VOLTAGE_MAX 4.2
#define VOLTAGE_MIN 3.4
#define ADC_MAX 3350
#define ADC_MIN 2900
short porcentaje_bateria = 0;
bool alarma = false;
bool battery_metter_available = false;
const float adcResolution = 3400.0; // Resolución del ADC
enum inicio{
  horas = 0,
  prox_comida,
  intervalo,
  tip_pez,
  cantidad,
  configuracion
};
enum config{
  config_hora = 8,
  config_intervalos,
  config_dispen,
  config_pez,
  config_motores,
  medidor_bat,
  salir
};
enum tipo_de_pez
{
  tilapia=1,
  tilapia_roja,
  cabeza_de_leon,
  salmon,
  tiburon_pangasio,
  koi,
  petra, 
};
///////////////////////////////////////////////////////////////////////
void setup()
{
  lcd.init();
  lcd.backlight();
  EEPROM.begin(4096);
  pinMode(BAT_PIN, INPUT);
  pinMode(PIN_ABAJO, INPUT);    // MODO SMALL
  pinMode(PIN_ENTER, INPUT);    // MODO MEDIUM
  pinMode(PIN_ARRIBA, INPUT);   // MODO BIG
  pinMode(PIN_DISPENSE, INPUT); // pin utilizado para el boton de desispendio manual
  pinMode(buzzer, OUTPUT);      // buzzer de alarma
  pinMode(RS485_DE_PIN, OUTPUT);
  digitalWrite(RS485_DE_PIN, LOW);
  battery_metter_available = EEPROM.read(battery_status);
  if (battery_metter_available == true)
  {
    volt_bat = analogRead(BAT_PIN);
    voltage = map(volt_bat += 40, ADC_MIN, ADC_MAX, VOLTAGE_MIN * 100, VOLTAGE_MAX * 100) / 100.0;
    bajo_voltage();
  }
  pinMode(pinservo1, OUTPUT);
  pinMode(pinservo2, OUTPUT);
  pinMode(pinservo3, OUTPUT);
  pinMode(pinservo4, OUTPUT);
  servo1.attach(pinservo1);
  servo2.attach(pinservo2);
  servo3.attach(pinservo3);
  servo4.attach(pinservo4);
  servo1.write(0); // inicia con la compuerta cerrada
  servo2.write(0); // inicia con la compuerta cerrada
  servo3.write(0); // inicia con la compuerta cerrada
  servo4.write(0); // inicia con la compuerta cerrada
  /////////////////////////pines declarados para salida y entrada////////////////////////
  ///////////////////////////////////////////////////////
  Serial.begin(115200);         // ESP-32
  RS485.begin(SERIAL_RS485_BAUDRATE, SERIAL_8N1, 16, 17);
  SerialBT.begin("POWER FISH"); // nombre del dispositivo Bluetooth EN EL ESP32
  Serial.println("Bluetooth iniciado");
  // Serial.begin(9600); arduino
  /////////////////////////////aviso del buzzer de que esta encendido el circuito//////////////////////////////////////
  if (!rtc.begin()){ // en caso de el no encontrar el modulo RTC sonara el buzzer y el circuito no iniciara
    for (int i = 0; i <= 6; i++)
    {
      digitalWrite(buzzer, HIGH);
      delay(50);
      digitalWrite(buzzer, LOW);
    }
    lcd.setCursor(0, 0);
    lcd.print("RTC NO ENCONTRADO");
    lcd.setCursor(0, 1);
    lcd.print("*PRESIONE RESET*");                // si falla la inicializacion del modulo
    Serial.println("Modulo RTC no encontrado !"); // muestra mensaje de error
    while (1); // bucle infinito que detiene ejecucion del programa
  }
  TempSensor.begin();
  TempSensor.requestTemperatures();
  Temperature= TempSensor.getTempCByIndex(0);
  if(Temperature==-127)
  {
    lcd.setCursor(1, 0);
    lcd.print("*SENSOR TEMP*");
    lcd.setCursor(1, 1);
    lcd.print("*DESCONECTADO*");
    delay(1500);
    lcd.clear();        
  }
  // rtc.adjust(DateTime(__DATE__, __TIME__)); // comentar al momento de compilar
  // rtc.adjust(DateTime(2022,6,5,8,59,0));
  DateTime fecha = rtc.now(); // funcion que devuelve fecha y horario
  hora = fecha.hour();
  minutos = fecha.minute();
  if (hora >= 9 && hora <= 17)
  {
    ultima_comida = hora;
    EEPROM.put(ultim_corrid, ultima_comida);
  }
  if (EEPROM.read(ultim_corrid) == 255)
  {
    ultima_comida = hora;
    EEPROM.put(0, ultima_comida);
  }
  ultima_comida = EEPROM.read(ultim_corrid); // recupera la ultima corrida en la memoria eeprom
  ////////////////////////bienvenida//////////////
  digitalWrite(buzzer, HIGH);
  delay(500);
  digitalWrite(buzzer, LOW);
  lcd.setCursor(1, 0);
  lcd.print("GONZALEZ TECH");
  delay(2000);
  lcd.setCursor(2, 1);
  lcd.print("*POWER FISH*");
  delay(2000);
  lcd.clear(); 
  /// lee las informaciones en la memoria eeprom
  tiempo_apertura = EEPROM.get(tiem_apertura, tiempo_apertura);
  intervalos_hora = EEPROM.read(intervalos_H);
  tipo_de_pez = EEPROM.read(TIPO_PEZ);
  M1_available = EEPROM.read(M1_status);
  M2_available = EEPROM.read(M2_status);
  M3_available = EEPROM.read(M3_status);
  M4_available = EEPROM.read(M4_status);
  switch (tipo_de_pez)
  {
  case tilapia:
    T_MAX=32.5;
    T_MIN=22.0;
  break;
  case tilapia_roja:
    T_MAX=32.5;
    T_MIN=22.0;
  break;
  case cabeza_de_leon:
    T_MAX=25.0;
    T_MIN=15.0;
  break;
  case tiburon_pangasio:
    T_MAX=30.0;
    T_MIN=23.0;
  break;  
  default:
    break;
  }
  /////////////////////////////////////////////
}
void loop()
{
  DateTime fecha = rtc.now(); // funcion que devuelve fecha y horario en formato
  botones = presionado();
  dispendio = digitalRead(PIN_DISPENSE);
  if (battery_metter_available == true)
  {
    volt_bat = analogRead(BAT_PIN);
    voltage = map(volt_bat += 40, ADC_MIN, ADC_MAX, VOLTAGE_MIN * 100, VOLTAGE_MAX * 100) / 100.0;
    bajo_voltage();
  }
  TempSensor.requestTemperatures();
  Temperature= TempSensor.getTempCByIndex(0);
  if(Temperature>T_MAX)
  {

      lcd.clear();
      lcd.setCursor(1,0);
      lcd.print("*TEMPERATURA*");
      lcd.setCursor(5,1);
      lcd.print("*ALTA*");
      digitalWrite(buzzer,HIGH);
      delay(1500);
      digitalWrite(buzzer,LOW);
      lcd.clear();
  }
  if(Temperature<T_MIN && Temperature>1.00)
  {
    lcd.clear();
    lcd.setCursor(1,0);
    lcd.print("*TEMPERATURA*");
    lcd.setCursor(5,1);
    lcd.print("*BAJA*");
    digitalWrite(buzzer,HIGH);
    delay(1500);
    digitalWrite(buzzer,LOW);
    lcd.clear();
  }   
  pantalla_principal();
  ///////////////////////////////////////////////////////////////////////////////////////////////////////////
  if (dispendio == true) // incia el proceso de alimentacion manualmente
  {
    servo_enable = 1;
    DISPENDIO(); // inicia el proceso de abrir la compuerta
  }
  if ((ultima_comida + intervalos_hora == hora && minutos == 1) || (hora == 9 && minutos == 0 && fecha.second() >= 0 && fecha.second() <= 2))
  {
    Serial.println("DISPENDIO");
    servo_enable = 1;
    ultima_comida = hora;
    EEPROM.put(ultim_corrid, ultima_comida);
    delay(20);
    DISPENDIO(); // inicia el proceso de abrir la compuerta
  }
  if (SerialBT.available())
  { // elimina los espacios en blanco al principio y al final de la cadena
    comandos_bluetooth();
  }
  if (entradaCompleta == HIGH) // comunicacion serial durant-e las horas de trabajo
  {
    Comando_serial();
  }
  if(entrada_RS485_completa==HIGH)
  {
    digitalWrite(RS485_DE_PIN,HIGH);
    comandos_Seriales_RS485();
  }
  while (hora < 9 || hora >= 17 && fecha.minute() >= 3) // while que desactiva la el circuito antes de la 9 AM y despues de las 6 PM
  {
    if (battery_metter_available == true)
    {
      bajo_voltage();
    }
    serialEvent();
    serialEvent_RS485();
    DateTime fecha = rtc.now();
    hora = fecha.hour();
    botones = presionado();
    TempSensor.requestTemperatures();
    Temperature= TempSensor.getTempCByIndex(0);
    if(Temperature>T_MAX)
    {
      lcd.clear();
      lcd.setCursor(1,0);
      lcd.print("*TEMPERATURA*");
      lcd.setCursor(5,1);
      lcd.print("*ALTA*");
      digitalWrite(buzzer,HIGH);
      delay(1500);
      digitalWrite(buzzer,LOW);
      lcd.clear();
    }
    if(Temperature<T_MIN && Temperature>1.00)
    {
      lcd.clear();
      lcd.setCursor(1,0);
      lcd.print("*TEMPERATURA*");
      lcd.setCursor(5,1);
      lcd.print("*BAJA*");
      digitalWrite(buzzer,HIGH);
      delay(1500);
      digitalWrite(buzzer,LOW);
      lcd.clear();
    }   
    pantalla_principal();
    dispendio = digitalRead(PIN_DISPENSE);
    if (dispendio == true) // incia el proceso de alimentacion manualm
    {
      servo_enable = 1;
      DISPENDIO(); // inicia el proceso de abrir la compuerta
    }
    if (entradaCompleta == HIGH)
    {
      Comando_serial();
    }
    else if (SerialBT.available())
    {                         // elimina los espacios en blanco al principio y al final de la cadena
      comandos_bluetooth();
    }
    else if(entrada_RS485_completa==HIGH)
    {
      digitalWrite(RS485_DE_PIN,HIGH);
      comandos_Seriales_RS485();
    }
  }
}
void bajo_voltage()
{
  if (voltage < V_MIN) // control de bajo voltaje
  {
    volt_bat = analogRead(BAT_PIN);
    voltage = map(volt_bat += 40, ADC_MIN, ADC_MAX, VOLTAGE_MIN * 100, VOLTAGE_MAX * 100) / 100.0;
    alarma = true;
    lcd.clear();
    while (alarma == true) // mientras la alarma esté activa y no hayan pasado más de 60 segundos
    {
      botones = presionado();
      volt_bat = analogRead(BAT_PIN);
      voltage = map(volt_bat += 40, ADC_MIN, ADC_MAX, VOLTAGE_MIN * 100, VOLTAGE_MAX * 100) / 100.0;
      lcd.setCursor(1, 0);
      lcd.print("*BATERIA BAJA*");
      lcd.setCursor(1, 1);
      lcd.print("PRESIONE RESET");
      delay(500);
      for (short i = 0; i > 2; i++)
      {
        digitalWrite(buzzer, HIGH);
        delay(750);
        digitalWrite(buzzer, LOW);
        delay(750);
      }
      if (botones == VALOR_ENTER)
      {
        delay(2500);
        alarma == false;
        battery_metter_available = false;
        EEPROM.put(battery_status, battery_metter_available);
        delay(antirebote);
        lcd.clear();
        return;
      }
      else if (voltage == V_OPERATED)
      {
        delay(250);
        alarma = false;
        lcd.clear();
        return;
      }
    }
  }
}
byte presionado()
{
  UP = digitalRead(PIN_ARRIBA);
  IN = digitalRead(PIN_ENTER);
  DOWN = digitalRead(PIN_ABAJO);
  short botones = 0;
  short resultado = 0;
  // variables asignadas para almacenar el valor de los pines
  if (UP == HIGH)
  {
    delay(150);
    resultado = VALOR_UP;
  }
  else if (IN == HIGH)
  {
    delay(150);
    resultado = VALOR_ENTER;
  }
  else if (DOWN == HIGH)
  {
    delay(150);
    resultado = VALOR_DOWN;
  }
  return (resultado);
}
void display_posicion(byte lower_posicion, byte upper_posicion)
{
  
  if (botones == VALOR_DOWN)
  {
    if (posicion < upper_posicion)
    {
      delay(antirebote);
      lcd.clear();
      posicion++;
      milisegundos = 0;
      lastActivityTime = millis(); // Reiniciar el temporizador
      lcd.setBacklight(HIGH);
    }
  }
  else if (botones == VALOR_UP)
  {
    if (posicion > lower_posicion)
    {
      delay(antirebote);
      lcd.clear();
      posicion--;
      milisegundos = 0;
      lastActivityTime = millis(); // Reiniciar el temporizador
      lcd.setBacklight(HIGH);
    }
  }
  else if (millis() - lastActivityTime >= BACKLIGHT_TIMEOUT)
  {
    lcd.setBacklight(LOW);
    if (posicion >= config_hora)
    {
      posicion = configuracion;
      lcd.clear();
    }
  }
}
void parametro_actualizado()
{
  lcd.clear();
  lcd.setCursor(4, 0);
  lcd.print("PARAMETRO");
  lcd.setCursor(3, 1);
  lcd.print("ACTUALIZADO");
  delay(2000);
  lcd.clear();
}
void DISPENDIO() // funcion de abrir y cerrar la compuerta
{
  lcd.clear();
  lcd.setCursor(2, 0);
  lcd.print("*DISPENSANDO*");
  lcd.setCursor(3, 1);
  lcd.print("*ALIMENTO*");
  /////////// avsio de que la compuerta abrira/////////////////////
  for (short i = 0; i < 2; i++) {
    digitalWrite(buzzer, HIGH);
    delay(750);
    digitalWrite(buzzer, LOW);
    delay(250);
  }
  //////////////////////////abre compuerta////////////////////
  if (servo_enable != 0)
  {
    for (pos = 0; pos <= 27; pos += 1)
    {
      if (M1_available == true)
      {
        servo1.write(pos);
      }
      if (M2_available == true)
      {
        servo2.write(pos);
      }
      if (M3_available == true)
      {
        servo3.write(pos);
      }
      if (M4_available == true)
      {
        servo4.write(pos);
      }
      delay(30);
    }
    delay(tiempo_apertura);
    //////////////////////////////cierra compuerta/////////////////////////
    for (pos = 27; pos >= 0; pos -= 1)
    {
      if (M1_available == true)
      {
        servo1.write(pos);
      }
      if (M2_available == true)
      {
        servo2.write(pos);
      }
      if (M3_available == true)
      {
        servo3.write(pos);
      }
      if (M4_available == true)
      {
        servo4.write(pos);
      }
      delay(30);
    }
    //////////////aviso de que la compuerta cerrara//////////////////////
    for (short i = 0; i < 2; i++) {
      digitalWrite(buzzer, HIGH);
      delay(750);
      digitalWrite(buzzer, LOW);
      delay(250);
    }
    lcd.clear();
    servo_enable = 0;
  }
}
void serialEvent() // funcion que se activa al recibir algo por el puerto serie////////////////////
{
  while (Serial.available())
  {
    // Obtener bytes de entrada:
    char inChar = (char)Serial.read();
    // Agregar al String de entrada:
    entradaSerial += inChar;
    // Para saber si el string está completo, se detendrá al recibir
    // el caracter de retorno de línea ENTER \n
    if (inChar == '\n')
    {
      entradaCompleta = true;
    }
  }
}
void serialEvent_RS485() // funcion que se activa al recibir algo por el puerto serie////////////////////
{
  while (RS485.available())
  {
    // Obtener bytes de entrada:
    char inChar = (char)RS485.read();
    // Agregar al String de entrada:
    entrada_RS485_Serial += inChar;
    // Para saber si el string está completo, se detendrá al recibir
    // el caracter de retorno de línea ENTER \n
    if (inChar == '\n')
    {
      entrada_RS485_completa = true;
    }
  }
}
void Comando_serial()
{
  DateTime fecha = rtc.now();      // funcion que devuelve fecha y horario
   if (entradaSerial == "estado\n")
  {
    Serial.printf("hora: %02d",hora);
    Serial.println();
    Serial.printf("minutos: %02d",minutos);
    Serial.println();
    Serial.printf("ultima comida fue: %02d",ultima_comida);
    Serial.println();
    Serial.printf("proxima comida a las: %02d",proxima_comida);
    Serial.println();    
    if (proxima_comida >= 17)
    {
      Serial.print("(fuera de horario)");
    }
    switch (tiempo_apertura)
    {
      case 45:Serial.print("DISPENDIO: pequeños");break;
      case 75:Serial.print("DISPENDIO: medianos");break;
      case 100:Serial.print("DISPENDIO: grandes");break;
      default:Serial.print("DISPENDIO: personalizado");break;
    }
    Serial.println();
    switch (tipo_de_pez)
    {
      case 1:Serial.print("TIPO DE PEZ: TILAPIA");break;
      case 2:Serial.print("TIPO DE PEZ: TILAPIA ROJA");break;
      case 3:Serial.print("TIPO DE PEZ: CABEZA DE LEON");break;
      case 4:Serial.print("TIPO DE PEZ: SALMON");break;
      case 5:Serial.print("TIPO DE PEZ: PANGASIO");break;
      case 6:Serial.print("TIPO DE PEZ: KOI");break;
      case 7:Serial.print("TIPO DE PEZ: PETRA");break;
      default:Serial.print("NO SELECCIONADO");break;
    }
    Serial.println();
    Serial.printf("voltaje:%d(%d%%)",voltage,porcentaje_bateria);
    SerialBT.println();    
    Serial.printf("Temperatura:%.2f'C",Temperature);
    if(Temperature==-127)Serial.print("(Sensor desconectado)");

  }
  else if (entradaSerial == "dispensa\n") // dispensa desde la pc
  {
    servo_enable = 1;
    DISPENDIO();
  }
  else if (entradaSerial == "prueba motores\n") // comando para probar que los motores funcionan bien
  {
    Serial.println("*probando motores*");
    prueba_motores();
  }
  else if (entradaSerial == "ajusta hora\n")
  {
    delay(50);
    rtc.adjust(DateTime(__DATE__, __TIME__));
  }
  else
  {
    Serial.print("NO SE RECONOCE EL COMANDO"); // en caso de que se ingrese comandos mal
  }
  entradaSerial = "";
  entradaCompleta = false;
}
void comandos_Seriales_RS485()
{
  DateTime fecha = rtc.now();      // función que devuelve fecha y horario
  if (entradaSerial == "estado\n")
  {
    RS485.printf("hora: %02d", hora);
    RS485.println();
    RS485.printf("minutos: %02d", minutos);
    RS485.println();
    RS485.printf("ultima comida fue: %02d", ultima_comida);
    RS485.println();
    RS485.printf("proxima comida a las: %02d", proxima_comida);
    RS485.println();
    if (proxima_comida >= 17)
    {
      RS485.print("(fuera de horario)");
    }
    switch (tiempo_apertura)
    {
      case 45: RS485.print("DISPENDIO: pequeños"); break;
      case 75: RS485.print("DISPENDIO: medianos"); break;
      case 100: RS485.print("DISPENDIO: grandes"); break;
      default: RS485.print("DISPENDIO: personalizado"); break;
    }
    RS485.println();
    switch (tipo_de_pez)
    {
      case 1: RS485.print("TIPO DE PEZ: TILAPIA"); break;
      case 2: RS485.print("TIPO DE PEZ: TILAPIA ROJA"); break;
      case 3: RS485.print("TIPO DE PEZ: CABEZA DE LEON"); break;
      case 4: RS485.print("TIPO DE PEZ: SALMON"); break;
      case 5: RS485.print("TIPO DE PEZ: PANGASIO"); break;
      case 6: RS485.print("TIPO DE PEZ: KOI"); break;
      case 7: RS485.print("TIPO DE PEZ: PETRA"); break;
      default: RS485.print("NO SELECCIONADO"); break;
    }
    RS485.println();
    RS485.printf("voltaje:%d(%d%%)", voltage, porcentaje_bateria);
    RS485.println();
    RS485.printf("Temperatura:%.2f'C", Temperature);
    if (Temperature == -127) RS485.print("(Sensor desconectado)");
  }
  else if (entradaSerial == "dispensa\n") // dispensa desde la pc
  {
    servo_enable = 1;
    DISPENDIO();
  }
  else if (entradaSerial == "prueba motores\n") // comando para probar que los motores funcionan bien
  {
    RS485.println("*probando motores*");
    prueba_motores();
  }
  else if (entradaSerial == "ajusta hora\n")
  {
    delay(50);
    rtc.adjust(DateTime(__DATE__, __TIME__));
  }
  else
  {
    RS485.print("NO SE RECONOCE EL COMANDO"); // en caso de que se ingrese comandos mal
  }
  entrada_RS485_Serial = "";
  entrada_RS485_completa = false;
  digitalWrite(RS485_DE_PIN,LOW);
}
void comandos_bluetooth()
{
  commandBT = SerialBT.readStringUntil('\n'); // lee la cadena hasta el caracter de nueva línea
  commandBT.trim();
  DateTime fecha = rtc.now(); // funcion que devuelve fecha y horario
  if (commandBT == "estado")
  {
    SerialBT.printf("hora: %02d",hora);
    SerialBT.println();
    SerialBT.printf("minutos: %02d",minutos);
    SerialBT.println();
    SerialBT.printf("ultima comida fue: %02d",ultima_comida);
    SerialBT.println();
    SerialBT.printf("proxima comida a las: %02d",proxima_comida);
    SerialBT.println();    
    if (proxima_comida >= 17)
    {
      SerialBT.print("(fuera de horario)");
    }
    switch (tiempo_apertura)
    {
      case 45:SerialBT.print("DISPENDIO: pequeños");break;
      case 75:SerialBT.print("DISPENDIO: medianos");break;
      case 100:SerialBT.print("DISPENDIO: grandes");break;
      default:SerialBT.print("DISPENDIO: personalizado");break;
    }
    SerialBT.println();
    switch (tipo_de_pez)
    {
      case 1:SerialBT.print("TIPO DE PEZ: TILAPIA");break;
      case 2:SerialBT.print("TIPO DE PEZ: TILAPIA ROJA");break;
      case 3:SerialBT.print("TIPO DE PEZ: CABEZA DE LEON");break;
      case 4:SerialBT.print("TIPO DE PEZ: SALMON");break;
      case 5:SerialBT.print("TIPO DE PEZ: PANGASIO");break;
      case 6:SerialBT.print("TIPO DE PEZ: KOI");break;
      case 7:SerialBT.print("TIPO DE PEZ: PETRA");break;
      default:SerialBT.print("NO SELECCIONADO");break;
    }
    SerialBT.println();
    SerialBT.printf("voltaje:%d(%d%%)",voltage,porcentaje_bateria);
    SerialBT.println();
    SerialBT.printf("Temperatura:%.2f'C",Temperature);
    if(Temperature==-127)SerialBT.print("(Sensor desconectado)");
  }
  else if (commandBT == "dispensa")
  {
    servo_enable = 1;
    SerialBT.println("DISPENSANDO ALIMENTO...");
    DISPENDIO();
    SerialBT.println("DISPENDIO FINALIZADO");
  }
  else if (commandBT == "prueba motores")
  {
    SerialBT.println("probando motores");
    servo_enable = 1;
    prueba_motores();
    SerialBT.println("probando motores finalizado");
  }
  else if (commandBT == "ajusta hora")
  {
    SerialBT.println("AJUSTANDO HORA");
    rtc.adjust(DateTime(__DATE__, __TIME__));
    SerialBT.println("HORA AJUSTADA");
  }
  else
  {
    SerialBT.println("Comando no reconocido");
  }
}
void prueba_motores()
{
  if (servo_enable != 0)
  {
    for (pos = 0; pos <= 180; pos += 1)
    {
      if (M1_available == true)
      {
        servo1.write(pos);
      }
      if (M2_available == true)
      {
        servo2.write(pos);
      }
      if (M3_available == true)
      {
        servo3.write(pos);
      }
      if (M4_available == true)
      {
        servo4.write(pos);
      }
      delay(40);
    }
    delay(tiempo_apertura);
    //////////////////////////////cierra compuerta/////////////////////////
    for (pos = 180; pos >= 0; pos -= 1)
    {
      if (M1_available == true)
      {
        servo1.write(pos);
      }
      if (M2_available == true)
      {
        servo2.write(pos);
      }
      if (M3_available == true)
      {
        servo3.write(pos);
      }
      if (M4_available == true)
      {
        servo4.write(pos);
      }
      delay(40);
    }
  }
}
void pantalla_principal()
{
  DateTime fecha = rtc.now();
  hora = fecha.hour();
  minutos = fecha.minute();
  botones = presionado();
  if (battery_metter_available == true)
  {
    volt_bat = analogRead(BAT_PIN);
    voltage = map(volt_bat += 40, ADC_MIN, ADC_MAX, VOLTAGE_MIN * 100, VOLTAGE_MAX * 100) / 100.0;
    porcentaje_bateria = short((voltage - 3.4) / (4.2 - 3.4) * 100);
  }
  switch (posicion)
  {
   case -1:
    posicion = horas;
    break;
   case horas:
    snprintf(buffer, sizeof(buffer), "HORA:%02d:%02d", fecha.hour(), minutos);
    lcd.setCursor(0, 0);
    lcd.print(buffer);
    if (battery_metter_available == true)
    {
      if (milisegundos >= 200)
      {
        lcd.setCursor(0, 1);
        lcd.printf("VBAT: %.2fV/%d%%", voltage, porcentaje_bateria);
        milisegundos = 0;
      }
    }
    if (hora < 9 || hora >= 17 && fecha.minute() >= 3)
    {
      lcd.setCursor(11, 0);
      lcd.print("(OUT)");
    }
    delay(1);
    milisegundos++;
    display_posicion(horas, configuracion);
    break;
   case prox_comida:
      lcd.setCursor(0, 0);
      lcd.printf("ULT. COMIDA: %d", ultima_comida);
      lcd.setCursor(0, 1);
      lcd.print("PROX.COMIDA:");
      lcd.setCursor(13,1);
      lcd.print(ultima_comida+intervalos_hora);
      display_posicion(horas, configuracion);
    break;
   case intervalo:
    EEPROM.read(intervalos_H);
    lcd.setCursor(0, 0);
    lcd.printf("INTERVALOS:C/%dH", intervalos_hora);
    lcd.setCursor(0, 1);
    lcd.printf("Temp:%.2f'C",Temperature);
    display_posicion(horas, configuracion);
    break;
   case tip_pez:
      EEPROM.read(TIPO_PEZ);
      lcd.setCursor(0, 0);
      lcd.print("TIPO DE PEZ:");
      lcd.setCursor(0, 1);
      switch (tipo_de_pez)
      {
        case tilapia:         lcd.print("TILAPIA"); break;
        case tilapia_roja:    lcd.print("TILAPIA ROJA");break;
        case cabeza_de_leon:  lcd.print("CABEZA DE LEON");break;
        case salmon:          lcd.print("SALMON");break;
        case tiburon_pangasio: lcd.print("PANGASIO");break;
        case koi :            lcd.print("KOI");break;
        case petra:           lcd.print("PETRA");break;
        default:              lcd.print("NO SELECCIONADO");break;
      }
      display_posicion(horas, configuracion);
    break;
   case cantidad:
    tiempo_apertura = EEPROM.get(tiem_apertura, tiempo_apertura);
    lcd.setCursor(0, 0);
    lcd.print("DISPENDIO PARA:");
    lcd.setCursor(0, 1);
    if      (tiempo_apertura == 45)  lcd.print("ALEVINE");
    else if (tiempo_apertura == 75)  lcd.print("JUVENILES");
    else if (tiempo_apertura == 100) lcd.print("ADULTOS");
    else
    {
      lcd.setCursor(0, 1);
      if (tiempo_apertura >= 1000)
        lcd.printf("TIEMPO:%-4.1fSEG", tiempo_apertura / 1000.0);
      else
        lcd.printf("%-8s%dMS", "TIEMPO:", tiempo_apertura);
    }
    display_posicion(horas, configuracion);
    break;
   case configuracion:
    lcd.setCursor(0, 0);
    lcd.print("CONFIGURACION");
    lcd.setCursor(0, 1);
    lcd.print("*PRESIONE ENTER*");
    if (botones == VALOR_ENTER)
    {
      lastActivityTime = millis(); // Reiniciar el temporizador
      lcd.setBacklight(HIGH);
      delay(antirebote);
      lcd.clear();
      pantalla_configuraciones();
    }
    display_posicion(horas, configuracion);
    break;
   case 6:
    posicion = configuracion;
    break;
  }
}
void pantalla_configuraciones()
{
  posicion = 7;
  while (posicion >= 7 && posicion <= 14)
  {
    dispendio = digitalRead(PIN_DISPENSE);
    botones = presionado();
    switch (posicion)
    {
    case 7:
      posicion = config_hora;
      break;
    case config_hora:
      lcd.setCursor(0, 0);
      lcd.print("*CONFIGURACION*");
      lcd.setCursor(0, 1);
      lcd.print("->HORA");
      if (botones == VALOR_ENTER)
      {
        delay(antirebote);
        lcd.clear();
        configuracion_hora();
      }
      display_posicion(config_hora, salir);
      break;
    case config_intervalos:
      lcd.setCursor(0, 0);
      lcd.print("*CONFIGURACION*");
      lcd.setCursor(0, 1);
      lcd.print("->INTERVALOS");
      if (botones == VALOR_ENTER)
      {
        delay(antirebote);
        lcd.clear();
        configuracion_intervalos();
      }
      display_posicion(config_hora, salir);
      break;
    case config_dispen:
      lcd.setCursor(0, 0);
      lcd.print("*CONFIGURACION*");
      lcd.setCursor(0, 1);
      lcd.print("->DISPENDIO");
      if (botones == VALOR_ENTER)
      {
        delay(antirebote);
        lcd.clear();
        configuracion_dispendio();
      }
      display_posicion(config_hora, salir);
      break;
    case config_pez:
      lcd.setCursor(0, 0);
      lcd.print("*CONFIGURACION*");
      lcd.setCursor(0, 1);
      lcd.print("->TIPO DE PEZ");
      if (botones == VALOR_ENTER)
      {
        delay(antirebote);
        lcd.clear();
        configuracion_tipo_pez();
      }
      display_posicion(config_hora, salir);
      break;
    case config_motores:
      lcd.setCursor(0, 0);
      lcd.print("*CONFIGURACION*");
      lcd.setCursor(0, 1);
      lcd.print("->MOTORES");
      if (botones == VALOR_ENTER)
      {
        delay(antirebote);
        lcd.clear();
        configuracion_motores();
      }
      else if (dispendio == true) // incia el proceso de alimentacion manualmente
      {
        servo_enable = 1;
        DISPENDIO(); // inicia el proceso de abrir la compuerta
      }
      display_posicion(config_hora, salir);
      break;
    case medidor_bat:
      lcd.setCursor(0, 0);
      lcd.print("*CONFIGURACION*");
      lcd.setCursor(0, 1);
      lcd.print("->MEDIDOR BAT.");
      if (botones == VALOR_ENTER)
      {
        delay(antirebote);
        lcd.clear();
        bat_metter();
      }
      display_posicion(config_hora, salir);
      break;
    case salir:
      lcd.setCursor(0, 0);
      lcd.print("*CONFIGURACION*");
      lcd.setCursor(0, 1);
      lcd.print("->SALIR");
      if (botones == VALOR_ENTER)
      {
        delay(antirebote);
        lcd.clear();
        posicion = configuracion;
        return;
      }
      display_posicion(config_hora, salir);
      break;
    }
  }
}
void configuracion_hora()
{
  posicion = 15;
  while (posicion >= 15)
  {
    delay(20);
    EEPROM.read(suma_H);
    EEPROM.read(suma_M);
    botones = presionado();
    switch (posicion)
    {
    case 15:
      lcd.setCursor(6, 0);
      lcd.print("HORA");
      lcd.setCursor(5, 1);
      lcd.printf("%02d:", suma_hora);
      if (botones == VALOR_UP)
      {
        delay(antirebote);
        suma_hora++;
      }

      else if (botones == VALOR_ENTER)
      {
        delay(antirebote);
        rtc.adjust(DateTime(2022, 6, 5, suma_hora, minutos, 0));
        EEPROM.put(suma_H, suma_hora);
        ultima_comida=suma_hora;
        posicion = 16;
      }

      else if (botones == VALOR_DOWN)
      {
        delay(antirebote);
        suma_hora--;
      }
      else if (suma_hora > 24)
      {
        suma_hora = 00;
      }
      else if (suma_hora < 0)
      {
        suma_hora = 23;
      }
      break;
    case 16: // cambia los minutos // guardar en la eeprom
      lcd.setCursor(6, 0);
      lcd.print("HORA");
      lcd.setCursor(5, 1);
      lcd.printf("%02d:%02d", suma_hora, suma_minutos);
      if (botones == VALOR_UP)
      {
        delay(antirebote);
        suma_minutos++;
      }
      else if (botones == VALOR_ENTER)
      {
        delay(antirebote);
        rtc.adjust(DateTime(2022, 6, 5, suma_hora, suma_minutos, 0));
        EEPROM.put(suma_M, suma_minutos);
        posicion = config_hora;
        parametro_actualizado();
        return;
      }
      else if (botones == VALOR_DOWN)
      {
        delay(antirebote);
        suma_minutos--;
      }
      if (suma_minutos > 59)
      {
        suma_minutos = 00;
      }
      else if (suma_minutos < 00)
      {
        suma_minutos = 59;
      }
      break;
    }
  }
}
void configuracion_intervalos()
{
  posicion = 17;
  while (posicion >= 17)
  {
    botones = presionado();
    lcd.setCursor(3, 0);
    lcd.print("INTERVALOS");
    lcd.setCursor(0, 1);
    lcd.printf("->%2d    (1-12)", intervalos_hora);
    if (botones == VALOR_UP)
    {
      delay(antirebote);
      intervalos_hora++;
    }
    else if (botones == VALOR_ENTER)
    {
      delay(antirebote);
      EEPROM.put(intervalos_H, intervalos_hora);
      EEPROM.commit();
      delay(50);
      parametro_actualizado();
      posicion = config_intervalos;
      return;
    }
    else if (botones == VALOR_DOWN)
    {
      delay(antirebote);
      intervalos_hora--;
    }
    if (intervalos_hora > 12)
    {
      intervalos_hora = 12;
    }
    else if (intervalos_hora <= 0)
    {
      intervalos_hora = 1;
    }
  }
}
void configuracion_dispendio()
{
  posicion = 18;
  EEPROM.read(tiem_apertura);
  while (posicion >= -18)
  {
    botones = presionado();
    switch (posicion)
    {
    case 18:
      posicion = 19;
      break;
    case 19:
      lcd.setCursor(0, 0);
      lcd.print("DISPENDIO PARA:");
      lcd.setCursor(0, 1);
      lcd.print("->ALEVINES");
      if (botones == VALOR_ENTER)
      {
        delay(antirebote);
        tiempo_apertura = 45;
        EEPROM.put(tiem_apertura, tiempo_apertura);
        EEPROM.commit();
        parametro_actualizado();
        posicion = config_dispen;
        return;
      }
      display_posicion(19, 22);
      break;
    case 20:
      lcd.setCursor(0, 0);
      lcd.print("DISPENDIO PARA:");
      lcd.setCursor(0, 1);
      lcd.print("->JUVENILES");
      if (botones == VALOR_ENTER)
      {
        delay(antirebote);
        tiempo_apertura = 75;
        EEPROM.put(tiem_apertura, tiempo_apertura);
        EEPROM.commit();
        parametro_actualizado();
        posicion = config_dispen;
        return;
      }
      display_posicion(19, 22);
      break;
    case 21:
      lcd.setCursor(0, 0);
      lcd.print("DISPENDIO PARA:");
      lcd.setCursor(0, 1);
      lcd.print("->ADULTOS");
      if (botones == VALOR_ENTER)
      {
        delay(antirebote);
        tiempo_apertura = 100;
        EEPROM.put(tiem_apertura, tiempo_apertura);
        EEPROM.commit();
        parametro_actualizado();
        posicion = config_dispen;
        return;
      }
      display_posicion(19, 22);
      break;
    case 22:
      lcd.setCursor(0, 0);
      lcd.print("DISPENDIO PARA:");
      lcd.setCursor(0, 1);
      lcd.print("->PERSONALIZADO");
      if (botones == VALOR_ENTER)
      {
        delay(antirebote);
        lcd.clear();
        posicion = 24;
      }
      display_posicion(19, 22);
      break;
    case 23:
      posicion = 22;
      break;
    case 24:
      lcd.setCursor(4, 0);
      lcd.print("DURACION");
      lcd.setCursor(0, 1);
      lcd.printf("->%2d (0-32.76)", tiempo_apertura);
      if (botones == VALOR_UP)
      {
        delay(50);
        tiempo_apertura += 20;
      }
      else if (botones == VALOR_ENTER)
      {
        delay(antirebote);
        EEPROM.put(tiem_apertura, tiempo_apertura);
        EEPROM.commit();
        delay(20);
        parametro_actualizado();
        posicion = config_dispen;
        return;
      }
      else if (botones == VALOR_DOWN)
      {
        delay(50);
        tiempo_apertura -= 20;
      }
      if (tiempo_apertura == -1)
      {
        lcd.clear();
        tiempo_apertura = 32760;
      }
      if (tiempo_apertura > 32760)
      {
        lcd.clear();
        tiempo_apertura = 0;
      }
      break;
    }
  }
}
void configuracion_tipo_pez()
{
  posicion = 25;
  while (posicion >= 25)
  {
    botones = presionado();
    switch (posicion)
    {
    case 25:
      posicion = 26;
      break;
    case 26:
      lcd.setCursor(0, 0);
      lcd.print("TIPO DE PEZ");
      lcd.setCursor(0, 1);
      lcd.print("->TILAPIA");
      if (botones == VALOR_ENTER)
      {
        delay(antirebote);
        tipo_de_pez = 1;
        EEPROM.put(TIPO_PEZ, tipo_de_pez);
        EEPROM.commit();
        T_MAX=32.5;
        T_MIN=22.0;
        posicion = config_pez;
        parametro_actualizado();
        return;
      }
      display_posicion(26, 30);
      break;
    case 27:
      lcd.setCursor(0, 0);
      lcd.print("TIPO DE PEZ");
      lcd.setCursor(0, 1);
      lcd.print("->TILAPIA ROJA");
      if (botones == VALOR_ENTER)
      {
        delay(antirebote);
        tipo_de_pez = 2;
        EEPROM.put(TIPO_PEZ, tipo_de_pez);
        EEPROM.commit();
        posicion = config_pez;
        T_MAX=32.5;
        T_MIN=22.0;
        parametro_actualizado();
        return;
      }
      display_posicion(26, 30);
      break;
    case 28:
      lcd.setCursor(0, 0);
      lcd.print("TIPO DE PEZ");
      lcd.setCursor(0, 1);
      lcd.print("->CAEBZA DE LEON");
      if (botones == VALOR_ENTER)
      {
        delay(antirebote);
        tipo_de_pez = 3;
        EEPROM.put(TIPO_PEZ, tipo_de_pez);
        EEPROM.commit();
        T_MAX=25.0;
        T_MIN=15.0;
        posicion = config_pez;
        parametro_actualizado();
        return;
      }
      display_posicion(26, 30);
      break;
    case 29:
      lcd.setCursor(0, 0);
      lcd.print("TIPO DE PEZ");
      lcd.setCursor(0, 1);
      lcd.print("->SALMON");
      if (botones == VALOR_ENTER)
      {
        delay(antirebote);
        tipo_de_pez = 4;
        EEPROM.put(TIPO_PEZ, tipo_de_pez);
        EEPROM.commit();
        posicion = config_pez;
        parametro_actualizado();
        return;
      }
      display_posicion(26, 30);
      break;
    case 30:
      lcd.setCursor(0, 0);
      lcd.print("TIPO DE PEZ");
      lcd.setCursor(0, 1);
      lcd.print("->PANGASIO");
      if (botones == VALOR_ENTER)
      {
        delay(antirebote);
        tipo_de_pez = 5;
        EEPROM.put(TIPO_PEZ, tipo_de_pez);
        EEPROM.commit();
        T_MAX=30.0;
        T_MIN=23.0;
        posicion = config_pez;
        parametro_actualizado();
        return;
      }
      display_posicion(26, 30);
      break;
    case 5:
      posicion = 30;
      break;
    }
  }
}
void configuracion_motores()
{
  posicion = 31;
  while (posicion >= 31)
  {
    botones = presionado();
    switch (posicion)
    {
    case 31:
      posicion = 32;
      break;
    case 32:
      delay(antirebote);
      lcd.setCursor(2, 0);
      lcd.print("*CONFIGURAR*");
      lcd.setCursor(0, 1);
      lcd.print("->M1");
      if (botones == VALOR_ENTER)
      {
        delay(antirebote);
        lcd.clear();
        posicion = 38;
      }
      display_posicion(32, 51);
      break;
    case 33:
      lcd.setCursor(2, 0);
      lcd.print("*CONFIGURAR*");
      lcd.setCursor(0, 1);
      lcd.print("->M2");
      if (botones == VALOR_ENTER)
      {
        delay(antirebote);
        lcd.clear();
        posicion = 42;
      }
      display_posicion(32, 51);
      break;
    case 34:
      lcd.setCursor(2, 0);
      lcd.print("*CONFIGURAR*");
      lcd.setCursor(0, 1);
      lcd.print("->M3");
      if (botones == VALOR_ENTER)
      {
        delay(antirebote);
        lcd.clear();
        posicion = 46;
      }
      display_posicion(32, 51);
      break;
    case 35:
      lcd.setCursor(2, 0);
      lcd.print("*CONFIGURAR*");
      lcd.setCursor(0, 1);
      lcd.print("->M4");
      if (botones == VALOR_ENTER)
      {
        delay(antirebote);
        lcd.clear();
        posicion = 50;
      }
      display_posicion(32, 51);
      break;
    case 36:
      posicion = 35;
      break;
    ///////////////M1////////////////////////
    case 37:
      posicion = 38;
      break;
    case 38:
      lcd.setCursor(6, 0);
      lcd.print("*M1*");
      lcd.setCursor(0, 1);
      lcd.print("->ACTIVAR");
      if (botones == VALOR_ENTER)
      {
        delay(antirebote);
        M1_available = true;
        EEPROM.put(M1_status, M1_available);

        EEPROM.commit();
        parametro_actualizado();
        posicion = config_motores;
        return;
      }
      display_posicion(32, 51);
      break;
    case 39:
      lcd.setCursor(6, 0);
      lcd.print("*M1*");
      lcd.setCursor(0, 1);
      lcd.print("->DESACTIVAR");
      if (botones == VALOR_ENTER)
      {
        delay(antirebote);
        M1_available = false;
        EEPROM.put(M1_status, M1_available);
        EEPROM.commit();
        parametro_actualizado();
        posicion = config_motores;
        return;
      }
      display_posicion(32, 51);
      break;
    case 40:
      posicion = 39;
      break;
    ///////////////M2////////////////////
    case 41:
      posicion = 42;
      break;
    case 42:
      lcd.setCursor(6, 0);
      lcd.print("*M2*");
      lcd.setCursor(0, 1);
      lcd.print("->ACTIVAR");
      if (botones == VALOR_ENTER)
      {
        delay(antirebote);
        M2_available = true;
        EEPROM.put(M2_status, M2_available);
        EEPROM.commit();
        parametro_actualizado();
        posicion = config_motores;
        return;
      }
      display_posicion(32, 51);
      break;
    case 43:
      lcd.setCursor(6, 0);
      lcd.print("*M2*");
      lcd.setCursor(0, 1);
      lcd.print("->DESACTIVAR");
      if (botones == VALOR_ENTER)
      {
        delay(antirebote);
        M2_available = false;
        EEPROM.put(M2_status, M2_available);
        EEPROM.commit();
        parametro_actualizado();
        posicion = config_motores;
        return;
      }
      display_posicion(32, 51);
      break;
    case 44:
      posicion = 43;
      break;
      /////////////////M3///////////////////////////
    case 45:
      posicion = 46;
      break;
    case 46:
      lcd.setCursor(6, 0);
      lcd.print("*M3*");
      lcd.setCursor(0, 1);
      lcd.print("->ACTIVAR");
      if (botones == VALOR_ENTER)
      {
        delay(antirebote);
        M3_available = true;
        EEPROM.put(M3_status, M3_available);
        EEPROM.commit();
        parametro_actualizado();
        posicion = config_motores;
        return;
      }
      display_posicion(32, 51);
      break;
    case 47:
      lcd.setCursor(6, 0);
      lcd.print("*M3*");
      lcd.setCursor(0, 1);
      lcd.print("->DESACTIVAR");
      if (botones == VALOR_ENTER)
      {
        delay(antirebote);
        M3_available = false;
        EEPROM.put(M3_status, M3_available);
        EEPROM.commit();
        parametro_actualizado();
        posicion = config_motores;
        return;
      }
      display_posicion(32, 51);
      break;
    case 48:
      posicion = 47;
      break;
    ////////////M4///////////////////
    case 49:
      posicion = 50;
      break;
    case 50:
      lcd.setCursor(6, 0);
      lcd.print("*M4*");
      lcd.setCursor(0, 1);
      lcd.print("->ACTIVAR");
      if (botones == VALOR_ENTER)
      {
        delay(antirebote);
        M4_available = true;
        EEPROM.put(M4_status, M4_available);
        EEPROM.commit();
        parametro_actualizado();
        posicion = config_motores;
        return;
      }
      display_posicion(32, 51);
      break;
    case 51:
      lcd.setCursor(6, 0);
      lcd.print("*M4*");
      lcd.setCursor(0, 1);
      lcd.print("->DESACTIVAR");
      if (botones == VALOR_ENTER)
      {
        delay(antirebote);
        M4_available = false;
        EEPROM.put(M4_status, M4_available);
        EEPROM.commit();
        parametro_actualizado();
        posicion = config_motores;
        return;
      }
      display_posicion(32, 51);
      break;
    }
  }
}
void bat_metter()
{
  posicion = 52;
  while (posicion >= 52)
  {
    botones = presionado();
    switch (posicion)
    {
    case 52:
      posicion = 53;
      break;
    case 53:
      lcd.setCursor(0, 0);
      lcd.print("MEDIDOR BATERIA");
      lcd.setCursor(0, 1);
      lcd.print("->ACTIVAR");
      if (botones == VALOR_ENTER)
      {
        delay(antirebote);
        battery_metter_available = true;
        EEPROM.put(battery_status, battery_metter_available);
        EEPROM.commit();
        parametro_actualizado();
        delay(50);
        ESP.restart();
      }
      if (botones == VALOR_DOWN)
      {
        delay(antirebote);
        lcd.clear();
        posicion++;
      }
      break;
    case 54:
      lcd.setCursor(0, 0);
      lcd.print("MEDIDOR BATERIA");
      lcd.setCursor(0, 1);
      lcd.print("->DESACTIVAR");
      if (botones == VALOR_ENTER)
      {
        delay(antirebote);
        battery_metter_available = false;
        EEPROM.put(battery_status, battery_metter_available);
        EEPROM.commit();
        parametro_actualizado();
        delay(50);
        ESP.restart();
      }
      else if (botones == VALOR_UP)
      {
        delay(antirebote);
        posicion--;
      }
      break;
    }
  }
}