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
//  hud.h
//
// class CHud declaration
//
// CHud handles the message, calculation, and drawing the HUD
//


#define RGB_YELLOWISH 0x00FFA000 //255,160,0
#define RGB_REDISH 0x00FF1010 //255,160,0
#define RGB_GREENISH 0x0000A000 //0,160,0
#define RGB_BLUEISH 0x0000A0FF //0,160,255

#ifndef _WIN32
#define _cdecl 
#endif

#include "wrect.h"
#include "cl_dll.h"
#include "ammo.h"
#include "FlameSystem.h"

#include "PlatformHeaders.h"

#ifndef __APPLE__
#include "GL/gl.h"
#endif

#define DHN_DRAWZERO 1
#define DHN_2DIGITS  2
#define DHN_3DIGITS  4
#define DHN_4DIGITS  8
#define DHN_PADZERO  16
#define MIN_ALPHA	 100	
#define MAX_ALPHA	 200	

#define		HUDELEM_ACTIVE	1

typedef struct {
	int x, y;
} POSITION;

#include "global_consts.h"

typedef struct {
	unsigned char r,g,b,a;
} RGBA;

typedef struct cvar_s cvar_t;


#define HUD_ACTIVE	1
#define HUD_INTERMISSION 2

#define MAX_PLAYER_NAME_LENGTH		32

#define	MAX_MOTD_LENGTH				1536

void TempEntity_Initialize();
void CL_TempEntInit();
//
//-----------------------------------------------------
//
class CHudBase
{
public:
	POSITION  m_pos;
	int   m_type;
	int	  m_iFlags; // active, moving, 
	virtual		~CHudBase() {}
	virtual int Init( void ) {return 0;}
	virtual int VidInit( void ) {return 0;}
	virtual int Draw(float flTime) {return 0;}
	virtual void Think(void) {return;}
	virtual void Reset(void) {return;}
	virtual void InitHUDData( void ) {}		// called every time a server is connected to


};

struct HUDLIST {
	CHudBase	*p;
	HUDLIST		*pNext;
};



//
//-----------------------------------------------------
//
#include "voice_status.h" // base voice handling class
#include "hud_spectator.h"


//
//-----------------------------------------------------
//
class CHudAmmo: public CHudBase
{
public:
	int Init( void );
	int VidInit( void );
	int Draw(float flTime);
	void Think(void);
	void Reset(void);
	int DrawWList(float flTime);
	int MsgFunc_CurWeapon(const char *pszName, int iSize, void *pbuf);
	int MsgFunc_WeaponList(const char *pszName, int iSize, void *pbuf);
	int MsgFunc_AmmoX(const char *pszName, int iSize, void *pbuf);
	int MsgFunc_AmmoPickup( const char *pszName, int iSize, void *pbuf );
	int MsgFunc_WeapPickup( const char *pszName, int iSize, void *pbuf );
	int MsgFunc_ItemPickup( const char *pszName, int iSize, void *pbuf );
	int MsgFunc_HideWeapon( const char *pszName, int iSize, void *pbuf );

	void SlotInput( int iSlot );
	void _cdecl UserCmd_Slot1( void );
	void _cdecl UserCmd_Slot2( void );
	void _cdecl UserCmd_Slot3( void );
	void _cdecl UserCmd_Slot4( void );
	void _cdecl UserCmd_Slot5( void );
	void _cdecl UserCmd_Slot6( void );
	void _cdecl UserCmd_Slot7( void );
	void _cdecl UserCmd_Slot8( void );
	void _cdecl UserCmd_Slot9( void );
	void _cdecl UserCmd_Slot10( void );
	void _cdecl UserCmd_Close( void );
	void _cdecl UserCmd_NextWeapon( void );
	void _cdecl UserCmd_PrevWeapon( void );

private:
	float m_fFade;
	RGBA  m_rgba;
	WEAPON *m_pWeapon;
	int	m_HUD_bucket0;
	int m_HUD_selection;

	int m_hCrosshairBrackets;
	int m_hCrosshairLeft;
	int m_hCrosshairRight;
};

//
//-----------------------------------------------------
//

class CHudAmmoSecondary: public CHudBase
{
public:
	int Init( void );
	int VidInit( void );
	void Reset( void );
	int Draw(float flTime);

	int MsgFunc_SecAmmoVal( const char *pszName, int iSize, void *pbuf );
	int MsgFunc_SecAmmoIcon( const char *pszName, int iSize, void *pbuf );

private:
	enum {
		MAX_SEC_AMMO_VALUES = 4
	};

	int m_HUD_ammoicon; // sprite indices
	int m_iAmmoAmounts[MAX_SEC_AMMO_VALUES];
	float m_fFade;
};


#include "health.h"
#include "lifebar.h"


#define FADE_TIME 100


//
//-----------------------------------------------------
//
class CHudGeiger: public CHudBase
{
public:
	int Init( void );
	int VidInit( void );
	int Draw(float flTime);
	int MsgFunc_Geiger(const char *pszName, int iSize, void *pbuf);
	
private:
	int m_iGeigerRange;

};

//
//-----------------------------------------------------
//
class CHudTrain: public CHudBase
{
public:
	int Init( void );
	int VidInit( void );
	int Draw(float flTime);
	int MsgFunc_Train(const char *pszName, int iSize, void *pbuf);

private:
	HSPRITE m_hSprite;
	int m_iPos;

};

//
//-----------------------------------------------------
//
class CHudMOTD : public CHudBase
{
public:
	int Init( void );
	int VidInit( void );
	int Draw( float flTime );
	void Reset( void );

	int MsgFunc_MOTD( const char *pszName, int iSize, void *pbuf );

protected:
	static int MOTD_DISPLAY_TIME;
	char m_szMOTD[ MAX_MOTD_LENGTH ];
	float m_flActiveTill;
	int m_iLines;
	float m_flActiveRemaining;
};

//
//-----------------------------------------------------
//

class CHudScoreboard: public CHudBase
{
public:
	int Init( void );
	void InitHUDData( void );
	int VidInit( void );
	int Draw( float flTime );
	int DrawPlayers( int xoffset, float listslot, int nameoffset = 0, char *team = NULL ); // returns the ypos where it finishes drawing
	void UserCmd_ShowScores( void );
	void UserCmd_HideScores( void );
	int MsgFunc_ScoreInfo2( const char *pszName, int iSize, void *pbuf );
	int MsgFunc_TeamInfo2( const char *pszName, int iSize, void *pbuf );
	int MsgFunc_TeamScore2( const char *pszName, int iSize, void *pbuf );
	void DeathMsg( int killer, int victim );

	enum {
		MAX_PLAYERS = 32,
		MAX_TEAMS = 32,
		MAX_TEAM_NAME = 16,
	};

	hud_player_info_t m_PlayerInfoList[MAX_PLAYERS+1];	   // player info from the engine

	int m_iNumTeams;

	int m_iLastKilledBy;
	int m_fLastKillTime;
	int m_iPlayerNum;
	int m_iShowscoresHeld;

	cvar_t *cl_showpacketloss;

	void GetAllPlayersInfo( void );
};

//
//-----------------------------------------------------
//
class CHudStatusBar : public CHudBase
{
public:
	int Init( void );
	int VidInit( void );
	int Draw( float flTime );
	void Reset( void );
	void ParseStatusString( int line_num );

	int MsgFunc_StatusText( const char *pszName, int iSize, void *pbuf );
	int MsgFunc_StatusValue( const char *pszName, int iSize, void *pbuf );

protected:
	enum { 
		MAX_STATUSTEXT_LENGTH = 128,
		MAX_STATUSBAR_VALUES = 8,
		MAX_STATUSBAR_LINES = 3,
	};

	char m_szStatusText[MAX_STATUSBAR_LINES][MAX_STATUSTEXT_LENGTH];  // a text string describing how the status bar is to be drawn
	char m_szStatusBar[MAX_STATUSBAR_LINES][MAX_STATUSTEXT_LENGTH];	// the constructed bar that is drawn
	int m_iStatusValues[MAX_STATUSBAR_VALUES];  // an array of values for use in the status bar

	int m_bReparseString; // set to TRUE whenever the m_szStatusBar needs to be recalculated

	// an array of colors...one color for each line
	float *m_pflNameColors[MAX_STATUSBAR_LINES];
};

struct extra_player_info_t 
{
	short frags;
	short deaths;
	short playerclass;
	short health; // UNUSED currently, spectator UI would like this
	bool dead; // UNUSED currently, spectator UI would like this
	short teamnumber;
	char teamname[MAX_TEAM_NAME];
	short vote;
};

struct team_info_t 
{
	char name[MAX_TEAM_NAME];
	short frags;
	short deaths;
	short ping;
	short packetloss;
	short ownteam;
	short players;
	int already_drawn;
	int scores_overriden;
	int teamnumber;
	int score;
};

#include "player_info.h"

//
//-----------------------------------------------------
//
class CHudDeathNotice : public CHudBase
{
public:
	int Init( void );
	void InitHUDData( void );
	int VidInit( void );
	int Draw( float flTime );
	void PlayKillSound( int killer, int victim );
	void CalculateUTKills(int killer, int victim);
	void PlayUTKills();
	int MsgFunc_DeathMsg( const char *pszName, int iSize, void *pbuf );

private:
	int m_HUD_d_skull;  // sprite index of skull icon
	int m_HUD_killspree;
};

//
//-----------------------------------------------------
//
class CHudMenu : public CHudBase
{
public:
	int Init( void );
	void InitHUDData( void );
	int VidInit( void );
	void Reset( void );
	int Draw( float flTime );
	int MsgFunc_ShowMenu( const char *pszName, int iSize, void *pbuf );

	void SelectMenuItem( int menu_item );

	int m_fMenuDisplayed;
	int m_bitsValidSlots;
	float m_flShutoffTime;
	int m_fWaitingForMore;
};

//
//-----------------------------------------------------
//
class CHudSayText : public CHudBase
{
public:
	int Init( void );
	void InitHUDData( void );
	int VidInit( void );
	int Draw( float flTime );
	int MsgFunc_SayText( const char *pszName, int iSize, void *pbuf );
	void SayTextPrint( const char *pszBuf, int iBufSize, int clientIndex = -1 );
	void EnsureTextFitsInOneLineAndWrapIfHaveTo( int line );
friend class CHudSpectator;

private:

	struct cvar_s *	m_HUD_saytext;
	struct cvar_s *	m_HUD_saytext_time;
};

//
//-----------------------------------------------------
//
class CHudBattery: public CHudBase
{
public:
	int Init( void );
	int VidInit( void );
	int Draw(float flTime);
	int MsgFunc_Battery(const char *pszName,  int iSize, void *pbuf );
	
private:
	HSPRITE m_hSprite1;
	HSPRITE m_hSprite2;
	wrect_t *m_prc1;
	wrect_t *m_prc2;
	int	  m_iBat;	
	int	  m_iBatMax;
	float m_fFade;
	int	  m_iHeight;		// width of the battery innards
};


//
//-----------------------------------------------------
//
class CHudFlashlight: public CHudBase
{
public:
	int Init( void );
	int VidInit( void );
	int Draw(float flTime);
	void Reset( void );
	int MsgFunc_Flashlight(const char *pszName,  int iSize, void *pbuf );
	int MsgFunc_FlashBat(const char *pszName,  int iSize, void *pbuf );
	
private:
	HSPRITE m_hSprite1;
	HSPRITE m_hSprite2;
	HSPRITE m_hBeam;
	wrect_t *m_prc1;
	wrect_t *m_prc2;
	wrect_t *m_prcBeam;
	float m_flBat;	
	int	  m_iBat;	
	int	  m_fOn;
	float m_fFade;
	int	  m_iWidth;		// width of the battery innards
};

//
//-----------------------------------------------------
//
const int maxHUDMessages = 16;
struct message_parms_t
{
	client_textmessage_t	*pMessage;
	float	time;
	int x, y;
	int	totalWidth, totalHeight;
	int width;
	int lines;
	int lineLength;
	int length;
	int r, g, b;
	int text;
	int fadeBlend;
	float charTime;
	float fadeTime;
};

//
//-----------------------------------------------------
//

class CHudTextMessage: public CHudBase
{
public:
	int Init( void );
	static char *LocaliseTextString( const char *msg, char *dst_buffer, int buffer_size );
	static char *BufferedLocaliseTextString( const char *msg );
	char *LookupString( const char *msg_name, int *msg_dest = NULL );
	int MsgFunc_TextMsg(const char *pszName, int iSize, void *pbuf);
};

//
//-----------------------------------------------------
//

class CHudMessage: public CHudBase
{
public:
	int Init( void );
	int VidInit( void );
	int Draw(float flTime);
	int MsgFunc_HudText(const char *pszName, int iSize, void *pbuf);
	int MsgFunc_HudTextPro(const char *pszName, int iSize, void *pbuf);
	int MsgFunc_GameTitle(const char *pszName, int iSize, void *pbuf);

	float FadeBlend( float fadein, float fadeout, float hold, float localTime );
	int	XPosition( float x, int width, int lineWidth );
	int YPosition( float y, int height );

	void MessageAdd( const char *pName, float time );
	void MessageAdd(client_textmessage_t * newMessage );
	void MessageDrawScan( client_textmessage_t *pMessage, float time );
	void MessageScanStart( void );
	void MessageScanNextChar( void );
	void Reset( void );

private:
	client_textmessage_t		*m_pMessages[maxHUDMessages];
	float						m_startTime[maxHUDMessages];
	message_parms_t				m_parms;
	float						m_gameTitleTime;
	client_textmessage_t		*m_pGameTitle;

	int m_HUD_title_life;
	int m_HUD_title_half;
};

//
//-----------------------------------------------------
//
#define MAX_SPRITE_NAME_LENGTH	24

struct mutators_t
{
	int mutatorId;
	float startTime = 0;
	float timeToLive = 0;
	mutators_t *next;
};

bool MutatorEnabled(int mutatorId);
mutators_t GetMutator(int mutatorId);

class CHudStatusIcons: public CHudBase
{
public:
	int Init( void );
	int VidInit( void );
	void Reset( void );
	int Draw(float flTime);
	void DrawMutators( void );
	int MsgFunc_StatusIcon(const char *pszName, int iSize, void *pbuf);

	enum { 
		MAX_ICONSPRITENAME_LENGTH = MAX_SPRITE_NAME_LENGTH,
		MAX_ICONSPRITES = 4,
	};

	
	//had to make these public so CHud could access them (to enable concussion icon)
	//could use a friend declaration instead...
	void EnableIcon( char *pszIconName, unsigned char red, unsigned char green, unsigned char blue, float timeToLive, float startTime );
	void DisableIcon( char *pszIconName );

private:

	typedef struct Icons_s
	{
		char szSpriteName[MAX_ICONSPRITENAME_LENGTH];
		HSPRITE spr;
		wrect_t rc;
		unsigned char r, g, b;
		int startTime = 0;
		int timeToLive = 0;
	} icon_sprite_t;

	icon_sprite_t m_IconList[MAX_ICONSPRITES];

	float m_flCheckMutators;

	void ToggleMutatorIcon(int mutatorId, const char *mutator);
};

//
//-----------------------------------------------------
//
class CHudBenchmark : public CHudBase
{
public:
	int Init( void );
	int VidInit( void );
	int Draw( float flTime );

	void SetScore( float score );

	void Think( void );

	void StartNextSection( int section );

	int MsgFunc_Bench(const char *pszName, int iSize, void *pbuf);

	void CountFrame( float dt );

	int GetObjects( void ) { return m_nObjects; };

	void SetCompositeScore( void );

	void Restart( void );

	int Bench_ScoreForValue( int stage, float raw );

private:
	float	m_fDrawTime;
	float	m_fDrawScore;
	float	m_fAvgScore;

	float   m_fSendTime;
	float	m_fReceiveTime;

	int		m_nFPSCount;
	float	m_fAverageFT;
	float	m_fAvgFrameRate;

	int		m_nSentFinish;
	float	m_fStageStarted;

	float	m_StoredLatency;
	float	m_StoredPacketLoss;
	int		m_nStoredHopCount;
	int		m_nTraceDone;

	int		m_nObjects;

	int		m_nScoreComputed;
	int 	m_nCompositeScore;
};

//
//-----------------------------------------------------
//

class CHudWallClimb: public CHudBase
{
public:
	int Init( void );
	int VidInit( void );
	int Draw(float flTime);

private:
	HSPRITE m_hSprite;
	int m_iPos;
};

class CHudParticle: public CHudBase
{
public:
	int Init( void );
	int VidInit( void );
	int Draw(float flTime);
	int MsgFunc_Particle(const char *pszName, int iSize, void *pbuf );
	void Reset( void );
private:
	void SetParticles( void );
	int iceModels;
};

class CHudNukeCrosshair : public CHudBase
{
public:
	int Init( void );
	int VidInit( void );
	int Draw(float flTime);
	int MsgFunc_NukeCross(const char *pszName,  int iSize, void *pbuf);
	int m_iHudMode;
private:
	HSPRITE m_hCrosshair;
	HSPRITE m_hStatic;
};

typedef struct {
	float angle, distance, height;
	int special;
} RADAR;

#define MAX_RADAR_DOTS 64
class CHudRadar: public CHudBase
{
public:
	int Init( void );
	int VidInit( void );
	int Draw(float flTime);
	void ProcessPlayerState( void );

private:
	int radar_height, radar_width;
	RADAR m_RadarInfo[MAX_RADAR_DOTS];
};

class CHudObjective : public CHudBase
{
public:
	int Init( void );
	int VidInit( void );
	int Draw(float flTime);
	int CalcPosition( );
	int CalcPlayer( char *team = NULL ); // returns the ypos where it finishes drawing
	int MsgFunc_Objective(const char *pszName,  int iSize, void *pbuf);

	enum {
		MAX_PLAYERS = 32,
		MAX_TEAMS = 32,
		MAX_TEAM_NAME = 16,
	};

private:
	int m_iPercent;
	char m_szGoalMessage[128];
	char m_szInfoMessage[128];
	char m_szWinsMessage[128];
	int m_iRoundWins;
	int m_iRoundPlays;
};

class CHudTimer : public CHudBase
{
public:
	int Init( void );
	int VidInit( void );
	int Draw(float fTime);
	int MsgFunc_RoundTime(const char *pszName, int iSize, void *pbuf);
	int MsgFunc_ShowTimer(const char *pszName, int iSize, void *pbuf);

private:
	int m_HUD_timer;
	int m_iTime;
	float m_fStartTime;
	bool m_bPanicColorChange;
	float m_flPanicTime;
};

typedef struct
{
	const char *message;
	float time;
	int y_pos;
} protip_s;

enum e_protips {
	THROW_TIP = 0,
	FIST_TIP,
	GRENADE_TIP,
	SNARK_TIP,
	VEST_TIP,
	KNIFE_TIP,
	DUAL_TIP,
	IRONSIGHTS_TIP,
	FLAMETHROWER_TIP,
	WRENCH_TIP,
	RUNE_TIP,
	SILENCER_TIP,
	CHAINSAW_TIP,
	RPG_TIP,
	JUMP_TIP,
	KICK_TIP,
	FEIGN_TIP,
	FORCEGRAB_TIP,
	PROP_TIP,
	DROP_TIP,
};

#define PROTIPS_AMT	19



class CHudProTip : public CHudBase
{
public:
	int Init( void );
	int VidInit( void );
	void AddMessage(int id, const char *message);
	int Draw(float flTime);
private:
	HSPRITE m_hMouseClick;
	protip_s m_MessageQueue[3];
	bool m_ShownTip[PROTIPS_AMT];
};

class CHudCtfInfo : public CHudBase
{
public:
	int Init( void );
	int VidInit( void );
	int Draw(float flTime);
	int MsgFunc_CtfInfo(const char *pszName,  int iSize, void *pbuf);

private:
	int m_iBlueScore;
	int m_iRedScore;
	int m_iBlueMode;
	int m_iRedMode;
	float m_fFade;
};

typedef struct
{
	HSPRITE spr;
	wrect_t rc;
	int r;
	int g;
	int b;
} crosspr_s;

class CHud
{
private:
	HUDLIST						*m_pHudList;
	HSPRITE						m_hsprLogo;
	int							m_iLogo;
	client_sprite_t				*m_pSpriteList;
	int							m_iSpriteCount;
	int							m_iSpriteCountAllRes;
	float						m_flMouseSensitivity;
	int							m_iConcussionEffect; 

public:

	HSPRITE						m_hsprCursor;
	float m_flTime;	   // the current client time
	float m_fOldTime;  // the time at which the HUD was last redrawn
	double m_flTimeDelta; // the difference between flTime and fOldTime
	Vector	m_vecOrigin;
	Vector	m_vecAngles;
	int		m_iKeyBits;
	int		m_iHideHUDDisplay;
	int		m_iFOV;
	int		m_Teamplay;
	int		m_GameMode;
	int		m_iRes;
	cvar_t  *m_pCvarStealMouse;
	cvar_t	*m_pCvarDraw;

	int		m_iShowingWeaponMenu;

	Vector portal1finalorg;
	Vector portal2finalorg;

	float m_iShownHelpMessage;
	void ShowTextTips( void );

	int m_iFontHeight;
	int DrawHudNumber(int x, int y, int iFlags, int iNumber, int r, int g, int b );
	int DrawHudString(int x, int y, int iMaxX, char *szString, int r, int g, int b );
	int DrawHudStringReverse( int xpos, int ypos, int iMinX, char *szString, int r, int g, int b );
	int DrawHudNumberString( int xpos, int ypos, int iMinX, int iNumber, int r, int g, int b );
	int GetNumWidth(int iNumber, int iFlags);

	int	m_IceModelsIndex;
	bool SetPLFlames;
	mutators_t *m_Mutators;
	int m_ChaosTime;
	int m_ChaosStartTime;
	int m_ChaosIncrement;
	int m_SafeSpotSize;

private:
	// the memory for these arrays are allocated in the first call to CHud::VidInit(), when the hud.txt and associated sprites are loaded.
	// freed in ~CHud()
	HSPRITE *m_rghSprites;	/*[HUD_SPRITE_COUNT]*/			// the sprites loaded from hud.txt
	wrect_t *m_rgrcRects;	/*[HUD_SPRITE_COUNT]*/
	char *m_rgszSpriteNames; /*[HUD_SPRITE_COUNT][MAX_SPRITE_NAME_LENGTH]*/

	struct cvar_s *default_fov;
public:
	HSPRITE GetSprite( int index ) 
	{
		return (index < 0) ? 0 : m_rghSprites[index];
	}

	wrect_t& GetSpriteRect( int index )
	{
		return m_rgrcRects[index];
	}

	
	int GetSpriteIndex( const char *SpriteName );	// gets a sprite index, for use in the m_rghSprites[] array

	CHudAmmo		m_Ammo;
	CHudHealth		m_Health;
	CHudSpectator		m_Spectator;
	CHudGeiger		m_Geiger;
	CHudBattery		m_Battery;
	CHudTrain		m_Train;
	CHudFlashlight	m_Flash;
	CHudMessage		m_Message;
	CHudStatusBar   m_StatusBar;
	CHudDeathNotice m_DeathNotice;
	CHudSayText		m_SayText;
	CHudMenu		m_Menu;
	CHudAmmoSecondary	m_AmmoSecondary;
	CHudTextMessage m_TextMessage;
	CHudStatusIcons m_StatusIcons;
	CHudBenchmark	m_Benchmark;
	CHudMOTD        m_MOTD;
	CHudScoreboard  m_Scoreboard;
	CHudLifeBar		m_LifeBar;
	CHudWallClimb   m_WallClimb;
	CHudParticle	m_Particle;
	CHudNukeCrosshair	m_NukeCrosshair;
	CHudRadar	m_Radar;
	CHudObjective	m_Objective;
	CHudTimer	m_Timer;
	CHudProTip	m_ProTip;
	CHudCtfInfo m_CtfInfo;

	void Init( void );
	void VidInit( void );
	void Think(void);
	int Redraw( float flTime, int intermission );
	int UpdateClientData( client_data_t *cdata, float time );

	void Mirror( void );

	CHud() : m_iSpriteCount(0), m_pHudList(NULL) {}  
	~CHud();			// destructor, frees allocated memory

	// user messages
	int _cdecl MsgFunc_Damage(const char *pszName, int iSize, void *pbuf );
	int _cdecl MsgFunc_GameMode(const char *pszName, int iSize, void *pbuf );
	int _cdecl MsgFunc_Logo(const char *pszName,  int iSize, void *pbuf);
	int _cdecl MsgFunc_ResetHUD(const char *pszName,  int iSize, void *pbuf);
	void _cdecl MsgFunc_InitHUD( const char *pszName, int iSize, void *pbuf );
	void _cdecl MsgFunc_ViewMode( const char *pszName, int iSize, void *pbuf );
	int _cdecl MsgFunc_SetFOV(const char *pszName,  int iSize, void *pbuf);
	int  _cdecl MsgFunc_Concuss( const char *pszName, int iSize, void *pbuf );

	// Cold Ice Remastered
	int  _cdecl MsgFunc_Acrobatics( const char *pszName, int iSize, void *pbuf );
	int  _cdecl MsgFunc_PlayCSound( const char *pszName, int iSize, void *pbuf );
	int  _cdecl MsgFunc_AddMut( const char *pszName, int iSize, void *pbuf );
	int  _cdecl MsgFunc_Chaos( const char *pszName, int iSize, void *pbuf );
	int _cdecl MsgFunc_Particle( const char *pszName, int iSize, void *pbuf );
	void _cdecl MsgFunc_DelPart( const char *pszName, int iSize, void *pbuf );
	void _cdecl MsgFunc_FlameMsg( const char *pszName, int iSize, void *pbuf );
	void _cdecl MsgFunc_FlameKill( const char *pszName, int iSize, void *pbuf );
	void _cdecl MsgFunc_MParticle( const char *pszName, int iSize, void *pbuf );
	int  _cdecl MsgFunc_Spot( const char *pszName, int iSize, void *pbuf );
	int  _cdecl MsgFunc_DEraser( const char *pszName, int iSize, void *pbuf );

	// Screen information
	SCREENINFO	m_scrinfo;

	int	m_iWeaponBits;
	int	m_iWeaponBits2;
	int	m_fPlayerDead;
	int m_fPlayerDeadTime;
	int m_iIntermission;
	int m_ShowKeyboard;

	// sprite indexes
	int m_HUD_number_0;


	void AddHudElem(CHudBase *p);

	float GetSensitivity();

	cl_entity_t m_ExtraViewModel;
	float m_flExtraViewModelTime = 0.0f;

	// Check flashlight state
	bool m_bFlashlight;
	void FlashHud();

	crosspr_s crossspr;

	int m_PlayersInRadar;

	int local_player_index;
};

#ifdef _WIN32
class CImguiManager
{
public:
	bool Init();
	bool VidInit();
	void Draw();
	void DrawChapter();

	// chapter selection variables
	bool isMenuOpen = false;
	int page;
	bool skillMode[4];
};
extern CImguiManager g_ImGUIManager;
#endif

#ifndef __APPLE__
class CPortalRenderer
{
public:
	void Init();
	void VidInit();
	void DrawPortal();
	void CapturePortalView(int pass);

	GLuint portalPass_1;
	GLuint portalPass_2;
	GLuint screenpass;
	GLuint blankshit;
	bool m_bIsDrawingPortal;
	int portal1index;
	int portal2index;

	Vector m_Portal1[2]; // origin, angle
	Vector m_Portal2[2]; // origin, angle
	Vector m_PortalVertex[2][4];

	GLuint finalPortal[2];
	Vector2D portalSize[2];
};
extern CPortalRenderer gPortalRenderer;
#endif

extern CHud gHUD;

extern int g_iPlayerClass;
extern int g_iTeamNumber;
extern int g_iUser1;
extern int g_iUser2;
extern int g_iUser3;

