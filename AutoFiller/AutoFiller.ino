
// include the library code:

#include <Wire.h>						//Watch the order of these 3 #includes.
#include <Adafruit_RGBLCDShield.h>		//Otherwise an error will occur
#include <Adafruit_MCP23017.h>
#include <SPI.h>
#include <Controllino.h>  /* Usage of CONTROLLINO library allows you to use CONTROLLINO_xx aliases in your sketch. */



// The shield uses the I2C SCL and SDA pins. On classic Arduinos
// this is Analog 4 and 5 so you can't use those for analogRead() anymore
// However, you can connect other I2C sensors to the I2C bus and share
// the I2C bus.
Adafruit_RGBLCDShield lcd = Adafruit_RGBLCDShield();

// These #defines make it easy to set the backlight color
#define RED 0x1
#define YELLOW 0x3
#define GREEN 0x2
#define TEAL 0x6
#define BLUE 0x4
#define VIOLET 0x5
#define WHITE 0x7

void setup() {
	// Debugging output
	Serial.begin(9600);

	Controllino_RTC_init(0);
	Controllino_SetTimeDate(7, 3, 11, 18, 21, 15, 00); // set initial values to the RTC chip
	Controllino_SetTimeDate(,,,,,)
	// set up the LCD's number of columns and rows: 
	lcd.begin(16, 2);

	// Print a message to the LCD. We track how long it takes since
	// this library has been optimized a bit and we're proud of it :)
	int time = millis();
	lcd.print("Hello, world!");
	time = millis() - time;
	Serial.print("Took "); Serial.print(time); Serial.println(" ms");
	lcd.setBacklight(WHITE);
}

uint8_t i = 0;
void loop() {
	int n;
	// set the cursor to column 0, line 1
	// (note: line 1 is the second row, since counting begins with 0):
	lcd.setCursor(0, 1);
	// print the number of seconds since reset:
	lcd.print(millis() / 1000); 
	lcd.setCursor(5, 1);
	n = Controllino_GetHour();
	lcd.print(n);
	lcd.setCursor(8, 1);
	n = Controllino_GetMinute();
	lcd.print(n);
	lcd.setCursor(11, 1);
	n = Controllino_GetSecond();
	lcd.print(n);
	
	//Serial.print("Day: "); n = Controllino_GetDay(); Serial.println(n);

	//Serial.print("WeekDay: "); n = Controllino_GetWeekDay(); Serial.println(n);

	//Serial.print("Month: "); n = Controllino_GetMonth(); Serial.println(n);

	//Serial.print("Year: "); n = Controllino_GetYear(); Serial.println(n);

	//Serial.print("Hour: "); n = Controllino_GetHour(); Serial.println(n);

	//Serial.print("Minute: "); n = Controllino_GetMinute(); Serial.println(n);

	//Serial.print("Second: "); n = Controllino_GetSecond(); Serial.println(n);



	uint8_t buttons = lcd.readButtons();

	if (buttons) {
		lcd.clear();
		lcd.setCursor(0, 0);
		if (buttons & BUTTON_UP) {
			lcd.print("UP ");
			lcd.setBacklight(RED);
		}
		if (buttons & BUTTON_DOWN) {
			lcd.print("DOWN ");
			lcd.setBacklight(YELLOW);
		}
		if (buttons & BUTTON_LEFT) {
			lcd.print("LEFT ");
			lcd.setBacklight(GREEN);
		}
		if (buttons & BUTTON_RIGHT) {
			lcd.print("RIGHT ");
			lcd.setBacklight(TEAL);
		}
		if (buttons & BUTTON_SELECT) {
			lcd.print("SELECT ");
			lcd.setBacklight(VIOLET);
		}
	}
}