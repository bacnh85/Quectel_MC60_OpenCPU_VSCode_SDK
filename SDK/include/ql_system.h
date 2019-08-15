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
 *   ql_system.h 
 *
 * Project:
 * --------
 *   OpenCPU
 *
 * Description:
 * ------------
 *  System related APIs
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
 

#ifndef __QL_SYSTEM_H__
#define __QL_SYSTEM_H__
#include "ql_common.h"
#include "ql_type.h"
#include "ql_memory.h"


/**************************************************************
 * Message Structure Definition
 **************************************************************/
typedef struct {
    u32  message;
    u32  param1;
    u32  param2;
    u32  srcTaskId;
} ST_MSG;

/**************************************************************
 * User Message ID Definition
 **************************************************************/
#define     MSG_ID_USER_START       0x1000
#define     MSG_ID_RIL_READY        (MSG_ID_USER_START + 1)
#define     MSG_ID_URC_INDICATION   (MSG_ID_USER_START + 2)


typedef enum {
	EVENT_FLAG0 = 0x00000001,
	EVENT_FLAG1 = 0x00000002,
	EVENT_FLAG2 = 0x00000004,
	EVENT_FLAG3 = 0x00000008,
	EVENT_FLAG4 = 0x00000010,
	EVENT_FLAG5 = 0x00000020,
	EVENT_FLAG6 = 0x00000040,
	EVENT_FLAG7 = 0x00000080,
	EVENT_FLAG8 = 0x00000100,
	EVENT_FLAG9 = 0x00000200,
	EVENT_FLAG_END
}Enum_EventFlag;

/**************************************************************
 * Error Code Definition
 **************************************************************/
typedef enum {
    OS_SUCCESS,
    OS_ERROR,
    OS_Q_FULL,
    OS_Q_EMPTY,
    OS_SEM_NOT_AVAILABLE,
    OS_WOULD_BLOCK,
    OS_MESSAGE_TOO_BIG,
    OS_INVALID_ID,
    OS_NOT_INITIALIZED,
    OS_INVALID_LENGHT,
    OS_NULL_ADDRESS,
    OS_NOT_RECEIVE,
    OS_NOT_SEND,
    OS_MEMORY_NOT_VALID,
    OS_NOT_PRESENT,
    OS_MEMORY_NOT_RELEASE
} Enum_OS_ErrCode;

typedef enum {
    SYS_STATE_START = 0,
    SYS_STATE_ATOK  = 1,
    SYS_STATE_PHBOK = 2,
    SYS_STATE_SMSOK = 3
}Enum_SysInitState;

typedef enum {
    CFUN_STATE_0 = 0,
    CFUN_STATE_1 = 1,
    CFUN_STATE_4 = 4
}Enum_CfunState;

/*****************************************************************
* Function:     Ql_Sleep
*
* Description:
*               Suspends the execution of the current task
*               until the time-out interval elapses.
* Parameters:
*               msec:
*                   The time interval for which execution is to
*                   be suspended, in milliseconds.
* Return:
*               None
*****************************************************************/
void Ql_Sleep(u32 msec);

/*****************************************************************
* Function:     Ql_Reset
*
* Description:
*               This function resets the system.
*
* Parameters:
*               resetType:
*                   must be 0.
* Return:
*               None
*****************************************************************/
void Ql_Reset(u8 resetType);

/*****************************************************************
* Function:     Ql_GetUID
*
* Description:
*               Get the module UID.
*
* Parameters:
*               ptrUID:
*                  [out] Point to the buffer to store the uid.
*                           Need 20 bytes length of buffer.
*               len:
*                   [in] The length of buffer.
* Return:        
*               If the uid_buf is null, this function will return
*               QL_RET_ERR_INVALID_PARAMETER.
*               If this function read the uid successfully, the length
*               of UID will be returned.
*****************************************************************/
s32 Ql_GetUID(u8* ptrUID, u32 len);

/*****************************************************************
* Function:     Ql_GetMsSincePwrOn 
* 
* Description:
*               This function returns the number of milliseconds 
*               since the device booted.
*
* Parameters:
*               None
* Return:        
*               Number of milliseconds
*****************************************************************/
u64 Ql_GetMsSincePwrOn(void);

/*****************************************************************
* Function:     Ql_GetCoreVer
*
* Description:
*               Get the version ID of the core.
*
* Parameters:
*               ptrVer:
*                   [out] Point to a unsigned char buffer, which is
*                           the the version ID of the core.
*               len:
*                   [in] It must be equal or greater than the length of version ID.
*                         Otherwise error code will be returned.
* Return:
*               The length of version ID indicates success.
*               Negative indicates failure. please see Error Code Definition.
*****************************************************************/
s32  Ql_GetCoreVer(u8* ptrVer, u32 len);

/*****************************************************************
* Function:     Ql_GetSDKVer
*
* Description:
*               Get the version ID of the SDK.
*
* Parameters:
*               ptrVer:
*                   [out] Point  to a unsigned char buffer, which is
*                           the the version ID of the SDK.
*               len:
*                   [in] A number will be compare with the length of version ID.
*
* Return:
*               The smaller between len and the length of version ID.
*****************************************************************/
s32  Ql_GetSDKVer(u8* ptrVer, u32 len);

/*****************************************************************
* Function:     Ql_OS_GetMessage 
* 
* Description:
*               This function retrieves a message from the current task's 
*               message queue. When there is no message in the task's
*               message queue, the task is in the waiting state..
* Parameters:
*               msg: 
*                   A pointer to ST_MSG.
* Return:        
*               Always return QL_RET_OK.
*****************************************************************/
s32 Ql_OS_GetMessage(ST_MSG* msg);

/*****************************************************************
* Function:     Ql_OS_SendMessage 
* 
* Description:
*               Send message between tasks.

* Parameters:
*               destTaskId: 
*                   One value of Enum_TaskId.
*               msgId: 
*                   User message id, must bigger than 0xFF.
*               param1:
*               param2:
*                   Parameters to send to another task.
* Return:        
*               OS_SUCCESS
*               OS_INVALID_ID
*               OS_MEMORY_NOT_VALID
*               OS_Q_FULL
*****************************************************************/
s32 Ql_OS_SendMessage(s32 destTaskId, u32 msgId, u32 param1, u32 param2);

/*****************************************************************
* Function:     Ql_OS_CreateMutex 
* 
* Description:
*               Create a mutex with name.

* Parameters:
*               mutexName: Mutex Name
* Return:        
*               Mutex Id
*****************************************************************/
u32 Ql_OS_CreateMutex(char* mutexName);

/*****************************************************************
* Function:     Ql_OS_TakeMutex 
* 
* Description:
*               Obtain an instance of the specified MUTEX.

* Parameters:
*               mutexId: Mutex Id
* Return:        
*               None
*****************************************************************/
void Ql_OS_TakeMutex(u32 mutexId);

/*****************************************************************
* Function:     Ql_OS_GiveMutex 
* 
* Description:
*               Release the instance of the specified MUTEX.

* Parameters:
*               mutexId: Mutex Id
* Return:        
*               None
*****************************************************************/
void Ql_OS_GiveMutex(u32 mutexId);

/*****************************************************************
* Function:     Ql_OS_CreateSemaphore 
* 
* Description:
*               Creates a counting semaphore.
* Parameters:
*               semName: 
*                   Name of semaphore
*               maxCount: 
*                   Initial value of semaphore
* Return:        
*               Value of created semaphore
*****************************************************************/
u32 Ql_OS_CreateSemaphore(char* semName, u32 maxCount);

/*****************************************************************
* Function:     Ql_OS_TakeSemaphore 
* 
* Description:
*               Obtain an instance of the specified semaphore.

* Parameters:
*               semId: Name of semaphore
*               wait: [IN] wait mode, specify the behavior when the semaphore is
*                   not ready immediately, it can be one the the following values:
*                   TRUE  - wait until ownership can be satisfied.
*                   FALSE - don't wait for other task gives ownership to it.
* Return:        
*               OS_SUCCESS: the operation is done successfully
*               OS_SEM_NOT_AVAILABLE: the semaphore is unavailable immediately.
*****************************************************************/
u32 Ql_OS_TakeSemaphore(u32 semId, bool wait);

/*****************************************************************
* Function:     Ql_OS_GiveSemaphore 
* 
* Description:
*               Release the instance of the specified semaphore.

* Parameters:
*               semId: Name of semaphore
* Return:        
*               None
*****************************************************************/
void Ql_OS_GiveSemaphore(u32 semId);

/*****************************************************************
* Function:     Ql_OS_CreateEvent 
* 
* Description:
* 			This function creates an event-flag group with the specified name.
*			Each event-flag group contains 10 event flags. 
*
* Parameters:
*			evtName:
*				Event name.
*
* Return:        
*			An event Id that identify this event uniquely.
*****************************************************************/
u32 Ql_OS_CreateEvent(char* evtName);

/*****************************************************************
* Function:     Ql_OS_WaitEvent 
* 
* Description:
* 			This function waits until the specified type of event is in the signaled 
*                  state. Developers can specify different types of events for purposes.
*                  The event flags are defined in 'Enum_EventFlag'.
*
* Parameters:
*			evtId:
*				Event id that is returned by calling Ql_OS_CreateEvent().
*			evtFlag:
*				Event flag type. Please refer to Enum_EventFlag.
*
* Return:        
*			Zero indicates success, an nonzero means failure.
*****************************************************************/
s32 Ql_OS_WaitEvent(u32 evtId, u32 evtFlag);

/*****************************************************************
* Function:     Ql_OS_SetEvent 
* 
* Description:
*                  This function sets the specified event flag. Any task waiting on the
*                  event, whose event flag request is satisfied, is resumed.
*
* Parameters:
*			evtId:
*				Event id that is returned by calling Ql_OS_CreateEvent().
*			evtFlag:
*				Event flag type. Please refer to Enum_EventFlag.
*
* Return:        
*			Zero indicates success, an nonzero means failure.
*****************************************************************/
s32 Ql_OS_SetEvent(u32 evtId, u32 evtFlag);

/*****************************************************************
* Function:     Ql_SetLastErrorCode 
* 
* Description:
*               Set error code

* Parameters:
*               errCode: Error code
* Return:        
*              True indicates success or failure indicates failure. 
*****************************************************************/
s32 Ql_SetLastErrorCode(s32 errCode);

/*****************************************************************
* Function:     Ql_GetLastErrorCode 
* 
* Description:
*               Retrieves the calling task's last-error code value.

* Parameters:
*               None
* Return:        
*               The return value is the calling task's last-error code.
*****************************************************************/
s32 Ql_GetLastErrorCode(void);

/*****************************************************************
* Function:     Ql_OS_GetCurrentTaskPriority 
* 
* Description:
*               Get the priority of the current task

* Parameters:
*               None
* Return:        
*               Task priority, ranges from 200 to 255.
*****************************************************************/
u32 Ql_OS_GetCurrentTaskPriority(void);

/*****************************************************************
* Function:     Ql_OS_GetCurrenTaskLeftStackSize 
* 
* Description:
*               Get the left number of bytes in the current task stack
* Parameters:
*               None
* Return:        
*               Number of bytes, ranges from 1024 to 10*1024.
*****************************************************************/
u32 Ql_OS_GetCurrenTaskLeftStackSize(void);

/*****************************************************************
* Function:     Ql_OS_GetActiveTaskId 
* 
* Description:
*               This function returns the task ID of the current task.
* Parameters:
*               None.
* Return:        
*               The Id number of current task.
*****************************************************************/
s32 Ql_OS_GetActiveTaskId(void);


#ifdef TASK_ENTRYFUNC_DEF
#undef TASK_ENTRYFUNC_DEF
#endif
#ifdef TASK_DEFINITION
#undef TASK_DEFINITION
#endif
#define TASK_ID_DEF
#include "ql_common.h"
TASK_DEFINITION_BEGIN
#include "custom_task_cfg.h"
TASK_DEFINITION_END
#undef TASK_ID_DEF

#endif  // End-of __QL_SYSTEM_H__
