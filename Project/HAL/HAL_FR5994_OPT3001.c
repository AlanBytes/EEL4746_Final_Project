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
// HAL_OPT3001.c - Hardware abstraction layer for interfacing OPT3001
//
//****************************************************************************

#include "HAL_FR5994_OPT3001.h"
#include "HAL_FR5994_I2CLIB.h"
#include "driverlib.h"
#include <stdint.h>

void OPT3001_init(uint8_t Opt3001Address)
{
    /* Specify address for OPT3001 */
    I2C_setAddress(Opt3001Address);

    /* Set Default configuration for OPT3001*/
    I2C_write16(CONFIG_REG, DEFAULT_CONFIG_100);
}

uint16_t OPT3001_readManufacturerId(uint8_t Opt3001Address)
{
    /* Specify address for OPT3001 */

    I2C_setAddress(Opt3001Address);
    return I2C_read16(MANUFACTUREID_REG);
}
uint16_t OPT3001_readDeviceId(uint8_t Opt3001Address)
{
    /* Specify address for OPT3001 */
    I2C_setAddress(Opt3001Address);

    return I2C_read16(DEVICEID_REG);
}
uint16_t OPT3001_readConfigReg(uint8_t Opt3001Address)
{
    /* Specify address for OPT3001 */
    I2C_setAddress(Opt3001Address);

    return I2C_read16(CONFIG_REG);
}
uint16_t OPT3001_readLowLimitReg(uint8_t Opt3001Address)
{
    /* Specify address for OPT3001 */
    I2C_setAddress(Opt3001Address);

    return I2C_read16(LOWLIMIT_REG);
}
uint16_t OPT3001_readHighLimitReg(uint8_t Opt3001Address)
{
    /* Specify address for OPT3001 */
    I2C_setAddress(Opt3001Address);

    return I2C_read16(HIGHLIMIT_REG);
}
uint32_t OPT3001_getLux(uint8_t Opt3001Address)
{
    /* Specify address for OPT3001 */

    I2C_setAddress(Opt3001Address);

    uint16_t exponent = 0;
    uint32_t result = 0;
    int16_t raw;
    raw = I2C_read16(RESULT_REG);
    /*Convert to LUX*/
    //extract result & exponent data from raw readings
    result = raw&0x0FFF;
    exponent = (raw>>12)&0x000F;
    //convert raw readings to LUX
    switch(exponent){
    case 0: //*0.015625
        result = result>>6;
        break;
    case 1: //*0.03125
        result = result>>5;
        break;
    case 2: //*0.0625
        result = result>>4;
        break;
    case 3: //*0.125
        result = result>>3;
        break;
    case 4: //*0.25
        result = result>>2;
        break;
    case 5: //*0.5
        result = result>>1;
        break;
    case 6:
        result = result;
        break;
    case 7: //*2
        result = result<<1;
        break;
    case 8: //*4
        result = result<<2;
        break;
    case 9: //*8
        result = result<<3;
        break;
    case 10: //*16
        result = result<<4;
        break;
    case 11: //*32
        result = result<<5;
        break;
    }
    return result;
}

