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

#include "shared/shared.h"
#include "common/msg.h"
#include "common/protocol.h"
#include "common/sizebuf.h"
#include "sharedgame/protocol.h"

/*
==============================================================================

            MESSAGE IO FUNCTIONS

Handles byte ordering and avoids alignment errors
==============================================================================
*/

sizebuf_t   msg_write;
byte        msg_write_buffer[MAX_MSGLEN];

sizebuf_t   msg_read;
byte        msg_read_buffer[MAX_MSGLEN];

const entity_packed_t   nullEntityState;
const player_packed_t   nullPlayerState;
const usercmd_t         nullUserCmd;

/*
=============
MSG_Init

Initialize default buffers, clearing allow overflow/underflow flags.

This is the only place where writing buffer is initialized. Writing buffer is
never allowed to overflow.

Reading buffer is reinitialized in many other places. Reinitializing will set
the allow underflow flag as appropriate.
=============
*/
void MSG_Init(void)
{
    SZ_TagInit(&msg_read, msg_read_buffer, MAX_MSGLEN, SZ_MSG_READ);
    SZ_TagInit(&msg_write, msg_write_buffer, MAX_MSGLEN, SZ_MSG_WRITE);
}


/*
==============================================================================

            WRITING

==============================================================================
*/

/*
=============
MSG_BeginWriting
=============
*/
void MSG_BeginWriting(void)
{
    msg_write.cursize = 0;
    msg_write.bitpos = 0;
    msg_write.overflowed = false;
}

//
//===============
// MSG_WriteChar
// 
//===============
//
void MSG_WriteChar(int c)
{
    byte    *buf;

#ifdef PARANOID
    if (c < -128 || c > 127)
        Com_Error(ERR_FATAL, "MSG_WriteChar: range error");
#endif

    buf = (byte*)SZ_GetSpace(&msg_write, 1); // CPP: Cast
    buf[0] = c;
}

//
//===============
// MSG_WriteByte
// 
//===============
//
void MSG_WriteByte(int c)
{
    byte    *buf;

#ifdef PARANOID
    if (c < 0 || c > 255)
        Com_Error(ERR_FATAL, "MSG_WriteByte: range error");
#endif

    buf = (byte*)SZ_GetSpace(&msg_write, 1); // CPP: Cast
    buf[0] = c;
}

//
//===============
// MSG_WriteShort
// 
//===============
//
void MSG_WriteShort(int c)
{
    byte    *buf;

#ifdef PARANOID
    if (c < ((short)0x8000) || c > (short)0x7fff)
        Com_Error(ERR_FATAL, "MSG_WriteShort: range error");
#endif

    buf = (byte*)SZ_GetSpace(&msg_write, 2); // CPP: Cast
    buf[0] = c & 0xff;
    buf[1] = c >> 8;
}

//
//===============
// MSG_WriteLong
// 
//===============
//
void MSG_WriteLong(int c)
{
    byte    *buf;

    buf = (byte*)SZ_GetSpace(&msg_write, 4); // CPP: Cast
    buf[0] = c & 0xff;
    buf[1] = (c >> 8) & 0xff;
    buf[2] = (c >> 16) & 0xff;
    buf[3] = c >> 24;
}

//
//===============
// MSG_WriteFloat
// 
// The idea is smart and taken from Quetoo, use an union for memory mapping.
// Write the float as an int32_t, use it after reading as a float.
//================
//
void MSG_WriteFloat(float c) {
    msg_float vec;
    vec.f = c;
    MSG_WriteLong(vec.i);
}

//
//===============
// MSG_WriteString
// 
//===============
//
void MSG_WriteString(const char *string)
{
    size_t length;

    if (!string) {
        MSG_WriteByte(0);
        return;
    }

    length = strlen(string);
    if (length >= MAX_NET_STRING) {
        Com_WPrintf("%s: overflow: %" PRIz " chars", __func__, length);
        MSG_WriteByte(0);
        return;
    }

    MSG_WriteData(string, length + 1);
}

//
//===============
// MSG_WritePosition
// 
//===============
//
void MSG_WritePosition(const vec3_t &pos)
{
    MSG_WriteFloat(pos[0]);
    MSG_WriteFloat(pos[1]);
    MSG_WriteFloat(pos[2]);
}

//
//===============
// MSG_WriteAngle
// 
//===============
//

#define ANGLE2BYTE(x)   ((int)((x)*256.0f/360)&255)
#define BYTE2ANGLE(x)   ((x)*(360.0f/256))

void MSG_WriteAngle(float f)
{
    MSG_WriteByte(ANGLE2BYTE(f));
}

#include "sharedgame/sharedgame.h"
#define BUTTON_MASK     (BUTTON_ATTACK|BUTTON_WALK|BUTTON_USE|BUTTON_ANY)

#if USE_CLIENT

//
//===============
// MSG_WriteDeltaUsercmd
// 
//===============
//
int MSG_WriteDeltaUsercmd(const usercmd_t *from, const usercmd_t *cmd)
{
    // Send a null message in case we had none.
    if (!from) {
        from = &nullUserCmd;
    }


    //
    // send the movement message
    //
    int32_t bits = 0;

    if (cmd->angles[0] != from->angles[0])
        bits |= CM_ANGLE1;
    if (cmd->angles[1] != from->angles[1])
        bits |= CM_ANGLE2;
    if (cmd->angles[2] != from->angles[2])
        bits |= CM_ANGLE3;
    if (cmd->forwardmove != from->forwardmove)
        bits |= CM_FORWARD;
    if (cmd->sidemove != from->sidemove)
        bits |= CM_SIDE;
    if (cmd->upmove != from->upmove)
        bits |= CM_UP;
    if (cmd->buttons != from->buttons)
        bits |= CM_BUTTONS;
    if (cmd->impulse != from->impulse)
        bits |= CM_IMPULSE;

    // Write out the changed bits.
    MSG_WriteByte(bits);

    if (bits & CM_ANGLE1)
        MSG_WriteShort(cmd->angles[0]);
    if (bits & CM_ANGLE2)
        MSG_WriteShort(cmd->angles[1]);
    if (bits & CM_ANGLE3)
        MSG_WriteShort(cmd->angles[2]);

    if (bits & CM_FORWARD)
        MSG_WriteShort(cmd->forwardmove);
    if (bits & CM_SIDE)
        MSG_WriteShort(cmd->sidemove);
    if (bits & CM_UP)
        MSG_WriteShort(cmd->upmove);

    if (bits & CM_BUTTONS)
        MSG_WriteByte(cmd->buttons);

    if (bits & CM_IMPULSE)
        MSG_WriteByte(cmd->impulse);

    MSG_WriteByte(cmd->msec);
    MSG_WriteByte(cmd->lightlevel);

    // (Returned bits isn't used anywhere, but might as well keep it around.)
    return bits;
}

#endif // USE_CLIENT

void MSG_WriteDirection(const vec3_t &dir)
{
    int     best;

    best = DirToByte(dir);
    MSG_WriteByte(best);
}

void MSG_PackEntity(entity_packed_t *out, const entity_state_t *in, qboolean short_angles)
{
    // allow 0 to accomodate empty baselines
    if (in->number < 0 || in->number >= MAX_EDICTS)
        Com_Error(ERR_DROP, "%s: bad number: %d", __func__, in->number);

    // N&C: Full float precision.
    out->number      = in->number;
    out->origin      = in->origin;
    out->angles      = in->angles;
    out->old_origin  = in->old_origin;
    out->modelindex  = in->modelindex;
    out->modelindex2 = in->modelindex2;
    out->modelindex3 = in->modelindex3;
    out->modelindex4 = in->modelindex4;
    out->skinnum     = in->skinnum;
    out->effects     = in->effects;
    out->renderfx    = in->renderfx;
    out->solid       = in->solid;
    out->frame       = in->frame;
    out->sound       = in->sound;
    out->event       = in->event;
}

void MSG_WriteDeltaEntity(const entity_packed_t *from,
                          const entity_packed_t *to,
                          msgEsFlags_t          flags)
{
    uint32_t    bits, mask;

    if (!to) {
        if (!from)
            Com_Error(ERR_DROP, "%s: NULL", __func__);

        if (from->number < 1 || from->number >= MAX_EDICTS)
            Com_Error(ERR_DROP, "%s: bad number: %d", __func__, from->number);

        bits = U_REMOVE;
        if (from->number & 0xff00)
            bits |= U_NUMBER16 | U_MOREBITS1;

        MSG_WriteByte(bits & 255);
        if (bits & 0x0000ff00)
            MSG_WriteByte((bits >> 8) & 255);

        if (bits & U_NUMBER16)
            MSG_WriteShort(from->number);
        else
            MSG_WriteByte(from->number);

        return; // remove entity
    }

    if (to->number < 1 || to->number >= MAX_EDICTS)
        Com_Error(ERR_DROP, "%s: bad number: %d", __func__, to->number);

    if (!from)
        from = &nullEntityState;

// send an update
    bits = 0;

    if (!(flags & MSG_ES_FIRSTPERSON)) {
        if (!EqualEpsilonf(to->origin[0], from->origin[0]))//if (to->origin[0] != from->origin[0])
            bits |= U_ORIGIN_X;
        if (!EqualEpsilonf(to->origin[1], from->origin[1]))
            bits |= U_ORIGIN_Y;
        if (!EqualEpsilonf(to->origin[2], from->origin[2]))
            bits |= U_ORIGIN_Z;

        // N&C: Full float precision.
        if (!EqualEpsilonf(to->angles[0], from->angles[0]))
            bits |= U_ANGLE_X | U_ANGLE16;
        if (!EqualEpsilonf(to->angles[1], from->angles[1]))
            bits |= U_ANGLE_Y | U_ANGLE16;
        if (!EqualEpsilonf(to->angles[2], from->angles[2]))
            bits |= U_ANGLE_Z | U_ANGLE16;

        if (flags & MSG_ES_NEWENTITY) {
            if (!EqualEpsilonf(to->old_origin[0], from->origin[0]) ||
                !EqualEpsilonf(to->old_origin[1], from->origin[1]) ||
                !EqualEpsilonf(to->old_origin[2], from->origin[2]))
                bits |= U_OLDORIGIN;
        }
    }

    if (flags & MSG_ES_UMASK)
        mask = 0xffff0000;
    else
        mask = 0xffff8000;  // don't confuse old clients

    if (to->skinnum != from->skinnum) {
        if (to->skinnum & mask)
            bits |= U_SKIN8 | U_SKIN16;
        else if (to->skinnum & 0x0000ff00)
            bits |= U_SKIN16;
        else
            bits |= U_SKIN8;
    }

    if (to->frame != from->frame) {
        if (to->frame & 0xff00)
            bits |= U_FRAME16;
        else
            bits |= U_FRAME8;
    }

    if (to->effects != from->effects) {
        if (to->effects & mask)
            bits |= U_EFFECTS8 | U_EFFECTS16;
        else if (to->effects & 0x0000ff00)
            bits |= U_EFFECTS16;
        else
            bits |= U_EFFECTS8;
    }

    if (to->renderfx != from->renderfx) {
        if (to->renderfx & mask)
            bits |= U_RENDERFX8 | U_RENDERFX16;
        else if (to->renderfx & 0x0000ff00)
            bits |= U_RENDERFX16;
        else
            bits |= U_RENDERFX8;
    }

    if (to->solid != from->solid)
        bits |= U_SOLID;

    // event is not delta compressed, just 0 compressed
    if (to->event)
        bits |= U_EVENT;

    if (to->modelindex != from->modelindex)
        bits |= U_MODEL;
    if (to->modelindex2 != from->modelindex2)
        bits |= U_MODEL2;
    if (to->modelindex3 != from->modelindex3)
        bits |= U_MODEL3;
    if (to->modelindex4 != from->modelindex4)
        bits |= U_MODEL4;

    if (to->sound != from->sound)
        bits |= U_SOUND;

    if (to->renderfx & RenderEffects::FrameLerp) {
        bits |= U_OLDORIGIN;
    } else if (to->renderfx & RenderEffects::Beam) {
        if (flags & MSG_ES_BEAMORIGIN) {
            if (!EqualEpsilonf(to->old_origin[0], from->old_origin[0]) ||
                !EqualEpsilonf(to->old_origin[1], from->old_origin[1]) ||
                !EqualEpsilonf(to->old_origin[2], from->old_origin[2]))
                bits |= U_OLDORIGIN;
        } else {
            bits |= U_OLDORIGIN;
        }
    }

    //
    // write the message
    //
    if (!bits && !(flags & MSG_ES_FORCE))
        return;     // nothing to send!

    if (flags & MSG_ES_REMOVE)
        bits |= U_REMOVE; // used for MVD stream only

    //----------

    if (to->number & 0xff00)
        bits |= U_NUMBER16;     // number8 is implicit otherwise

    if (bits & 0xff000000)
        bits |= U_MOREBITS3 | U_MOREBITS2 | U_MOREBITS1;
    else if (bits & 0x00ff0000)
        bits |= U_MOREBITS2 | U_MOREBITS1;
    else if (bits & 0x0000ff00)
        bits |= U_MOREBITS1;

    MSG_WriteByte(bits & 255);

    if (bits & 0xff000000) {
        MSG_WriteByte((bits >> 8) & 255);
        MSG_WriteByte((bits >> 16) & 255);
        MSG_WriteByte((bits >> 24) & 255);
    } else if (bits & 0x00ff0000) {
        MSG_WriteByte((bits >> 8) & 255);
        MSG_WriteByte((bits >> 16) & 255);
    } else if (bits & 0x0000ff00) {
        MSG_WriteByte((bits >> 8) & 255);
    }

    //----------

    if (bits & U_NUMBER16)
        MSG_WriteShort(to->number);
    else
        MSG_WriteByte(to->number);

    if (bits & U_MODEL)
        MSG_WriteByte(to->modelindex);
    if (bits & U_MODEL2)
        MSG_WriteByte(to->modelindex2);
    if (bits & U_MODEL3)
        MSG_WriteByte(to->modelindex3);
    if (bits & U_MODEL4)
        MSG_WriteByte(to->modelindex4);

    if (bits & U_FRAME8)
        MSG_WriteByte(to->frame);
    else if (bits & U_FRAME16)
        MSG_WriteShort(to->frame);

    if ((bits & (U_SKIN8 | U_SKIN16)) == (U_SKIN8 | U_SKIN16))  //used for laser colors
        MSG_WriteLong(to->skinnum);
    else if (bits & U_SKIN8)
        MSG_WriteByte(to->skinnum);
    else if (bits & U_SKIN16)
        MSG_WriteShort(to->skinnum);

    if ((bits & (U_EFFECTS8 | U_EFFECTS16)) == (U_EFFECTS8 | U_EFFECTS16))
        MSG_WriteLong(to->effects);
    else if (bits & U_EFFECTS8)
        MSG_WriteByte(to->effects);
    else if (bits & U_EFFECTS16)
        MSG_WriteShort(to->effects);

    if ((bits & (U_RENDERFX8 | U_RENDERFX16)) == (U_RENDERFX8 | U_RENDERFX16))
        MSG_WriteLong(to->renderfx);
    else if (bits & U_RENDERFX8)
        MSG_WriteByte(to->renderfx);
    else if (bits & U_RENDERFX16)
        MSG_WriteShort(to->renderfx);
    
    // N&C: Full float precision.
    if (bits & U_ORIGIN_X)
        MSG_WriteFloat(to->origin[0]);
    if (bits & U_ORIGIN_Y)
        MSG_WriteFloat(to->origin[1]);
    if (bits & U_ORIGIN_Z)
        MSG_WriteFloat(to->origin[2]);

    // N&C: Full float precision.
    if (bits & U_ANGLE16) {
        if (bits & U_ANGLE_X)
            MSG_WriteFloat(to->angles[0]);
        if (bits & U_ANGLE_Y)
            MSG_WriteFloat(to->angles[1]);
        if (bits & U_ANGLE_Z)
            MSG_WriteFloat(to->angles[2]);
    }

    // N&C: Full float precision.
    if (bits & U_OLDORIGIN) {
        MSG_WriteFloat(to->old_origin[0]);
        MSG_WriteFloat(to->old_origin[1]);
        MSG_WriteFloat(to->old_origin[2]);
    }

    if (bits & U_SOUND)
        MSG_WriteByte(to->sound);
    if (bits & U_EVENT)
        MSG_WriteByte(to->event);
    if (bits & U_SOLID) {
        MSG_WriteLong(to->solid);
    }
}

void MSG_PackPlayer(player_packed_t *out, const player_state_t *in)
{
    int i;

    out->pmove = in->pmove;
    out->viewAngles[0] = ANGLE2SHORT(in->viewAngles.x);
    out->viewAngles[1] = ANGLE2SHORT(in->viewAngles.y);
    out->viewAngles[2] = ANGLE2SHORT(in->viewAngles.z);
    out->viewoffset[0] = in->viewoffset[0] * 4;
    out->viewoffset[1] = in->viewoffset[1] * 4;
    out->viewoffset[2] = in->viewoffset[2] * 4;
    out->kickAngles[0] = in->kickAngles[0] * 4;
    out->kickAngles[1] = in->kickAngles[1] * 4;
    out->kickAngles[2] = in->kickAngles[2] * 4;
    out->gunoffset[0] = in->gunoffset[0] * 4;
    out->gunoffset[1] = in->gunoffset[1] * 4;
    out->gunoffset[2] = in->gunoffset[2] * 4;
    out->gunangles[0] = in->gunangles[0] * 4;
    out->gunangles[1] = in->gunangles[1] * 4;
    out->gunangles[2] = in->gunangles[2] * 4;
    out->gunindex = in->gunindex;
    out->gunframe = in->gunframe;
    out->blend[0] = in->blend[0] * 255;
    out->blend[1] = in->blend[1] * 255;
    out->blend[2] = in->blend[2] * 255;
    out->blend[3] = in->blend[3] * 255;
    out->fov = in->fov;
    out->rdflags = in->rdflags;
    for (i = 0; i < MAX_STATS; i++)
        out->stats[i] = in->stats[i];
}

void MSG_WriteDeltaPlayerstate_Default(const player_packed_t *from, const player_packed_t *to)
{
    int     i;
    int     pflags;
    int     statbits;

    if (!to)
        Com_Error(ERR_DROP, "%s: NULL", __func__);

    if (!from)
        from = &nullPlayerState;

    //
    // determine what needs to be sent
    //
    pflags = 0;

    if (to->pmove.type != from->pmove.type)
        pflags |= PS_M_TYPE;

    if (!EqualEpsilonf(to->pmove.origin[0], from->pmove.origin[0]) ||
        !EqualEpsilonf(to->pmove.origin[1], from->pmove.origin[1]) ||
        !EqualEpsilonf(to->pmove.origin[2], from->pmove.origin[2]))
        pflags |= PS_M_ORIGIN;

    if (!EqualEpsilonf(to->pmove.velocity[0], from->pmove.velocity[0]) ||
        !EqualEpsilonf(to->pmove.velocity[1], from->pmove.velocity[1]) ||
        !EqualEpsilonf(to->pmove.velocity[2], from->pmove.velocity[2]))
        pflags |= PS_M_VELOCITY;

    if (to->pmove.time != from->pmove.time)
        pflags |= PS_M_TIME;

    if (to->pmove.flags != from->pmove.flags)
        pflags |= PS_M_FLAGS;

    if (to->pmove.gravity != from->pmove.gravity)
        pflags |= PS_M_GRAVITY;

    if (to->pmove.delta_angles[0] != from->pmove.delta_angles[0] ||
        to->pmove.delta_angles[1] != from->pmove.delta_angles[1] ||
        to->pmove.delta_angles[2] != from->pmove.delta_angles[2])
        pflags |= PS_M_DELTA_ANGLES;

    if (to->viewoffset[0] != from->viewoffset[0] ||
        to->viewoffset[1] != from->viewoffset[1] ||
        to->viewoffset[2] != from->viewoffset[2])
        pflags |= PS_VIEWOFFSET;

    if (to->viewAngles[0] != from->viewAngles[0] ||
        to->viewAngles[1] != from->viewAngles[1] ||
        to->viewAngles[2] != from->viewAngles[2])
        pflags |= PS_VIEWANGLES;

    if (to->kickAngles[0] != from->kickAngles[0] ||
        to->kickAngles[1] != from->kickAngles[1] ||
        to->kickAngles[2] != from->kickAngles[2])
        pflags |= PS_KICKANGLES;

    if (to->blend[0] != from->blend[0] ||
        to->blend[1] != from->blend[1] ||
        to->blend[2] != from->blend[2] ||
        to->blend[3] != from->blend[3])
        pflags |= PS_BLEND;

    if (to->fov != from->fov)
        pflags |= PS_FOV;

    if (to->rdflags != from->rdflags)
        pflags |= PS_RDFLAGS;

    if (to->gunframe != from->gunframe ||
        to->gunoffset[0] != from->gunoffset[0] ||
        to->gunoffset[1] != from->gunoffset[1] ||
        to->gunoffset[2] != from->gunoffset[2] ||
        to->gunangles[0] != from->gunangles[0] ||
        to->gunangles[1] != from->gunangles[1] ||
        to->gunangles[2] != from->gunangles[2])
        pflags |= PS_WEAPONFRAME;

    if (to->gunindex != from->gunindex)
        pflags |= PS_WEAPONINDEX;

    //
    // write it
    //
    MSG_WriteShort(pflags);

    //
    // write the pm_state_t
    //
    if (pflags & PS_M_TYPE)
        MSG_WriteByte(to->pmove.type);

    if (pflags & PS_M_ORIGIN) {
        MSG_WriteFloat(to->pmove.origin[0]);
        MSG_WriteFloat(to->pmove.origin[1]);
        MSG_WriteFloat(to->pmove.origin[2]);
    }

    if (pflags & PS_M_VELOCITY) {
        MSG_WriteFloat(to->pmove.velocity[0]);
        MSG_WriteFloat(to->pmove.velocity[1]);
        MSG_WriteFloat(to->pmove.velocity[2]);
    }

    if (pflags & PS_M_TIME)
        MSG_WriteShort(to->pmove.time);

    if (pflags & PS_M_FLAGS)
        MSG_WriteShort(to->pmove.flags);

    if (pflags & PS_M_GRAVITY)
        MSG_WriteShort(to->pmove.gravity);

    if (pflags & PS_M_DELTA_ANGLES) {
        MSG_WriteShort(to->pmove.delta_angles[0]);
        MSG_WriteShort(to->pmove.delta_angles[1]);
        MSG_WriteShort(to->pmove.delta_angles[2]);
    }

    //
    // write the rest of the player_state_t
    //
    if (pflags & PS_VIEWOFFSET) {
        MSG_WriteChar(to->viewoffset[0]);
        MSG_WriteChar(to->viewoffset[1]);
        MSG_WriteChar(to->viewoffset[2]);
    }

    if (pflags & PS_VIEWANGLES) {
        MSG_WriteShort(to->viewAngles[0]);
        MSG_WriteShort(to->viewAngles[1]);
        MSG_WriteShort(to->viewAngles[2]);
    }

    if (pflags & PS_KICKANGLES) {
        MSG_WriteChar(to->kickAngles[0]);
        MSG_WriteChar(to->kickAngles[1]);
        MSG_WriteChar(to->kickAngles[2]);
    }

    if (pflags & PS_WEAPONINDEX)
        MSG_WriteByte(to->gunindex);

    if (pflags & PS_WEAPONFRAME) {
        MSG_WriteByte(to->gunframe);
        MSG_WriteChar(to->gunoffset[0]);
        MSG_WriteChar(to->gunoffset[1]);
        MSG_WriteChar(to->gunoffset[2]);
        MSG_WriteChar(to->gunangles[0]);
        MSG_WriteChar(to->gunangles[1]);
        MSG_WriteChar(to->gunangles[2]);
    }

    if (pflags & PS_BLEND) {
        MSG_WriteByte(to->blend[0]);
        MSG_WriteByte(to->blend[1]);
        MSG_WriteByte(to->blend[2]);
        MSG_WriteByte(to->blend[3]);
    }

    if (pflags & PS_FOV)
        MSG_WriteByte(to->fov);

    if (pflags & PS_RDFLAGS)
        MSG_WriteByte(to->rdflags);

    // Send stats
    statbits = 0;
    for (i = 0; i < MAX_STATS; i++)
        if (to->stats[i] != from->stats[i])
            statbits |= 1 << i;

    MSG_WriteLong(statbits);
    for (i = 0; i < MAX_STATS; i++)
        if (statbits & (1 << i))
            MSG_WriteShort(to->stats[i]);
}

int MSG_WriteDeltaPlayerstate_Enhanced(const player_packed_t    *from,
                                             player_packed_t    *to,
                                             msgPsFlags_t       flags)
{
    int     i;
    int     pflags, eflags;
    int     statbits;

    if (!to)
        Com_Error(ERR_DROP, "%s: NULL", __func__);

    if (!from)
        from = &nullPlayerState;

    //
    // Determine what needs to be sent
    //
    pflags = 0;
    eflags = 0;

    if (to->pmove.type != from->pmove.type)
        pflags |= PS_M_TYPE;

    if (!EqualEpsilonf(to->pmove.origin[0], from->pmove.origin[0]) ||
        !EqualEpsilonf(to->pmove.origin[1], from->pmove.origin[1]))
        pflags |= PS_M_ORIGIN;

    if (!EqualEpsilonf(to->pmove.origin[2], from->pmove.origin[2]))
        eflags |= EPS_M_ORIGIN2;

    if (!(flags & MSG_PS_IGNORE_PREDICTION)) {
        if (!EqualEpsilonf(to->pmove.velocity[0], from->pmove.velocity[0]) ||
            !EqualEpsilonf(to->pmove.velocity[1], from->pmove.velocity[1]))
            pflags |= PS_M_VELOCITY;

        if (!EqualEpsilonf(to->pmove.velocity[2], from->pmove.velocity[2]))
            eflags |= EPS_M_VELOCITY2;

        if (to->pmove.time != from->pmove.time)
            pflags |= PS_M_TIME;

        if (to->pmove.flags != from->pmove.flags)
            pflags |= PS_M_FLAGS;

        if (to->pmove.gravity != from->pmove.gravity)
            pflags |= PS_M_GRAVITY;
    } else {
        // save previous state
        VectorCopy(from->pmove.velocity, to->pmove.velocity);
        to->pmove.time = from->pmove.time;
        to->pmove.flags = from->pmove.flags;
        to->pmove.gravity = from->pmove.gravity;
    }

    if (!(flags & MSG_PS_IGNORE_DELTAANGLES)) {
        if (to->pmove.delta_angles[0] != from->pmove.delta_angles[0] ||
            to->pmove.delta_angles[1] != from->pmove.delta_angles[1] ||
            to->pmove.delta_angles[2] != from->pmove.delta_angles[2])
            pflags |= PS_M_DELTA_ANGLES;
    } else {
        // save previous state
        VectorCopy(from->pmove.delta_angles, to->pmove.delta_angles);
    }

    if (from->viewoffset[0] != to->viewoffset[0] ||
        from->viewoffset[1] != to->viewoffset[1] ||
        from->viewoffset[2] != to->viewoffset[2])
        pflags |= PS_VIEWOFFSET;

    if (!(flags & MSG_PS_IGNORE_VIEWANGLES)) {
        if (from->viewAngles[0] != to->viewAngles[0] ||
            from->viewAngles[1] != to->viewAngles[1])
            pflags |= PS_VIEWANGLES;

        if (from->viewAngles[2] != to->viewAngles[2])
            eflags |= EPS_VIEWANGLE2;
    } else {
        // save previous state
        to->viewAngles[0] = from->viewAngles[0];
        to->viewAngles[1] = from->viewAngles[1];
        to->viewAngles[2] = from->viewAngles[2];
    }

    if (from->kickAngles[0] != to->kickAngles[0] ||
        from->kickAngles[1] != to->kickAngles[1] ||
        from->kickAngles[2] != to->kickAngles[2])
        pflags |= PS_KICKANGLES;

    if (from->blend[0] != to->blend[0] ||
        from->blend[1] != to->blend[1] ||
        from->blend[2] != to->blend[2] ||
        from->blend[3] != to->blend[3])
        pflags |= PS_BLEND;

    if (from->fov != to->fov)
        pflags |= PS_FOV;

    if (to->rdflags != from->rdflags)
        pflags |= PS_RDFLAGS;

    if (to->gunindex != from->gunindex)
        pflags |= PS_WEAPONINDEX;

    if (to->gunframe != from->gunframe)
        pflags |= PS_WEAPONFRAME;

    if (from->gunoffset[0] != to->gunoffset[0] ||
        from->gunoffset[1] != to->gunoffset[1] ||
        from->gunoffset[2] != to->gunoffset[2])
        eflags |= EPS_GUNOFFSET;

    if (from->gunangles[0] != to->gunangles[0] ||
        from->gunangles[1] != to->gunangles[1] ||
        from->gunangles[2] != to->gunangles[2])
        eflags |= EPS_GUNANGLES;

    statbits = 0;
    for (i = 0; i < MAX_STATS; i++)
        if (to->stats[i] != from->stats[i])
            statbits |= 1 << i;

    if (statbits)
        eflags |= EPS_STATS;

    //
    // write it
    //
    MSG_WriteShort(pflags);

    //
    // write the pm_state_t
    //
    if (pflags & PS_M_TYPE)
        MSG_WriteByte(to->pmove.type);

    if (pflags & PS_M_ORIGIN) {
        MSG_WriteFloat(to->pmove.origin[0]);
        MSG_WriteFloat(to->pmove.origin[1]);
    }

    if (eflags & EPS_M_ORIGIN2)
        MSG_WriteFloat(to->pmove.origin[2]);

    if (pflags & PS_M_VELOCITY) {
        MSG_WriteFloat(to->pmove.velocity[0]);
        MSG_WriteFloat(to->pmove.velocity[1]);
    }

    if (eflags & EPS_M_VELOCITY2)
        MSG_WriteFloat(to->pmove.velocity[2]);

    if (pflags & PS_M_TIME)
        MSG_WriteShort(to->pmove.time);

    if (pflags & PS_M_FLAGS)
        MSG_WriteShort(to->pmove.flags);

    if (pflags & PS_M_GRAVITY)
        MSG_WriteShort(to->pmove.gravity);

    if (pflags & PS_M_DELTA_ANGLES) {
        MSG_WriteShort(to->pmove.delta_angles[0]);
        MSG_WriteShort(to->pmove.delta_angles[1]);
        MSG_WriteShort(to->pmove.delta_angles[2]);
    }

    //
    // write the rest of the player_state_t
    //
    if (pflags & PS_VIEWOFFSET) {
        MSG_WriteChar(to->viewoffset[0]);
        MSG_WriteChar(to->viewoffset[1]);
        MSG_WriteChar(to->viewoffset[2]);
    }

    if (pflags & PS_VIEWANGLES) {
        MSG_WriteShort(to->viewAngles[0]);
        MSG_WriteShort(to->viewAngles[1]);
    }

    if (eflags & EPS_VIEWANGLE2)
        MSG_WriteShort(to->viewAngles[2]);

    if (pflags & PS_KICKANGLES) {
        MSG_WriteChar(to->kickAngles[0]);
        MSG_WriteChar(to->kickAngles[1]);
        MSG_WriteChar(to->kickAngles[2]);
    }

    if (pflags & PS_WEAPONINDEX)
        MSG_WriteByte(to->gunindex);

    if (pflags & PS_WEAPONFRAME)
        MSG_WriteByte(to->gunframe);

    if (eflags & EPS_GUNOFFSET) {
        MSG_WriteChar(to->gunoffset[0]);
        MSG_WriteChar(to->gunoffset[1]);
        MSG_WriteChar(to->gunoffset[2]);
    }

    if (eflags & EPS_GUNANGLES) {
        MSG_WriteChar(to->gunangles[0]);
        MSG_WriteChar(to->gunangles[1]);
        MSG_WriteChar(to->gunangles[2]);
    }

    if (pflags & PS_BLEND) {
        MSG_WriteByte(to->blend[0]);
        MSG_WriteByte(to->blend[1]);
        MSG_WriteByte(to->blend[2]);
        MSG_WriteByte(to->blend[3]);
    }

    if (pflags & PS_FOV)
        MSG_WriteByte(to->fov);

    if (pflags & PS_RDFLAGS)
        MSG_WriteByte(to->rdflags);

    // send stats
    if (eflags & EPS_STATS) {
        MSG_WriteLong(statbits);
        for (i = 0; i < MAX_STATS; i++)
            if (statbits & (1 << i))
                MSG_WriteShort(to->stats[i]);
    }

    return eflags;
}

/*
==============================================================================

            READING

==============================================================================
*/

void MSG_BeginReading(void)
{
    msg_read.readcount = 0;
    msg_read.bitpos = 0;
}

byte *MSG_ReadData(size_t len)
{
    byte *buf = msg_read.data + msg_read.readcount;

    msg_read.readcount += len;
    msg_read.bitpos = msg_read.readcount << 3;

    if (msg_read.readcount > msg_read.cursize) {
        if (!msg_read.allowunderflow) {
            Com_Error(ERR_DROP, "%s: read past end of message", __func__);
        }
        return NULL;
    }

    return buf;
}

// returns -1 if no more characters are available
int MSG_ReadChar(void)
{
    byte *buf = MSG_ReadData(1);
    int c;

    if (!buf) {
        c = -1;
    } else {
        c = (signed char)buf[0];
    }

    return c;
}

int MSG_ReadByte(void)
{
    byte *buf = MSG_ReadData(1);
    int c;

    if (!buf) {
        c = -1;
    } else {
        c = (unsigned char)buf[0];
    }

    return c;
}

int MSG_ReadShort(void)
{
    byte *buf = MSG_ReadData(2);
    int c;

    if (!buf) {
        c = -1;
    } else {
        c = (signed short)LittleShortMem(buf);
    }

    return c;
}

int MSG_ReadWord(void)
{
    byte *buf = MSG_ReadData(2);
    int c;

    if (!buf) {
        c = -1;
    } else {
        c = (unsigned short)LittleShortMem(buf);
    }

    return c;
}

int MSG_ReadLong(void)
{
    byte *buf = MSG_ReadData(4);
    int c;

    if (!buf) {
        c = -1;
    } else {
        c = LittleLongMem(buf);
    }

    return c;
}

//
//===============
// MSG_ReadFloat
// 
// The idea is smart and taken from Quetoo, use an union for memory mapping.
// Read the float as an int32_t, use the union struct trick to convert it to a float.
//================
//
float MSG_ReadFloat(void) {
    msg_float vec;
    vec.i = MSG_ReadLong();
    return vec.f;
}

size_t MSG_ReadString(char *dest, size_t size)
{
    int     c;
    size_t  len = 0;

    while (1) {
        c = MSG_ReadByte();
        if (c == -1 || c == 0) {
            break;
        }
        if (len + 1 < size) {
            *dest++ = c;
        }
        len++;
    }
    if (size) {
        *dest = 0;
    }

    return len;
}

size_t MSG_ReadStringLine(char *dest, size_t size)
{
    int     c;
    size_t  len = 0;

    while (1) {
        c = MSG_ReadByte();
        if (c == -1 || c == 0 || c == '\n') {
            break;
        }
        if (len + 1 < size) {
            *dest++ = c;
        }
        len++;
    }
    if (size) {
        *dest = 0;
    }

    return len;
}

vec3_t MSG_ReadPosition(void) {
    return vec3_t{
        MSG_ReadFloat(),
        MSG_ReadFloat(),
        MSG_ReadFloat()
    };
}

static inline float MSG_ReadAngle(void)
{
    return BYTE2ANGLE(MSG_ReadChar());
}

static inline float MSG_ReadAngle16(void)
{
    return SHORT2ANGLE(MSG_ReadShort());
}

vec3_t MSG_ReadDirection(void)
{
    int     b;
    
    b = MSG_ReadByte();
    if (b < 0 || b >= NUMVERTEXNORMALS)
        Com_Error(ERR_DROP, "MSG_ReadDirection: out of range");
    
    return bytedirs[b];
}

void MSG_ReadDeltaUsercmd(const usercmd_t *from, usercmd_t *to)
{
    int bits;

    if (from) {
        memcpy(to, from, sizeof(*to));
    }
    else {
        memset(to, 0, sizeof(*to));
    }

    bits = MSG_ReadByte();

    // read current angles
    if (bits & CM_ANGLE1)
        to->angles[0] = MSG_ReadShort();
    if (bits & CM_ANGLE2)
        to->angles[1] = MSG_ReadShort();
    if (bits & CM_ANGLE3)
        to->angles[2] = MSG_ReadShort();

    // read movement
    if (bits & CM_FORWARD)
        to->forwardmove = MSG_ReadShort();
    if (bits & CM_SIDE)
        to->sidemove = MSG_ReadShort();
    if (bits & CM_UP)
        to->upmove = MSG_ReadShort();

    // read buttons
    if (bits & CM_BUTTONS)
        to->buttons = MSG_ReadByte();

    if (bits & CM_IMPULSE)
        to->impulse = MSG_ReadByte();

    // read time to run command
    to->msec = MSG_ReadByte();

    // read the light level
    to->lightlevel = MSG_ReadByte();
}

#if USE_CLIENT

/*
=================
MSG_ParseEntityBits

Returns the entity number and the header bits
=================
*/
int MSG_ParseEntityBits(int *bits)
{
    int         b, total;
    int         number;

    total = MSG_ReadByte();
    if (total & U_MOREBITS1) {
        b = MSG_ReadByte();
        total |= b << 8;
    }
    if (total & U_MOREBITS2) {
        b = MSG_ReadByte();
        total |= b << 16;
    }
    if (total & U_MOREBITS3) {
        b = MSG_ReadByte();
        total |= b << 24;
    }

    if (total & U_NUMBER16)
        number = MSG_ReadShort();
    else
        number = MSG_ReadByte();

    *bits = total;

    return number;
}

/*
==================
MSG_ParseDeltaEntity

Can go from either a baseline or a previous packet_entity
==================
*/
void MSG_ParseDeltaEntity(const entity_state_t *from,
                          entity_state_t *to,
                          int            number,
                          int            bits,
                          msgEsFlags_t   flags)
{
    if (!to) {
        Com_Error(ERR_DROP, "%s: NULL", __func__);
    }

    if (number < 1 || number >= MAX_EDICTS) {
        Com_Error(ERR_DROP, "%s: bad entity number: %d", __func__, number);
    }

    // set everything to the state we are delta'ing from
    if (!from) {
        memset(to, 0, sizeof(*to));
    } else if (to != from) {
        memcpy(to, from, sizeof(*to));
    }

    to->number = number;
    to->event = 0;

    if (!bits) {
        return;
    }

    // Model Indexes.
    if (bits & U_MODEL) {
        to->modelindex = MSG_ReadByte();
    }
    if (bits & U_MODEL2) {
        to->modelindex2 = MSG_ReadByte();
    }
    if (bits & U_MODEL3) {
        to->modelindex3 = MSG_ReadByte();
    }
    if (bits & U_MODEL4) {
        to->modelindex4 = MSG_ReadByte();
    }

    // Frame.
    if (bits & U_FRAME8)
        to->frame = MSG_ReadByte();
    if (bits & U_FRAME16)
        to->frame = MSG_ReadShort();

    // Skinnum.
    if ((bits & (U_SKIN8 | U_SKIN16)) == (U_SKIN8 | U_SKIN16))  //used for laser colors
        to->skinnum = MSG_ReadLong();
    else if (bits & U_SKIN8)
        to->skinnum = MSG_ReadByte();
    else if (bits & U_SKIN16)
        to->skinnum = MSG_ReadWord();

    // Effects.
    if ((bits & (U_EFFECTS8 | U_EFFECTS16)) == (U_EFFECTS8 | U_EFFECTS16))
        to->effects = MSG_ReadLong();
    else if (bits & U_EFFECTS8)
        to->effects = MSG_ReadByte();
    else if (bits & U_EFFECTS16)
        to->effects = MSG_ReadWord();

    // RenderFX.
    if ((bits & (U_RENDERFX8 | U_RENDERFX16)) == (U_RENDERFX8 | U_RENDERFX16))
        to->renderfx = MSG_ReadLong();
    else if (bits & U_RENDERFX8)
        to->renderfx = MSG_ReadByte();
    else if (bits & U_RENDERFX16)
        to->renderfx = MSG_ReadWord();

    // Origin.
    if (bits & U_ORIGIN_X)
        to->origin[0] = MSG_ReadFloat();
    if (bits & U_ORIGIN_Y) {
        to->origin[1] = MSG_ReadFloat();
    }
    if (bits & U_ORIGIN_Z) {
        to->origin[2] = MSG_ReadFloat();
    }

    // Angle.
    if (bits & U_ANGLE16) {
        if (bits & U_ANGLE_X)
            to->angles[0] = MSG_ReadFloat();
        if (bits & U_ANGLE_Y)
            to->angles[1] = MSG_ReadFloat();
        if (bits & U_ANGLE_Z)
            to->angles[2] = MSG_ReadFloat();
    }

    // Old Origin.
    if (bits & U_OLDORIGIN) {
        to->old_origin = MSG_ReadPosition(); // MSG: !! ReadPos
    }

    // Sound.
    if (bits & U_SOUND) {
        to->sound = MSG_ReadByte();
    }

    // Event.
    if (bits & U_EVENT) {
        to->event = MSG_ReadByte();
    }

    // Solid.
    if (bits & U_SOLID) {
        to->solid = MSG_ReadLong();
    }
}

#endif // USE_CLIENT

#if USE_CLIENT

/*
===================
MSG_ParseDeltaPlayerstate_Default
===================
*/
void MSG_ParseDeltaPlayerstate_Default(const player_state_t *from,
                                       player_state_t *to,
                                       int            flags)
{
    int         i;
    int         statbits;

    if (!to) {
        Com_Error(ERR_DROP, "%s: NULL", __func__);
    }

    // clear to old value before delta parsing
    if (!from) {
        memset(to, 0, sizeof(*to));
    } else if (to != from) {
        memcpy(to, from, sizeof(*to));
    }

    //
    // parse the pm_state_t
    //
    // PM Type.
    if (flags & PS_M_TYPE)
        to->pmove.type = (pm_type_t)MSG_ReadByte(); // CPP: Cast

    // Origin.
    if (flags & PS_M_ORIGIN) {
        to->pmove.origin[0] = MSG_ReadFloat();
        to->pmove.origin[1] = MSG_ReadFloat();
        to->pmove.origin[2] = MSG_ReadFloat();
    }

    // Velocity.
    if (flags & PS_M_VELOCITY) {
        to->pmove.velocity[0] = MSG_ReadFloat();
        to->pmove.velocity[1] = MSG_ReadFloat();
        to->pmove.velocity[2] = MSG_ReadFloat();
    }

    // Time.
    if (flags & PS_M_TIME)
        to->pmove.time = MSG_ReadShort();

    // PM Flags.
    if (flags & PS_M_FLAGS)
        to->pmove.flags = MSG_ReadShort();

    // PM Gravity.
    if (flags & PS_M_GRAVITY)
        to->pmove.gravity = MSG_ReadShort();

    // PM Delta Angles.
    if (flags & PS_M_DELTA_ANGLES) {
        to->pmove.delta_angles[0] = MSG_ReadShort();
        to->pmove.delta_angles[1] = MSG_ReadShort();
        to->pmove.delta_angles[2] = MSG_ReadShort();
    }

    //
    // parse the rest of the player_state_t
    //
    // View Offset.
    if (flags & PS_VIEWOFFSET) {
        to->viewoffset[0] = MSG_ReadChar() * 0.25f;
        to->viewoffset[1] = MSG_ReadChar() * 0.25f;
        to->viewoffset[2] = MSG_ReadChar() * 0.25f;
    }

    // View Angles.
    if (flags & PS_VIEWANGLES) {
        to->viewAngles[0] = MSG_ReadAngle16();
        to->viewAngles[1] = MSG_ReadAngle16();
        to->viewAngles[2] = MSG_ReadAngle16();
    }

    // Kick Angles.
    if (flags & PS_KICKANGLES) {
        to->kickAngles[0] = MSG_ReadChar() * 0.25f;
        to->kickAngles[1] = MSG_ReadChar() * 0.25f;
        to->kickAngles[2] = MSG_ReadChar() * 0.25f;
    }

    // Weapon Index.
    if (flags & PS_WEAPONINDEX) {
        to->gunindex = MSG_ReadByte();
    }

    // Weapon Frame.
    if (flags & PS_WEAPONFRAME) {
        to->gunframe = MSG_ReadByte();
        to->gunoffset[0] = MSG_ReadChar() * 0.25f;
        to->gunoffset[1] = MSG_ReadChar() * 0.25f;
        to->gunoffset[2] = MSG_ReadChar() * 0.25f;
        to->gunangles[0] = MSG_ReadChar() * 0.25f;
        to->gunangles[1] = MSG_ReadChar() * 0.25f;
        to->gunangles[2] = MSG_ReadChar() * 0.25f;
    }

    // Blend.
    if (flags & PS_BLEND) {
        to->blend[0] = MSG_ReadByte() / 255.0f;
        to->blend[1] = MSG_ReadByte() / 255.0f;
        to->blend[2] = MSG_ReadByte() / 255.0f;
        to->blend[3] = MSG_ReadByte() / 255.0f;
    }

    // FOV.
    if (flags & PS_FOV)
        to->fov = MSG_ReadByte();

    // RDFlags.
    if (flags & PS_RDFLAGS)
        to->rdflags = MSG_ReadByte();

    // parse stats
    statbits = MSG_ReadLong();
    for (i = 0; i < MAX_STATS; i++)
        if (statbits & (1 << i))
            to->stats[i] = MSG_ReadShort();
}


/*
===================
MSG_ParseDeltaPlayerstate_Default
===================
*/
void MSG_ParseDeltaPlayerstate_Enhanced(const player_state_t    *from,
                                        player_state_t    *to,
                                        int               flags,
                                        int               extraflags)
{
    int         i;
    int         statbits;

    if (!to) {
        Com_Error(ERR_DROP, "%s: NULL", __func__);
    }

    // clear to old value before delta parsing
    if (!from) {
        memset(to, 0, sizeof(*to));
    } else if (to != from) {
        memcpy(to, from, sizeof(*to));
    }

    //
    // parse the pm_state_t
    //
    // PM Type
    if (flags & PS_M_TYPE)
        to->pmove.type = (pm_type_t)MSG_ReadByte(); // CPP: Cast

    // Origin X Y.
    if (flags & PS_M_ORIGIN) {
        to->pmove.origin[0] = MSG_ReadFloat();
        to->pmove.origin[1] = MSG_ReadFloat();
    }

    // Origin Z.
    if (extraflags & EPS_M_ORIGIN2) {
        to->pmove.origin[2] = MSG_ReadFloat();
    }

    // Velocity X Y.
    if (flags & PS_M_VELOCITY) {
        to->pmove.velocity[0] = MSG_ReadFloat();
        to->pmove.velocity[1] = MSG_ReadFloat();
    }

    // Velocity Z.
    if (extraflags & EPS_M_VELOCITY2) {
        to->pmove.velocity[2] = MSG_ReadFloat();
    }

    // PM Time.
    if (flags & PS_M_TIME)
        to->pmove.time = MSG_ReadShort();

    // PM Flags.
    if (flags & PS_M_FLAGS)
        to->pmove.flags = MSG_ReadShort();

    // PM Gravity.
    if (flags & PS_M_GRAVITY)
        to->pmove.gravity = MSG_ReadShort();

    // PM Delta Angles.
    if (flags & PS_M_DELTA_ANGLES) {
        to->pmove.delta_angles[0] = MSG_ReadShort();
        to->pmove.delta_angles[1] = MSG_ReadShort();
        to->pmove.delta_angles[2] = MSG_ReadShort();
    }

    //
    // parse the rest of the player_state_t
    //
    // View Offset.
    if (flags & PS_VIEWOFFSET) {
        to->viewoffset[0] = MSG_ReadChar() * 0.25f;
        to->viewoffset[1] = MSG_ReadChar() * 0.25f;
        to->viewoffset[2] = MSG_ReadChar() * 0.25f;
    }

    // View Angles X Y.
    if (flags & PS_VIEWANGLES) {
        to->viewAngles[0] = MSG_ReadAngle16();
        to->viewAngles[1] = MSG_ReadAngle16();
    }
    
    // ViewAngles Z.
    if (extraflags & EPS_VIEWANGLE2) {
        to->viewAngles[2] = MSG_ReadAngle16();
    }

    // Kick Angles.
    if (flags & PS_KICKANGLES) {
        to->kickAngles[0] = MSG_ReadChar() * 0.25f;
        to->kickAngles[1] = MSG_ReadChar() * 0.25f;
        to->kickAngles[2] = MSG_ReadChar() * 0.25f;
    }
    
    // Weapon Index.
    if (flags & PS_WEAPONINDEX) {
        to->gunindex = MSG_ReadByte();
    }

    // Weapon Frame.
    if (flags & PS_WEAPONFRAME) {
        to->gunframe = MSG_ReadByte();
    }

    // Gun Offset.
    if (extraflags & EPS_GUNOFFSET) {
        to->gunoffset[0] = MSG_ReadChar() * 0.25f;
        to->gunoffset[1] = MSG_ReadChar() * 0.25f;
        to->gunoffset[2] = MSG_ReadChar() * 0.25f;
    }

    // Gun Angles.
    if (extraflags & EPS_GUNANGLES) {
        to->gunangles[0] = MSG_ReadChar() * 0.25f;
        to->gunangles[1] = MSG_ReadChar() * 0.25f;
        to->gunangles[2] = MSG_ReadChar() * 0.25f;
    }

    // Blend.
    if (flags & PS_BLEND) {
        to->blend[0] = MSG_ReadByte() / 255.0f;
        to->blend[1] = MSG_ReadByte() / 255.0f;
        to->blend[2] = MSG_ReadByte() / 255.0f;
        to->blend[3] = MSG_ReadByte() / 255.0f;
    }

    // FOV.
    if (flags & PS_FOV)
        to->fov = MSG_ReadByte();

    // RDFlags.
    if (flags & PS_RDFLAGS)
        to->rdflags = MSG_ReadByte();

    // Parse Stats.
    if (extraflags & EPS_STATS) {
        statbits = MSG_ReadLong();
        for (i = 0; i < MAX_STATS; i++) {
            if (statbits & (1 << i)) {
                to->stats[i] = MSG_ReadShort();
            }
        }
    }

}

#endif // USE_CLIENT


/*
==============================================================================

            DEBUGGING STUFF

==============================================================================
*/

#ifdef _DEBUG

#define SHOWBITS(x) Com_LPrintf(PRINT_DEVELOPER, x " ")

#if USE_CLIENT

void MSG_ShowDeltaPlayerstateBits_Default(int flags)
{
#define S(b,s) if(flags&PS_##b) SHOWBITS(s)
    S(M_TYPE,           "pmove.type");
    S(M_ORIGIN,         "pmove.origin");
    S(M_VELOCITY,       "pmove.velocity");
    S(M_TIME,           "pmove.time");
    S(M_FLAGS,          "pmove.flags");
    S(M_GRAVITY,        "pmove.gravity");
    S(M_DELTA_ANGLES,   "pmove.delta_angles");
    S(VIEWOFFSET,       "viewoffset");
    S(VIEWANGLES,       "viewAngles");
    S(KICKANGLES,       "kickAngles");
    S(WEAPONINDEX,      "gunindex");
    S(WEAPONFRAME,      "gunframe");
    S(BLEND,            "blend");
    S(FOV,              "fov");
    S(RDFLAGS,          "rdflags");
#undef S
}

void MSG_ShowDeltaPlayerstateBits_Enhanced(int flags, int extraflags)
{
#define SP(b,s) if(flags&PS_##b) SHOWBITS(s)
#define SE(b,s) if(extraflags&EPS_##b) SHOWBITS(s)
    SP(M_TYPE,          "pmove.type");
    SP(M_ORIGIN,        "pmove.origin[0,1]");
    SE(M_ORIGIN2,       "pmove.origin[2]");
    SP(M_VELOCITY,      "pmove.velocity[0,1]");
    SE(M_VELOCITY2,     "pmove.velocity[2]");
    SP(M_TIME,          "pmove.time");
    SP(M_FLAGS,         "pmove.flags");
    SP(M_GRAVITY,       "pmove.gravity");
    SP(M_DELTA_ANGLES,  "pmove.delta_angles");
    SP(VIEWOFFSET,      "viewoffset");
    SP(VIEWANGLES,      "viewAngles[0,1]");
    SE(VIEWANGLE2,      "viewAngles[2]");
    SP(KICKANGLES,      "kickAngles");
    SP(WEAPONINDEX,     "gunindex");
    SP(WEAPONFRAME,     "gunframe");
    SE(GUNOFFSET,       "gunoffset");
    SE(GUNANGLES,       "gunangles");
    SP(BLEND,           "blend");
    SP(FOV,             "fov");
    SP(RDFLAGS,         "rdflags");
    SE(STATS,           "stats");
#undef SP
#undef SE
}

void MSG_ShowDeltaUsercmdBits_Enhanced(int bits)
{
    if (!bits) {
        SHOWBITS("<none>");
        return;
    }

#define S(b,s) if(bits&CM_##b) SHOWBITS(s)
    S(ANGLE1,   "angle1");
    S(ANGLE2,   "angle2");
    S(ANGLE3,   "angle3");
    S(FORWARD,  "forward");
    S(SIDE,     "side");
    S(UP,       "up");
    S(BUTTONS,  "buttons");
    S(IMPULSE,  "msec");
#undef S
}

#endif // USE_CLIENT

#if USE_CLIENT

void MSG_ShowDeltaEntityBits(int bits)
{
#define S(b,s) if(bits&U_##b) SHOWBITS(s)
    S(MODEL, "modelindex");
    S(MODEL2, "modelindex2");
    S(MODEL3, "modelindex3");
    S(MODEL4, "modelindex4");

    if (bits & U_FRAME8)
        SHOWBITS("frame8");
    if (bits & U_FRAME16)
        SHOWBITS("frame16");

    if ((bits & (U_SKIN8 | U_SKIN16)) == (U_SKIN8 | U_SKIN16))
        SHOWBITS("skinnum32");
    else if (bits & U_SKIN8)
        SHOWBITS("skinnum8");
    else if (bits & U_SKIN16)
        SHOWBITS("skinnum16");

    if ((bits & (U_EFFECTS8 | U_EFFECTS16)) == (U_EFFECTS8 | U_EFFECTS16))
        SHOWBITS("effects32");
    else if (bits & U_EFFECTS8)
        SHOWBITS("effects8");
    else if (bits & U_EFFECTS16)
        SHOWBITS("effects16");

    if ((bits & (U_RENDERFX8 | U_RENDERFX16)) == (U_RENDERFX8 | U_RENDERFX16))
        SHOWBITS("renderfx32");
    else if (bits & U_RENDERFX8)
        SHOWBITS("renderfx8");
    else if (bits & U_RENDERFX16)
        SHOWBITS("renderfx16");

    S(ORIGIN_X, "origin[0]");
    S(ORIGIN_Y, "origin[1]");
    S(ORIGIN_Z, "origin[2]");
    S(ANGLE_X, "angles[0]");
    S(ANGLE_Y, "angles[1]");
    S(ANGLE_Z, "angles[2]");
    S(OLDORIGIN, "old_origin");
    S(SOUND, "sound");
    S(EVENT, "event");
    S(SOLID, "solid");
#undef S
}

void MSG_ShowDeltaPlayerstateBits_Packet(int flags)
{
#define S(b,s) if(flags&PPS_##b) SHOWBITS(s)
    S(M_TYPE,       "pmove.type");
    S(M_ORIGIN,     "pmove.origin[0,1]");
    S(M_ORIGIN2,    "pmove.origin[2]");
    S(VIEWOFFSET,   "viewoffset");
    S(VIEWANGLES,   "viewAngles[0,1]");
    S(VIEWANGLE2,   "viewAngles[2]");
    S(KICKANGLES,   "kickAngles");
    S(WEAPONINDEX,  "gunindex");
    S(WEAPONFRAME,  "gunframe");
    S(GUNOFFSET,    "gunoffset");
    S(GUNANGLES,    "gunangles");
    S(BLEND,        "blend");
    S(FOV,          "fov");
    S(RDFLAGS,      "rdflags");
    S(STATS,        "stats");
#undef S
}

const char *MSG_ServerCommandString(int cmd)
{
    switch (cmd) {
    case -1: return "END OF MESSAGE";
    default: return "UNKNOWN COMMAND";
#define S(x) case svc_##x: return "svc_" #x;
        S(bad)
        // N&C: Protocol todo: add a game callback for this...?
        //S(muzzleflash)
        //S(muzzleflash2)
        //S(temp_entity)
        //S(layout)
        //S(inventory)
        S(nop)
        S(disconnect)
        S(reconnect)
        S(sound)
        S(print)
        S(stufftext)
        S(serverdata)
        S(configstring)
        S(spawnbaseline)
        S(centerprint)
        S(download)
        S(playerinfo)
        S(packetentities)
        S(deltapacketentities)
        S(frame)
        S(zpacket)
        S(zdownload)
        S(gamestate)
#undef S
    }
}

#endif // USE_CLIENT

#endif // _DEBUG

