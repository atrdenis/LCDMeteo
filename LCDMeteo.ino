// Actual version on https://github.com/atrdenis/LCDMeteo
// Interfacing Arduino with DHT22 humidity/temperature sensor and BMP280 pressure/temperature sensor.
// The results are displayed on the LCD screen 1602 with I2C.
// Сhange in pressure predicts change in weather
// Weather prediction used functions from AlexGyver github repositories https://github.com/AlexGyver/WeatherPredict and https://github.com/AlexGyver/MeteoClock
#include "DHT.h" // include DHT library code
#define DHTPIN 8            // DHT22 data pin is connected to Arduino pin 8
#define DHTTYPE DHT22       // DHT22 sensor is used
DHT dht(DHTPIN, DHTTYPE);   // Initialize DHT library

#include <Adafruit_BMP280.h> // BMP280 library
#include <SPI.h> // library for SPI communication

//SPI parameters for BMP280 connection
#define BMP_SCK 13
#define BMP_MISO 12
#define BMP_MOSI 11 
#define BMP_CS 10
Adafruit_BMP280 bme(BMP_CS);

#include <Wire.h>  // library for I2C communication
#include <LiquidCrystal_I2C.h> // library for I2C communication with LCD
LiquidCrystal_I2C lcd(0x27,16,2);  // Set address and params for LCD 1602

byte arrowUp[] = { // LCD params for up arrow character
  B00100,
  B01110,
  B10101,
  B00100,
  B00100,
  B00100,
  B00100,
  B00100
};
byte arrowDown[] = { // LCD params for down arrow character
  B00100,
  B00100,
  B00100,
  B00100,
  B00100,
  B10101,
  B01110,
  B00100
};
byte smileUp[] = { // LCD params for funny smiley character
  B00000,
  B00000,
  B01010,
  B00000,
  B10001,
  B01110,
  B00000,
  B00000
};
byte smileDown[] = { // LCD params for sad smiley character
  B00000,
  B00000,
  B01010,
  B00000,
  B00000,
  B01110,
  B10001,
  B00000
};
byte drop[] = { // LCD params for drop character
  B00100,
  B00100,
  B01110,
  B01110,
  B11111,
  B11111,
  B11111,
  B01110
};
byte sun[] = { // LCD params for sun character
  B00000,
  B00100,
  B10101,
  B01110,
  B11111,
  B01110,
  B10101,
  B00100
};

boolean calculate_flag; // flag for performing weather prediction calculations
int time_count = 0, angle, delta, dispRain;
unsigned long pressure, aver_pressure, pressure_array[6], time_array[6];
unsigned long sumX, sumY, sumX2, sumXY;
float a, b;

void setup() {
  //Serial.begin(9600);
  dht.begin();
  bme.begin();
  /*if (!bme.begin()) {  
    Serial.println(F("Could not find a valid BMP280 sensor, check wiring!"));
    while (1);
  }*/
  lcd.init();
  lcd.createChar(0, arrowUp);
  lcd.createChar(1, arrowDown);
  lcd.createChar(2, smileUp);
  lcd.createChar(3, smileDown);
  lcd.createChar(4, drop);
  lcd.createChar(5, sun);
  lcd.backlight();// Turn on LCD backlight

  pressure = aver_sens();          // find the current pressure by the arithmetic mean
  for (byte i = 0; i < 6; i++) {   // counter from 0 to 5
    pressure_array[i] = pressure;  // fill array with current pressure
    time_array[i] = i;             // fill the time array with numbers 0 - 5
  }
}
 
void loop() {
delay(4000);               // wait 1s between readings
  float RH = dht.readHumidity();  // Read humidity
  float Temp = dht.readTemperature();  //Read temperature in degree Celsius
  // Check if any reads failed and exit early (to try again)
  if (isnan(RH) || isnan(Temp)) {
    lcd.clear();
    lcd.setCursor(5, 0);
    lcd.print("Error");
    return;
  }
  lcd.clear();
  lcd.setCursor(0, 0);  lcd.print("T = ");  lcd.print(Temp, 1);  lcd.print(" "); lcd.print(char(223)); lcd.print("C");
  //show arrows and smilies depending on values of temperature
  if ((Temp>=20) && (Temp<=26)) {lcd.print("    ");lcd.print((char) 0x02);}
  if (Temp<20) {lcd.print("   ");lcd.print((char) 0x01);lcd.print((char) 0x03);}
  if (Temp>26) {lcd.print("   ");lcd.print((char) 0x00);lcd.print((char) 0x03);}
  
  lcd.setCursor(0, 1);  lcd.print("H = ");  lcd.print(RH, 1);  lcd.print(" %");
  //show arrows and smilies depending on values of humidity
  if ((RH>30) && (RH<60)) {lcd.print("     ");lcd.print((char) 0x02);}
  if (RH<=30) {lcd.print("    ");lcd.print((char) 0x01);lcd.print((char) 0x03);}
  if (RH>=60) {lcd.print("    ");lcd.print((char) 0x00);lcd.print((char) 0x03);}
   //Serial.println("Data from DHT22:");
   //Serial.print(F("Temperature1 = "));    Serial.print(Temp);    Serial.println(" C");
   //Serial.print(F("Humidity = "));    Serial.print(RH);    Serial.println(" %");
   //Serial.println("");
   
delay (4000);
  float Pressure = bme.readPressure()*0.00750062; // Read Pressure in mmHg
  float Temp2 = bme.readTemperature();  //Read temperature in degree Celsius
  lcd.clear();
  lcd.setCursor(0, 1);  lcd.print("P = ");  lcd.print(Pressure, 0);  lcd.print(" mmHg");
  //show arrows and smilies depending on values of pressure
  if ((Pressure>750) && (Pressure<770)) {lcd.print("   ");lcd.print((char) 0x02);}
  if (Pressure<=750) {lcd.print("  ");lcd.print((char) 0x01);lcd.print((char) 0x03);}
  if (Pressure>=770) {lcd.print("  ");lcd.print((char) 0x00);lcd.print((char) 0x03);}
   //Serial.println("Data from BMP280:");
   //Serial.print(F("Temperature2 = "));    Serial.print(Temp2);    Serial.println(" C");
   //Serial.print(F("Pressure = "));    Serial.print(Pressure);    Serial.println(" mmHg");
   //Serial.println("");

if (calculate_flag) {
    pressure = aver_sens();                          // find the current pressure by the arithmetic mean
    for (byte i = 0; i < 5; i++) {                   // counter from 0 to 5
      pressure_array[i] = pressure_array[i + 1];     // move the pressure array EXCEPT THE LAST CELL one step back
    }
    pressure_array[5] = pressure;                    // the last element of the array is now the new pressure
    sumX = 0; sumY = 0;sumX2 = 0;sumXY = 0;
    for (int i = 0; i < 6; i++) {                    // for all array elements
      sumX += time_array[i];
      sumY += (long)pressure_array[i];
      sumX2 += time_array[i] * time_array[i];
      sumXY += (long)time_array[i] * pressure_array[i];
    }
    a = 0;
    a = (long)6 * sumXY;             // calculation of the coefficient of inclination of a straight line
    a = a - (long)sumX * sumY;
    a = (float)a / (6 * sumX2 - sumX * sumX);
    delta = a * 6;                   // pressure change calculation
    dispRain = map(delta, -250, 250, 100, -100); // conversion into the probability of worsening weather (rain or snow). If the values are negative, then there will be an improvement in the weather
     //Serial.print("Angle a = ");Serial.println(a);
     //Serial.print("DispRain = ");Serial.println(dispRain);
    //show sun or drop icons depending on the likelihood of better or worse weather
    if ((dispRain>=20) && (dispRain<40)) {lcd.setCursor(0, 0);  lcd.print((char) 0x04); lcd.print("   "); lcd.print(dispRain);  lcd.print(" %");}
    if ((dispRain>=40) && (dispRain<70)) {lcd.setCursor(0, 0);  lcd.print((char) 0x04); lcd.print((char) 0x04); lcd.print("  "); lcd.print(dispRain);  lcd.print(" %");}
    if ((dispRain>=70)) {lcd.setCursor(0, 0);  lcd.print((char) 0x04);lcd.print((char) 0x04); lcd.print((char) 0x04); lcd.print(" "); lcd.print(dispRain);  lcd.print(" %");}
    if ((dispRain>-20) && (dispRain<20)) {lcd.setCursor(0, 0);  lcd.print((char) 0x04); lcd.print((char) 0x05); lcd.print("  "); lcd.print(abs(dispRain));  lcd.print(" %");}
    if ((dispRain<=-20) && (dispRain>-40)) {lcd.setCursor(0, 0);  lcd.print((char) 0x05); lcd.print("   "); lcd.print(-dispRain);  lcd.print(" %");}
    if ((dispRain<=-40) && (dispRain>-70)) {lcd.setCursor(0, 0);  lcd.print((char) 0x05); lcd.print((char) 0x05); lcd.print("  "); lcd.print(-dispRain);  lcd.print(" %");}
    if ((dispRain<=-70)) {lcd.setCursor(0, 0);  lcd.print((char) 0x05);lcd.print((char) 0x05); lcd.print((char) 0x05); lcd.print(" "); lcd.print(-dispRain);  lcd.print(" %");}
    delay(10);                       // delay for stability
  }
  time_count++;
  if (time_count >= 70) {  //i f the running time has exceeded 10 minutes (75 times for 8 seconds - trim = 70)
  calculate_flag = 1;          // enable calculation
  time_count = 0;        // reset the counter
  delay(2);               // delay for stability
  }
  
}

// arithmetic mean of pressure
long aver_sens() {
  pressure = 0;
  for (byte i = 0; i < 10; i++) {
    pressure += bme.readPressure();
  }
  aver_pressure = pressure / 10;
  return aver_pressure;
}
