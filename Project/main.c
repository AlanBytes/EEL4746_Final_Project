// EEL-4746L LABFINAL
// Alan Bernal
// Fall 2024
// Section Friday
// 10/24/2025

// Include Files
#include "LcdDrivermsp430/Crystalfontz128x128_ST7735.h"
#include "LcdDrivermsp430/HAL_MSP_EXP430FR5994_Crystalfontz128x128_ST7735.h"
#include "grlib.h"
#include "driverlib.h"
#include <stdint.h>
#include <stdio.h>

// Defines
#define PWMTimerPeriod 50000
#define PWMClkDivider TIMER_B_CLOCKSOURCE_DIVIDER_20

// Global Variables
Graphics_Context g_sContext;
Timer_B_outputPWMParam MyTimerB;
Timer_A_initUpModeParam MyTimerA;
typedef enum{motorOff, CW, CCW, motorON} motorMode;
char buffer[100];
uint8_t motorSeq = 0;


//Function Headers
void LCD_init(void);
void configTimerA(uint16_t,uint16_t);
void myTimerADelay(uint16_t,uint16_t);
void configGPIO(void);
void myMotorDriver();
void myMotorController();

void main (void){
    //Stop WDT
    WDT_A_hold(WDT_A_BASE);

    // Activate Configuration
    PMM_unlockLPM5();

    // Initialize LCD
    LCD_init();
    configGPIO();

    motorSeq = 0;
    motorState = motorOff;

    sprintf(buffer,"FINAL PROJECT EEL4746");
    Graphics_drawStringCentered(&g_sContext,buffer,AUTO_STRING_LENGTH,64,50,OPAQUE_TEXT);

    sprintf(buffer,"A.B. , R. , F.");
    Graphics_drawStringCentered(&g_sContext,(int8_t*)buffer, AUTO_STRING_LENGTH,64,30,OPAQUE_TEXT);

    sprintf(buffer,"");
    Graphics_drawStringCentered(&g_sContext,buffer,AUTO_STRING_LENGTH,64,50,OPAQUE_TEXT);


    configTimerA(37500,TIMER_A_CLOCKSOURCE_DIVIDER_8);
    Timer_A_initUpMode(TIMER_A0_BASE,&MyTimerA);
    Timer_A_enableInterrupt(TIMER_A0_BASE);
    Timer_A_startCounter(TIMER_A0_BASE, TIMER_A_UP_MODE);
    __enable_interrupt();
    __low_power_mode_0();
    __no_operation();
}



// config_GPIO
// Configures mkII RGB LED and PB S1 and S2
// Inputs: none
// Returns: none

void configGPIO(){

    // Set output pins
    GPIO_setAsOutputPin(GPIO_PORT_P3,GPIO_PIN7); //A
    GPIO_setAsOutputPin(GPIO_PORT_P3,GPIO_PIN6); //B
    GPIO_setAsOutputPin(GPIO_PORT_P3,GPIO_PIN5); //ABar
    GPIO_setAsOutputPin(GPIO_PORT_P3,GPIO_PIN4); //BBar

    GPIO_setAsOutputPin(GPIO_PORT_P1,GPIO_PIN0); //LED0
    GPIO_setAsOutputPin(GPIO_PORT_P1,GPIO_PIN1); //LED1

    // Set input pins
    GPIO_setAsInputPinWithPullUpResistor(GPIO_PORT_P4,GPIO_PIN3); //PB1
    GPIO_setAsInputPinWithPullUpResistor(GPIO_PORT_P4,GPIO_PIN2); //PB2

    //Set Light Sensors Pins as input.

    //Turn Off all Pins
    GPIO_setOutputLowOnPin(GPIO_PORT_P3,GPIO_PIN7);
    GPIO_setOutputLowOnPin(GPIO_PORT_P3,GPIO_PIN6);
    GPIO_setOutputLowOnPin(GPIO_PORT_P3,GPIO_PIN5);
    GPIO_setOutputLowOnPin(GPIO_PORT_P3,GPIO_PIN4);

    GPIO_setOutputLowOnPin(GPIO_PORT_P1,GPIO_PIN0);
    GPIO_setOutputLowOnPin(GPIO_PORT_P1,GPIO_PIN1);

}



// LCD_Init
// Configures mkII LCD display
// Inputs: none
// Returns: none

void LCD_init(){

/* Initializes display */
Crystalfontz128x128_Init();

/* Set default screen orientation */
Crystalfontz128x128_SetOrientation(0);


/* Initializes graphics context */
Graphics_initContext(&g_sContext, &g_sCrystalfontz128x128);
Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_RED);
Graphics_setBackgroundColor(&g_sContext, GRAPHICS_COLOR_BLACK);
GrContextFontSet(&g_sContext, &g_sFontFixed6x8);
Graphics_clearDisplay(&g_sContext);
}



// configTimerA
// Configuration Parameters for TimerA
// Inputs: delayValue -- number of count cycles
//         clockDividerValue -- clock divider
// Returns: None

void configTimerA(uint16_t delayValue, uint16_t clockDividerValue)
{
    MyTimerA.clockSource = TIMER_A_CLOCKSOURCE_SMCLK;
    MyTimerA.clockSourceDivider = clockDividerValue;
    MyTimerA.timerPeriod = delayValue;
    MyTimerA.timerClear = TIMER_A_DO_CLEAR;
    MyTimerA.startTimer = false;
}

// myTimerADelay
// Hardware Timer Delay function using polling with Timer A
// Inputs: delayValue -- number of count cycles
//         clockDividerValue -- clock divider
// Returns: none

void myTimerADelay(uint16_t delayValue, uint16_t clockDividerValue)
{

   configTimerA(delayValue,clockDividerValue);  // Configure the timer parameters
   Timer_A_initUpMode(TIMER_A0_BASE,&MyTimerA); // Initialize the timer
   Timer_A_startCounter(TIMER_A0_BASE,TIMER_A_UP_MODE);  // Start Timer
   while((TA0CTL&TAIFG) == 0);                   // Wait for TAIFG to become Set
   Timer_A_stop(TIMER_A0_BASE);                  // Stop timer
   Timer_A_clearTimerInterrupt(TIMER_A0_BASE);   // Reset TAIFG to Zero
}



void config_mkII_interrupts(){

    //mkII PBS1
    GPIO_selectInterruptEdge(GPIO_PORT_P4, GPIO_PIN3, GPIO_HIGH_TO_LOW_TRANSITION);
    GPIO_enableInterrupt(GPIO_PORT_P4,GPIO_PIN3);
    GPIO_clearInterrupt(GPIO_PORT_P4,GPIO_PIN3);

    //mkII PBS2
    GPIO_selectInterruptEdge(GPIO_PORT_P4, GPIO_PIN2, GPIO_HIGH_TO_LOW_TRANSITION);
    GPIO_enableInterrupt(GPIO_PORT_P4,GPIO_PIN2);
    GPIO_clearInterrupt(GPIO_PORT_P4,GPIO_PIN2);
}



void LCD_turnOffBacklight(void){
    GPIO_setAsOutputPin(GPIO_PORT_P2, GPIO_PIN6);
    GPIO_setOutputLowOnPin(GPIO_PORT_P2, GPIO_PIN6);
}

void LCD_turnOnBacklight(void){
    GPIO_setAsOutputPin(GPIO_PORT_P2, GPIO_PIN6);
    GPIO_setOutputHighOnPin(GPIO_PORT_P2, GPIO_PIN6);
}


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

void myMotorController(){
    uint8_t PBS1 = GPIO_getInputPinValue(GPIO_PORT_P5, GPIO_PIN6);
    uint8_t PBS2 = GPIO_getInputPinValue(GPIO_PORT_P5, GPIO_PIN5);

    if(PBS1 == GPIO_INPUT_PIN_LOW && PBS2 == GPIO_INPUT_PIN_LOW){
        motorState = motorOff;
        GPIO_setOutputHighOnPin(GPIO_PORT_P1,GPIO_PIN1);
        GPIO_setOutputHighOnPin(GPIO_PORT_P1,GPIO_PIN1);
    } else if (PBS1 == GPIO_INPUT_PIN_LOW){
        motorState = CW;
        if(motorSeq == 7) motorSeq = 0;
        else motorSeq++;
        GPIO_setOutputHighOnPin(GPIO_PORT_P1,GPIO_PIN0);
        GPIO_setOutputLowOnPin(GPIO_PORT_P1,GPIO_PIN1);
    } else if (PBS2 == GPIO_INPUT_PIN_LOW){
        motorState = CCW;
        if(motorSeq == 0) motorSeq = 7;
        else motorSeq--;
        GPIO_setOutputHighOnPin(GPIO_PORT_P1,GPIO_PIN1);
        GPIO_setOutputLowOnPin(GPIO_PORT_P1,GPIO_PIN0);
    } else {
        motorState = motorOff;
        motorSeq = 0;
        GPIO_setOutputLowOnPin(GPIO_PORT_P1,GPIO_PIN1);
        GPIO_setOutputLowOnPin(GPIO_PORT_P1,GPIO_PIN0);
    }

    myMotorDriver();

}


#pragma vector = TIMER0_A1_VECTOR
__interrupt void motorISR(){
    myMotorController();
    Timer_A_clearTimerInterrupt(TIMER_A0_BASE);
}


