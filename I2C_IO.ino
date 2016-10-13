#include <Wire.h>

#define INTERN_PIN    1
#define MCP23008_PIN  2
#define PCF8475_PIN   3

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


//Skriv til valgfritt register i MCP23008
void mcp_write_single_reg(byte adr,byte reg,byte data){
  Wire.beginTransmission(adr);
  Wire.write(reg);
  Wire.write(data);
  byte success = Wire.endTransmission();
  if(success != 0){
    Serial.print(F("I2c transmit error:"));
    Serial.println(success);
  }
}

//Skriv til utgangslatchen på MCP23008
void mcp_write_port(byte adr,byte data){
  mcp_write_single_reg(adr,MCP_GPIO,data);
}

//Les valgfritt register i MCP23008
byte mcp_read_single_reg(byte adr,byte reg){
  Wire.beginTransmission(adr);
  Wire.write(reg);
  byte success1=Wire.endTransmission(false);
  byte success2=Wire.requestFrom((int)adr,1,true);
  if(Wire.available() >0){
    return Wire.read();
  }
  if(success1 !=0 || success2 !=0){
    Serial.println(F("mcp_read_single_reg"));
    Serial.print(F("I2c transmit error:"));
    Serial.println(success1);
    Serial.print(F("I2c receive error:"));
    Serial.println(success2);
    return -1;
  }
}

//Les pinnestatus på MCP23008
byte mcp_read_port(byte adr){
  return mcp_read_single_reg(adr,MCP_GPIO);
}

//setter retning og pullups på IO pinner på MCP23008
void setup_mcp(byte adr,byte dir,byte pullup){
  //Sett retning
  mcp_write_single_reg(adr,MCP_IODIR,dir);
  //Slå på pullups på innganger
  mcp_write_single_reg(adr,MCP_GPPU,pullup);
}

void setup_pcf(byte adr){
  Wire.beginTransmission(adr);
  Wire.write(0xff);  //Sett alle høy for å bruke som innganger
  byte success=Wire.endTransmission(); 
  if(success != 0){
    Serial.print(F("I2c PCF transmit error:"));
    Serial.println(success);
  }
}

byte pcf_read_port(byte adr){
  byte success=Wire.requestFrom((int)adr,1,true);
  if(Wire.available() >0){
    return Wire.read();
  }
  if(success != 0){
    Serial.print(F("I2c PCF receive error:"));
    Serial.println(success);
  }
}


//Erstatning for pinMode()
void pinDir(pin_t pin,byte mode){
  if(pin.pintype == INTERN_PIN){
    pinMode(pin.pinno,mode);
  } else if(pin.pintype == MCP23008_PIN){
    for(byte i=0;i<ARRAYELEMENTCOUNT(ioExpanderStatus);i++){
      if(ioExpanderStatus[i].adr == pin.adr && (mode==INPUT || mode==INPUT_PULLUP)){
        ioExpanderStatus[i].dir = ioExpanderStatus[i].dir | 1<<pin.pinno;
        if(mode==INPUT_PULLUP){
          ioExpanderStatus[i].pullup=ioExpanderStatus[i].pullup | 1<<pin.pinno;
        }
      }
    }
  }
}

//Erstatning for digitalRead()
boolean pinGet(pin_t pin){
  if(pin.pintype == INTERN_PIN){
    return digitalRead(pin.pinno);
  } else if(pin.pintype == MCP23008_PIN){
    for(byte i=0;i<ARRAYELEMENTCOUNT(ioExpanderStatus);i++){
      if(ioExpanderStatus[i].adr == pin.adr){
        return ioExpanderStatus[i].data & 1<<pin.pinno;
      }
    }    
  }
}

//Erstatning for digitalWrite()
void pinSet(pin_t pin,byte state){
  if(pin.pintype == INTERN_PIN){
    return digitalWrite(pin.pinno,state);
  } else if(pin.pintype == MCP23008_PIN){
    for(byte i=0;i<ARRAYELEMENTCOUNT(ioExpanderStatus);i++){
      if(ioExpanderStatus[i].adr == pin.adr){
        if(state){
          ioExpanderStatus[i].data = ioExpanderStatus[i].data | 1<<pin.pinno;
        }else{
          ioExpanderStatus[i].data = ioExpanderStatus[i].data & ~(1<<pin.pinno);        
        }
        ioExpanderStatus[i].changed=true;  //Tag for at data må overføres til MCP krets.
      }
    }    
  }
}

//Skriv data til IO expander krets.
void writeOutports(){
   for(byte i=0;i<ARRAYELEMENTCOUNT(ioExpanderStatus);i++){
      if(ioExpanderStatus[i].changed){  //Har blitt endret.
        mcp_write_port(ioExpanderStatus[i].adr,ioExpanderStatus[i].data);
        ioExpanderStatus[i].changed=false;
      }
   }
}

