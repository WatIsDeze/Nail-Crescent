/*
Copyright (C) 1997-2001 Id Software, Inc.

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#ifndef MSG_H
#define MSG_H

#include "common/protocol.h"
#include "common/sizebuf.h"

//---------------
// A trick taken from Quetoo. Use an int32_t and a float in an union to
// simplify the networking of a float.
//---------------
typedef union {
    int32_t i;
    float f;
} msg_float;

//---------------
// This is the actual packed entity data that is transferred over the network.
// It is bit precise, sensitive, and any change here would required changes elsewhere.
// 
// Be careful, and only touch this if you know what you are doing.
//---------------
typedef struct {
    uint16_t    number;
    // N&C: Full float precision for entities.
    vec3_t      origin;
    vec3_t      angles;
    vec3_t      old_origin;

    // Model indexes.
    uint8_t     modelindex;
    uint8_t     modelindex2;
    uint8_t     modelindex3;
    uint8_t     modelindex4;

    // Rendering effects.
    uint32_t    skinnum;
    uint32_t    effects;
    uint32_t    renderfx;
    uint32_t    solid;
    uint16_t    frame;

    // Sound ID, and Event ID.
    uint8_t     sound;
    uint8_t     event;
} entity_packed_t;

//---------------
// Player state messaging flags.
//---------------
typedef enum {
    MSG_PS_IGNORE_VIEWANGLES    = (1 << 0),
    MSG_PS_IGNORE_DELTAANGLES   = (1 << 1),
    MSG_PS_IGNORE_PREDICTION    = (1 << 2),      // mutually exclusive with IGNORE_VIEWANGLES
    MSG_PS_FORCE                = (1 << 3),
    MSG_PS_REMOVE               = (1 << 4)
} msgPsFlags_t;

//---------------
// Entity state messaging flags.
//---------------
typedef enum msgEsFlags_s {
    MSG_ES_FORCE = (1 << 0),
    MSG_ES_NEWENTITY = (1 << 1),
    MSG_ES_FIRSTPERSON = (1 << 2),
    MSG_ES_UMASK = (1 << 4),
    MSG_ES_BEAMORIGIN = (1 << 5),
    MSG_ES_REMOVE = (1 << 7)
} msgEsFlags_t;

// CPP: Commented out, this was C code. 
//typedef enum {
//    MSG_ES_FORCE        = (1 << 0),
//    MSG_ES_NEWENTITY    = (1 << 1),
//    MSG_ES_FIRSTPERSON  = (1 << 2),
//    MSG_ES_LONGSOLID    = (1 << 3),
//    MSG_ES_UMASK        = (1 << 4),
//    MSG_ES_BEAMORIGIN   = (1 << 5),
//    MSG_ES_SHORTANGLES  = (1 << 6),
//    MSG_ES_REMOVE       = (1 << 7)
//} msgEsFlags_t;

extern sizebuf_t    msg_write;
extern byte         msg_write_buffer[MAX_MSGLEN];

extern sizebuf_t    msg_read;
extern byte         msg_read_buffer[MAX_MSGLEN];

extern const entity_packed_t    nullEntityState;
extern const player_state_t     nullPlayerState;
extern const usercmd_t          nullUserCmd;

void    MSG_Init(void);

void    MSG_BeginWriting(void);
void    MSG_WriteChar(int c);
void    MSG_WriteByte(int c);
void    MSG_WriteShort(int c);
void    MSG_WriteLong(int c);
void    MSG_WriteFloat(float c);
void    MSG_WriteString(const char *s);
void    MSG_WritePosition(const vec3_t &pos);
#if USE_CLIENT
void    MSG_WriteBits(int value, int bits);
int     MSG_WriteDeltaUsercmd(const usercmd_t *from, const usercmd_t *cmd);
#endif
void    MSG_WriteDirection(const vec3_t &dir);
void    MSG_PackEntity(entity_packed_t *out, const entity_state_t *in, qboolean short_angles);
void    MSG_WriteDeltaEntity(const entity_packed_t *from, const entity_packed_t *to, msgEsFlags_t flags);
void    MSG_WriteDeltaPlayerstate_Default(const player_state_t *from, const player_state_t *to);
int     MSG_WriteDeltaPlayerstate_Enhanced(const player_state_t *from, player_state_t *to, msgPsFlags_t flags);

static inline void *MSG_WriteData(const void *data, size_t len)
{
    return memcpy(SZ_GetSpace(&msg_write, len), data, len);
}

static inline void MSG_FlushTo(sizebuf_t *buf)
{
    SZ_Write(buf, msg_write.data, msg_write.cursize);
    SZ_Clear(&msg_write);
}

void    MSG_BeginReading(void);
byte    *MSG_ReadData(size_t len);
int     MSG_ReadChar(void);
int     MSG_ReadByte(void);
int     MSG_ReadShort(void);
int     MSG_ReadWord(void);
int     MSG_ReadLong(void);
float   MSG_ReadFloat(void);
size_t  MSG_ReadString(char *dest, size_t size);
size_t  MSG_ReadStringLine(char *dest, size_t size);
#if USE_CLIENT
vec3_t  MSG_ReadPosition(void);
vec3_t  MSG_ReadDirection(void);
#endif
void    MSG_ReadDeltaUsercmd(const usercmd_t *from, usercmd_t *cmd);
int     MSG_ParseEntityBits(int *bits);
void    MSG_ParseDeltaEntity(const entity_state_t *from, entity_state_t *to, int number, int bits, msgEsFlags_t flags);
#if USE_CLIENT
void    MSG_ParseDeltaPlayerstate_Default(const player_state_t *from, player_state_t *to, int flags);
void    MSG_ParseDeltaPlayerstate_Enhanced(const player_state_t *from, player_state_t *to, int flags, int extraflags);
#endif

#ifdef _DEBUG
#if USE_CLIENT
void    MSG_ShowDeltaPlayerstateBits_Default(int flags);
void    MSG_ShowDeltaPlayerstateBits_Enhanced(int flags, int extraflags);
void    MSG_ShowDeltaUsercmdBits_Enhanced(int bits);
#endif
#if USE_CLIENT
void    MSG_ShowDeltaEntityBits(int bits);
void    MSG_ShowDeltaPlayerstateBits_Packet(int flags);
const char *MSG_ServerCommandString(int cmd);
#define MSG_ShowSVC(cmd) \
    Com_LPrintf(PRINT_DEVELOPER, "%3" PRIz ":%s\n", msg_read.readcount - 1, \
        MSG_ServerCommandString(cmd))
#endif // USE_CLIENT
#endif // _DEBUG


//============================================================================

static inline int MSG_PackSolid16(const vec3_t mins, const vec3_t maxs)
{
    int x, zd, zu;

    // assume that x/y are equal and symetric
    x = maxs[0] / 8;
    clamp(x, 1, 31);

    // z is not symetric
    zd = -mins[2] / 8;
    clamp(zd, 1, 31);

    // and z maxs can be negative...
    zu = (maxs[2] + 32) / 8;
    clamp(zu, 1, 63);

    return (zu << 10) | (zd << 5) | x;
}

static inline int MSG_PackSolid32(const vec3_t mins, const vec3_t maxs)
{
    int x, zd, zu;

    // assume that x/y are equal and symetric
    x = maxs[0];
    clamp(x, 1, 255);

    // z is not symetric
    zd = -mins[2];
    clamp(zd, 1, 255);

    // and z maxs can be negative...
    zu = maxs[2] + 32768;
    clamp(zu, 1, 65535);

    return (zu << 16) | (zd << 8) | x;
}

static inline void MSG_UnpackSolid16(int solid, vec3_t mins, vec3_t maxs)
{
    int x, zd, zu;

    x = 8 * (solid & 31);
    zd = 8 * ((solid >> 5) & 31);
    zu = 8 * ((solid >> 10) & 63) - 32;

    mins[0] = mins[1] = -x;
    maxs[0] = maxs[1] = x;
    mins[2] = -zd;
    maxs[2] = zu;
}

static inline void MSG_UnpackSolid32(int solid, vec3_t mins, vec3_t maxs)
{
    int x, zd, zu;

    x = solid & 255;
    zd = (solid >> 8) & 255;
    zu = ((solid >> 16) & 65535) - 32768;

    mins[0] = mins[1] = -x;
    maxs[0] = maxs[1] = x;
    mins[2] = -zd;
    maxs[2] = zu;
}

#endif // MSG_H
