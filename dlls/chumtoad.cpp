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
#include "weapons.h"
#include "nodes.h"
#include "player.h"
#include "soundent.h"
#include "gamerules.h"
#include "game.h"

enum w_chumtoad_e {
	WCHUMTOAD_IDLE1 = 0,
	WCHUMTOAD_FIDGET,
	WCHUMTOAD_JUMP,
	WCHUMTOAD_RUN,
};

enum chumtoad_e {
	CHUMTOAD_IDLE1 = 0,
	CHUMTOAD_FIDGETFIT,
	CHUMTOAD_FIDGETNIP,
	CHUMTOAD_DOWN,
	CHUMTOAD_UP,
	CHUMTOAD_THROW,
	CHUMTOAD_RELEASE,
	CHUMTOAD_DRAW_LOWKEY,
	CHUMTOAD_DRAW,
};

// =====

#ifndef CLIENT_DLL

class CCaptureChumtoad : public CGrenade
{
	void Spawn( void );
	void Precache( void );
	int  Classify( void );
	void EXPORT SuperBounceTouch( CBaseEntity *pOther );
	void EXPORT HuntThink( void );
	void Killed( entvars_t *pevAttacker, int iGib );
	void GibMonster( void );

	static float m_flNextBounceSoundTime;

	float m_flDie;
	Vector m_vecTarget;
	float m_flNextHunt;
	float m_flNextHit;
	Vector m_posPrev;
	EHANDLE m_hOwner;
	int  m_iMyClass;
};

float CCaptureChumtoad::m_flNextBounceSoundTime = 0;

LINK_ENTITY_TO_CLASS( monster_ctctoad, CCaptureChumtoad );

#define SQUEEK_DETONATE_DELAY	15.0

int CCaptureChumtoad::Classify( void )
{
	if (m_iMyClass != 0)
		return m_iMyClass; // protect against recursion

	if (m_hEnemy != NULL)
	{
		m_iMyClass = CLASS_INSECT; // no one cares about it
		switch( m_hEnemy->Classify( ) )
		{
			case CLASS_PLAYER:
			case CLASS_HUMAN_PASSIVE:
			case CLASS_HUMAN_MILITARY:
				m_iMyClass = 0;
				return CLASS_ALIEN_MILITARY; // barney's get mad, grunts get mad at it
		}
		m_iMyClass = 0;
	}

	return CLASS_ALIEN_BIOWEAPON;
}

void CCaptureChumtoad::Spawn( void )
{
	Precache( );
	// motor
	pev->movetype = MOVETYPE_BOUNCE;
	pev->solid = SOLID_TRIGGER;

	SET_MODEL(ENT(pev), "models/w_chumtoad.mdl");
	UTIL_SetSize(pev, Vector(-16, -16, 0), Vector(16, 16, 8));
	UTIL_SetOrigin( pev, pev->origin );

	SetTouch( &CCaptureChumtoad::SuperBounceTouch );
	SetThink( &CCaptureChumtoad::HuntThink );
	pev->nextthink = gpGlobals->time + 0.1;
	m_flNextHunt = gpGlobals->time + 1E6;

	pev->flags |= FL_MONSTER;
	pev->takedamage	= DAMAGE_NO;
	pev->health	= gSkillData.chumtoadHealth;
	pev->gravity = 0.5;
	pev->friction = 0.5;

	pev->dmg = gSkillData.chumtoadDmgPop;

	// This chumtoad has a shell
	pev->renderfx = kRenderFxGlowShell;
	pev->renderamt = 10;
	pev->rendercolor.x = 0;
	pev->rendercolor.y = 200;
	pev->rendercolor.z = 0;

	m_flDie = gpGlobals->time + SQUEEK_DETONATE_DELAY;

	m_flFieldOfView = 0; // 180 degrees

	if ( pev->owner )
		m_hOwner = Instance( pev->owner );

	m_flNextBounceSoundTime = gpGlobals->time;// reset each time a snark is spawned.

	pev->sequence = WCHUMTOAD_RUN;
	ResetSequenceInfo( );
}

void CCaptureChumtoad::Precache( void )
{
	PRECACHE_MODEL("models/w_chumtoad.mdl");
	PRECACHE_SOUND("chumtoad_blast1.wav");
	PRECACHE_SOUND("common/bodysplat.wav");
	PRECACHE_SOUND("squeek/sqk_die1.wav");
	PRECACHE_SOUND("chumtoad_hunt1.wav");
	PRECACHE_SOUND("chumtoad_hunt2.wav");
	PRECACHE_SOUND("chumtoad_hunt3.wav");
}

void CCaptureChumtoad::Killed( entvars_t *pevAttacker, int iGib )
{
	pev->model = iStringNull;// make invisible
	SetThink( &CCaptureChumtoad::SUB_Remove );
	SetTouch( NULL );
	pev->nextthink = gpGlobals->time + 0.1;

	// since squeak grenades never leave a body behind, clear out their takedamage now.
	// Squeaks do a bit of radius damage when they pop, and that radius damage will
	// continue to call this function unless we acknowledge the Squeak's death now. (sjb)
	pev->takedamage = DAMAGE_NO;

	// play squeek blast
	EMIT_SOUND_DYN(ENT(pev), CHAN_ITEM, "chumtoad_blast1.wav", 1, 0.5, 0, PITCH_NORM);

	CSoundEnt::InsertSound ( bits_SOUND_COMBAT, pev->origin, SMALL_EXPLOSION_VOLUME, 3.0 );

	UTIL_BloodDrips( pev->origin, g_vecZero, iceblood.value ? BLOOD_COLOR_BLUE : BLOOD_COLOR_GREEN, 80 );

	if (m_hOwner != NULL)
		RadiusDamage ( pev, m_hOwner->pev, pev->dmg, CLASS_NONE, DMG_BLAST );
	else
		RadiusDamage ( pev, pev, pev->dmg, CLASS_NONE, DMG_BLAST );

	// reset owner so death message happens
	if (m_hOwner != NULL)
		pev->owner = m_hOwner->edict();

	CBaseMonster::Killed( pevAttacker, GIB_ALWAYS );
}

void CCaptureChumtoad::GibMonster( void )
{
	EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "common/bodysplat.wav", 0.75, ATTN_NORM, 0, 200);		
}

void CCaptureChumtoad::HuntThink( void )
{
	// ALERT( at_console, "think\n" );

	if (!IsInWorld())
	{
		SetTouch( NULL );
		UTIL_Remove( this );
		return;
	}
	
	StudioFrameAdvance( );
	pev->nextthink = gpGlobals->time + 0.1;

	// explode when ready
	/*if (gpGlobals->time >= m_flDie)
	{
		g_vecAttackDir = pev->velocity.Normalize( );
		pev->health = -1;
		Killed( pev, 0 );
		return;
	}*/

	// float
	if (pev->waterlevel != 0)
	{
		if (pev->movetype == MOVETYPE_BOUNCE)
		{
			pev->movetype = MOVETYPE_FLY;
		}
		pev->velocity = pev->velocity * 0.9;
		pev->velocity.z += 8.0;
	}
	else if (pev->movetype = MOVETYPE_FLY)
	{
		pev->movetype = MOVETYPE_BOUNCE;
	}

	// return if not time to hunt
	if (m_flNextHunt > gpGlobals->time)
		return;

	m_flNextHunt = gpGlobals->time + 2.0;
	
	CBaseEntity *pOther = NULL;
	Vector vecDir;
	TraceResult tr;

	Vector vecFlat = pev->velocity;
	vecFlat.z = 0;
	vecFlat = vecFlat.Normalize( );

	UTIL_MakeVectors( pev->angles );

	if (m_hEnemy == NULL || !m_hEnemy->IsAlive())
	{
		// find target, bounce a bit towards it.
		Look( 512 );
		m_hEnemy = BestVisibleEnemy( );
	}

/*
	// squeek if it's about time blow up
	if ((m_flDie - gpGlobals->time <= 0.5) && (m_flDie - gpGlobals->time >= 0.3))
	{
		EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "squeek/sqk_die1.wav", 1, ATTN_NORM, 0, 100 + RANDOM_LONG(0,0x3F));
		CSoundEnt::InsertSound ( bits_SOUND_COMBAT, pev->origin, 256, 0.25 );
	}
*/

	// higher pitch as squeeker gets closer to detonation time
	float flpitch = 155.0 - 60.0 * ((m_flDie - gpGlobals->time) / SQUEEK_DETONATE_DELAY);
	if (flpitch < 80)
		flpitch = 80;

	if (m_hEnemy != NULL)
	{
		if (FVisible( m_hEnemy ))
		{
			vecDir = m_hEnemy->EyePosition() - pev->origin;
			m_vecTarget = vecDir.Normalize( );
		}

		float flVel = pev->velocity.Length();
		float flAdj = 50.0 / (flVel + 10.0);

		if (flAdj > 1.2)
			flAdj = 1.2;
		
		// ALERT( at_console, "think : enemy\n");

		// ALERT( at_console, "%.0f %.2f %.2f %.2f\n", flVel, m_vecTarget.x, m_vecTarget.y, m_vecTarget.z );

		pev->velocity = pev->velocity * flAdj + m_vecTarget * 150;
	}

	if (pev->flags & FL_ONGROUND)
	{
		pev->avelocity = Vector( 0, 0, 0 );
	}
	else
	{
		if (pev->avelocity == Vector( 0, 0, 0))
		{
			pev->avelocity.x = RANDOM_FLOAT( -100, 100 );
			pev->avelocity.z = RANDOM_FLOAT( -100, 100 );
		}
	}

	if ((pev->origin - m_posPrev).Length() < 1.0)
	{
		pev->velocity.x = RANDOM_FLOAT( -100, 100 );
		pev->velocity.y = RANDOM_FLOAT( -100, 100 );
	}
	m_posPrev = pev->origin;

	pev->angles = UTIL_VecToAngles( pev->velocity );
	pev->angles.z = 0;
	pev->angles.x = 0;
}

void CCaptureChumtoad::SuperBounceTouch( CBaseEntity *pOther )
{
	float flpitch;

	TraceResult tr = UTIL_GetGlobalTrace( );

	// don't hit the guy that launched this grenade
	if ( pev->owner && pOther->edict() == pev->owner )
		return;

	// at least until we've bounced once
	pev->owner = NULL;

	pev->angles.x = 0;
	pev->angles.z = 0;

	// avoid bouncing too much
	if (m_flNextHit > gpGlobals->time)
		return;

	if (pOther->IsPlayer() && pOther->IsAlive())
	{
		CBasePlayer *pPlayer = (CBasePlayer *)pOther;
		if (!pPlayer->m_iHoldingChumtoad)
		{
			g_pGameRules->CaptureCharm(pPlayer);
	
			if (!pPlayer->HasNamedPlayerItem("weapon_chumtoad")) {
				pPlayer->GiveNamedItem("weapon_chumtoad");
				pPlayer->SelectItem("weapon_chumtoad");
				m_flNextHit = gpGlobals->time + 1.0;
				SetTouch(NULL);
				UTIL_Remove(this);
				return;
			}
		}
	}

	// higher pitch as squeeker gets closer to detonation time
	flpitch = 155.0 - 60.0 * ((m_flDie - gpGlobals->time) / SQUEEK_DETONATE_DELAY);

	if ( pOther->pev->takedamage && m_flNextAttack < gpGlobals->time )
	{
		// attack!

		// make sure it's me who has touched them
		if (tr.pHit == pOther->edict())
		{
			// and it's not another squeakgrenade
			if (tr.pHit->v.modelindex != pev->modelindex)
			{
				// make bite sound
				EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, "chumtoad_hunt2.wav", 1.0, ATTN_NORM, 0, PITCH_NORM /*(int)flpitch*/);
				m_flNextAttack = gpGlobals->time + 0.5;
			}
		}
	}

	m_flNextHit = gpGlobals->time + 0.1;
	m_flNextHunt = gpGlobals->time;

	if ( g_pGameRules->IsMultiplayer() )
	{
		// in multiplayer, we limit how often snarks can make their bounce sounds to prevent overflows.
		if ( gpGlobals->time < m_flNextBounceSoundTime )
		{
			// too soon!
			return;
		}
	}

	if (!(pev->flags & FL_ONGROUND))
	{
		// play bounce sound
		float flRndSound = RANDOM_FLOAT ( 0 , 1 );

		if ( flRndSound <= 0.33 )
			EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "chumtoad_hunt1.wav", 1, ATTN_NORM, 0, PITCH_NORM/*(int)flpitch*/);		
		else if (flRndSound <= 0.66)
			EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "chumtoad_hunt2.wav", 1, ATTN_NORM, 0, PITCH_NORM /*(int)flpitch*/);
		else 
			EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "chumtoad_hunt3.wav", 1, ATTN_NORM, 0, PITCH_NORM /*(int)flpitch*/);
		CSoundEnt::InsertSound ( bits_SOUND_COMBAT, pev->origin, 256, 0.25 );
	}
	else
	{
		// skittering sound
		CSoundEnt::InsertSound ( bits_SOUND_COMBAT, pev->origin, 100, 0.1 );
	}

	m_flNextBounceSoundTime = gpGlobals->time + 0.5;// half second.
}

#endif

// ======

#ifndef CLIENT_DLL

class CChumtoadGrenade : public CGrenade
{
	void Spawn( void );
	void Precache( void );
	int  Classify( void );
	void EXPORT SuperBounceTouch( CBaseEntity *pOther );
	void EXPORT HuntThink( void );
	void Killed( entvars_t *pevAttacker, int iGib );
	void GibMonster( void );

	virtual int		Save( CSave &save ); 
	virtual int		Restore( CRestore &restore );
	
	static	TYPEDESCRIPTION m_SaveData[];

	static float m_flNextBounceSoundTime;

	float m_flDie;
	Vector m_vecTarget;
	float m_flNextHunt;
	float m_flNextHit;
	Vector m_posPrev;
	EHANDLE m_hOwner;
	int  m_iMyClass;
};

float CChumtoadGrenade::m_flNextBounceSoundTime = 0;

LINK_ENTITY_TO_CLASS( monster_chumtoad, CChumtoadGrenade );
TYPEDESCRIPTION	CChumtoadGrenade::m_SaveData[] = 
{
	DEFINE_FIELD( CChumtoadGrenade, m_flDie, FIELD_TIME ),
	DEFINE_FIELD( CChumtoadGrenade, m_vecTarget, FIELD_VECTOR ),
	DEFINE_FIELD( CChumtoadGrenade, m_flNextHunt, FIELD_TIME ),
	DEFINE_FIELD( CChumtoadGrenade, m_flNextHit, FIELD_TIME ),
	DEFINE_FIELD( CChumtoadGrenade, m_posPrev, FIELD_POSITION_VECTOR ),
	DEFINE_FIELD( CChumtoadGrenade, m_hOwner, FIELD_EHANDLE ),
};

IMPLEMENT_SAVERESTORE( CChumtoadGrenade, CGrenade );

#define SQUEEK_DETONATE_DELAY	15.0

int CChumtoadGrenade :: Classify ( void )
{
	if (m_iMyClass != 0)
		return m_iMyClass; // protect against recursion

	if (m_hEnemy != NULL)
	{
		m_iMyClass = CLASS_INSECT; // no one cares about it
		switch( m_hEnemy->Classify( ) )
		{
			case CLASS_PLAYER:
			case CLASS_HUMAN_PASSIVE:
			case CLASS_HUMAN_MILITARY:
				m_iMyClass = 0;
				return CLASS_ALIEN_MILITARY; // barney's get mad, grunts get mad at it
		}
		m_iMyClass = 0;
	}

	return CLASS_ALIEN_BIOWEAPON;
}

void CChumtoadGrenade :: Spawn( void )
{
	Precache( );
	// motor
	pev->movetype = MOVETYPE_BOUNCE;
	pev->solid = SOLID_BBOX;

	SET_MODEL(ENT(pev), "models/w_chumtoad.mdl");
	UTIL_SetSize(pev, Vector( -4, -4, 0), Vector(4, 4, 8));
	UTIL_SetOrigin( pev, pev->origin );

	SetTouch( &CChumtoadGrenade::SuperBounceTouch );
	SetThink( &CChumtoadGrenade::HuntThink );
	pev->nextthink = gpGlobals->time + 0.1;
	m_flNextHunt = gpGlobals->time + 1E6;

	pev->flags |= FL_MONSTER;
	pev->takedamage		= DAMAGE_AIM;
	pev->health			= gSkillData.chumtoadHealth;
	pev->gravity		= 0.5;
	pev->friction		= 0.5;

	pev->dmg = gSkillData.chumtoadDmgPop;

	m_flDie = gpGlobals->time + SQUEEK_DETONATE_DELAY;

	m_flFieldOfView = 0; // 180 degrees

	if ( pev->owner )
		m_hOwner = Instance( pev->owner );

	m_flNextBounceSoundTime = gpGlobals->time;// reset each time a snark is spawned.

	pev->sequence = WCHUMTOAD_RUN;
	ResetSequenceInfo( );
}

void CChumtoadGrenade::Precache( void )
{
	PRECACHE_MODEL("models/w_chumtoad.mdl");
	PRECACHE_SOUND("chumtoad_blast1.wav");
	PRECACHE_SOUND("common/bodysplat.wav");
	PRECACHE_SOUND("squeek/sqk_die1.wav");
	PRECACHE_SOUND("chumtoad_hunt1.wav");
	PRECACHE_SOUND("chumtoad_hunt2.wav");
	PRECACHE_SOUND("chumtoad_hunt3.wav");
}

void CChumtoadGrenade :: Killed( entvars_t *pevAttacker, int iGib )
{
	pev->model = iStringNull;// make invisible
	SetThink( &CChumtoadGrenade::SUB_Remove );
	SetTouch( NULL );
	pev->nextthink = gpGlobals->time + 0.1;

	// since squeak grenades never leave a body behind, clear out their takedamage now.
	// Squeaks do a bit of radius damage when they pop, and that radius damage will
	// continue to call this function unless we acknowledge the Squeak's death now. (sjb)
	pev->takedamage = DAMAGE_NO;

	// play squeek blast
	EMIT_SOUND_DYN(ENT(pev), CHAN_ITEM, "chumtoad_blast1.wav", 1, 0.5, 0, PITCH_NORM);	

	CSoundEnt::InsertSound ( bits_SOUND_COMBAT, pev->origin, SMALL_EXPLOSION_VOLUME, 3.0 );

	UTIL_BloodDrips( pev->origin, g_vecZero, iceblood.value ? BLOOD_COLOR_BLUE : BLOOD_COLOR_GREEN, 80 );

	if (m_hOwner != NULL)
		RadiusDamage ( pev, m_hOwner->pev, pev->dmg, CLASS_NONE, DMG_BLAST );
	else
		RadiusDamage ( pev, pev, pev->dmg, CLASS_NONE, DMG_BLAST );

	// reset owner so death message happens
	if (m_hOwner != NULL)
		pev->owner = m_hOwner->edict();

	CBaseMonster :: Killed( pevAttacker, GIB_ALWAYS );
}

void CChumtoadGrenade :: GibMonster( void )
{
	EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "common/bodysplat.wav", 0.75, ATTN_NORM, 0, 200);		
}

void CChumtoadGrenade::HuntThink( void )
{
	// ALERT( at_console, "think\n" );

	if (!IsInWorld())
	{
		SetTouch( NULL );
		UTIL_Remove( this );
		return;
	}
	
	StudioFrameAdvance( );
	pev->nextthink = gpGlobals->time + 0.1;

	// explode when ready
	if (gpGlobals->time >= m_flDie)
	{
		g_vecAttackDir = pev->velocity.Normalize( );
		pev->health = -1;
		Killed( pev, 0 );
		return;
	}

	// float
	if (pev->waterlevel != 0)
	{
		if (pev->movetype == MOVETYPE_BOUNCE)
		{
			pev->movetype = MOVETYPE_FLY;
		}
		pev->velocity = pev->velocity * 0.9;
		pev->velocity.z += 8.0;
	}
	else if (pev->movetype = MOVETYPE_FLY)
	{
		pev->movetype = MOVETYPE_BOUNCE;
	}

	// return if not time to hunt
	if (m_flNextHunt > gpGlobals->time)
		return;

	m_flNextHunt = gpGlobals->time + 2.0;
	
	CBaseEntity *pOther = NULL;
	Vector vecDir;
	TraceResult tr;

	Vector vecFlat = pev->velocity;
	vecFlat.z = 0;
	vecFlat = vecFlat.Normalize( );

	UTIL_MakeVectors( pev->angles );

	if (m_hEnemy == NULL || !m_hEnemy->IsAlive())
	{
		// find target, bounce a bit towards it.
		Look( 512 );
		m_hEnemy = BestVisibleEnemy( );
	}

	// squeek if it's about time blow up
	if ((m_flDie - gpGlobals->time <= 0.5) && (m_flDie - gpGlobals->time >= 0.3))
	{
		EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "squeek/sqk_die1.wav", 1, ATTN_NORM, 0, 100 + RANDOM_LONG(0,0x3F));
		CSoundEnt::InsertSound ( bits_SOUND_COMBAT, pev->origin, 256, 0.25 );
	}

	// higher pitch as squeeker gets closer to detonation time
	float flpitch = 155.0 - 60.0 * ((m_flDie - gpGlobals->time) / SQUEEK_DETONATE_DELAY);
	if (flpitch < 80)
		flpitch = 80;

	if (m_hEnemy != NULL)
	{
		if (FVisible( m_hEnemy ))
		{
			vecDir = m_hEnemy->EyePosition() - pev->origin;
			m_vecTarget = vecDir.Normalize( );
		}

		float flVel = pev->velocity.Length();
		float flAdj = 50.0 / (flVel + 10.0);

		if (flAdj > 1.2)
			flAdj = 1.2;
		
		// ALERT( at_console, "think : enemy\n");

		// ALERT( at_console, "%.0f %.2f %.2f %.2f\n", flVel, m_vecTarget.x, m_vecTarget.y, m_vecTarget.z );

		pev->velocity = pev->velocity * flAdj + m_vecTarget * 300;
	}

	if (pev->flags & FL_ONGROUND)
	{
		pev->avelocity = Vector( 0, 0, 0 );
	}
	else
	{
		if (pev->avelocity == Vector( 0, 0, 0))
		{
			pev->avelocity.x = RANDOM_FLOAT( -100, 100 );
			pev->avelocity.z = RANDOM_FLOAT( -100, 100 );
		}
	}

	if ((pev->origin - m_posPrev).Length() < 1.0)
	{
		pev->velocity.x = RANDOM_FLOAT( -100, 100 );
		pev->velocity.y = RANDOM_FLOAT( -100, 100 );
	}
	m_posPrev = pev->origin;

	pev->angles = UTIL_VecToAngles( pev->velocity );
	pev->angles.z = 0;
	pev->angles.x = 0;
}

void CChumtoadGrenade::SuperBounceTouch( CBaseEntity *pOther )
{
	float	flpitch;

	TraceResult tr = UTIL_GetGlobalTrace( );

	// don't hit the guy that launched this grenade
	if ( pev->owner && pOther->edict() == pev->owner )
		return;

	// at least until we've bounced once
	pev->owner = NULL;

	pev->angles.x = 0;
	pev->angles.z = 0;

	// avoid bouncing too much
	if (m_flNextHit > gpGlobals->time)
		return;

	// higher pitch as squeeker gets closer to detonation time
	flpitch = 155.0 - 60.0 * ((m_flDie - gpGlobals->time) / SQUEEK_DETONATE_DELAY);

	if ( pOther->pev->takedamage && m_flNextAttack < gpGlobals->time )
	{
		// attack!

		// make sure it's me who has touched them
		if (tr.pHit == pOther->edict())
		{
			// and it's not another squeakgrenade
			if (tr.pHit->v.modelindex != pev->modelindex)
			{
				// ALERT( at_console, "hit enemy\n");
				ClearMultiDamage( );
				pOther->TraceAttack(pev, gSkillData.chumtoadDmgBite, gpGlobals->v_forward, &tr, DMG_SLASH ); 
				if (m_hOwner != NULL)
					ApplyMultiDamage( pev, m_hOwner->pev );
				else
					ApplyMultiDamage( pev, pev );

				pev->dmg += gSkillData.chumtoadDmgPop; // add more explosion damage
				// m_flDie += 2.0; // add more life

				// make bite sound
				EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, "chumtoad_hunt2.wav", 1.0, ATTN_NORM, 0, (int)flpitch);
				m_flNextAttack = gpGlobals->time + 0.5;
			}
		}
		else
		{
			// ALERT( at_console, "been hit\n");
		}
	}

	m_flNextHit = gpGlobals->time + 0.1;
	m_flNextHunt = gpGlobals->time;

	if ( g_pGameRules->IsMultiplayer() )
	{
		// in multiplayer, we limit how often snarks can make their bounce sounds to prevent overflows.
		if ( gpGlobals->time < m_flNextBounceSoundTime )
		{
			// too soon!
			return;
		}
	}

	if (!(pev->flags & FL_ONGROUND))
	{
		// play bounce sound
		float flRndSound = RANDOM_FLOAT ( 0 , 1 );

		if ( flRndSound <= 0.33 )
			EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "chumtoad_hunt1.wav", 1, ATTN_NORM, 0, (int)flpitch);		
		else if (flRndSound <= 0.66)
			EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "chumtoad_hunt2.wav", 1, ATTN_NORM, 0, (int)flpitch);
		else 
			EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "chumtoad_hunt3.wav", 1, ATTN_NORM, 0, (int)flpitch);
		CSoundEnt::InsertSound ( bits_SOUND_COMBAT, pev->origin, 256, 0.25 );
	}
	else
	{
		// skittering sound
		CSoundEnt::InsertSound ( bits_SOUND_COMBAT, pev->origin, 100, 0.1 );
	}

	m_flNextBounceSoundTime = gpGlobals->time + 0.5;// half second.
}

#endif

#ifdef CHUMTOAD
LINK_ENTITY_TO_CLASS( weapon_chumtoad, CChumtoad );
#endif

void CChumtoad::Spawn( )
{
	Precache( );
	m_iId = WEAPON_CHUMTOAD;
	SET_MODEL(ENT(pev), "models/w_weapons.mdl");
	pev->body = WEAPON_CHUMTOAD - 1;

	FallInit();//get ready to fall down.

#ifndef CLIENT_DLL
	m_iDefaultAmmo = g_pGameRules->IsCtC() ? 1 : SNARK_DEFAULT_GIVE;
#endif

	pev->animtime = gpGlobals->time;
	pev->framerate = 1.0;
}

void CChumtoad::Precache( void )
{
	PRECACHE_MODEL("models/w_weapons.mdl");
	PRECACHE_MODEL("models/v_chumtoad.mdl");
	PRECACHE_MODEL("models/p_weapons.mdl");
	PRECACHE_SOUND("chumtoad_hunt2.wav");
	PRECACHE_SOUND("chumtoad_hunt3.wav");
	UTIL_PrecacheOther("monster_chumtoad");
	UTIL_PrecacheOther("monster_ctctoad");

	m_usChumtoadFire = PRECACHE_EVENT ( 1, "events/chumtoadfire.sc" );
	m_usChumtoadRelease = PRECACHE_EVENT ( 1, "events/chumtoadrelease.sc" );
}

int CChumtoad::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "Chumtoads";
#ifndef CLIENT_DLL
	p->iMaxAmmo1 = g_pGameRules->IsCtC() ? 1 : SNARK_MAX_CARRY;
#endif
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = WEAPON_NOCLIP;
	p->iSlot = 4;
	p->iPosition = 5;
	p->iId = m_iId = WEAPON_CHUMTOAD;
	p->iWeight = SNARK_WEIGHT;
	p->iFlags = ITEM_FLAG_LIMITINWORLD | ITEM_FLAG_EXHAUSTIBLE;
	p->pszDisplayName = "Chumtoad";

	return 1;
}

BOOL CChumtoad::DeployLowKey( )
{
	m_pPlayer->m_iWeaponVolume = QUIET_GUN_VOLUME;
	return DefaultDeploy( "models/v_chumtoad.mdl", "models/p_weapons.mdl", CHUMTOAD_DRAW_LOWKEY, "squeak" );
}

BOOL CChumtoad::Deploy( )
{
	// play hunt sound
	float flRndSound = RANDOM_FLOAT ( 0 , 1 );

	if ( flRndSound <= 0.5 )
		EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "chumtoad_hunt2.wav", 1, ATTN_NORM, 0, 100);
	else 
		EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "chumtoad_hunt3.wav", 1, ATTN_NORM, 0, 100);

	m_pPlayer->m_iWeaponVolume = QUIET_GUN_VOLUME;

	return DefaultDeploy( "models/v_chumtoad.mdl", "models/p_weapons.mdl", CHUMTOAD_DRAW, "squeak" );
}

BOOL CChumtoad::CanHolster( void )
{
	if ( m_pPlayer && m_pPlayer->m_iHoldingChumtoad )
	{
		return FALSE;
	}

	return TRUE;
}

void CChumtoad::Holster( int skiplocal /* = 0 */ )
{
	CBasePlayerWeapon::DefaultHolster(-1);
	
	if ( !m_pPlayer->m_rgAmmo[ m_iPrimaryAmmoType ] )
	{
		m_pPlayer->m_iWeapons2 &= ~(1<<(WEAPON_CHUMTOAD - 32));
		SetThink( &CChumtoad::DestroyItem );
		pev->nextthink = gpGlobals->time + 0.1;
		return;
	}
	
	SendWeaponAnim( CHUMTOAD_DOWN );
	EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_WEAPON, "common/null.wav", 1.0, ATTN_NORM);
}

void CChumtoad::PrimaryAttack()
{
	if ( m_pPlayer->m_rgAmmo[ m_iPrimaryAmmoType ] )
	{
		Vector vecAiming = m_pPlayer->GetAutoaimVector( AUTOAIM_10DEGREES );
		TraceResult tr;
		Vector trace_origin;

		// HACK HACK:  Ugly hacks to handle change in origin based on new physics code for players
		// Move origin up if crouched and start trace a bit outside of body ( 20 units instead of 16 )
		trace_origin = m_pPlayer->pev->origin;
		if ( m_pPlayer->pev->flags & FL_DUCKING )
		{
			trace_origin = trace_origin - ( VEC_HULL_MIN - VEC_DUCK_HULL_MIN );
		}

		// find place to toss monster
		UTIL_TraceLine( trace_origin + vecAiming * 20, trace_origin + vecAiming * 64, dont_ignore_monsters, NULL, &tr );

	int flags;
#ifdef CLIENT_WEAPONS
	flags = FEV_NOTHOST;
#else
	flags = 0;
#endif

	    PLAYBACK_EVENT_FULL( flags, m_pPlayer->edict(), m_usChumtoadFire, 0.0, (float *)&g_vecZero, (float *)&g_vecZero, 0.0, 0.0, 0, 0, 0, 0 );

		if ( tr.fAllSolid == 0 && tr.fStartSolid == 0 && tr.flFraction > 0.25 )
		{
			// player "shoot" animation
			m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

#ifndef CLIENT_DLL
			CBaseEntity *pChumtoad = NULL;
			if (g_pGameRules->IsCtC())
			{
				m_pPlayer->m_iHoldingChumtoad = FALSE;
				pChumtoad = g_pGameRules->DropCharm(m_pPlayer, tr.vecEndPos);
			}
			else
				pChumtoad = CBaseEntity::Create( "monster_chumtoad", tr.vecEndPos, vecAiming, m_pPlayer->edict() );

			if (pChumtoad)
				pChumtoad->pev->velocity = vecAiming * 200 + m_pPlayer->pev->velocity;
#endif

			// play hunt sound
			float flRndSound = RANDOM_FLOAT ( 0 , 1 );

			if ( flRndSound <= 0.5 )
				EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "chumtoad_hunt2.wav", 1, ATTN_NORM, 0, 105);
			else 
				EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "chumtoad_hunt3.wav", 1, ATTN_NORM, 0, 105);

			m_pPlayer->m_iWeaponVolume = QUIET_GUN_VOLUME;

			m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType]--;

			m_fJustThrown = 1;

			m_flNextPrimaryAttack = m_flNextSecondaryAttack = GetNextAttackDelay(0.3);
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 1.0;
		}
	}
}

void CChumtoad::SecondaryAttack()
{
	if ( m_pPlayer->m_rgAmmo[ m_iPrimaryAmmoType ] )
	{
		Vector vecAiming = m_pPlayer->GetAutoaimVector( AUTOAIM_10DEGREES );
		TraceResult tr;
		Vector trace_origin;

		// HACK HACK:  Ugly hacks to handle change in origin based on new physics code for players
		// Move origin up if crouched and start trace a bit outside of body ( 20 units instead of 16 )
		trace_origin = m_pPlayer->pev->origin;
		if ( m_pPlayer->pev->flags & FL_DUCKING )
		{
			trace_origin = trace_origin - ( VEC_HULL_MIN - VEC_DUCK_HULL_MIN );
		}

		// find place to toss monster
		UTIL_TraceLine( trace_origin + vecAiming * 20, trace_origin + vecAiming * 64, dont_ignore_monsters, NULL, &tr );

	int flags;
#ifdef CLIENT_WEAPONS
	flags = FEV_NOTHOST;
#else
	flags = 0;
#endif

	    PLAYBACK_EVENT_FULL( flags, m_pPlayer->edict(), m_usChumtoadRelease, 0.0, (float *)&g_vecZero, (float *)&g_vecZero, 0.0, 0.0, 0, 0, 0, 0 );

		if ( tr.fAllSolid == 0 && tr.fStartSolid == 0 && tr.flFraction > 0.25 )
		{
			// player "shoot" animation
			m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

#ifndef CLIENT_DLL
			int dif = m_pPlayer->m_rgAmmo[ m_iPrimaryAmmoType ] * -10;
			for (int i = 0; i < m_pPlayer->m_rgAmmo[ m_iPrimaryAmmoType ]; i++) {
				CBaseEntity *pChumtoad = NULL;
				if (g_pGameRules->IsCtC())
				{
					m_pPlayer->m_iHoldingChumtoad = FALSE;
					pChumtoad = g_pGameRules->DropCharm(m_pPlayer, tr.vecEndPos + (gpGlobals->v_right * ((20 * i) + dif)));
				}
				else
					pChumtoad = CBaseEntity::Create( "monster_chumtoad", tr.vecEndPos + (gpGlobals->v_right * ((20 * i) + dif)), vecAiming, m_pPlayer->edict() );

				if (pChumtoad)
					pChumtoad->pev->velocity = vecAiming * 200 + m_pPlayer->pev->velocity;
			}
#endif

			// play hunt sound
			float flRndSound = RANDOM_FLOAT ( 0 , 1 );

			if ( flRndSound <= 0.5 )
				EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "chumtoad_hunt2.wav", 1, ATTN_NORM, 0, 105);
			else 
				EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "chumtoad_hunt3.wav", 1, ATTN_NORM, 0, 105);

			m_pPlayer->m_iWeaponVolume = QUIET_GUN_VOLUME;

			m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] = 0;

			m_fJustThrown = 1;

			m_flNextPrimaryAttack = m_flNextSecondaryAttack = GetNextAttackDelay(0.3);
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 1.0;
		}
	}
}

void CChumtoad::WeaponIdle( void )
{
	m_pPlayer->GetAutoaimVector( AUTOAIM_10DEGREES );

	if ( m_flTimeWeaponIdle > UTIL_WeaponTimeBase() )
		return;

	if (m_fJustThrown)
	{
		m_fJustThrown = 0;

		if ( !m_pPlayer->m_rgAmmo[PrimaryAmmoIndex()] )
		{
			RetireWeapon();
			return;
		}

		SendWeaponAnim( CHUMTOAD_UP );
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );
		return;
	}

	int iAnim;
	float flRand = UTIL_SharedRandomFloat( m_pPlayer->random_seed, 0, 1 );
	if (flRand <= 0.75)
	{
		iAnim = CHUMTOAD_IDLE1;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 30.0 / 16 * (2);
	}
	else if (flRand <= 0.875)
	{
		iAnim = CHUMTOAD_FIDGETFIT;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 70.0 / 16.0;
	}
	else
	{
		iAnim = CHUMTOAD_FIDGETNIP;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 80.0 / 16.0;
	}
	SendWeaponAnim( iAnim );

#ifndef CLIENT_DLL
	if (g_pGameRules->IsCtC())
	{
		float flRndSound = RANDOM_FLOAT(0 , 1);

		if ( flRndSound <= 0.33 )
			EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "chumtoad_hunt1.wav", 1, ATTN_NORM, 0, PITCH_NORM);		
		else if (flRndSound <= 0.66)
			EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "chumtoad_hunt2.wav", 1, ATTN_NORM, 0, PITCH_NORM);
		else 
			EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "chumtoad_hunt3.wav", 1, ATTN_NORM, 0, PITCH_NORM);
	}
#endif
}
