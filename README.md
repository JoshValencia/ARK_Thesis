# ARK - Automated Wastepaper Recycling Machine

STI College Santa Rosa | BS Computer Engineering

---

## Required Libraries

Install both via Arduino IDE → Sketch → Include Library → Manage Libraries:

1. **LiquidCrystal I2C** by Frank de Brabander
2. **HX711 Arduino Library** by Bogdan Necula

---

## Hardware List

| Component                 | Qty                   |
| ------------------------- | --------------------- |
| Arduino Mega 2560         | 1                     |
| HX711 + Load Cell         | 1                     |
| I2C LCD 16x2              | 1                     |
| L298N Motor Driver        | 2 (or 1 dual-channel) |
| DC Motors                 | 3                     |
| PIR Sensor                | 1                     |
| HC-SR04 Ultrasonic Sensor | 1                     |
| Relay Module              | 1                     |
| Push Buttons              | 2                     |
| 12V Power Supply          | 1                     |

---

## System Workflow

```
WEIGHING → READY → SHREDDING → PULPING → PRESSING → DRYING → COMPLETE
```

Each state is a function in the code. If the machine is stuck, open Serial Monitor
(9600 baud) and it will tell you exactly which state it is in and why.

---

## Wiring Summary

### HX711 (Load Cell Amplifier)

| HX711 Pin | Arduino Mega Pin |
| --------- | ---------------- |
| DT        | 3                |
| SCK       | 2                |
| VCC       | 5V               |
| GND       | GND              |

### LCD I2C 16x2

| LCD Pin | Arduino Mega Pin |
| ------- | ---------------- |
| SDA     | 20               |
| SCL     | 21               |
| VCC     | 5V               |
| GND     | GND              |

> **Note:** Pins 20 and 21 are the dedicated I2C pins on the Mega 2560.
> Do not use any other pins for I2C — it will not work.

### Push Buttons

| Button | Arduino Pin | Other leg |
| ------ | ----------- | --------- |
| START  | 4           | GND       |
| RESET  | 5           | GND       |

> Buttons use `INPUT_PULLUP` — no resistor needed. Wire one leg to the pin,
> the other leg directly to GND.

### PIR Sensor

| PIR Pin | Arduino Mega Pin |
| ------- | ---------------- |
| OUT     | 6                |
| VCC     | 5V               |
| GND     | GND              |

### HC-SR04 Ultrasonic Sensor

| HC-SR04 Pin | Arduino Mega Pin |
| ----------- | ---------------- |
| TRIG        | 30               |
| ECHO        | 31               |
| VCC         | 5V               |
| GND         | GND              |

### Relay Module (UV Light)

| Relay Pin | Arduino Mega Pin |
| --------- | ---------------- |
| IN        | 7                |
| VCC       | 5V               |
| GND       | GND              |

> The relay switches the UV lamp on/off. Connect the UV lamp to the relay's
> NO (Normally Open) and COM terminals.

### Shredder Motor — L298N Channel A

| L298N Pin | Arduino Mega Pin |
| --------- | ---------------- |
| IN1       | 8                |
| IN2       | 9                |
| ENA       | 10 (PWM)         |

### Mixer Motor — L298N Channel B

| L298N Pin | Arduino Mega Pin |
| --------- | ---------------- |
| IN1       | 11               |
| IN2       | 12               |
| ENB       | 13 (PWM)         |

> **Known quirk:** Pin 13 is also the built-in LED on the Mega. The onboard LED
> will flicker during the pulping stage. This is normal and does not affect function.

### Press Motor — Second L298N Channel A

| L298N Pin | Arduino Mega Pin |
| --------- | ---------------- |
| IN1       | 22               |
| IN2       | 23               |
| ENA       | 44 (PWM)         |

> **PWM pins on Mega 2560:** 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 44, 45, 46.
> Pins 10, 13, and 44 are used for motor speed control. Do not move them to
> non-PWM pins or `analogWrite()` will silently fail and motors won't spin.

### Common Ground Rule (CRITICAL)

```
Arduino GND ──┬── L298N GND (Motor Driver 1)
              ├── L298N GND (Motor Driver 2)
              ├── HX711 GND
              ├── PIR GND
              ├── HC-SR04 GND
              ├── Relay GND
              └── LCD GND
```

**All grounds must be connected together.** If a motor runs erratically or
sensors give garbage readings, missing common ground is the most likely cause.

---

## Load Cell Calibration

The load cell must be calibrated before use or weight readings will be wrong.

**Step 1** — Set `CAL_FACTOR = 1.0` in the code temporarily.

**Step 2** — Upload the code, open Serial Monitor at 9600 baud.

**Step 3** — Place a known weight on the load cell (e.g. a 100g object).

**Step 4** — Read the raw value printed in Serial Monitor.

**Step 5** — Calculate: `CAL_FACTOR = raw_value / known_weight_in_grams`

**Step 6** — Replace the value in the code and re-upload. Confirm the reading matches.

> If the scale reads negative, the load cell wires are reversed.
> Swap the A+ and A- wires on the HX711.

---

## Serial Monitor Debug Guide

Open Serial Monitor (Tools → Serial Monitor) at **9600 baud** before powering on.
Every state transition and sensor reading is printed there in real time.

### What normal output looks like:

```
ARK System Starting...
Setup complete. Entering WEIGHING state.
Weight: 12.3g / Need 50.0g
Weight: 48.7g / Need 50.0g
Weight OK: 52.1g — moving to READY.
Waiting for START button...
START pressed — moving to SHREDDING.
Shredding started.
Shredding complete. Moving to PULPING.
Pulping started.
Pulping complete. Moving to PRESSING.
Pressing complete. Moving to DRYING.
Drying started.
Drying elapsed: 1s
Person detected! PIR=1 Distance=34cm
Drying elapsed: 2s
...
Drying complete. Moving to COMPLETE.
Cycle complete. Waiting for RESET.
```

### If Serial Monitor shows nothing at all:

- Check that baud rate is set to 9600 in the bottom-right dropdown
- Check the correct COM port is selected under Tools → Port
- Try pressing the RESET button on the Arduino board itself

---

## Common Problems and Fixes

### LCD stays completely blank

1. First try: change `LiquidCrystal_I2C lcd(0x27, 16, 2)` to `LiquidCrystal_I2C lcd(0x3F, 16, 2)` — the I2C address varies by manufacturer.
2. If still blank: adjust the contrast potentiometer on the back of the I2C backboard.
3. If still blank: run an I2C scanner sketch (search "Arduino I2C scanner" online) to find the actual address of your module.
4. Confirm SDA is on pin 20 and SCL is on pin 21 — not pins 18/19 (those are Serial1 TX/RX).

### Motor doesn't spin

1. Check that the L298N 12V input is connected to your 12V supply, not the Arduino.
2. Check that L298N GND is connected to both the 12V supply GND and Arduino GND.
3. Confirm ENA/ENB jumper is removed — the jumper bypasses PWM control and locks speed at full. If the jumper is on, `analogWrite()` has no effect.
4. Test the motor directly: in setup(), add `motorForward(SHRED_IN1, SHRED_IN2, SHRED_EN, 255); delay(3000); motorStop(...)` then check if it spins.

### Motor spins the wrong direction

- Swap IN1 and IN2 connections on the L298N for that channel. No code change needed.

### Load cell reads 0 or random numbers

1. Check DT → pin 3 and SCK → pin 2. These are easy to mix up.
2. Confirm the load cell wires are connected to HX711 correctly (color codes vary by brand — check your specific load cell datasheet).
3. Make sure `scale.tare()` runs after the load cell is physically stable (not being touched).

### Machine stuck in WEIGHING despite paper on scale

- Load cell not calibrated yet — `CAL_FACTOR` is wrong. Follow the calibration steps above.
- Check that `MIN_WEIGHT` in the code matches what you're actually placing on the scale.

### UV relay clicks but UV light doesn't turn on

- Confirm the UV lamp is wired to NO (Normally Open) and COM on the relay, not NC (Normally Closed).
- Relay logic: when Arduino sends HIGH to the relay IN pin, the NO contact closes and the lamp gets power.

### PIR triggers randomly / never triggers

- PIR sensors need 30–60 seconds to stabilize after power-on. Wait before testing.
- Adjust the sensitivity and delay potentiometers on the PIR board — there are two small orange dials.
- If it triggers constantly even with no one present, point it away from heat sources (windows, the UV lamp itself).

### HC-SR04 always reads 999cm

- Confirm TRIG → pin 30 and ECHO → pin 31.
- Keep sensor away from soft surfaces (foam, cloth) — they absorb ultrasonic pulses and return no echo.
- Make sure there is a clear line of sight in front of the sensor.

### RESET button doesn't respond during drying

- This is fixed in the current code — the drying loop checks the RESET button every 500ms.
- If it still doesn't respond, check the button is wired to pin 5 with the other leg to GND (not 5V).

---

## Adjustable Parameters (top of the .ino file)

```cpp
float CAL_FACTOR       = 2280.0;   // Change after load cell calibration
float MIN_WEIGHT       = 50.0;     // Grams of paper needed to start
int   SAFE_DISTANCE_CM = 50;       // UV turns off if person within this distance (cm)
unsigned long DRY_DURATION_MS = 20000UL; // Total UV drying time in milliseconds
```

For testing, lower `MIN_WEIGHT` to 10.0 and `DRY_DURATION_MS` to 10000 so you
don't have to wait as long each cycle.

Motor durations are set with `delay()` inside each state function.
Search for `delay(10000)` in the code to find and adjust shredding time,
`delay(15000)` for pulping, and the other `delay(10000)` for pressing.

---

## Notes

- Motor timings are placeholders — adjust after physical testing with actual paper load.
- All motor driver grounds and Arduino grounds must share a common connection.
- Do not power the motors from the Arduino 5V pin — always use the external 12V supply through the L298N.
- The Arduino can be powered via USB during development or via the 12V supply through its barrel jack during deployment.
