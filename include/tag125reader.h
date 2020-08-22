#ifndef _TAG125READER_H
#define _TAG125READER_H

#include <Arduino.h>

//Pinos
#define OUTPUT_PORT 9
#define INPUT_PORT 7 //PD7

class Tag125Reader
{
private:
    /* Métodos */
    void setPwmGen();
    void setInputInt();

    /* Métodos Inline */
    inline void disablePwmOutput()
    {
        TCCR1A &= 0xBF;
        digitalWrite(OUTPUT_PORT, HIGH);
    }

    inline void enablePwmOutput()
    {
        TCCR1A |= 0x40;
    }

    inline uint8_t readInputPort()
    {
        return ((PIND & 0x80) >> 7);
    }

public:
    void setup();
    bool decode(uint32_t *decodedData);
};

//Instância alocada no cpp
extern Tag125Reader tag125;

#endif