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

#ifndef SHAREDGAME_PMOVE_H
#define SHAREDGAME_PMOVE_H

/*
==============================================================

PLAYER MOVEMENT CODE

Common between server and client so prediction matches

==============================================================
*/

//
// The server has a copy of this struct.
// Any additional game data should be added at THE BOTTOM.
//
typedef struct {
    qboolean    qwmode;
    qboolean    airaccelerate;
    qboolean    strafehack;
    qboolean    flyhack;
    qboolean    waterhack;
    float       speedmult;
    float       watermult;
    float       maxspeed;
    float       friction;
    float       waterfriction;
    float       flyfriction;

    //--------------------------------------
    // Additional game data here.
} pmoveParams_t;

void PMove(pmove_t* pmove, pmoveParams_t* params);

void PMoveInit(pmoveParams_t* pmp);
void PMoveEnableQW(pmoveParams_t* pmp);

#endif // PMOVE_H
