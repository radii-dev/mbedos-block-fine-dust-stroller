/**
 ******************************************************************************
 * @file    lis3mdl_class.cpp
 * @author  AST / EST
 * @version V0.0.1
 * @date    14-April-2015
 * @brief   Implementation file for the LIS3MDL driver class
 ******************************************************************************
 * @attention
 *
 * <h2><center>&copy; COPYRIGHT(c) 2015 STMicroelectronics</center></h2>
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *   1. Redistributions of source code must retain the above copyright notice,
 *      this list of conditions and the following disclaimer.
 *   2. Redistributions in binary form must reproduce the above copyright notice,
 *      this list of conditions and the following disclaimer in the documentation
 *      and/or other materials provided with the distribution.
 *   3. Neither the name of STMicroelectronics nor the names of its contributors
 *      may be used to endorse or promote products derived from this software
 *      without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 ******************************************************************************
*/

/* Includes ------------------------------------------------------------------*/
#include "lis3mdl_class.h"
#include "lis3mdl.h"

/* Methods -------------------------------------------------------------------*/
/* betzw - based on:
           X-CUBE-MEMS1/trunk/Drivers/BSP/Components/lis3mdl/lis3mdl.c: revision #400,
           X-CUBE-MEMS1/trunk: revision #416
*/

LIS3MDL::LIS3MDL(SPI *spi, PinName cs_pin, PinName int_pin, SPI_type_t spi_type) : 
                _dev_spi(spi), _cs_pin(cs_pin), _int_pin(int_pin), _spi_type(spi_type)
{
    assert (spi);
    if (cs_pin == NC) 
    {
        printf ("ERROR LIS3MDL CS MUST NOT BE NC\n\r");       
        _dev_spi = NULL;
        _dev_i2c=NULL;
        return;
    }       

    _cs_pin = 0;    
    _dev_i2c=NULL;    
    
    if (_spi_type == SPI3W) LIS3MDL_Set_SpiInterface ((void *)this, LIS3MDL_SPI_3_WIRE);
    else if (_spi_type == SPI4W) LIS3MDL_Set_SpiInterface ((void *)this, LIS3MDL_SPI_4_WIRE);
}  

LIS3MDL::LIS3MDL(DevI2C *i2c, uint8_t address, PinName int_pin) :
       _dev_i2c(i2c), _address(address), _cs_pin(NC), _int_pin(int_pin)  
{
    assert (i2c);
    _dev_spi = NULL;
}  


MAGNETO_StatusTypeDef LIS3MDL::LIS3MDL_Set_SpiInterface (void *handle, LIS3MDL_SPIMode_t spimode)
{
    uint8_t tmp=0x03;  //deft LIS3MDL_CTRL_REG3 value  

    tmp |= (uint8_t)spimode;
    if (LIS3MDL_IO_Write(&tmp, LIS3MDL_M_CTRL_REG3_M, 1) != MAGNETO_OK) return MAGNETO_ERROR;      
    return MAGNETO_OK;
}

/**
 * @brief  Set LIS3MDL Initialization
 * @param  LIS3MDL_Init the configuration setting for the LIS3MDL
 * @retval MAGNETO_OK in case of success, an error code otherwise
 */
MAGNETO_StatusTypeDef LIS3MDL::LIS3MDL_Init(MAGNETO_InitTypeDef *LIS3MDL_Init)
{
  uint8_t tmp1 = 0x00;
  MAGNETO_InitTypeDef *initStructure = LIS3MDL_Init;
  MAGNETO_InitTypeDef tempInit;  
  
  if (initStructure == NULL) {// default params   
    tempInit.M_FullScale = LIS3MDL_M_FS_4;
    tempInit.M_OperatingMode = LIS3MDL_M_MD_CONTINUOUS;
    tempInit.M_XYOperativeMode = LIS3MDL_M_OM_HP;
    tempInit.M_OutputDataRate = LIS3MDL_M_DO_80;
    initStructure = &tempInit;
  }
  
  
  /* Configure the low level interface ---------------------------------------*/
  if(LIS3MDL_IO_Init() != MAGNETO_OK)
  {
    return MAGNETO_ERROR;
  }
  
  /****** Magnetic sensor *******/
  
  if(LIS3MDL_IO_Read(&tmp1, LIS3MDL_M_CTRL_REG3_M, 1) != MAGNETO_OK)
  {
    return MAGNETO_ERROR;
  }
  
  /* Conversion mode selection */
  tmp1 &= ~(LIS3MDL_M_MD_MASK);
  tmp1 |= initStructure->M_OperatingMode;
  
  if(LIS3MDL_IO_Write(&tmp1, LIS3MDL_M_CTRL_REG3_M, 1) != MAGNETO_OK)
  {
    return MAGNETO_ERROR;
  }
  
  if(LIS3MDL_IO_Read(&tmp1, LIS3MDL_M_CTRL_REG1_M, 1) != MAGNETO_OK)
  {
    return MAGNETO_ERROR;
  }
  
  /* Output data rate selection */
  tmp1 &= ~(LIS3MDL_M_DO_MASK);
  tmp1 |= initStructure->M_OutputDataRate;
  
  /* X and Y axes Operative mode selection */
  tmp1 &= ~(LIS3MDL_M_OM_MASK);
  tmp1 |= initStructure->M_XYOperativeMode;
  
  if(LIS3MDL_IO_Write(&tmp1, LIS3MDL_M_CTRL_REG1_M, 1) != MAGNETO_OK)
  {
    return MAGNETO_ERROR;
  }
  
  if(LIS3MDL_IO_Read(&tmp1, LIS3MDL_M_CTRL_REG2_M, 1) != MAGNETO_OK)
  {
    return MAGNETO_ERROR;
  }
  
  /* Full scale selection */
  tmp1 &= ~(LIS3MDL_M_FS_MASK);
  tmp1 |= initStructure->M_FullScale;
  
  if(LIS3MDL_IO_Write(&tmp1, LIS3MDL_M_CTRL_REG2_M, 1) != MAGNETO_OK)
  {
    return MAGNETO_ERROR;
  }
  
  /* Configure interrupt lines */
  LIS3MDL_IO_ITConfig();
  
  return MAGNETO_OK;
  
  /******************************/
}


/**
 * @brief  Read ID of LIS3MDL Magnetic sensor
 * @param  m_id the pointer where the ID of the device is stored
 * @retval MAGNETO_OK in case of success, an error code otherwise
 */
MAGNETO_StatusTypeDef LIS3MDL::LIS3MDL_Read_M_ID(uint8_t *m_id)
{
  if(!m_id)
  {
    return MAGNETO_ERROR;
  }
  
  return LIS3MDL_IO_Read(m_id, LIS3MDL_M_WHO_AM_I_ADDR, 1);
}


/**
 * @brief  Read raw data from LIS3MDL Magnetic sensor output register
 * @param  pData the pointer where the magnetometer raw data are stored
 * @retval MAGNETO_OK in case of success, an error code otherwise
 */
MAGNETO_StatusTypeDef LIS3MDL::LIS3MDL_M_GetAxesRaw(int16_t *pData)
{
  uint8_t tempReg[2] = {0, 0};
  
  if(LIS3MDL_IO_Read(&tempReg[0], (LIS3MDL_M_OUT_X_L_M | LIS3MDL_I2C_MULTIPLEBYTE_CMD),
                     2) != MAGNETO_OK)
  {
    return MAGNETO_ERROR;
  }
  
  pData[0] = ((((int16_t)tempReg[1]) << 8) + (int16_t)tempReg[0]);
  
  if(LIS3MDL_IO_Read(&tempReg[0], (LIS3MDL_M_OUT_Y_L_M | LIS3MDL_I2C_MULTIPLEBYTE_CMD),
                     2) != MAGNETO_OK)
  {
    return MAGNETO_ERROR;
  }
  
  pData[1] = ((((int16_t)tempReg[1]) << 8) + (int16_t)tempReg[0]);
  
  if(LIS3MDL_IO_Read(&tempReg[0], (LIS3MDL_M_OUT_Z_L_M | LIS3MDL_I2C_MULTIPLEBYTE_CMD),
                     2) != MAGNETO_OK)
  {
    return MAGNETO_ERROR;
  }
  
  pData[2] = ((((int16_t)tempReg[1]) << 8) + (int16_t)tempReg[0]);
  
  return MAGNETO_OK;
}


/**
 * @brief Read data from LIS3MDL Magnetic sensor and calculate Magnetic in mgauss
 * @param pData the pointer where the magnetometer data are stored
 * @retval MAGNETO_OK in case of success, an error code otherwise
 */
MAGNETO_StatusTypeDef LIS3MDL::LIS3MDL_M_GetAxes(int32_t *pData)
{
  uint8_t tempReg = 0x00;
  int16_t pDataRaw[3];
  float sensitivity = 0;
  
  if(LIS3MDL_M_GetAxesRaw(pDataRaw) != MAGNETO_OK)
  {
    return MAGNETO_ERROR;
  }
  
  if(LIS3MDL_IO_Read(&tempReg, LIS3MDL_M_CTRL_REG2_M, 1) != MAGNETO_OK)
  {
    return MAGNETO_ERROR;
  }
  
  tempReg &= LIS3MDL_M_FS_MASK;
  
  switch(tempReg)
  {
    case LIS3MDL_M_FS_4:
      sensitivity = 0.14;
      break;
    case LIS3MDL_M_FS_8:
      sensitivity = 0.29;
      break;
    case LIS3MDL_M_FS_12:
      sensitivity = 0.43;
      break;
    case LIS3MDL_M_FS_16:
      sensitivity = 0.58;
      break;
  }
  
  pData[0] = (int32_t)(pDataRaw[0] * sensitivity);
  pData[1] = (int32_t)(pDataRaw[1] * sensitivity);
  pData[2] = (int32_t)(pDataRaw[2] * sensitivity);
  
  return MAGNETO_OK;
}

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
