/*
Sketch: datalogger.ino
Versión: 0.1
Autor/es:
Descripción: Datalogger de humedad y temperatura usando sensor DHT y Arduino Nano.
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

#define dht_pin 14
#define int_pin 2
#define sd_pin 15
#define led_pin 13
#define bclk_pin 16
#define button_pin 3

//Intervalo para lectura de datos (días, horas, minutos, segundos)
TimeSpan interval = TimeSpan(0, 0, 0, 10);
DateTime now;
unsigned long currrent_millis = 0;
float tt;
float hh;
int count = 1;
volatile bool write = false;
volatile bool show_lcd = false;

LiquidCrystal lcd(rs, rw, en, d4, d5, d6, d7);
DHT dht(dht_pin, DHT11);
RTC_DS3231 rtc;

void int00_isr() {
    write = true;
}

void int01_isr() {
    show_lcd = true;
}

void setup() {
    /*
    TODO: Agregar alguna forma para chequear que los sensores y modulos esten
    conectados/funcionen correctamente
    */
    Serial.begin(9600);
    pinMode(bclk_pin, OUTPUT);
    digitalWrite(bclk_pin, HIGH);
    lcd.begin(16, 2);
    dht.begin();
    rtc.begin();
    if (!SD.begin(sd_pin)) {
        lcd.print("Check SD card");
        while(1);
    }
    //Header para el csv
    File datalog = SD.open("datalog.csv", FILE_WRITE);
    datalog.println("numero,fecha,hora,temperatura,humedad");
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
    
    pinMode(led_pin, OUTPUT);
    pinMode(int_pin, INPUT_PULLUP);
    pinMode(button_pin, INPUT_PULLUP);
    //Modo FALLING para evitar que la interrupción se active al desacativar la alarma
    attachInterrupt(digitalPinToInterrupt(int_pin), int00_isr, FALLING);
    attachInterrupt(digitalPinToInterrupt(button_pin), int01_isr, FALLING);
    
}

void loop() {
    if (show_lcd) {
        tt = dht.readTemperature();
        hh = dht.readHumidity();
        lcd.setCursor(0, 0);
        lcd.print("T:");
        lcd.setCursor(3, 0);
        lcd.print(tt);
        lcd.setCursor(9, 0);
        lcd.print("C");
        lcd.setCursor(0, 1);
        lcd.print("H:");
        lcd.setCursor(3, 1);
        lcd.print(hh);
        lcd.setCursor(9, 1);
        lcd.print("%");
        digitalWrite(bclk_pin, HIGH);
        currrent_millis = millis();
        show_lcd = false;
    }
    else if (millis() >= currrent_millis+3000) {
        digitalWrite(bclk_pin, LOW);
    }

    if (write) {
        now = rtc.now();
        tt = dht.readTemperature();
        hh = dht.readHumidity();
        rtc.disableAlarm(1);
        rtc.clearAlarm(1);
        rtc.setAlarm1(now + interval, DS3231_A1_Second);
        write_sd();
        write = false;
        count++;
    }
}

void write_sd() {
    digitalWrite(led_pin, HIGH);
    File datalog = SD.open("datalog.csv", FILE_WRITE);
    datalog.print(count);
    datalog.print(",");
    datalog.print(now.timestamp(DateTime::TIMESTAMP_DATE));
    datalog.print(",");
    datalog.print(now.timestamp(DateTime::TIMESTAMP_TIME));
    datalog.print(",");
    datalog.print(tt);
    datalog.print(",");
    datalog.println(hh);
    datalog.close();
    digitalWrite(led_pin, LOW);
}
