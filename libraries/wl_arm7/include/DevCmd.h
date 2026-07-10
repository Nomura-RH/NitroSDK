#ifndef __DEVCMD_H_
#define __DEVCMD_H_

// u16 DEV_IfcReqCmd         (WlCmdReq* pReqt, WlCmdCfm* pCfmt);
u16 DEV_ShutdownReqCmd(WlCmdReq *pReqt, WlCmdCfm *pCfmt);
u16 DEV_IdleReqCmd(WlCmdReq *pReqt, WlCmdCfm *pCfmt);
u16 DEV_Class1ReqCmd(WlCmdReq *pReqt, WlCmdCfm *pCfmt);
u16 DEV_RebootReqCmd(WlCmdReq *pReqt, WlCmdCfm *pCfmt);
u16 DEV_ClearWlInfoReqCmd(WlCmdReq *pReqt, WlCmdCfm *pCfmt);
u16 DEV_GetVerInfoReqCmd(WlCmdReq *pReqt, WlCmdCfm *pCfmt);
u16 DEV_GetWlInfoReqCmd(WlCmdReq *pReqt, WlCmdCfm *pCfmt);
u16 DEV_GetStateReqCmd(WlCmdReq *pReqt, WlCmdCfm *pCfmt);
u16 DEV_TestSignalReqCmd(WlCmdReq *pReqt, WlCmdCfm *pCfmt);
u16 DEV_TestRxReqCmd(WlCmdReq *pReqt, WlCmdCfm *pCfmt);

#ifdef __DEVCMD_C_

static void TestSignalRestart(void *arg);
static void CarrierSuppresionSignal(WlDevTestSignalReq *pReq);
void IntrCarrierSuppresionSignal(void);

#endif // __DEVCMD_C_
#endif // __DEVCMD_H_
