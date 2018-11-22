/** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * A small program to read and display the DRO output of a digital tensiometer for bicycle spokes.
 * Two measurements are sent to a 16x2 character LCD:
 * 1) The gauge's raw measurement (as hundreths of millimeters), mimicking the device's own LCD.
 * 2) The corresponding tension in the spoke, based on a formula derived from a series of controlled
 * sample measurements with the current types of spoke during a previously performed calibration.
 * 
 * Most code is by Martin Bergman <bergman dot martin at gmail dot com> 2016.
 * License: CC BY-SA 4.0 - http://creativecommons.org/licenses/by-sa/4.0/legalcode
 *
 * DRO hookup to an Arduino Pro Mini 5V:
 * PIN 1  V+:  1V6
 * PIN 2 CLK:  D2 (INT0)
 * PIN 2 DAT:  D5
 * PIN 4  V-:  GND
 *
 * LCD1602 I2C V1 pinout:
 * PIN 1: GND
 * PIN 2: +5V
 * PIN 3: SDA  A4
 * PIN 4: SCL  A5
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include <Wire.h> 
#include <LiquidCrystal_I2C.h>      // That's the YWROBOT Library 1.1 "LiquidCrystal_I2C2004V1"
#include "spoke_defines_234x18x20.h" // Change this to a pertinent spoke definition file

//===============================================================================================
#define  CLK_PIN    2       // DRO clock line (amplified) on pin D2 (INT0)
#define  DAT_PIN    5       // DRO data line (amplified) on pin D5
#define  MAXJUMP    50      // Value must accomodate changing, legitimate measurements @ 3Hz
#define  NUMSAMPL   4       // Number of raw sample readings to average (select a power of 2)
#define  BIN_DIV    2       // Number of samples expressed as number of bit-shifts for division
#define  NUDGER     7       // Small value to "nudge" the output in line with "true" displacement
//===============================================================================================

LiquidCrystal_I2C  lcd(0x27, 16, 2);    // Set the LCD address to 0x27 for a 16 chars 2 line display

volatile uint8_t    gogo        = 0;    // Flag signalling a new complete mesurement data sample
        uint16_t    value       = 0;    // Input packet accumulator variable
volatile int16_t    final_value = 0;    // Ditto when ready for primary output

        uint16_t    bits_so_far, prev_value = 0;
        uint16_t    corr_value  = 0;    // The corrected value for estimated tension (dN)
        uint16_t    tweaked_val = 0 ;   // Tweak to resemble device display's "true" displacement val
        uint8_t     skipped     = 0;    // Flag (for glitch filter tracking)
        uint32_t    latest_interrupt = 0;
        uint16_t    buffer[NUMSAMPL];   // the circular buffer with samples from the DRO
        uint8_t     buf_index   = 0;    // the index of the current sample
        uint16_t    total       = 0;    // the running total
        uint16_t    average     = 0;    // the running average
        


void setup()
{
    pinMode(CLK_PIN, INPUT);                    // DRO clock signal => D2 (INT0)
    pinMode(DAT_PIN, INPUT);                    // DRO data signal  => D5

    lcd.init(); lcd.init();                     // Init LCD twice...
    lcd.backlight();                            // Turn on the light
    lcd.setCursor(0,0);   lcd.print(ELBOW);     // Display spoke dimension @ elbow (head, neck)
    lcd.setCursor(6,0);   lcd.print("|");
    lcd.setCursor(0,1);   lcd.print(TRUNK);     // Display spoke dimension @ trunk (mid-section)
    lcd.setCursor(6,1);   lcd.print("|");
    lcd.setCursor(13,1);  lcd.print(" dN");     // Using decanewtons because "kg-force" is so corny

    delay(500);

    attachInterrupt(0, cccClk, FALLING);        // Interrupt #0 maps to pin D2
    
    for (uint8_t i = 0; i < NUMSAMPL; i++) {    // Initialize all the buffer array values to 0
        buffer[i] = 0;
    }
}


void loop()
{
    if (gogo){                                  // A new DRO packet has been flagged already!
        filters();                              // Weeding out any flagrant glitches first

        prev_value = final_value;               // Prepare next-round filter

        tweaked_val = schmoozer(final_value);   // Smooth it and tweak it
        corr_value = mapIt(tweaked_val);        // Get corrected value signifying spoke's tension

        lcd.setCursor(7, 0);        lcd.print("        ");  // Clearing eight spaces
        int cp = alignCursor(tweaked_val);
        lcd.setCursor(8 + cp, 0);   lcd.print(tweaked_val); // Right-aligning 0-999
        lcd.setCursor(7, 1);        lcd.print("       ");
        if (corr_value < 45 || corr_value > 1000){          // Value is off the scale on low side ...
            lcd.setCursor(8, 1);    lcd.print("<<<  ");     // ... and possibly "rolled-under"
        }
        else if (corr_value > 145){                         // Value is off the scale on high side
            lcd.setCursor(8, 1);    lcd.print("  >>>");
        }
        else {
            cp = alignCursor(corr_value);
            lcd.setCursor(8 + cp, 1); lcd.print(corr_value);
        }
        
        final_value = 0;                        // Tidy up again
        gogo = 0;                               // Here we gogo... not
    }
}

/// //////////////////////////////////////////////////////////////////////////////////////////////

void filters(void) {
    
    int16_t nextStep = 0;

    if(final_value > 3200) {                     // Implicating a general rollover (negative) value
        final_value = 0;                         // Simply hide negative values for now
    }
    nextStep = int16_t(final_value - prev_value);// Value volatility check
    
    if((!skipped) && (abs(nextStep) > MAXJUMP)){ // Previous packet's value was deemed legit, but not this.
        skipped = 1;                             // Note-to-ourselves: Skipped the received value this time
        final_value = prev_value;                // Substitute latest known-good value
    }
    else {              // If we have come down here we should have checked the obvious glitches, ...
        skipped = 0;    // ... and can accept the latest proposed value by clearing the skipped-flag
    }
}


uint16_t schmoozer(uint16_t inpt) {

    total = total - buffer[buf_index];  // Subtract the last reading from total
    buffer[buf_index] = inpt;           // Insert latest input value into buffer
    total = total + buffer[buf_index];  // Add it to the total
    buf_index = buf_index + 1;          // Advance to the next position in the buffer array
    
    if (buf_index >= NUMSAMPL){         // If we're at the end of the array...
        buf_index = 0;                  // ...wrap around to the beginning
    }
    average = (uint16_t)total >> BIN_DIV; // Calculate new average. Divide by right-shifting
    
    if(average < 100){
        return average;                 // Just so we can display a correct zero point at rest
    }
    else {
        return average - NUDGER;        // To better resemble the "official" displacement
    }
}


uint16_t mapIt(uint16_t y) {            // Translate deflection value y to calibrated tension value x

    uint16_t x = (y - Y_OFFSET) * GRADE;
    return x;                              
}


int16_t alignCursor(uint16_t val) {     // For right-aligning the LCD display figures 

    if     (val < 10) return 3;
    else if(val < 100) return 2;
    else if(val < 1000) return 1;
    else return 0;
}


void cccClk(void) {       // This ISR marches to the beat of the "Cheap Chinese Calipers" clock!

    uint8_t data = 0;                   // First things first: Grab the present data bit
    if(PIND & B00100000){ data = 1;}    // Mask port D bit 5 (pin D5): Is it True yet? ...
                                        // ... If it isn't: "Move on! Nothing-to-see-here"
    uint32_t now = millis();            // Time-stamp

    if((now - latest_interrupt) > 5){   // This should be sufficient to distinguish arrival of a new packet
        final_value = value;            // Hand off pending measurement data to the main loop
        gogo = 1;                       // Come-and-get-it!
        value = 0;                      // Purge all them old bits in preparation for a new packet
        bits_so_far = 0;                // Reset tick-tock counter (it will increment instantly anyway)
    }
    else {
        if (bits_so_far > 25 && bits_so_far < 42 ){ // Want the first 16 of the last 24 reversed order bits
            if (data == 0){          // if DAT_PIN is LOW we record a HIGH (for an immediate inversion) ...
                value |= 0x8000;     // ... by setting the most significant bit ...
            }
        value = value >> 1;          // ... and right-shifting each pass, reversing bit order to MSB-first.
        }
    }
    bits_so_far++;
    latest_interrupt = now;          // latest it may be... but probably not the last!
}

