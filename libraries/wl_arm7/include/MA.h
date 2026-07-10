#ifndef __MA_H_
#define __MA_H_

u16 MA_DataReqCmd(WlCmdReq *pReqt, WlCmdCfm *pCfmt);
u16 MA_KeyDataReqCmd(WlCmdReq *pReqt, WlCmdCfm *pCfmt);
u16 MA_MpReqCmd(WlCmdReq *pReqt, WlCmdCfm *pCfmt);
u16 MA_TestDataReqCmd(WlCmdReq *pReqt, WlCmdCfm *pCfmt);
u16 MA_ClrDataReqCmd(WlCmdReq *pReqt, WlCmdCfm *pCfmt);

void IssueMaDataConfirm(LPHEAPBUF_MAN pBufMan, void *pBuf);

#ifdef __MA_C_

#define TXOP_NULL_2M_S (TIME_PREAMBLE_SHORT + 28 * TIME_BYTE_2M)
// #define	TXOP_NULL_1M_S	(TIME_PREAMBLE_SHORT + 28*TIME_BYTE_1M)
#define TXOP_NULL_2M_L (TIME_PREAMBLE_LONG + 28 * TIME_BYTE_2M)
#define TXOP_NULL_1M_L (TIME_PREAMBLE_LONG + 28 * TIME_BYTE_1M)

#define MPACK_2M_S (TIME_SIFS + TIME_PREAMBLE_SHORT + 32 * TIME_BYTE_2M)
// #define	MPACK_1M_S		(TIME_SIFS + TIME_PREAMBLE_SHORT +
// 32*TIME_BYTE_1M)
#define MPACK_2M_L (TIME_SIFS + TIME_PREAMBLE_LONG + 32 * TIME_BYTE_2M)
#define MPACK_1M_L (TIME_SIFS + TIME_PREAMBLE_LONG + 32 * TIME_BYTE_1M)

#define TIME_MAX_MP_BACKOFF (TIME_DIFS + 620)
#define TIME_DELT_MP        (50)

#define DELT_TXOP 2

static void MpTmpttZero(void *arg);

#endif // __MA_C_
#endif // __MA_H_
