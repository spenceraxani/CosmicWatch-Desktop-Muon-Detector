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
#include <TimerOne.h>  

const byte OLED = 1;                    // Turn on/off the OLED [1,0] (Set to 0 to improve time)
const byte LED  = 1;                    // Turn on/off the LED delay [1,0] (Set to 0 to improve time)
const int SIGNAL_THRESHOLD = 25;        // Min threshold to trigger on
const int LED_BRIGHTNESS = 255;         // Change the brightness on the LED [0-255]. 5 is a dim value.

//Calibration fit data
const float cal[] = {-5.5098831352873452E-14, 1.3827917597333878E-10, -1.2623647111629911E-07, 5.385584258312143E-05, -0.010166454350597643, 1.0023837059046123, 6.4910446124360828};

//INTERUPT SETUP
#define TIMER_INTERVAL 1000000          // Every 1,000,000 us the timer will update the clock on the screen

//OLED SETUP
#define OLED_RESET 10             
Adafruit_SSD1306 display(OLED_RESET);   // Setup the OLED

//initialize variables
long int measurement_t1               = 0L;      // Time stamp
long int measurement_t2               = 0L;      // Time stamp
int start_time                        = 0L;      // Start time reference variable
long int OLED_t1                      = 0L;      // Time stamp
long int OLED_t2                      = 0L;      // Time stamp
long int total_deadtime               = 0L;      // total time between signals
int serial_time = 0;
unsigned int OLED_time        = 0L;              // time from the time it takes to update the OLED
unsigned int measurement_time = 0L;              // time from the time it takes to perform the measuremnt
long int count                    = -1L;         // A tally of the number of muon counts observed
float ADC_measurement              = 0;

void setup() {
  Serial.begin(9600);
  ADCSRA &= ~(bit (ADPS0) | bit (ADPS1) | bit (ADPS2));    // clear prescaler bits
  ADCSRA |= bit (ADPS1);                                   // Set prescaler to 4  
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);               // Begin the OLED screen
  display.setRotation(2);                       // 2 = upside down; 0 = rightside down
  
  if (OLED == 1){
    OpeningScreen();
    delay(2000); 
    display.setTextSize(1);
  }

pinMode(3, OUTPUT);                            // Setup the LED pin to output
Timer1.initialize(TIMER_INTERVAL);             // Initialise timer 1
Timer1.attachInterrupt(timerIsr);              // attach the routine that updates the OLED every second
start_time = millis();                            // setup reference time
getTime();                                     // Update the OLED
}

void loop() {
  while (1) {
    if (analogRead(A0) > SIGNAL_THRESHOLD) 
        {
        int A0_1 = analogRead(A0);            //Make three measurements. These three measurements take 5.8x3 = 17.4 microseconds
        int A0_2 = analogRead(A0); 
        int A0_3 = analogRead(A0); 
               
        measurement_t1 = millis() - start_time;  // Get the time stamp.        
        noInterrupts();                       // Turn of the interupts, so that it doesn't update the OLED screen mid measuremnt
        analogWrite(3, LED_BRIGHTNESS);
        count++;                              // Increment the count

        ADC_measurement = (A0_1 + A0_2 + A0_3) / 3.;  // Take the average measurement

        float SiPM_voltage = Calibration_fit(ADC_measurement); // Convert measured value to a SiPM voltage using the calibration.
        measurement_t2 = millis() - start_time;
        total_deadtime += (measurement_t2 - measurement_t1);
        
        if (count > 0){      
          Serial.println((String)count + " " + measurement_t1 + " " + ADC_measurement+" " + SiPM_voltage+" " + ((measurement_t2 - measurement_t1) + OLED_time + serial_time));
        }
        
        OLED_time = 0;                       // Set the OLED time back to zero
        interrupts();                        // Allow for interupts to update OLED       
        delay(10);                           // Delay at least 10ms to allow the pusle to decay away.                           
        if (LED == 1){
          delay(ADC_measurement);}
        digitalWrite(3, LOW);
        serial_time = (millis()-start_time) - measurement_t2;
        total_deadtime += serial_time;  
        }
    }
}

//TIMER INTERUPT
void timerIsr(){
  interrupts();
  if (OLED == 1){
    getTime();
  }
  noInterrupts();
}

//Update OLED screen. 
void getTime(){
  long int OLED_t1                = (millis()-start_time);
  display.setCursor(0, 0);
  float count_average                   = 0;
  float count_std                       = 0;
  long int OLED_t2                = (millis()-start_time);
  total_deadtime += OLED_t2 - OLED_t1;
  
  if ((OLED_t2 - total_deadtime) > 0){
    count_average = count / ((OLED_t2 - total_deadtime) / 1000.);
    count_std = sqrt(count) / ((OLED_t2 - total_deadtime)/ 1000.);
  }
  else {
    count_average   = 0;
    count_std       = 0;
   }
   
  display.clearDisplay();
  display.print(F("Total Count: "));
  display.println(count);
  display.print(F("Uptime: "));
  
  int minutes     = (OLED_t2 / 1000 / 60) % 60;
  int seconds     = (OLED_t2 / 1000) % 60;
  char     min_char[4];
  char     sec_char[4];
  sprintf(min_char,"%02d", minutes);
  sprintf(sec_char,"%02d", seconds);
  
  display.println((String) (OLED_t2 / 1000 / 3600) + ":" + min_char + ":" + sec_char);
  
  if (count < 0){ 
    display.println(F("Initiallizing..."));
  }
  else{
    if (ADC_measurement > 800){
      display.print(F("===---- WOW! ----==="));
    }
    else {
      for (int i = 1; i <=  (ADC_measurement + 40) / 40; i++){
      display.print(F("-"));
      }
    }
    display.println(F(""));
  }
  char     tmp_std[4];
  char     tmp_average[4];
  dtostrf(count_average, 1, 3, tmp_average);
  dtostrf(count_std, 1, 3, tmp_std);
  display.print(F("Rate: "));
  display.print((String)tmp_average);  
  display.print(F("+/-"));
  display.println((String)tmp_std);  
  display.display();
  OLED_time           += (millis()-start_time) - OLED_t1;
  total_deadtime      += (millis()-start_time) - OLED_t2;
}

// This function runs the splash screen as you turn on the detector
void OpeningScreen(void) {
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor(8,0);
  display.clearDisplay();
  display.print(F("Cosmic \n     Watch"));
  display.display();
  display.clearDisplay();
}

// This function converts the measured ADC value to a SiPM voltage
float Calibration_fit(float x){
  return cal[0]*pow(x,6)+ cal[1]*pow(x,5)+cal[2]*pow(x,4)+cal[3]*pow(x,3)+cal[4]*pow(x,2)+cal[5]*pow(x,1)+cal[6];
}
