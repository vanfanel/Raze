//-------------------------------------------------------------------------
/*
Copyright (C) 1996, 2003 - 3D Realms Entertainment
Copyright (C) 2000, 2003 - Matt Saettler (EDuke Enhancements)
Copyright (C) 2020 - Christoph Oelckers

This file is part of Enhanced Duke Nukem 3D version 1.5 - Atomic Edition

Duke Nukem 3D is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

Original Source: 1996 - Todd Replogle
Prepared for public release: 03/21/2003 - Charlie Wiederhold, 3D Realms

EDuke enhancements integrated: 04/13/2003 - Matt Saettler

Note: EDuke source was in transition.  Changes are in-progress in the
source as it is released.

*/
//-------------------------------------------------------------------------
#include "ns.h"
#include "concmd.h"
#include "duke3d.h"
#include "gamevar.h"
#include "mapinfo.h"
#include "gamestate.h"
#include "conlabel.h"
#include "automap.h"

BEGIN_DUKE_NS

// Player Actions - used by ifp instruction.
enum playeraction_t {
	pstanding                   = 0x00000001,
	pwalking                    = 0x00000002,
	prunning                    = 0x00000004,
	pducking                    = 0x00000008,
	pfalling                    = 0x00000010,
	pjumping                    = 0x00000020,
	phigher                     = 0x00000040,
	pwalkingback                = 0x00000080,
	prunningback                = 0x00000100,
	pkicking                    = 0x00000200,
	pshrunk                     = 0x00000400,
	pjetpack                    = 0x00000800,
	ponsteroids                 = 0x00001000,
	ponground                   = 0x00002000,
	palive                      = 0x00004000,
	pdead                       = 0x00008000,
	pfacing                     = 0x00010000
};



struct ParseState
{
	int g_i, g_p;
	int g_x;
	int* g_t;
	uint8_t killit_flag;
	spritetype* g_sp;
	int* insptr;

	int parse(void);
	void parseifelse(int condition);
};

int furthestcanseepoint(int i, spritetype* ts, int* dax, int* day);
bool ifsquished(int i, int p);
void fakebubbaspawn(int g_i, int g_p);
void tearitup(int sect);
void destroyit(int g_i);
void mamaspawn(int g_i);
void forceplayerangle(int snum);

bool killthesprite = false;

void addspritetodelete(int spnum)
{
	killthesprite = true;
}

static void DoUserDef(bool bSet, int lVar1, int lLabelID, int lVar2, int sActor, int sPlayer, int lParm2)
{
	int lValue;

	lValue = GetGameVarID((int)lVar2, sActor, sPlayer);

	// most settings have been removed because they are either pointless, no longer existent or simply too dangerous to access.
	// Others have been made read-only.
	switch (lLabelID)
	{
	case USERDEFS_GOD: // redid this so that the script won't be able to disable user-set god mode.
		if (bSet) ud.god = (ud.god & ~2) | (lValue? 2:0);
		else SetGameVarID((int)lVar2, !!ud.god, sActor, sPlayer);
		break;

	case USERDEFS_CASHMAN:
		if (bSet) ud.cashman = lValue;
		else SetGameVarID((int)lVar2, ud.cashman, sActor, sPlayer);
		break;

	case USERDEFS_EOG:
		if (bSet) ud.eog = lValue;
		else SetGameVarID((int)lVar2, ud.eog, sActor, sPlayer);
		break;

	case USERDEFS_SHOWALLMAP:
		if (bSet) gFullMap = lValue;
		else SetGameVarID((int)lVar2, gFullMap, sActor, sPlayer);
		break;

	case USERDEFS_SHOWWEAPONS:
		if (bSet) ud.showweapons = lValue;
		else SetGameVarID((int)lVar2, ud.showweapons, sActor, sPlayer);
		break;

	case USERDEFS_FROM_BONUS:
		if (bSet) ud.from_bonus = lValue;
		else SetGameVarID((int)lVar2, ud.from_bonus, sActor, sPlayer);
		break;

	case USERDEFS_CAMERASPRITE:
		if (bSet) ud.camerasprite = lValue;
		else SetGameVarID((int)lVar2, ud.camerasprite, sActor, sPlayer);
		break;

	case USERDEFS_LAST_CAMSPRITE:
		if (!bSet) SetGameVarID((int)lVar2, -1, sActor, sPlayer);
		break;

	case USERDEFS_LAST_LEVEL:
		if (bSet) ud.last_level = lValue;
		else SetGameVarID((int)lVar2, ud.last_level, sActor, sPlayer);
		break;

	case USERDEFS_SECRETLEVEL:
		if (bSet) ud.secretlevel = lValue;
		else SetGameVarID((int)lVar2, ud.secretlevel, sActor, sPlayer);
		break;

	case USERDEFS_CONST_VISIBILITY:
		if (bSet) ud.const_visibility = lValue;
		else SetGameVarID((int)lVar2, ud.const_visibility, sActor, sPlayer);
		break;

	case USERDEFS_SHADOWS:
		if (bSet) ud.shadows = lValue;
		else SetGameVarID((int)lVar2, ud.shadows, sActor, sPlayer);
		break;
	case USERDEFS_M_COOP:
		if (!bSet) SetGameVarID((int)lVar2, ud.m_coop, sActor, sPlayer);
		break;

	case USERDEFS_COOP:
		if (!bSet) SetGameVarID((int)lVar2, ud.coop, sActor, sPlayer);
		break;

	case USERDEFS_RESPAWN_MONSTERS:
		if (bSet) ud.respawn_monsters = lValue;
		else SetGameVarID((int)lVar2, ud.respawn_monsters, sActor, sPlayer);
		break;

	case USERDEFS_RESPAWN_ITEMS:
		if (bSet) ud.respawn_items = lValue;
		else SetGameVarID((int)lVar2, ud.respawn_items, sActor, sPlayer);
		break;

	case USERDEFS_RESPAWN_INVENTORY:
		if (bSet) ud.respawn_inventory = lValue;
		else SetGameVarID((int)lVar2, ud.respawn_inventory, sActor, sPlayer);
		break;

	case USERDEFS_RECSTAT:
		if (!bSet) SetGameVarID((int)lVar2, ud.recstat, sActor, sPlayer);
		break;

	case USERDEFS_MONSTERS_OFF:
		if (bSet) ud.monsters_off = lValue;
		else SetGameVarID((int)lVar2, ud.monsters_off, sActor, sPlayer);
		break;

	case USERDEFS_BRIGHTNESS:
		if (bSet) ud.brightness = lValue;
		else SetGameVarID((int)lVar2, ud.brightness, sActor, sPlayer);
		break;

	case USERDEFS_M_RESPAWN_ITEMS:
		if (bSet) ud.m_respawn_items = lValue;
		else SetGameVarID((int)lVar2, ud.m_respawn_items, sActor, sPlayer);
		break;

	case USERDEFS_M_RESPAWN_MONSTERS:
		if (bSet) ud.m_respawn_monsters = lValue;
		else SetGameVarID((int)lVar2, ud.m_respawn_monsters, sActor, sPlayer);
		break;

	case USERDEFS_M_RESPAWN_INVENTORY:
		if (bSet) ud.m_respawn_inventory = lValue;
		else SetGameVarID((int)lVar2, ud.m_respawn_inventory, sActor, sPlayer);
		break;

	case USERDEFS_M_MONSTERS_OFF:
		if (bSet) ud.m_monsters_off = lValue;
		else SetGameVarID((int)lVar2, ud.m_monsters_off, sActor, sPlayer);
		break;

	case USERDEFS_M_FFIRE:
		if (bSet) ud.m_ffire = lValue;
		else SetGameVarID((int)lVar2, ud.m_ffire, sActor, sPlayer);
		break;

	case USERDEFS_FFIRE:
		if (bSet) ud.ffire = lValue;
		else SetGameVarID((int)lVar2, ud.ffire, sActor, sPlayer);
		break;

	case USERDEFS_MULTIMODE:
		if (!bSet) SetGameVarID((int)lVar2, ud.multimode, sActor, sPlayer);
		break;

	case USERDEFS_PLAYER_SKILL:
		if (bSet) ud.player_skill = lValue;
		else SetGameVarID((int)lVar2, ud.player_skill, sActor, sPlayer);
		break;

	case USERDEFS_LEVEL_NUMBER:
		if (!bSet) SetGameVarID((int)lVar2, mapfromlevelnum(currentLevel->levelNumber), sActor, sPlayer);
		break;

	case USERDEFS_VOLUME_NUMBER:
		if (!bSet) SetGameVarID((int)lVar2, volfromlevelnum(currentLevel->levelNumber), sActor, sPlayer);
		break;

	case USERDEFS_MARKER:
		if (bSet) ud.marker = lValue;
		else SetGameVarID((int)lVar2, ud.marker, sActor, sPlayer);
		break;

	default:
		// This will also catch all deleted cases.
		// make sure that the return value is always defined.
		if (!bSet) SetGameVarID((int)lVar2, 0, sActor, sPlayer);
		break;
	}
	return;
}

///////////////////////////////////////////
void DoPlayer(bool bSet, int lVar1, int lLabelID, int lVar2, int sActor, int sPlayer, int lParm2)
{
	int iPlayer;
	int lValue;
	int lTemp;

	lValue = GetGameVarID((int)lVar2, sActor, sPlayer);

	if (lVar1 == g_iThisActorID)
	{
		// if they've asked for 'this', then use 'this player'...
		iPlayer = sPlayer;
	}
	else
	{
		iPlayer = GetGameVarID((int)lVar1, sActor, sPlayer);
	}

	if (iPlayer < 0 || iPlayer >= MAXPLAYERS)
		return;

	switch (lLabelID)
	{
	case PLAYER_ZOOM:
		SetGameVarID((int)lVar2, 768, sActor, sPlayer);	//return default for AM zoom.
		break;

	case PLAYER_EXITX:
		if (bSet) ps[iPlayer].exitx = lValue;
		else SetGameVarID((int)lVar2, ps[iPlayer].exitx, sActor, sPlayer);
		break;

	case PLAYER_EXITY:
		if (bSet) ps[iPlayer].exity = lValue;
		else SetGameVarID((int)lVar2, ps[iPlayer].exity, sActor, sPlayer);
		break;

	case PLAYER_LOOGIEX:
		if (bSet) ps[iPlayer].loogiex[lParm2] = lValue;
		else SetGameVarID((int)lVar2, ps[iPlayer].loogiex[lParm2], sActor, sPlayer);
		break;

	case PLAYER_LOOGIEY:
		if (bSet) ps[iPlayer].loogiey[lParm2] = lValue;
		else SetGameVarID((int)lVar2, ps[iPlayer].loogiey[lParm2], sActor, sPlayer);
		break;

	case PLAYER_NUMLOOGS:
		if (bSet) ps[iPlayer].numloogs = lValue;
		else SetGameVarID((int)lVar2, ps[iPlayer].numloogs, sActor, sPlayer);
		break;

	case PLAYER_LOOGCNT:
		if (bSet) ps[iPlayer].loogcnt = lValue;
		else SetGameVarID((int)lVar2, ps[iPlayer].loogcnt, sActor, sPlayer);
		break;

	case PLAYER_POSX: // oh, my... :( Writing to these has been disabled until I know how to do it without the engine shitting all over itself.
		if (!bSet) SetGameVarID((int)lVar2, ps[iPlayer].posx, sActor, sPlayer);
		break;

	case PLAYER_POSY:
		if (!bSet) SetGameVarID((int)lVar2, ps[iPlayer].posy, sActor, sPlayer);
		break;

	case PLAYER_POSZ:
		if (!bSet) SetGameVarID((int)lVar2, ps[iPlayer].posz, sActor, sPlayer);
		break;

	case PLAYER_HORIZ:
		if (bSet) playerSetHoriz(&ps[iPlayer].q16horiz, &ps[iPlayer].horizTarget, lValue);
		else SetGameVarID((int)lVar2, FixedToInt(ps[iPlayer].q16horiz), sActor, sPlayer);
		break;

	case PLAYER_OHORIZ:
		if (!bSet) SetGameVarID((int)lVar2, FixedToInt(ps[iPlayer].q16horiz), sActor, sPlayer);
		break;

	case PLAYER_OHORIZOFF:
		if (!bSet) SetGameVarID((int)lVar2, FixedToInt(ps[iPlayer].q16horizoff), sActor, sPlayer);
		break;

	case PLAYER_INVDISPTIME:
		if (bSet) ps[iPlayer].invdisptime = lValue;
		else SetGameVarID((int)lVar2, ps[iPlayer].invdisptime, sActor, sPlayer);
		break;

	case PLAYER_BOBPOSX:
		if (bSet) ps[iPlayer].bobposx = lValue;
		else SetGameVarID((int)lVar2, ps[iPlayer].bobposx, sActor, sPlayer);
		break;

	case PLAYER_BOBPOSY:
		if (bSet) ps[iPlayer].bobposy = lValue;
		else SetGameVarID((int)lVar2, ps[iPlayer].bobposy, sActor, sPlayer);
		break;

	case PLAYER_OPOSX:
		if (bSet) ps[iPlayer].oposx = lValue;
		else SetGameVarID((int)lVar2, ps[iPlayer].oposx, sActor, sPlayer);
		break;

	case PLAYER_OPOSY:
		if (bSet) ps[iPlayer].oposy = lValue;
		else SetGameVarID((int)lVar2, ps[iPlayer].oposy, sActor, sPlayer);
		break;

	case PLAYER_OPOSZ:
		if (bSet) ps[iPlayer].oposz = lValue;
		else SetGameVarID((int)lVar2, ps[iPlayer].oposz, sActor, sPlayer);
		break;

	case PLAYER_PYOFF:
		if (bSet) ps[iPlayer].pyoff = lValue;
		else SetGameVarID((int)lVar2, ps[iPlayer].pyoff, sActor, sPlayer);
		break;

	case PLAYER_OPYOFF:
		if (bSet) ps[iPlayer].opyoff = lValue;
		else SetGameVarID((int)lVar2, ps[iPlayer].opyoff, sActor, sPlayer);
		break;

	case PLAYER_POSXV:
		if (bSet) ps[iPlayer].posxv = lValue;
		else SetGameVarID((int)lVar2, ps[iPlayer].posxv, sActor, sPlayer);
		break;

	case PLAYER_POSYV:
		if (bSet) ps[iPlayer].posyv = lValue;
		else SetGameVarID((int)lVar2, ps[iPlayer].posyv, sActor, sPlayer);
		break;

	case PLAYER_POSZV:
		if (bSet) ps[iPlayer].poszv = lValue;
		else SetGameVarID((int)lVar2, ps[iPlayer].poszv, sActor, sPlayer);
		break;

	case PLAYER_LAST_PISSED_TIME:
		if (bSet) ps[iPlayer].last_pissed_time = lValue;
		else SetGameVarID((int)lVar2, ps[iPlayer].last_pissed_time, sActor, sPlayer);
		break;

	case PLAYER_TRUEFZ:
		if (bSet) ps[iPlayer].truefz = lValue;
		else SetGameVarID((int)lVar2, ps[iPlayer].truefz, sActor, sPlayer);
		break;

	case PLAYER_TRUECZ:
 		if (bSet) ps[iPlayer].truecz = lValue;
		else SetGameVarID((int)lVar2, ps[iPlayer].truecz, sActor, sPlayer);
		break;

	case PLAYER_PLAYER_PAR:
		if (bSet) ps[iPlayer].player_par = lValue;
		else SetGameVarID((int)lVar2, ps[iPlayer].player_par, sActor, sPlayer);
		break;

	case PLAYER_VISIBILITY:
		if (bSet) ps[iPlayer].visibility = lValue;
		else SetGameVarID((int)lVar2, ps[iPlayer].visibility, sActor, sPlayer);
		break;

	case PLAYER_BOBCOUNTER:
		if (bSet) ps[iPlayer].bobcounter = lValue;
		else SetGameVarID((int)lVar2, ps[iPlayer].bobcounter, sActor, sPlayer);
		break;

	case PLAYER_WEAPON_SWAY:
		if (bSet) ps[iPlayer].weapon_sway = lValue;
		else SetGameVarID((int)lVar2, ps[iPlayer].weapon_sway, sActor, sPlayer);
		break;

	case PLAYER_PALS_TIME:
		if (bSet) ps[iPlayer].pals.a = lValue;
		else SetGameVarID((int)lVar2, ps[iPlayer].pals.a, sActor, sPlayer);
		break;

	case PLAYER_RANDOMFLAMEX:
		if (bSet) ps[iPlayer].randomflamex = lValue;
		else SetGameVarID((int)lVar2, ps[iPlayer].randomflamex, sActor, sPlayer);
		break;

	case PLAYER_CRACK_TIME:
		if (bSet) ps[iPlayer].crack_time = lValue;
		else SetGameVarID((int)lVar2, ps[iPlayer].crack_time, sActor, sPlayer);
		break;

	case PLAYER_AIM_MODE: // game has no business enforcing this in any way.
		if (!bSet) SetGameVarID((int)lVar2, ps[iPlayer].aim_mode, sActor, sPlayer);
		break;

	case PLAYER_ANG:
		if (bSet) playerSetAngle(&ps[iPlayer].q16ang, &ps[iPlayer].angTarget, lValue);
		else SetGameVarID((int)lVar2, ps[iPlayer].getang(), sActor, sPlayer);
		break;

	case PLAYER_OANG:
		if (!bSet) SetGameVarID((int)lVar2, ps[iPlayer].getang(), sActor, sPlayer);
		break;

	case PLAYER_ANGVEL: // This no longer exists.
		if (!bSet) SetGameVarID((int)lVar2, 0, sActor, sPlayer);
		break;

	case PLAYER_CURSECTNUM:
		if (bSet) ps[iPlayer].cursectnum = lValue;
		else SetGameVarID((int)lVar2, ps[iPlayer].cursectnum, sActor, sPlayer);
		break;

	case PLAYER_LOOK_ANG:
		if (bSet) ps[iPlayer].q16look_ang = IntToFixed(lValue);
		else SetGameVarID((int)lVar2, FixedToInt(ps[iPlayer].q16look_ang), sActor, sPlayer);
		break;

	case PLAYER_LAST_EXTRA:
		if (bSet) ps[iPlayer].last_extra = lValue;
		else SetGameVarID((int)lVar2, ps[iPlayer].last_extra, sActor, sPlayer);
		break;

	case PLAYER_SUBWEAPON:
		if (bSet) ps[iPlayer].subweapon = lValue;
		else SetGameVarID((int)lVar2, ps[iPlayer].subweapon, sActor, sPlayer);
		break;

	case PLAYER_AMMO_AMOUNT:
		lTemp = GetGameVarID((int)lParm2, sActor, sPlayer);
		if (bSet) ps[iPlayer].ammo_amount[lTemp] = lValue;
		else SetGameVarID((int)lVar2, ps[iPlayer].ammo_amount[lTemp], sActor, sPlayer);
		break;

	case PLAYER_WACKEDBYACTOR:
		if (bSet) ps[iPlayer].wackedbyactor = lValue;
		else SetGameVarID((int)lVar2, ps[iPlayer].wackedbyactor, sActor, sPlayer);
		break;

	case PLAYER_FRAG:
		if (bSet) ps[iPlayer].frag = lValue;
		else SetGameVarID((int)lVar2, ps[iPlayer].frag, sActor, sPlayer);
		break;

	case PLAYER_FRAGGEDSELF:
		if (bSet) ps[iPlayer].fraggedself = lValue;
		else SetGameVarID((int)lVar2, ps[iPlayer].fraggedself, sActor, sPlayer);
		break;

	case PLAYER_CURR_WEAPON:
		if (bSet) ps[iPlayer].curr_weapon = lValue;
		else SetGameVarID((int)lVar2, ps[iPlayer].curr_weapon, sActor, sPlayer);
		break;

	case PLAYER_LAST_WEAPON:
		if (bSet) ps[iPlayer].last_weapon = lValue;
		else SetGameVarID((int)lVar2, ps[iPlayer].last_weapon, sActor, sPlayer);
		break;

	case PLAYER_TIPINCS:
		if (bSet) ps[iPlayer].tipincs = lValue;
		else SetGameVarID((int)lVar2, ps[iPlayer].tipincs, sActor, sPlayer);
		break;

	case PLAYER_HORIZOFF:
		if (bSet) ps[iPlayer].q16horizoff = IntToFixed(lValue);
		else SetGameVarID((int)lVar2, FixedToInt(ps[iPlayer].q16horizoff), sActor, sPlayer);
		break;

	case PLAYER_WANTWEAPONFIRE:
		if (bSet) ps[iPlayer].wantweaponfire = lValue;
		else SetGameVarID((int)lVar2, ps[iPlayer].wantweaponfire, sActor, sPlayer);
		break;

	case PLAYER_HOLODUKE_AMOUNT:
		if (bSet) ps[iPlayer].holoduke_amount = lValue;
		else SetGameVarID((int)lVar2, ps[iPlayer].holoduke_amount, sActor, sPlayer);
		break;

	case PLAYER_NEWOWNER:
		if (bSet) ps[iPlayer].newowner = lValue;
		else SetGameVarID((int)lVar2, ps[iPlayer].newowner, sActor, sPlayer);
		break;

	case PLAYER_HURT_DELAY:
		if (bSet) ps[iPlayer].hurt_delay = lValue;
		else SetGameVarID((int)lVar2, ps[iPlayer].hurt_delay, sActor, sPlayer);
		break;

	case PLAYER_HBOMB_HOLD_DELAY:
		if (bSet) ps[iPlayer].hbomb_hold_delay = lValue;
		else SetGameVarID((int)lVar2, ps[iPlayer].hbomb_hold_delay, sActor, sPlayer);
		break;

	case PLAYER_JUMPING_COUNTER:
		if (bSet) ps[iPlayer].jumping_counter = lValue;
		else SetGameVarID((int)lVar2, ps[iPlayer].jumping_counter, sActor, sPlayer);
		break;

	case PLAYER_AIRLEFT:
		if (bSet) ps[iPlayer].airleft = lValue;
		else SetGameVarID((int)lVar2, ps[iPlayer].airleft, sActor, sPlayer);
		break;

	case PLAYER_KNEE_INCS:
		if (bSet) ps[iPlayer].knee_incs = lValue;
		else SetGameVarID((int)lVar2, ps[iPlayer].knee_incs, sActor, sPlayer);
		break;

	case PLAYER_ACCESS_INCS:
		if (bSet) ps[iPlayer].access_incs = lValue;
		else SetGameVarID((int)lVar2, ps[iPlayer].access_incs, sActor, sPlayer);
		break;

	case PLAYER_ACCESS_WALLNUM:
		if (bSet) ps[iPlayer].access_wallnum = lValue;
		else SetGameVarID((int)lVar2, ps[iPlayer].access_wallnum, sActor, sPlayer);
		break;

	case PLAYER_ACCESS_SPRITENUM:
		if (bSet) ps[iPlayer].access_spritenum = lValue;
		else SetGameVarID((int)lVar2, ps[iPlayer].access_spritenum, sActor, sPlayer);
		break;

	case PLAYER_KICKBACK_PIC:
		if (bSet) ps[iPlayer].kickback_pic = lValue;
		else SetGameVarID((int)lVar2, ps[iPlayer].kickback_pic, sActor, sPlayer);
		break;

	case PLAYER_GOT_ACCESS:
		if (bSet) ps[iPlayer].got_access = lValue;
		else SetGameVarID((int)lVar2, ps[iPlayer].got_access, sActor, sPlayer);
		break;

	case PLAYER_WEAPON_ANG:
		if (bSet) ps[iPlayer].weapon_ang = lValue;
		else SetGameVarID((int)lVar2, ps[iPlayer].weapon_ang, sActor, sPlayer);
		break;

	case PLAYER_FIRSTAID_AMOUNT:
		if (bSet) ps[iPlayer].firstaid_amount = lValue;
		else SetGameVarID((int)lVar2, ps[iPlayer].firstaid_amount, sActor, sPlayer);
		break;

	case PLAYER_SOMETHINGONPLAYER:
		if (bSet) ps[iPlayer].somethingonplayer = lValue;
		else SetGameVarID((int)lVar2, ps[iPlayer].somethingonplayer, sActor, sPlayer);
		break;

	case PLAYER_ON_CRANE:
		if (bSet) ps[iPlayer].on_crane = lValue;
		else SetGameVarID((int)lVar2, ps[iPlayer].on_crane, sActor, sPlayer);
		break;

	case PLAYER_I:	// This is dangerous!!!
		if (bSet) ps[iPlayer].i = lValue;
		else SetGameVarID((int)lVar2, ps[iPlayer].i, sActor, sPlayer);
		break;

	case PLAYER_ONE_PARALLAX_SECTNUM:
		if (bSet) ps[iPlayer].one_parallax_sectnum = lValue;
		else SetGameVarID((int)lVar2, ps[iPlayer].one_parallax_sectnum, sActor, sPlayer);
		break;

	case PLAYER_OVER_SHOULDER_ON:
		if (bSet) ps[iPlayer].over_shoulder_on = lValue;
		else SetGameVarID((int)lVar2, ps[iPlayer].over_shoulder_on, sActor, sPlayer);
		break;

	case PLAYER_RANDOM_CLUB_FRAME:
		if (bSet) ps[iPlayer].random_club_frame = lValue;
		else SetGameVarID((int)lVar2, ps[iPlayer].random_club_frame, sActor, sPlayer);
		break;

	case PLAYER_FIST_INCS:
		if (bSet) 	ps[iPlayer].fist_incs = lValue;
		else SetGameVarID((int)lVar2, ps[iPlayer].fist_incs, sActor, sPlayer);
		break;

	case PLAYER_ONE_EIGHTY_COUNT:
		if (bSet) ps[iPlayer].one_eighty_count = lValue;
		else SetGameVarID((int)lVar2, ps[iPlayer].one_eighty_count, sActor, sPlayer);
		break;

	case PLAYER_CHEAT_PHASE:
		if (bSet) ps[iPlayer].cheat_phase = lValue;
		else SetGameVarID((int)lVar2, ps[iPlayer].cheat_phase, sActor, sPlayer);
		break;

	case PLAYER_DUMMYPLAYERSPRITE:
		if (bSet) ps[iPlayer].dummyplayersprite = lValue;
		else SetGameVarID((int)lVar2, ps[iPlayer].dummyplayersprite, sActor, sPlayer);
		break;

	case PLAYER_EXTRA_EXTRA8:
		if (bSet) ps[iPlayer].extra_extra8 = lValue;
		else SetGameVarID((int)lVar2, ps[iPlayer].extra_extra8, sActor, sPlayer);
		break;

	case PLAYER_QUICK_KICK:
		if (bSet) ps[iPlayer].quick_kick = lValue;
		else SetGameVarID((int)lVar2, ps[iPlayer].quick_kick, sActor, sPlayer);
		break;

	case PLAYER_HEAT_AMOUNT:
		if (bSet) ps[iPlayer].heat_amount = lValue;
		else SetGameVarID((int)lVar2, ps[iPlayer].heat_amount, sActor, sPlayer);
		break;

	case PLAYER_ACTORSQU:
		if (bSet) ps[iPlayer].actorsqu = lValue;
		else SetGameVarID((int)lVar2, ps[iPlayer].actorsqu, sActor, sPlayer);
		break;

	case PLAYER_TIMEBEFOREEXIT:
		if (bSet) ps[iPlayer].timebeforeexit = lValue;
		else SetGameVarID((int)lVar2, ps[iPlayer].timebeforeexit, sActor, sPlayer);
		break;

	case PLAYER_CUSTOMEXITSOUND:
		if (bSet) ps[iPlayer].customexitsound = lValue;
		else SetGameVarID((int)lVar2, ps[iPlayer].customexitsound, sActor, sPlayer);
		break;

	case PLAYER_WEAPRECS:
		if (bSet) ps[iPlayer].weaprecs[lParm2] = lValue;
		else SetGameVarID((int)lVar2, ps[iPlayer].weaprecs[lParm2], sActor, sPlayer);
		break;

	case PLAYER_WEAPRECCNT:
		if (bSet) ps[iPlayer].weapreccnt = lValue;
		else SetGameVarID((int)lVar2, ps[iPlayer].weapreccnt, sActor, sPlayer);
		break;

	case PLAYER_INTERFACE_TOGGLE_FLAG:
		if (bSet) ps[iPlayer].interface_toggle_flag = lValue;
		else SetGameVarID((int)lVar2, ps[iPlayer].interface_toggle_flag, sActor, sPlayer);
		break;

	case PLAYER_ROTSCRNANG:
		if (bSet) ps[iPlayer].setrotscrnang(lValue);
		else SetGameVarID((int)lVar2, FixedToInt(ps[iPlayer].q16rotscrnang), sActor, sPlayer);
		break;

	case PLAYER_DEAD_FLAG:
		if (bSet) ps[iPlayer].dead_flag = lValue;
		else SetGameVarID((int)lVar2, ps[iPlayer].dead_flag, sActor, sPlayer);
		break;

	case PLAYER_SHOW_EMPTY_WEAPON:
		if (bSet) ps[iPlayer].show_empty_weapon = lValue;
		else SetGameVarID((int)lVar2, ps[iPlayer].show_empty_weapon, sActor, sPlayer);
		break;

	case PLAYER_SCUBA_AMOUNT:
		if (bSet) ps[iPlayer].scuba_amount = lValue;
		else SetGameVarID((int)lVar2, ps[iPlayer].scuba_amount, sActor, sPlayer);
		break;

	case PLAYER_JETPACK_AMOUNT:
		if (bSet) ps[iPlayer].jetpack_amount = lValue;
		else SetGameVarID((int)lVar2, ps[iPlayer].jetpack_amount, sActor, sPlayer);
		break;

	case PLAYER_STEROIDS_AMOUNT:
		if (bSet) ps[iPlayer].steroids_amount = lValue;
		else SetGameVarID((int)lVar2, ps[iPlayer].steroids_amount, sActor, sPlayer);
		break;

	case PLAYER_SHIELD_AMOUNT:
		if (bSet) ps[iPlayer].shield_amount = lValue;
		else SetGameVarID((int)lVar2, ps[iPlayer].shield_amount, sActor, sPlayer);
		break;

	case PLAYER_HOLODUKE_ON:
		if (bSet) ps[iPlayer].holoduke_on = lValue;
		else SetGameVarID((int)lVar2, ps[iPlayer].holoduke_on, sActor, sPlayer);
		break;

	case PLAYER_PYCOUNT:
		if (bSet) ps[iPlayer].pycount = lValue;
		else SetGameVarID((int)lVar2, ps[iPlayer].pycount, sActor, sPlayer);
		break;

	case PLAYER_WEAPON_POS:
		if (bSet) ps[iPlayer].weapon_pos = lValue;
		else SetGameVarID((int)lVar2, ps[iPlayer].weapon_pos, sActor, sPlayer);
		break;

	case PLAYER_FRAG_PS:
		if (bSet) 	ps[iPlayer].frag_ps = lValue;
		else SetGameVarID((int)lVar2, ps[iPlayer].frag_ps, sActor, sPlayer);
		break;

	case PLAYER_TRANSPORTER_HOLD:
		if (bSet) ps[iPlayer].transporter_hold = lValue;
		else SetGameVarID((int)lVar2, ps[iPlayer].transporter_hold, sActor, sPlayer);
		break;

	case PLAYER_LAST_FULL_WEAPON:
		if (bSet) ps[iPlayer].last_full_weapon = lValue;
		else SetGameVarID((int)lVar2, ps[iPlayer].last_full_weapon, sActor, sPlayer);
		break;

	case PLAYER_FOOTPRINTSHADE:
		if (bSet) ps[iPlayer].footprintshade = lValue;
		else SetGameVarID((int)lVar2, ps[iPlayer].footprintshade, sActor, sPlayer);
		break;

	case PLAYER_BOOT_AMOUNT:
		if (bSet) ps[iPlayer].boot_amount = lValue;
		else SetGameVarID((int)lVar2, ps[iPlayer].boot_amount, sActor, sPlayer);
		break;

	case PLAYER_GM:
		if (!bSet) SetGameVarID((int)lVar2, MODE_GAME, sActor, sPlayer);
		break;

	case PLAYER_ON_WARPING_SECTOR:
		if (bSet) ps[iPlayer].on_warping_sector = lValue;
		else SetGameVarID((int)lVar2, ps[iPlayer].on_warping_sector, sActor, sPlayer);
		break;

	case PLAYER_FOOTPRINTCOUNT:
		if (bSet) ps[iPlayer].footprintcount = lValue;
		else SetGameVarID((int)lVar2, ps[iPlayer].footprintcount, sActor, sPlayer);
		break;

	case PLAYER_HBOMB_ON:
		if (bSet) ps[iPlayer].hbomb_on = lValue;
		else SetGameVarID((int)lVar2, ps[iPlayer].hbomb_on, sActor, sPlayer);
		break;

	case PLAYER_JUMPING_TOGGLE:
		if (bSet) ps[iPlayer].jumping_toggle = lValue;
		else SetGameVarID((int)lVar2, ps[iPlayer].jumping_toggle, sActor, sPlayer);
		break;

	case PLAYER_RAPID_FIRE_HOLD:
		if (bSet) ps[iPlayer].rapid_fire_hold = lValue;
		else SetGameVarID((int)lVar2, ps[iPlayer].rapid_fire_hold, sActor, sPlayer);
		break;

	case PLAYER_ON_GROUND:
		if (bSet) ps[iPlayer].on_ground = lValue;
		else SetGameVarID((int)lVar2, ps[iPlayer].on_ground, sActor, sPlayer);
		break;

	case PLAYER_INVEN_ICON:
		if (bSet) ps[iPlayer].inven_icon = lValue;
		else SetGameVarID((int)lVar2, ps[iPlayer].inven_icon, sActor, sPlayer);
		break;

	case PLAYER_BUTTONPALETTE:
		if (bSet) ps[iPlayer].buttonpalette = lValue;
		else SetGameVarID((int)lVar2, ps[iPlayer].buttonpalette, sActor, sPlayer);
		break;

	case PLAYER_JETPACK_ON:
		if (bSet) ps[iPlayer].jetpack_on = lValue;
		else SetGameVarID((int)lVar2, ps[iPlayer].jetpack_on, sActor, sPlayer);
		break;

	case PLAYER_SPRITEBRIDGE:
		if (bSet) ps[iPlayer].spritebridge = lValue;
		else SetGameVarID((int)lVar2, ps[iPlayer].spritebridge, sActor, sPlayer);
		break;

	case PLAYER_LASTRANDOMSPOT:
		if (bSet) ps[iPlayer].lastrandomspot = lValue;
		else SetGameVarID((int)lVar2, ps[iPlayer].lastrandomspot, sActor, sPlayer);
		break;

	case PLAYER_SCUBA_ON:
		if (bSet) ps[iPlayer].scuba_on = lValue;
		else SetGameVarID((int)lVar2, ps[iPlayer].scuba_on, sActor, sPlayer);
		break;

	case PLAYER_FOOTPRINTPAL:
		if (bSet) ps[iPlayer].footprintpal = lValue;
		else SetGameVarID((int)lVar2, ps[iPlayer].footprintpal, sActor, sPlayer);
		break;

	case PLAYER_HEAT_ON:
		if (bSet) ps[iPlayer].heat_on = lValue;
		else SetGameVarID((int)lVar2, ps[iPlayer].heat_on, sActor, sPlayer);
		break;

	case PLAYER_HOLSTER_WEAPON:
		if (bSet) ps[iPlayer].holster_weapon = lValue;
		else SetGameVarID((int)lVar2, ps[iPlayer].holster_weapon, sActor, sPlayer);
		break;

	case PLAYER_FALLING_COUNTER:
		if (bSet) ps[iPlayer].falling_counter = lValue;
		else SetGameVarID((int)lVar2, ps[iPlayer].falling_counter, sActor, sPlayer);
		break;

	case PLAYER_GOTWEAPON:
		if (bSet) ps[iPlayer].gotweapon.Set(lParm2, lValue);
		else SetGameVarID((int)lVar2, ps[iPlayer].gotweapon[lParm2], sActor, sPlayer);
		break;

	case PLAYER_REFRESH_INVENTORY:
		if (bSet) ps[iPlayer].refresh_inventory = lValue;
		else SetGameVarID((int)lVar2, ps[iPlayer].refresh_inventory, sActor, sPlayer);
		break;

	case PLAYER_TOGGLE_KEY_FLAG:
		if (bSet) ps[iPlayer].toggle_key_flag = lValue;
		else SetGameVarID((int)lVar2, ps[iPlayer].toggle_key_flag, sActor, sPlayer);
		break;

	case PLAYER_KNUCKLE_INCS:
		if (bSet) ps[iPlayer].knuckle_incs = lValue;
		else SetGameVarID((int)lVar2, ps[iPlayer].knuckle_incs, sActor, sPlayer);
		break;

	case PLAYER_WALKING_SND_TOGGLE:
		if (bSet) ps[iPlayer].walking_snd_toggle = lValue;
		else SetGameVarID((int)lVar2, ps[iPlayer].walking_snd_toggle, sActor, sPlayer);
		break;

	case PLAYER_PALOOKUP:
		if (bSet) ps[iPlayer].palookup = lValue;
		else SetGameVarID((int)lVar2, ps[iPlayer].palookup, sActor, sPlayer);
		break;

	case PLAYER_HARD_LANDING:
		if (bSet) ps[iPlayer].hard_landing = lValue;
		else SetGameVarID((int)lVar2, ps[iPlayer].hard_landing, sActor, sPlayer);
		break;

	case PLAYER_MAX_SECRET_ROOMS:
		if (bSet) ps[iPlayer].max_secret_rooms = lValue;
		else SetGameVarID((int)lVar2, ps[iPlayer].max_secret_rooms, sActor, sPlayer);
		break;

	case PLAYER_SECRET_ROOMS:
		if (bSet) ps[iPlayer].secret_rooms = lValue;
		else SetGameVarID((int)lVar2, ps[iPlayer].secret_rooms, sActor, sPlayer);
		break;

	case PLAYER_MAX_ACTORS_KILLED:
		if (bSet) ps[iPlayer].max_actors_killed = lValue;
		else SetGameVarID((int)lVar2, ps[iPlayer].max_actors_killed, sActor, sPlayer);
		break;

	case PLAYER_ACTORS_KILLED:
		if (bSet) ps[iPlayer].actors_killed = lValue;
		else SetGameVarID((int)lVar2, ps[iPlayer].actors_killed, sActor, sPlayer);
		break;

	default:
		if (!bSet) SetGameVarID((int)lVar2, 0, sActor, sPlayer);
		break;
	}
	return;
}

////////////////////
void DoWall(char bSet, int lVar1, int lLabelID, int lVar2, short sActor, short sPlayer, int lParm2)
{
	int iWall;
	int lValue;

	lValue = GetGameVarID((int)lVar2, sActor, sPlayer);

	iWall = GetGameVarID((int)lVar1, sActor, sPlayer);

	if (iWall < 0 || iWall >= numwalls)
	{
		if (!bSet) SetGameVarID((int)lVar2, 0, sActor, sPlayer);
		return;
	}

	// All fields affecting map geometry have been made read-only!
	switch (lLabelID)
	{
	case WALL_X:
		if (!bSet) SetGameVarID((int)lVar2, wall[iWall].x, sActor, sPlayer);
		break;
	case WALL_Y:
		if (bSet) SetGameVarID((int)lVar2, wall[iWall].y, sActor, sPlayer);
		break;
	case WALL_POINT2:
		if (!bSet) SetGameVarID((int)lVar2, wall[iWall].point2, sActor, sPlayer);
		break;
	case WALL_NEXTWALL:
		if (!bSet) SetGameVarID((int)lVar2, wall[iWall].nextwall, sActor, sPlayer);
		break;
	case WALL_NEXTSECTOR:
		if (!bSet) SetGameVarID((int)lVar2, wall[iWall].nextsector, sActor, sPlayer);
		break;
	case WALL_CSTAT:
		if (bSet) wall[iWall].cstat = lValue;
		else SetGameVarID((int)lVar2, wall[iWall].cstat, sActor, sPlayer);
		break;
	case WALL_PICNUM:
		if (bSet) wall[iWall].picnum = lValue;
		else SetGameVarID((int)lVar2, wall[iWall].picnum, sActor, sPlayer);
		break;
	case WALL_OVERPICNUM:
		if (bSet) wall[iWall].overpicnum = lValue;
		else SetGameVarID((int)lVar2, wall[iWall].overpicnum, sActor, sPlayer);
		break;
	case WALL_SHADE:
		if (bSet) wall[iWall].x = lValue;
		else SetGameVarID((int)lVar2, wall[iWall].shade, sActor, sPlayer);
		break;
	case WALL_PAL:
		if (bSet) wall[iWall].pal = lValue;
		else SetGameVarID((int)lVar2, wall[iWall].pal, sActor, sPlayer);
		break;
	case WALL_XREPEAT:
		if (bSet) wall[iWall].xrepeat = lValue;
		else SetGameVarID((int)lVar2, wall[iWall].xrepeat, sActor, sPlayer);
		break;
	case WALL_YREPEAT:
		if (bSet) wall[iWall].yrepeat = lValue;
		else SetGameVarID((int)lVar2, wall[iWall].yrepeat, sActor, sPlayer);
		break;
	case WALL_XPANNING:
		if (bSet) wall[iWall].xpanning = lValue;
		else SetGameVarID((int)lVar2, wall[iWall].xpanning, sActor, sPlayer);
		break;
	case WALL_YPANNING:
		if (bSet) wall[iWall].ypanning = lValue;
		else SetGameVarID((int)lVar2, wall[iWall].ypanning, sActor, sPlayer);
		break;
	case WALL_LOTAG:
		if (bSet) wall[iWall].lotag = lValue;
		else SetGameVarID((int)lVar2, wall[iWall].lotag, sActor, sPlayer);
		break;
	case WALL_HITAG:
		if (bSet) wall[iWall].hitag = lValue;
		else SetGameVarID((int)lVar2, wall[iWall].hitag, sActor, sPlayer);
		break;
	case WALL_EXTRA:
		if (bSet) wall[iWall].x = lValue;
		else SetGameVarID((int)lVar2, wall[iWall].x, sActor, sPlayer);
		break;
	default:
		break;
	}
	return;
}

void DoSector(char bSet, int lVar1, int lLabelID, int lVar2, short sActor, short sPlayer, int lParm2)
{
	int iSector;
	int lValue;


	if (lVar1 == g_iThisActorID)
	{
		// if they've asked for 'this', then use 'this'...
		iSector = sprite[sActor].sectnum;
	}
	else
	{
		iSector = GetGameVarID((int)lVar1, sActor, sPlayer);
	}

	if (iSector < 0 || iSector >= numsectors)
	{
		if (!bSet) SetGameVarID((int)lVar2, 0, sActor, sPlayer);
		return;
	}

	lValue = GetGameVarID((int)lVar2, sActor, sPlayer);

	// All fields affecting map geometry have been made read-only!
	switch (lLabelID)
	{
	case SECTOR_WALLPTR:
		if (!bSet) SetGameVarID((int)lVar2, sector[iSector].wallptr, sActor, sPlayer);
		break;
	case SECTOR_WALLNUM:
		if (!bSet) SetGameVarID((int)lVar2, sector[iSector].wallnum, sActor, sPlayer);
		break;
	case SECTOR_CEILINGZ:
		if (bSet) sector[iSector].ceilingz = lValue;
		else SetGameVarID((int)lVar2, sector[iSector].ceilingz, sActor, sPlayer);
		break;
	case SECTOR_FLOORZ:
		if (bSet) sector[iSector].floorz = lValue;
		else SetGameVarID((int)lVar2, sector[iSector].floorz, sActor, sPlayer);
		break;
	case SECTOR_CEILINGSTAT:
		if (bSet) sector[iSector].ceilingstat = lValue;
		else SetGameVarID((int)lVar2, sector[iSector].ceilingstat, sActor, sPlayer);
		break;
	case SECTOR_FLOORSTAT:
		if (bSet) sector[iSector].floorstat = lValue;
		else SetGameVarID((int)lVar2, sector[iSector].floorstat, sActor, sPlayer);
		break;
	case SECTOR_CEILINGPICNUM:
		if (bSet) sector[iSector].ceilingpicnum = lValue;
		else SetGameVarID((int)lVar2, sector[iSector].ceilingpicnum, sActor, sPlayer);
		break;
	case SECTOR_CEILINGSLOPE:
		if (bSet) sector[iSector].ceilingheinum = lValue;
		else SetGameVarID((int)lVar2, sector[iSector].ceilingheinum, sActor, sPlayer);
		break;
	case SECTOR_CEILINGSHADE:
		if (bSet) sector[iSector].ceilingshade = lValue;
		else SetGameVarID((int)lVar2, sector[iSector].ceilingshade, sActor, sPlayer);
		break;
	case SECTOR_CEILINGPAL:
		if (bSet) sector[iSector].ceilingpal = lValue;
		else SetGameVarID((int)lVar2, sector[iSector].ceilingpal, sActor, sPlayer);
		break;
	case SECTOR_CEILINGXPANNING:
		if (bSet) sector[iSector].ceilingxpanning = lValue;
		else SetGameVarID((int)lVar2, sector[iSector].ceilingypanning, sActor, sPlayer);
		break;
	case SECTOR_FLOORPICNUM:
		if (bSet) sector[iSector].floorpicnum = lValue;
		else SetGameVarID((int)lVar2, sector[iSector].floorpicnum, sActor, sPlayer);
		break;
	case SECTOR_FLOORSLOPE:
		if (bSet) sector[iSector].floorheinum = lValue;
		else SetGameVarID((int)lVar2, sector[iSector].floorheinum, sActor, sPlayer);
		break;
	case SECTOR_FLOORSHADE:
		if (bSet) sector[iSector].floorshade = lValue;
		else SetGameVarID((int)lVar2, sector[iSector].floorshade, sActor, sPlayer);
		break;
	case SECTOR_FLOORPAL:
		if (bSet) sector[iSector].floorpal = lValue;
		else SetGameVarID((int)lVar2, sector[iSector].floorpal, sActor, sPlayer);
		break;
	case SECTOR_FLOORXPANNING:
		if (bSet) sector[iSector].floorxpanning = lValue;
		else SetGameVarID((int)lVar2, sector[iSector].floorxpanning, sActor, sPlayer);
		break;
	case SECTOR_FLOORYPANNING:
		if (bSet) sector[iSector].floorypanning = lValue;
		else SetGameVarID((int)lVar2, sector[iSector].floorypanning, sActor, sPlayer);
		break;
	case SECTOR_VISIBILITY:
		if (bSet) sector[iSector].visibility = lValue;
		else SetGameVarID((int)lVar2, sector[iSector].visibility, sActor, sPlayer);
		break;
	case SECTOR_LOTAG:
		if (bSet) sector[iSector].lotag = lValue;
		else SetGameVarID((int)lVar2, sector[iSector].lotag, sActor, sPlayer);
		break;
	case SECTOR_HITAG:
		if (bSet) sector[iSector].hitag = lValue;
		else SetGameVarID((int)lVar2, sector[iSector].hitag, sActor, sPlayer);
		break;
	case SECTOR_EXTRA:
		if (bSet) sector[iSector].extra = lValue;
		else SetGameVarID((int)lVar2, sector[iSector].extra, sActor, sPlayer);
		break;
	default:
		break;

	}
	return;
}
void DoActor(char bSet, int lVar1, int lLabelID, int lVar2, short sActor, short sPlayer, int lParm2)
{
	int iActor;
	int lValue;

	lValue = GetGameVarID((int)lVar2, sActor, sPlayer);

	if (lVar1 == g_iThisActorID)
	{
		// if they've asked for 'this', then use 'this'...
		iActor = sActor;
	}
	else
	{
		iActor = GetGameVarID((int)lVar1, sActor, sPlayer);
	}

	if (iActor < 0 || iActor >= MAXSPRITES || sprite[iActor].statnum == MAXSTATUS)
	{
		if (!bSet) SetGameVarID((int)lVar2, 0, sActor, sPlayer);
		return;
	}

	switch (lLabelID)
	{
	case ACTOR_X:
		if (bSet) sprite[iActor].x = lValue;
		else SetGameVarID((int)lVar2, sprite[iActor].x, sActor, sPlayer);
		break;
	case ACTOR_Y:
		if (bSet) sprite[iActor].y = lValue;
		else SetGameVarID((int)lVar2, sprite[iActor].y, sActor, sPlayer);
		break;
	case ACTOR_Z:
		if (bSet) sprite[iActor].z = lValue;
		else SetGameVarID((int)lVar2, sprite[iActor].z, sActor, sPlayer);
		break;
	case ACTOR_CSTAT:
		if (bSet) sprite[iActor].cstat = lValue;
		else SetGameVarID((int)lVar2, sprite[iActor].cstat, sActor, sPlayer);
		break;
	case ACTOR_PICNUM:
		if (bSet) sprite[iActor].picnum = lValue;
		else SetGameVarID((int)lVar2, sprite[iActor].picnum, sActor, sPlayer);
		break;
	case ACTOR_SHADE:
		if (bSet) sprite[iActor].shade = lValue;
		else SetGameVarID((int)lVar2, sprite[iActor].shade, sActor, sPlayer);
		break;
	case ACTOR_PAL:
		if (bSet) sprite[iActor].pal = lValue;
		else SetGameVarID((int)lVar2, sprite[iActor].pal, sActor, sPlayer);
		break;
	case ACTOR_CLIPDIST:
		if (bSet) sprite[iActor].clipdist = lValue;
		else SetGameVarID((int)lVar2, sprite[iActor].clipdist, sActor, sPlayer);
		break;
	case ACTOR_DETAIL:
		if (bSet) hittype[iActor].spriteextra = lValue;
		else SetGameVarID((int)lVar2, hittype[iActor].spriteextra, sActor, sPlayer);
		break;
	case ACTOR_XREPEAT:
		if (bSet) sprite[iActor].xrepeat = lValue;
		else SetGameVarID((int)lVar2, sprite[iActor].xrepeat, sActor, sPlayer);
		break;
	case ACTOR_YREPEAT:
		if (bSet) sprite[iActor].yrepeat = lValue;
		else SetGameVarID((int)lVar2, sprite[iActor].yrepeat, sActor, sPlayer);
		break;
	case ACTOR_XOFFSET:
		if (bSet) sprite[iActor].xoffset = lValue;
		else SetGameVarID((int)lVar2, sprite[iActor].xoffset, sActor, sPlayer);
		break;
	case ACTOR_YOFFSET:
		if (bSet) sprite[iActor].yoffset = lValue;
		else SetGameVarID((int)lVar2, sprite[iActor].yoffset, sActor, sPlayer);
		break;
	case ACTOR_SECTNUM:
		if (bSet) changespritesect(iActor, lValue);
		else SetGameVarID((int)lVar2, sprite[iActor].sectnum, sActor, sPlayer);
		break;
	case ACTOR_STATNUM: 
		if (bSet) changespritestat(iActor, lValue);
		else SetGameVarID((int)lVar2, sprite[iActor].statnum, sActor, sPlayer);
		break;
	case ACTOR_ANG:
		if (bSet) sprite[iActor].ang = lValue;
		else SetGameVarID((int)lVar2, sprite[iActor].ang, sActor, sPlayer);
		break;
	case ACTOR_OWNER:
		if (bSet) sprite[iActor].owner = lValue;
		else SetGameVarID((int)lVar2, sprite[iActor].owner, sActor, sPlayer);
		break;
	case ACTOR_XVEL:
		if (bSet) sprite[iActor].xvel = lValue;
		else SetGameVarID((int)lVar2, sprite[iActor].xvel, sActor, sPlayer);
		break;
	case ACTOR_YVEL:
		if (bSet) sprite[iActor].yvel = lValue;
		else SetGameVarID((int)lVar2, sprite[iActor].yvel, sActor, sPlayer);
		break;
	case ACTOR_ZVEL:
		if (bSet) sprite[iActor].zvel = lValue;
		else SetGameVarID((int)lVar2, sprite[iActor].zvel, sActor, sPlayer);
		break;
	case ACTOR_LOTAG:
		if (bSet) sprite[iActor].lotag = lValue;
		else SetGameVarID((int)lVar2, sprite[iActor].lotag, sActor, sPlayer);
		break;
	case ACTOR_HITAG:
		if (bSet) sprite[iActor].hitag = lValue;
		else SetGameVarID((int)lVar2, sprite[iActor].hitag, sActor, sPlayer);
		break;
	case ACTOR_EXTRA:
		if (bSet) sprite[iActor].extra = lValue;
		else SetGameVarID((int)lVar2, sprite[iActor].extra, sActor, sPlayer);
		break;

	case ACTOR_HTCGG:
		if (bSet) hittype[iActor].cgg = lValue;
		else SetGameVarID((int)lVar2, hittype[iActor].cgg, sActor, sPlayer);
		break;
	case ACTOR_HTPICNUM:
		if (bSet) hittype[iActor].picnum = lValue;
		else SetGameVarID((int)lVar2, hittype[iActor].picnum, sActor, sPlayer);
		break;
	case ACTOR_HTANG:
		if (bSet) hittype[iActor].ang = lValue;
		else SetGameVarID((int)lVar2, hittype[iActor].ang, sActor, sPlayer);
		break;
	case ACTOR_HTEXTRA:
		if (bSet) hittype[iActor].extra = lValue;
		else SetGameVarID((int)lVar2, hittype[iActor].extra, sActor, sPlayer);
		break;
	case ACTOR_HTOWNER:
		if (bSet) hittype[iActor].owner = lValue;
		else SetGameVarID((int)lVar2, hittype[iActor].owner, sActor, sPlayer);
		break;
	case ACTOR_HTMOVFLAG:
		if (bSet) hittype[iActor].movflag = lValue;
		else SetGameVarID((int)lVar2, hittype[iActor].movflag, sActor, sPlayer);
		break;
	case ACTOR_HTTEMPANG:
		if (bSet) hittype[iActor].tempang = lValue;
		else SetGameVarID((int)lVar2, hittype[iActor].tempang, sActor, sPlayer);
		break;
	case ACTOR_HTACTORSTAYPUT:
		if (bSet) hittype[iActor].actorstayput = lValue;
		else SetGameVarID((int)lVar2, hittype[iActor].actorstayput, sActor, sPlayer);
		break;
	case ACTOR_HTDISPICNUM:
		if (bSet) hittype[iActor].dispicnum = lValue;
		else SetGameVarID((int)lVar2, hittype[iActor].dispicnum, sActor, sPlayer);
		break;
	case ACTOR_HTTIMETOSLEEP:
		if (bSet) hittype[iActor].timetosleep = lValue;
		else SetGameVarID((int)lVar2, hittype[iActor].timetosleep, sActor, sPlayer);
		break;
	case ACTOR_HTFLOORZ:
		if (bSet) hittype[iActor].floorz = lValue;
		else SetGameVarID((int)lVar2, hittype[iActor].floorz, sActor, sPlayer);
		break;
	case ACTOR_HTCEILINGZ:
		if (bSet) hittype[iActor].ceilingz = lValue;
		else SetGameVarID((int)lVar2, hittype[iActor].ceilingz, sActor, sPlayer);
		break;
	case ACTOR_HTLASTVX:
		if (bSet) hittype[iActor].lastvx = lValue;
		else SetGameVarID((int)lVar2, hittype[iActor].lastvx, sActor, sPlayer);
		break;
	case ACTOR_HTLASTVY:
		if (bSet) hittype[iActor].lastvy = lValue;
		else SetGameVarID((int)lVar2, hittype[iActor].lastvy, sActor, sPlayer);
		break;
	case ACTOR_HTBPOSX:
		if (bSet) hittype[iActor].bposx = lValue;
		else SetGameVarID((int)lVar2, hittype[iActor].bposx, sActor, sPlayer);
		break;
	case ACTOR_HTBPOSY:
		if (bSet) hittype[iActor].bposy = lValue;
		else SetGameVarID((int)lVar2, hittype[iActor].bposy, sActor, sPlayer);
		break;
	case ACTOR_HTBPOSZ:
		if (bSet) hittype[iActor].bposz = lValue;
		else SetGameVarID((int)lVar2, hittype[iActor].bposz, sActor, sPlayer);
		break;
	case ACTOR_HTG_T0:
		if (bSet) hittype[iActor].temp_data[0] = lValue;
		else SetGameVarID((int)lVar2, hittype[iActor].temp_data[0], sActor, sPlayer);
		break;
	case ACTOR_HTG_T1:
		if (bSet) hittype[iActor].temp_data[1] = lValue;
		else SetGameVarID((int)lVar2, hittype[iActor].temp_data[1], sActor, sPlayer);
		break;
	case ACTOR_HTG_T2:
		if (bSet) hittype[iActor].temp_data[2] = lValue;
		else SetGameVarID((int)lVar2, hittype[iActor].temp_data[2], sActor, sPlayer);
		break;
	case ACTOR_HTG_T3:
		if (bSet) hittype[iActor].temp_data[3] = lValue;
		else SetGameVarID((int)lVar2, hittype[iActor].temp_data[3], sActor, sPlayer);
		break;
	case ACTOR_HTG_T4:
		if (bSet) hittype[iActor].temp_data[4] = lValue;
		else SetGameVarID((int)lVar2, hittype[iActor].temp_data[4], sActor, sPlayer);
		break;
	case ACTOR_HTG_T5:
		if (bSet) hittype[iActor].temp_data[5] = lValue;
		else SetGameVarID((int)lVar2, hittype[iActor].temp_data[5], sActor, sPlayer);
		break;

	default:
		break;
	}
	return;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void ParseState::parseifelse(int condition)
{
	if( condition )
	{
		// skip 'else' pointer.. and...
		insptr+=2;
		parse();
	}
	else
	{
		insptr = &ScriptCode[*(insptr+1)];
		if(*insptr == 10)
		{
			// else...

			// skip 'else' and...
			insptr+=2;
			parse();
		}
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

static int ifcanshoottarget(int g_i, int g_p, int g_x)
{
	int j;
	auto g_sp = &sprite[g_i];
	if (g_x > 1024)
	{
		short temphit, sclip, angdif;

		if (badguy(g_sp) && g_sp->xrepeat > 56)
		{
			sclip = 3084;
			angdif = 48;
		}
		else
		{
			sclip = 768;
			angdif = 16;
		}

		j = hitasprite(g_i, &temphit);
		if (j == (1 << 30))
		{
			return 1;
		}
		if (j > sclip)
		{
			if (temphit >= 0 && sprite[temphit].picnum == g_sp->picnum)
				j = 0;
			else
			{
				g_sp->ang += angdif; j = hitasprite(g_i, &temphit); g_sp->ang -= angdif;
				if (j > sclip)
				{
					if (temphit >= 0 && sprite[temphit].picnum == g_sp->picnum)
						j = 0;
					else
					{
						g_sp->ang -= angdif; j = hitasprite(g_i, &temphit); g_sp->ang += angdif;
						if (j > 768)
						{
							if (temphit >= 0 && sprite[temphit].picnum == g_sp->picnum)
								j = 0;
							else j = 1;
						}
						else j = 0;
					}
				}
				else j = 0;
			}
		}
		else j = 0;
	}
	else j = 1;
	return j;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

static bool ifcansee(int g_i, int g_p)
{
	auto g_sp = &sprite[g_i];
	spritetype* s;
	int j;

	// select sprite for monster to target
	// if holoduke is on, let them target holoduke first.
	// 
	if (ps[g_p].holoduke_on >= 0 && !isRR())
	{
		s = &sprite[ps[g_p].holoduke_on];
		j = cansee(g_sp->x, g_sp->y, g_sp->z - (krand() & ((32 << 8) - 1)), g_sp->sectnum,
			s->x, s->y, s->z, s->sectnum);

		if (j == 0)
		{
			// they can't see player's holoduke
			// check for player...
			s = &sprite[ps[g_p].i];
		}
	}
	else s = &sprite[ps[g_p].i];	// holoduke not on. look for player

	// can they see player, (or player's holoduke)
	j = cansee(g_sp->x, g_sp->y, g_sp->z - (krand() & ((47 << 8))), g_sp->sectnum,
		s->x, s->y, s->z - ((isRR()? 28 : 24) << 8), s->sectnum);

	if (j == 0)
	{
		// they can't see it.

		// Huh?.  This does nothing....
		// (the result is always j==0....)
		if ((abs(hittype[g_i].lastvx - g_sp->x) + abs(hittype[g_i].lastvy - g_sp->y)) <
			(abs(hittype[g_i].lastvx - s->x) + abs(hittype[g_i].lastvy - s->y)))
			j = 0;

		// um yeah, this if() will always fire....
		if (j == 0)
		{
			// search around for target player

			// also modifies 'target' x&y if found..

			j = furthestcanseepoint(g_i, s, &hittype[g_i].lastvx, &hittype[g_i].lastvy);

			if (j == -1) j = 0;
			else j = 1;
		}
	}
	else
	{
		// else, they did see it.
		// save where we were looking...
		hittype[g_i].lastvx = s->x;
		hittype[g_i].lastvy = s->y;
	}

	if (j == 1 && (g_sp->statnum == 1 || g_sp->statnum == 6))
		hittype[g_i].timetosleep = SLEEPTIME;

	return j == 1;
}


// int *it = 0x00589a04;

int ParseState::parse(void)
{
	int j, l, s;

	if(killit_flag) return 1;

	switch (*insptr)
	{
	case concmd_ifrnd:
	{
		insptr++;
		// HACK ALERT! The fire animation uses a broken ifrnd setup to delay its start because original CON has no variables.
		// But the chosen random value of 16/255 is too low and can cause delays of a second or more.
		int spnum = sprite[g_i].picnum;
		if (spnum == TILE_FIRE && g_t[4] == 0 && *insptr == 16)
		{
			parseifelse(rnd(64));
			break;
		}
		parseifelse(rnd(*insptr));
		break;
	}
	case concmd_ifcanshoottarget:
		parseifelse(ifcanshoottarget(g_i, g_p, g_x));
		break;
	case concmd_ifcanseetarget:
		j = cansee(g_sp->x, g_sp->y, g_sp->z - ((krand() & 41) << 8), g_sp->sectnum, ps[g_p].posx, ps[g_p].posy, ps[g_p].posz/*-((krand()&41)<<8)*/, sprite[ps[g_p].i].sectnum);
		parseifelse(j);
		if (j) hittype[g_i].timetosleep = SLEEPTIME;
		break;
	case concmd_ifnocover:
		j = cansee(g_sp->x, g_sp->y, g_sp->z, g_sp->sectnum, ps[g_p].posx, ps[g_p].posy, ps[g_p].posz, sprite[ps[g_p].i].sectnum);
		parseifelse(j);
		if (j) hittype[g_i].timetosleep = SLEEPTIME;
		break;

	case concmd_ifactornotstayput:
		parseifelse(hittype[g_i].actorstayput == -1);
		break;
	case concmd_ifcansee:
		parseifelse(ifcansee(g_i, g_p));
		break;

	case concmd_ifhitweapon:
		parseifelse(fi.ifhitbyweapon(g_i) >= 0);
		break;
	case concmd_ifsquished:
		parseifelse(ifsquished(g_i, g_p) == 1);
		break;
	case concmd_ifdead:
	{
		j = g_sp->extra;
		if (g_sp->picnum == TILE_APLAYER)
			j--;
		parseifelse(j < 0);
	}
	break;
	case concmd_ai:
		insptr++;
		g_t[5] = *insptr;
		g_t[4] = ScriptCode[g_t[5]];		  // Action
		g_t[1] = ScriptCode[g_t[5] + 1];		// move
		g_sp->hitag = ScriptCode[g_t[5] + 2];	  // Ai
		g_t[0] = g_t[2] = g_t[3] = 0;
		if (g_sp->hitag & random_angle)
			g_sp->ang = krand() & 2047;
		insptr++;
		break;
	case concmd_action:
		insptr++;
		g_t[2] = 0;
		g_t[3] = 0;
		g_t[4] = *insptr;
		insptr++;
		break;

	case concmd_ifpdistl:
		insptr++;
		parseifelse(g_x < *insptr);
		if (g_x > MAXSLEEPDIST && hittype[g_i].timetosleep == 0)
			hittype[g_i].timetosleep = SLEEPTIME;
		break;
	case concmd_ifpdistg:
		insptr++;
		parseifelse(g_x > * insptr);
		if (g_x > MAXSLEEPDIST && hittype[g_i].timetosleep == 0)
			hittype[g_i].timetosleep = SLEEPTIME;
		break;
	case concmd_else:
		insptr = &ScriptCode[*(insptr + 1)];
		break;
	case concmd_addstrength:
		insptr++;
		g_sp->extra += *insptr;
		insptr++;
		break;
	case concmd_strength:
		insptr++;
		g_sp->extra = *insptr;
		insptr++;
		break;
	case concmd_smacksprite:
		switch (krand() & 1)
		{
		case 0:
			g_sp->ang = (+512 + g_sp->ang + (krand() & 511)) & 2047;
			break;
		case 1:
			g_sp->ang = (-512 + g_sp->ang - (krand() & 511)) & 2047;
			break;
		}
		insptr++;
		break;
	case concmd_fakebubba:
		insptr++;
		fakebubbaspawn(g_i, g_p);
		break;

	case concmd_rndmove:
		g_sp->ang = krand() & 2047;
		g_sp->xvel = 25;
		insptr++;
		break;
	case concmd_mamatrigger:
		operateactivators(667, ps[g_p].i);
		insptr++;
		break;
	case concmd_mamaspawn:
		mamaspawn(g_i);
		insptr++;
		break;
	case concmd_mamaquake:
		if (g_sp->pal == 31)
			earthquaketime = 4;
		else if (g_sp->pal == 32)
			earthquaketime = 6;
		insptr++;
		break;
	case concmd_garybanjo:
		if (banjosound == 0)
		{
			short rnum = (krand() & 3) + 1;
			if (rnum == 4)
			{
				banjosound = 262;
			}
			else if (rnum == 1)
			{
				banjosound = 272;
			}
			else if (rnum == 2)
			{
				banjosound = 273;
			}
			else
			{
				banjosound = 273;
			}
			S_PlayActorSound(banjosound, g_i, CHAN_WEAPON);
		}
		else if (!S_CheckSoundPlaying(g_i, banjosound))
			S_PlayActorSound(banjosound, g_i, CHAN_WEAPON);
		insptr++;
		break;
	case concmd_motoloopsnd:
		if (!S_CheckSoundPlaying(g_i, 411))
			S_PlayActorSound(411, g_i, CHAN_VOICE);
		insptr++;
		break;
	case concmd_ifgotweaponce:
		insptr++;

		if (ud.coop >= 1 && ud.multimode > 1)
		{
			if (*insptr == 0)
			{
				for (j = 0; j < ps[g_p].weapreccnt; j++)
					if (ps[g_p].weaprecs[j] == g_sp->picnum)
						break;

				parseifelse(j < ps[g_p].weapreccnt&& g_sp->owner == g_i);
			}
			else if (ps[g_p].weapreccnt < 16)
			{
				ps[g_p].weaprecs[ps[g_p].weapreccnt++] = g_sp->picnum;
				parseifelse(g_sp->owner == g_i);
			}
		}
		else parseifelse(0);
		break;
	case concmd_getlastpal:
		insptr++;
		if (g_sp->picnum == TILE_APLAYER)
			g_sp->pal = ps[g_sp->yvel].palookup;
		else
		{
			// Copied from DukeGDX.
			if (g_sp->picnum == TILE_EGG && hittype[g_i].temp_data[5] == TILE_EGG + 2 && g_sp->pal == 1) 
			{
				ps[connecthead].max_actors_killed++; //revive the egg
				hittype[g_i].temp_data[5] = 0;
			}
			g_sp->pal = hittype[g_i].tempang;
		}
		hittype[g_i].tempang = 0;
		break;
	case concmd_tossweapon:
		insptr++;
		fi.checkweapons(&ps[g_sp->yvel]);
		break;
	case concmd_nullop:
		insptr++;
		break;
	case concmd_mikesnd:
		insptr++;
		if (!S_CheckSoundPlaying(g_i, g_sp->yvel))
			S_PlayActorSound(g_sp->yvel, g_i, CHAN_VOICE);
		break;
	case concmd_pkick:
		insptr++;

		if (ud.multimode > 1 && g_sp->picnum == TILE_APLAYER)
		{
			if (ps[otherp].quick_kick == 0)
				ps[otherp].quick_kick = 14;
		}
		else if (g_sp->picnum != TILE_APLAYER && ps[g_p].quick_kick == 0)
			ps[g_p].quick_kick = 14;
		break;
	case concmd_sizeto:
		insptr++;

		// JBF 20030805: As I understand it, if xrepeat becomes 0 it basically kills the
		// sprite, which is why the "sizeto 0 41" calls in 1.3d became "sizeto 4 41" in
		// 1.4, so instead of patching the CONs I'll surruptitiously patch the code here
		//if (!PLUTOPAK && *insptr == 0) *insptr = 4;

		j = ((*insptr) - g_sp->xrepeat) << 1;
		g_sp->xrepeat += ksgn(j);

		insptr++;

		if ((g_sp->picnum == TILE_APLAYER && g_sp->yrepeat < 36) || *insptr < g_sp->yrepeat || ((g_sp->yrepeat * (tilesiz[g_sp->picnum].y + 8)) << 2) < (hittype[g_i].floorz - hittype[g_i].ceilingz))
		{
			j = ((*insptr) - g_sp->yrepeat) << 1;
			if (abs(j)) g_sp->yrepeat += ksgn(j);
		}

		insptr++;

		break;
	case concmd_sizeat:
		insptr++;
		g_sp->xrepeat = (uint8_t)*insptr;
		insptr++;
		g_sp->yrepeat = (uint8_t)*insptr;
		insptr++;
		break;
	case concmd_shoot:
		insptr++;
		fi.shoot(g_i, (short)*insptr);
		insptr++;
		break;
	case concmd_ifsoundid:
		insptr++;
		parseifelse((short)*insptr == ambientlotag[g_sp->ang]);
		break;
	case concmd_ifsounddist:
		insptr++;
		if (*insptr == 0)
			parseifelse(ambienthitag[g_sp->ang] > g_x);
		else if (*insptr == 1)
			parseifelse(ambienthitag[g_sp->ang] < g_x);
		break;
	case concmd_soundtag:
		insptr++;
		S_PlayActorSound(ambientlotag[g_sp->ang], g_i);
		break;
	case concmd_soundtagonce:
		insptr++;
		if (!S_CheckSoundPlaying(g_i, ambientlotag[g_sp->ang]))
			S_PlayActorSound(ambientlotag[g_sp->ang], g_i);
		break;
	case concmd_soundonce:
		insptr++;
		if (!S_CheckSoundPlaying(g_i, *insptr++))
			S_PlayActorSound(*(insptr - 1), g_i);
		break;
	case concmd_stopsound:
		insptr++;
		if (S_CheckSoundPlaying(g_i, *insptr))
			S_StopSound((int)*insptr);
		insptr++;
		break;
	case concmd_globalsound:
		insptr++;
		if (g_p == screenpeek || ud.coop == 1)
			S_PlayActorSound((int)*insptr, ps[screenpeek].i);
		insptr++;
		break;
	case concmd_smackbubba:
		insptr++;
		if (!isRRRA() || g_sp->pal != 105)
		{
			setnextmap(false);
		}
		break;
	case concmd_mamaend:
		insptr++;
		ps[myconnectindex].MamaEnd = 150;
		break;

	case concmd_ifactorhealthg:
		insptr++;
		parseifelse(g_sp->extra > (short)*insptr);
		break;
	case concmd_ifactorhealthl:
		insptr++;
		parseifelse(g_sp->extra < (short)*insptr);
		break;
	case concmd_sound:
		insptr++;
		S_PlayActorSound((short) *insptr,g_i);
		insptr++;
		break;
	case concmd_tip:
		insptr++;
		ps[g_p].tipincs = 26;
		break;
	case concmd_iftipcow:
	case concmd_ifhittruck: // both have the same code.
		if (hittype[g_i].spriteextra == 1) // 
		{
			j = 1;
			hittype[g_i].spriteextra++;
		}
		else
			j = 0;
		parseifelse(j > 0);
		break;
	case concmd_tearitup:
		insptr++;
		tearitup(g_sp->sectnum);
		break;
	case concmd_fall:
		insptr++;
		g_sp->xoffset = 0;
		g_sp->yoffset = 0;
		fi.fall(g_i, g_p);
		break;
	case concmd_enda:
	case concmd_break:
	case concmd_ends:
	case concmd_endevent:
		return 1;
	case concmd_rightbrace:
		insptr++;
		return 1;
	case concmd_addammo:
		insptr++;
		if( ps[g_p].ammo_amount[*insptr] >= max_ammo_amount[*insptr] )
		{
			killit_flag = 2;
			break;
		}
		addammo( *insptr, &ps[g_p], *(insptr+1) );
		if(ps[g_p].curr_weapon == KNEE_WEAPON)
			if( ps[g_p].gotweapon[*insptr] )
				fi.addweapon( &ps[g_p], *insptr );
		insptr += 2;
		break;
	case concmd_money:
		insptr++;
		fi.lotsofmoney(g_sp,*insptr);
		insptr++;
		break;
	case concmd_mail:
		insptr++;
		fi.lotsofmail(g_sp,*insptr);
		insptr++;
		break;
	case concmd_sleeptime:
		insptr++;
		hittype[g_i].timetosleep = (short)*insptr;
		insptr++;
		break;
	case concmd_paper:
		insptr++;
		fi.lotsofpaper(g_sp,*insptr);
		insptr++;
		break;
	case concmd_addkills:
		insptr++;
		if (isRR())
		{
			if (hittype[g_i].spriteextra < 1 || hittype[g_i].spriteextra == 128)
			{
				if (actorfella(g_i))
					ps[g_p].actors_killed += *insptr;
			}
		}
		else ps[g_p].actors_killed += *insptr;
		hittype[g_i].actorstayput = -1;
		insptr++;
		break;
	case concmd_lotsofglass:
		insptr++;
		spriteglass(g_i,*insptr);
		insptr++;
		break;
	case concmd_killit:
		insptr++;
		killit_flag = 1;
		break;
	case concmd_addweapon:
		insptr++;
		if( ps[g_p].gotweapon[*insptr] == 0 ) fi.addweapon( &ps[g_p], *insptr );
		else if( ps[g_p].ammo_amount[*insptr] >= max_ammo_amount[*insptr] )
		{
				killit_flag = 2;
				break;
		}
		addammo( *insptr, &ps[g_p], *(insptr+1) );
		if(ps[g_p].curr_weapon == KNEE_WEAPON)
			if( ps[g_p].gotweapon[*insptr] )
				fi.addweapon( &ps[g_p], *insptr );
		insptr+=2;
		break;
	case concmd_debug:
		insptr++;
		Printf("%d\n",*insptr);
		insptr++;
		break;
	case concmd_endofgame:
		insptr++;
		ps[g_p].timebeforeexit = *insptr;
		ps[g_p].customexitsound = -1;
		ud.eog = true;
		insptr++;
		break;

	case concmd_isdrunk: // todo: move out to player_r.
		insptr++;
		ps[g_p].drink_amt += *insptr;
		j = sprite[ps[g_p].i].extra;
		if (j > 0)
			j += *insptr;
		if (j > max_player_health * 2)
			j = max_player_health * 2;
		if (j < 0)
			j = 0;

		if (ud.god == 0)
		{
			if (*insptr > 0)
			{
				if ((j - *insptr) < (max_player_health >> 2) &&
					j >= (max_player_health >> 2))
					S_PlayActorSound(DUKE_GOTHEALTHATLOW, ps[g_p].i);

				ps[g_p].last_extra = j;
			}

			sprite[ps[g_p].i].extra = j;
		}
		if (ps[g_p].drink_amt > 100)
			ps[g_p].drink_amt = 100;

		if (sprite[ps[g_p].i].extra >= max_player_health)
		{
			sprite[ps[g_p].i].extra = max_player_health;
			ps[g_p].last_extra = max_player_health;
		}
		insptr++;
		break;
	case concmd_strafeleft:
		insptr++;
		fi.movesprite(g_i, sintable[(g_sp->ang + 1024) & 2047] >> 10, sintable[(g_sp->ang + 512) & 2047] >> 10, g_sp->zvel, CLIPMASK0);
		break;
	case concmd_straferight:
		insptr++;
		fi.movesprite(g_i, sintable[(g_sp->ang - 0) & 2047] >> 10, sintable[(g_sp->ang - 512) & 2047] >> 10, g_sp->zvel, CLIPMASK0);
		break;
	case concmd_larrybird:
		insptr++;
		ps[g_p].posz = sector[sprite[ps[g_p].i].sectnum].ceilingz;
		sprite[ps[g_p].i].z = ps[g_p].posz;
		break;
	case concmd_destroyit:
		insptr++;
		destroyit(g_i);
		break;
	case concmd_iseat: // move out to player_r.
		insptr++;
		ps[g_p].eat += *insptr;
		if (ps[g_p].eat > 100)
		{
			ps[g_p].eat = 100;
		}
		ps[g_p].drink_amt -= *insptr;
		if (ps[g_p].drink_amt < 0)
			ps[g_p].drink_amt = 0;
		j = sprite[ps[g_p].i].extra;
		if (g_sp->picnum != TILE_ATOMICHEALTH)
		{
			if (j > max_player_health && *insptr > 0)
			{
				insptr++;
				break;
			}
			else
			{
				if (j > 0)
					j += (*insptr) * 3;
				if (j > max_player_health && *insptr > 0)
					j = max_player_health;
			}
		}
		else
		{
			if (j > 0)
				j += *insptr;
			if (j > (max_player_health << 1))
				j = (max_player_health << 1);
		}

		if (j < 0) j = 0;

		if (ud.god == 0)
		{
			if (*insptr > 0)
			{
				if ((j - *insptr) < (max_player_health >> 2) &&
					j >= (max_player_health >> 2))
					S_PlayActorSound(229, ps[g_p].i);

				ps[g_p].last_extra = j;
			}

			sprite[ps[g_p].i].extra = j;
		}

		insptr++;
		break;

	case concmd_addphealth: // todo: move out to player.
		insptr++;

		if(!isRR() && ps[g_p].newowner >= 0)
		{
			ps[g_p].newowner = -1;
			ps[g_p].posx = ps[g_p].oposx;
			ps[g_p].posy = ps[g_p].oposy;
			ps[g_p].posz = ps[g_p].oposz;
			ps[g_p].q16ang = ps[g_p].oq16ang;
			updatesector(ps[g_p].posx,ps[g_p].posy,&ps[g_p].cursectnum);
			setpal(&ps[g_p]);

			j = headspritestat[1];
			while (j >= 0)
			{
				if (sprite[j].picnum == TILE_CAMERA1)
					sprite[j].yvel = 0;
				j = nextspritestat[j];
			}
		}

		j = sprite[ps[g_p].i].extra;

		if(g_sp->picnum != TILE_ATOMICHEALTH)
		{
			if( j > max_player_health && *insptr > 0 )
			{
				insptr++;
				break;
			}
			else
			{
				if(j > 0)
					j += *insptr;
				if ( j > max_player_health && *insptr > 0 )
					j = max_player_health;
			}
		}
		else
		{
			if( j > 0 )
				j += *insptr;
			if ( j > (max_player_health<<1) )
				j = (max_player_health<<1);
		}

		if(j < 0) j = 0;

		if(ud.god == 0)
		{
			if(*insptr > 0)
			{
				if( ( j - *insptr ) < (max_player_health>>2) &&
					j >= (max_player_health>>2) )
						S_PlayActorSound(isRR()? 229 : DUKE_GOTHEALTHATLOW,ps[g_p].i);

				ps[g_p].last_extra = j;
			}

			sprite[ps[g_p].i].extra = j;
		}

		insptr++;
		break;

	case concmd_state:
		{
			auto tempscrptr = insptr + 2;
			insptr = &ScriptCode[*(insptr + 1)];
			while (1) if (parse()) break;
			insptr = tempscrptr;
		}
		break;
	case concmd_leftbrace:
		insptr++;
		while (1) if (parse()) break;
		break;
	case concmd_move:
		g_t[0]=0;
		insptr++;
		g_t[1] = *insptr;
		insptr++;
		g_sp->hitag = *insptr;
		insptr++;
		if(g_sp->hitag&random_angle)
			g_sp->ang = krand()&2047;
		break;
	case concmd_spawn:
		insptr++;
		if(g_sp->sectnum >= 0 && g_sp->sectnum < MAXSECTORS)
			fi.spawn(g_i,*insptr);
		insptr++;
		break;
	case concmd_ifwasweapon:
	case concmd_ifspawnedby:	// these two are the same
		insptr++;
		parseifelse( hittype[g_i].picnum == *insptr);
		break;
	case concmd_ifai:
		insptr++;
		parseifelse(g_t[5] == *insptr);
		break;
	case concmd_ifaction:
		insptr++;
		parseifelse(g_t[4] == *insptr);
		break;
	case concmd_ifactioncount:
		insptr++;
		parseifelse(g_t[2] >= *insptr);
		break;
	case concmd_resetactioncount:
		insptr++;
		g_t[2] = 0;
		break;
	case concmd_debris:
	{
			short dnum;

			insptr++;
			dnum = *insptr;
			insptr++;
			bool weap = fi.spawnweapondebris(g_sp->picnum, dnum);

			if(g_sp->sectnum >= 0 && g_sp->sectnum < MAXSECTORS)
				for(j=(*insptr)-1;j>=0;j--)
			{
				if(weap)
					s = 0;
				else s = (krand()%3);

				l = EGS(g_sp->sectnum,
					g_sp->x + (krand() & 255) - 128, g_sp->y + (krand() & 255) - 128, g_sp->z - (8 << 8) - (krand() & 8191),
					dnum + s, g_sp->shade, 32 + (krand() & 15), 32 + (krand() & 15),
					krand() & 2047, (krand() & 127) + 32, -(krand() & 2047), g_i, 5);
				if(weap)
					sprite[l].yvel = weaponsandammosprites[j%14];
				else sprite[l].yvel = -1;
				sprite[l].pal = g_sp->pal;
			}
			insptr++;
		}
		break;
	case concmd_count:
		insptr++;
		g_t[0] = (short) *insptr;
		insptr++;
		break;
	case concmd_cstator:
		insptr++;
		g_sp->cstat |= (short)*insptr;
		insptr++;
		break;
	case concmd_clipdist:
		insptr++;
		g_sp->clipdist = (short) *insptr;
		insptr++;
		break;
	case concmd_cstat:
		insptr++;
		g_sp->cstat = (short) *insptr;
		insptr++;
		break;
	case concmd_newpic:
		insptr++;
		g_sp->picnum = (short)*insptr;
		insptr++;
		break;
	case concmd_ifmove:
		insptr++;
		parseifelse(g_t[1] == *insptr);
		break;
	case concmd_resetplayer:
		insptr++;

		if(ud.multimode < 2)
		{
			gameaction = ga_autoloadgame;
			killit_flag = 2;
		}
		else
		{
			// I am not convinced this is even remotely smart to be executed from here...
			pickrandomspot(g_p);
			g_sp->x = hittype[g_i].bposx = ps[g_p].bobposx = ps[g_p].oposx = ps[g_p].posx;
			g_sp->y = hittype[g_i].bposy = ps[g_p].bobposy = ps[g_p].oposy = ps[g_p].posy;
			g_sp->z = hittype[g_i].bposy = ps[g_p].oposz = ps[g_p].posz;
			updatesector(ps[g_p].posx, ps[g_p].posy, &ps[g_p].cursectnum);
			setsprite(ps[g_p].i, ps[g_p].posx, ps[g_p].posy, ps[g_p].posz + PHEIGHT);
			g_sp->cstat = 257;

			g_sp->shade = -12;
			g_sp->clipdist = 64;
			g_sp->xrepeat = 42;
			g_sp->yrepeat = 36;
			g_sp->owner = g_i;
			g_sp->xoffset = 0;
			g_sp->pal = ps[g_p].palookup;

			ps[g_p].last_extra = g_sp->extra = max_player_health;
			ps[g_p].wantweaponfire = -1;
			ps[g_p].sethoriz(100);
			ps[g_p].on_crane = -1;
			ps[g_p].frag_ps = g_p;
			ps[g_p].sethorizoff(0);
			ps[g_p].opyoff = 0;
			ps[g_p].wackedbyactor = -1;
			ps[g_p].shield_amount = max_armour_amount;
			ps[g_p].dead_flag = 0;
			ps[g_p].pals.a = 0;
			ps[g_p].footprintcount = 0;
			ps[g_p].weapreccnt = 0;
			ps[g_p].ftq = 0;
			ps[g_p].posxv = ps[g_p].posyv = 0;
			if (!isRR()) ps[g_p].setrotscrnang(0);

			ps[g_p].falling_counter = 0;

			hittype[g_i].extra = -1;
			hittype[g_i].owner = g_i;

			hittype[g_i].cgg = 0;
			hittype[g_i].movflag = 0;
			hittype[g_i].tempang = 0;
			hittype[g_i].actorstayput = -1;
			hittype[g_i].dispicnum = 0;
			hittype[g_i].owner = ps[g_p].i;
			hittype[g_i].temp_data[4] = 0;

			resetinventory(g_p);
			resetweapons(g_p);
		}
		setpal(&ps[g_p]);
		break;
	case concmd_ifcoop:
		parseifelse(ud.coop || numplayers > 2);
		break;
	case concmd_ifonmud:
		parseifelse(abs(g_sp->z - sector[g_sp->sectnum].floorz) < (32 << 8) && sector[g_sp->sectnum].floorpicnum == 3073); // eew, hard coded tile numbers... :?
		break;
	case concmd_ifonwater:
		parseifelse( abs(g_sp->z-sector[g_sp->sectnum].floorz) < (32<<8) && sector[g_sp->sectnum].lotag == ST_1_ABOVE_WATER);
		break;
	case concmd_ifmotofast:
		parseifelse(ps[g_p].MotoSpeed > 60);
		break;
	case concmd_ifonmoto:
		parseifelse(ps[g_p].OnMotorcycle == 1);
		break;
	case concmd_ifonboat:
		parseifelse(ps[g_p].OnBoat == 1);
		break;
	case concmd_ifsizedown:
		g_sp->xrepeat--;
		g_sp->yrepeat--;
		parseifelse(g_sp->xrepeat <= 5);
		break;
	case concmd_ifwind:
		parseifelse(WindTime > 0);
		break;

	case concmd_ifinwater:
		parseifelse( sector[g_sp->sectnum].lotag == 2);
		break;
	case concmd_ifcount:
		insptr++;
		parseifelse(g_t[0] >= *insptr);
		break;
	case concmd_ifactor:
		insptr++;
		parseifelse(g_sp->picnum == *insptr);
		break;
	case concmd_resetcount:
		insptr++;
		g_t[0] = 0;
		break;
	case concmd_addinventory:
		insptr+=2;
		switch(*(insptr-1))
		{
			case 0:
				ps[g_p].steroids_amount = *insptr;
				ps[g_p].inven_icon = 2;
				break;
			case 1:
				ps[g_p].shield_amount +=		  *insptr;// 100;
				if(ps[g_p].shield_amount > max_player_health)
					ps[g_p].shield_amount = max_player_health;
				break;
			case 2:
				ps[g_p].scuba_amount =			   *insptr;// 1600;
				ps[g_p].inven_icon = 6;
				break;
			case 3:
				ps[g_p].holoduke_amount =		   *insptr;// 1600;
				ps[g_p].inven_icon = 3;
				break;
			case 4:
				ps[g_p].jetpack_amount =		   *insptr;// 1600;
				ps[g_p].inven_icon = 4;
				break;
			case 6:
				if (isRR())
				{
					switch (g_sp->lotag)
					{
					case 100: ps[g_p].keys[1] = 1; break;
					case 101: ps[g_p].keys[2] = 1; break;
					case 102: ps[g_p].keys[3] = 1; break;
					case 103: ps[g_p].keys[4] = 1; break;
					}
				}
				else
				{
					switch (g_sp->pal)
					{
					case  0: ps[g_p].got_access |= 1; break;
					case 21: ps[g_p].got_access |= 2; break;
					case 23: ps[g_p].got_access |= 4; break;
					}
				}
				break;
			case 7:
				ps[g_p].heat_amount = *insptr;
				ps[g_p].inven_icon = 5;
				break;
			case 9:
				ps[g_p].inven_icon = 1;
				ps[g_p].firstaid_amount = *insptr;
				break;
			case 10:
				ps[g_p].inven_icon = 7;
				ps[g_p].boot_amount = *insptr;
				break;
		}
		insptr++;
		break;
	case concmd_hitradius:
		fi.hitradius(g_i,*(insptr+1),*(insptr+2),*(insptr+3),*(insptr+4),*(insptr+5));
		insptr+=6;
		break;
	case concmd_ifp:
	{
			insptr++;

			l = *insptr;
			j = 0;

			s = g_sp->xvel;

			// sigh... this was yet another place where number literals were used as bit masks for every single value, making the code totally unreadable.
			if( (l& pducking) && ps[g_p].on_ground && (PlayerInput(g_p, SB_CROUCH) ^ !!(ps[g_p].crouch_toggle) ))
					j = 1;
			else if( (l& pfalling) && ps[g_p].jumping_counter == 0 && !ps[g_p].on_ground &&	ps[g_p].poszv > 2048 )
					j = 1;
			else if( (l& pjumping) && ps[g_p].jumping_counter > 348 )
					j = 1;
			else if( (l& pstanding) && s >= 0 && s < 8)
					j = 1;
			else if( (l& pwalking) && s >= 8 && !(PlayerInput(g_p, SB_RUN)) )
					j = 1;
			else if( (l& prunning) && s >= 8 && PlayerInput(g_p, SB_RUN) )
					j = 1;
			else if( (l& phigher) && ps[g_p].posz < (g_sp->z-(48<<8)) )
					j = 1;
			else if( (l& pwalkingback) && s <= -8 && !(PlayerInput(g_p, SB_RUN)) )
					j = 1;
			else if( (l& prunningback) && s <= -8 && (PlayerInput(g_p, SB_RUN)) )
					j = 1;
			else if( (l& pkicking) && ( ps[g_p].quick_kick > 0 || ( ps[g_p].curr_weapon == KNEE_WEAPON && ps[g_p].kickback_pic > 0 ) ) )
					j = 1;
			else if( (l& pshrunk) && sprite[ps[g_p].i].xrepeat < (isRR() ? 8 : 32))
					j = 1;
			else if( (l& pjetpack) && ps[g_p].jetpack_on )
					j = 1;
			else if( (l& ponsteroids) && ps[g_p].steroids_amount > 0 && ps[g_p].steroids_amount < 400 )
					j = 1;
			else if( (l& ponground) && ps[g_p].on_ground)
					j = 1;
			else if( (l& palive) && sprite[ps[g_p].i].xrepeat > (isRR() ? 8 : 32) && sprite[ps[g_p].i].extra > 0 && ps[g_p].timebeforeexit == 0 )
					j = 1;
			else if( (l& pdead) && sprite[ps[g_p].i].extra <= 0)
					j = 1;
			else if( (l& pfacing) )
			{
				if (g_sp->picnum == TILE_APLAYER && ud.multimode > 1)
					j = getincangle(ps[otherp].getang(), getangle(ps[g_p].posx - ps[otherp].posx, ps[g_p].posy - ps[otherp].posy));
				else
					j = getincangle(ps[g_p].getang(), getangle(g_sp->x - ps[g_p].posx, g_sp->y - ps[g_p].posy));

				if( j > -128 && j < 128 )
					j = 1;
				else
					j = 0;
			}

			parseifelse((int) j);

		}
		break;
	case concmd_ifstrength:
		insptr++;
		parseifelse(g_sp->extra <= *insptr);
		break;
	case concmd_guts:
		insptr += 2;
		fi.guts(g_sp,*(insptr-1),*insptr,g_p);
		insptr++;
		break;
	case concmd_slapplayer:
		insptr++;
		forceplayerangle(g_p);
		ps[g_p].posxv -= sintable[(ps[g_p].getang() + 512) & 2047] << 7;
		ps[g_p].posyv -= sintable[ps[g_p].getang() & 2047] << 7;
		return 0;
	case concmd_wackplayer:
		insptr++;
		if (!isRR())
			forceplayerangle(g_p);
		else
		{
			ps[g_p].posxv -= sintable[(ps[g_p].getang() + 512) & 2047] << 10;
			ps[g_p].posyv -= sintable[ps[g_p].getang() & 2047] << 10;
			ps[g_p].jumping_counter = 767;
			ps[g_p].jumping_toggle = 1;
		}
		return 0;
	case concmd_ifgapzl:
		insptr++;
		parseifelse( (( hittype[g_i].floorz - hittype[g_i].ceilingz ) >> 8 ) < *insptr);
		break;
	case concmd_ifhitspace:
		parseifelse(PlayerInput(g_p, SB_OPEN));
		break;
	case concmd_ifoutside:
		parseifelse(sector[g_sp->sectnum].ceilingstat & 1);
		break;
	case concmd_ifmultiplayer:
		parseifelse(ud.multimode > 1);
		break;
	case concmd_operate:
		insptr++;
		if( sector[g_sp->sectnum].lotag == 0 )
		{
			int16_t neartagsector, neartagwall, neartagsprite;
			int32_t neartaghitdist;
			neartag(g_sp->x,g_sp->y,g_sp->z-(32<<8),g_sp->sectnum,g_sp->ang,&neartagsector,&neartagwall,&neartagsprite,&neartaghitdist,768L,1);
			if( neartagsector >= 0 && isanearoperator(sector[neartagsector].lotag) )
				if( (sector[neartagsector].lotag&0xff) == ST_23_SWINGING_DOOR || sector[neartagsector].floorz == sector[neartagsector].ceilingz )
					if( (sector[neartagsector].lotag&16384) == 0 )
						if( (sector[neartagsector].lotag&32768) == 0 )
					{
						j = headspritesect[neartagsector];
						while(j >= 0)
						{
							if(sprite[j].picnum == ACTIVATOR)
								break;
							j = nextspritesect[j];
						}
						if(j == -1)
							operatesectors(neartagsector,g_i);
					}
		}
		break;
	case concmd_ifinspace:
		parseifelse(fi.ceilingspace(g_sp->sectnum));
		break;

	case concmd_spritepal:
		insptr++;
		if(g_sp->picnum != TILE_APLAYER)
			hittype[g_i].tempang = g_sp->pal;
		g_sp->pal = *insptr;
		insptr++;
		break;

	case concmd_cactor:
		insptr++;
		g_sp->picnum = *insptr;
		insptr++;
		break;

	case concmd_ifbulletnear:
		parseifelse( dodge(g_sp) == 1);
		break;
	case concmd_ifrespawn:
		if( badguy(g_sp) )
			parseifelse( ud.respawn_monsters );
		else if( inventory(g_sp) )
			parseifelse( ud.respawn_inventory );
		else
			parseifelse( ud.respawn_items );
		break;
	case concmd_iffloordistl:
		insptr++;
		parseifelse( (hittype[g_i].floorz - g_sp->z) <= ((*insptr)<<8));
		break;
	case concmd_ifceilingdistl:
		insptr++;
		parseifelse( ( g_sp->z - hittype[g_i].ceilingz ) <= ((*insptr)<<8));
		break;
	case concmd_palfrom:
		insptr++;
		SetPlayerPal(&ps[g_p], PalEntry(insptr[0], insptr[1], insptr[2], insptr[3]));
		insptr += 4;
		break;

/*		  case 74:
		insptr++;
		getglobalz(g_i);
		parseifelse( (( hittype[g_i].floorz - hittype[g_i].ceilingz ) >> 8 ) >= *insptr);
		break;
*/
	case concmd_addlog:
	{	int l;
		int lFile;
		insptr++;
		lFile=*(insptr++);	// file
		l=*(insptr++);	// line
		// this was only printing file name and line number as debug output.
		break;
	}
	case concmd_addlogvar:
	{	int l;
		int lFile;
		insptr++;
		lFile=*(insptr++);	// file
		l=*(insptr++);	// l=Line number, *instpr=varID
		if( (*insptr >= iGameVarCount)
			|| *insptr < 0
			)
		{
			// invalid varID
			insptr++;
			break;	// out of switch
		}
		DPrintf(DMSG_NOTIFY, "ADDLOGVAR: ");
			
		if( aGameVars[*insptr].dwFlags & GAMEVAR_FLAG_READONLY)
		{
			DPrintf(DMSG_NOTIFY, " (read-only)");
		}
		if( aGameVars[*insptr].dwFlags & GAMEVAR_FLAG_PERPLAYER)
		{
			DPrintf(DMSG_NOTIFY, " (Per Player. Player=%d)",g_p);
		}
		else if( aGameVars[*insptr].dwFlags & GAMEVAR_FLAG_PERACTOR)
		{
			DPrintf(DMSG_NOTIFY, " (Per Actor. Actor=%d)",g_i);
		}
		else
		{
			DPrintf(DMSG_NOTIFY, " (Global)");
		}
		DPrintf(DMSG_NOTIFY, " =%d",	GetGameVarID(*insptr, g_i, g_p));
		insptr++;
		break;
	}
	case concmd_setvar:
	{	int i;
		insptr++;
		i=*(insptr++);	// ID of def
		SetGameVarID(i, *insptr, g_i, g_p );
		insptr++;
		break;
	}
	case concmd_setvarvar:
	{	int i;
		insptr++;
		i=*(insptr++);	// ID of def
		SetGameVarID(i, GetGameVarID(*insptr, g_i, g_p), g_i, g_p );
//			aGameVars[i].lValue = aGameVars[*insptr].lValue;
		insptr++;
		break;
	}
	case concmd_addvar:
	{	int i;		
		insptr++;
		i=*(insptr++);	// ID of def
//sprintf(g_szBuf,"AddVar %d to Var ID=%d, g_i=%d, g_p=%d\n",*insptr, i, g_i, g_p);
//AddLog(g_szBuf);
		SetGameVarID(i, GetGameVarID(i, g_i, g_p) + *insptr, g_i, g_p );
		insptr++;
		break;
	}
		
	case concmd_addvarvar:
	{	int i;
		insptr++;
		i=*(insptr++);	// ID of def
		SetGameVarID(i, GetGameVarID(i, g_i, g_p) + GetGameVarID(*insptr, g_i, g_p), g_i, g_p );
		insptr++;
		break;
	}
	case concmd_ifvarvare:
	{
		int i;
		insptr++;
		i=*(insptr++);	// ID of def
		j=0;
		if(GetGameVarID(i, g_i, g_p) == GetGameVarID(*(insptr), g_i, g_p) )
		{
			j=1;
		}
		parseifelse( j );
		break;
	}
	case concmd_ifvarvarg:
	{
		int i;
		insptr++;
		i=*(insptr++);	// ID of def
		j=0;
		if(GetGameVarID(i, g_i, g_p) > GetGameVarID(*(insptr), g_i, g_p) )
		{
			j=1;
		}
		parseifelse( j );
		break;
	}
	case concmd_ifvarvarl:
	{
		int i;
		insptr++;
		i=*(insptr++);	// ID of def
		j=0;
		if(GetGameVarID(i, g_i, g_p) < GetGameVarID(*(insptr), g_i, g_p) )
		{
			j=1;
		}
		parseifelse( j );
		break;
	}
	case concmd_ifvare:
	{
		int i;
		insptr++;
		i=*(insptr++);	// ID of def
		j=0;
		if(GetGameVarID(i, g_i, g_p) == *insptr)
		{
			j=1;
		}
		parseifelse( j );
		break;
	}
	case concmd_ifvarg:
	{
		int i;
		insptr++;
		i=*(insptr++);	// ID of def
		j=0;
		if(GetGameVarID(i, g_i, g_p) > *insptr)
		{
			j=1;
		}
		parseifelse( j );
		break;
	}
	case concmd_ifvarl:
	{
		int i;
		insptr++;
		i=*(insptr++);	// ID of def
		j=0;
		if(GetGameVarID(i, g_i, g_p) < *insptr)
		{
			j=1;
		}
		parseifelse( j );
		break;
	}
	case concmd_ifphealthl:
		insptr++;
		parseifelse( sprite[ps[g_p].i].extra < *insptr);
		break;

	case concmd_ifpinventory:
	{
			insptr++;
			j = 0;
			switch(*(insptr++))
			{
				case 0:if( ps[g_p].steroids_amount != *insptr)
						j = 1;
					break;
				case 1:if(ps[g_p].shield_amount != max_player_health )
						j = 1;
					break;
				case 2:if(ps[g_p].scuba_amount != *insptr) j = 1;break;
				case 3:if(ps[g_p].holoduke_amount != *insptr) j = 1;break;
				case 4:if(ps[g_p].jetpack_amount != *insptr) j = 1;break;
				case 6:
					if (isRR())
					{
						switch (g_sp->lotag)
						{
						case 100: if (ps[g_p].keys[1]) j = 1; break;
						case 101: if (ps[g_p].keys[2]) j = 1; break;
						case 102: if (ps[g_p].keys[3]) j = 1; break;
						case 103: if (ps[g_p].keys[4]) j = 1; break;
						}
					}
					else
					{
						switch (g_sp->pal)
						{
						case  0: if (ps[g_p].got_access & 1) j = 1; break;
						case 21: if (ps[g_p].got_access & 2) j = 1; break;
						case 23: if (ps[g_p].got_access & 4) j = 1; break;
						}
					}
					break;
				case 7:if(ps[g_p].heat_amount != *insptr) j = 1;break;
				case 9:
					if(ps[g_p].firstaid_amount != *insptr) j = 1;break;
				case 10:
					if(ps[g_p].boot_amount != *insptr) j = 1;break;
			}

			parseifelse(j);
			break;
		}
	case concmd_pstomp:
		insptr++;
		if( ps[g_p].knee_incs == 0 && sprite[ps[g_p].i].xrepeat >= (isRR()? 9: 40) )
			if( cansee(g_sp->x,g_sp->y,g_sp->z-(4<<8),g_sp->sectnum,ps[g_p].posx,ps[g_p].posy,ps[g_p].posz+(16<<8),sprite[ps[g_p].i].sectnum) )
		{
			ps[g_p].knee_incs = 1;
			if(ps[g_p].weapon_pos == 0)
				ps[g_p].weapon_pos = -1;
			ps[g_p].actorsqu = g_i;
		}
		break;
	case concmd_ifawayfromwall:
	{
		short s1;

		s1 = g_sp->sectnum;

		j = 0;

		updatesector(g_sp->x + 108, g_sp->y + 108, &s1);
		if (s1 == g_sp->sectnum)
		{
			updatesector(g_sp->x - 108, g_sp->y - 108, &s1);
			if (s1 == g_sp->sectnum)
			{
				updatesector(g_sp->x + 108, g_sp->y - 108, &s1);
				if (s1 == g_sp->sectnum)
				{
					updatesector(g_sp->x - 108, g_sp->y + 108, &s1);
					if (s1 == g_sp->sectnum)
						j = 1;
				}
			}
		}
		parseifelse(j);
		break;
	}

	case concmd_quote:
		insptr++;
		FTA(*insptr,&ps[g_p]);
		insptr++;
		break;
	case concmd_ifinouterspace:
		parseifelse( fi.floorspace(g_sp->sectnum));
		break;
	case concmd_ifnotmoving:
		parseifelse( (hittype[g_i].movflag&49152) > 16384 );
		break;
	case concmd_respawnhitag:
		insptr++;
		fi.respawnhitag(g_sp);
		break;
	case concmd_ifspritepal:
		insptr++;
		parseifelse( g_sp->pal == *insptr);
		break;

	case concmd_ifangdiffl:
		insptr++;
		j = abs(getincangle(ps[g_p].getang(),g_sp->ang));
		parseifelse( j <= *insptr);
		break;

	case concmd_ifnosounds:
		parseifelse(!S_CheckAnyActorSoundPlaying(g_i) );
		break;

	case concmd_ifplaybackon: //Twentieth Anniversary World Tour
		parseifelse(false);
		break;

	case concmd_espawnvar:
	{
		int lReturn;
		int lIn;
		lReturn = -1;
		insptr++;

		lIn = *insptr++;
		lIn = GetGameVarID(lIn, g_i, g_p);
		if (g_sp->sectnum >= 0 && g_sp->sectnum < MAXSECTORS)
			lReturn = fi.spawn(g_i, lIn);

		SetGameVarID(g_iReturnVarID, lReturn, g_i, g_p);
		break;
	}
	case concmd_espawn:
	{
		int lReturn;
		lReturn = -1;
		insptr++;
		if (g_sp->sectnum >= 0 && g_sp->sectnum < MAXSECTORS)
			lReturn = fi.spawn(g_i, *insptr);
		insptr++;
		SetGameVarID(g_iReturnVarID, lReturn, g_i, g_p);
		break;
	}
	case concmd_setsector:
	case concmd_getsector:
	{
		// syntax [gs]etsector[<var>].x <VAR>
		// <varid> <xxxid> <varid>
		int lLabelID;
		int lVar1, lVar2;
		int lWhat;
		int lParm2;

		lWhat = *(insptr++);
		lVar1 = *(insptr++);
		lLabelID = *(insptr++);
		lParm2 = *(insptr++);
		lVar2 = *(insptr++);
		DoSector(lWhat == concmd_setsector, lVar1, lLabelID, lVar2, g_i, g_p, lParm2);
		break;
	}
	case concmd_sqrt:
	{
		// syntax sqrt <invar> <outvar>

		int lInVarID;
		int lOutVarID;
		int lIn;

		insptr++;
		lInVarID = *(insptr++);
		lOutVarID = *(insptr++);
		lIn = GetGameVarID(lInVarID, g_i, g_p);
		SetGameVarID(lOutVarID, ksqrt(lIn), g_i, g_p);
		break;
	}
	case concmd_findnearactor:
	{
		// syntax findnearactorvar <type> <maxdist> <getvar>
		// gets the sprite ID of the nearest actor within max dist
		// that is of <type> into <getvar>
		// -1 for none found
		// <type> <maxdist> <varid>
		int lType;
		int lMaxDist;
		int lVarID;
		int lTemp;
		int lFound;
		int lDist;
		short j;

		insptr++;

		lType = *(insptr++);
		lMaxDist = *(insptr++);
		lVarID = *(insptr++);

		lFound = -1;
		lDist = 32767;	// big number

		j = headspritestat[1];	// all sprites
		while (j >= 0)
		{
			if (sprite[j].picnum == lType)
			{
				lTemp = ldist(&sprite[g_i], &sprite[j]);
				if (lTemp < lMaxDist)
				{
					if (lTemp < lDist)
					{
						lFound = j;
					}
				}

			}
			j = nextspritestat[j];
		}
		SetGameVarID(lVarID, lFound, g_i, g_p);

		break;
	}
	case concmd_findnearactorvar:
	{
		// syntax findnearactorvar <type> <maxdistvar> <getvar>
		// gets the sprite ID of the nearest actor within max dist
		// that is of <type> into <getvar>
		// -1 for none found
		// <type> <maxdistvarid> <varid>
		int lType;
		int lMaxDistVar;
		int lMaxDist;
		int lVarID;
		int lTemp;
		int lFound;
		int lDist;
		short j;

		insptr++;

		lType = *(insptr++);
		lMaxDistVar = *(insptr++);
		lVarID = *(insptr++);
		lMaxDist = GetGameVarID(lMaxDistVar, g_i, g_p);
		lFound = -1;
		lDist = 32767;	// big number

		j = headspritestat[1];	// all sprites
		while (j >= 0)
		{
			if (sprite[j].picnum == lType)
			{
				lTemp = ldist(&sprite[g_i], &sprite[j]);
				if (lTemp < lMaxDist)
				{
					if (lTemp < lDist)
					{
						lFound = j;
					}
				}

			}
			j = nextspritestat[j];
		}
		SetGameVarID(lVarID, lFound, g_i, g_p);

		break;
	}
	case concmd_setplayer:
	case concmd_getplayer:
	{
		// syntax [gs]etplayer[<var>].x <VAR>
		// <varid> <xxxid> <varid>
		int lLabelID;
		int lVar1, lVar2;
		int lWhat;
		int lParm2;

		lWhat = *(insptr++);
		lVar1 = *(insptr++);
		lLabelID = *(insptr++);
		lParm2 = *(insptr++);
		lVar2 = *(insptr++);

		DoPlayer(lWhat == concmd_setplayer, lVar1, lLabelID, lVar2, g_i, g_p, lParm2);
		break;
	}
	case concmd_getuserdef:
	case concmd_setuserdef:
	{
		// syntax [gs]etuserdef.xxx <VAR>
		//  <xxxid> <varid>
		int lLabelID;
		int lVar1, lVar2;
		int lWhat;
		int lParm2;

		lWhat = *(insptr++);
		lVar1 = -1;
		lLabelID = *(insptr++);
		lParm2 = *(insptr++);
		lVar2 = *(insptr++);

		DoUserDef(lWhat == concmd_setuserdef, lVar1, lLabelID, lVar2, g_i, g_p, lParm2);
		break;
	}
	case concmd_setwall:
	case concmd_getwall:
	{
		// syntax [gs]etwall[<var>].x <VAR>
		// <varid> <xxxid> <varid>
		int lLabelID;
		int lVar1, lVar2;
		int lWhat;
		int lParm2;

		lWhat = *(insptr++);
		lVar1 = *(insptr++);
		lLabelID = *(insptr++);
		lParm2 = *(insptr++);
		lVar2 = *(insptr++);

		DoWall(lWhat == concmd_setwall, lVar1, lLabelID, lVar2, g_i, g_p, lParm2);
		break;
	}
	case concmd_setactorvar:
	{
		// syntax [gs]etactorvar[<var>].<varx> <VAR>
		// gets the value of the per-actor variable varx into VAR
		// <var> <varx> <VAR>
		int lVar1, lVar2, lVar3;
		int lTemp, lSprite;

		insptr++;

		lVar1 = *(insptr++);
		lVar2 = *(insptr++);
		lVar3 = *(insptr++);

		lSprite = GetGameVarID(lVar1, g_i, g_p);
		if (lSprite >= 0)
		{
			lTemp = GetGameVarID(lVar3, g_i, g_p);
			SetGameVarID(lVar2, lTemp, lSprite, g_p);
		}

		break;
	}
	case concmd_getactorvar:
	{
		// syntax [gs]etactorvar[<var>].<varx> <VAR>
		// gets the value of the per-actor variable varx into VAR
		// <var> <varx> <VAR>
		int lVar1, lVar2, lVar3;
		int lTemp, lSprite;

		insptr++;

		lVar1 = *(insptr++);
		lVar2 = *(insptr++);
		lVar3 = *(insptr++);

		lSprite = GetGameVarID(lVar1, g_i, g_p);
		if (lSprite >= 0)
		{
			lTemp = GetGameVarID(lVar2, lSprite, g_p);
			SetGameVarID(lVar3, lTemp, g_i, g_p);
		}

		break;
	}
	case concmd_setactor:
	case concmd_getactor:
	{
		// syntax [gs]etactor[<var>].x <VAR>
		// <varid> <xxxid> <varid>
		int lLabelID;
		int lVar1, lVar2;
		int lWhat;
		int lParm2;

		lWhat = *(insptr++);
		lVar1 = *(insptr++);
		lLabelID = *(insptr++);
		lParm2 = *(insptr++);
		lVar2 = *(insptr++);

		DoActor(lWhat == concmd_setactor, lVar1, lLabelID, lVar2, g_i, g_p, lParm2);
		break;
	}
	case concmd_getangletotarget:
	{
		int i;
		short ang;

		insptr++;
		i = *(insptr++);	// ID of def

		// hittype[g_i].lastvx and lastvy are last known location of target.
		ang = getangle(hittype[g_i].lastvx - g_sp->x, hittype[g_i].lastvy - g_sp->y);
		SetGameVarID(i, ang, g_i, g_p);
		break;
	}
	case concmd_lockplayer:
	{
		int i;
		insptr++;
		i = *(insptr++);	// ID of def
		ps[g_p].transporter_hold = GetGameVarID(i, g_i, g_p);
		break;
	}
	case concmd_getplayerangle:
	{
		int i;
		insptr++;
		i = *(insptr++);	// ID of def
		SetGameVarID(i, ps[g_p].getang(), g_i, g_p);
		break;
	}
	case concmd_setplayerangle:
	{
		int i;
		insptr++;
		i = *(insptr++);	// ID of def
		ps[g_p].setang(GetGameVarID(i, g_i, g_p) & 2047);
		break;
	}
	case concmd_getactorangle:
	{
		int i;
		insptr++;
		i = *(insptr++);	// ID of def
		SetGameVarID(i, g_sp->ang, g_i, g_p);
		break;
	}
	case concmd_setactorangle:
	{
		int i;
		insptr++;
		i = *(insptr++);	// ID of def
		g_sp->ang = GetGameVarID(i, g_i, g_p);
		g_sp->ang &= 2047;
		break;
	}
	case concmd_randvar:
	{
		int i;
		insptr++;
		i = *(insptr++);	// ID of def
		SetGameVarID(i, mulscale(rand(), *insptr, 15), g_i, g_p);
		insptr++;
		break;
	}
	case concmd_mulvar:
	{
		int i;
		insptr++;
		i = *(insptr++);	// ID of def
		SetGameVarID(i, GetGameVarID(i, g_i, g_p) * (*insptr), g_i, g_p);
		insptr++;
		break;
	}
	case concmd_divvar:
	{
		int i;
		insptr++;
		i = *(insptr++);	// ID of def
		if ((*insptr) == 0)
		{
			I_Error("Divide by Zero in CON.");
		}
		SetGameVarID(i, GetGameVarID(i, g_i, g_p) / (*insptr), g_i, g_p);
		insptr++;
		break;
	}
	case concmd_modvar:
	{
		int i;
		int l;
		int lResult;
		insptr++;
		i = *(insptr++);	// ID of def
		l = (*insptr);
		if (l == 0)
		{
			I_Error("Divide by Zero in CON");
		}
		lResult = GetGameVarID(i, g_i, g_p) % l;
		SetGameVarID(i, lResult, g_i, g_p);
		insptr++;
		break;
	}
	case concmd_andvar:
	{
		int i;
		int l;
		int lResult;
		insptr++;
		i = *(insptr++);	// ID of def
		l = (*insptr);
		lResult = GetGameVarID(i, g_i, g_p) & l;
		SetGameVarID(i, lResult, g_i, g_p);
		insptr++;
		break;
	}
	case concmd_xorvar:
	{
		int i;
		int l;
		int lResult;
		insptr++;
		i = *(insptr++);	// ID of def
		l = (*insptr);
		lResult = GetGameVarID(i, g_i, g_p) ^ l;
		SetGameVarID(i, lResult, g_i, g_p);
		insptr++;
		break;
	}
	case concmd_orvar:
	{
		int i;
		int l;
		int lResult;
		insptr++;
		i = *(insptr++);	// ID of def
		l = (*insptr);
		lResult = GetGameVarID(i, g_i, g_p) | l;
		SetGameVarID(i, lResult, g_i, g_p);
		insptr++;
		break;
	}
	case concmd_randvarvar:
	{
		int i;
		int l1, l2;
		int lResult;
		insptr++;
		i = *(insptr++);	// ID of def
		l1 = GetGameVarID(i, g_i, g_p); // not used for this command
		l2 = GetGameVarID(*insptr, g_i, g_p);
		lResult = mulscale(rand(), l2, 15);
		SetGameVarID(i, lResult, g_i, g_p);
		insptr++;
		break;
	}
	case concmd_gmaxammo:
	{
		int i;
		int l1, l2;
		int lResult;
		insptr++;
		i = *(insptr++);	// ID of def
		l1 = GetGameVarID(i, g_i, g_p);
		l2 = GetGameVarID(*insptr, g_i, g_p); // l2 not used in this one
		lResult = max_ammo_amount[l1];
		SetGameVarID(*insptr, lResult, g_i, g_p);
		insptr++;
		break;
	}
	case concmd_smaxammo:
	{
		int i;
		int l1, l2;
		insptr++;
		i = *(insptr++);	// ID of def
		l1 = GetGameVarID(i, g_i, g_p);
		l2 = GetGameVarID(*insptr, g_i, g_p);
		max_ammo_amount[l1] = l2;

		insptr++;
		break;
	}
	case concmd_mulvarvar:
	{
		int i;
		int l1, l2;
		int lResult;
		insptr++;
		i = *(insptr++);	// ID of def
		l1 = GetGameVarID(i, g_i, g_p);
		l2 = GetGameVarID(*insptr, g_i, g_p);
		lResult = l1 * l2;
		SetGameVarID(i, lResult, g_i, g_p);
		insptr++;
		break;
	}
	case concmd_divvarvar:
	{
		int i;
		int l1, l2;
		int lResult;
		insptr++;
		i = *(insptr++);	// ID of def
		l1 = GetGameVarID(i, g_i, g_p);
		l2 = GetGameVarID(*insptr, g_i, g_p);
		if (l2 == 0)
		{
			I_Error("Divide by Zero in CON");
		}
		lResult = l1 / l2;
		SetGameVarID(i, lResult, g_i, g_p);
		insptr++;
		break;
	}
	case concmd_modvarvar:
	{
		int i;
		int l1, l2;
		int lResult;
		insptr++;
		i = *(insptr++);	// ID of def
		l1 = GetGameVarID(i, g_i, g_p);
		l2 = GetGameVarID(*insptr, g_i, g_p);
		if (l2 == 0)
		{
			I_Error("Mod by Zero in CON");
		}
		lResult = l1 % l2;
		SetGameVarID(i, lResult, g_i, g_p);
		insptr++;
		break;
	}
	case concmd_andvarvar:
	{
		int i;
		int l1, l2;
		int lResult;
		insptr++;
		i = *(insptr++);	// ID of def
		l1 = GetGameVarID(i, g_i, g_p);
		l2 = GetGameVarID(*insptr, g_i, g_p);
		lResult = l1 & l2;
		SetGameVarID(i, lResult, g_i, g_p);
		insptr++;
		break;
	}
	case concmd_xorvarvar:
	{
		int i;
		int l1, l2;
		int lResult;
		insptr++;
		i = *(insptr++);	// ID of def
		l1 = GetGameVarID(i, g_i, g_p);
		l2 = GetGameVarID(*insptr, g_i, g_p);
		lResult = l1 ^ l2;
		SetGameVarID(i, lResult, g_i, g_p);
		insptr++;
		break;
	}
	case concmd_orvarvar:
	{
		int i;
		int l1, l2;
		int lResult;
		insptr++;
		i = *(insptr++);	// ID of def
		l1 = GetGameVarID(i, g_i, g_p);
		l2 = GetGameVarID(*insptr, g_i, g_p);
		lResult = l1 | l2;
		SetGameVarID(i, lResult, g_i, g_p);
		insptr++;
		break;
	}
	case concmd_subvar:
	{
		int i;
		insptr++;
		i = *(insptr++);	// ID of def
		SetGameVarID(i, GetGameVarID(i, g_i, g_p) - *insptr, g_i, g_p);
		insptr++;
		break;
	}
	case concmd_subvarvar:
	{
		int i;
		insptr++;
		i = *(insptr++);	// ID of def
		SetGameVarID(i, GetGameVarID(i, g_i, g_p) - GetGameVarID(*insptr, g_i, g_p), g_i, g_p);
		insptr++;
		break;
	}
	case concmd_sin:
	{
		int i;
		int lValue;
		insptr++;
		i = *(insptr++);	// ID of def
		lValue = GetGameVarID(*insptr, g_i, g_p);
		lValue = sintable[lValue & 2047];
		SetGameVarID(i, lValue, g_i, g_p);
		insptr++;
		break;
	}

	case concmd_spgetlotag:
	{
		insptr++;
		SetGameVarID(g_iLoTagID, g_sp->lotag, g_i, g_p);
		break;
	}
	case concmd_spgethitag:
	{
		insptr++;
		SetGameVarID(g_iHiTagID, g_sp->hitag, g_i, g_p);
		break;
	}
	case concmd_sectgetlotag:
	{
		insptr++;
		SetGameVarID(g_iLoTagID, sector[g_sp->sectnum].lotag, g_i, g_p);
		break;
	}
	case concmd_sectgethitag:
	{
		insptr++;
		SetGameVarID(g_iHiTagID, sector[g_sp->sectnum].hitag, g_i, g_p);
		break;
	}
	case concmd_gettexturefloor:
	{
		insptr++;
		SetGameVarID(g_iTextureID, sector[g_sp->sectnum].floorpicnum, g_i, g_p);
		break;
	}

	case concmd_startlevel:
	{
		// from 'level' cheat in game.c (about line 6250)
		int volnume;
		int levnume;

		insptr++; // skip command
		volnume = GetGameVarID(*(insptr++), g_i, g_p);
		levnume = GetGameVarID(*(insptr++), g_i, g_p);
		auto level = FindMapByLevelNum(levelnum(volnume - 1, levnume - 1));
		if (level != nullptr)
			ChangeLevel(level, -1);
		break;
	}
	case concmd_myosx:
	case concmd_myospalx:

	case concmd_myos:
	case concmd_myospal:
	{
		int x, y;
		int tilenum;
		int shade;
		int orientation;
		int pal;
		int tw = *insptr++;
		x = GetGameVarID(*insptr++, g_i, g_p);
		y = GetGameVarID(*insptr++, g_i, g_p);
		tilenum = GetGameVarID(*insptr++, g_i, g_p);
		shade = GetGameVarID(*insptr++, g_i, g_p);
		orientation = GetGameVarID(*insptr++, g_i, g_p);
		if (tw == concmd_myospal)
		{
			pal = GetGameVarID(*insptr++, g_i, g_p);
			//myospal(x, y, tilenum, shade, orientation, pal);
		}
		else if (tw == concmd_myos)
		{
			//myos(x, y, tilenum, shade, orientation);
		}
		else if (tw == concmd_myosx)
		{
			//myos640(x, y, tilenum, shade, orientation);
		}
		else if (tw == concmd_myospalx)
		{
			pal = GetGameVarID(*insptr++, g_i, g_p);
			//myospal640(x, y, tilenum, shade, orientation, pal);
		}
		break;
	}

	case concmd_displayrand:
	{
		int i;
		insptr++;

		i = *(insptr++);	// ID of def
		SetGameVarID(i, rand(), g_i, g_p);
		break;
	}
	case concmd_switch:
	{
		int lVarID;
		int lValue;
		int* lpDefault;
		int* lpCases;
		int lCases;
		int lEnd;
		int lCheckCase;
		char bMatched;
		int* lTempInsPtr;

		// command format:
		// variable ID to check
		// script offset to 'end'
		// count of case statements
		// script offset to default case (null if none)
		// For each case: value, ptr to code
		insptr++; // p-code
		lVarID = *insptr++;
		lValue = GetGameVarID(lVarID, g_i, g_p);
		lEnd = *insptr++;
		lCases = *insptr++;
		lpDefault = insptr++;
		lpCases = insptr;
		insptr += lCases * 2;
		bMatched = 0;
		lTempInsPtr = insptr;
		for (lCheckCase = 0; lCheckCase < lCases && !bMatched; lCheckCase++)
		{
			if (lpCases[lCheckCase * 2] == lValue)
			{
				insptr = &ScriptCode[lpCases[lCheckCase * 2 + 1]];
				while (1)
				{
					if (parse())
						break;
				}
				bMatched = 1;
			}
		}
		if (!bMatched)
		{
			if (*lpDefault)
			{
				insptr = &ScriptCode[*lpDefault];
				while (1) if (parse()) break;
			}
			else
			{
				//AddLog("No Matching Case: No Default to use");
			}
		}
		insptr = &ScriptCode[lEnd];
		break;
	}
	case concmd_endswitch:
		insptr++;
		return 1;
		break;

	case concmd_starttrack:
	{
		insptr++;
		int music_select = *insptr++;
		auto level = FindMapByLevelNum(levelnum(currentLevel->levelNumber, music_select));
		if (level) S_PlayLevelMusic(level);
		break;
	}
	case concmd_gettextureceiling:
	{
		insptr++;
		SetGameVarID(g_iTextureID, sector[g_sp->sectnum].ceilingpicnum, g_i, g_p);
		break;
	}
	case concmd_ifvarvarand:
	{
		int i;
		insptr++;
		i = *(insptr++);	// ID of def
		j = 0;
		if (GetGameVarID(i, g_i, g_p) & GetGameVarID(*(insptr), g_i, g_p))
		{
			j = 1;
		}
		parseifelse(j);
		break;
	}
	case concmd_ifvarvarn:
	{
		int i;
		insptr++;
		i = *(insptr++);	// ID of def
		j = 0;
		if (GetGameVarID(i, g_i, g_p) != GetGameVarID(*(insptr), g_i, g_p))
		{
			j = 1;
		}
		parseifelse(j);
		break;
	}
	case concmd_ifvarn:
	{
		int i;
		insptr++;
		i = *(insptr++);	// ID of def
		j = 0;
		if (GetGameVarID(i, g_i, g_p) != *insptr)
		{
			j = 1;
		}
		parseifelse(j);
		break;
	}
	case concmd_ifvarand:
	{
		int i;
		insptr++;
		i = *(insptr++);	// ID of def
		j = 0;
		if (GetGameVarID(i, g_i, g_p) & *insptr)
		{
			j = 1;
		}
		parseifelse(j);
		break;
	}

	default:
		Printf(TEXTCOLOR_RED "Unrecognized PCode of %d  in parse.  Killing current sprite.\n",*insptr);
		Printf(TEXTCOLOR_RED "Offset=%0X\n",int(insptr-ScriptCode.Data()));
		killit_flag = 1;
		break;
	}
	return 0;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void LoadActor(int i, int p, int x)
{
	char done;
	spritetype* g_sp;

	ParseState s;
	s.g_i = i;	// Sprite ID
	s.g_p = p;	// Player ID
	s.g_x = x;	// ??
	g_sp = s.g_sp = &sprite[i];	// Pointer to sprite structure
	s.g_t = &hittype[i].temp_data[0];	// Sprite's 'extra' data

	auto addr = tileinfo[s.g_sp->picnum].loadeventscriptptr;
	if (addr == 0) return;

	int *insptr = &ScriptCode[addr + 1];

	s.killit_flag = 0;

	if (g_sp->sectnum < 0 || g_sp->sectnum >= MAXSECTORS)
	{
		deletesprite(i);
		return;
	}
	do
		done = s.parse();
	while (done == 0);

	if (s.killit_flag == 1)
	{
		// if player was set to squish, first stop that...
		if (p >= 0)
		{
			if (ps[p].actorsqu == i)
				ps[p].actorsqu = -1;
		}
		deletesprite(i);
	}
	else
	{
		fi.move(i, p, x);

		if (g_sp->statnum == 1)
		{
			if (badguy(g_sp))
			{
				if (g_sp->xrepeat > 60) return;
				if (ud.respawn_monsters == 1 && g_sp->extra <= 0) return;
			}
			else if (ud.respawn_items == 1 && (g_sp->cstat & 32768)) return;

			if (hittype[i].timetosleep > 1)
				hittype[i].timetosleep--;
			else if (hittype[i].timetosleep == 1)
				changespritestat(i, 2);
		}

		else if (g_sp->statnum == 6)
		{
#if 0
			switch (g_sp->picnum)
			{
			case RUBBERCAN:
			case EXPLODINGBARREL:
			case WOODENHORSE:
			case HORSEONSIDE:
			case CANWITHSOMETHING:
			case FIREBARREL:
			case NUKEBARREL:
			case NUKEBARRELDENTED:
			case NUKEBARRELLEAKED:
			case TRIPBOMB:
			case EGG:
				if (hittype[i].timetosleep > 1)
					hittype[i].timetosleep--;
				else if (hittype[i].timetosleep == 1)
					changespritestat(i, 2);
				break;
			}
#endif
		}
	}

}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void execute(int i,int p,int x)
{
	if (actorinfo[sprite[i].picnum].scriptaddress == 0) return;

	int done;
	spritetype* g_sp;

	ParseState s;
	s.g_i = i;	// Sprite ID
	s.g_p = p;	// Player ID
	s.g_x = x;	// ??
	g_sp = s.g_sp = &sprite[i];	// Pointer to sprite structure
	s.g_t = &hittype[i].temp_data[0];	// Sprite's 'extra' data

	if (actorinfo[g_sp->picnum].scriptaddress == 0) return;
	s.insptr = &ScriptCode[4 + (actorinfo[g_sp->picnum].scriptaddress)];

	s.killit_flag = 0;

	if(g_sp->sectnum < 0 || g_sp->sectnum >= MAXSECTORS)
	{
		if(badguy(g_sp))
			ps[p].actors_killed++;
		deletesprite(i);
		return;
	}

	if (s.g_t[4])
	{
		// This code was utterly cryptic in the original source.
		auto ptr = &ScriptCode[s.g_t[4]];
		int numframes = ptr[1];
		int increment = ptr[3];
		int delay =  ptr[4];

		g_sp->lotag += TICSPERFRAME;
		if (g_sp->lotag > delay)
		{
			s.g_t[2]++;
			g_sp->lotag = 0;
			s.g_t[3] += increment;
		}
		if (abs(s.g_t[3]) >= abs(numframes * increment))
			s.g_t[3] = 0;
	}

	do
		done = s.parse();
	while( done == 0 );

	if(s.killit_flag == 1)
	{
		// if player was set to squish, first stop that...
		if(ps[p].actorsqu == i)
			ps[p].actorsqu = -1;
		killthesprite = true;
	}
	else
	{
		fi.move(i, p, x);

		if (g_sp->statnum == STAT_ACTOR)
		{
			if (badguy(g_sp))
			{
				if (g_sp->xrepeat > 60) goto quit;
				if (ud.respawn_monsters == 1 && g_sp->extra <= 0) goto quit;
			}
			else if (ud.respawn_items == 1 && (g_sp->cstat & 32768)) goto quit;

			if (hittype[i].timetosleep > 1)
				hittype[i].timetosleep--;
			else if (hittype[i].timetosleep == 1)
				changespritestat(i, STAT_ZOMBIEACTOR);
		}

		else if (g_sp->statnum == STAT_STANDABLE)
			fi.checktimetosleep(i);
	}
quit:
	if (killthesprite) deletesprite(i);
	killthesprite = false;
}


//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void OnEvent(int iEventID, int p, int i, int x)
{
	char done;

	if (iEventID >= MAXGAMEEVENTS)
	{
		Printf("Invalid Event ID\n");
		return;
	}
	if (apScriptGameEvent[iEventID] == 0)
	{
		return;
	}

	ParseState s;
	s.g_i = i;	// current sprite ID
	s.g_p = p;	/// current player ID
	s.g_x = x;	// ?
	s.g_sp = &sprite[i];
	s.g_t = &hittype[i].temp_data[0];

	s.insptr = &ScriptCode[apScriptGameEvent[iEventID]];

	s.killit_flag = 0;
	do
		done = s.parse();
	while (done == 0);
}

END_DUKE_NS