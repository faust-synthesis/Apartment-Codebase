/***
*
*	Copyright (c) 1996-2002, Valve LLC. All rights reserved.
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/
#include "hud.h"
#include "cl_util.h"

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "weapons.h"

#include "com_weapons.h"
#include "const.h"
#include "entity_state.h"
#include "cl_entity.h"
#include "entity_types.h"
#include "usercmd.h"
#include "pm_defs.h"
#include "pm_materials.h"

#include "eventscripts.h"
#include "ev_hldm.h"

#include "r_efx.h"
#include "event_api.h"
#include "event_args.h"
#include "in_defs.h"

#include <string.h>

#include "r_studioint.h"
#include "com_model.h"

extern engine_studio_api_t IEngineStudio;

static int tracerCount[MAX_PLAYERS];

#include "pm_shared.h"

void V_PunchAxis(int axis, float punch);
void VectorAngles(const float* forward, float* angles);

extern cvar_t* cl_lw;
extern cvar_t* r_decals;

static inline bool EV_HLDM_IsBSPModel(physent_t* pe)
{
	return pe != nullptr && (pe->solid == SOLID_BSP || pe->movetype == MOVETYPE_PUSHSTEP);
}

// play a strike sound based on the texture that was hit by the attack traceline.  VecSrc/VecEnd are the
// original traceline endpoints used by the attacker, iBulletType is the type of bullet that hit the texture.
// returns volume of strike instrument (crowbar) to play
float EV_HLDM_PlayTextureSound(int idx, pmtrace_t* ptr, float* vecSrc, float* vecEnd, int iBulletType)
{
	// hit the world, try to play sound based on texture material type
	char chTextureType = CHAR_TEX_CONCRETE;
	cl_entity_t* cl_entity = NULL;
	float fvol;
	float fvolbar;
	const char* rgsz[4];
	int cnt;
	float fattn = ATTN_NORM;
	int entity;
	char* pTextureName;
	char texname[64];
	char szbuffer[64];

	entity = gEngfuncs.pEventAPI->EV_IndexFromTrace(ptr);

	// FIXME check if playtexture sounds movevar is set
	//

	chTextureType = 0;

	// Player
	if (entity == 0)
	{
		// get texture from entity or world (world is ent(0))
		pTextureName = (char*)gEngfuncs.pEventAPI->EV_TraceTexture(ptr->ent, vecSrc, vecEnd);

		if (pTextureName)
		{
			strcpy(texname, pTextureName);
			pTextureName = texname;

			// strip leading '-0' or '+0~' or '{' or '!'
			if (*pTextureName == '-' || *pTextureName == '+')
			{
				pTextureName += 2;
			}

			if (*pTextureName == '{' || *pTextureName == '!' || *pTextureName == '~' || *pTextureName == ' ')
			{
				pTextureName++;
			}

			// '}}'
			strcpy(szbuffer, pTextureName);
			szbuffer[CBTEXTURENAMEMAX - 1] = 0;

			// get texture type
			chTextureType = PM_FindTextureType(szbuffer);
		}
	}
	else
	{
		// JoshA: Look up the entity and find the EFLAG_FLESH_SOUND flag.
		// This broke at some point then TF:C added prediction.
		//
		// It used to use Classify of pEntity->Classify() != CLASS_NONE && pEntity->Classify() != CLASS_MACHINE
		// to determine what sound to play, but that's server side and isn't available on the client
		// and got lost in the translation to that.
		// Now the server will replicate that state via an eflag.
		cl_entity = gEngfuncs.GetEntityByIndex(entity);

		if (cl_entity && !!(cl_entity->curstate.eflags & EFLAG_FLESH_SOUND))
			chTextureType = CHAR_TEX_FLESH;
	}

	switch (chTextureType)
	{
	default:
	case CHAR_TEX_CONCRETE:
		fvol = 0.9;
		fvolbar = 0.6;
		rgsz[0] = "player/pl_step1.wav";
		rgsz[1] = "player/pl_step2.wav";
		cnt = 2;
		break;
	case CHAR_TEX_METAL:
		fvol = 0.9;
		fvolbar = 0.3;
		rgsz[0] = "player/pl_metal1.wav";
		rgsz[1] = "player/pl_metal2.wav";
		cnt = 2;
		break;
	case CHAR_TEX_DIRT:
		fvol = 0.9;
		fvolbar = 0.1;
		rgsz[0] = "player/pl_dirt1.wav";
		rgsz[1] = "player/pl_dirt2.wav";
		rgsz[2] = "player/pl_dirt3.wav";
		cnt = 3;
		break;
	case CHAR_TEX_VENT:
		fvol = 0.5;
		fvolbar = 0.3;
		rgsz[0] = "player/pl_duct1.wav";
		rgsz[1] = "player/pl_duct1.wav";
		cnt = 2;
		break;
	case CHAR_TEX_GRATE:
		fvol = 0.9;
		fvolbar = 0.5;
		rgsz[0] = "player/pl_grate1.wav";
		rgsz[1] = "player/pl_grate4.wav";
		cnt = 2;
		break;
	case CHAR_TEX_TILE:
		fvol = 0.8;
		fvolbar = 0.2;
		rgsz[0] = "player/pl_tile1.wav";
		rgsz[1] = "player/pl_tile3.wav";
		rgsz[2] = "player/pl_tile2.wav";
		rgsz[3] = "player/pl_tile4.wav";
		cnt = 4;
		break;
	case CHAR_TEX_SLOSH:
		fvol = 0.9;
		fvolbar = 0.0;
		rgsz[0] = "player/pl_slosh1.wav";
		rgsz[1] = "player/pl_slosh3.wav";
		rgsz[2] = "player/pl_slosh2.wav";
		rgsz[3] = "player/pl_slosh4.wav";
		cnt = 4;
		break;
	case CHAR_TEX_WOOD:
		fvol = 0.9;
		fvolbar = 0.2;
		rgsz[0] = "debris/wood1.wav";
		rgsz[1] = "debris/wood2.wav";
		rgsz[2] = "debris/wood3.wav";
		cnt = 3;
		break;
	case CHAR_TEX_GLASS:
	case CHAR_TEX_COMPUTER:
		fvol = 0.8;
		fvolbar = 0.2;
		rgsz[0] = "debris/glass1.wav";
		rgsz[1] = "debris/glass2.wav";
		rgsz[2] = "debris/glass3.wav";
		cnt = 3;
		break;
	case CHAR_TEX_FLESH:
		if (iBulletType == BULLET_PLAYER_SLEDGEHAMMER)
			return 0.0; // crowbar already makes this sound
		fvol = 1.0;
		fvolbar = 0.2;
		rgsz[0] = "weapons/bullet_hit1.wav";
		rgsz[1] = "weapons/bullet_hit2.wav";
		fattn = 1.0;
		cnt = 2;
		break;
	}

	// play material hit sound
	gEngfuncs.pEventAPI->EV_PlaySound(0, ptr->endpos, CHAN_STATIC, rgsz[gEngfuncs.pfnRandomLong(0, cnt - 1)], fvol, fattn, 0, 96 + gEngfuncs.pfnRandomLong(0, 0xf));
	return fvolbar;
}

char* EV_HLDM_DamageDecal(physent_t* pe)
{
	static char decalname[32];
	int idx;

	if (pe->rendermode == kRenderTransAlpha)
		return nullptr;

	if (pe->classnumber == 1)
	{
		idx = gEngfuncs.pfnRandomLong(0, 2);
		sprintf(decalname, "{break%i", idx + 1);
	}
	else if (pe->rendermode != kRenderNormal)
	{
		sprintf(decalname, "{bproof1");
	}
	else
	{
		idx = gEngfuncs.pfnRandomLong(0, 4);
		sprintf(decalname, "{shot%i", idx + 1);
	}
	return decalname;
}

void EV_HLDM_GunshotDecalTrace(pmtrace_t* pTrace, char* decalName)
{
	int iRand;

	gEngfuncs.pEfxAPI->R_BulletImpactParticles(pTrace->endpos);

	iRand = gEngfuncs.pfnRandomLong(0, 0x7FFF);
	if (iRand < (0x7fff / 2)) // not every bullet makes a sound.
	{
		switch (iRand % 5)
		{
		case 0:
			gEngfuncs.pEventAPI->EV_PlaySound(-1, pTrace->endpos, 0, "weapons/ric1.wav", 1.0, ATTN_NORM, 0, PITCH_NORM);
			break;
		case 1:
			gEngfuncs.pEventAPI->EV_PlaySound(-1, pTrace->endpos, 0, "weapons/ric2.wav", 1.0, ATTN_NORM, 0, PITCH_NORM);
			break;
		case 2:
			gEngfuncs.pEventAPI->EV_PlaySound(-1, pTrace->endpos, 0, "weapons/ric3.wav", 1.0, ATTN_NORM, 0, PITCH_NORM);
			break;
		case 3:
			gEngfuncs.pEventAPI->EV_PlaySound(-1, pTrace->endpos, 0, "weapons/ric4.wav", 1.0, ATTN_NORM, 0, PITCH_NORM);
			break;
		case 4:
			gEngfuncs.pEventAPI->EV_PlaySound(-1, pTrace->endpos, 0, "weapons/ric5.wav", 1.0, ATTN_NORM, 0, PITCH_NORM);
			break;
		}
	}

	// Only decal brush models such as the world etc.
	if (decalName && '\0' != decalName[0] && r_decals->value > 0)
	{
		gEngfuncs.pEfxAPI->R_DecalShoot(
			gEngfuncs.pEfxAPI->Draw_DecalIndex(gEngfuncs.pEfxAPI->Draw_DecalIndexFromName(decalName)),
			gEngfuncs.pEventAPI->EV_IndexFromTrace(pTrace), 0, pTrace->endpos, 0);
	}
}

void EV_HLDM_DecalGunshot(pmtrace_t* pTrace, int iBulletType)
{
	physent_t* pe;

	pe = gEngfuncs.pEventAPI->EV_GetPhysent(pTrace->ent);

	if (EV_HLDM_IsBSPModel(pe))
	{
		switch (iBulletType)
		{
		case BULLET_PLAYER_MP5:
		//case BULLET_MONSTER_MP5:
		case BULLET_PLAYER_50:
		default:
			// smoke and decal
			EV_HLDM_GunshotDecalTrace(pTrace, EV_HLDM_DamageDecal(pe));
			break;
		}
	}
}

void EV_HLDM_CheckTracer(int idx, float* vecSrc, float* end, float* forward, float* right, int iBulletType, int iTracerFreq, int* tracerCount)
{
	int i;
	bool player = idx >= 1 && idx <= gEngfuncs.GetMaxClients();

	if (iTracerFreq != 0 && ((*tracerCount)++ % iTracerFreq) == 0)
	{
		Vector vecTracerSrc;

		if (player)
		{
			Vector offset(0, 0, -4);

			// adjust tracer position for player
			for (i = 0; i < 3; i++)
			{
				vecTracerSrc[i] = vecSrc[i] + offset[i] + right[i] * 2 + forward[i] * 16;
			}
		}
		else
		{
			VectorCopy(vecSrc, vecTracerSrc);
		}

		switch (iBulletType)
		{
		case BULLET_PLAYER_MP5:
		default:
			EV_CreateTracer(vecTracerSrc, end);
			break;
		}
	}
}


/*
================
FireBullets

Go to the trouble of combining multiple pellets into a single damage call.
================
*/
void EV_HLDM_FireBullets(int idx, float* forward, float* right, float* up, int cShots, float* vecSrc, float* vecDirShooting, float flDistance, int iBulletType, int iTracerFreq, int* tracerCount, float flSpreadX, float flSpreadY)
{
	int i;
	pmtrace_t tr;
	int iShot;

	for (iShot = 1; iShot <= cShots; iShot++)
	{
		Vector vecDir, vecEnd;

		float x, y, z;
		for (i = 0; i < 3; i++)
		{
			vecDir[i] = vecDirShooting[i] + flSpreadX * right[i] + flSpreadY * up[i];
			vecEnd[i] = vecSrc[i] + flDistance * vecDir[i];
		}
		
		gEngfuncs.pEventAPI->EV_SetUpPlayerPrediction(0, 1);

		// Store off the old count
		gEngfuncs.pEventAPI->EV_PushPMStates();

		// Now add in all of the players.
		gEngfuncs.pEventAPI->EV_SetSolidPlayers(idx - 1);

		gEngfuncs.pEventAPI->EV_SetTraceHull(2);
		// JoshA: Changed from PM_STUDIO_BOX to PM_NORMAL in prediction code as otherwise if you hit an NPC or player's
		// bounding box but not one of their hitboxes, the shot won't hit on the server but it will
		// play a hit sound on the client and not make a decal (as if it hit the NPC/player).
		// We should mirror the way the server does the test here as close as possible.
		//
		// I initially thought I was just fixing some stupid Half-Life bug but no,
		// this is *the* root cause of all the ghost shot bad prediction bugs in Half-Life Deathmatch!
		//
		// Also... CStrike was always using PM_NORMAL for all of these so it didn't have the problem.
		gEngfuncs.pEventAPI->EV_PlayerTrace(vecSrc, vecEnd, PM_NORMAL, -1, &tr);

		EV_HLDM_CheckTracer(idx, vecSrc, tr.endpos, forward, right, iBulletType, iTracerFreq, tracerCount);

		// do damage, paint decals
		if (tr.fraction != 1.0)
		{
			switch (iBulletType)
			{
			default:
			case BULLET_PLAYER_MP5:

				EV_HLDM_PlayTextureSound(idx, &tr, vecSrc, vecEnd, iBulletType);
				EV_HLDM_DecalGunshot(&tr, iBulletType);

				break;
			case BULLET_PLAYER_50:

				EV_HLDM_PlayTextureSound(idx, &tr, vecSrc, vecEnd, iBulletType);
				EV_HLDM_DecalGunshot(&tr, iBulletType);

				break;
			}
		}

		gEngfuncs.pEventAPI->EV_PopPMStates();
	}
}

//======================
//	    MP5 START
//======================
void EV_FireMP5(event_args_t* args)
{
	int idx;
	Vector origin;
	Vector angles;
	Vector velocity;

	Vector ShellVelocity;
	Vector ShellOrigin;
	int shell;
	Vector vecSrc, vecAiming;
	Vector up, right, forward;

	idx = args->entindex;
	VectorCopy(args->origin, origin);
	VectorCopy(args->angles, angles);
	VectorCopy(args->velocity, velocity);

	AngleVectors(angles, forward, right, up);

	shell = gEngfuncs.pEventAPI->EV_FindModelIndex("models/shell.mdl"); // brass shell

	if (EV_IsLocal(idx))
	{
		// Add muzzle flash to current weapon model
		EV_MuzzleFlash();
		gEngfuncs.pEventAPI->EV_WeaponAnimation(MP5_FIRE1 + gEngfuncs.pfnRandomLong(0, 2), 0);

		V_PunchAxis(0, gEngfuncs.pfnRandomFloat(-2, 2));
	}

	EV_GetDefaultShellInfo(args, origin, velocity, ShellVelocity, ShellOrigin, forward, right, up, 20, -12, 4);

	EV_EjectBrass(ShellOrigin, ShellVelocity, angles[YAW], shell, TE_BOUNCE_SHELL);

	switch (gEngfuncs.pfnRandomLong(0, 1))
	{
	case 0:
		gEngfuncs.pEventAPI->EV_PlaySound(idx, origin, CHAN_WEAPON, "weapons/hks1.wav", 1, ATTN_NORM, 0, 94 + gEngfuncs.pfnRandomLong(0, 0xf));
		break;
	case 1:
		gEngfuncs.pEventAPI->EV_PlaySound(idx, origin, CHAN_WEAPON, "weapons/hks2.wav", 1, ATTN_NORM, 0, 94 + gEngfuncs.pfnRandomLong(0, 0xf));
		break;
	}

	EV_GetGunPosition(args, vecSrc, origin);
	VectorCopy(forward, vecAiming);

	EV_HLDM_FireBullets(idx, forward, right, up, 1, vecSrc, vecAiming, 8192, BULLET_PLAYER_MP5, 2, &tracerCount[idx - 1], args->fparam1, args->fparam2);
}
//======================
//		 MP5 END
//======================

//======================
//	   FIFTY CAL START
//
//======================
void EV_FireFiftyCal(event_args_t* args)
{
	int idx;
	Vector origin;
	Vector angles;
	Vector velocity;

	Vector vecSrc, vecAiming;
	Vector up, right, forward;

	idx = args->entindex;
	VectorCopy(args->origin, origin);
	VectorCopy(args->angles, angles);
	VectorCopy(args->velocity, velocity);

	AngleVectors(angles, forward, right, up);

	if (EV_IsLocal(idx))
	{
		// Python uses different body in multiplayer versus single player
		bool multiplayer = gEngfuncs.GetMaxClients() != 1;

		// Add muzzle flash to current weapon model
		EV_MuzzleFlash();
		gEngfuncs.pEventAPI->EV_WeaponAnimation(FIFTYCAL_FIRE1, multiplayer ? 1 : 0);

		V_PunchAxis(0, -10.0);
	}

	switch (gEngfuncs.pfnRandomLong(0, 1))
	{
	case 0:
		gEngfuncs.pEventAPI->EV_PlaySound(idx, origin, CHAN_WEAPON, "weapons/357_shot1.wav", gEngfuncs.pfnRandomFloat(0.8, 0.9), ATTN_NORM, 0, PITCH_NORM);
		break;
	case 1:
		gEngfuncs.pEventAPI->EV_PlaySound(idx, origin, CHAN_WEAPON, "weapons/357_shot2.wav", gEngfuncs.pfnRandomFloat(0.8, 0.9), ATTN_NORM, 0, PITCH_NORM);
		break;
	}

	EV_GetGunPosition(args, vecSrc, origin);

	VectorCopy(forward, vecAiming);

	EV_HLDM_FireBullets(idx, forward, right, up, 1, vecSrc, vecAiming, 8192, BULLET_PLAYER_50, 0, &tracerCount[idx - 1], args->fparam1, args->fparam2);
}
//======================
//	   FIFTYCAL END
//======================

//======================
//     SLEDGEHAMMER START
//======================
int g_iSwing;

void EV_Sledgehammer(event_args_t* args)
{
	int idx;
	Vector origin;

	idx = args->entindex;
	VectorCopy(args->origin, origin);

	// Play Swing sound
	gEngfuncs.pEventAPI->EV_PlaySound(idx, origin, CHAN_WEAPON, "weapons/cbar_miss1.wav", 1, ATTN_NORM, 0, PITCH_NORM);

	if (EV_IsLocal(idx))
	{
		switch ((g_iSwing++) % 3)// This switch is so cursed
		{
		case 0:
			gEngfuncs.pEventAPI->EV_WeaponAnimation(SLEDGEHAMMER_ATTACK1MISS, 0);
			break;
		case 1:
			gEngfuncs.pEventAPI->EV_WeaponAnimation(SLEDGEHAMMER_ATTACK2MISS, 0);
			break;
		case 2:
			gEngfuncs.pEventAPI->EV_WeaponAnimation(SLEDGEHAMMER_ATTACK3MISS, 0);
			break;
		}
	}
}
//======================
//    SLEDGEHAMMER END
//======================

void EV_TrainPitchAdjust(event_args_t* args)
{
	int idx;
	Vector origin;

	unsigned short us_params;
	int noise;
	float m_flVolume;
	int pitch;
	bool stop;

	char sz[256];

	idx = args->entindex;

	VectorCopy(args->origin, origin);

	us_params = (unsigned short)args->iparam1;
	stop = 0 != args->bparam1;

	m_flVolume = (float)(us_params & 0x003f) / 40.0;
	noise = (int)(((us_params) >> 12) & 0x0007);
	pitch = (int)(10.0 * (float)((us_params >> 6) & 0x003f));

	switch (noise)
	{
	case 1:
		strcpy(sz, "plats/ttrain1.wav");
		break;
	case 2:
		strcpy(sz, "plats/ttrain2.wav");
		break;
	case 3:
		strcpy(sz, "plats/ttrain3.wav");
		break;
	case 4:
		strcpy(sz, "plats/ttrain4.wav");
		break;
	case 5:
		strcpy(sz, "plats/ttrain6.wav");
		break;
	case 6:
		strcpy(sz, "plats/ttrain7.wav");
		break;
	default:
		// no sound
		strcpy(sz, "");
		return;
	}

	if (stop)
	{
		gEngfuncs.pEventAPI->EV_StopSound(idx, CHAN_STATIC, sz);
	}
	else
	{
		gEngfuncs.pEventAPI->EV_PlaySound(idx, origin, CHAN_STATIC, sz, m_flVolume, ATTN_NORM, SND_CHANGE_PITCH, pitch);
	}
}

void EV_VehiclePitchAdjust(event_args_t* args)
{
	int idx;
	Vector origin;

	unsigned short us_params;
	int noise;
	float m_flVolume;
	int pitch;
	bool stop;

	char sz[256];

	idx = args->entindex;

	VectorCopy(args->origin, origin);

	us_params = (unsigned short)args->iparam1;
	stop = 0 != args->bparam1;

	m_flVolume = (float)(us_params & 0x003f) / 40.0;
	noise = (int)(((us_params) >> 12) & 0x0007);
	pitch = (int)(10.0 * (float)((us_params >> 6) & 0x003f));

	switch (noise)
	{
	case 1:
		strcpy(sz, "plats/vehicle1.wav");
		break;
	case 2:
		strcpy(sz, "plats/vehicle2.wav");
		break;
	case 3:
		strcpy(sz, "plats/vehicle3.wav");
		break;
	case 4:
		strcpy(sz, "plats/vehicle4.wav");
		break;
	case 5:
		strcpy(sz, "plats/vehicle6.wav");
		break;
	case 6:
		strcpy(sz, "plats/vehicle7.wav");
		break;
	default:
		// no sound
		strcpy(sz, "");
		return;
	}

	if (stop)
	{
		gEngfuncs.pEventAPI->EV_StopSound(idx, CHAN_STATIC, sz);
	}
	else
	{
		gEngfuncs.pEventAPI->EV_PlaySound(idx, origin, CHAN_STATIC, sz, m_flVolume, ATTN_NORM, SND_CHANGE_PITCH, pitch);
	}
}

bool EV_TFC_IsAllyTeam(int iTeam1, int iTeam2)
{
	return false;
}
