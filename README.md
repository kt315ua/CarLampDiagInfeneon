# CarLampDiagInfeneon
Car lamp diagnostic by Adruino Nano for Infeneon BTS5016 lamp driver

## Description
- controller have status led, ALWAYS blinking in IDLE/NORMAL state!
- watchdog 120MS used to prevent controller freeze
- get lamp status (current) from 2-ch Infeneon BTS5016 with 20 sec periods
  - perform 4 measures for AVG value calc
- if AVG value:
  - is less than expected:
    - increase failure counter
    - trouble beep appeared
    - related lamp status leds start blinking
  - is as expected:
    - reset failure counter
    - restore beep appeared
    - related lamp status leds stop blinking
- if failure counter eq 3:
  - failure song is played with predefined repetition
  - if lamp issue become restored:
    - reset failure counter
    - leds turn off
    - failure song stop repetitive playing

## Arduino libs
Project used with Arduino MiniCore board library:
[MCUdude MiniCore](https://mcudude.github.io/MiniCore/package_MCUdude_MiniCore_index.json)


### Troubleshoots
Not recommended to use controller with external SMD oscillator.

Chinese arduino board clones are not stable with ext smd oscillator under waterproof coating
