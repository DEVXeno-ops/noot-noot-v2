#include "Aimbot.h"
#include "AimbotHitscan/AimbotHitscan.h"
#include "AimbotProjectile/AimbotProjectile.h"
#include "AimbotMelee/AimbotMelee.h"
#include "AutoDetonate/AutoDetonate.h"
#include "AutoAirblast/AutoAirblast.h"
#include "AutoHeal/AutoHeal.h"
#include "AutoRocketJump/AutoRocketJump.h"
#include "../Misc/Misc.h"
#include "../Visuals/Visuals.h"
#include "../NavBot/NavEngine/NavEngine.h"
#include <iostream>

bool CAimbot::ShouldRun(CTFPlayer* pLocal, CTFWeaponBase* pWeapon)
{
    if (!pLocal || !pWeapon || !pLocal->CanAttack()
        || !SDK::AttribHookValue(1, "mult_dmg", pWeapon)
        || I::EngineVGui->IsGameUIVisible())
    {
        std::cout << "[Aimbot] ShouldRun failed: Invalid player, weapon, or UI visible." << std::endl;
        return false;
    }

    return true;
}

void CAimbot::RunAimbot(CTFPlayer* pLocal, CTFWeaponBase* pWeapon, CUserCmd* pCmd, bool bSecondaryType)
{
    m_bRunningSecondary = bSecondaryType;
    EWeaponType eWeaponType = bSecondaryType ? G::SecondaryWeaponType : G::PrimaryWeaponType;

    bool bOriginal = G::CanPrimaryAttack;
    if (bSecondaryType)
        G::CanPrimaryAttack = G::CanSecondaryAttack;

    switch (eWeaponType)
    {
    case EWeaponType::HITSCAN:
        std::cout << "[Aimbot] Running Hitscan Aimbot." << std::endl;
        F::AimbotHitscan.Run(pLocal, pWeapon, pCmd);
        break;
    case EWeaponType::PROJECTILE:
        std::cout << "[Aimbot] Running Projectile Aimbot." << std::endl;
        F::AimbotProjectile.Run(pLocal, pWeapon, pCmd);
        break;
    case EWeaponType::MELEE:
        std::cout << "[Aimbot] Running Melee Aimbot." << std::endl;
        F::AimbotMelee.Run(pLocal, pWeapon, pCmd);
        break;
    default:
        std::cout << "[Aimbot] Unknown weapon type: " << static_cast<int>(eWeaponType) << std::endl;
        break;
    }

    if (bSecondaryType)
        G::CanPrimaryAttack = bOriginal;
}

void CAimbot::RunMain(CTFPlayer* pLocal, CTFWeaponBase* pWeapon, CUserCmd* pCmd)
{
    if (!pLocal || !pWeapon)
    {
        std::cout << "[Aimbot] Invalid local player or weapon!" << std::endl;
        return;
    }

    if (F::AimbotProjectile.m_iLastTickCancel)
    {
        pCmd->weaponselect = F::AimbotProjectile.m_iLastTickCancel;
        F::AimbotProjectile.m_iLastTickCancel = 0;
        std::cout << "[Aimbot] Weapon switched to: " << pCmd->weaponselect << std::endl;
    }

    m_bRan = false;
    if (abs(G::AimTarget.m_iTickCount - I::GlobalVars->tickcount) > G::AimTarget.m_iDuration)
        G::AimTarget = {};
    if (abs(G::AimPoint.m_iTickCount - I::GlobalVars->tickcount) > G::AimPoint.m_iDuration)
        G::AimPoint = {};

    if (pCmd->weaponselect)
    {
        std::cout << "[Aimbot] Weapon select active, skipping aimbot." << std::endl;
        return;
    }

    F::AutoRocketJump.Run(pLocal, pWeapon, pCmd);
    if (!ShouldRun(pLocal, pWeapon))
    {
        std::cout << "[Aimbot] ShouldRun returned false, skipping." << std::endl;
        return;
    }

    F::AutoDetonate.Run(pLocal, pWeapon, pCmd);
    F::AutoAirblast.Run(pLocal, pWeapon, pCmd);
    F::AutoHeal.Run(pLocal, pWeapon, pCmd);

    RunAimbot(pLocal, pWeapon, pCmd);
    RunAimbot(pLocal, pWeapon, pCmd, true);
}

void CAimbot::Run(CTFPlayer* pLocal, CTFWeaponBase* pWeapon, CUserCmd* pCmd)
{
    RunMain(pLocal, pWeapon, pCmd);
    G::Attacking = SDK::IsAttacking(pLocal, pWeapon, pCmd, true);
}

void CAimbot::Draw(CTFPlayer* pLocal)
{
    if (!Vars::Aimbot::General::FOVCircle.Value || !Vars::Colors::FOVCircle.Value.a || !pLocal->CanAttack(false))
        return;

    auto pWeapon = H::Entities.GetWeapon();
    if (pWeapon && !SDK::AttribHookValue(1, "mult_dmg", pWeapon))
        return;

    float aimFOV = std::clamp(Vars::Aimbot::General::AimFOV.Value, 0.f, 89.9f); // Prevent invalid FOV
    if (aimFOV >= 90.f)
        return;

    float flW = H::Draw.m_nScreenW, flH = H::Draw.m_nScreenH;
    float flAspectRatio = flW / flH;
    float flFOV = pLocal->m_iFOV() > 0 ? pLocal->m_iFOV() : 90.f; // Fallback to 90 if FOV is invalid
    float flRadius = tanf(DEG2RAD(aimFOV)) / tanf(DEG2RAD(flFOV) / 2) * flW * (flAspectRatio / (16.f / 9.f));
    H::Draw.LineCircle(flW / 2, flH / 2, flRadius, 68, Vars::Colors::FOVCircle.Value);
}

void CAimbot::Store(CBaseEntity* pEntity, size_t iSize)
{
    if (!Vars::Visuals::Simulation::RealPath.Value)
        return;

    if (pEntity)
    {
        auto pResource = H::Entities.GetPR();
        if (pEntity->IsPlayer() && pResource && pEntity->entindex() > 0)
        {
            m_tPath = { { pEntity->m_vecOrigin() }, I::GlobalVars->curtime + Vars::Visuals::Simulation::DrawDuration.Value, Vars::Colors::RealPath.Value, Vars::Visuals::Simulation::RealPath.Value };
            m_iSize = iSize;
            m_iPlayer = pResource->m_iUserID(pEntity->entindex());
            std::cout << "[Aimbot] Storing path for player: " << m_iPlayer << std::endl;
        }
        return;
    }

    if (!m_tPath.m_flTime)
        return;

    if (m_tPath.m_vPath.size() >= m_iSize || m_tPath.m_flTime < I::GlobalVars->curtime)
    {
        G::PathStorage.push_back(m_tPath);
        std::cout << "[Aimbot] Path stored, size: " << m_tPath.m_vPath.size() << std::endl;
        m_tPath = {};
        return;
    }

    auto pPlayer = I::ClientEntityList->GetClientEntity(I::EngineClient->GetPlayerForUserID(m_iPlayer))->As<CTFPlayer>();
    if (!pPlayer || !pPlayer->IsAlive())
    {
        G::PathStorage.push_back(m_tPath);
        std::cout << "[Aimbot] Player invalid or dead, storing path." << std::endl;
        m_tPath = {};
        return;
    }

    static int iStaticTickcount = I::GlobalVars->tickcount;
    int iLag = I::GlobalVars->tickcount - iStaticTickcount;
    iStaticTickcount = I::GlobalVars->tickcount;

    for (int i = 0; i < iLag; i++)
        m_tPath.m_vPath.push_back(pPlayer->m_vecOrigin());
}
