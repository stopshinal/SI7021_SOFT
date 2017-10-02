/*
Copyright 2014 Marcus Sorensen <marcus@electron14.com>

This program is licensed, please check with the copyright holder for terms

Updated: Jul 16, 2015: TomWS1:
        eliminated Byte constants,
        fixed 'sizeof' error in _command(),
        added getTempAndRH() function to simplify calls for C & RH only
*/
#include <Particle_SI7021.h>
#include <SlowSoftWire.h>

#define I2C_ADDR 0x40

// I2C commands
#define RH_READ             0xE5
#define TEMP_READ           0xE3
#define POST_RH_TEMP_READ   0xE0
#define RESET               0xFE
#define USER1_READ          0xE7
#define USER1_WRITE         0xE6

// compound commands
byte SERIAL1_READ[]      ={ 0xFA, 0x0F };
byte SERIAL2_READ[]      ={ 0xFC, 0xC9 };

bool _si_exists = false;

SlowSoftWire WireSoft = SlowSoftWire(D7, D6, true);

SI7021::SI7021() {
}


bool SI7021::begin() {
    WireSoft.begin();
    WireSoft.beginTransmission(I2C_ADDR);
    if (WireSoft.endTransmission() == 0) {
        _si_exists = true;
    }
    return _si_exists;
}

bool SI7021::sensorExists() {
    return _si_exists;
}

int SI7021::getFahrenheitHundredths() {
    int c = getCelsiusHundredths();
    return (1.8 * c) + 3200;
}

int SI7021::getCelsiusHundredths() {
    byte tempbytes[2];
    _command(TEMP_READ, tempbytes);
    long tempraw = (long)tempbytes[0] << 8 | tempbytes[1];
    return ((17572 * tempraw) >> 16) - 4685;
}

long SI7021::getTempraw() {
    byte tempbytes[2];
    _command(TEMP_READ, tempbytes);
    long tempraw = (long)tempbytes[0] << 8 | tempbytes[1];
    return tempraw;
}

int SI7021::_getCelsiusPostHumidity() {
    byte tempbytes[2];
    _command(POST_RH_TEMP_READ, tempbytes);
    long tempraw = (long)tempbytes[0] << 8 | tempbytes[1];
    return ((17572 * tempraw) >> 16) - 4685;
}


unsigned int SI7021::getHumidityPercent() {
    byte humbytes[2];
    _command(RH_READ, humbytes);
    long humraw = (long)humbytes[0] << 8 | humbytes[1];
    return ((125 * humraw) >> 16) - 6;
}

long SI7021::getHumidityraw() {
    byte humbytes[2];
    _command(RH_READ, humbytes);
    long humraw = (long)humbytes[0] << 8 | humbytes[1];
    return humraw;
}

unsigned int SI7021::getHumidityBasisPoints() {
    byte humbytes[2];
    _command(RH_READ, humbytes);
    long humraw = (long)humbytes[0] << 8 | humbytes[1];
    return ((12500 * humraw) >> 16) - 600;
}

void SI7021::_command(byte cmd, byte * buf ) {
    _writeReg(&cmd, sizeof cmd);
    _readReg(buf, 2);
}

void SI7021::_writeReg(byte * reg, int reglen) {
    WireSoft.beginTransmission(I2C_ADDR);
    for(int i = 0; i < reglen; i++) {
        reg += i;
        WireSoft.write(*reg);
    }
    WireSoft.endTransmission();
}

int SI7021::_readReg(byte * reg, int reglen) {
    WireSoft.requestFrom(I2C_ADDR, reglen);
    while(WireSoft.available() < reglen) {
    }
    for(int i = 0; i < reglen; i++) {
        reg[i] = WireSoft.read();
    }
    return 1;
}

//note this has crc bytes embedded, per datasheet, so provide 12 byte buf
int SI7021::getSerialBytes(byte * buf) {
    _writeReg(SERIAL1_READ, sizeof SERIAL1_READ);
    _readReg(buf, 6);

    _writeReg(SERIAL2_READ, sizeof SERIAL2_READ);
    _readReg(buf + 6, 6);

    // could verify crc here and return only the 8 bytes that matter
    return 1;
}

int SI7021::getDeviceId() {
    byte serial[12];
    getSerialBytes(serial);
    int id = serial[6];
    return id;
}

void SI7021::setHeater(bool on) {
    byte userbyte;
    if (on) {
        userbyte = 0x3E;
    } else {
        userbyte = 0x3A;
    }
    byte userwrite[] = {USER1_WRITE, userbyte};
    _writeReg(userwrite, sizeof userwrite);
}

// get humidity, then get temperature reading from humidity measurement
struct si7021_env SI7021::getHumidityAndTemperature() {
    si7021_env ret = {0, 0, 0};
    ret.humidityBasisPoints      = getHumidityBasisPoints();
    ret.celsiusHundredths        = _getCelsiusPostHumidity();
    ret.fahrenheitHundredths     = (1.8 * ret.celsiusHundredths) + 3200;
    return ret;
}

// get temperature (C only) and RH Percent
struct si7021_thc SI7021::getTempAndRH()
{
    si7021_thc ret;

    ret.humidityPercent   = getHumidityPercent();
    ret.celsiusHundredths = _getCelsiusPostHumidity();
    return ret;

}
