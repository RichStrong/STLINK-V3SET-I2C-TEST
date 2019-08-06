/**
  ******************************************************************************
  * @file    main_example.cpp
  * @author  MCD Application Team
  * @brief   This module is an example, it can be integrated to a C++ console project
  *          to do a basic USB connection and  GPIO bridge test with the
  *          STLINK-V3SET probe, using the Bridge C++ open API.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2019 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under Ultimate Liberty license
  * SLA0044, the "License"; You may not use this file except in compliance with
  * the License. You may obtain a copy of the License at:
  *                             www.st.com/SLA0044
  *
  ******************************************************************************
  */
#if defined(_MSC_VER) &&  (_MSC_VER >= 1000)
#include "stdafx.h"  // first include for windows visual studio
#else
#include <cstdlib>
#include <stdio.h>
#endif
#include "bridge.h"
/*
 * 
 */
// main() Defines the entry point for the console application.
#ifdef WIN32 //Defined for applications for Win32 and Win64.
int _tmain(int argc, _TCHAR* argv[])

#else
using namespace std;

int main(int argc, char** argv)
#endif
{
	Brg_StatusT brgStat = BRG_NO_ERR;
    STLinkIf_StatusT ifStat = STLINKIF_NO_ERR;
	char path[MAX_PATH];
#ifdef WIN32 //Defined for applications for Win32 and Win64.
	char *pEndOfPrefix;
#endif
	uint32_t i, numDevices;
	int firstDevNotInUse=-1;
	STLink_DeviceInfo2T devInfo2;
	Brg* m_pBrg = NULL;
	STLinkInterface *m_pStlinkIf = NULL;
	char m_serialNumber[SERIAL_NUM_STR_MAX_LEN];

	// Note: cErrLog g_ErrLog; to be instanciated and initialized if used with USING_ERRORLOG

	// In case previously used, close the previous connection (not the case here)
	if( m_pBrg!=NULL )
	{
		m_pBrg->CloseBridge(COM_UNDEF_ALL);
		m_pBrg->CloseStlink();
		delete m_pBrg;
		m_pBrg = NULL;
	}
	if( m_pStlinkIf !=NULL ) // always delete STLinkInterface after Brg that is using it.
	{
		delete m_pStlinkIf;
		m_pStlinkIf = NULL;
	}

	// USB interface initialization and device detection done using STLinkInterface

	// Create USB BRIDGE interface
	m_pStlinkIf = new STLinkInterface(STLINK_BRIDGE);
#ifdef USING_ERRORLOG
	m_pStlinkIf->BindErrLog(&g_ErrLog);
#endif

#ifdef WIN32 //Defined for applications for Win32 and Win64.
	GetModuleFileNameA(NULL, path, MAX_PATH);
	// Remove process file name from the path
	pEndOfPrefix = strrchr(path,'\\');

	if (pEndOfPrefix != NULL)
	{
		*(pEndOfPrefix + 1) = '\0';
	}
#else
	strcpy(path, "");
#endif
	// Load STLinkUSBDriver library 
	// In this example STLinkUSBdriver (dll on windows) must be copied near test executable
	ifStat = m_pStlinkIf->LoadStlinkLibrary(path);
	if( ifStat!=STLINKIF_NO_ERR ) {
		printf("STLinkUSBDriver library (dll) issue \n");
	}

	if( ifStat==STLINKIF_NO_ERR ) {
		ifStat = m_pStlinkIf->EnumDevices(&numDevices, false);
		// or brgStat = Brg::ConvSTLinkIfToBrgStatus(m_pStlinkIf->EnumDevices(&numDevices, false));
	}

	// Choose the first STLink Bridge available
	if( (ifStat==STLINKIF_NO_ERR) || (ifStat==STLINKIF_PERMISSION_ERR) ) {
		printf("%d BRIDGE device found\n", (int)numDevices);

		for( i=0; i<numDevices; i++ ) {
			ifStat = m_pStlinkIf->GetDeviceInfo2(i, &devInfo2, sizeof(devInfo2));
			printf("Bridge %d PID: 0X%04hx SN:%s\n", (int)i, (unsigned short)devInfo2.ProductId, devInfo2.EnumUniqueId);

			if( (firstDevNotInUse==-1) && (devInfo2.DeviceUsed == false) ) {
				firstDevNotInUse = i;
				memcpy(m_serialNumber, &devInfo2.EnumUniqueId, SERIAL_NUM_STR_MAX_LEN);
				printf("SELECTED BRIDGE Stlink SN:%s\n", m_serialNumber);
			}
		}
	} else if( ifStat==STLINKIF_CONNECT_ERR ) {
		printf("No STLink BRIDGE device detected\n");
	} else {
		printf("enum error\n");
	}

	brgStat = Brg::ConvSTLinkIfToBrgStatus(ifStat);

	// USB Connection to a given device done with Brg
	if( brgStat==BRG_NO_ERR ) {
		m_pBrg = new Brg(*m_pStlinkIf);
#ifdef USING_ERRORLOG
		m_pBrg->BindErrLog(&g_ErrLog);
#endif
	}

	// The firmware may not be the very last one, but it may be OK like that (just inform)
	bool bOldFirmwareWarning=false;


	// Open the STLink connection
	if( brgStat==BRG_NO_ERR ) {
		m_pBrg->SetOpenModeExclusive(true);

		brgStat = m_pBrg->OpenStlink(firstDevNotInUse);

		if( brgStat==BRG_NOT_SUPPORTED ) {
			brgStat = Brg::ConvSTLinkIfToBrgStatus(m_pStlinkIf->GetDeviceInfo2(0, &devInfo2, sizeof(devInfo2)));
			printf("BRIDGE not supported PID: 0X%04hx SN:%s\n", (unsigned short)devInfo2.ProductId, devInfo2.EnumUniqueId);
		}

		if( brgStat==BRG_OLD_FIRMWARE_WARNING ) {
			// Status to restore at the end if all is OK
			bOldFirmwareWarning = true;
			brgStat = BRG_NO_ERR;
		}
	}
	// Test Voltage command
	if( brgStat==BRG_NO_ERR ) {
		float voltage = 0;
		// T_VCC pin must be connected to target voltage on debug connector
		brgStat = m_pBrg->GetTargetVoltage(&voltage);
		if( (brgStat != BRG_NO_ERR) || (voltage == 0) ) {
			printf("BRIDGE get voltage error (check if T_VCC pin is connected to target voltage on debug connector)\n");
		} else {
			printf("BRIDGE get voltage: %f V \n", (double)voltage);
		}
	}

	if( (brgStat == BRG_NO_ERR) && (bOldFirmwareWarning == true) ) {
		// brgStat = BRG_OLD_FIRMWARE_WARNING;
		printf("BRG_OLD_FIRMWARE_WARNING: v%d B%d \n",(int)m_pBrg->m_Version.Major_Ver, (int)m_pBrg->m_Version.Bridge_Ver);
	}

	// Test GET CLOCK command
	if( brgStat==BRG_NO_ERR ) {
		uint32_t StlHClkKHz, comInputClkKHz;
		// Get the current bridge input Clk for all com:
		brgStat = m_pBrg->GetClk(COM_SPI, (uint32_t*)&comInputClkKHz, (uint32_t*)&StlHClkKHz);
		printf( "SPI input CLK: %d KHz, ST-Link HCLK: %d KHz \n", (int)comInputClkKHz, (int)StlHClkKHz);
		if( brgStat==BRG_NO_ERR ) {
			brgStat = m_pBrg->GetClk(COM_I2C, (uint32_t*)&comInputClkKHz, (uint32_t*)&StlHClkKHz);
			printf( "I2C input CLK: %d KHz, ST-Link HCLK: %d KHz \n", (int)comInputClkKHz, (int)StlHClkKHz);
		}
		if( brgStat==BRG_NO_ERR ) {
			brgStat = m_pBrg->GetClk(COM_CAN, (uint32_t*)&comInputClkKHz, (uint32_t*)&StlHClkKHz);
			printf( "CAN input CLK: %d KHz, ST-Link HCLK: %d KHz \n", (int)comInputClkKHz, (int)StlHClkKHz);
		}
		if( brgStat==BRG_NO_ERR ) {
			brgStat = m_pBrg->GetClk(COM_GPIO, (uint32_t*)&comInputClkKHz, (uint32_t*)&StlHClkKHz);
			printf( "GPIO input CLK: %d KHz, ST-Link HCLK: %d KHz \n", (int)comInputClkKHz, (int)StlHClkKHz);
		}
		if( brgStat!=BRG_NO_ERR ) {
			printf( "Error in GetClk()\n" );
		}
	}

	// I2C Test
	if (brgStat == BRG_NO_ERR)
	{
		printf("I2C test start\n");
		Brg_I2cInitT i2cParam;
		int freqKHz = 400; //400KHz
		uint32_t timingReg; 
		int riseTimeNs, fallTimeNs, DNF;
		bool analogFilter;
		uint16_t sizeWithoutErr =0;
		uint8_t dataRx[3072], dataTx[3072]; //max size must be aligned with target buffer
		uint16_t i2cSlaveAddr = 0x64>>1;// convert to 7bit address

		// I2C_FAST freqKHz: 1-400KHz I2C_FAST_PLUS: 1-1000KHz
		riseTimeNs = 0; //0-300ns
		fallTimeNs = 0; //0-300ns
		DNF = 0; // digital filter OFF
		analogFilter = true;

		brgStat = m_pBrg->GetI2cTiming(I2C_FAST, freqKHz, DNF, riseTimeNs, fallTimeNs, analogFilter, &timingReg);
		// example I2C_STANDARD, I2C input CLK= 192MHz, rise/fall time (ns) = 0, analog filter on, dnf=0
		// I2C freq = 400KHz timingReg = 0x20602274
		if( brgStat == BRG_NO_ERR ) {
			i2cParam.TimingReg = timingReg;
			i2cParam.OwnAddr = 0;// 0  unused in I2C master mode
			i2cParam.AddrMode = I2C_ADDR_7BIT;
			i2cParam.AnFilterEn = I2C_FILTER_ENABLE;
			i2cParam.DigitalFilterEn = I2C_FILTER_DISABLE;
			i2cParam.Dnf = (uint8_t)DNF; //0
			brgStat = m_pBrg->InitI2C(&i2cParam);
		} else {
				printf("I2C timing error, timing reg: 0x%08x\n", timingReg);
		}

		// 1- send 2 bytes : 1 byte regist address,1 byte data
		dataTx[0] = 0xff;// register address
		dataTx[1] = 0x60;// data
		brgStat = m_pBrg->WriteI2C(dataTx, i2cSlaveAddr, 2, &sizeWithoutErr);
		// or brgStat = m_pBrg->WriteI2C((uint8_t*)&txSize, I2C_7B_ADDR(_i2cSlaveAddr), 4, &sizeWithoutErr);
		if( brgStat != BRG_NO_ERR ) {
			printf("BRG Write data size error (sent %d instead of 2)\n", (int)sizeWithoutErr);
		}else{
			printf("write reg 0xff sucess\n");
		}

		// 2-send to read regist address
		dataTx[0] = 0x00;// read regist address
		brgStat = m_pBrg->WriteI2C(dataTx, i2cSlaveAddr, 1, &sizeWithoutErr);
		if( brgStat != BRG_NO_ERR ) {
			printf("BRG Write data size error (sent %d instead of 4)\n", (int)sizeWithoutErr);
		}
		// 3- read 4bytes data
		if( brgStat == BRG_NO_ERR ) {
			sizeWithoutErr = 0;
			brgStat = m_pBrg->ReadI2C(dataRx, i2cSlaveAddr, 4, &sizeWithoutErr);
			if( brgStat != BRG_NO_ERR ) {
				printf("BRG Read back data size error (read %d instead of 4)\n", (int)sizeWithoutErr);
			} else {
				for (int i = 0; i < 4; i++)
				{
					printf("reg value is : 0x%02x\n", dataRx[i]);
				}
			}
		}

		printf("I2C test end\n");
	}	

	// Disconnect
	if( m_pBrg!=NULL ) {
		m_pBrg->CloseBridge(COM_UNDEF_ALL);
		m_pBrg->CloseStlink();
		delete m_pBrg;
		m_pBrg = NULL;
	}
	// unload STLinkUSBdriver library
	if( m_pStlinkIf!=NULL ) {
		// always delete STLinkInterface after Brg (because Brg uses STLinkInterface)
		delete m_pStlinkIf;
		m_pStlinkIf = NULL;
	}

	if( brgStat == BRG_NO_ERR )
	{
		printf("TEST SUCCESS \n");
	} else {
		printf("TEST FAIL (Bridge error: %d) \n", (int)brgStat);
	}

	return 0;
}

