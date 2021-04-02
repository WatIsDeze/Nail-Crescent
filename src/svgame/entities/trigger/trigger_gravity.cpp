// LICENSE HERE.

//
// svgame/entities/trigger_gravity.c
//
//
// trigger_gravity entity implementation.
//

#include "../../g_local.h"      // SVGame funcs.
#include "../../utils.h"        // Util funcs.
#include "../../brushfuncs.h"   // Brush funcs.
#include "../../trigger.h"

//=====================================================
/*QUAKED trigger_gravity (.5 .5 .5) ?
Changes the touching entites gravity to
the value of "gravity".  1.0 is standard
gravity for the level.
*/

void trigger_gravity_touch(edict_t* self, edict_t* other, cplane_t* plane, csurface_t* surf)
{
    other->gravity = self->gravity;
}

void SP_trigger_gravity(edict_t* self)
{
    if (st.gravity == 0) {
        gi.dprintf("trigger_gravity without gravity set at %s\n", Vec3ToString(self->s.origin));
        G_FreeEdict(self);
        return;
    }

    InitTrigger(self);
    self->gravity = atoi(st.gravity);
    self->touch = trigger_gravity_touch;
}