#include "tag125reader.h"

/******************************************************************************/
/* Constants                                                                  */
/******************************************************************************/
//Vetor Circular
#define VECT_SIZE 50

//Estados
#define FIND_STOP 0
#define FIND_HEADER 1
#define RECV_DATA 2
#define DATA_RECEIVED 3

//Tempos do código
#define INV_TIME 350

/******************************************************************************/
/* Variável do objeto                                                         */
/******************************************************************************/
Tag125Reader tag125;

//Reset no PWM
//static unsigned long time_reset;
//static bool isPwmDisable;

//Vetor circular de tempo das interrupções.
static uint16_t vect[VECT_SIZE];
static volatile unsigned long int_timer;
static volatile uint8_t inPos;
static volatile uint8_t outPos;

//Máquina de estados.
static uint8_t state;
static uint8_t ninesCnt;
static uint64_t data;
static uint8_t dtCnt;
static uint8_t pulsesCnt;
static uint8_t parity;

/*******************************************************************************/
/*  Fpwm = Fclk / N x (1 + TOP)                                                */
/*  Fclp = 16000000                                                            */
/*  N = 8                                                                     */
/* TOP = 7                                                                     */
/*******************************************************************************/
void Tag125Reader::setPwmGen()
{
    //WGM13     WGM12(CTC1)     WGM11(PWM11)    WGM10(PWM10)    Timer/Counter Mode of Operation     TOP     Update of OCR1x at TOV1     Flag Set on
    //1         1               1               0               Fast PWM                            ICR1    BOTTOM                      TOP
    //TCCR1A = COM1A1   COM1A0  COM1B1  COM1B0  –   –   WGM11   WGM10
    TCCR1A = 0x43;

    //CS12      CS11    CS10    Description
    //0         1       1       clkI/O/64 (from prescaler)
    //0         1       0       clkI/O/8 (from prescaler)
    //TCCR1B = ICNC1    ICES1   –   WGM13   WGM12   CS12    CS11    CS10
    TCCR1B = 0x1A;

    OCR1A = 7;
}

/************************************************************/
/* Setup Porta PD6 como entrada                             */
/* Interrupção PCINT22                                      */
/************************************************************/
void Tag125Reader::setInputInt()
{
    //PCICR = –   –   –      –   PCIE2   PCIE1   PCIE0
    //PCIE2 = PCINT23...16
    PCICR |= (0x01 << PCIE2);
    //PCMSK2 = PCINT23  PCINT22    PCINT21     PCINT20     PCINT19     PCINT18     PCINT17     PCINT16
    PCMSK2 = (0x01 << PCINT23);
}

/************************************************************/
/* Configura timer e interrupção                            */
/************************************************************/
void Tag125Reader::setup()
{
    pinMode(OUTPUT_PORT, OUTPUT);
    digitalWrite(OUTPUT_PORT, HIGH);
    pinMode(INPUT_PORT, INPUT_PULLUP);
    setPwmGen();
    setInputInt();

    //Vetor circular na interrupção
    //time_reset = millis();
    //isPwmDisable = false;
    int_timer = micros();
    inPos = 0;
    outPos = 0;
    memset(vect, 0, sizeof(uint16_t) * VECT_SIZE);

    //Estados
    state = FIND_STOP;
    ninesCnt = 0;

    //Armazenamento dos dados
    dtCnt = 0;
    data = 0;

    //amostragem
    pulsesCnt = 0;

    //Paridade
    parity = 0;
}

/************************************************************/
/* Imprimi informações na tela                              */
/************************************************************/
bool Tag125Reader::decode(uint32_t *decodedData)
{
    uint8_t cpIndex = 0;
    uint8_t excludeIndex = 4;
    uint8_t rotatedBit = 0;

    if (outPos != inPos)
    {
        uint16_t val = vect[outPos++ % VECT_SIZE];
        switch (state)
        {
        case FIND_STOP:
            if (val > INV_TIME)
            {
                state = FIND_HEADER;
                ninesCnt = 0;
            }
            break;
        case FIND_HEADER:
            if (val > INV_TIME)
            {
                state = FIND_STOP;
                return false;
            }
            if (ninesCnt++ == 15)
            {
                state = RECV_DATA;
                dtCnt = 0;
                data = 0;
                pulsesCnt = 0;
                parity = 0;
            }
            break;
        case RECV_DATA:
            if (val < INV_TIME && pulsesCnt % 2 == 1)
            {
                if (dtCnt == 0)
                {
                    data |= ((uint64_t)1);
                    ++parity;
                }
                else
                {
                    uint8_t before = (uint8_t)((data & (((uint64_t)1) << (dtCnt - 1))) >> (dtCnt - 1));
                    if (before == 1)
                    {
                        data |= (((uint64_t)1) << dtCnt);
                        ++parity;
                    }
                }
                ++dtCnt;
                ++pulsesCnt;
            }
            else if (val > INV_TIME)
            {
                if (dtCnt > 0)
                {
                    uint8_t before = (uint8_t)((data & (((uint64_t)1) << (dtCnt - 1))) >> (dtCnt - 1));
                    if (before == 0)
                    {
                        data |= (((uint64_t)1) << dtCnt);
                        ++parity;
                    }
                }
                pulsesCnt = 0;
                ++dtCnt;
            }
            else
                ++pulsesCnt;

            //Validação Paridade (par, % 2 == 0)
            if (dtCnt != 0 && dtCnt % 5 == 0)
            {
                if (dtCnt != 55)
                {
                    uint8_t parityVal = (uint8_t)((data & (((uint64_t)1) << (dtCnt - 1))) >> (dtCnt - 1));
                    //Já foi somado anteriormente
                    if (parityVal == 1)
                        --parity;

                    if (parity % 2 != parityVal)
                    {
                        state = FIND_STOP;
                        return false;
                    }
                    parity = 0;
                }
                else
                {
                    //Verificar Paridade das colunas.
                    for (uint8_t c = 0; c < 4; ++c)
                    {
                        uint8_t column_parity = 0;
                        for (uint8_t l = 0; l < 10; ++l)
                        {
                            uint8_t index = c + l * 5;
                            uint8_t bitVal = (uint8_t)((data & (((uint64_t)1) << index)) >> index);
                            column_parity += bitVal;
                        }
                        uint8_t val_parity_column = (uint8_t)((data & (((uint64_t)1) << (50 + c))) >> (50 + c));
                        if (column_parity % 2 != val_parity_column)
                        {
                            state = FIND_STOP;
                            return false;
                        }
                    }

                    state = DATA_RECEIVED;
                }
            }

            break;
        case DATA_RECEIVED:
            //Remover os valores da paridade
            for (uint8_t i = 0; i < 50; ++i)
            {
                if (i == excludeIndex)
                {
                    excludeIndex += 5;
                    continue;
                }
                uint8_t bit = (uint8_t)((data & (((uint64_t)1) << i)) >> i);
                if (bit == 0)
                    data &= ~(((uint64_t)1) << cpIndex++);
                else
                    data |= ((uint64_t)1) << cpIndex++;
            }

            Serial.println("Bits Dados (sem paridade):");
            for(int i=0;i<40;++i)
            {
                uint8_t bit = (uint8_t)((data & (((uint64_t)1) << i)) >> i);
                Serial.print(bit);
            }
            Serial.println("");
         
            data = data >> 8;
            *decodedData = 0;
            for(uint8_t i=32; i >0; --i)
            {
                rotatedBit = ((uint64_t)1) & data;
                if(rotatedBit > 0)
                {
                    *decodedData |= (((uint32_t)1) << (i-1));
                }
                data = data >> 1;
            }

            state = FIND_STOP;
            return true;
        }
    }
    return false;
}

/************************************************************/
/*Tratamento da interrupção                                 */
/************************************************************/
ISR(PCINT2_vect)
{
    unsigned long m = micros();
    vect[inPos++ % VECT_SIZE] = m - int_timer;
    int_timer = m;
}