// EEL-4746L LABFINAL
// Fernando Colon, Alan Bernal, Ryan Lanier
// Team 14 - Light Sensor, Pattern 2
// Fall 2024 - Section Friday

#include "LcdDrivermsp430/Crystalfontz128x128_ST7735.h"
#include "LcdDrivermsp430/HAL_MSP_EXP430FR5994_Crystalfontz128x128_ST7735.h"
#include "LcdDrivermsp430/HAL_FR5994_OPT3001.h"
#include "LcdDrivermsp430/HAL_FR5994_I2CLIB.h"
#include "grlib.h"
#include "driverlib.h"
#include <stdint.h>
#include <stdio.h>

#define OPTADDRESS            0x44
#define DELAYVALUE            1000
#define TIMER_DIV             TIMER_A_CLOCKSOURCE_DIVIDER_2
#define phoneLight            2000
#define blockedLight          10
#define LOW_SPEED_DIV         4
#define PATTERN_STEP_DELAY    6
#define HOLD_1SEC             100

typedef enum { standby, motorOff, CW, CCW, motorON } motorMode;
typedef enum {
    MODE_NORMAL = 0,
    MODE_DBG_FWD_HI,
    MODE_DBG_REV_LO,
    MODE_DBG_PATTERN
} systemMode_t;

Graphics_Context        g_sContext;
Timer_A_initUpModeParam MyTimerA;

char         buffer[100];
uint8_t      motorSeq      = 0;
uint16_t     lightLevel    = 0;
motorMode    motorState    = standby;
systemMode_t systemMode    = MODE_NORMAL;

static uint8_t  slowCnt    = 0;
static uint8_t  patDelay   = 0;
static uint8_t  patIndex   = 0;
static uint8_t  flashCount = 0;
static uint16_t standbyCnt = 0; 
static uint8_t currentpos = 0;
uint16_t holdCnt = 0;

static const uint8_t pattern2Seq[10] = {12, 4, 8, 10, 5, 8, 3, 9, 3, 6};
volatile uint8_t patStepCnt = 0;

static EUSCI_B_I2C_initMasterParam i2cConfig;

void LCD_init(void);
void configTimerA(uint16_t,uint16_t);
void configGPIO(void);
void myMotorDriver(void);
void myMotorController(void);
void FR5994_I2C_init(void);
static void enterPatternMode(void);


// Main Function Module
// Body: Setups the ports, display characters on the LCD screen and initiates timer
// Authors: Alan
void main (void)
{
    WDT_A_hold(WDT_A_BASE);
    PMM_unlockLPM5();

    LCD_init();
    configGPIO();

    motorSeq   = 0;
    motorState = standby;

    sprintf(buffer,"FINAL PROJECT EEL4746");
    Graphics_drawStringCentered(&g_sContext,(int8_t*)buffer,
                                AUTO_STRING_LENGTH,64,50,OPAQUE_TEXT);

    sprintf(buffer,"F.C.  A.B.  R.L.");
    Graphics_drawStringCentered(&g_sContext,(int8_t*)buffer,
                                AUTO_STRING_LENGTH,64,30,OPAQUE_TEXT);

    FR5994_I2C_init();
    OPT3001_init(OPTADDRESS);

    configTimerA(DELAYVALUE, TIMER_DIV);
    Timer_A_initUpMode(TIMER_A0_BASE, &MyTimerA);
    Timer_A_enableInterrupt(TIMER_A0_BASE);
    Timer_A_startCounter(TIMER_A0_BASE, TIMER_A_UP_MODE);

    __enable_interrupt();

    while(1)
        __low_power_mode_0();
}

// Main Function Module
// Body: Setups the ports, display characters on the LCD screen and initiates timer
// Authors: Alan
void configGPIO(void)
{
    // Stepper motor A B ~A ~B
    GPIO_setAsOutputPin(GPIO_PORT_P3, GPIO_PIN7);
    GPIO_setAsOutputPin(GPIO_PORT_P3, GPIO_PIN6);
    GPIO_setAsOutputPin(GPIO_PORT_P3, GPIO_PIN5);
    GPIO_setAsOutputPin(GPIO_PORT_P3, GPIO_PIN4);

    // LEDs
    GPIO_setAsOutputPin(GPIO_PORT_P1, GPIO_PIN0);
    GPIO_setAsOutputPin(GPIO_PORT_P1, GPIO_PIN1);

    // MKII pushbuttons
    GPIO_setAsInputPinWithPullUpResistor(GPIO_PORT_P4, GPIO_PIN3);
    GPIO_setAsInputPinWithPullUpResistor(GPIO_PORT_P4, GPIO_PIN2);

    // LaunchPad S1/S2 for DEBUG MODE (active low)
    GPIO_setAsInputPinWithPullUpResistor(GPIO_PORT_P5, GPIO_PIN6);
    GPIO_setAsInputPinWithPullUpResistor(GPIO_PORT_P5, GPIO_PIN5);

    //outputs
    GPIO_setOutputLowOnPin(GPIO_PORT_P3, GPIO_PIN7)
    GPIO_setOutputLowOnPin(GPIO_PORT_P3, GPIO_PIN6)
    GPIO_setOutputLowOnPin(GPIO_PORT_P3, GPIO_PIN5)
    GPIO_setOutputLowOnPin(GPIO_PORT_P3, GPIO_PIN4);
    GPIO_setOutputLowOnPin(GPIO_PORT_P1, GPIO_PIN0|GPIO_PIN1);
}

// FR5994_I2C_init
// Body: Set ups the light sensor
// Authors: Ryan
void FR5994_I2C_init(void)
{
    uint32_t smclk;

    // P7.1 = SCL, P7.0 = SDA (J12 on the LaunchPad)
    GPIO_setAsPeripheralModuleFunctionOutputPin(GPIO_PORT_P7,GPIO_PIN1,GPIO_PRIMARY_MODULE_FUNCTION); // SCL
    GPIO_setAsPeripheralModuleFunctionOutputPin(GPIO_PORT_P7,GPIO_PIN0,GPIO_PRIMARY_MODULE_FUNCTION); // SDA

    // Fill I2C master config structure
    smclk = CS_getSMCLK();

    i2cConfig.selectClockSource      = EUSCI_B_I2C_CLOCKSOURCE_SMCLK;
    i2cConfig.i2cClk                 = smclk;
    i2cConfig.dataRate               = EUSCI_B_I2C_SET_DATA_RATE_100KBPS;
    i2cConfig.byteCounterThreshold   = 0;
    i2cConfig.autoSTOPGeneration     = EUSCI_B_I2C_NO_AUTO_STOP;

    // Disable, configure, then enable I2C on EUSCI_B2
    EUSCI_B_I2C_disable(EUSCI_B2_BASE);
    EUSCI_B_I2C_initMaster(EUSCI_B2_BASE, &i2cConfig);
    EUSCI_B_I2C_enable(EUSCI_B2_BASE);
}

// Main LCD INIT
// Body: Initialize LCD Screen
// Authors: Alan
void LCD_init(void)
{
    Crystalfontz128x128_Init();
    Crystalfontz128x128_SetOrientation(0);
    Graphics_initContext(&g_sContext, &g_sCrystalfontz128x128);
    Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_RED);
    Graphics_setBackgroundColor(&g_sContext, GRAPHICS_COLOR_BLACK);
    GrContextFontSet(&g_sContext, &g_sFontFixed6x8);
    Graphics_clearDisplay(&g_sContext);
}

// Main ConfigTimerA
// Body: Setup the timer configurations
// Authors: Alan
void configTimerA(uint16_t delayValue, uint16_t clockDividerValue)
{
    MyTimerA.clockSource        = TIMER_A_CLOCKSOURCE_SMCLK;
    MyTimerA.clockSourceDivider = clockDividerValue;
    MyTimerA.timerPeriod        = delayValue;
    MyTimerA.timerClear         = TIMER_A_DO_CLEAR;
    MyTimerA.startTimer         = false;
}

// myMotorDriver
// Body: it has an input, according to the value of the parameter order the motor to move.
// Authors: Alan Bernal
void myMotorDriver(){

    switch(motorSeq){

    case 0:
        // A RED LP
        GPIO_setOutputHighOnPin(GPIO_PORT_P3,GPIO_PIN7);
        // B RED EB
        GPIO_setOutputLowOnPin(GPIO_PORT_P3,GPIO_PIN6);
        // ABAR BLUE EB
        GPIO_setOutputLowOnPin(GPIO_PORT_P3,GPIO_PIN5);
        // BBAR GREEN EB
        GPIO_setOutputLowOnPin(GPIO_PORT_P3,GPIO_PIN4);
        break;

    case 1:
        // A RED LP
        GPIO_setOutputHighOnPin(GPIO_PORT_P3,GPIO_PIN7);
        // B RED EB
        GPIO_setOutputHighOnPin(GPIO_PORT_P3,GPIO_PIN6);
        // ABAR BLUE EB
        GPIO_setOutputLowOnPin(GPIO_PORT_P3,GPIO_PIN5);
        // BBAR GREEN EB
        GPIO_setOutputLowOnPin(GPIO_PORT_P3,GPIO_PIN4);
        break;
    case 2:
        // A RED LP
        GPIO_setOutputLowOnPin(GPIO_PORT_P3,GPIO_PIN7);
        // B RED EB
        GPIO_setOutputHighOnPin(GPIO_PORT_P3,GPIO_PIN6);
        // ABAR BLUE EB
        GPIO_setOutputLowOnPin(GPIO_PORT_P3,GPIO_PIN5);
        // BBAR GREEN EB
        GPIO_setOutputLowOnPin(GPIO_PORT_P3,GPIO_PIN4);
        break;
    case 3:
        // A RED LP
        GPIO_setOutputLowOnPin(GPIO_PORT_P3,GPIO_PIN7);
        // B RED EB
        GPIO_setOutputHighOnPin(GPIO_PORT_P3,GPIO_PIN6);
        // ABAR BLUE EB
        GPIO_setOutputHighOnPin(GPIO_PORT_P3,GPIO_PIN5);
        // BBAR GREEN EB
        GPIO_setOutputLowOnPin(GPIO_PORT_P3,GPIO_PIN4);
        break;
    case 4:
        // A RED LP
        GPIO_setOutputLowOnPin(GPIO_PORT_P3,GPIO_PIN7);
        // B RED EB
        GPIO_setOutputLowOnPin(GPIO_PORT_P3,GPIO_PIN6);
        // ABAR BLUE EB
        GPIO_setOutputHighOnPin(GPIO_PORT_P3,GPIO_PIN5);
        // BBAR GREEN EB
        GPIO_setOutputLowOnPin(GPIO_PORT_P3,GPIO_PIN4);
        break;
    case 5:
        // A RED LP
        GPIO_setOutputLowOnPin(GPIO_PORT_P3,GPIO_PIN7);
        // B RED EB
        GPIO_setOutputLowOnPin(GPIO_PORT_P3,GPIO_PIN6);
        // ABAR BLUE EB
        GPIO_setOutputHighOnPin(GPIO_PORT_P3,GPIO_PIN5);
        // BBAR GREEN EB
        GPIO_setOutputHighOnPin(GPIO_PORT_P3,GPIO_PIN4);
        break;
    case 6:
        // A RED LP
        GPIO_setOutputLowOnPin(GPIO_PORT_P3,GPIO_PIN7);
        // B RED EB
        GPIO_setOutputLowOnPin(GPIO_PORT_P3,GPIO_PIN6);
        // ABAR BLUE EB
        GPIO_setOutputLowOnPin(GPIO_PORT_P3,GPIO_PIN5);
        // BBAR GREEN EB
        GPIO_setOutputHighOnPin(GPIO_PORT_P3,GPIO_PIN4);
        break;
    case 7:
        // A RED LP
        GPIO_setOutputHighOnPin(GPIO_PORT_P3,GPIO_PIN7);
        // B RED EB
        GPIO_setOutputLowOnPin(GPIO_PORT_P3,GPIO_PIN6);
        // ABAR BLUE EB
        GPIO_setOutputLowOnPin(GPIO_PORT_P3,GPIO_PIN5);
        // BBAR GREEN EB
        GPIO_setOutputHighOnPin(GPIO_PORT_P3,GPIO_PIN4);
        break;

    default:
        // A RED LP
        GPIO_setOutputLowOnPin(GPIO_PORT_P3,GPIO_PIN7);
        // B RED EB
        GPIO_setOutputLowOnPin(GPIO_PORT_P3,GPIO_PIN6);
        // ABAR BLUE EB
        GPIO_setOutputLowOnPin(GPIO_PORT_P3,GPIO_PIN5);
        // BBAR GREEN EB
        GPIO_setOutputLowOnPin(GPIO_PORT_P3,GPIO_PIN4);
        break;

    }
}

// myMotorController
// Body: controls the motor debug mode pushbottoms and CCW and Cw behaviors
// Authors: Alan  
void myMotorController(void)
{
    uint8_t PBS1 = GPIO_getInputPinValue(GPIO_PORT_P4, GPIO_PIN3);
    uint8_t PBS2 = GPIO_getInputPinValue(GPIO_PORT_P4, GPIO_PIN2);

    lightLevel = OPT3001_getLux(OPTADDRESS);

    // hard off
    if (PBS1 == GPIO_INPUT_PIN_LOW && PBS2 == GPIO_INPUT_PIN_LOW)
    {
        motorState = motorOff;
        GPIO_setOutputHighOnPin(GPIO_PORT_P1, GPIO_PIN0 | GPIO_PIN1);
        motorSeq = 0;
        standbyCnt = 0;
        GPIO_setOutputLowOnPin(GPIO_PORT_P3, GPIO_PIN7 | GPIO_PIN6 | GPIO_PIN5 | GPIO_PIN4);
        return;
    }

    // standby window
    if (lightLevel > blockedLight && lightLevel < phoneLight)
    {
        motorState = standby;

        if (++standbyCnt >= 500)          // ~1 Hz blink with 2 ms ISR
        {
            standbyCnt = 0;
            GPIO_toggleOutputOnPin(GPIO_PORT_P1, GPIO_PIN1);
        }
        GPIO_setOutputLowOnPin(GPIO_PORT_P1, GPIO_PIN0);

        GPIO_setOutputLowOnPin(GPIO_PORT_P3, GPIO_PIN7 | GPIO_PIN6 | GPIO_PIN5 | GPIO_PIN4);
        return;
    }

    // leaving standby → reset blink counter
    standbyCnt = 0;

    // bright → CW
    if (lightLevel >= phoneLight)
    {
        motorState = CW;
        motorSeq   = (motorSeq + 1) & 0x07;
        GPIO_setOutputLowOnPin(GPIO_PORT_P1, GPIO_PIN0);
        GPIO_setOutputHighOnPin(GPIO_PORT_P1, GPIO_PIN1);
    }
    // blocked → CCW (slow)
    else
    {
        motorState = CCW;

        if (++slowCnt >= LOW_SPEED_DIV)
        {
            slowCnt  = 0;
            motorSeq = (motorSeq == 0) ? 7u : (motorSeq - 1);
        }

        GPIO_setOutputHighOnPin(GPIO_PORT_P1, GPIO_PIN0);
        GPIO_setOutputLowOnPin(GPIO_PORT_P1, GPIO_PIN1);
    }

    myMotorDriver();
}

// EnterPatternMode
// Body: Change parameter and set the state into patternmode
// Authors: Fernando
static void enterPatternMode(void)
{
    systemMode = MODE_DBG_PATTERN;
    patIndex   = 0;
    patDelay   = 0;
    slowCnt    = 0;
    standbyCnt = 0;

    motorState = motorON;

    GPIO_setOutputLowOnPin(GPIO_PORT_P1, GPIO_PIN0)
    GPIO_setOutputLowOnPin(GPIO_PORT_P1, GPIO_PIN1);
    // A RED LP
    GPIO_setOutputLowOnPin(GPIO_PORT_P3,GPIO_PIN7);
    // B RED EB
    GPIO_setOutputLowOnPin(GPIO_PORT_P3,GPIO_PIN6);
    // ABAR BLUE EB
    GPIO_setOutputLowOnPin(GPIO_PORT_P3,GPIO_PIN5);
    // BBAR GREEN EB
    GPIO_setOutputLowOnPin(GPIO_PORT_P3,GPIO_PIN4);
}

// motorISR
// Body: Contains the states behaviour and works according to the timer interrupt
// Authors: Alan, Fernando, Ryan
#pragma vector = TIMER0_A1_VECTOR
__interrupt void motorISR(void)
{
    static uint8_t   lastS1    = GPIO_INPUT_PIN_HIGH;
    static uint8_t   lastS2    = GPIO_INPUT_PIN_HIGH;
    static motorMode prevState = standby;

    uint8_t s1 = GPIO_getInputPinValue(GPIO_PORT_P5, GPIO_PIN6);
    uint8_t s2 = GPIO_getInputPinValue(GPIO_PORT_P5, GPIO_PIN5);

    if (systemMode != MODE_DBG_PATTERN)
    {
        if (s1 == GPIO_INPUT_PIN_LOW && s2 == GPIO_INPUT_PIN_LOW &&
            !(lastS1 == GPIO_INPUT_PIN_LOW && lastS2 == GPIO_INPUT_PIN_LOW))
        {
            enterPatternMode();
        }
        else if (s1 == GPIO_INPUT_PIN_HIGH && s2 == GPIO_INPUT_PIN_HIGH)
        {
            systemMode = MODE_NORMAL;
        }
        else if (s1 == GPIO_INPUT_PIN_HIGH && s2 == GPIO_INPUT_PIN_LOW)
        {
            systemMode = MODE_DBG_FWD_HI;
        }
        else if (s1 == GPIO_INPUT_PIN_LOW && s2 == GPIO_INPUT_PIN_HIGH)
        {
            systemMode = MODE_DBG_REV_LO;
        }
    }

    switch(systemMode)
    {
    case MODE_NORMAL:
        myMotorController();

        // count CW "flashes"
        if (motorState == CW && prevState != CW)
        {
            if (++flashCount >= 3)   // tweak if it still feels off
            {
                flashCount = 0;
                enterPatternMode();
            }
        }
        prevState = motorState;
        break;

    case MODE_DBG_FWD_HI:
        motorState = CW;
        motorSeq   = (motorSeq + 1) & 0x07;
        myMotorDriver();
        break;

    case MODE_DBG_REV_LO:
        if (++slowCnt >= LOW_SPEED_DIV)
        {
            slowCnt    = 0;
            motorState = CCW;
            motorSeq   = (motorSeq == 0) ? 7 : (motorSeq - 1);
            myMotorDriver();
        }
        break;

    case MODE_DBG_PATTERN:
       
        uint8_t target = pattern2Seq[patIndex];


        if (currentpos != target)
        {
            holdCnt = 0;

            if (patStepCnt < 33)
            {
                // 1 motor micro-step CW
                motorSeq = (motorSeq + 1) & 0x07;   // 0..7 wrap
                myMotorDriver();
                patStepCnt++;
            }
            else
            {
                // after 15 micro-steps, advance logical position
                currentpos++;
                if (currentpos > 12)     // when currentpos = 13 -> 0 on next step
                {
                    currentpos = 1;
                }
                patStepCnt = 0;
            }
        }
        else
        {
            if (++holdCnt >= HOLD_1SEC)
                {
                    holdCnt = 0;
                    patIndex++;      // move to next pattern entry
                    patStepCnt = 0;
                }
        }


        if (patIndex >= 10)
        {
            patIndex    = 0;
            slowCnt     = 0;
            flashCount  = 0;
            standbyCnt  = 0;
            patStepCnt = 0;
            motorState  = standby;
            systemMode  = MODE_NORMAL;
            GPIO_setOutputLowOnPin(GPIO_PORT_P1, GPIO_PIN0)
            GPIO_setOutputLowOnPin(GPIO_PORT_P1, GPIO_PIN1);
            // A RED LP
            GPIO_setOutputLowOnPin(GPIO_PORT_P3,GPIO_PIN7);
            // B RED EB
            GPIO_setOutputLowOnPin(GPIO_PORT_P3,GPIO_PIN6);
            // ABAR BLUE EB
            GPIO_setOutputLowOnPin(GPIO_PORT_P3,GPIO_PIN5);
            // BBAR GREEN EB
            GPIO_setOutputLowOnPin(GPIO_PORT_P3,GPIO_PIN4);
        }
        break;
    }

    lastS1 = s1;
    lastS2 = s2;

    Timer_A_clearTimerInterrupt(TIMER_A0_BASE);
}