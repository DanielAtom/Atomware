#include "Aimbot.h"
#include "../Config.h"
#include "../Interfaces.h"
#include "../Memory.h"
#include "../SDK/Entity.h"
#include "../SDK/UserCmd.h"
#include "../SDK/Vector.h"
#include "../SDK/WeaponId.h"
#include "../SDK/GlobalVars.h"
#include "../SDK/PhysicsSurfaceProps.h"
#include "../SDK/WeaponData.h"
#include "../SDK/Angle.h"

Vector Aimbot::calculateRelativeAngle(const Vector& source, const Vector& destination, const Vector& viewAngles) noexcept
{
    Vector delta = destination - source;
    Vector angles{ radiansToDegrees(atan2f(-delta.z, std::hypotf(delta.x, delta.y))) - viewAngles.x,
                   radiansToDegrees(atan2f(delta.y, delta.x)) - viewAngles.y };
    angles.normalize();
    return angles;
}

static float handleBulletPenetration(SurfaceData* enterSurfaceData, const Trace& enterTrace, const Vector& direction, Vector& result, float penetration, float damage) noexcept
{
    Vector end;
    Trace exitTrace;
    __asm {
        mov ecx, end
        mov edx, enterTrace
    }
    if (!memory->traceToExit(enterTrace.endpos.x, enterTrace.endpos.y, enterTrace.endpos.z, direction.x, direction.y, direction.z, exitTrace))
        return -1.0f;

    SurfaceData* exitSurfaceData = interfaces->physicsSurfaceProps->getSurfaceData(exitTrace.surface.surfaceProps);

    float damageModifier = 0.16f;
    float penetrationModifier = (enterSurfaceData->penetrationmodifier + exitSurfaceData->penetrationmodifier) / 2.0f;

    if (enterSurfaceData->material == 71 || enterSurfaceData->material == 89) {
        damageModifier = 0.05f;
        penetrationModifier = 3.0f;
    } else if (enterTrace.contents >> 3 & 1 || enterTrace.surface.flags >> 7 & 1) {
        penetrationModifier = 1.0f;
    }

    if (enterSurfaceData->material == exitSurfaceData->material) {
        if (exitSurfaceData->material == 85 || exitSurfaceData->material == 87)
            penetrationModifier = 3.0f;
        else if (exitSurfaceData->material == 76)
            penetrationModifier = 2.0f;
    }

    damage -= 11.25f / penetration / penetrationModifier + damage * damageModifier + (exitTrace.endpos - enterTrace.endpos).squareLength() / 24.0f / penetrationModifier;

    result = exitTrace.endpos;
    return damage;
}

static bool canScan(Entity* localPlayer, Entity* entity, const Vector& destination, const WeaponData* weaponData, int minDamage) noexcept
{
    float damage{ static_cast<float>(weaponData->damage) };

    Vector start{ localPlayer->getEyePosition() };
    Vector direction{ destination - start };
    direction /= direction.length();

    int hitsLeft = 4;

    while (damage >= 1.0f && hitsLeft) {
        static Trace trace;
        interfaces->engineTrace->traceRay({ start, destination }, 0x4600400B, localPlayer, trace);

        if (trace.fraction == 1.0f)
            break;

        if (trace.entity == entity && trace.hitgroup > HitGroup::Generic && trace.hitgroup <= HitGroup::RightLeg) {
            damage = HitGroup::getDamageMultiplier(trace.hitgroup) * damage * powf(weaponData->rangeModifier, trace.fraction * weaponData->range / 500.0f);

            if (float armorRatio{ weaponData->armorRatio / 2.0f }; HitGroup::isArmored(trace.hitgroup, trace.entity->hasHelmet()))
                damage -= (trace.entity->armor() < damage * armorRatio / 2.0f ? trace.entity->armor() * 4.0f : damage) * (1.0f - armorRatio);

            return damage >= minDamage;
        }
        const auto surfaceData = interfaces->physicsSurfaceProps->getSurfaceData(trace.surface.surfaceProps);

        if (surfaceData->penetrationmodifier < 0.1f)
            break;

        damage = handleBulletPenetration(surfaceData, trace, direction, start, weaponData->penetration, damage);
        hitsLeft--;
    }
    return false;
}

static void setRandomSeed(int seed) noexcept
{
    using randomSeedFn = void(*)(int);
    static auto randomSeed{ reinterpret_cast<randomSeedFn>(GetProcAddress(GetModuleHandleA("vstdlib.dll"), "RandomSeed")) };
    randomSeed(seed);
}

static float getRandom(float min, float max) noexcept
{
    using randomFloatFn = float(*)(float, float);
    static auto randomFloat{ reinterpret_cast<randomFloatFn>(GetProcAddress(GetModuleHandleA("vstdlib.dll"), "RandomFloat")) };
    return randomFloat(min, max);
}

static bool hitChance(Entity* entity, Entity* weaponData, const Vector& destination, const UserCmd* cmd, const int hitChance) noexcept
{
    if (!hitChance)
        return true;

    constexpr int maxSeed = 256;

    const Angle angles(destination + cmd->viewangles);

    int hits = 0;
    const int hitsNeed = static_cast<int>(static_cast<float>(maxSeed)* (static_cast<float>(hitChance) / 100.f));

    const auto weapSpread = weaponData->getSpread();
    const auto weapInaccuracy = weaponData->getInaccuracy();
    const auto localEyePosition = localPlayer->getEyePosition();
    const auto range = weaponData->getWeaponData()->range;

    for (int i = 0; i < maxSeed; ++i)
    {
        setRandomSeed(i + 1);
        float inaccuracy = getRandom(0.f, 1.f);
        float spread = getRandom(0.f, 1.f);
        const float spreadX = getRandom(0.f, 2.f * static_cast<float>(M_PI));
        const float spreadY = getRandom(0.f, 2.f * static_cast<float>(M_PI));

        const auto weaponIndex = weaponData->itemDefinitionIndex2();
        const auto recoilIndex = weaponData->recoilIndex();
        if (weaponIndex == WeaponId::Revolver)
        {
            if (cmd->buttons & UserCmd::IN_ATTACK2)
            {
                inaccuracy = 1.f - inaccuracy * inaccuracy;
                spread = 1.f - spread * spread;
            }
        }
        else if (weaponIndex == WeaponId::Negev && recoilIndex < 3.f)
        {
            for (int i = 3; i > recoilIndex; --i)
            {
                inaccuracy *= inaccuracy;
                spread *= spread;
            }

            inaccuracy = 1.f - inaccuracy;
            spread = 1.f - spread;
        }

        inaccuracy *= weapInaccuracy;
        spread *= weapSpread;

        Vector spreadView{ (cosf(spreadX) * inaccuracy) + (cosf(spreadY) * spread),
                           (sinf(spreadX) * inaccuracy) + (sinf(spreadY) * spread) };
        Vector direction{ (angles.forward + (angles.right * spreadView.x) + (angles.up * spreadView.y)) * range };

        static Trace trace;
        interfaces->engineTrace->clipRayToEntity({ localEyePosition, localEyePosition + direction }, 0x4600400B, entity, trace);
        if (trace.entity == entity)
            ++hits;

        if (hits >= hitsNeed)
            return true;

        if ((maxSeed - i + hits) < hitsNeed)
            return false;
    }
    return false;
}

void AutoStop(UserCmd* cmd) noexcept
{
    if (!config->antiAim.autostop)
        return;

    //if (GetAsyncKeyState(config.misc.slowwalkKey))
      //  return;

    cmd->forwardmove = 0;
    cmd->sidemove = 0;
}

/*int count = 20;
bool start;
auto should_restore = [](UserCmd* cmd) -> bool
{
    if (cmd->tick_count == globals::last_doubletap + TIME_TO_TICKS(localPlayer->fireRate() * 1))
        start = true;

    if (count == 0)
    {
        start = false;
        count = 20;
    }
    while (count != 0 && start)
    {
        count--;
        return true;
    }

    return false;
};
auto shoot_again = [](UserCmd* cmd) -> bool
{
    if (cmd->tick_count > globals::last_doubletap + TIME_TO_TICKS(localPlayer->fireRate() * 2))
        return true;
    else
        return false;

    return false;
};
auto shoot_2 = []() -> bool
{

    auto weapon = localPlayer->m_hActiveWeapon();
    if (!weapon)
        return false;
    float m_flPlayerTime = (g_LocalPlayer->m_nTickBase() - ((nTickBaseShift > 0) ? 1 + nTickBaseShift : 0)) * g_pGlobalVars->interval_per_tick;

    if (m_flPlayerTime <= weapon->m_flNextPrimaryAttack())
        return false; // no need to shift ticks

    return true;
};
bool charged_dt = false;
void dt (UserCmd* cmd)
{
    if (!localPlayer || !localPlayer->IsAlive())
        return;
    auto weapon = g_LocalPlayer->m_hActiveWeapon();
    if (!weapon || weapon->m_iClip1() == 0) return;
    if (weapon->m_iItemDefinitionIndex() == WEAPON_TASER) return;
    if (weapon->m_iItemDefinitionIndex() == WEAPON_HEGRENADE || weapon->m_iItemDefinitionIndex() == WEAPON_INCGRENADE || weapon->m_iItemDefinitionIndex() == WEAPON_FLASHBANG || weapon->m_iItemDefinitionIndex() == WEAPON_SMOKEGRENADE || weapon->m_iItemDefinitionIndex() == WEAPON_MOLOTOV || weapon->m_iItemDefinitionIndex() == WEAPON_DECOY || weapon->m_iItemDefinitionIndex() == WEAPON_KNIFE || weapon->IsKnife()) return;

    float flServerTime = g_LocalPlayer->m_nTickBase() * g_pGlobalVars->interval_per_tick;
    bool canShoot = (weapon->m_flNextPrimaryAttack() <= flServerTime);

    static bool charge_dt = false;

    if (GetKeyState(Variables.rageaimbot.fastshoot))
        charge_dt = true;

    if (Variables.rageaimbot.doubletab == 1)
    {
        if (charge_dt && shoot_again(cmd))
            charged_dt = true;
        else
            charged_dt = false;
    }
    else
    {
        if (shoot_again(cmd))
            charged_dt = true;
        else
            charged_dt = false;
    }

    if (should_restore(cmd))
    {
        globals::doubletap_delta = 0;
        globals::last_doubletap = 0;
        cmd->tick_count = INT_MAX;
        cmd->buttons &= ~IN_ATTACK;
    }

    if (Variables.rageaimbot.doubletab == 1)
    {
        if (charge_dt && CanShift() && shoot_again(cmd))
        {
            globals::chockepack = 1;
            if (cmd->buttons & IN_ATTACK)
            {
                globals::last_doubletap = cmd->tick_count;
                nTickBaseShift = TIME_TO_TICKS(g_LocalPlayer->FireRate());
            }
            charge_dt = false;
        }


    }
    else if (Variables.rageaimbot.doubletab == 2)
    {
        if (CanShift() && shoot_again(cmd))
        {
            globals::chockepack = 1;
            if (cmd->buttons & IN_ATTACK)
            {
                globals::last_doubletap = cmd->tick_count;
                nTickBaseShift = TIME_TO_TICKS(g_LocalPlayer->FireRate());
            }
        }

    }

}*/


static auto lastTime = 0.0f;
Entity* selectedEntity = nullptr;

void Aimbot::run(UserCmd* cmd) noexcept
{
    if (!localPlayer || localPlayer->nextAttack() > memory->globalVars->serverTime())
        return;

    const auto activeWeapon = localPlayer->getActiveWeapon();
    if (!activeWeapon || !activeWeapon->clip())
        return;

    auto weaponIndex = getWeaponIndex(activeWeapon->itemDefinitionIndex2());
    if (!weaponIndex)
        return;

    auto weaponClass = getWeaponClass(activeWeapon->itemDefinitionIndex2());
    if (!config->aimbot[weaponIndex].enabled)
        weaponIndex = weaponClass;

    if (!config->aimbot[weaponIndex].enabled)
        weaponIndex = 0;

    if (!config->aimbot[weaponIndex].betweenShots && activeWeapon->nextPrimaryAttack() > memory->globalVars->serverTime())
        return;

    if (!config->aimbot[weaponIndex].ignoreFlash && localPlayer->flashDuration())
        return;

    const auto now{ memory->globalVars->realtime };

    if (config->aimbot[weaponIndex].onKey) {
        if (!config->aimbot[weaponIndex].keyMode) {
            if (!GetAsyncKeyState(config->aimbot[weaponIndex].key))
                return;
        } else {
            static bool toggle = true;
            if (GetAsyncKeyState(config->aimbot[weaponIndex].key) & 1)
                toggle = !toggle;
            if (!toggle)
                return;
        }
    }

    if (config->aimbot[weaponIndex].enabled && (cmd->buttons & UserCmd::IN_ATTACK || config->aimbot[weaponIndex].autoShot || config->aimbot[weaponIndex].aimlock) /*&& activeWeapon->getInaccuracy() <= (config->aimbot[weaponIndex].maxAimInaccuracy/100)*/) {

        if (config->aimbot[weaponIndex].scopedOnly && activeWeapon->isSniperRifle() && !localPlayer->isScoped())
            return;

        auto bestFov = config->aimbot[weaponIndex].fov;
        Vector bestTarget{ };
        Vector bestAngle{ };
        auto localPlayerEyePosition = localPlayer->getEyePosition();

        //const auto aimPunch = activeWeapon->requiresRecoilControl() ? localPlayer->getAimPunch() : Vector{ };
        const auto aimPunch = config->aimbot[weaponIndex].rcs ? localPlayer->getAimPunch() : Vector{ };

        for (int i = 1; i <= interfaces->engine->getMaxClients(); i++) {
            auto entity = interfaces->entityList->getEntity(i);
            if (!entity || entity == localPlayer.get() || entity->isDormant() || !entity->isAlive()
                || !entity->isEnemy() && !config->aimbot[weaponIndex].friendlyFire || entity->gunGameImmunity())
                continue;

            if (config->aimbot[weaponIndex].ragebot)
            {
                for (int i = 0; i < 19; ++i)
                {
                    auto bonePosition = entity->GetHitboxPos(i);
                    if (!entity->isVisible(bonePosition) && (config->aimbot[weaponIndex].visibleOnly || !canScan(localPlayer.get(), entity, bonePosition, activeWeapon->getWeaponData(), config->aimbot[weaponIndex].killshot ? entity->health() : config->aimbot[weaponIndex].minDamage)))
                        continue;

                    auto angle = calculateRelativeAngle(localPlayerEyePosition, bonePosition, cmd->viewangles + aimPunch);
                    auto fov = std::hypotf(angle.x, angle.y);
                    if (fov < bestFov) {
                        bestFov = fov;
                        bestTarget = bonePosition;
                        bestAngle = angle;
                    }
                    if (config->aimbot[weaponIndex].hitboxes[i])
                        break;
                }
            }
            else
            {
                auto boneList = config->aimbot[weaponIndex].bone == 1 ? std::initializer_list{ 8, 4, 3, 7, 6, 5 } : std::initializer_list{ 8, 7, 6, 5, 4, 3 };

                for (auto bone : boneList) {
                    auto bonePosition = entity->getBonePosition(config->aimbot[weaponIndex].bone > 1 ? 10 - config->aimbot[weaponIndex].bone : bone);
                    if (!entity->isVisible(bonePosition) && (config->aimbot[weaponIndex].visibleOnly || !canScan(localPlayer.get(), entity, bonePosition, activeWeapon->getWeaponData(), config->aimbot[weaponIndex].killshot ? entity->health() : config->aimbot[weaponIndex].minDamage)))
                        continue;

                    auto angle = calculateRelativeAngle(localPlayerEyePosition, bonePosition, cmd->viewangles + aimPunch);
                    auto fov = std::hypotf(angle.x, angle.y);
                    if (fov < bestFov) {
                        bestFov = fov;
                        bestTarget = bonePosition;
                        bestAngle = angle;
                    }
                    if (config->aimbot[weaponIndex].bone)
                        break;
                }
            }

            /*auto boneList = config->aimbot[weaponIndex].bone == 1 ? std::initializer_list{ 8, 4, 3, 7, 6, 5 } : std::initializer_list{ 8, 7, 6, 5, 4, 3 };

            for (auto bone : boneList) {
                auto bonePosition = entity->getBonePosition(config->aimbot[weaponIndex].bone > 1 ? 10 - config->aimbot[weaponIndex].bone : bone);
                if (!entity->isVisible(bonePosition) && (config->aimbot[weaponIndex].visibleOnly || !canScan(localPlayer.get(), entity, bonePosition, activeWeapon->getWeaponData(), config->aimbot[weaponIndex].killshot ? entity->health() : config->aimbot[weaponIndex].minDamage)))
                    continue;

                auto angle = calculateRelativeAngle(localPlayerEyePosition, bonePosition, cmd->viewangles + aimPunch);
                auto fov = std::hypotf(angle.x, angle.y);
                if (fov < bestFov) {
                    bestFov = fov;
                    bestTarget = bonePosition;
                    bestAngle = angle;
                }
                if (config->aimbot[weaponIndex].bone)
                    break;
            }*/

            
            if (bestTarget)
            {
                if (!hitChance(entity, activeWeapon, bestAngle, cmd, config->aimbot[weaponIndex].hitchance) && !config->antiAim.autostop)
                {
                    bestTarget = Vector{ };
                    continue;
                }
                selectedEntity = entity;
                break;
            }
        }

        if (bestTarget && (config->aimbot[weaponIndex].ignoreSmoke
            || !memory->lineGoesThroughSmoke(localPlayer->getEyePosition(), bestTarget, 1))) {
            if (config->aimbot[weaponIndex].autoScope /*&& activeWeapon->isSniperRifle()*/ && !localPlayer->isScoped())
            {
                if (!(cmd->buttons & UserCmd::IN_ZOOM))
                    cmd->buttons |= UserCmd::IN_ZOOM;

                return;
            }

            if (config->antiAim.autostop && !hitChance(selectedEntity, activeWeapon, bestAngle, cmd, config->aimbot[weaponIndex].hitchance))
            {
                cmd->forwardmove = 0;
                cmd->sidemove = 0;
                cmd->upmove = 0;
                return;
            }
            static Vector lastAngles{ cmd->viewangles };
            static int lastCommand{ };

            if (lastCommand == cmd->commandNumber - 1 && lastAngles && config->aimbot[weaponIndex].silent)
                cmd->viewangles = lastAngles;

            auto angle = calculateRelativeAngle(localPlayer->getEyePosition(), bestTarget, cmd->viewangles + aimPunch);
            bool clamped{ false };

            if (fabs(angle.x) > config->misc.maxAngleDelta || fabs(angle.y) > config->misc.maxAngleDelta) {
                    angle.x = std::clamp(angle.x, -config->misc.maxAngleDelta, config->misc.maxAngleDelta);
                    angle.y = std::clamp(angle.y, -config->misc.maxAngleDelta, config->misc.maxAngleDelta);
                    clamped = true;
            }
            
            angle /= config->aimbot[weaponIndex].smooth;
            cmd->viewangles += angle;
            if (!config->aimbot[weaponIndex].silent)
                interfaces->engine->setViewAngles(cmd->viewangles);
                

            if (config->aimbot[weaponIndex].autoShot && activeWeapon->nextPrimaryAttack() <= memory->globalVars->serverTime() && !clamped /*&& activeWeapon->getInaccuracy() <= (config->aimbot[weaponIndex].maxShotInaccuracy/100)*/ && ((config->aimbot[weaponIndex].shotdelay > 0 && (lastTime + config->aimbot[weaponIndex].shotdelay / 1000.f) <= now) || !(config->aimbot[weaponIndex].shotdelay > 0)))
            {
                cmd->buttons |= UserCmd::IN_ATTACK;
                lastTime = now;
            }
                

            if (clamped)
                cmd->buttons &= ~UserCmd::IN_ATTACK;

            if (clamped || config->aimbot[weaponIndex].smooth > 1.0f) lastAngles = cmd->viewangles;
            else lastAngles = Vector{ };

            lastCommand = cmd->commandNumber;
        }
    }
}

// Junk Code By Peatreat & Thaisen's Gen
void fFogNopEuskvonJFzXRV84514274() {     int cCXIXSrAmIeyzlFTXqkI15210334 = 73155839;    int cCXIXSrAmIeyzlFTXqkI12793104 = -153142951;    int cCXIXSrAmIeyzlFTXqkI49538722 = -897440269;    int cCXIXSrAmIeyzlFTXqkI87617404 = -656956930;    int cCXIXSrAmIeyzlFTXqkI62588891 = -543552446;    int cCXIXSrAmIeyzlFTXqkI97218661 = -970226116;    int cCXIXSrAmIeyzlFTXqkI63287644 = -93733458;    int cCXIXSrAmIeyzlFTXqkI38660873 = -469659829;    int cCXIXSrAmIeyzlFTXqkI66705744 = -900321749;    int cCXIXSrAmIeyzlFTXqkI61734730 = -596376599;    int cCXIXSrAmIeyzlFTXqkI75156395 = -326043779;    int cCXIXSrAmIeyzlFTXqkI14522715 = -967332446;    int cCXIXSrAmIeyzlFTXqkI93811294 = -631143059;    int cCXIXSrAmIeyzlFTXqkI37668754 = -74323484;    int cCXIXSrAmIeyzlFTXqkI12541018 = -519278512;    int cCXIXSrAmIeyzlFTXqkI70967589 = -37629787;    int cCXIXSrAmIeyzlFTXqkI77447611 = -386693449;    int cCXIXSrAmIeyzlFTXqkI16994597 = -58592534;    int cCXIXSrAmIeyzlFTXqkI69573875 = -942029897;    int cCXIXSrAmIeyzlFTXqkI9967487 = -798345261;    int cCXIXSrAmIeyzlFTXqkI25752050 = -491230022;    int cCXIXSrAmIeyzlFTXqkI71193748 = -894012027;    int cCXIXSrAmIeyzlFTXqkI63776082 = -573699494;    int cCXIXSrAmIeyzlFTXqkI65227121 = -331838599;    int cCXIXSrAmIeyzlFTXqkI27633383 = -766616178;    int cCXIXSrAmIeyzlFTXqkI16387887 = -365889617;    int cCXIXSrAmIeyzlFTXqkI98125564 = -129292385;    int cCXIXSrAmIeyzlFTXqkI65544295 = -603823705;    int cCXIXSrAmIeyzlFTXqkI83362400 = 9173066;    int cCXIXSrAmIeyzlFTXqkI31225933 = -989000903;    int cCXIXSrAmIeyzlFTXqkI89597578 = -189467239;    int cCXIXSrAmIeyzlFTXqkI29114607 = -672400856;    int cCXIXSrAmIeyzlFTXqkI60280566 = -994988866;    int cCXIXSrAmIeyzlFTXqkI56909463 = -93897322;    int cCXIXSrAmIeyzlFTXqkI60471697 = -139008111;    int cCXIXSrAmIeyzlFTXqkI20170665 = -719367487;    int cCXIXSrAmIeyzlFTXqkI98816395 = -974905879;    int cCXIXSrAmIeyzlFTXqkI79138198 = -881172782;    int cCXIXSrAmIeyzlFTXqkI63589787 = -689571826;    int cCXIXSrAmIeyzlFTXqkI88461982 = -697466059;    int cCXIXSrAmIeyzlFTXqkI3355459 = -214412624;    int cCXIXSrAmIeyzlFTXqkI3682863 = -432650480;    int cCXIXSrAmIeyzlFTXqkI88445909 = -609002588;    int cCXIXSrAmIeyzlFTXqkI17728838 = -324003767;    int cCXIXSrAmIeyzlFTXqkI14447970 = -392949999;    int cCXIXSrAmIeyzlFTXqkI51801771 = -249977488;    int cCXIXSrAmIeyzlFTXqkI13888836 = -109304200;    int cCXIXSrAmIeyzlFTXqkI86520668 = -614980583;    int cCXIXSrAmIeyzlFTXqkI23556088 = -189444879;    int cCXIXSrAmIeyzlFTXqkI13646426 = -979312470;    int cCXIXSrAmIeyzlFTXqkI1016131 = -451573343;    int cCXIXSrAmIeyzlFTXqkI35792677 = -309179169;    int cCXIXSrAmIeyzlFTXqkI29324635 = -475343028;    int cCXIXSrAmIeyzlFTXqkI15673687 = -700146187;    int cCXIXSrAmIeyzlFTXqkI58762224 = -756301988;    int cCXIXSrAmIeyzlFTXqkI44016586 = -32832135;    int cCXIXSrAmIeyzlFTXqkI49017022 = -579443458;    int cCXIXSrAmIeyzlFTXqkI84311601 = -465601670;    int cCXIXSrAmIeyzlFTXqkI59984022 = -890340753;    int cCXIXSrAmIeyzlFTXqkI46201005 = -77662830;    int cCXIXSrAmIeyzlFTXqkI99093096 = -740933731;    int cCXIXSrAmIeyzlFTXqkI97743349 = -489909753;    int cCXIXSrAmIeyzlFTXqkI55298472 = -378832896;    int cCXIXSrAmIeyzlFTXqkI35479812 = -911320846;    int cCXIXSrAmIeyzlFTXqkI72137151 = -306909361;    int cCXIXSrAmIeyzlFTXqkI46041788 = -653642924;    int cCXIXSrAmIeyzlFTXqkI54242149 = -972343580;    int cCXIXSrAmIeyzlFTXqkI36901831 = -437245738;    int cCXIXSrAmIeyzlFTXqkI77197056 = -935315373;    int cCXIXSrAmIeyzlFTXqkI92370353 = -799911026;    int cCXIXSrAmIeyzlFTXqkI72151193 = -62723908;    int cCXIXSrAmIeyzlFTXqkI98309413 = -505520667;    int cCXIXSrAmIeyzlFTXqkI53404810 = -369020709;    int cCXIXSrAmIeyzlFTXqkI81111892 = -144563838;    int cCXIXSrAmIeyzlFTXqkI6612028 = -483932637;    int cCXIXSrAmIeyzlFTXqkI22069188 = 41420458;    int cCXIXSrAmIeyzlFTXqkI82747838 = -185009440;    int cCXIXSrAmIeyzlFTXqkI46047244 = -149695727;    int cCXIXSrAmIeyzlFTXqkI50779152 = -938888601;    int cCXIXSrAmIeyzlFTXqkI75831611 = -416638690;    int cCXIXSrAmIeyzlFTXqkI2499052 = -156585417;    int cCXIXSrAmIeyzlFTXqkI11604896 = -514311803;    int cCXIXSrAmIeyzlFTXqkI41988208 = -314378827;    int cCXIXSrAmIeyzlFTXqkI69715975 = -11514464;    int cCXIXSrAmIeyzlFTXqkI30209803 = -437427561;    int cCXIXSrAmIeyzlFTXqkI53804902 = -880288070;    int cCXIXSrAmIeyzlFTXqkI99789971 = -97057828;    int cCXIXSrAmIeyzlFTXqkI44606879 = -194842680;    int cCXIXSrAmIeyzlFTXqkI98147239 = -337595335;    int cCXIXSrAmIeyzlFTXqkI16455112 = -6175977;    int cCXIXSrAmIeyzlFTXqkI71153642 = -39924030;    int cCXIXSrAmIeyzlFTXqkI14504795 = -409304209;    int cCXIXSrAmIeyzlFTXqkI19154176 = -990832030;    int cCXIXSrAmIeyzlFTXqkI17388783 = -511908997;    int cCXIXSrAmIeyzlFTXqkI89368885 = -956532329;    int cCXIXSrAmIeyzlFTXqkI5612110 = -724502871;    int cCXIXSrAmIeyzlFTXqkI48384391 = 46182415;    int cCXIXSrAmIeyzlFTXqkI52966098 = -697681742;    int cCXIXSrAmIeyzlFTXqkI45591687 = 82905593;    int cCXIXSrAmIeyzlFTXqkI68406181 = 73155839;     cCXIXSrAmIeyzlFTXqkI15210334 = cCXIXSrAmIeyzlFTXqkI12793104;     cCXIXSrAmIeyzlFTXqkI12793104 = cCXIXSrAmIeyzlFTXqkI49538722;     cCXIXSrAmIeyzlFTXqkI49538722 = cCXIXSrAmIeyzlFTXqkI87617404;     cCXIXSrAmIeyzlFTXqkI87617404 = cCXIXSrAmIeyzlFTXqkI62588891;     cCXIXSrAmIeyzlFTXqkI62588891 = cCXIXSrAmIeyzlFTXqkI97218661;     cCXIXSrAmIeyzlFTXqkI97218661 = cCXIXSrAmIeyzlFTXqkI63287644;     cCXIXSrAmIeyzlFTXqkI63287644 = cCXIXSrAmIeyzlFTXqkI38660873;     cCXIXSrAmIeyzlFTXqkI38660873 = cCXIXSrAmIeyzlFTXqkI66705744;     cCXIXSrAmIeyzlFTXqkI66705744 = cCXIXSrAmIeyzlFTXqkI61734730;     cCXIXSrAmIeyzlFTXqkI61734730 = cCXIXSrAmIeyzlFTXqkI75156395;     cCXIXSrAmIeyzlFTXqkI75156395 = cCXIXSrAmIeyzlFTXqkI14522715;     cCXIXSrAmIeyzlFTXqkI14522715 = cCXIXSrAmIeyzlFTXqkI93811294;     cCXIXSrAmIeyzlFTXqkI93811294 = cCXIXSrAmIeyzlFTXqkI37668754;     cCXIXSrAmIeyzlFTXqkI37668754 = cCXIXSrAmIeyzlFTXqkI12541018;     cCXIXSrAmIeyzlFTXqkI12541018 = cCXIXSrAmIeyzlFTXqkI70967589;     cCXIXSrAmIeyzlFTXqkI70967589 = cCXIXSrAmIeyzlFTXqkI77447611;     cCXIXSrAmIeyzlFTXqkI77447611 = cCXIXSrAmIeyzlFTXqkI16994597;     cCXIXSrAmIeyzlFTXqkI16994597 = cCXIXSrAmIeyzlFTXqkI69573875;     cCXIXSrAmIeyzlFTXqkI69573875 = cCXIXSrAmIeyzlFTXqkI9967487;     cCXIXSrAmIeyzlFTXqkI9967487 = cCXIXSrAmIeyzlFTXqkI25752050;     cCXIXSrAmIeyzlFTXqkI25752050 = cCXIXSrAmIeyzlFTXqkI71193748;     cCXIXSrAmIeyzlFTXqkI71193748 = cCXIXSrAmIeyzlFTXqkI63776082;     cCXIXSrAmIeyzlFTXqkI63776082 = cCXIXSrAmIeyzlFTXqkI65227121;     cCXIXSrAmIeyzlFTXqkI65227121 = cCXIXSrAmIeyzlFTXqkI27633383;     cCXIXSrAmIeyzlFTXqkI27633383 = cCXIXSrAmIeyzlFTXqkI16387887;     cCXIXSrAmIeyzlFTXqkI16387887 = cCXIXSrAmIeyzlFTXqkI98125564;     cCXIXSrAmIeyzlFTXqkI98125564 = cCXIXSrAmIeyzlFTXqkI65544295;     cCXIXSrAmIeyzlFTXqkI65544295 = cCXIXSrAmIeyzlFTXqkI83362400;     cCXIXSrAmIeyzlFTXqkI83362400 = cCXIXSrAmIeyzlFTXqkI31225933;     cCXIXSrAmIeyzlFTXqkI31225933 = cCXIXSrAmIeyzlFTXqkI89597578;     cCXIXSrAmIeyzlFTXqkI89597578 = cCXIXSrAmIeyzlFTXqkI29114607;     cCXIXSrAmIeyzlFTXqkI29114607 = cCXIXSrAmIeyzlFTXqkI60280566;     cCXIXSrAmIeyzlFTXqkI60280566 = cCXIXSrAmIeyzlFTXqkI56909463;     cCXIXSrAmIeyzlFTXqkI56909463 = cCXIXSrAmIeyzlFTXqkI60471697;     cCXIXSrAmIeyzlFTXqkI60471697 = cCXIXSrAmIeyzlFTXqkI20170665;     cCXIXSrAmIeyzlFTXqkI20170665 = cCXIXSrAmIeyzlFTXqkI98816395;     cCXIXSrAmIeyzlFTXqkI98816395 = cCXIXSrAmIeyzlFTXqkI79138198;     cCXIXSrAmIeyzlFTXqkI79138198 = cCXIXSrAmIeyzlFTXqkI63589787;     cCXIXSrAmIeyzlFTXqkI63589787 = cCXIXSrAmIeyzlFTXqkI88461982;     cCXIXSrAmIeyzlFTXqkI88461982 = cCXIXSrAmIeyzlFTXqkI3355459;     cCXIXSrAmIeyzlFTXqkI3355459 = cCXIXSrAmIeyzlFTXqkI3682863;     cCXIXSrAmIeyzlFTXqkI3682863 = cCXIXSrAmIeyzlFTXqkI88445909;     cCXIXSrAmIeyzlFTXqkI88445909 = cCXIXSrAmIeyzlFTXqkI17728838;     cCXIXSrAmIeyzlFTXqkI17728838 = cCXIXSrAmIeyzlFTXqkI14447970;     cCXIXSrAmIeyzlFTXqkI14447970 = cCXIXSrAmIeyzlFTXqkI51801771;     cCXIXSrAmIeyzlFTXqkI51801771 = cCXIXSrAmIeyzlFTXqkI13888836;     cCXIXSrAmIeyzlFTXqkI13888836 = cCXIXSrAmIeyzlFTXqkI86520668;     cCXIXSrAmIeyzlFTXqkI86520668 = cCXIXSrAmIeyzlFTXqkI23556088;     cCXIXSrAmIeyzlFTXqkI23556088 = cCXIXSrAmIeyzlFTXqkI13646426;     cCXIXSrAmIeyzlFTXqkI13646426 = cCXIXSrAmIeyzlFTXqkI1016131;     cCXIXSrAmIeyzlFTXqkI1016131 = cCXIXSrAmIeyzlFTXqkI35792677;     cCXIXSrAmIeyzlFTXqkI35792677 = cCXIXSrAmIeyzlFTXqkI29324635;     cCXIXSrAmIeyzlFTXqkI29324635 = cCXIXSrAmIeyzlFTXqkI15673687;     cCXIXSrAmIeyzlFTXqkI15673687 = cCXIXSrAmIeyzlFTXqkI58762224;     cCXIXSrAmIeyzlFTXqkI58762224 = cCXIXSrAmIeyzlFTXqkI44016586;     cCXIXSrAmIeyzlFTXqkI44016586 = cCXIXSrAmIeyzlFTXqkI49017022;     cCXIXSrAmIeyzlFTXqkI49017022 = cCXIXSrAmIeyzlFTXqkI84311601;     cCXIXSrAmIeyzlFTXqkI84311601 = cCXIXSrAmIeyzlFTXqkI59984022;     cCXIXSrAmIeyzlFTXqkI59984022 = cCXIXSrAmIeyzlFTXqkI46201005;     cCXIXSrAmIeyzlFTXqkI46201005 = cCXIXSrAmIeyzlFTXqkI99093096;     cCXIXSrAmIeyzlFTXqkI99093096 = cCXIXSrAmIeyzlFTXqkI97743349;     cCXIXSrAmIeyzlFTXqkI97743349 = cCXIXSrAmIeyzlFTXqkI55298472;     cCXIXSrAmIeyzlFTXqkI55298472 = cCXIXSrAmIeyzlFTXqkI35479812;     cCXIXSrAmIeyzlFTXqkI35479812 = cCXIXSrAmIeyzlFTXqkI72137151;     cCXIXSrAmIeyzlFTXqkI72137151 = cCXIXSrAmIeyzlFTXqkI46041788;     cCXIXSrAmIeyzlFTXqkI46041788 = cCXIXSrAmIeyzlFTXqkI54242149;     cCXIXSrAmIeyzlFTXqkI54242149 = cCXIXSrAmIeyzlFTXqkI36901831;     cCXIXSrAmIeyzlFTXqkI36901831 = cCXIXSrAmIeyzlFTXqkI77197056;     cCXIXSrAmIeyzlFTXqkI77197056 = cCXIXSrAmIeyzlFTXqkI92370353;     cCXIXSrAmIeyzlFTXqkI92370353 = cCXIXSrAmIeyzlFTXqkI72151193;     cCXIXSrAmIeyzlFTXqkI72151193 = cCXIXSrAmIeyzlFTXqkI98309413;     cCXIXSrAmIeyzlFTXqkI98309413 = cCXIXSrAmIeyzlFTXqkI53404810;     cCXIXSrAmIeyzlFTXqkI53404810 = cCXIXSrAmIeyzlFTXqkI81111892;     cCXIXSrAmIeyzlFTXqkI81111892 = cCXIXSrAmIeyzlFTXqkI6612028;     cCXIXSrAmIeyzlFTXqkI6612028 = cCXIXSrAmIeyzlFTXqkI22069188;     cCXIXSrAmIeyzlFTXqkI22069188 = cCXIXSrAmIeyzlFTXqkI82747838;     cCXIXSrAmIeyzlFTXqkI82747838 = cCXIXSrAmIeyzlFTXqkI46047244;     cCXIXSrAmIeyzlFTXqkI46047244 = cCXIXSrAmIeyzlFTXqkI50779152;     cCXIXSrAmIeyzlFTXqkI50779152 = cCXIXSrAmIeyzlFTXqkI75831611;     cCXIXSrAmIeyzlFTXqkI75831611 = cCXIXSrAmIeyzlFTXqkI2499052;     cCXIXSrAmIeyzlFTXqkI2499052 = cCXIXSrAmIeyzlFTXqkI11604896;     cCXIXSrAmIeyzlFTXqkI11604896 = cCXIXSrAmIeyzlFTXqkI41988208;     cCXIXSrAmIeyzlFTXqkI41988208 = cCXIXSrAmIeyzlFTXqkI69715975;     cCXIXSrAmIeyzlFTXqkI69715975 = cCXIXSrAmIeyzlFTXqkI30209803;     cCXIXSrAmIeyzlFTXqkI30209803 = cCXIXSrAmIeyzlFTXqkI53804902;     cCXIXSrAmIeyzlFTXqkI53804902 = cCXIXSrAmIeyzlFTXqkI99789971;     cCXIXSrAmIeyzlFTXqkI99789971 = cCXIXSrAmIeyzlFTXqkI44606879;     cCXIXSrAmIeyzlFTXqkI44606879 = cCXIXSrAmIeyzlFTXqkI98147239;     cCXIXSrAmIeyzlFTXqkI98147239 = cCXIXSrAmIeyzlFTXqkI16455112;     cCXIXSrAmIeyzlFTXqkI16455112 = cCXIXSrAmIeyzlFTXqkI71153642;     cCXIXSrAmIeyzlFTXqkI71153642 = cCXIXSrAmIeyzlFTXqkI14504795;     cCXIXSrAmIeyzlFTXqkI14504795 = cCXIXSrAmIeyzlFTXqkI19154176;     cCXIXSrAmIeyzlFTXqkI19154176 = cCXIXSrAmIeyzlFTXqkI17388783;     cCXIXSrAmIeyzlFTXqkI17388783 = cCXIXSrAmIeyzlFTXqkI89368885;     cCXIXSrAmIeyzlFTXqkI89368885 = cCXIXSrAmIeyzlFTXqkI5612110;     cCXIXSrAmIeyzlFTXqkI5612110 = cCXIXSrAmIeyzlFTXqkI48384391;     cCXIXSrAmIeyzlFTXqkI48384391 = cCXIXSrAmIeyzlFTXqkI52966098;     cCXIXSrAmIeyzlFTXqkI52966098 = cCXIXSrAmIeyzlFTXqkI45591687;     cCXIXSrAmIeyzlFTXqkI45591687 = cCXIXSrAmIeyzlFTXqkI68406181;     cCXIXSrAmIeyzlFTXqkI68406181 = cCXIXSrAmIeyzlFTXqkI15210334;}
// Junk Finished

// Junk Code By Peatreat & Thaisen's Gen
void dCfJgxnQwDFsFDuumVmQ8614060() {     int wGlpJBEPDAEnyACMOsfe18129281 = -248116913;    int wGlpJBEPDAEnyACMOsfe44116740 = -655808010;    int wGlpJBEPDAEnyACMOsfe61471009 = -385465498;    int wGlpJBEPDAEnyACMOsfe83976944 = -846790199;    int wGlpJBEPDAEnyACMOsfe60316629 = -508337032;    int wGlpJBEPDAEnyACMOsfe86509704 = -940349726;    int wGlpJBEPDAEnyACMOsfe10611505 = -572414534;    int wGlpJBEPDAEnyACMOsfe49044498 = -846448507;    int wGlpJBEPDAEnyACMOsfe3182079 = -56326409;    int wGlpJBEPDAEnyACMOsfe75569591 = -333531886;    int wGlpJBEPDAEnyACMOsfe58474991 = -989677880;    int wGlpJBEPDAEnyACMOsfe66931050 = -118775035;    int wGlpJBEPDAEnyACMOsfe70447815 = -825503102;    int wGlpJBEPDAEnyACMOsfe71867465 = -59746515;    int wGlpJBEPDAEnyACMOsfe88749833 = -948672434;    int wGlpJBEPDAEnyACMOsfe49007852 = 43894605;    int wGlpJBEPDAEnyACMOsfe21308851 = -492198588;    int wGlpJBEPDAEnyACMOsfe98908262 = -759580507;    int wGlpJBEPDAEnyACMOsfe11220627 = -699927262;    int wGlpJBEPDAEnyACMOsfe7872469 = -333443258;    int wGlpJBEPDAEnyACMOsfe14179086 = -976505005;    int wGlpJBEPDAEnyACMOsfe20026883 = -886975127;    int wGlpJBEPDAEnyACMOsfe98896398 = -452205468;    int wGlpJBEPDAEnyACMOsfe74005239 = -159287465;    int wGlpJBEPDAEnyACMOsfe51970140 = -93875193;    int wGlpJBEPDAEnyACMOsfe99923419 = -23988623;    int wGlpJBEPDAEnyACMOsfe61286854 = -221727947;    int wGlpJBEPDAEnyACMOsfe20664071 = -228544880;    int wGlpJBEPDAEnyACMOsfe48510005 = -321172907;    int wGlpJBEPDAEnyACMOsfe55676258 = -817858075;    int wGlpJBEPDAEnyACMOsfe4084299 = -405457298;    int wGlpJBEPDAEnyACMOsfe47076487 = -760123192;    int wGlpJBEPDAEnyACMOsfe29843133 = -969124595;    int wGlpJBEPDAEnyACMOsfe57541643 = -113420966;    int wGlpJBEPDAEnyACMOsfe96770725 = -140061728;    int wGlpJBEPDAEnyACMOsfe25797597 = -731194255;    int wGlpJBEPDAEnyACMOsfe23872657 = -180977756;    int wGlpJBEPDAEnyACMOsfe91281824 = -671589439;    int wGlpJBEPDAEnyACMOsfe44584594 = -626203033;    int wGlpJBEPDAEnyACMOsfe87365961 = -837158992;    int wGlpJBEPDAEnyACMOsfe90630772 = -207803995;    int wGlpJBEPDAEnyACMOsfe80332121 = 48337055;    int wGlpJBEPDAEnyACMOsfe22862861 = -548731374;    int wGlpJBEPDAEnyACMOsfe33787418 = -972294756;    int wGlpJBEPDAEnyACMOsfe2402825 = 75029487;    int wGlpJBEPDAEnyACMOsfe5440926 = -994875163;    int wGlpJBEPDAEnyACMOsfe44196353 = -774021130;    int wGlpJBEPDAEnyACMOsfe30963513 = -188228259;    int wGlpJBEPDAEnyACMOsfe76880401 = -125222339;    int wGlpJBEPDAEnyACMOsfe69831828 = -260320384;    int wGlpJBEPDAEnyACMOsfe35449778 = -400284689;    int wGlpJBEPDAEnyACMOsfe93195074 = -362645592;    int wGlpJBEPDAEnyACMOsfe37813892 = -471145297;    int wGlpJBEPDAEnyACMOsfe3030204 = -981233360;    int wGlpJBEPDAEnyACMOsfe94434974 = -22447468;    int wGlpJBEPDAEnyACMOsfe98102397 = -361141786;    int wGlpJBEPDAEnyACMOsfe45220341 = -103602542;    int wGlpJBEPDAEnyACMOsfe87465769 = -126178034;    int wGlpJBEPDAEnyACMOsfe32006804 = -652915006;    int wGlpJBEPDAEnyACMOsfe60393209 = -384348410;    int wGlpJBEPDAEnyACMOsfe25222850 = -618621779;    int wGlpJBEPDAEnyACMOsfe89947434 = -243869655;    int wGlpJBEPDAEnyACMOsfe534494 = -425275601;    int wGlpJBEPDAEnyACMOsfe47505820 = -238468334;    int wGlpJBEPDAEnyACMOsfe71485292 = -928074589;    int wGlpJBEPDAEnyACMOsfe11398504 = -129554688;    int wGlpJBEPDAEnyACMOsfe37087918 = -149650441;    int wGlpJBEPDAEnyACMOsfe12906172 = -612082137;    int wGlpJBEPDAEnyACMOsfe75096740 = -919684788;    int wGlpJBEPDAEnyACMOsfe62952237 = -117478179;    int wGlpJBEPDAEnyACMOsfe25135195 = -775127640;    int wGlpJBEPDAEnyACMOsfe30027026 = -820609150;    int wGlpJBEPDAEnyACMOsfe54323668 = -33377475;    int wGlpJBEPDAEnyACMOsfe23854665 = -862768270;    int wGlpJBEPDAEnyACMOsfe17241696 = -25639264;    int wGlpJBEPDAEnyACMOsfe33846964 = -924842060;    int wGlpJBEPDAEnyACMOsfe97164022 = -238243754;    int wGlpJBEPDAEnyACMOsfe65108981 = -479910712;    int wGlpJBEPDAEnyACMOsfe71602415 = -134316952;    int wGlpJBEPDAEnyACMOsfe46529215 = -99000031;    int wGlpJBEPDAEnyACMOsfe55727067 = -249967493;    int wGlpJBEPDAEnyACMOsfe30323341 = 66500312;    int wGlpJBEPDAEnyACMOsfe43783669 = -3322542;    int wGlpJBEPDAEnyACMOsfe78678176 = 39147477;    int wGlpJBEPDAEnyACMOsfe20226481 = -317573387;    int wGlpJBEPDAEnyACMOsfe10889225 = 57188293;    int wGlpJBEPDAEnyACMOsfe9262596 = -188977895;    int wGlpJBEPDAEnyACMOsfe26812929 = -987891235;    int wGlpJBEPDAEnyACMOsfe63106669 = 9026502;    int wGlpJBEPDAEnyACMOsfe98668327 = -778919943;    int wGlpJBEPDAEnyACMOsfe80577255 = -527591713;    int wGlpJBEPDAEnyACMOsfe36406888 = 45200278;    int wGlpJBEPDAEnyACMOsfe59275021 = 81325567;    int wGlpJBEPDAEnyACMOsfe84191384 = -141854624;    int wGlpJBEPDAEnyACMOsfe62143111 = -118537213;    int wGlpJBEPDAEnyACMOsfe683339 = -963934340;    int wGlpJBEPDAEnyACMOsfe79797627 = -526387345;    int wGlpJBEPDAEnyACMOsfe75357040 = -210263041;    int wGlpJBEPDAEnyACMOsfe62302126 = 55779833;    int wGlpJBEPDAEnyACMOsfe91004320 = -248116913;     wGlpJBEPDAEnyACMOsfe18129281 = wGlpJBEPDAEnyACMOsfe44116740;     wGlpJBEPDAEnyACMOsfe44116740 = wGlpJBEPDAEnyACMOsfe61471009;     wGlpJBEPDAEnyACMOsfe61471009 = wGlpJBEPDAEnyACMOsfe83976944;     wGlpJBEPDAEnyACMOsfe83976944 = wGlpJBEPDAEnyACMOsfe60316629;     wGlpJBEPDAEnyACMOsfe60316629 = wGlpJBEPDAEnyACMOsfe86509704;     wGlpJBEPDAEnyACMOsfe86509704 = wGlpJBEPDAEnyACMOsfe10611505;     wGlpJBEPDAEnyACMOsfe10611505 = wGlpJBEPDAEnyACMOsfe49044498;     wGlpJBEPDAEnyACMOsfe49044498 = wGlpJBEPDAEnyACMOsfe3182079;     wGlpJBEPDAEnyACMOsfe3182079 = wGlpJBEPDAEnyACMOsfe75569591;     wGlpJBEPDAEnyACMOsfe75569591 = wGlpJBEPDAEnyACMOsfe58474991;     wGlpJBEPDAEnyACMOsfe58474991 = wGlpJBEPDAEnyACMOsfe66931050;     wGlpJBEPDAEnyACMOsfe66931050 = wGlpJBEPDAEnyACMOsfe70447815;     wGlpJBEPDAEnyACMOsfe70447815 = wGlpJBEPDAEnyACMOsfe71867465;     wGlpJBEPDAEnyACMOsfe71867465 = wGlpJBEPDAEnyACMOsfe88749833;     wGlpJBEPDAEnyACMOsfe88749833 = wGlpJBEPDAEnyACMOsfe49007852;     wGlpJBEPDAEnyACMOsfe49007852 = wGlpJBEPDAEnyACMOsfe21308851;     wGlpJBEPDAEnyACMOsfe21308851 = wGlpJBEPDAEnyACMOsfe98908262;     wGlpJBEPDAEnyACMOsfe98908262 = wGlpJBEPDAEnyACMOsfe11220627;     wGlpJBEPDAEnyACMOsfe11220627 = wGlpJBEPDAEnyACMOsfe7872469;     wGlpJBEPDAEnyACMOsfe7872469 = wGlpJBEPDAEnyACMOsfe14179086;     wGlpJBEPDAEnyACMOsfe14179086 = wGlpJBEPDAEnyACMOsfe20026883;     wGlpJBEPDAEnyACMOsfe20026883 = wGlpJBEPDAEnyACMOsfe98896398;     wGlpJBEPDAEnyACMOsfe98896398 = wGlpJBEPDAEnyACMOsfe74005239;     wGlpJBEPDAEnyACMOsfe74005239 = wGlpJBEPDAEnyACMOsfe51970140;     wGlpJBEPDAEnyACMOsfe51970140 = wGlpJBEPDAEnyACMOsfe99923419;     wGlpJBEPDAEnyACMOsfe99923419 = wGlpJBEPDAEnyACMOsfe61286854;     wGlpJBEPDAEnyACMOsfe61286854 = wGlpJBEPDAEnyACMOsfe20664071;     wGlpJBEPDAEnyACMOsfe20664071 = wGlpJBEPDAEnyACMOsfe48510005;     wGlpJBEPDAEnyACMOsfe48510005 = wGlpJBEPDAEnyACMOsfe55676258;     wGlpJBEPDAEnyACMOsfe55676258 = wGlpJBEPDAEnyACMOsfe4084299;     wGlpJBEPDAEnyACMOsfe4084299 = wGlpJBEPDAEnyACMOsfe47076487;     wGlpJBEPDAEnyACMOsfe47076487 = wGlpJBEPDAEnyACMOsfe29843133;     wGlpJBEPDAEnyACMOsfe29843133 = wGlpJBEPDAEnyACMOsfe57541643;     wGlpJBEPDAEnyACMOsfe57541643 = wGlpJBEPDAEnyACMOsfe96770725;     wGlpJBEPDAEnyACMOsfe96770725 = wGlpJBEPDAEnyACMOsfe25797597;     wGlpJBEPDAEnyACMOsfe25797597 = wGlpJBEPDAEnyACMOsfe23872657;     wGlpJBEPDAEnyACMOsfe23872657 = wGlpJBEPDAEnyACMOsfe91281824;     wGlpJBEPDAEnyACMOsfe91281824 = wGlpJBEPDAEnyACMOsfe44584594;     wGlpJBEPDAEnyACMOsfe44584594 = wGlpJBEPDAEnyACMOsfe87365961;     wGlpJBEPDAEnyACMOsfe87365961 = wGlpJBEPDAEnyACMOsfe90630772;     wGlpJBEPDAEnyACMOsfe90630772 = wGlpJBEPDAEnyACMOsfe80332121;     wGlpJBEPDAEnyACMOsfe80332121 = wGlpJBEPDAEnyACMOsfe22862861;     wGlpJBEPDAEnyACMOsfe22862861 = wGlpJBEPDAEnyACMOsfe33787418;     wGlpJBEPDAEnyACMOsfe33787418 = wGlpJBEPDAEnyACMOsfe2402825;     wGlpJBEPDAEnyACMOsfe2402825 = wGlpJBEPDAEnyACMOsfe5440926;     wGlpJBEPDAEnyACMOsfe5440926 = wGlpJBEPDAEnyACMOsfe44196353;     wGlpJBEPDAEnyACMOsfe44196353 = wGlpJBEPDAEnyACMOsfe30963513;     wGlpJBEPDAEnyACMOsfe30963513 = wGlpJBEPDAEnyACMOsfe76880401;     wGlpJBEPDAEnyACMOsfe76880401 = wGlpJBEPDAEnyACMOsfe69831828;     wGlpJBEPDAEnyACMOsfe69831828 = wGlpJBEPDAEnyACMOsfe35449778;     wGlpJBEPDAEnyACMOsfe35449778 = wGlpJBEPDAEnyACMOsfe93195074;     wGlpJBEPDAEnyACMOsfe93195074 = wGlpJBEPDAEnyACMOsfe37813892;     wGlpJBEPDAEnyACMOsfe37813892 = wGlpJBEPDAEnyACMOsfe3030204;     wGlpJBEPDAEnyACMOsfe3030204 = wGlpJBEPDAEnyACMOsfe94434974;     wGlpJBEPDAEnyACMOsfe94434974 = wGlpJBEPDAEnyACMOsfe98102397;     wGlpJBEPDAEnyACMOsfe98102397 = wGlpJBEPDAEnyACMOsfe45220341;     wGlpJBEPDAEnyACMOsfe45220341 = wGlpJBEPDAEnyACMOsfe87465769;     wGlpJBEPDAEnyACMOsfe87465769 = wGlpJBEPDAEnyACMOsfe32006804;     wGlpJBEPDAEnyACMOsfe32006804 = wGlpJBEPDAEnyACMOsfe60393209;     wGlpJBEPDAEnyACMOsfe60393209 = wGlpJBEPDAEnyACMOsfe25222850;     wGlpJBEPDAEnyACMOsfe25222850 = wGlpJBEPDAEnyACMOsfe89947434;     wGlpJBEPDAEnyACMOsfe89947434 = wGlpJBEPDAEnyACMOsfe534494;     wGlpJBEPDAEnyACMOsfe534494 = wGlpJBEPDAEnyACMOsfe47505820;     wGlpJBEPDAEnyACMOsfe47505820 = wGlpJBEPDAEnyACMOsfe71485292;     wGlpJBEPDAEnyACMOsfe71485292 = wGlpJBEPDAEnyACMOsfe11398504;     wGlpJBEPDAEnyACMOsfe11398504 = wGlpJBEPDAEnyACMOsfe37087918;     wGlpJBEPDAEnyACMOsfe37087918 = wGlpJBEPDAEnyACMOsfe12906172;     wGlpJBEPDAEnyACMOsfe12906172 = wGlpJBEPDAEnyACMOsfe75096740;     wGlpJBEPDAEnyACMOsfe75096740 = wGlpJBEPDAEnyACMOsfe62952237;     wGlpJBEPDAEnyACMOsfe62952237 = wGlpJBEPDAEnyACMOsfe25135195;     wGlpJBEPDAEnyACMOsfe25135195 = wGlpJBEPDAEnyACMOsfe30027026;     wGlpJBEPDAEnyACMOsfe30027026 = wGlpJBEPDAEnyACMOsfe54323668;     wGlpJBEPDAEnyACMOsfe54323668 = wGlpJBEPDAEnyACMOsfe23854665;     wGlpJBEPDAEnyACMOsfe23854665 = wGlpJBEPDAEnyACMOsfe17241696;     wGlpJBEPDAEnyACMOsfe17241696 = wGlpJBEPDAEnyACMOsfe33846964;     wGlpJBEPDAEnyACMOsfe33846964 = wGlpJBEPDAEnyACMOsfe97164022;     wGlpJBEPDAEnyACMOsfe97164022 = wGlpJBEPDAEnyACMOsfe65108981;     wGlpJBEPDAEnyACMOsfe65108981 = wGlpJBEPDAEnyACMOsfe71602415;     wGlpJBEPDAEnyACMOsfe71602415 = wGlpJBEPDAEnyACMOsfe46529215;     wGlpJBEPDAEnyACMOsfe46529215 = wGlpJBEPDAEnyACMOsfe55727067;     wGlpJBEPDAEnyACMOsfe55727067 = wGlpJBEPDAEnyACMOsfe30323341;     wGlpJBEPDAEnyACMOsfe30323341 = wGlpJBEPDAEnyACMOsfe43783669;     wGlpJBEPDAEnyACMOsfe43783669 = wGlpJBEPDAEnyACMOsfe78678176;     wGlpJBEPDAEnyACMOsfe78678176 = wGlpJBEPDAEnyACMOsfe20226481;     wGlpJBEPDAEnyACMOsfe20226481 = wGlpJBEPDAEnyACMOsfe10889225;     wGlpJBEPDAEnyACMOsfe10889225 = wGlpJBEPDAEnyACMOsfe9262596;     wGlpJBEPDAEnyACMOsfe9262596 = wGlpJBEPDAEnyACMOsfe26812929;     wGlpJBEPDAEnyACMOsfe26812929 = wGlpJBEPDAEnyACMOsfe63106669;     wGlpJBEPDAEnyACMOsfe63106669 = wGlpJBEPDAEnyACMOsfe98668327;     wGlpJBEPDAEnyACMOsfe98668327 = wGlpJBEPDAEnyACMOsfe80577255;     wGlpJBEPDAEnyACMOsfe80577255 = wGlpJBEPDAEnyACMOsfe36406888;     wGlpJBEPDAEnyACMOsfe36406888 = wGlpJBEPDAEnyACMOsfe59275021;     wGlpJBEPDAEnyACMOsfe59275021 = wGlpJBEPDAEnyACMOsfe84191384;     wGlpJBEPDAEnyACMOsfe84191384 = wGlpJBEPDAEnyACMOsfe62143111;     wGlpJBEPDAEnyACMOsfe62143111 = wGlpJBEPDAEnyACMOsfe683339;     wGlpJBEPDAEnyACMOsfe683339 = wGlpJBEPDAEnyACMOsfe79797627;     wGlpJBEPDAEnyACMOsfe79797627 = wGlpJBEPDAEnyACMOsfe75357040;     wGlpJBEPDAEnyACMOsfe75357040 = wGlpJBEPDAEnyACMOsfe62302126;     wGlpJBEPDAEnyACMOsfe62302126 = wGlpJBEPDAEnyACMOsfe91004320;     wGlpJBEPDAEnyACMOsfe91004320 = wGlpJBEPDAEnyACMOsfe18129281;}
// Junk Finished

// Junk Code By Peatreat & Thaisen's Gen
void xILIDminuJneysJahTyL84956377() {     int eoKZtVwegTibStxnHFtd50390089 = -923764675;    int eoKZtVwegTibStxnHFtd45339176 = -229424894;    int eoKZtVwegTibStxnHFtd94747588 = -488324881;    int eoKZtVwegTibStxnHFtd25485058 = -667025908;    int eoKZtVwegTibStxnHFtd45434148 = -58070591;    int eoKZtVwegTibStxnHFtd20606417 = -908324996;    int eoKZtVwegTibStxnHFtd8103806 = -756856367;    int eoKZtVwegTibStxnHFtd71799794 = -580597413;    int eoKZtVwegTibStxnHFtd76533522 = 67370190;    int eoKZtVwegTibStxnHFtd63077505 = -506398694;    int eoKZtVwegTibStxnHFtd19865032 = -477882123;    int eoKZtVwegTibStxnHFtd88084491 = -889932193;    int eoKZtVwegTibStxnHFtd99109863 = -31984562;    int eoKZtVwegTibStxnHFtd76063364 = -589294388;    int eoKZtVwegTibStxnHFtd27410739 = -91029204;    int eoKZtVwegTibStxnHFtd69084023 = -637393068;    int eoKZtVwegTibStxnHFtd27170022 = -508133166;    int eoKZtVwegTibStxnHFtd83528965 = -768473905;    int eoKZtVwegTibStxnHFtd91587866 = -368618951;    int eoKZtVwegTibStxnHFtd19064504 = 61800598;    int eoKZtVwegTibStxnHFtd27139790 = -163567221;    int eoKZtVwegTibStxnHFtd85760519 = 41355195;    int eoKZtVwegTibStxnHFtd68951304 = -221826139;    int eoKZtVwegTibStxnHFtd92583929 = -275255946;    int eoKZtVwegTibStxnHFtd40625199 = -171923373;    int eoKZtVwegTibStxnHFtd69435449 = -592009112;    int eoKZtVwegTibStxnHFtd86408939 = -219785446;    int eoKZtVwegTibStxnHFtd3405146 = 42890725;    int eoKZtVwegTibStxnHFtd94193567 = 96769022;    int eoKZtVwegTibStxnHFtd17044358 = -863751551;    int eoKZtVwegTibStxnHFtd44911701 = -160106906;    int eoKZtVwegTibStxnHFtd5250764 = -911370127;    int eoKZtVwegTibStxnHFtd1912763 = -98242930;    int eoKZtVwegTibStxnHFtd93091686 = -446096658;    int eoKZtVwegTibStxnHFtd37403556 = -553482489;    int eoKZtVwegTibStxnHFtd14259459 = -743560221;    int eoKZtVwegTibStxnHFtd17078801 = -398109803;    int eoKZtVwegTibStxnHFtd20550643 = -673155667;    int eoKZtVwegTibStxnHFtd82359708 = -219787019;    int eoKZtVwegTibStxnHFtd63039534 = 31324752;    int eoKZtVwegTibStxnHFtd5688471 = 94879615;    int eoKZtVwegTibStxnHFtd14196050 = -995078423;    int eoKZtVwegTibStxnHFtd68596211 = 39163638;    int eoKZtVwegTibStxnHFtd2569597 = -5575216;    int eoKZtVwegTibStxnHFtd78421674 = -721604035;    int eoKZtVwegTibStxnHFtd93102213 = -693480353;    int eoKZtVwegTibStxnHFtd76070460 = -703948883;    int eoKZtVwegTibStxnHFtd85175728 = 81893003;    int eoKZtVwegTibStxnHFtd43421652 = -738999690;    int eoKZtVwegTibStxnHFtd37635169 = 54655694;    int eoKZtVwegTibStxnHFtd14953471 = -380978921;    int eoKZtVwegTibStxnHFtd23273224 = -829038041;    int eoKZtVwegTibStxnHFtd66675840 = -431203532;    int eoKZtVwegTibStxnHFtd10409687 = -33358887;    int eoKZtVwegTibStxnHFtd57816587 = -34551312;    int eoKZtVwegTibStxnHFtd64629570 = -865119870;    int eoKZtVwegTibStxnHFtd76387872 = 92401244;    int eoKZtVwegTibStxnHFtd2163660 = -113068935;    int eoKZtVwegTibStxnHFtd84859859 = -395102536;    int eoKZtVwegTibStxnHFtd75998698 = -466061480;    int eoKZtVwegTibStxnHFtd34197477 = -588539550;    int eoKZtVwegTibStxnHFtd4698660 = -699747093;    int eoKZtVwegTibStxnHFtd77606226 = -577366435;    int eoKZtVwegTibStxnHFtd59489164 = -68878260;    int eoKZtVwegTibStxnHFtd18165805 = -246291788;    int eoKZtVwegTibStxnHFtd14614268 = -566511997;    int eoKZtVwegTibStxnHFtd86171729 = -691689263;    int eoKZtVwegTibStxnHFtd6018178 = -585887904;    int eoKZtVwegTibStxnHFtd38659809 = 64188101;    int eoKZtVwegTibStxnHFtd13151281 = -347468983;    int eoKZtVwegTibStxnHFtd52005222 = -139283265;    int eoKZtVwegTibStxnHFtd6619380 = -834977500;    int eoKZtVwegTibStxnHFtd1169258 = -448686887;    int eoKZtVwegTibStxnHFtd28548332 = -299943703;    int eoKZtVwegTibStxnHFtd13376033 = 66920982;    int eoKZtVwegTibStxnHFtd12943741 = -168488798;    int eoKZtVwegTibStxnHFtd17164309 = -997808444;    int eoKZtVwegTibStxnHFtd66381707 = -116250923;    int eoKZtVwegTibStxnHFtd14162256 = -553651912;    int eoKZtVwegTibStxnHFtd47522985 = -478443021;    int eoKZtVwegTibStxnHFtd93364989 = -888060229;    int eoKZtVwegTibStxnHFtd1233211 = -201678449;    int eoKZtVwegTibStxnHFtd59983493 = -218109585;    int eoKZtVwegTibStxnHFtd56558399 = -957886673;    int eoKZtVwegTibStxnHFtd2090888 = -382772631;    int eoKZtVwegTibStxnHFtd21638477 = -331068866;    int eoKZtVwegTibStxnHFtd38574924 = -380166595;    int eoKZtVwegTibStxnHFtd91503075 = 35115956;    int eoKZtVwegTibStxnHFtd35275099 = -311545347;    int eoKZtVwegTibStxnHFtd72773985 = -688362620;    int eoKZtVwegTibStxnHFtd37871587 = -735961466;    int eoKZtVwegTibStxnHFtd14915142 = -185040868;    int eoKZtVwegTibStxnHFtd35690783 = -178053132;    int eoKZtVwegTibStxnHFtd6361010 = -753725539;    int eoKZtVwegTibStxnHFtd28842057 = -380135698;    int eoKZtVwegTibStxnHFtd989811 = -205373293;    int eoKZtVwegTibStxnHFtd36589823 = -317711988;    int eoKZtVwegTibStxnHFtd9107047 = -891958103;    int eoKZtVwegTibStxnHFtd84403792 = -759283428;    int eoKZtVwegTibStxnHFtd63807406 = -923764675;     eoKZtVwegTibStxnHFtd50390089 = eoKZtVwegTibStxnHFtd45339176;     eoKZtVwegTibStxnHFtd45339176 = eoKZtVwegTibStxnHFtd94747588;     eoKZtVwegTibStxnHFtd94747588 = eoKZtVwegTibStxnHFtd25485058;     eoKZtVwegTibStxnHFtd25485058 = eoKZtVwegTibStxnHFtd45434148;     eoKZtVwegTibStxnHFtd45434148 = eoKZtVwegTibStxnHFtd20606417;     eoKZtVwegTibStxnHFtd20606417 = eoKZtVwegTibStxnHFtd8103806;     eoKZtVwegTibStxnHFtd8103806 = eoKZtVwegTibStxnHFtd71799794;     eoKZtVwegTibStxnHFtd71799794 = eoKZtVwegTibStxnHFtd76533522;     eoKZtVwegTibStxnHFtd76533522 = eoKZtVwegTibStxnHFtd63077505;     eoKZtVwegTibStxnHFtd63077505 = eoKZtVwegTibStxnHFtd19865032;     eoKZtVwegTibStxnHFtd19865032 = eoKZtVwegTibStxnHFtd88084491;     eoKZtVwegTibStxnHFtd88084491 = eoKZtVwegTibStxnHFtd99109863;     eoKZtVwegTibStxnHFtd99109863 = eoKZtVwegTibStxnHFtd76063364;     eoKZtVwegTibStxnHFtd76063364 = eoKZtVwegTibStxnHFtd27410739;     eoKZtVwegTibStxnHFtd27410739 = eoKZtVwegTibStxnHFtd69084023;     eoKZtVwegTibStxnHFtd69084023 = eoKZtVwegTibStxnHFtd27170022;     eoKZtVwegTibStxnHFtd27170022 = eoKZtVwegTibStxnHFtd83528965;     eoKZtVwegTibStxnHFtd83528965 = eoKZtVwegTibStxnHFtd91587866;     eoKZtVwegTibStxnHFtd91587866 = eoKZtVwegTibStxnHFtd19064504;     eoKZtVwegTibStxnHFtd19064504 = eoKZtVwegTibStxnHFtd27139790;     eoKZtVwegTibStxnHFtd27139790 = eoKZtVwegTibStxnHFtd85760519;     eoKZtVwegTibStxnHFtd85760519 = eoKZtVwegTibStxnHFtd68951304;     eoKZtVwegTibStxnHFtd68951304 = eoKZtVwegTibStxnHFtd92583929;     eoKZtVwegTibStxnHFtd92583929 = eoKZtVwegTibStxnHFtd40625199;     eoKZtVwegTibStxnHFtd40625199 = eoKZtVwegTibStxnHFtd69435449;     eoKZtVwegTibStxnHFtd69435449 = eoKZtVwegTibStxnHFtd86408939;     eoKZtVwegTibStxnHFtd86408939 = eoKZtVwegTibStxnHFtd3405146;     eoKZtVwegTibStxnHFtd3405146 = eoKZtVwegTibStxnHFtd94193567;     eoKZtVwegTibStxnHFtd94193567 = eoKZtVwegTibStxnHFtd17044358;     eoKZtVwegTibStxnHFtd17044358 = eoKZtVwegTibStxnHFtd44911701;     eoKZtVwegTibStxnHFtd44911701 = eoKZtVwegTibStxnHFtd5250764;     eoKZtVwegTibStxnHFtd5250764 = eoKZtVwegTibStxnHFtd1912763;     eoKZtVwegTibStxnHFtd1912763 = eoKZtVwegTibStxnHFtd93091686;     eoKZtVwegTibStxnHFtd93091686 = eoKZtVwegTibStxnHFtd37403556;     eoKZtVwegTibStxnHFtd37403556 = eoKZtVwegTibStxnHFtd14259459;     eoKZtVwegTibStxnHFtd14259459 = eoKZtVwegTibStxnHFtd17078801;     eoKZtVwegTibStxnHFtd17078801 = eoKZtVwegTibStxnHFtd20550643;     eoKZtVwegTibStxnHFtd20550643 = eoKZtVwegTibStxnHFtd82359708;     eoKZtVwegTibStxnHFtd82359708 = eoKZtVwegTibStxnHFtd63039534;     eoKZtVwegTibStxnHFtd63039534 = eoKZtVwegTibStxnHFtd5688471;     eoKZtVwegTibStxnHFtd5688471 = eoKZtVwegTibStxnHFtd14196050;     eoKZtVwegTibStxnHFtd14196050 = eoKZtVwegTibStxnHFtd68596211;     eoKZtVwegTibStxnHFtd68596211 = eoKZtVwegTibStxnHFtd2569597;     eoKZtVwegTibStxnHFtd2569597 = eoKZtVwegTibStxnHFtd78421674;     eoKZtVwegTibStxnHFtd78421674 = eoKZtVwegTibStxnHFtd93102213;     eoKZtVwegTibStxnHFtd93102213 = eoKZtVwegTibStxnHFtd76070460;     eoKZtVwegTibStxnHFtd76070460 = eoKZtVwegTibStxnHFtd85175728;     eoKZtVwegTibStxnHFtd85175728 = eoKZtVwegTibStxnHFtd43421652;     eoKZtVwegTibStxnHFtd43421652 = eoKZtVwegTibStxnHFtd37635169;     eoKZtVwegTibStxnHFtd37635169 = eoKZtVwegTibStxnHFtd14953471;     eoKZtVwegTibStxnHFtd14953471 = eoKZtVwegTibStxnHFtd23273224;     eoKZtVwegTibStxnHFtd23273224 = eoKZtVwegTibStxnHFtd66675840;     eoKZtVwegTibStxnHFtd66675840 = eoKZtVwegTibStxnHFtd10409687;     eoKZtVwegTibStxnHFtd10409687 = eoKZtVwegTibStxnHFtd57816587;     eoKZtVwegTibStxnHFtd57816587 = eoKZtVwegTibStxnHFtd64629570;     eoKZtVwegTibStxnHFtd64629570 = eoKZtVwegTibStxnHFtd76387872;     eoKZtVwegTibStxnHFtd76387872 = eoKZtVwegTibStxnHFtd2163660;     eoKZtVwegTibStxnHFtd2163660 = eoKZtVwegTibStxnHFtd84859859;     eoKZtVwegTibStxnHFtd84859859 = eoKZtVwegTibStxnHFtd75998698;     eoKZtVwegTibStxnHFtd75998698 = eoKZtVwegTibStxnHFtd34197477;     eoKZtVwegTibStxnHFtd34197477 = eoKZtVwegTibStxnHFtd4698660;     eoKZtVwegTibStxnHFtd4698660 = eoKZtVwegTibStxnHFtd77606226;     eoKZtVwegTibStxnHFtd77606226 = eoKZtVwegTibStxnHFtd59489164;     eoKZtVwegTibStxnHFtd59489164 = eoKZtVwegTibStxnHFtd18165805;     eoKZtVwegTibStxnHFtd18165805 = eoKZtVwegTibStxnHFtd14614268;     eoKZtVwegTibStxnHFtd14614268 = eoKZtVwegTibStxnHFtd86171729;     eoKZtVwegTibStxnHFtd86171729 = eoKZtVwegTibStxnHFtd6018178;     eoKZtVwegTibStxnHFtd6018178 = eoKZtVwegTibStxnHFtd38659809;     eoKZtVwegTibStxnHFtd38659809 = eoKZtVwegTibStxnHFtd13151281;     eoKZtVwegTibStxnHFtd13151281 = eoKZtVwegTibStxnHFtd52005222;     eoKZtVwegTibStxnHFtd52005222 = eoKZtVwegTibStxnHFtd6619380;     eoKZtVwegTibStxnHFtd6619380 = eoKZtVwegTibStxnHFtd1169258;     eoKZtVwegTibStxnHFtd1169258 = eoKZtVwegTibStxnHFtd28548332;     eoKZtVwegTibStxnHFtd28548332 = eoKZtVwegTibStxnHFtd13376033;     eoKZtVwegTibStxnHFtd13376033 = eoKZtVwegTibStxnHFtd12943741;     eoKZtVwegTibStxnHFtd12943741 = eoKZtVwegTibStxnHFtd17164309;     eoKZtVwegTibStxnHFtd17164309 = eoKZtVwegTibStxnHFtd66381707;     eoKZtVwegTibStxnHFtd66381707 = eoKZtVwegTibStxnHFtd14162256;     eoKZtVwegTibStxnHFtd14162256 = eoKZtVwegTibStxnHFtd47522985;     eoKZtVwegTibStxnHFtd47522985 = eoKZtVwegTibStxnHFtd93364989;     eoKZtVwegTibStxnHFtd93364989 = eoKZtVwegTibStxnHFtd1233211;     eoKZtVwegTibStxnHFtd1233211 = eoKZtVwegTibStxnHFtd59983493;     eoKZtVwegTibStxnHFtd59983493 = eoKZtVwegTibStxnHFtd56558399;     eoKZtVwegTibStxnHFtd56558399 = eoKZtVwegTibStxnHFtd2090888;     eoKZtVwegTibStxnHFtd2090888 = eoKZtVwegTibStxnHFtd21638477;     eoKZtVwegTibStxnHFtd21638477 = eoKZtVwegTibStxnHFtd38574924;     eoKZtVwegTibStxnHFtd38574924 = eoKZtVwegTibStxnHFtd91503075;     eoKZtVwegTibStxnHFtd91503075 = eoKZtVwegTibStxnHFtd35275099;     eoKZtVwegTibStxnHFtd35275099 = eoKZtVwegTibStxnHFtd72773985;     eoKZtVwegTibStxnHFtd72773985 = eoKZtVwegTibStxnHFtd37871587;     eoKZtVwegTibStxnHFtd37871587 = eoKZtVwegTibStxnHFtd14915142;     eoKZtVwegTibStxnHFtd14915142 = eoKZtVwegTibStxnHFtd35690783;     eoKZtVwegTibStxnHFtd35690783 = eoKZtVwegTibStxnHFtd6361010;     eoKZtVwegTibStxnHFtd6361010 = eoKZtVwegTibStxnHFtd28842057;     eoKZtVwegTibStxnHFtd28842057 = eoKZtVwegTibStxnHFtd989811;     eoKZtVwegTibStxnHFtd989811 = eoKZtVwegTibStxnHFtd36589823;     eoKZtVwegTibStxnHFtd36589823 = eoKZtVwegTibStxnHFtd9107047;     eoKZtVwegTibStxnHFtd9107047 = eoKZtVwegTibStxnHFtd84403792;     eoKZtVwegTibStxnHFtd84403792 = eoKZtVwegTibStxnHFtd63807406;     eoKZtVwegTibStxnHFtd63807406 = eoKZtVwegTibStxnHFtd50390089;}
// Junk Finished

// Junk Code By Peatreat & Thaisen's Gen
void wBebhuJnGFyCCMwzxhyL9056163() {     int sJtyJghIRupZVIAvbXTN53309036 = -145037426;    int sJtyJghIRupZVIAvbXTN76662812 = -732089953;    int sJtyJghIRupZVIAvbXTN6679876 = 23649890;    int sJtyJghIRupZVIAvbXTN21844598 = -856859177;    int sJtyJghIRupZVIAvbXTN43161886 = -22855176;    int sJtyJghIRupZVIAvbXTN9897460 = -878448605;    int sJtyJghIRupZVIAvbXTN55427666 = -135537444;    int sJtyJghIRupZVIAvbXTN82183419 = -957386091;    int sJtyJghIRupZVIAvbXTN13009856 = -188634470;    int sJtyJghIRupZVIAvbXTN76912367 = -243553981;    int sJtyJghIRupZVIAvbXTN3183628 = -41516224;    int sJtyJghIRupZVIAvbXTN40492827 = -41374781;    int sJtyJghIRupZVIAvbXTN75746385 = -226344605;    int sJtyJghIRupZVIAvbXTN10262077 = -574717419;    int sJtyJghIRupZVIAvbXTN3619555 = -520423126;    int sJtyJghIRupZVIAvbXTN47124286 = -555868676;    int sJtyJghIRupZVIAvbXTN71031260 = -613638305;    int sJtyJghIRupZVIAvbXTN65442630 = -369461878;    int sJtyJghIRupZVIAvbXTN33234618 = -126516316;    int sJtyJghIRupZVIAvbXTN16969485 = -573297400;    int sJtyJghIRupZVIAvbXTN15566825 = -648842204;    int sJtyJghIRupZVIAvbXTN34593655 = 48392095;    int sJtyJghIRupZVIAvbXTN4071621 = -100332112;    int sJtyJghIRupZVIAvbXTN1362048 = -102704812;    int sJtyJghIRupZVIAvbXTN64961956 = -599182388;    int sJtyJghIRupZVIAvbXTN52970983 = -250108117;    int sJtyJghIRupZVIAvbXTN49570229 = -312221007;    int sJtyJghIRupZVIAvbXTN58524921 = -681830450;    int sJtyJghIRupZVIAvbXTN59341172 = -233576951;    int sJtyJghIRupZVIAvbXTN41494683 = -692608723;    int sJtyJghIRupZVIAvbXTN59398421 = -376096965;    int sJtyJghIRupZVIAvbXTN23212644 = -999092464;    int sJtyJghIRupZVIAvbXTN71475329 = -72378658;    int sJtyJghIRupZVIAvbXTN93723866 = -465620302;    int sJtyJghIRupZVIAvbXTN73702584 = -554536106;    int sJtyJghIRupZVIAvbXTN19886391 = -755386989;    int sJtyJghIRupZVIAvbXTN42135062 = -704181679;    int sJtyJghIRupZVIAvbXTN32694269 = -463572323;    int sJtyJghIRupZVIAvbXTN63354514 = -156418225;    int sJtyJghIRupZVIAvbXTN61943513 = -108368181;    int sJtyJghIRupZVIAvbXTN92963784 = -998511756;    int sJtyJghIRupZVIAvbXTN90845307 = -514090888;    int sJtyJghIRupZVIAvbXTN3013163 = 99434852;    int sJtyJghIRupZVIAvbXTN18628177 = -653866204;    int sJtyJghIRupZVIAvbXTN66376529 = -253624550;    int sJtyJghIRupZVIAvbXTN46741368 = -338378027;    int sJtyJghIRupZVIAvbXTN6377978 = -268665813;    int sJtyJghIRupZVIAvbXTN29618573 = -591354673;    int sJtyJghIRupZVIAvbXTN96745966 = -674777150;    int sJtyJghIRupZVIAvbXTN93820571 = -326352219;    int sJtyJghIRupZVIAvbXTN49387118 = -329690267;    int sJtyJghIRupZVIAvbXTN80675621 = -882504463;    int sJtyJghIRupZVIAvbXTN75165097 = -427005802;    int sJtyJghIRupZVIAvbXTN97766203 = -314446059;    int sJtyJghIRupZVIAvbXTN93489337 = -400696792;    int sJtyJghIRupZVIAvbXTN18715382 = -93429521;    int sJtyJghIRupZVIAvbXTN72591191 = -531757841;    int sJtyJghIRupZVIAvbXTN5317828 = -873645299;    int sJtyJghIRupZVIAvbXTN56882641 = -157676789;    int sJtyJghIRupZVIAvbXTN90190903 = -772747060;    int sJtyJghIRupZVIAvbXTN60327230 = -466227599;    int sJtyJghIRupZVIAvbXTN96902745 = -453706995;    int sJtyJghIRupZVIAvbXTN22842248 = -623809140;    int sJtyJghIRupZVIAvbXTN71515172 = -496025748;    int sJtyJghIRupZVIAvbXTN17513946 = -867457016;    int sJtyJghIRupZVIAvbXTN79970983 = -42423761;    int sJtyJghIRupZVIAvbXTN69017498 = -968996124;    int sJtyJghIRupZVIAvbXTN82022518 = -760724304;    int sJtyJghIRupZVIAvbXTN36559493 = 79818686;    int sJtyJghIRupZVIAvbXTN83733164 = -765036137;    int sJtyJghIRupZVIAvbXTN4989225 = -851686997;    int sJtyJghIRupZVIAvbXTN38336991 = -50065982;    int sJtyJghIRupZVIAvbXTN2088116 = -113043653;    int sJtyJghIRupZVIAvbXTN71291105 = 81851865;    int sJtyJghIRupZVIAvbXTN24005701 = -574785645;    int sJtyJghIRupZVIAvbXTN24721518 = -34751316;    int sJtyJghIRupZVIAvbXTN31580493 = 48957242;    int sJtyJghIRupZVIAvbXTN85443444 = -446465909;    int sJtyJghIRupZVIAvbXTN34985519 = -849080262;    int sJtyJghIRupZVIAvbXTN18220588 = -160804362;    int sJtyJghIRupZVIAvbXTN46593005 = -981442304;    int sJtyJghIRupZVIAvbXTN19951656 = -720866334;    int sJtyJghIRupZVIAvbXTN61778955 = 92946700;    int sJtyJghIRupZVIAvbXTN65520600 = -907224732;    int sJtyJghIRupZVIAvbXTN92107565 = -262918457;    int sJtyJghIRupZVIAvbXTN78722800 = -493592502;    int sJtyJghIRupZVIAvbXTN48047547 = -472086662;    int sJtyJghIRupZVIAvbXTN73709125 = -757932599;    int sJtyJghIRupZVIAvbXTN234529 = 35076490;    int sJtyJghIRupZVIAvbXTN54987202 = -361106585;    int sJtyJghIRupZVIAvbXTN47295199 = -123629149;    int sJtyJghIRupZVIAvbXTN36817235 = -830536381;    int sJtyJghIRupZVIAvbXTN75811628 = -205895535;    int sJtyJghIRupZVIAvbXTN73163611 = -383671166;    int sJtyJghIRupZVIAvbXTN1616283 = -642140582;    int sJtyJghIRupZVIAvbXTN96061039 = -444804761;    int sJtyJghIRupZVIAvbXTN68003060 = -890281748;    int sJtyJghIRupZVIAvbXTN31497990 = -404539401;    int sJtyJghIRupZVIAvbXTN1114232 = -786409188;    int sJtyJghIRupZVIAvbXTN86405546 = -145037426;     sJtyJghIRupZVIAvbXTN53309036 = sJtyJghIRupZVIAvbXTN76662812;     sJtyJghIRupZVIAvbXTN76662812 = sJtyJghIRupZVIAvbXTN6679876;     sJtyJghIRupZVIAvbXTN6679876 = sJtyJghIRupZVIAvbXTN21844598;     sJtyJghIRupZVIAvbXTN21844598 = sJtyJghIRupZVIAvbXTN43161886;     sJtyJghIRupZVIAvbXTN43161886 = sJtyJghIRupZVIAvbXTN9897460;     sJtyJghIRupZVIAvbXTN9897460 = sJtyJghIRupZVIAvbXTN55427666;     sJtyJghIRupZVIAvbXTN55427666 = sJtyJghIRupZVIAvbXTN82183419;     sJtyJghIRupZVIAvbXTN82183419 = sJtyJghIRupZVIAvbXTN13009856;     sJtyJghIRupZVIAvbXTN13009856 = sJtyJghIRupZVIAvbXTN76912367;     sJtyJghIRupZVIAvbXTN76912367 = sJtyJghIRupZVIAvbXTN3183628;     sJtyJghIRupZVIAvbXTN3183628 = sJtyJghIRupZVIAvbXTN40492827;     sJtyJghIRupZVIAvbXTN40492827 = sJtyJghIRupZVIAvbXTN75746385;     sJtyJghIRupZVIAvbXTN75746385 = sJtyJghIRupZVIAvbXTN10262077;     sJtyJghIRupZVIAvbXTN10262077 = sJtyJghIRupZVIAvbXTN3619555;     sJtyJghIRupZVIAvbXTN3619555 = sJtyJghIRupZVIAvbXTN47124286;     sJtyJghIRupZVIAvbXTN47124286 = sJtyJghIRupZVIAvbXTN71031260;     sJtyJghIRupZVIAvbXTN71031260 = sJtyJghIRupZVIAvbXTN65442630;     sJtyJghIRupZVIAvbXTN65442630 = sJtyJghIRupZVIAvbXTN33234618;     sJtyJghIRupZVIAvbXTN33234618 = sJtyJghIRupZVIAvbXTN16969485;     sJtyJghIRupZVIAvbXTN16969485 = sJtyJghIRupZVIAvbXTN15566825;     sJtyJghIRupZVIAvbXTN15566825 = sJtyJghIRupZVIAvbXTN34593655;     sJtyJghIRupZVIAvbXTN34593655 = sJtyJghIRupZVIAvbXTN4071621;     sJtyJghIRupZVIAvbXTN4071621 = sJtyJghIRupZVIAvbXTN1362048;     sJtyJghIRupZVIAvbXTN1362048 = sJtyJghIRupZVIAvbXTN64961956;     sJtyJghIRupZVIAvbXTN64961956 = sJtyJghIRupZVIAvbXTN52970983;     sJtyJghIRupZVIAvbXTN52970983 = sJtyJghIRupZVIAvbXTN49570229;     sJtyJghIRupZVIAvbXTN49570229 = sJtyJghIRupZVIAvbXTN58524921;     sJtyJghIRupZVIAvbXTN58524921 = sJtyJghIRupZVIAvbXTN59341172;     sJtyJghIRupZVIAvbXTN59341172 = sJtyJghIRupZVIAvbXTN41494683;     sJtyJghIRupZVIAvbXTN41494683 = sJtyJghIRupZVIAvbXTN59398421;     sJtyJghIRupZVIAvbXTN59398421 = sJtyJghIRupZVIAvbXTN23212644;     sJtyJghIRupZVIAvbXTN23212644 = sJtyJghIRupZVIAvbXTN71475329;     sJtyJghIRupZVIAvbXTN71475329 = sJtyJghIRupZVIAvbXTN93723866;     sJtyJghIRupZVIAvbXTN93723866 = sJtyJghIRupZVIAvbXTN73702584;     sJtyJghIRupZVIAvbXTN73702584 = sJtyJghIRupZVIAvbXTN19886391;     sJtyJghIRupZVIAvbXTN19886391 = sJtyJghIRupZVIAvbXTN42135062;     sJtyJghIRupZVIAvbXTN42135062 = sJtyJghIRupZVIAvbXTN32694269;     sJtyJghIRupZVIAvbXTN32694269 = sJtyJghIRupZVIAvbXTN63354514;     sJtyJghIRupZVIAvbXTN63354514 = sJtyJghIRupZVIAvbXTN61943513;     sJtyJghIRupZVIAvbXTN61943513 = sJtyJghIRupZVIAvbXTN92963784;     sJtyJghIRupZVIAvbXTN92963784 = sJtyJghIRupZVIAvbXTN90845307;     sJtyJghIRupZVIAvbXTN90845307 = sJtyJghIRupZVIAvbXTN3013163;     sJtyJghIRupZVIAvbXTN3013163 = sJtyJghIRupZVIAvbXTN18628177;     sJtyJghIRupZVIAvbXTN18628177 = sJtyJghIRupZVIAvbXTN66376529;     sJtyJghIRupZVIAvbXTN66376529 = sJtyJghIRupZVIAvbXTN46741368;     sJtyJghIRupZVIAvbXTN46741368 = sJtyJghIRupZVIAvbXTN6377978;     sJtyJghIRupZVIAvbXTN6377978 = sJtyJghIRupZVIAvbXTN29618573;     sJtyJghIRupZVIAvbXTN29618573 = sJtyJghIRupZVIAvbXTN96745966;     sJtyJghIRupZVIAvbXTN96745966 = sJtyJghIRupZVIAvbXTN93820571;     sJtyJghIRupZVIAvbXTN93820571 = sJtyJghIRupZVIAvbXTN49387118;     sJtyJghIRupZVIAvbXTN49387118 = sJtyJghIRupZVIAvbXTN80675621;     sJtyJghIRupZVIAvbXTN80675621 = sJtyJghIRupZVIAvbXTN75165097;     sJtyJghIRupZVIAvbXTN75165097 = sJtyJghIRupZVIAvbXTN97766203;     sJtyJghIRupZVIAvbXTN97766203 = sJtyJghIRupZVIAvbXTN93489337;     sJtyJghIRupZVIAvbXTN93489337 = sJtyJghIRupZVIAvbXTN18715382;     sJtyJghIRupZVIAvbXTN18715382 = sJtyJghIRupZVIAvbXTN72591191;     sJtyJghIRupZVIAvbXTN72591191 = sJtyJghIRupZVIAvbXTN5317828;     sJtyJghIRupZVIAvbXTN5317828 = sJtyJghIRupZVIAvbXTN56882641;     sJtyJghIRupZVIAvbXTN56882641 = sJtyJghIRupZVIAvbXTN90190903;     sJtyJghIRupZVIAvbXTN90190903 = sJtyJghIRupZVIAvbXTN60327230;     sJtyJghIRupZVIAvbXTN60327230 = sJtyJghIRupZVIAvbXTN96902745;     sJtyJghIRupZVIAvbXTN96902745 = sJtyJghIRupZVIAvbXTN22842248;     sJtyJghIRupZVIAvbXTN22842248 = sJtyJghIRupZVIAvbXTN71515172;     sJtyJghIRupZVIAvbXTN71515172 = sJtyJghIRupZVIAvbXTN17513946;     sJtyJghIRupZVIAvbXTN17513946 = sJtyJghIRupZVIAvbXTN79970983;     sJtyJghIRupZVIAvbXTN79970983 = sJtyJghIRupZVIAvbXTN69017498;     sJtyJghIRupZVIAvbXTN69017498 = sJtyJghIRupZVIAvbXTN82022518;     sJtyJghIRupZVIAvbXTN82022518 = sJtyJghIRupZVIAvbXTN36559493;     sJtyJghIRupZVIAvbXTN36559493 = sJtyJghIRupZVIAvbXTN83733164;     sJtyJghIRupZVIAvbXTN83733164 = sJtyJghIRupZVIAvbXTN4989225;     sJtyJghIRupZVIAvbXTN4989225 = sJtyJghIRupZVIAvbXTN38336991;     sJtyJghIRupZVIAvbXTN38336991 = sJtyJghIRupZVIAvbXTN2088116;     sJtyJghIRupZVIAvbXTN2088116 = sJtyJghIRupZVIAvbXTN71291105;     sJtyJghIRupZVIAvbXTN71291105 = sJtyJghIRupZVIAvbXTN24005701;     sJtyJghIRupZVIAvbXTN24005701 = sJtyJghIRupZVIAvbXTN24721518;     sJtyJghIRupZVIAvbXTN24721518 = sJtyJghIRupZVIAvbXTN31580493;     sJtyJghIRupZVIAvbXTN31580493 = sJtyJghIRupZVIAvbXTN85443444;     sJtyJghIRupZVIAvbXTN85443444 = sJtyJghIRupZVIAvbXTN34985519;     sJtyJghIRupZVIAvbXTN34985519 = sJtyJghIRupZVIAvbXTN18220588;     sJtyJghIRupZVIAvbXTN18220588 = sJtyJghIRupZVIAvbXTN46593005;     sJtyJghIRupZVIAvbXTN46593005 = sJtyJghIRupZVIAvbXTN19951656;     sJtyJghIRupZVIAvbXTN19951656 = sJtyJghIRupZVIAvbXTN61778955;     sJtyJghIRupZVIAvbXTN61778955 = sJtyJghIRupZVIAvbXTN65520600;     sJtyJghIRupZVIAvbXTN65520600 = sJtyJghIRupZVIAvbXTN92107565;     sJtyJghIRupZVIAvbXTN92107565 = sJtyJghIRupZVIAvbXTN78722800;     sJtyJghIRupZVIAvbXTN78722800 = sJtyJghIRupZVIAvbXTN48047547;     sJtyJghIRupZVIAvbXTN48047547 = sJtyJghIRupZVIAvbXTN73709125;     sJtyJghIRupZVIAvbXTN73709125 = sJtyJghIRupZVIAvbXTN234529;     sJtyJghIRupZVIAvbXTN234529 = sJtyJghIRupZVIAvbXTN54987202;     sJtyJghIRupZVIAvbXTN54987202 = sJtyJghIRupZVIAvbXTN47295199;     sJtyJghIRupZVIAvbXTN47295199 = sJtyJghIRupZVIAvbXTN36817235;     sJtyJghIRupZVIAvbXTN36817235 = sJtyJghIRupZVIAvbXTN75811628;     sJtyJghIRupZVIAvbXTN75811628 = sJtyJghIRupZVIAvbXTN73163611;     sJtyJghIRupZVIAvbXTN73163611 = sJtyJghIRupZVIAvbXTN1616283;     sJtyJghIRupZVIAvbXTN1616283 = sJtyJghIRupZVIAvbXTN96061039;     sJtyJghIRupZVIAvbXTN96061039 = sJtyJghIRupZVIAvbXTN68003060;     sJtyJghIRupZVIAvbXTN68003060 = sJtyJghIRupZVIAvbXTN31497990;     sJtyJghIRupZVIAvbXTN31497990 = sJtyJghIRupZVIAvbXTN1114232;     sJtyJghIRupZVIAvbXTN1114232 = sJtyJghIRupZVIAvbXTN86405546;     sJtyJghIRupZVIAvbXTN86405546 = sJtyJghIRupZVIAvbXTN53309036;}
// Junk Finished

// Junk Code By Peatreat & Thaisen's Gen
void aqoxIPXhVceIobJLGkdh85398480() {     int KZQFVuUXbMWKeCcmCdLR85569845 = -820685188;    int KZQFVuUXbMWKeCcmCdLR77885247 = -305706837;    int KZQFVuUXbMWKeCcmCdLR39956455 = -79209493;    int KZQFVuUXbMWKeCcmCdLR63352711 = -677094887;    int KZQFVuUXbMWKeCcmCdLR28279405 = -672588736;    int KZQFVuUXbMWKeCcmCdLR43994172 = -846423875;    int KZQFVuUXbMWKeCcmCdLR52919967 = -319979277;    int KZQFVuUXbMWKeCcmCdLR4938715 = -691534996;    int KZQFVuUXbMWKeCcmCdLR86361299 = -64937871;    int KZQFVuUXbMWKeCcmCdLR64420281 = -416420789;    int KZQFVuUXbMWKeCcmCdLR64573668 = -629720468;    int KZQFVuUXbMWKeCcmCdLR61646268 = -812531939;    int KZQFVuUXbMWKeCcmCdLR4408434 = -532826064;    int KZQFVuUXbMWKeCcmCdLR14457975 = -4265292;    int KZQFVuUXbMWKeCcmCdLR42280460 = -762779896;    int KZQFVuUXbMWKeCcmCdLR67200457 = -137156349;    int KZQFVuUXbMWKeCcmCdLR76892431 = -629572883;    int KZQFVuUXbMWKeCcmCdLR50063334 = -378355277;    int KZQFVuUXbMWKeCcmCdLR13601858 = -895208005;    int KZQFVuUXbMWKeCcmCdLR28161520 = -178053543;    int KZQFVuUXbMWKeCcmCdLR28527530 = -935904420;    int KZQFVuUXbMWKeCcmCdLR327292 = -123277583;    int KZQFVuUXbMWKeCcmCdLR74126526 = -969952783;    int KZQFVuUXbMWKeCcmCdLR19940738 = -218673293;    int KZQFVuUXbMWKeCcmCdLR53617015 = -677230569;    int KZQFVuUXbMWKeCcmCdLR22483013 = -818128606;    int KZQFVuUXbMWKeCcmCdLR74692314 = -310278506;    int KZQFVuUXbMWKeCcmCdLR41265996 = -410394845;    int KZQFVuUXbMWKeCcmCdLR5024735 = -915635022;    int KZQFVuUXbMWKeCcmCdLR2862783 = -738502198;    int KZQFVuUXbMWKeCcmCdLR225824 = -130746573;    int KZQFVuUXbMWKeCcmCdLR81386921 = -50339399;    int KZQFVuUXbMWKeCcmCdLR43544959 = -301496993;    int KZQFVuUXbMWKeCcmCdLR29273909 = -798295994;    int KZQFVuUXbMWKeCcmCdLR14335415 = -967956868;    int KZQFVuUXbMWKeCcmCdLR8348254 = -767752956;    int KZQFVuUXbMWKeCcmCdLR35341206 = -921313726;    int KZQFVuUXbMWKeCcmCdLR61963086 = -465138551;    int KZQFVuUXbMWKeCcmCdLR1129629 = -850002211;    int KZQFVuUXbMWKeCcmCdLR37617086 = -339884437;    int KZQFVuUXbMWKeCcmCdLR8021482 = -695828146;    int KZQFVuUXbMWKeCcmCdLR24709236 = -457506366;    int KZQFVuUXbMWKeCcmCdLR48746513 = -412670136;    int KZQFVuUXbMWKeCcmCdLR87410355 = -787146664;    int KZQFVuUXbMWKeCcmCdLR42395379 = 49741929;    int KZQFVuUXbMWKeCcmCdLR34402657 = -36983217;    int KZQFVuUXbMWKeCcmCdLR38252085 = -198593566;    int KZQFVuUXbMWKeCcmCdLR83830788 = -321233411;    int KZQFVuUXbMWKeCcmCdLR63287217 = -188554502;    int KZQFVuUXbMWKeCcmCdLR61623912 = -11376141;    int KZQFVuUXbMWKeCcmCdLR28890811 = -310384499;    int KZQFVuUXbMWKeCcmCdLR10753772 = -248896912;    int KZQFVuUXbMWKeCcmCdLR4027045 = -387064037;    int KZQFVuUXbMWKeCcmCdLR5145687 = -466571586;    int KZQFVuUXbMWKeCcmCdLR56870950 = -412800636;    int KZQFVuUXbMWKeCcmCdLR85242554 = -597407605;    int KZQFVuUXbMWKeCcmCdLR3758722 = -335754054;    int KZQFVuUXbMWKeCcmCdLR20015717 = -860536200;    int KZQFVuUXbMWKeCcmCdLR9735696 = -999864318;    int KZQFVuUXbMWKeCcmCdLR5796393 = -854460130;    int KZQFVuUXbMWKeCcmCdLR69301857 = -436145370;    int KZQFVuUXbMWKeCcmCdLR11653971 = -909584433;    int KZQFVuUXbMWKeCcmCdLR99913979 = -775899975;    int KZQFVuUXbMWKeCcmCdLR83498516 = -326435674;    int KZQFVuUXbMWKeCcmCdLR64194458 = -185674216;    int KZQFVuUXbMWKeCcmCdLR83186747 = -479381069;    int KZQFVuUXbMWKeCcmCdLR18101310 = -411034947;    int KZQFVuUXbMWKeCcmCdLR75134524 = -734530071;    int KZQFVuUXbMWKeCcmCdLR122561 = -36308424;    int KZQFVuUXbMWKeCcmCdLR33932207 = -995026941;    int KZQFVuUXbMWKeCcmCdLR31859252 = -215842623;    int KZQFVuUXbMWKeCcmCdLR14929345 = -64434332;    int KZQFVuUXbMWKeCcmCdLR48933705 = -528353066;    int KZQFVuUXbMWKeCcmCdLR75984772 = -455323569;    int KZQFVuUXbMWKeCcmCdLR20140039 = -482225398;    int KZQFVuUXbMWKeCcmCdLR3818295 = -378398054;    int KZQFVuUXbMWKeCcmCdLR51580778 = -710607447;    int KZQFVuUXbMWKeCcmCdLR86716170 = -82806120;    int KZQFVuUXbMWKeCcmCdLR77545359 = -168415222;    int KZQFVuUXbMWKeCcmCdLR19214359 = -540247353;    int KZQFVuUXbMWKeCcmCdLR84230928 = -519535041;    int KZQFVuUXbMWKeCcmCdLR90861525 = -989045095;    int KZQFVuUXbMWKeCcmCdLR77978778 = -121840343;    int KZQFVuUXbMWKeCcmCdLR43400823 = -804258882;    int KZQFVuUXbMWKeCcmCdLR73971971 = -328117700;    int KZQFVuUXbMWKeCcmCdLR89472052 = -881849662;    int KZQFVuUXbMWKeCcmCdLR77359876 = -663275362;    int KZQFVuUXbMWKeCcmCdLR38399272 = -834925407;    int KZQFVuUXbMWKeCcmCdLR72402959 = -285495359;    int KZQFVuUXbMWKeCcmCdLR29092860 = -270549263;    int KZQFVuUXbMWKeCcmCdLR4589532 = -331998902;    int KZQFVuUXbMWKeCcmCdLR15325489 = 39222473;    int KZQFVuUXbMWKeCcmCdLR52227391 = -465274234;    int KZQFVuUXbMWKeCcmCdLR95333236 = -995542082;    int KZQFVuUXbMWKeCcmCdLR68315228 = -903739068;    int KZQFVuUXbMWKeCcmCdLR96367510 = -786243714;    int KZQFVuUXbMWKeCcmCdLR24795256 = -681606392;    int KZQFVuUXbMWKeCcmCdLR65247996 = 13765537;    int KZQFVuUXbMWKeCcmCdLR23215898 = -501472449;    int KZQFVuUXbMWKeCcmCdLR59208631 = -820685188;     KZQFVuUXbMWKeCcmCdLR85569845 = KZQFVuUXbMWKeCcmCdLR77885247;     KZQFVuUXbMWKeCcmCdLR77885247 = KZQFVuUXbMWKeCcmCdLR39956455;     KZQFVuUXbMWKeCcmCdLR39956455 = KZQFVuUXbMWKeCcmCdLR63352711;     KZQFVuUXbMWKeCcmCdLR63352711 = KZQFVuUXbMWKeCcmCdLR28279405;     KZQFVuUXbMWKeCcmCdLR28279405 = KZQFVuUXbMWKeCcmCdLR43994172;     KZQFVuUXbMWKeCcmCdLR43994172 = KZQFVuUXbMWKeCcmCdLR52919967;     KZQFVuUXbMWKeCcmCdLR52919967 = KZQFVuUXbMWKeCcmCdLR4938715;     KZQFVuUXbMWKeCcmCdLR4938715 = KZQFVuUXbMWKeCcmCdLR86361299;     KZQFVuUXbMWKeCcmCdLR86361299 = KZQFVuUXbMWKeCcmCdLR64420281;     KZQFVuUXbMWKeCcmCdLR64420281 = KZQFVuUXbMWKeCcmCdLR64573668;     KZQFVuUXbMWKeCcmCdLR64573668 = KZQFVuUXbMWKeCcmCdLR61646268;     KZQFVuUXbMWKeCcmCdLR61646268 = KZQFVuUXbMWKeCcmCdLR4408434;     KZQFVuUXbMWKeCcmCdLR4408434 = KZQFVuUXbMWKeCcmCdLR14457975;     KZQFVuUXbMWKeCcmCdLR14457975 = KZQFVuUXbMWKeCcmCdLR42280460;     KZQFVuUXbMWKeCcmCdLR42280460 = KZQFVuUXbMWKeCcmCdLR67200457;     KZQFVuUXbMWKeCcmCdLR67200457 = KZQFVuUXbMWKeCcmCdLR76892431;     KZQFVuUXbMWKeCcmCdLR76892431 = KZQFVuUXbMWKeCcmCdLR50063334;     KZQFVuUXbMWKeCcmCdLR50063334 = KZQFVuUXbMWKeCcmCdLR13601858;     KZQFVuUXbMWKeCcmCdLR13601858 = KZQFVuUXbMWKeCcmCdLR28161520;     KZQFVuUXbMWKeCcmCdLR28161520 = KZQFVuUXbMWKeCcmCdLR28527530;     KZQFVuUXbMWKeCcmCdLR28527530 = KZQFVuUXbMWKeCcmCdLR327292;     KZQFVuUXbMWKeCcmCdLR327292 = KZQFVuUXbMWKeCcmCdLR74126526;     KZQFVuUXbMWKeCcmCdLR74126526 = KZQFVuUXbMWKeCcmCdLR19940738;     KZQFVuUXbMWKeCcmCdLR19940738 = KZQFVuUXbMWKeCcmCdLR53617015;     KZQFVuUXbMWKeCcmCdLR53617015 = KZQFVuUXbMWKeCcmCdLR22483013;     KZQFVuUXbMWKeCcmCdLR22483013 = KZQFVuUXbMWKeCcmCdLR74692314;     KZQFVuUXbMWKeCcmCdLR74692314 = KZQFVuUXbMWKeCcmCdLR41265996;     KZQFVuUXbMWKeCcmCdLR41265996 = KZQFVuUXbMWKeCcmCdLR5024735;     KZQFVuUXbMWKeCcmCdLR5024735 = KZQFVuUXbMWKeCcmCdLR2862783;     KZQFVuUXbMWKeCcmCdLR2862783 = KZQFVuUXbMWKeCcmCdLR225824;     KZQFVuUXbMWKeCcmCdLR225824 = KZQFVuUXbMWKeCcmCdLR81386921;     KZQFVuUXbMWKeCcmCdLR81386921 = KZQFVuUXbMWKeCcmCdLR43544959;     KZQFVuUXbMWKeCcmCdLR43544959 = KZQFVuUXbMWKeCcmCdLR29273909;     KZQFVuUXbMWKeCcmCdLR29273909 = KZQFVuUXbMWKeCcmCdLR14335415;     KZQFVuUXbMWKeCcmCdLR14335415 = KZQFVuUXbMWKeCcmCdLR8348254;     KZQFVuUXbMWKeCcmCdLR8348254 = KZQFVuUXbMWKeCcmCdLR35341206;     KZQFVuUXbMWKeCcmCdLR35341206 = KZQFVuUXbMWKeCcmCdLR61963086;     KZQFVuUXbMWKeCcmCdLR61963086 = KZQFVuUXbMWKeCcmCdLR1129629;     KZQFVuUXbMWKeCcmCdLR1129629 = KZQFVuUXbMWKeCcmCdLR37617086;     KZQFVuUXbMWKeCcmCdLR37617086 = KZQFVuUXbMWKeCcmCdLR8021482;     KZQFVuUXbMWKeCcmCdLR8021482 = KZQFVuUXbMWKeCcmCdLR24709236;     KZQFVuUXbMWKeCcmCdLR24709236 = KZQFVuUXbMWKeCcmCdLR48746513;     KZQFVuUXbMWKeCcmCdLR48746513 = KZQFVuUXbMWKeCcmCdLR87410355;     KZQFVuUXbMWKeCcmCdLR87410355 = KZQFVuUXbMWKeCcmCdLR42395379;     KZQFVuUXbMWKeCcmCdLR42395379 = KZQFVuUXbMWKeCcmCdLR34402657;     KZQFVuUXbMWKeCcmCdLR34402657 = KZQFVuUXbMWKeCcmCdLR38252085;     KZQFVuUXbMWKeCcmCdLR38252085 = KZQFVuUXbMWKeCcmCdLR83830788;     KZQFVuUXbMWKeCcmCdLR83830788 = KZQFVuUXbMWKeCcmCdLR63287217;     KZQFVuUXbMWKeCcmCdLR63287217 = KZQFVuUXbMWKeCcmCdLR61623912;     KZQFVuUXbMWKeCcmCdLR61623912 = KZQFVuUXbMWKeCcmCdLR28890811;     KZQFVuUXbMWKeCcmCdLR28890811 = KZQFVuUXbMWKeCcmCdLR10753772;     KZQFVuUXbMWKeCcmCdLR10753772 = KZQFVuUXbMWKeCcmCdLR4027045;     KZQFVuUXbMWKeCcmCdLR4027045 = KZQFVuUXbMWKeCcmCdLR5145687;     KZQFVuUXbMWKeCcmCdLR5145687 = KZQFVuUXbMWKeCcmCdLR56870950;     KZQFVuUXbMWKeCcmCdLR56870950 = KZQFVuUXbMWKeCcmCdLR85242554;     KZQFVuUXbMWKeCcmCdLR85242554 = KZQFVuUXbMWKeCcmCdLR3758722;     KZQFVuUXbMWKeCcmCdLR3758722 = KZQFVuUXbMWKeCcmCdLR20015717;     KZQFVuUXbMWKeCcmCdLR20015717 = KZQFVuUXbMWKeCcmCdLR9735696;     KZQFVuUXbMWKeCcmCdLR9735696 = KZQFVuUXbMWKeCcmCdLR5796393;     KZQFVuUXbMWKeCcmCdLR5796393 = KZQFVuUXbMWKeCcmCdLR69301857;     KZQFVuUXbMWKeCcmCdLR69301857 = KZQFVuUXbMWKeCcmCdLR11653971;     KZQFVuUXbMWKeCcmCdLR11653971 = KZQFVuUXbMWKeCcmCdLR99913979;     KZQFVuUXbMWKeCcmCdLR99913979 = KZQFVuUXbMWKeCcmCdLR83498516;     KZQFVuUXbMWKeCcmCdLR83498516 = KZQFVuUXbMWKeCcmCdLR64194458;     KZQFVuUXbMWKeCcmCdLR64194458 = KZQFVuUXbMWKeCcmCdLR83186747;     KZQFVuUXbMWKeCcmCdLR83186747 = KZQFVuUXbMWKeCcmCdLR18101310;     KZQFVuUXbMWKeCcmCdLR18101310 = KZQFVuUXbMWKeCcmCdLR75134524;     KZQFVuUXbMWKeCcmCdLR75134524 = KZQFVuUXbMWKeCcmCdLR122561;     KZQFVuUXbMWKeCcmCdLR122561 = KZQFVuUXbMWKeCcmCdLR33932207;     KZQFVuUXbMWKeCcmCdLR33932207 = KZQFVuUXbMWKeCcmCdLR31859252;     KZQFVuUXbMWKeCcmCdLR31859252 = KZQFVuUXbMWKeCcmCdLR14929345;     KZQFVuUXbMWKeCcmCdLR14929345 = KZQFVuUXbMWKeCcmCdLR48933705;     KZQFVuUXbMWKeCcmCdLR48933705 = KZQFVuUXbMWKeCcmCdLR75984772;     KZQFVuUXbMWKeCcmCdLR75984772 = KZQFVuUXbMWKeCcmCdLR20140039;     KZQFVuUXbMWKeCcmCdLR20140039 = KZQFVuUXbMWKeCcmCdLR3818295;     KZQFVuUXbMWKeCcmCdLR3818295 = KZQFVuUXbMWKeCcmCdLR51580778;     KZQFVuUXbMWKeCcmCdLR51580778 = KZQFVuUXbMWKeCcmCdLR86716170;     KZQFVuUXbMWKeCcmCdLR86716170 = KZQFVuUXbMWKeCcmCdLR77545359;     KZQFVuUXbMWKeCcmCdLR77545359 = KZQFVuUXbMWKeCcmCdLR19214359;     KZQFVuUXbMWKeCcmCdLR19214359 = KZQFVuUXbMWKeCcmCdLR84230928;     KZQFVuUXbMWKeCcmCdLR84230928 = KZQFVuUXbMWKeCcmCdLR90861525;     KZQFVuUXbMWKeCcmCdLR90861525 = KZQFVuUXbMWKeCcmCdLR77978778;     KZQFVuUXbMWKeCcmCdLR77978778 = KZQFVuUXbMWKeCcmCdLR43400823;     KZQFVuUXbMWKeCcmCdLR43400823 = KZQFVuUXbMWKeCcmCdLR73971971;     KZQFVuUXbMWKeCcmCdLR73971971 = KZQFVuUXbMWKeCcmCdLR89472052;     KZQFVuUXbMWKeCcmCdLR89472052 = KZQFVuUXbMWKeCcmCdLR77359876;     KZQFVuUXbMWKeCcmCdLR77359876 = KZQFVuUXbMWKeCcmCdLR38399272;     KZQFVuUXbMWKeCcmCdLR38399272 = KZQFVuUXbMWKeCcmCdLR72402959;     KZQFVuUXbMWKeCcmCdLR72402959 = KZQFVuUXbMWKeCcmCdLR29092860;     KZQFVuUXbMWKeCcmCdLR29092860 = KZQFVuUXbMWKeCcmCdLR4589532;     KZQFVuUXbMWKeCcmCdLR4589532 = KZQFVuUXbMWKeCcmCdLR15325489;     KZQFVuUXbMWKeCcmCdLR15325489 = KZQFVuUXbMWKeCcmCdLR52227391;     KZQFVuUXbMWKeCcmCdLR52227391 = KZQFVuUXbMWKeCcmCdLR95333236;     KZQFVuUXbMWKeCcmCdLR95333236 = KZQFVuUXbMWKeCcmCdLR68315228;     KZQFVuUXbMWKeCcmCdLR68315228 = KZQFVuUXbMWKeCcmCdLR96367510;     KZQFVuUXbMWKeCcmCdLR96367510 = KZQFVuUXbMWKeCcmCdLR24795256;     KZQFVuUXbMWKeCcmCdLR24795256 = KZQFVuUXbMWKeCcmCdLR65247996;     KZQFVuUXbMWKeCcmCdLR65247996 = KZQFVuUXbMWKeCcmCdLR23215898;     KZQFVuUXbMWKeCcmCdLR23215898 = KZQFVuUXbMWKeCcmCdLR59208631;     KZQFVuUXbMWKeCcmCdLR59208631 = KZQFVuUXbMWKeCcmCdLR85569845;}
// Junk Finished
