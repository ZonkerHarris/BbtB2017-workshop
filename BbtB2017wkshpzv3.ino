// Sketch:  BbtB2017wkshpzv3   2017-08-03   Zonker Harris
//   The workshop is done, now I can add more functions to the sequence!
// A simple sketch to demonstrate lighting ideas, and Arduino features
//  for Bricks by the Bay 2017, for the Low-Tech, High-Tech Lighting workshop.
// The sketch has a pushbutton to cycle through many functions for four LEDs.
//  Trinket 3.3v info:  https://www.adafruit.com/product/1500 
//  Intro to Trinket:  https://learn.adafruit.com/introducing-trinket 
//  Programming app:  https://learn.adafruit.com/adafruit-arduino-ide-setup
//  Arduino.cc learning page:  https://www.arduino.cc/en/Guide/HomePage 
//  Temperature-sensing sketch:  https://arduining.com/?s=trinket+3.3v 
//  How to read pushbuttons: http://www.gammon.com.au/interrupts
//    clues: http://www.avrfreaks.net/forum/attiny84-external-interrupt-problem

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
* The Trinket uses the USBTiny drivers to upload...
*  get ready to click the upload icon
*  press the reset button, red LED starts a fast fade... 
*  click upload icon!
*3.3v trinket is only 8 MHz,  5v trinket is 12...
*
* https://learn.adafruit.com/introducing-trinket/windows-setup  
*  Mac and Linux seem to know about this driver...
* #0 = digital-write, candle-flicker LED
*       (The candle-flicker LED probably won't respond well to PWM fading...)
* #3, #4 = analog write (PWM out to warm white LEDs) #1 is onboard LED
*       (The white LEDs work very well with PWM fading!)
* #2 = Interrupt pin?, pushbutton goes here...
* _ _ _ _ _
*  If you want to add fancy sensors, use these pins;
* #0 = i2c SDA,  #2 = i2c SCL
* #0 = SPI MOSI,  #1 = SPI MISO,  #2 = SPI SCK
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/*    "ASCII Art" schematic of the circuit...
     *** NOTE: Pin 3 is NOT capable of PWM! My bad!           
         New versions should have D2 on pin #1
          
(3v)--+--- -----------------.
      | +                  |                  
 C1  ===                 ---------       '' 
.1 uf |                 |  3v   #0|-----|>|--.  D1 Yellow Candle-Flicker
 10-v |     --------+---|#2       |      ''  |
      |    | 0.1 uf |   |       #3|-----|>|--+  D2 Warm White
      |   ===  C2   |   |(Trinket)|      ''  |
      |    |      | O   |   GND #4|-----|>|  +  D3 Warm White          
      |    |     [|      ---------           |
      |    |      | O (mode) |               |
      |    |        |        |               |
(GND)-+----+--------+--------+---------------'

*/

// We'll use "presses" as a global variable. It must be declared VOLATILE to write in an ISR!
volatile int presses = 0;  // if > 0, button was pressed, so we should increment "mode"
/* Interrupt Service Routines (ISRs) are complicated, are a bit finicky to use, and there
 *  are not many well-documented resources. I found Nick Gammon's great example page, which
 *  helped me clear this up (http://gammon.com.au/interrupts).
 * ISRs should be kept short and simple since they interrupt counting "millis", and 
 *  will impact serial writes (while the ISR is running), among other things.
 * BUT, let me add "Try this in a simple sketch first! See that it works, and then add
 *  some simple delay to the main loop, to see if that affects your sketch. See if you can 
 *  modify a variable in the ISR, or make a decision based on reading a pin, or even try to
 *  do something a little more complex, and see the ISR affect a change.
 * You *can* do some more interesting things, where you can use a range of pins, which 
 *  will trigger one interrupt, but your code needs to look at which pins have changed.
 *  This is handy for things like Rotary Encoders, I have read (but I haven't made it work...)
 */
#define PIN_SWITCH   2

int cycles = 1;
int speed = 15;
int mode = 0;  // this variable maps to the sequence that we WANT to select
int sequence = 0;  //  which sequence number is up next?

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
//  Make one LED flash like a lighthouse beacon...
int fireballCycle[] = {
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 5, 10, 15, 20, 25, 
  35, 45, 55, 65, 75, 85, 85, 85, 85, 90, 120, 150, 190, 230, 250, 
  250, 230, 190, 150, 120, 90, 85, 85, 85, 85, 75, 65, 55, 45, 35, 
  25, 20, 15, 10, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0} ;

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
//  Start fading one LED up, and then down, and pause while off...
//  At the same time, the other LEDs waits off, and then fades up & down
int overlapFade[] = {
  150, 120, 100, 80, 65, 50, 35, 25, 15, 10, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 5, 
  10, 15, 25, 35, 50, 65, 80, 100, 120, 150, 180, 200, 220, 235, 245, 250, 
  253, 254, 254, 253, 250, 245, 230, 220, 200, 180} ;

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// Interrupt Service Routine (ISR)
//  (Keep your ISRs short and simple (which means FAST!) for reliability.)
void buttonSense() {
    presses++;
}

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
void setup() {
  // put your setup code here, to run once:
  pinMode(0, OUTPUT);  // LED D1  Yellow Candle Flicker LED
  pinMode(3, OUTPUT);  // LED D2  Warm White #1
  pinMode(4, OUTPUT);  // LED D3  Warm White #2
  pinMode(PIN_SWITCH, INPUT_PULLUP);  
  attachInterrupt(0, buttonSense, FALLING); 

  presses = 0;
  mode = 0;
}

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
void loop() {
  // put your main code here, to run repeatedly:
  while (presses > 3) {
    presses = (presses - 3);
  }
  if(presses > 0) {
    mode = (mode + presses);
    presses = 0;
    if (mode > 5) {  // we've exceeded the number of animations, wrap to 0!
      mode = 0;
    }
  }
  sequence = mode;  // Use a button to step through the sequences (comment the other line)
  //sequence = 3;  // to debug, pick the pattern you are debugging, comment the other two)
  /*  sequences
   *   0 - Flicker LED
   *   1 - White LED 1 (pin #3)on full brightness
   *   2 - White LED 2 (pin #4) on full brightness
   *   3 - Slow Fireball/Lighthoue beacon with flash
   *   4 - Fast Fireball/Lighthoue beacon with flash
   *   5 - Sequence all three for "cycletime" 
   *  ===  The functions below won't work with the LED on pin #3
   *   6 - Fade whites: #1 up, #1 down, #2 up, #2 down at "fadespeed"
   *   7 - Overlap`ping Fades (whites)
   *   8 - Lightning with "strikepause" delay between strikes.
   *   9 - Wig-Wag Whites
   */
  if (sequence == 0) {
    Flicker();  // (Turn off the white LEDs, turn on the Flicker LED)
    delay(100);
    }
  else if (sequence == 1) { // White LED 1 on full brightness
    White1(); 
    delay(100);
    }
  else if (sequence == 2) { // White LED 2 on full brightness
    White2(); 
    delay(100);    
    }
  else if (sequence == 3) { // Slow Lighthouse beacon with flash
    Fireball(1,40);  // LED pin [1|2], then msec delay in the sequence steps
    }
  else if (sequence == 4) { // Fast Lighthouse beacon with flash 
    Fireball(1,20);  // LED pin [1|2], then fewer msec delay in the sequence steps
    }
  else if (sequence == 5) { // Sequence all three for "cycletime" 
    Sequence(6000); 
    }
  else if (sequence == 6) { 
    // Fade whites: #1 up, #1 down, #2 up, #2 down, "fadespeed"
    White1();  // Fade1to2(20);  //  msec delay in the sequence steps
    delay(100);
    }
  else if (sequence == 7) { // Overlapping Fade whites: one LED starts up before the other is off
    Crossfade(30);  // Overlapping fade, msec delay in the sequence steps
    }
  else if (sequence == 8) { // Lightning with "strikepause" delay btwn strikes.
    White1(); 
    delay(100);
    }
else if (sequence == 9) { // Fast Wig-wag White LEDs (wagTime delay ~ 500 msec)
    // This function allows you to control the brightness! 
    // The first variable is speed (msec between wig-wag)
    // The second variable is brightness 0 thru 255 (25 is dim, 255 is bright)
    WigWag(500, 255); 
    }
else if (sequence == 10) { // Slow Wig-wag White LEDs (wagTime delay ~ 1000 msec)
    // This function allows you to control the brightness! 
    // The first variable is speed (msec between wig-wag)
    // The second variable is brightness 0 thru 255 (25 is dim, 255 is bright)
    WigWag(1000, 150); 
    }

  else { 
    Flicker();  // (Turn off the white LEDs, turn on the Flicker LED)
    delay(500);
    digitalWrite (0, LOW);
    delay(300);
  }
}
  /*  sequences
   *   5 - Sequence all three for "cycletime" 
   *   6 - Fade whites: #1 up, #1 down, #2 up, #2 down at "fadespeed"
   *   7 - Overlapping Fades (whites)
   *   8 - Lightning with "strikepause" delay between strikes.
   *   9 - Wig-Wag Whites
   */

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
int Flicker() {
  digitalWrite (3, LOW); // make sure the other LEDs are off...
  digitalWrite (4, LOW); // this prevents damage to the trinket...
  digitalWrite (0, HIGH);
}

int White1() {
  digitalWrite (0, LOW); // make sure the other LEDs are off...
  digitalWrite (4, LOW); // this prevents damage to the trinket...
  digitalWrite (3, HIGH);
}

int White2() {
  digitalWrite (0, LOW); // make sure the other LEDs are off...
  digitalWrite (3, LOW); // this prevents damage to the trinket...
  digitalWrite (4, HIGH);
}

int Sequence(uint16_t cycletime) {
  digitalWrite (3, LOW); // make sure the other LEDs are off...
  digitalWrite (4, LOW); // this prevents damage to the trinket...
  digitalWrite (0, HIGH); // Flicker on
  delay(cycletime);

  digitalWrite (0, LOW); // Flicker off, LED 1 on
  digitalWrite (3, HIGH);
  delay(cycletime);

  digitalWrite (3, LOW); // LED 1 off, LED 2 on
  digitalWrite (4, HIGH);
  delay(cycletime);
  digitalWrite (4, LOW);
}

void Fireball(uint8_t whichLED, uint8_t fireballSpeed) {
  int fireballPin = 4;  // You can also use pin #1
  // Start the fireball
  for (int myFireball = 0; myFireball < 77; myFireball++)
  {
    analogWrite (fireballPin, fireballCycle[myFireball]);
    delay(fireballSpeed);
  }
}

void Crossfade(uint16_t Speed) {
  for (int myFadePos = 0; myFadePos < 70; myFadePos++)
  {
    int otherFadePos = (70 - myFadePos);
    analogWrite (1, overlapFade[myFadePos]);
    analogWrite (4, overlapFade[otherFadePos]);
    delay(Speed);
  }
}

void WigWag(uint16_t wagTime, uint8_t brightness) {
  // This function uses PWM, to allow you to set the brightness...
  //  We use the analogWrite command, declare the pin, and the brightness
  digitalWrite (0, LOW); // make sure the other LEDs are off...
  analogWrite (4, 0); // this prevents damage to the trinket...
  analogWrite (1, brightness);
  delay(wagTime);

  digitalWrite (0, LOW); // make sure the other LEDs are off...
  analogWrite (4, brightness); // this prevents damage to the trinket...
  analogWrite (1, 0);
  delay(wagTime);
}









