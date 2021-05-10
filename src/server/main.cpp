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

#include "server.h"
#include "client/input.h"

LIST_DECL(sv_masterlist);   // address of group servers
LIST_DECL(sv_banlist);
LIST_DECL(sv_blacklist);
LIST_DECL(sv_cmdlist_connect);
LIST_DECL(sv_cmdlist_begin);
LIST_DECL(sv_filterlist);
LIST_DECL(sv_clientlist);   // linked list of non-free clients

client_t    *sv_client;         // current client
entity_t     *sv_player;         // current client edict

qboolean     sv_pending_autosave = 0;

cvar_t  *sv_enforcetime;
cvar_t  *sv_allow_nodelta;

cvar_t  *sv_timeout;            // seconds without any message
cvar_t  *sv_zombietime;         // seconds to sink messages after disconnect
cvar_t  *sv_ghostime;
cvar_t  *sv_idlekick;

cvar_t  *sv_password;
cvar_t  *sv_reserved_password;

cvar_t  *sv_force_reconnect;
cvar_t  *sv_show_name_changes;

cvar_t  *sv_airaccelerate;
cvar_t  *sv_qwmod;              // atu QW Physics modificator
cvar_t  *sv_novis;

cvar_t  *sv_maxclients;
cvar_t  *sv_reserved_slots;
cvar_t  *sv_showclamp;
cvar_t  *sv_locked;
cvar_t  *sv_downloadserver;
cvar_t  *sv_redirect_address;

cvar_t  *sv_hostname;
cvar_t  *sv_public;            // should heartbeats be sent

#ifdef _DEBUG
cvar_t  *sv_debug;
cvar_t  *sv_pad_packets;
#endif
cvar_t  *sv_lan_force_rate;
cvar_t  *sv_calcpings_method;
cvar_t  *sv_changemapcmd;

cvar_t  *sv_strafejump_hack;
cvar_t  *sv_waterjump_hack;
#if USE_PACKETDUP
cvar_t  *sv_packetdup_hack;
#endif
cvar_t  *sv_allow_map;
#if !USE_CLIENT
cvar_t  *sv_recycle;
#endif
cvar_t  *sv_enhanced_setplayer;

cvar_t  *sv_iplimit;
cvar_t  *sv_status_limit;
cvar_t  *sv_status_show;
cvar_t  *sv_uptime;
cvar_t  *sv_auth_limit;
cvar_t  *sv_rcon_limit;
cvar_t  *sv_namechange_limit;

cvar_t  *sv_restrict_rtx;

cvar_t  *sv_allow_unconnected_cmds;

cvar_t  *map_override_path;

qboolean sv_registered;

//============================================================================

void SV_RemoveClient(client_t *client)
{
    if (client->msg_pool) {
        SV_ShutdownClientSend(client);
    }

    if (client->netchan) {
        Netchan_Close(client->netchan);
        client->netchan = NULL;
    }

    // unlink them from active client list, but don't clear the list entry
    // itself to make code that traverses client list in a loop happy!
    List_Remove(&client->entry);

    Com_DPrintf("Going from ConnectionState::Zombie to ConnectionState::Free for %s\n", client->name);

    client->connectionState = ConnectionState::Free;    // can now be reused
    client->name[0] = 0;
}

void SV_CleanClient(client_t *client)
{
    int i;

    // close any existing donwload
    SV_CloseDownload(client);

    if (client->versionString) {
        Z_Free(client->versionString);
        client->versionString = NULL;
    }

    // free entityBaselines allocated for this client
    for (i = 0; i < SV_BASELINES_CHUNKS; i++) {
        if (client->entityBaselines[i]) {
            Z_Free(client->entityBaselines[i]);
            client->entityBaselines[i] = NULL;
        }
    }
}

//
//===============
// SV_GetServerState
// 
// Returns the current state of the server.
//===============
//
uint32_t SV_GetServerState (void) {
    return sv.serverState;
}

//
//===============
// SV_SetServerState
// 
// Sets the current state of the server.
//===============
//
void SV_SetServerState (uint32_t serverState) {
    sv.serverState = serverState;
}

static void print_drop_reason(client_t *client, const char *reason, int32_t oldConnectionState)
{
    int announce = oldConnectionState == ConnectionState::Spawned ? 2 : 1;
    const char *prefix = " was dropped: ";

    // parse flags
    if (*reason == '!') {
        reason++;
        announce = 0;
    }
    if (*reason == '?') {
        reason++;
        prefix = " ";
    }

    if (announce == 2) {
        // announce to others
            SV_BroadcastPrintf(PRINT_HIGH, "%s%s%s\n",
                               client->name, prefix, reason);
    }

    if (announce)
        // print this to client as they will not receive broadcast
        SV_ClientPrintf(client, PRINT_HIGH, "%s%s%s\n",
                        client->name, prefix, reason);

    // print to server console
    if (COM_DEDICATED && client->netchan)
        Com_Printf("%s[%s]%s%s\n", client->name,
                   NET_AdrToString(&client->netchan->remoteAddress),
                   prefix, reason);
}

/*
=====================
SV_DropClient

Called when the player is totally leaving the server, either willingly
or unwillingly.  This is NOT called if the entire server is quiting
or crashing.
=====================
*/
void SV_DropClient(client_t *client, const char *reason)
{
    int32_t oldConnectionState;

    if (client->connectionState <= ConnectionState::Zombie)
        return; // called recursively?

    oldConnectionState = client->connectionState;
    client->connectionState = ConnectionState::Zombie;        // become free in a few seconds
    client->lastMessage = svs.realtime;

    // print the reason
    if (reason)
        print_drop_reason(client, reason, oldConnectionState);

    // add the disconnect
    MSG_WriteByte(svc_disconnect);
    SV_ClientAddMessage(client, MSG_RELIABLE | MSG_CLEAR);

    if (oldConnectionState == ConnectionState::Spawned) {
        // call the prog function for removing a client
        // this will remove the body, among other things
        ge->ClientDisconnect(client->edict);
    }

    SV_CleanClient(client);

    Com_DPrintf("Going to ConnectionState::Zombie for %s\n", client->name);
}


//============================================================================

// highest power of two that avoids credit overflow for 1 day
#define CREDITS_PER_MSEC    32
#define CREDITS_PER_SEC     (CREDITS_PER_MSEC * 1000)

// allows rates up to 10,000 hits per second
#define RATE_LIMIT_SCALE    10000

/*
===============
SV_RateLimited

Implements simple token bucket filter. Inspired by xt_limit.c from the Linux
kernel. Returns true if limit is exceeded.
===============
*/
qboolean SV_RateLimited(RateLimit *r)
{
    r->credit += (svs.realtime - r->time) * CREDITS_PER_MSEC;
    r->time = svs.realtime;
    if (r->credit > r->credit_cap)
        r->credit = r->credit_cap;

    if (r->credit >= r->cost) {
        r->credit -= r->cost;
        return false;
    }

    return true;
}

/*
===============
SV_RateRecharge

Reverts the effect of SV_RateLimited.
===============
*/
void SV_RateRecharge(RateLimit *r)
{
    r->credit += r->cost;
    if (r->credit > r->credit_cap)
        r->credit = r->credit_cap;
}

static unsigned rate2credits(unsigned rate)
{
    if (rate > UINT_MAX / CREDITS_PER_SEC)
        return (rate / RATE_LIMIT_SCALE) * CREDITS_PER_SEC;

    return (rate * CREDITS_PER_SEC) / RATE_LIMIT_SCALE;
}

/*
===============
SV_RateInit

Full syntax is: <limit>[/<period>[sec|min|hour]][*<burst>]
===============
*/
void SV_RateInit(RateLimit *r, const char *s)
{
    unsigned limit, period, mult, burst, rate;
    char *p;

    limit = strtoul(s, &p, 10);
    if (*p == '/') {
        period = strtoul(p + 1, &p, 10);
        if (*p == 's' || *p == 'S') {
            mult = 1;
            p++;
        } else if (*p == 'm' || *p == 'M') {
            mult = 60;
            p++;
        } else if (*p == 'h' || *p == 'H') {
            mult = 60 * 60;
            p++;
        } else {
            // everything else are seconds
            mult = 1;
        }
        if (!period)
            period = 1;
    } else {
        // default period is one second
        period = 1;
        mult = 1;
    }

    if (!limit) {
        // unlimited
        memset(r, 0, sizeof(*r));
        return;
    }

    if (period > UINT_MAX / (RATE_LIMIT_SCALE * mult)) {
        Com_Printf("Period too large: %u\n", period);
        return;
    }

    rate = (RATE_LIMIT_SCALE * period * mult) / limit;

    p = strchr(p, '*');
    if (p) {
        burst = strtoul(p + 1, NULL, 10);
    } else {
        // default burst is 5 hits
        burst = 5;
    }

    if (burst > UINT_MAX / rate) {
        Com_Printf("Burst too large: %u\n", burst);
        return;
    }

    r->time = svs.realtime;
    r->credit = rate2credits(rate * burst);
    r->credit_cap = rate2credits(rate * burst);
    r->cost = rate2credits(rate);
}

AddressMatch *SV_MatchAddress(list_t *list, netadr_t *addr)
{
    AddressMatch *match;

    LIST_FOR_EACH(AddressMatch, match, list, entry) {
        if (NET_IsEqualBaseAdrMask(addr, &match->addr, &match->mask)) {
            match->hits++;
            match->time = time(NULL);
            return match;
        }
    }

    return NULL;
}

/*
==============================================================================

CONNECTIONLESS COMMANDS

==============================================================================
*/

/*
===============
SV_StatusString

Builds the string that is sent as heartbeats and status replies.
It is assumed that size of status buffer is at least SV_OUTPUTBUF_LENGTH!
===============
*/
static size_t SV_StatusString(char *status)
{
    char entry[MAX_STRING_CHARS];
    client_t *cl;
    size_t total, len;
    char *tmp = sv_maxclients->string;

    // XXX: ugly hack to hide reserved slots
    if (sv_reserved_slots->integer) {
        Q_snprintf(entry, sizeof(entry), "%d",
                   sv_maxclients->integer - sv_reserved_slots->integer);
        sv_maxclients->string = entry;
    }

    // add server info
    total = Cvar_BitInfo(status, CVAR_SERVERINFO);

    sv_maxclients->string = tmp;

    // add uptime
    if (sv_uptime->integer > 0) {
        if (sv_uptime->integer > 1) {
            len = Com_UptimeLong_m(entry, MAX_INFO_VALUE);
        } else {
            len = Com_Uptime_m(entry, MAX_INFO_VALUE);
        }
        if (total + 8 + len < MAX_INFO_STRING) {
            memcpy(status + total, "\\uptime\\", 8);
            memcpy(status + total + 8, entry, len);
            total += 8 + len;
        }
    }

    status[total++] = '\n';

    // add player list
    if (sv_status_show->integer > 1) {
        FOR_EACH_CLIENT(cl) {
            if (cl->connectionState == ConnectionState::Zombie) {
                continue;
            }
            len = Q_snprintf(entry, sizeof(entry),
                             "%i %i \"%s\"\n",
                             cl->edict->client->playerState.stats[STAT_FRAGS],
                             cl->ping, cl->name);
            if (len >= sizeof(entry)) {
                continue;
            }
            if (total + len >= SV_OUTPUTBUF_LENGTH) {
                break;        // can't hold any more
            }
            memcpy(status + total, entry, len);
            total += len;
        }
    }

    status[total] = 0;

    return total;
}

/*
================
SVC_Status

Responds with all the info that qplug or qspy can see
================
*/
static void SVC_Status(void)
{
    char    buffer[MAX_PACKETLEN_DEFAULT];
    size_t  len;

    if (!sv_status_show->integer) {
        return;
    }

    if (SV_RateLimited(&svs.ratelimit_status)) {
        Com_DPrintf("Dropping status request from %s\n",
                    NET_AdrToString(&net_from));
        return;
    }

    // write the packet header
    memcpy(buffer, "\xff\xff\xff\xffprint\n", 10);
    len = 10;

    len += SV_StatusString(buffer + len);

    // send the datagram
    NET_SendPacket(NS_SERVER, buffer, len, &net_from);
}

/*
================
SVC_Ack

================
*/
static void SVC_Ack(void)
{
    master_t *m;

    FOR_EACH_MASTER(m) {
        if (!m->adr.port) {
            continue;
        }
        if (NET_IsEqualBaseAdr(&m->adr, &net_from)) {
            Com_DPrintf("Ping acknowledge from %s\n",
                        NET_AdrToString(&net_from));
            m->last_ack = svs.realtime;
            break;
        }
    }
}

/*
================
SVC_Info

Responds with short info for broadcast scans
The second parameter should be the current protocol version number.
================
*/
static void SVC_Info(void)
{
    char    buffer[MAX_QPATH+10];
    size_t  len;
    int     version;

    if (sv_maxclients->integer == 1)
        return; // ignore in single player

    version = atoi(Cmd_Argv(1));
    if (version != PROTOCOL_VERSION_NAC)
        return; // ignore invalid versions

    len = Q_scnprintf(buffer, sizeof(buffer),
                      "\xff\xff\xff\xffinfo\n%16s %8s %2i/%2i\n",
                      sv_hostname->string, sv.name, SV_CountClients(),
                      sv_maxclients->integer - sv_reserved_slots->integer);

    NET_SendPacket(NS_SERVER, buffer, len, &net_from);
}

/*
================
SVC_Ping

Just responds with an acknowledgement
================
*/
static void SVC_Ping(void)
{
    OOB_PRINT(NS_SERVER, &net_from, "ack");
}

/*
=================
SVC_GetChallenge

Returns a challenge number that can be used
in a subsequent client_connect command.
We do this to prevent denial of service attacks that
flood the server with invalid connection IPs.  With a
challenge, they must give a valid IP address.
=================
*/
static void SVC_GetChallenge(void)
{
    int         i, oldest;
    unsigned    challenge;
    unsigned    oldestTime;

    oldest = 0;
    oldestTime = 0xffffffff;

    // see if we already have a challenge for this ip
    for (i = 0; i < MAX_CHALLENGES; i++) {
        if (NET_IsEqualBaseAdr(&net_from, &svs.challenges[i].adr))
            break;
        if (svs.challenges[i].time > com_eventTime) {
            svs.challenges[i].time = com_eventTime;
        }
        if (svs.challenges[i].time < oldestTime) {
            oldestTime = svs.challenges[i].time;
            oldest = i;
        }
    }

    challenge = ((rand() << 16) | rand()) & 0x7fffffff;
    if (i == MAX_CHALLENGES) {
        // overwrite the oldest
        svs.challenges[oldest].challenge = challenge;
        svs.challenges[oldest].adr = net_from;
        svs.challenges[oldest].time = com_eventTime;
    } else {
        svs.challenges[i].challenge = challenge;
        svs.challenges[i].time = com_eventTime;
    }

    // send it back
    Netchan_OutOfBand(NS_SERVER, &net_from,
                      "challenge %i p=%i", challenge, PROTOCOL_VERSION_NAC);
}

/*
==================
SVC_DirectConnect

A connection request that did not come from the master
==================
*/

typedef struct {
    int         protocol;   // major version
    int         version;    // minor version
    int         qport;
    int         challenge;

    int         maxlength;
    int         nctype;
    qboolean    has_zlib;

    int         reserved;   // hidden client slots
    char        reconnectKey[16];
    char        reconnectValue[16];
} conn_params_t;

#define __reject(...) \
    Netchan_OutOfBand(NS_SERVER, &net_from, "print\n" __VA_ARGS__)

// small hack to permit one-line return statement :) // C++20: reject2 fix - added a cast
#define reject(...) __reject(__VA_ARGS__), false
#define reject2(...) __reject(__VA_ARGS__), (client_t*)NULL

static qboolean parse_basic_params(conn_params_t *p)
{
    p->protocol = atoi(Cmd_Argv(1));
    p->qport = atoi(Cmd_Argv(2)) ;
    p->challenge = atoi(Cmd_Argv(3));

    // check for invalid protocol version
    if (p->protocol != PROTOCOL_VERSION_NAC)
        return reject("Unsupported protocol version %d.\n", p->protocol);

    // check for valid, but outdated protocol version
    if (p->protocol < PROTOCOL_VERSION_DEFAULT)
        return reject("Unsupported protocol version %i, You need protocol version (%i) or higher.\n", p->protocol, PROTOCOL_VERSION_DEFAULT);

    return true;
}

static qboolean permit_connection(conn_params_t *p)
{
    AddressMatch *match;
    int i, count;
    client_t *cl;
    const char *s;

    // loopback clients are permitted without any checks
    if (NET_IsLocalAddress(&net_from))
        return true;

    // see if the challenge is valid
    for (i = 0; i < MAX_CHALLENGES; i++) {
        if (!svs.challenges[i].challenge)
            continue;

        if (NET_IsEqualBaseAdr(&net_from, &svs.challenges[i].adr)) {
            if (svs.challenges[i].challenge == p->challenge)
                break;        // good

            return reject("Bad challenge.\n");
        }
    }

    if (i == MAX_CHALLENGES)
        return reject("No challenge for address.\n");

    svs.challenges[i].challenge = 0;

    // check for banned address
    if ((match = SV_MatchAddress(&sv_banlist, &net_from)) != NULL) {
        s = match->comment;
        if (!*s) {
            s = "Your IP address is banned from this server."; // C++20: Added cast.
        }
        return reject("%s\nConnection refused.\n", s);
    }

    // check for locked server
    if (sv_locked->integer)
        return reject("Server is locked.\n");

    // link-local IPv6 addresses are permitted without sv_iplimit check
    if (net_from.type == NA_IP6 && NET_IsLanAddress(&net_from))
        return true;

    // limit number of connections from single IPv4 address or /48 IPv6 network
    if (sv_iplimit->integer > 0) {
        count = 0;
        FOR_EACH_CLIENT(cl) {
            netadr_t *adr = &cl->netchan->remoteAddress;

            if (net_from.type != adr->type)
                continue;
            if (net_from.type == NA_IP && net_from.ip.u32[0] != adr->ip.u32[0])
                continue;
            if (net_from.type == NA_IP6 && memcmp(net_from.ip.u8, adr->ip.u8, 48 / CHAR_BIT))
                continue;

            if (cl->connectionState == ConnectionState::Zombie)
                count += 1;
            else
                count += 2;
        }
        if (count / 2 >= sv_iplimit->integer) {
            if (net_from.type == NA_IP6)
                return reject("Too many connections from your IPv6 network.\n");
            else
                return reject("Too many connections from your IP address.\n");
        }
    }

    return true;
}

static qboolean parse_packet_length(conn_params_t *p)
{
    const char *s;

    // set maximum packet length
    p->maxlength = MAX_PACKETLEN_WRITABLE_DEFAULT;

    s = Cmd_Argv(5); // C++20: Added cast.
    if (*s) {
        p->maxlength = atoi(s);
        if (p->maxlength < 0 || p->maxlength > MAX_PACKETLEN_WRITABLE)
            return reject("Invalid maximum message length.\n");

        // 0 means highest available
        if (!p->maxlength)
            p->maxlength = MAX_PACKETLEN_WRITABLE;
    }

    if (!NET_IsLocalAddress(&net_from) && net_maxmsglen->integer > 0) {
        // cap to server defined maximum value
        if (p->maxlength > net_maxmsglen->integer)
            p->maxlength = net_maxmsglen->integer;
    }

    // don't allow too small packets
    if (p->maxlength < MIN_PACKETLEN)
        p->maxlength = MIN_PACKETLEN;

    return true;
}

static qboolean parse_enhanced_params(conn_params_t *p)
{
    const char *s;

    // set netchan type
    s = Cmd_Argv(6);
    if (*s) {
        p->nctype = atoi(s);
        if (p->nctype != 1)
            return reject("Invalid netchan type.\n");
    } else {
        p->nctype = 1;
    }

    // set zlib
    s = Cmd_Argv(7);
    if (*s) {
        p->has_zlib = !!atoi(s);
    } else {
        p->has_zlib = true;
    }

    // set minor protocol version
    s = Cmd_Argv(8);
    if (*s) {
        p->version = atoi(s);
        clamp(p->version,
                PROTOCOL_VERSION_NAC_MINIMUM,
                PROTOCOL_VERSION_NAC_CURRENT);
    } else {
        p->version = PROTOCOL_VERSION_NAC_MINIMUM;
    }

    return true;
}

static char *userinfo_ip_string(void)
{
    static char s[MAX_QPATH];

    return NET_AdrToString(&net_from);
}

static qboolean parse_userinfo(conn_params_t *params, char *userinfo)
{
    const char* info;// , * s;
    char* s;

    // validate userinfo
    info = Cmd_Argv(4);
    if (!info[0])
        return reject("Empty userinfo string.\n");

    if (!Info_Validate(info))
        return reject("Malformed userinfo string.\n");

    s = Info_ValueForKey(info, "name");
    s[MAX_CLIENT_NAME - 1] = 0;
    if (COM_IsWhite(s))
        return reject("Please set your name before connecting.\n");

    // check password
    s = Info_ValueForKey(info, "password");
    if (sv_password->string[0]) {
        if (!s[0])
            return reject("Please set your password before connecting.\n");

        if (SV_RateLimited(&svs.ratelimit_auth))
            return reject("Invalid password.\n");

        if (strcmp(sv_password->string, s))
            return reject("Invalid password.\n");

        // valid connect packets are not rate limited
        SV_RateRecharge(&svs.ratelimit_auth);

        // allow them to use reserved slots
    } else if (!sv_reserved_password->string[0] ||
               strcmp(sv_reserved_password->string, s)) {
        // if no reserved password is set on the server, do not allow
        // anyone to access reserved slots at all
        params->reserved = sv_reserved_slots->integer;
    }

	if (sv_restrict_rtx->integer)
	{
		s = Info_ValueForKey(info, "version");
		if (strncmp(s, "nac", 3) != 0)
		{
			return reject("This server is only available to Q2RTX clients.\n");
		}
	}

    // copy userinfo off
    Q_strlcpy(userinfo, info, MAX_INFO_STRING);

    return true;
}

static client_t *redirect(const char *addr)
{
    Netchan_OutOfBand(NS_SERVER, &net_from, "client_connect");

    // set up a fake server netchan
    MSG_WriteLong(1);
    MSG_WriteLong(0);
    MSG_WriteByte(svc_print);
    MSG_WriteByte(PRINT_HIGH);
    MSG_WriteString(va("Server is full. Redirecting you to %s...\n", addr));
    MSG_WriteByte(svc_stufftext);
    MSG_WriteString(va("connect %s\n", addr));

    NET_SendPacket(NS_SERVER, msg_write.data, msg_write.cursize, &net_from);
    SZ_Clear(&msg_write);
    return NULL;
}

static client_t *find_client_slot(conn_params_t *params)
{
    client_t *cl;
    char *s;
    int i;

    // if there is already a slot for this ip, reuse it
    FOR_EACH_CLIENT(cl) {
        if (NET_IsEqualAdr(&net_from, &cl->netchan->remoteAddress)) {
            if (cl->connectionState == ConnectionState::Zombie) {
                strcpy(params->reconnectKey, cl->reconnectKey);
                strcpy(params->reconnectValue, cl->reconnectValue);
            } else {
                SV_DropClient(cl, "reconnected");
            }

            Com_DPrintf("%s: reconnect\n", NET_AdrToString(&net_from));
            SV_RemoveClient(cl);
            return cl;
        }
    }

    // check for forced redirect to a different address
    s = sv_redirect_address->string;
    if (*s == '!' && sv_reserved_slots->integer == params->reserved)
        return redirect(s + 1);

    // find a free client slot
    for (i = 0; i < sv_maxclients->integer - params->reserved; i++) {
        cl = &svs.client_pool[i];
        if (cl->connectionState == ConnectionState::Free)
            return cl;
    }

    // clients that know the password are never redirected
    if (sv_reserved_slots->integer != params->reserved) {
        return reject2("Server and reserved slots are full.\n");
        //return NULL; // C++20: return reject2("Server and reserved slots are full.\n");
    }

    // optionally redirect them to a different address
    if (*s) {
        return redirect(s);
    }

    return reject2("Server is full.\n");
    //return NULL; // C++20: return reject2("Server is full.\n");
}

static void init_pmove_and_es_flags(client_t *newcl)
{
    //newcl->esFlags = (EntityStateMessageFlags)(newcl->esFlags | MSG_ES_UMASK); // CPP: Cast bitflag
    newcl->esFlags = (EntityStateMessageFlags)(newcl->esFlags | MSG_ES_BEAMORIGIN); // CPP: Cast bitflag
}

static void send_connect_packet(client_t *newcl, int nctype)
{
    const char *ncstring    = "";
    const char *acstring    = "";
    const char *dlstring1   = "";
    const char *dlstring2   = "";

    // MSG: !! Removed: PROTOCOL_VERSION_NAC - TODO: NETCHAN_NEW?
    //if (nctype == NETCHAN_NEW)
        ncstring = " nc=1";
    //else
    //    ncstring = " nc=0";

    if (sv_downloadserver->string[0]) {
        dlstring1 = " dlserver=";
        dlstring2 = sv_downloadserver->string;
    }

    Netchan_OutOfBand(NS_SERVER, &net_from, "client_connect%s%s%s%s map=%s",
                      ncstring, acstring, dlstring1, dlstring2, newcl->mapname);
}

// converts all the extra positional parameters to `connect' command into an
// infostring appended to normal userinfo after terminating NUL. game mod can
// then access these parameters in ClientConnect callback.
static void append_extra_userinfo(conn_params_t *params, char *userinfo)
{
    Q_snprintf(userinfo + strlen(userinfo) + 1, MAX_INFO_STRING,
               "\\challenge\\%d\\ip\\%s"
               "\\major\\%d\\minor\\%d\\netchan\\%d"
               "\\packetlen\\%d\\qport\\%d\\zlib\\%d",
               params->challenge, userinfo_ip_string(),
               params->protocol, params->version, params->nctype,
               params->maxlength, params->qport, params->has_zlib);
}

static void SVC_DirectConnect(void)
{
    char            userinfo[MAX_INFO_STRING * 2];
    conn_params_t   params;
    client_t        *newcl;
    int             number;
    qboolean        allow;
    char            *reason;

    memset(&params, 0, sizeof(params));

    // parse and validate parameters
    if (!parse_basic_params(&params))
        return;
    if (!permit_connection(&params))
        return;
    if (!parse_packet_length(&params))
        return;
    if (!parse_enhanced_params(&params))
        return;
    if (!parse_userinfo(&params, userinfo))
        return;

    // find a free client slot
    newcl = find_client_slot(&params);
    if (!newcl)
        return;

    number = newcl - svs.client_pool;

    // build a new connection
    // accept the new client
    // this is the only place a client_t is ever initialized
    memset(newcl, 0, sizeof(*newcl));
    newcl->number = newcl->slot = number;
    newcl->challenge = params.challenge; // save challenge for checksumming
    newcl->protocol = params.protocol;
    newcl->version = params.version;
    newcl->has_zlib = params.has_zlib;
    newcl->edict = EDICT_NUM(number + 1);
    newcl->gamedir = fs_game->string;
    newcl->mapname = sv.name;
    newcl->configstrings = (char *)sv.configstrings;
    newcl->pool = (EntityPool*)&ge->edicts; // N&C: Edict_pool_t change
    newcl->cm = &sv.cm;
    newcl->spawncount = sv.spawncount;
    newcl->maxClients = sv_maxclients->integer;
	newcl->lastValidCluster = -1;
    strcpy(newcl->reconnectKey, params.reconnectKey);
    strcpy(newcl->reconnectValue, params.reconnectValue);

    init_pmove_and_es_flags(newcl);

    append_extra_userinfo(&params, userinfo);

    // get the game a chance to reject this connection or modify the userinfo
    sv_client = newcl;
    sv_player = newcl->edict;
    allow = ge->ClientConnect(newcl->edict, userinfo);
    sv_client = NULL;
    sv_player = NULL;
    if (!allow) {
        reason = Info_ValueForKey(userinfo, "rejmsg");
        if (*reason) {
            __reject("%s\nConnection refused.\n", reason);
        } else {
            __reject("Connection refused.\n");
        }
        return;
    }

    // setup netchan
    newcl->netchan = Netchan_Setup(NS_SERVER, &net_from, params.qport, params.maxlength, params.protocol);
    newcl->numpackets = 1;

    // parse some info from the info strings
    Q_strlcpy(newcl->userinfo, userinfo, sizeof(newcl->userinfo));
    SV_UserinfoChanged(newcl);

    // send the connect packet to the client
    send_connect_packet(newcl, params.nctype);

    SV_RateInit(&newcl->ratelimitNameChange, sv_namechange_limit->string);

    SV_InitClientSend(newcl);

    // MSG: !!
    //if (newcl->protocol == PROTOCOL_VERSION_DEFAULT) {
    //    newcl->WriteFrame = __OLD_SV_WriteFrameToClient_Default;
    //} else {
        newcl->WriteFrame = SV_WriteFrameToClient;
//    }

    // loopback client doesn't need to reconnect
    if (NET_IsLocalAddress(&net_from)) {
        newcl->reconnected = true;
    }

    // add them to the linked list of connected clients
    List_SeqAdd(&sv_clientlist, &newcl->entry);

    Com_DPrintf("Going from ConnectionState::Free to ConnectionState::Assigned for %s\n", newcl->name);
    newcl->connectionState = ConnectionState::Assigned;
    newcl->frameNumber = 1; // frame 0 can't be used
    newcl->lastFrame = -1;
    newcl->lastMessage = svs.realtime;    // don't timeout
    newcl->lastActivity = svs.realtime;
    newcl->pingMinimum = 9999;
}

static qboolean rcon_valid(void)
{
    if (!rcon_password->string[0])
        return false;

    if (strcmp(Cmd_Argv(1), rcon_password->string))
        return false;

    return true;
}

/*
===============
SVC_RemoteCommand

A client issued an rcon command.
Redirect all printfs.
===============
*/
static void SVC_RemoteCommand(void)
{
    const char *s;

    if (SV_RateLimited(&svs.ratelimit_rcon)) {
        Com_DPrintf("Dropping rcon from %s\n",
                    NET_AdrToString(&net_from));
        return;
    }

    s = Cmd_RawArgsFrom(2);
    if (!rcon_valid()) {
        Com_Printf("Invalid rcon from %s:\n%s\n",
                   NET_AdrToString(&net_from), s);
        Netchan_OutOfBand(NS_SERVER, &net_from,
                          "print\nBad rcon_password.\n");
        return;
    }

    // valid rcon packets are not rate limited
    SV_RateRecharge(&svs.ratelimit_rcon);

	if (dedicated->integer)
	{
		Com_Printf("Rcon from %s: \"%s\"\n", NET_AdrToString(&net_from), s);
	}

    SV_PacketRedirect();
    Cmd_ExecuteString(&cmd_buffer, s);
    Com_EndRedirect();
}

static const ucmd_t svcmds[] = {
    { "ping",           SVC_Ping          },
    { "ack",            SVC_Ack           },
    { "status",         SVC_Status        },
    { "info",           SVC_Info          },
    { "getchallenge",   SVC_GetChallenge  },
    { "connect",        SVC_DirectConnect },
    { NULL }
};

/*
=================
SV_ConnectionlessPacket

A connectionless packet has four leading 0xff
characters to distinguish it from a game channel.
Clients that are in the game can still send
connectionless packets.
=================
*/
static void SV_ConnectionlessPacket(void)
{
    char    string[MAX_STRING_CHARS];
    const char    *c;
    int     i;
    size_t  len;

    if (SV_MatchAddress(&sv_blacklist, &net_from)) {
        Com_DPrintf("ignored blackholed connectionless packet\n");
        return;
    }

    MSG_BeginReading();
    MSG_ReadLong();        // skip the -1 marker

    len = MSG_ReadStringLine(string, sizeof(string));
    if (len >= sizeof(string)) {
        Com_DPrintf("ignored oversize connectionless packet\n");
        return;
    }

    Cmd_TokenizeString(string, false);

    c = Cmd_Argv(0); 
    Com_DPrintf("ServerPacket[%s]: %s\n", NET_AdrToString(&net_from), c);

    if (!strcmp(c, "rcon")) {
        SVC_RemoteCommand();
        return; // accept rcon commands even if not active
    }

    if (!svs.initialized) {
        Com_DPrintf("ignored connectionless packet\n");
        return;
    }

    for (i = 0; svcmds[i].name; i++) {
        if (!strcmp(c, svcmds[i].name)) {
            svcmds[i].func();
            return;
        }
    }

    Com_DPrintf("bad connectionless packet\n");
}


//============================================================================

int SV_CountClients(void)
{
    client_t *cl;
    int count = 0;

    FOR_EACH_CLIENT(cl) {
        if (cl->connectionState > ConnectionState::Zombie) {
            count++;
        }
    }

    return count;
}

static int ping_nop(client_t *cl)
{
    return 0;
}

static int ping_min(client_t *cl)
{
    ClientFrame *frame;
    int i, j, count = INT_MAX;

    for (i = 0; i < UPDATE_BACKUP; i++) {
        j = cl->frameNumber - i - 1;
        frame = &cl->frames[j & UPDATE_MASK];
        if (frame->number != j)
            continue;
        if (frame->latency == -1)
            continue;
        if (count > frame->latency)
            count = frame->latency;
    }

    return count == INT_MAX ? 0 : count;
}

static int ping_avg(client_t *cl)
{
    ClientFrame *frame;
    int i, j, total = 0, count = 0;

    for (i = 0; i < UPDATE_BACKUP; i++) {
        j = cl->frameNumber - i - 1;
        frame = &cl->frames[j & UPDATE_MASK];
        if (frame->number != j)
            continue;
        if (frame->latency == -1)
            continue;
        count++;
        total += frame->latency;
    }

    return count ? total / count : 0;
}

/*
===================
SV_CalcPings

Updates the cl->ping and cl->fps variables
===================
*/
static void SV_CalcPings(void)
{
    client_t    *cl;
    int         (*calc)(client_t *);
    int         res;

    switch (sv_calcpings_method->integer) {
        case 0:  calc = ping_nop; break;
        case 2:  calc = ping_min; break;
        default: calc = ping_avg; break;
    }

    // update avg ping and fps every 10 seconds
    res = sv.frameNumber % (10 * SV_FRAMERATE);

    FOR_EACH_CLIENT(cl) {
        if (cl->connectionState == ConnectionState::Spawned) {
            cl->ping = calc(cl);
            if (cl->ping) {
                if (cl->ping < cl->pingMinimum) {
                    cl->pingMinimum = cl->ping;
                } else if (cl->ping > cl->pingMaximum) {
                    cl->pingMaximum = cl->ping;
                }
                if (!res) {
                    cl->averagePingTime += cl->ping;
                    cl->averagePingCount++;
                }
            }
            if (!res) {
                cl->movesPerSecond = cl->numberOfMoves / 10;
                cl->numberOfMoves = 0;
            }
        } else {
            cl->ping = 0;
            cl->movesPerSecond = 0;
            cl->numberOfMoves = 0;
        }

        // let the game dll know about the ping
        cl->edict->client->ping = cl->ping;
    }
}


/*
===================
SV_GiveMsec

Every few frames, gives all clients an allotment of milliseconds
for their command moves.  If they exceed it, assume cheating.
===================
*/
static void SV_GiveMsec(void)
{
    client_t    *cl;

    if (sv.frameNumber % (16 * SV_FRAMEDIV))
        return;

    FOR_EACH_CLIENT(cl) {
        cl->clientUserCommandMiliseconds = 1800; // 1600 + some slop
    }
}


/*
=================
SV_PacketEvent
=================
*/
static void SV_PacketEvent(void)
{
    client_t    *client;
    netchan_t   *netchan;
    int         qport;

    // check for connectionless packet (0xffffffff) first
    // connectionless packets are processed even if the server is down
    if (*(int *)msg_read.data == -1) {
        SV_ConnectionlessPacket();
        return;
    }

    if (!svs.initialized) {
        return;
    }

    // check for packets from connected clients
    FOR_EACH_CLIENT(client) {
        netchan = client->netchan;
        if (!NET_IsEqualBaseAdr(&net_from, &netchan->remoteAddress)) {
            continue;
        }

        // read the qport out of the message so we can fix up
        // stupid address translating routers
        // MSG: !!
/*        if (client->protocol == PROTOCOL_VERSION_DEFAULT) {
            qport = msg_read.data[8] | (msg_read.data[9] << 8);
            if (netchan->qport != qport) {
                continue;
            }
        } else */
        if (netchan->qport) {
            qport = msg_read.data[8];
            if (netchan->qport != qport) {
                continue;
            }
        } else {
            if (netchan->remoteAddress.port != net_from.port) {
                continue;
            }
        }

        if (netchan->remoteAddress.port != net_from.port) {
            Com_DPrintf("Fixing up a translated port for %s: %d --> %d\n",
                        client->name, netchan->remoteAddress.port, net_from.port);
            netchan->remoteAddress.port = net_from.port;
        }

        if (!Netchan_Process(netchan))
            break;

        if (client->connectionState == ConnectionState::Zombie)
            break;

        // this is a valid, sequenced packet, so process it
        client->lastMessage = svs.realtime;    // don't timeout
#if USE_ICMP
        client->unreachable = false; // don't drop
#endif
        if (netchan->dropped > 0)
            client->frameFlags |= FF_CLIENTDROP;

        SV_ExecuteClientMessage(client);
        break;
    }
}

#if USE_PMTUDISC
// We are doing path MTU discovery and got ICMP fragmentation-needed.
// Update MTU for connecting clients only to minimize spoofed ICMP interference.
// Total 64 bytes of headers is assumed.
static void update_client_mtu(client_t *client, int ee_info)
{
    netchan_t *netchan = client->netchan;
    size_t newpacketlen;

    // sanity check discovered MTU
    if (ee_info < 576 || ee_info > 4096)
        return;

    if (client->state != ConnectionState::Primed)
        return;

    // TODO: old clients require entire queue flush :(
    if (netchan->type == NETCHAN_OLD)
        return;

    if (!netchan->reliableLength)
        return;

    newpacketlen = ee_info - 64;
    if (newpacketlen >= netchan->maxpacketlen)
        return;

    Com_Printf("Fixing up maxmsglen for %s: %" PRIz " --> %" PRIz "\n",
               client->name, netchan->maxpacketlen, newpacketlen);
    netchan->maxpacketlen = newpacketlen;
}
#endif

#if USE_ICMP
/*
=================
SV_ErrorEvent
=================
*/
void SV_ErrorEvent(netadr_t *from, int ee_errno, int ee_info)
{
    client_t    *client;
    netchan_t   *netchan;

    if (!svs.initialized) {
        return;
    }

    // check for errors from connected clients
    FOR_EACH_CLIENT(client) {
        if (client->connectionState == ConnectionState::Zombie) {
            continue; // already a zombie
        }
        netchan = client->netchan;
        if (!NET_IsEqualBaseAdr(from, &netchan->remoteAddress)) {
            continue;
        }
        if (from->port && netchan->remoteAddress.port != from->port) {
            continue;
        }
#if USE_PMTUDISC
        // for EMSGSIZE ee_info should hold discovered MTU
        if (ee_errno == EMSGSIZE) {
            update_client_mtu(client, ee_info);
            continue;
        }
#endif
        client->unreachable = true; // drop them soon
        break;
    }
}
#endif

/*
==================
SV_CheckTimeouts

If a packet has not been received from a client for timeout->value
seconds, drop the conneciton.

When a client is normally dropped, the client_t goes into a zombie state
for a few seconds to make sure any final reliable message gets resent
if necessary
==================
*/
static void SV_CheckTimeouts(void)
{
    client_t    *client;
    unsigned    zombie_time = 1000 * sv_zombietime->value;
    unsigned    drop_time   = 1000 * sv_timeout->value;
    unsigned    ghost_time  = 1000 * sv_ghostime->value;
    unsigned    idle_time   = 1000 * sv_idlekick->value;
    unsigned    delta;

    FOR_EACH_CLIENT(client) {
        // never timeout local clients
        if (NET_IsLocalAddress(&client->netchan->remoteAddress)) {
            continue;
        }
        // NOTE: delta calculated this way is not sensitive to overflow
        delta = svs.realtime - client->lastMessage;
        if (client->connectionState == ConnectionState::Zombie) {
            if (delta > zombie_time) {
                SV_RemoveClient(client);
            }
            continue;
        }
        if (client->drop_hack) {
            SV_DropClient(client, NULL);
            continue;
        }
#if USE_ICMP
        if (client->unreachable) {
            if (delta > ghost_time) {
                SV_DropClient(client, "connection reset by peer");
                SV_RemoveClient(client);      // don't bother with zombie state
                continue;
            }
        }
#endif
        if (delta > drop_time || (client->connectionState == ConnectionState::Assigned && delta > ghost_time)) {
            SV_DropClient(client, "?timed out");
            SV_RemoveClient(client);      // don't bother with zombie state
            continue;
        }

        if (client->framesNoDelta > 64 && !sv_allow_nodelta->integer) {
            SV_DropClient(client, "too many nodelta frames");
            continue;
        }

        delta = svs.realtime - client->lastActivity;
        if (idle_time && delta > idle_time) {
            SV_DropClient(client, "idling");
            continue;
        }
    }
}

/*
================
SV_PrepWorldFrame

This has to be done before the world logic, because
player processing happens outside RunWorldFrame
================
*/
static void SV_PrepWorldFrame(void)
{
    entity_t    *ent;
    int        i;

    sv.tracecount = 0;

    if (!SV_FRAMESYNC)
        return;

    for (i = 1; i < ge->num_edicts; i++) {
        ent = EDICT_NUM(i);

        // events only last for a single keyframe
        ent->state.event = 0;
    }
}

// pause if there is only local client on the server
static inline qboolean check_paused(void)
{
#if USE_CLIENT
    if (dedicated->integer)
        goto resume;

    if (!cl_paused->integer)
        goto resume;

    if (com_timedemo->integer)
        goto resume;

	if (!LIST_SINGLE(&sv_clientlist) && !Cvar_VariableInteger("coop"))
        goto resume;

    if (!sv_paused->integer) {
        Cvar_Set("sv_paused", "1");
        IN_Activate();
    }

    return true; // don't run if paused

resume:
    if (sv_paused->integer) {
        Cvar_Set("sv_paused", "0");
        IN_Activate();
    }
#endif

    return false;
}

/*
=================
SV_RunGameFrame
=================
*/
static void SV_RunGameFrame(void)
{
#if USE_CLIENT
    if (host_speeds->integer)
        time_before_game = Sys_Milliseconds();
#endif

    ge->RunFrame();

#if USE_CLIENT
    if (host_speeds->integer)
        time_after_game = Sys_Milliseconds();
#endif

    if (msg_write.cursize) {
        Com_WPrintf("Game left %" PRIz " bytes "
                    "in multicast buffer, cleared.\n",
                    msg_write.cursize);
        SZ_Clear(&msg_write);
    }
}

/*
================
SV_MasterHeartbeat

Send a message to the master every few minutes to
let it know we are alive, and log information
================
*/
static void SV_MasterHeartbeat(void)
{
    char    buffer[MAX_PACKETLEN_DEFAULT];
    size_t  len;
    master_t *m;

    if (!COM_DEDICATED)
        return;        // only dedicated servers send heartbeats

    if (!sv_public->integer)
        return;        // a private dedicated game

    if (svs.realtime - svs.last_heartbeat < HEARTBEAT_SECONDS * 1000)
        return;        // not time to send yet

    svs.last_heartbeat = svs.realtime;

    // write the packet header
    memcpy(buffer, "\xff\xff\xff\xffheartbeat\n", 14);
    len = 14;

    // send the same string that we would give for a status OOB command
    len += SV_StatusString(buffer + len);

    // send to group master
    FOR_EACH_MASTER(m) {
        if (m->adr.port) {
            Com_DPrintf("Sending heartbeat to %s\n",
                        NET_AdrToString(&m->adr));
            NET_SendPacket(NS_SERVER, buffer, len, &m->adr);
        }
    }
}

/*
=================
SV_MasterShutdown

Informs all masters that this server is going down
=================
*/
static void SV_MasterShutdown(void)
{
    master_t *m;

    // reset ack times
    FOR_EACH_MASTER(m) {
        m->last_ack = 0;
    }

    if (!COM_DEDICATED)
        return;        // only dedicated servers send heartbeats

    if (!sv_public || !sv_public->integer)
        return;        // a private dedicated game

    // send to group master
    FOR_EACH_MASTER(m) {
        if (m->adr.port) {
            Com_DPrintf("Sending shutdown to %s\n",
                        NET_AdrToString(&m->adr));
            OOB_PRINT(NS_SERVER, &m->adr, "shutdown");
        }
    }
}

/*
==================
SV_Frame

Some things like MVD client connections and command buffer
processing are run even when server is not yet initalized.

Returns amount of extra frameTime available for sleeping on IO.
==================
*/
unsigned SV_Frame(unsigned msec)
{
#if USE_CLIENT
    time_before_game = time_after_game = 0;
#endif

    // advance local server time
    svs.realtime += msec;

    if (COM_DEDICATED) {
        // process console commands if not running a client
        Cbuf_Execute(&cmd_buffer);
    }

    // read packets from UDP clients
    NET_GetPackets(NS_SERVER, SV_PacketEvent);

    if (svs.initialized) {
        // deliver fragments and reliable messages for connecting clients
        SV_SendAsyncPackets();
    }

    // move autonomous things around if enough time has passed
    sv.frameResidual += msec;
    if (sv.frameResidual < SV_FRAMETIME) {
        return SV_FRAMETIME - sv.frameResidual;
    }

    if (svs.initialized && !check_paused()) {
        // check timeouts
        SV_CheckTimeouts();

        // update ping based on the last known frame from all clients
        SV_CalcPings();

        // give the clients some timeslices
        SV_GiveMsec();

        // let everything in the world Think and move
        SV_RunGameFrame();

        // send messages back to the UDP clients
        SV_SendClientMessages();

        // send a heartbeat to the master if needed
        SV_MasterHeartbeat();

        // clear teleport flags, etc for next frame
        SV_PrepWorldFrame();

        // advance for next frame
        sv.frameNumber++;
    }

    if (COM_DEDICATED) {
        // run cmd buffer in dedicated mode
        if (cmd_buffer.waitCount > 0) {
            cmd_buffer.waitCount--;
        }
    }

    // decide how long to sleep next frame
    sv.frameResidual -= SV_FRAMETIME;
    if (sv.frameResidual < SV_FRAMETIME) {
        return SV_FRAMETIME - sv.frameResidual;
    }

    // don't accumulate bogus residual
    if (sv.frameResidual > 250) {
        Com_DDDPrintf("Reset residual %u\n", sv.frameResidual);
        sv.frameResidual = 100;
    }

    return 0;
}

//============================================================================

/*
=================
SV_UserinfoChanged

Pull specific info from a newly changed userinfo string
into a more C freindly form.
=================
*/
void SV_UserinfoChanged(client_t *cl)
{
    char    name[MAX_CLIENT_NAME];
    char    *val;
    size_t  len;
    int     i;

    // call prog code to allow overrides
    ge->ClientUserinfoChanged(cl->edict, cl->userinfo);

    // name for C code
    val = Info_ValueForKey(cl->userinfo, "name");
    len = Q_strlcpy(name, val, sizeof(name));
    if (len >= sizeof(name)) {
        len = sizeof(name) - 1;
    }
    // mask off high bit
    for (i = 0; i < len; i++)
        name[i] &= 127;
    if (cl->name[0] && strcmp(cl->name, name)) {
        if (COM_DEDICATED) {
            Com_Printf("%s[%s] changed name to %s\n", cl->name,
                       NET_AdrToString(&cl->netchan->remoteAddress), name);
        }

        if (sv_show_name_changes->integer) {
            SV_BroadcastPrintf(PRINT_HIGH, "%s changed name to %s\n",
                                cl->name, name);
        }
    }
    memcpy(cl->name, name, len + 1);

    // rate command
    val = Info_ValueForKey(cl->userinfo, "rate");
    if (*val) {
        cl->rate = atoi(val);
        clamp(cl->rate, 100, 15000);
    } else {
        cl->rate = 5000;
    }

    // never drop over the loopback
    if (NET_IsLocalAddress(&cl->netchan->remoteAddress)) {
        cl->rate = 0;
    }

    // don't drop over LAN connections
    if (sv_lan_force_rate->integer &&
        NET_IsLanAddress(&cl->netchan->remoteAddress)) {
        cl->rate = 0;
    }

    // msg command
    val = Info_ValueForKey(cl->userinfo, "msg");
    if (*val) {
        cl->messageLevel = atoi(val);
        clamp(cl->messageLevel, PRINT_LOW, PRINT_CHAT + 1);
    }
}


//============================================================================

#if USE_SYSCON
void SV_SetConsoleTitle(void)
{
    char buffer[MAX_STRING_CHARS];

    Q_snprintf(buffer, sizeof(buffer), "%s (port %d%s)",
               sv_hostname->string, net_port->integer,
               sv_running->integer ? "" : ", down");

    Sys_SetConsoleTitle(buffer);
}
#endif

static void sv_status_limit_changed(cvar_t *self)
{
    SV_RateInit(&svs.ratelimit_status, self->string);
}

static void sv_auth_limit_changed(cvar_t *self)
{
    SV_RateInit(&svs.ratelimit_auth, self->string);
}

static void sv_rcon_limit_changed(cvar_t *self)
{
    SV_RateInit(&svs.ratelimit_rcon, self->string);
}

static void init_rate_limits(void)
{
    SV_RateInit(&svs.ratelimit_status, sv_status_limit->string);
    SV_RateInit(&svs.ratelimit_auth, sv_auth_limit->string);
    SV_RateInit(&svs.ratelimit_rcon, sv_rcon_limit->string);
}

static void sv_namechange_limit_changed(cvar_t *self)
{
    client_t *client;

    FOR_EACH_CLIENT(client) {
        SV_RateInit(&client->ratelimitNameChange, self->string);
    }
}

#if USE_SYSCON
static void sv_hostname_changed(cvar_t *self)
{
    SV_SetConsoleTitle();
}
#endif

#if USE_ZLIB
voidpf SV_zalloc(voidpf opaque, uInt items, uInt size)
{
    return SV_Malloc(items * size);
}

void SV_zfree(voidpf opaque, voidpf address)
{
    Z_Free(address);
}
#endif

/*
===============
SV_Init

Only called at quake2.exe startup, not for each game
===============
*/
void SV_Init(void)
{
    SV_InitOperatorCommands();

    SV_RegisterSavegames();

    Cvar_Get("protocol", STRINGIFY(PROTOCOL_VERSION_DEFAULT), CVAR_SERVERINFO | CVAR_ROM);

    Cvar_Get("skill", "1", CVAR_LATCH);
    Cvar_Get("deathmatch", "1", CVAR_SERVERINFO | CVAR_LATCH);
    Cvar_Get("coop", "0", /*CVAR_SERVERINFO|*/CVAR_LATCH);
    Cvar_Get("cheats", "0", CVAR_SERVERINFO | CVAR_LATCH);
    Cvar_Get("dmflags", "16", CVAR_SERVERINFO); // 16 = DF_INSTANT_ITEMS
    Cvar_Get("fraglimit", "0", CVAR_SERVERINFO);
    Cvar_Get("timelimit", "0", CVAR_SERVERINFO);

    sv_maxclients = Cvar_Get("maxClients", "8", CVAR_SERVERINFO | CVAR_LATCH);
    sv_reserved_slots = Cvar_Get("sv_reserved_slots", "0", CVAR_LATCH);
    sv_hostname = Cvar_Get("hostname", "noname", CVAR_SERVERINFO | CVAR_ARCHIVE);
#if USE_SYSCON
    sv_hostname->changed = sv_hostname_changed;
#endif
    sv_timeout = Cvar_Get("timeout", "90", 0);
    sv_zombietime = Cvar_Get("zombietime", "2", 0);
    sv_ghostime = Cvar_Get("sv_ghostime", "6", 0);
    sv_idlekick = Cvar_Get("sv_idlekick", "0", 0);
    sv_showclamp = Cvar_Get("showclamp", "0", 0);
    sv_enforcetime = Cvar_Get("sv_enforcetime", "1", 0);
    sv_allow_nodelta = Cvar_Get("sv_allow_nodelta", "1", 0);

    sv_force_reconnect = Cvar_Get("sv_force_reconnect", "", CVAR_LATCH);
    sv_show_name_changes = Cvar_Get("sv_show_name_changes", "0", 0);

    sv_airaccelerate = Cvar_Get("sv_airaccelerate", "0", CVAR_LATCH);
    sv_qwmod = Cvar_Get("sv_qwmod", "1", CVAR_LATCH);   //atu QWMod
    sv_public = Cvar_Get("public", "0", CVAR_LATCH);
    sv_password = Cvar_Get("sv_password", "", CVAR_PRIVATE);
    sv_reserved_password = Cvar_Get("sv_reserved_password", "", CVAR_PRIVATE);
    sv_locked = Cvar_Get("sv_locked", "0", 0);
    sv_novis = Cvar_Get("sv_novis", "0", 0);
    sv_downloadserver = Cvar_Get("sv_downloadserver", "", 0);
    sv_redirect_address = Cvar_Get("sv_redirect_address", "", 0);

#ifdef _DEBUG
    sv_debug = Cvar_Get("sv_debug", "0", 0);
    sv_pad_packets = Cvar_Get("sv_pad_packets", "0", 0);
#endif
    sv_lan_force_rate = Cvar_Get("sv_lan_force_rate", "0", CVAR_LATCH);
    sv_calcpings_method = Cvar_Get("sv_calcpings_method", "2", 0);
    sv_changemapcmd = Cvar_Get("sv_changemapcmd", "", 0);

    sv_strafejump_hack = Cvar_Get("sv_strafejump_hack", "1", CVAR_LATCH);
    sv_waterjump_hack = Cvar_Get("sv_waterjump_hack", "0", CVAR_LATCH);

#if USE_PACKETDUP
    sv_packetdup_hack = Cvar_Get("sv_packetdup_hack", "0", 0);
#endif

    sv_allow_map = Cvar_Get("sv_allow_map", "0", 0);

#if !USE_CLIENT
    sv_recycle = Cvar_Get("sv_recycle", "0", 0);
#endif

    sv_enhanced_setplayer = Cvar_Get("sv_enhanced_setplayer", "0", 0);

    sv_iplimit = Cvar_Get("sv_iplimit", "3", 0);

    sv_status_show = Cvar_Get("sv_status_show", "2", 0);

    sv_status_limit = Cvar_Get("sv_status_limit", "15", 0);
    sv_status_limit->changed = sv_status_limit_changed;

    sv_uptime = Cvar_Get("sv_uptime", "0", 0);

    sv_auth_limit = Cvar_Get("sv_auth_limit", "1", 0);
    sv_auth_limit->changed = sv_auth_limit_changed;

    sv_rcon_limit = Cvar_Get("sv_rcon_limit", "1", 0);
    sv_rcon_limit->changed = sv_rcon_limit_changed;

    sv_namechange_limit = Cvar_Get("sv_namechange_limit", "5/min", 0);
    sv_namechange_limit->changed = sv_namechange_limit_changed;

	sv_restrict_rtx = Cvar_Get("sv_restrict_rtx", "1", 0);

    sv_allow_unconnected_cmds = Cvar_Get("sv_allow_unconnected_cmds", "0", 0);

    map_override_path = Cvar_Get("map_override_path", "", 0);

    init_rate_limits();

#if USE_SYSCON
    SV_SetConsoleTitle();
#endif

    sv_registered = true;
}

/*
==================
SV_FinalMessage

Used by SV_Shutdown to send a final message to all
connected clients before the server goes down. The messages are sent
immediately, not just stuck on the outgoing message list, because the
server is going to totally exit after returning from this function.

Also resposible for freeing all clients.
==================
*/
static void SV_FinalMessage(const char *message, ErrorType type)
{
    client_t    *client;
    netchan_t   *netchan;
    int         i;

    if (LIST_EMPTY(&sv_clientlist))
        return;

    if (message) {
        MSG_WriteByte(svc_print);
        MSG_WriteByte(PRINT_HIGH);
        MSG_WriteString(message);
    }

    if (type == ERR_RECONNECT)
        MSG_WriteByte(svc_reconnect);
    else
        MSG_WriteByte(svc_disconnect);

    // send it twice
    // stagger the packets to crutch operating system limited buffers
    for (i = 0; i < 2; i++) {
        FOR_EACH_CLIENT(client) {
            if (client->connectionState == ConnectionState::Zombie) {
                continue;
            }
            netchan = client->netchan;
            while (netchan->fragmentPending) {
                Netchan_TransmitNextFragment(netchan);
            }
            Netchan_Transmit(netchan, msg_write.cursize, msg_write.data, 1);
        }
    }

    SZ_Clear(&msg_write);

    // free any data dynamically allocated
    FOR_EACH_CLIENT(client) {
        if (client->connectionState != ConnectionState::Zombie) {
            SV_CleanClient(client);
        }
        SV_RemoveClient(client);
    }

    List_Init(&sv_clientlist);
}

/*
================
SV_Shutdown

Called when each game quits, from Com_Quit or Com_Error.
Should be safe to call even if server is not fully initalized yet.
================
*/
void SV_Shutdown(const char *finalmsg, ErrorType type)
{
    if (!sv_registered)
        return;

    SV_FinalMessage(finalmsg, type);
    SV_MasterShutdown();
    SV_ShutdownGameProgs();

    // free current level
    CM_FreeMap(&sv.cm);
    SV_FreeFile(sv.entityString);
    memset(&sv, 0, sizeof(sv));

    // free server static data
    Z_Free(svs.client_pool);
    Z_Free(svs.entities);
#if USE_ZLIB
    deflateEnd(&svs.z);
#endif
    memset(&svs, 0, sizeof(svs));

    // reset rate limits
    init_rate_limits();

    sv_client = NULL;
    sv_player = NULL;

    Cvar_Set("sv_running", "0");
    Cvar_Set("sv_paused", "0");

#if USE_SYSCON
    SV_SetConsoleTitle();
#endif

    Z_LeakTest(TAG_SERVER);
}

