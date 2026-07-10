#ifndef __DBGCHAR_H_
#define __DBGCHAR_H_

#define SYS_DEF_DBGLEVEL (0x00000000)
#define WL_DEF_DBGLEVEL  (B_WL_DBG_MSG | B_WL_DBG_MLME)

#define B_WL_DBG_MSG       0x00000001
#define B_WL_DBG_DIAGMSG   0x00000002
#define B_WL_DBG_ERRMSG    0x00000004
#define B_WL_DBG_ERRDETAIL 0x00000008
#define B_WL_DBG_WARNMSG   0x00000010
#define B_WL_DBG_CMDIF     0x00000020
#define B_WL_DBG_MLME      0x00000040
#define B_WL_DBG_MA        0x00000080
#define B_WL_DBG_PARAM     0x00000100
#define B_WL_DBG_DEVCMD    0x00000200
#define B_WL_DBG_SYSCMD    0x00000400
#define B_WL_DBG_INT       0x00000800
#define B_WL_DBG_MP        0x00001000
#define B_WL_DBG_MANCTRL   0x00002000
#define B_WL_DBG_BEACON    0x00004000
#define B_WL_DBG_DEFRAG    0x00008000
#define B_WL_DBG_POWERSAVE 0x00010000
#define B_WL_DBG_WRITE_DEV 0x00020000
#define B_WL_DBG_CMD_CODE  0x00040000

#define B_WL_DBG_MACSTOP      0x80000000
#define B_WL_DBG_MAC_NOT_POLL 0x01000000
#define B_WL_DBG_DECSEQCTRL   0x00800000
#define B_WL_DBG_MAC          0x00400000

#define BPOS_WL_DBG 0
#define BPOS_WL_MAC 24

#define BM_WL_DBG (u32)0x00FFFFFFul
#define BM_WL_MAC (u32)0x000000FFul

#define WL_DBG_WARNMSG_K1 '1'
#define WL_DBG_WARNMSG_K2 '2'

#define WL_DBG_CMDIF_MLME      'a'
#define WL_DBG_CMDIF_MA        'b'
#define WL_DBG_CMDIF_PARAMSET  'c'
#define WL_DBG_CMDIF_PARAMSET2 'd'
#define WL_DBG_CMDIF_PARAMGET  'e'
#define WL_DBG_CMDIF_PARAMGET2 'f'
#define WL_DBG_CMDIF_DEVCMD    'g'
#define WL_DBG_CMDIF_SYSCMD    'h'
#define WL_DBG_CMDIF_NOTCMD    'i'

#define WL_DBG_DEVCMD_IFC        'A'
#define WL_DBG_DEVCMD_SHUTDOWN   'B'
#define WL_DBG_DEVCMD_IDLE       'C'
#define WL_DBG_DEVCMD_CLASS1     'D'
#define WL_DBG_DEVCMD_REBOOT     'E'
#define WL_DBG_DEVCMD_CLRINFO    'F'
#define WL_DBG_DEVCMD_VERINFO    'G'
#define WL_DBG_DEVCMD_GETINFO    'H'
#define WL_DBG_DEVCMD_GETSTA     'I'
#define WL_DBG_DEVCMD_TESTSIGNAL 'J'
#define WL_DBG_DEVCMD_TESTRX     'K'

#define WL_DBG_MLME_RESET  'A'
#define WL_DBG_MLME_PWRMGT 'B'
#define WL_DBG_MLME_SCAN   'C'
#define WL_DBG_MLME_JOIN   'D'
#define WL_DBG_MLME_AUTH   'E'
#define WL_DBG_MLME_DEAUTH 'F'
#define WL_DBG_MLME_ASS    'G'
#define WL_DBG_MLME_REASS  'H'
#define WL_DBG_MLME_DISASS 'I'
#define WL_DBG_MLME_START  'J'
#define WL_DBG_MLME_MSCH   'K'

#define WL_DBG_MLME_SCAN_IN  'L'
#define WL_DBG_MLME_SCAN_CH  'M'
#define WL_DBG_MLME_SCAN_ST  'N'
#define WL_DBG_MLME_SCAN_FIN 'O'
#define WL_DBG_MLME_SCAN_TO  'P'

#define WL_DBG_MLME_JOIN_IN  'Q'
#define WL_DBG_MLME_JOIN_FIN 'R'
#define WL_DBG_MLME_JOIN_TO  'S'

#define WL_DBG_MLME_AUTH_IN  'Q'
#define WL_DBG_MLME_AUTH_FIN 'R'
#define WL_DBG_MLME_AUTH_TO  'S'

#define WL_DBG_MLME_ASS_IN  'Q'
#define WL_DBG_MLME_ASS_FIN 'R'
#define WL_DBG_MLME_ASS_TO  'S'

#define WL_DBG_MA_DATA_REQ    'Z'
#define WL_DBG_MA_DATA_CFM    'Y'
#define WL_DBG_MA_DATA_IND    'X'
#define WL_DBG_MA_KEYDATA_REQ 'W'
#define WL_DBG_MA_MP_REQ      'V'
#define WL_DBG_MA_CLRDATA_REQ 'U'

#define WL_DBG_INT_PRETBTT     'a'
#define WL_DBG_INT_TBTT        'b'
#define WL_DBG_INT_TXEND_TST   'c'
#define WL_DBG_INT_RXEND_TST   'd'
#define WL_DBG_INT_RDMA_END    'e'
#define WL_DBG_INT_WDMA_END    'f'
#define WL_DBG_INT_ACKCNTOVF   'g'
#define WL_DBG_INT_CNTOVF      'h'
#define WL_DBG_INT_TXERR       'i'
#define WL_DBG_INT_RXERR       'j'
#define WL_DBG_INT_TXEND       'k'
#define WL_DBG_INT_RXEND       'l'
#define WL_DBG_INT_MPEND       'm'
#define WL_DBG_INT_ACTEND      'n'
#define WL_DBG_INT_START_TX    'o'
#define WL_DBG_INT_START_RX    'p'
#define WL_DBG_INT_RFWAKEUP    'r'
#define WL_DBG_INT_RXMPDUP_ERR 's'
#define WL_DBG_INT_ICVOK       't'
#define WL_DBG_INT_ICVERR      'u'
#define WL_DBG_INT_BUF_OVF_ERR 'v'

#define WL_DBG_TX_MP      'A'
#define WL_DBG_TX_MP_KEY  'B'
#define WL_DBG_TX_MP_NULL 'C'
#define WL_DBG_TX_MP_ACK  'D'

#define WL_DBG_RX_MP      'E'
#define WL_DBG_RX_MP_KEY  'F'
#define WL_DBG_RX_MP_NULL 'G'
#define WL_DBG_RX_MP_ACK  'H'

#define WL_DBG_MA_KEY_NM 'K'

#define WL_DBG_TX_ATIM     'A'
#define WL_DBG_TX_DISASS   'B'
#define WL_DBG_TX_ASSREQ   'C'
#define WL_DBG_TX_ASSRES   'D'
#define WL_DBG_TX_REASSREQ 'E'
#define WL_DBG_TX_REASSRES 'F'
#define WL_DBG_TX_PRBREQ   'G'
#define WL_DBG_TX_PRBRES   'H'
#define WL_DBG_TX_AUTH     'I'
#define WL_DBG_TX_END      'J'
#define WL_DBG_TX_DEAUTH   'K'
#define WL_DBG_TX_PSPOLL   'L'
#define WL_DBG_TX_CFEND    'M'

#define WL_DBG_RX_BEACON   'N'
#define WL_DBG_RX_DISASS   'O'
#define WL_DBG_RX_ASSREQ   'P'
#define WL_DBG_RX_ASSRES   'Q'
#define WL_DBG_RX_REASSREQ 'R'
#define WL_DBG_RX_REASSRES 'S'
#define WL_DBG_RX_PRBREQ   'T'
#define WL_DBG_RX_PRBRES   'U'
#define WL_DBG_RX_AUTH     'V'
#define WL_DBG_RX_AUTH1    'W'
#define WL_DBG_RX_AUTH2    'X'
#define WL_DBG_RX_AUTH3    'Y'
#define WL_DBG_RX_AUTH4    'Z'
#define WL_DBG_RX_AUTHX    '!'
#define WL_DBG_RX_DEAUTH   '"' //"
#define WL_DBG_RX_PSPOLL   '#'
#define WL_DBG_RX_CFEND    '$'
#define WL_DBG_RX_ATIM     '%%'
#define WL_DBG_RX_DUP      '-'

#define WL_DBG_DEFRAG_IN      'A'
#define WL_DBG_DEFRAG_NEW     'B'
#define WL_DBG_DEFRAG_DUP     'C'
#define WL_DBG_DEFRAG_ADD     'D'
#define WL_DBG_DEFRAG_MORE    'E'
#define WL_DBG_DEFRAG_FOUND   'F'
#define WL_DBG_DEFRAG_LAST    'G'
#define WL_DBG_DEFRAG_TIMEOUT 'H'

#define WL_DBG_FPS_DISABLE 'A'
#define WL_DBG_FPS_ACTIVE  'B'
#define WL_DBG_FPS_SLEEP   'C'
#define WL_DBG_PS_ACTIVE   'D'
#define WL_DBG_PS_SLEEP    'E'
#define WL_DBG_PS_TBTT     'F'

#define B_WM_DBG_MSG      0x00000001
#define B_WM_DBG_INIT     0x00000002
#define B_WM_DBG_MLME     0x00000004
#define B_WM_DBG_MLME_MSG 0x00000008

#define WM_DBG_INIT_DEV    'A'
#define WM_DBG_INIT_IDLE   'B'
#define WM_DBG_INIT_PARAM  'C'
#define WM_DBG_INIT_CLASS1 'D'
#define WM_DBG_INIT_REBOOT 'E'

#define WM_DBG_MLME_RESET  'A'
#define WM_DBG_MLME_PWRMGT 'B'
#define WM_DBG_MLME_SCAN   'C'
#define WM_DBG_MLME_JOIN   'D'
#define WM_DBG_MLME_AUTH   'E'
#define WM_DBG_MLME_DEAUTH 'F'
#define WM_DBG_MLME_ASS    'G'
#define WM_DBG_MLME_REASS  'H'
#define WM_DBG_MLME_DISASS 'I'
#define WM_DBG_MLME_START  'J'
#define WM_DBG_MLME_MSCH   'K'

#define WM_DBG_MLME_CFM 'L'
#define WM_DBG_MLME_IND 'M'
#define WM_DBG_MA_IND   'N'

#define B_MACTEST_RX 0x0001

#define DDO_NONE              0
#define DDO_WL_MAC            1
#define DDO_WL_PS             2
#define DDO_WL_MP             3
#define DDO_WL_TASK           4
#define DDO_WL_INT            5
#define DDO_WL_PSPOLL         6
#define DDO_WL_RESET_TXQ      7
#define DDO_WL_PS2            8
#define DDO_WL_MACBUG_NOTPOLL 9
#define DDO_WL_LSTN_INT       10
#define DDO_WL_TSF            11
#define DDO_WL_CONNECT        12

#define DDO_WL_TEST 15

#define DDO_HOST_IF  100
#define DDO_SOUND_TM 101

#define DDO_WL_MAC_RX_LEN_ERR     0x8000
#define DDO_WL_MAC_CURRERR        0x4000
#define DDO_WL_MAC_CURROVRRING    0x2000
#define DDO_WL_MAC_RESUME_SEQ_ERR 0x1000
#define DDO_WL_MAC_MPING          0x0800
#define DDO_WL_MAC_MP_RESUME      0x0400
#define DDO_WL_MAC_MPEND_ERR      0x0200
#define DDO_WL_MAC_RX_MISBSSID    0x0100
#define DDO_WL_MAC_RXMP_SEQERR    0x0080
#define DDO_WL_MAC_BUF_OVF_ERR    0x0040
#define DDO_WL_MAC_WEP_ERR        0x0004
#define DDO_WL_MAC_UPDATESEQ      0x0020
#define DDO_WL_MAC_FC_ERR         0x0010
#define DDO_WL_MAC_DESTROY_TXBUF  0x0008
// #define	DDO_WL_MAC_RESTORE_TXBUF	0x0004
#define DDO_WL_MAC_MPEND_BUFERR 0x0002
#define DDO_WL_MAC_NOT_MPSEQ    0x0001
#define DDO_WL_MAC_FATAL_ERR    0x0001
// #define	DDO_WL_MAC_BUF_OVF_ERR		0x1000
#define DDO_WL_MAC_TX_FROM_RXBUF 0x8000
// #define	DDO_WL_MAC_MP_AID_MISSMATCH	0x2000
#define DDO_WL_MAC_MP_CNT_MISSMATCH 0x1000
#define DDO_WL_MAC_DESTROY_TXBUF2   0x2000
#define DDO_WL_MAC_RXKEYLEN_ERR     0x0100

#define DDO_WL_PS_PRETBTT   0x0001
#define DDO_WL_PS_TBTT      0x0002
#define DDO_WL_PS_ACTEND    0x0004
#define DDO_WL_PS_RF_WAKEUP 0x0008
#define DDO_WL_PS_ACTZONE   0x0010
#define DDO_WL_PS_MPING     0x0020
#define DDO_WL_PS_SETTMPTT  0x0040
#define DDO_WL_PS_CHG_STATE 0x0100
#define DDO_WL_PS_TEST      0x0200
// #define	DDO_WL_PS_PSPOLL			0x0800
#define DDO_WL_PS_RX_MP      0x1000
#define DDO_WL_PS_RX_MPACK   0x2000
#define DDO_WL_PS_SET_POWER  0x4000
#define DDO_WL_PS_FIRM_ACT   0x8000
#define DDO_WL_PS_MADATA_IND 0x0080

#define DDO_WL_PS2_PRETBTT 0x0001
#define DDO_WL_PS2_TBTT    0x0002
// #define	DDO_WL_PS2_ACTEND			0x0004
#define DDO_WL_PS2_RF_WAKEUP 0x0008
#define DDO_WL_PS2_ACTZONE   0x0010
#define DDO_WL_PS2_MPING     0x0020
#define DDO_WL_PS2_TXQ0      0x0100
#define DDO_WL_PS2_TXQ1      0x0200
#define DDO_WL_PS2_TXQ2      0x0400
#define DDO_WL_PS2_BCN_LOST  0x8000

#define DDO_WL_MP_KEYDATA_REQ   0x1000
#define DDO_WL_MP_RX_NP         0x2000
#define DDO_WL_MP_MPING         0x4000
#define DDO_WL_MP_KEYBUF_0      0x0001
#define DDO_WL_MP_KEYBUF_1      0x0002
#define DDO_WL_MP_RX_MP         0x0004
#define DDO_WL_MP_RX_MPACK      0x0008
#define DDO_WL_MP_ACTZONE       0x0010
#define DDO_WL_MP_RXBEACON      0x0020
#define DDO_WL_MP_LAST_MP       0x0040
#define DDO_WL_MP_BLOCK_ERR     0x0080
#define DDO_WL_MP_NOT_MPSEQ     0x0100
#define DDO_WL_MP_RXKEY_CAM_ERR 0x0200
#define DDO_WL_MP_KEYBUF_ERR_1  0x0400
#define DDO_WL_MP_KEYBUF_ERR_2  0x0800

#define DDO_WL_MP_RXEN_WAIT 0x0080

#define DDO_WL_TASK_NUM        0x007F
#define DDO_WL_TASK_INT_WL     0x8000
#define DDO_WL_TASK_INT_VBLANK 0x4000
#define DDO_WL_TASK_INT_UART   0x2000
#define DDO_WL_TASK_INT_TIMER1 0x1000
#define DDO_WL_TASK_INT_TIMER3 0x0800
// #define	DDO_WL_TASK_INT_		0x0400
#define DDO_WL_TASK_TEMP  0x0200
#define DDO_WL_TASK_RXMSG 0x0100
#define DDO_WL_TASK_TEMP2 0x0080

#define DDO_WL_INT_PRETBTT     0x0001
#define DDO_WL_INT_TBTT        0x0002
#define DDO_WL_INT_TXEND_TST   0x0003
#define DDO_WL_INT_RXEND_TST   0x0004
#define DDO_WL_INT_RDMA_END    0x0005
#define DDO_WL_INT_WDMA_END    0x0006
#define DDO_WL_INT_ACKCNTOVF   0x0007
#define DDO_WL_INT_CNTOVF      0x0008
#define DDO_WL_INT_TXERR       0x0009
#define DDO_WL_INT_RXERR       0x000A
#define DDO_WL_INT_TXEND       0x000B
#define DDO_WL_INT_RXEND       0x000C
#define DDO_WL_INT_MPEND       0x000D
#define DDO_WL_INT_ACTEND      0x000E
#define DDO_WL_INT_START_TX    0x000F
#define DDO_WL_INT_START_RX    0x0010
#define DDO_WL_INT_RFWAKEUP    0x0011
#define DDO_WL_INT_RXMPDUP_ERR 0x0012

#define DDO_WL_INT_RXFRAME         0x8000
#define DDO_WL_INT_RXDMA           0x4000
#define DDO_WL_INT_RXHEAPOVF       0x2000
#define DDO_WL_INT_TXQ_RESTORE_ERR 0x2000
#define DDO_WL_INT_TXQ_SUSPEND_ERR 0x1000
#define DDO_WL_INT_RXFRAMETYPE     0x0F00

#define DDO_WL_PSPOLL_TBTT     0x0001
#define DDO_WL_PSPOLL_RXBEACON 0x0002
#define DDO_WL_PSPOLL_DUP      0x0004
#define DDO_WL_PSPOLL_BUSY     0x0008

#define DDO_WL_CONNECT_ACTZONE 0x0001
#define DDO_WL_CONNECT_JOIN    0x0002
#define DDO_WL_CONNECT_AUTH    0x0004
#define DDO_WL_CONNECT_ASS     0x0008

#define DDO_WL_TEST_MLME_TIMER 0x0001

#define DDO_WL_RESET_TXQ_QED 0x0001
#define DDO_WL_RESET_TXQ_ERR 0x0002

#define DDO_WL_MACBUG_NOTPOLL_RXSTART  0x8000
#define DDO_WL_MACBUG_NOTPOLL_CHK_FC   0x4000
#define DDO_WL_MACBUG_NOTPOLL_ACTZONE  0x2000
#define DDO_WL_MACBUG_NOTPOLL_RESET    0x0800
#define DDO_WL_MACBUG_NOTPOLL_TIMEOUT3 0x0400
#define DDO_WL_MACBUG_NOTPOLL_TIMEOUT2 0x0200
#define DDO_WL_MACBUG_NOTPOLL_TIMEOUT1 0x0100
#define DDO_WL_MACBUG_NOTPOLL_RES_ERR  0x0040
#define DDO_WL_MACBUG_NOTPOLL_SUS_ERR  0x0020
#define DDO_WL_MACBUG_NOTPOLL_SUSPEND  0x0010
#define DDO_WL_MACBUG_NOTPOLL_RXMP     0x0008
#define DDO_WL_MACBUG_NOTPOLL_TXQ2     0x0004
#define DDO_WL_MACBUG_NOTPOLL_TXQ1     0x0002
#define DDO_WL_MACBUG_NOTPOLL_TXQ0     0x0001

#define DDO_WL_LSTN_INT_TBTT        0x0001
#define DDO_WL_LSTN_INT_TBTT_PS     0x0002
#define DDO_WL_LSTN_INT_TBTT_ACTIVE 0x0004
#define DDO_WL_LSTN_INT_RX_MYBEACON 0x0008
#define DDO_WL_LSTN_INT_BEACON_LOST 0x0010

#define DDO_WL_TSF_SET     0x0001
#define DDO_WL_TSF_RXMP    0x0002
#define DDO_WL_TSF_RXMPACK 0x0004
#define DDO_WL_TSF_OVRTSF  0x0008

#define DDO_HOSTIF         0x0001
#define DDO_HOSTIF_IDLE    0x0002
#define DDO_HOSTIF_97      0x0004
#define DDO_HOSTIF_ERR     0x0006
#define DDO_HOSTIF_IND     0x0008
#define DDO_HOSTIF_MA      0x000A
#define DDO_HOSTIF_MA_DATA 0x000C

// #define DDO_HOSTIF_WINT		0xFF00
// #define DDO_HOSTIF_WTSK		0x001E

#define DDO_ARM9_OPE       0x000F
#define DDO_ARM9_OPE_WAIT  0x00BF
#define DDO_HIT_MADATACFM1 0xFAFA
#define DDO_HIT_MADATACFM2 0xFFE1

#define DDO_BUSY1 0x1111
#define DDO_BUSY2 0x2222
#define DDO_BUSY3 0x3333
#define DDO_BUSY4 0x4444
#define DDO_BUSY5 0x5555
#define DDO_BUSY6 0x6666
#define DDO_BUSY7 0x7777
#define DDO_BUSY8 0x8888

#define DDO_MA_DATA_reqh   0x0100
#define DDO_MA_KEY_reqh    0x0200
#define DDO_MA_MP_reqh     0x0300
#define DDO_MA_DATA_cnfh   0x0500
#define DDO_MA_KEY_cnfh    0x0600
#define DDO_MA_MP_cnfh     0x0700
#define DDO_MA_DATA_cnfDh  0x05D0
#define DDO_MA_KEY_cnfDh   0x06D0
#define DDO_MA_MP_cnfDh    0x07D0
#define DDO_MA_DATA_Indh   0x0A00
#define DDO_MA_MP_Indh     0x0B00
#define DDO_MA_MP_END_Indh 0x0C00

#define DDO_MA_CFM_CNT 0x8800

#define DDO_MA_MP_IndhNull 0xFB00

#define DDO_CMP 0x0010

#define DDO_WCMP 0x0020

#define DDO_MA_DATA_reqW 0x0100
#define DDO_MA_KEY_reqW  0x0200
#define DDO_MA_MP_reqW   0x0300

#define DDO_WMT_MAIN 0x0040

#define DDO_WMT_RECV 0x0080
#define DDO_MP_DUP_P 0x5678

#define DDO_WMT_RECVM 0x0081

#define DDO_MA_DATA_cnfR   0x0500
#define DDO_MA_KEY_cnfR    0x0600
#define DDO_MA_MP_cnfR     0x0700
#define DDO_MA_DATA_IndR   0x0A00
#define DDO_MA_MP_IndR     0x0B00
#define DDO_MA_MP_END_IndR 0x0C00

#define DDO_MAFAST      0x1000
#define DDO_MA_DCFM_ERR 0x2000
#define DDO_EMEM_ERR    0x8000

#define DDO_IOIFMANY_ERR  0xAA00
#define DDO_IOIFMANY_ERR1 0xAB00

#define DDO_BUFOVF      0xAABB
#define DDO_SENDOVF     0xAADD
#define DDO_ALLOERR     0xAACC
#define DDO_HIF_ALLOERR 0xFFF0
#define DDO_OSREL       0x0D00

#define DDO_ERR_TMPTTw 0xFFF2

#define DDO_WMT_CFM      0x0200
#define DDO_WMT_MLME_IND 0x0400
#define DDO_WMT_MA_IND   0x0800
#define DDO_WMT_ERR      0x0F00
#define DDO_IOIF         0x1000
#define DDO_IOIF_97      0x2000
#define DDO_IOIF_79      0x4000
#define DDO_IOIF_SU      0x8000

#define DDO_EXE9 0x0040
#define DDO_CMD9 0x0080

#define DDO_MA_MP_IndW    0xAA00
#define DDO_MA_MPEND_IndW 0xBB00
#define DDO_MA_DATA_IndW  0xCC00
// #define DDO_MA_DEQUEUE_IndW	0xDD00

#define DDO_MA_MP_req   0x0011
#define DDO_MA_DATA_req 0x0022
#define DDO_MA_KEY_req  0x0033

#define DDO_MA_MP_Ind     0x00AA
#define DDO_MA_MP_END_Ind 0x00BB
#define DDO_MA_DATA_Ind   0x00CC
// #define DDO_MA_DEQU_Ind		0x00DD
#define DDO_MA_MP_cnf   0x0066
#define DDO_MA_DATA_cnf 0x0077
#define DDO_MA_KEY_cnf  0x0088

#define DDO_0x0000 0x0000

#define DDO_ERR_MPEND_BUF 0xFFFD
#define DDO_ERR_K1        0xFFFE
#define DDO_ERR_K2        0xFFFF

#define DDO_ERR_TMPTT 0xFFF1

#define DDO_ERR_MIXCMP 0xEEE1
#define DDO_ERR_MIXHIF 0xEEE2

#define DDO_ERR_MIXCMP_Operating 0xDD00

#define DDO_WCMDP_FIRST 0xEE00
#define DDO_STS         0xBB00

#define DDO_Nodat 0xAA00

#define DDO_CMDBUSYh 0x9900

#define DDO_CMDBUSYi 0x0099

#define DDO_FULL_IND_ERR 0x0077

#define DDO_BUSY 0x7700

#define DDO_WDAT 0x7770

#define DDO_TM_0xFFFF 0xFFFF
#define DDO_TM_0x0000 0x0000

#define TEST_WL_NONE 0
#define TEST_WL_PS_1 1
#define TEST_WL_PS_2 2
#define TEST_WL_PS_3 3
#define TEST_WL_PS_4 4
#define TEST_WL_PS_5 5
#define TEST_WL_PS_6 6
#define TEST_WL_PS_7 7
#define TEST_WL_PS_8 8
#define TEST_WL_PS_9 9

#define TEST_WL_MP_RESUME 2
#define TEST_WL_MP_NORES  2

#define TEST_WL_RECONNECT   3
#define TEST_WL_RST_TXQ     4
#define TEST_WL_NOTOPEN_TXQ 4
#define TEST_WL_NOTPOLL_STA 4

#define TEST_WL_DIFF_TBTT 5

#endif // __DBGCHAR_H_
