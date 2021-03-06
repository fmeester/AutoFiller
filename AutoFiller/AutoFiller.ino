/*
* AutoFiller
* Created:		7 / 12 / 2018 
* Author :		Frank
* Changed :		7 - 12 - 2018
*/

// include the library code:
#include <Wire.h>						//Watch the order of these 3 #includes.
#include <Adafruit_RGBLCDShield.h>		//Otherwise an error will occur
#include <Adafruit_MCP23017.h>
#include <SPI.h>
#include <Controllino.h>				/* Usage of CONTROLLINO library allows you to use CONTROLLINO_xx aliases in your sketch. */
#include <Button.h>
#include <eeprom.h>


// The shield uses the I2C SCL and SDA pins. On classic Arduinos
// this is Analog 4 and 5 so you can't use those for analogRead() anymore
// However, you can connect other I2C sensors to the I2C bus and share
// the I2C bus.
Adafruit_RGBLCDShield lcd = Adafruit_RGBLCDShield();

// Define Constants
//int				iSpeed;										// Speed of the motor.
//const int		maxSpeed = 4800;							// Max speed of shaker 
//const int		minSpeed = 1000;  							// Min speed op shaker 
//static int		iSpeedSetting = 19;							// speedsteps of the motor.
//int				SpeedSetAmount = 20;						// true rpm steps
//
//int				Accelleration;
//int				AccelerationSetAmount = 50;
//int				maxAccelleration = 10000;
//int				AccellerationSetting = maxAccelleration / 250;
int				LongPressIncrementTime = 200;
int				totalLiters;								// Total liters added

float			flowSensorCalibrationFactor;				//4 bytes Value for ticks/liter
															// Debietbereik: 1 ~ 25 l / min
															// Frequentie: F = 11 * Q(Q = L / MIN)
int				pulsePerLiter ;

int				volumeToAddAdress = 4;						
int				flowSensorCalibrationFactorAdress = 8;		//4 bytes
int				RunTimeAdress = 12;							//4 bytes
int				totalLitersAdress = 16;						//4 bytes
int				flowSpeedAdress = 20;

int				hour = 1000;
int				minute = 1000;
int				second = 1000;

int				lowVolumeCouter = 0;						//
float			flowCount = 0;								//
unsigned long	fillFlowCount = 0;							// Flowcount for checking volume
unsigned long	flowCountCheck = 0;							// Check flowcount
int				volumeAdded = 0;							// volume filled
int		 		lowVolumeInput = CONTROLLINO_A0;			// Low volume senor
int				maxVolumeInput = CONTROLLINO_A1;			// Max volume sensor
int				waterSolenoid = CONTROLLINO_D4;

unsigned long	TimerrefreshTime;
unsigned long	refreshTime = 1000;							//Refreshtime display in ms


unsigned long	volumeToAdd;								//Volume to add
unsigned long	TimerLongPress;								//	
unsigned long	LongPressTime = 500;						// time to detect long press
unsigned long	TimerLongPressIncrement;				
unsigned long	eepromSaveTime = 0;							// timer for displaying 'saved'
unsigned long	displayOffTime = 200000;					// time out for backlight / display 
unsigned long	timerDisplayOff;
unsigned long	fillCheckTime = 5000;						// Timer for flow check
unsigned long	timerFillCheck = 0;	
float			flowSpeed;									// Flowspeed Sec/Liter
float			flowSpeedLpM;
unsigned long	flowSpeedTimer;
unsigned long	flowTime = 0;





bool			refreshTimeSet = false;   
bool			buttonstate = false;
bool			LongButtonPress = false;
bool			LongPress = false;
bool			LongPressRun = false;
bool			displayTimeOutTimer = false;
bool			sleep = false;
bool			Saving = false;
bool			lowVolumeTrigger = false;
bool			ERROR = false;
bool			flowFail = false;
bool			maxVolumeFail = false;
bool			fillFail = false;
bool			stopTimeDisplay = false;
bool			calibrateMode = false;
bool			stopDisplayFlow = false;
bool			running = false;
bool			calibrateSave = false;
bool			flowSpeedCheckRun = false;

String			errorText;



/*****************************************************************************************
* IO DEFINITION													                          *
******************************************************************************************/
const byte		startButtonPin = CONTROLLINO_A3;			// Manual Start on pin
const byte		stopButtonPin = CONTROLLINO_A2;				// Manual Stop on pin
const byte		flowsensorPin = CONTROLLINO_IN0;			// Interrupt Flowsensor count pin

byte			menu1Select = 1;							// Menu selection
byte			menu1Length = 7;							// Menu1 length 


Button			startButton = Button(startButtonPin, BUTTON_PULLDOWN);
Button			stopButton = Button(stopButtonPin, BUTTON_PULLDOWN);

//const int		numOfInputs = 5;
//int				inputState[numOfInputs];
//int				lastInputState[numOfInputs] = { LOW,LOW,LOW,LOW };
//bool			inputFlags[numOfInputs] = { LOW,LOW,LOW,LOW };



/*****************************************************************************************
* LCD Menu Logic													                          *
******************************************************************************************/
//const int numOfScreens = 7;
//int currentScreen = 0;
//String screens[numOfScreens][2] = { {"Time set","HH-MM-SS"},{"Volume set","Liters"}, 
//{"Calibrate man", "Pulse/Liter"}, {"Total liters","Liters"}, {"Reset", ""}, {"Start",""}, {"Calibrate auto", "Pulse/Liter"} };
//int parameters[numOfScreens];



void setup() {
	// Define Pins
	
	pinMode(flowsensorPin, INPUT);
	pinMode(maxVolumeInput, INPUT);
	pinMode(lowVolumeInput, INPUT);
	pinMode(startButtonPin, INPUT);
	pinMode(stopButtonPin, INPUT);
	attachInterrupt(digitalPinToInterrupt(flowsensorPin), flowCounter, RISING);
	pinMode(waterSolenoid, OUTPUT);

	totalLiters = EEPROMReadlong(totalLitersAdress);
	volumeToAdd = EEPROMReadlong(volumeToAddAdress);
	flowSensorCalibrationFactor = EEPROMReadlong(flowSensorCalibrationFactorAdress);
	pulsePerLiter = (flowSensorCalibrationFactor * 60);
	flowSpeed = EEPROMReadlong(flowSpeedAdress);


	digitalWrite(waterSolenoid, LOW);



	// Debugging output
	Serial.begin(9600);

	Controllino_RTC_init(0);
//	Controllino_SetTimeDate(29, 7, 12, 18, 17, 57, 00); // (Day of the month, Day of the week, Month, Year, Hour, Minute, Second)
	
	// set up the LCD's number of columns and rows: 
	lcd.begin(16, 2);
	lcd.clear();
	lcd.setCursor(0, 0);
	lcd.print("AutoFiller v1.0"); // 20181207
	delay(2000);
	lcd.clear();
	//lcd.setBacklight(WHITE);

}

uint8_t i = 0;
void loop() {
	buttonScan();
	//resolveInputFlags();


	// Display Timeout
	if (!displayTimeOutTimer & !Error)
	{
		displayTimeOutTimer = true;
		timerDisplayOff = millis();
	}
	if (displayTimeOutTimer & ((millis() - timerDisplayOff) > displayOffTime) & !sleep)
	{
		lcd.setBacklight(0);
		lcd.noDisplay();
		sleep = true;
	}


	// Display Refresh
	if (!refreshTimeSet)
	{
		refreshTimeSet = true;
		TimerrefreshTime = millis();
	}
	if ((millis() - refreshTime) > TimerrefreshTime)
	{
		if (!stopTimeDisplay)
		{
		//	DisplayTime();
		}
		if (!stopDisplayFlow)
		{
			DisplayFlow();
		}
		refreshTimeSet = false;
	}

	if (!ERROR & running)
	{
		if (stopButton.isPressed()) 
		{
			Error("");
			Reset();
		}
	}


	if (lowVolumeTrigger == false & !ERROR & running)		// Low volume detect
	{
		if (digitalRead(lowVolumeInput))
		{													// First time sensor triggered
			startFilling();									// Start to fill
		}
	}
	if (lowVolumeTrigger == true & !ERROR)					// Filling & no error
	{
		if (flowCount >= fillFlowCount)						// Check if fill amount reached
		{
			StopFilling();	
			Serial.println(flowCount);// If so Stop filling
			Serial.println(fillFlowCount);
			Serial.println(volumeToAdd);
			Serial.println(pulsePerLiter);
			if (digitalRead(lowVolumeInput))			// Check if low volume sensor still activated	
			{
				Error("FillFail");
				fillFail = true;
			}
		}
		if (lowVolumeTrigger)
		{
			if ((millis() - fillCheckTime) > timerFillCheck)	// Check if there is flow
			{
				if (flowCount < (flowCountCheck + 100))			// If there is no signal from flow sensor
				{											// in the fillCheckTime
					flowFail = true;						// Stop Filling Error
					Error("FlowFail");
				}
				else
				{
					timerFillCheck = millis();
					flowCountCheck = flowCount;
				}
			}
		}
	}

	if (digitalRead(maxVolumeInput) & !ERROR & running)				// Check Max volume sensor
	{													// Should never be triggered
		maxVolumeFail = true;
		Error("Max Volume Fail");
	}
	if (calibrateMode)
	{
		Calibrate();		
	}

	

	
}


void buttonScan()
{	uint8_t buttons = lcd.readButtons();
	if (!buttons)
	{
		buttonstate = false;
		LongButtonPress = false;
		LongPress = false;
	}
	if (buttons) {
		if (sleep) 
			{
			DisplayOn();
			return;
			}
		displayTimeOutTimer = false;
		if (buttonstate)
		{
			if (!LongButtonPress)
			{
				TimerLongPress = millis();
				LongButtonPress = true;
			}
			if (!LongPress & (millis() - TimerLongPress) > LongPressTime)
			{
				LongPress = true;
				Serial.println("LongPress");
				TimerLongPress = millis();
			}
			if (LongPress & !LongPressRun)
			{
				if ((millis() - TimerLongPressIncrement) > LongPressIncrementTime)
					if (buttons & BUTTON_UP)
					{
						//SettingUp();
					}
				if (buttons & BUTTON_DOWN)
				{
				//	SettingDown();
				}
			}
			return;

		}
		buttonstate = true;
		if (buttons & BUTTON_UP)
		{
			//if (!calibrateMode)
			//{
			//	inputFlags[2] = HIGH;		//Setting Up
			//}
			//if (calibrateMode)
			//{
			Serial.println("Up");
				SettingUp();
			//}
		}
		if (buttons & BUTTON_DOWN)
		{
			//if (!calibrateMode)
			//{
			//
			//inputFlags[3] = HIGH;		//Setting down
			//}
			//if (calibrateMode)
			//{
			Serial.println("Down");
				SettingDown();
			//}

		//	SettingDown();
		}
		if (buttons & BUTTON_LEFT)
		{
			//inputFlags[0] = HIGH;		//Select
			Serial.println("Select");	// Selection
			//lcd.print("Left ");
			++menu1Select;
			if (menu1Select > menu1Length) menu1Select = 1;
			Menu1Select();
		}
		if (buttons & BUTTON_RIGHT)			// Not used
		{
			//lcd.print("RIGHT ");
		}
		if (buttons & BUTTON_SELECT)
		{
			//inputFlags[4] = HIGH;		//Save-enter
			Serial.println("Select");
			//Serial.println(currentScreen);
			//lcd.print("SELECT ");
			Select();
		}
	}

}



void startFilling()
	{
		pulsePerLiter = (flowSensorCalibrationFactor * 60);
		flowCount = 0;
		lowVolumeCouter++;
		lowVolumeTrigger = true;
		fillFlowCount = volumeToAdd * pulsePerLiter;
		digitalWrite(waterSolenoid, HIGH);
		timerFillCheck = millis();
		flowCountCheck = flowCount;
		DisplayOn();
				
	}


void StopFilling()
	{
		lowVolumeTrigger = false;
		lcd.setCursor(0, 0);
		lcd.print("     ");
		digitalWrite(waterSolenoid, LOW);
		totalLiters = (totalLiters + (flowCount/pulsePerLiter));
		EEPROMWritelong(totalLitersAdress, totalLiters);
		flowCount = 0;
		DisplayOn();
		DisplayFlow();
	}

void Error(String  _errorText)
	{
		ERROR = true;
		running = false;
		digitalWrite(waterSolenoid, LOW);					
		lcd.clear();
		lcd.setCursor(0, 0);
		lcd.print(_errorText);
		menu1Select = 5;
		lcd.noBlink();
		DisplayOn();
	}

void Reset()
	{
		lowVolumeTrigger = false;
		flowFail = false;
		maxVolumeFail = false;
		calibrateMode = false;
		ERROR = false;
		lcd.clear();

	}


void flowCounter()  
	{
		flowCount++;
	}

void DisplayTime() 
	{
	// Show time on display
		if ((Controllino_GetHour()) != hour)
		{
			hour = Controllino_GetHour();
			lcd.setCursor(0, 1);
			lcd.print("  ");
			if (hour < 10)
			{
				lcd.setCursor(0, 1);
				lcd.print("0");
				lcd.print(hour);
			}
			else if (hour >= 10)
			{
				lcd.setCursor(0, 1);
				lcd.print(hour);
			}
			lcd.print(":");
		}

		if ((Controllino_GetMinute()) != minute)
		{
			minute = Controllino_GetMinute();
			lcd.setCursor(3, 1);
			lcd.print("  ");
			if (minute < 10)
			{
				lcd.setCursor(3, 1);
				lcd.print("0");
				lcd.print(minute);
			}
			else if (minute >= 10)
			{
				lcd.setCursor(3, 1);
				lcd.print(minute);
			}
			lcd.print(":");
		}
		if ((Controllino_GetSecond()) != second)
		{
			second = Controllino_GetSecond();
			lcd.setCursor(6, 1);
			lcd.print("  ");
			if (second < 10)
			{
				lcd.setCursor(6, 1);
				lcd.print("0");
				lcd.print(second);
			}
			else 
			{
				lcd.setCursor(6, 1);
				lcd.print(second);
			}
		
		}

}


void DisplayFlow()
	{
	// Show flowcounter
	lcd.setCursor(0, 1);
	lcd.print(flowCount);
	lcd.setCursor(6, 1);
	lcd.print(flowSpeedLpM);
	lcd.setCursor(12, 1);
	lcd.print(lowVolumeCouter);
	}


void DisplayOn()
{
	lcd.setBacklight(1);
	lcd.display();
	displayTimeOutTimer = false;
	sleep = false;
	hour = 1000;
	minute = 1000;
	second = 1000;
}




void Calibrate()
{
	
	if (!ERROR & !running)
	{
		if (!calibrateMode)
		{
			flowCount = 0;
			flowTime = 0;
			calibrateMode = true;
			stopTimeDisplay = true;
			stopDisplayFlow = true;
			lcd.clear();
			lcd.setCursor(0, 0);
			lcd.print("10 L --> Stop");
			lcd.setCursor(0, 1);
			lcd.print(flowCount);
		}
			
		if (calibrateMode)
		{
			if (startButton.isPressed())
			{
				digitalWrite(waterSolenoid, HIGH);
				lcd.setCursor(0, 1);
				lcd.print(flowCount);
				if (!flowSpeedCheckRun)
				{
					flowSpeedCheckRun = true;
					flowSpeedTimer = millis();
					Serial.println("FlowSpeedTimer");
					Serial.println(flowSpeedTimer);
				}
				
			}
			if (!startButton.isPressed())
			{
				digitalWrite(waterSolenoid, LOW);
				if (flowSpeedCheckRun)
				{
					flowTime = flowTime + ((millis() - flowSpeedTimer));
					Serial.println("FlowTime");
					Serial.println(flowTime);
					Serial.println("flowCount");
					Serial.println(flowCount);
					flowSpeedCheckRun = false;
				}
			}

			if (stopButton.isPressed())
			{
				flowSpeed =(flowTime / 1000) ; //Sec / Liter
				flowSpeedLpM = 60 / flowSpeed;
				calibrateMode = false;
				digitalWrite(waterSolenoid, LOW);
				lcd.clear();
				hour = 1000;
				minute = 1000;
				second = 1000;
				menu1Select = 21;
				flowSensorCalibrationFactor = (flowCount / 600); //puls/sec/10liter
				Serial.println("Calibrate");
				lcd.setCursor(0, 0);
				lcd.print("Calibrate: ");
				lcd.print(flowSensorCalibrationFactor);
				lcd.setCursor(0, 1);
				lcd.print("Value ok? ");
				lcd.setCursor(9, 1);
				lcd.print("No ");
				Serial.println("FlowSpeed");
				Serial.println(flowSpeedLpM);
			}
		}
	}
}






void Menu1Select()
{
	switch (menu1Select) {
	case 1:														//Time Set
		Serial.println("Time Setting");
		lcd.clear();
		lcd.setCursor(0, 0);
		lcd.print("Time Set        ");
		break;
	case 2:														//Calibrate
		Serial.println("Calibrate manual");
		printCalFac();
		break;
	case 3:														//Calibrate
		Serial.println("Calibrate auto");
		lcd.clear();
		lcd.setCursor(0, 0);
		lcd.print("Cal. auto:");
		lcd.print(flowSensorCalibrationFactor);
		break;
	case 4:														//Volume setting
		Serial.println("Volume setting");
		printVolumeToAdd();
		break;
	case 5:														//Show total liters
		Serial.println("Total liters:");
		lcd.setCursor(0, 0);
		lcd.print("Total liters:");
		lcd.print(totalLiters);
		break;
	case 6:														//Reset
		Serial.println("Reset");
		lcd.clear();
		lcd.setCursor(0, 0);
		lcd.print("Reset");
		break;
	case 7:														//Start
		Serial.println("Start");
		lcd.clear();
		lcd.setCursor(0, 0);
		lcd.print("Start");
		break;
	case 21:													//Auto calibration save
		{

		}
		break;

		//default:
		// if nothing else matches, do the default
		// default is optional
	}

}

//void setInputFlags() {
//	for (int i = 0; i < numOfInputs; i++) {
//		int reading = digitalRead(inputPins[i]);
//		if (reading != lastInputState[i]) {
//			lastDebounceTime[i] = millis();
//		}
//		if ((millis() - lastDebounceTime[i]) > debounceDelay) {
//			if (reading != inputState[i]) {
//				inputState[i] = reading;
//				if (inputState[i] == HIGH) {
//					inputFlags[i] = HIGH;
//				}
//			}
//		}
//		lastInputState[i] = reading;
//	}
//}

//void resolveInputFlags() {
//	for (int i = 0; i < numOfInputs; i++) {
//		if (inputFlags[i] == HIGH) {
//			inputAction(i);
//			inputFlags[i] = LOW;
//			if (currentScreen == 1)								//Dosing volume
//			{
//				printScreen(volumeToAdd);
//			}
//			else if (currentScreen == 2)						//Calibation factor manual
//				{
//					printScreen(flowSensorCalibrationFactor);
//				}
//			else 
//				{
//				printScreen(parameters[currentScreen]);
//				}
//			
//		}
//	}
//}

//void inputAction(int input) {
//	if (input == 0) {
//		if (currentScreen == numOfScreens - 1) 
//			{
//				currentScreen = 0;
//			}
//		else {
//			currentScreen++;
//		}
//	}
//	else if (input == 1) {
//		if (currentScreen == numOfScreens - 1) {
//			currentScreen = 0;
//		}
//		else {
//			currentScreen++;
//		}
//	}
//	else if (input == 2) {
//		parameterChange(0);
//	}
//	else if (input == 3) {
//		parameterChange(1);
//	}
//	else if (input == 4) {
//		writeSettings();
//	}
//}

//void parameterChange(int key) {
//	if (key == 0 & currentScreen== 1) {
//		volumeToAdd++;
//	}
//	else if (key == 1 & currentScreen == 1) {
//		volumeToAdd--;
//	}
//	if (key == 0 & currentScreen == 2) {
//		flowSensorCalibrationFactor++;
//	}
//	else if (key == 1 & currentScreen == 2) {
//		flowSensorCalibrationFactor--;
//	}
//
//}

void printScreen(int parameter) {
	lcd.clear();
	//lcd.print(screens[currentScreen][0]);
	lcd.setCursor(0, 1);
	if (parameter != 0)
		{
			lcd.print(parameter); 
			lcd.print(" ");
		}
	
	//lcd.print(screens[currentScreen][1]);
}



void Select()
{
	switch (menu1Select) {

	case 1:											// Time setting
	
		/*if (EEPROMReadlong(flowSensorCalibrationFactor) != iSpeed)
		{
			Serial.println("writing speed to eeprom");
			EEPROMWritelong(flowSensorCalibrationFactor, iSpeed);
		}*/
		break;

	case 2:											//Manual Calibration
		if (EEPROMReadlong(flowSensorCalibrationFactorAdress) != flowSensorCalibrationFactor)
		{
			Serial.println("writing Calibration to eeprom");
			EEPROMWritelong(flowSensorCalibrationFactorAdress, flowSensorCalibrationFactor);
			pulsePerLiter = (flowSensorCalibrationFactor * 60);
		}
		break;
	case 3:											//Auto Calibration
		{
		Calibrate();
					}
	break;
	case 4:											// Volume to add
		if (EEPROMReadlong(volumeToAddAdress) != volumeToAdd)
		{
			Serial.println("writing Calibration to eeprom");
			EEPROMWritelong(flowSensorCalibrationFactorAdress, flowSensorCalibrationFactor);
			pulsePerLiter = (flowSensorCalibrationFactor * 60);

		}
		break;
	case 5:											// Show total liters
		{

		}
		break;
	case 6:											//Reset
		{
		Reset();
		}
		break;
	case 7:											//Start
	{
		running = true;
		lcd.setCursor(0, 0);
		lcd.print("Running");
		lcd.blink();
	}
	break;

	//case 20:											// Save after calibration
	//{
	//	if (EEPROMReadlong(flowSensorCalibrationFactorAdress) != flowSensorCalibrationFactor)
	//	{
	//		Serial.println("writing Calibration to eeprom");
	//		EEPROMWritelong(flowSensorCalibrationFactorAdress, flowSensorCalibrationFactor);
	//		menu1Select = 1;
	//	}
	//}
	//break;

	case 21:
	{
		if (calibrateSave)
		{
			Serial.println("writing Calibration to eeprom");
			EEPROMWritelong(flowSensorCalibrationFactorAdress, flowSensorCalibrationFactor);
			EEPROMWritelong(flowSpeedAdress, flowSpeed);
			calibrateSave = false;
			lcd.clear();
			DisplayFlow();
			menu1Select = 1;
		}
		else
		{
			lcd.clear();
			DisplayFlow();
			menu1Select = 1;
		}

	}
			break;
	}
}


void SettingDown()
{
	switch (menu1Select) 
	{
		case 1://Speed Down

			break;
		case 2:									//Calibration factor down
		{
			flowSensorCalibrationFactor = (flowSensorCalibrationFactor -0.1);
			if (flowSensorCalibrationFactor < 1)
			{
				flowSensorCalibrationFactor = 1;
			}
			printCalFac();
		}
		break;
		case 4:									//Volume Down
		{
			volumeToAdd--;
			if (volumeToAdd < 1)
			{
				volumeToAdd = 1;
			}
			printVolumeToAdd();
		}
		break;


		case 6:									//Calibrate save y/n
			lcd.setCursor(0, 1);
			lcd.print("Value ok? ");
			lcd.setCursor(9, 1);
			lcd.print("No ");
			break;


		case 21:								//Auto calibration save Y/N
		{
			lcd.setCursor(0, 0);
			lcd.print("Calibrate: ");
			lcd.print((flowCount / 60));
			lcd.setCursor(0, 1);
			lcd.print("Value ok? ");
			lcd.setCursor(9, 1);
			lcd.print("No ");
			calibrateSave = false;
		}
			break;

	}
}

void SettingUp()
{
	switch (menu1Select) {
	case 1://Speed Down

		break;
	case 2: 								//Calibration factor up
		{
			flowSensorCalibrationFactor = (flowSensorCalibrationFactor +0.1);
			if (flowSensorCalibrationFactor > 20)
			{
				flowSensorCalibrationFactor = 20;
			}
		printCalFac();
		}
		break;
	case 4:									//Volume Up
	{
		volumeToAdd++;
		if (volumeToAdd > 100)
		{
			volumeToAdd = 100;
		}
		printVolumeToAdd();
	}
	break;


	case 6: //Calibrate save y/n
		lcd.setCursor(0, 1);
		lcd.print("Value ok? ");
		lcd.setCursor(9, 1);
		lcd.print("Yes ");
		break;
	case 21:								//Auto calibration save Y/N
		{
		lcd.setCursor(0, 0);
		lcd.print("Calibrate: ");
		lcd.print((flowCount / 60));
		lcd.setCursor(0, 1);
		lcd.print("Value ok? ");
		lcd.setCursor(9, 1);
		lcd.print("Yes ");
		calibrateSave = true;
		}
		break;

	}
}


void printCalFac()
{
	lcd.clear();
	lcd.setCursor(0, 0);
	lcd.print("Cal. man: ");
	lcd.print(flowSensorCalibrationFactor);
}

void printVolumeToAdd()
{
	lcd.clear();
	lcd.setCursor(0, 0);
	lcd.print("Volume:");
	lcd.print("    ");
	lcd.setCursor(7, 0);
	lcd.print(volumeToAdd);
	lcd.setCursor(10, 0);
	lcd.print("Liter");
}

//This function will write a 4 byte (32bit) long to the eeprom at
//the specified address to adress + 3.
void EEPROMWritelong(int address, long value)
{
	//Decomposition from a long to 4 bytes by using bitshift.
	//One = Most significant -> Four = Least significant byte
	byte four = (value & 0xFF);
	byte three = ((value >> 8) & 0xFF);
	byte two = ((value >> 16) & 0xFF);
	byte one = ((value >> 24) & 0xFF);

	//Write the 4 bytes into the eeprom memory.
	EEPROM.write(address, four);
	EEPROM.write(address + 1, three);
	EEPROM.write(address + 2, two);
	EEPROM.write(address + 3, one);

	//lcd.setCursor(9, 1);
	//lcd.print("Saved");
	//eepromSaveTime = millis();
	//Saving = true;
}


//This function will return a 4 byte (32bit) long from the eeprom
//at the specified address to adress + 3.
long EEPROMReadlong(long address)
{
	//Read the 4 bytes from the eeprom memory.
	long four = EEPROM.read(address);
	long three = EEPROM.read(address + 1);
	long two = EEPROM.read(address + 2);
	long one = EEPROM.read(address + 3);

	//Return the recomposed long by using bitshift.
	return ((four << 0) & 0xFF) + ((three << 8) & 0xFFFF) + ((two << 16) & 0xFFFFFF) + ((one << 24) & 0xFFFFFFFF);
}
