#include <Wire.h>
#include "INA226.h"
#include <math.h>

INA226 INA(0x40);

// ---------- Pins ----------
const int SDA_PIN = 4;
const int SCL_PIN = 5;
const int THERMISTOR_PIN = 6;
const int MOSFET_PIN = 7;

// ---------- INA226 ----------
const float SHUNT_OHMS = 0.1;
const float MAX_CURRENT_A = 1.0;   // 4.2 V / ~6 ohm = ~0.7 A, so 1 A gives headroom
const float MAX_VALID_VOLTAGE = 5.0;   // sanity ceiling for a single Li-ion cell

// ---------- Thermistor divider (fixed resistor on 3V3 side, thermistor on GND side) ----------
const float SUPPLY_VOLTAGE = 3.3;
const float FIXED_RESISTOR = 10000.0;
const float THERMISTOR_R0 = 10000.0;
const float THERMISTOR_T0 = 298.15;
const float THERMISTOR_BETA = 3950.0;

// ---------- Test limits ----------
const float CUTOFF_VOLTAGE = 3.0;
const float MAX_TEMP_C = 45.0;
const float RATED_CAPACITY_AH = 2.8;   // P28A rated capacity
const int CUTOFF_COUNT_REQUIRED = 3;   // consecutive low readings before cutoff

const unsigned long SAMPLE_INTERVAL = 1000;

// ---------- State ----------
unsigned long startTime;
unsigned long previousSampleTime;

float capacityAh = 0.0;
float energyWh = 0.0;
float previousCurrentA = 0.0;
float previousPowerW = 0.0;

float restVoltage = 0.0;
float internalResistanceOhm = NAN;

int lowVoltageCount = 0;
bool dischargeActive = false;

float readTemperatureC(){
	float voltage = analogReadMilliVolts(THERMISTOR_PIN) / 1000.0;

	if(voltage <= 0.01 || voltage >= SUPPLY_VOLTAGE - 0.01){
		return NAN;
	}

	float resistance = FIXED_RESISTOR * voltage / (SUPPLY_VOLTAGE - voltage);
	float inverseTemp = (1.0 / THERMISTOR_T0) + log(resistance / THERMISTOR_R0) / THERMISTOR_BETA;

	return (1.0 / inverseTemp) - 273.15;
}

bool voltageValid(float v){
	return !isnan(v) && v > 0.0 && v <= MAX_VALID_VOLTAGE;
}

void stopDischarge(const char* reason){
	digitalWrite(MOSFET_PIN, LOW);
	dischargeActive = false;

	float elapsed = (millis() - startTime) / 1000.0;
	float soh = (capacityAh / RATED_CAPACITY_AH) * 100.0;

	Serial.println();
	Serial.println("===== DISCHARGE COMPLETE =====");
	Serial.printf("Stopped by: %s\n", reason);
	Serial.printf("Duration: %.0f s\n", elapsed);
	Serial.printf("Rest voltage (start): %.3f V\n", restVoltage);
	Serial.printf("Capacity: %.4f Ah\n", capacityAh);
	Serial.printf("Energy: %.4f Wh\n", energyWh);
	Serial.printf("SOH: %.1f %% (of %.2f Ah rated)\n", soh, RATED_CAPACITY_AH);
	if(!isnan(internalResistanceOhm)){
		Serial.printf("Internal resistance: %.1f mOhm\n", internalResistanceOhm * 1000.0);
	}
}

void startDischarge(){
	if(dischargeActive){
		Serial.println("Discharge already running.");
		return;
	}

	if(!INA.isConnected()){
		Serial.println("ERROR: INA226 not responding. Not starting.");
		return;
	}

	float temperature = readTemperatureC();
	if(isnan(temperature) || temperature >= MAX_TEMP_C){
		Serial.println("ERROR: temperature invalid or above limit. Not starting.");
		return;
	}

	// Average the resting voltage before the load turns on
	float sum = 0.0;
	for(int i = 0; i < 10; i++){
		float v = INA.getBusVoltage();
		if(!voltageValid(v)){
			Serial.println("ERROR: invalid INA226 reading. Not starting.");
			return;
		}
		sum += v;
		delay(100);
	}
	restVoltage = sum / 10.0;
	Serial.printf("Rest voltage: %.3f V\n", restVoltage);

	if(restVoltage <= CUTOFF_VOLTAGE){
		Serial.println("ERROR: battery already at or below cutoff. Not starting.");
		return;
	}

	// Reset results for a fresh run
	capacityAh = 0.0;
	energyWh = 0.0;
	lowVoltageCount = 0;
	internalResistanceOhm = NAN;

	Serial.println("Time(s),Voltage(V),Current(mA),Power(W),Temp(C),Capacity(Ah),Energy(Wh)");

	// Start discharge
	digitalWrite(MOSFET_PIN, HIGH);
	dischargeActive = true;
	startTime = millis();

	// Baseline reading shortly after turn-on (also used for internal resistance)
	delay(200);
	float loadedV = INA.getBusVoltage();
	float loadedI = INA.getCurrent();
	previousSampleTime = millis();

	if(!voltageValid(loadedV) || isnan(loadedI)){
		stopDischarge("invalid INA226 reading");
		return;
	}

	if(loadedI > 0.05){
		internalResistanceOhm = (restVoltage - loadedV) / loadedI;
	}

	// Count the time from MOSFET on to the baseline reading (load turns on almost instantly)
	float dt0 = (previousSampleTime - startTime) / 1000.0;
	float loadedP = loadedV * loadedI;
	if(loadedI > 0){
		capacityAh += loadedI * dt0 / 3600.0;
	}
	if(loadedP > 0){
		energyWh += loadedP * dt0 / 3600.0;
	}

	previousCurrentA = loadedI;
	previousPowerW = loadedP;
}

void handleSerial(){
	if(!Serial.available()){
		return;
	}

	String cmd = Serial.readStringUntil('\n');
	cmd.trim();
	cmd.toUpperCase();

	if(cmd == "START"){
		startDischarge();
	} else if(cmd == "STOP"){
		if(dischargeActive){
			stopDischarge("manual stop");
		} else{
			Serial.println("No discharge running.");
		}
	} else if(cmd.length() > 0){
		Serial.println("Unknown command. Use START or STOP.");
	}
}

void setup(){
	// Keep the MOSFET off from the very start
	pinMode(MOSFET_PIN, OUTPUT);
	digitalWrite(MOSFET_PIN, LOW);

	Serial.begin(115200);
	Serial.setTimeout(50);

	Wire.begin(SDA_PIN, SCL_PIN);
	analogReadResolution(12);
	analogSetPinAttenuation(THERMISTOR_PIN, ADC_11db);

	if(!INA.begin()){
		Serial.println("ERROR: INA226 not detected");
		while(true){
			delay(5000);
		}
	}

	INA.setMaxCurrentShunt(MAX_CURRENT_A, SHUNT_OHMS);

	Serial.println("Battery Monitoring System");
	Serial.println("-------------------------");
	Serial.println("Send START to begin discharge, STOP to end it.");
}

void loop(){
	handleSerial();

	if(!dischargeActive){
		return;
	}

	unsigned long now = millis();

	if(now - previousSampleTime < SAMPLE_INTERVAL){
		return;
	}

	if(!INA.isConnected()){
		stopDischarge("INA226 not responding");
		return;
	}

	float dt = (now - previousSampleTime) / 1000.0;
	previousSampleTime = now;

	float voltage = INA.getBusVoltage();
	float currentA = INA.getCurrent();

	if(!voltageValid(voltage) || isnan(currentA)){
		stopDischarge("invalid INA226 reading");
		return;
	}

	float currentmA = currentA * 1000.0;
	float temperature = readTemperatureC();
	float power = voltage * currentA;
	float elapsed = (now - startTime) / 1000.0;

	float avgCurrent = (previousCurrentA + currentA) / 2.0;
	float avgPower = (previousPowerW + power) / 2.0;

	if(avgCurrent > 0){
		capacityAh += avgCurrent * dt / 3600.0;
	}

	if(avgPower > 0){
		energyWh += avgPower * dt / 3600.0;
	}

	previousCurrentA = currentA;
	previousPowerW = power;

	// CSV log line
	Serial.printf("%.0f,%.3f,%.2f,%.3f,", elapsed, voltage, currentmA, power);
	if(isnan(temperature)){
		Serial.print("NaN");
	} else{
		Serial.printf("%.2f", temperature);
	}
	Serial.printf(",%.6f,%.6f\n", capacityAh, energyWh);

	// Cutoff checks
	if(voltage <= CUTOFF_VOLTAGE){
		lowVoltageCount++;
	} else{
		lowVoltageCount = 0;
	}

	if(isnan(temperature)){
		stopDischarge("invalid temperature reading");
	} else if(temperature >= MAX_TEMP_C){
		stopDischarge("over temperature");
	} else if(lowVoltageCount >= CUTOFF_COUNT_REQUIRED){
		stopDischarge("cutoff voltage");
	}
}