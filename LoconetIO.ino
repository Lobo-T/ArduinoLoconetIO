#include <LocoNet.h>
#include <Wire.h>

#define ARRAYELEMENTCOUNT(x) (sizeof (x) / sizeof (x)[0])

#define DEBUG

//Locoshield transmit pin. (Receive pin må alltid være 8 (ICP1) på UNO, og 48 (ICP5) på MEGA).
#define LNtxPin 7

//Startadresse.  JMRI Hardware Address. Verdi mellom 1 og 2048.  Unngå 1017 - 1020.
//Dette er adressen til første inngang og utgang.  De andre i arrayen blir nummerert fortløpende.
//Eksempel:  Med JMRI_ADR 1, fire utganger og 8 innganger vil du få JMRI adresser LL1 til LL4 på utgangene
//og LS1 til LS8 på inngangene.
#define JMRI_ADR 1

//Addressene til i2c kretsene
#define MCP1_ADR B0100000
#define MCP2_ADR B0100001
#define MCP3_ADR B0100010
#define MCP4_ADR B0100100
#define MCP5_ADR B0100101
#define MCP6_ADR B0100110
#define PCF1_ADR B0111000
#define PCF2_ADR B0111001
#define PCF3_ADR B0111010

struct ioExpanderStat_t{
  byte adr;           //i2c adresse
  byte type;          //Kretstype
  byte data;          //Data
  byte dir;           //Bitmask for retning 1 in, 0 ut.
  byte pullup;        //1 pullup slått på 0 pullup slått av for de kretsene som støtter det.
  boolean changed;    //true betyr at data er endret og må sendes til kretsen
};

#define IC_MCP23008  2
#define IC_PCF8574   3

//Legg til alle io expander kretser som skal brukes i denne arrayen.
//Posisjon .adr er i2c adressen. .type er type krets, enten IC_MCP23008 eller IC_PCF8574
//Eksempel for en MCP23008 krets:   {.adr = B0100000, .type = IC_MCP23008}
ioExpanderStat_t ioExpanderStatus[]={
  {.adr = MCP4_ADR, .type = IC_MCP23008},
  {.adr = MCP5_ADR, .type = IC_MCP23008},
  {.adr = MCP6_ADR, .type = IC_MCP23008}
};

//MCP23008
#define GP0 0
#define GP1 1
#define GP2 2
#define GP3 3
#define GP4 4
#define GP5 5
#define GP6 6
#define GP7 7
#define MCP_IODIR   0
#define MCP_IPOL    1
#define MCP_GPINTEN 2
#define MCP_DEFVAL  3
#define MCP_INTCON  4
#define MCP_IOCON   5
#define MCP_GPPU    6
#define MCP_INTF    7
#define MCP_INTCAP  8
#define MCP_GPIO    9
#define MCP_OLAT    10

//Pinnetyper
#define INTERN_PIN    1
#define MCP23008_PIN  2
#define PCF8574_PIN   3

struct pin_t{
  byte pintype;
  byte pinno;
  byte adr;
};

//De pinnene som er i denne arrayen vil bli satt som innganger
pin_t innportPins[] = {  
  {INTERN_PIN,6,0},
  {INTERN_PIN,9,0},
  {INTERN_PIN,10,0},
  {INTERN_PIN,11,0},
  {INTERN_PIN,12,0},
  {MCP23008_PIN,GP0,MCP4_ADR},
  {MCP23008_PIN,GP1,MCP4_ADR},
  {MCP23008_PIN,GP2,MCP4_ADR},
  {MCP23008_PIN,GP3,MCP4_ADR},
  {MCP23008_PIN,GP4,MCP4_ADR},
  {MCP23008_PIN,GP5,MCP4_ADR},
  {MCP23008_PIN,GP6,MCP4_ADR},
  {MCP23008_PIN,GP7,MCP4_ADR},
  {MCP23008_PIN,GP0,MCP6_ADR},
  {MCP23008_PIN,GP1,MCP6_ADR},
  {MCP23008_PIN,GP2,MCP6_ADR},
  {MCP23008_PIN,GP3,MCP6_ADR},
  {MCP23008_PIN,GP4,MCP6_ADR},
  {MCP23008_PIN,GP5,MCP6_ADR},
  {MCP23008_PIN,GP6,MCP6_ADR},
  {MCP23008_PIN,GP7,MCP6_ADR}
};

//De pinnene som er i denne arrayen vil bli satt som utganger
pin_t utportPins[] = {
  {INTERN_PIN,2,0},
  {INTERN_PIN,3,0},
  {INTERN_PIN,4,0},
  {INTERN_PIN,5,0},
  {INTERN_PIN,13,0},
  {MCP23008_PIN,GP0,MCP5_ADR},
  {MCP23008_PIN,GP1,MCP5_ADR},
  {MCP23008_PIN,GP2,MCP5_ADR},
  {MCP23008_PIN,GP3,MCP5_ADR},
  {MCP23008_PIN,GP4,MCP5_ADR},
  {MCP23008_PIN,GP5,MCP5_ADR},
  {MCP23008_PIN,GP6,MCP5_ADR},
  {MCP23008_PIN,GP7,MCP5_ADR}
};

boolean innportState[ARRAYELEMENTCOUNT(innportPins)];
boolean innportStateLast[ARRAYELEMENTCOUNT(innportPins)];


void setup() {
  #ifdef DEBUG
  Serial.begin(57600);
  #endif
  Wire.begin();
  
  //Sett pinmode
  for (byte i = 0; i < ARRAYELEMENTCOUNT(innportPins); i++) {
    pinDir(innportPins[i], INPUT_PULLUP);
  }
  for (byte i = 0; i < ARRAYELEMENTCOUNT(utportPins); i++) {
    pinDir(utportPins[i], OUTPUT);
  }
  for (byte i = 0; i < ARRAYELEMENTCOUNT(ioExpanderStatus); i++) {
    setup_mcp(ioExpanderStatus[i].adr,ioExpanderStatus[i].dir,ioExpanderStatus[i].pullup);
  }

  //Vi leser pinnene og oppdaterer innportStateLast før vi starter hovedprogrammet.
  for (byte i = 0; i < ARRAYELEMENTCOUNT(ioExpanderStatus); i++) {
    ioExpanderStatus[i].data=mcp_read_port(ioExpanderStatus[i].adr);
  }
  for (byte i = 0; i < ARRAYELEMENTCOUNT(innportPins); i++) {
    innportState[i] = pinGet(innportPins[i]);
  }
  memcpy(innportStateLast, innportState, sizeof(innportState));

  // initialize the LocoNet interface
  LocoNet.init(LNtxPin);

  #ifdef DEBUG
  Serial.print(F("Setup end RAM:"));
  Serial.println(freeRam());
  #endif
}


void loop() {
  //Les først inn tilstand på eksterne IO kretser
  for (byte i = 0; i < ARRAYELEMENTCOUNT(ioExpanderStatus); i++) {
    ioExpanderStatus[i].data=mcp_read_port(ioExpanderStatus[i].adr);
  }

  //Sjekk om vi har fått en Loconetpakke
  if (lnMsg *LnPacket = LocoNet.receive()) {
    #ifdef DEBUG
    Serial.print(F("From lnet: "));
    Serial.print(F("OPCode: "));
    Serial.print(LnPacket->sz.command);
    Serial.print(F(" Data1: "));
    Serial.print(LnPacket->data[1]);
    Serial.print(F(" Data2: "));
    Serial.println(LnPacket->data[2]);
    #endif

    //Vi vil få en callback til en av notify-funksjonene under hvis dette er en pakke av Switch eller Sensor type.
    LocoNet.processSwitchSensorMessage(LnPacket);
  }

  //Sjekk om innganger er endret fra sist gang vi leste
  for (byte i = 0; i < ARRAYELEMENTCOUNT(innportPins); i++) {
    innportState[i] = pinGet(innportPins[i]);

    //Hvis endret send OPC_INPUT_REP
    if (innportState[i] != innportStateLast[i]) {
      int adr = i + JMRI_ADR;
      #ifdef DEBUG
      Serial.println(F("--------------------------------"));
      Serial.print(F("\nUlik: "));
      Serial.println(innportPins[i].pinno);
      Serial.print(F("Adresse:"));
      Serial.println(adr);
      #endif

      LN_STATUS lnstat = LocoNet.reportSensor(adr, innportState[i]);
      #ifdef DEBUG
      Serial.print(F("ReportSensorLoconetStatus: "));
      Serial.println(lnstat);
      Serial.println(F("--------------------------------"));
      #endif
    }
  }

  //Kopier det vi leste nå til det vi skal sammenligne med neste gang.
  memcpy(innportStateLast, innportState, sizeof(innportState));
  
  //Skriv data til eksterne io porter hvis vi har endret dem
  writeOutports();
}  //loop() slutt


//Callbacks fra processSwitchSensorMessage
void notifySensor( uint16_t Address, uint8_t State ) {
  #ifdef DEBUG
  Serial.println(F("notifySensor"));
  Serial.print(F("Adr: "));
  Serial.print(Address);
  Serial.print(F(" State: "));
  Serial.println(State);
  #endif
}

void notifySwitchRequest( uint16_t Address, uint8_t Output, uint8_t Direction ) {
  #ifdef DEBUG
  Serial.println(F("notifySwitchRequest"));
  Serial.print(F("Adr: "));
  Serial.print(Address);
  Serial.print(F(" Output: "));
  Serial.print(Output);
  Serial.print(F(" Direction: "));
  Serial.println(Direction);
  #endif

  //Sett utgang hvis dette er en av våre aresser
  if (Output && (Address >= JMRI_ADR && Address < JMRI_ADR + ARRAYELEMENTCOUNT(utportPins))) {
    #ifdef DEBUG
    Serial.println(F("---Set output---"));
    #endif
    pinSet(utportPins[Address - JMRI_ADR], Direction);    
  }

  //Sensor forespørsel fra JMRI, rapporter status på innganger.
  //Hvilken av forespørslene (1017-1020) vi egentlig skal svare på er avhengig av hvilken adresse vi har
  //men jeg orker ikke prøve å finne ut av hvordan dettte egentlig er ment å fungere nå, så vi svarer på 1018.
  if (!Output && !Direction && Address == 1018) {
    for (int i = 0; i < ARRAYELEMENTCOUNT(innportPins); i++) {
      LN_STATUS lnstat = LocoNet.reportSensor(i + JMRI_ADR, innportState[i]);
      #ifdef DEBUG
      Serial.print(F("ReportSensorLoconetStatus: "));
      Serial.println(lnstat);
      #endif
    }
  }
}

void notifySwitchReport( uint16_t Address, uint8_t Output, uint8_t Direction ) {
  #ifdef DEBUG
  Serial.println(F("notifySwitchReport"));
  Serial.print(F("Adr: "));
  Serial.print(Address);
  Serial.print(F(" Outp: "));
  Serial.println(Direction);
  #endif
}

void notifySwitchState( uint16_t Address, uint8_t Output, uint8_t Direction ) {
  #ifdef DEBUG
  Serial.println(F("notifySwtichState"));
  Serial.print(F("Adr: "));
  Serial.print(Address);
  Serial.print(F(" Outp: "));
  Serial.println(Direction);
  #endif
}

int freeRam () {
  extern int __heap_start, *__brkval;
  int v;
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval);
}

