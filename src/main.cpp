/*
 * main.cpp
 * Author/s: Lucas J. Mina. Dario D. Larrea & Miriam P. Damborsky
 * Description: Code for a humidity and temperatura detalogger prototipe using a DHT sensor and
 * Arduino. Codename "Oreo".
 * 
 * TODO:
 * - Sleep "better" for more energy saving https://www.gammon.com.au/power
 * - Set a limit for measurement using a counter or date/time (RTC's alarm 2 maybe?)
 * - Find a way to check that sensors and other modules are connected and working correctly.
 */

#include <Arduino.h>
#include <Wire.h>
#include <RTClib.h>
#include <LiquidCrystal.h>
#include <DHT.h>
#include <SdFat.h>
#include <SPI.h>
#include <avr/sleep.h>

//1 for sincronizing RTC time with system's
#define set_time 0

//LCD pins definition
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

//Measurement taking interval (days, hours, minutes, seconds)
TimeSpan interval = TimeSpan(0, 0, 1, 0);
int count = 1;
DateTime time_now;
float tt;
float hh;
unsigned long previous_millis;
volatile bool write = false;
volatile bool lcd_on = false;
char filename[27] = "YYYYMMDDhhmmss_DATALOG.csv";

LiquidCrystal lcd(rs, rw, en, d4, d5, d6, d7);
DHT dht(dht_pin, DHT22);
RTC_DS3231 rtc;
SdFat SD;

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

    //Unique name for new files using timestamp (YYYYMMDDhhmmss_DATALOG.csv)
    time_now = rtc.now();
    time_now.toString(filename);

    //CSV header
    File datalog = SD.open(filename, FILE_WRITE);
    datalog.println("number,date,time,temperature,humidity");
    datalog.close();

    //When "off" SQW pin is normally high and goes low when alarm "rings"
    rtc.writeSqwPinMode(DS3231_OFF);
    //Making sure no alarms are set
    rtc.disableAlarm(1);
    rtc.disableAlarm(2);
    rtc.clearAlarm(1);
    rtc.clearAlarm(2);
    //Time interval for first measurement after powering on. Following measurements
    //use the time from variable "interval"
    rtc.setAlarm1(rtc.now() + TimeSpan(0, 0, 0, 3), DS3231_A1_Second);
    
    pinMode(int_pin, INPUT_PULLUP);
    pinMode(button_pin, INPUT_PULLUP);
    //FALLING mode so interrupt doesn't trigger when turning off the alarm
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

        File datalog = SD.open(filename, FILE_WRITE);
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
    //Do NOT suspend MCU if LCD backlight is on or there are pending measurements
    if (digitalRead(bclk_pin) == LOW && write == false) {
        sleep_time();
    }
}
