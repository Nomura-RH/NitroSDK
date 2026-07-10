#ifndef __DIAG_H_
#define __DIAG_H_

void DiagEEPRom(void);
void DiagMacRegister(void);
void DiagMacMemory(void);
void DiagBaseBand(void);
void DiagBaseBand2(void);
void DiagRF(void);

#ifdef __DIAG_C_

#define MAX_DIAG_ERR 32

void Diag_DmaClear(u16 data);

typedef struct {
    u16 adrs;
    u16 mask;
} TEST_REGS;

#endif // __DIAG_C_
#endif // __DIAG_H_
