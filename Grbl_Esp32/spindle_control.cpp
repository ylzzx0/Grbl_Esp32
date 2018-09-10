/*
  spindle_control.cpp - Header for system level commands and real-time processes
  Part of Grbl
  Copyright (c) 2014-2016 Sungeun K. Jeon for Gnea Research LLC
	
	2018 -	Bart Dring This file was modified for use on the ESP32
					CPU. Do not use this with Grbl for atMega328P
	
  Grbl is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.
  Grbl is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
  You should have received a copy of the GNU General Public License
  along with Grbl.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "grbl.h"

void spindle_init()
{
	
	// Use DIR and Enable if pins are defined
	#ifdef SPINDLE_ENABLE_PIN
		pinMode(SPINDLE_ENABLE_PIN, OUTPUT);
	#endif
	
	#ifdef SPINDLE_DIR_PIN
		pinMode(SPINDLE_DIR_PIN, OUTPUT);
	#endif
	
    // use the LED control feature to setup PWM   https://esp-idf.readthedocs.io/en/v1.0/api/ledc.html
    ledcSetup(SPINDLE_PWM_CHANNEL, SPINDLE_PWM_BASE_FREQ, SPINDLE_PWM_BIT_PRECISION); // setup the channel
    ledcAttachPin(SPINDLE_PWM_PIN, SPINDLE_PWM_CHANNEL); // attach the PWM to the pin
		
    // Start with spindle off off
	  spindle_stop();
}

void spindle_stop()
{		
  spindle_set_enable(false);	
	grbl_analogWrite(SPINDLE_PWM_CHANNEL, 0);	
}

uint8_t spindle_get_state()  // returns SPINDLE_STATE_DISABLE, SPINDLE_STATE_CW or SPINDLE_STATE_CCW
{	  
  // TODO Update this when direction and enable pin are added 
	if (ledcRead(SPINDLE_PWM_CHANNEL) == 0) // Check the PWM value		
		return(SPINDLE_STATE_DISABLE);
	else
	{
		#ifdef SPINDLE_DIR_PIN			
			if (digitalRead(SPINDLE_DIR_PIN))
				return (SPINDLE_STATE_CW);
			else
				return(SPINDLE_STATE_CCW);
		#else
			return(SPINDLE_STATE_CW);
		#endif		
	}
}

void spindle_set_speed(uint8_t pwm_value)
{	
	#ifndef SPINDLE_ENABLE_OFF_WITH_ZERO_SPEED
		spindle_set_enable(true);
	#else
		spindle_set_enable(pwm_value != 0);
	#endif
	grbl_analogWrite(SPINDLE_PWM_CHANNEL, pwm_value);	
}

// Called by spindle_set_state() and step segment generator. Keep routine small and efficient.
uint8_t spindle_compute_pwm_value(float rpm)
{
	uint8_t pwm_value;
	pwm_value = map(rpm, settings.rpm_min, settings.rpm_max, SPINDLE_PWM_OFF_VALUE, SPINDLE_PWM_MAX_VALUE);		  
	// TODO_ESP32  .. make it 16 bit
	
  return(pwm_value);
}


void spindle_set_state(uint8_t state, float rpm)
{
  if (sys.abort) { return; } // Block during abort.
  if (state == SPINDLE_DISABLE) { // Halt or set spindle direction and rpm.    
    sys.spindle_speed = 0.0;    
    spindle_stop();  
  } else {
  
    // TODO ESP32 Enable and direction control
		#ifdef SPINDLE_DIR_PIN
		if (state == SPINDLE_ENABLE_CW) {
        digitalWrite(SPINDLE_DIR_PIN, 1);
    } 
		else 
		{
        digitalWrite(SPINDLE_DIR_PIN, 0);
    }
		#endif
  
    
      // NOTE: Assumes all calls to this function is when Grbl is not moving or must remain off.
      if (settings.flags & BITFLAG_LASER_MODE) { 
        if (state == SPINDLE_ENABLE_CCW) { rpm = 0.0; } // TODO: May need to be rpm_min*(100/MAX_SPINDLE_SPEED_OVERRIDE);
      }
						
      spindle_set_speed(spindle_compute_pwm_value(rpm));     
  }  
  sys.report_ovr_counter = 0; // Set to report change immediately
}


void spindle_sync(uint8_t state, float rpm)
{
	if (sys.state == STATE_CHECK_MODE) { return; }
  protocol_buffer_synchronize(); // Empty planner buffer to ensure spindle is set when programmed.
  spindle_set_state(state,rpm);
}


void grbl_analogWrite(uint8_t chan, uint32_t duty)
{
	if (ledcRead(chan) != duty) // reduce unnecessary calls to ledcWrite()
	{
		ledcWrite(chan, duty);
	}
}

void spindle_set_enable(bool enable)
{
	#ifdef SPINDLE_ENABLE_PIN	
		#ifndef INVERT_SPINDLE_ENABLE_PIN
				digitalWrite(SPINDLE_ENABLE_PIN, enable); // turn off (low) with zero speed
		#else
				digitalWrite(SPINDLE_ENABLE_PIN, !enable); // turn off (high) with zero speed
		#endif
	#endif
}

