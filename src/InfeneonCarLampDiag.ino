#include <avr/wdt.h>
#include "starwars.h"

//#define DEBUG

/*
#AVRDUDE_CONF="/etc/avrdude.conf"
AVRDUDE_CONF="/home/user/.arduino15/packages/arduino/tools/avrdude/6.3.0-arduino17/etc/avrdude.conf"
CHIP="atmega328pb"
MINICORE_VER="3.0.1"

/usr/bin/avrdude \
-C "${AVRDUDE_CONF}" -v -p "${CHIP}" \
-cusbasp -Pusb -Uflash:w:/home/user/.arduino15/packages/MiniCore/hardware/avr/3.0.1/bootloaders/optiboot_flash/bootloaders/atmega328p/16000000L/optiboot_flash_atmega328p_UART0_115200_16000000L_B5.hex:i \
-Ulock:w:0x0f:m

*/

unsigned long test_target_time = 0L;
unsigned long TEST_PERIOD = 20 * 1000L;  // Period 20 sec minute

unsigned long melody_target_time = 0L;
unsigned long MELODY_PERIOD = 5 * 60 * 1000L;  // Period 5 minute

unsigned int LED_CH0_R = 3;             // Digital: D3 - RIGHT
unsigned int LED_CH1_L = 2;             // Digital: D2 - LEFT
unsigned int OUT_CH0_R = 7;             // Digital: D7 - RIGHT
unsigned int OUT_CH1_L = 8;             // Digital: D8 - LEFT
unsigned int BUZZER = 9;                // Digital: D9
unsigned int DiagSelect = 5;            // Digital: D5
unsigned int DiagEnable = 6;            // Digital: D6
unsigned int CurrentSenseAnalogIN = 0;  // Analog: A0

bool BlinkLedState = false;
unsigned int StatusLed = 13;
unsigned int sensorRawValue = 0;
unsigned int sensorVoltageValue = 0;

unsigned int buffer_size = 4;
unsigned int lamp_ch0[4] = { 0, 0, 0, 0 };
unsigned int lamp_ch1[4] = { 0, 0, 0, 0 };

unsigned int ch0_false_cnt = 0;
unsigned int ch1_false_cnt = 0;
unsigned int max_false_cnt = 3;
unsigned int CurrentSenseMin = 50;

bool success_beep_enabled = true;
bool trouble_beep_enabled = true;
bool melody_enabled = true;
bool melody_was_played = false;

void wd_delay(unsigned int sleep_ms) {
  if (sleep_ms > 10) {
    wdt_reset();
    for (int i = 0; i < sleep_ms / 10; i++) {
      wdt_reset();
      delay(10);
    }
  } else {
    wdt_reset();
    delay(sleep_ms);
  }
  wdt_reset();
}

void trouble_beep() {
  wdt_reset();
  if (trouble_beep_enabled == true) {
    for (int i = 0; i < 3; i++) {
      tone(BUZZER, 500, 100);
      wd_delay(200);
    }
    tone(BUZZER, 300, 200);
    wd_delay(200);
    noTone(BUZZER);
  }
  wdt_reset();
}

void success_beep() {
  wdt_reset();
  if (success_beep_enabled == true) {
    for (int i = 0; i < 3; i++) {
      tone(BUZZER, 600, 100);
      wd_delay(200);
    }
    tone(BUZZER, 800, 200);
    wd_delay(200);
    noTone(BUZZER);
  }
  wdt_reset();
}

#ifdef DEBUG
void print_serial_data() {
  for (int i = 0; i < 2; i++) {
    if (i == 0) {
      Serial.println("Selected RIGHT: CH=0 / OUT=0");
      for (int c = 0; c < buffer_size; c++) {
        sensorVoltageValue = map(lamp_ch0[c], 0, 1023, 0, 5000);
        Serial.print("RIGHT/CH0: ");
        Serial.println(lamp_ch0[c]);
        Serial.print("RIGHT/CH0 mV: ");
        Serial.println(sensorVoltageValue);
      }
    } else {
      Serial.println("Selected LEFT: CH=1 / OUT=1");
      for (int c = 0; c < buffer_size; c++) {
        sensorVoltageValue = map(lamp_ch1[c], 0, 1023, 0, 5000);
        Serial.print("LEFT/CH1: ");
        Serial.println(lamp_ch1[c]);
        Serial.print("LEFT/CH1 mV: ");
        Serial.println(sensorVoltageValue);
      }
    }
  }
}
#endif

void test_outputs() {
#ifdef DEBUG
  Serial.print(">>>>>>>>>>>>>>>>>>>>>>>>>> Exec test start: ");
  Serial.println(millis());
#endif
  digitalWrite(StatusLed, true);

  digitalWrite(DiagEnable, true);
  wd_delay(100);

  for (int i = 0; i < 2; i++) {
    if (i == 0) {
      digitalWrite(DiagSelect, false);  // false -> ch0/out0/right
      digitalWrite(LED_CH0_R, true);
      digitalWrite(OUT_CH0_R, true);
      // Delay before measure to avoid first measure be 0
      delay(1);

      for (int c = 0; c < buffer_size; c++) {
        lamp_ch0[c] = analogRead(CurrentSenseAnalogIN);
        wdt_reset();
        //delay(1);
      }
      digitalWrite(OUT_CH0_R, false);
      digitalWrite(LED_CH0_R, false);

    } else {
      digitalWrite(DiagSelect, true);  // true -> ch1/out1/left

      digitalWrite(LED_CH1_L, true);
      digitalWrite(OUT_CH1_L, true);
      // Delay before measure to avoid first measure be 0
      delay(1);

      for (int c = 0; c < buffer_size; c++) {
        lamp_ch1[c] = analogRead(CurrentSenseAnalogIN);
        wdt_reset();
        //delay(1);
      }
      digitalWrite(OUT_CH1_L, false);
      digitalWrite(LED_CH1_L, false);
    }
    wd_delay(100);
  }

  // For safety
  digitalWrite(OUT_CH1_L, false);
  digitalWrite(OUT_CH0_R, false);
  digitalWrite(DiagEnable, false);
  digitalWrite(LED_CH0_R, false);
  digitalWrite(LED_CH1_L, false);

  digitalWrite(StatusLed, false);

#ifdef DEBUG
  Serial.print(">>>>>>>>>>>>>>>>>>>>>>>>>> Exec test stop: ");
  Serial.println(millis());
#endif

#ifdef DEBUG
  Serial.print(">>>>>>>>>>>>>>>>>>>>>>>>>> Print data start: ");
  Serial.println(millis());
  print_serial_data();
  Serial.print(">>>>>>>>>>>>>>>>>>>>>>>>>> Print data stop: ");
  Serial.println(millis());
#endif
}

void analyzer() {
#ifdef DEBUG
  Serial.print(">>>>>>>>>>>>>>>>>>>>>>>>>> Analyzer start: ");
  Serial.println(millis());
#endif

  unsigned int avg = 0;  // var for avg calcs
  unsigned int buf = 0;  // var for transitional calcs

  //Cal AVG values for RIGHT/CH0
  buf = 0;
  for (int i = 0; i < buffer_size; i++) {
    buf += lamp_ch0[i];
  }
  avg = buf / buffer_size;
  if (avg < CurrentSenseMin) {
    if (ch0_false_cnt < max_false_cnt) {
      ch0_false_cnt += 1;
      trouble_beep();
    } else {
      ch0_false_cnt = 255;
    }
  } else {
    if (ch0_false_cnt != 0) {
      success_beep();
    }
    ch0_false_cnt = 0;
  }
#ifdef DEBUG
  Serial.print("RIGHT/CH0 AVG: ");
  Serial.println(avg);
  Serial.print("RIGHT/CH0 FALSE COUNT: ");
  Serial.println(ch0_false_cnt);
#endif
  wd_delay(200);

  //Cal AVG values for LEFT/CH1
  buf = 0;
  for (int i = 0; i < buffer_size; i++) {
    buf += lamp_ch1[i];
  }
  avg = buf / buffer_size;
  if (avg < CurrentSenseMin) {
    if (ch1_false_cnt < max_false_cnt) {
      ch1_false_cnt += 1;
      trouble_beep();
    } else {
      ch1_false_cnt = 255;
    }
  } else {
    if (ch1_false_cnt != 0) {
      success_beep();
    }
    ch1_false_cnt = 0;
  }
#ifdef DEBUG
  Serial.print("LEFT/CH1 AVG: ");
  Serial.println(avg);
  Serial.print("LEFT/CH1 FALSE COUNT: ");
  Serial.println(ch1_false_cnt);
#endif
  wd_delay(200);

  // If fails detected -> Turn ON LED(s) -> play melody if it wasn't played and set melody played status
  // if no fails detected -> Turn OFF LED(s) -> reset melody status and don't play it
  if (ch0_false_cnt >= max_false_cnt || ch1_false_cnt >= max_false_cnt) {

    //Turn ON LED(s)
    if (ch0_false_cnt >= max_false_cnt) {
      digitalWrite(LED_CH0_R, true);
    }
    if (ch1_false_cnt >= max_false_cnt) {
      digitalWrite(LED_CH1_L, true);
    }

    // Play melody if wasn't played and set melody played status
    if (melody_enabled == true && melody_was_played == false) {
      melody_was_played = true;
      wdt_disable();
      star_wars_melody_play();
      wdt_enable(WDTO_120MS);
    }

  } else {
    // Turn OFF LED(s) and reset melody played status
    digitalWrite(LED_CH0_R, false);
    digitalWrite(LED_CH1_L, false);
    melody_was_played == false;
  }

#ifdef DEBUG
  Serial.print(">>>>>>>>>>>>>>>>>>>>>>>>>> Analyzer stop: ");
  Serial.println(millis());
#endif
}

void test_run() {
#ifdef DEBUG
  Serial.print(">>>>>>>>>>>>>>>>>>>>>>>>>> FULL TEST START: ");
  Serial.println(millis());
#endif
  test_outputs();
  analyzer();
#ifdef DEBUG
  Serial.print(">>>>>>>>>>>>>>>>>>>>>>>>>> FULL TEST STOP: ");
  Serial.println(millis());
#endif
}

void setup() {
  wdt_enable(WDTO_120MS);
  pinMode(StatusLed, OUTPUT);
  pinMode(LED_CH0_R, OUTPUT);
  pinMode(LED_CH1_L, OUTPUT);
  pinMode(OUT_CH0_R, OUTPUT);
  pinMode(OUT_CH1_L, OUTPUT);
  pinMode(BUZZER, OUTPUT);
  pinMode(DiagEnable, OUTPUT);
  pinMode(DiagSelect, OUTPUT);

  digitalWrite(StatusLed, false);
  digitalWrite(OUT_CH1_L, false);
  digitalWrite(OUT_CH0_R, false);
  digitalWrite(LED_CH0_R, false);
  digitalWrite(LED_CH1_L, false);
  digitalWrite(DiagEnable, false);

#ifdef DEBUG
  Serial.begin(115200);
  Serial.println("Bootup...");
#endif

  // Run 1st test immediatelly after powerup with some delay
  wd_delay(100);
  test_run();
  wdt_reset();
}




void loop() {
  wdt_reset();
  if (millis() - test_target_time >= TEST_PERIOD) {
    test_target_time += TEST_PERIOD;  // change scheduled time exactly, no slippage will happen
    test_run();
  }

  // Reset melody status each MELODY_PERIOD to play it again if problem still present
  if (millis() - melody_target_time >= MELODY_PERIOD) {
    melody_target_time += MELODY_PERIOD;
    melody_was_played = false;
  }

  // Blink by StatusLed to show working state
  if (BlinkLedState == false) {
    BlinkLedState = true;
  } else {
    BlinkLedState = false;
  }
  digitalWrite(StatusLed, BlinkLedState);

  wdt_reset();

  if (ch0_false_cnt != 0 && ch0_false_cnt < max_false_cnt) {
    digitalWrite(LED_CH0_R, BlinkLedState);
  }
  if (ch1_false_cnt != 0 && ch1_false_cnt < max_false_cnt) {
    digitalWrite(LED_CH1_L, BlinkLedState);
  }

  wd_delay(250);
}
