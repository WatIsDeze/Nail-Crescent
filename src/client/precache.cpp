/*
Copyright (C) 1997-2001 Id Software, Inc.
Copyright (C) 2019, NVIDIA CORPORATION. All rights reserved.

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

//
// cl_precache.c
//

#include "client.h"
#include "client/gamemodule.h"
#include "client/sound/vorbis.h"

/*
================
CL_ParsePlayerSkin

Breaks up playerskin into name (optional), model and skin components.
If model or skin are found to be invalid, replaces them with sane defaults.
================
*/
void CL_ParsePlayerSkin(char *name, char *model, char *skin, const char *s)
{
    size_t len;
    char *t;

    // configstring parsing guarantees that playerskins can never
    // overflow, but still check the length to be entirely fool-proof
    len = strlen(s);
    if (len >= MAX_QPATH) {
        Com_Error(ERR_DROP, "%s: oversize playerskin", __func__);
    }

    // isolate the player's name
    t = (char*)strchr(s, '\\'); // CPP: WARNING: Cast from const char* to char*
    if (t) {
        len = t - s;
        strcpy(model, t + 1);
    } else {
        len = 0;
        strcpy(model, s);
    }

    // copy the player's name
    if (name) {
        memcpy(name, s, len);
        name[len] = 0;
    }

    // isolate the model name
    t = strchr(model, '/');
    if (!t)
        t = strchr(model, '\\');
    if (!t)
        goto default_model;
    *t = 0;

    // isolate the skin name
    strcpy(skin, t + 1);

    // fix empty model to male
    if (t == model)
        strcpy(model, "male");

    // apply restrictions on skins
    if (cl_noskins->integer == 2 || !COM_IsPath(skin))
        goto default_skin;

    if (cl_noskins->integer || !COM_IsPath(model))
        goto default_model;

    return;

default_skin:
    if (!Q_stricmp(model, "female")) {
        strcpy(model, "female");
        strcpy(skin, "athena");
    } else {
default_model:
        strcpy(model, "male");
        strcpy(skin, "grunt");
    }
}


/*
=================
CL_RegisterBspModels

Registers main BSP file and inline models
=================
*/
void CL_RegisterBspModels(void)
{
    qerror_t ret;
    char *name;
    int i;

    ret = BSP_Load(cl.configstrings[CS_MODELS + 1], &cl.bsp);
    if (cl.bsp == NULL) {
        Com_Error(ERR_DROP, "Couldn't load %s: %s",
                  cl.configstrings[CS_MODELS + 1], Q_ErrorString(ret));
    }

#if USE_MAPCHECKSUM
    if (cl.bsp->checksum != atoi(cl.configstrings[CS_MAPCHECKSUM])) {
        if (cls.demo.playback) {
            Com_WPrintf("Local map version differs from demo: %i != %s\n",
                        cl.bsp->checksum, cl.configstrings[CS_MAPCHECKSUM]);
        } else {
            Com_Error(ERR_DROP, "Local map version differs from server: %i != %s",
                      cl.bsp->checksum, cl.configstrings[CS_MAPCHECKSUM]);
        }
    }
#endif

    for (i = 1; i < MAX_MODELS; i++) {
        name = cl.configstrings[CS_MODELS + i];
        if (!name[0]) {
            break;
        }
        if (name[0] == '*')
            cl.model_clip[i] = BSP_InlineModel(cl.bsp, name);
        else
            cl.model_clip[i] = NULL;
    }
}


/*
=================
CL_PrepareMedia

Call before entering a new level, or after changing dlls
=================
*/
void CL_PrepareMedia(void)
{
    if (!cls.ref_initialized)
        return;
    if (!cl.mapname[0])
        return;     // no map loaded


    // register models, pics, and skins
    R_BeginRegistration(cl.mapname);
    // register sounds.
    S_BeginRegistration();

    // N&C: Pass over loading to the CG Module so it can actively
    // manage the load state. This is useful for load screen information.
    CL_GM_LoadWorldMedia();
    
    // The sound engine can now free unneeded stuff
    S_EndRegistration();

    // the renderer can now free unneeded stuff
    R_EndRegistration(cl.mapname);

    // clear any lines of console text
    Con_ClearNotify_f();

    SCR_UpdateScreen();

	int cdtrack = atoi(cl.configstrings[CS_CDTRACK]);
    OGG_PlayTrack(cdtrack);
}

/*
=================
CL_UpdateConfigstring

A configstring update has been parsed.
=================
*/
void CL_UpdateConfigstring(int index)
{
    const char *s = cl.configstrings[index];

    // Let the CG Module handle the string.
    // If it returns TRUE it has succeeded.
    // If it returns false, we move on.
    if (CL_GM_UpdateConfigString(index, s)) {
        // We're done here.
        return;
    }

    if (index == CS_MAXCLIENTS) {
        cl.maxclients = atoi(s);
        return;
    }

    if (index == CS_MODELS + 1) {
        size_t len = strlen(s);

        if (len <= 9) {
            Com_Error(ERR_DROP, "%s: bad world model: %s", __func__, s);
        }
        memcpy(cl.mapname, s + 5, len - 9);   // skip "maps/"
        cl.mapname[len - 9] = 0; // cut off ".bsp"
        return;
    }

    // Anything processed after this if statement is done so only when we're
    // not fully precached yet. 
    if (cls.state < ca_precached) {
        return;
    }

    // TODO: Move all over to CG Module and ONLY
    // handle the BSP Model loading. EXAMPLE:
    //if (index >= CS_MODELS + 2 && index < CS_MODELS + MAX_MODELS) {
    //    if (*s == '*') {
    //        int i = index - CS_MODELS;
    //        cl.model_draw[i] = R_RegisterModel(s);
    //        cl.model_clip[i] = BSP_InlineModel(cl.bsp, s);
    //    }
    //    return;
    //}
    
    if (index >= CS_MODELS + 2 && index < CS_MODELS + MAX_MODELS) {
        int i = index - CS_MODELS;

        cl.model_draw[i] = R_RegisterModel(s);
        if (*s == '*')
            cl.model_clip[i] = BSP_InlineModel(cl.bsp, s);
        else
            cl.model_clip[i] = NULL;
        return;
    }
    if (index >= CS_SOUNDS && index < CS_SOUNDS + MAX_SOUNDS) {
        cl.sound_precache[index - CS_SOUNDS] = S_RegisterSound(s);
        return;
    }
    if (index >= CS_IMAGES && index < CS_IMAGES + MAX_IMAGES) {
        cl.image_precache[index - CS_IMAGES] = R_RegisterPic2(s);
        return;
    }
}
