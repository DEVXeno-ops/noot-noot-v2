#pragma once
#include "../../SDK/SDK.h"

// Structure to store path data for visualization
struct DrawPath_t {
    std::vector<Vec3> m_vPath; // Path points
    float m_flTime;            // Expiration time
    Color_t m_Color;           // Path color
    bool m_bEnabled;           // Whether path drawing is enabled
};

class CAimbot
{
private:
    // Check if aimbot should run based on player and weapon state
    bool ShouldRun(CTFPlayer* pLocal, CTFWeaponBase* pWeapon);

    // Main aimbot logic, coordinating various features
    void RunMain(CTFPlayer* pLocal, CTFWeaponBase* pWeapon, CUserCmd* pCmd);

    // Run aimbot for specific weapon type (primary or secondary)
    void RunAimbot(CTFPlayer* pLocal, CTFWeaponBase* pWeapon, CUserCmd* pCmd, bool bSecondaryType = false);

    size_t m_iSize = 0;    // Maximum size of path storage
    int m_iPlayer = 0;     // UserID of the tracked player

public:
    // Entry point to run the aimbot system
    void Run(CTFPlayer* pLocal, CTFWeaponBase* pWeapon, CUserCmd* pCmd);

    // Draw visual elements like FOV circle
    void Draw(CTFPlayer* pLocal);

    // Store player movement path for visualization
    void Store(CBaseEntity* pEntity = nullptr, size_t iSize = 0);

    bool m_bRan = false;            // Flag indicating if aimbot ran this frame
    bool m_bRunningSecondary = false; // Flag for secondary weapon mode
    DrawPath_t m_tPath = {};        // Current path being tracked
};

ADD_FEATURE(CAimbot, Aimbot);
