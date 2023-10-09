/*
Sketch: datalogger.ino
Versión: 0.1
Autor/es:
Descripción: Datalogger de humedad y temperatura usando sensor DHT.
Licencia: 

TODO: Poner la placa en "sleep" luego de un periodo de inactividad para ahorrar
energía https://www.gammon.com.au/power
*/

#include <Wire.h>
#include <RTClib.h>
#include <LiquidCrystal.h>
#include <DHT.h>
#include <SD.h>
#include <SPI.h>

//Pines LCD
#define rs 10
#define rw 9
#define en 8
#define d4 7
#define d5 6
#define d6 5
#define d7 4

#define dht_pin 3
#define int_pin 19
#define sd_pin 53

//Intervalo para lectura de datos (días, horas, minutos, segundos)
TimeSpan interval = TimeSpan(0, 0, 0, 5); 
DateTime now;
volatile float tt;
volatile float hh;
volatile bool write = false;

LiquidCrystal lcd(rs, rw, en, d4, d5, d6, d7);
DHT dht(dht_pin, DHT11);
RTC_DS3231 rtc;

void int0_isr() {
    tt = dht.readTemperature();
    hh = dht.readHumidity();
    write = true;
}

void setup() {
    /*
    TODO: Agregar alguna forma para chequear que los sensores y modulos esten
    conectados/funcionen correctamente
    */
    lcd.begin(16, 2);
    dht.begin();
    rtc.begin();
    if (!SD.begin(sd_pin)) {
        lcd.print("Check SD card");
        while(1);
    }

    //Header para el csv
    File datalog = SD.open("datalog.csv", FILE_WRITE);
    datalog.println("fecha,hora,temperatura,humedad");
    datalog.close();

    //Descomentar para sincronizar la hora del RTC con el sistema
    //rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));

    //En "OFF", pin sqw está normalmente alto y baja cuando la alarma "suena"
    rtc.writeSqwPinMode(DS3231_OFF);
    //Asegurando que no hay alarmas
    rtc.disableAlarm(1);
    rtc.disableAlarm(2);
    rtc.clearAlarm(1);
    rtc.clearAlarm(2);
    now = rtc.now();
    //Intervalo para la primer medición, se puede usar uno más corto, por ejemplo
    //un minuto: rtc.SetAlarm1(now + TimeSpan(0, 0, 1, 0), DS3231_A1_Second)
    //Las siguientes mediciones continuan con el intervalo elegido en "interval"
    rtc.setAlarm1(now + interval, DS3231_A1_Second);
    
    pinMode(int_pin, INPUT_PULLUP);
    //Modo FALLING para evitar que la interrupción se active al desacativar la alarma
    attachInterrupt(digitalPinToInterrupt(int_pin), int0_isr, FALLING);
}

void loop() {
    /*
    TODO: Leer sensores y mostrar valores en LCD unicamente al presionar un boton
    y apagarlo luego de un periodo de inactividad (usando otro interrupt?)
    */
    tt = dht.readTemperature();
    String temp_0 = "T: ";
    String temp = temp_0 + tt + " C";
    lcd.setCursor(0, 0);
    lcd.print(temp);
    hh = dht.readHumidity();
    String hum_0 = "HH: ";
    String hum = hum_0 + hh + " %";
    lcd.setCursor(0, 1);
    lcd.print(hum);

    if (write) {
        now = rtc.now();
        rtc.disableAlarm(1);
        rtc.clearAlarm(1);
        rtc.setAlarm1(now + interval, DS3231_A1_Second);
        write_sd();
        write = false;
    }
}

void write_sd() {
   File datalog = SD.open("datalog.csv", FILE_WRITE);
   datalog.print(now.timestamp(DateTime::TIMESTAMP_DATE));
   datalog.print(",");
   datalog.print(now.timestamp(DateTime::TIMESTAMP_TIME));
   datalog.print(",");
   datalog.print(tt);
   datalog.print(",");
   datalog.println(hh);
   datalog.close();
}
