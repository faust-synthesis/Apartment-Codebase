/***
*
*	Copyright (c) 1996-2001, Valve LLC. All rights reserved.
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
//=========================================================
// skill.h - skill level concerns
//=========================================================

#pragma once

struct skilldata_t
{

	int iSkillLevel;

	float barneyHealth;

	float hgruntHealth;
	float hgruntDmgKick;
	float hgruntShotgunPellets;

	float zombieHealth;
	float zombieDmgOneSlash;
	float zombieDmgBothSlash;


	// Player Weapons
	float plrDmgSledgehammer;
	float plrDmg50Cal;
	float plrDmgMP5;

	// weapons shared by monsters
	float monDmgMP5;

	// health/suit charge
	float healthkitCapacity;

	// monster damage adj
	float monHead;
	float monChest;
	float monStomach;
	float monLeg;
	float monArm;

	// player damage adj
	float plrHead;
	float plrChest;
	float plrStomach;
	float plrLeg;
	float plrArm;
};

inline DLL_GLOBAL skilldata_t gSkillData;
float GetSkillCvar(const char* pName);

inline DLL_GLOBAL int g_iSkillLevel;

#define SKILL_EASY 1
#define SKILL_MEDIUM 2
#define SKILL_HARD 3
