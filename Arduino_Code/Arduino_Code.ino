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

const byte OLED = 1;                    // Turn on/off the OLED [1,0] (Set to 0 to improve deadtime)
const byte LED  = 1;                    // Turn on/off the LED delay [1,0] (Set to 0 to improve deadtime)
const int SIGNAL_THRESHOLD = 25;        // Min threshold to trigger on

       
//INTERUPT SETUP
#define TIMER_INTERVAL 1000000          // Every 1,000,000 us the timer will update the clock on the screen

//OLED SETUP
#define OLED_RESET 10             
Adafruit_SSD1306 display(OLED_RESET);   // Setup the OLED

//initialize variables
long measurement_t1               = 0L;      // Time stamp
long measurement_t2               = 0L;      // Time stamp

long t_start                      = 0L;      // Start time reference variable
long total_deadtime               = 0L;      // total deadtime between signals
unsigned int OLED_deadtime        = 0L;      // Deadtime from the time it takes to update the OLED
unsigned int measurement_deadtime = 0L;      // Deadtime from the time it takes to perform the measuremnt
long int count                    = -1L;     // A tally of the number of muon counts observed
float measurement_y1              = 0;

void setup() {
  Serial.begin(9600);
  ADCSRA &= ~(bit (ADPS0) | bit (ADPS1) | bit (ADPS2));    // clear prescaler bits
  ADCSRA |= bit (ADPS1);                                   // Set prescaler to 4  
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);               // Begin the OLED screen
 
  if (OLED == 1){
    display.setTextSize(1);
    display.setRotation(2); //upside down
    OpeningScreen();
    delay(2000); 
    display.setTextSize(1);
  }


pinMode(3, OUTPUT);                            // Setup the LED pin to output
Timer1.initialize(TIMER_INTERVAL);             // Initialise timer 1
Timer1.attachInterrupt(timerIsr);              // attach the routine that updates the OLED every second
t_start = millis();                            // setup reference time
getTime();                                     // Update the OLED
}

void loop() {
  while (1) {
    if (analogRead(A0) > SIGNAL_THRESHOLD) 
        {
        int A0_1 = analogRead(A0);            //Make three measurements. These three measurements take 5.8x3 = 17.4 microseconds
        int A0_2 = analogRead(A0); 
        int A0_3 = analogRead(A0); 
               
        measurement_t1 = millis() - t_start;  // Get the time stamp.        
        noInterrupts();                       // Turn of the interupts, so that it doesn't update the OLED screen mid measuremnt
        digitalWrite(3, HIGH);                // Turn on the LED
        count++;                              // Increment the count
        
        measurement_y1 = (A0_1 + A0_2 + A0_3) / 3.;  // Take the average measurement

        // Model = N1 * (1-np.exp(-c1 * (x-a1))) + b1
        // From calibration data
        float N1 = 868.211092;
        float a1 = 0.717076153;
        float b1 = -15.1376897;
        float c1 = 0.00380320728;

        float SiPM_voltage1 = log(1-(measurement_y1 - b1)/N1)/(-c1) + a1; // Convert measured value to a SiPM voltage using the calibration.
                
        total_deadtime += (millis() - t_start - measurement_t1) + OLED_deadtime + measurement_deadtime; // Total deadtime from last measurement
        
        // Send the information through the serial port.
        if (count > 0){      
          Serial.println((String)count + " " + measurement_t1 + " " +measurement_y1+" "+SiPM_voltage1+" " + (measurement_deadtime + OLED_deadtime));
        }
        
        OLED_deadtime = 0;                  // Set the OLED deadtime back to 
        interrupts();                       // Allow for interupts to update OLED       
        delay(10);                          // Delay at least 1ms to allow the pusle to decay away.                           
        if (LED == 1){
          delay(measurement_y1);}
        digitalWrite(3, LOW);
        measurement_deadtime = (millis()-t_start) - total_deadtime;
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
  long int t1               = (millis()-t_start);
  display.setCursor(0, 0);
  float average               = count / ((t1 - total_deadtime) / 1000.);
  float stdev                 = sqrt(count) / ((t1 - total_deadtime)/ 1000.);
  
  display.clearDisplay();
  display.print(F("Total Count: "));
  display.println(count);
  display.print(F("Uptime: "));
  
  int minutes = (t1 / 1000 / 60) % 60 ;
  int seconds =  (t1 / 1000) % 60;
  char     min_char[4];
  char     sec_char[4];
  sprintf(min_char,"%02d", minutes);
  sprintf(sec_char,"%02d", seconds);
  
  display.println((String) (t1 / 1000 / 3600) + ":" + min_char + ":" + sec_char);
  
  if (measurement_y1 == 0){ 
    display.println(F("Initiallizing..."));
  }
  else{
    if (measurement_y1 > 800){
      display.print(F("===---- WOW! ----==="));
    }
    else {
      for (int i = 1; i <=  (measurement_y1 + 40) / 40; i++){
      display.print(F("-"));
      }
    }
    display.println(F(""));
  }
  char     tmp2[4];
  char     tmp[4];
  dtostrf(average, 1, 3, tmp);
  dtostrf(stdev, 1, 3, tmp2);
  display.print(F("Rate: "));
  display.print((String)tmp);  
  display.print(F("+/-"));
  display.println((String)tmp2);  
  display.display();
  OLED_deadtime += ((millis()-t_start)-(t1));
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

