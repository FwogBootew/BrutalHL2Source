//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:		Incarcerator of hell
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h" //yes
#include "npcevent.h" //yes
#include "basehlcombatweapon.h"
#include "basehlcombatweapon_shared.h"
#include "basecombatcharacter.h" //yes
#include "ai_basenpc.h" //yes
#include "player.h" //yes 
#include "gamerules.h"//yes 
#include "in_buttons.h"//yes
#include "soundent.h"//yes
#include "game.h"
#include "vstdlib/random.h"//yes
#include "engine/IEngineSound.h"
#include "te_effect_dispatch.h"
#include "gamestats.h"//yes
#include "beam_shared.h"
#include "ammodef.h"

//

#define PHYSCANNON_BEAM_SPRITE "sprites/orangelight1.vmt"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// CWeaponIncarcerator
//-----------------------------------------------------------------------------

class CWeaponIncarcerator : public CHLMachineGun
{
	DECLARE_CLASS(CWeaponIncarcerator, CHLMachineGun);
public:

	CWeaponIncarcerator(void);

	void	AddViewKick(void);
	void	PrimaryAttack(void);
	bool	Reload(void);
	float	WeaponAutoAimScale()	{ return 0.6f; }

	DECLARE_SERVERCLASS();

	float	GetFireRate(void) { return 0.050f; }	// 13.3hz
	int		CapabilitiesGet(void) { return bits_CAP_WEAPON_RANGE_ATTACK1; }

	virtual const Vector& GetBulletSpread(void)
	{
		static const Vector cone = VECTOR_CONE_4DEGREES;
		return cone;
	}

	DECLARE_DATADESC();



	DECLARE_ACTTABLE();

	int        m_nBulletType;
};

LINK_ENTITY_TO_CLASS(weapon_Incarcerator, CWeaponIncarcerator);

PRECACHE_WEAPON_REGISTER(weapon_incarcerator);

IMPLEMENT_SERVERCLASS_ST(CWeaponIncarcerator, DT_WeaponIncarcerator)
END_SEND_TABLE()

BEGIN_DATADESC(CWeaponIncarcerator)

DEFINE_FIELD(m_nBulletType, FIELD_INTEGER),

END_DATADESC()

acttable_t	CWeaponIncarcerator::m_acttable[] =
{
	{ ACT_RANGE_ATTACK1, ACT_RANGE_ATTACK_SMG1, true },
	{ ACT_RELOAD, ACT_RELOAD_SMG1, true },
	{ ACT_IDLE, ACT_IDLE_SMG1, true },
	{ ACT_IDLE_ANGRY, ACT_IDLE_ANGRY_SMG1, true },

	{ ACT_WALK, ACT_WALK_RIFLE, true },
	{ ACT_WALK_AIM, ACT_WALK_AIM_RIFLE, true },

	// Readiness activities (not aiming)
	{ ACT_IDLE_RELAXED, ACT_IDLE_SMG1_RELAXED, false },//never aims
	{ ACT_IDLE_STIMULATED, ACT_IDLE_SMG1_STIMULATED, false },
	{ ACT_IDLE_AGITATED, ACT_IDLE_ANGRY_SMG1, false },//always aims

	{ ACT_WALK_RELAXED, ACT_WALK_RIFLE_RELAXED, false },//never aims
	{ ACT_WALK_STIMULATED, ACT_WALK_RIFLE_STIMULATED, false },
	{ ACT_WALK_AGITATED, ACT_WALK_AIM_RIFLE, false },//always aims

	{ ACT_RUN_RELAXED, ACT_RUN_RIFLE_RELAXED, false },//never aims
	{ ACT_RUN_STIMULATED, ACT_RUN_RIFLE_STIMULATED, false },
	{ ACT_RUN_AGITATED, ACT_RUN_AIM_RIFLE, false },//always aims

	// Readiness activities (aiming)
	{ ACT_IDLE_AIM_RELAXED, ACT_IDLE_SMG1_RELAXED, false },//never aims	
	{ ACT_IDLE_AIM_STIMULATED, ACT_IDLE_AIM_RIFLE_STIMULATED, false },
	{ ACT_IDLE_AIM_AGITATED, ACT_IDLE_ANGRY_SMG1, false },//always aims

	{ ACT_WALK_AIM_RELAXED, ACT_WALK_RIFLE_RELAXED, false },//never aims
	{ ACT_WALK_AIM_STIMULATED, ACT_WALK_AIM_RIFLE_STIMULATED, false },
	{ ACT_WALK_AIM_AGITATED, ACT_WALK_AIM_RIFLE, false },//always aims

	{ ACT_RUN_AIM_RELAXED, ACT_RUN_RIFLE_RELAXED, false },//never aims
	{ ACT_RUN_AIM_STIMULATED, ACT_RUN_AIM_RIFLE_STIMULATED, false },
	{ ACT_RUN_AIM_AGITATED, ACT_RUN_AIM_RIFLE, false },//always aims
	//End readiness activities

	{ ACT_WALK_AIM, ACT_WALK_AIM_RIFLE, true },
	{ ACT_WALK_CROUCH, ACT_WALK_CROUCH_RIFLE, true },
	{ ACT_WALK_CROUCH_AIM, ACT_WALK_CROUCH_AIM_RIFLE, true },
	{ ACT_RUN, ACT_RUN_RIFLE, true },
	{ ACT_RUN_AIM, ACT_RUN_AIM_RIFLE, true },
	{ ACT_RUN_CROUCH, ACT_RUN_CROUCH_RIFLE, true },
	{ ACT_RUN_CROUCH_AIM, ACT_RUN_CROUCH_AIM_RIFLE, true },
	{ ACT_GESTURE_RANGE_ATTACK1, ACT_GESTURE_RANGE_ATTACK_SMG1, true },
	{ ACT_RANGE_ATTACK1_LOW, ACT_RANGE_ATTACK_SMG1_LOW, true },
	{ ACT_COVER_LOW, ACT_COVER_SMG1_LOW, false },
	{ ACT_RANGE_AIM_LOW, ACT_RANGE_AIM_SMG1_LOW, false },
	{ ACT_RELOAD_LOW, ACT_RELOAD_SMG1_LOW, false },
	{ ACT_GESTURE_RELOAD, ACT_GESTURE_RELOAD_SMG1, true },
};

IMPLEMENT_ACTTABLE(CWeaponIncarcerator);


//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CWeaponIncarcerator::CWeaponIncarcerator(void)
{
	m_bReloadsSingly = false;
	m_bFiresUnderwater = false;
}
//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
bool CWeaponIncarcerator::Reload(void)
{
	bool fRet;
	float fCacheTime = m_flNextSecondaryAttack;

	fRet = DefaultReload(GetMaxClip1(), GetMaxClip2(), ACT_VM_RELOAD);
	if (fRet)
	{
		// Undo whatever the reload process has done to our secondary
		// attack timer. We allow you to interrupt reloading to fire
		// a grenade.
		m_flNextSecondaryAttack = GetOwner()->m_flNextAttack = fCacheTime;

		WeaponSound(RELOAD);
	}

	return fRet;
}

void CWeaponIncarcerator::AddViewKick(void)
{
#define	EASY_DAMPEN			0.5f
#define	MAX_VERTICAL_KICK	1.0f	//Degrees
#define	SLIDE_LIMIT			2.0f	//Seconds

	//Get the view kick
	CBasePlayer *pPlayer = ToBasePlayer(GetOwner());

	if (pPlayer == NULL)
		return;

	DoMachineGunKick(pPlayer, EASY_DAMPEN, MAX_VERTICAL_KICK, m_fFireDuration, SLIDE_LIMIT);
}
//-----------------------------------------------------------------------------
// Purpose: Constructor
//
//
//-----------------------------------------------------------------------------
void CWeaponIncarcerator::PrimaryAttack(void)
{
	// Only the player fires this way so we can cast
	CBasePlayer *pPlayer = ToBasePlayer(GetOwner());
	if (!pPlayer)
		return;

	// Abort here to handle burst and auto fire modes
	if ((UsesClipsForAmmo1() && m_iClip1 == 0) || (!UsesClipsForAmmo1() && !pPlayer->GetAmmoCount(m_iPrimaryAmmoType)))
		return;

	m_nShotsFired++;

	pPlayer->DoMuzzleFlash();

	// To make the firing framerate independent, we may have to fire more than one bullet here on low-framerate systems, 
	// especially if the weapon we're firing has a really fast rate of fire.
	int iBulletsToFire = 0;
	float fireRate = GetFireRate();

	// MUST call sound before removing a round from the clip of a CHLMachineGun
	while (m_flNextPrimaryAttack <= gpGlobals->curtime)
	{
		WeaponSound(SINGLE, m_flNextPrimaryAttack);
		m_flNextPrimaryAttack = m_flNextPrimaryAttack + fireRate;
		iBulletsToFire++;
	}

	// Make sure we don't fire more than the amount in the clip, if this weapon uses clips
	if (UsesClipsForAmmo1())
	{
		if (iBulletsToFire > m_iClip1)
			iBulletsToFire = m_iClip1;
		m_iClip1 -= iBulletsToFire;
	}

	m_iPrimaryAttacks++;
	gamestats->Event_WeaponFired(pPlayer, true, GetClassname());

	// Fire the bullets
	FireBulletsInfo_t info;
	info.m_iShots = iBulletsToFire;
	info.m_vecSrc = pPlayer->Weapon_ShootPosition();
	info.m_vecDirShooting = pPlayer->GetAutoaimVector(AUTOAIM_SCALE_DEFAULT);
	info.m_vecSpread = pPlayer->GetAttackSpread(this);
	info.m_flDistance = MAX_TRACE_LENGTH;
	info.m_iAmmoType = m_iPrimaryAmmoType;
	info.m_iTracerFreq = 2;
	FireBullets(info);

	//Factor in the view kick
	AddViewKick();

	CSoundEnt::InsertSound(SOUND_COMBAT, GetAbsOrigin(), SOUNDENT_VOLUME_MACHINEGUN, 0.2, pPlayer);
	//pointingshit
	if (!pPlayer)
		return; //Always validate a pointer

	//Create our trace_t class to hold the end result
	trace_t tr;

	//Create Vectors for the start, stop, and direction
	Vector vecAbsStart, vecAbsEnd, vecDir;

	//Take the Player's EyeAngles and turn it into a direction
	AngleVectors(pPlayer->EyeAngles(), &vecDir);

	//Get the Start/End
	vecAbsStart = pPlayer->EyePosition();
	vecAbsEnd = vecAbsStart + (vecDir * MAX_TRACE_LENGTH);

	//Do the TraceLine, and write our results to our trace_t class, tr.
	UTIL_TraceLine(vecAbsStart, vecAbsEnd, MASK_ALL, pPlayer, COLLISION_GROUP_NONE, &tr);

	//Do something with the end results
	if (tr.m_pEnt)
	{
		if (tr.m_pEnt->IsNPC())
		{
			Msg("TraceLine hit an NPC!\n");
		}
		if (tr.m_pEnt->IsPlayer())
		{
			Msg("TraceLine hit a Player!\n");
		}
	}

	if (!m_iClip1 && pPlayer->GetAmmoCount(m_iPrimaryAmmoType) <= 0)
	{
		// HEV suit - indicate out of ammo condition
		pPlayer->SetSuitUpdate("!HEV_AMO0", FALSE, 0);
	}
	//Credit to Obsidian Conflict team and Maestro Fénix for grappling code
	//REMOVE THIS LATER MAYBE
	/*trace_t tr;
	Vector vecShootOrigin, vecShootDir, vecDir, vecEnd;
	//Gets the direction where the player is aiming
	AngleVectors(pPlayer->EyeAngles(), &vecDir);
	//Gets the position of the player
	vecShootOrigin = pPlayer->Weapon_ShootPosition();
	//Gets the position where the hook will hit
	vecEnd = vecShootOrigin + (vecDir * MAX_TRACE_LENGTH);
	//Traces a line between the two vectors
	UTIL_TraceLine(vecShootOrigin, vecEnd, MASK_SHOT, pPlayer, COLLISION_GROUP_NONE, &tr);
	//Draws the beam
	DrawBeam(vecShootOrigin, tr.endpos, 15.5);
	//Creates an energy impact effect if we don't hit the sky or other places
	if ((tr.surface.flags & SURF_SKY) == false)
	{
		CPVSFilter filter(tr.endpos);
		te->GaussExplosion(filter, 0.0f, tr.endpos, tr.plane.normal, 0);
		m_nBulletType = GetAmmoDef()->Index("GaussEnergy");
		UTIL_ImpactTrace(&tr, m_nBulletType);
		//Makes a sprite at the end of the beam
		m_pLightGlow = CSprite::SpriteCreate("sprites/physcannon_bluecore2b.vmt", GetAbsOrigin(), TRUE);
		//Sets FX render and color
		m_pLightGlow->SetTransparency(9, 255, 255, 255, 200, kRenderFxNoDissipation);
		//Sets the position
		m_pLightGlow->SetAbsOrigin(tr.endpos);
		//Bright
		m_pLightGlow->SetBrightness(255);
		//Scale
		m_pLightGlow->SetScale(0.65);*/

	SendWeaponAnim(GetPrimaryAttackActivity());
	pPlayer->SetAnimation(PLAYER_ATTACK1);

	// Register a muzzleflash for the AI
	pPlayer->SetMuzzleFlashTime(gpGlobals->curtime + 0.5);
}
//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &amp;startPos - 
//			&amp;endPos - 
//			width - 
//			useMuzzle - 
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &amp;tr - 
//			nDamageType - 
//-----------------------------------------------------------------------------
