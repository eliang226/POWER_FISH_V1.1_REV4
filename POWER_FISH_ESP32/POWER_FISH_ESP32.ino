//#include <Servo.h> // incluye libreria de Servo
#include <EEPROM.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <ESP32_Servo.h> 
#include <ESP.h>
#include <Wire.h>   // incluye libreria para interfaz I2C
#include <RTClib.h> // incluye libreria para el manejo del modulo RTC
#include <BluetoothSerial.h>
BluetoothSerial SerialBT;
RTC_DS3231 rtc;     // crea objeto del tipo RTC_DS_3231
LiquidCrystal_I2C lcd(0x26, 16, 2); // set the LCD address to 0x27 for a 16 chars and 2 line displ
////////////////////*asignacion de servos*//////////////////////////////////////////////////////
Servo servo1, servo2, servo3, servo4; 
///////////////////////**asignacion de nombres a los pines**//////////////////////
#define pinservo1  25 
#define pinservo2  26
#define pinservo3  27
#define pinservo4  32 
bool M1_available = true;
bool M2_available = true;
bool M3_available = true;
bool M4_available = true;
//////////////////////////////variables que nombraran los espacios de la eeprom///////////////
byte ultim_corrid=0;
byte intervalos_H=2;
byte tiem_apertura=4;
byte TIPO_PEZ=6;
byte suma_H=8;
byte suma_M=10;
byte M1_status=12;
byte M2_status=14;
byte M3_status=16;
byte M4_status=18;
byte battery_status=20;

///////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////variables de los botones//////////
const byte VALOR_UP=1;
const byte VALOR_ENTER=2;
const byte VALOR_DOWN=3;
#define PIN_ABAJO 13
#define PIN_ENTER 12
#define PIN_ARRIBA 14
#define PIN_DISPENSE 4
#define BAT_PIN 34
#define buzzer  15 
unsigned short UP;
unsigned short IN;
unsigned short DOWN;
short resultado=0;
short botones=0;
///////////////////// variables de las posiciones de los diferentes menus////////
signed short posicion=0;
/////////////////////variables del cambio de hora/////////////////////////
short dispendio = 0;
short milisegundos=0;
byte servo_enable = 1;
byte aviso_cambio = 0;
String entradaSerial = "";    // String para almacenar entrada
String commandBT;
bool entradaCompleta = false; // Indicar si el String está completo
//////////////////////////////////////////////////////////////////
int16_t tiempo_apertura = 500;
byte intervalos_hora = 1;
byte ultima_corrida = 0;
byte tipo_de_pez=0;
short pos = 0;
byte hora = 0;
byte minutos=0;
byte suma_hora=0;
byte suma_minutos=0;
float volt_bat=0;
const float  V_MIN=3.3;
const float  V_MAX=4.2;
const float  V_OPERATED=3.6;
float voltage=0;
#define VOLTAGE_MAX 4.2
#define VOLTAGE_MIN 3.4
#define ADC_MAX 3350
#define ADC_MIN 2900
short porcentaje_bateria=0;
bool alarma=false;
bool battery_metter_available=false;
const float adcResolution = 3400.0; // Resolución del ADC
enum inicio {horas=0,prox_comida,intervalo,tip_pez,tamano,configuracion};
enum config {config_hora=8,config_intervalos,config_dispen,config_pez,config_motores,medidor_bat,salir};
///////////////////////////////////////////////////////////////////////
void setup()
{ 
  lcd.init();
  lcd.backlight();
  EEPROM.begin(512);
  pinMode(BAT_PIN,INPUT);
  pinMode(PIN_ABAJO, INPUT);      // MODO SMALL
  pinMode(PIN_ENTER, INPUT);      // MODO MEDIUM
  pinMode(PIN_ARRIBA, INPUT);      // MODO BIG
  pinMode(PIN_DISPENSE, INPUT);      // pin utilizado para el boton de desispendio manual
  pinMode(buzzer, OUTPUT); // buzzer de alarma
  battery_metter_available=EEPROM.read(battery_status);
  delay(20);
  if(battery_metter_available==true){
    volt_bat=analogRead(BAT_PIN);
    voltage = map(volt_bat+=40, ADC_MIN, ADC_MAX, VOLTAGE_MIN * 100, VOLTAGE_MAX * 100) / 100.0;
    bajo_voltage();
  }
  pinMode(pinservo1,OUTPUT);
  pinMode(pinservo2,OUTPUT);
  pinMode(pinservo3,OUTPUT);
  pinMode(pinservo4,OUTPUT);
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
  Serial.begin(115200); //ESP-32
  SerialBT.begin("POWER FISH"); // nombre del dispositivo Bluetooth EN EL ESP32
  Serial.println("Bluetooth iniciado");
  //Serial.begin(9600); arduino 
  /////////////////INICIALIZACION de los servos ////////////////////////////////
  ///////////////////////////////////////////////////////////////////
  /////////////////////////////aviso del buzzer de que esta encendido el circuito//////////////////////////////////////
  if (!rtc.begin())
  { // en caso de el no encontrar el modulo RTC sonara el buzzer y el circuito no iniciara
    for (int i = 0; i <= 6; i++)
    {
      digitalWrite(buzzer, HIGH);
      delay(50);
      digitalWrite(buzzer, LOW);
    }
    lcd.setCursor(0,0);
    lcd.print("RTC NO ENCONTRADO");
    lcd.setCursor(0,1);
    lcd.print("*PRESIONE RESET*");                                             // si falla la inicializacion del modulo
    Serial.println("Modulo RTC no encontrado !"); // muestra mensaje de error
    while (1); // bucle infinito que detiene ejecucion del programa
  }
  //rtc.adjust(DateTime(__DATE__, __TIME__)); // comentar al momento de compilar
  //rtc.adjust(DateTime(2022,6,5,8,59,0));
  DateTime fecha = rtc.now(); // funcion que devuelve fecha y horario 
  hora = fecha.hour();
  minutos=fecha.minute();
  if(hora>=9 && hora<=17)
  {
    ultima_corrida=hora;
    EEPROM.write(ultim_corrid,ultima_corrida);
  }
  if (EEPROM.read(ultim_corrid)==255)
  {
    ultima_corrida=hora;
    EEPROM.write(0,ultima_corrida);
  }
  ultima_corrida = EEPROM.get(ultim_corrid,ultima_corrida); // recupera la ultima corrida en la memoria eeprom 
  ///////////////////////////////////bienvenida////////////// 
  digitalWrite(buzzer, HIGH);
  delay(500);
  digitalWrite(buzzer,LOW);
	lcd.setCursor(1,0);
	lcd.print("GONZALEZ TECH");
	delay(2000);
	lcd.setCursor(2,1);
	lcd.print("*POWER FISH*");
	delay(2000);
	lcd.clear();
  /// lee las informaciones en la memoria eeprom 
  EEPROM.read(tiem_apertura);
  delay(100);
  EEPROM.read(TIPO_PEZ);
  delay(100);
  EEPROM.read(intervalos_H);
  EEPROM.read(M1_status);
  EEPROM.read(M2_status);
  EEPROM.read(M3_status);
  EEPROM.read(M4_status);
  /////////////////////////////////////////////
}
void loop()
{
  DateTime fecha = rtc.now(); // funcion que devuelve fecha y horario en formato
  botones=presionado();
  dispendio = digitalRead(PIN_DISPENSE);
  if(battery_metter_available==true) {
    volt_bat=analogRead(BAT_PIN);
    voltage = map(volt_bat+=40, ADC_MIN, ADC_MAX, VOLTAGE_MIN * 100, VOLTAGE_MAX * 100) / 100.0;
    bajo_voltage();
  }
  pantalla_principal();
  ///////////////////////////////////////////////////////////////////////////////////////////////////////////
  if (dispendio == HIGH) //incia el proceso de alimentacion manualmente
  {
    servo_enable = 1;
    DISPENDIO(); // inicia el proceso de abrir la compuerta 
  }
  if ((ultima_corrida + intervalos_hora == hora && fecha.minute() == 1) || (fecha.hour()==9 && fecha.minute()==0 && fecha.second()>0 && fecha.second()<2 )){
    Serial.println("funcione");
    servo_enable = 1;
    DISPENDIO(); // inicia el proceso de abrir la compuerta 
    ultima_corrida = hora;
    EEPROM.write(ultim_corrid,ultima_corrida);
  }
  if (SerialBT.available()) {
    commandBT = SerialBT.readStringUntil('\n'); // lee la cadena hasta el caracter de nueva línea
    commandBT.trim(); // elimina los espacios en blanco al principio y al final de la cadena
    comandos_bluetooth();
  }
  if (entradaCompleta == HIGH) // comunicacion serial durante las horas de trabajo 
  {
    Comando_serial();
  }
  while (hora < 9 || hora >= 17 && fecha.minute() >= 3) // while que desactiva la el circuito antes de la 9 AM y despues de las 6 PM
  {
    if (battery_metter_available==true){
    bajo_voltage();
    }
    DateTime fecha = rtc.now();
    hora = fecha.hour();
    botones=presionado();
    pantalla_principal();
    dispendio=digitalRead(PIN_DISPENSE);
    if (dispendio == HIGH) //incia el proceso de alimentacion manualmente
    {
      servo_enable = 1;
      DISPENDIO(); // inicia el proceso de abrir la compuerta 
    }
    if (entradaCompleta == HIGH)
    {
      Comando_serial();
    }
    if (SerialBT.available()) {
    commandBT = SerialBT.readStringUntil('\n'); // lee la cadena hasta el caracter de nueva línea
    commandBT.trim(); // elimina los espacios en blanco al principio y al final de la cadena
    comandos_bluetooth();
  }
  }
}
void bajo_voltage()
{
  if(voltage<V_MIN) //control de bajo voltaje
  {
    volt_bat=analogRead(BAT_PIN);
     voltage = map(volt_bat+=40, ADC_MIN, ADC_MAX, VOLTAGE_MIN * 100, VOLTAGE_MAX * 100) / 100.0;
    alarma=true;
    lcd.clear();
    while (alarma==true) // mientras la alarma esté activa y no hayan pasado más de 60 segundos
    { 
       botones=presionado();
      volt_bat=analogRead(BAT_PIN);
      voltage = map(volt_bat+=40, ADC_MIN, ADC_MAX, VOLTAGE_MIN * 100, VOLTAGE_MAX * 100) / 100.0;
      lcd.setCursor(1,0);
      lcd.print("*BATERIA BAJA*");
      lcd.setCursor(1,1);
      lcd.print("PRESIONE RESET");
      delay(500);
      for(short i=0;i>2;i++)
      {
        digitalWrite(buzzer,HIGH);
        delay(750);
        digitalWrite(buzzer,LOW);
        delay(750);
      }
        if(botones==VALOR_ENTER)
       {
        delay(2500);
        alarma==false;
        battery_metter_available=false;
        EEPROM.write(battery_status,battery_metter_available);
        delay(100);
        lcd.clear();
        return;
       }
      else if (voltage==V_OPERATED)
      {
        delay(250);
        alarma=false;
        lcd.clear();
        return;   
      } 
    }
  }
}
byte presionado()
{
  UP=digitalRead(PIN_ARRIBA);
  IN=digitalRead(PIN_ENTER);
  DOWN=digitalRead(PIN_ABAJO);
  short botones=0;
  short resultado=0;
  //variables asignadas para almacenar el valor de los pines
  if(UP==HIGH)
  {
    delay(150);
    resultado=VALOR_UP;
  }
  else if(IN==HIGH)
  {
    delay(150);
    resultado=VALOR_ENTER;
   }
  else if(DOWN==HIGH)
  {
    delay(150);
    resultado=VALOR_DOWN;
  }
  return(resultado);
}
void display_posicion()
{
	if(botones==VALOR_DOWN)
	{
		lcd.clear();
		posicion++;
    milisegundos = 0;
	}
	else if(botones==VALOR_UP)
	{
		lcd.clear();
		posicion--;
    milisegundos = 0;
	}	
}
void parametro_actualizado()
{
	lcd.clear();
	lcd.setCursor(0, 0);
	lcd.print("PARAMETRO");
	lcd.setCursor(0, 1);
	lcd.print("ACTUALIZADO");
	delay(2000);
	lcd.clear();
}
void DISPENDIO() // funcion de abrir y cerrar la compuerta
{

  delay(100);
  lcd.clear();
  lcd.setCursor(2,0);
  lcd.print("*DISPENSANDO*");
  /////////// avsio de que la compuerta abrira/////////////////////
  digitalWrite(buzzer, HIGH);
  delay(750);
  digitalWrite(buzzer, LOW);
  delay(250);
  digitalWrite(buzzer, HIGH);
  delay(750);
  digitalWrite(buzzer, LOW);
  //////////////////////////abre compuerta////////////////////
  if (servo_enable != 0)
  {  
    for (pos = 0; pos <= 27; pos += 1)
    {
      if(M1_available==true){
        servo1.write(pos);
      }
      if(M2_available==true){
        servo2.write(pos);
      }
      if(M3_available==true){
        servo3.write(pos);
      }
      if(M4_available==true){
        servo4.write(pos);
      }
      delay(30);
    }
    delay(tiempo_apertura);
    //////////////////////////////cierra compuerta/////////////////////////
    for (pos = 27; pos >= 0; pos -= 1)
    {
      if(M1_available==true){
        servo1.write(pos);
      }
      if(M2_available==true){
        servo2.write(pos);
      }
      if(M3_available==true){
        servo3.write(pos);
      }
      if(M4_available==true){
        servo4.write(pos);
      }
      delay(30);
    }
    //////////////aviso de que la compuerta cerrara//////////////////////
    digitalWrite(buzzer, HIGH);
    delay(750);
    digitalWrite(buzzer, LOW);
    delay(250);
    digitalWrite(buzzer, HIGH);
    delay(750);
    digitalWrite(buzzer, LOW);
    delay(250);
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
void Comando_serial()
{
  DateTime fecha = rtc.now(); // funcion que devuelve fecha y horario 
  if (entradaSerial == "estado\n") // enviame todas las informaciones de la tarjeta 
  {
    // estado(fecha);
    Serial.println("hora:");
    Serial.println(hora);
    Serial.println("minutos:");
    Serial.println(fecha.minute());
    Serial.println("ultima comida fue:");
    Serial.println(ultima_corrida);
    Serial.println("proxima comida a las:");
    if (ultima_corrida + intervalos_hora>=17)
    {
      Serial.print("(fuera de horario)");
    }
    Serial.println(ultima_corrida + intervalos_hora);
    Serial.println("tamaño:");
    switch (tiempo_apertura)
    {
      case 45:
        Serial.println("pequeños");
      break;
      case 75:
        Serial.println("medianos");
      break;
      case 100:
        Serial.println("grandes");
      break;
      default:
        Serial.println("personalizado");
      break;
    }
    Serial.println("TIPO DE PEZ:");
    switch(tipo_de_pez)
    {
    case 1:
      Serial.println("TILAPIA");
    break;
    case 2:
      Serial.println("TILAPIA ROJA");
    break;
    case 3:
      Serial.println("CABEZA DE LEON");
    break;
    case 4:
      Serial.println("SALMON");
    break;
    case 5:
      Serial.println("BEBE DE TIBURON");
    break;
    case 6:
      Serial.println("KOI");
    break;
    case 7:
      Serial.println("PETRA");
    break;
    default:
      Serial.println("NO SELECCIONADO");
    break;
    }
    Serial.println("volatage:");
    Serial.println(voltage);
    Serial.print("(");
    Serial.print(porcentaje_bateria);
    Serial.print("%");
    Serial.print(")");
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
  else if(entradaSerial == "ajusta hora\n")
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
void comandos_bluetooth()
{
  DateTime fecha = rtc.now(); // funcion que devuelve fecha y horario 
  if (commandBT == "estado") {
    SerialBT.println("hora:");
    SerialBT.println(hora);
    SerialBT.println("minutos:");
    SerialBT.println(fecha.minute());
    SerialBT.println("ultima comida fue:");
    SerialBT.println(ultima_corrida);
    SerialBT.println("proxima comida a las:");
    if (ultima_corrida + intervalos_hora>=17)
    {
      SerialBT.print("(fuera de horario)");
    }
    SerialBT.println(ultima_corrida + intervalos_hora);
    SerialBT.println("tamaño:");
    switch (tiempo_apertura)
    {
      case 45:
        SerialBT.println("pequeños");
      break;
      case 75:
        SerialBT.println("medianos");
      break;
      case 100:
        SerialBT.println("grandes");
      break;
      default:
        SerialBT.println("personalizado");
      break;
    }
    SerialBT.println("TIPO DE PEZ:");
    switch(tipo_de_pez)
    {
    case 1:
      SerialBT.println("TILAPIA");
    break;
    case 2:
      SerialBT.println("TILAPIA ROJA");
    break;
    case 3:
      SerialBT.println("CABEZA DE LEON");
    break;
    case 4:
      SerialBT.println("SALMON");
    break;
    case 5:
      SerialBT.println("BEBE DE TIBURON");
    break;
    case 6:
      SerialBT.println("KOI");
    break;
    case 7:
      SerialBT.println("PETRA");
    break;
    default:
      SerialBT.println("NO SELECCIONADO");
    break;
    }
    SerialBT.println("volatage:");
    SerialBT.println(voltage);
    SerialBT.print("(");
    SerialBT.print(porcentaje_bateria);
    SerialBT.print("%");
    SerialBT.print(")");
  }
  else if (commandBT == "dispensa") {
    servo_enable = 1;
    SerialBT.println("DISPENSANDO...");
    DISPENDIO();
    SerialBT.println("DISPENDIO FINALIZADO");
  }
  else if(commandBT=="prueba motores")
  {
    SerialBT.println("pronbando motores");
    servo_enable = 1;
    prueba_motores();
  }
  else if(commandBT== "ajusta hora")
  {
    SerialBT.print("AJUSTANDO HORA");
    delay(50);
    rtc.adjust(DateTime(__DATE__, __TIME__));
    SerialBT.println("HORA AJUSTADA");
  }
  else {
    SerialBT.println("Comando no reconocido");
  }
}
void prueba_motores()
{
  if (servo_enable != 0)
  {  
    for (pos = 0; pos <= 180; pos += 1)
    {
      if(M1_available==true){
        servo1.write(pos);
      }
      if(M2_available==true){
        servo2.write(pos);
      }
      if(M3_available==true){
        servo3.write(pos);
      }
      if(M4_available==true){
        servo4.write(pos);
      }
      delay(40);
    }
    delay(tiempo_apertura);
    //////////////////////////////cierra compuerta/////////////////////////
    for (pos = 180; pos >= 0; pos -= 1)
    {
      if(M1_available==true){
        servo1.write(pos);
      }
      if(M2_available==true){
        servo2.write(pos);
      }
      if(M3_available==true){
        servo3.write(pos);
      }
      if(M4_available==true){
        servo4.write(pos);
      }
       delay(40);
    } 
  
  } 

}
void pantalla_principal(){
  DateTime fecha= rtc.now();
  hora=fecha.hour();
  minutos=fecha.minute();
  botones=presionado();
  if(battery_metter_available==true){
    volt_bat=analogRead(BAT_PIN);
    voltage = map(volt_bat+=40, ADC_MIN, ADC_MAX, VOLTAGE_MIN * 100, VOLTAGE_MAX * 100) / 100.0;
    porcentaje_bateria=short((voltage - 3.4) / (4.2 - 3.4) * 100);
  }
  switch(posicion)
  {
   case -1: 
     posicion=horas;
   break;
   case horas:
        lcd.setCursor(0,0);
        lcd.print("HORA:");
        lcd.setCursor(5,0);
        lcd.print(fecha.hour());
        lcd.setCursor(7,0);
        lcd.print(":");
        lcd.setCursor(9,0);
        lcd.print(minutos);
        if(battery_metter_available==true) 
        {
          if(milisegundos>=200)
          { //refresca los voltajes de la pantalla
            lcd.setCursor(0,1);
            lcd.print("VBAT:");
            lcd.setCursor(5,1);
            lcd.print(voltage);
            lcd.setCursor(9,1);
            lcd.print("V");
            lcd.setCursor(10,1);
            lcd.print("/");
            lcd.setCursor(11,1);
            lcd.print(porcentaje_bateria);
            lcd.setCursor(13,1);
            lcd.print("%");
            milisegundos = 0;
          }
        }
        if(hora < 9 || hora >= 17 && fecha.minute() >= 3)
        {
          lcd.setCursor(12,0);
          lcd.print("OUT");
        }
        delay(1);
        milisegundos ++;
       display_posicion();
      break;
    case prox_comida:
    if(milisegundos>=400)
    {
      lcd.scrollDisplayLeft();
     lcd.setCursor(0,0);
     lcd.print("ULTIMA COMIDA:");
     lcd.setCursor(15,0);
     lcd.print(ultima_corrida);
     lcd.setCursor(0,1);
     lcd.print("PROXIMA COMIDA:");
     lcd.setCursor(15,1);
     lcd.print(ultima_corrida+intervalos_hora);
     milisegundos=0;
    }
    delay(1);
    milisegundos++;
    display_posicion();
    
   break;
   case intervalo:
      lcd.setCursor(3,0);
      lcd.print("INTERVALOS:");
      lcd.setCursor(3,1);
      lcd.print("C/");
      lcd.setCursor(5,1);
      lcd.print(intervalos_hora);
      lcd.setCursor(7,1);
      lcd.print("HORAS");
      display_posicion();
   break;
   case tip_pez:
     EEPROM.get(TIPO_PEZ,tipo_de_pez);
     lcd.setCursor(0,0);
     lcd.print("TIPO DE PEZ:");
     lcd.setCursor(0,1);
     switch(tipo_de_pez)
     {
      case 1:
        lcd.print("TILAPIA");
      break;
      case 2:
        lcd.print("TILAPIA ROJA");
      break;
      case 3:
        lcd.print("CABEZA DE LEON");
      break;
      case 4:
        lcd.print("SALMON");
      break;
      case 5:
        lcd.print("BEBE DE TIBURON");
      break;
      case 6:
        lcd.print("KOI");
      break;
      case 7:
        lcd.print("PETRA");
      break;
      default:
        lcd.print("NO SELECCIONADO");
      break;
      }
    display_posicion();
   break;
   case tamano:
   EEPROM.get(tiem_apertura,tiempo_apertura);
   lcd.setCursor(0,0);
   lcd.print("DISPENDIO PARA:");
   lcd.setCursor(0,1);
    switch (tiempo_apertura)
    {
     case 45:
      lcd.print("ALEVINE");
     break;
     case 75:
      lcd.print("JUVENILES");
     break;
     case 100:
     lcd.print("ADULTOS");
     default:
      lcd.print("PERSONALIZADO");
     break;
    }
    display_posicion();
   break;
   case configuracion:
    lcd.setCursor(0,0);
    lcd.print("CONFIGURACION");
    lcd.setCursor(0,1);
    lcd.print("*PRESIONE ENTER*");
    if(botones==VALOR_ENTER)
    {
     delay(250);
     lcd.clear();
     pantalla_configuraciones();
    }
    display_posicion();
   break;
   case 6:
     posicion=configuracion;
   break;
  }
}
void pantalla_configuraciones(){
  posicion=7;
  while(posicion>=7 && posicion<=14)
  {
    dispendio=digitalRead(PIN_DISPENSE);
   botones=presionado();
   switch(posicion)
   {
      case 7: 
      posicion=config_hora;
    break;
    case config_hora:
      lcd.setCursor(0,0);
      lcd.print("*CONFIGURACION*");
      lcd.setCursor(0,1);
      lcd.print("->");
      lcd.setCursor(2,1);
      lcd.print("HORA");
    if (botones == VALOR_ENTER)
    {
     delay(100);
     lcd.clear();
     configuracion_hora();
     }
     display_posicion();
     break;
    case config_intervalos:
      lcd.setCursor(0,0);
      lcd.print("*CONFIGURACION*");
      lcd.setCursor(0,1);
      lcd.print("->");
      lcd.setCursor(2,1);
      lcd.print("INTERVALOS");
      if(botones==VALOR_ENTER)
      {
        delay(250);
        lcd.clear();
        configuracion_intervalos();
      }
      display_posicion();
    break;                    
    case config_dispen:
      lcd.setCursor(0,0);
      lcd.print("*CONFIGURACION*");
      lcd.setCursor(0,1);
      lcd.print("->");
      lcd.setCursor(2,1);
      lcd.print("DISPENDIO");
    if (botones == VALOR_ENTER)
    {
      delay(100);
      lcd.clear();
     configuracion_dispendio();
     }
     display_posicion();
    break;
    case config_pez:
      lcd.setCursor(0,0);
      lcd.print("*CONFIGURACION*");
      lcd.setCursor(0,1);
      lcd.print("->");
      lcd.setCursor(2,1);
      lcd.print("TIPO DE PEZ");
      if (botones==VALOR_ENTER)
      {
        delay(100);
        lcd.clear();
        configuracion_tipo_pez();
      }
      display_posicion();
    break;
    case config_motores:
      lcd.setCursor(0,0);
      lcd.print("*CONFIGURACION*");
      lcd.setCursor(0,1);
      lcd.print("->");
      lcd.setCursor(2,1);
      lcd.print("MOTORES");
      if (botones == VALOR_ENTER)
      {
        delay(100);
        lcd.clear();
        configuracion_motores();
      }
      if (dispendio == HIGH) //incia el proceso de alimentacion manualmente
      {
        servo_enable = 1;
        DISPENDIO(); // inicia el proceso de abrir la compuerta 
      }
      display_posicion();
    break;
    case medidor_bat:
      lcd.setCursor(0,0);
      lcd.print("*CONFIGURACION*");
      lcd.setCursor(0,1);
      lcd.print("->");
      lcd.setCursor(2,1);
      lcd.print("MEDIDOR BAT.");
      if (botones == VALOR_ENTER)
      {
        delay(100);
        lcd.clear();
        bat_metter();
      }
      display_posicion();
    break;
    case salir:
      lcd.setCursor(0,0);
      lcd.print("*CONFIGURACION*");
      lcd.setCursor(0,1);
      lcd.print("->");
      lcd.setCursor(2,1);
      lcd.print("SALIR");
      if (botones == VALOR_ENTER)
      {
        delay(100);
        lcd.clear();
        posicion=configuracion;
        return;
      }
     if(botones==VALOR_UP)
     {
      delay(100);
      posicion--;
     }
    break;
   }
  }
}
void configuracion_hora()
{
  posicion=15;
  while(posicion>=15)
  {
   delay(20);
   EEPROM.get(suma_H,suma_hora);
   EEPROM.get(suma_M,suma_minutos);
   botones=presionado();
   switch(posicion)
   {
      case 15:
      lcd.setCursor(6,0);
      lcd.print("HORA");
      lcd.setCursor(5,1);
      lcd.print(suma_hora);
      lcd.setCursor(7,1);
      lcd.print(":");
      if(botones==VALOR_UP)
     {
        delay(100);
        suma_hora++;
     }
                                
     else if (botones == VALOR_ENTER)
     {
        delay(250);
        rtc.adjust(DateTime(2022,6,5,suma_hora,minutos,0));
        EEPROM.write(suma_H,suma_hora);
        posicion=16;
     }

     else if(botones==VALOR_DOWN)
     {
        delay(100);
        suma_hora--;
     }
     else if(suma_hora>24)
     {
      suma_hora=00;
     }
      else if(suma_hora<0)
      {
        suma_hora=23;
      }
      break;
      case 16:  // cambia los minutos // guardar en la eeprom
        lcd.setCursor(6,0);
        lcd.print("HORA");
        lcd.setCursor(5,1);
        lcd.print(suma_hora);
        lcd.setCursor(7,1);
        lcd.print(":");
        lcd.setCursor(8,1);
        lcd.print(suma_minutos);
        if(botones == VALOR_UP)
        {
          delay(100);
          suma_minutos++;
        }
        else if(botones == VALOR_ENTER)
        {
        delay(250);
        rtc.adjust(DateTime(2022,6,5,suma_hora,suma_minutos,0));
        EEPROM.write(suma_M,suma_minutos);
        posicion=config_hora;
        parametro_actualizado();
        return;
        }
        else if(botones == VALOR_DOWN)
        { 
        delay(100);
        suma_minutos--;
        }
       if(suma_minutos>59)
        {
        suma_minutos=00;
        }
        else if(suma_minutos<00)
        {
        suma_minutos=59;
        }
     break;
    }
  }
}
void configuracion_intervalos()
{
  posicion=17;
  while(posicion>=17)
  {
    botones=presionado();
    lcd.setCursor(3,0);
    lcd.print("INTERVALOS");
    lcd.setCursor(0,1);
    lcd.print("->");
    lcd.setCursor(2,1);
    lcd.print(intervalos_hora);
    lcd.setCursor(8,1);
    lcd.print("(1-12)");
    if(botones==VALOR_UP)
    {
      delay(100);
      intervalos_hora++;
    }
    else if(botones==VALOR_ENTER)
    {
      delay(250);
      EEPROM.write(intervalos_H,intervalos_hora);
      parametro_actualizado();
      posicion=config_intervalos;
      return;
    }
    else if(botones==VALOR_DOWN)
    {
      delay(100);
      intervalos_hora--;
    }
    if(intervalos_hora>12)
    {
      intervalos_hora=12;
    }
    else if(intervalos_hora<=0)
    {
      intervalos_hora=1;
    }
  }
}
void configuracion_dispendio(){
  posicion=18;
  EEPROM.read(tiem_apertura);
  while(posicion>=-18)
  {
   botones=presionado();
   switch(posicion)
   {
      case 18: 
        posicion=19; 
      break;
      case 19:
        lcd.setCursor(0,0);
        lcd.print("DISPENDIO PARA");
        lcd.setCursor(0,1);
        lcd.print("->");
        lcd.setCursor(3,1);
        lcd.print("ALEVINES");
        if(botones==VALOR_ENTER)
        {
          delay(100);
          tiempo_apertura=45;
          EEPROM.write(tiem_apertura,tiempo_apertura);
          parametro_actualizado();
          posicion=config_dispen;
          return;
        }
        display_posicion();
      break;
      case 20:
        lcd.setCursor(0,0);
        lcd.print("DISPENDIO PARA");
        lcd.setCursor(0,1);
        lcd.print("->");
        lcd.setCursor(3,1);
        lcd.print("JUVENILES");
        if(botones==VALOR_ENTER)
        {
          delay(100);
          tiempo_apertura=75;
          EEPROM.write(tiem_apertura,tiempo_apertura);
          parametro_actualizado();
          posicion=config_dispen;
          return;
        }
        display_posicion();
      break;
      case 21:
        lcd.setCursor(0,0);
        lcd.print("DISPENDIO PARA");
        lcd.setCursor(0,1);
        lcd.print("->");
        lcd.setCursor(3,1);
        lcd.print("ADULTOS");
        if(botones==VALOR_ENTER)
        {
          delay(100);
          tiempo_apertura=100;
          EEPROM.write(tiem_apertura,tiempo_apertura);
          parametro_actualizado();
          posicion=config_dispen;
          return;
        }
        display_posicion();
      break;
      case 22:
        lcd.setCursor(0,0);
        lcd.print("DISPENDIO PARA");
        lcd.setCursor(0,1);
        lcd.print("->");
        lcd.setCursor(3,1);
        lcd.print("PERSONALIZADO");
        if(botones==VALOR_ENTER)
        {
          delay(100);
          lcd.clear();
          posicion=24;
        }
        display_posicion();
      break;
      case 23: posicion=22; break;
      case 24:
        lcd.setCursor(0,0);
        lcd.print("DURACION");
        lcd.setCursor(0,1);
        lcd.print("->");
        lcd.setCursor(2,1);
        lcd.print(tiempo_apertura);
        lcd.setCursor(8,1);
        lcd.print("(0-32.555)");
        if(botones==VALOR_UP)
        {
           delay(50);
           tiempo_apertura+=20;
        }
        else if(botones==VALOR_ENTER)
        {   

          EEPROM.write(tiem_apertura,tiempo_apertura);
          delay(20);
          parametro_actualizado();
          posicion=config_dispen;
          return;
        }   
          else if(botones==VALOR_DOWN)
        {
          delay(50);
          tiempo_apertura-=20;
        }
        if(tiempo_apertura==-1)
        {
          tiempo_apertura=32500;
        }
        if(tiempo_apertura>32500)
        {  
          tiempo_apertura=0;
        }
      break;
    }
  }
}
void configuracion_tipo_pez(){
   posicion=25;
   while(posicion>=25)
   {
    botones=presionado();
    switch(posicion)
    {
      case 25: 
       posicion=26;
      break;
      case 26:
        lcd.setCursor(0,0);
        lcd.print("TIPO DE PEZ");
        lcd.setCursor(0,1);
        lcd.print("->");
        lcd.setCursor(2,1);
        lcd.print("TILAPIA");
        if(botones==VALOR_ENTER)
        {
          delay(250);
          tipo_de_pez=1;
          EEPROM.write(TIPO_PEZ,tipo_de_pez);
          posicion=config_pez;
          parametro_actualizado();
          return;
        }
        display_posicion();
      break;
      case 27:
        lcd.setCursor(0,0);
        lcd.print("TIPO DE PEZ");
        lcd.setCursor(0,1);
        lcd.print("->");
        lcd.setCursor(2,1);
        lcd.print("TILAPIA ROJA");
        if(botones==VALOR_ENTER)
        {
          delay(250);
          tipo_de_pez=2;
          EEPROM.write(TIPO_PEZ,tipo_de_pez);
          posicion=config_pez;
          parametro_actualizado();
          return;
        }
        display_posicion();
      break;
      case 28:
        lcd.setCursor(0,0);
        lcd.print("TIPO DE PEZ");
        lcd.setCursor(0,1);
        lcd.print("->");
        lcd.setCursor(2,1);
        lcd.print("CAEBZA DE LEON");
        if(botones==VALOR_ENTER)
        {
          delay(250);
          tipo_de_pez=3;
          EEPROM.write(TIPO_PEZ,tipo_de_pez);
          posicion=config_pez;
          parametro_actualizado();
          return;
        }
        display_posicion();
      break;
      case 29:
        lcd.setCursor(0,0);
        lcd.print("TIPO DE PEZ");
        lcd.setCursor(0,1);
        lcd.print("->");
        lcd.setCursor(2,1);
        lcd.print("SALMON");
        if(botones==VALOR_ENTER)
        {
           delay(250);
           tipo_de_pez=4;
           EEPROM.write(TIPO_PEZ,tipo_de_pez);
           posicion=config_pez;
           parametro_actualizado();
           return;
        }
        display_posicion();
      break;
      case 30:
        lcd.setCursor(0,0);
        lcd.print("TIPO DE PEZ");
        lcd.setCursor(0,1);
        lcd.print("->");
        lcd.setCursor(2,1);
        lcd.print("BEBE DE TIBURON");
        if(botones==VALOR_ENTER)
        {
          delay(250);
          tipo_de_pez=5;
          EEPROM.write(TIPO_PEZ,tipo_de_pez);
          posicion=config_pez;
          parametro_actualizado();
          return;
        }
        display_posicion();
      break;
      case 5: posicion=30; break;
    }
  }
}
void configuracion_motores() {
  posicion=31;
  while(posicion>=31)
  {
    
    switch (posicion)
    {
      case 31: posicion=32; break;
      case 32: 
        lcd.setCursor(2,0);
        lcd.print("*CONFIGURAR*");
        lcd.setCursor(0,1);
        lcd.print("->");
        lcd.setCursor(2,1);
        lcd.print("M1");
        if(botones==VALOR_ENTER)
        {
          delay(250);
          lcd.clear();
          posicion=38;
        }
        display_posicion();
      break;
      case 33:
        lcd.setCursor(2,0);
        lcd.print("*CONFIGURAR*");
        lcd.setCursor(0,1);
        lcd.print("->");
        lcd.setCursor(2,1);
        lcd.print("M2");
        if(botones==VALOR_ENTER)
        {
          delay(250);
          lcd.clear();
          posicion=42;
        }
        display_posicion();
      break;
      case 34:
        lcd.setCursor(2,0);
        lcd.print("*CONFIGURAR*");
        lcd.setCursor(0,1);
        lcd.print("->");
        lcd.setCursor(2,1);
        lcd.print("M3");  
        if(botones==VALOR_ENTER)
        {
          delay(250);
          lcd.clear();
          posicion=46;
        }
        display_posicion();
      break;
      case 35:
        lcd.setCursor(2,0);
        lcd.print("*CONFIGURAR*");
        lcd.setCursor(0,1);
        lcd.print("->");
        lcd.setCursor(2,1);
        lcd.print("M4");
        if(botones==VALOR_ENTER)
        {
          delay(250);
          lcd.clear(); 
          posicion=50;
        }
        display_posicion();
      break;
      case 36: posicion=35; break;
      ///////////////M1////////////////////////
      case 37: posicion=38; break;
      case 38:
        lcd.setCursor(6,0);
        lcd.print("*M1*");
        lcd.setCursor(0,1);
        lcd.print("->");
        lcd.setCursor(2,1);
        lcd.print("ACTIVAR");
        if(botones==VALOR_ENTER){
          delay(250);
          M1_available=true;
          EEPROM.write(M1_status,M1_available);
          EEPROM.commit();
          parametro_actualizado();
          posicion=config_motores;
          return;
        }
        display_posicion();
      break;
      case 39:
        lcd.setCursor(6,0);
        lcd.print("*M1*");
        lcd.setCursor(0,1);
        lcd.print("->");
        lcd.setCursor(2,1);
        lcd.print("DESACTIVAR");
        if(botones==VALOR_ENTER){
          delay(250);
          M1_available=false;
          EEPROM.write(M1_status,M1_available);
          EEPROM.commit();
          parametro_actualizado();
          posicion=config_motores;
          return;
        }
        display_posicion();
      break;
      case 40: posicion=39; break;
      ///////////////M2////////////////////
      case 41: posicion=42; break;
      case 42:
        lcd.setCursor(6,0);
        lcd.print("*M2*");
        lcd.setCursor(0,1);
        lcd.print("->");
        lcd.setCursor(2,1);
        lcd.print("ACTIVAR");
        if(botones==VALOR_ENTER){
          delay(250);
          M2_available=true;
          EEPROM.write(M2_status,M2_available);
          EEPROM.commit();
          parametro_actualizado();
          posicion=config_motores;
          return;
        }
        display_posicion();
      break;
      case 43:
        lcd.setCursor(6,0);
        lcd.print("*M2*");
        lcd.setCursor(0,1);
        lcd.print("->");
        lcd.setCursor(2,1);
        lcd.print("DESACTIVAR");
        if(botones==VALOR_ENTER){
          delay(250);
          M2_available=false;
          EEPROM.write(M2_status,M2_available);
          EEPROM.commit();
          parametro_actualizado();
          posicion=config_motores;
          return;
        }
        display_posicion();
      break;
      case 44: posicion=43; break; 
      /////////////////M3///////////////////////////
        case 45: posicion = 46; break;
        case 46:
        lcd.setCursor(6,0);
        lcd.print("*M3*");
        lcd.setCursor(0,1);
        lcd.print("->");
        lcd.setCursor(2,1);
        lcd.print("ACTIVAR");
        if(botones==VALOR_ENTER){
          delay(250);
          M3_available=true;
          EEPROM.write(M3_status,M3_available);
          EEPROM.commit();
          parametro_actualizado();
          posicion=config_motores;
          return;
        }
        display_posicion();
      break;
      case 47:
        lcd.setCursor(6,0);
        lcd.print("*M3*");
        lcd.setCursor(0,1);
        lcd.print("->");
        lcd.setCursor(2,1);
        lcd.print("DESACTIVAR");
        if(botones==VALOR_ENTER){
          delay(250);
          M3_available=false;
          EEPROM.write(M3_status,M3_available);
          EEPROM.commit();
          parametro_actualizado();
          posicion=config_motores;
          return;
        }
        display_posicion();
      break;
      case 48: posicion=47; break;
      ////////////M4///////////////////
      case 49:posicion=50; break;
      case 50:
        lcd.setCursor(6,0);
        lcd.print("*M4*");
        lcd.setCursor(0,1);
        lcd.print("->");
        lcd.setCursor(2,1);
        lcd.print("ACTIVAR");
        if(botones==VALOR_ENTER){
          delay(250);
          M4_available=true;
          EEPROM.write(M4_status,M4_available);
          EEPROM.commit();
          parametro_actualizado();
          posicion=config_motores;
          return;
        }
        display_posicion();
      break;
      case 51:
        lcd.setCursor(6,0);
        lcd.print("*M4*");
        lcd.setCursor(0,1);
        lcd.print("->");
        lcd.setCursor(2,1);
        lcd.print("DESACTIVAR");
        if(botones==VALOR_ENTER){
          delay(250);
          M4_available=false;
          EEPROM.write(M4_status,M4_available);
          EEPROM.commit();
          parametro_actualizado();
          posicion=config_motores;
          return;
        } 
      break;
    }
  }
}
void bat_metter(){  
  posicion=52;
  while (posicion>=52)
  {
    botones=presionado();
    switch(posicion)
    {
      case 52:
        posicion=53;
      break;
      case 53:
        lcd.setCursor(6,0);
        lcd.print("MEDIDOR BATERIA");
        lcd.setCursor(0,1);
        lcd.print("->");
        lcd.setCursor(2,1);
        lcd.print("ACTIVAR");
        if(botones==VALOR_ENTER){
          delay(250);
          battery_metter_available=true;
          EEPROM.write(battery_status,battery_metter_available);
          EEPROM.commit();
          parametro_actualizado();
          delay(50);
          ESP.restart();
        }
        if(botones==VALOR_DOWN)
        {
          delay(100);
          posicion++;
        }
      break;
      case 54:
        lcd.setCursor(6,0);
        lcd.print("MEDIDOR BATERIA");
        lcd.setCursor(0,1);
        lcd.print("->");
        lcd.setCursor(2,1);
        lcd.print("DESACTIVAR");
        if(botones==VALOR_ENTER){
          delay(250);
          battery_metter_available=false;
          EEPROM.write(battery_status,battery_metter_available);
          EEPROM.commit();
          parametro_actualizado();
          delay(50);
          ESP.restart();
        }
        else if(botones==VALOR_UP)
        {
          delay(100);
          posicion--;
        }
      break;  
    }

  }
}