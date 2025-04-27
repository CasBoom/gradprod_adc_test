/// \file
#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>

 //! Slave select pin
#define SS_PIN 10

//! Defines the SPI frequency
#define SPI_FREQ 100000 

//! If 1, the ADC will sample Vin0. If 0, the ADC will sample Iin0.
#define VOLTAGE 1 


//! Function that will perform a software reset on the ADC. Always returns 0.
uint8_t resetADC(){
  SPI.beginTransaction(SPISettings(SPI_FREQ, MSBFIRST, SPI_MODE3));
  digitalWrite(SS_PIN, HIGH);
  delay(100);
  digitalWrite(SS_PIN, LOW);

  for(uint8_t i=0; i<8; i++){
    SPI.transfer(0xFF);
  }
  digitalWrite(SS_PIN, HIGH);
  SPI.endTransaction(); // end SPI com
  Serial.println("Device reset.");
  delay(500);
  return 0;
}

//! Function that first writes to the comm register, then writes data to the selected register. Always returns 0.
uint8_t writeADCRegister(uint8_t address, uint8_t data[], uint8_t size){
  // start SPI
  SPI.beginTransaction(SPISettings(SPI_FREQ, MSBFIRST, SPI_MODE3));

  // toggle SS pin
  digitalWrite(SS_PIN, HIGH);
  delay(100);
  digitalWrite(SS_PIN, LOW);

  SPI.transfer(address); // write to the comms register at address 0x00 to select a register 
  for(int i=0; i<size; i++){
    SPI.transfer(data[i]); // then, write data to register
  }
  digitalWrite(SS_PIN, HIGH); // disable SS
  SPI.endTransaction(); // end SPI com
  delay(500);
  return 0;
}

//! Function that first writes to the comm register, then reads data from the selected register. Always returns 0.
uint8_t readADCRegister(uint8_t address, uint8_t data[], uint8_t size){
  // start SPI
  SPI.beginTransaction(SPISettings(SPI_FREQ, MSBFIRST, SPI_MODE3));
  address = address | 0b01000000; // set flag to read

  // toggle SS pin
  digitalWrite(SS_PIN, HIGH);
  delay(100);
  digitalWrite(SS_PIN, LOW);

  SPI.transfer(address); // write to the comms register at address 0x00 to select a register 
  delay(50);
  for(int i=0; i<size; i++){
    data[i] = SPI.transfer(0); // then, read data from register
  }
  digitalWrite(SS_PIN, HIGH); // disable SS
  SPI.endTransaction(); // end SPI com
  delay(500);
  return 0;
}

//! Array that is passed to both the reading and writing functions
uint8_t data[3] = {0, 0, 0}; //data buffer

void setup() {
  // set up serial
  Serial.begin(9600);

  // set up SPI
  pinMode(SS_PIN, OUTPUT);
  digitalWrite(SS_PIN, HIGH);

  SPI.begin();

  /* 
    set up ADC
  */
  readADCRegister(0x07, data, 1); 
 
  // set ADCmode
  data[0] = 0b10000000; // enable refout
  data[1] = 0b00100000; // enable stand by mode
  writeADCRegister(0x01, data, 2); 

  // set interface mode
  data[0] = 0b00000000; // -
  data[1] = 0b00000001; // set resolution to 16-bit data
  writeADCRegister(0x02, data, 2); 

  // Set up channel 0
  if(VOLTAGE){
    // enable voltage input
    data[0] = 0b10000000; // enable Channel 0
    data[1] = 0b00010000; // select Vin0
    writeADCRegister(0x10, data, 2); 
  }else{
    // enable current input
    data[0] = 0b10000001; // enable Channel 0
    data[1] = 0b11101000; // select Iin0+, Iin0-
    writeADCRegister(0x10, data, 2); 
  }
  // set setup register config 0
  data[0] = 0b00000011; // Enable input buffers
  data[1] = 0b00100000; // Use internal reference
  writeADCRegister(0x20, data, 2); 

  readADCRegister(0x01, data, 2); 
  readADCRegister(0x02, data, 2); 
  readADCRegister(0x10, data, 2); 
  readADCRegister(0x20, data, 2); 
}

void loop() {
  // start conversion
  data[0] = 0b10000000; // enable refout
  data[1] = 0b00010000; // enable single conversion
  writeADCRegister(0x01, data, 2); 

  // wait for conversion to be ready
  do {
    data[0] = 0b10000000; // preset data buffer
    readADCRegister(0x00, data, 1); 
  } while (data[0] & 0b10000000); // keep polling while ready is high
  
  Serial.println("...");
  Serial.println("...");
  Serial.println("...");
  readADCRegister(0x04, data, 2); // get Data
  uint16_t readout = (data[0]<<8)+data[1];
  Serial.print("readout: ");
  Serial.println(readout);
  delay(2000);
}