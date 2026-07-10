#ifndef __SCMD_H_
#define __SCMD_H_

#include "AppBuf.h"

typedef struct {
    u16 Verify : 1;
    u16 DataInc : 1;
    u16 NotAdrsInc : 1;
    u16 DispOk : 1;
    u16 : 0;
} SCMD_MM_FLAG;

typedef struct {
    u16 Status;
    u16 rsv0;
    u8 RetryCount;
    u8 rsv1;
    u8 AppRate;
    u8 rsv2;
    u8 Rate;
    u8 Service;
    u16 MPDU;

    u16 FC;
    u16 Duration;
    u16 Adr1[3];
    u16 Adr2[3];
    u16 Adr3[3];
    u16 SeqCon;
    u8 FBoby[100];

} TST_TX_STRUCT;

void DumpMemory(u8 *pData, u32 Length, u32 type);
void DumpMemory2(u8 *pData, u32 Length, u32 type, u32 width);
void ModifyMemory(vu8 *pAdrs, u32 Data, u32 type, u32 size, SCMD_MM_FLAG flag);
void DumpWlParam(u32 type);
void DispRateSetList(u16 rateMap);

void DumpMACRegister(u32 adrs, u32 siz);
void DumpBBPRegister(u32 adrs);
void DumpEEPRom(u32 adrs);
void ModifyMACRegister(u32 adrs, u32 data, u32 verify);
void ModifyBBPRegister(u32 adrs, u32 data, u32 verify);
void ModifyRFRegister(u32 data);

void EepromReadCmd(u32 adrs);
void EepromWriteEnableCmd(void);
void EepromWriteCmd(u32 adrs, u32 data, u32 verify);
void EepromWriteAllCmd(u32 data, u32 verify);
void EepromWriteDisableCmd(void);
void EepromEraseCmd(u32 adrs, u32 verify);
void EepromEraseAllCmd(u32 verify);
void NinReadCmd(u32 adrs, u32 siz);
void NinWriteCmd(u32 adrs, u32 data, u32 siz, u32 verify);
void NinReadStatusCmd(void);
void DiagMTM_EEPRom(void);
void DiagNTD_EEPRom(void);

void DispWlTxCtrl(void);

void DispWlCounter(u32 flag);
void DispHeapBufMan(void);
void DispHeapBufManOne(char *pStr, APP_HEAPBUF_MAN *pBufMant);
u32 CheckHeapBuf(u32 bDisp);

void StartMPDBMode(void);
void StopMPDBMode(void);
void StartTxMPDB(u16 count, u16 bitmap, u16 size, u8 dup, u16 txop);
void StartRxMPDB(u16 ksId, u8 disp);
void DispMPDBCnt(void);

void TstParentTx(void);
void TstChildTx(void);
u32 MaTstCmd(void);

void DispTxMacRegs(void);
void DispRxMacRegs(void);

void TestLockDetect(u32 tst);
void MeasureLockDetectForSCmd(u32 avg_c);

void TestCheckSum(void);

#ifdef __SCMD_C_

#endif // __SCMD_C_

#ifdef __SCMDSUBP_C_

#define MAX_DIAG_ERR 32

#endif // __SCMDSUBP_C_

#endif // __SCMD_H_
