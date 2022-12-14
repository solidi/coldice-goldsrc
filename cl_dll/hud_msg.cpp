/***
*
*	Copyright (c) 1999, Valve LLC. All rights reserved.
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
//
//  hud_msg.cpp
//

#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"
#include "r_efx.h"
#include "view.h"
#include "in_defs.h"

#include "particleman.h"
extern IParticleMan *g_pParticleMan;

#define MAX_CLIENTS 32

#if !defined( _TFC )
extern BEAM *pBeam;
extern BEAM *pBeam2;
#endif 

#if defined( _TFC )
void ClearEventList( void );
#endif

float g_SlideTime = 0;
float g_AcrobatTime = 0;
extern cvar_t *cl_antivomit;

/// USER-DEFINED SERVER MESSAGE HANDLERS

int CHud :: MsgFunc_ResetHUD(const char *pszName, int iSize, void *pbuf )
{
	ASSERT( iSize == 0 );

	// clear all hud data
	HUDLIST *pList = m_pHudList;

	while ( pList )
	{
		if ( pList->p )
			pList->p->Reset();
		pList = pList->pNext;
	}

	// reset sensitivity
	m_flMouseSensitivity = 0;

	// reset concussion effect
	m_iConcussionEffect = 0;

	return 1;
}

void CAM_ToFirstPerson(void);

void CHud :: MsgFunc_ViewMode( const char *pszName, int iSize, void *pbuf )
{
	CAM_ToFirstPerson();
}

void CHud :: MsgFunc_InitHUD( const char *pszName, int iSize, void *pbuf )
{
	// prepare all hud data
	HUDLIST *pList = m_pHudList;

	while (pList)
	{
		if ( pList->p )
			pList->p->InitHUDData();
		pList = pList->pNext;
	}

#if defined( _TFC )
	ClearEventList();

	// catch up on any building events that are going on
	gEngfuncs.pfnServerCmd("sendevents");
#endif

	if ( g_pParticleMan )
		 g_pParticleMan->ResetParticles();

#if !defined( _TFC )
	//Probably not a good place to put this.
	pBeam = pBeam2 = NULL;
#endif
}


int CHud :: MsgFunc_GameMode(const char *pszName, int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );
	m_Teamplay = READ_BYTE();

	return 1;
}


int CHud :: MsgFunc_Damage(const char *pszName, int iSize, void *pbuf )
{
	int		armor, blood;
	Vector	from;
	int		i;
	float	count;
	
	BEGIN_READ( pbuf, iSize );
	armor = READ_BYTE();
	blood = READ_BYTE();

	for (i=0 ; i<3 ; i++)
		from[i] = READ_COORD();

	count = (blood * 0.5) + (armor * 0.5);

	if (count < 10)
		count = 10;

	// TODO: kick viewangles,  show damage visually

	return 1;
}

int CHud :: MsgFunc_Concuss( const char *pszName, int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );
	m_iConcussionEffect = READ_BYTE();
	if (m_iConcussionEffect)
		this->m_StatusIcons.EnableIcon("dmg_concuss",255,160,0);
	else
		this->m_StatusIcons.DisableIcon("dmg_concuss");
	return 1;
}

int CHud :: MsgFunc_Acrobatics(const char *pszName, int iSize, void *pbuf )
{
	// Unstick slide after levelchange
	if ((gEngfuncs.GetClientTime() + 2) < g_SlideTime) {
		g_SlideTime = 0;
	}

	if ((gEngfuncs.GetClientTime() + 2) < g_AcrobatTime) {
		g_AcrobatTime = 0;
	}

	BEGIN_READ( pbuf, iSize );
	int mode = READ_BYTE();
	int axis = 0, amount = 0;
	switch (mode)
	{
	case ACROBATICS_ROLL_RIGHT:
		axis = ROLL;
		amount = -360;
		break;
	case ACROBATICS_ROLL_LEFT:
		axis = ROLL;
		amount = 360;
		break;
	case ACROBATICS_FLIP_BACK:
		axis = PITCH;
		amount = 360;
		break;
	default:
		g_SlideTime = gEngfuncs.GetClientTime() + 1;
	}

	if (amount > 0 || amount < 0) {
		if (!cl_antivomit->value)
			V_PunchAxis(axis, amount);
		g_AcrobatTime = gEngfuncs.GetClientTime() + 1;
	}

	return 1;
}

int CHud :: MsgFunc_PlayCSound( const char *pszName, int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );
	int index = READ_BYTE();
	switch (index)
	{
		case CLIENT_SOUND_PREPAREFORBATTLE:
			PlaySound("prepareforbattle.wav", 1);
			break;
		case CLIENT_SOUND_OUTSTANDING:
			PlaySound("outstanding.wav", 1);
			break;
		case CLIENT_SOUND_HULIMATING_DEAFEAT:
			PlaySound("humiliatingdefeat.wav", 1);
			break;
		case CLIENT_SOUND_MASSACRE:
			PlaySound("massacre.wav", 1);
			break;
		case CLIENT_SOUND_KILLINGMACHINE:
			PlaySound("killingmachine.wav", 1);
			break;
		case CLIENT_SOUND_PICKUPYOURWEAPON:
			PlaySound("pickupyourweaponsandfight.wav", 1);
			break;
		case CLIENT_SOUND_LMS:
			PlaySound("youarethelastmanstanding.wav", 1);
			break;
		case CLIENT_SOUND_PREPARETOFIGHT:
			PlaySound("preparetofight.wav", 1);
			break;
	}
	return 1;
}

int CHud :: MsgFunc_Mutators( const char *pszName, int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );
	strncpy( gHUD.szActiveMutators, READ_STRING(), 64 );
	return 1;
}
