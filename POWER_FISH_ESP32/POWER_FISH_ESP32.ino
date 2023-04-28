//#include <Servo.h> // incluye libreria de Servo
#include <EEPROM.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <ESP32_Servo.h> 
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
//////////////////////////////variables que nombraran los espacios de la eeprom///////////////
byte ultim_corrid=0;
byte intervalos_H=2;
byte tiem_apertura=4;
byte TIPO_PEZ=6;
byte suma_H=8;
byte suma_M=10;
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
signed short sub_menu=0;
signed short sub_menu1=0;
signed short sub_menu2=0;
signed short sub_menu3=0;
signed short sub_menu4=0;
/////////////////////variables del cambio de hora/////////////////////////
short dispendio = 0;
short milisegundos=0;
byte servo_enable = 1;
byte aviso_cambio = 0;
String entradaSerial = "";    // String para almacenar entrada
String commandBT;
bool entradaCompleta = false; // Indicar si el String está completo
//////////////////////////////////////////////////////////////////
int16_t tiempo_apertura = 0;
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
const float adcResolution = 3400.0; // Resolución del ADC
enum inicio {horas=0,prox_comida,intervalo,tip_pez,tamano,configuracion};
enum config {config_hora=0,intervalos,config_dispen,config_pez,salir};
///////////////////////////////////////////////////////////////////////
void setup()
{ 
  lcd.init();
  lcd.backlight();
  /////////////////////////pines declarados para salida y entrada////////////////////////
  pinMode(BAT_PIN,OUTPUT);
  pinMode(PIN_ABAJO, INPUT);      // MODO SMALL
  pinMode(PIN_ENTER, INPUT);      // MODO MEDIUM
  pinMode(PIN_ARRIBA, INPUT);      // MODO BIG
  pinMode(PIN_DISPENSE, INPUT);      // pin utilizado para el boton de desispendio manual
  pinMode(buzzer, OUTPUT); // buzzer de alarma
  volt_bat=analogRead(BAT_PIN);
   voltage = map(volt_bat+=40, ADC_MIN, ADC_MAX, VOLTAGE_MIN * 100, VOLTAGE_MAX * 100) / 100.0;
  bajo_voltage();
    ///////////////////////////////////////////////////////
  Serial.begin(115200); //ESP-32
  SerialBT.begin("POWER FISH"); // nombre del dispositivo Bluetooth EN EL ESP32
  Serial.println("Bluetooth iniciado");
  //Serial.begin(9600); arduino 
  /////////////////INICIALIZACION de los servos ////////////////////////////////
  servo1.write(0); // inicia con la compuerta cerrada
  servo2.write(0); // inicia con la compuerta cerrada
  servo3.write(0); // inicia con la compuerta cerrada
  servo4.write(0); // inicia con la compuerta cerrada
  servo1.attach(pinservo1);
  servo2.attach(pinservo2);
  servo3.attach(pinservo3);
  servo4.attach(pinservo4);
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
    EEPROM.put(0,ultima_corrida);
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
  EEPROM.get(tiem_apertura,tiempo_apertura);
  delay(100);
  EEPROM.get(TIPO_PEZ,tipo_de_pez);
  delay(100);
  EEPROM.get(intervalos_H,intervalos_hora);
  /////////////////////////////////////////////
}
void loop()
{
  DateTime fecha = rtc.now(); // funcion que devuelve fecha y horario en formato
  botones=presionado();
  dispendio = digitalRead(PIN_DISPENSE);
  volt_bat=analogRead(BAT_PIN);
   voltage = map(volt_bat+=40, ADC_MIN, ADC_MAX, VOLTAGE_MIN * 100, VOLTAGE_MAX * 100) / 100.0;
  pantalla_principal();
  bajo_voltage();

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
    bajo_voltage();
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
      delay(60000);
      if (voltage==V_OPERATED)
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
    sub_menu++;
    sub_menu1++;
    sub_menu2++;
    sub_menu3++;
    sub_menu4++;
    milisegundos = 0;
	}
	else if(botones==VALOR_UP)
	{
		lcd.clear();
		posicion--;
    sub_menu--;
    sub_menu1--;
    sub_menu2--;
    sub_menu3--;
    sub_menu4--;
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
      servo1.write(pos);
      delay(20);
    }
    delay(tiempo_apertura);
    //////////////////////////////cierra compuerta/////////////////////////
    for (pos = 27; pos >= 0; pos -= 1)
    {
      servo1.write(pos);
      delay(20);
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
  for (pos = 10; pos <= 180; pos += 1)
  { // goes from 0 degrees to 180 degrees
    // in steps of 1 degree
    servo1.write(pos); // tell servo to go to position in variable 'pos'
    delay(15);         // waits 15ms for the servo to reach the position
  }
  for (pos = 180; pos >= 10; pos -= 1)
  {                    // goes from 180 degrees to 0 degrees
    servo1.write(pos); // tell servo to go to position in variable 'pos'
    delay(15);         // waits 15ms for the servo to reach the position
  }
}
void pantalla_principal(){
  DateTime fecha= rtc.now();
  hora=fecha.hour();
  minutos=fecha.minute();
  botones=presionado();
  volt_bat=analogRead(BAT_PIN);
  voltage = map(volt_bat+=40, ADC_MIN, ADC_MAX, VOLTAGE_MIN * 100, VOLTAGE_MAX * 100) / 100.0;
  porcentaje_bateria=short((voltage - 3.4) / (4.2 - 3.4) * 100);
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
      if(milisegundos>=200){ //refresca los voltajes de la pantalla
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
        if(hora < 9 || hora >= 17 && fecha.minute() >= 3)
        {
          lcd.setCursor(12,0);
          lcd.print("OUT");
        }
        milisegundos = 0;
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
     configuraciones();
    }
    display_posicion();
   break;
   case 6:
     posicion=configuracion;
   break;
  }
}
void configuraciones(){
    sub_menu=0;
   while(sub_menu>=-1)
  {
   botones=presionado();
   switch(sub_menu)
   {
    case -1: 
    sub_menu=0;
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
    case intervalos:
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
     display_posicion();
    break;
    case 5:
      sub_menu=salir;
    break;
   }
  }
 }
void configuracion_hora(){
   sub_menu1=0;
   while(sub_menu1>-1)
 {
   delay(20);
   EEPROM.get(suma_H,suma_hora);
   EEPROM.get(suma_M,suma_minutos);
   botones=presionado();
   switch(sub_menu1)
   {
    case 0:
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
      sub_menu1=1;
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
    case 1:  // cambia los minutos // guardar en la eeprom
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
       sub_menu=horas;
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
  sub_menu2=0;
  while(sub_menu2>-1)
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
      sub_menu=intervalos;
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
  sub_menu3=0;
  EEPROM.read(tiem_apertura);
  while(sub_menu3>=-1)
  {
   botones=presionado();
   switch(sub_menu3)
   {
      case -1: 
        sub_menu3=0; 
      break;
      case 0:
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
          sub_menu=config_dispen;
          return;
        }
        display_posicion();
      break;
      case 1:
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
          sub_menu=config_dispen;
          return;
        }
        display_posicion();
      break;
      case 2:
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
          sub_menu=config_dispen;
          return;
        }
        display_posicion();
      break;
      case 3:
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
          sub_menu3=5;
        }
        display_posicion();
      break;
      case 4: sub_menu3=3; break;
      case 5:
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
          sub_menu=config_dispen;
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
   sub_menu4=0;
   while(sub_menu4>=-1)
   {
    botones=presionado();
    switch(sub_menu4)
    {
      case -1: 
       sub_menu4=0;
      break;
      case 0:
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
          sub_menu=config_pez;
          parametro_actualizado();
          return;
        }
        display_posicion();
      break;
      case 1:
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
          sub_menu=config_pez;
          parametro_actualizado();
          return;
        }
        display_posicion();
      break;
      case 2:
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
          sub_menu=config_pez;
          parametro_actualizado();
          return;
        }
        display_posicion();
      break;
      case 3:
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
           sub_menu=config_pez;
           parametro_actualizado();
           return;
        }
        display_posicion();
        break;
       case 4:
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
          sub_menu=config_pez;
          parametro_actualizado();
          return;
        }
        display_posicion();
      break;
      case 5: sub_menu3=4; break;
     }
   }
 }