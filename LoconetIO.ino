#include <LocoNet.h>

#define LNtxPin 7

//Startadresse.  JMRI Hardware Address. Verdi mellom 1 og 2048.  Unngå 1017 - 1020.
#define JMRI_ADR 1

//Setter navn på pinnenummre
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
  Serial.begin(57600);

  //Sett pinmode
  for (byte i = 0; i < sizeof(innportPins); i++) {
    pinMode(innportPins[i], INPUT_PULLUP);
  }
  for (byte i = 0; i < sizeof(utportPins); i++) {
    pinMode(utportPins[i], OUTPUT);
  }

  //Vi leser pinnene og oppdaterer innportStateLast før vi starter hovedprogrammet.
  for (byte i = 0; i < sizeof(innportPins); i++) {
    innportState[i] = digitalRead(innportPins[i]);
  }
  memcpy(innportStateLast, innportState, sizeof(innportState));

  // initialize the LocoNet interface
  LocoNet.init(LNtxPin);

  Serial.print(F("Setup end RAM:"));
  Serial.println(freeRam());
}


void loop() {
  if (lnMsg *LnPacket = LocoNet.receive()) {
    Serial.print(F("From lnet: "));
    Serial.print(F("OPCode: "));
    Serial.print(LnPacket->sz.command);
    Serial.print(F(" Data1: "));
    Serial.print(LnPacket->data[1]);
    Serial.print(F(" Data2: "));
    Serial.println(LnPacket->data[2]);

    //Vi vil få en callback til en av notify-funksjonene under hvis dette er en pakke av Switch eller Sensor type.
    LocoNet.processSwitchSensorMessage(LnPacket);
  }

  //Les pinner sjekk om de er endret fra sist gang vi leste
  for (byte i = 0; i < sizeof(innportPins); i++) {
    innportState[i] = digitalRead(innportPins[i]);

    //Hvis endret send OPC_INPUT_REP
    if (innportState[i] != innportStateLast[i]) {
      Serial.println("--------------------------------");
      Serial.print("\nUlik: ");
      Serial.println(innportPins[i]);
      int adr = i + JMRI_ADR;
      Serial.print("Adresse:");
      Serial.println(adr);

      LN_STATUS lnstat = LocoNet.reportSensor(adr, innportState[i]);
      Serial.print("ReportSensorLoconetStatus: ");
      Serial.println(lnstat);

      Serial.println("--------------------------------");
    }
  }

  //Kopier det vi leste nå til det vi skal sammenligne med neste gang.
  memcpy(innportStateLast, innportState, sizeof(innportState));
}  //loop() slutt


//Callbacks fra processSwitchSensorMessage
void notifySensor( uint16_t Address, uint8_t State ) {
  Serial.println("notifySensor");
  Serial.print("Adr: ");
  Serial.print(Address);
  Serial.print(" State: ");
  Serial.println(State);
}

void notifySwitchRequest( uint16_t Address, uint8_t Output, uint8_t Direction ) {
  Serial.println("notifySwitchRequest");
  Serial.print("Adr: ");
  Serial.print(Address);
  Serial.print(" Output: ");
  Serial.print(Output);
  Serial.print(" Direction: ");
  Serial.println(Direction);

  //Sett utgang hvis dette er en av våre aresser
  if (Output && (Address >= JMRI_ADR && Address < JMRI_ADR + sizeof(utportPins))) {
    Serial.println("---Set output---");
    digitalWrite(utportPins[Address - JMRI_ADR], Direction);
  }

  //Sensor forespørsel fra JMRI, rapporter status på innganger.
  //Hvilken av forespørslene (1017-1020) vi egentlig skal svare på er avhengig av hvilken adresse vi har
  //men jeg orker ikke prøve å finne ut av hvordan dettte egentlig er ment å fungrere nå, så vi svarer på 1018.
  if (!Output && !Direction && Address == 1018) {
    for (int i = 0; i < sizeof(innportPins); i++) {
      LN_STATUS lnstat = LocoNet.reportSensor(i + JMRI_ADR, innportState[i]);
      Serial.print("ReportSensorLoconetStatus: ");
      Serial.println(lnstat);
    }
  }
}

void notifySwitchReport( uint16_t Address, uint8_t Output, uint8_t Direction ) {
  Serial.println("notifySwitchReport");
  Serial.print("Adr: ");
  Serial.print(Address);
  Serial.print(" Outp: ");
  Serial.println(Direction);
}

void notifySwitchState( uint16_t Address, uint8_t Output, uint8_t Direction ) {
  Serial.println("notifySwtichState");
  Serial.print("Adr: ");
  Serial.print(Address);
  Serial.print(" Outp: ");
  Serial.println(Direction);
}

int freeRam () {
  extern int __heap_start, *__brkval;
  int v;
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval);
}

