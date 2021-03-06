// LICENSE HERE.

//
// svgame/weapons/supershotgun.c
//
//
// Super Shotgun weapon code.
//

// Include local game header.
#include "../g_local.h"

// Include player headers.
#include "../player/animations.h"
#include "../player/weapons.h"

// Include super shotgun weapon header.
#include "supershotgun.h"

//
//======================================================================
//
// SUPERSHOTGUN
//
//======================================================================
//
void weapon_supershotgun_fire(edict_t* ent)
{
    vec3_t      start;
    vec3_t      forward, right;
    vec3_t      offset;
    vec3_t      v;
    int         damage = 6;
    int         kick = 12;

    AngleVectors(ent->client->v_angle, forward, right, NULL);

    VectorScale(forward, -2, ent->client->kick_origin);
    ent->client->kick_angles[0] = -2;

    VectorSet(offset, 0, 8, ent->viewheight - 8);
    P_ProjectSource(ent->client, ent->s.origin, offset, forward, right, start);

    if (is_quad) {
        damage *= 4;
        kick *= 4;
    }

    v[PITCH] = ent->client->v_angle[PITCH];
    v[YAW] = ent->client->v_angle[YAW] - 5;
    v[ROLL] = ent->client->v_angle[ROLL];
    AngleVectors(v, forward, NULL, NULL);
    fire_shotgun(ent, start, forward, damage, kick, DEFAULT_SHOTGUN_HSPREAD, DEFAULT_SHOTGUN_VSPREAD, DEFAULT_SSHOTGUN_COUNT / 2, MOD_SSHOTGUN);
    v[YAW] = ent->client->v_angle[YAW] + 5;
    AngleVectors(v, forward, NULL, NULL);
    fire_shotgun(ent, start, forward, damage, kick, DEFAULT_SHOTGUN_HSPREAD, DEFAULT_SHOTGUN_VSPREAD, DEFAULT_SSHOTGUN_COUNT / 2, MOD_SSHOTGUN);

    // send muzzle flash
    gi.WriteByte(svg_muzzleflash);
    gi.WriteShort(ent - g_edicts);
    gi.WriteByte(MZ_SSHOTGUN | is_silenced);
    gi.multicast(ent->s.origin, MULTICAST_PVS);

    ent->client->ps.gunframe++;
    PlayerNoise(ent, start, PNOISE_WEAPON);

    if (!((int)dmflags->value & DF_INFINITE_AMMO))
        ent->client->pers.inventory[ent->client->ammo_index] -= 2;
}

void Weapon_SuperShotgun(edict_t* ent)
{
    static int  pause_frames[] = { 29, 42, 57, 0 };
    static int  fire_frames[] = { 7, 0 };

    Weapon_Generic(ent, 6, 17, 57, 61, pause_frames, fire_frames, weapon_supershotgun_fire);
}