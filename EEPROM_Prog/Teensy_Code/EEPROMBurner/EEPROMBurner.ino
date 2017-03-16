/* 28C64 EEPROM Programmer

  Project to implement an EEPROM programmer for 28C64 8k EEPROM.
   
*/
// Hardware ports for fast writes to 74*595.  Github source core_pins.h kinetis.h
const int SER_DATA = 0;   // GPIOB_PSOR (1 << 16) 
const int SCLK_CLOCK = 1; // GPIOB_PSOR (1 << 17)
const int RCLK_LATCH = 2; // GPIOD_PSOR (1 << 0)

const int CE = 3;         // GPIOA_PSOR (1 << 12)
const int OE = 4;         // GPIOA_PSOR (1 << 13)
const int WE = 5;         // GPIOD_PSOR (1 << 7)

const int D0 = 16;        // GPIOB_PSOR (1 << 0)
const int D1 = 17;        // GPIOB_PSOR (1 << 1)
const int D2 = 18;        // GPIOB_PSOR (1 << 3)
const int D3 = 19;        // GPIOB_PSOR (1 << 2)
const int D4 = 20;        // GPIOD_PSOR (1 << 5)
const int D5 = 21;        // GPIOD_PSOR (1 << 6)  
const int D6 = 22;        // GPIOC_PSOR (1 << 1)
const int D7 = 23;        // GPIOC_PSOR (1 << 2)

const int ledPin = 13;    // GPIOC_PSOR (1 << 5)

uint16_t EEPROM_Address = 0x0000;  

byte DataToBurn[] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f};
byte DataRead[2048] =   {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
uint8_t DataByte = 0;

bool onoff = true;
bool programOnce = false;   // read only
bool displayOnce = true;

//  Outputs the 12 bit address to the shift register.  I shift out all sixteen bits
//  because I may want to use this for a 64k device eventually, I could only do the
//  tweleve bits and save a few cycles but I believe it is fast enough.
//  The method used is direct writes to the set and clear registers, these registers
//  take care of only effecting the bit your are setting or clearing hence no masking
//  of bits is required.
  
void setAddressBus(uint16_t address) {
  uint16_t Mask = 1;
  GPIOD_PCOR = (1<<0);    // Clear the latch i.e. set it low
  do {
    if(address & Mask)
      GPIOB_PSOR = (1 << 16);
    else
      GPIOB_PCOR = (1 << 16);

    GPIOB_PSOR = (1 << 17); // Clock high
    GPIOB_PCOR = (1 << 16); // Clear the data port to prevent bleed through...
    GPIOB_PCOR = (1 << 17); // Clock back to low
    Mask <<= 1;
  } while(Mask > 0);
  GPIOD_PSOR = (1<<0);    // Set the latch i.. set it high puts data on pins of 74hc595
}

// Put the data onto the pins for the data bus. 
void writeDataBus(uint8_t data) {
  (data & 1) ? GPIOB_PSOR = (1 << 0) : GPIOB_PCOR = (1 << 0);
  (data & 2) ? GPIOB_PSOR = (1 << 1) : GPIOB_PCOR = (1 << 1);
  (data & 4) ? GPIOB_PSOR = (1 << 3) : GPIOB_PCOR = (1 << 3);
  (data & 8) ? GPIOB_PSOR = (1 << 2) : GPIOB_PCOR = (1 << 2);
  (data & 16) ? GPIOD_PSOR = (1 << 5) : GPIOD_PCOR = (1 << 5);
  (data & 32) ? GPIOD_PSOR = (1 << 6) : GPIOD_PCOR = (1 << 6);
  (data & 64) ? GPIOC_PSOR = (1 << 1) : GPIOC_PCOR = (1 << 1);
  (data & 128) ? GPIOC_PSOR = (1 << 2) : GPIOC_PCOR = (1 << 2);
}

void setDataBusRead() {
  pinMode(D0, INPUT);
  pinMode(D1, INPUT);
  pinMode(D2, INPUT);
  pinMode(D3, INPUT);
  pinMode(D4, INPUT);
  pinMode(D5, INPUT);
  pinMode(D6, INPUT);
  pinMode(D7, INPUT);
}

void setDataBusWrite() {
  pinMode(D0, OUTPUT);
  pinMode(D1, OUTPUT);
  pinMode(D2, OUTPUT);
  pinMode(D3, OUTPUT);
  pinMode(D4, OUTPUT);
  pinMode(D5, OUTPUT);
  pinMode(D6, OUTPUT);
  pinMode(D7, OUTPUT);  
}

byte readDataBus() {
  return ((digitalRead(D7) << 7) +
          (digitalRead(D6) << 6) +
          (digitalRead(D5) << 5) +
          (digitalRead(D4) << 4) +
          (digitalRead(D3) << 3) +
          (digitalRead(D2) << 2) +
          (digitalRead(D1) << 1) +
          (digitalRead(D0) << 0));
}

void set_ce(byte state) {
  digitalWrite(CE, state);
}

void set_oe(byte state) {
  digitalWrite(OE, state);
}

void set_we(byte state) {
  digitalWrite(WE, state);
}

byte readByte(uint16_t address) {
  byte data = 0;

  setDataBusRead();
  set_oe(HIGH);
  set_ce(LOW);
  set_we(HIGH);
  setAddressBus(address);
  set_oe(LOW);
  data = readDataBus();
  set_oe(HIGH);
  return data;
}

void writeByte(uint16_t address, byte data) {
  byte loopcount = 0;

    set_oe(HIGH);
    set_we(HIGH);
    setAddressBus(address);
    setDataBusWrite();
    writeDataBus(data);
    set_ce(LOW);
    delayMicroseconds(1);
    set_we(LOW);
    delayMicroseconds(1);
    set_we(HIGH);
    setDataBusRead();
    set_oe(LOW);
    while(data != readDataBus()) {
      loopcount++;
    };

    set_oe(HIGH);
    set_ce(HIGH);
}


void PrintData(int address, int numberBytes)
{
  int workingAddress = address;
  int workingBytes = numberBytes;
  
  do {
    Serial.printf("0x%04x ", workingAddress);
    for(int x = 0; x < 16; x++)
    {
      if(workingBytes)
      {
        Serial.printf("%02x ", DataRead[x+workingAddress]);
        workingBytes--;
      }
    }
    Serial.println();
    workingAddress+=16;
  } while(workingBytes > 0);
  Serial.println();
}

// the setup() method runs once, when the sketch starts
void setup() {
  // initialize the digital pins as output.
  pinMode(SER_DATA, OUTPUT);
  pinMode(SCLK_CLOCK, OUTPUT);
  pinMode(RCLK_LATCH, OUTPUT);

  pinMode(CE, OUTPUT);
  pinMode(OE, OUTPUT);
  pinMode(WE, OUTPUT);
  
  setDataBusWrite();
  
  pinMode(ledPin, OUTPUT);
  Serial.begin(9600);
}

// the loop() methor runs over and over again,
// as long as the board has power

void loop() {
  onoff ^= 1;

// this is the program test,,,
  if (programOnce) {
    for(int x = 0; x < 16; x++){
        writeByte(EEPROM_Address, DataToBurn[x]);
        EEPROM_Address++;
    }
    EEPROM_Address = 0;
    programOnce = false;
  }

  for(EEPROM_Address = 0; EEPROM_Address < 2048; EEPROM_Address++)
  {
    setAddressBus(EEPROM_Address);
    DataRead[EEPROM_Address] = readByte(EEPROM_Address);
  }

  PrintData(0, 128);
  
  if(onoff)
    GPIOC_PSOR = 0x20;
   else
    GPIOC_PCOR = 0x20;
 
  EEPROM_Address++;
 
  if(EEPROM_Address > 15){
    EEPROM_Address = 0;
    DataByte++;
  }

  delay(1000);
}

