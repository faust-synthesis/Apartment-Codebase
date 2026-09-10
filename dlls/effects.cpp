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
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "customentity.h"
#include "effects.h"
#include "weapons.h"
#include "decals.h"
#include "func_break.h"
#include "shake.h"

#define SF_GIBSHOOTER_REPEATABLE 1 // allows a gibshooter to be refired

#define SF_FUNNEL_REVERSE 1 // funnel effect repels particles instead of attracting them.


// Lightning target, just alias landmark
LINK_ENTITY_TO_CLASS(info_target, CPointEntity);


class CBubbling : public CBaseEntity
{
public:
	void Spawn() override;
	void Precache() override;
	bool KeyValue(KeyValueData* pkvd) override;

	void EXPORT FizzThink();
	void Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value) override;

	bool Save(CSave& save) override;
	bool Restore(CRestore& restore) override;
	int ObjectCaps() override { return CBaseEntity::ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }
	static TYPEDESCRIPTION m_SaveData[];

	int m_density;
	int m_frequency;
	int m_bubbleModel;
	bool m_state;
};

LINK_ENTITY_TO_CLASS(env_bubbles, CBubbling);

TYPEDESCRIPTION CBubbling::m_SaveData[] =
	{
		DEFINE_FIELD(CBubbling, m_density, FIELD_INTEGER),
		DEFINE_FIELD(CBubbling, m_frequency, FIELD_INTEGER),
		DEFINE_FIELD(CBubbling, m_state, FIELD_BOOLEAN),
		// Let spawn restore this!
		//	DEFINE_FIELD( CBubbling, m_bubbleModel, FIELD_INTEGER ),
};

IMPLEMENT_SAVERESTORE(CBubbling, CBaseEntity);


#define SF_BUBBLES_STARTOFF 0x0001

void CBubbling::Spawn()
{
	Precache();
	SET_MODEL(ENT(pev), STRING(pev->model)); // Set size

	pev->solid = SOLID_NOT; // Remove model & collisions
	pev->renderamt = 0;		// The engine won't draw this model if this is set to 0 and blending is on
	pev->rendermode = kRenderTransTexture;
	int speed = pev->speed > 0 ? pev->speed : -pev->speed;

	// HACKHACK!!! - Speed in rendercolor
	pev->rendercolor.x = speed >> 8;
	pev->rendercolor.y = speed & 255;
	pev->rendercolor.z = (pev->speed < 0) ? 1 : 0;


	if ((pev->spawnflags & SF_BUBBLES_STARTOFF) == 0)
	{
		SetThink(&CBubbling::FizzThink);
		pev->nextthink = gpGlobals->time + 2.0;
		m_state = true;
	}
	else
		m_state = false;
}

void CBubbling::Precache()
{
	m_bubbleModel = PRECACHE_MODEL("sprites/bubble.spr"); // Precache bubble sprite
}


void CBubbling::Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value)
{
	if (ShouldToggle(useType, m_state))
		m_state = !m_state;

	if (m_state)
	{
		SetThink(&CBubbling::FizzThink);
		pev->nextthink = gpGlobals->time + 0.1;
	}
	else
	{
		SetThink(NULL);
		pev->nextthink = 0;
	}
}


bool CBubbling::KeyValue(KeyValueData* pkvd)
{
	if (FStrEq(pkvd->szKeyName, "density"))
	{
		m_density = atoi(pkvd->szValue);
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "frequency"))
	{
		m_frequency = atoi(pkvd->szValue);
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "current"))
	{
		pev->speed = atoi(pkvd->szValue);
		return true;
	}

	return CBaseEntity::KeyValue(pkvd);
}


void CBubbling::FizzThink()
{
	MESSAGE_BEGIN(MSG_PAS, SVC_TEMPENTITY, VecBModelOrigin(pev));
	WRITE_BYTE(TE_FIZZ);
	WRITE_SHORT((short)ENTINDEX(edict()));
	WRITE_SHORT((short)m_bubbleModel);
	WRITE_BYTE(m_density);
	MESSAGE_END();

	if (m_frequency > 19)
		pev->nextthink = gpGlobals->time + 0.5;
	else
		pev->nextthink = gpGlobals->time + 2.5 - (0.1 * m_frequency);
}



class CGlow : public CPointEntity
{
public:
	void Spawn() override;
	void Think() override;
	void Animate(float frames);
	bool Save(CSave& save) override;
	bool Restore(CRestore& restore) override;
	static TYPEDESCRIPTION m_SaveData[];

	float m_lastTime;
	float m_maxFrame;
};

LINK_ENTITY_TO_CLASS(env_glow, CGlow);

TYPEDESCRIPTION CGlow::m_SaveData[] =
	{
		DEFINE_FIELD(CGlow, m_lastTime, FIELD_TIME),
		DEFINE_FIELD(CGlow, m_maxFrame, FIELD_FLOAT),
};

IMPLEMENT_SAVERESTORE(CGlow, CPointEntity);

void CGlow::Spawn()
{
	pev->solid = SOLID_NOT;
	pev->movetype = MOVETYPE_NONE;
	pev->effects = 0;
	pev->frame = 0;

	PRECACHE_MODEL((char*)STRING(pev->model));
	SET_MODEL(ENT(pev), STRING(pev->model));

	m_maxFrame = (float)MODEL_FRAMES(pev->modelindex) - 1;
	if (m_maxFrame > 1.0 && pev->framerate != 0)
		pev->nextthink = gpGlobals->time + 0.1;

	m_lastTime = gpGlobals->time;
}


void CGlow::Think()
{
	Animate(pev->framerate * (gpGlobals->time - m_lastTime));

	pev->nextthink = gpGlobals->time + 0.1;
	m_lastTime = gpGlobals->time;
}


void CGlow::Animate(float frames)
{
	if (m_maxFrame > 0)
		pev->frame = fmod(pev->frame + frames, m_maxFrame);
}


LINK_ENTITY_TO_CLASS(env_sprite, CSprite);

TYPEDESCRIPTION CSprite::m_SaveData[] =
	{
		DEFINE_FIELD(CSprite, m_lastTime, FIELD_TIME),
		DEFINE_FIELD(CSprite, m_maxFrame, FIELD_FLOAT),
};

IMPLEMENT_SAVERESTORE(CSprite, CPointEntity);

void CSprite::Spawn()
{
	pev->solid = SOLID_NOT;
	pev->movetype = MOVETYPE_NONE;
	pev->effects = 0;
	pev->frame = 0;

	Precache();
	SET_MODEL(ENT(pev), STRING(pev->model));

	m_maxFrame = (float)MODEL_FRAMES(pev->modelindex) - 1;
	if (!FStringNull(pev->targetname) && (pev->spawnflags & SF_SPRITE_STARTON) == 0)
		TurnOff();
	else
		TurnOn();

	// Worldcraft only sets y rotation, copy to Z
	if (pev->angles.y != 0 && pev->angles.z == 0)
	{
		pev->angles.z = pev->angles.y;
		pev->angles.y = 0;
	}
}


void CSprite::Precache()
{
	PRECACHE_MODEL((char*)STRING(pev->model));

	// Reset attachment after save/restore
	if (pev->aiment)
		SetAttachment(pev->aiment, pev->body);
	else
	{
		// Clear attachment
		pev->skin = 0;
		pev->body = 0;
	}
}


void CSprite::SpriteInit(const char* pSpriteName, const Vector& origin)
{
	pev->model = MAKE_STRING(pSpriteName);
	pev->origin = origin;
	Spawn();
}

CSprite* CSprite::SpriteCreate(const char* pSpriteName, const Vector& origin, bool animate)
{
	CSprite* pSprite = GetClassPtr((CSprite*)NULL);
	pSprite->SpriteInit(pSpriteName, origin);
	pSprite->pev->classname = MAKE_STRING("env_sprite");
	pSprite->pev->solid = SOLID_NOT;
	pSprite->pev->movetype = MOVETYPE_NOCLIP;
	if (animate)
		pSprite->TurnOn();

	return pSprite;
}


void CSprite::AnimateThink()
{
	Animate(pev->framerate * (gpGlobals->time - m_lastTime));

	pev->nextthink = gpGlobals->time + 0.1;
	m_lastTime = gpGlobals->time;
}

void CSprite::AnimateUntilDead()
{
	if (gpGlobals->time > pev->dmgtime)
		UTIL_Remove(this);
	else
	{
		AnimateThink();
		pev->nextthink = gpGlobals->time;
	}
}

void CSprite::Expand(float scaleSpeed, float fadeSpeed)
{
	pev->speed = scaleSpeed;
	pev->health = fadeSpeed;
	SetThink(&CSprite::ExpandThink);

	pev->nextthink = gpGlobals->time;
	m_lastTime = gpGlobals->time;
}


void CSprite::ExpandThink()
{
	float frametime = gpGlobals->time - m_lastTime;
	pev->scale += pev->speed * frametime;
	pev->renderamt -= pev->health * frametime;
	if (pev->renderamt <= 0)
	{
		pev->renderamt = 0;
		UTIL_Remove(this);
	}
	else
	{
		pev->nextthink = gpGlobals->time + 0.1;
		m_lastTime = gpGlobals->time;
	}
}


void CSprite::Animate(float frames)
{
	pev->frame += frames;
	if (pev->frame > m_maxFrame)
	{
		if ((pev->spawnflags & SF_SPRITE_ONCE) != 0)
		{
			TurnOff();
		}
		else
		{
			if (m_maxFrame > 0)
				pev->frame = fmod(pev->frame, m_maxFrame);
		}
	}
}


void CSprite::TurnOff()
{
	pev->effects = EF_NODRAW;
	pev->nextthink = 0;
}


void CSprite::TurnOn()
{
	pev->effects = 0;
	if ((0 != pev->framerate && m_maxFrame > 1.0) || (pev->spawnflags & SF_SPRITE_ONCE) != 0)
	{
		SetThink(&CSprite::AnimateThink);
		pev->nextthink = gpGlobals->time;
		m_lastTime = gpGlobals->time;
	}
	pev->frame = 0;
}


void CSprite::Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value)
{
	bool on = pev->effects != EF_NODRAW;
	if (ShouldToggle(useType, on))
	{
		if (on)
		{
			TurnOff();
		}
		else
		{
			TurnOn();
		}
	}
}


class CGibShooter : public CBaseDelay
{
public:
	void Spawn() override;
	void Precache() override;
	bool KeyValue(KeyValueData* pkvd) override;
	void EXPORT ShootThink();
	void Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value) override;

	virtual CGib* CreateGib();

	bool Save(CSave& save) override;
	bool Restore(CRestore& restore) override;
	static TYPEDESCRIPTION m_SaveData[];

	int m_iGibs;
	int m_iGibCapacity;
	int m_iGibMaterial;
	int m_iGibModelIndex;
	float m_flGibVelocity;
	float m_flVariance;
	float m_flGibLife;
};

TYPEDESCRIPTION CGibShooter::m_SaveData[] =
	{
		DEFINE_FIELD(CGibShooter, m_iGibs, FIELD_INTEGER),
		DEFINE_FIELD(CGibShooter, m_iGibCapacity, FIELD_INTEGER),
		DEFINE_FIELD(CGibShooter, m_iGibMaterial, FIELD_INTEGER),
		DEFINE_FIELD(CGibShooter, m_iGibModelIndex, FIELD_INTEGER),
		DEFINE_FIELD(CGibShooter, m_flGibVelocity, FIELD_FLOAT),
		DEFINE_FIELD(CGibShooter, m_flVariance, FIELD_FLOAT),
		DEFINE_FIELD(CGibShooter, m_flGibLife, FIELD_FLOAT),
};

IMPLEMENT_SAVERESTORE(CGibShooter, CBaseDelay);
LINK_ENTITY_TO_CLASS(gibshooter, CGibShooter);


void CGibShooter::Precache()
{
	if (g_Language == LANGUAGE_GERMAN)
	{
		m_iGibModelIndex = PRECACHE_MODEL("models/germanygibs.mdl");
	}
	else
	{
		m_iGibModelIndex = PRECACHE_MODEL("models/hgibs.mdl");
	}
}


bool CGibShooter::KeyValue(KeyValueData* pkvd)
{
	if (FStrEq(pkvd->szKeyName, "m_iGibs"))
	{
		m_iGibs = m_iGibCapacity = atoi(pkvd->szValue);
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "m_flVelocity"))
	{
		m_flGibVelocity = atof(pkvd->szValue);
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "m_flVariance"))
	{
		m_flVariance = atof(pkvd->szValue);
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "m_flGibLife"))
	{
		m_flGibLife = atof(pkvd->szValue);
		return true;
	}

	return CBaseDelay::KeyValue(pkvd);
}

void CGibShooter::Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value)
{
	SetThink(&CGibShooter::ShootThink);
	pev->nextthink = gpGlobals->time;
}

void CGibShooter::Spawn()
{
	Precache();

	pev->solid = SOLID_NOT;
	pev->effects = EF_NODRAW;

	if (m_flDelay == 0)
	{
		m_flDelay = 0.1;
	}

	if (m_flGibLife == 0)
	{
		m_flGibLife = 25;
	}

	SetMovedir(pev);
	pev->body = MODEL_FRAMES(m_iGibModelIndex);
}


CGib* CGibShooter::CreateGib()
{
	if (CVAR_GET_FLOAT("violence_hgibs") == 0)
		return NULL;

	CGib* pGib = GetClassPtr((CGib*)NULL);
	pGib->Spawn("models/hgibs.mdl");
	pGib->m_bloodColor = BLOOD_COLOR_RED;

	if (pev->body <= 1)
	{
		ALERT(at_aiconsole, "GibShooter Body is <= 1!\n");
	}

	pGib->pev->body = RANDOM_LONG(1, pev->body - 1); // avoid throwing random amounts of the 0th gib. (skull).

	return pGib;
}


void CGibShooter::ShootThink()
{
	pev->nextthink = gpGlobals->time + m_flDelay;

	Vector vecShootDir;

	vecShootDir = pev->movedir;

	vecShootDir = vecShootDir + gpGlobals->v_right * RANDOM_FLOAT(-1, 1) * m_flVariance;
	vecShootDir = vecShootDir + gpGlobals->v_forward * RANDOM_FLOAT(-1, 1) * m_flVariance;
	vecShootDir = vecShootDir + gpGlobals->v_up * RANDOM_FLOAT(-1, 1) * m_flVariance;

	vecShootDir = vecShootDir.Normalize();
	CGib* pGib = CreateGib();

	if (pGib)
	{
		pGib->pev->origin = pev->origin;
		pGib->pev->velocity = vecShootDir * m_flGibVelocity;

		pGib->pev->avelocity.x = RANDOM_FLOAT(100, 200);
		pGib->pev->avelocity.y = RANDOM_FLOAT(100, 300);

		float thinkTime = pGib->pev->nextthink - gpGlobals->time;

		pGib->m_lifeTime = (m_flGibLife * RANDOM_FLOAT(0.95, 1.05)); // +/- 5%
		if (pGib->m_lifeTime < thinkTime)
		{
			pGib->pev->nextthink = gpGlobals->time + pGib->m_lifeTime;
			pGib->m_lifeTime = 0;
		}
	}

	if (--m_iGibs <= 0)
	{
		if ((pev->spawnflags & SF_GIBSHOOTER_REPEATABLE) != 0)
		{
			m_iGibs = m_iGibCapacity;
			SetThink(NULL);
			pev->nextthink = gpGlobals->time;
		}
		else
		{
			SetThink(&CGibShooter::SUB_Remove);
			pev->nextthink = gpGlobals->time;
		}
	}
}


class CEnvShooter : public CGibShooter
{
	void Precache() override;
	bool KeyValue(KeyValueData* pkvd) override;

	CGib* CreateGib() override;
};

LINK_ENTITY_TO_CLASS(env_shooter, CEnvShooter);

bool CEnvShooter::KeyValue(KeyValueData* pkvd)
{
	if (FStrEq(pkvd->szKeyName, "shootmodel"))
	{
		pev->model = ALLOC_STRING(pkvd->szValue);
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "shootsounds"))
	{
		int iNoise = atoi(pkvd->szValue);

		switch (iNoise)
		{
		case 0:
			m_iGibMaterial = matGlass;
			break;
		case 1:
			m_iGibMaterial = matWood;
			break;
		case 2:
			m_iGibMaterial = matMetal;
			break;
		case 3:
			m_iGibMaterial = matFlesh;
			break;
		case 4:
			m_iGibMaterial = matRocks;
			break;

		default:
		case -1:
			m_iGibMaterial = matNone;
			break;
		}

		return true;
	}

	return CGibShooter::KeyValue(pkvd);
}


void CEnvShooter::Precache()
{
	m_iGibModelIndex = PRECACHE_MODEL((char*)STRING(pev->model));
	CBreakable::MaterialSoundPrecache((Materials)m_iGibMaterial);
}


CGib* CEnvShooter::CreateGib()
{
	CGib* pGib = GetClassPtr((CGib*)NULL);

	pGib->Spawn(STRING(pev->model));

	int bodyPart = 0;

	if (pev->body > 1)
		bodyPart = RANDOM_LONG(0, pev->body - 1);

	pGib->pev->body = bodyPart;
	pGib->m_bloodColor = DONT_BLEED;
	pGib->m_material = m_iGibMaterial;

	pGib->pev->rendermode = pev->rendermode;
	pGib->pev->renderamt = pev->renderamt;
	pGib->pev->rendercolor = pev->rendercolor;
	pGib->pev->renderfx = pev->renderfx;
	pGib->pev->scale = pev->scale;
	pGib->pev->skin = pev->skin;

	return pGib;
}



// Blood effects
class CBlood : public CPointEntity
{
public:
	void Spawn() override;
	void Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value) override;
	bool KeyValue(KeyValueData* pkvd) override;

	inline int Color() { return pev->impulse; }
	inline float BloodAmount() { return pev->dmg; }

	inline void SetColor(int color) { pev->impulse = color; }
	inline void SetBloodAmount(float amount) { pev->dmg = amount; }

	Vector Direction();
	Vector BloodPosition(CBaseEntity* pActivator);

private:
};

LINK_ENTITY_TO_CLASS(env_blood, CBlood);



#define SF_BLOOD_RANDOM 0x0001
#define SF_BLOOD_STREAM 0x0002
#define SF_BLOOD_PLAYER 0x0004
#define SF_BLOOD_DECAL 0x0008

void CBlood::Spawn()
{
	pev->solid = SOLID_NOT;
	pev->movetype = MOVETYPE_NONE;
	pev->effects = 0;
	pev->frame = 0;
	SetMovedir(pev);
}


bool CBlood::KeyValue(KeyValueData* pkvd)
{
	if (FStrEq(pkvd->szKeyName, "color"))
	{
		int color = atoi(pkvd->szValue);
		switch (color)
		{
		case 1:
			SetColor(BLOOD_COLOR_YELLOW);
			break;
		default:
			SetColor(BLOOD_COLOR_RED);
			break;
		}

		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "amount"))
	{
		SetBloodAmount(atof(pkvd->szValue));
		return true;
	}

	return CPointEntity::KeyValue(pkvd);
}


Vector CBlood::Direction()
{
	if ((pev->spawnflags & SF_BLOOD_RANDOM) != 0)
		return UTIL_RandomBloodVector();

	return pev->movedir;
}


Vector CBlood::BloodPosition(CBaseEntity* pActivator)
{
	if ((pev->spawnflags & SF_BLOOD_PLAYER) != 0)
	{
		CBaseEntity* pPlayer;

		if (pActivator && pActivator->IsPlayer())
		{
			pPlayer = pActivator;
		}
		else
			pPlayer = UTIL_GetLocalPlayer();
		if (pPlayer)
			return (pPlayer->pev->origin + pPlayer->pev->view_ofs) + Vector(RANDOM_FLOAT(-10, 10), RANDOM_FLOAT(-10, 10), RANDOM_FLOAT(-10, 10));
	}

	return pev->origin;
}


void CBlood::Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value)
{
	if ((pev->spawnflags & SF_BLOOD_STREAM) != 0)
		UTIL_BloodStream(BloodPosition(pActivator), Direction(), (Color() == BLOOD_COLOR_RED) ? 70 : Color(), BloodAmount());
	else
		UTIL_BloodDrips(BloodPosition(pActivator), Direction(), Color(), BloodAmount());

	if ((pev->spawnflags & SF_BLOOD_DECAL) != 0)
	{
		Vector forward = Direction();
		Vector start = BloodPosition(pActivator);
		TraceResult tr;

		UTIL_TraceLine(start, start + forward * BloodAmount() * 2, ignore_monsters, NULL, &tr);
		if (tr.flFraction != 1.0)
			UTIL_BloodDecalTrace(&tr, Color());
	}
}



// Screen shake
class CShake : public CPointEntity
{
public:
	void Spawn() override;
	void Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value) override;
	bool KeyValue(KeyValueData* pkvd) override;

	inline float Amplitude() { return pev->scale; }
	inline float Frequency() { return pev->dmg_save; }
	inline float Duration() { return pev->dmg_take; }
	inline float Radius() { return pev->dmg; }

	inline void SetAmplitude(float amplitude) { pev->scale = amplitude; }
	inline void SetFrequency(float frequency) { pev->dmg_save = frequency; }
	inline void SetDuration(float duration) { pev->dmg_take = duration; }
	inline void SetRadius(float radius) { pev->dmg = radius; }

private:
};

LINK_ENTITY_TO_CLASS(env_shake, CShake);

// pev->scale is amplitude
// pev->dmg_save is frequency
// pev->dmg_take is duration
// pev->dmg is radius
// radius of 0 means all players
// NOTE: UTIL_ScreenShake() will only shake players who are on the ground

#define SF_SHAKE_EVERYONE 0x0001 // Don't check radius
// UNDONE: These don't work yet
#define SF_SHAKE_DISRUPT 0x0002 // Disrupt controls
#define SF_SHAKE_INAIR 0x0004	// Shake players in air

void CShake::Spawn()
{
	pev->solid = SOLID_NOT;
	pev->movetype = MOVETYPE_NONE;
	pev->effects = 0;
	pev->frame = 0;

	if ((pev->spawnflags & SF_SHAKE_EVERYONE) != 0)
		pev->dmg = 0;
}


bool CShake::KeyValue(KeyValueData* pkvd)
{
	if (FStrEq(pkvd->szKeyName, "amplitude"))
	{
		SetAmplitude(atof(pkvd->szValue));
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "frequency"))
	{
		SetFrequency(atof(pkvd->szValue));
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "duration"))
	{
		SetDuration(atof(pkvd->szValue));
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "radius"))
	{
		SetRadius(atof(pkvd->szValue));
		return true;
	}

	return CPointEntity::KeyValue(pkvd);
}


void CShake::Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value)
{
	UTIL_ScreenShake(pev->origin, Amplitude(), Frequency(), Duration(), Radius());
}


class CFade : public CPointEntity
{
public:
	void Spawn() override;
	void Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value) override;
	bool KeyValue(KeyValueData* pkvd) override;

	inline float Duration() { return pev->dmg_take; }
	inline float HoldTime() { return pev->dmg_save; }

	inline void SetDuration(float duration) { pev->dmg_take = duration; }
	inline void SetHoldTime(float hold) { pev->dmg_save = hold; }

private:
};

LINK_ENTITY_TO_CLASS(env_fade, CFade);

// pev->dmg_take is duration
// pev->dmg_save is hold duration
#define SF_FADE_IN 0x0001		// Fade in, not out
#define SF_FADE_MODULATE 0x0002 // Modulate, don't blend
#define SF_FADE_ONLYONE 0x0004

void CFade::Spawn()
{
	pev->solid = SOLID_NOT;
	pev->movetype = MOVETYPE_NONE;
	pev->effects = 0;
	pev->frame = 0;
}


bool CFade::KeyValue(KeyValueData* pkvd)
{
	if (FStrEq(pkvd->szKeyName, "duration"))
	{
		SetDuration(atof(pkvd->szValue));
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "holdtime"))
	{
		SetHoldTime(atof(pkvd->szValue));
		return true;
	}

	return CPointEntity::KeyValue(pkvd);
}


void CFade::Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value)
{
	int fadeFlags = 0;

	if ((pev->spawnflags & SF_FADE_IN) == 0)
		fadeFlags |= FFADE_OUT;

	if ((pev->spawnflags & SF_FADE_MODULATE) != 0)
		fadeFlags |= FFADE_MODULATE;

	if ((pev->spawnflags & SF_FADE_ONLYONE) != 0)
	{
		if (pActivator->IsNetClient())
		{
			UTIL_ScreenFade(pActivator, pev->rendercolor, Duration(), HoldTime(), pev->renderamt, fadeFlags);
		}
	}
	else
	{
		UTIL_ScreenFadeAll(pev->rendercolor, Duration(), HoldTime(), pev->renderamt, fadeFlags);
	}
	SUB_UseTargets(this, USE_TOGGLE, 0);
}


class CMessage : public CPointEntity
{
public:
	void Spawn() override;
	void Precache() override;
	void Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value) override;
	bool KeyValue(KeyValueData* pkvd) override;

private:
};

LINK_ENTITY_TO_CLASS(env_message, CMessage);


void CMessage::Spawn()
{
	Precache();

	pev->solid = SOLID_NOT;
	pev->movetype = MOVETYPE_NONE;

	switch (pev->impulse)
	{
	case 1: // Medium radius
		pev->speed = ATTN_STATIC;
		break;

	case 2: // Large radius
		pev->speed = ATTN_NORM;
		break;

	case 3: //EVERYWHERE
		pev->speed = ATTN_NONE;
		break;

	default:
	case 0: // Small radius
		pev->speed = ATTN_IDLE;
		break;
	}
	pev->impulse = 0;

	// No volume, use normal
	if (pev->scale <= 0)
		pev->scale = 1.0;
}


void CMessage::Precache()
{
	if (!FStringNull(pev->noise))
		PRECACHE_SOUND((char*)STRING(pev->noise));
}

bool CMessage::KeyValue(KeyValueData* pkvd)
{
	if (FStrEq(pkvd->szKeyName, "messagesound"))
	{
		pev->noise = ALLOC_STRING(pkvd->szValue);
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "messagevolume"))
	{
		pev->scale = atof(pkvd->szValue) * 0.1;
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "messageattenuation"))
	{
		pev->impulse = atoi(pkvd->szValue);
		return true;
	}

	return CPointEntity::KeyValue(pkvd);
}


void CMessage::Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value)
{
	CBaseEntity* pPlayer = NULL;

	if ((pev->spawnflags & SF_MESSAGE_ALL) != 0)
		UTIL_ShowMessageAll(STRING(pev->message));
	else
	{
		if (pActivator && pActivator->IsPlayer())
			pPlayer = pActivator;
		else
		{
			pPlayer = UTIL_GetLocalPlayer();
		}
		if (pPlayer)
			UTIL_ShowMessage(STRING(pev->message), pPlayer);
	}
	if (!FStringNull(pev->noise))
	{
		EMIT_SOUND(edict(), CHAN_BODY, STRING(pev->noise), pev->scale, pev->speed);
	}
	if ((pev->spawnflags & SF_MESSAGE_ONCE) != 0)
		UTIL_Remove(this);

	SUB_UseTargets(this, USE_TOGGLE, 0);
}



//=========================================================
// FunnelEffect
//=========================================================
class CEnvFunnel : public CBaseDelay
{
public:
	void Spawn() override;
	void Precache() override;
	void Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value) override;

	int m_iSprite; // Don't save, precache
};

void CEnvFunnel::Precache()
{
	m_iSprite = PRECACHE_MODEL("sprites/flare6.spr");
}

LINK_ENTITY_TO_CLASS(env_funnel, CEnvFunnel);

void CEnvFunnel::Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value)
{
	MESSAGE_BEGIN(MSG_BROADCAST, SVC_TEMPENTITY);
	WRITE_BYTE(TE_LARGEFUNNEL);
	WRITE_COORD(pev->origin.x);
	WRITE_COORD(pev->origin.y);
	WRITE_COORD(pev->origin.z);
	WRITE_SHORT(m_iSprite);

	if ((pev->spawnflags & SF_FUNNEL_REVERSE) != 0) // funnel flows in reverse?
	{
		WRITE_SHORT(1);
	}
	else
	{
		WRITE_SHORT(0);
	}


	MESSAGE_END();

	SetThink(&CEnvFunnel::SUB_Remove);
	pev->nextthink = gpGlobals->time;
}

void CEnvFunnel::Spawn()
{
	Precache();
	pev->solid = SOLID_NOT;
	pev->effects = EF_NODRAW;
}

//=========================================================
// Beverage Dispenser
// overloaded pev->frags, is now a flag for whether or not a can is stuck in the dispenser.
// overloaded pev->health, is now how many cans remain in the machine.
//=========================================================
class CEnvBeverage : public CBaseDelay
{
public:
	void Spawn() override;
	void Precache() override;
	void Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value) override;
};

void CEnvBeverage::Precache()
{
	PRECACHE_MODEL("models/can.mdl");
	PRECACHE_SOUND("weapons/g_bounce3.wav");
}

LINK_ENTITY_TO_CLASS(env_beverage, CEnvBeverage);

void CEnvBeverage::Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value)
{
	if (pev->frags != 0 || pev->health <= 0)
	{
		// no more cans while one is waiting in the dispenser, or if I'm out of cans.
		return;
	}

	CBaseEntity* pCan = CBaseEntity::Create("item_sodacan", pev->origin, pev->angles, edict());

	if (pev->skin == 6)
	{
		// random
		pCan->pev->skin = RANDOM_LONG(0, 5);
	}
	else
	{
		pCan->pev->skin = pev->skin;
	}

	pev->frags = 1;
	pev->health--;

	//SetThink (SUB_Remove);
	//pev->nextthink = gpGlobals->time;
}

void CEnvBeverage::Spawn()
{
	Precache();
	pev->solid = SOLID_NOT;
	pev->effects = EF_NODRAW;
	pev->frags = 0;

	if (pev->health == 0)
	{
		pev->health = 10;
	}
}

//=========================================================
// Soda can
//=========================================================
class CItemSoda : public CBaseEntity
{
public:
	void Spawn() override;
	void Precache() override;
	void EXPORT CanThink();
	void EXPORT CanTouch(CBaseEntity* pOther);
};

void CItemSoda::Precache()
{
}

LINK_ENTITY_TO_CLASS(item_sodacan, CItemSoda);

void CItemSoda::Spawn()
{
	Precache();
	pev->solid = SOLID_NOT;
	pev->movetype = MOVETYPE_TOSS;

	SET_MODEL(ENT(pev), "models/can.mdl");
	UTIL_SetSize(pev, Vector(0, 0, 0), Vector(0, 0, 0));

	SetThink(&CItemSoda::CanThink);
	pev->nextthink = gpGlobals->time + 0.5;
}

void CItemSoda::CanThink()
{
	EMIT_SOUND(ENT(pev), CHAN_WEAPON, "weapons/g_bounce3.wav", 1, ATTN_NORM);

	pev->solid = SOLID_TRIGGER;
	UTIL_SetSize(pev, Vector(-8, -8, 0), Vector(8, 8, 8));
	SetThink(NULL);
	SetTouch(&CItemSoda::CanTouch);
}

void CItemSoda::CanTouch(CBaseEntity* pOther)
{
	if (!pOther->IsPlayer())
	{
		return;
	}

	// spoit sound here

	pOther->TakeHealth(1, DMG_GENERIC); // a bit of health.

	if (!FNullEnt(pev->owner))
	{
		// tell the machine the can was taken
		pev->owner->v.frags = 0;
	}

	pev->solid = SOLID_NOT;
	pev->movetype = MOVETYPE_NONE;
	pev->effects = EF_NODRAW;
	SetTouch(NULL);
	SetThink(&CItemSoda::SUB_Remove);
	pev->nextthink = gpGlobals->time;
}
