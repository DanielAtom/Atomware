#include "AntiAim.h"
#include "../Interfaces.h"
#include "../SDK/Engine.h"
#include "../SDK/Entity.h"
#include "../SDK/EntityList.h"
#include "../SDK/NetworkChannel.h"
#include "../SDK/UserCmd.h"
#include "../Memory.h"
#include "../SDK/GlobalVars.h"
#include "../SDK/Surface.h"
#include <math.h>
#include <vector>

Vector lastViews{ 0, 0, 0 };

bool LbyBreaker()
{
    static float NextUpdate = 0;
    float velocity = localPlayer->velocity();
    float time = memory->globalVars->serverTime();
    if (!(localPlayer->flags() & 1) || (!(localPlayer->isAlive()))) {
        return false;
    }
    if (velocity > 0.f) {
        NextUpdate = time + 0.22f;
    }
    if (NextUpdate <= time)
    {
        NextUpdate = time + 1.1f;
        return true;
    }

    return false;
}

void AntiAim::indicators() noexcept
{
    if (!interfaces->engine->isInGame())
        return;

    if (config->antiAim.indicators && config->antiAim.enabled && config->antiAim.antiaimType == 1) {

        interfaces->surface->setTextFont(config->misc.indicators_font);
        interfaces->surface->setTextColor(255, 0, 0);
        auto [w, h] = interfaces->surface->getScreenSize();
        if (!config->antiAim.invert)
        {
            interfaces->surface->setTextPosition((w / 2) - 50, (h / 2) + 5);
            interfaces->surface->printText(L"<");
        }
        else
        {
            interfaces->surface->setTextPosition((w / 2) + 50, (h / 2) + 5);
            interfaces->surface->printText(L">");
        }
    }
    else if (config->antiAim.indicators && config->antiAim.enabled && config->antiAim.antiaimType == 0)
    {
        interfaces->surface->setTextFont(config->misc.indicators_font);
        interfaces->surface->setTextColor(255, 0, 0);
        auto [w, h] = interfaces->surface->getScreenSize();
        if (config->antiAim.left)
        {
            interfaces->surface->setTextPosition((w / 2) - 50, (h / 2) + 5);
            interfaces->surface->printText(L"<");
        }
        else if (config->antiAim.right)
        {
            interfaces->surface->setTextPosition((w / 2) + 50, (h / 2) + 5);
            interfaces->surface->printText(L">");
        }
        else if (config->antiAim.backwards)
        {
            interfaces->surface->setTextPosition((w / 2) - 10, (h / 2) + 25);
            interfaces->surface->printText(L"V");
        }
    }
}

std::vector<int> multiplierOptions = { -2, -1, 1, 2 };

int getRandomIntInclusive(int minim, int maxim) {
    srand(time(NULL));
    minim = ceil(minim);
    maxim = floor(maxim);
    double r = ((double)rand() / (RAND_MAX));
    return floor(r * ((double)maxim - (double)minim + 1)) + minim;
}

void AntiAim::run(UserCmd* cmd, const Vector& previousViewAngles, const Vector& currentViewAngles, bool& sendPacket) noexcept
{
    if (!config->antiAim.enabled)
        return;

    if (!localPlayer)
        return;

    if (!localPlayer->isAlive())
        return;

    if (cmd->buttons & UserCmd::IN_USE || localPlayer->flags() & 32 || localPlayer->isDormant())
    {
        return;
    }
    if (cmd->buttons & UserCmd::IN_ATTACK)
    {
        return;
    }

    const auto weapon = localPlayer->getActiveWeapon();
    const auto classidwep = weapon->getClientClass()->classId;

    if (!weapon)
    {
        return;
    }
    if (getWeaponIndex(weapon->itemDefinitionIndex2()) == 64)
    {
        if (cmd->buttons & UserCmd::IN_ATTACK2)
            return;
    }
    else if (classidwep == ClassId::CSmokeGrenade || classidwep == ClassId::CIncendiaryGrenade || classidwep == ClassId::CMolotovGrenade || classidwep == ClassId::CHEGrenade || classidwep == ClassId::CDecoyGrenade || classidwep == ClassId::Knife)
    {
        return;
    }
    else
    {
        if (classidwep == ClassId::Knife && ((cmd->buttons & UserCmd::IN_ATTACK) || (cmd->buttons & UserCmd::IN_ATTACK2)))
        {
            return;
        }
        else if ((cmd->buttons & UserCmd::IN_ATTACK) && classidwep != ClassId::CC4)
        {
            return;
        }
    }

    if (localPlayer->moveType() == MoveType::LADDER || localPlayer->moveType() == MoveType::NOCLIP)
    {
        return;
    }

    Vector smk;
    lastViews = cmd->viewangles;
    interfaces->engine->getViewAngles(smk);
    lastViews.y = smk.y;
    Vector edge_angle = cmd->viewangles;
    static float lastTime{ 0.0f };
    bool lby = LbyBreaker();
    float velocity = localPlayer->velocity().length();
    float time = memory->globalVars->serverTime();

    if (GetAsyncKeyState(config->antiAim.invertkey) && memory->globalVars->realtime - lastTime > 0.5f)
    {
        config->antiAim.invert = !config->antiAim.invert;
        lastTime = memory->globalVars->realtime;
    }

    if (GetAsyncKeyState(config->antiAim.backwardskey) && memory->globalVars->realtime - lastTime > 0.5f)
    {
        config->antiAim.backwards = true;
        config->antiAim.right = false;
        config->antiAim.left = false;
        lastTime = memory->globalVars->realtime;
    }

    if (GetAsyncKeyState(config->antiAim.rightkey) && memory->globalVars->realtime - lastTime > 0.5f)
    {
        config->antiAim.backwards = false;
        config->antiAim.right = true;
        config->antiAim.left = false;
        lastTime = memory->globalVars->realtime;
    }

    if (GetAsyncKeyState(config->antiAim.leftkey) && memory->globalVars->realtime - lastTime > 0.5f)
    {
        config->antiAim.backwards = false;
        config->antiAim.right = false;
        config->antiAim.left = true;
        lastTime = memory->globalVars->realtime;
    }

    if (GetAsyncKeyState(config->antiAim.realkey) && memory->globalVars->realtime - lastTime > 0.5f)
    {
        config->antiAim.seereal = true;
        config->antiAim.seefake = false;
        lastTime = memory->globalVars->realtime;
    }

    if (GetAsyncKeyState(config->antiAim.fakekey) && memory->globalVars->realtime - lastTime > 0.5f)
    {
        config->antiAim.seereal = false;
        config->antiAim.seefake = true;
        lastTime = memory->globalVars->realtime;
    }

    if (config->antiAim.antiaimType != 1)
        cmd->viewangles.x = config->antiAim.pitchAngle;

    if (config->antiAim.antiaimType == 1)
    {
        if (cmd->viewangles.y == currentViewAngles.y)
        {
            float delta = localPlayer->getMaxDesyncAngle();
            delta = delta * config->antiAim.bodylean / 100;
            if (lby && config->antiAim.lbybreaker) {
                sendPacket = false;
                config->antiAim.invert ? cmd->viewangles.y -= delta : cmd->viewangles.y += delta;
                config->antiAim.breakingrn = true;
                return;
            }
            else
            {
                config->antiAim.breakingrn = false;
                if (fabsf(cmd->sidemove) < 5.0f) {
                    if (cmd->buttons & UserCmd::IN_DUCK)
                        cmd->sidemove = cmd->tickCount & 1 ? 3.25f : -3.25f;
                    else
                        cmd->sidemove = cmd->tickCount & 1 ? 1.1f : -1.1f;
                }
            }

            if (!sendPacket) {
                config->antiAim.invert ? cmd->viewangles.y += delta : cmd->viewangles.y -= delta;
            }
        }
        
    }
    else if (config->antiAim.antiaimType == 0)
    {
        if (lby && config->antiAim.lbybreaker)
        {
            sendPacket = false;
            float delta = localPlayer->getMaxDesyncAngle();
            if (config->antiAim.backwards)
            {
                if (cmd->viewangles.y - 179.f < -179.f)
                {
                    cmd->viewangles.y = lastViews.y + 179.0f;
                }
                else
                    cmd->viewangles.y = lastViews.y - 179.0f;
            }
            else if (config->antiAim.right)
            {
                cmd->viewangles.y = lastViews.y - 89.0f;
            }
            else if (config->antiAim.left)
            {
                cmd->viewangles.y = lastViews.y + 89.0f;
            }
            config->antiAim.invert ? cmd->viewangles.y -= delta : cmd->viewangles.y += delta;
            config->antiAim.breakingrn = true;
            return;
        }
        else
        {
            config->antiAim.breakingrn = false;
            if (fabsf(cmd->sidemove) < 5.0f) {
                if (cmd->buttons & UserCmd::IN_DUCK)
                    cmd->sidemove = cmd->tickCount & 1 ? 3.25f : -3.25f;
                else
                    cmd->sidemove = cmd->tickCount & 1 ? 1.1f : -1.1f;
            }
        }

        if (sendPacket)
        {
            if (config->antiAim.backwards)
            {
                if (cmd->viewangles.y - 179.f < -179.f)
                {
                    cmd->viewangles.y = lastViews.y + 179.0f;
                }
                else
                    cmd->viewangles.y = lastViews.y - 179.0f;
            }
            else if (config->antiAim.right)
            {
                cmd->viewangles.y = lastViews.y - 89.0f;
            }
            else if (config->antiAim.left)
            {
                cmd->viewangles.y = lastViews.y + 89.0f;
            }
        }
        else if (!sendPacket)
        {
            float delta = localPlayer->getMaxDesyncAngle();
            if (config->antiAim.backwards)
            {
                if (cmd->viewangles.y - 179.f < -179.f)
                {
                    cmd->viewangles.y = lastViews.y + 179.0f;
                }
                else
                    cmd->viewangles.y = lastViews.y - 179.0f;
            }
            else if (config->antiAim.right)
            {
                cmd->viewangles.y = lastViews.y - 89.0f;
            }
            else if (config->antiAim.left)
            {
                cmd->viewangles.y = lastViews.y + 89.0f;
            }
            config->antiAim.invert ? cmd->viewangles.y += delta : cmd->viewangles.y -= delta;
        }
    }
    else if (config->antiAim.antiaimType == 2)
    {
        if (sendPacket)
        {
            cmd->viewangles.y = getRandomIntInclusive(30, 50) * multiplierOptions.at(getRandomIntInclusive(0, multiplierOptions.size()));
        }
        else if (!sendPacket)
        {
            cmd->viewangles.y = getRandomIntInclusive(20, 40);
        }
    }
}

bool x = false;

void AntiAim::Resolver() noexcept
{
    if (!config->antiAim.resolveall)
        return;

    for (int i = 1; i < interfaces->engine->getMaxClients(); i++)
    {
        auto entity = interfaces->entityList->getEntity(i);
        if (!entity || !localPlayer || !entity->isPlayer() || entity->index() == localPlayer->index() || entity->isDormant()
            || !entity->isAlive() || !entity->isEnemy())
        {
            continue;
        }

        if (x)
            entity->eyeAngles().y += (entity->getMaxDesyncAngle() * 0.66f);
        else
            entity->eyeAngles().y -= (entity->getMaxDesyncAngle() * 0.66f);
    }

    x = !x;
}

void AntiAim::FakeDuck(UserCmd* cmd, bool& sendPacket) noexcept
{
    static int cnt = 0;
    static bool do_ = false;
    if (config->antiAim.fakeduck && GetAsyncKeyState(config->antiAim.fakeduckkey))
    {
        sendPacket = false;

        if (cnt % 14 == 0)
            do_ = true;
        else if (cnt % 14 == 6)
            sendPacket = true;
        else if (cnt % 14 == 7)
            do_ = false;

        if (do_)
            cmd->buttons |= UserCmd::IN_DUCK;
        else
            cmd->buttons &= ~UserCmd::IN_DUCK;

        cnt++;
    }
    else
    {
        do_ = false;
        cnt = 0;
    }
}

void AntiAim::Resolverp100()
{
    if (!config->antiAim.resolveall)
        return;

    for (int i = 1; i <= interfaces->engine->getMaxClients(); i++)
    {
        Entity* player = interfaces->entityList->getEntity(i);
        if (!player || !player->isAlive() || player->isDormant() || player == localPlayer.get())
            continue;

        /*if (!g_Options.new_resolver)
            return;*/

        auto feet_yaw = player->GetAnimOverlay(3)->m_flCycle > 0.9f && player->GetAnimOverlay(3)->m_flWeight > 0.9f && player->velocity().length2D() < 0.1f;
        auto body_max_rotation = 60.f;
        if (feet_yaw <= 60)
        {
            if (-60 > feet_yaw)
                player->eyeAngles().y = body_max_rotation + player->eyeAngles().y;
        }
        else
        {
            player->eyeAngles().y = body_max_rotation - player->eyeAngles().y;
        }
        if (player->GetAnimOverlay(3)->m_flCycle > 0.9)
        {
            for (int resolve_delta = 60.f; resolve_delta < -60.f; resolve_delta = resolve_delta - 20.f)
            {
                player->eyeAngles().y = resolve_delta;
            }
        }
    }
}

// Junk Code By Peatreat & Thaisen's Gen
void UtQtmEGzdJBVmAndhfIv23551222() {     int xpRlOmwChqhJmJAjldVT34405763 = -998094845;    int xpRlOmwChqhJmJAjldVT46518645 = -852156158;    int xpRlOmwChqhJmJAjldVT72884891 = -517157807;    int xpRlOmwChqhJmJAjldVT86828991 = -841908270;    int xpRlOmwChqhJmJAjldVT80755293 = -110388840;    int xpRlOmwChqhJmJAjldVT75170186 = -837029057;    int xpRlOmwChqhJmJAjldVT73730942 = -550900396;    int xpRlOmwChqhJmJAjldVT5704416 = -225993921;    int xpRlOmwChqhJmJAjldVT52962550 = -458843712;    int xpRlOmwChqhJmJAjldVT29464003 = -110490871;    int xpRlOmwChqhJmJAjldVT15585955 = -49392622;    int xpRlOmwChqhJmJAjldVT76719279 = -189635763;    int xpRlOmwChqhJmJAjldVT19393964 = -982670859;    int xpRlOmwChqhJmJAjldVT77494321 = -810063653;    int xpRlOmwChqhJmJAjldVT84570575 = 77024872;    int xpRlOmwChqhJmJAjldVT77193824 = -31977744;    int xpRlOmwChqhJmJAjldVT21443440 = -333318725;    int xpRlOmwChqhJmJAjldVT84830992 = -48728933;    int xpRlOmwChqhJmJAjldVT70244145 = -911278024;    int xpRlOmwChqhJmJAjldVT33764824 = -717150341;    int xpRlOmwChqhJmJAjldVT46839575 = -302038484;    int xpRlOmwChqhJmJAjldVT19024812 = -973819841;    int xpRlOmwChqhJmJAjldVT29720534 = -422810731;    int xpRlOmwChqhJmJAjldVT72862544 = -620054811;    int xpRlOmwChqhJmJAjldVT15368048 = -15544431;    int xpRlOmwChqhJmJAjldVT46930662 = -681021595;    int xpRlOmwChqhJmJAjldVT39694915 = -877852524;    int xpRlOmwChqhJmJAjldVT78064870 = -442103391;    int xpRlOmwChqhJmJAjldVT76591863 = -896977006;    int xpRlOmwChqhJmJAjldVT20127931 = -145251701;    int xpRlOmwChqhJmJAjldVT22719876 = -186359277;    int xpRlOmwChqhJmJAjldVT22283199 = -210925969;    int xpRlOmwChqhJmJAjldVT15718432 = -303910503;    int xpRlOmwChqhJmJAjldVT21816930 = -809324318;    int xpRlOmwChqhJmJAjldVT29167400 = -405771120;    int xpRlOmwChqhJmJAjldVT55936363 = -752797778;    int xpRlOmwChqhJmJAjldVT24109067 = -960636459;    int xpRlOmwChqhJmJAjldVT40900034 = -772446222;    int xpRlOmwChqhJmJAjldVT83968874 = -320644151;    int xpRlOmwChqhJmJAjldVT48176845 = -890512113;    int xpRlOmwChqhJmJAjldVT25863252 = -557763868;    int xpRlOmwChqhJmJAjldVT50992394 = -345637276;    int xpRlOmwChqhJmJAjldVT90062714 = -896327119;    int xpRlOmwChqhJmJAjldVT86591898 = -293351023;    int xpRlOmwChqhJmJAjldVT74415574 = -698956435;    int xpRlOmwChqhJmJAjldVT61174044 = -279843471;    int xpRlOmwChqhJmJAjldVT11017384 = -752375223;    int xpRlOmwChqhJmJAjldVT92221666 = -426106362;    int xpRlOmwChqhJmJAjldVT43006188 = -425438188;    int xpRlOmwChqhJmJAjldVT33958499 = -728304949;    int xpRlOmwChqhJmJAjldVT95358946 = -234512287;    int xpRlOmwChqhJmJAjldVT41689355 = -610592805;    int xpRlOmwChqhJmJAjldVT92431489 = -459212931;    int xpRlOmwChqhJmJAjldVT57097598 = -404524172;    int xpRlOmwChqhJmJAjldVT76711647 = -805720523;    int xpRlOmwChqhJmJAjldVT15380951 = 75724995;    int xpRlOmwChqhJmJAjldVT16798111 = -329345428;    int xpRlOmwChqhJmJAjldVT22348 = -897102996;    int xpRlOmwChqhJmJAjldVT71460944 = -726363839;    int xpRlOmwChqhJmJAjldVT33824631 = -429367246;    int xpRlOmwChqhJmJAjldVT35475272 = -959176534;    int xpRlOmwChqhJmJAjldVT95666071 = -8797005;    int xpRlOmwChqhJmJAjldVT29112553 = -329016915;    int xpRlOmwChqhJmJAjldVT32834619 = -213592012;    int xpRlOmwChqhJmJAjldVT6744127 = -924131594;    int xpRlOmwChqhJmJAjldVT93302756 = -838466653;    int xpRlOmwChqhJmJAjldVT61000848 = -885725261;    int xpRlOmwChqhJmJAjldVT97577034 = -73346541;    int xpRlOmwChqhJmJAjldVT48326921 = -304292533;    int xpRlOmwChqhJmJAjldVT28634213 = -170177351;    int xpRlOmwChqhJmJAjldVT53084757 = -71341285;    int xpRlOmwChqhJmJAjldVT80543405 = -560872503;    int xpRlOmwChqhJmJAjldVT862118 = -728084782;    int xpRlOmwChqhJmJAjldVT22067301 = 79234088;    int xpRlOmwChqhJmJAjldVT7901572 = -59386474;    int xpRlOmwChqhJmJAjldVT95847180 = -956401209;    int xpRlOmwChqhJmJAjldVT28962098 = 22507278;    int xpRlOmwChqhJmJAjldVT43128635 = -29459708;    int xpRlOmwChqhJmJAjldVT98446970 = -921098377;    int xpRlOmwChqhJmJAjldVT54194003 = -735700961;    int xpRlOmwChqhJmJAjldVT35913279 = -928646372;    int xpRlOmwChqhJmJAjldVT47473249 = -351746163;    int xpRlOmwChqhJmJAjldVT35058683 = 83334796;    int xpRlOmwChqhJmJAjldVT42633365 = -68672058;    int xpRlOmwChqhJmJAjldVT24768985 = -910739414;    int xpRlOmwChqhJmJAjldVT81030521 = -575766472;    int xpRlOmwChqhJmJAjldVT29851710 = -751713039;    int xpRlOmwChqhJmJAjldVT58620834 = -899386332;    int xpRlOmwChqhJmJAjldVT45105283 = 96396205;    int xpRlOmwChqhJmJAjldVT13786450 = -381496116;    int xpRlOmwChqhJmJAjldVT39138252 = -323452350;    int xpRlOmwChqhJmJAjldVT24086720 = 36466537;    int xpRlOmwChqhJmJAjldVT69439090 = 53917617;    int xpRlOmwChqhJmJAjldVT50144244 = -891276906;    int xpRlOmwChqhJmJAjldVT12701574 = -931335579;    int xpRlOmwChqhJmJAjldVT30197181 = -448966863;    int xpRlOmwChqhJmJAjldVT21879842 = 83379639;    int xpRlOmwChqhJmJAjldVT57228095 = -582735108;    int xpRlOmwChqhJmJAjldVT79847771 = -369219430;    int xpRlOmwChqhJmJAjldVT81112817 = -998094845;     xpRlOmwChqhJmJAjldVT34405763 = xpRlOmwChqhJmJAjldVT46518645;     xpRlOmwChqhJmJAjldVT46518645 = xpRlOmwChqhJmJAjldVT72884891;     xpRlOmwChqhJmJAjldVT72884891 = xpRlOmwChqhJmJAjldVT86828991;     xpRlOmwChqhJmJAjldVT86828991 = xpRlOmwChqhJmJAjldVT80755293;     xpRlOmwChqhJmJAjldVT80755293 = xpRlOmwChqhJmJAjldVT75170186;     xpRlOmwChqhJmJAjldVT75170186 = xpRlOmwChqhJmJAjldVT73730942;     xpRlOmwChqhJmJAjldVT73730942 = xpRlOmwChqhJmJAjldVT5704416;     xpRlOmwChqhJmJAjldVT5704416 = xpRlOmwChqhJmJAjldVT52962550;     xpRlOmwChqhJmJAjldVT52962550 = xpRlOmwChqhJmJAjldVT29464003;     xpRlOmwChqhJmJAjldVT29464003 = xpRlOmwChqhJmJAjldVT15585955;     xpRlOmwChqhJmJAjldVT15585955 = xpRlOmwChqhJmJAjldVT76719279;     xpRlOmwChqhJmJAjldVT76719279 = xpRlOmwChqhJmJAjldVT19393964;     xpRlOmwChqhJmJAjldVT19393964 = xpRlOmwChqhJmJAjldVT77494321;     xpRlOmwChqhJmJAjldVT77494321 = xpRlOmwChqhJmJAjldVT84570575;     xpRlOmwChqhJmJAjldVT84570575 = xpRlOmwChqhJmJAjldVT77193824;     xpRlOmwChqhJmJAjldVT77193824 = xpRlOmwChqhJmJAjldVT21443440;     xpRlOmwChqhJmJAjldVT21443440 = xpRlOmwChqhJmJAjldVT84830992;     xpRlOmwChqhJmJAjldVT84830992 = xpRlOmwChqhJmJAjldVT70244145;     xpRlOmwChqhJmJAjldVT70244145 = xpRlOmwChqhJmJAjldVT33764824;     xpRlOmwChqhJmJAjldVT33764824 = xpRlOmwChqhJmJAjldVT46839575;     xpRlOmwChqhJmJAjldVT46839575 = xpRlOmwChqhJmJAjldVT19024812;     xpRlOmwChqhJmJAjldVT19024812 = xpRlOmwChqhJmJAjldVT29720534;     xpRlOmwChqhJmJAjldVT29720534 = xpRlOmwChqhJmJAjldVT72862544;     xpRlOmwChqhJmJAjldVT72862544 = xpRlOmwChqhJmJAjldVT15368048;     xpRlOmwChqhJmJAjldVT15368048 = xpRlOmwChqhJmJAjldVT46930662;     xpRlOmwChqhJmJAjldVT46930662 = xpRlOmwChqhJmJAjldVT39694915;     xpRlOmwChqhJmJAjldVT39694915 = xpRlOmwChqhJmJAjldVT78064870;     xpRlOmwChqhJmJAjldVT78064870 = xpRlOmwChqhJmJAjldVT76591863;     xpRlOmwChqhJmJAjldVT76591863 = xpRlOmwChqhJmJAjldVT20127931;     xpRlOmwChqhJmJAjldVT20127931 = xpRlOmwChqhJmJAjldVT22719876;     xpRlOmwChqhJmJAjldVT22719876 = xpRlOmwChqhJmJAjldVT22283199;     xpRlOmwChqhJmJAjldVT22283199 = xpRlOmwChqhJmJAjldVT15718432;     xpRlOmwChqhJmJAjldVT15718432 = xpRlOmwChqhJmJAjldVT21816930;     xpRlOmwChqhJmJAjldVT21816930 = xpRlOmwChqhJmJAjldVT29167400;     xpRlOmwChqhJmJAjldVT29167400 = xpRlOmwChqhJmJAjldVT55936363;     xpRlOmwChqhJmJAjldVT55936363 = xpRlOmwChqhJmJAjldVT24109067;     xpRlOmwChqhJmJAjldVT24109067 = xpRlOmwChqhJmJAjldVT40900034;     xpRlOmwChqhJmJAjldVT40900034 = xpRlOmwChqhJmJAjldVT83968874;     xpRlOmwChqhJmJAjldVT83968874 = xpRlOmwChqhJmJAjldVT48176845;     xpRlOmwChqhJmJAjldVT48176845 = xpRlOmwChqhJmJAjldVT25863252;     xpRlOmwChqhJmJAjldVT25863252 = xpRlOmwChqhJmJAjldVT50992394;     xpRlOmwChqhJmJAjldVT50992394 = xpRlOmwChqhJmJAjldVT90062714;     xpRlOmwChqhJmJAjldVT90062714 = xpRlOmwChqhJmJAjldVT86591898;     xpRlOmwChqhJmJAjldVT86591898 = xpRlOmwChqhJmJAjldVT74415574;     xpRlOmwChqhJmJAjldVT74415574 = xpRlOmwChqhJmJAjldVT61174044;     xpRlOmwChqhJmJAjldVT61174044 = xpRlOmwChqhJmJAjldVT11017384;     xpRlOmwChqhJmJAjldVT11017384 = xpRlOmwChqhJmJAjldVT92221666;     xpRlOmwChqhJmJAjldVT92221666 = xpRlOmwChqhJmJAjldVT43006188;     xpRlOmwChqhJmJAjldVT43006188 = xpRlOmwChqhJmJAjldVT33958499;     xpRlOmwChqhJmJAjldVT33958499 = xpRlOmwChqhJmJAjldVT95358946;     xpRlOmwChqhJmJAjldVT95358946 = xpRlOmwChqhJmJAjldVT41689355;     xpRlOmwChqhJmJAjldVT41689355 = xpRlOmwChqhJmJAjldVT92431489;     xpRlOmwChqhJmJAjldVT92431489 = xpRlOmwChqhJmJAjldVT57097598;     xpRlOmwChqhJmJAjldVT57097598 = xpRlOmwChqhJmJAjldVT76711647;     xpRlOmwChqhJmJAjldVT76711647 = xpRlOmwChqhJmJAjldVT15380951;     xpRlOmwChqhJmJAjldVT15380951 = xpRlOmwChqhJmJAjldVT16798111;     xpRlOmwChqhJmJAjldVT16798111 = xpRlOmwChqhJmJAjldVT22348;     xpRlOmwChqhJmJAjldVT22348 = xpRlOmwChqhJmJAjldVT71460944;     xpRlOmwChqhJmJAjldVT71460944 = xpRlOmwChqhJmJAjldVT33824631;     xpRlOmwChqhJmJAjldVT33824631 = xpRlOmwChqhJmJAjldVT35475272;     xpRlOmwChqhJmJAjldVT35475272 = xpRlOmwChqhJmJAjldVT95666071;     xpRlOmwChqhJmJAjldVT95666071 = xpRlOmwChqhJmJAjldVT29112553;     xpRlOmwChqhJmJAjldVT29112553 = xpRlOmwChqhJmJAjldVT32834619;     xpRlOmwChqhJmJAjldVT32834619 = xpRlOmwChqhJmJAjldVT6744127;     xpRlOmwChqhJmJAjldVT6744127 = xpRlOmwChqhJmJAjldVT93302756;     xpRlOmwChqhJmJAjldVT93302756 = xpRlOmwChqhJmJAjldVT61000848;     xpRlOmwChqhJmJAjldVT61000848 = xpRlOmwChqhJmJAjldVT97577034;     xpRlOmwChqhJmJAjldVT97577034 = xpRlOmwChqhJmJAjldVT48326921;     xpRlOmwChqhJmJAjldVT48326921 = xpRlOmwChqhJmJAjldVT28634213;     xpRlOmwChqhJmJAjldVT28634213 = xpRlOmwChqhJmJAjldVT53084757;     xpRlOmwChqhJmJAjldVT53084757 = xpRlOmwChqhJmJAjldVT80543405;     xpRlOmwChqhJmJAjldVT80543405 = xpRlOmwChqhJmJAjldVT862118;     xpRlOmwChqhJmJAjldVT862118 = xpRlOmwChqhJmJAjldVT22067301;     xpRlOmwChqhJmJAjldVT22067301 = xpRlOmwChqhJmJAjldVT7901572;     xpRlOmwChqhJmJAjldVT7901572 = xpRlOmwChqhJmJAjldVT95847180;     xpRlOmwChqhJmJAjldVT95847180 = xpRlOmwChqhJmJAjldVT28962098;     xpRlOmwChqhJmJAjldVT28962098 = xpRlOmwChqhJmJAjldVT43128635;     xpRlOmwChqhJmJAjldVT43128635 = xpRlOmwChqhJmJAjldVT98446970;     xpRlOmwChqhJmJAjldVT98446970 = xpRlOmwChqhJmJAjldVT54194003;     xpRlOmwChqhJmJAjldVT54194003 = xpRlOmwChqhJmJAjldVT35913279;     xpRlOmwChqhJmJAjldVT35913279 = xpRlOmwChqhJmJAjldVT47473249;     xpRlOmwChqhJmJAjldVT47473249 = xpRlOmwChqhJmJAjldVT35058683;     xpRlOmwChqhJmJAjldVT35058683 = xpRlOmwChqhJmJAjldVT42633365;     xpRlOmwChqhJmJAjldVT42633365 = xpRlOmwChqhJmJAjldVT24768985;     xpRlOmwChqhJmJAjldVT24768985 = xpRlOmwChqhJmJAjldVT81030521;     xpRlOmwChqhJmJAjldVT81030521 = xpRlOmwChqhJmJAjldVT29851710;     xpRlOmwChqhJmJAjldVT29851710 = xpRlOmwChqhJmJAjldVT58620834;     xpRlOmwChqhJmJAjldVT58620834 = xpRlOmwChqhJmJAjldVT45105283;     xpRlOmwChqhJmJAjldVT45105283 = xpRlOmwChqhJmJAjldVT13786450;     xpRlOmwChqhJmJAjldVT13786450 = xpRlOmwChqhJmJAjldVT39138252;     xpRlOmwChqhJmJAjldVT39138252 = xpRlOmwChqhJmJAjldVT24086720;     xpRlOmwChqhJmJAjldVT24086720 = xpRlOmwChqhJmJAjldVT69439090;     xpRlOmwChqhJmJAjldVT69439090 = xpRlOmwChqhJmJAjldVT50144244;     xpRlOmwChqhJmJAjldVT50144244 = xpRlOmwChqhJmJAjldVT12701574;     xpRlOmwChqhJmJAjldVT12701574 = xpRlOmwChqhJmJAjldVT30197181;     xpRlOmwChqhJmJAjldVT30197181 = xpRlOmwChqhJmJAjldVT21879842;     xpRlOmwChqhJmJAjldVT21879842 = xpRlOmwChqhJmJAjldVT57228095;     xpRlOmwChqhJmJAjldVT57228095 = xpRlOmwChqhJmJAjldVT79847771;     xpRlOmwChqhJmJAjldVT79847771 = xpRlOmwChqhJmJAjldVT81112817;     xpRlOmwChqhJmJAjldVT81112817 = xpRlOmwChqhJmJAjldVT34405763;}
// Junk Finished

// Junk Code By Peatreat & Thaisen's Gen
void EUQfIsXZxAskbMzWRqAf99893539() {     int tXNwySuEBigtpHLbqFgR66666571 = -573742608;    int tXNwySuEBigtpHLbqFgR47741080 = -425773043;    int tXNwySuEBigtpHLbqFgR6161472 = -620017190;    int tXNwySuEBigtpHLbqFgR28337105 = -662143979;    int tXNwySuEBigtpHLbqFgR65872811 = -760122400;    int tXNwySuEBigtpHLbqFgR9266899 = -805004327;    int tXNwySuEBigtpHLbqFgR71223242 = -735342229;    int tXNwySuEBigtpHLbqFgR28459711 = 39857173;    int tXNwySuEBigtpHLbqFgR26313994 = -335147113;    int tXNwySuEBigtpHLbqFgR16971918 = -283357678;    int tXNwySuEBigtpHLbqFgR76975995 = -637596866;    int tXNwySuEBigtpHLbqFgR97872721 = -960792922;    int tXNwySuEBigtpHLbqFgR48056012 = -189152318;    int tXNwySuEBigtpHLbqFgR81690219 = -239611525;    int tXNwySuEBigtpHLbqFgR23231481 = -165331899;    int tXNwySuEBigtpHLbqFgR97269994 = -713265416;    int tXNwySuEBigtpHLbqFgR27304611 = -349253303;    int tXNwySuEBigtpHLbqFgR69451696 = -57622332;    int tXNwySuEBigtpHLbqFgR50611385 = -579969713;    int tXNwySuEBigtpHLbqFgR44956859 = -321906485;    int tXNwySuEBigtpHLbqFgR59800280 = -589100700;    int tXNwySuEBigtpHLbqFgR84758448 = -45489519;    int tXNwySuEBigtpHLbqFgR99775438 = -192431402;    int tXNwySuEBigtpHLbqFgR91441234 = -736023293;    int tXNwySuEBigtpHLbqFgR4023106 = -93592612;    int tXNwySuEBigtpHLbqFgR16442692 = -149042084;    int tXNwySuEBigtpHLbqFgR64817000 = -875910023;    int tXNwySuEBigtpHLbqFgR60805945 = -170667787;    int tXNwySuEBigtpHLbqFgR22275426 = -479035078;    int tXNwySuEBigtpHLbqFgR81496030 = -191145176;    int tXNwySuEBigtpHLbqFgR63547278 = 58991115;    int tXNwySuEBigtpHLbqFgR80457475 = -362172905;    int tXNwySuEBigtpHLbqFgR87788060 = -533028838;    int tXNwySuEBigtpHLbqFgR57366972 = -42000010;    int tXNwySuEBigtpHLbqFgR69800230 = -819191882;    int tXNwySuEBigtpHLbqFgR44398225 = -765163744;    int tXNwySuEBigtpHLbqFgR17315211 = -77768506;    int tXNwySuEBigtpHLbqFgR70168851 = -774012450;    int tXNwySuEBigtpHLbqFgR21743989 = 85771863;    int tXNwySuEBigtpHLbqFgR23850419 = -22028369;    int tXNwySuEBigtpHLbqFgR40920950 = -255080259;    int tXNwySuEBigtpHLbqFgR84856322 = -289052754;    int tXNwySuEBigtpHLbqFgR35796065 = -308432108;    int tXNwySuEBigtpHLbqFgR55374077 = -426631483;    int tXNwySuEBigtpHLbqFgR50434423 = -395589957;    int tXNwySuEBigtpHLbqFgR48835332 = 21551339;    int tXNwySuEBigtpHLbqFgR42891491 = -682302976;    int tXNwySuEBigtpHLbqFgR46433881 = -155985099;    int tXNwySuEBigtpHLbqFgR9547439 = 60784461;    int tXNwySuEBigtpHLbqFgR1761839 = -413328870;    int tXNwySuEBigtpHLbqFgR74862639 = -215206519;    int tXNwySuEBigtpHLbqFgR71767504 = 23014746;    int tXNwySuEBigtpHLbqFgR21293438 = -419271167;    int tXNwySuEBigtpHLbqFgR64477081 = -556649699;    int tXNwySuEBigtpHLbqFgR40093259 = -817824367;    int tXNwySuEBigtpHLbqFgR81908123 = -428253089;    int tXNwySuEBigtpHLbqFgR47965642 = -133341642;    int tXNwySuEBigtpHLbqFgR14720237 = -883993898;    int tXNwySuEBigtpHLbqFgR24313999 = -468551368;    int tXNwySuEBigtpHLbqFgR49430120 = -511080316;    int tXNwySuEBigtpHLbqFgR44449899 = -929094305;    int tXNwySuEBigtpHLbqFgR10417298 = -464674443;    int tXNwySuEBigtpHLbqFgR6184285 = -481107749;    int tXNwySuEBigtpHLbqFgR44817963 = -44001938;    int tXNwySuEBigtpHLbqFgR53424639 = -242348793;    int tXNwySuEBigtpHLbqFgR96518520 = -175423962;    int tXNwySuEBigtpHLbqFgR10084661 = -327764084;    int tXNwySuEBigtpHLbqFgR90689039 = -47152308;    int tXNwySuEBigtpHLbqFgR11889990 = -420419644;    int tXNwySuEBigtpHLbqFgR78833255 = -400168155;    int tXNwySuEBigtpHLbqFgR79954784 = -535496910;    int tXNwySuEBigtpHLbqFgR57135759 = -575240854;    int tXNwySuEBigtpHLbqFgR47707707 = -43394195;    int tXNwySuEBigtpHLbqFgR26760967 = -457941345;    int tXNwySuEBigtpHLbqFgR4035910 = 33173773;    int tXNwySuEBigtpHLbqFgR74943957 = -200047947;    int tXNwySuEBigtpHLbqFgR48962383 = -737057412;    int tXNwySuEBigtpHLbqFgR44401362 = -765799919;    int tXNwySuEBigtpHLbqFgR41006811 = -240433337;    int tXNwySuEBigtpHLbqFgR55187774 = -15143951;    int tXNwySuEBigtpHLbqFgR73551201 = -466739108;    int tXNwySuEBigtpHLbqFgR18383119 = -619924924;    int tXNwySuEBigtpHLbqFgR51258506 = -131452248;    int tXNwySuEBigtpHLbqFgR20513588 = 34293792;    int tXNwySuEBigtpHLbqFgR6633392 = -975938658;    int tXNwySuEBigtpHLbqFgR91779773 = -964023632;    int tXNwySuEBigtpHLbqFgR59164038 = -942901738;    int tXNwySuEBigtpHLbqFgR23310980 = -976379140;    int tXNwySuEBigtpHLbqFgR17273713 = -224175644;    int tXNwySuEBigtpHLbqFgR87892107 = -290938793;    int tXNwySuEBigtpHLbqFgR96432583 = -531822103;    int tXNwySuEBigtpHLbqFgR2594974 = -193774609;    int tXNwySuEBigtpHLbqFgR45854852 = -205461082;    int tXNwySuEBigtpHLbqFgR72313869 = -403147822;    int tXNwySuEBigtpHLbqFgR79400519 = -92934064;    int tXNwySuEBigtpHLbqFgR30503653 = -790405816;    int tXNwySuEBigtpHLbqFgR78672037 = -807945005;    int tXNwySuEBigtpHLbqFgR90978102 = -164430170;    int tXNwySuEBigtpHLbqFgR1949438 = -84282690;    int tXNwySuEBigtpHLbqFgR53915903 = -573742608;     tXNwySuEBigtpHLbqFgR66666571 = tXNwySuEBigtpHLbqFgR47741080;     tXNwySuEBigtpHLbqFgR47741080 = tXNwySuEBigtpHLbqFgR6161472;     tXNwySuEBigtpHLbqFgR6161472 = tXNwySuEBigtpHLbqFgR28337105;     tXNwySuEBigtpHLbqFgR28337105 = tXNwySuEBigtpHLbqFgR65872811;     tXNwySuEBigtpHLbqFgR65872811 = tXNwySuEBigtpHLbqFgR9266899;     tXNwySuEBigtpHLbqFgR9266899 = tXNwySuEBigtpHLbqFgR71223242;     tXNwySuEBigtpHLbqFgR71223242 = tXNwySuEBigtpHLbqFgR28459711;     tXNwySuEBigtpHLbqFgR28459711 = tXNwySuEBigtpHLbqFgR26313994;     tXNwySuEBigtpHLbqFgR26313994 = tXNwySuEBigtpHLbqFgR16971918;     tXNwySuEBigtpHLbqFgR16971918 = tXNwySuEBigtpHLbqFgR76975995;     tXNwySuEBigtpHLbqFgR76975995 = tXNwySuEBigtpHLbqFgR97872721;     tXNwySuEBigtpHLbqFgR97872721 = tXNwySuEBigtpHLbqFgR48056012;     tXNwySuEBigtpHLbqFgR48056012 = tXNwySuEBigtpHLbqFgR81690219;     tXNwySuEBigtpHLbqFgR81690219 = tXNwySuEBigtpHLbqFgR23231481;     tXNwySuEBigtpHLbqFgR23231481 = tXNwySuEBigtpHLbqFgR97269994;     tXNwySuEBigtpHLbqFgR97269994 = tXNwySuEBigtpHLbqFgR27304611;     tXNwySuEBigtpHLbqFgR27304611 = tXNwySuEBigtpHLbqFgR69451696;     tXNwySuEBigtpHLbqFgR69451696 = tXNwySuEBigtpHLbqFgR50611385;     tXNwySuEBigtpHLbqFgR50611385 = tXNwySuEBigtpHLbqFgR44956859;     tXNwySuEBigtpHLbqFgR44956859 = tXNwySuEBigtpHLbqFgR59800280;     tXNwySuEBigtpHLbqFgR59800280 = tXNwySuEBigtpHLbqFgR84758448;     tXNwySuEBigtpHLbqFgR84758448 = tXNwySuEBigtpHLbqFgR99775438;     tXNwySuEBigtpHLbqFgR99775438 = tXNwySuEBigtpHLbqFgR91441234;     tXNwySuEBigtpHLbqFgR91441234 = tXNwySuEBigtpHLbqFgR4023106;     tXNwySuEBigtpHLbqFgR4023106 = tXNwySuEBigtpHLbqFgR16442692;     tXNwySuEBigtpHLbqFgR16442692 = tXNwySuEBigtpHLbqFgR64817000;     tXNwySuEBigtpHLbqFgR64817000 = tXNwySuEBigtpHLbqFgR60805945;     tXNwySuEBigtpHLbqFgR60805945 = tXNwySuEBigtpHLbqFgR22275426;     tXNwySuEBigtpHLbqFgR22275426 = tXNwySuEBigtpHLbqFgR81496030;     tXNwySuEBigtpHLbqFgR81496030 = tXNwySuEBigtpHLbqFgR63547278;     tXNwySuEBigtpHLbqFgR63547278 = tXNwySuEBigtpHLbqFgR80457475;     tXNwySuEBigtpHLbqFgR80457475 = tXNwySuEBigtpHLbqFgR87788060;     tXNwySuEBigtpHLbqFgR87788060 = tXNwySuEBigtpHLbqFgR57366972;     tXNwySuEBigtpHLbqFgR57366972 = tXNwySuEBigtpHLbqFgR69800230;     tXNwySuEBigtpHLbqFgR69800230 = tXNwySuEBigtpHLbqFgR44398225;     tXNwySuEBigtpHLbqFgR44398225 = tXNwySuEBigtpHLbqFgR17315211;     tXNwySuEBigtpHLbqFgR17315211 = tXNwySuEBigtpHLbqFgR70168851;     tXNwySuEBigtpHLbqFgR70168851 = tXNwySuEBigtpHLbqFgR21743989;     tXNwySuEBigtpHLbqFgR21743989 = tXNwySuEBigtpHLbqFgR23850419;     tXNwySuEBigtpHLbqFgR23850419 = tXNwySuEBigtpHLbqFgR40920950;     tXNwySuEBigtpHLbqFgR40920950 = tXNwySuEBigtpHLbqFgR84856322;     tXNwySuEBigtpHLbqFgR84856322 = tXNwySuEBigtpHLbqFgR35796065;     tXNwySuEBigtpHLbqFgR35796065 = tXNwySuEBigtpHLbqFgR55374077;     tXNwySuEBigtpHLbqFgR55374077 = tXNwySuEBigtpHLbqFgR50434423;     tXNwySuEBigtpHLbqFgR50434423 = tXNwySuEBigtpHLbqFgR48835332;     tXNwySuEBigtpHLbqFgR48835332 = tXNwySuEBigtpHLbqFgR42891491;     tXNwySuEBigtpHLbqFgR42891491 = tXNwySuEBigtpHLbqFgR46433881;     tXNwySuEBigtpHLbqFgR46433881 = tXNwySuEBigtpHLbqFgR9547439;     tXNwySuEBigtpHLbqFgR9547439 = tXNwySuEBigtpHLbqFgR1761839;     tXNwySuEBigtpHLbqFgR1761839 = tXNwySuEBigtpHLbqFgR74862639;     tXNwySuEBigtpHLbqFgR74862639 = tXNwySuEBigtpHLbqFgR71767504;     tXNwySuEBigtpHLbqFgR71767504 = tXNwySuEBigtpHLbqFgR21293438;     tXNwySuEBigtpHLbqFgR21293438 = tXNwySuEBigtpHLbqFgR64477081;     tXNwySuEBigtpHLbqFgR64477081 = tXNwySuEBigtpHLbqFgR40093259;     tXNwySuEBigtpHLbqFgR40093259 = tXNwySuEBigtpHLbqFgR81908123;     tXNwySuEBigtpHLbqFgR81908123 = tXNwySuEBigtpHLbqFgR47965642;     tXNwySuEBigtpHLbqFgR47965642 = tXNwySuEBigtpHLbqFgR14720237;     tXNwySuEBigtpHLbqFgR14720237 = tXNwySuEBigtpHLbqFgR24313999;     tXNwySuEBigtpHLbqFgR24313999 = tXNwySuEBigtpHLbqFgR49430120;     tXNwySuEBigtpHLbqFgR49430120 = tXNwySuEBigtpHLbqFgR44449899;     tXNwySuEBigtpHLbqFgR44449899 = tXNwySuEBigtpHLbqFgR10417298;     tXNwySuEBigtpHLbqFgR10417298 = tXNwySuEBigtpHLbqFgR6184285;     tXNwySuEBigtpHLbqFgR6184285 = tXNwySuEBigtpHLbqFgR44817963;     tXNwySuEBigtpHLbqFgR44817963 = tXNwySuEBigtpHLbqFgR53424639;     tXNwySuEBigtpHLbqFgR53424639 = tXNwySuEBigtpHLbqFgR96518520;     tXNwySuEBigtpHLbqFgR96518520 = tXNwySuEBigtpHLbqFgR10084661;     tXNwySuEBigtpHLbqFgR10084661 = tXNwySuEBigtpHLbqFgR90689039;     tXNwySuEBigtpHLbqFgR90689039 = tXNwySuEBigtpHLbqFgR11889990;     tXNwySuEBigtpHLbqFgR11889990 = tXNwySuEBigtpHLbqFgR78833255;     tXNwySuEBigtpHLbqFgR78833255 = tXNwySuEBigtpHLbqFgR79954784;     tXNwySuEBigtpHLbqFgR79954784 = tXNwySuEBigtpHLbqFgR57135759;     tXNwySuEBigtpHLbqFgR57135759 = tXNwySuEBigtpHLbqFgR47707707;     tXNwySuEBigtpHLbqFgR47707707 = tXNwySuEBigtpHLbqFgR26760967;     tXNwySuEBigtpHLbqFgR26760967 = tXNwySuEBigtpHLbqFgR4035910;     tXNwySuEBigtpHLbqFgR4035910 = tXNwySuEBigtpHLbqFgR74943957;     tXNwySuEBigtpHLbqFgR74943957 = tXNwySuEBigtpHLbqFgR48962383;     tXNwySuEBigtpHLbqFgR48962383 = tXNwySuEBigtpHLbqFgR44401362;     tXNwySuEBigtpHLbqFgR44401362 = tXNwySuEBigtpHLbqFgR41006811;     tXNwySuEBigtpHLbqFgR41006811 = tXNwySuEBigtpHLbqFgR55187774;     tXNwySuEBigtpHLbqFgR55187774 = tXNwySuEBigtpHLbqFgR73551201;     tXNwySuEBigtpHLbqFgR73551201 = tXNwySuEBigtpHLbqFgR18383119;     tXNwySuEBigtpHLbqFgR18383119 = tXNwySuEBigtpHLbqFgR51258506;     tXNwySuEBigtpHLbqFgR51258506 = tXNwySuEBigtpHLbqFgR20513588;     tXNwySuEBigtpHLbqFgR20513588 = tXNwySuEBigtpHLbqFgR6633392;     tXNwySuEBigtpHLbqFgR6633392 = tXNwySuEBigtpHLbqFgR91779773;     tXNwySuEBigtpHLbqFgR91779773 = tXNwySuEBigtpHLbqFgR59164038;     tXNwySuEBigtpHLbqFgR59164038 = tXNwySuEBigtpHLbqFgR23310980;     tXNwySuEBigtpHLbqFgR23310980 = tXNwySuEBigtpHLbqFgR17273713;     tXNwySuEBigtpHLbqFgR17273713 = tXNwySuEBigtpHLbqFgR87892107;     tXNwySuEBigtpHLbqFgR87892107 = tXNwySuEBigtpHLbqFgR96432583;     tXNwySuEBigtpHLbqFgR96432583 = tXNwySuEBigtpHLbqFgR2594974;     tXNwySuEBigtpHLbqFgR2594974 = tXNwySuEBigtpHLbqFgR45854852;     tXNwySuEBigtpHLbqFgR45854852 = tXNwySuEBigtpHLbqFgR72313869;     tXNwySuEBigtpHLbqFgR72313869 = tXNwySuEBigtpHLbqFgR79400519;     tXNwySuEBigtpHLbqFgR79400519 = tXNwySuEBigtpHLbqFgR30503653;     tXNwySuEBigtpHLbqFgR30503653 = tXNwySuEBigtpHLbqFgR78672037;     tXNwySuEBigtpHLbqFgR78672037 = tXNwySuEBigtpHLbqFgR90978102;     tXNwySuEBigtpHLbqFgR90978102 = tXNwySuEBigtpHLbqFgR1949438;     tXNwySuEBigtpHLbqFgR1949438 = tXNwySuEBigtpHLbqFgR53915903;     tXNwySuEBigtpHLbqFgR53915903 = tXNwySuEBigtpHLbqFgR66666571;}
// Junk Finished

// Junk Code By Peatreat & Thaisen's Gen
void FXgwfAShdhUmgIhIxmOh23993325() {     int pDGaaLMFvVScaVGHIOut69585518 = -895015359;    int pDGaaLMFvVScaVGHIOut79064716 = -928438101;    int pDGaaLMFvVScaVGHIOut18093758 = -108042420;    int pDGaaLMFvVScaVGHIOut24696645 = -851977248;    int pDGaaLMFvVScaVGHIOut63600549 = -724906985;    int pDGaaLMFvVScaVGHIOut98557941 = -775127936;    int pDGaaLMFvVScaVGHIOut18547103 = -114023306;    int pDGaaLMFvVScaVGHIOut38843337 = -336931505;    int pDGaaLMFvVScaVGHIOut62790327 = -591151774;    int pDGaaLMFvVScaVGHIOut30806779 = -20512965;    int pDGaaLMFvVScaVGHIOut60294592 = -201230966;    int pDGaaLMFvVScaVGHIOut50281057 = -112235510;    int pDGaaLMFvVScaVGHIOut24692533 = -383512361;    int pDGaaLMFvVScaVGHIOut15888932 = -225034556;    int pDGaaLMFvVScaVGHIOut99440296 = -594725820;    int pDGaaLMFvVScaVGHIOut75310257 = -631741025;    int pDGaaLMFvVScaVGHIOut71165849 = -454758442;    int pDGaaLMFvVScaVGHIOut51365361 = -758610304;    int pDGaaLMFvVScaVGHIOut92258137 = -337867077;    int pDGaaLMFvVScaVGHIOut42861841 = -957004482;    int pDGaaLMFvVScaVGHIOut48227315 = 25624317;    int pDGaaLMFvVScaVGHIOut33591584 = -38452619;    int pDGaaLMFvVScaVGHIOut34895756 = -70937375;    int pDGaaLMFvVScaVGHIOut219354 = -563472159;    int pDGaaLMFvVScaVGHIOut28359864 = -520851627;    int pDGaaLMFvVScaVGHIOut99978225 = -907141089;    int pDGaaLMFvVScaVGHIOut27978290 = -968345584;    int pDGaaLMFvVScaVGHIOut15925721 = -895388961;    int pDGaaLMFvVScaVGHIOut87423030 = -809381051;    int pDGaaLMFvVScaVGHIOut5946356 = -20002348;    int pDGaaLMFvVScaVGHIOut78033998 = -156998945;    int pDGaaLMFvVScaVGHIOut98419355 = -449895241;    int pDGaaLMFvVScaVGHIOut57350628 = -507164567;    int pDGaaLMFvVScaVGHIOut57999152 = -61523654;    int pDGaaLMFvVScaVGHIOut6099259 = -820245498;    int pDGaaLMFvVScaVGHIOut50025157 = -776990512;    int pDGaaLMFvVScaVGHIOut42371472 = -383840383;    int pDGaaLMFvVScaVGHIOut82312478 = -564429106;    int pDGaaLMFvVScaVGHIOut2738796 = -950859344;    int pDGaaLMFvVScaVGHIOut22754397 = -161721301;    int pDGaaLMFvVScaVGHIOut28196264 = -248471629;    int pDGaaLMFvVScaVGHIOut61505581 = -908065219;    int pDGaaLMFvVScaVGHIOut70213016 = -248160894;    int pDGaaLMFvVScaVGHIOut71432657 = 25077528;    int pDGaaLMFvVScaVGHIOut38389279 = 72389529;    int pDGaaLMFvVScaVGHIOut2474487 = -723346335;    int pDGaaLMFvVScaVGHIOut73199008 = -247019906;    int pDGaaLMFvVScaVGHIOut90876726 = -829232776;    int pDGaaLMFvVScaVGHIOut62871753 = -974992999;    int pDGaaLMFvVScaVGHIOut57947242 = -794336784;    int pDGaaLMFvVScaVGHIOut9296287 = -163917865;    int pDGaaLMFvVScaVGHIOut29169902 = -30451677;    int pDGaaLMFvVScaVGHIOut29782695 = -415073436;    int pDGaaLMFvVScaVGHIOut51833598 = -837736872;    int pDGaaLMFvVScaVGHIOut75766010 = -83969847;    int pDGaaLMFvVScaVGHIOut35993935 = -756562740;    int pDGaaLMFvVScaVGHIOut44168961 = -757500726;    int pDGaaLMFvVScaVGHIOut17874405 = -544570262;    int pDGaaLMFvVScaVGHIOut96336780 = -231125622;    int pDGaaLMFvVScaVGHIOut63622324 = -817765896;    int pDGaaLMFvVScaVGHIOut70579652 = -806782353;    int pDGaaLMFvVScaVGHIOut2621383 = -218634345;    int pDGaaLMFvVScaVGHIOut51420306 = -527550455;    int pDGaaLMFvVScaVGHIOut56843971 = -471149426;    int pDGaaLMFvVScaVGHIOut52772780 = -863514021;    int pDGaaLMFvVScaVGHIOut61875236 = -751335726;    int pDGaaLMFvVScaVGHIOut92930428 = -605070944;    int pDGaaLMFvVScaVGHIOut66693381 = -221988708;    int pDGaaLMFvVScaVGHIOut9789674 = -404789059;    int pDGaaLMFvVScaVGHIOut49415139 = -817735309;    int pDGaaLMFvVScaVGHIOut32938786 = -147900642;    int pDGaaLMFvVScaVGHIOut88853371 = -890329336;    int pDGaaLMFvVScaVGHIOut48626566 = -807750961;    int pDGaaLMFvVScaVGHIOut69503740 = -76145777;    int pDGaaLMFvVScaVGHIOut14665577 = -608532854;    int pDGaaLMFvVScaVGHIOut86721734 = -66310465;    int pDGaaLMFvVScaVGHIOut63378568 = -790291726;    int pDGaaLMFvVScaVGHIOut63463098 = 3985096;    int pDGaaLMFvVScaVGHIOut61830074 = -535861688;    int pDGaaLMFvVScaVGHIOut25885377 = -797505292;    int pDGaaLMFvVScaVGHIOut26779217 = -560121184;    int pDGaaLMFvVScaVGHIOut37101564 = -39112809;    int pDGaaLMFvVScaVGHIOut53053968 = -920395962;    int pDGaaLMFvVScaVGHIOut29475789 = 84955733;    int pDGaaLMFvVScaVGHIOut96650069 = -856084484;    int pDGaaLMFvVScaVGHIOut48864097 = -26547268;    int pDGaaLMFvVScaVGHIOut68636661 = 65178194;    int pDGaaLMFvVScaVGHIOut5517031 = -669427696;    int pDGaaLMFvVScaVGHIOut82233142 = -977553807;    int pDGaaLMFvVScaVGHIOut70105324 = 36317241;    int pDGaaLMFvVScaVGHIOut5856196 = 80510214;    int pDGaaLMFvVScaVGHIOut24497067 = -839270122;    int pDGaaLMFvVScaVGHIOut85975697 = -233303485;    int pDGaaLMFvVScaVGHIOut39116471 = -33093448;    int pDGaaLMFvVScaVGHIOut52174745 = -354938949;    int pDGaaLMFvVScaVGHIOut25574882 = 70162716;    int pDGaaLMFvVScaVGHIOut10085275 = -280514765;    int pDGaaLMFvVScaVGHIOut13369045 = -777011469;    int pDGaaLMFvVScaVGHIOut18659877 = -111408451;    int pDGaaLMFvVScaVGHIOut76514043 = -895015359;     pDGaaLMFvVScaVGHIOut69585518 = pDGaaLMFvVScaVGHIOut79064716;     pDGaaLMFvVScaVGHIOut79064716 = pDGaaLMFvVScaVGHIOut18093758;     pDGaaLMFvVScaVGHIOut18093758 = pDGaaLMFvVScaVGHIOut24696645;     pDGaaLMFvVScaVGHIOut24696645 = pDGaaLMFvVScaVGHIOut63600549;     pDGaaLMFvVScaVGHIOut63600549 = pDGaaLMFvVScaVGHIOut98557941;     pDGaaLMFvVScaVGHIOut98557941 = pDGaaLMFvVScaVGHIOut18547103;     pDGaaLMFvVScaVGHIOut18547103 = pDGaaLMFvVScaVGHIOut38843337;     pDGaaLMFvVScaVGHIOut38843337 = pDGaaLMFvVScaVGHIOut62790327;     pDGaaLMFvVScaVGHIOut62790327 = pDGaaLMFvVScaVGHIOut30806779;     pDGaaLMFvVScaVGHIOut30806779 = pDGaaLMFvVScaVGHIOut60294592;     pDGaaLMFvVScaVGHIOut60294592 = pDGaaLMFvVScaVGHIOut50281057;     pDGaaLMFvVScaVGHIOut50281057 = pDGaaLMFvVScaVGHIOut24692533;     pDGaaLMFvVScaVGHIOut24692533 = pDGaaLMFvVScaVGHIOut15888932;     pDGaaLMFvVScaVGHIOut15888932 = pDGaaLMFvVScaVGHIOut99440296;     pDGaaLMFvVScaVGHIOut99440296 = pDGaaLMFvVScaVGHIOut75310257;     pDGaaLMFvVScaVGHIOut75310257 = pDGaaLMFvVScaVGHIOut71165849;     pDGaaLMFvVScaVGHIOut71165849 = pDGaaLMFvVScaVGHIOut51365361;     pDGaaLMFvVScaVGHIOut51365361 = pDGaaLMFvVScaVGHIOut92258137;     pDGaaLMFvVScaVGHIOut92258137 = pDGaaLMFvVScaVGHIOut42861841;     pDGaaLMFvVScaVGHIOut42861841 = pDGaaLMFvVScaVGHIOut48227315;     pDGaaLMFvVScaVGHIOut48227315 = pDGaaLMFvVScaVGHIOut33591584;     pDGaaLMFvVScaVGHIOut33591584 = pDGaaLMFvVScaVGHIOut34895756;     pDGaaLMFvVScaVGHIOut34895756 = pDGaaLMFvVScaVGHIOut219354;     pDGaaLMFvVScaVGHIOut219354 = pDGaaLMFvVScaVGHIOut28359864;     pDGaaLMFvVScaVGHIOut28359864 = pDGaaLMFvVScaVGHIOut99978225;     pDGaaLMFvVScaVGHIOut99978225 = pDGaaLMFvVScaVGHIOut27978290;     pDGaaLMFvVScaVGHIOut27978290 = pDGaaLMFvVScaVGHIOut15925721;     pDGaaLMFvVScaVGHIOut15925721 = pDGaaLMFvVScaVGHIOut87423030;     pDGaaLMFvVScaVGHIOut87423030 = pDGaaLMFvVScaVGHIOut5946356;     pDGaaLMFvVScaVGHIOut5946356 = pDGaaLMFvVScaVGHIOut78033998;     pDGaaLMFvVScaVGHIOut78033998 = pDGaaLMFvVScaVGHIOut98419355;     pDGaaLMFvVScaVGHIOut98419355 = pDGaaLMFvVScaVGHIOut57350628;     pDGaaLMFvVScaVGHIOut57350628 = pDGaaLMFvVScaVGHIOut57999152;     pDGaaLMFvVScaVGHIOut57999152 = pDGaaLMFvVScaVGHIOut6099259;     pDGaaLMFvVScaVGHIOut6099259 = pDGaaLMFvVScaVGHIOut50025157;     pDGaaLMFvVScaVGHIOut50025157 = pDGaaLMFvVScaVGHIOut42371472;     pDGaaLMFvVScaVGHIOut42371472 = pDGaaLMFvVScaVGHIOut82312478;     pDGaaLMFvVScaVGHIOut82312478 = pDGaaLMFvVScaVGHIOut2738796;     pDGaaLMFvVScaVGHIOut2738796 = pDGaaLMFvVScaVGHIOut22754397;     pDGaaLMFvVScaVGHIOut22754397 = pDGaaLMFvVScaVGHIOut28196264;     pDGaaLMFvVScaVGHIOut28196264 = pDGaaLMFvVScaVGHIOut61505581;     pDGaaLMFvVScaVGHIOut61505581 = pDGaaLMFvVScaVGHIOut70213016;     pDGaaLMFvVScaVGHIOut70213016 = pDGaaLMFvVScaVGHIOut71432657;     pDGaaLMFvVScaVGHIOut71432657 = pDGaaLMFvVScaVGHIOut38389279;     pDGaaLMFvVScaVGHIOut38389279 = pDGaaLMFvVScaVGHIOut2474487;     pDGaaLMFvVScaVGHIOut2474487 = pDGaaLMFvVScaVGHIOut73199008;     pDGaaLMFvVScaVGHIOut73199008 = pDGaaLMFvVScaVGHIOut90876726;     pDGaaLMFvVScaVGHIOut90876726 = pDGaaLMFvVScaVGHIOut62871753;     pDGaaLMFvVScaVGHIOut62871753 = pDGaaLMFvVScaVGHIOut57947242;     pDGaaLMFvVScaVGHIOut57947242 = pDGaaLMFvVScaVGHIOut9296287;     pDGaaLMFvVScaVGHIOut9296287 = pDGaaLMFvVScaVGHIOut29169902;     pDGaaLMFvVScaVGHIOut29169902 = pDGaaLMFvVScaVGHIOut29782695;     pDGaaLMFvVScaVGHIOut29782695 = pDGaaLMFvVScaVGHIOut51833598;     pDGaaLMFvVScaVGHIOut51833598 = pDGaaLMFvVScaVGHIOut75766010;     pDGaaLMFvVScaVGHIOut75766010 = pDGaaLMFvVScaVGHIOut35993935;     pDGaaLMFvVScaVGHIOut35993935 = pDGaaLMFvVScaVGHIOut44168961;     pDGaaLMFvVScaVGHIOut44168961 = pDGaaLMFvVScaVGHIOut17874405;     pDGaaLMFvVScaVGHIOut17874405 = pDGaaLMFvVScaVGHIOut96336780;     pDGaaLMFvVScaVGHIOut96336780 = pDGaaLMFvVScaVGHIOut63622324;     pDGaaLMFvVScaVGHIOut63622324 = pDGaaLMFvVScaVGHIOut70579652;     pDGaaLMFvVScaVGHIOut70579652 = pDGaaLMFvVScaVGHIOut2621383;     pDGaaLMFvVScaVGHIOut2621383 = pDGaaLMFvVScaVGHIOut51420306;     pDGaaLMFvVScaVGHIOut51420306 = pDGaaLMFvVScaVGHIOut56843971;     pDGaaLMFvVScaVGHIOut56843971 = pDGaaLMFvVScaVGHIOut52772780;     pDGaaLMFvVScaVGHIOut52772780 = pDGaaLMFvVScaVGHIOut61875236;     pDGaaLMFvVScaVGHIOut61875236 = pDGaaLMFvVScaVGHIOut92930428;     pDGaaLMFvVScaVGHIOut92930428 = pDGaaLMFvVScaVGHIOut66693381;     pDGaaLMFvVScaVGHIOut66693381 = pDGaaLMFvVScaVGHIOut9789674;     pDGaaLMFvVScaVGHIOut9789674 = pDGaaLMFvVScaVGHIOut49415139;     pDGaaLMFvVScaVGHIOut49415139 = pDGaaLMFvVScaVGHIOut32938786;     pDGaaLMFvVScaVGHIOut32938786 = pDGaaLMFvVScaVGHIOut88853371;     pDGaaLMFvVScaVGHIOut88853371 = pDGaaLMFvVScaVGHIOut48626566;     pDGaaLMFvVScaVGHIOut48626566 = pDGaaLMFvVScaVGHIOut69503740;     pDGaaLMFvVScaVGHIOut69503740 = pDGaaLMFvVScaVGHIOut14665577;     pDGaaLMFvVScaVGHIOut14665577 = pDGaaLMFvVScaVGHIOut86721734;     pDGaaLMFvVScaVGHIOut86721734 = pDGaaLMFvVScaVGHIOut63378568;     pDGaaLMFvVScaVGHIOut63378568 = pDGaaLMFvVScaVGHIOut63463098;     pDGaaLMFvVScaVGHIOut63463098 = pDGaaLMFvVScaVGHIOut61830074;     pDGaaLMFvVScaVGHIOut61830074 = pDGaaLMFvVScaVGHIOut25885377;     pDGaaLMFvVScaVGHIOut25885377 = pDGaaLMFvVScaVGHIOut26779217;     pDGaaLMFvVScaVGHIOut26779217 = pDGaaLMFvVScaVGHIOut37101564;     pDGaaLMFvVScaVGHIOut37101564 = pDGaaLMFvVScaVGHIOut53053968;     pDGaaLMFvVScaVGHIOut53053968 = pDGaaLMFvVScaVGHIOut29475789;     pDGaaLMFvVScaVGHIOut29475789 = pDGaaLMFvVScaVGHIOut96650069;     pDGaaLMFvVScaVGHIOut96650069 = pDGaaLMFvVScaVGHIOut48864097;     pDGaaLMFvVScaVGHIOut48864097 = pDGaaLMFvVScaVGHIOut68636661;     pDGaaLMFvVScaVGHIOut68636661 = pDGaaLMFvVScaVGHIOut5517031;     pDGaaLMFvVScaVGHIOut5517031 = pDGaaLMFvVScaVGHIOut82233142;     pDGaaLMFvVScaVGHIOut82233142 = pDGaaLMFvVScaVGHIOut70105324;     pDGaaLMFvVScaVGHIOut70105324 = pDGaaLMFvVScaVGHIOut5856196;     pDGaaLMFvVScaVGHIOut5856196 = pDGaaLMFvVScaVGHIOut24497067;     pDGaaLMFvVScaVGHIOut24497067 = pDGaaLMFvVScaVGHIOut85975697;     pDGaaLMFvVScaVGHIOut85975697 = pDGaaLMFvVScaVGHIOut39116471;     pDGaaLMFvVScaVGHIOut39116471 = pDGaaLMFvVScaVGHIOut52174745;     pDGaaLMFvVScaVGHIOut52174745 = pDGaaLMFvVScaVGHIOut25574882;     pDGaaLMFvVScaVGHIOut25574882 = pDGaaLMFvVScaVGHIOut10085275;     pDGaaLMFvVScaVGHIOut10085275 = pDGaaLMFvVScaVGHIOut13369045;     pDGaaLMFvVScaVGHIOut13369045 = pDGaaLMFvVScaVGHIOut18659877;     pDGaaLMFvVScaVGHIOut18659877 = pDGaaLMFvVScaVGHIOut76514043;     pDGaaLMFvVScaVGHIOut76514043 = pDGaaLMFvVScaVGHIOut69585518;}
// Junk Finished

// Junk Code By Peatreat & Thaisen's Gen
void zQNijXomzNiQktdmHgSA335643() {     int MAWLPZguMAXaNlMbvKPl1846328 = -470663121;    int MAWLPZguMAXaNlMbvKPl80287152 = -502054986;    int MAWLPZguMAXaNlMbvKPl51370338 = -210901803;    int MAWLPZguMAXaNlMbvKPl66204758 = -672212958;    int MAWLPZguMAXaNlMbvKPl48718068 = -274640544;    int MAWLPZguMAXaNlMbvKPl32654654 = -743103206;    int MAWLPZguMAXaNlMbvKPl16039404 = -298465139;    int MAWLPZguMAXaNlMbvKPl61598632 = -71080410;    int MAWLPZguMAXaNlMbvKPl36141771 = -467455175;    int MAWLPZguMAXaNlMbvKPl18314693 = -193379773;    int MAWLPZguMAXaNlMbvKPl21684633 = -789435210;    int MAWLPZguMAXaNlMbvKPl71434498 = -883392668;    int MAWLPZguMAXaNlMbvKPl53354581 = -689993820;    int MAWLPZguMAXaNlMbvKPl20084831 = -754582429;    int MAWLPZguMAXaNlMbvKPl38101202 = -837082591;    int MAWLPZguMAXaNlMbvKPl95386428 = -213028697;    int MAWLPZguMAXaNlMbvKPl77027020 = -470693020;    int MAWLPZguMAXaNlMbvKPl35986065 = -767503703;    int MAWLPZguMAXaNlMbvKPl72625377 = -6558767;    int MAWLPZguMAXaNlMbvKPl54053876 = -561760626;    int MAWLPZguMAXaNlMbvKPl61188020 = -261437899;    int MAWLPZguMAXaNlMbvKPl99325220 = -210122297;    int MAWLPZguMAXaNlMbvKPl4950661 = -940558046;    int MAWLPZguMAXaNlMbvKPl18798043 = -679440640;    int MAWLPZguMAXaNlMbvKPl17014923 = -598899807;    int MAWLPZguMAXaNlMbvKPl69490255 = -375161579;    int MAWLPZguMAXaNlMbvKPl53100375 = -966403083;    int MAWLPZguMAXaNlMbvKPl98666795 = -623953356;    int MAWLPZguMAXaNlMbvKPl33106593 = -391439122;    int MAWLPZguMAXaNlMbvKPl67314455 = -65895824;    int MAWLPZguMAXaNlMbvKPl18861401 = 88351447;    int MAWLPZguMAXaNlMbvKPl56593633 = -601142176;    int MAWLPZguMAXaNlMbvKPl29420257 = -736282902;    int MAWLPZguMAXaNlMbvKPl93549194 = -394199346;    int MAWLPZguMAXaNlMbvKPl46732089 = -133666260;    int MAWLPZguMAXaNlMbvKPl38487020 = -789356478;    int MAWLPZguMAXaNlMbvKPl35577615 = -600972430;    int MAWLPZguMAXaNlMbvKPl11581296 = -565995334;    int MAWLPZguMAXaNlMbvKPl40513910 = -544443330;    int MAWLPZguMAXaNlMbvKPl98427969 = -393237558;    int MAWLPZguMAXaNlMbvKPl43253961 = 54211981;    int MAWLPZguMAXaNlMbvKPl95369508 = -851480697;    int MAWLPZguMAXaNlMbvKPl15946367 = -760265882;    int MAWLPZguMAXaNlMbvKPl40214836 = -108202932;    int MAWLPZguMAXaNlMbvKPl14408128 = -724243993;    int MAWLPZguMAXaNlMbvKPl90135775 = -421951525;    int MAWLPZguMAXaNlMbvKPl5073115 = -176947659;    int MAWLPZguMAXaNlMbvKPl45088942 = -559111513;    int MAWLPZguMAXaNlMbvKPl29413004 = -488770351;    int MAWLPZguMAXaNlMbvKPl25750582 = -479360706;    int MAWLPZguMAXaNlMbvKPl88799979 = -144612097;    int MAWLPZguMAXaNlMbvKPl59248051 = -496844126;    int MAWLPZguMAXaNlMbvKPl58644642 = -375131671;    int MAWLPZguMAXaNlMbvKPl59213080 = -989862399;    int MAWLPZguMAXaNlMbvKPl39147623 = -96073691;    int MAWLPZguMAXaNlMbvKPl2521108 = -160540825;    int MAWLPZguMAXaNlMbvKPl75336491 = -561496940;    int MAWLPZguMAXaNlMbvKPl32572295 = -531461163;    int MAWLPZguMAXaNlMbvKPl49189836 = 26686849;    int MAWLPZguMAXaNlMbvKPl79227813 = -899478966;    int MAWLPZguMAXaNlMbvKPl79554279 = -776700124;    int MAWLPZguMAXaNlMbvKPl17372608 = -674511783;    int MAWLPZguMAXaNlMbvKPl28492039 = -679641289;    int MAWLPZguMAXaNlMbvKPl68827315 = -301559352;    int MAWLPZguMAXaNlMbvKPl99453292 = -181731221;    int MAWLPZguMAXaNlMbvKPl65090999 = -88293034;    int MAWLPZguMAXaNlMbvKPl42014241 = -47109767;    int MAWLPZguMAXaNlMbvKPl59805386 = -195794475;    int MAWLPZguMAXaNlMbvKPl73352741 = -520916170;    int MAWLPZguMAXaNlMbvKPl99614181 = 52273887;    int MAWLPZguMAXaNlMbvKPl59808813 = -612056268;    int MAWLPZguMAXaNlMbvKPl65445725 = -904697686;    int MAWLPZguMAXaNlMbvKPl95472154 = -123060373;    int MAWLPZguMAXaNlMbvKPl74197407 = -613321210;    int MAWLPZguMAXaNlMbvKPl10799915 = -515972607;    int MAWLPZguMAXaNlMbvKPl65818511 = -409957203;    int MAWLPZguMAXaNlMbvKPl83378853 = -449856415;    int MAWLPZguMAXaNlMbvKPl64735825 = -732355115;    int MAWLPZguMAXaNlMbvKPl4389916 = -955196648;    int MAWLPZguMAXaNlMbvKPl26879147 = -76948283;    int MAWLPZguMAXaNlMbvKPl64417140 = -98213920;    int MAWLPZguMAXaNlMbvKPl8011434 = -307291570;    int MAWLPZguMAXaNlMbvKPl69253792 = -35183006;    int MAWLPZguMAXaNlMbvKPl7356012 = -912078417;    int MAWLPZguMAXaNlMbvKPl78514476 = -921283727;    int MAWLPZguMAXaNlMbvKPl59613349 = -414804427;    int MAWLPZguMAXaNlMbvKPl97948990 = -126010505;    int MAWLPZguMAXaNlMbvKPl70207177 = -746420504;    int MAWLPZguMAXaNlMbvKPl54401572 = -198125656;    int MAWLPZguMAXaNlMbvKPl44210982 = -973125436;    int MAWLPZguMAXaNlMbvKPl63150528 = -127859539;    int MAWLPZguMAXaNlMbvKPl3005321 = 30488732;    int MAWLPZguMAXaNlMbvKPl62391460 = -492682184;    int MAWLPZguMAXaNlMbvKPl61286096 = -644964364;    int MAWLPZguMAXaNlMbvKPl18873691 = -616537434;    int MAWLPZguMAXaNlMbvKPl25881353 = -271276237;    int MAWLPZguMAXaNlMbvKPl66877470 = -71839408;    int MAWLPZguMAXaNlMbvKPl47119051 = -358706531;    int MAWLPZguMAXaNlMbvKPl40761543 = -926471712;    int MAWLPZguMAXaNlMbvKPl49317128 = -470663121;     MAWLPZguMAXaNlMbvKPl1846328 = MAWLPZguMAXaNlMbvKPl80287152;     MAWLPZguMAXaNlMbvKPl80287152 = MAWLPZguMAXaNlMbvKPl51370338;     MAWLPZguMAXaNlMbvKPl51370338 = MAWLPZguMAXaNlMbvKPl66204758;     MAWLPZguMAXaNlMbvKPl66204758 = MAWLPZguMAXaNlMbvKPl48718068;     MAWLPZguMAXaNlMbvKPl48718068 = MAWLPZguMAXaNlMbvKPl32654654;     MAWLPZguMAXaNlMbvKPl32654654 = MAWLPZguMAXaNlMbvKPl16039404;     MAWLPZguMAXaNlMbvKPl16039404 = MAWLPZguMAXaNlMbvKPl61598632;     MAWLPZguMAXaNlMbvKPl61598632 = MAWLPZguMAXaNlMbvKPl36141771;     MAWLPZguMAXaNlMbvKPl36141771 = MAWLPZguMAXaNlMbvKPl18314693;     MAWLPZguMAXaNlMbvKPl18314693 = MAWLPZguMAXaNlMbvKPl21684633;     MAWLPZguMAXaNlMbvKPl21684633 = MAWLPZguMAXaNlMbvKPl71434498;     MAWLPZguMAXaNlMbvKPl71434498 = MAWLPZguMAXaNlMbvKPl53354581;     MAWLPZguMAXaNlMbvKPl53354581 = MAWLPZguMAXaNlMbvKPl20084831;     MAWLPZguMAXaNlMbvKPl20084831 = MAWLPZguMAXaNlMbvKPl38101202;     MAWLPZguMAXaNlMbvKPl38101202 = MAWLPZguMAXaNlMbvKPl95386428;     MAWLPZguMAXaNlMbvKPl95386428 = MAWLPZguMAXaNlMbvKPl77027020;     MAWLPZguMAXaNlMbvKPl77027020 = MAWLPZguMAXaNlMbvKPl35986065;     MAWLPZguMAXaNlMbvKPl35986065 = MAWLPZguMAXaNlMbvKPl72625377;     MAWLPZguMAXaNlMbvKPl72625377 = MAWLPZguMAXaNlMbvKPl54053876;     MAWLPZguMAXaNlMbvKPl54053876 = MAWLPZguMAXaNlMbvKPl61188020;     MAWLPZguMAXaNlMbvKPl61188020 = MAWLPZguMAXaNlMbvKPl99325220;     MAWLPZguMAXaNlMbvKPl99325220 = MAWLPZguMAXaNlMbvKPl4950661;     MAWLPZguMAXaNlMbvKPl4950661 = MAWLPZguMAXaNlMbvKPl18798043;     MAWLPZguMAXaNlMbvKPl18798043 = MAWLPZguMAXaNlMbvKPl17014923;     MAWLPZguMAXaNlMbvKPl17014923 = MAWLPZguMAXaNlMbvKPl69490255;     MAWLPZguMAXaNlMbvKPl69490255 = MAWLPZguMAXaNlMbvKPl53100375;     MAWLPZguMAXaNlMbvKPl53100375 = MAWLPZguMAXaNlMbvKPl98666795;     MAWLPZguMAXaNlMbvKPl98666795 = MAWLPZguMAXaNlMbvKPl33106593;     MAWLPZguMAXaNlMbvKPl33106593 = MAWLPZguMAXaNlMbvKPl67314455;     MAWLPZguMAXaNlMbvKPl67314455 = MAWLPZguMAXaNlMbvKPl18861401;     MAWLPZguMAXaNlMbvKPl18861401 = MAWLPZguMAXaNlMbvKPl56593633;     MAWLPZguMAXaNlMbvKPl56593633 = MAWLPZguMAXaNlMbvKPl29420257;     MAWLPZguMAXaNlMbvKPl29420257 = MAWLPZguMAXaNlMbvKPl93549194;     MAWLPZguMAXaNlMbvKPl93549194 = MAWLPZguMAXaNlMbvKPl46732089;     MAWLPZguMAXaNlMbvKPl46732089 = MAWLPZguMAXaNlMbvKPl38487020;     MAWLPZguMAXaNlMbvKPl38487020 = MAWLPZguMAXaNlMbvKPl35577615;     MAWLPZguMAXaNlMbvKPl35577615 = MAWLPZguMAXaNlMbvKPl11581296;     MAWLPZguMAXaNlMbvKPl11581296 = MAWLPZguMAXaNlMbvKPl40513910;     MAWLPZguMAXaNlMbvKPl40513910 = MAWLPZguMAXaNlMbvKPl98427969;     MAWLPZguMAXaNlMbvKPl98427969 = MAWLPZguMAXaNlMbvKPl43253961;     MAWLPZguMAXaNlMbvKPl43253961 = MAWLPZguMAXaNlMbvKPl95369508;     MAWLPZguMAXaNlMbvKPl95369508 = MAWLPZguMAXaNlMbvKPl15946367;     MAWLPZguMAXaNlMbvKPl15946367 = MAWLPZguMAXaNlMbvKPl40214836;     MAWLPZguMAXaNlMbvKPl40214836 = MAWLPZguMAXaNlMbvKPl14408128;     MAWLPZguMAXaNlMbvKPl14408128 = MAWLPZguMAXaNlMbvKPl90135775;     MAWLPZguMAXaNlMbvKPl90135775 = MAWLPZguMAXaNlMbvKPl5073115;     MAWLPZguMAXaNlMbvKPl5073115 = MAWLPZguMAXaNlMbvKPl45088942;     MAWLPZguMAXaNlMbvKPl45088942 = MAWLPZguMAXaNlMbvKPl29413004;     MAWLPZguMAXaNlMbvKPl29413004 = MAWLPZguMAXaNlMbvKPl25750582;     MAWLPZguMAXaNlMbvKPl25750582 = MAWLPZguMAXaNlMbvKPl88799979;     MAWLPZguMAXaNlMbvKPl88799979 = MAWLPZguMAXaNlMbvKPl59248051;     MAWLPZguMAXaNlMbvKPl59248051 = MAWLPZguMAXaNlMbvKPl58644642;     MAWLPZguMAXaNlMbvKPl58644642 = MAWLPZguMAXaNlMbvKPl59213080;     MAWLPZguMAXaNlMbvKPl59213080 = MAWLPZguMAXaNlMbvKPl39147623;     MAWLPZguMAXaNlMbvKPl39147623 = MAWLPZguMAXaNlMbvKPl2521108;     MAWLPZguMAXaNlMbvKPl2521108 = MAWLPZguMAXaNlMbvKPl75336491;     MAWLPZguMAXaNlMbvKPl75336491 = MAWLPZguMAXaNlMbvKPl32572295;     MAWLPZguMAXaNlMbvKPl32572295 = MAWLPZguMAXaNlMbvKPl49189836;     MAWLPZguMAXaNlMbvKPl49189836 = MAWLPZguMAXaNlMbvKPl79227813;     MAWLPZguMAXaNlMbvKPl79227813 = MAWLPZguMAXaNlMbvKPl79554279;     MAWLPZguMAXaNlMbvKPl79554279 = MAWLPZguMAXaNlMbvKPl17372608;     MAWLPZguMAXaNlMbvKPl17372608 = MAWLPZguMAXaNlMbvKPl28492039;     MAWLPZguMAXaNlMbvKPl28492039 = MAWLPZguMAXaNlMbvKPl68827315;     MAWLPZguMAXaNlMbvKPl68827315 = MAWLPZguMAXaNlMbvKPl99453292;     MAWLPZguMAXaNlMbvKPl99453292 = MAWLPZguMAXaNlMbvKPl65090999;     MAWLPZguMAXaNlMbvKPl65090999 = MAWLPZguMAXaNlMbvKPl42014241;     MAWLPZguMAXaNlMbvKPl42014241 = MAWLPZguMAXaNlMbvKPl59805386;     MAWLPZguMAXaNlMbvKPl59805386 = MAWLPZguMAXaNlMbvKPl73352741;     MAWLPZguMAXaNlMbvKPl73352741 = MAWLPZguMAXaNlMbvKPl99614181;     MAWLPZguMAXaNlMbvKPl99614181 = MAWLPZguMAXaNlMbvKPl59808813;     MAWLPZguMAXaNlMbvKPl59808813 = MAWLPZguMAXaNlMbvKPl65445725;     MAWLPZguMAXaNlMbvKPl65445725 = MAWLPZguMAXaNlMbvKPl95472154;     MAWLPZguMAXaNlMbvKPl95472154 = MAWLPZguMAXaNlMbvKPl74197407;     MAWLPZguMAXaNlMbvKPl74197407 = MAWLPZguMAXaNlMbvKPl10799915;     MAWLPZguMAXaNlMbvKPl10799915 = MAWLPZguMAXaNlMbvKPl65818511;     MAWLPZguMAXaNlMbvKPl65818511 = MAWLPZguMAXaNlMbvKPl83378853;     MAWLPZguMAXaNlMbvKPl83378853 = MAWLPZguMAXaNlMbvKPl64735825;     MAWLPZguMAXaNlMbvKPl64735825 = MAWLPZguMAXaNlMbvKPl4389916;     MAWLPZguMAXaNlMbvKPl4389916 = MAWLPZguMAXaNlMbvKPl26879147;     MAWLPZguMAXaNlMbvKPl26879147 = MAWLPZguMAXaNlMbvKPl64417140;     MAWLPZguMAXaNlMbvKPl64417140 = MAWLPZguMAXaNlMbvKPl8011434;     MAWLPZguMAXaNlMbvKPl8011434 = MAWLPZguMAXaNlMbvKPl69253792;     MAWLPZguMAXaNlMbvKPl69253792 = MAWLPZguMAXaNlMbvKPl7356012;     MAWLPZguMAXaNlMbvKPl7356012 = MAWLPZguMAXaNlMbvKPl78514476;     MAWLPZguMAXaNlMbvKPl78514476 = MAWLPZguMAXaNlMbvKPl59613349;     MAWLPZguMAXaNlMbvKPl59613349 = MAWLPZguMAXaNlMbvKPl97948990;     MAWLPZguMAXaNlMbvKPl97948990 = MAWLPZguMAXaNlMbvKPl70207177;     MAWLPZguMAXaNlMbvKPl70207177 = MAWLPZguMAXaNlMbvKPl54401572;     MAWLPZguMAXaNlMbvKPl54401572 = MAWLPZguMAXaNlMbvKPl44210982;     MAWLPZguMAXaNlMbvKPl44210982 = MAWLPZguMAXaNlMbvKPl63150528;     MAWLPZguMAXaNlMbvKPl63150528 = MAWLPZguMAXaNlMbvKPl3005321;     MAWLPZguMAXaNlMbvKPl3005321 = MAWLPZguMAXaNlMbvKPl62391460;     MAWLPZguMAXaNlMbvKPl62391460 = MAWLPZguMAXaNlMbvKPl61286096;     MAWLPZguMAXaNlMbvKPl61286096 = MAWLPZguMAXaNlMbvKPl18873691;     MAWLPZguMAXaNlMbvKPl18873691 = MAWLPZguMAXaNlMbvKPl25881353;     MAWLPZguMAXaNlMbvKPl25881353 = MAWLPZguMAXaNlMbvKPl66877470;     MAWLPZguMAXaNlMbvKPl66877470 = MAWLPZguMAXaNlMbvKPl47119051;     MAWLPZguMAXaNlMbvKPl47119051 = MAWLPZguMAXaNlMbvKPl40761543;     MAWLPZguMAXaNlMbvKPl40761543 = MAWLPZguMAXaNlMbvKPl49317128;     MAWLPZguMAXaNlMbvKPl49317128 = MAWLPZguMAXaNlMbvKPl1846328;}
// Junk Finished

// Junk Code By Peatreat & Thaisen's Gen
void mXEzZZlZyIlvLXQBZibN24435428() {     int vAFJGTdlyWIcZLvmVRgv4765275 = -791935872;    int vAFJGTdlyWIcZLvmVRgv11610789 = 95279956;    int vAFJGTdlyWIcZLvmVRgv63302625 = -798927032;    int vAFJGTdlyWIcZLvmVRgv62564298 = -862046226;    int vAFJGTdlyWIcZLvmVRgv46445806 = -239425130;    int vAFJGTdlyWIcZLvmVRgv21945697 = -713226816;    int vAFJGTdlyWIcZLvmVRgv63363264 = -777146215;    int vAFJGTdlyWIcZLvmVRgv71982257 = -447869089;    int vAFJGTdlyWIcZLvmVRgv72618104 = -723459835;    int vAFJGTdlyWIcZLvmVRgv32149555 = 69464940;    int vAFJGTdlyWIcZLvmVRgv5003229 = -353069311;    int vAFJGTdlyWIcZLvmVRgv23842834 = -34835257;    int vAFJGTdlyWIcZLvmVRgv29991103 = -884353864;    int vAFJGTdlyWIcZLvmVRgv54283542 = -740005460;    int vAFJGTdlyWIcZLvmVRgv14310017 = -166476512;    int vAFJGTdlyWIcZLvmVRgv73426691 = -131504306;    int vAFJGTdlyWIcZLvmVRgv20888259 = -576198159;    int vAFJGTdlyWIcZLvmVRgv17899730 = -368491675;    int vAFJGTdlyWIcZLvmVRgv14272129 = -864456131;    int vAFJGTdlyWIcZLvmVRgv51958857 = -96858624;    int vAFJGTdlyWIcZLvmVRgv49615055 = -746712882;    int vAFJGTdlyWIcZLvmVRgv48158356 = -203085397;    int vAFJGTdlyWIcZLvmVRgv40070978 = -819064020;    int vAFJGTdlyWIcZLvmVRgv27576162 = -506889506;    int vAFJGTdlyWIcZLvmVRgv41351680 = 73841177;    int vAFJGTdlyWIcZLvmVRgv53025788 = -33260584;    int vAFJGTdlyWIcZLvmVRgv16261665 = 41161356;    int vAFJGTdlyWIcZLvmVRgv53786571 = -248674531;    int vAFJGTdlyWIcZLvmVRgv98254197 = -721785095;    int vAFJGTdlyWIcZLvmVRgv91764780 = -994752996;    int vAFJGTdlyWIcZLvmVRgv33348121 = -127638612;    int vAFJGTdlyWIcZLvmVRgv74555513 = -688864513;    int vAFJGTdlyWIcZLvmVRgv98982824 = -710418630;    int vAFJGTdlyWIcZLvmVRgv94181375 = -413722990;    int vAFJGTdlyWIcZLvmVRgv83031117 = -134719876;    int vAFJGTdlyWIcZLvmVRgv44113951 = -801183246;    int vAFJGTdlyWIcZLvmVRgv60633877 = -907044307;    int vAFJGTdlyWIcZLvmVRgv23724922 = -356411991;    int vAFJGTdlyWIcZLvmVRgv21508716 = -481074537;    int vAFJGTdlyWIcZLvmVRgv97331948 = -532930490;    int vAFJGTdlyWIcZLvmVRgv30529275 = 60820610;    int vAFJGTdlyWIcZLvmVRgv72018767 = -370493162;    int vAFJGTdlyWIcZLvmVRgv50363318 = -699994668;    int vAFJGTdlyWIcZLvmVRgv56273416 = -756493920;    int vAFJGTdlyWIcZLvmVRgv2362984 = -256264508;    int vAFJGTdlyWIcZLvmVRgv43774930 = -66849199;    int vAFJGTdlyWIcZLvmVRgv35380633 = -841664589;    int vAFJGTdlyWIcZLvmVRgv89531786 = -132359190;    int vAFJGTdlyWIcZLvmVRgv82737318 = -424547811;    int vAFJGTdlyWIcZLvmVRgv81935985 = -860368620;    int vAFJGTdlyWIcZLvmVRgv23233627 = -93323443;    int vAFJGTdlyWIcZLvmVRgv16650449 = -550310549;    int vAFJGTdlyWIcZLvmVRgv67133899 = -370933940;    int vAFJGTdlyWIcZLvmVRgv46569597 = -170949571;    int vAFJGTdlyWIcZLvmVRgv74820373 = -462219171;    int vAFJGTdlyWIcZLvmVRgv56606919 = -488850476;    int vAFJGTdlyWIcZLvmVRgv71539811 = -85656025;    int vAFJGTdlyWIcZLvmVRgv35726463 = -192037527;    int vAFJGTdlyWIcZLvmVRgv21212618 = -835887404;    int vAFJGTdlyWIcZLvmVRgv93420018 = -106164546;    int vAFJGTdlyWIcZLvmVRgv5684033 = -654388173;    int vAFJGTdlyWIcZLvmVRgv9576694 = -428471685;    int vAFJGTdlyWIcZLvmVRgv73728060 = -726083994;    int vAFJGTdlyWIcZLvmVRgv80853324 = -728706840;    int vAFJGTdlyWIcZLvmVRgv98801433 = -802896448;    int vAFJGTdlyWIcZLvmVRgv30447716 = -664204798;    int vAFJGTdlyWIcZLvmVRgv24860009 = -324416627;    int vAFJGTdlyWIcZLvmVRgv35809728 = -370630874;    int vAFJGTdlyWIcZLvmVRgv71252425 = -505285584;    int vAFJGTdlyWIcZLvmVRgv70196066 = -365293266;    int vAFJGTdlyWIcZLvmVRgv12792815 = -224460000;    int vAFJGTdlyWIcZLvmVRgv97163337 = -119786168;    int vAFJGTdlyWIcZLvmVRgv96391013 = -887417139;    int vAFJGTdlyWIcZLvmVRgv16940180 = -231525642;    int vAFJGTdlyWIcZLvmVRgv21429582 = -57679234;    int vAFJGTdlyWIcZLvmVRgv77596287 = -276219721;    int vAFJGTdlyWIcZLvmVRgv97795037 = -503090729;    int vAFJGTdlyWIcZLvmVRgv83797561 = 37429899;    int vAFJGTdlyWIcZLvmVRgv25213178 = -150624998;    int vAFJGTdlyWIcZLvmVRgv97576750 = -859309624;    int vAFJGTdlyWIcZLvmVRgv17645156 = -191595995;    int vAFJGTdlyWIcZLvmVRgv26729879 = -826479454;    int vAFJGTdlyWIcZLvmVRgv71049253 = -824126721;    int vAFJGTdlyWIcZLvmVRgv16318213 = -861416476;    int vAFJGTdlyWIcZLvmVRgv68531154 = -801429553;    int vAFJGTdlyWIcZLvmVRgv16697673 = -577328064;    int vAFJGTdlyWIcZLvmVRgv7421614 = -217930573;    int vAFJGTdlyWIcZLvmVRgv52413227 = -439469059;    int vAFJGTdlyWIcZLvmVRgv19361002 = -951503819;    int vAFJGTdlyWIcZLvmVRgv26424199 = -645869401;    int vAFJGTdlyWIcZLvmVRgv72574140 = -615527222;    int vAFJGTdlyWIcZLvmVRgv24907414 = -615006781;    int vAFJGTdlyWIcZLvmVRgv2512305 = -520524587;    int vAFJGTdlyWIcZLvmVRgv28088698 = -274909991;    int vAFJGTdlyWIcZLvmVRgv91647916 = -878542318;    int vAFJGTdlyWIcZLvmVRgv20952582 = -510707706;    int vAFJGTdlyWIcZLvmVRgv98290707 = -644409169;    int vAFJGTdlyWIcZLvmVRgv69509993 = -971287829;    int vAFJGTdlyWIcZLvmVRgv57471982 = -953597472;    int vAFJGTdlyWIcZLvmVRgv71915268 = -791935872;     vAFJGTdlyWIcZLvmVRgv4765275 = vAFJGTdlyWIcZLvmVRgv11610789;     vAFJGTdlyWIcZLvmVRgv11610789 = vAFJGTdlyWIcZLvmVRgv63302625;     vAFJGTdlyWIcZLvmVRgv63302625 = vAFJGTdlyWIcZLvmVRgv62564298;     vAFJGTdlyWIcZLvmVRgv62564298 = vAFJGTdlyWIcZLvmVRgv46445806;     vAFJGTdlyWIcZLvmVRgv46445806 = vAFJGTdlyWIcZLvmVRgv21945697;     vAFJGTdlyWIcZLvmVRgv21945697 = vAFJGTdlyWIcZLvmVRgv63363264;     vAFJGTdlyWIcZLvmVRgv63363264 = vAFJGTdlyWIcZLvmVRgv71982257;     vAFJGTdlyWIcZLvmVRgv71982257 = vAFJGTdlyWIcZLvmVRgv72618104;     vAFJGTdlyWIcZLvmVRgv72618104 = vAFJGTdlyWIcZLvmVRgv32149555;     vAFJGTdlyWIcZLvmVRgv32149555 = vAFJGTdlyWIcZLvmVRgv5003229;     vAFJGTdlyWIcZLvmVRgv5003229 = vAFJGTdlyWIcZLvmVRgv23842834;     vAFJGTdlyWIcZLvmVRgv23842834 = vAFJGTdlyWIcZLvmVRgv29991103;     vAFJGTdlyWIcZLvmVRgv29991103 = vAFJGTdlyWIcZLvmVRgv54283542;     vAFJGTdlyWIcZLvmVRgv54283542 = vAFJGTdlyWIcZLvmVRgv14310017;     vAFJGTdlyWIcZLvmVRgv14310017 = vAFJGTdlyWIcZLvmVRgv73426691;     vAFJGTdlyWIcZLvmVRgv73426691 = vAFJGTdlyWIcZLvmVRgv20888259;     vAFJGTdlyWIcZLvmVRgv20888259 = vAFJGTdlyWIcZLvmVRgv17899730;     vAFJGTdlyWIcZLvmVRgv17899730 = vAFJGTdlyWIcZLvmVRgv14272129;     vAFJGTdlyWIcZLvmVRgv14272129 = vAFJGTdlyWIcZLvmVRgv51958857;     vAFJGTdlyWIcZLvmVRgv51958857 = vAFJGTdlyWIcZLvmVRgv49615055;     vAFJGTdlyWIcZLvmVRgv49615055 = vAFJGTdlyWIcZLvmVRgv48158356;     vAFJGTdlyWIcZLvmVRgv48158356 = vAFJGTdlyWIcZLvmVRgv40070978;     vAFJGTdlyWIcZLvmVRgv40070978 = vAFJGTdlyWIcZLvmVRgv27576162;     vAFJGTdlyWIcZLvmVRgv27576162 = vAFJGTdlyWIcZLvmVRgv41351680;     vAFJGTdlyWIcZLvmVRgv41351680 = vAFJGTdlyWIcZLvmVRgv53025788;     vAFJGTdlyWIcZLvmVRgv53025788 = vAFJGTdlyWIcZLvmVRgv16261665;     vAFJGTdlyWIcZLvmVRgv16261665 = vAFJGTdlyWIcZLvmVRgv53786571;     vAFJGTdlyWIcZLvmVRgv53786571 = vAFJGTdlyWIcZLvmVRgv98254197;     vAFJGTdlyWIcZLvmVRgv98254197 = vAFJGTdlyWIcZLvmVRgv91764780;     vAFJGTdlyWIcZLvmVRgv91764780 = vAFJGTdlyWIcZLvmVRgv33348121;     vAFJGTdlyWIcZLvmVRgv33348121 = vAFJGTdlyWIcZLvmVRgv74555513;     vAFJGTdlyWIcZLvmVRgv74555513 = vAFJGTdlyWIcZLvmVRgv98982824;     vAFJGTdlyWIcZLvmVRgv98982824 = vAFJGTdlyWIcZLvmVRgv94181375;     vAFJGTdlyWIcZLvmVRgv94181375 = vAFJGTdlyWIcZLvmVRgv83031117;     vAFJGTdlyWIcZLvmVRgv83031117 = vAFJGTdlyWIcZLvmVRgv44113951;     vAFJGTdlyWIcZLvmVRgv44113951 = vAFJGTdlyWIcZLvmVRgv60633877;     vAFJGTdlyWIcZLvmVRgv60633877 = vAFJGTdlyWIcZLvmVRgv23724922;     vAFJGTdlyWIcZLvmVRgv23724922 = vAFJGTdlyWIcZLvmVRgv21508716;     vAFJGTdlyWIcZLvmVRgv21508716 = vAFJGTdlyWIcZLvmVRgv97331948;     vAFJGTdlyWIcZLvmVRgv97331948 = vAFJGTdlyWIcZLvmVRgv30529275;     vAFJGTdlyWIcZLvmVRgv30529275 = vAFJGTdlyWIcZLvmVRgv72018767;     vAFJGTdlyWIcZLvmVRgv72018767 = vAFJGTdlyWIcZLvmVRgv50363318;     vAFJGTdlyWIcZLvmVRgv50363318 = vAFJGTdlyWIcZLvmVRgv56273416;     vAFJGTdlyWIcZLvmVRgv56273416 = vAFJGTdlyWIcZLvmVRgv2362984;     vAFJGTdlyWIcZLvmVRgv2362984 = vAFJGTdlyWIcZLvmVRgv43774930;     vAFJGTdlyWIcZLvmVRgv43774930 = vAFJGTdlyWIcZLvmVRgv35380633;     vAFJGTdlyWIcZLvmVRgv35380633 = vAFJGTdlyWIcZLvmVRgv89531786;     vAFJGTdlyWIcZLvmVRgv89531786 = vAFJGTdlyWIcZLvmVRgv82737318;     vAFJGTdlyWIcZLvmVRgv82737318 = vAFJGTdlyWIcZLvmVRgv81935985;     vAFJGTdlyWIcZLvmVRgv81935985 = vAFJGTdlyWIcZLvmVRgv23233627;     vAFJGTdlyWIcZLvmVRgv23233627 = vAFJGTdlyWIcZLvmVRgv16650449;     vAFJGTdlyWIcZLvmVRgv16650449 = vAFJGTdlyWIcZLvmVRgv67133899;     vAFJGTdlyWIcZLvmVRgv67133899 = vAFJGTdlyWIcZLvmVRgv46569597;     vAFJGTdlyWIcZLvmVRgv46569597 = vAFJGTdlyWIcZLvmVRgv74820373;     vAFJGTdlyWIcZLvmVRgv74820373 = vAFJGTdlyWIcZLvmVRgv56606919;     vAFJGTdlyWIcZLvmVRgv56606919 = vAFJGTdlyWIcZLvmVRgv71539811;     vAFJGTdlyWIcZLvmVRgv71539811 = vAFJGTdlyWIcZLvmVRgv35726463;     vAFJGTdlyWIcZLvmVRgv35726463 = vAFJGTdlyWIcZLvmVRgv21212618;     vAFJGTdlyWIcZLvmVRgv21212618 = vAFJGTdlyWIcZLvmVRgv93420018;     vAFJGTdlyWIcZLvmVRgv93420018 = vAFJGTdlyWIcZLvmVRgv5684033;     vAFJGTdlyWIcZLvmVRgv5684033 = vAFJGTdlyWIcZLvmVRgv9576694;     vAFJGTdlyWIcZLvmVRgv9576694 = vAFJGTdlyWIcZLvmVRgv73728060;     vAFJGTdlyWIcZLvmVRgv73728060 = vAFJGTdlyWIcZLvmVRgv80853324;     vAFJGTdlyWIcZLvmVRgv80853324 = vAFJGTdlyWIcZLvmVRgv98801433;     vAFJGTdlyWIcZLvmVRgv98801433 = vAFJGTdlyWIcZLvmVRgv30447716;     vAFJGTdlyWIcZLvmVRgv30447716 = vAFJGTdlyWIcZLvmVRgv24860009;     vAFJGTdlyWIcZLvmVRgv24860009 = vAFJGTdlyWIcZLvmVRgv35809728;     vAFJGTdlyWIcZLvmVRgv35809728 = vAFJGTdlyWIcZLvmVRgv71252425;     vAFJGTdlyWIcZLvmVRgv71252425 = vAFJGTdlyWIcZLvmVRgv70196066;     vAFJGTdlyWIcZLvmVRgv70196066 = vAFJGTdlyWIcZLvmVRgv12792815;     vAFJGTdlyWIcZLvmVRgv12792815 = vAFJGTdlyWIcZLvmVRgv97163337;     vAFJGTdlyWIcZLvmVRgv97163337 = vAFJGTdlyWIcZLvmVRgv96391013;     vAFJGTdlyWIcZLvmVRgv96391013 = vAFJGTdlyWIcZLvmVRgv16940180;     vAFJGTdlyWIcZLvmVRgv16940180 = vAFJGTdlyWIcZLvmVRgv21429582;     vAFJGTdlyWIcZLvmVRgv21429582 = vAFJGTdlyWIcZLvmVRgv77596287;     vAFJGTdlyWIcZLvmVRgv77596287 = vAFJGTdlyWIcZLvmVRgv97795037;     vAFJGTdlyWIcZLvmVRgv97795037 = vAFJGTdlyWIcZLvmVRgv83797561;     vAFJGTdlyWIcZLvmVRgv83797561 = vAFJGTdlyWIcZLvmVRgv25213178;     vAFJGTdlyWIcZLvmVRgv25213178 = vAFJGTdlyWIcZLvmVRgv97576750;     vAFJGTdlyWIcZLvmVRgv97576750 = vAFJGTdlyWIcZLvmVRgv17645156;     vAFJGTdlyWIcZLvmVRgv17645156 = vAFJGTdlyWIcZLvmVRgv26729879;     vAFJGTdlyWIcZLvmVRgv26729879 = vAFJGTdlyWIcZLvmVRgv71049253;     vAFJGTdlyWIcZLvmVRgv71049253 = vAFJGTdlyWIcZLvmVRgv16318213;     vAFJGTdlyWIcZLvmVRgv16318213 = vAFJGTdlyWIcZLvmVRgv68531154;     vAFJGTdlyWIcZLvmVRgv68531154 = vAFJGTdlyWIcZLvmVRgv16697673;     vAFJGTdlyWIcZLvmVRgv16697673 = vAFJGTdlyWIcZLvmVRgv7421614;     vAFJGTdlyWIcZLvmVRgv7421614 = vAFJGTdlyWIcZLvmVRgv52413227;     vAFJGTdlyWIcZLvmVRgv52413227 = vAFJGTdlyWIcZLvmVRgv19361002;     vAFJGTdlyWIcZLvmVRgv19361002 = vAFJGTdlyWIcZLvmVRgv26424199;     vAFJGTdlyWIcZLvmVRgv26424199 = vAFJGTdlyWIcZLvmVRgv72574140;     vAFJGTdlyWIcZLvmVRgv72574140 = vAFJGTdlyWIcZLvmVRgv24907414;     vAFJGTdlyWIcZLvmVRgv24907414 = vAFJGTdlyWIcZLvmVRgv2512305;     vAFJGTdlyWIcZLvmVRgv2512305 = vAFJGTdlyWIcZLvmVRgv28088698;     vAFJGTdlyWIcZLvmVRgv28088698 = vAFJGTdlyWIcZLvmVRgv91647916;     vAFJGTdlyWIcZLvmVRgv91647916 = vAFJGTdlyWIcZLvmVRgv20952582;     vAFJGTdlyWIcZLvmVRgv20952582 = vAFJGTdlyWIcZLvmVRgv98290707;     vAFJGTdlyWIcZLvmVRgv98290707 = vAFJGTdlyWIcZLvmVRgv69509993;     vAFJGTdlyWIcZLvmVRgv69509993 = vAFJGTdlyWIcZLvmVRgv57471982;     vAFJGTdlyWIcZLvmVRgv57471982 = vAFJGTdlyWIcZLvmVRgv71915268;     vAFJGTdlyWIcZLvmVRgv71915268 = vAFJGTdlyWIcZLvmVRgv4765275;}
// Junk Finished
