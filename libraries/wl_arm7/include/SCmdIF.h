#ifndef __SCMDIF_H_
#define __SCMDIF_H_

void InitializeSerialCmd(void);
void InitSCmd(void);
void SerialCmdTask(void);
void SCmdBTXTTask(void);

u32 myatoi(u8 *pStr);
void StrToMacAdrs(u8 *strMacAdrs, u16 *pMacAdrs);
u32 mystrncmp(u8 *pStr1, u8 *pStr2, u32 length);

extern u16 PTstTXOP;
extern u16 PTstBitmap;
extern u16 PTstDataLen;
extern u16 PTstDuration;
extern u16 PTstMaTxMode;
extern u16 PTstChannel;
#define MAX_PARAM_NUM 20
#define SCMD_BUF_SIZE 160

typedef struct {
    volatile union {
        u16 Word;
        struct {
            u16 Busy : 1;
            u16 Mode : 1;
            u16 dum : 14;
        } Bit;
    } Flag;
    u16 OutDevice;
    u16 BufOffset;
    u16 ParamCount;
    u16 ParamOffset[MAX_PARAM_NUM];
    u8 RxBuf[SCMD_BUF_SIZE];
} SCMD_TABLE;

extern SCMD_TABLE SCmd;

extern u16 app_num;

#define CHAR_A     'A'
#define CHAR_S     'S'
#define CHAR_X     'X'
#define CHAR_Z     'Z'
#define CHAR_E     'E'
#define CHAR_I     'I'
#define CHAR_NULL  0x00
#define CHAR_BS    0x08
#define CHAR_CR    0x0d
#define CHAR_LF    0x0a
#define CHAR_SPACE ' '
#define CHAR_YEN   '\\'

#define PARAM_ADRS(_x_) ((u32) & SCmd.RxBuf[0] + SCmd.ParamOffset[_x_])
// #define	PARAM_SIZE(_x_)		(SCmd.ParamOffset[_x_+1] -
// SCmd.ParamOffset[_x_] - 1)
#define PARAM_SIZE(_x_) (SCmd.ParamOffset[_x_ + 1] - SCmd.ParamOffset[_x_] - 1)
#define PARAM_COUNT     (SCmd.ParamCount)

#define CMD_NO_ERR                    0
#define CMD_ERR_NOT_FOUND             10
#define CMD_ERR_NOT_SUPPORT           23
#define CMD_ERR_ONLY_SUPPORT_FOR_ARM7 34
#define CMD_ERR_ONLY_SUPPORT_FOR_ARM9 43
#define CMD_ERR_PARAMETER             58

#ifdef __SCMDIF_C_

typedef struct {
    char *pCmdStr;
    int (*pFunc)(void);
} SCMD_TBL;

#define SCMD_NUM (sizeof(SCmdTbl) / sizeof(SCMD_TBL))

#define SCMD_DEVICE *(vu32 *)SCMD_SELECT_DEV_ADRS

#define SCMD_ARM7 1
#define SCMD_ARM9 2
#define SCMD_BOTH (SCMD_ARM7 | SCMD_ARM9)

#define SCMD_FUNC_BOTH 1
#define SCMD_FUNC_ARM7 1
#define SCMD_FUNC_ARM9 0

#define SCmdFunction_BOTH(_func_) \
    int _func_(void);             \
    int _func_(void)
#define SCmdFunction_ARM7(_func_) \
    int _func_(void);             \
    int _func_(void)
#define SCmdFunction_ARM9(_func_)             \
    int _func_(void);                         \
    int _func_(void)                          \
    {                                         \
        return CMD_ERR_ONLY_SUPPORT_FOR_ARM9; \
    }

u32 WaitMessage(WlCmdReq *pReq);
void StateShutdown(void);
void StateIdle(void);
void ExecuteCommand(void);
void AnalyzeCommand(void);
void InitSCmd(void);

#endif // __SCMDIF_C_
#endif // __SCMDIF_H_
