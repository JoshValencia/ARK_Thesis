#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// This is the most common failure point. If the address is wrong (0x27 vs 0x3F), the screen stays blank and you'll think everything is broken when it's not.

LiquidCrystal_I2C lcd(0x27, 16, 2); // Change to 0x3F if blank

void setup() {
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("LCD OK!");
}
void loop() {}