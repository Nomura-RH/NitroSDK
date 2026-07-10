#define __WLSYS_C_
#define __INSYSROM__

#include "WlSys.h"
#include "WlLib.h"
#include "DbgChar.h"

#include "TaskMan.h"
#include "BufMan.h"
#include "WlCmdIf.h"
#include "WlNic.h"
#include "WlIntr.h"
#include "Diag.h"
#include "WlCmdLabel.h"
#include "MAC.h"
#include "Compati.h"

#include "libirissubpeeprom.h"
#include "EEPROM.h"
#include "Flash.h"
#include "CfgDevs.h"

void WlessLibReboot(void)
{

    ClearTimeOut();

    WShutdown();

    InitMac();

    ReleaseAllWlHeapBuf();
    InitializeTask();
    InitializeParam(wlMan->Config.pCAM, wlMan->Config.CamMaxStaNum);
    InitializeCmdIf();
    InitializeMLME();
    InitializeCAM();

    WSetDefaultParameters();
}

u32 WL_InitDriver(WlInit *pInit)
{
    u32 adrs = pInit->workingMemAdrs;
    u16 sts;

    *(u32 *)(WL_WORKADRS_BUF_ADRS) = adrs;

    MI_CpuClearFast((void *)adrs, sizeof(WL_MAN));

    wlMan->lockID = OS_GetLockID();

    wlMan->DmaChannel = pInit->dmaChannel;
    wlMan->DmaMaxCount = pInit->dmaMaxSize / 2;
    if (wlMan->DmaMaxCount == 0) {
        wlMan->DmaMaxCount = 0xFFFFFFFF;
    }

    InitializeHeapBuf((LPHEAP_INFO)&pInit->heapType);

#if (USE_FLASH)
    FLASH_MakeImage();
#endif

#ifndef SDK_TEG
    reg_SND_POWCNT |= 0x0002;
#endif

#ifndef SDK_TEG
    *(vu16 *)REG_EXMEMCNT_H = 0x0030;
#else
    *(vu16 *)REG_EXMEMCNT_H = PHI33M_OUT_ENABLE | WLMEM0_1ST_9WAIT | WLMEM0_2ND_5WAIT | WLMEM1_1ST_9WAIT | WLMEM1_2ND_9WAIT;
#endif

#ifdef SDK_TEG
    wlMan->bUnLocked = FALSE;
    OS_LockCartridge(wlMan->lockID);
#endif

#if (USE_FLASH)
    wlMan->WlDbgLevel = 0x0003;
    sts = 0;
#else
    AccConfigROM(ACC_CFG_ROM_ADRS, ACC_CFG_ROM_DATA, ACC_CFG_ROM_ERASE);
    sts = WLi_IrisReadEeprom((u16 *)IRIS_EEPROM_ADRS(wmTemp.WlDbgLevel), (u16 *)&wlMan->WlDbgLevel, 4);
    if (sts) {
        wlMan->Config.DiagResult |= WL_DIAG_ERR_EEPROM_ACC;
    }
#endif

#if (USE_FLASH)
    FLASH_Read(FLASH_ADRS(enableChannel), 2, (u8 *)&wlMan->EnableChannel);
    FLASH_Read(FLASH_ADRS(wlOperation), 2, (u8 *)&wlMan->WlOperation);
#else
    wlMan->EnableChannel = 0x7FFE;
    WLi_IrisReadEeprom((u16 *)IRIS_EEPROM_ADRS(wlOperation), &wlMan->WlOperation, 2);
#endif

    wlMan->pRecvMsgQueue = pInit->sendMsgQueuep;
    wlMan->pSendMsgQueue = pInit->recvMsgQueuep;

    InitializeParam((LPCAM_ELEMENT)pInit->camAdrs, pInit->camSize / sizeof(CAM_ELEMENT));
    InitializeTask();
    InitializeCmdIf();
    InitializeMLME();
    InitializeCAM();
    InitializeAlarm();

#if (USE_FLASH)
#ifndef SDK_SKIP_CHECKSUM_WL
    {
        u32 crc;
        if (FLASH_VerifyCheckSum(&crc) != 0) {
            wlMan->Config.DiagResult |= WL_DIAG_ERR_CHECKSUM;
#ifndef SDK_NOCHK_ERR_WL
            goto exit_init;
#endif
        }
    }
#endif
#endif

    WConfigDevice();

    DiagMacRegister();

    WWakeUp();

    InitMac();
    if (sts == 0) {
        InitRF();
    }

    DiagMacMemory();
#ifndef SDK_CPU_COPY_WL
    DiagMacDma();
#else
    DbgPrint("Not use MAC DMA\r\n");
#endif
    DiagBaseBand();

    if (sts == 0) {
        InitBaseBand();
    }

    WSetDefaultParameters();

    WShutdown();

exit_init:
#ifdef SDK_TEG
    OS_UnLockCartridge(wlMan->lockID);
    wlMan->bUnLocked = TRUE;
#endif

    OS_CreateThread(&wlMan->TaskMan.Thread, MainTaskRoutine, NULL, pInit->stack, pInit->stacksize, pInit->priority);
//#ifndef ONLY_WL
//    MI_DmaFill32(3, (void *)((u32)pInit->stack - pInit->stacksize + 4), STACK_CHECK_ID, pInit->stacksize - 4);
//#endif
    OS_WakeupThreadDirect(&wlMan->TaskMan.Thread);

    InitializeIntr();

    return wlMan->Config.DiagResult;
}

OSThread *WL_GetThreadStruct(void)
{
    return &wlMan->TaskMan.Thread;
}

void WL_Terminate(void)
{
    OSThread *pThread;

    pThread = WL_GetThreadStruct();
    if (TRUE == OS_IsThreadTerminated(pThread)) {
        DbgPrint("Not active WL Thread\r\n");
        return;
    }

    DbgPrint("AddTask TERMINATE_WL_TASK_ID\r\n");
    AddTask(TASK_NORMAL_PRIORITY, TERMINATE_WL_TASK_ID);
    DbgPrint("Exit AddTask TERMINATE_WL_TASK_ID\r\n");

    OS_JoinThread(pThread);
    while (OS_IsThreadTerminated(pThread) == FALSE) {}

    return;
}
