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
#define mcp1_adr B0100000
#define mcp2_adr B0100001
#define mcp3_adr B0100010
#define mcp4_adr B0100100
#define mcp5_adr B0100101
#define mcp6_adr B0100110
#define pcf1_adr B0111000
#define pcf2_adr B0111001
#define pcf3_adr B0111010

//Legg til alle io expander kretser som skal brukes i denne arrayen.
//Posisjon 0 er i2c adressen, Posisjon 1 vil bli brukt til data, 2 til retningstatus, 3 til pullupstatus
//og 4 angir om porten har blitt skrevet til og med sendes til kretsen.
byte ioExpanderStatus[][5]={ 
  {mcp4_adr,0,0,0,0},
  {mcp5_adr,0,0,0,0},
  {mcp6_adr,0,0,0,0}  
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

//De pinnene som er i denne arrayen vil bli satt som innganger
byte innportPins[][3] = {  
  {INTERN_PIN,6,0},
  {INTERN_PIN,9,0},
  {INTERN_PIN,10,0},
  {INTERN_PIN,11,0},
  {INTERN_PIN,12,0},
  {MCP23008_PIN,GP0,mcp4_adr},
  {MCP23008_PIN,GP1,mcp4_adr},
  {MCP23008_PIN,GP2,mcp4_adr},
  {MCP23008_PIN,GP3,mcp4_adr},
  {MCP23008_PIN,GP4,mcp4_adr},
  {MCP23008_PIN,GP5,mcp4_adr},
  {MCP23008_PIN,GP6,mcp4_adr},
  {MCP23008_PIN,GP7,mcp4_adr},
  {MCP23008_PIN,GP0,mcp6_adr},
  {MCP23008_PIN,GP1,mcp6_adr},
  {MCP23008_PIN,GP2,mcp6_adr},
  {MCP23008_PIN,GP3,mcp6_adr},
  {MCP23008_PIN,GP4,mcp6_adr},
  {MCP23008_PIN,GP5,mcp6_adr},
  {MCP23008_PIN,GP6,mcp6_adr},
  {MCP23008_PIN,GP7,mcp6_adr}
};

//De pinnene som er i denne arrayen vil bli satt som utganger
byte utportPins[][3] = {
  {INTERN_PIN,2,0},
  {INTERN_PIN,3,0},
  {INTERN_PIN,4,0},
  {INTERN_PIN,5,0},
  {MCP23008_PIN,GP0,mcp5_adr},
  {MCP23008_PIN,GP1,mcp5_adr},
  {MCP23008_PIN,GP2,mcp5_adr},
  {MCP23008_PIN,GP3,mcp5_adr},
  {MCP23008_PIN,GP4,mcp5_adr},
  {MCP23008_PIN,GP5,mcp5_adr},
  {MCP23008_PIN,GP6,mcp5_adr},
  {MCP23008_PIN,GP7,mcp5_adr}
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
    setup_mcp(ioExpanderStatus[i][0],ioExpanderStatus[i][2],ioExpanderStatus[i][3]);
  }

  //Vi leser pinnene og oppdaterer innportStateLast før vi starter hovedprogrammet.
  for (byte i = 0; i < ARRAYELEMENTCOUNT(ioExpanderStatus); i++) {
    ioExpanderStatus[i][1]=mcp_read_port(ioExpanderStatus[i][0]);
  }
  for (byte i = 0; i < ARRAYELEMENTCOUNT(innportPins); i++) {
    innportState[i] = pinGet(innportPins[i]);
  }
  memcpy(innportStateLast, innportState, ARRAYELEMENTCOUNT(innportState));

  // initialize the LocoNet interface
  LocoNet.init(LNtxPin);

  #ifdef DEBUG
  Serial.print(F("Setup end RAM:"));
  Serial.println(freeRam());
  #endif
}


void loop() {
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

  //Les pinner sjekk om de er endret fra sist gang vi leste
  for (byte i = 0; i < ARRAYELEMENTCOUNT(ioExpanderStatus); i++) {
    ioExpanderStatus[i][1]=mcp_read_port(ioExpanderStatus[i][0]);
  }
  for (byte i = 0; i < ARRAYELEMENTCOUNT(innportPins); i++) {
    innportState[i] = pinGet(innportPins[i]);
    //Serial.print(innportState[i]);
    //Hvis endret send OPC_INPUT_REP
    if (innportState[i] != innportStateLast[i]) {
      int adr = i + JMRI_ADR;
      #ifdef DEBUG
      Serial.println("--------------------------------");
      Serial.print("\nUlik: ");
      Serial.println(innportPins[i][1]);
      Serial.print("Adresse:");
      Serial.println(adr);
      #endif

      LN_STATUS lnstat = LocoNet.reportSensor(adr, innportState[i]);
      #ifdef DEBUG
      Serial.print("ReportSensorLoconetStatus: ");
      Serial.println(lnstat);
      Serial.println("--------------------------------");
      #endif

    }
  }
  //Serial.println();

  //Kopier det vi leste nå til det vi skal sammenligne med neste gang.
  memcpy(innportStateLast, innportState, ARRAYELEMENTCOUNT(innportState));
  writeOutports();
}  //loop() slutt


//Callbacks fra processSwitchSensorMessage
void notifySensor( uint16_t Address, uint8_t State ) {
  #ifdef DEBUG
  Serial.println("notifySensor");
  Serial.print("Adr: ");
  Serial.print(Address);
  Serial.print(" State: ");
  Serial.println(State);
  #endif
}

void notifySwitchRequest( uint16_t Address, uint8_t Output, uint8_t Direction ) {
  #ifdef DEBUG
  Serial.println("notifySwitchRequest");
  Serial.print("Adr: ");
  Serial.print(Address);
  Serial.print(" Output: ");
  Serial.print(Output);
  Serial.print(" Direction: ");
  Serial.println(Direction);
  #endif

  //Sett utgang hvis dette er en av våre aresser
  if (Output && (Address >= JMRI_ADR && Address < JMRI_ADR + ARRAYELEMENTCOUNT(utportPins))) {
    #ifdef DEBUG
    Serial.println("---Set output---");
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
      Serial.print("ReportSensorLoconetStatus: ");
      Serial.println(lnstat);
      #endif
    }
  }
}

void notifySwitchReport( uint16_t Address, uint8_t Output, uint8_t Direction ) {
  #ifdef DEBUG
  Serial.println("notifySwitchReport");
  Serial.print("Adr: ");
  Serial.print(Address);
  Serial.print(" Outp: ");
  Serial.println(Direction);
  #endif
}

void notifySwitchState( uint16_t Address, uint8_t Output, uint8_t Direction ) {
  #ifdef DEBUG
  Serial.println("notifySwtichState");
  Serial.print("Adr: ");
  Serial.print(Address);
  Serial.print(" Outp: ");
  Serial.println(Direction);
  #endif
}

int freeRam () {
  extern int __heap_start, *__brkval;
  int v;
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval);
}

