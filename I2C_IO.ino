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



void mcp_write_single_reg(byte adr,byte reg,byte data){
  Wire.beginTransmission(adr);
  Wire.write(reg);
  Wire.write(data);
  byte success = Wire.endTransmission();
  if(success != 0){
    Serial.print("I2c transmit error:");
    Serial.println(success);
  }
}

void mcp_write_port(byte adr,byte data){
  mcp_write_single_reg(adr,MCP_GPIO,data);
}

byte mcp_read_single_reg(byte adr,byte reg){
  Wire.beginTransmission(adr);
  Wire.write(reg);
  byte success1=Wire.endTransmission(false);
  byte success2=Wire.requestFrom((int)adr,1,true);
  if(Wire.available() >0){
    return Wire.read();
  }
  if(success1 !=0 || success2 !=0){
    Serial.print("I2c transmit error:");
    Serial.println(success1);
    Serial.print("I2c receive error:");
    Serial.println(success2);
    return -1;
  }
}

byte mcp_read_port(byte adr){
  return mcp_read_single_reg(adr,MCP_GPIO);
}

void setup_pcf(byte adr){
  Wire.beginTransmission(adr);
  Wire.write(0xff);  //Sett alle høy for å bruke som innganger
  byte success=Wire.endTransmission(); 
  if(success != 0){
    Serial.print("I2c PCF transmit error:");
    Serial.println(success);
  }
}

byte pcf_read_port(byte adr){
  byte success=Wire.requestFrom((int)adr,1,true);
  if(Wire.available() >0){
    return Wire.read();
  }
  if(success != 0){
    Serial.print("I2c PCF receive error:");
    Serial.println(success);
  }
}

