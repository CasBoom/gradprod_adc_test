#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>

#define SS_PIN 7
#define SPI_FREQ 1000

// continously checks both 1 voltage channel or current channel
// by changing VOLTAGE to 1, a Vin0 will be selected
// by chanign VOLTAGE to 0, Iin0 will be selected
#define VOLTAGE 1

void printBinary(uint8_t value)
{
    for (int i = 7; i >= 0; i-- )
    {
        Serial.print(value>>i & 0b00000001);
    }
}

/*
@brief
Function that first writes to the comm register, then writes data to the selected register
*/
uint8_t readWriteADCRegister(uint8_t read, uint8_t address, uint8_t data[], uint8_t size){
  // start SPI
  uint8_t temp=0;
  SPI.beginTransaction(SPISettings(SPI_FREQ, MSBFIRST, SPI_MODE3));
  if(read){
    Serial.println("Reading...");
    address |= address | 0b01000000;
  } else {
    Serial.println("Writing...");
  }
    
  Serial.print("Address: ");
  Serial.println(address & 0b00111111);
  // toggle SS pin
  digitalWrite(SS_PIN, HIGH);
  delay(100);
  digitalWrite(SS_PIN, LOW);

  // Serial.println(SPI.transfer(address), HEX); // first, write to the comms register at address 0x00 to select a register 
  for(int i=0; i<size; i++){
    temp = SPI.transfer(data[i]); // then, write data to register
    data[i] = temp;
    Serial.print(i);
    Serial.print(": ");
    printBinary(data[i]);
    Serial.println(" BIN");
    
  }
  digitalWrite(SS_PIN, HIGH); // disable SS
  SPI.endTransaction(); // end SPI com
  delay(500);
  return 0;
}

uint8_t data[3] = {0, 0, 0}; //data buffer

void setup() {
  // set up serial
  Serial.begin(9600);

  // set up SPI
  SPI.begin();
  
  
  /* 
    set up ADC
  */

  while(true){

  // set ADCmode
  data[0] = 0b10000000; // enable refout
  data[1] = 0b00100000; // enable stand by mode
  readWriteADCRegister(0, 0x01, data, 2); 

  // set interface mode
  data[0] = 0b00000000; // -
  data[1] = 0b00000001; // set resolution to 16-bit data
  readWriteADCRegister(0, 0x02, data, 2); 

  // Set up channel 0
  if(VOLTAGE){
    // enable voltage input
    data[0] = 0b10000000; // enable Channel 0
    data[1] = 0b00010000; // select Vin0
    readWriteADCRegister(0, 0x10, data, 2); 
  }else{
    // enable current input
    data[0] = 0b10000001; // enable Channel 0
    data[1] = 0b11101000; // select Iin0+, Iin0-
    readWriteADCRegister(0, 0x10, data, 2); 
  }
  // set setup register config 0
  data[0] = 0b00000011; // Enable input buffers
  data[1] = 0b00100000; // Use internal reference
  readWriteADCRegister(0, 0x20, data, 2); 

  readWriteADCRegister(1, 0x01, data, 2); 
  readWriteADCRegister(1, 0x02, data, 2); 
  readWriteADCRegister(1, 0x10, data, 2); 
  readWriteADCRegister(1, 0x20, data, 2); 
    delay(1000);
  }
}

uint32_t conversionResult = 0;
uint32_t calcVolt = 0;
uint32_t calcCur = 0;

void loop() {
  // start conversion
  data[0] = 0b10000000; // enable refout
  data[1] = 0b00010000; // enable single conversion
  readWriteADCRegister(0, 0x01, data, 2); 

  // wait for conversion to be ready
  do {
    data[0] = 0b10000000; // clear data buffer
    readWriteADCRegister(1, 0x00, data, 1); 
  } while (data[0] & 0b10000000); // while ready is high keep polling
  
  readWriteADCRegister(1, 0x04, data, 3); // get Data
  // Serial.println(data[0]);
  // Serial.println(data[1]);
  // Serial.println(data[2]);
  conversionResult = (((uint32_t)data[0]) << 16) + (data[1] << 8) + + (data[0] << 0);

  // print result
  Serial.print("Raw conversion result: ");
  Serial.println(conversionResult);

  Serial.print("Result: ");
  if(VOLTAGE){
    calcVolt = (10*conversionResult/1048576); // multiply by 10 Voltage range, divide by amount of bits
    Serial.print(calcVolt);
    Serial.println("V ");
  } else {
    calcVolt = (24*conversionResult/1048576); // multiply by 24 mA range, divide by amount of bits
    Serial.print(calcCur);
    Serial.println("mA ");
  }

  // delay for a bit
  delay(1000);
}

