/*****************************************************************************
*  Copyright Statement:
*  --------------------
*  This software is protected by Copyright and the information contained
*  herein is confidential. The software may not be copied and the information
*  contained herein may not be used or disclosed except with the written
*  permission of Quectel Co., Ltd. 2013
*
*****************************************************************************/
/*****************************************************************************
 *
 * Filename:
 * ---------
 *   ql_clock.h 
 *
 * Project:
 * --------
 *   OpenCPU
 *
 * Description:
 * ------------
 *   The module defines the information, and APIs related to the clock function.
 *
 * Author:
 * -------
 * -------
 *
 *============================================================================
 *             HISTORY
 *----------------------------------------------------------------------------
 * 
 ****************************************************************************/


#ifndef __QL_CLOCK_H__
#define __QL_CLOCK_H__


typedef enum{
    CLOCKSOURCE_26M= 0,
    CLOCKSOURCE_13M,
    CLOCKSOURCE_6DOT5M,
    CLOCKSOURCE_32K,
    END_OF_CLOCKSOURCE
}Enum_ClockSource;

/*****************************************************************
* Function:     Ql_CLK_Init 
* 
* Description:
*               This function Initialization a clock pin. 
*
*               NOTES:
*                   The clock can't out immediately after Ql_CLK_Init Initialization
*                   you must invoke Ql_CLK_Output functinon to control clock on or off
* Parameters:
*               pinName:
*                   Pin name, one value of Enum_PinName.
*               Enum_ClockSource:
*                   source clock , one value of Enum_ClockSource.
* Return:        
*               QL_RET_OK, this function succeeds.
*               QL_RET_ERR_NOSUPPORTPIN, the input pin is invalid. 
*               QL_RET_ERR_PINALREADYSUBCRIBE, the pin is in use in
*               other place. For example this pin has been using as EINT.
*               QL_RET_ERR_NOGPIOMODE, the input pin no clock mode
*               QL_RET_ERR_NOSUPPORTSET not support this function
*****************************************************************/
s32 Ql_CLK_Init(Enum_PinName clkName,Enum_ClockSource clkSrc);

/*****************************************************************
* Function:     Ql_CLK_Uninit 
* 
* Description:
*               This function release a clock pin.
*
* Parameters:
*               pinName:
*                   Pin name, one value of Enum_PinName.    
* Return:        
*               QL_RET_OK, this function succeeds.
*               QL_RET_ERR_NOSUPPORTPIN, the input pin is invalid. 
*               QL_RET_ERR_BUSSUBBUSY, the PIN not used as clock, 
*               Maby is used by IIC or SPI,this function can't release it 
*****************************************************************/
s32 Ql_CLK_Uninit(Enum_PinName clkName);

/*****************************************************************
* Function:     Ql_CLK_Output 
* 
* Description:
*               This function control clock on or off
*
* Parameters:
*               pinName:
*                   Pin name, one value of Enum_PinName.
*
* Return:        
*               QL_RET_OK, this function succeeds.
*               QL_RET_ERR_NOSUPPORTPIN, the input pin is invalid. 
*               QL_RET_ERR_NORIGHTOPERATE, the PIN not in clock mode or not init, 
*               QL_RET_ERR_NOSUPPORTCONTROL not support control   
*****************************************************************/
s32 Ql_CLK_Output(Enum_PinName pinName,bool clkOnOff);

#endif
