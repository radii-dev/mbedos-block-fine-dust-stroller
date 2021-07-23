/**
 ******************************************************************************
 * @file    lis3mdl_class.h
 * @author  AST / EST
 * @version V0.0.1
 * @date    14-April-2015
 * @brief   Header file for component LIS3MDL
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
 *       without specific prior written permission.
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

#ifndef __LIS3MDL_CLASS_H
#define __LIS3MDL_CLASS_H

/* Includes ------------------------------------------------------------------*/
#include "mbed.h"
#include "DevI2C.h"
#include "lis3mdl.h"
#include "MagneticSensor.h"
#include <assert.h>


/* Classes -------------------------------------------------------------------*/
/** Class representing a LIS3MDL sensor component
 */
class LIS3MDL : public MagneticSensor {
 public:
    enum SPI_type_t {SPI3W, SPI4W};    
    
    LIS3MDL(SPI *spi, PinName cs_pin, PinName int_pin=NC, SPI_type_t spi_type=SPI4W);
        
	/** Constructor
	 * @param[in] i2c device I2C to be used for communication
	 */    
	 
    LIS3MDL(DevI2C *i2c, uint8_t address=LIS3MDL_M_MEMS_ADDRESS_HIGH, PinName int_pin=NC);
	
	/** Destructor
	 */
        virtual ~LIS3MDL() {}
	
	/*** Interface Methods ***/
	virtual int init(void *init_struct) {
		return LIS3MDL_Init((MAGNETO_InitTypeDef*)init_struct);
	}

	virtual int read_id(uint8_t *m_id) {
		return LIS3MDL_Read_M_ID(m_id);
	}

	virtual int get_m_axes(int32_t *pData) {
		return LIS3MDL_M_GetAxes(pData);
	}

	virtual int get_m_axes_raw(int16_t *pData) {
		return LIS3MDL_M_GetAxesRaw(pData);
	}

 protected:
	/*** Methods ***/
	MAGNETO_StatusTypeDef LIS3MDL_Init(MAGNETO_InitTypeDef *LIS3MDL_Init);
	MAGNETO_StatusTypeDef LIS3MDL_Read_M_ID(uint8_t *m_id);
	MAGNETO_StatusTypeDef LIS3MDL_M_GetAxes(int32_t *pData);
	MAGNETO_StatusTypeDef LIS3MDL_M_GetAxesRaw(int16_t *pData);
	MAGNETO_StatusTypeDef LIS3MDL_Set_SpiInterface (void *handle, LIS3MDL_SPIMode_t spimode);

	/**
	 * @brief  Configures LIS3MDL interrupt lines for NUCLEO boards
	 */
	void LIS3MDL_IO_ITConfig(void)
	{
		/* To be implemented */
	}

	/**
	 * @brief  Configures LIS3MDL I2C interface
	 * @return MAGNETO_OK in case of success, an error code otherwise
	 */
	MAGNETO_StatusTypeDef LIS3MDL_IO_Init(void)
	{
		return MAGNETO_OK; /* done in constructor */
	}

	/**
	 * @brief      Utility function to read data from LIS3MDL
	 * @param[out] pBuffer pointer to the byte-array to read data in to
	 * @param[in]  RegisterAddr specifies internal address register to read from.
	 * @param[in]  NumByteToRead number of bytes to be read.
	 * @retval     MAGNETO_OK if ok, 
	 * @retval     MAGNETO_ERROR if an I2C error has occured
	 */
	MAGNETO_StatusTypeDef LIS3MDL_IO_Read(uint8_t* pBuffer, 
					      uint8_t RegisterAddr, uint16_t NumByteToRead)
	{
        if (_dev_spi) {
        /* Write Reg Address */
            _dev_spi->lock();
            _cs_pin = 0;           
            if (_spi_type == SPI4W) {            
                _dev_spi->write(RegisterAddr | 0x80);
                for (int i=0; i<NumByteToRead; i++) {
                    *(pBuffer+i) = _dev_spi->write(0x00);
                }
            } else if (_spi_type == SPI3W){
                /* Write RD Reg Address with RD bit*/
                uint8_t TxByte = RegisterAddr | 0x80;    
                _dev_spi->write((char *)&TxByte, 1, (char *)pBuffer, (int) NumByteToRead);
            }            
            _cs_pin = 1;
            _dev_spi->unlock(); 
            return MAGNETO_OK;
        }            		
        if (!_dev_i2c) return MAGNETO_ERROR;			
		int ret = _dev_i2c->i2c_read(pBuffer,
					   _address,
					   RegisterAddr,
					   NumByteToRead);
		if(ret != 0) {
			return MAGNETO_ERROR;
		}
		return MAGNETO_OK;
	}
	
	/**
	 * @brief      Utility function to write data to LIS3MDL
	 * @param[in]  pBuffer pointer to the byte-array data to send
	 * @param[in]  RegisterAddr specifies internal address register to read from.
	 * @param[in]  NumByteToWrite number of bytes to write.
	 * @retval     MAGNETO_OK if ok, 
	 * @retval     MAGNETO_ERROR if an I2C error has occured
	 */
	MAGNETO_StatusTypeDef LIS3MDL_IO_Write(uint8_t* pBuffer, 
					       uint8_t RegisterAddr, uint16_t NumByteToWrite)
	{
		
        if (_dev_spi) { 
            _dev_spi->lock();
            _cs_pin = 0;
            int data = _dev_spi->write(RegisterAddr);                    
            _dev_spi->write((char *)pBuffer, (int) NumByteToWrite, NULL, 0);                     
            _cs_pin = 1;                    
            _dev_spi->unlock();
            return MAGNETO_OK;                    
        }        
		
        if (!_dev_i2c) return MAGNETO_ERROR;	
		int ret = _dev_i2c->i2c_write(pBuffer, _address, RegisterAddr, NumByteToWrite);
		if(ret != 0) {
			return MAGNETO_ERROR;
		}
		return MAGNETO_OK;
	}
	
	/*** Instance Variables ***/
	/* IO Device */
	DevI2C *_dev_i2c;
	SPI    *_dev_spi;
    uint8_t _address;	
    DigitalOut  _cs_pin; 
    InterruptIn _int_pin;    
    SPI_type_t _spi_type;        
};

#endif // __LIS3MDL_CLASS_H
