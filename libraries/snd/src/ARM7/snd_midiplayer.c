#include <nitro/snd/common/midiplayer.h>

#ifndef SDK_FROM_TOOL
#include <nitro/misc.h>
#else
#define SDK_ASSERT(exp)                  ((void)0)
#define SDK_NULL_ASSERT(exp)             ((void)0)
#define SDK_MINMAX_ASSERT(exp, min, max) ((void)0)
#endif

#include <nitro/snd/common/bank.h>
#include <nitro/snd/common/util.h>

#define MID_TRACK_INVALID_ENVELOPE 0xFF

static void InitTrack(SNDMidiTrack *track);
static void InitTrackController(SNDMidiTrack *track);
static void ResetTrack(SNDMidiTrack *track);
static void ChannelCallback(SNDExChannel *drop_p, SNDExChannelCallbackStatus status, void *userData);
static void UpdateTrackChannel(SNDExChannel *chp, SNDMidiTrack *track, SNDMidiPlayer *player);

static void MsgNoteOn(SNDMidiPlayer *player, u8 ch, u8 key, u8 velocity);
static void MsgNoteOff(SNDMidiPlayer *player, u8 ch, u8 key, u8 velocity);
static void MsgProgramChange(SNDMidiPlayer *player, u8 ch, u16 prgNo);
static void MsgControlChange(SNDMidiPlayer *player, u8 ch, u8 ctrl, u8 value);
static void MsgPitchBendChange(SNDMidiPlayer *player, u8 ch, u8 data1, u8 data2);
static void MsgChannelMode(SNDMidiPlayer *player, u8 ctrl, u8 value);
static void MsgReset(SNDMidiPlayer *player);
static void MsgAllSoundOff(SNDMidiPlayer *player);
static void MsgAllNoteOff(SNDMidiPlayer *player);
static void MsgResetAllController(SNDMidiPlayer *player);

void SND_MidiPlayerInit(SNDMidiPlayer *player)
{
    int trackNo;

    SDK_NULL_ASSERT(player);

    for (trackNo = 0; trackNo < SND_MIDI_TRACK_NUM; trackNo++) {
        InitTrack(&player->track[trackNo]);
    }

    player->bank = NULL;
    player->main_volume = 127;
    player->prio = 64;
}

void SND_MidiPlayerSetBank(SNDMidiPlayer *player, const struct SNDBankData *bank)
{
    SDK_NULL_ASSERT(player);

    player->bank = bank;
}

void SND_MidiPlayerMain(SNDMidiPlayer *player)
{
    SNDMidiTrack *track;
    SNDExChannel *chp;
    int trackNo;
    int i;

    SDK_NULL_ASSERT(player);

    for (trackNo = 0; trackNo < SND_MIDI_TRACK_NUM; trackNo++) {
        track = &player->track[trackNo];

        for (i = 0; i < SND_CHANNEL_NUM; i++) {
            chp = track->channels[i].chp;
            if (chp != NULL) {
                UpdateTrackChannel(chp, track, player);
            }
        }
    }
}

void SND_MidiPlayerReset(SNDMidiPlayer *player)
{
    SNDMidiTrack *track;
    SNDExChannel *chp;
    int trackNo;
    int i;

    SDK_NULL_ASSERT(player);

    for (trackNo = 0; trackNo < SND_MIDI_TRACK_NUM; trackNo++) {
        track = &player->track[trackNo];

        for (i = 0; i < SND_CHANNEL_NUM; i++) {
            chp = track->channels[i].chp;

            if (chp != NULL) {
                UpdateTrackChannel(chp, track, player);

                if (SND_IsExChannelActive(chp)) {
                    chp->prio = SND_EX_CHANNEL_RELEASE_PRIO;
                    SND_ReleaseExChannel(chp);
                }
                SND_FreeExChannel(chp);
                track->channels[i].chp = NULL;
            }
        }

        InitTrack(&player->track[trackNo]);
    }
}

void SND_MidiPlayerProgramChange(SNDMidiPlayer *player, int channel, int prgNo)
{
    SDK_MINMAX_ASSERT(channel, 0, 0xFF);
    SDK_MINMAX_ASSERT(prgNo, 0, 0xFFFF);

    MsgProgramChange(player, channel, prgNo);
}

void SND_MidiPlayerSendMessage(SNDMidiPlayer *player, u8 status, u8 data1, u8 data2)
{
    u8 ch = status & 0xF;

    SDK_NULL_ASSERT(player);

    switch (status & 0xF0) {
    case 0x80:
        MsgNoteOff(player, ch, data1, data2);
        break;
    case 0x90:
        if (data2 != 0) {
            MsgNoteOn(player, ch, data1, data2);
        } else {
            MsgNoteOff(player, ch, data1, data2);
        }
        break;
    case 0xA0:
        break;
    case 0xB0:
        if (data1 < 120) {
            MsgControlChange(player, ch, data1, data2);
        } else {
            MsgChannelMode(player, data1, data2);
        }
        break;
    case 0xC0:
        MsgProgramChange(player, ch, data1);
        break;
    case 0xD0:
        break;
    case 0xE0:
        MsgPitchBendChange(player, ch, data1, data2);
        break;
    case 0xF0:
        switch (status) {
        case 0xFE:
            break;
        case 0xFF:
            MsgReset(player);
            break;
        }
        break;
    }
}

static void InitTrack(SNDMidiTrack *track)
{
    int i;

    for (i = 0; i < SND_CHANNEL_NUM; i++) {
        track->channels[i].chp = NULL;
    }

    InitTrackController(track);
}

static void InitTrackController(SNDMidiTrack *track)
{
    SND_InitLfoParam(&track->mod);

    track->prgNo = 0;
    track->pitchbend = 0;
    track->porta_time = 0;
    track->volume = 127;
    track->pan = 0;
    track->expression = 127;
    track->transpose = 0;
    track->prio = 64;
    track->bendrange = 2;
    track->sweep_pitch = 0;
    track->porta_flag = FALSE;
    track->porta_key = 60;
    track->attack = MID_TRACK_INVALID_ENVELOPE;
    track->decay = MID_TRACK_INVALID_ENVELOPE;
    track->sustain = MID_TRACK_INVALID_ENVELOPE;
    track->release = MID_TRACK_INVALID_ENVELOPE;
}

static void ResetTrack(SNDMidiTrack *track)
{
    SNDExChannel *chp;
    int i;

    for (i = 0; i < SND_CHANNEL_NUM; i++) {
        chp = track->channels[i].chp;

        if (chp == NULL) {
            continue;
        }

        if (SND_IsExChannelActive(chp)) {
            chp->prio = SND_EX_CHANNEL_RELEASE_PRIO;
            SND_SetExChannelRelease(chp, 127);
            SND_ReleaseExChannel(chp);
        }
        SND_FreeExChannel(chp);
    }

    InitTrack(track);
}

static void UpdateTrackChannel(SNDExChannel *chp, SNDMidiTrack *track, SNDMidiPlayer *player)
{
    s32 decay = SND_CalcDecibelSquare(track->volume) + SND_CalcDecibelSquare(track->expression) + SND_CalcDecibelSquare(player->main_volume);
    s32 pan = track->pan;
    s32 pitch;

    pitch = track->pitchbend;
    pitch *= track->bendrange << SND_PITCH_DIVISION_BIT;
    pitch >>= 7;

    if (pan < SND_EX_CHANNEL_USER_PAN_MIN) {
        pan = SND_EX_CHANNEL_USER_PAN_MIN;
    } else if (pan > SND_EX_CHANNEL_USER_PAN_MAX) {
        pan = SND_EX_CHANNEL_USER_PAN_MAX;
    }

    chp->user_decay = decay;
    chp->user_pitch = pitch;
    chp->user_pan = pan;

    chp->lfo.param = track->mod;
}

static void ChannelCallback(SNDExChannel *drop_p, SNDExChannelCallbackStatus status, void *userData)
{
    SNDMidiTrack *track = (SNDMidiTrack *)userData;
    int i;

    if (status == SND_EX_CHANNEL_CALLBACK_FINISH) {
        drop_p->prio = SND_EX_CHANNEL_STOP_PRIO;
        SND_FreeExChannel(drop_p);
    }

    for (i = 0; i < SND_CHANNEL_NUM; i++) {
        if (track->channels[i].chp == drop_p) {
            track->channels[i].chp = NULL;
            break;
        }
    }
}

static void MsgNoteOn(SNDMidiPlayer *player, u8 ch, u8 key8, u8 velocity)
{
    SNDMidiTrack *track;
    SNDExChannel *chp;
    SNDInstData inst;
    u32 allocChannelMask;
    int key;
    int i;

    if (player->bank == NULL) {
        return;
    }

    track = &player->track[ch];

    key = key8 + track->transpose;
    if (key < 0) {
        key = 0;
    } else if (key > 127) {
        key = 127;
    }

    if (!SND_ReadInstData(player->bank, track->prgNo, key, &inst)) {
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

    chp = SND_AllocExChannel(allocChannelMask, player->prio + track->prio, FALSE, ChannelCallback, track);
    if (chp == NULL) {
        return;
    }

    if (!SND_NoteOn(chp, key, velocity, -1, player->bank, &inst)) {
        chp->prio = SND_EX_CHANNEL_STOP_PRIO;
        SND_FreeExChannel(chp);
        return;
    }

    for (i = 0; i < SND_CHANNEL_NUM; i++) {
        if (track->channels[i].chp == NULL) {
            track->channels[i].chp = chp;
            track->channels[i].key = key8;
            break;
        }
    }

    if (track->attack != MID_TRACK_INVALID_ENVELOPE) {
        SND_SetExChannelAttack(chp, track->attack);
    }
    if (track->decay != MID_TRACK_INVALID_ENVELOPE) {
        SND_SetExChannelDecay(chp, track->decay);
    }
    if (track->sustain != MID_TRACK_INVALID_ENVELOPE) {
        SND_SetExChannelSustain(chp, track->sustain);
    }
    if (track->release != MID_TRACK_INVALID_ENVELOPE) {
        SND_SetExChannelRelease(chp, track->release);
    }

    chp->sweep_pitch = track->sweep_pitch;

    if (track->porta_flag) {
        chp->sweep_pitch += (s16)((track->porta_key - key) << SND_PITCH_DIVISION_BIT);
    }
    if (track->porta_time == 0) {
        chp->sweep_length = 64;
    } else {
        int length = track->porta_time;
        length *= length;
        length *= chp->sweep_pitch >= 0 ? chp->sweep_pitch : -chp->sweep_pitch;
        length >>= 5 + SND_PITCH_DIVISION_BIT;
        chp->sweep_length = length;
    }
    chp->sweep_counter = 0;

    track->porta_key = key;
}

static void MsgNoteOff(SNDMidiPlayer *player, u8 ch, u8 key, u8 velocity)
{
    SNDMidiTrack *track;
    SNDExChannel *chp;
    int i;

    track = &player->track[ch];

    for (i = 0; i < SND_CHANNEL_NUM; i++) {
        chp = track->channels[i].chp;

        if (chp != NULL && track->channels[i].key == key) {
            UpdateTrackChannel(chp, track, player);

            if (SND_IsExChannelActive(chp)) {
                chp->prio = SND_EX_CHANNEL_RELEASE_PRIO;
                SND_ReleaseExChannel(chp);
            }
        }
    }
}

static void MsgProgramChange(SNDMidiPlayer *player, u8 ch, u16 prgNo)
{
    SNDMidiTrack *track = &player->track[ch];

    track->prgNo = prgNo;
}

static void MsgPitchBendChange(SNDMidiPlayer *player, u8 ch, u8 data1, u8 data2)
{
    SNDMidiTrack *track = &player->track[ch];
    s32 bend;

    bend = ((data2 << 7) | data1) - 0x2000;
    bend >>= 6;

    track->pitchbend = bend;
}

static void MsgControlChange(SNDMidiPlayer *player, u8 ch, u8 ctrl, u8 value)
{
    SNDMidiTrack *track = &player->track[ch];

    switch (ctrl) {
    case 1:
        track->mod.depth = value;
        break;
    case 5:
        track->porta_time = value;
        break;
    case 7:
        track->volume = value;
        break;
    case 10:
        track->pan = value - 64;
        break;
    case 11:
        track->expression = value;
        break;
    case 12:
        player->main_volume = value;
        break;
    case 13:
        track->transpose = value - 64;
        break;
    case 14:
        track->prio = value;
        break;
    case 16:
    case 17:
    case 18:
    case 19:
        break;
    case 20:
        track->bendrange = value;
        break;
    case 21:
        track->mod.speed = value;
        break;
    case 22:
        track->mod.target = value;
        break;
    case 23:
        track->mod.range = value;
        break;
    case 26:
        track->mod.delay = value;
        break;
    case 27:
        track->mod.delay = value * 10;
        break;
    case 28:
        track->sweep_pitch = value - 64;
        break;
    case 29:
        track->sweep_pitch = (value - 64) * 24;
        break;
    case 65:
        track->porta_flag = (value >= 64) ? TRUE : FALSE;
        break;
    case 84:
        track->porta_key = value + track->transpose;
        track->porta_flag = TRUE;
        break;
    case 85:
        track->attack = value;
        break;
    case 86:
        track->decay = value;
        break;
    case 87:
        track->sustain = value;
        break;
    case 88:
        track->release = value;
        break;
    }
}

static void MsgChannelMode(SNDMidiPlayer *player, u8 ctrl, u8 value)
{
    switch (ctrl) {
    case 120:
        MsgAllSoundOff(player);
        break;
    case 121:
        MsgResetAllController(player);
        break;
    case 123:
        MsgAllNoteOff(player);
        break;
    }
}

static void MsgAllSoundOff(SNDMidiPlayer *player)
{
    SNDMidiTrack *track;
    SNDExChannel *chp;
    int trackNo;
    int i;

    SDK_NULL_ASSERT(player);

    for (trackNo = 0; trackNo < SND_MIDI_TRACK_NUM; trackNo++) {
        track = &player->track[trackNo];

        for (i = 0; i < SND_CHANNEL_NUM; i++) {
            chp = track->channels[i].chp;

            if (chp != NULL) {
                UpdateTrackChannel(chp, track, player);

                if (SND_IsExChannelActive(chp)) {
                    chp->prio = SND_EX_CHANNEL_RELEASE_PRIO;
                    SND_SetExChannelRelease(chp, 127);
                    SND_ReleaseExChannel(chp);
                }
            }
        }
    }
}

static void MsgAllNoteOff(SNDMidiPlayer *player)
{
    SNDMidiTrack *track;
    SNDExChannel *chp;
    int trackNo;
    int i;

    SDK_NULL_ASSERT(player);

    for (trackNo = 0; trackNo < SND_MIDI_TRACK_NUM; trackNo++) {
        track = &player->track[trackNo];

        for (i = 0; i < SND_CHANNEL_NUM; i++) {
            chp = track->channels[i].chp;

            if (chp != NULL) {
                UpdateTrackChannel(chp, track, player);

                if (SND_IsExChannelActive(chp)) {
                    chp->prio = SND_EX_CHANNEL_RELEASE_PRIO;
                    SND_ReleaseExChannel(chp);
                }
            }
        }
    }
}

static void MsgResetAllController(SNDMidiPlayer *player)
{
    SNDMidiTrack *track;
    int trackNo;

    SDK_NULL_ASSERT(player);

    for (trackNo = 0; trackNo < SND_MIDI_TRACK_NUM; trackNo++) {
        track = &player->track[trackNo];
        InitTrackController(track);
    }
}

static void MsgReset(SNDMidiPlayer *player)
{
    int trackNo;

    for (trackNo = 0; trackNo < SND_MIDI_TRACK_NUM; trackNo++) {
        ResetTrack(&player->track[trackNo]);
    }

    player->main_volume = 127;
    player->prio = 64;
}
