/*
Desktop Muon Counter Arduino Code
Questions?
Spencer N. Axani 
saxani@mit.edu

Requirements: Sketch->Include->Manage Libraries:
  1. Adafruit SSD1306     -- by Adafruit Version 1.0.1
  2. Adafruit GFX Library -- by Adafruit Version 1.0.2
  3. TimerOne             -- by Jesse Tane et al. Version 1.1.0
*/

#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h>
#include <Wire.h>
#include <SPI.h>
#include <TimerOne.h>  
#include <EEPROM.h>

const byte OLED = 1;                    // Turn on/off the OLED [1,0] (Set to 0 to improve time)
const byte LED  = 0;                    // Turn on/off the LED delay [1,0] (Set to 0 to improve time)
const int SIGNAL_THRESHOLD = 180;        // Min threshold to trigger on
const int RESET_THRESHOLD = 20;          // Threshold to reset for next trigger
const int LED_BRIGHTNESS = 255;          // Change the brightness on the LED [0-255]. 5 is a dim value.


//This calibration is for 499, 5pF, 10k
//const float cal[] = {4.0614902396231111e-14, -7.8414735932155768e-11, 5.6819206951234577e-08, -1.7873823684604088e-05, 0.0029455520399522652, 0.01701073093454207, 26.268331951783924};
//This calibration is for 499, 5pF, 10k
const float cal[] = {9.038130548712698e-15, -1.4599909946491252e-12, -1.2626490003703338e-08, 1.1406923013075114e-05, -0.002891003749200569, 0.5003740521722947, 15.615470780166067};


//INTERUPT SETUP
#define TIMER_INTERVAL 1000000          // Every 1,000,000 us the timer will update the clock on the screen

//OLED SETUP
#define OLED_RESET 10             
Adafruit_SSD1306 display(OLED_RESET);   // Setup the OLED


//initialize variables
char detector_name[40];

unsigned long time_stamp                      = 0L;
unsigned long measurement_deadtime            = 0L;
unsigned long time_measurement                = 0L;      // Time stamp
unsigned long interrupt_timer                 = 0L;      // Time stamp
unsigned long start_time                      = 0L;      // Reference time for all the time measurements
unsigned long total_deadtime                  = 0L;      // total measured deadtime
unsigned long waiting_t1                      = 0L;
unsigned long measurement_t1;
unsigned long measurement_t2;
float sipm_voltage =0;
unsigned long count                           = 0L;      // A tally of the number of muon counts observed
float last_sipm_voltage                          = 0;

byte waiting_for_interupt = 0;

void setup() {
  ADCSRA &= ~(bit (ADPS0) | bit (ADPS1) | bit (ADPS2));    // clear prescaler bits
  ADCSRA |= bit (ADPS1);                                   // Set prescaler to 4  
  Serial.begin(9600);
  
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);               // Begin the OLED screen
  
  if (OLED == 1)
  {
      display.setRotation(2);                       // 2 = upside down; 0 = rightside down
      OpeningScreen();
      delay(2000); 
      display.setTextSize(1);
  }
  
  pinMode(3, OUTPUT);
  
  get_detector_name(detector_name);
  Serial.println(detector_name);
  
  analogRead(A0);
  
  start_time = millis();
  Timer1.initialize(TIMER_INTERVAL);             // Initialise timer 1
  Timer1.attachInterrupt(timerIsr);              // attach the ISR routine
}

// The loop runs continuously, looking for a signal to pass SIGNAL_THRESHOLD
void loop() 
{
  while (1) 
  {
    if (analogRead(A0) > SIGNAL_THRESHOLD)
    { 
      count++;      
      time_stamp = millis() - start_time; 
      float avg_adc_measurement = (analogRead(A0) + analogRead(A0) + analogRead(A0) + analogRead(A0)) / 4.;
      measurement_deadtime = total_deadtime;
      
      if((interrupt_timer + 1000 - millis()) < 15)
      { 
          waiting_t1 = millis();
          waiting_for_interupt = 1;
          delay(30);
          waiting_for_interupt = 0;
      }
      measurement_t1 = micros();
      analogWrite(3, LED_BRIGHTNESS);
      sipm_voltage = get_sipm_voltage(avg_adc_measurement);
      last_sipm_voltage = sipm_voltage; 
      Serial.println((String)count + " " + time_stamp+ " " + avg_adc_measurement+ " " + sipm_voltage + " " + total_deadtime);

      
      digitalWrite(6, LOW);  
      digitalWrite(3, LOW);
      // Wait for signal to fall back down
      while((analogRead(A0)+analogRead(A0)+analogRead(A0))/3. > RESET_THRESHOLD){continue;}
      
      total_deadtime += (micros() - measurement_t1 + 1062) / 1000.;
                       
    }
  }
}
  

//TIMER INTERUPT
void timerIsr() 
{
  interrupts();
  interrupt_timer                       = millis();
  if (waiting_for_interupt == 1){total_deadtime += (millis() - waiting_t1);}
  waiting_for_interupt = 0;
  if (OLED == 1){get_time();}
}

// This function updates the OLED screen with the stats.
void get_time() {
  unsigned long int OLED_t1        = micros();
  float count_average         = 0;
  float count_std             = 0;

  if (count > 0.) 
  {
  count_average = count / ((interrupt_timer - start_time - total_deadtime) / 1000.);
  count_std     = sqrt(count) / ((interrupt_timer - start_time - total_deadtime) / 1000.);
  }
   else 
  {
      count_average   = 0;
      count_std       = 0;
  }
  
  display.setCursor(0, 0);
  display.clearDisplay();
  display.print(F("Total Count: "));
  display.println(count);
  display.print(F("Uptime: "));

  int minutes                 = ((interrupt_timer- start_time) / 1000 / 60) % 60;
  int seconds                 = ((interrupt_timer- start_time) / 1000) % 60;
  char min_char[4];
  char sec_char[4];
  
  sprintf(min_char, "%02d", minutes);
  sprintf(sec_char, "%02d", seconds);

  display.println((String) (interrupt_timer / 1000 / 3600) + ":" + min_char + ":" + sec_char);

  if (count == 0) 
  {
      display.println(F("Initializing..."));
  }
  
  else 
  {
      if (last_sipm_voltage > 180) 
      {
          display.print(F("===---- WOW! ----==="));
      }

      else 
      {
       for (int i = 1; i <=  (last_sipm_voltage + 10) / 10; i++) {display.print(F("-"));}
      }
      display.println(F(""));
  }

  char tmp_average[4];
  char tmp_std[4];

  int decimals = 2;
  if (count_average < 10) {decimals = 3;}
  
  dtostrf(count_average, 1, decimals, tmp_average);
  dtostrf(count_std, 1, decimals, tmp_std);
   
  display.print(F("Rate: "));
  display.print((String)tmp_average);
  display.print(F("+/-"));
  display.println((String)tmp_std);
  display.display();
  
  total_deadtime                      += (micros() - OLED_t1 +73.)/1000.;
}


// This function runs the splash screen as you turn on the detector
void OpeningScreen(void) {
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor(8, 0);
  display.clearDisplay();
  display.print(F("Cosmic \n     Watch"));
  display.display();
  display.setTextSize(1);
  display.clearDisplay();
}

// This function converts the measured ADC value to a SiPM voltage
//float get_sipm_voltage(float x) 
//{
//    return cal[0] * pow(x, 6) + cal[1] * pow(x, 5) + cal[2] * pow(x, 4) + cal[3] * pow(x, 3) + cal[4] * pow(x, 2) + cal[5] * pow(x, 1) + cal[6];
//}

float get_sipm_voltage(float adc_value)
{
  float voltage = 0;
  for (int i = 0; i < (sizeof(cal)/sizeof(float)); i++) {
    voltage += cal[i] * pow(adc_value,(sizeof(cal)/sizeof(float)-i-1));
    }
    return voltage;
}


// This function reads the first 40 characters written to the EEPROM, where the name is stored
// To change the name of the detector, upload the Naming.ino Arduino code.
boolean get_detector_name(char* det_name) 
{
    byte ch;                              // byte read from eeprom
    int bytesRead = 0;                    // number of bytes read so far
    ch = EEPROM.read(bytesRead);          // read next byte from eeprom
    det_name[bytesRead] = ch;               // store it into the user buffer
    bytesRead++;                          // increment byte counter

    while ( (ch != 0x00) && (bytesRead < 40) && ((bytesRead) <= 511) ) 
    {
        ch = EEPROM.read(bytesRead);
        det_name[bytesRead] = ch;           // store it into the user buffer
        bytesRead++;                      // increment byte counter
    }
    if ((ch != 0x00) && (bytesRead >= 1)) {det_name[bytesRead - 1] = 0;}
    return true;
}

