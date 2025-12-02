/* --COPYRIGHT--,BSD
 * Copyright (c) 2015, Texas Instruments Incorporated
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * --/COPYRIGHT--*/
//****************************************************************************
//
// HAL_I2CLIB.c - Hardware abstraction layer for interfacing OPT3001
//
//****************************************************************************

#include "HAL_FR5994_I2CLIB.h"
#include "driverlib.h"
#include <stdint.h>


/* I2C Master Configuration Parameter */
EUSCI_B_I2C_initMasterParam i2cConfig =
{
        EUSCI_B_I2C_CLOCKSOURCE_SMCLK,          // SMCLK Clock Source
        10000000,                               // SMCLK = 1.00MHzMHz
        EUSCI_B_I2C_SET_DATA_RATE_400KBPS,      // Desired I2C Clock of 100khz
};



void FR5994_I2C_init(){

    // Init I2C GPIO

        GPIO_setAsPeripheralModuleFunctionOutputPin(GPIO_PORT_P7, GPIO_PIN1,GPIO_PRIMARY_MODULE_FUNCTION);  // SCL
        GPIO_setAsPeripheralModuleFunctionOutputPin(GPIO_PORT_P7, GPIO_PIN0,GPIO_PRIMARY_MODULE_FUNCTION);  // SDA

    // Disable I2C module to make changes
        EUSCI_B_I2C_disable(EUSCI_B2_BASE);

    //Initialize USCI_B1 device
        EUSCI_B_I2C_initMaster(EUSCI_B2_BASE, &i2cConfig);

    // Enable I2C Module to start operations
        EUSCI_B_I2C_enable(EUSCI_B2_BASE);

}



/***************************************************************************//**
 * @brief  Reads data from the sensor
 * @param  writeByte Address of register to read from
 * @return Register contents
 ******************************************************************************/

int16_t I2C_read16(uint8_t writeByte)
{
    volatile int16_t val = 0,temp;
    volatile int16_t valScratch = 0;

    /* Set master to transmit mode PL */
    EUSCI_B_I2C_setMode(EUSCI_B2_BASE,
        EUSCI_B_I2C_TRANSMIT_MODE);

    /* Clear any existing interrupt flag PL */
    EUSCI_B_I2C_clearInterrupt(EUSCI_B2_BASE,
        EUSCI_B_I2C_TRANSMIT_INTERRUPT0+EUSCI_B_I2C_RECEIVE_INTERRUPT0);

    /* Wait until ready to write PL */
    while (EUSCI_B_I2C_isBusBusy(EUSCI_B2_BASE));

    /* Initiate start and send first character */
    EUSCI_B_I2C_masterSendMultiByteStart(EUSCI_B2_BASE, writeByte);

    /* Wait for TX to finish */
    while(!(EUSCI_B_I2C_getInterruptStatus(EUSCI_B2_BASE,
        EUSCI_B_I2C_TRANSMIT_INTERRUPT0)));

    /* Initiate stop only */
    EUSCI_B_I2C_masterSendMultiByteStop(EUSCI_B2_BASE);

    /* Wait for Stop to finish */
 //   while(!EUSCI_B_I2C_getInterruptStatus(EUSCI_B2_BASE,
 //       EUSCI_B_I2C_STOP_INTERRUPT));

    /*
     * Generate Start condition and set it to receive mode.
     * This sends out the slave address and continues to read
     * until you issue a STOP
     */
    EUSCI_B_I2C_masterReceiveStart(EUSCI_B2_BASE);

    /* Wait for RX buffer to fill */
    while(!(EUSCI_B_I2C_getInterruptStatus(EUSCI_B2_BASE,
        EUSCI_B_I2C_RECEIVE_INTERRUPT0)));

 //   EUSCI_B_I2C_clearInterrupt(EUSCI_B2_BASE,
 //       EUSCI_B_I2C_RECEIVE_INTERRUPT);

    /* Read from I2C RX register */
    val = EUSCI_B_I2C_masterReceiveMultiByteNext(EUSCI_B2_BASE);

    EUSCI_B_I2C_masterReceiveMultiByteStop(EUSCI_B2_BASE);

    while(!(EUSCI_B_I2C_getInterruptStatus(EUSCI_B2_BASE,
        EUSCI_B_I2C_RECEIVE_INTERRUPT0)));

    /* Receive second byte then send STOP condition */
    valScratch = EUSCI_B_I2C_masterReceiveMultiByteNext(EUSCI_B2_BASE);

    /* Shift val to top MSB */
    val = (val << 8);

    /* Read from I2C RX Register and write to LSB of val */
    val |= valScratch;

    /* Return temperature value */
    return (int16_t)val;
}


/***************************************************************************//**
 * @brief  Writes data to the sensor
 * @param  pointer  Address of register you want to modify
 * @param  writeByte Data to be written to the specified register
 * @return none
 ******************************************************************************/

void I2C_write16 (uint8_t pointer, uint16_t writeByte)
{
    /* Set master to transmit mode PL */
    EUSCI_B_I2C_setMode(EUSCI_B2_BASE,
        EUSCI_B_I2C_TRANSMIT_MODE);

    /* Clear any existing interrupt flag PL */
    EUSCI_B_I2C_clearInterrupt(EUSCI_B2_BASE,
        EUSCI_B_I2C_TRANSMIT_INTERRUPT0+EUSCI_B_I2C_RECEIVE_INTERRUPT0);

    /* Wait until ready to write PL */
    while (EUSCI_B_I2C_isBusBusy(EUSCI_B2_BASE));

    /* Initiate start and send first character */
    EUSCI_B_I2C_masterSendMultiByteStart(EUSCI_B2_BASE, pointer);

    /* Send the MSB to SENSOR */
    EUSCI_B_I2C_masterSendMultiByteNext(EUSCI_B2_BASE, (unsigned char)(writeByte>>8));

    EUSCI_B_I2C_masterSendMultiByteFinish(EUSCI_B2_BASE, (unsigned char)(writeByte&0xFF));

}


void I2C_setAddress(uint16_t Address)
{
    /* Specify slave address for I2C */
    EUSCI_B_I2C_setSlaveAddress(EUSCI_B2_BASE, Address);

    /* Enable and clear the interrupt flag */
    EUSCI_B_I2C_clearInterrupt(EUSCI_B2_BASE,EUSCI_B_I2C_TRANSMIT_INTERRUPT0 + EUSCI_B_I2C_RECEIVE_INTERRUPT0);
    return;
}

