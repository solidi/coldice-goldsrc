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
#include "player.h"
#include "weapons.h"
#include "gamerules.h"
#include "jvs_gamerules.h"
#include "game.h"
#include "items.h"
#include "voice_gamemgr.h"

extern int gmsgPlayClientSound;
extern int gmsgStatusIcon;
extern int gmsgTeamInfo;
extern int gmsgTeamNames;
extern int gmsgScoreInfo;
extern int gmsgObjective;
extern int gmsgShowTimer;
extern int gmsgRoundTime;

CHalfLifeJesusVsSanta::CHalfLifeJesusVsSanta()
{
	// Nothing
}

void CHalfLifeJesusVsSanta::Think( void )
{
	CHalfLifeMultiplay::Think();

	if ( flUpdateTime > gpGlobals->time )
		return;

	if ( m_flRoundTimeLimit )
	{
		if ( CheckGameTimer() )
			return;
	}

	if ( g_GameInProgress )
	{
		int clients_alive = 0;

		for ( int i = 1; i <= gpGlobals->maxClients; i++ )
		{
			CBasePlayer *plr = (CBasePlayer *)UTIL_PlayerByIndex( i );

			if ( plr && plr->IsPlayer() && !plr->HasDisconnected )
			{
				// Force spectate on those that died.
				if ( plr->m_flForceToObserverTime && plr->m_flForceToObserverTime < gpGlobals->time )
				{
					edict_t *pentSpawnSpot = g_pGameRules->GetPlayerSpawnSpot( plr );
					plr->StartObserver(plr->pev->origin, VARS(pentSpawnSpot)->angles);
					plr->m_flForceToObserverTime = 0;
				}

				if ( plr->IsInArena && plr->IsAlive() )
				{
					clients_alive++;
				}
				else
				{
					//for clients who connected while game in progress.
					if ( plr->IsSpectator() )
					{
						MESSAGE_BEGIN(MSG_ONE, gmsgObjective, NULL, plr->edict());
							WRITE_STRING("Jesus vs Santa in progress");
							WRITE_STRING(UTIL_VarArgs("Jesus: %s (%.0f/%.0f)\n",
								STRING(pArmoredMan->pev->netname),
								pArmoredMan->pev->health,
								pArmoredMan->pev->armorvalue ));
							WRITE_BYTE((pArmoredMan->pev->health / pArmoredMan->pev->max_health) * 100);
						MESSAGE_END();
					} else {
						// Send them to observer
						plr->m_flForceToObserverTime = gpGlobals->time;
					}
				}
			}
		}

		for ( int i = 1; i <= gpGlobals->maxClients; i++ )
		{
			CBasePlayer *plr = (CBasePlayer *)UTIL_PlayerByIndex( i );
			if ( plr && plr->IsPlayer() && !plr->HasDisconnected && !FBitSet(plr->pev->flags, FL_FAKECLIENT))
			{
				if (pArmoredMan && plr == pArmoredMan)
				{
					if ((clients_alive - 1) >= 1)
					{
						MESSAGE_BEGIN(MSG_ONE, gmsgObjective, NULL, plr->edict());
							WRITE_STRING("Defeat all Santas");
							WRITE_STRING(UTIL_VarArgs("Santas alive: %d", clients_alive - 1));
							WRITE_BYTE(float(clients_alive - 1) / (m_iPlayersInGame - 1) * 100);
						MESSAGE_END();
					}
				}
				else
				{
					if ((clients_alive - 1) >= 1)
					{
						MESSAGE_BEGIN(MSG_ONE, gmsgObjective, NULL, plr->edict());
							WRITE_STRING(UTIL_VarArgs("Defeat %s as Jesus", STRING(pArmoredMan->pev->netname)));
							WRITE_STRING(UTIL_VarArgs("Santas remain: %d", clients_alive - 1));
							WRITE_BYTE(float(clients_alive - 1) / (m_iPlayersInGame - 1) * 100);
						MESSAGE_END();
					}
				}
			}
		}

		if (m_fSendArmoredManMessage < gpGlobals->time)
		{
			if (!FBitSet(pArmoredMan->pev->flags, FL_FAKECLIENT))
			{
				MESSAGE_BEGIN( MSG_ONE, gmsgStatusIcon, NULL, pArmoredMan->pev );
					WRITE_BYTE(1);
					WRITE_STRING("jesus");
					WRITE_BYTE(0);
					WRITE_BYTE(160);
					WRITE_BYTE(255);
				MESSAGE_END();
			}
		}

		//santas all dead or armored man defeated.
		if ( clients_alive <= 1 || !pArmoredMan->IsAlive() || pArmoredMan->HasDisconnected )
		{
			//stop timer / end game.
			m_flRoundTimeLimit = 0;
			g_GameInProgress = FALSE;
			MESSAGE_BEGIN(MSG_ALL, gmsgShowTimer);
				WRITE_BYTE(0);
			MESSAGE_END();

			//hack to allow for logical code below.
			if ( pArmoredMan->HasDisconnected )
				pArmoredMan->pev->health = 0;

			if (!FBitSet(pArmoredMan->pev->flags, FL_FAKECLIENT))
			{
				MESSAGE_BEGIN( MSG_ONE, gmsgStatusIcon, NULL, pArmoredMan->pev );
					WRITE_BYTE(0);
					WRITE_STRING("jesus");
				MESSAGE_END();
			}

			//armored man is alive.
			if ( pArmoredMan->IsAlive() && clients_alive == 1 )
			{
				UTIL_ClientPrintAll(HUD_PRINTCENTER, UTIL_VarArgs("%s has defeated all Santas!\n", STRING(pArmoredMan->pev->netname) ));

				MESSAGE_BEGIN(MSG_ALL, gmsgObjective);
					WRITE_STRING("Jesus has saved!");
					WRITE_STRING("All Santas Defeated!");
					WRITE_BYTE(0);
				MESSAGE_END();

				CheckRounds();

				DisplayWinnersGoods( pArmoredMan );
				MESSAGE_BEGIN( MSG_BROADCAST, gmsgPlayClientSound );
					WRITE_BYTE(CLIENT_SOUND_KILLINGMACHINE);
				MESSAGE_END();
			}
			//the man has been killed.
			else if ( !pArmoredMan->IsAlive() )
			{
				MESSAGE_BEGIN(MSG_ALL, gmsgObjective);
					WRITE_STRING("Santas win!");
					WRITE_STRING("Time to buy presents.");
					WRITE_BYTE(0);
				MESSAGE_END();

				//find highest damage amount.
				float highest = 1;
				BOOL IsEqual = FALSE;
				CBasePlayer *highballer = NULL;

				for ( int i = 1; i <= gpGlobals->maxClients; i++ )
				{
					CBasePlayer *plr = (CBasePlayer *)UTIL_PlayerByIndex( i );

					if ( plr && plr->IsPlayer() && plr->IsInArena )
					{
						if ( highest <= plr->m_fArmoredManHits )
						{
							if ( highest == plr->m_fArmoredManHits )
							{
								IsEqual = TRUE;
								break;
							}

							highest = plr->m_fArmoredManHits;
							highballer = plr;
						}
					}
				}

				if ( !IsEqual && highballer )
				{
					CheckRounds();
					UTIL_ClientPrintAll(HUD_PRINTCENTER,
						UTIL_VarArgs("Jesus has been destroyed!\n\n%s doled the most damage!\n",
						STRING(highballer->pev->netname)));
					DisplayWinnersGoods( highballer );
					MESSAGE_BEGIN( MSG_BROADCAST, gmsgPlayClientSound );
						WRITE_BYTE(CLIENT_SOUND_OUTSTANDING);
					MESSAGE_END();
				}
				else
				{
					UTIL_ClientPrintAll(HUD_PRINTCENTER, "Jesus has been destroyed!\n");
					UTIL_ClientPrintAll(HUD_PRINTTALK, "* Round ends in a tie!");
					MESSAGE_BEGIN( MSG_BROADCAST, gmsgPlayClientSound );
						WRITE_BYTE(CLIENT_SOUND_HULIMATING_DEAFEAT);
					MESSAGE_END();
				}

			}
			//everyone died.
			else
			{
				UTIL_ClientPrintAll(HUD_PRINTCENTER, "Everyone has been killed!\n");
				UTIL_ClientPrintAll(HUD_PRINTTALK, "* No winners in this round!");
				MESSAGE_BEGIN( MSG_BROADCAST, gmsgPlayClientSound );
					WRITE_BYTE(CLIENT_SOUND_HULIMATING_DEAFEAT);
				MESSAGE_END();
			}

			flUpdateTime = gpGlobals->time + 5.0;
			return;
		}

		flUpdateTime = gpGlobals->time + 3.0;
		return;
	}

	int clients = CheckClients();

	if ( clients > 1 )
	{
		if ( m_iCountDown > 0 )
		{
			if (m_iCountDown == 2) {
				MESSAGE_BEGIN( MSG_BROADCAST, gmsgPlayClientSound );
					WRITE_BYTE(CLIENT_SOUND_PREPAREFORBATTLE);
				MESSAGE_END();
			}
			SuckAllToSpectator(); // in case players join during a countdown.
			UTIL_ClientPrintAll(HUD_PRINTCENTER,
				UTIL_VarArgs("Prepare for Jesus vs Santa\n\n%i...\n", m_iCountDown));
			m_iCountDown--;
			flUpdateTime = gpGlobals->time + 1.0;
			return;
		}

		ALERT(at_console, "Players in arena:\n");

		//frags + time.
		SetRoundLimits();

		int armoredman = m_iPlayersInArena[RANDOM_LONG(0, clients-1)];
		ALERT(at_console, "clients set to %d, armor man set to index=%d\n", clients, armoredman);
		pArmoredMan = (CBasePlayer *)UTIL_PlayerByIndex( armoredman );
		pArmoredMan->IsArmoredMan = TRUE;
		pArmoredMan->pev->fuser4 = 1;

		g_GameInProgress = TRUE;

		InsertClientsIntoArena();

		m_fSendArmoredManMessage = gpGlobals->time + 1.0;

		m_iCountDown = 3;

		if (roundtimelimit.value > 0)
		{
			MESSAGE_BEGIN(MSG_ALL, gmsgShowTimer);
				WRITE_BYTE(1);
			MESSAGE_END();

			MESSAGE_BEGIN(MSG_ALL, gmsgRoundTime);
				WRITE_SHORT(roundtimelimit.value * 60.0);
			MESSAGE_END();
		}

		// Resend team info
		MESSAGE_BEGIN( MSG_ALL, gmsgTeamNames );
			WRITE_BYTE( 2 );
			WRITE_STRING( "santa" );
			WRITE_STRING( "jesus" );
		MESSAGE_END();

		UTIL_ClientPrintAll(HUD_PRINTCENTER, UTIL_VarArgs("Jesus vs Santa has begun!\n%s is Jesus!\n", STRING(pArmoredMan->pev->netname)));
		UTIL_ClientPrintAll(HUD_PRINTTALK, UTIL_VarArgs("* %d players have entered the arena!\n", clients));
	}
	else
	{
		SuckAllToSpectator();
		m_flRoundTimeLimit = 0;
		MESSAGE_BEGIN(MSG_ALL, gmsgObjective);
			WRITE_STRING("Jesus vs Santa");
			WRITE_STRING("Waiting for other players");
			WRITE_BYTE(0);
			WRITE_STRING(UTIL_VarArgs("%d Rounds", (int)roundlimit.value));
		MESSAGE_END();
	}

	flUpdateTime = gpGlobals->time + 1.0;
}

void CHalfLifeJesusVsSanta::InitHUD( CBasePlayer *pPlayer )
{
	CHalfLifeMultiplay::InitHUD( pPlayer );

	if (!FBitSet(pPlayer->pev->flags, FL_FAKECLIENT))
	{
		MESSAGE_BEGIN(MSG_ONE, gmsgObjective, NULL, pPlayer->edict());
			WRITE_STRING("Jesus vs Santa");
			WRITE_STRING("");
			WRITE_BYTE(0);
		MESSAGE_END();

		MESSAGE_BEGIN( MSG_ONE, gmsgTeamNames, NULL, pPlayer->edict() );
			WRITE_BYTE( 2 );
			WRITE_STRING( "santa" );
			WRITE_STRING( "jesus" );
		MESSAGE_END();
	}
}

BOOL CHalfLifeJesusVsSanta::CheckGameTimer( void )
{
	g_engfuncs.pfnCvar_DirectSet(&roundtimeleft, UTIL_VarArgs( "%i", int(m_flRoundTimeLimit) - int(gpGlobals->time)));

	if ( !_30secwarning && (m_flRoundTimeLimit - 30) < gpGlobals->time )
	{
		UTIL_ClientPrintAll(HUD_PRINTTALK, "* 30 second warning...\n");
		_30secwarning = TRUE;
	}
	else if ( !_15secwarning && (m_flRoundTimeLimit - 15) < gpGlobals->time )
	{
		UTIL_ClientPrintAll(HUD_PRINTTALK, "* 15 second warning...\n");
		_15secwarning = TRUE;
	}
	else if ( !_3secwarning && (m_flRoundTimeLimit - 3) < gpGlobals->time )
	{
		UTIL_ClientPrintAll(HUD_PRINTTALK, "* 3 second warning...\n");
		_3secwarning = TRUE;
	}

//===================================================

	//time is up for this game.

//===================================================

	//time is up
	if ( m_flRoundTimeLimit < gpGlobals->time )
	{
		int highest = 1;
		BOOL IsEqual = FALSE;
		CBasePlayer *highballer = NULL;

		//find highest damage amount.
		for ( int i = 1; i <= gpGlobals->maxClients; i++ )
		{
			CBasePlayer *plr = (CBasePlayer *)UTIL_PlayerByIndex( i );

			if ( plr && plr->IsPlayer() && plr->IsInArena )
			{
				if ( highest <= plr->m_fArmoredManHits )
				{
					if ( highest == plr->m_fArmoredManHits )
					{
						IsEqual = TRUE;
						break;
					}

					highest = plr->m_fArmoredManHits;
					highballer = plr;
				}
			}
		}

		if ( !IsEqual && highballer )
		{
			CheckRounds();
			UTIL_ClientPrintAll(HUD_PRINTCENTER, 
				UTIL_VarArgs("Time is up!\n\n%s doled the most damage!\n",
				STRING(highballer->pev->netname)));
			DisplayWinnersGoods( highballer );

			MESSAGE_BEGIN( MSG_BROADCAST, gmsgPlayClientSound );
				WRITE_BYTE(CLIENT_SOUND_OUTSTANDING);
			MESSAGE_END();
		}
		else
		{
			UTIL_ClientPrintAll(HUD_PRINTCENTER, "Time is up!\nNo one has won!\n");
			UTIL_ClientPrintAll(HUD_PRINTTALK, "* Round ends in a tie!");
			MESSAGE_BEGIN( MSG_BROADCAST, gmsgPlayClientSound );
				WRITE_BYTE(CLIENT_SOUND_HULIMATING_DEAFEAT);
			MESSAGE_END();
		}

		g_GameInProgress = FALSE;
		MESSAGE_BEGIN(MSG_ALL, gmsgShowTimer);
			WRITE_BYTE(0);
		MESSAGE_END();

		flUpdateTime = gpGlobals->time + 5.0;
		m_flRoundTimeLimit = 0;
		return TRUE;
	}

	return FALSE;
}

void CHalfLifeJesusVsSanta::ClientUserInfoChanged( CBasePlayer *pPlayer, char *infobuffer )
{
	if ( pPlayer->IsArmoredMan )
	{
		g_engfuncs.pfnSetClientKeyValue( ENTINDEX( pPlayer->edict() ), infobuffer, "model", "jesus" );
		g_engfuncs.pfnSetClientKeyValue( ENTINDEX( pPlayer->edict() ), infobuffer, "team", "jesus" );
		strncpy( pPlayer->m_szTeamName, "jesus", TEAM_NAME_LENGTH );
	}
	else
	{
		g_engfuncs.pfnSetClientKeyValue( ENTINDEX( pPlayer->edict() ), infobuffer, "model", "santa" );
		g_engfuncs.pfnSetClientKeyValue( ENTINDEX( pPlayer->edict() ), infobuffer, "team", "santa" );
		strncpy( pPlayer->m_szTeamName, "santa", TEAM_NAME_LENGTH );
	}

	// notify everyone's HUD of the team change
	MESSAGE_BEGIN( MSG_ALL, gmsgTeamInfo );
		WRITE_BYTE( ENTINDEX(pPlayer->edict()) );
		WRITE_STRING( pPlayer->IsSpectator() ? "" : pPlayer->m_szTeamName );
	MESSAGE_END();

	MESSAGE_BEGIN( MSG_ALL, gmsgScoreInfo );
		WRITE_BYTE( ENTINDEX(pPlayer->edict()) );
		WRITE_SHORT( pPlayer->pev->frags );
		WRITE_SHORT( pPlayer->m_iDeaths );
		WRITE_SHORT( 0 );
		WRITE_SHORT( g_pGameRules->GetTeamIndex( pPlayer->m_szTeamName ) + 1 );
	MESSAGE_END();
}

BOOL CHalfLifeJesusVsSanta::FPlayerCanTakeDamage( CBasePlayer *pPlayer, CBaseEntity *pAttacker )
{
	if ( pPlayer->pev->fuser4 == pAttacker->pev->fuser4 )
	{
		// my teammate hit me.
		if ( (friendlyfire.value == 0) && (pAttacker != pPlayer) )
		{
			// friendly fire is off, and this hit came from someone other than myself,  then don't get hurt
			return FALSE;
		}
	}

	return CHalfLifeMultiplay::FPlayerCanTakeDamage( pPlayer, pAttacker );
}

void CHalfLifeJesusVsSanta::FPlayerTookDamage( float flDamage, CBasePlayer *pVictim, CBaseEntity *pKiller)
{
	CBasePlayer *pPlayerAttacker = NULL;

	if (pKiller && pKiller->IsPlayer())
	{
		pPlayerAttacker = (CBasePlayer *)pKiller;
		if ( pPlayerAttacker != pVictim && pVictim->IsArmoredMan )
		{
			pPlayerAttacker->m_fArmoredManHits += flDamage;
			ALERT(at_notice, UTIL_VarArgs("Total damage against Jesus is: %.2f\n",
				pPlayerAttacker->m_fArmoredManHits));
		}
		else if ( pPlayerAttacker != pVictim && !pPlayerAttacker->IsArmoredMan && !pVictim->IsArmoredMan )
		{
			ClientPrint(pPlayerAttacker->pev, HUD_PRINTCENTER, "Destroy Jesus!\nNot your teammate!");
		}
	}
}

void CHalfLifeJesusVsSanta::PlayerSpawn( CBasePlayer *pPlayer )
{
	CHalfLifeMultiplay::PlayerSpawn(pPlayer);

	// Place player in spectator mode if joining during a game
	// Or if the game begins that requires spectators
	if ((g_GameInProgress && !pPlayer->IsInArena)
		|| (!g_GameInProgress && HasSpectators()))
	{
		return;
	}

	if ( pPlayer->IsArmoredMan )
	{
		pPlayer->GiveMelees();
		pPlayer->GiveExplosives();
		pPlayer->pev->max_health = pPlayer->pev->health = pPlayer->pev->armorvalue = 750;
		pPlayer->pev->maxspeed = CVAR_GET_FLOAT("sv_maxspeed");
		g_engfuncs.pfnSetPhysicsKeyValue(pPlayer->edict(), "haste", "1");
		pPlayer->GiveNamedItem("rune_cloak");
		g_engfuncs.pfnSetClientKeyValue(ENTINDEX(pPlayer->edict()),
			g_engfuncs.pfnGetInfoKeyBuffer(pPlayer->edict()), "model", "jesus");
		g_engfuncs.pfnSetClientKeyValue(ENTINDEX(pPlayer->edict()),
			g_engfuncs.pfnGetInfoKeyBuffer(pPlayer->edict()), "team", "jesus");
		strncpy( pPlayer->m_szTeamName, "jesus", TEAM_NAME_LENGTH );
	}
	else
	{
		pPlayer->GiveRandomWeapon("weapon_nuke");
		pPlayer->pev->maxspeed = CVAR_GET_FLOAT("sv_maxspeed") * .5;
		g_engfuncs.pfnSetClientKeyValue(ENTINDEX(pPlayer->edict()),
			g_engfuncs.pfnGetInfoKeyBuffer(pPlayer->edict()), "model", "santa");
		g_engfuncs.pfnSetClientKeyValue(ENTINDEX(pPlayer->edict()),
			g_engfuncs.pfnGetInfoKeyBuffer(pPlayer->edict()), "team", "santa");
		strncpy( pPlayer->m_szTeamName, "santa", TEAM_NAME_LENGTH );
	}
}

BOOL CHalfLifeJesusVsSanta::FPlayerCanRespawn( CBasePlayer *pPlayer )
{
	if ( !pPlayer->m_flForceToObserverTime )
		pPlayer->m_flForceToObserverTime = gpGlobals->time + 3.0;

	return FALSE;
}

void CHalfLifeJesusVsSanta::PlayerKilled( CBasePlayer *pVictim, entvars_t *pKiller, entvars_t *pInflictor )
{
	CHalfLifeMultiplay::PlayerKilled(pVictim, pKiller, pInflictor);

	if ( !pVictim->IsArmoredMan )
	{
		int clientsLeft = 0;
		for (int i = 1; i <= gpGlobals->maxClients; i++) {
			if (m_iPlayersInArena[i-1] > 0)
			{
				CBasePlayer *pPlayer = (CBasePlayer *)UTIL_PlayerByIndex(m_iPlayersInArena[i-1]);
				if (pPlayer && pPlayer->IsAlive() && !pPlayer->HasDisconnected)
					clientsLeft++;
			}
		}
		UTIL_ClientPrintAll(HUD_PRINTTALK,
			UTIL_VarArgs("* %s has been eliminated! %d Santas remain!\n",
			STRING(pVictim->pev->netname), clientsLeft > 0 ? clientsLeft - 1 : 0));
		if (clientsLeft > 1)
		{
			MESSAGE_BEGIN( MSG_BROADCAST, gmsgPlayClientSound );
				WRITE_BYTE(CLIENT_SOUND_MASSACRE);
			MESSAGE_END();
		}
	}
}

BOOL CHalfLifeJesusVsSanta::CanHavePlayerItem( CBasePlayer *pPlayer, CBasePlayerItem *pItem )
{
	if (pPlayer->IsArmoredMan)
	{
		if (!strcmp(STRING(pItem->pev->classname), "weapon_vest"))
			return FALSE;
	}

	if (!strcmp(STRING(pItem->pev->classname), "weapon_nuke"))
		return FALSE;

	return CHalfLifeMultiplay::CanHavePlayerItem( pPlayer, pItem );
}

int CHalfLifeJesusVsSanta::GetTeamIndex( const char *pTeamName )
{
	if ( pTeamName && *pTeamName != 0 )
	{
		if (!strcmp(pTeamName, "jesus"))
			return 1;
		else
			return 0; // santa
	}
	
	return -1;	// No match
}