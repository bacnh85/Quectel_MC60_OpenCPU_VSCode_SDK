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
 *   ql_fs.h 
 *
 * Project:
 * --------
 *   OpenCPU
 *
 * Description:
 * ------------
 *   File  API defines.
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
 

#ifndef __QL_FS_H__
#define __QL_FS_H__
#include "ql_type.h"


/****************************************************************************
 * Type of file access permitted
 ***************************************************************************/
#define QL_FS_READ_WRITE            0x00000000L
#define QL_FS_READ_ONLY             0x00000100L
#define QL_FS_CREATE                0x00010000L
#define QL_FS_CREATE_ALWAYS         0x00020000L
#define QL_FS_OPEN_SHARED           0x00000200L


/****************************************************************************
 * Type of file move permitted
 ***************************************************************************/
#define QL_FS_MOVE_COPY          0x00000001     // Move file|folder by copy
#define QL_FS_MOVE_KILL          0x00000002     // Delete the moved file|folder after moving
#define QL_FS_MOVE_OVERWRITE     0x00010000      // Overwrite the existed file in destination path when move file|folder


/****************************************************************************
 * 
 ***************************************************************************/
#define QL_FS_FILE_TYPE              0x00000004     
#define QL_FS_DIR_TYPE               0x00000008     
#define QL_FS_RECURSIVE_TYPE         0x00000010

/****************************************************************************
 * Constants for File Seek
 ***************************************************************************/
typedef enum
{
   QL_FS_FILE_BEGIN,    // Beginning of file
   QL_FS_FILE_CURRENT,  // Current position of file pointer
   QL_FS_FILE_END       // End of file
}Enum_FsSeekPos;

typedef enum 
{
    Ql_FS_UFS = 1,
    Ql_FS_SD  = 2,
    Ql_FS_RAM = 3,
    QL_FS_FAT = 0xFF
}Enum_FSStorage;



/******************************************************************************
* Function:     Ql_FS_Open
*  
* Description:
*               Opens or creates a file with a specified name in the UFS or SD card. 
*               If you want to create a file in the UFS , you only need to use a relative path.
*               If you want to create a file in the SD card , you also need to add prefix "SD:"
*               in front of the file name.
*
* Parameters:    
*               lpFileName:
*                   [in]The name of the file. The name is limited to 252 characters.
*                       You must use a relative path, such as "filename.ext" or 
*                       "dirname\filename.ext". 

*
*               flag:
*                   [in]A u32 that defines the file's opening and access mode.
*                       The possible values are shown as follow:
*                       QL_FS_READ_WRITE, can read and write
*                       QL_FS_READ_ONLY, can only read
*                       QL_FS_CREATE, opens the file, if it exists. 
*                           If the file does not exist, the function creates the file
*                       QL_FS_CREATE_ALWAYS, creates a new file. 
*                           If the file exists, the function overwrites the file 
*                           and clears the existing attributes
* Return:  
*               If the function succeeds, the return value specifies a file handle.
*               If the function fails, the return value is an error codes. 
*               QL_RET_ERR_FILE_NO_CARD indicates no sd card.
*               QL_RET_ERR_PARAM indicates parameter error. 
*               QL_RET_ERR_FILENAMETOOLENGTH indicates filename too length.
*               QL_RET_ERR_FILEOPENFAILED indicates open file failed.
*
*               QL_RET_ERR_FS_FATAL_ERR1 indicates some fatal error happens to 
*                  the file system. Develops may call Ql_GetLastErrorCode() to 
*                  retrieve the inner error code. And develops can call Ql_Fs_Format(QL_FS_FAT) 
*                  to format FAT to retore the file system.
******************************************************************************/
s32 Ql_FS_Open(char* lpFileName, u32 flag);

/******************************************************************************
* Function:     Ql_FS_OpenRAMFile
*  
* Description:
*               Opens or creates a file with a specified name in the RAM,you need
*               to add prefix "RAM:" in front of the file. You can create at most 15 files.
*
* Parameters:    
*               lpFileName:
*                   [in]The name of the file. The name is limited to 252 characters.
*                       You must use a relative path, such as "RAM:filename.ext". 
*                       

*
*               flag:
*                   [in]A u32 that defines the file's opening and access mode.
*                       The possible values are shown as follow:
*                       QL_FS_READ_WRITE, can read and write
*                       QL_FS_READ_ONLY, can only read
*                       QL_FS_CREATE, opens the file, if it exists. 
*                           If the file does not exist, the function creates the file
*                       QL_FS_CREATE_ALWAYS, creates a new file. 
*                           If the file exists, the function overwrites the file 
*                           and clears the existing attributes
*               ramFileSize: 
*                   [in]The size of the specified file which you want to create.
* Return:  
*               If the function succeeds, the return value specifies a file handle.
*               If the function fails, the return value is an error codes. 
*               QL_RET_ERR_PARAM indicates parameter error. 
*               QL_RET_ERR_FILENAMETOOLENGTH indicates filename too length.
*               QL_RET_ERR_FILEOPENFAILED indicates open file failed.
******************************************************************************/
s32 Ql_FS_OpenRAMFile(char *lpFileName, u32 flag, u32 ramFileSize);

/******************************************************************************
* Function:     Ql_FS_Read
*  
* Description:
*               Reads data from the specified file, starting at the position 
*               indicated by the file pointer. After the read operation has been 
*               completed, the file pointer is adjusted by the number of bytes actually read.
*
* Parameters:    
*               fileHandle:
*                   [in] A handle to the file to be read, which is the return value
*                        of the function Ql_FS_Open.
*
*               readBuffer:
*                   [out] Point to the buffer that receives the data read from the file.
*
*               numberOfBytesToRead:
*                   [in] Number of bytes to be read from the file.
*
*               numberOfBytesRead:
*                   [out] The number of bytes has been read. sets this value to zero before
*                         doing taking action or checking errors.
* Return:
*               QL_RET_OK, suceess
*               QL_RET_ERR_FILEREADFAILED, read file failed.
******************************************************************************/
s32 Ql_FS_Read(s32 fileHandle, u8 *readBuffer, u32 numberOfBytesToRead, u32 *numberOfBytesRead);


/******************************************************************************
* Function:     Ql_FS_Write
*  
* Description:
*               This function writes data to a file. Ql_FS_Write starts writing 
*               data to the file at the position indicated by the file pointer.
*               After the write operation has been completed, the file pointer 
*               is adjusted by the number of bytes actually written. 
*
* Parameters:    
*               fileHandle:
*                   [in] A handle to the file to be read, which is the return value 
*                        of the function Ql_FS_Open.
*
*               writeBuffer:
*                   [in] Point to the buffer containing the data to be written to the file.
*
*               numberOfBytesToWrite:
*                   [in] Number of bytes to be write to the file.
*
*               numberOfBytesWritten:
*                   [out]  The number of bytes has been written. Sets this value to zero 
*                        before doing taking action or checking errors.
* Return:  
*               QL_RET_OK, suceess
*               QL_RET_ERR_FILEDISKFULL, file disk is full.
*               QL_RET_ERR_FILEWRITEFAILED, write file failed.
*
*               QL_RET_ERR_FS_FATAL_ERR1 indicates some fatal error happens to 
*                  the file system. Develops may call Ql_GetLastErrorCode() to 
*                  retrieve the inner error code. And develops can call Ql_Fs_Format(QL_FS_FAT) 
*                  to format FAT to retore the file system.
******************************************************************************/
s32 Ql_FS_Write(s32 fileHandle, u8 *writeBuffer, u32 numberOfBytesToWrite, u32 *numberOfBytesWritten);


/******************************************************************************
* Function:     Ql_FS_Seek
*  
* Description:
*               Repositions the pointer in the previously opened file. 
*
* Parameters:    
*               fileHandle:
*                   [in] A handle to the file to be read, which is the return value 
*                        of the function Ql_FS_Open.
*
*               offset:
*                   [in] Number of bytes to move the file pointer.
*
*               whence:
*                   [in] The file pointer reference position. See Enum_FsSeekPos.
* Return:  
*               QL_RET_OK, suceess
*               QL_RET_ERR_FILESEEKFAILED, file seek failed
******************************************************************************/
s32 Ql_FS_Seek(s32 fileHandle, s32 offset, u32 whence);


/******************************************************************************
* Function:     Ql_FS_GetFilePosition
*  
* Description:
*               Gets the current value of the file pointer.
*
* Parameters:    
*               fileHandle:
*                   [in] A file handle, which was returned by calling 'Ql_FS_Open'.
*
* Return:  
*               The return value is the current offset from the beginning of the file
*               if this function succeeds. Otherwise, the return value is an error code. 
*               QL_RET_ERR_FILEFAILED, fail to operate file.
******************************************************************************/
s32 Ql_FS_GetFilePosition(s32 fileHandle);


/******************************************************************************
* Function:     Ql_FS_Truncate
*  
* Description:
*               This function truncates a file to ZERO size.
*
* Parameters:    
*               fileHandle: 
*                   [in] A file handle, which was returned by calling 'Ql_FS_Open'.
* Return:  
*               QL_RET_OK, success.
*               QL_RET_ERR_FILEFAILED, fail to operate file.
******************************************************************************/
s32 Ql_FS_Truncate(s32 fileHandle);


/******************************************************************************
* Function:     Ql_FS_Flush
*  
* Description:
*               Forces any data remaining in the file buffer to be written to the file.
*
* Parameters:    
*               fileHandle: 
*                   [in] A file handle, which was returned by calling 'Ql_FS_Open'.
* Return:  
*               None
******************************************************************************/
void Ql_FS_Flush(s32 fileHandle);


/******************************************************************************
* Function:     Ql_FS_Close
*  
* Description:
*               Closes the file associated with the file handle and makes 
*               the file unavailable for reading or writing.
*
* Parameters:    
*               fileHandle: 
*                   [in] A file handle, which was returned by calling 'Ql_FS_Open'.
* Return:  
*               None
******************************************************************************/
void Ql_FS_Close(s32 fileHandle);


/******************************************************************************
* Function:     Ql_FS_GetSize
*  
* Description:
*               Retrieves the size, in bytes, of the specified file.
*
* Parameters:    
*               lpFileName:
*                   [in] The name of the file. 
*
* Return:  
*               The return value is the bytes of the file if this function succeeds. 
*               Otherwise, the return value is an error code. 
*               QL_RET_ERR_FILE_NO_CARD, no sd card.
*               QL_RET_ERR_PARAM, parameter error. 
*               QL_RET_ERR_FILENAMETOOLENGTH , filename too length.
*               QL_RET_ERR_FILEFAILED, fail to operate file.
******************************************************************************/
s32 Ql_FS_GetSize(char *lpFileName);


/******************************************************************************
* Function:     Ql_FS_Delete
*  
* Description:
*               This function deletes an existing file.
*
* Parameters:    
*               lpFileName:
*                   [in]The name of the file to be deleted. The name is limited 
*                       to 252 characters. You must use a relative path, such as 
*                       "filename.ext" or "dirname\filename.ext".
* Return:  
*               QL_RET_OK, success.
*               QL_RET_ERR_FILE_NO_CARD, no sd card.
*               QL_RET_ERR_PARAM, parameter error. 
*               QL_RET_ERR_FILENAMETOOLENGTH , filename too length.
*               QL_RET_ERR_FILEFAILED, fail to operate file.
******************************************************************************/
s32 Ql_FS_Delete(char *lpFileName);


/******************************************************************************
* Function:     Ql_FS_Check
*  
* Description:
*               Check whether the file exists or not.
*
* Parameters:    
*               lpFileName:
*                   [in] The name of the file. The name is limited to 252 characters.
*                        You must use a relative path, such as "filename.ext" or 
*                        "dirname\filename.ext".
* Return:  
*               QL_RET_OK, success.
*               QL_RET_ERR_FILE_NO_CARD, no sd card.
*               QL_RET_ERR_PARAM, parameter error. 
*               QL_RET_ERR_FILENAMETOOLENGTH , filename too length.
*               QL_RET_ERR_FILEFAILED, fail to operate file.
*               QL_RET_ERR_FILENOTFOUND, file not found.
******************************************************************************/
s32 Ql_FS_Check(char *lpFileName);


/******************************************************************************
* Function:     Ql_FS_Rename
*  
* Description:
*               Rename a file.
*
* Parameters:    
*               lpFileName:
*                  [in] File to be renamed.The name is limited to 252 characters. 
*                       You must use a relative path, such as "filename.ext" or 
*                       "dirname\filename.ext".
*
*               newLpFileName:
*                  [in] New name of file. The name is limited to 252 characters. 
*                       You must use a relative path, such as "filename.ext" or 
*                       "dirname\filename.ext".
* Return:  
*               QL_RET_OK, success.
*               QL_RET_ERR_FILE_NO_CARD, no sd card.
*               QL_RET_ERR_PARAM, parameter error. 
*               QL_RET_ERR_FILENAMETOOLENGTH , filename too length.
*               QL_RET_ERR_FILEFAILED, fail to operate file.
******************************************************************************/
s32 Ql_FS_Rename(char *lpFileName, char *newLpFileName);


/******************************************************************************
* Function:     Ql_FS_CreateDir
*  
* Description:
*               Creates a directory in the UFS or SD card. 
*               If you want to create a directory in the UFS , you only need to use a relative path.
*               If you want to create a directory in the SD card , you also need to add prefix "SD:"
*               in front of the file name.
*
* Parameters:    
*               lpDirName:
*                   [in] The name of the directory.The name is limited to 252 characters.
*                   You must use a relative path,such as "dirname1" or "dirname1\dirname2".
* Return:  
*               QL_RET_OK, success.
*               QL_RET_ERR_FILE_NO_CARD, no sd card.
*               QL_RET_ERR_PARAM, parameter error. 
*               QL_RET_ERR_FILENAMETOOLENGTH , filename too length.
*               QL_RET_ERR_FILEFAILED, fail to operate file.
* Notes:
*               Don't support to create a directory in the RAM.
*
*               QL_RET_ERR_FS_FATAL_ERR1 indicates some fatal error happens to 
*                  the file system. Develops may call Ql_GetLastErrorCode() to 
*                  retrieve the inner error code. And develops can call Ql_Fs_Format(QL_FS_FAT) 
*                  to format FAT to retore the file system.
******************************************************************************/
s32 Ql_FS_CreateDir(char *lpDirName);


/******************************************************************************
* Function:     Ql_FS_DeleteDir
*  
* Description:
*               Deletes an existing directory.
*
* Parameters:    
*               lpDirName:
*                   [in] The name of the directory.The name is limited to 252 characters.
*                   You must use a relative path,such as "dirname1" or "dirname1\dirname2".
* Return:  
*               QL_RET_OK, success.
*               QL_RET_ERR_FILE_NO_CARD, no sd card.
*               QL_RET_ERR_PARAM, parameter error. 
*               QL_RET_ERR_FILENAMETOOLENGTH , filename too length.
*               QL_RET_ERR_FILEFAILED, fail to operate file.
* Notes:
*               Don't support to delete a directory in the RAM.
******************************************************************************/
s32 Ql_FS_DeleteDir(char *lpDirName);


/******************************************************************************
* Function:     Ql_FS_CheckDir
*  
* Description:
*               Check whether the file exists or not.
*
* Parameters:    
*               lpDirName:
*                   [in] The name of the directory.The name is limited to 252 characters.
*                   You must use a relative path,such as "dirname1" or "dirname1\dirname2".
* Return:  
*               QL_RET_OK, success.
*               QL_RET_ERR_FILE_NO_CARD, no sd card.
*               QL_RET_ERR_PARAM, parameter error. 
*               QL_RET_ERR_FILENAMETOOLENGTH , filename too length.
*               QL_RET_ERR_FILEFAILED, fail to operate file.
*               QL_RET_ERR_FILENOTFOUND, file not found.
* Notes:
*               Don't support to check directory in the RAM.
******************************************************************************/
s32 Ql_FS_CheckDir(char *lpDirName);


/******************************************************************************
* Function:     Ql_FS_FindFirst
*  
* Description:
*               This function searches a directory for a file or subdirectory 
*               whose name matches the specified file name. 
*
* Parameters:    
*               lpPath:
*                   [in] Point to a null-terminated string that specifies a valid directory or path.
*
*               lpFileName:
*                   [in] Point to a null-terminated string that specifies a valid file name, 
*                        which can contain wildcard characters, such as * and ?.
*
*               fileNameLength:
*                   [in] The maximum number of bytes to be received of the name.
*
*               fileSize:
*                   [out] A pointer to the variable which represents the size specified by the file.
*
*               isDir:
*                   [out] A pointer to the variable which represents the type specified by the file.
* Return:  
*               If the function succeeds, the return value is a search handle 
*               that can be used in a subsequent call to the "Ql_FindNextFile" or "Ql_FindClose" function.
*               If the function fails, the return value is an error codes.:
*               QL_RET_ERR_FILE_NO_CARD, no sd card.
*               QL_RET_ERR_PARAM, parameter error. 
*               QL_RET_ERR_FILENAMETOOLENGTH , filename too length.
*               QL_RET_ERR_FILEFAILED, fail to operate file.
*               QL_RET_ERR_FILENOMORE,  no more file.
*
*               QL_RET_ERR_FS_FATAL_ERR1 indicates some fatal error happens to 
*                  the file system. Develops may call Ql_GetLastErrorCode() to 
*                  retrieve the inner error code. And develops can call Ql_Fs_Format(QL_FS_FAT) 
*                  to format FAT to retore the file system.
******************************************************************************/
s32 Ql_FS_FindFirst(char *lpPath, char *lpFileName, u32 fileNameLength, u32 *fileSize, bool *isDir);

/******************************************************************************
* Function:     Ql_FS_FindNext
*  
* Description:
*               Continues a file search from a previous call to the Ql_FS_FindFirst 
*               function.
*
* Parameters:    
*               handle:
*                   [in] Search handle returned by a previous call to the Ql_FS_FindFirst
*
*               lpFileName:
*                   [in] Point to a null-terminated string that specifies a valid file name, 
*                        which can contain wildcard characters, such as * and ?.
*
*               fileNameLength:
*                   [in] The maximum number of bytes to be received of the name.
*
*               fileSize:
*                   [out] A pointer to the variable which represents the size specified by the file.
*
*               isDir:
*                   [out] A pointer to the variable which represents the type specified by the file.
* Return:  
*               QL_RET_OK, success.
*               QL_RET_ERR_PARAM, parameter error. 
*               QL_RET_ERR_FILEFAILED, fail to operate file.
*               QL_RET_ERR_FILENOMORE, file not found.
*
*               QL_RET_ERR_FS_FATAL_ERR1 indicates some fatal error happens to 
*                  the file system. Develops may call Ql_GetLastErrorCode() to 
*                  retrieve the inner error code. And develops can call Ql_Fs_Format(QL_FS_FAT) 
*                  to format FAT to retore the file system.
******************************************************************************/
s32 Ql_FS_FindNext(s32 handle, char *lpFileName, u32 fileNameLength, u32 *fileSize, bool *isDir);


/******************************************************************************
* Function:     Ql_FS_FindClose
*  
* Description:
*               Closes the specified search handle.
*
* Parameters:    
*               handle:
*                   [in] Find handle, 
*                        returned by a previous call of the Ql_FS_FindFirst function. 
* Return:  
*               None
******************************************************************************/
void Ql_FS_FindClose(s32 handle);


/******************************************************************************
* Function:     Ql_FS_XDelete
*                           
* Description:
*               Delete a file or directory.
*           
* Parameters:    
*               lpPath: 
*                   [in] File path to be deleted
*
*               flag: 
*                   [in] A u32 that defines the file's opening and access mode.
*                         The possible values are shown as follow:
*                         QL_FS_FILE_TYPE, 
*                         QL_FS_DIR_TYPE, 
*                         QL_FS_RECURSIVE_TYPE
*
* Return:     
*               QL_RET_OK, success.
*               QL_RET_ERR_PARAM, parameter error. 
*               QL_RET_ERR_FILENAMETOOLENGTH, filename too length.
*               QL_RET_ERR_FILENOTFOUND, file not found.
*               QL_RET_ERR_PATHNOTFOUND, path not found.
*               QL_RET_ERR_GET_MEM, fail to get memory.
*               QL_RET_ERR_GENERAL_FAILURE, general failture.
******************************************************************************/
s32  Ql_FS_XDelete(char* lpPath, u32 flag);


/******************************************************************************
* Function:     Ql_FS_XMove  
*                           
* Description:
*               This function provides a facility to move/copy a file or folder
*           
* Parameters:    
*               lpSrcPath: 
*                   [in] Source path to be moved/copied
*
*               lpDestPath:
*                   [in] Destination path
*
*               flag:
*                   [in] A u32 that defines the file's opening and access mode.
*                         The possible values are shown as follow:
*                          QL_FS_MOVE_COPY, 
*                          QL_FS_MOVE_KILL, 
*                          QL_FS_MOVE_OVERWRITE
*
* Return:       
*               QL_RET_OK, success.
*               QL_RET_ERR_PARAM, parameter error. 
*               QL_RET_ERR_FILENAMETOOLENGTH, filename too length.
*               QL_RET_ERR_FILENOTFOUND, file not found.
*               QL_RET_ERR_PATHNOTFOUND, path not found.
*               QL_RET_ERR_GET_MEM, fail to get memory.
*               QL_RET_ERR_FILE_EXISTS, file existed.
*               QL_RET_ERR_GENERAL_FAILURE, general failture.
******************************************************************************/
s32  Ql_FS_XMove(char* lpSrcPath, char* lpDestPath, u32 flag);


/******************************************************************************
* Function:     Ql_FS_GetFreeSpace
*  
* Description:
*               This function obtains the amount of free space on Flash or SD card.
*
* Parameters:    
*               storage:
*                   [in]The type of storage, which is one value of "Enum_FSStorage".
*                   typedef enum
*                   {
*                      Ql_FS_UFS = 1,
*                      Ql_FS_SD  = 2,
*                      Ql_FS_RAM = 3,
*                   } Enum_FSStorage;
*
* Return:  
*               The return value is the total number of bytes of the free space 
*               in the specified storage, if this function succeeds. Otherwise, 
*               the return value is an error code.
*               Ql_RET_ERR_UNKOWN, unkown error.
*
*               QL_RET_ERR_FS_FATAL_ERR1 indicates some fatal error happens to 
*                  the file system. Develops may call Ql_GetLastErrorCode() to 
*                  retrieve the inner error code. And develops can call Ql_Fs_Format(QL_FS_FAT) 
*                  to format FAT to retore the file system.
******************************************************************************/
s64  Ql_FS_GetFreeSpace (u32 storage);


/******************************************************************************
* Function:     Ql_FS_GetTotalSpace
*  
* Description:
*               This function obtains the amount of total space on Flash or SD card.
*
* Parameters:    
*               storage:
*                   [in]The type of storage, which is one value of "Enum_FSStorage".
*                   typedef enum
*                   {
*                      Ql_FS_UFS = 1,
*                      Ql_FS_SD  = 2,
*                      Ql_FS_RAM = 3,
*                   } Enum_FSStorage;
*
* Return:  
*               The return value is the total number of bytes in the specified  
*               storage, if this function succeeds. Otherwise, the return value
*               is an error code.
*               Ql_RET_ERR_UNKOWN, unkown error.
*
*               QL_RET_ERR_FS_FATAL_ERR1 indicates some fatal error happens to 
*                  the file system. Develops may call Ql_GetLastErrorCode() to 
*                  retrieve the inner error code. And develops can call Ql_Fs_Format(QL_FS_FAT) 
*                  to format FAT to retore the file system.
******************************************************************************/
s64  Ql_FS_GetTotalSpace(u32 storage);


/******************************************************************************
* Function:     Ql_Fs_Format  
*                           
* Description:
*               This function format the SD card or UFS
*           
* Parameters:    
*               storage:
*                   [in]The type of storage, which is one value of "Enum_FSStorage".
*
*               storage: can be one of the following values:
*                       Ql_FS_UFS,  // delete all existing files in UFS
*                       Ql_FS_SD,   // delete all existing data in SD card
*                       QL_FS_FAT   // format the whole file system, which takes tens of seconds.
*
* Return:       
*               QL_RET_OK, success.
*               QL_RET_ERR_PARAM, parameter error. 
*               QL_RET_ERR_FILENAMETOOLENGTH, filename too length.
*               QL_RET_ERR_FILENOTFOUND, file not found.
*               QL_RET_ERR_PATHNOTFOUND, path not found.
*               QL_RET_ERR_GET_MEM, fail to get memory.
*               QL_RET_ERR_GENERAL_FAILURE, general failture.
*               QL_RET_ERR_FILE_NO_CARD, no sd card
*               QL_RET_ERR_FILEFAILED, fail to operate file.
******************************************************************************/
s32 Ql_FS_Format(u8 storage);

#endif  //__QL_FS_H__
