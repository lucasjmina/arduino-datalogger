/*
 * main.cpp
 * Autor/es: Lucas J. Mina. Dario D. Larrea & Miriam P. Damborsky
 * Descripción: Código para prototipo de datalogger de humedad y temperatura usando sensor DHT y 
 * Arduino Nano. Nombre clave "Oreo".
 *
 * TODO:
 * - Mejorar sleep para mayor ahorro de energía https://www.gammon.com.au/power
 * - Límite de mediciones usando conteo o una fecha/hora (usando alarma 2 del RTC?)
 * - Agregar alguna forma para chequear que los sensores y módulos estén conectados/funcionen
 * correctamente
 */

#include <Arduino.h>
#include <Wire.h>
#include <RTClib.h>
#include <LiquidCrystal.h>
#include <DHT.h>
#include <SD.h>
#include <SPI.h>
#include <avr/sleep.h>

//1 para sincronizar hora del RTC con el sistema
#define set_time 0

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
#define bclk_pin 16
#define button_pin 3

//Intervalo para lectura de datos (días, horas, minutos, segundos)
TimeSpan interval = TimeSpan(0, 0, 1, 0);
int count = 1;
DateTime time_now;
float tt;
float hh;
unsigned long previous_millis;
volatile bool write = false;
volatile bool lcd_on = false;

LiquidCrystal lcd(rs, rw, en, d4, d5, d6, d7);
DHT dht(dht_pin, DHT22);
RTC_DS3231 rtc;

void int00_isr() {
    sleep_disable();
    write = true;
}

void int01_isr() {
    sleep_disable();
    lcd_on = true;
}

void sleep_time() {
    noInterrupts();
    set_sleep_mode(SLEEP_MODE_PWR_DOWN);
    sleep_enable();
    interrupts();
    sleep_cpu();
}

void setup() {
    pinMode(bclk_pin, OUTPUT);
    digitalWrite(bclk_pin, HIGH);
    lcd.begin(16, 2);
    dht.begin();
    rtc.begin();

#if set_time == 1
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
#endif

    if (!SD.begin(sd_pin)) {
        lcd.print("SD?");
        while(1);
    }
    if (SD.exists("datalog.csv")) {
        lcd.print("Archivo existe!");
        while(1);
    }
    //Header para el csv
    File datalog = SD.open("datalog.csv", FILE_WRITE);
    datalog.println("numero,fecha,hora,temperatura,humedad");
    datalog.close();

    //En "OFF", pin sqw está normalmente alto y baja cuando la alarma "suena"
    rtc.writeSqwPinMode(DS3231_OFF);
    //Asegurando que no hay alarmas
    rtc.disableAlarm(1);
    rtc.disableAlarm(2);
    rtc.clearAlarm(1);
    rtc.clearAlarm(2);
    //Intervalo para la primer medición, se puede usar uno más corto, por ejemplo
    //un minuto: rtc.SetAlarm1(time_now + TimeSpan(0, 0, 1, 0), DS3231_A1_Second)
    //Las siguientes mediciones continuan con el intervalo elegido en "interval"
    rtc.setAlarm1(rtc.now() + TimeSpan(0, 0, 0, 3), DS3231_A1_Second);
    
    pinMode(int_pin, INPUT_PULLUP);
    pinMode(button_pin, INPUT_PULLUP);
    //Modo FALLING para evitar que la interrupción se active al desacativar la alarma
    attachInterrupt(digitalPinToInterrupt(int_pin), int00_isr, FALLING);
    attachInterrupt(digitalPinToInterrupt(button_pin), int01_isr, FALLING);
    
}

void loop() {
    if (write) {
        time_now = rtc.now();
        tt = dht.readTemperature();
        hh = dht.readHumidity();
        rtc.disableAlarm(1);
        rtc.clearAlarm(1);
        rtc.setAlarm1(time_now + interval, DS3231_A1_Second);

        File datalog = SD.open("datalog.csv", FILE_WRITE);
        datalog.print(count);
        datalog.print(",");
        datalog.print(time_now.timestamp(DateTime::TIMESTAMP_DATE));
        datalog.print(",");
        datalog.print(time_now.timestamp(DateTime::TIMESTAMP_TIME));
        datalog.print(",");
        datalog.print(tt);
        datalog.print(",");
        datalog.println(hh);
        datalog.close();

        write = false;
        count++;
    }

    if (lcd_on) {
        tt = dht.readTemperature();
        hh = dht.readHumidity();
        lcd.setCursor(0, 0);
        lcd.print("T: ");
        lcd.print(tt);
        lcd.print(" C");
        lcd.setCursor(0, 1);
        lcd.print("H: ");
        lcd.print(hh);
        lcd.print(" %");
        digitalWrite(bclk_pin, HIGH);
        previous_millis = millis();
        lcd_on = false;
    }
    else if (millis() - previous_millis >= 3000) {
        digitalWrite(bclk_pin, LOW);
    }
    //NO suspender si el LCD está encendido o hay mediciones pendientes
    if (digitalRead(bclk_pin) == LOW && write == false) {
        sleep_time();
    }
}
