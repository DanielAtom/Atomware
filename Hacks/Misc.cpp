#include <sstream>

#include "../Config.h"
#include "../Interfaces.h"
#include "../Memory.h"
#include "../Netvars.h"
#include "Misc.h"
#include "../SDK/ConVar.h"
#include "../SDK/Surface.h"
#include "../SDK/GlobalVars.h"
#include "../SDK/NetworkChannel.h"
#include "../SDK/WeaponData.h"
#include "EnginePrediction.h"

void Misc::edgejump(UserCmd* cmd) noexcept
{
    if (!config->misc.edgejump || !GetAsyncKeyState(config->misc.edgejumpkey))
        return;

    if (!localPlayer || !localPlayer->isAlive())
        return;

    if (const auto mt = localPlayer->moveType(); mt == MoveType::LADDER || mt == MoveType::NOCLIP)
        return;

    if ((EnginePrediction::getFlags() & 1) && !(localPlayer->flags() & 1))
        cmd->buttons |= UserCmd::IN_JUMP;
}

void Misc::slowwalk(UserCmd* cmd) noexcept
{
    if (!config->misc.slowwalk || !GetAsyncKeyState(config->misc.slowwalkKey))
        return;

    if (!localPlayer || !localPlayer->isAlive())
        return;

    const auto activeWeapon = localPlayer->getActiveWeapon();
    if (!activeWeapon)
        return;

    const auto weaponData = activeWeapon->getWeaponData();
    if (!weaponData)
        return;

    const float maxSpeed = (localPlayer->isScoped() ? weaponData->maxSpeedAlt : weaponData->maxSpeed) / 3;

    if (cmd->forwardmove && cmd->sidemove) {
        const float maxSpeedRoot = maxSpeed * static_cast<float>(M_SQRT1_2);
        cmd->forwardmove = cmd->forwardmove < 0.0f ? -maxSpeedRoot : maxSpeedRoot;
        cmd->sidemove = cmd->sidemove < 0.0f ? -maxSpeedRoot : maxSpeedRoot;
    } else if (cmd->forwardmove) {
        cmd->forwardmove = cmd->forwardmove < 0.0f ? -maxSpeed : maxSpeed;
    } else if (cmd->sidemove) {
        cmd->sidemove = cmd->sidemove < 0.0f ? -maxSpeed : maxSpeed;
    }
}

void Misc::inverseRagdollGravity() noexcept
{
    static auto ragdollGravity = interfaces->cvar->findVar("cl_ragdoll_gravity");
    ragdollGravity->setValue(config->visuals.inverseRagdollGravity ? -600 : 600);
}

void Misc::updateClanTag(bool tagChanged) noexcept
{
    if (config->misc.customClanTag) {
        static std::string clanTag;
        if (config->misc.clantagtype == 0)
            clanTag = "atomware.cf";
        else if (config->misc.clantagtype == 1)
            clanTag = "[VALV\xE1\xB4\xB1]";
        else if (config->misc.clantagtype == 2)
            clanTag = "[\xCE\x91 & \xCE\xA9]";

        if (tagChanged) {
            if (config->misc.clantagtype == 0)
                clanTag = "atomware.cf";
            else if (config->misc.clantagtype == 1)
                clanTag = "[VALV\xE1\xB4\xB1]";
            else if (config->misc.clantagtype == 2)
                clanTag = "[\xCE\x91 & \xCE\xA9]";
            if (!isblank(clanTag.front()) && !isblank(clanTag.back()))
                clanTag.push_back(' ');
        }

        static auto lastTime{ 0.0f };
        if (memory->globalVars->realtime - lastTime < 0.6f) return;
        lastTime = memory->globalVars->realtime;

        if (config->misc.animatedClanTag && !clanTag.empty())
            std::rotate(std::begin(clanTag), std::next(std::begin(clanTag)), std::end(clanTag));

        memory->setClanTag(clanTag.c_str(), clanTag.c_str());

        if (config->misc.clocktag) {
            const auto time{ std::time(nullptr) };
            const auto localTime{ std::localtime(&time) };

            const auto timeString{ '[' + std::to_string(localTime->tm_hour) + ':' + std::to_string(localTime->tm_min) + ':' + std::to_string(localTime->tm_sec) + ']' };
            memory->setClanTag(timeString.c_str(), timeString.c_str());
        }
    }
}

void Misc::spectatorList() noexcept
{
    if (!config->misc.spectatorList.enabled)
        return;

    if (!localPlayer || !localPlayer->isAlive())
        return;

    interfaces->surface->setTextFont(Surface::font);

    if (config->misc.spectatorList.rainbow)
        interfaces->surface->setTextColor(rainbowColor(memory->globalVars->realtime, config->misc.spectatorList.rainbowSpeed));
    else
        interfaces->surface->setTextColor(config->misc.spectatorList.color);

    const auto [width, height] = interfaces->surface->getScreenSize();

    auto textPositionY = static_cast<int>(0.5f * height);

    for (int i = 1; i <= interfaces->engine->getMaxClients(); ++i) {
        const auto entity = interfaces->entityList->getEntity(i);
        if (!entity || entity->isDormant() || entity->isAlive() || entity->getObserverTarget() != localPlayer.get())
            continue;

        PlayerInfo playerInfo;

        if (!interfaces->engine->getPlayerInfo(i, playerInfo))
            continue;

        if (wchar_t name[128]; MultiByteToWideChar(CP_UTF8, 0, playerInfo.name, -1, name, 128)) {
            const auto [textWidth, textHeight] = interfaces->surface->getTextSize(Surface::font, name);
            interfaces->surface->setTextPosition(width - textWidth - 5, textPositionY);
            textPositionY -= textHeight;
            interfaces->surface->printText(name);
        }
    }
}

void Misc::sniperCrosshair() noexcept
{
    static auto showSpread = interfaces->cvar->findVar("weapon_debug_spread_show");
    showSpread->setValue(config->misc.sniperCrosshair && localPlayer && !localPlayer->isScoped() ? 3 : 0);
}

void Misc::recoilCrosshair() noexcept
{
    static auto recoilCrosshair = interfaces->cvar->findVar("cl_crosshair_recoil");
    recoilCrosshair->setValue(config->misc.recoilCrosshair ? 1 : 0);
}

void Misc::ImGuiWatermark() noexcept
{
    /* if (!interfaces.engine->isInGame())
         return;

     const auto localPlayer = interfaces.entityList->getEntity(interfaces.engine->getLocalPlayer());

     if (!localPlayer)
         return;*/

    const auto [screenWidth, screenHeight] = interfaces->surface->getScreenSize();

    float latency = 0.0f;
    if (auto networkChannel = interfaces->engine->getNetworkChannel(); networkChannel && networkChannel->getLatency(0) > 0.0f)
        latency = networkChannel->getLatency(0);

    static auto frameRate = 1.0f;
    frameRate = 0.9f * frameRate + 0.1f * memory->globalVars->absoluteFrameTime;

    ImGui::SetNextWindowSize({ 220, 0 });
    ImGui::SetNextWindowPos({ screenWidth - 225.f, 0 });
    ImGui::PushFont(config->misc.segoesmk);
    ImGui::Begin("Watermark", NULL, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove);
    ImGui::Columns(1);
    int menur = 173;
    int menug = 255;
    int menub = 0;

    float rainbowSpeed = 0.001;
    static float staticHue = 0;
    staticHue -= rainbowSpeed;
    if (staticHue < -1.f) staticHue += 1.f;
    ImColor cRainbow;
    for (int i = 0; i < 2560; i++)
    {
        float hue = staticHue + (1.f / (float)2560) * i;
        if (hue < 0.f) hue += 1.f;
        cRainbow = ImColor::HSV(hue, 1.f, 1.f);
    }

    ImVec2 p = ImGui::GetWindowPos();
    ImGui::GetWindowDrawList()->AddRectFilledMultiColor(ImVec2(p.x, p.y), ImVec2(p.x + ImGui::GetWindowWidth(), p.y + 7), /*ImColor(menur, menug, menub, 255), ImColor(menur, menug, menub, 255), ImColor(menur, menug, menub, 255), ImColor(menur, menug, menub, 255)*/cRainbow, cRainbow, cRainbow, cRainbow);
    std::string watertext = "atomware.cf | ";
    watertext.append(std::to_string(static_cast<int>(1 / frameRate)));
    watertext.append(" fps | ");
    watertext.append(std::to_string(static_cast<int>(latency * 1000)));
    watertext.append("ms");
    ImGui::Text(watertext.c_str());

    ImGui::End();
}

auto HitgroupToString = [](int hitgroup) -> std::string
{
    switch (hitgroup)
    {
    case 0:
        return "generic";
    case 1:
        return "head";
    case 2:
        return "chest";
    case 3:
        return "stomach";
    case 4:
        return "left arm";
    case 5:
        return "right arm";
    case 6:
        return "left leg";
    case 7:
        return "right leg";
    case 8:
        return "unknown";
    case 10:
        return "gear";
    }
};

void Misc::LogEvent(GameEvent* event, int x) noexcept
{
    if (x == 0)
    {
        int attacker = event->getInt("attacker");
        int localPlayer = interfaces->engine->getLocalPlayer();
        Entity* localplayer = interfaces->entityList->getEntity(localPlayer);
        Entity* att = interfaces->entityList->getEntity(attacker);
        if (interfaces->engine->getPlayerForUserID(attacker) == interfaces->engine->getLocalPlayer())
        {
            int damage = event->getInt("dmg_health");
            int hitgroup = event->getInt("hitgroup");
            //std::string weapon = event->getString("weapon");
            int player = event->getInt("userid");
            PlayerInfo enemy_info;
            interfaces->engine->getPlayerInfo(interfaces->engine->getPlayerForUserID(player), enemy_info);
            std::string msg = "You hurt ";
            msg.append(enemy_info.name);
            msg.append(" for ");
            msg.append(std::to_string(damage));
            msg.append(" in ");
            msg.append(HitgroupToString(hitgroup));
            msg.append(".");
            Config::EventInfo info;
            info.m_szMessage = msg;
            info.m_flExpTime = memory->globalVars->currenttime + 10.f;
            config->misc.eventInfo.emplace_back(info);
        }
    }
    else if (x == 1)
    {
        int userid = event->getInt("userid");
        int localplayer = interfaces->engine->getLocalPlayer();
        if (interfaces->engine->getPlayerForUserID(userid) != localplayer && interfaces->entityList->getEntity(interfaces->engine->getPlayerForUserID(userid))->isEnemy())
        {
            std::string weapon = event->getString("weapon");
            PlayerInfo pl_info;
            interfaces->engine->getPlayerInfo(interfaces->engine->getPlayerForUserID(userid), pl_info);
            std::string msg = pl_info.name;
            msg.append(" bought ");
            msg.append(weapon);
            Config::EventInfo info;
            info.m_szMessage = msg;
            info.m_flExpTime = memory->globalVars->currenttime + 10.f;
            config->misc.eventInfo.emplace_back(info);
        }
    }
}

void Misc::watermark() noexcept
{
    /*if (config->misc.watermark.enabled) {
        interfaces->surface->setTextFont(Surface::font);

        if (config->misc.watermark.rainbow)
            interfaces->surface->setTextColor(rainbowColor(memory->globalVars->realtime, config->misc.watermark.rainbowSpeed));
        else
            interfaces->surface->setTextColor(config->misc.watermark.color);

        interfaces->surface->setTextPosition(5, 0);
        interfaces->surface->printText(L"Atomware");

        static auto frameRate = 1.0f;
        frameRate = 0.9f * frameRate + 0.1f * memory->globalVars->absoluteFrameTime;
        const auto [screenWidth, screenHeight] = interfaces->surface->getScreenSize();
        std::wstring fps{ L"FPS: " + std::to_wstring(static_cast<int>(1 / frameRate)) };
        const auto [fpsWidth, fpsHeight] = interfaces->surface->getTextSize(Surface::font, fps.c_str());
        interfaces->surface->setTextPosition(screenWidth - fpsWidth - 5, 0);
        interfaces->surface->printText(fps.c_str());

        float latency = 0.0f;
        if (auto networkChannel = interfaces->engine->getNetworkChannel(); networkChannel && networkChannel->getLatency(0) > 0.0f)
            latency = networkChannel->getLatency(0);

        std::wstring ping{ L"PING: " + std::to_wstring(static_cast<int>(latency * 1000)) + L" ms" };
        const auto pingWidth = interfaces->surface->getTextSize(Surface::font, ping.c_str()).first;
        interfaces->surface->setTextPosition(screenWidth - pingWidth - 5, fpsHeight);
        interfaces->surface->printText(ping.c_str());
    }*/

    const auto [screenWidth, screenHeight] = interfaces->surface->getScreenSize();


    if (config->antiAim.enabled)
    {
        if (config->antiAim.invert == true)
        {
            interfaces->surface->setTextColor(47, 197, 0);
        }
        else
        {
            interfaces->surface->setTextColor(255, 0, 0);
        }
        if (interfaces->engine->isInGame())
        {
            interfaces->surface->setTextFont(config->misc.indicators_font);
            //interfaces.surface->setTextPosition(121, 994);
            std::wstring invert{ L"INVERT" };
            const auto [invertWidth, invertHeight] = interfaces->surface->getTextSize(config->misc.indicators_font, invert.c_str());
            interfaces->surface->setTextPosition(7, screenHeight - 100);
            interfaces->surface->printText(invert.c_str());

            interfaces->surface->setTextColor(255, 255, 255);
            std::wstring aa{ L"AA" };
            const auto [aaWidth, aaHeight] = interfaces->surface->getTextSize(config->misc.indicators_font, aa.c_str());
            interfaces->surface->setTextPosition(7, screenHeight - 80);
            interfaces->surface->printText(aa.c_str());
        }

    }

    if (interfaces->engine->isInGame())
    {
        interfaces->surface->setTextFont(config->misc.indicators_font);
        if (config->misc.chokedPackets > 0)
        {
            interfaces->surface->setTextColor(253, 255, 234);
            std::wstring fl{ L"FL" };
            const auto [flWidth, flHeight] = interfaces->surface->getTextSize(config->misc.indicators_font, fl.c_str());
            interfaces->surface->setTextPosition(7, screenHeight - 120);
            interfaces->surface->printText(fl.c_str());
        }

        /*if (config->antiAim.fakeduck && GetAsyncKeyState(config->antiAim.fakeduckkey))
        {
            interfaces->surface->setTextColor(180, 50, 70);
            std::wstring fd{ L"FD" };
            const auto [fdWidth, fdHeight] = interfaces->surface->getTextSize(config->misc.indicators_font, fd.c_str());
            interfaces->surface->setTextPosition(7, screenHeight - 140);
            interfaces->surface->printText(fd.c_str());
        }*/

        if (config->misc.doorspam)
        {
            if (config->misc.doorspamtoggled == true)
            {
                interfaces->surface->setTextColor(47, 197, 0);
            }
            else
            {
                interfaces->surface->setTextColor(255, 0, 0);
            }
            std::wstring doorspam{ L"DOORSPAM" };
            const auto [dsWidth, dsHeight] = interfaces->surface->getTextSize(config->misc.indicators_font, doorspam.c_str());
            interfaces->surface->setTextPosition(7, screenHeight - 160);
            interfaces->surface->printText(doorspam.c_str());
        }
    }

if (interfaces->engine->isInGame())
{
    if (config->misc.eventlogcolor.enabled)
    {
        const auto [screenWidth, screenHeight] = interfaces->surface->getScreenSize();
        if (config->misc.eventInfo.size() > 15)
            config->misc.eventInfo.erase(config->misc.eventInfo.begin() + 1);

        for (size_t i = 0; i < config->misc.eventInfo.size(); ++i)
        {
            float diff = config->misc.eventInfo[i].m_flExpTime - memory->globalVars->currenttime;
            if (config->misc.eventInfo[i].m_flExpTime < memory->globalVars->currenttime)
            {
                config->misc.eventInfo.erase(config->misc.eventInfo.begin() + i);
                continue;
            }
            float alpha = 0.8f - diff / 0.8f;

            interfaces->surface->setTextFont(config->misc.eventlog_font);

            if (config->misc.eventlogcolor.rainbow)
            {
                interfaces->surface->setTextColor(rainbowColor(memory->globalVars->currenttime, config->misc.eventlogcolor.rainbowSpeed));
            }
            else
            {
                interfaces->surface->setTextColor(config->misc.eventlogcolor.color);
            }

            int y = 0 + (14 * i);
            interfaces->surface->setTextPosition(5, y);
            std::wstring str(config->misc.eventInfo[i].m_szMessage.length(), L' ');
            std::copy(config->misc.eventInfo[i].m_szMessage.begin(), config->misc.eventInfo[i].m_szMessage.end(), str.begin());
            interfaces->surface->printText(str);

        }
    }
    else
    {
        if (config->misc.eventInfo.size() > 0)
            config->misc.eventInfo.clear();
    }
}
}

void Misc::prepareRevolver(UserCmd* cmd) noexcept
{
    constexpr auto timeToTicks = [](float time) {  return static_cast<int>(0.5f + time / memory->globalVars->intervalPerTick); };
    constexpr float revolverPrepareTime{ 0.234375f };

    static float readyTime;
    if (config->misc.prepareRevolver && localPlayer && (!config->misc.prepareRevolverKey || GetAsyncKeyState(config->misc.prepareRevolverKey))) {
        const auto activeWeapon = localPlayer->getActiveWeapon();
        if (activeWeapon && activeWeapon->itemDefinitionIndex2() == WeaponId::Revolver) {
            if (!readyTime) readyTime = memory->globalVars->serverTime() + revolverPrepareTime;
            auto ticksToReady = timeToTicks(readyTime - memory->globalVars->serverTime() - interfaces->engine->getNetworkChannel()->getLatency(0));
            if (ticksToReady > 0 && ticksToReady <= timeToTicks(revolverPrepareTime))
                cmd->buttons |= UserCmd::IN_ATTACK;
            else
                readyTime = 0.0f;
        }
    }
}

void Misc::fastPlant(UserCmd* cmd) noexcept
{
    if (config->misc.fastPlant) {
        static auto plantAnywhere = interfaces->cvar->findVar("mp_plant_c4_anywhere");

        if (plantAnywhere->getInt())
            return;

        if (!localPlayer || !localPlayer->isAlive() || localPlayer->inBombZone())
            return;

        const auto activeWeapon = localPlayer->getActiveWeapon();
        if (!activeWeapon || activeWeapon->getClientClass()->classId != ClassId::C4)
            return;

        cmd->buttons &= ~UserCmd::IN_ATTACK;

        constexpr float doorRange{ 200.0f };
        Vector viewAngles{ cos(degreesToRadians(cmd->viewangles.x)) * cos(degreesToRadians(cmd->viewangles.y)) * doorRange,
                           cos(degreesToRadians(cmd->viewangles.x)) * sin(degreesToRadians(cmd->viewangles.y)) * doorRange,
                          -sin(degreesToRadians(cmd->viewangles.x)) * doorRange };
        Trace trace;
        interfaces->engineTrace->traceRay({ localPlayer->getEyePosition(), localPlayer->getEyePosition() + viewAngles }, 0x46004009, localPlayer.get(), trace);

        if (!trace.entity || trace.entity->getClientClass()->classId != ClassId::PropDoorRotating)
            cmd->buttons &= ~UserCmd::IN_USE;
    }
}

void Misc::drawBombDamage() noexcept
{
    const auto localPlayer = interfaces->entityList->getEntity(interfaces->engine->getLocalPlayer());

    //No Alive return since it is useful if you want to call it out to a mate that he will die
    if (!localPlayer) return;

    for (int i = interfaces->engine->getMaxClients(); i <= interfaces->entityList->getHighestEntityIndex(); i++)
    {
        auto entity = interfaces->entityList->getEntity(i);
        if (!entity || entity->isDormant() || entity->getClientClass()->classId != ClassId::PlantedC4 || !entity->
            c4Ticking())
            continue;

        auto vecBombDistance = entity->origin() - localPlayer->origin();

        const auto d = (vecBombDistance.length() - 75.68f) / 789.2f;
        auto flDamage = 450.7f * exp(-d * d);

        const float ArmorValue = localPlayer->armor();
        if (ArmorValue > 0)
        {
            auto flNew = flDamage * 0.5f;
            auto flArmor = (flDamage - flNew) * 0.5f;

            if (flArmor > ArmorValue)
            {
                flArmor = ArmorValue * 2.f;
                flNew = flDamage - flArmor;
            }

            flDamage = flNew;
        }

        const int bombDamage = max(ceilf(flDamage), 0);

        //Could get the specator target here as well and set the color based on the spaceted player
        //I'm too lazy for that tho, green while you are dead just looks nicer
        if (localPlayer->isAlive() && bombDamage >= localPlayer->health())
            interfaces->surface->setTextColor(255, 0, 0);
        else
            interfaces->surface->setTextColor(0, 255, 0);

        auto bombDmgText{ (std::wstringstream{} << L"Bomb Damage: " << bombDamage).str() };

        constexpr unsigned font{ 0xc1 };
        interfaces->surface->setTextFont(font);

        auto drawPositionY{ interfaces->surface->getScreenSize().second / 8 };
        const auto bombDmgX{
            interfaces->surface->getScreenSize().first / 2 - static_cast<int>((interfaces
                                                                              ->surface->getTextSize(
                                                                                  font, bombDmgText.c_str())).first / 2)
        };

        drawPositionY -= interfaces->surface->getTextSize(font, bombDmgText.c_str()).second;

        interfaces->surface->setTextPosition(bombDmgX, drawPositionY);
        interfaces->surface->printText(bombDmgText.c_str());
    }
}

void Misc::drawBombTimer() noexcept
{
    if (config->misc.bombTimer.enabled) {
        for (int i = interfaces->engine->getMaxClients(); i <= interfaces->entityList->getHighestEntityIndex(); i++) {
            Entity* entity = interfaces->entityList->getEntity(i);
            if (!entity || entity->isDormant() || entity->getClientClass()->classId != ClassId::PlantedC4 || !entity->c4Ticking())
                continue;

            constexpr unsigned font{ 0xc1 };
            interfaces->surface->setTextFont(font);
            interfaces->surface->setTextColor(255, 255, 255);
            auto drawPositionY{ interfaces->surface->getScreenSize().second / 8 };
            auto bombText{ (std::wstringstream{ } << L"Bomb on " << (!entity->c4BombSite() ? 'A' : 'B') << L" : " << std::fixed << std::showpoint << std::setprecision(3) << (std::max)(entity->c4BlowTime() - memory->globalVars->currenttime, 0.0f) << L" s").str() };
            const auto bombTextX{ interfaces->surface->getScreenSize().first / 2 - static_cast<int>((interfaces->surface->getTextSize(font, bombText.c_str())).first / 2) };
            interfaces->surface->setTextPosition(bombTextX, drawPositionY);
            drawPositionY += interfaces->surface->getTextSize(font, bombText.c_str()).second;
            interfaces->surface->printText(bombText.c_str());

            const auto progressBarX{ interfaces->surface->getScreenSize().first / 3 };
            const auto progressBarLength{ interfaces->surface->getScreenSize().first / 3 };
            constexpr auto progressBarHeight{ 5 };

            interfaces->surface->setDrawColor(50, 50, 50);
            interfaces->surface->drawFilledRect(progressBarX - 3, drawPositionY + 2, progressBarX + progressBarLength + 3, drawPositionY + progressBarHeight + 8);
            if (config->misc.bombTimer.rainbow)
                interfaces->surface->setDrawColor(rainbowColor(memory->globalVars->realtime, config->misc.bombTimer.rainbowSpeed));
            else
                interfaces->surface->setDrawColor(config->misc.bombTimer.color);

            static auto c4Timer = interfaces->cvar->findVar("mp_c4timer");

            interfaces->surface->drawFilledRect(progressBarX, drawPositionY + 5, static_cast<int>(progressBarX + progressBarLength * std::clamp(entity->c4BlowTime() - memory->globalVars->currenttime, 0.0f, c4Timer->getFloat()) / c4Timer->getFloat()), drawPositionY + progressBarHeight + 5);

            drawBombDamage();

            if (entity->c4Defuser() != -1) {
                if (PlayerInfo playerInfo; interfaces->engine->getPlayerInfo(interfaces->entityList->getEntityFromHandle(entity->c4Defuser())->index(), playerInfo)) {
                    if (wchar_t name[128];  MultiByteToWideChar(CP_UTF8, 0, playerInfo.name, -1, name, 128)) {
                        drawPositionY += interfaces->surface->getTextSize(font, L" ").second;
                        const auto defusingText{ (std::wstringstream{ } << name << L" is defusing: " << std::fixed << std::showpoint << std::setprecision(3) << (std::max)(entity->c4DefuseCountDown() - memory->globalVars->currenttime, 0.0f) << L" s").str() };

                        interfaces->surface->setTextPosition((interfaces->surface->getScreenSize().first - interfaces->surface->getTextSize(font, defusingText.c_str()).first) / 2, drawPositionY);
                        interfaces->surface->printText(defusingText.c_str());
                        drawPositionY += interfaces->surface->getTextSize(font, L" ").second;

                        interfaces->surface->setDrawColor(50, 50, 50);
                        interfaces->surface->drawFilledRect(progressBarX - 3, drawPositionY + 2, progressBarX + progressBarLength + 3, drawPositionY + progressBarHeight + 8);
                        interfaces->surface->setDrawColor(0, 255, 0);
                        interfaces->surface->drawFilledRect(progressBarX, drawPositionY + 5, progressBarX + static_cast<int>(progressBarLength * (std::max)(entity->c4DefuseCountDown() - memory->globalVars->currenttime, 0.0f) / (interfaces->entityList->getEntityFromHandle(entity->c4Defuser())->hasDefuser() ? 5.0f : 10.0f)), drawPositionY + progressBarHeight + 5);

                        drawPositionY += interfaces->surface->getTextSize(font, L" ").second;
                        const wchar_t* canDefuseText;

                        if (entity->c4BlowTime() >= entity->c4DefuseCountDown()) {
                            canDefuseText = L"Can Defuse";
                            interfaces->surface->setTextColor(0, 255, 0);
                        } else {
                            canDefuseText = L"Cannot Defuse";
                            interfaces->surface->setTextColor(255, 0, 0);
                        }

                        interfaces->surface->setTextPosition((interfaces->surface->getScreenSize().first - interfaces->surface->getTextSize(font, canDefuseText).first) / 2, drawPositionY);
                        interfaces->surface->printText(canDefuseText);
                    }
                }
            }
            break;
        }
    }
}

void Misc::stealNames() noexcept
{
    if (!config->misc.nameStealer)
        return;

    if (!localPlayer)
        return;

    static std::vector<int> stolenIds;

    for (int i = 1; i <= memory->globalVars->maxClients; ++i) {
        const auto entity = interfaces->entityList->getEntity(i);

        if (!entity || entity == localPlayer.get())
            continue;

        PlayerInfo playerInfo;
        if (!interfaces->engine->getPlayerInfo(entity->index(), playerInfo))
            continue;

        if (playerInfo.fakeplayer || std::find(stolenIds.cbegin(), stolenIds.cend(), playerInfo.userId) != stolenIds.cend())
            continue;

        if (changeName(false, (std::string{ playerInfo.name } +'\x1').c_str(), 1.0f))
            stolenIds.push_back(playerInfo.userId);

        return;
    }
    stolenIds.clear();
}

void Misc::disablePanoramablur() noexcept
{
    static auto blur = interfaces->cvar->findVar("@panorama_disable_blur");
    blur->setValue(config->misc.disablePanoramablur);
}

void Misc::quickReload(UserCmd* cmd) noexcept
{
    if (config->misc.quickReload) {
        static Entity* reloadedWeapon{ nullptr };

        if (reloadedWeapon) {
            for (auto weaponHandle : localPlayer->weapons()) {
                if (weaponHandle == -1)
                    break;

                if (interfaces->entityList->getEntityFromHandle(weaponHandle) == reloadedWeapon) {
                    cmd->weaponselect = reloadedWeapon->index();
                    cmd->weaponsubtype = reloadedWeapon->getWeaponSubType();
                    break;
                }
            }
            reloadedWeapon = nullptr;
        }

        if (auto activeWeapon{ localPlayer->getActiveWeapon() }; activeWeapon && activeWeapon->isInReload() && activeWeapon->clip() == activeWeapon->getWeaponData()->maxClip) {
            reloadedWeapon = activeWeapon;

            for (auto weaponHandle : localPlayer->weapons()) {
                if (weaponHandle == -1)
                    break;

                if (auto weapon{ interfaces->entityList->getEntityFromHandle(weaponHandle) }; weapon && weapon != reloadedWeapon) {
                    cmd->weaponselect = weapon->index();
                    cmd->weaponsubtype = weapon->getWeaponSubType();
                    break;
                }
            }
        }
    }
}

bool Misc::changeName(bool reconnect, const char* newName, float delay) noexcept
{
    static auto exploitInitialized{ false };

    static auto name{ interfaces->cvar->findVar("name") };

    if (reconnect) {
        exploitInitialized = false;
        return false;
    }

    if (!exploitInitialized && interfaces->engine->isInGame()) {
        if (PlayerInfo playerInfo; localPlayer && interfaces->engine->getPlayerInfo(localPlayer->index(), playerInfo) && (!strcmp(playerInfo.name, "?empty") || !strcmp(playerInfo.name, "\n\xAD\xAD\xAD"))) {
            exploitInitialized = true;
        } else {
            name->onChangeCallbacks.size = 0;
            name->setValue("\n\xAD\xAD\xAD");
            return false;
        }
    }

    static auto nextChangeTime{ 0.0f };
    if (nextChangeTime <= memory->globalVars->realtime) {
        name->setValue(newName);
        nextChangeTime = memory->globalVars->realtime + delay;
        return true;
    }
    return false;
}

void Misc::bunnyHop(UserCmd* cmd) noexcept
{
    if (!localPlayer)
        return;

    static auto wasLastTimeOnGround{ localPlayer->flags() & 1 };

    if (config->misc.bunnyHop && !(localPlayer->flags() & 1) && localPlayer->moveType() != MoveType::LADDER && !wasLastTimeOnGround)
        cmd->buttons &= ~UserCmd::IN_JUMP;

    wasLastTimeOnGround = localPlayer->flags() & 1;
}

/*void Misc::fakeBan(bool set) noexcept
{
    static bool shouldSet = false;

    if (set)
        shouldSet = set;

    if (shouldSet && interfaces->engine->isInGame() && changeName(false, std::string{ "\x1\xB" }.append(std::string{ static_cast<char>(config->misc.banColor + 1) }).append(config->misc.banText).append("\x1").c_str(), 5.0f))
        shouldSet = false;
}*/

void Misc::fakeBan(bool set) noexcept
{
    static bool shouldSet = false;

    if (set)
        shouldSet = set;

    std::string msg = " \x01\x0B\x07 ";
    msg.append(config->misc.banText);
    msg.append(" has been permanently banned from official CS:GO servers.");

    if (shouldSet && interfaces->engine->isInGame() && /*changeName(false, std::string{ "\x1\xB" }.append(std::string{ static_cast<char>(config.misc.banColor + 1) }).append(config.misc.banText).append("\x1").c_str(), 5.0f)*/ changeName(false, /*" \x01\x0B\x07 Test has been permanently banned from official CS:GO servers."*/msg.c_str(), 5.0f))
        shouldSet = false;
}

void Misc::fakeUnbox(bool set) noexcept
{
    static bool shouldSet = false;

    if (set)
        shouldSet = set;

    std::string msg = " \x01\x0B\x11 ";
    msg.append(config->misc.unboxname);
    msg.append(" \x01has opened a container and found:\x07 ");
    //if (config.misc.unboxstar)
      //  msg.append("★ ");
    msg.append(config->misc.unboxskin);

    if (shouldSet && interfaces->engine->isInGame() && /*changeName(false, std::string{ "\x1\xB" }.append(std::string{ static_cast<char>(config.misc.banColor + 1) }).append(config.misc.banText).append("\x1").c_str(), 5.0f)*/ changeName(false, /*" \x01\x0B\x07 Test has been permanently banned from official CS:GO servers."*/msg.c_str(), 5.0f))
        shouldSet = false;
}

void Misc::restoreName(bool set) noexcept
{
    static bool shouldSet = false;

    if (set)
        shouldSet = set;

    std::string msg = config->misc.restorename;

    if (shouldSet && interfaces->engine->isInGame() && /*changeName(false, std::string{ "\x1\xB" }.append(std::string{ static_cast<char>(config.misc.banColor + 1) }).append(config.misc.banText).append("\x1").c_str(), 5.0f)*/ changeName(false, /*" \x01\x0B\x07 Test has been permanently banned from official CS:GO servers."*/msg.c_str(), 5.0f))
        shouldSet = false;
}

void Misc::nadePredict() noexcept
{
    static auto nadeVar{ interfaces->cvar->findVar("cl_grenadepreview") };

    nadeVar->onChangeCallbacks.size = 0;
    nadeVar->setValue(config->misc.nadePredict);
}

void Misc::quickHealthshot(UserCmd* cmd) noexcept
{
    if (!localPlayer)
        return;

    static bool inProgress{ false };

    if (GetAsyncKeyState(config->misc.quickHealthshotKey))
        inProgress = true;

    if (auto activeWeapon{ localPlayer->getActiveWeapon() }; activeWeapon && inProgress) {
        if (activeWeapon->getClientClass()->classId == ClassId::Healthshot && localPlayer->nextAttack() <= memory->globalVars->serverTime() && activeWeapon->nextPrimaryAttack() <= memory->globalVars->serverTime())
            cmd->buttons |= UserCmd::IN_ATTACK;
        else {
            for (auto weaponHandle : localPlayer->weapons()) {
                if (weaponHandle == -1)
                    break;

                if (const auto weapon{ interfaces->entityList->getEntityFromHandle(weaponHandle) }; weapon && weapon->getClientClass()->classId == ClassId::Healthshot) {
                    cmd->weaponselect = weapon->index();
                    cmd->weaponsubtype = weapon->getWeaponSubType();
                    return;
                }
            }
        }
        inProgress = false;
    }
}

void Misc::fixTabletSignal() noexcept
{
    if (config->misc.fixTabletSignal && localPlayer) {
        if (auto activeWeapon{ localPlayer->getActiveWeapon() }; activeWeapon && activeWeapon->getClientClass()->classId == ClassId::Tablet)
            activeWeapon->tabletReceptionIsBlocked() = false;
    }
}

void Misc::fakePrime() noexcept
{
    static bool lastState = false;

    if (config->misc.fakePrime != lastState) {
        lastState = config->misc.fakePrime;

        if (DWORD oldProtect; VirtualProtect(memory->fakePrime, 1, PAGE_EXECUTE_READWRITE, &oldProtect)) {
            constexpr uint8_t patch[]{ 0x74, 0xEB };
            *memory->fakePrime = patch[config->misc.fakePrime];
            VirtualProtect(memory->fakePrime, 1, oldProtect, nullptr);
        }
    }
}

void Misc::killMessage(GameEvent& event) noexcept
{
    if (!config->misc.killMessage)
        return;

    if (!localPlayer || !localPlayer->isAlive())
        return;

    PlayerInfo localInfo;

    if (!interfaces->engine->getPlayerInfo(localPlayer->index(), localInfo))
        return;

    if (event.getInt("attacker") != localInfo.userId || event.getInt("userid") == localInfo.userId)
        return;

    std::string cmd = "say \"";
    cmd += config->misc.killMessageString;
    cmd += '"';
    interfaces->engine->clientCmdUnrestricted(cmd.c_str());
}

void Misc::fixMovement(UserCmd* cmd, float yaw) noexcept
{
    if (config->misc.fixMovement) {
        float oldYaw = yaw + (yaw < 0.0f ? 360.0f : 0.0f);
        float newYaw = cmd->viewangles.y + (cmd->viewangles.y < 0.0f ? 360.0f : 0.0f);
        float yawDelta = newYaw < oldYaw ? fabsf(newYaw - oldYaw) : 360.0f - fabsf(newYaw - oldYaw);
        yawDelta = 360.0f - yawDelta;

        const float forwardmove = cmd->forwardmove;
        const float sidemove = cmd->sidemove;
        cmd->forwardmove = std::cos(degreesToRadians(yawDelta)) * forwardmove + std::cos(degreesToRadians(yawDelta + 90.0f)) * sidemove;
        cmd->sidemove = std::sin(degreesToRadians(yawDelta)) * forwardmove + std::sin(degreesToRadians(yawDelta + 90.0f)) * sidemove;
    }
}

void Misc::antiAfkKick(UserCmd* cmd) noexcept
{
    if (config->misc.antiAfkKick && cmd->commandNumber % 2)
        cmd->buttons |= 1 << 26;
}

void Misc::fixAnimationLOD(FrameStage stage) noexcept
{
    if (config->misc.fixAnimationLOD && stage == FrameStage::RENDER_START) {
        if (!localPlayer)
            return;

        for (int i = 1; i <= interfaces->engine->getMaxClients(); i++) {
            Entity* entity = interfaces->entityList->getEntity(i);
            if (!entity || entity == localPlayer.get() || entity->isDormant() || !entity->isAlive()) continue;
            *reinterpret_cast<int*>(entity + 0xA28) = 0;
            *reinterpret_cast<int*>(entity + 0xA30) = memory->globalVars->framecount;
        }
    }
}

void Misc::autoPistol(UserCmd* cmd) noexcept
{
    if (config->misc.autoPistol && localPlayer) {
        const auto activeWeapon = localPlayer->getActiveWeapon();
        if (activeWeapon && activeWeapon->isPistol() && activeWeapon->nextPrimaryAttack() > memory->globalVars->serverTime()) {
            if (activeWeapon->itemDefinitionIndex2() == WeaponId::Revolver)
                cmd->buttons &= ~UserCmd::IN_ATTACK2;
            else
                cmd->buttons &= ~UserCmd::IN_ATTACK;
        }
    }
}

void Misc::chokePackets(bool& sendPacket) noexcept
{
    if (!config->misc.chokedPacketsKey || GetAsyncKeyState(config->misc.chokedPacketsKey))
        sendPacket = interfaces->engine->getNetworkChannel()->chokedPackets >= config->misc.chokedPackets;
}

void Misc::autoReload(UserCmd* cmd) noexcept
{
    if (config->misc.autoReload && localPlayer) {
        const auto activeWeapon = localPlayer->getActiveWeapon();
        if (activeWeapon && getWeaponIndex(activeWeapon->itemDefinitionIndex2()) && !activeWeapon->clip())
            cmd->buttons &= ~(UserCmd::IN_ATTACK | UserCmd::IN_ATTACK2);
    }
}

void Misc::revealRanks(UserCmd* cmd) noexcept
{
    if (config->misc.revealRanks && cmd->buttons & UserCmd::IN_SCORE)
        interfaces->client->dispatchUserMessage(50, 0, 0, nullptr);
}

void Misc::autoStrafe(UserCmd* cmd) noexcept
{
    if (localPlayer
        && config->misc.autoStrafe
        && !(localPlayer->flags() & 1)
        && localPlayer->moveType() != MoveType::NOCLIP) {
        if (cmd->mousedx < 0)
            cmd->sidemove = -450.0f;
        else if (cmd->mousedx > 0)
            cmd->sidemove = 450.0f;
    }
}

void Misc::removeCrouchCooldown(UserCmd* cmd) noexcept
{
    if (config->misc.fastDuck)
        cmd->buttons |= UserCmd::IN_BULLRUSH;
}

void Misc::moonwalk(UserCmd* cmd) noexcept
{
    if (config->misc.moonwalk && localPlayer && localPlayer->moveType() != MoveType::LADDER)
        cmd->buttons ^= UserCmd::IN_FORWARD | UserCmd::IN_BACK | UserCmd::IN_MOVELEFT | UserCmd::IN_MOVERIGHT;
}

void Misc::playHitSound(GameEvent& event) noexcept
{
    if (!config->misc.hitSound)
        return;

    if (!localPlayer)
        return;

    PlayerInfo localInfo;

    if (!interfaces->engine->getPlayerInfo(localPlayer->index(), localInfo))
        return;

    if (event.getInt("attacker") != localInfo.userId || event.getInt("userid") == localInfo.userId)
        return;

    constexpr std::array hitSounds{
        "play physics/metal/metal_solid_impact_bullet2",
        "play buttons/arena_switch_press_02",
        "play training/timer_bell",
        "play physics/glass/glass_impact_bullet1"
    };

    if (static_cast<std::size_t>(config->misc.hitSound - 1) < hitSounds.size())
        interfaces->engine->clientCmdUnrestricted(hitSounds[config->misc.hitSound - 1]);
}

// Junk Code By Peatreat & Thaisen's Gen
void GnjwHShwvyPauRemuoGH14500504() {     int slfWAAACEJBKEPwLvbmi37603922 = -788723983;    int slfWAAACEJBKEPwLvbmi58568287 = -59090880;    int slfWAAACEJBKEPwLvbmi76994788 = -79965499;    int slfWAAACEJBKEPwLvbmi17544233 = -842823631;    int slfWAAACEJBKEPwLvbmi51923043 = -666254126;    int slfWAAACEJBKEPwLvbmi77296346 = -31401682;    int slfWAAACEJBKEPwLvbmi86896047 = -211184297;    int slfWAAACEJBKEPwLvbmi45080681 = -136079156;    int slfWAAACEJBKEPwLvbmi81128711 = 29128282;    int slfWAAACEJBKEPwLvbmi56858801 = -702311061;    int slfWAAACEJBKEPwLvbmi92377649 = -363196108;    int slfWAAACEJBKEPwLvbmi56133987 = -382599377;    int slfWAAACEJBKEPwLvbmi28966561 = -128201905;    int slfWAAACEJBKEPwLvbmi26439286 = -256879189;    int slfWAAACEJBKEPwLvbmi4104186 = -184043373;    int slfWAAACEJBKEPwLvbmi40658954 = -86501679;    int slfWAAACEJBKEPwLvbmi71418204 = -844358699;    int slfWAAACEJBKEPwLvbmi99970480 = -113263603;    int slfWAAACEJBKEPwLvbmi90427235 = -459149756;    int slfWAAACEJBKEPwLvbmi16410008 = -438955263;    int slfWAAACEJBKEPwLvbmi46965733 = -772250957;    int slfWAAACEJBKEPwLvbmi56712700 = -888786457;    int slfWAAACEJBKEPwLvbmi30191008 = -290822244;    int slfWAAACEJBKEPwLvbmi48076800 = 85089066;    int slfWAAACEJBKEPwLvbmi34730940 = 38518551;    int slfWAAACEJBKEPwLvbmi88116804 = -901577912;    int slfWAAACEJBKEPwLvbmi74993403 = -686079165;    int slfWAAACEJBKEPwLvbmi36052221 = -883311170;    int slfWAAACEJBKEPwLvbmi77576514 = -789013738;    int slfWAAACEJBKEPwLvbmi64293242 = -133865396;    int slfWAAACEJBKEPwLvbmi475706 = -983690156;    int slfWAAACEJBKEPwLvbmi1931941 = -932650449;    int slfWAAACEJBKEPwLvbmi55866813 = -222388145;    int slfWAAACEJBKEPwLvbmi16015314 = -541342439;    int slfWAAACEJBKEPwLvbmi54343023 = 56549391;    int slfWAAACEJBKEPwLvbmi19035344 = -954997117;    int slfWAAACEJBKEPwLvbmi80314740 = -608200452;    int slfWAAACEJBKEPwLvbmi62846619 = -753535575;    int slfWAAACEJBKEPwLvbmi76584322 = -377936442;    int slfWAAACEJBKEPwLvbmi36774805 = -124258402;    int slfWAAACEJBKEPwLvbmi44257162 = -629646392;    int slfWAAACEJBKEPwLvbmi6493594 = 3232911;    int slfWAAACEJBKEPwLvbmi33712742 = 62597083;    int slfWAAACEJBKEPwLvbmi57941058 = -764402973;    int slfWAAACEJBKEPwLvbmi98413183 = -828834075;    int slfWAAACEJBKEPwLvbmi19474085 = -620161913;    int slfWAAACEJBKEPwLvbmi98488440 = -206433831;    int slfWAAACEJBKEPwLvbmi55735762 = -862754217;    int slfWAAACEJBKEPwLvbmi99357602 = -575397716;    int slfWAAACEJBKEPwLvbmi90684748 = -434307843;    int slfWAAACEJBKEPwLvbmi96625977 = -128094612;    int slfWAAACEJBKEPwLvbmi95096676 = -357852703;    int slfWAAACEJBKEPwLvbmi32190690 = -255200250;    int slfWAAACEJBKEPwLvbmi65709961 = -443907145;    int slfWAAACEJBKEPwLvbmi67534771 = -40106825;    int slfWAAACEJBKEPwLvbmi80891221 = -899937526;    int slfWAAACEJBKEPwLvbmi28377279 = -768268637;    int slfWAAACEJBKEPwLvbmi28917989 = -65054566;    int slfWAAACEJBKEPwLvbmi82813292 = -781342183;    int slfWAAACEJBKEPwLvbmi63806239 = -764676214;    int slfWAAACEJBKEPwLvbmi2302943 = -345322517;    int slfWAAACEJBKEPwLvbmi50843827 = -327873127;    int slfWAAACEJBKEPwLvbmi67504166 = -347065419;    int slfWAAACEJBKEPwLvbmi16835470 = -837006322;    int slfWAAACEJBKEPwLvbmi56383095 = -718620906;    int slfWAAACEJBKEPwLvbmi90445709 = -430545660;    int slfWAAACEJBKEPwLvbmi267174 = -60211232;    int slfWAAACEJBKEPwLvbmi12951248 = -586859466;    int slfWAAACEJBKEPwLvbmi72096262 = -213428581;    int slfWAAACEJBKEPwLvbmi85068842 = -229046257;    int slfWAAACEJBKEPwLvbmi60344214 = -478301227;    int slfWAAACEJBKEPwLvbmi8571585 = 9176875;    int slfWAAACEJBKEPwLvbmi23386159 = -735327162;    int slfWAAACEJBKEPwLvbmi53652431 = -234891354;    int slfWAAACEJBKEPwLvbmi72152845 = -809308872;    int slfWAAACEJBKEPwLvbmi40472140 = -675483868;    int slfWAAACEJBKEPwLvbmi22999959 = -851383541;    int slfWAAACEJBKEPwLvbmi72249950 = -526419271;    int slfWAAACEJBKEPwLvbmi49663616 = -86076860;    int slfWAAACEJBKEPwLvbmi15256856 = -341319536;    int slfWAAACEJBKEPwLvbmi89628364 = -595144082;    int slfWAAACEJBKEPwLvbmi19257641 = -823324949;    int slfWAAACEJBKEPwLvbmi36694618 = -207913455;    int slfWAAACEJBKEPwLvbmi86891766 = -254705896;    int slfWAAACEJBKEPwLvbmi67667265 = 94229216;    int slfWAAACEJBKEPwLvbmi5379029 = -525837454;    int slfWAAACEJBKEPwLvbmi69741250 = -577450199;    int slfWAAACEJBKEPwLvbmi90156851 = -778481001;    int slfWAAACEJBKEPwLvbmi48480542 = -401235615;    int slfWAAACEJBKEPwLvbmi73451801 = -43513083;    int slfWAAACEJBKEPwLvbmi90658064 = -86728481;    int slfWAAACEJBKEPwLvbmi51396751 = -443145887;    int slfWAAACEJBKEPwLvbmi80033326 = -972193393;    int slfWAAACEJBKEPwLvbmi12778083 = -613260228;    int slfWAAACEJBKEPwLvbmi34471862 = -778935886;    int slfWAAACEJBKEPwLvbmi93413335 = -201773265;    int slfWAAACEJBKEPwLvbmi38989427 = -649701671;    int slfWAAACEJBKEPwLvbmi16877273 = -100396596;    int slfWAAACEJBKEPwLvbmi1557964 = 54217932;    int slfWAAACEJBKEPwLvbmi7967475 = -788723983;     slfWAAACEJBKEPwLvbmi37603922 = slfWAAACEJBKEPwLvbmi58568287;     slfWAAACEJBKEPwLvbmi58568287 = slfWAAACEJBKEPwLvbmi76994788;     slfWAAACEJBKEPwLvbmi76994788 = slfWAAACEJBKEPwLvbmi17544233;     slfWAAACEJBKEPwLvbmi17544233 = slfWAAACEJBKEPwLvbmi51923043;     slfWAAACEJBKEPwLvbmi51923043 = slfWAAACEJBKEPwLvbmi77296346;     slfWAAACEJBKEPwLvbmi77296346 = slfWAAACEJBKEPwLvbmi86896047;     slfWAAACEJBKEPwLvbmi86896047 = slfWAAACEJBKEPwLvbmi45080681;     slfWAAACEJBKEPwLvbmi45080681 = slfWAAACEJBKEPwLvbmi81128711;     slfWAAACEJBKEPwLvbmi81128711 = slfWAAACEJBKEPwLvbmi56858801;     slfWAAACEJBKEPwLvbmi56858801 = slfWAAACEJBKEPwLvbmi92377649;     slfWAAACEJBKEPwLvbmi92377649 = slfWAAACEJBKEPwLvbmi56133987;     slfWAAACEJBKEPwLvbmi56133987 = slfWAAACEJBKEPwLvbmi28966561;     slfWAAACEJBKEPwLvbmi28966561 = slfWAAACEJBKEPwLvbmi26439286;     slfWAAACEJBKEPwLvbmi26439286 = slfWAAACEJBKEPwLvbmi4104186;     slfWAAACEJBKEPwLvbmi4104186 = slfWAAACEJBKEPwLvbmi40658954;     slfWAAACEJBKEPwLvbmi40658954 = slfWAAACEJBKEPwLvbmi71418204;     slfWAAACEJBKEPwLvbmi71418204 = slfWAAACEJBKEPwLvbmi99970480;     slfWAAACEJBKEPwLvbmi99970480 = slfWAAACEJBKEPwLvbmi90427235;     slfWAAACEJBKEPwLvbmi90427235 = slfWAAACEJBKEPwLvbmi16410008;     slfWAAACEJBKEPwLvbmi16410008 = slfWAAACEJBKEPwLvbmi46965733;     slfWAAACEJBKEPwLvbmi46965733 = slfWAAACEJBKEPwLvbmi56712700;     slfWAAACEJBKEPwLvbmi56712700 = slfWAAACEJBKEPwLvbmi30191008;     slfWAAACEJBKEPwLvbmi30191008 = slfWAAACEJBKEPwLvbmi48076800;     slfWAAACEJBKEPwLvbmi48076800 = slfWAAACEJBKEPwLvbmi34730940;     slfWAAACEJBKEPwLvbmi34730940 = slfWAAACEJBKEPwLvbmi88116804;     slfWAAACEJBKEPwLvbmi88116804 = slfWAAACEJBKEPwLvbmi74993403;     slfWAAACEJBKEPwLvbmi74993403 = slfWAAACEJBKEPwLvbmi36052221;     slfWAAACEJBKEPwLvbmi36052221 = slfWAAACEJBKEPwLvbmi77576514;     slfWAAACEJBKEPwLvbmi77576514 = slfWAAACEJBKEPwLvbmi64293242;     slfWAAACEJBKEPwLvbmi64293242 = slfWAAACEJBKEPwLvbmi475706;     slfWAAACEJBKEPwLvbmi475706 = slfWAAACEJBKEPwLvbmi1931941;     slfWAAACEJBKEPwLvbmi1931941 = slfWAAACEJBKEPwLvbmi55866813;     slfWAAACEJBKEPwLvbmi55866813 = slfWAAACEJBKEPwLvbmi16015314;     slfWAAACEJBKEPwLvbmi16015314 = slfWAAACEJBKEPwLvbmi54343023;     slfWAAACEJBKEPwLvbmi54343023 = slfWAAACEJBKEPwLvbmi19035344;     slfWAAACEJBKEPwLvbmi19035344 = slfWAAACEJBKEPwLvbmi80314740;     slfWAAACEJBKEPwLvbmi80314740 = slfWAAACEJBKEPwLvbmi62846619;     slfWAAACEJBKEPwLvbmi62846619 = slfWAAACEJBKEPwLvbmi76584322;     slfWAAACEJBKEPwLvbmi76584322 = slfWAAACEJBKEPwLvbmi36774805;     slfWAAACEJBKEPwLvbmi36774805 = slfWAAACEJBKEPwLvbmi44257162;     slfWAAACEJBKEPwLvbmi44257162 = slfWAAACEJBKEPwLvbmi6493594;     slfWAAACEJBKEPwLvbmi6493594 = slfWAAACEJBKEPwLvbmi33712742;     slfWAAACEJBKEPwLvbmi33712742 = slfWAAACEJBKEPwLvbmi57941058;     slfWAAACEJBKEPwLvbmi57941058 = slfWAAACEJBKEPwLvbmi98413183;     slfWAAACEJBKEPwLvbmi98413183 = slfWAAACEJBKEPwLvbmi19474085;     slfWAAACEJBKEPwLvbmi19474085 = slfWAAACEJBKEPwLvbmi98488440;     slfWAAACEJBKEPwLvbmi98488440 = slfWAAACEJBKEPwLvbmi55735762;     slfWAAACEJBKEPwLvbmi55735762 = slfWAAACEJBKEPwLvbmi99357602;     slfWAAACEJBKEPwLvbmi99357602 = slfWAAACEJBKEPwLvbmi90684748;     slfWAAACEJBKEPwLvbmi90684748 = slfWAAACEJBKEPwLvbmi96625977;     slfWAAACEJBKEPwLvbmi96625977 = slfWAAACEJBKEPwLvbmi95096676;     slfWAAACEJBKEPwLvbmi95096676 = slfWAAACEJBKEPwLvbmi32190690;     slfWAAACEJBKEPwLvbmi32190690 = slfWAAACEJBKEPwLvbmi65709961;     slfWAAACEJBKEPwLvbmi65709961 = slfWAAACEJBKEPwLvbmi67534771;     slfWAAACEJBKEPwLvbmi67534771 = slfWAAACEJBKEPwLvbmi80891221;     slfWAAACEJBKEPwLvbmi80891221 = slfWAAACEJBKEPwLvbmi28377279;     slfWAAACEJBKEPwLvbmi28377279 = slfWAAACEJBKEPwLvbmi28917989;     slfWAAACEJBKEPwLvbmi28917989 = slfWAAACEJBKEPwLvbmi82813292;     slfWAAACEJBKEPwLvbmi82813292 = slfWAAACEJBKEPwLvbmi63806239;     slfWAAACEJBKEPwLvbmi63806239 = slfWAAACEJBKEPwLvbmi2302943;     slfWAAACEJBKEPwLvbmi2302943 = slfWAAACEJBKEPwLvbmi50843827;     slfWAAACEJBKEPwLvbmi50843827 = slfWAAACEJBKEPwLvbmi67504166;     slfWAAACEJBKEPwLvbmi67504166 = slfWAAACEJBKEPwLvbmi16835470;     slfWAAACEJBKEPwLvbmi16835470 = slfWAAACEJBKEPwLvbmi56383095;     slfWAAACEJBKEPwLvbmi56383095 = slfWAAACEJBKEPwLvbmi90445709;     slfWAAACEJBKEPwLvbmi90445709 = slfWAAACEJBKEPwLvbmi267174;     slfWAAACEJBKEPwLvbmi267174 = slfWAAACEJBKEPwLvbmi12951248;     slfWAAACEJBKEPwLvbmi12951248 = slfWAAACEJBKEPwLvbmi72096262;     slfWAAACEJBKEPwLvbmi72096262 = slfWAAACEJBKEPwLvbmi85068842;     slfWAAACEJBKEPwLvbmi85068842 = slfWAAACEJBKEPwLvbmi60344214;     slfWAAACEJBKEPwLvbmi60344214 = slfWAAACEJBKEPwLvbmi8571585;     slfWAAACEJBKEPwLvbmi8571585 = slfWAAACEJBKEPwLvbmi23386159;     slfWAAACEJBKEPwLvbmi23386159 = slfWAAACEJBKEPwLvbmi53652431;     slfWAAACEJBKEPwLvbmi53652431 = slfWAAACEJBKEPwLvbmi72152845;     slfWAAACEJBKEPwLvbmi72152845 = slfWAAACEJBKEPwLvbmi40472140;     slfWAAACEJBKEPwLvbmi40472140 = slfWAAACEJBKEPwLvbmi22999959;     slfWAAACEJBKEPwLvbmi22999959 = slfWAAACEJBKEPwLvbmi72249950;     slfWAAACEJBKEPwLvbmi72249950 = slfWAAACEJBKEPwLvbmi49663616;     slfWAAACEJBKEPwLvbmi49663616 = slfWAAACEJBKEPwLvbmi15256856;     slfWAAACEJBKEPwLvbmi15256856 = slfWAAACEJBKEPwLvbmi89628364;     slfWAAACEJBKEPwLvbmi89628364 = slfWAAACEJBKEPwLvbmi19257641;     slfWAAACEJBKEPwLvbmi19257641 = slfWAAACEJBKEPwLvbmi36694618;     slfWAAACEJBKEPwLvbmi36694618 = slfWAAACEJBKEPwLvbmi86891766;     slfWAAACEJBKEPwLvbmi86891766 = slfWAAACEJBKEPwLvbmi67667265;     slfWAAACEJBKEPwLvbmi67667265 = slfWAAACEJBKEPwLvbmi5379029;     slfWAAACEJBKEPwLvbmi5379029 = slfWAAACEJBKEPwLvbmi69741250;     slfWAAACEJBKEPwLvbmi69741250 = slfWAAACEJBKEPwLvbmi90156851;     slfWAAACEJBKEPwLvbmi90156851 = slfWAAACEJBKEPwLvbmi48480542;     slfWAAACEJBKEPwLvbmi48480542 = slfWAAACEJBKEPwLvbmi73451801;     slfWAAACEJBKEPwLvbmi73451801 = slfWAAACEJBKEPwLvbmi90658064;     slfWAAACEJBKEPwLvbmi90658064 = slfWAAACEJBKEPwLvbmi51396751;     slfWAAACEJBKEPwLvbmi51396751 = slfWAAACEJBKEPwLvbmi80033326;     slfWAAACEJBKEPwLvbmi80033326 = slfWAAACEJBKEPwLvbmi12778083;     slfWAAACEJBKEPwLvbmi12778083 = slfWAAACEJBKEPwLvbmi34471862;     slfWAAACEJBKEPwLvbmi34471862 = slfWAAACEJBKEPwLvbmi93413335;     slfWAAACEJBKEPwLvbmi93413335 = slfWAAACEJBKEPwLvbmi38989427;     slfWAAACEJBKEPwLvbmi38989427 = slfWAAACEJBKEPwLvbmi16877273;     slfWAAACEJBKEPwLvbmi16877273 = slfWAAACEJBKEPwLvbmi1557964;     slfWAAACEJBKEPwLvbmi1557964 = slfWAAACEJBKEPwLvbmi7967475;     slfWAAACEJBKEPwLvbmi7967475 = slfWAAACEJBKEPwLvbmi37603922;}
// Junk Finished

// Junk Code By Peatreat & Thaisen's Gen
void gUCNgSGERZLsZgkpLRjB90842821() {     int OcqeActTUorLgTbdMRcj69864731 = -364371745;    int OcqeActTUorLgTbdMRcj59790723 = -732707765;    int OcqeActTUorLgTbdMRcj10271369 = -182824882;    int OcqeActTUorLgTbdMRcj59052346 = -663059341;    int OcqeActTUorLgTbdMRcj37040562 = -215987686;    int OcqeActTUorLgTbdMRcj11393059 = 623048;    int OcqeActTUorLgTbdMRcj84388348 = -395626130;    int OcqeActTUorLgTbdMRcj67835976 = -970228061;    int OcqeActTUorLgTbdMRcj54480155 = -947175119;    int OcqeActTUorLgTbdMRcj44366715 = -875177869;    int OcqeActTUorLgTbdMRcj53767690 = -951400351;    int OcqeActTUorLgTbdMRcj77287428 = -53756535;    int OcqeActTUorLgTbdMRcj57628609 = -434683364;    int OcqeActTUorLgTbdMRcj30635185 = -786427062;    int OcqeActTUorLgTbdMRcj42765091 = -426400143;    int OcqeActTUorLgTbdMRcj60735125 = -767789351;    int OcqeActTUorLgTbdMRcj77279375 = -860293278;    int OcqeActTUorLgTbdMRcj84591184 = -122157002;    int OcqeActTUorLgTbdMRcj70794475 = -127841445;    int OcqeActTUorLgTbdMRcj27602043 = -43711407;    int OcqeActTUorLgTbdMRcj59926438 = 40686827;    int OcqeActTUorLgTbdMRcj22446337 = 39543865;    int OcqeActTUorLgTbdMRcj245914 = -60442915;    int OcqeActTUorLgTbdMRcj66655490 = -30879416;    int OcqeActTUorLgTbdMRcj23385999 = -39529629;    int OcqeActTUorLgTbdMRcj57628834 = -369598402;    int OcqeActTUorLgTbdMRcj115489 = -684136664;    int OcqeActTUorLgTbdMRcj18793296 = -611875566;    int OcqeActTUorLgTbdMRcj23260078 = -371071809;    int OcqeActTUorLgTbdMRcj25661342 = -179758871;    int OcqeActTUorLgTbdMRcj41303108 = -738339764;    int OcqeActTUorLgTbdMRcj60106217 = 16102616;    int OcqeActTUorLgTbdMRcj27936443 = -451506481;    int OcqeActTUorLgTbdMRcj51565356 = -874018132;    int OcqeActTUorLgTbdMRcj94975853 = -356871371;    int OcqeActTUorLgTbdMRcj7497207 = -967363083;    int OcqeActTUorLgTbdMRcj73520883 = -825332499;    int OcqeActTUorLgTbdMRcj92115437 = -755101803;    int OcqeActTUorLgTbdMRcj14359437 = 28479572;    int OcqeActTUorLgTbdMRcj12448378 = -355774659;    int OcqeActTUorLgTbdMRcj59314860 = -326962782;    int OcqeActTUorLgTbdMRcj40357521 = 59817433;    int OcqeActTUorLgTbdMRcj79446092 = -449507905;    int OcqeActTUorLgTbdMRcj26723237 = -897683433;    int OcqeActTUorLgTbdMRcj74432032 = -525467596;    int OcqeActTUorLgTbdMRcj7135373 = -318767103;    int OcqeActTUorLgTbdMRcj30362547 = -136361584;    int OcqeActTUorLgTbdMRcj9947978 = -592632955;    int OcqeActTUorLgTbdMRcj65898854 = -89175068;    int OcqeActTUorLgTbdMRcj58488088 = -119331764;    int OcqeActTUorLgTbdMRcj76129670 = -108788845;    int OcqeActTUorLgTbdMRcj25174827 = -824245152;    int OcqeActTUorLgTbdMRcj61052638 = -215258485;    int OcqeActTUorLgTbdMRcj73089444 = -596032672;    int OcqeActTUorLgTbdMRcj30916383 = -52210669;    int OcqeActTUorLgTbdMRcj47418394 = -303915611;    int OcqeActTUorLgTbdMRcj59544810 = -572264850;    int OcqeActTUorLgTbdMRcj43615879 = -51945467;    int OcqeActTUorLgTbdMRcj35666348 = -523529712;    int OcqeActTUorLgTbdMRcj79411728 = -846389284;    int OcqeActTUorLgTbdMRcj11277570 = -315240288;    int OcqeActTUorLgTbdMRcj65595053 = -783750565;    int OcqeActTUorLgTbdMRcj44575899 = -499156253;    int OcqeActTUorLgTbdMRcj28818813 = -667416248;    int OcqeActTUorLgTbdMRcj3063608 = -36838105;    int OcqeActTUorLgTbdMRcj93661472 = -867502968;    int OcqeActTUorLgTbdMRcj49350986 = -602250055;    int OcqeActTUorLgTbdMRcj6063254 = -560665233;    int OcqeActTUorLgTbdMRcj35659331 = -329555692;    int OcqeActTUorLgTbdMRcj35267885 = -459037061;    int OcqeActTUorLgTbdMRcj87214241 = -942456852;    int OcqeActTUorLgTbdMRcj85163938 = -5191475;    int OcqeActTUorLgTbdMRcj70231748 = -50636575;    int OcqeActTUorLgTbdMRcj58346098 = -772066787;    int OcqeActTUorLgTbdMRcj68287182 = -716748625;    int OcqeActTUorLgTbdMRcj19568917 = 80869393;    int OcqeActTUorLgTbdMRcj43000244 = -510948230;    int OcqeActTUorLgTbdMRcj73522676 = -162759482;    int OcqeActTUorLgTbdMRcj92223457 = -505411820;    int OcqeActTUorLgTbdMRcj16250626 = -720762527;    int OcqeActTUorLgTbdMRcj27266287 = -133236818;    int OcqeActTUorLgTbdMRcj90167510 = 8496290;    int OcqeActTUorLgTbdMRcj52894441 = -422700498;    int OcqeActTUorLgTbdMRcj64771990 = -151740045;    int OcqeActTUorLgTbdMRcj49531672 = 29029973;    int OcqeActTUorLgTbdMRcj16128281 = -914094613;    int OcqeActTUorLgTbdMRcj99053579 = -768638899;    int OcqeActTUorLgTbdMRcj54846998 = -855473809;    int OcqeActTUorLgTbdMRcj20648973 = -721807463;    int OcqeActTUorLgTbdMRcj47557459 = 47044239;    int OcqeActTUorLgTbdMRcj47952397 = -295098233;    int OcqeActTUorLgTbdMRcj29905005 = -673387033;    int OcqeActTUorLgTbdMRcj56449089 = -131572091;    int OcqeActTUorLgTbdMRcj34947708 = -125131144;    int OcqeActTUorLgTbdMRcj1170808 = 59465629;    int OcqeActTUorLgTbdMRcj93719807 = -543212218;    int OcqeActTUorLgTbdMRcj95781622 = -441026314;    int OcqeActTUorLgTbdMRcj50627279 = -782091658;    int OcqeActTUorLgTbdMRcj23659630 = -760845329;    int OcqeActTUorLgTbdMRcj80770560 = -364371745;     OcqeActTUorLgTbdMRcj69864731 = OcqeActTUorLgTbdMRcj59790723;     OcqeActTUorLgTbdMRcj59790723 = OcqeActTUorLgTbdMRcj10271369;     OcqeActTUorLgTbdMRcj10271369 = OcqeActTUorLgTbdMRcj59052346;     OcqeActTUorLgTbdMRcj59052346 = OcqeActTUorLgTbdMRcj37040562;     OcqeActTUorLgTbdMRcj37040562 = OcqeActTUorLgTbdMRcj11393059;     OcqeActTUorLgTbdMRcj11393059 = OcqeActTUorLgTbdMRcj84388348;     OcqeActTUorLgTbdMRcj84388348 = OcqeActTUorLgTbdMRcj67835976;     OcqeActTUorLgTbdMRcj67835976 = OcqeActTUorLgTbdMRcj54480155;     OcqeActTUorLgTbdMRcj54480155 = OcqeActTUorLgTbdMRcj44366715;     OcqeActTUorLgTbdMRcj44366715 = OcqeActTUorLgTbdMRcj53767690;     OcqeActTUorLgTbdMRcj53767690 = OcqeActTUorLgTbdMRcj77287428;     OcqeActTUorLgTbdMRcj77287428 = OcqeActTUorLgTbdMRcj57628609;     OcqeActTUorLgTbdMRcj57628609 = OcqeActTUorLgTbdMRcj30635185;     OcqeActTUorLgTbdMRcj30635185 = OcqeActTUorLgTbdMRcj42765091;     OcqeActTUorLgTbdMRcj42765091 = OcqeActTUorLgTbdMRcj60735125;     OcqeActTUorLgTbdMRcj60735125 = OcqeActTUorLgTbdMRcj77279375;     OcqeActTUorLgTbdMRcj77279375 = OcqeActTUorLgTbdMRcj84591184;     OcqeActTUorLgTbdMRcj84591184 = OcqeActTUorLgTbdMRcj70794475;     OcqeActTUorLgTbdMRcj70794475 = OcqeActTUorLgTbdMRcj27602043;     OcqeActTUorLgTbdMRcj27602043 = OcqeActTUorLgTbdMRcj59926438;     OcqeActTUorLgTbdMRcj59926438 = OcqeActTUorLgTbdMRcj22446337;     OcqeActTUorLgTbdMRcj22446337 = OcqeActTUorLgTbdMRcj245914;     OcqeActTUorLgTbdMRcj245914 = OcqeActTUorLgTbdMRcj66655490;     OcqeActTUorLgTbdMRcj66655490 = OcqeActTUorLgTbdMRcj23385999;     OcqeActTUorLgTbdMRcj23385999 = OcqeActTUorLgTbdMRcj57628834;     OcqeActTUorLgTbdMRcj57628834 = OcqeActTUorLgTbdMRcj115489;     OcqeActTUorLgTbdMRcj115489 = OcqeActTUorLgTbdMRcj18793296;     OcqeActTUorLgTbdMRcj18793296 = OcqeActTUorLgTbdMRcj23260078;     OcqeActTUorLgTbdMRcj23260078 = OcqeActTUorLgTbdMRcj25661342;     OcqeActTUorLgTbdMRcj25661342 = OcqeActTUorLgTbdMRcj41303108;     OcqeActTUorLgTbdMRcj41303108 = OcqeActTUorLgTbdMRcj60106217;     OcqeActTUorLgTbdMRcj60106217 = OcqeActTUorLgTbdMRcj27936443;     OcqeActTUorLgTbdMRcj27936443 = OcqeActTUorLgTbdMRcj51565356;     OcqeActTUorLgTbdMRcj51565356 = OcqeActTUorLgTbdMRcj94975853;     OcqeActTUorLgTbdMRcj94975853 = OcqeActTUorLgTbdMRcj7497207;     OcqeActTUorLgTbdMRcj7497207 = OcqeActTUorLgTbdMRcj73520883;     OcqeActTUorLgTbdMRcj73520883 = OcqeActTUorLgTbdMRcj92115437;     OcqeActTUorLgTbdMRcj92115437 = OcqeActTUorLgTbdMRcj14359437;     OcqeActTUorLgTbdMRcj14359437 = OcqeActTUorLgTbdMRcj12448378;     OcqeActTUorLgTbdMRcj12448378 = OcqeActTUorLgTbdMRcj59314860;     OcqeActTUorLgTbdMRcj59314860 = OcqeActTUorLgTbdMRcj40357521;     OcqeActTUorLgTbdMRcj40357521 = OcqeActTUorLgTbdMRcj79446092;     OcqeActTUorLgTbdMRcj79446092 = OcqeActTUorLgTbdMRcj26723237;     OcqeActTUorLgTbdMRcj26723237 = OcqeActTUorLgTbdMRcj74432032;     OcqeActTUorLgTbdMRcj74432032 = OcqeActTUorLgTbdMRcj7135373;     OcqeActTUorLgTbdMRcj7135373 = OcqeActTUorLgTbdMRcj30362547;     OcqeActTUorLgTbdMRcj30362547 = OcqeActTUorLgTbdMRcj9947978;     OcqeActTUorLgTbdMRcj9947978 = OcqeActTUorLgTbdMRcj65898854;     OcqeActTUorLgTbdMRcj65898854 = OcqeActTUorLgTbdMRcj58488088;     OcqeActTUorLgTbdMRcj58488088 = OcqeActTUorLgTbdMRcj76129670;     OcqeActTUorLgTbdMRcj76129670 = OcqeActTUorLgTbdMRcj25174827;     OcqeActTUorLgTbdMRcj25174827 = OcqeActTUorLgTbdMRcj61052638;     OcqeActTUorLgTbdMRcj61052638 = OcqeActTUorLgTbdMRcj73089444;     OcqeActTUorLgTbdMRcj73089444 = OcqeActTUorLgTbdMRcj30916383;     OcqeActTUorLgTbdMRcj30916383 = OcqeActTUorLgTbdMRcj47418394;     OcqeActTUorLgTbdMRcj47418394 = OcqeActTUorLgTbdMRcj59544810;     OcqeActTUorLgTbdMRcj59544810 = OcqeActTUorLgTbdMRcj43615879;     OcqeActTUorLgTbdMRcj43615879 = OcqeActTUorLgTbdMRcj35666348;     OcqeActTUorLgTbdMRcj35666348 = OcqeActTUorLgTbdMRcj79411728;     OcqeActTUorLgTbdMRcj79411728 = OcqeActTUorLgTbdMRcj11277570;     OcqeActTUorLgTbdMRcj11277570 = OcqeActTUorLgTbdMRcj65595053;     OcqeActTUorLgTbdMRcj65595053 = OcqeActTUorLgTbdMRcj44575899;     OcqeActTUorLgTbdMRcj44575899 = OcqeActTUorLgTbdMRcj28818813;     OcqeActTUorLgTbdMRcj28818813 = OcqeActTUorLgTbdMRcj3063608;     OcqeActTUorLgTbdMRcj3063608 = OcqeActTUorLgTbdMRcj93661472;     OcqeActTUorLgTbdMRcj93661472 = OcqeActTUorLgTbdMRcj49350986;     OcqeActTUorLgTbdMRcj49350986 = OcqeActTUorLgTbdMRcj6063254;     OcqeActTUorLgTbdMRcj6063254 = OcqeActTUorLgTbdMRcj35659331;     OcqeActTUorLgTbdMRcj35659331 = OcqeActTUorLgTbdMRcj35267885;     OcqeActTUorLgTbdMRcj35267885 = OcqeActTUorLgTbdMRcj87214241;     OcqeActTUorLgTbdMRcj87214241 = OcqeActTUorLgTbdMRcj85163938;     OcqeActTUorLgTbdMRcj85163938 = OcqeActTUorLgTbdMRcj70231748;     OcqeActTUorLgTbdMRcj70231748 = OcqeActTUorLgTbdMRcj58346098;     OcqeActTUorLgTbdMRcj58346098 = OcqeActTUorLgTbdMRcj68287182;     OcqeActTUorLgTbdMRcj68287182 = OcqeActTUorLgTbdMRcj19568917;     OcqeActTUorLgTbdMRcj19568917 = OcqeActTUorLgTbdMRcj43000244;     OcqeActTUorLgTbdMRcj43000244 = OcqeActTUorLgTbdMRcj73522676;     OcqeActTUorLgTbdMRcj73522676 = OcqeActTUorLgTbdMRcj92223457;     OcqeActTUorLgTbdMRcj92223457 = OcqeActTUorLgTbdMRcj16250626;     OcqeActTUorLgTbdMRcj16250626 = OcqeActTUorLgTbdMRcj27266287;     OcqeActTUorLgTbdMRcj27266287 = OcqeActTUorLgTbdMRcj90167510;     OcqeActTUorLgTbdMRcj90167510 = OcqeActTUorLgTbdMRcj52894441;     OcqeActTUorLgTbdMRcj52894441 = OcqeActTUorLgTbdMRcj64771990;     OcqeActTUorLgTbdMRcj64771990 = OcqeActTUorLgTbdMRcj49531672;     OcqeActTUorLgTbdMRcj49531672 = OcqeActTUorLgTbdMRcj16128281;     OcqeActTUorLgTbdMRcj16128281 = OcqeActTUorLgTbdMRcj99053579;     OcqeActTUorLgTbdMRcj99053579 = OcqeActTUorLgTbdMRcj54846998;     OcqeActTUorLgTbdMRcj54846998 = OcqeActTUorLgTbdMRcj20648973;     OcqeActTUorLgTbdMRcj20648973 = OcqeActTUorLgTbdMRcj47557459;     OcqeActTUorLgTbdMRcj47557459 = OcqeActTUorLgTbdMRcj47952397;     OcqeActTUorLgTbdMRcj47952397 = OcqeActTUorLgTbdMRcj29905005;     OcqeActTUorLgTbdMRcj29905005 = OcqeActTUorLgTbdMRcj56449089;     OcqeActTUorLgTbdMRcj56449089 = OcqeActTUorLgTbdMRcj34947708;     OcqeActTUorLgTbdMRcj34947708 = OcqeActTUorLgTbdMRcj1170808;     OcqeActTUorLgTbdMRcj1170808 = OcqeActTUorLgTbdMRcj93719807;     OcqeActTUorLgTbdMRcj93719807 = OcqeActTUorLgTbdMRcj95781622;     OcqeActTUorLgTbdMRcj95781622 = OcqeActTUorLgTbdMRcj50627279;     OcqeActTUorLgTbdMRcj50627279 = OcqeActTUorLgTbdMRcj23659630;     OcqeActTUorLgTbdMRcj23659630 = OcqeActTUorLgTbdMRcj80770560;     OcqeActTUorLgTbdMRcj80770560 = OcqeActTUorLgTbdMRcj69864731;}
// Junk Finished

// Junk Code By Peatreat & Thaisen's Gen
void OjpEneWzCiqCFlTZYTCY14942607() {     int AsNajqpkDPqUqaDLlmyR72783678 = -685644496;    int AsNajqpkDPqUqaDLlmyR91114359 = -135372824;    int AsNajqpkDPqUqaDLlmyR22203655 = -770850112;    int AsNajqpkDPqUqaDLlmyR55411886 = -852892610;    int AsNajqpkDPqUqaDLlmyR34768300 = -180772271;    int AsNajqpkDPqUqaDLlmyR684102 = 30499438;    int AsNajqpkDPqUqaDLlmyR31712209 = -874307207;    int AsNajqpkDPqUqaDLlmyR78219602 = -247016740;    int AsNajqpkDPqUqaDLlmyR90956488 = -103179779;    int AsNajqpkDPqUqaDLlmyR58201576 = -612333156;    int AsNajqpkDPqUqaDLlmyR37086286 = -515034452;    int AsNajqpkDPqUqaDLlmyR29695764 = -305199123;    int AsNajqpkDPqUqaDLlmyR34265130 = -629043407;    int AsNajqpkDPqUqaDLlmyR64833896 = -771850093;    int AsNajqpkDPqUqaDLlmyR18973907 = -855794065;    int AsNajqpkDPqUqaDLlmyR38775388 = -686264960;    int AsNajqpkDPqUqaDLlmyR21140614 = -965798416;    int AsNajqpkDPqUqaDLlmyR66504849 = -823144974;    int AsNajqpkDPqUqaDLlmyR12441228 = -985738810;    int AsNajqpkDPqUqaDLlmyR25507024 = -678809404;    int AsNajqpkDPqUqaDLlmyR48353473 = -444588156;    int AsNajqpkDPqUqaDLlmyR71279472 = 46580765;    int AsNajqpkDPqUqaDLlmyR35366231 = 61051111;    int AsNajqpkDPqUqaDLlmyR75433608 = -958328281;    int AsNajqpkDPqUqaDLlmyR47722756 = -466788645;    int AsNajqpkDPqUqaDLlmyR41164367 = -27697407;    int AsNajqpkDPqUqaDLlmyR63276778 = -776572226;    int AsNajqpkDPqUqaDLlmyR73913071 = -236596740;    int AsNajqpkDPqUqaDLlmyR88407681 = -701417782;    int AsNajqpkDPqUqaDLlmyR50111667 = -8616044;    int AsNajqpkDPqUqaDLlmyR55789828 = -954329824;    int AsNajqpkDPqUqaDLlmyR78068097 = -71619720;    int AsNajqpkDPqUqaDLlmyR97499009 = -425642209;    int AsNajqpkDPqUqaDLlmyR52197536 = -893541775;    int AsNajqpkDPqUqaDLlmyR31274882 = -357924987;    int AsNajqpkDPqUqaDLlmyR13124139 = -979189851;    int AsNajqpkDPqUqaDLlmyR98577145 = -31404376;    int AsNajqpkDPqUqaDLlmyR4259064 = -545518460;    int AsNajqpkDPqUqaDLlmyR95354242 = 91848366;    int AsNajqpkDPqUqaDLlmyR11352357 = -495467591;    int AsNajqpkDPqUqaDLlmyR46590174 = -320354153;    int AsNajqpkDPqUqaDLlmyR17006780 = -559195032;    int AsNajqpkDPqUqaDLlmyR13863044 = -389236692;    int AsNajqpkDPqUqaDLlmyR42781817 = -445974421;    int AsNajqpkDPqUqaDLlmyR62386888 = -57488111;    int AsNajqpkDPqUqaDLlmyR60774527 = 36335223;    int AsNajqpkDPqUqaDLlmyR60670065 = -801078514;    int AsNajqpkDPqUqaDLlmyR54390822 = -165880632;    int AsNajqpkDPqUqaDLlmyR19223168 = -24952527;    int AsNajqpkDPqUqaDLlmyR14673492 = -500339678;    int AsNajqpkDPqUqaDLlmyR10563318 = -57500190;    int AsNajqpkDPqUqaDLlmyR82577224 = -877711574;    int AsNajqpkDPqUqaDLlmyR69541895 = -211060754;    int AsNajqpkDPqUqaDLlmyR60445961 = -877119844;    int AsNajqpkDPqUqaDLlmyR66589134 = -418356149;    int AsNajqpkDPqUqaDLlmyR1504206 = -632225262;    int AsNajqpkDPqUqaDLlmyR55748129 = -96423935;    int AsNajqpkDPqUqaDLlmyR46770047 = -812521831;    int AsNajqpkDPqUqaDLlmyR7689130 = -286103966;    int AsNajqpkDPqUqaDLlmyR93603932 = -53074864;    int AsNajqpkDPqUqaDLlmyR37407323 = -192928337;    int AsNajqpkDPqUqaDLlmyR57799138 = -537710467;    int AsNajqpkDPqUqaDLlmyR89811920 = -545598958;    int AsNajqpkDPqUqaDLlmyR40844822 = 5436264;    int AsNajqpkDPqUqaDLlmyR2411749 = -658003333;    int AsNajqpkDPqUqaDLlmyR59018188 = -343414732;    int AsNajqpkDPqUqaDLlmyR32196754 = -879556915;    int AsNajqpkDPqUqaDLlmyR82067594 = -735501632;    int AsNajqpkDPqUqaDLlmyR33559014 = -313925106;    int AsNajqpkDPqUqaDLlmyR5849769 = -876604214;    int AsNajqpkDPqUqaDLlmyR40198243 = -554860584;    int AsNajqpkDPqUqaDLlmyR16881550 = -320279957;    int AsNajqpkDPqUqaDLlmyR71150606 = -814993341;    int AsNajqpkDPqUqaDLlmyR1088872 = -390271219;    int AsNajqpkDPqUqaDLlmyR78916850 = -258455252;    int AsNajqpkDPqUqaDLlmyR31346694 = -885393124;    int AsNajqpkDPqUqaDLlmyR57416429 = -564182544;    int AsNajqpkDPqUqaDLlmyR92584413 = -492974468;    int AsNajqpkDPqUqaDLlmyR13046720 = -800840170;    int AsNajqpkDPqUqaDLlmyR86948228 = -403123868;    int AsNajqpkDPqUqaDLlmyR80494302 = -226618894;    int AsNajqpkDPqUqaDLlmyR8885956 = -510691595;    int AsNajqpkDPqUqaDLlmyR54689903 = -111644213;    int AsNajqpkDPqUqaDLlmyR73734190 = -101078104;    int AsNajqpkDPqUqaDLlmyR39548350 = -951115854;    int AsNajqpkDPqUqaDLlmyR73212604 = 23381750;    int AsNajqpkDPqUqaDLlmyR8526203 = -860558966;    int AsNajqpkDPqUqaDLlmyR37053048 = -548522365;    int AsNajqpkDPqUqaDLlmyR85608402 = -375185627;    int AsNajqpkDPqUqaDLlmyR29770676 = -725699726;    int AsNajqpkDPqUqaDLlmyR57376009 = -782765917;    int AsNajqpkDPqUqaDLlmyR51807098 = -218882545;    int AsNajqpkDPqUqaDLlmyR96569934 = -159414494;    int AsNajqpkDPqUqaDLlmyR1750310 = -855076770;    int AsNajqpkDPqUqaDLlmyR73945033 = -202539255;    int AsNajqpkDPqUqaDLlmyR88791035 = -782643686;    int AsNajqpkDPqUqaDLlmyR27194860 = 86403926;    int AsNajqpkDPqUqaDLlmyR73018221 = -294672956;    int AsNajqpkDPqUqaDLlmyR40370068 = -787971089;    int AsNajqpkDPqUqaDLlmyR3368700 = -685644496;     AsNajqpkDPqUqaDLlmyR72783678 = AsNajqpkDPqUqaDLlmyR91114359;     AsNajqpkDPqUqaDLlmyR91114359 = AsNajqpkDPqUqaDLlmyR22203655;     AsNajqpkDPqUqaDLlmyR22203655 = AsNajqpkDPqUqaDLlmyR55411886;     AsNajqpkDPqUqaDLlmyR55411886 = AsNajqpkDPqUqaDLlmyR34768300;     AsNajqpkDPqUqaDLlmyR34768300 = AsNajqpkDPqUqaDLlmyR684102;     AsNajqpkDPqUqaDLlmyR684102 = AsNajqpkDPqUqaDLlmyR31712209;     AsNajqpkDPqUqaDLlmyR31712209 = AsNajqpkDPqUqaDLlmyR78219602;     AsNajqpkDPqUqaDLlmyR78219602 = AsNajqpkDPqUqaDLlmyR90956488;     AsNajqpkDPqUqaDLlmyR90956488 = AsNajqpkDPqUqaDLlmyR58201576;     AsNajqpkDPqUqaDLlmyR58201576 = AsNajqpkDPqUqaDLlmyR37086286;     AsNajqpkDPqUqaDLlmyR37086286 = AsNajqpkDPqUqaDLlmyR29695764;     AsNajqpkDPqUqaDLlmyR29695764 = AsNajqpkDPqUqaDLlmyR34265130;     AsNajqpkDPqUqaDLlmyR34265130 = AsNajqpkDPqUqaDLlmyR64833896;     AsNajqpkDPqUqaDLlmyR64833896 = AsNajqpkDPqUqaDLlmyR18973907;     AsNajqpkDPqUqaDLlmyR18973907 = AsNajqpkDPqUqaDLlmyR38775388;     AsNajqpkDPqUqaDLlmyR38775388 = AsNajqpkDPqUqaDLlmyR21140614;     AsNajqpkDPqUqaDLlmyR21140614 = AsNajqpkDPqUqaDLlmyR66504849;     AsNajqpkDPqUqaDLlmyR66504849 = AsNajqpkDPqUqaDLlmyR12441228;     AsNajqpkDPqUqaDLlmyR12441228 = AsNajqpkDPqUqaDLlmyR25507024;     AsNajqpkDPqUqaDLlmyR25507024 = AsNajqpkDPqUqaDLlmyR48353473;     AsNajqpkDPqUqaDLlmyR48353473 = AsNajqpkDPqUqaDLlmyR71279472;     AsNajqpkDPqUqaDLlmyR71279472 = AsNajqpkDPqUqaDLlmyR35366231;     AsNajqpkDPqUqaDLlmyR35366231 = AsNajqpkDPqUqaDLlmyR75433608;     AsNajqpkDPqUqaDLlmyR75433608 = AsNajqpkDPqUqaDLlmyR47722756;     AsNajqpkDPqUqaDLlmyR47722756 = AsNajqpkDPqUqaDLlmyR41164367;     AsNajqpkDPqUqaDLlmyR41164367 = AsNajqpkDPqUqaDLlmyR63276778;     AsNajqpkDPqUqaDLlmyR63276778 = AsNajqpkDPqUqaDLlmyR73913071;     AsNajqpkDPqUqaDLlmyR73913071 = AsNajqpkDPqUqaDLlmyR88407681;     AsNajqpkDPqUqaDLlmyR88407681 = AsNajqpkDPqUqaDLlmyR50111667;     AsNajqpkDPqUqaDLlmyR50111667 = AsNajqpkDPqUqaDLlmyR55789828;     AsNajqpkDPqUqaDLlmyR55789828 = AsNajqpkDPqUqaDLlmyR78068097;     AsNajqpkDPqUqaDLlmyR78068097 = AsNajqpkDPqUqaDLlmyR97499009;     AsNajqpkDPqUqaDLlmyR97499009 = AsNajqpkDPqUqaDLlmyR52197536;     AsNajqpkDPqUqaDLlmyR52197536 = AsNajqpkDPqUqaDLlmyR31274882;     AsNajqpkDPqUqaDLlmyR31274882 = AsNajqpkDPqUqaDLlmyR13124139;     AsNajqpkDPqUqaDLlmyR13124139 = AsNajqpkDPqUqaDLlmyR98577145;     AsNajqpkDPqUqaDLlmyR98577145 = AsNajqpkDPqUqaDLlmyR4259064;     AsNajqpkDPqUqaDLlmyR4259064 = AsNajqpkDPqUqaDLlmyR95354242;     AsNajqpkDPqUqaDLlmyR95354242 = AsNajqpkDPqUqaDLlmyR11352357;     AsNajqpkDPqUqaDLlmyR11352357 = AsNajqpkDPqUqaDLlmyR46590174;     AsNajqpkDPqUqaDLlmyR46590174 = AsNajqpkDPqUqaDLlmyR17006780;     AsNajqpkDPqUqaDLlmyR17006780 = AsNajqpkDPqUqaDLlmyR13863044;     AsNajqpkDPqUqaDLlmyR13863044 = AsNajqpkDPqUqaDLlmyR42781817;     AsNajqpkDPqUqaDLlmyR42781817 = AsNajqpkDPqUqaDLlmyR62386888;     AsNajqpkDPqUqaDLlmyR62386888 = AsNajqpkDPqUqaDLlmyR60774527;     AsNajqpkDPqUqaDLlmyR60774527 = AsNajqpkDPqUqaDLlmyR60670065;     AsNajqpkDPqUqaDLlmyR60670065 = AsNajqpkDPqUqaDLlmyR54390822;     AsNajqpkDPqUqaDLlmyR54390822 = AsNajqpkDPqUqaDLlmyR19223168;     AsNajqpkDPqUqaDLlmyR19223168 = AsNajqpkDPqUqaDLlmyR14673492;     AsNajqpkDPqUqaDLlmyR14673492 = AsNajqpkDPqUqaDLlmyR10563318;     AsNajqpkDPqUqaDLlmyR10563318 = AsNajqpkDPqUqaDLlmyR82577224;     AsNajqpkDPqUqaDLlmyR82577224 = AsNajqpkDPqUqaDLlmyR69541895;     AsNajqpkDPqUqaDLlmyR69541895 = AsNajqpkDPqUqaDLlmyR60445961;     AsNajqpkDPqUqaDLlmyR60445961 = AsNajqpkDPqUqaDLlmyR66589134;     AsNajqpkDPqUqaDLlmyR66589134 = AsNajqpkDPqUqaDLlmyR1504206;     AsNajqpkDPqUqaDLlmyR1504206 = AsNajqpkDPqUqaDLlmyR55748129;     AsNajqpkDPqUqaDLlmyR55748129 = AsNajqpkDPqUqaDLlmyR46770047;     AsNajqpkDPqUqaDLlmyR46770047 = AsNajqpkDPqUqaDLlmyR7689130;     AsNajqpkDPqUqaDLlmyR7689130 = AsNajqpkDPqUqaDLlmyR93603932;     AsNajqpkDPqUqaDLlmyR93603932 = AsNajqpkDPqUqaDLlmyR37407323;     AsNajqpkDPqUqaDLlmyR37407323 = AsNajqpkDPqUqaDLlmyR57799138;     AsNajqpkDPqUqaDLlmyR57799138 = AsNajqpkDPqUqaDLlmyR89811920;     AsNajqpkDPqUqaDLlmyR89811920 = AsNajqpkDPqUqaDLlmyR40844822;     AsNajqpkDPqUqaDLlmyR40844822 = AsNajqpkDPqUqaDLlmyR2411749;     AsNajqpkDPqUqaDLlmyR2411749 = AsNajqpkDPqUqaDLlmyR59018188;     AsNajqpkDPqUqaDLlmyR59018188 = AsNajqpkDPqUqaDLlmyR32196754;     AsNajqpkDPqUqaDLlmyR32196754 = AsNajqpkDPqUqaDLlmyR82067594;     AsNajqpkDPqUqaDLlmyR82067594 = AsNajqpkDPqUqaDLlmyR33559014;     AsNajqpkDPqUqaDLlmyR33559014 = AsNajqpkDPqUqaDLlmyR5849769;     AsNajqpkDPqUqaDLlmyR5849769 = AsNajqpkDPqUqaDLlmyR40198243;     AsNajqpkDPqUqaDLlmyR40198243 = AsNajqpkDPqUqaDLlmyR16881550;     AsNajqpkDPqUqaDLlmyR16881550 = AsNajqpkDPqUqaDLlmyR71150606;     AsNajqpkDPqUqaDLlmyR71150606 = AsNajqpkDPqUqaDLlmyR1088872;     AsNajqpkDPqUqaDLlmyR1088872 = AsNajqpkDPqUqaDLlmyR78916850;     AsNajqpkDPqUqaDLlmyR78916850 = AsNajqpkDPqUqaDLlmyR31346694;     AsNajqpkDPqUqaDLlmyR31346694 = AsNajqpkDPqUqaDLlmyR57416429;     AsNajqpkDPqUqaDLlmyR57416429 = AsNajqpkDPqUqaDLlmyR92584413;     AsNajqpkDPqUqaDLlmyR92584413 = AsNajqpkDPqUqaDLlmyR13046720;     AsNajqpkDPqUqaDLlmyR13046720 = AsNajqpkDPqUqaDLlmyR86948228;     AsNajqpkDPqUqaDLlmyR86948228 = AsNajqpkDPqUqaDLlmyR80494302;     AsNajqpkDPqUqaDLlmyR80494302 = AsNajqpkDPqUqaDLlmyR8885956;     AsNajqpkDPqUqaDLlmyR8885956 = AsNajqpkDPqUqaDLlmyR54689903;     AsNajqpkDPqUqaDLlmyR54689903 = AsNajqpkDPqUqaDLlmyR73734190;     AsNajqpkDPqUqaDLlmyR73734190 = AsNajqpkDPqUqaDLlmyR39548350;     AsNajqpkDPqUqaDLlmyR39548350 = AsNajqpkDPqUqaDLlmyR73212604;     AsNajqpkDPqUqaDLlmyR73212604 = AsNajqpkDPqUqaDLlmyR8526203;     AsNajqpkDPqUqaDLlmyR8526203 = AsNajqpkDPqUqaDLlmyR37053048;     AsNajqpkDPqUqaDLlmyR37053048 = AsNajqpkDPqUqaDLlmyR85608402;     AsNajqpkDPqUqaDLlmyR85608402 = AsNajqpkDPqUqaDLlmyR29770676;     AsNajqpkDPqUqaDLlmyR29770676 = AsNajqpkDPqUqaDLlmyR57376009;     AsNajqpkDPqUqaDLlmyR57376009 = AsNajqpkDPqUqaDLlmyR51807098;     AsNajqpkDPqUqaDLlmyR51807098 = AsNajqpkDPqUqaDLlmyR96569934;     AsNajqpkDPqUqaDLlmyR96569934 = AsNajqpkDPqUqaDLlmyR1750310;     AsNajqpkDPqUqaDLlmyR1750310 = AsNajqpkDPqUqaDLlmyR73945033;     AsNajqpkDPqUqaDLlmyR73945033 = AsNajqpkDPqUqaDLlmyR88791035;     AsNajqpkDPqUqaDLlmyR88791035 = AsNajqpkDPqUqaDLlmyR27194860;     AsNajqpkDPqUqaDLlmyR27194860 = AsNajqpkDPqUqaDLlmyR73018221;     AsNajqpkDPqUqaDLlmyR73018221 = AsNajqpkDPqUqaDLlmyR40370068;     AsNajqpkDPqUqaDLlmyR40370068 = AsNajqpkDPqUqaDLlmyR3368700;     AsNajqpkDPqUqaDLlmyR3368700 = AsNajqpkDPqUqaDLlmyR72783678;}
// Junk Finished

// Junk Code By Peatreat & Thaisen's Gen
void MDWBPIPonllzkoTNHGtg91284924() {     int IBYsxHSlDwPONgRLvclA5044487 = -261292259;    int IBYsxHSlDwPONgRLvclA92336795 = -808989708;    int IBYsxHSlDwPONgRLvclA55480235 = -873709495;    int IBYsxHSlDwPONgRLvclA96919999 = -673128319;    int IBYsxHSlDwPONgRLvclA19885819 = -830505830;    int IBYsxHSlDwPONgRLvclA34780814 = 62524168;    int IBYsxHSlDwPONgRLvclA29204510 = 41250960;    int IBYsxHSlDwPONgRLvclA974898 = 18834355;    int IBYsxHSlDwPONgRLvclA64307932 = 20516819;    int IBYsxHSlDwPONgRLvclA45709491 = -785199963;    int IBYsxHSlDwPONgRLvclA98476326 = -3238696;    int IBYsxHSlDwPONgRLvclA50849205 = 23643718;    int IBYsxHSlDwPONgRLvclA62927178 = -935524866;    int IBYsxHSlDwPONgRLvclA69029795 = -201397966;    int IBYsxHSlDwPONgRLvclA57634812 = 1849165;    int IBYsxHSlDwPONgRLvclA58851558 = -267552632;    int IBYsxHSlDwPONgRLvclA27001785 = -981732995;    int IBYsxHSlDwPONgRLvclA51125553 = -832038373;    int IBYsxHSlDwPONgRLvclA92808467 = -654430499;    int IBYsxHSlDwPONgRLvclA36699059 = -283565548;    int IBYsxHSlDwPONgRLvclA61314178 = -731650372;    int IBYsxHSlDwPONgRLvclA37013109 = -125088913;    int IBYsxHSlDwPONgRLvclA5421136 = -808569560;    int IBYsxHSlDwPONgRLvclA94012298 = 25703237;    int IBYsxHSlDwPONgRLvclA36377815 = -544836825;    int IBYsxHSlDwPONgRLvclA10676397 = -595717896;    int IBYsxHSlDwPONgRLvclA88398863 = -774629724;    int IBYsxHSlDwPONgRLvclA56654146 = 34838865;    int IBYsxHSlDwPONgRLvclA34091245 = -283475853;    int IBYsxHSlDwPONgRLvclA11479767 = -54509519;    int IBYsxHSlDwPONgRLvclA96617230 = -708979432;    int IBYsxHSlDwPONgRLvclA36242374 = -222866655;    int IBYsxHSlDwPONgRLvclA69568639 = -654760544;    int IBYsxHSlDwPONgRLvclA87747578 = -126217467;    int IBYsxHSlDwPONgRLvclA71907712 = -771345749;    int IBYsxHSlDwPONgRLvclA1586001 = -991555818;    int IBYsxHSlDwPONgRLvclA91783288 = -248536423;    int IBYsxHSlDwPONgRLvclA33527881 = -547084688;    int IBYsxHSlDwPONgRLvclA33129357 = -601735620;    int IBYsxHSlDwPONgRLvclA87025929 = -726983847;    int IBYsxHSlDwPONgRLvclA61647871 = -17670543;    int IBYsxHSlDwPONgRLvclA50870708 = -502610510;    int IBYsxHSlDwPONgRLvclA59596394 = -901341680;    int IBYsxHSlDwPONgRLvclA11563996 = -579254882;    int IBYsxHSlDwPONgRLvclA38405737 = -854121633;    int IBYsxHSlDwPONgRLvclA48435815 = -762269967;    int IBYsxHSlDwPONgRLvclA92544171 = -731006267;    int IBYsxHSlDwPONgRLvclA8603038 = -995759369;    int IBYsxHSlDwPONgRLvclA85764419 = -638729879;    int IBYsxHSlDwPONgRLvclA82476831 = -185363600;    int IBYsxHSlDwPONgRLvclA90067010 = -38194423;    int IBYsxHSlDwPONgRLvclA12655374 = -244104024;    int IBYsxHSlDwPONgRLvclA98403842 = -171118990;    int IBYsxHSlDwPONgRLvclA67825444 = 70754629;    int IBYsxHSlDwPONgRLvclA29970747 = -430459993;    int IBYsxHSlDwPONgRLvclA68031378 = -36203346;    int IBYsxHSlDwPONgRLvclA86915659 = 99579851;    int IBYsxHSlDwPONgRLvclA61467937 = -799412732;    int IBYsxHSlDwPONgRLvclA60542184 = -28291495;    int IBYsxHSlDwPONgRLvclA9209422 = -134787935;    int IBYsxHSlDwPONgRLvclA46381950 = -162846108;    int IBYsxHSlDwPONgRLvclA72550363 = -993587905;    int IBYsxHSlDwPONgRLvclA66883652 = -697689793;    int IBYsxHSlDwPONgRLvclA52828166 = -924973662;    int IBYsxHSlDwPONgRLvclA49092261 = 23779468;    int IBYsxHSlDwPONgRLvclA62233952 = -780372041;    int IBYsxHSlDwPONgRLvclA81280566 = -321595738;    int IBYsxHSlDwPONgRLvclA75179599 = -709307399;    int IBYsxHSlDwPONgRLvclA97122082 = -430052217;    int IBYsxHSlDwPONgRLvclA56048812 = -6595018;    int IBYsxHSlDwPONgRLvclA67068270 = 80983791;    int IBYsxHSlDwPONgRLvclA93473903 = -334648307;    int IBYsxHSlDwPONgRLvclA17996196 = -130302753;    int IBYsxHSlDwPONgRLvclA5782538 = -927446652;    int IBYsxHSlDwPONgRLvclA75051188 = -165895006;    int IBYsxHSlDwPONgRLvclA10443471 = -129039862;    int IBYsxHSlDwPONgRLvclA77416714 = -223747234;    int IBYsxHSlDwPONgRLvclA93857139 = -129314679;    int IBYsxHSlDwPONgRLvclA55606561 = -120175130;    int IBYsxHSlDwPONgRLvclA87941999 = -782566858;    int IBYsxHSlDwPONgRLvclA18132225 = -864711630;    int IBYsxHSlDwPONgRLvclA79795825 = -778870356;    int IBYsxHSlDwPONgRLvclA70889727 = -326431257;    int IBYsxHSlDwPONgRLvclA51614414 = 1887746;    int IBYsxHSlDwPONgRLvclA21412757 = 83684903;    int IBYsxHSlDwPONgRLvclA83961856 = -364875409;    int IBYsxHSlDwPONgRLvclA37838532 = 48252334;    int IBYsxHSlDwPONgRLvclA1743195 = -625515173;    int IBYsxHSlDwPONgRLvclA57776832 = -695757475;    int IBYsxHSlDwPONgRLvclA3876334 = -635142403;    int IBYsxHSlDwPONgRLvclA14670341 = -991135669;    int IBYsxHSlDwPONgRLvclA30315352 = -449123691;    int IBYsxHSlDwPONgRLvclA72985697 = -418793193;    int IBYsxHSlDwPONgRLvclA23919935 = -366947686;    int IBYsxHSlDwPONgRLvclA40643979 = -464137740;    int IBYsxHSlDwPONgRLvclA89097507 = -24082639;    int IBYsxHSlDwPONgRLvclA83987055 = -804920718;    int IBYsxHSlDwPONgRLvclA6768229 = -976368018;    int IBYsxHSlDwPONgRLvclA62471735 = -503034350;    int IBYsxHSlDwPONgRLvclA76171785 = -261292259;     IBYsxHSlDwPONgRLvclA5044487 = IBYsxHSlDwPONgRLvclA92336795;     IBYsxHSlDwPONgRLvclA92336795 = IBYsxHSlDwPONgRLvclA55480235;     IBYsxHSlDwPONgRLvclA55480235 = IBYsxHSlDwPONgRLvclA96919999;     IBYsxHSlDwPONgRLvclA96919999 = IBYsxHSlDwPONgRLvclA19885819;     IBYsxHSlDwPONgRLvclA19885819 = IBYsxHSlDwPONgRLvclA34780814;     IBYsxHSlDwPONgRLvclA34780814 = IBYsxHSlDwPONgRLvclA29204510;     IBYsxHSlDwPONgRLvclA29204510 = IBYsxHSlDwPONgRLvclA974898;     IBYsxHSlDwPONgRLvclA974898 = IBYsxHSlDwPONgRLvclA64307932;     IBYsxHSlDwPONgRLvclA64307932 = IBYsxHSlDwPONgRLvclA45709491;     IBYsxHSlDwPONgRLvclA45709491 = IBYsxHSlDwPONgRLvclA98476326;     IBYsxHSlDwPONgRLvclA98476326 = IBYsxHSlDwPONgRLvclA50849205;     IBYsxHSlDwPONgRLvclA50849205 = IBYsxHSlDwPONgRLvclA62927178;     IBYsxHSlDwPONgRLvclA62927178 = IBYsxHSlDwPONgRLvclA69029795;     IBYsxHSlDwPONgRLvclA69029795 = IBYsxHSlDwPONgRLvclA57634812;     IBYsxHSlDwPONgRLvclA57634812 = IBYsxHSlDwPONgRLvclA58851558;     IBYsxHSlDwPONgRLvclA58851558 = IBYsxHSlDwPONgRLvclA27001785;     IBYsxHSlDwPONgRLvclA27001785 = IBYsxHSlDwPONgRLvclA51125553;     IBYsxHSlDwPONgRLvclA51125553 = IBYsxHSlDwPONgRLvclA92808467;     IBYsxHSlDwPONgRLvclA92808467 = IBYsxHSlDwPONgRLvclA36699059;     IBYsxHSlDwPONgRLvclA36699059 = IBYsxHSlDwPONgRLvclA61314178;     IBYsxHSlDwPONgRLvclA61314178 = IBYsxHSlDwPONgRLvclA37013109;     IBYsxHSlDwPONgRLvclA37013109 = IBYsxHSlDwPONgRLvclA5421136;     IBYsxHSlDwPONgRLvclA5421136 = IBYsxHSlDwPONgRLvclA94012298;     IBYsxHSlDwPONgRLvclA94012298 = IBYsxHSlDwPONgRLvclA36377815;     IBYsxHSlDwPONgRLvclA36377815 = IBYsxHSlDwPONgRLvclA10676397;     IBYsxHSlDwPONgRLvclA10676397 = IBYsxHSlDwPONgRLvclA88398863;     IBYsxHSlDwPONgRLvclA88398863 = IBYsxHSlDwPONgRLvclA56654146;     IBYsxHSlDwPONgRLvclA56654146 = IBYsxHSlDwPONgRLvclA34091245;     IBYsxHSlDwPONgRLvclA34091245 = IBYsxHSlDwPONgRLvclA11479767;     IBYsxHSlDwPONgRLvclA11479767 = IBYsxHSlDwPONgRLvclA96617230;     IBYsxHSlDwPONgRLvclA96617230 = IBYsxHSlDwPONgRLvclA36242374;     IBYsxHSlDwPONgRLvclA36242374 = IBYsxHSlDwPONgRLvclA69568639;     IBYsxHSlDwPONgRLvclA69568639 = IBYsxHSlDwPONgRLvclA87747578;     IBYsxHSlDwPONgRLvclA87747578 = IBYsxHSlDwPONgRLvclA71907712;     IBYsxHSlDwPONgRLvclA71907712 = IBYsxHSlDwPONgRLvclA1586001;     IBYsxHSlDwPONgRLvclA1586001 = IBYsxHSlDwPONgRLvclA91783288;     IBYsxHSlDwPONgRLvclA91783288 = IBYsxHSlDwPONgRLvclA33527881;     IBYsxHSlDwPONgRLvclA33527881 = IBYsxHSlDwPONgRLvclA33129357;     IBYsxHSlDwPONgRLvclA33129357 = IBYsxHSlDwPONgRLvclA87025929;     IBYsxHSlDwPONgRLvclA87025929 = IBYsxHSlDwPONgRLvclA61647871;     IBYsxHSlDwPONgRLvclA61647871 = IBYsxHSlDwPONgRLvclA50870708;     IBYsxHSlDwPONgRLvclA50870708 = IBYsxHSlDwPONgRLvclA59596394;     IBYsxHSlDwPONgRLvclA59596394 = IBYsxHSlDwPONgRLvclA11563996;     IBYsxHSlDwPONgRLvclA11563996 = IBYsxHSlDwPONgRLvclA38405737;     IBYsxHSlDwPONgRLvclA38405737 = IBYsxHSlDwPONgRLvclA48435815;     IBYsxHSlDwPONgRLvclA48435815 = IBYsxHSlDwPONgRLvclA92544171;     IBYsxHSlDwPONgRLvclA92544171 = IBYsxHSlDwPONgRLvclA8603038;     IBYsxHSlDwPONgRLvclA8603038 = IBYsxHSlDwPONgRLvclA85764419;     IBYsxHSlDwPONgRLvclA85764419 = IBYsxHSlDwPONgRLvclA82476831;     IBYsxHSlDwPONgRLvclA82476831 = IBYsxHSlDwPONgRLvclA90067010;     IBYsxHSlDwPONgRLvclA90067010 = IBYsxHSlDwPONgRLvclA12655374;     IBYsxHSlDwPONgRLvclA12655374 = IBYsxHSlDwPONgRLvclA98403842;     IBYsxHSlDwPONgRLvclA98403842 = IBYsxHSlDwPONgRLvclA67825444;     IBYsxHSlDwPONgRLvclA67825444 = IBYsxHSlDwPONgRLvclA29970747;     IBYsxHSlDwPONgRLvclA29970747 = IBYsxHSlDwPONgRLvclA68031378;     IBYsxHSlDwPONgRLvclA68031378 = IBYsxHSlDwPONgRLvclA86915659;     IBYsxHSlDwPONgRLvclA86915659 = IBYsxHSlDwPONgRLvclA61467937;     IBYsxHSlDwPONgRLvclA61467937 = IBYsxHSlDwPONgRLvclA60542184;     IBYsxHSlDwPONgRLvclA60542184 = IBYsxHSlDwPONgRLvclA9209422;     IBYsxHSlDwPONgRLvclA9209422 = IBYsxHSlDwPONgRLvclA46381950;     IBYsxHSlDwPONgRLvclA46381950 = IBYsxHSlDwPONgRLvclA72550363;     IBYsxHSlDwPONgRLvclA72550363 = IBYsxHSlDwPONgRLvclA66883652;     IBYsxHSlDwPONgRLvclA66883652 = IBYsxHSlDwPONgRLvclA52828166;     IBYsxHSlDwPONgRLvclA52828166 = IBYsxHSlDwPONgRLvclA49092261;     IBYsxHSlDwPONgRLvclA49092261 = IBYsxHSlDwPONgRLvclA62233952;     IBYsxHSlDwPONgRLvclA62233952 = IBYsxHSlDwPONgRLvclA81280566;     IBYsxHSlDwPONgRLvclA81280566 = IBYsxHSlDwPONgRLvclA75179599;     IBYsxHSlDwPONgRLvclA75179599 = IBYsxHSlDwPONgRLvclA97122082;     IBYsxHSlDwPONgRLvclA97122082 = IBYsxHSlDwPONgRLvclA56048812;     IBYsxHSlDwPONgRLvclA56048812 = IBYsxHSlDwPONgRLvclA67068270;     IBYsxHSlDwPONgRLvclA67068270 = IBYsxHSlDwPONgRLvclA93473903;     IBYsxHSlDwPONgRLvclA93473903 = IBYsxHSlDwPONgRLvclA17996196;     IBYsxHSlDwPONgRLvclA17996196 = IBYsxHSlDwPONgRLvclA5782538;     IBYsxHSlDwPONgRLvclA5782538 = IBYsxHSlDwPONgRLvclA75051188;     IBYsxHSlDwPONgRLvclA75051188 = IBYsxHSlDwPONgRLvclA10443471;     IBYsxHSlDwPONgRLvclA10443471 = IBYsxHSlDwPONgRLvclA77416714;     IBYsxHSlDwPONgRLvclA77416714 = IBYsxHSlDwPONgRLvclA93857139;     IBYsxHSlDwPONgRLvclA93857139 = IBYsxHSlDwPONgRLvclA55606561;     IBYsxHSlDwPONgRLvclA55606561 = IBYsxHSlDwPONgRLvclA87941999;     IBYsxHSlDwPONgRLvclA87941999 = IBYsxHSlDwPONgRLvclA18132225;     IBYsxHSlDwPONgRLvclA18132225 = IBYsxHSlDwPONgRLvclA79795825;     IBYsxHSlDwPONgRLvclA79795825 = IBYsxHSlDwPONgRLvclA70889727;     IBYsxHSlDwPONgRLvclA70889727 = IBYsxHSlDwPONgRLvclA51614414;     IBYsxHSlDwPONgRLvclA51614414 = IBYsxHSlDwPONgRLvclA21412757;     IBYsxHSlDwPONgRLvclA21412757 = IBYsxHSlDwPONgRLvclA83961856;     IBYsxHSlDwPONgRLvclA83961856 = IBYsxHSlDwPONgRLvclA37838532;     IBYsxHSlDwPONgRLvclA37838532 = IBYsxHSlDwPONgRLvclA1743195;     IBYsxHSlDwPONgRLvclA1743195 = IBYsxHSlDwPONgRLvclA57776832;     IBYsxHSlDwPONgRLvclA57776832 = IBYsxHSlDwPONgRLvclA3876334;     IBYsxHSlDwPONgRLvclA3876334 = IBYsxHSlDwPONgRLvclA14670341;     IBYsxHSlDwPONgRLvclA14670341 = IBYsxHSlDwPONgRLvclA30315352;     IBYsxHSlDwPONgRLvclA30315352 = IBYsxHSlDwPONgRLvclA72985697;     IBYsxHSlDwPONgRLvclA72985697 = IBYsxHSlDwPONgRLvclA23919935;     IBYsxHSlDwPONgRLvclA23919935 = IBYsxHSlDwPONgRLvclA40643979;     IBYsxHSlDwPONgRLvclA40643979 = IBYsxHSlDwPONgRLvclA89097507;     IBYsxHSlDwPONgRLvclA89097507 = IBYsxHSlDwPONgRLvclA83987055;     IBYsxHSlDwPONgRLvclA83987055 = IBYsxHSlDwPONgRLvclA6768229;     IBYsxHSlDwPONgRLvclA6768229 = IBYsxHSlDwPONgRLvclA62471735;     IBYsxHSlDwPONgRLvclA62471735 = IBYsxHSlDwPONgRLvclA76171785;     IBYsxHSlDwPONgRLvclA76171785 = IBYsxHSlDwPONgRLvclA5044487;}
// Junk Finished

// Junk Code By Peatreat & Thaisen's Gen
void XjwgPppNhrYbXsncKvvk15384710() {     int jvnFiXXnminyRbIvDfJr7963434 = -582565010;    int jvnFiXXnminyRbIvDfJr23660432 = -211654767;    int jvnFiXXnminyRbIvDfJr67412521 = -361734724;    int jvnFiXXnminyRbIvDfJr93279538 = -862961588;    int jvnFiXXnminyRbIvDfJr17613557 = -795290416;    int jvnFiXXnminyRbIvDfJr24071857 = 92400559;    int jvnFiXXnminyRbIvDfJr76528369 = -437430116;    int jvnFiXXnminyRbIvDfJr11358523 = -357954323;    int jvnFiXXnminyRbIvDfJr784267 = -235487841;    int jvnFiXXnminyRbIvDfJr59544352 = -522355250;    int jvnFiXXnminyRbIvDfJr81794922 = -666872796;    int jvnFiXXnminyRbIvDfJr3257541 = -227798870;    int jvnFiXXnminyRbIvDfJr39563700 = -29884909;    int jvnFiXXnminyRbIvDfJr3228508 = -186820997;    int jvnFiXXnminyRbIvDfJr33843628 = -427544757;    int jvnFiXXnminyRbIvDfJr36891822 = -186028241;    int jvnFiXXnminyRbIvDfJr70863024 = 12761867;    int jvnFiXXnminyRbIvDfJr33039218 = -433026345;    int jvnFiXXnminyRbIvDfJr34455219 = -412327864;    int jvnFiXXnminyRbIvDfJr34604041 = -918663546;    int jvnFiXXnminyRbIvDfJr49741213 = -116925355;    int jvnFiXXnminyRbIvDfJr85846243 = -118052013;    int jvnFiXXnminyRbIvDfJr40541453 = -687075533;    int jvnFiXXnminyRbIvDfJr2790417 = -901745628;    int jvnFiXXnminyRbIvDfJr60714572 = -972095840;    int jvnFiXXnminyRbIvDfJr94211930 = -253816902;    int jvnFiXXnminyRbIvDfJr51560153 = -867065286;    int jvnFiXXnminyRbIvDfJr11773921 = -689882310;    int jvnFiXXnminyRbIvDfJr99238848 = -613821826;    int jvnFiXXnminyRbIvDfJr35930092 = -983366691;    int jvnFiXXnminyRbIvDfJr11103950 = -924969491;    int jvnFiXXnminyRbIvDfJr54204255 = -310588992;    int jvnFiXXnminyRbIvDfJr39131206 = -628896272;    int jvnFiXXnminyRbIvDfJr88379759 = -145741111;    int jvnFiXXnminyRbIvDfJr8206741 = -772399365;    int jvnFiXXnminyRbIvDfJr7212933 = 96617414;    int jvnFiXXnminyRbIvDfJr16839550 = -554608300;    int jvnFiXXnminyRbIvDfJr45671508 = -337501344;    int jvnFiXXnminyRbIvDfJr14124164 = -538366827;    int jvnFiXXnminyRbIvDfJr85929907 = -866676780;    int jvnFiXXnminyRbIvDfJr48923185 = -11061914;    int jvnFiXXnminyRbIvDfJr27519966 = -21622975;    int jvnFiXXnminyRbIvDfJr94013345 = -841070466;    int jvnFiXXnminyRbIvDfJr27622576 = -127545870;    int jvnFiXXnminyRbIvDfJr26360593 = -386142148;    int jvnFiXXnminyRbIvDfJr2074971 = -407167642;    int jvnFiXXnminyRbIvDfJr22851690 = -295723197;    int jvnFiXXnminyRbIvDfJr53045882 = -569007046;    int jvnFiXXnminyRbIvDfJr39088733 = -574507339;    int jvnFiXXnminyRbIvDfJr38662234 = -566371514;    int jvnFiXXnminyRbIvDfJr24500658 = 13094231;    int jvnFiXXnminyRbIvDfJr70057771 = -297570446;    int jvnFiXXnminyRbIvDfJr6893100 = -166921259;    int jvnFiXXnminyRbIvDfJr55181961 = -210332544;    int jvnFiXXnminyRbIvDfJr65643497 = -796605473;    int jvnFiXXnminyRbIvDfJr22117190 = -364512997;    int jvnFiXXnminyRbIvDfJr83118979 = -524579234;    int jvnFiXXnminyRbIvDfJr64622105 = -459989096;    int jvnFiXXnminyRbIvDfJr32564966 = -890865748;    int jvnFiXXnminyRbIvDfJr23401627 = -441473514;    int jvnFiXXnminyRbIvDfJr72511703 = -40534156;    int jvnFiXXnminyRbIvDfJr64754449 = -747547807;    int jvnFiXXnminyRbIvDfJr12119674 = -744132498;    int jvnFiXXnminyRbIvDfJr64854174 = -252121150;    int jvnFiXXnminyRbIvDfJr48440402 = -597385760;    int jvnFiXXnminyRbIvDfJr27590668 = -256283805;    int jvnFiXXnminyRbIvDfJr64126335 = -598902599;    int jvnFiXXnminyRbIvDfJr51183941 = -884143799;    int jvnFiXXnminyRbIvDfJr95021766 = -414421632;    int jvnFiXXnminyRbIvDfJr26630696 = -424162172;    int jvnFiXXnminyRbIvDfJr20052272 = -631419941;    int jvnFiXXnminyRbIvDfJr25191516 = -649736790;    int jvnFiXXnminyRbIvDfJr18915054 = -894659519;    int jvnFiXXnminyRbIvDfJr48525311 = -545651084;    int jvnFiXXnminyRbIvDfJr85680855 = -807601633;    int jvnFiXXnminyRbIvDfJr22221247 = 4697620;    int jvnFiXXnminyRbIvDfJr91832898 = -276981548;    int jvnFiXXnminyRbIvDfJr12918877 = -459529664;    int jvnFiXXnminyRbIvDfJr76429824 = -415603481;    int jvnFiXXnminyRbIvDfJr58639602 = -464928199;    int jvnFiXXnminyRbIvDfJr71360241 = -958093705;    int jvnFiXXnminyRbIvDfJr98514270 = -198058240;    int jvnFiXXnminyRbIvDfJr72685188 = -15374971;    int jvnFiXXnminyRbIvDfJr60576614 = 52549687;    int jvnFiXXnminyRbIvDfJr11429435 = -896460923;    int jvnFiXXnminyRbIvDfJr41046179 = -527399045;    int jvnFiXXnminyRbIvDfJr47311155 = -43667733;    int jvnFiXXnminyRbIvDfJr83949245 = -318563729;    int jvnFiXXnminyRbIvDfJr22736262 = -349135639;    int jvnFiXXnminyRbIvDfJr86089550 = -307886369;    int jvnFiXXnminyRbIvDfJr24093954 = -378803353;    int jvnFiXXnminyRbIvDfJr52217445 = 5380796;    int jvnFiXXnminyRbIvDfJr13106542 = -446635596;    int jvnFiXXnminyRbIvDfJr90722536 = 3106687;    int jvnFiXXnminyRbIvDfJr13418205 = -726142625;    int jvnFiXXnminyRbIvDfJr84168736 = -263514108;    int jvnFiXXnminyRbIvDfJr15400292 = -277490478;    int jvnFiXXnminyRbIvDfJr29159171 = -488949317;    int jvnFiXXnminyRbIvDfJr79182173 = -530160110;    int jvnFiXXnminyRbIvDfJr98769924 = -582565010;     jvnFiXXnminyRbIvDfJr7963434 = jvnFiXXnminyRbIvDfJr23660432;     jvnFiXXnminyRbIvDfJr23660432 = jvnFiXXnminyRbIvDfJr67412521;     jvnFiXXnminyRbIvDfJr67412521 = jvnFiXXnminyRbIvDfJr93279538;     jvnFiXXnminyRbIvDfJr93279538 = jvnFiXXnminyRbIvDfJr17613557;     jvnFiXXnminyRbIvDfJr17613557 = jvnFiXXnminyRbIvDfJr24071857;     jvnFiXXnminyRbIvDfJr24071857 = jvnFiXXnminyRbIvDfJr76528369;     jvnFiXXnminyRbIvDfJr76528369 = jvnFiXXnminyRbIvDfJr11358523;     jvnFiXXnminyRbIvDfJr11358523 = jvnFiXXnminyRbIvDfJr784267;     jvnFiXXnminyRbIvDfJr784267 = jvnFiXXnminyRbIvDfJr59544352;     jvnFiXXnminyRbIvDfJr59544352 = jvnFiXXnminyRbIvDfJr81794922;     jvnFiXXnminyRbIvDfJr81794922 = jvnFiXXnminyRbIvDfJr3257541;     jvnFiXXnminyRbIvDfJr3257541 = jvnFiXXnminyRbIvDfJr39563700;     jvnFiXXnminyRbIvDfJr39563700 = jvnFiXXnminyRbIvDfJr3228508;     jvnFiXXnminyRbIvDfJr3228508 = jvnFiXXnminyRbIvDfJr33843628;     jvnFiXXnminyRbIvDfJr33843628 = jvnFiXXnminyRbIvDfJr36891822;     jvnFiXXnminyRbIvDfJr36891822 = jvnFiXXnminyRbIvDfJr70863024;     jvnFiXXnminyRbIvDfJr70863024 = jvnFiXXnminyRbIvDfJr33039218;     jvnFiXXnminyRbIvDfJr33039218 = jvnFiXXnminyRbIvDfJr34455219;     jvnFiXXnminyRbIvDfJr34455219 = jvnFiXXnminyRbIvDfJr34604041;     jvnFiXXnminyRbIvDfJr34604041 = jvnFiXXnminyRbIvDfJr49741213;     jvnFiXXnminyRbIvDfJr49741213 = jvnFiXXnminyRbIvDfJr85846243;     jvnFiXXnminyRbIvDfJr85846243 = jvnFiXXnminyRbIvDfJr40541453;     jvnFiXXnminyRbIvDfJr40541453 = jvnFiXXnminyRbIvDfJr2790417;     jvnFiXXnminyRbIvDfJr2790417 = jvnFiXXnminyRbIvDfJr60714572;     jvnFiXXnminyRbIvDfJr60714572 = jvnFiXXnminyRbIvDfJr94211930;     jvnFiXXnminyRbIvDfJr94211930 = jvnFiXXnminyRbIvDfJr51560153;     jvnFiXXnminyRbIvDfJr51560153 = jvnFiXXnminyRbIvDfJr11773921;     jvnFiXXnminyRbIvDfJr11773921 = jvnFiXXnminyRbIvDfJr99238848;     jvnFiXXnminyRbIvDfJr99238848 = jvnFiXXnminyRbIvDfJr35930092;     jvnFiXXnminyRbIvDfJr35930092 = jvnFiXXnminyRbIvDfJr11103950;     jvnFiXXnminyRbIvDfJr11103950 = jvnFiXXnminyRbIvDfJr54204255;     jvnFiXXnminyRbIvDfJr54204255 = jvnFiXXnminyRbIvDfJr39131206;     jvnFiXXnminyRbIvDfJr39131206 = jvnFiXXnminyRbIvDfJr88379759;     jvnFiXXnminyRbIvDfJr88379759 = jvnFiXXnminyRbIvDfJr8206741;     jvnFiXXnminyRbIvDfJr8206741 = jvnFiXXnminyRbIvDfJr7212933;     jvnFiXXnminyRbIvDfJr7212933 = jvnFiXXnminyRbIvDfJr16839550;     jvnFiXXnminyRbIvDfJr16839550 = jvnFiXXnminyRbIvDfJr45671508;     jvnFiXXnminyRbIvDfJr45671508 = jvnFiXXnminyRbIvDfJr14124164;     jvnFiXXnminyRbIvDfJr14124164 = jvnFiXXnminyRbIvDfJr85929907;     jvnFiXXnminyRbIvDfJr85929907 = jvnFiXXnminyRbIvDfJr48923185;     jvnFiXXnminyRbIvDfJr48923185 = jvnFiXXnminyRbIvDfJr27519966;     jvnFiXXnminyRbIvDfJr27519966 = jvnFiXXnminyRbIvDfJr94013345;     jvnFiXXnminyRbIvDfJr94013345 = jvnFiXXnminyRbIvDfJr27622576;     jvnFiXXnminyRbIvDfJr27622576 = jvnFiXXnminyRbIvDfJr26360593;     jvnFiXXnminyRbIvDfJr26360593 = jvnFiXXnminyRbIvDfJr2074971;     jvnFiXXnminyRbIvDfJr2074971 = jvnFiXXnminyRbIvDfJr22851690;     jvnFiXXnminyRbIvDfJr22851690 = jvnFiXXnminyRbIvDfJr53045882;     jvnFiXXnminyRbIvDfJr53045882 = jvnFiXXnminyRbIvDfJr39088733;     jvnFiXXnminyRbIvDfJr39088733 = jvnFiXXnminyRbIvDfJr38662234;     jvnFiXXnminyRbIvDfJr38662234 = jvnFiXXnminyRbIvDfJr24500658;     jvnFiXXnminyRbIvDfJr24500658 = jvnFiXXnminyRbIvDfJr70057771;     jvnFiXXnminyRbIvDfJr70057771 = jvnFiXXnminyRbIvDfJr6893100;     jvnFiXXnminyRbIvDfJr6893100 = jvnFiXXnminyRbIvDfJr55181961;     jvnFiXXnminyRbIvDfJr55181961 = jvnFiXXnminyRbIvDfJr65643497;     jvnFiXXnminyRbIvDfJr65643497 = jvnFiXXnminyRbIvDfJr22117190;     jvnFiXXnminyRbIvDfJr22117190 = jvnFiXXnminyRbIvDfJr83118979;     jvnFiXXnminyRbIvDfJr83118979 = jvnFiXXnminyRbIvDfJr64622105;     jvnFiXXnminyRbIvDfJr64622105 = jvnFiXXnminyRbIvDfJr32564966;     jvnFiXXnminyRbIvDfJr32564966 = jvnFiXXnminyRbIvDfJr23401627;     jvnFiXXnminyRbIvDfJr23401627 = jvnFiXXnminyRbIvDfJr72511703;     jvnFiXXnminyRbIvDfJr72511703 = jvnFiXXnminyRbIvDfJr64754449;     jvnFiXXnminyRbIvDfJr64754449 = jvnFiXXnminyRbIvDfJr12119674;     jvnFiXXnminyRbIvDfJr12119674 = jvnFiXXnminyRbIvDfJr64854174;     jvnFiXXnminyRbIvDfJr64854174 = jvnFiXXnminyRbIvDfJr48440402;     jvnFiXXnminyRbIvDfJr48440402 = jvnFiXXnminyRbIvDfJr27590668;     jvnFiXXnminyRbIvDfJr27590668 = jvnFiXXnminyRbIvDfJr64126335;     jvnFiXXnminyRbIvDfJr64126335 = jvnFiXXnminyRbIvDfJr51183941;     jvnFiXXnminyRbIvDfJr51183941 = jvnFiXXnminyRbIvDfJr95021766;     jvnFiXXnminyRbIvDfJr95021766 = jvnFiXXnminyRbIvDfJr26630696;     jvnFiXXnminyRbIvDfJr26630696 = jvnFiXXnminyRbIvDfJr20052272;     jvnFiXXnminyRbIvDfJr20052272 = jvnFiXXnminyRbIvDfJr25191516;     jvnFiXXnminyRbIvDfJr25191516 = jvnFiXXnminyRbIvDfJr18915054;     jvnFiXXnminyRbIvDfJr18915054 = jvnFiXXnminyRbIvDfJr48525311;     jvnFiXXnminyRbIvDfJr48525311 = jvnFiXXnminyRbIvDfJr85680855;     jvnFiXXnminyRbIvDfJr85680855 = jvnFiXXnminyRbIvDfJr22221247;     jvnFiXXnminyRbIvDfJr22221247 = jvnFiXXnminyRbIvDfJr91832898;     jvnFiXXnminyRbIvDfJr91832898 = jvnFiXXnminyRbIvDfJr12918877;     jvnFiXXnminyRbIvDfJr12918877 = jvnFiXXnminyRbIvDfJr76429824;     jvnFiXXnminyRbIvDfJr76429824 = jvnFiXXnminyRbIvDfJr58639602;     jvnFiXXnminyRbIvDfJr58639602 = jvnFiXXnminyRbIvDfJr71360241;     jvnFiXXnminyRbIvDfJr71360241 = jvnFiXXnminyRbIvDfJr98514270;     jvnFiXXnminyRbIvDfJr98514270 = jvnFiXXnminyRbIvDfJr72685188;     jvnFiXXnminyRbIvDfJr72685188 = jvnFiXXnminyRbIvDfJr60576614;     jvnFiXXnminyRbIvDfJr60576614 = jvnFiXXnminyRbIvDfJr11429435;     jvnFiXXnminyRbIvDfJr11429435 = jvnFiXXnminyRbIvDfJr41046179;     jvnFiXXnminyRbIvDfJr41046179 = jvnFiXXnminyRbIvDfJr47311155;     jvnFiXXnminyRbIvDfJr47311155 = jvnFiXXnminyRbIvDfJr83949245;     jvnFiXXnminyRbIvDfJr83949245 = jvnFiXXnminyRbIvDfJr22736262;     jvnFiXXnminyRbIvDfJr22736262 = jvnFiXXnminyRbIvDfJr86089550;     jvnFiXXnminyRbIvDfJr86089550 = jvnFiXXnminyRbIvDfJr24093954;     jvnFiXXnminyRbIvDfJr24093954 = jvnFiXXnminyRbIvDfJr52217445;     jvnFiXXnminyRbIvDfJr52217445 = jvnFiXXnminyRbIvDfJr13106542;     jvnFiXXnminyRbIvDfJr13106542 = jvnFiXXnminyRbIvDfJr90722536;     jvnFiXXnminyRbIvDfJr90722536 = jvnFiXXnminyRbIvDfJr13418205;     jvnFiXXnminyRbIvDfJr13418205 = jvnFiXXnminyRbIvDfJr84168736;     jvnFiXXnminyRbIvDfJr84168736 = jvnFiXXnminyRbIvDfJr15400292;     jvnFiXXnminyRbIvDfJr15400292 = jvnFiXXnminyRbIvDfJr29159171;     jvnFiXXnminyRbIvDfJr29159171 = jvnFiXXnminyRbIvDfJr79182173;     jvnFiXXnminyRbIvDfJr79182173 = jvnFiXXnminyRbIvDfJr98769924;     jvnFiXXnminyRbIvDfJr98769924 = jvnFiXXnminyRbIvDfJr7963434;}
// Junk Finished
