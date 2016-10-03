#include <LocoNet.h>

//#define DEBUG

//Locoshield transmit pin. (Receive pin må alltid være 8 (ICP1) på UNO, og 48 (ICP5) på MEGA).
#define LNtxPin 7

//Startadresse.  JMRI Hardware Address. Verdi mellom 1 og 2048.  Unngå 1017 - 1020.
//Dette er adressen til første inngang og utgang.  De andre i arrayen blir nummerert fortløpende.
//Eksempel:  Med JMRI_ADR 1, fire utganger og 8 innganger vil du få JMRI adresser LL1 til LL4 på utgangene
//og LS1 til LS8 på inngangene.
#define JMRI_ADR 1

//Setter navn på pinnenummre.
#define ut0 3
#define ut1 4
#define ut2 5
#define ut3 6
#define inn0 9
#define inn1 10
#define inn2 11
#define inn3 12
#define inn4 A0
#define inn5 A1
#define inn6 A2
#define inn7 A3


//De pinnene som er i denne arrayen vil bli satt som innganger
byte innportPins[] = {
  inn0,
  inn1,
  inn2,
  inn3,
  inn4,
  inn5,
  inn6,
  inn7
};

//De pinnene som er i denne arrayen vil bli satt som utganger
byte utportPins[] = {
  ut0,
  ut1,
  ut2,
  ut3
};

boolean innportState[sizeof(innportPins)];
boolean innportStateLast[sizeof(innportPins)];


void setup() {
  #ifdef DEBUG
  Serial.begin(57600);
  #endif
  
  //Sett pinmode
  for (byte i = 0; i < sizeof(innportPins); i++) {
    pinMode(innportPins[i], INPUT_PULLUP);
  }
  for (byte i = 0; i < sizeof(utportPins); i++) {
    pinMode(utportPins[i], OUTPUT);
  }
  //Over blir alle pinnene i innportPins satt til input med innebyggde pullupmotstander slått på.
  //Hvis du har behov for at noen av dem ikke har pullup så bare redefiner dem det gjelder som INPUT under her.

  //Vi leser pinnene og oppdaterer innportStateLast før vi starter hovedprogrammet.
  for (byte i = 0; i < sizeof(innportPins); i++) {
    innportState[i] = digitalRead(innportPins[i]);
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
  for (byte i = 0; i < sizeof(innportPins); i++) {
    innportState[i] = digitalRead(innportPins[i]);

    //Hvis endret send OPC_INPUT_REP
    if (innportState[i] != innportStateLast[i]) {
      int adr = i + JMRI_ADR;
      #ifdef DEBUG
      Serial.println("--------------------------------");
      Serial.print("\nUlik: ");
      Serial.println(innportPins[i]);
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

  //Kopier det vi leste nå til det vi skal sammenligne med neste gang.
  memcpy(innportStateLast, innportState, sizeof(innportState));
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
  if (Output && (Address >= JMRI_ADR && Address < JMRI_ADR + sizeof(utportPins))) {
    #ifdef DEBUG
    Serial.println("---Set output---");
    #endif
    digitalWrite(utportPins[Address - JMRI_ADR], Direction);
  }

  //Sensor forespørsel fra JMRI, rapporter status på innganger.
  //Hvilken av forespørslene (1017-1020) vi egentlig skal svare på er avhengig av hvilken adresse vi har
  //men jeg orker ikke prøve å finne ut av hvordan dettte egentlig er ment å fungere nå, så vi svarer på 1018.
  if (!Output && !Direction && Address == 1018) {
    for (int i = 0; i < sizeof(innportPins); i++) {
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

