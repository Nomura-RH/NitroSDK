#include "nitro/snd/common/seq.h"
#include "nitro/snd/common/bank.h"
#include "nitro/snd/common/exchannel.h"
#include "nitro/snd/common/mml.h"
#include "nitro/snd/common/util.h"
#include "nitro/snd/common/work.h"

#ifndef SDK_FROM_TOOL
#include "nitro/snd/common/main.h"
#include <nitro/misc.h>
#else
#define SDK_WARNING(exp, msg)              ((void)0)
#define SDK_ASSERT(exp)                    ((void)0)
#define SDK_NULL_ASSERT(exp)               ((void)0)
#define SDK_MINMAX_ASSERT(exp, min, max)   ((void)0)
#define SDK_ASSERTMSG(exp, msg)            ((void)0)
#define SND_StartIntervalTimer()           ((void)0)
#define SND_StopIntervalTimer()            ((void)0)
#define OS_TPrintf(format, a1, a2, a3, a4) ((void)0)
#endif

#define USE_CACHE

#define SND_SEQ_PLAYER_PAUSE_RELEASE 127
#define SND_SEQ_PLAYER_MUTE_RELEASE  127

#define SND_TRACK_INVALID_ENVELOPE 0xFF

#define SND_TRACK_DEFAULT_PRIO      64
#define SND_TRACK_DEFAULT_VOLUME    127
#define SND_TRACK_DEFAULT_BENDRANGE 2
#define SND_TRACK_DEFAULT_PANRANGE  127
#define SND_TRACK_DEFAULT_PORTA_KEY 60

#ifdef USE_CACHE
#define SND_SEQCACHE_BUFNUM 4
#endif

typedef enum {
    SEQ_ARG_U8,
    SEQ_ARG_S16,
    SEQ_ARG_VMIDI,
    SEQ_ARG_RANDOM,
    SEQ_ARG_VARIABLE
} SNDSeqArgType;

#ifdef USE_CACHE
typedef struct SNDSeqCache {
    const u32 *base;
    const u32 *endp;
    u32 buffer[SND_SEQCACHE_BUFNUM];
} SNDSeqCache;

static SNDSeqCache seqCache;
#endif
static BOOL sMmlPrintEnable = FALSE;

#ifdef USE_CACHE
static void InitCache(const void *ptr);
static u8 GetByteCache(const void *ptr);
#endif

static u8 ReadByte(SNDTrack *track_p);
static u16 Read16(SNDTrack *track_p);
static u32 Read24(SNDTrack *track_p);
static s32 ReadVar(SNDTrack *track_p);
static s32 ReadArg(SNDTrack *track_p, SNDPlayer *player_p, SNDSeqArgType argType);

static void InitPlayer(SNDPlayer *player_p, struct SNDBankData *bank_p);
static void InitTrack(SNDTrack *track_p);
static int PlayerSeqMain(SNDPlayer *player_p, BOOL doNoteOn);
static int TrackSeqMain(SNDTrack *track_p, SNDPlayer *player_p, int trackNo, BOOL doNoteOn);
static void PlayerTempoMain(SNDPlayer *player_p);
static void StartTrack(SNDTrack *track_p, const void *seqBase, u32 seqOffset);
static void ReleaseTrackChannelAll(SNDTrack *track_p, SNDPlayer *player_p, int release);
static void FreeTrackChannelAll(SNDTrack *track_p);
static struct SNDTrack *GetPlayerTrack(SNDPlayer *player_p, int trackNo);
static void CloseTrack(SNDTrack *track_p, SNDPlayer *player_p);
static void ClosePlayerTrack(SNDPlayer *player_p, int trackNo);
static void FinishPlayer(SNDPlayer *player_p);
static void ChannelCallback(SNDExChannel *drop_p, SNDExChannelCallbackStatus status, void *userData);
static void UpdateTrackChannel(SNDTrack *track_p, SNDPlayer *player_p, BOOL doRelease);
static void UpdatePlayerChannel(SNDPlayer *player_p);
static void NoteOnCommandProc(SNDTrack *track_p, SNDPlayer *player_p, int key, int velocity, s32 length);
static int CountChannel(SNDTrack *track_p);
static vs16 *GetVariablePtr(SNDPlayer *player_p, int varNo);
static void SetTrackMute(SNDTrack *track_p, SNDPlayer *player_p, SNDSeqMute mute);

static int AllocTrack(void);
static void FreeTrack(int trackID);
static struct SNDTrack *GetTrackPtr(int trackID);

#ifndef _MSC_VER
static inline
#else
__inline
#endif
u8 ReadByte(SNDTrack *track_p)
{
#ifdef USE_CACHE
    u8 ret = GetByteCache(track_p->cur);
#else
    u8 ret = *track_p->cur;
#endif
    track_p->cur++;
    return ret;
}

#ifndef _MSC_VER
static inline
#else
__inline
#endif
void FreeTrack(int trackID)
{
    SDK_MINMAX_ASSERT(trackID, SND_TRACK_MIN, SND_TRACK_MAX);

    SNDi_Work.track[trackID].active_flag = FALSE;
}

#ifndef _MSC_VER
static inline
#else
__inline
#endif
SNDTrack *GetTrackPtr(int trackID)
{
    SDK_MINMAX_ASSERT(trackID, SND_TRACK_MIN, SND_TRACK_MAX);

    return &SNDi_Work.track[trackID];
}

void SND_SeqInit(void)
{
    for (int playerNo = 0; playerNo < SND_PLAYER_NUM; playerNo++) {
        SNDPlayer *player_p = &SNDi_Work.player[playerNo];

        player_p->active_flag = FALSE;
        player_p->myNo = (u8)playerNo;
    }

    for (int trackNo = 0; trackNo < SND_TRACK_NUM; trackNo++) {
        SNDTrack *track_p = &SNDi_Work.track[trackNo];

        track_p->active_flag = FALSE;
    }
}

void SND_SeqMain(BOOL doPeriodicProc)
{
    SNDPlayer *player_p;
    int playerNo;
    u32 playerStatus = 0;

    for (playerNo = 0; playerNo < SND_PLAYER_NUM; playerNo++) {
        player_p = &SNDi_Work.player[playerNo];

        if (!player_p->active_flag) {
            continue;
        }

        if (player_p->prepared_flag) {
            if (doPeriodicProc && !player_p->pause_flag) {
                PlayerTempoMain(player_p);
            }
            UpdatePlayerChannel(player_p);
        }

        if (player_p->active_flag) {
            playerStatus |= (1 << playerNo);
        }
    }

    if (SNDi_SharedWork != NULL) {
        SNDi_SharedWork->playerStatus = playerStatus;
    }
}

void SND_PrepareSeq(int playerNo, const void *seqBase, u32 seqOffset, SNDBankData *bank_p)
{
    int trackNo;

    SDK_MINMAX_ASSERT(playerNo, SND_PLAYER_MIN, SND_PLAYER_MAX);

    SNDPlayer *player_p = &SNDi_Work.player[playerNo];

    if (player_p->active_flag) {
        FinishPlayer(player_p);
    }

    InitPlayer(player_p, bank_p);

    int trackID = AllocTrack();
    if (trackID < 0) {
        return;
    }
    SNDTrack *track_p = GetTrackPtr(trackID);

    InitTrack(track_p);
    StartTrack(track_p, seqBase, seqOffset);
    player_p->tracks[0] = trackID;

#ifdef USE_CACHE
    InitCache(track_p->cur);
#endif

    u8 command = ReadByte(track_p);
    if (command != SND_MML_ALLOC_TRACK) {
        track_p->cur--;
    } else {
        u16 trackBitMask = Read16(track_p);

        trackBitMask >>= 1;
        for (trackNo = 1; trackBitMask != 0; trackNo++, trackBitMask >>= 1) {
            if ((trackBitMask & 0x01) == 0) {
                continue;
            }

            trackID = AllocTrack();
            if (trackID < 0) {
                break;
            }
            track_p = GetTrackPtr(trackID);

            InitTrack(track_p);
            player_p->tracks[trackNo] = (u8)trackID;
        }
    }

    player_p->active_flag = TRUE;
    player_p->prepared_flag = FALSE;

    if (SNDi_SharedWork != NULL) {
        SNDi_SharedWork->playerStatus |= (1 << playerNo);
    }
}

void SND_StartPreparedSeq(int playerNo)
{
    SDK_MINMAX_ASSERT(playerNo, SND_PLAYER_MIN, SND_PLAYER_MAX);

    SNDPlayer *player_p = &SNDi_Work.player[playerNo];

    player_p->prepared_flag = TRUE;
}

void SND_StartSeq(int playerNo, const void *seqBase, u32 seqOffset, SNDBankData *bank_p)
{
    SND_PrepareSeq(playerNo, seqBase, seqOffset, bank_p);
    SND_StartPreparedSeq(playerNo);
}

void SND_StopSeq(int playerNo)
{
    SDK_MINMAX_ASSERT(playerNo, SND_PLAYER_MIN, SND_PLAYER_MAX);

    SNDPlayer *player_p = &SNDi_Work.player[playerNo];

    if (player_p->active_flag) {
        FinishPlayer(player_p);

        if (SNDi_SharedWork != NULL) {
            SNDi_SharedWork->playerStatus &= ~(1 << playerNo);
        }
    }
}

void SND_PauseSeq(int playerNo, BOOL flag)
{
    SDK_MINMAX_ASSERT(playerNo, SND_PLAYER_MIN, SND_PLAYER_MAX);

    SNDPlayer *player_p = &SNDi_Work.player[playerNo];

    player_p->pause_flag = flag;

    if (flag) {
        for (int trackNo = 0; trackNo < SND_TRACK_NUM_PER_PLAYER; trackNo++) {
            SNDTrack *track_p = GetPlayerTrack(player_p, trackNo);
            if (track_p == NULL) {
                continue;
            }

            ReleaseTrackChannelAll(track_p, player_p, SND_SEQ_PLAYER_PAUSE_RELEASE);
            FreeTrackChannelAll(track_p);
        }
    }
}

void SND_SkipSeq(int playerNo, u32 tick)
{
    SDK_MINMAX_ASSERT(playerNo, SND_PLAYER_MIN, SND_PLAYER_MAX);

    SNDPlayer *player_p = &SNDi_Work.player[playerNo];

    for (int trackNo = 0; trackNo < SND_TRACK_NUM_PER_PLAYER; trackNo++) {
        SNDTrack *track_p = GetPlayerTrack(player_p, trackNo);
        if (track_p == NULL) {
            continue;
        }

        ReleaseTrackChannelAll(track_p, player_p, SND_SEQ_PLAYER_PAUSE_RELEASE);
        FreeTrackChannelAll(track_p);
    }

    SND_StopIntervalTimer();

    u32 skipCount = 0;
    while (skipCount < tick) {
        if (PlayerSeqMain(player_p, FALSE) != 0) {
            FinishPlayer(player_p);
            break;
        }
        skipCount++;
    }

    SND_StartIntervalTimer();

    if (SNDi_SharedWork != NULL) {
        SNDi_SharedWork->player[player_p->myNo].tickCounter += skipCount;
    }
}

void SND_SetTrackMute(int playerNo, u32 trackBitMask, SNDSeqMute mute)
{
    SDK_MINMAX_ASSERT(playerNo, SND_PLAYER_MIN, SND_PLAYER_MAX);

    SNDPlayer *player_p = &SNDi_Work.player[playerNo];

    for (int trackNo = 0; trackNo < SND_TRACK_NUM_PER_PLAYER && trackBitMask != 0; trackNo++, trackBitMask >>= 1) {
        if ((trackBitMask & 1) == 0) {
            continue;
        }

        SNDTrack *track_p = GetPlayerTrack(player_p, trackNo);
        if (track_p == NULL) {
            continue;
        }

        SetTrackMute(track_p, player_p, mute);
    }
}

void SND_SetTrackAllocatableChannel(int playerNo, u32 trackBitMask, u32 chBitMask)
{
    SDK_MINMAX_ASSERT(playerNo, SND_PLAYER_MIN, SND_PLAYER_MAX);

    SNDPlayer *player_p = &SNDi_Work.player[playerNo];

    for (int trackNo = 0; trackNo < SND_TRACK_NUM_PER_PLAYER && trackBitMask != 0; trackNo++, trackBitMask >>= 1) {
        if ((trackBitMask & 1) == 0) {
            continue;
        }

        SNDTrack *track_p = GetPlayerTrack(player_p, trackNo);
        if (track_p == NULL) {
            continue;
        }

        track_p->channel_mask = chBitMask;
        track_p->channel_mask_flag = TRUE;
    }
}

void SND_InvalidateSeq(const void *start, const void *end)
{
    for (int playerNo = 0; playerNo < SND_PLAYER_NUM; playerNo++) {
        SNDPlayer *player_p = &SNDi_Work.player[playerNo];

        if (!player_p->active_flag) {
            continue;
        }

        for (int trackNo = 0; trackNo < SND_TRACK_NUM_PER_PLAYER; trackNo++) {
            SNDTrack *track_p = GetPlayerTrack(player_p, trackNo);
            if (track_p == NULL) {
                continue;
            }

            if (start <= (const void *)(track_p->cur) && (const void *)(track_p->cur) <= end) {
                FinishPlayer(player_p);
                break;
            }
        }
    }
}

void SND_InvalidateBank(const void *start, const void *end)
{
    for (int playerNo = 0; playerNo < SND_PLAYER_NUM; playerNo++) {
        SNDPlayer *player_p = &SNDi_Work.player[playerNo];

        if (!player_p->active_flag) {
            continue;
        }

        if (start <= (const void *)(player_p->bank) && (const void *)(player_p->bank) <= end) {
            FinishPlayer(player_p);
        }
    }
}

void SND_MmlPrintEnable(BOOL enable)
{
    sMmlPrintEnable = enable;
}

void SNDi_SetPlayerParam(int playerNo, u32 offset, u32 data, int size)
{
    SDK_MINMAX_ASSERT(playerNo, SND_PLAYER_MIN, SND_PLAYER_MAX);

    SNDPlayer *player_p = &SNDi_Work.player[playerNo];

    switch (size) {
    case 1:
        *(u8 *)((u8 *)player_p + offset) = (u8)data;
        break;
    case 2:
        *(u16 *)((u8 *)player_p + offset) = (u16)data;
        break;
    case 4:
        *(u32 *)((u8 *)player_p + offset) = (u32)data;
        break;
    }
}

void SNDi_SetTrackParam(int playerNo, u32 trackBitMask, u32 offset, u32 data, int size)
{
    SDK_MINMAX_ASSERT(playerNo, SND_PLAYER_MIN, SND_PLAYER_MAX);

    SNDPlayer *player_p = &SNDi_Work.player[playerNo];

    for (int trackNo = 0; trackNo < SND_TRACK_NUM_PER_PLAYER && trackBitMask != 0; trackNo++, trackBitMask >>= 1) {
        if ((trackBitMask & 1) == 0) {
            continue;
        }

        SNDTrack *track_p = GetPlayerTrack(player_p, trackNo);
        if (track_p == NULL) {
            continue;
        }

        switch (size) {
        case 1:
            *(u8 *)((u8 *)track_p + offset) = (u8)data;
            break;
        case 2:
            *(u16 *)((u8 *)track_p + offset) = (u16)data;
            break;
        case 4:
            *(u32 *)((u8 *)track_p + offset) = (u32)data;
            break;
        }
    }
}

static int CountChannel(SNDTrack *track_p)
{
    SNDExChannel *ch_p = track_p->channel_list;
    int count = 0;

    while (ch_p != NULL) {
        count++;
        ch_p = ch_p->nextLink;
    }

    return count;
}

#ifdef USE_CACHE
static void InitCache(const void *ptr)
{
#ifndef _MSC_VER
    seqCache.base = (const u32 *)((u32)ptr & 0xFFFFFFFC);
#else
    seqCache.base = (const u32 *)((u64)ptr & 0xFFFFFFFC);
#endif
    seqCache.endp = seqCache.base + SND_SEQCACHE_BUFNUM;

    seqCache.buffer[0] = seqCache.base[0];
    seqCache.buffer[1] = seqCache.base[1];
    seqCache.buffer[2] = seqCache.base[2];
    seqCache.buffer[3] = seqCache.base[3];
}

static u8 GetByteCache(const void *ptr)
{
    if (ptr < (const void *)(seqCache.base) || ptr >= (const void *)(seqCache.endp)) {
        InitCache(ptr);
    }

    return ((const u8 *)seqCache.buffer)[(const u8 *)ptr - (const u8 *)seqCache.base];
}
#endif

static u16 Read16(SNDTrack *track_p)
{
    u16 ret = ReadByte(track_p);
    ret |= ReadByte(track_p) << 8;
    return ret;
}

static u32 Read24(SNDTrack *track_p)
{
    u32 ret = ReadByte(track_p);
    ret |= ReadByte(track_p) << 8;
    ret |= ReadByte(track_p) << 16;
    return ret;
}

static s32 ReadVar(SNDTrack *track_p)
{
    s32 ret = 0;

    for (int i = 0;; i++) {
        u8 b = ReadByte(track_p);
        ret <<= 7;
        ret |= b & 0x7F;
        if (!(b & 0x80)) {
            break;
        }
    }

    return ret;
}

static s32 ReadArg(SNDTrack *track_p, SNDPlayer *player_p, SNDSeqArgType argType)
{
    s32 var;

    switch (argType) {
    case SEQ_ARG_U8:
        var = ReadByte(track_p);
        break;
    case SEQ_ARG_S16:
        var = Read16(track_p);
        break;
    case SEQ_ARG_VMIDI:
        var = ReadVar(track_p);
        break;
    case SEQ_ARG_VARIABLE: {
        u8 varNo = ReadByte(track_p);
        const vs16 *varPtr = GetVariablePtr(player_p, varNo);
        if (varPtr != NULL) {
            var = *varPtr;
        }
        break;
    }
    case SEQ_ARG_RANDOM: {
        s16 min = Read16(track_p);
        s16 max = Read16(track_p);

        s32 rand = SND_CalcRandom();
        rand *= (max - min) + 1;
        rand >>= 16;
        rand += min;
        var = rand;
        break;
    }
    }

    return var;
}

static void InitTrack(SNDTrack *track_p)
{
    track_p->base = NULL;
    track_p->cur = NULL;

    track_p->note_wait = TRUE;
    track_p->mute_flag = FALSE;
    track_p->tie_flag = FALSE;
    track_p->note_finish_wait = FALSE;
    track_p->porta_flag = FALSE;
    track_p->cmp_flag = TRUE;
    track_p->channel_mask_flag = FALSE;

    track_p->call_stack_depth = 0;

    track_p->prgNo = 0;
    track_p->prio = SND_TRACK_DEFAULT_PRIO;

    track_p->volume = SND_TRACK_DEFAULT_VOLUME;
    track_p->volume2 = SND_TRACK_DEFAULT_VOLUME;
    track_p->extFader = 0;
    track_p->pan = 0;
    track_p->ext_pan = 0;
    track_p->pitch_bend = 0;
    track_p->ext_pitch = 0;

    track_p->attack = SND_TRACK_INVALID_ENVELOPE;
    track_p->decay = SND_TRACK_INVALID_ENVELOPE;
    track_p->sustain = SND_TRACK_INVALID_ENVELOPE;
    track_p->release = SND_TRACK_INVALID_ENVELOPE;

    track_p->pan_range = SND_TRACK_DEFAULT_PANRANGE;
    track_p->bend_range = SND_TRACK_DEFAULT_BENDRANGE;
    track_p->porta_key = SND_TRACK_DEFAULT_PORTA_KEY;
    track_p->porta_time = 0;
    track_p->sweep_pitch = 0;
    track_p->transpose = 0;

    track_p->channel_mask = 0xFFFF;

    SND_InitLfoParam(&track_p->mod);

    track_p->wait = 0;

    track_p->channel_list = NULL;
}

static void StartTrack(SNDTrack *track_p, const void *seqBase, u32 seqOffset)
{
    track_p->base = (const u8 *)seqBase;
    track_p->cur = track_p->base + seqOffset;
}

static void InitPlayer(SNDPlayer *player_p, SNDBankData *bank_p)
{
    player_p->pause_flag = FALSE;

    player_p->bank = bank_p;

    player_p->tempo = SND_DEFAULT_TEMPO;
    player_p->tempo_ratio = 256;
    player_p->tempo_counter = SND_BASE_TEMPO;

    player_p->volume = 127;
    player_p->extFader = 0;

    player_p->prio = 64;

    for (int trackNo = 0; trackNo < SND_TRACK_NUM_PER_PLAYER; trackNo++) {
        player_p->tracks[trackNo] = SND_INVALID_TRACK_INDEX;
    }

    if (SNDi_SharedWork != NULL) {
        SNDi_SharedWork->player[player_p->myNo].tickCounter = 0;

        for (int varNo = 0; varNo < SND_PLAYER_VARIABLE_NUM; varNo++) {
            SNDi_SharedWork->player[player_p->myNo].variable[varNo] = SND_DEFAULT_VARIABLE;
        }
    }
}

static void ReleaseTrackChannelAll(SNDTrack *track_p, SNDPlayer *player_p, int release)
{
    UpdateTrackChannel(track_p, player_p, FALSE);

    SNDExChannel *ch_p = track_p->channel_list;

    while (ch_p != NULL) {
        if (SND_IsExChannelActive(ch_p)) {
            if (release >= 0) {
                SND_SetExChannelRelease(ch_p, (u8)release);
            }
            ch_p->prio = SND_EX_CHANNEL_RELEASE_PRIO;
            SND_ReleaseExChannel(ch_p);
        }
        ch_p = ch_p->nextLink;
    }
}

static void FreeTrackChannelAll(SNDTrack *track_p)
{
    SNDExChannel *ch_p = track_p->channel_list;

    while (ch_p != NULL) {
        SND_FreeExChannel(ch_p);
        ch_p = ch_p->nextLink;
    }

    track_p->channel_list = NULL;
}

static void PlayerTempoMain(SNDPlayer *player_p)
{
    int tick = 0;

    while (player_p->tempo_counter >= SND_BASE_TEMPO) {
        player_p->tempo_counter -= SND_BASE_TEMPO;
        tick++;
    }

    int tickCount = 0;
    while (tickCount < tick) {
        if (PlayerSeqMain(player_p, TRUE) != 0) {
            FinishPlayer(player_p);
            break;
        }
        tickCount++;
    }

    if (SNDi_SharedWork != NULL) {
        SNDi_SharedWork->player[player_p->myNo].tickCounter += tickCount;
    }

    int tempo = player_p->tempo;
    tempo *= player_p->tempo_ratio;
    tempo >>= 8;

    player_p->tempo_counter += tempo;
}

static SNDTrack *GetPlayerTrack(SNDPlayer *player_p, int trackNo)
{
    if (trackNo > SND_TRACK_NUM_PER_PLAYER - 1) {
        return NULL;
    }

    u8 trackID = player_p->tracks[trackNo];
    if (trackID == SND_INVALID_TRACK_INDEX) {
        return NULL;
    }

    return GetTrackPtr(trackID);
}

static void CloseTrack(SNDTrack *track_p, SNDPlayer *player_p)
{
    ReleaseTrackChannelAll(track_p, player_p, -1);
    FreeTrackChannelAll(track_p);
}

static void ClosePlayerTrack(SNDPlayer *player_p, int trackNo)
{
    SDK_MINMAX_ASSERT(trackNo, 0, SND_TRACK_NUM_PER_PLAYER - 1);

    SNDTrack *track_p = GetPlayerTrack(player_p, trackNo);
    if (track_p == NULL) {
        return;
    }

    CloseTrack(track_p, player_p);

    FreeTrack(player_p->tracks[trackNo]);
    player_p->tracks[trackNo] = SND_INVALID_TRACK_INDEX;
}

static void FinishPlayer(SNDPlayer *player_p)
{
    for (int trackNo = 0; trackNo < SND_TRACK_NUM_PER_PLAYER; trackNo++) {
        ClosePlayerTrack(player_p, trackNo);
    }

    player_p->active_flag = FALSE;
}

static void ChannelCallback(SNDExChannel *drop_p, SNDExChannelCallbackStatus status, void *userData)
{
    SNDTrack *track_p = (SNDTrack *)userData;

    SDK_NULL_ASSERT(drop_p);
    SDK_NULL_ASSERT(track_p);

    if (status == SND_EX_CHANNEL_CALLBACK_FINISH) {
        drop_p->prio = SND_EX_CHANNEL_STOP_PRIO;
        SND_FreeExChannel(drop_p);
    }

    if (track_p->channel_list == drop_p) {
        track_p->channel_list = drop_p->nextLink;
        return;
    }

    SNDExChannel *ch_p = track_p->channel_list;

    while (ch_p->nextLink != NULL) {
        if (ch_p->nextLink == drop_p) {
            ch_p->nextLink = drop_p->nextLink;
            return;
        }
        ch_p = ch_p->nextLink;
    }
}

static void UpdateTrackChannel(SNDTrack *track_p, SNDPlayer *player_p, BOOL doRelease)
{
    SNDExChannel *ch_p;
    s32 decay = SND_CalcDecibelSquare(track_p->volume) + SND_CalcDecibelSquare(track_p->volume2) + SND_CalcDecibelSquare(player_p->volume);
    s32 decay2 = track_p->extFader + player_p->extFader;

    s32 pitch = track_p->pitch_bend;
    pitch *= track_p->bend_range << SND_PITCH_DIVISION_BIT;
    pitch >>= 7;
    pitch += track_p->ext_pitch;

    s32 pan = track_p->pan;
    if (track_p->pan_range != 127) {
        pan *= track_p->pan_range;
        pan += 64;
        pan >>= 7;
    }
    pan += track_p->ext_pan;

    if (decay < -32768) {
        decay = -32768;
    }
    if (decay2 < -32768) {
        decay2 = -32768;
    }

    if (pan < SND_EX_CHANNEL_USER_PAN_MIN) {
        pan = SND_EX_CHANNEL_USER_PAN_MIN;
    } else if (pan > SND_EX_CHANNEL_USER_PAN_MAX) {
        pan = SND_EX_CHANNEL_USER_PAN_MAX;
    }

    ch_p = track_p->channel_list;
    while (ch_p != NULL) {
        ch_p->user_decay2 = decay2;

        if (ch_p->env_status != SND_ENV_RELEASE) {
            ch_p->user_decay = decay;
            ch_p->user_pitch = pitch;
            ch_p->user_pan = pan;
            ch_p->pan_range = track_p->pan_range;

            ch_p->lfo.param = track_p->mod;

            if (ch_p->length == 0 && doRelease) {
                ch_p->prio = SND_EX_CHANNEL_RELEASE_PRIO;
                SND_ReleaseExChannel(ch_p);
            }
        }

        ch_p = ch_p->nextLink;
    }
}

static void UpdatePlayerChannel(SNDPlayer *player_p)
{
    for (int trackNo = 0; trackNo < SND_TRACK_NUM_PER_PLAYER; trackNo++) {
        SNDTrack *track_p = GetPlayerTrack(player_p, trackNo);
        if (track_p == NULL) {
            continue;
        }

        UpdateTrackChannel(track_p, player_p, TRUE);
    }
}

static void NoteOnCommandProc(SNDTrack *track_p, SNDPlayer *player_p, int key, int velocity, s32 length)
{
    SDK_MINMAX_ASSERT(key, 0, 127);
    SDK_MINMAX_ASSERT(velocity, 0, 127);

    SNDExChannel *ch_p = NULL;
    SNDInstData inst;
    u32 allocChannelMask;

    if (track_p->tie_flag) {
        ch_p = track_p->channel_list;
        if (ch_p != NULL) {
            ch_p->key = key;
            ch_p->velocity = velocity;
        }
    }

    if (ch_p == NULL) {
        if (!SND_ReadInstData(player_p->bank, track_p->prgNo, key, &inst)) {
            return;
        }

        switch (inst.type) {
        case SND_INST_PCM:
        case SND_INST_DIRECTPCM:
            allocChannelMask = SND_PCM_CHANNEL_MASK;
            break;

        case SND_INST_PSG:
            allocChannelMask = SND_PSG_CHANNEL_MASK;
            break;

        case SND_INST_NOISE:
            allocChannelMask = SND_NOISE_CHANNEL_MASK;
            break;

        default:
            return;
        }

        allocChannelMask &= track_p->channel_mask;

        ch_p = SND_AllocExChannel(allocChannelMask, player_p->prio + track_p->prio, track_p->channel_mask_flag, ChannelCallback, track_p);
        if (ch_p == NULL) {
            return;
        }

        if (!SND_NoteOn(ch_p, key, velocity, track_p->tie_flag ? -1 : length, player_p->bank, &inst)) {
            ch_p->prio = SND_EX_CHANNEL_STOP_PRIO;
            SND_FreeExChannel(ch_p);
            return;
        }

        ch_p->nextLink = track_p->channel_list;
        track_p->channel_list = ch_p;
    }

    if (track_p->attack != SND_TRACK_INVALID_ENVELOPE) {
        SND_SetExChannelAttack(ch_p, track_p->attack);
    }
    if (track_p->decay != SND_TRACK_INVALID_ENVELOPE) {
        SND_SetExChannelDecay(ch_p, track_p->decay);
    }
    if (track_p->sustain != SND_TRACK_INVALID_ENVELOPE) {
        SND_SetExChannelSustain(ch_p, track_p->sustain);
    }
    if (track_p->release != SND_TRACK_INVALID_ENVELOPE) {
        SND_SetExChannelRelease(ch_p, track_p->release);
    }

    ch_p->sweep_pitch = track_p->sweep_pitch;

    if (track_p->porta_flag) {
        ch_p->sweep_pitch += (s16)((track_p->porta_key - key) << SND_PITCH_DIVISION_BIT);
    }
    if (track_p->porta_time == 0) {
        ch_p->sweep_length = length;
        ch_p->auto_sweep = FALSE;
    } else {
        int length = track_p->porta_time;
        length *= length;
        length *= ch_p->sweep_pitch >= 0 ? ch_p->sweep_pitch : -ch_p->sweep_pitch;
        length >>= 5 + SND_PITCH_DIVISION_BIT;
        ch_p->sweep_length = length;
    }
    ch_p->sweep_counter = 0;
}

static int TrackSeqMain(SNDTrack *track_p, SNDPlayer *player_p, int trackNo, BOOL doNoteOn)
{
    SDK_NULL_ASSERT(track_p);
    SDK_NULL_ASSERT(player_p);

    SNDExChannel *ch_p = track_p->channel_list;
    while (ch_p != NULL) {
        if (ch_p->length > 0) {
            ch_p->length--;
        }
        if (!ch_p->auto_sweep && ch_p->sweep_counter < ch_p->sweep_length) {
            ch_p->sweep_counter++;
        }
        ch_p = ch_p->nextLink;
    }

    if (track_p->note_finish_wait) {
        if (track_p->channel_list != NULL) {
            return 0;
        }
        track_p->note_finish_wait = FALSE;
    }
    if (track_p->wait > 0) {
        track_p->wait--;
        if (track_p->wait > 0) {
            return 0;
        }
    }

#ifdef USE_CACHE
    InitCache(track_p->cur);
#endif

    while (track_p->wait == 0 && !track_p->note_finish_wait) {
        SNDSeqArgType argType;
        BOOL useArgType = FALSE;
        BOOL doExecCommand = TRUE;

        u8 cmd = ReadByte(track_p);

        if (cmd == SND_MML_IF) {
            cmd = ReadByte(track_p);
            doExecCommand = track_p->cmp_flag;
        }
        if (cmd == SND_MML_RANDOM) {
            cmd = ReadByte(track_p);
            argType = SEQ_ARG_RANDOM;
            useArgType = TRUE;
        }
        if (cmd == SND_MML_VARIABLE) {
            cmd = ReadByte(track_p);
            argType = SEQ_ARG_VARIABLE;
            useArgType = TRUE;
        }

        if ((cmd & 0x80) == 0) {
            const u8 velocity = ReadByte(track_p);
            const s32 length = ReadArg(track_p, player_p, useArgType ? argType : SEQ_ARG_VMIDI);
            int key = cmd + track_p->transpose;

            if (!doExecCommand) {
                continue;
            }

            if (key < 0) {
                key = 0;
            } else if (key > 127) {
                key = 127;
            }

            if (!track_p->mute_flag && doNoteOn) {
                NoteOnCommandProc(track_p, player_p, key, velocity, length > 0 ? length : -1);
            }

            track_p->porta_key = key;

            if (track_p->note_wait) {
                track_p->wait = length;
                if (length == 0) {
                    track_p->note_finish_wait = TRUE;
                }
            }
        } else {
            switch (cmd & 0xF0) {
            case 0x80: {
                s32 arg = ReadArg(track_p, player_p, useArgType ? argType : SEQ_ARG_VMIDI);

                if (!doExecCommand) {
                    break;
                }

                switch (cmd) {
                case SND_MML_WAIT:
                    track_p->wait = arg;
                    break;

                case SND_MML_PRG:
                    if (arg < 0x10000) {
                        track_p->prgNo = arg;
                    }
                    break;
                }
                break;
            }

            case 0x90: {
                switch (cmd) {
                case SND_MML_OPEN_TRACK: {
                    u8 trackNo = ReadByte(track_p);
                    u32 offset = Read24(track_p);
                    SNDTrack *new_track_p;

                    if (!doExecCommand) {
                        break;
                    }

                    new_track_p = GetPlayerTrack(player_p, trackNo);
                    if (new_track_p == NULL) {
                        SDK_WARNING(FALSE, "SND: opentrack for not allocated track");
                        break;
                    }
                    if (new_track_p == track_p) {
                        SDK_WARNING(FALSE, "SND: opentrack for self track");
                        break;
                    }

                    CloseTrack(new_track_p, player_p);

                    StartTrack(new_track_p, track_p->base, offset);
                    break;
                }

                case SND_MML_JUMP: {
                    u32 offset = Read24(track_p);

                    if (!doExecCommand) {
                        break;
                    }

                    track_p->cur = track_p->base + offset;
                    break;
                }

                case SND_MML_CALL: {
                    u32 offset = Read24(track_p);

                    if (!doExecCommand) {
                        break;
                    }

                    if (track_p->call_stack_depth >= SND_TRACK_CALL_STACK_DEPTH) {
                        SDK_WARNING(FALSE, "SND: cannot 'call' because already too deep");
                        break;
                    }

                    track_p->call_stack[track_p->call_stack_depth] = track_p->cur;
                    track_p->call_stack_depth++;
                    track_p->cur = track_p->base + offset;
                    break;
                }
                }
                break;
            }

            case 0xC0:
            case 0xD0: {
                u8 arg = ReadArg(track_p, player_p, useArgType ? argType : SEQ_ARG_U8);

                if (!doExecCommand) {
                    break;
                }

                switch (cmd) {
                case SND_MML_VOLUME:
                    track_p->volume = arg;
                    break;

                case SND_MML_VOLUME2:
                    track_p->volume2 = arg;
                    break;

                case SND_MML_MAIN_VOLUME:
                    player_p->volume = arg;
                    break;

                case SND_MML_BEND_RANGE:
                    track_p->bend_range = arg;
                    break;

                case SND_MML_PRIO:
                    track_p->prio = arg;
                    break;

                case SND_MML_NOTE_WAIT:
                    track_p->note_wait = arg;
                    break;

                case SND_MML_PORTA_TIME:
                    track_p->porta_time = arg;
                    break;

                case SND_MML_MOD_DEPTH:
                    track_p->mod.depth = arg;
                    break;

                case SND_MML_MOD_SPEED:
                    track_p->mod.speed = arg;
                    break;

                case SND_MML_MOD_TYPE:
                    track_p->mod.target = arg;
                    break;

                case SND_MML_MOD_RANGE:
                    track_p->mod.range = arg;
                    break;

                case SND_MML_ATTACK:
                    track_p->attack = arg;
                    break;

                case SND_MML_DECAY:
                    track_p->decay = arg;
                    break;

                case SND_MML_SUSTAIN:
                    track_p->sustain = arg;
                    break;

                case SND_MML_RELEASE:
                    track_p->release = arg;
                    break;

                case SND_MML_LOOP_START:
                    if (track_p->call_stack_depth >= SND_TRACK_CALL_STACK_DEPTH) {
                        SDK_WARNING(FALSE, "SND: cannot 'loop_start' because already too deep");
                        break;
                    }

                    track_p->call_stack[track_p->call_stack_depth] = track_p->cur;
                    track_p->loop_count[track_p->call_stack_depth] = arg;
                    track_p->call_stack_depth++;
                    break;

                case SND_MML_TIE:
                    track_p->tie_flag = arg;
                    ReleaseTrackChannelAll(track_p, player_p, -1);
                    FreeTrackChannelAll(track_p);
                    break;

                case SND_MML_MUTE:
                    SetTrackMute(track_p, player_p, arg);
                    break;

                case SND_MML_PORTA:
                    track_p->porta_key = arg + track_p->transpose;
                    track_p->porta_flag = TRUE;
                    break;

                case SND_MML_PORTA_SW:
                    track_p->porta_flag = arg;
                    break;

                case SND_MML_TRANSPOSE:
                    track_p->transpose = *(s8 *)&arg;
                    break;

                case SND_MML_PITCH_BEND:
                    track_p->pitch_bend = *(s8 *)&arg;
                    break;

                case SND_MML_PAN:
                    track_p->pan = arg - SND_CHANNEL_PAN_CENTER;
                    break;

                case SND_MML_PRINTVAR:
                    if (sMmlPrintEnable) {
                        const vs16 *const varPtr = GetVariablePtr(player_p, arg);
                        OS_TPrintf("#%d[%d]: printvar No.%d = %d\n", player_p->myNo, trackNo, arg, *varPtr);
                    }
                    break;
                }
                break;
            }

            case 0xE0: {
                s16 arg = ReadArg(track_p, player_p, useArgType ? argType : SEQ_ARG_S16);

                if (!doExecCommand) {
                    break;
                }

                switch (cmd) {
                case SND_MML_TEMPO:
                    player_p->tempo = arg;
                    break;

                case SND_MML_MOD_DELAY:
                    track_p->mod.delay = arg;
                    break;

                case SND_MML_SWEEP_PITCH:
                    track_p->sweep_pitch = arg;
                    break;
                }
                break;
            }

            case 0xB0: {
                u8 varNo = ReadByte(track_p);
                s16 arg = ReadArg(track_p, player_p, useArgType ? argType : SEQ_ARG_S16);
                vs16 *varPtr = GetVariablePtr(player_p, varNo);

                if (!doExecCommand) {
                    break;
                }

                if (varPtr != NULL) {
                    switch (cmd) {
                    case SND_MML_SETVAR:
                        *varPtr = arg;
                        break;

                    case SND_MML_ADDVAR:
                        *varPtr += arg;
                        break;

                    case SND_MML_SUBVAR:
                        *varPtr -= arg;
                        break;

                    case SND_MML_MULVAR:
                        *varPtr *= arg;
                        break;

                    case SND_MML_DIVVAR:
                        if (arg != 0) {
                            *varPtr /= arg;
                        }
                        break;

                    case SND_MML_SHIFTVAR:
                        if (arg >= 0) {
                            *varPtr <<= arg;
                        } else {
                            *varPtr >>= -arg;
                        }
                        break;

                    case SND_MML_RANDVAR: {
                        BOOL minus_flag = FALSE;

                        if (arg < 0) {
                            minus_flag = TRUE;
                            arg = -arg;
                        }

                        s32 rand = SND_CalcRandom();
                        rand *= arg + 1;
                        rand >>= 16;
                        if (minus_flag) {
                            rand = -rand;
                        }
                        *varPtr = rand;
                        break;
                    }

                    case SND_MML_CMP_EQ:
                        track_p->cmp_flag = (*varPtr == arg);
                        break;

                    case SND_MML_CMP_GE:
                        track_p->cmp_flag = (*varPtr >= arg);
                        break;

                    case SND_MML_CMP_GT:
                        track_p->cmp_flag = (*varPtr > arg);
                        break;

                    case SND_MML_CMP_LE:
                        track_p->cmp_flag = (*varPtr <= arg);
                        break;

                    case SND_MML_CMP_LT:
                        track_p->cmp_flag = (*varPtr < arg);
                        break;

                    case SND_MML_CMP_NE:
                        track_p->cmp_flag = (*varPtr != arg);
                        break;
                    }
                }

                break;
            }

            case 0xF0:
                if (!doExecCommand) {
                    break;
                }

                switch (cmd) {
                case SND_MML_RET:
                    if (track_p->call_stack_depth == 0) {
                        SDK_WARNING(FALSE, "SND: unmatched sequence command 'ret'");
                        break;
                    }

                    track_p->call_stack_depth--;
                    track_p->cur = track_p->call_stack[track_p->call_stack_depth];
                    break;

                case SND_MML_LOOP_END:
                    if (track_p->call_stack_depth == 0) {
                        SDK_WARNING(FALSE, "SND: unmatched sequence command 'loop_end'");
                        break;
                    }

                    u8 loop_count = track_p->loop_count[track_p->call_stack_depth - 1];
                    if (loop_count > 0) {
                        loop_count--;
                        if (loop_count == 0) {
                            track_p->call_stack_depth--;
                            break;
                        }
                    }

                    track_p->loop_count[track_p->call_stack_depth - 1] = loop_count;
                    track_p->cur = track_p->call_stack[track_p->call_stack_depth - 1];
                    break;

                case SND_MML_ALLOC_TRACK:
                    break;

                case SND_MML_FIN:
                    return -1;
                }
                break;
            }
        }
    }

    return 0;
}

static int PlayerSeqMain(SNDPlayer *player_p, BOOL doNoteOn)
{
    BOOL active_flag = FALSE;

    for (int trackNo = 0; trackNo < SND_TRACK_NUM_PER_PLAYER; trackNo++) {
        SNDTrack *track_p = GetPlayerTrack(player_p, trackNo);
        if (track_p == NULL) {
            continue;
        }
        if (track_p->cur == NULL) {
            continue;
        }

        if (TrackSeqMain(track_p, player_p, trackNo, doNoteOn) == 0) {
            active_flag = TRUE;
        } else {
            ClosePlayerTrack(player_p, trackNo);
        }
    }

    if (!active_flag) {
        return 1;
    }

    return 0;
}

static vs16 *GetVariablePtr(SNDPlayer *player_p, int varNo)
{
    SDK_MINMAX_ASSERT(varNo, 0, SND_PLAYER_VARIABLE_NUM + SND_GLOBAL_VARIABLE_NUM);
    SDK_WARNING(SNDi_SharedWork != NULL, "SND: Cannot use sequence variable because shared work don't set");

    if (SNDi_SharedWork == NULL) {
        return NULL;
    }

    if (varNo < SND_PLAYER_VARIABLE_NUM) {
        return &SNDi_SharedWork->player[player_p->myNo].variable[varNo];
    } else {
        return &SNDi_SharedWork->globalVariable[varNo - SND_PLAYER_VARIABLE_NUM];
    }

    return NULL;
}

static int AllocTrack(void)
{
    for (int trackID = 0; trackID < SND_TRACK_NUM; trackID++) {
        SNDTrack *track_p = &SNDi_Work.track[trackID];

        if (!track_p->active_flag) {
            track_p->active_flag = TRUE;
            return trackID;
        }
    }

    return -1;
}

static void SetTrackMute(SNDTrack *track_p, SNDPlayer *player_p, SNDSeqMute mute)
{
    SDK_NULL_ASSERT(track_p);

    switch (mute) {
    case SND_SEQ_MUTE_OFF:
        track_p->mute_flag = FALSE;
        break;
    case SND_SEQ_MUTE_NO_STOP:
        track_p->mute_flag = TRUE;
        break;
    case SND_SEQ_MUTE_RELEASE:
        track_p->mute_flag = TRUE;
        ReleaseTrackChannelAll(track_p, player_p, -1);
        break;
    case SND_SEQ_MUTE_STOP:
        track_p->mute_flag = TRUE;
        ReleaseTrackChannelAll(track_p, player_p, SND_SEQ_PLAYER_MUTE_RELEASE);
        FreeTrackChannelAll(track_p);
        break;
    default:
        break;
    }
}
