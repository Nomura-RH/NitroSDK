#include "card_common.h"
#include "card_rom.h"
#include "card_spi.h"

void CARDi_OnFifoRecv(PXIFifoTag tag, u32 data, BOOL err)
{
    if (tag == PXI_FIFO_TAG_FS && err) {
        CARDiCommon *const p = &cardi_common;

        if (!p->recv_step) {
            p->command = (int)data;
        }

        switch (p->command) {
        case CARD_REQ_INIT:
            switch (p->recv_step) {
            case 0:
                break;
            case 1:
                p->cmd = (CARDiCommandArg *)data;
                p->flag |= CARD_STAT_RECV;
                break;
            }
            break;
        case CARD_REQ_IDENTIFY:
        case CARD_REQ_READ_ID:
        case CARD_REQ_READ_ROM:
        case CARD_REQ_WRITE_ROM:
        case CARD_REQ_READ_BACKUP:
        case CARD_REQ_WRITE_BACKUP:
        case CARD_REQ_PROGRAM_BACKUP:
        case CARD_REQ_VERIFY_BACKUP:
        case CARD_REQ_ERASE_PAGE_BACKUP:
        case CARD_REQ_ERASE_SECTOR_BACKUP:
        case CARD_REQ_ERASE_CHIP_BACKUP:
        case CARD_REQ_READ_STATUS:
        case CARD_REQ_WRITE_STATUS:
        case CARD_REQ_ERASE_SUBSECTOR_BACKUP:
            p->flag |= CARD_STAT_RECV;
            break;
        }

        if (!(p->flag & CARD_STAT_RECV)) {
            p->recv_step++;
        } else {
            p->recv_step = 0;
            OS_WakeupThreadDirect(((p->flag & CARD_STAT_BUSY) != 0) ? p->cur_th : p->thread);
        }
    }
}

void CARDi_TaskThread(void *arg)
{
    CARDiCommon *const p = &cardi_common;

    while (TRUE) {
        BOOL is_req = FALSE;
        OSIntrMode bak_psr = OS_DisableInterrupts();

        while (TRUE) {
            if ((p->flag & CARD_STAT_BUSY) == 0) {
                if ((p->flag & CARD_STAT_RECV) != 0) {
                    p->flag |= CARD_STAT_BUSY;
                    p->flag &= ~CARD_STAT_RECV;
                    is_req = TRUE;
                    break;
                }
            } else {
                if ((p->flag & CARD_STAT_TASK) != 0) {
                    break;
                }
            }
            p->cur_th = p->thread;
            OS_SleepThread(NULL);
        }
        OS_RestoreInterrupts(bak_psr);

        if (is_req) {
            p->cmd->result = CARD_RESULT_SUCCESS;

            if ((p->cmd->spec.caps & (1 << p->command)) == 0) {
                p->cmd->result = CARD_RESULT_UNSUPPORTED;
            } else {
                CARDiCommandArg *cmd = p->cmd;
                switch (p->command) {
                case CARD_REQ_INIT:
                case CARD_REQ_ACK:
                    break;

                case CARD_REQ_IDENTIFY:
                    CARDi_InitStatusRegister();
                    break;

                case CARD_REQ_READ_ID:
                    p->cmd->id = CARDi_ReadRomIDCore();
                    break;

                case CARD_REQ_READ_ROM:
#if defined(SDK_ARM7_READROM_SUPPORT)
                    CARDi_ReadRomCore((const void *)p->cmd->src, (void *)p->cmd->dst, p->cmd->len);
#else
                    p->cmd->result = CARD_RESULT_UNSUPPORTED;
#endif
                    break;

                case CARD_REQ_READ_BACKUP:
                    CARDi_ReadBackupCore(cmd->src, (void *)cmd->dst, cmd->len);
                    break;

                case CARD_REQ_WRITE_BACKUP:
                    CARDi_WriteBackupCore(cmd->dst, (const void *)cmd->src, cmd->len);
                    break;

                case CARD_REQ_PROGRAM_BACKUP:
                    CARDi_ProgramBackupCore(cmd->dst, (const void *)cmd->src, cmd->len);
                    break;

                case CARD_REQ_VERIFY_BACKUP:
                    CARDi_VerifyBackupCore(cmd->dst, (const void *)cmd->src, cmd->len);
                    break;

                case CARD_REQ_ERASE_SECTOR_BACKUP:
                    CARDi_EraseBackupSectorCore(cmd->dst, cmd->len);
                    break;

                case CARD_REQ_ERASE_SUBSECTOR_BACKUP:
                    CARDi_EraseBackupSubSectorCore(cmd->dst, cmd->len);
                    break;

                case CARD_REQ_ERASE_CHIP_BACKUP:
                    CARDi_EraseChipCore();
                    break;

                case CARD_REQ_READ_STATUS:
                    *(u8 *)p->cmd->dst = CARDi_CommandReadStatus();
                    break;

                case CARD_REQ_WRITE_STATUS:
                    CARDi_SetWriteProtectCore(*(u8 *)cmd->src);
                    break;

                default:
                case CARD_REQ_ERASE_PAGE_BACKUP:
                    cmd->result = CARD_RESULT_UNSUPPORTED;
                    break;
                }
            }

            CARDi_SendPxi(CARD_REQ_ACK);
            CARDi_EndTask(p, FALSE);
        } else {
            (*p->task_func)(p);
        }
    }
}
