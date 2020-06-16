#define NOMINMAX

#include <sstream>

#include "Esp.h"
#include "../Config.h"
#include "../Interfaces.h"
#include "../Memory.h"

#include "../SDK/ConVar.h"
#include "../SDK/Entity.h"
#include "../SDK/GlobalVars.h"
#include "../SDK/Localize.h"
#include "../SDK/Surface.h"
#include "../SDK/Vector.h"
#include "../SDK/WeaponData.h"

bool canHear(LocalPlayer& local, Entity& target) noexcept
{
    bool x = target.velocity().length2D() > 110.0f;
    bool z = target.getShotsFired() >= 1;
    //bool y = isInRange(&target, 25.4f);
    bool y = (target.getAbsOrigin() - localPlayer->getAbsOrigin()).length() * 0.0254f <= 25.4f;
    bool smk = target.isVisible();
    bool line = memory->lineGoesThroughSmoke(local->getEyePosition(), target.getBonePosition(8), 1);
    //return (x || z || smk) && (y || smk);
    return (x || z || (smk && !line)) && (y || (smk && !line));
}

static constexpr bool worldToScreen(const Vector& in, Vector& out) noexcept
{
    const auto& matrix = interfaces->engine->worldToScreenMatrix();
    float w = matrix._41 * in.x + matrix._42 * in.y + matrix._43 * in.z + matrix._44;

    if (w > 0.001f) {
        const auto [width, height] = interfaces->surface->getScreenSize();
        out.x = width / 2 * (1 + (matrix._11 * in.x + matrix._12 * in.y + matrix._13 * in.z + matrix._14) / w);
        out.y = height / 2 * (1 - (matrix._21 * in.x + matrix._22 * in.y + matrix._23 * in.z + matrix._24) / w);
        out.z = 0.0f;
        return true;
    }
    return false;
}

static void renderSnaplines(Entity* entity, const Config::Esp::Shared& config) noexcept
{
    if (!config.snaplines.enabled)
        return;

    Vector position;
    if (!worldToScreen(entity->getAbsOrigin(), position))
        return;

    if (config.snaplines.rainbow)
        interfaces->surface->setDrawColor(rainbowColor(memory->globalVars->realtime, config.snaplines.rainbowSpeed));
    else
        interfaces->surface->setDrawColor(config.snaplines.color);

    const auto [width, height] = interfaces->surface->getScreenSize();
    interfaces->surface->drawLine(width / 2, height, static_cast<int>(position.x), static_cast<int>(position.y));
}

static void renderEyeTraces(Entity* entity, const Config::Esp::Player& config) noexcept
{
    if (config.eyeTraces.enabled) {
        constexpr float maxRange{ 8192.0f };

        auto eyeAngles = entity->eyeAngles();
        Vector viewAngles{ cos(degreesToRadians(eyeAngles.x)) * cos(degreesToRadians(eyeAngles.y)) * maxRange,
                           cos(degreesToRadians(eyeAngles.x)) * sin(degreesToRadians(eyeAngles.y)) * maxRange,
                          -sin(degreesToRadians(eyeAngles.x)) * maxRange };
        static Trace trace;
        Vector headPosition{ entity->getBonePosition(8) };
        interfaces->engineTrace->traceRay({ headPosition, headPosition + viewAngles }, 0x46004009, { entity }, trace);
        Vector start, end;
        if (worldToScreen(trace.startpos, start) && worldToScreen(trace.endpos, end)) {
            if (config.eyeTraces.rainbow)
                interfaces->surface->setDrawColor(rainbowColor(memory->globalVars->realtime, config.eyeTraces.rainbowSpeed));
            else
                interfaces->surface->setDrawColor(config.eyeTraces.color);

            interfaces->surface->drawLine(start.x, start.y, end.x, end.y);
        }
    }
}

static constexpr void renderPositionedText(unsigned font, const wchar_t* text, std::pair<float, float&> position) noexcept
{
    interfaces->surface->setTextFont(font);
    interfaces->surface->setTextPosition(position.first, position.second);
    position.second += interfaces->surface->getTextSize(font, text).second;
    interfaces->surface->printText(text);
}

struct BoundingBox {
    float x0, y0;
    float x1, y1;
    Vector vertices[8];

    BoundingBox(Entity* entity) noexcept
    {
        const auto [width, height] = interfaces->surface->getScreenSize();

        x0 = static_cast<float>(width * 2);
        y0 = static_cast<float>(height * 2);
        x1 = -x0;
        y1 = -y0;

        const auto& mins = entity->getCollideable()->obbMins();
        const auto& maxs = entity->getCollideable()->obbMaxs();

        for (int i = 0; i < 8; ++i) {
            const Vector point{ i & 1 ? maxs.x : mins.x,
                                i & 2 ? maxs.y : mins.y,
                                i & 4 ? maxs.z : mins.z };

            if (!worldToScreen(point.transform(entity->coordinateFrame()), vertices[i])) {
                valid = false;
                return;
            }
            x0 = std::min(x0, vertices[i].x);
            y0 = std::min(y0, vertices[i].y);
            x1 = std::max(x1, vertices[i].x);
            y1 = std::max(y1, vertices[i].y);
        }
        valid = true;
    }

    operator bool() noexcept
    {
        return valid;
    }
private:
    bool valid;
};

static void renderBox(const BoundingBox& bbox, const Config::Esp::Shared& config) noexcept
{
    if (config.box.enabled) {
        if (config.box.rainbow)
            interfaces->surface->setDrawColor(rainbowColor(memory->globalVars->realtime, config.box.rainbowSpeed));
        else
            interfaces->surface->setDrawColor(config.box.color);
        
        switch (config.boxType) {
        case 0:
            interfaces->surface->drawOutlinedRect(bbox.x0, bbox.y0, bbox.x1, bbox.y1);

            if (config.outline.enabled) {
                if (config.outline.rainbow)
                    interfaces->surface->setDrawColor(rainbowColor(memory->globalVars->realtime, config.outline.rainbowSpeed));
                else
                    interfaces->surface->setDrawColor(config.outline.color);

                interfaces->surface->drawOutlinedRect(bbox.x0 + 1, bbox.y0 + 1, bbox.x1 - 1, bbox.y1 - 1);
                interfaces->surface->drawOutlinedRect(bbox.x0 - 1, bbox.y0 - 1, bbox.x1 + 1, bbox.y1 + 1);
            }
            break;
        case 1:
            interfaces->surface->drawLine(bbox.x0, bbox.y0, bbox.x0, bbox.y0 + fabsf(bbox.y1 - bbox.y0) / 4);
            interfaces->surface->drawLine(bbox.x0, bbox.y0, bbox.x0 + fabsf(bbox.x1 - bbox.x0) / 4, bbox.y0);
            interfaces->surface->drawLine(bbox.x1, bbox.y0, bbox.x1 - fabsf(bbox.x1 - bbox.x0) / 4, bbox.y0);
            interfaces->surface->drawLine(bbox.x1, bbox.y0, bbox.x1, bbox.y0 + fabsf(bbox.y1 - bbox.y0) / 4);
            interfaces->surface->drawLine(bbox.x0, bbox.y1, bbox.x0, bbox.y1 - fabsf(bbox.y1 - bbox.y0) / 4);
            interfaces->surface->drawLine(bbox.x0, bbox.y1, bbox.x0 + fabsf(bbox.x1 - bbox.x0) / 4, bbox.y1);
            interfaces->surface->drawLine(bbox.x1, bbox.y1, bbox.x1 - fabsf(bbox.x1 - bbox.x0) / 4, bbox.y1);
            interfaces->surface->drawLine(bbox.x1, bbox.y1, bbox.x1, bbox.y1 - fabsf(bbox.y1 - bbox.y0) / 4);

            if (config.outline.enabled) {
                if (config.outline.rainbow)
                    interfaces->surface->setDrawColor(rainbowColor(memory->globalVars->realtime, config.outline.rainbowSpeed));
                else
                    interfaces->surface->setDrawColor(config.outline.color);

                // TODO: get rid of fabsf()

                interfaces->surface->drawLine(bbox.x0 - 1, bbox.y0 - 1, bbox.x0 - 1, bbox.y0 + fabsf(bbox.y1 - bbox.y0) / 4);
                interfaces->surface->drawLine(bbox.x0 - 1, bbox.y0 - 1, bbox.x0 + fabsf(bbox.x1 - bbox.x0) / 4, bbox.y0 - 1);
                interfaces->surface->drawLine(bbox.x1 + 1, bbox.y0 - 1, bbox.x1 - fabsf(bbox.x1 - bbox.x0) / 4, bbox.y0 - 1);
                interfaces->surface->drawLine(bbox.x1 + 1, bbox.y0 - 1, bbox.x1 + 1, bbox.y0 + fabsf(bbox.y1 - bbox.y0) / 4);
                interfaces->surface->drawLine(bbox.x0 - 1, bbox.y1 + 1, bbox.x0 - 1, bbox.y1 - fabsf(bbox.y1 - bbox.y0) / 4);
                interfaces->surface->drawLine(bbox.x0 - 1, bbox.y1 + 1, bbox.x0 + fabsf(bbox.x1 - bbox.x0) / 4, bbox.y1 + 1);
                interfaces->surface->drawLine(bbox.x1 + 1, bbox.y1 + 1, bbox.x1 - fabsf(bbox.x1 - bbox.x0) / 4, bbox.y1 + 1);
                interfaces->surface->drawLine(bbox.x1 + 1, bbox.y1 + 1, bbox.x1 + 1, bbox.y1 - fabsf(bbox.y1 - bbox.y0) / 4);


                interfaces->surface->drawLine(bbox.x0 + 1, bbox.y0 + 1, bbox.x0 + 1, bbox.y0 + fabsf(bbox.y1 - bbox.y0) / 4);
                interfaces->surface->drawLine(bbox.x0 + 2, bbox.y0 + 1, bbox.x0 + fabsf(bbox.x1 - bbox.x0) / 4, bbox.y0 + 1);


                interfaces->surface->drawLine(bbox.x1 - 1, bbox.y0 + 1, bbox.x1 - fabsf(bbox.x1 - bbox.x0) / 4, (bbox.y0 + 1));
                interfaces->surface->drawLine(bbox.x1 - 1, bbox.y0 + 1, bbox.x1 - 1, bbox.y0 + fabsf(bbox.y1 - bbox.y0) / 4);

                interfaces->surface->drawLine(bbox.x0 + 1, bbox.y1 - 1, bbox.x0 + 1, (bbox.y1) - fabsf(bbox.y1 - bbox.y0) / 4);
                interfaces->surface->drawLine(bbox.x0 + 1, bbox.y1 - 1, bbox.x0 + fabsf(bbox.x1 - bbox.x0) / 4, bbox.y1 - 1);

                interfaces->surface->drawLine(bbox.x1 - 1, bbox.y1 - 1, bbox.x1 - fabsf(bbox.x1 - bbox.x0) / 4, bbox.y1 - 1);
                interfaces->surface->drawLine(bbox.x1 - 1, bbox.y1 - 2, (bbox.x1 - 1), (bbox.y1 - 1) - fabsf(bbox.y1 - bbox.y0) / 4);

                interfaces->surface->drawLine(bbox.x0 - 1, fabsf((bbox.y1 - bbox.y0) / 4) + bbox.y0, bbox.x0 + 2, fabsf((bbox.y1 - bbox.y0) / 4) + bbox.y0);
                interfaces->surface->drawLine(bbox.x1 + 1, fabsf((bbox.y1 - bbox.y0) / 4) + bbox.y0, bbox.x1 - 2, fabsf((bbox.y1 - bbox.y0) / 4) + bbox.y0);
                interfaces->surface->drawLine(bbox.x0 - 1, fabsf((bbox.y1 - bbox.y0) * 3 / 4) + bbox.y0, bbox.x0 + 2, fabsf((bbox.y1 - bbox.y0) * 3 / 4) + bbox.y0);
                interfaces->surface->drawLine(bbox.x1 + 1, fabsf((bbox.y1 - bbox.y0) * 3 / 4) + bbox.y0, bbox.x1 - 2, fabsf((bbox.y1 - bbox.y0) * 3 / 4) + bbox.y0);
                interfaces->surface->drawLine(fabsf((bbox.x1 - bbox.x0) / 4) + bbox.x0, bbox.y0 + 1, fabsf((bbox.x1 - bbox.x0) / 4) + bbox.x0, bbox.y0 - 2);
                interfaces->surface->drawLine(fabsf((bbox.x1 - bbox.x0) / 4) + bbox.x0, bbox.y1 + 1, fabsf((bbox.x1 - bbox.x0) / 4) + bbox.x0, bbox.y1 - 2);
                interfaces->surface->drawLine(fabsf((bbox.x1 - bbox.x0) * 3 / 4) + bbox.x0, bbox.y0 + 1, fabsf((bbox.x1 - bbox.x0) * 3 / 4) + bbox.x0, bbox.y0 - 2);
                interfaces->surface->drawLine(fabsf((bbox.x1 - bbox.x0) * 3 / 4) + bbox.x0, bbox.y1 + 1, fabsf((bbox.x1 - bbox.x0) * 3 / 4) + bbox.x0, bbox.y1 - 2);
            }
            break;
        case 2:
            for (int i = 0; i < 8; i++) {
                for (int j = 1; j <= 4; j <<= 1) {
                    if (!(i & j))
                        interfaces->surface->drawLine(bbox.vertices[i].x, bbox.vertices[i].y, bbox.vertices[i + j].x, bbox.vertices[i + j].y);
                }
            }
            break;
        case 3:
            for (int i = 0; i < 8; i++) {
                for (int j = 1; j <= 4; j <<= 1) {
                    if (!(i & j)) {
                        interfaces->surface->drawLine(bbox.vertices[i].x, bbox.vertices[i].y, bbox.vertices[i].x + (bbox.vertices[i + j].x - bbox.vertices[i].x) * 0.25f, bbox.vertices[i].y + (bbox.vertices[i + j].y - bbox.vertices[i].y) * 0.25f);
                        interfaces->surface->drawLine(bbox.vertices[i].x + (bbox.vertices[i + j].x - bbox.vertices[i].x) * 0.75f, bbox.vertices[i].y + (bbox.vertices[i + j].y - bbox.vertices[i].y) * 0.75f, bbox.vertices[i + j].x, bbox.vertices[i + j].y);
                    }
                }
            }
            break;
        }
    }
}

static void renderPlayerBox(Entity* entity, const Config::Esp::Player& config) noexcept
{
    if (BoundingBox bbox{ entity }) {
        renderBox(bbox, config);

        float drawPositionX = bbox.x0 - 5;

        if (config.healthBar.enabled) {
            static auto gameType{ interfaces->cvar->findVar("game_type") };
            static auto survivalMaxHealth{ interfaces->cvar->findVar("sv_dz_player_max_health") };

            const auto maxHealth{ (std::max)((gameType->getInt() == 6 ? survivalMaxHealth->getInt() : 100), entity->health()) };

            if (config.healthBar.rainbow)
                interfaces->surface->setDrawColor(rainbowColor(memory->globalVars->realtime, config.healthBar.rainbowSpeed));
            else
            {
                int health = entity->health();
                if (health > 0 && health <= 20)
                {
                    interfaces->surface->setDrawColor(255, 0, 0);
                }
                else if (health > 20 && health <= 40)
                {
                    interfaces->surface->setDrawColor(218, 182, 0);
                }
                else if (health > 40 && health <= 60)
                {
                    interfaces->surface->setDrawColor(233, 215, 0);
                }
                else if (health > 60 && health <= 80)
                {
                    interfaces->surface->setDrawColor(255, 199, 54);
                }
                else if (health > 80)
                {
                    interfaces->surface->setDrawColor(0, 255, 131);
                }
            }

            interfaces->surface->drawFilledRect(drawPositionX - 3, bbox.y0 + abs(bbox.y1 - bbox.y0) * (maxHealth - entity->health()) / static_cast<float>(maxHealth), drawPositionX, bbox.y1);
            
            if (config.outline.enabled) {
                if (config.outline.rainbow)
                    interfaces->surface->setDrawColor(rainbowColor(memory->globalVars->realtime, config.outline.rainbowSpeed));
                else
                    interfaces->surface->setDrawColor(config.outline.color);

                interfaces->surface->drawOutlinedRect(drawPositionX - 4, bbox.y0 - 1, drawPositionX + 1, bbox.y1 + 1);
            }
            drawPositionX -= 7;
        }

        if (config.armorBar.enabled) {
            if (config.armorBar.rainbow)
                interfaces->surface->setDrawColor(rainbowColor(memory->globalVars->realtime, config.armorBar.rainbowSpeed));
            else
                interfaces->surface->setDrawColor(config.armorBar.color);

            interfaces->surface->drawFilledRect(drawPositionX - 3, bbox.y0 + abs(bbox.y1 - bbox.y0) * (100.0f - entity->armor()) / 100.0f, drawPositionX, bbox.y1);

            if (config.outline.enabled) {
                if (config.outline.rainbow)
                    interfaces->surface->setDrawColor(rainbowColor(memory->globalVars->realtime, config.outline.rainbowSpeed));
                else
                    interfaces->surface->setDrawColor(config.outline.color);

                interfaces->surface->drawOutlinedRect(drawPositionX - 4, bbox.y0 - 1, drawPositionX + 1, bbox.y1 + 1);
            }
            drawPositionX -= 7;
        }

        if (config.name.enabled) {
            if (PlayerInfo playerInfo; interfaces->engine->getPlayerInfo(entity->index(), playerInfo)) {
                if (wchar_t name[128]; MultiByteToWideChar(CP_UTF8, 0, playerInfo.name, -1, name, 128)) {
                    const auto [width, height] { interfaces->surface->getTextSize(config.font, name) };
                    interfaces->surface->setTextFont(config.font);
                    if (config.name.rainbow)
                        interfaces->surface->setTextColor(rainbowColor(memory->globalVars->realtime, config.name.rainbowSpeed));
                    else
                        interfaces->surface->setTextColor(config.name.color);

                    interfaces->surface->setTextPosition((bbox.x0 + bbox.x1 - width) / 2, bbox.y0 - 5 - height);
                    interfaces->surface->printText(name);
                }
            }
        }

        if (const auto activeWeapon{ entity->getActiveWeapon() };  config.activeWeapon.enabled && activeWeapon) {
            const auto name{ interfaces->localize->find(activeWeapon->getWeaponData()->name) };
            const auto [width, height] { interfaces->surface->getTextSize(config.font, name) };
            interfaces->surface->setTextFont(config.font);
            if (config.activeWeapon.rainbow)
                interfaces->surface->setTextColor(rainbowColor(memory->globalVars->realtime, config.activeWeapon.rainbowSpeed));
            else
                interfaces->surface->setTextColor(config.activeWeapon.color);

            interfaces->surface->setTextPosition((bbox.x0 + bbox.x1 - width) / 2, bbox.y1 + 5);
            interfaces->surface->printText(name);
        }     

        float drawPositionY = bbox.y0;

        if (config.health.enabled) {
            if (config.health.rainbow)
                interfaces->surface->setTextColor(rainbowColor(memory->globalVars->realtime, config.health.rainbowSpeed));
            else
                interfaces->surface->setTextColor(config.health.color);

            renderPositionedText(config.font, (std::to_wstring(entity->health()) + L" HP").c_str(), { bbox.x1 + 5, drawPositionY });
         }

        if (config.armor.enabled) {
            if (config.armor.rainbow)
                interfaces->surface->setTextColor(rainbowColor(memory->globalVars->realtime, config.armor.rainbowSpeed));
            else
                interfaces->surface->setTextColor(config.armor.color);

            renderPositionedText(config.font, (std::to_wstring(entity->armor()) + L" AR").c_str(), { bbox.x1 + 5, drawPositionY });
        }

        if (config.money.enabled) {
            if (config.money.rainbow)
                interfaces->surface->setTextColor(rainbowColor(memory->globalVars->realtime, config.money.rainbowSpeed));
            else
                interfaces->surface->setTextColor(config.money.color);

            renderPositionedText(config.font, (L'$' + std::to_wstring(entity->account())).c_str(), { bbox.x1 + 5, drawPositionY });
        }

        if (config.distance.enabled && localPlayer) {
            if (config.distance.rainbow)
                interfaces->surface->setTextColor(rainbowColor(memory->globalVars->realtime, config.distance.rainbowSpeed));
            else
                interfaces->surface->setTextColor(config.distance.color);

            renderPositionedText(config.font, (std::wostringstream{ } << std::fixed << std::showpoint << std::setprecision(2) << (entity->getAbsOrigin() - localPlayer->getAbsOrigin()).length() * 0.0254f << L'm').str().c_str(), { bbox.x1 + 5, drawPositionY });
        }
    }
}

static void renderWeaponBox(Entity* entity, const Config::Esp::Weapon& config) noexcept
{
    BoundingBox bbox{ entity };

    if (!bbox)
        return;

    renderBox(bbox, config);

    if (config.name.enabled) {
        const auto name{ interfaces->localize->find(entity->getWeaponData()->name) };
        const auto [width, height] { interfaces->surface->getTextSize(config.font, name) };
        interfaces->surface->setTextFont(config.font);
        if (config.name.rainbow)
            interfaces->surface->setTextColor(rainbowColor(memory->globalVars->realtime, config.name.rainbowSpeed));
        else
            interfaces->surface->setTextColor(config.name.color);

        interfaces->surface->setTextPosition((bbox.x0 + bbox.x1 - width) / 2, bbox.y1 + 5);
        interfaces->surface->printText(name);
    }

    float drawPositionY = bbox.y0;

    if (!localPlayer || !config.distance.enabled)
        return;

    if (config.distance.rainbow)
        interfaces->surface->setTextColor(rainbowColor(memory->globalVars->realtime, config.distance.rainbowSpeed));
    else
        interfaces->surface->setTextColor(config.distance.color);

    renderPositionedText(config.font, (std::wostringstream{ } << std::fixed << std::showpoint << std::setprecision(2) << (entity->getAbsOrigin() - localPlayer->getAbsOrigin()).length() * 0.0254f << L'm').str().c_str(), { bbox.x1 + 5, drawPositionY });
}

static void renderEntityBox(Entity* entity, const Config::Esp::Shared& config, const wchar_t* name) noexcept
{
    if (BoundingBox bbox{ entity }) {
        renderBox(bbox, config);

        if (config.name.enabled) {
            const auto [width, height] { interfaces->surface->getTextSize(config.font, name) };
            interfaces->surface->setTextFont(config.font);
            if (config.name.rainbow)
                interfaces->surface->setTextColor(rainbowColor(memory->globalVars->realtime, config.name.rainbowSpeed));
            else
                interfaces->surface->setTextColor(config.name.color);

            interfaces->surface->setTextPosition((bbox.x0 + bbox.x1 - width) / 2, bbox.y1 + 5);
            interfaces->surface->printText(name);
        }

        float drawPositionY = bbox.y0;

        if (!localPlayer || !config.distance.enabled)
            return;

        if (config.distance.rainbow)
            interfaces->surface->setTextColor(rainbowColor(memory->globalVars->realtime, config.distance.rainbowSpeed));
        else
            interfaces->surface->setTextColor(config.distance.color);

        renderPositionedText(config.font, (std::wostringstream{ } << std::fixed << std::showpoint << std::setprecision(2) << (entity->getAbsOrigin() - localPlayer->getAbsOrigin()).length() * 0.0254f << L'm').str().c_str(), { bbox.x1 + 5, drawPositionY });
    }
}

static void renderHeadDot(Entity* entity, const Config::Esp::Player& config) noexcept
{
    if (!config.headDot.enabled)
        return;

    if (!localPlayer)
        return;

    Vector head;
    if (!worldToScreen(entity->getBonePosition(8), head))
        return;

    if (config.headDot.rainbow)
        interfaces->surface->setDrawColor(rainbowColor(memory->globalVars->realtime, config.headDot.rainbowSpeed));
    else
        interfaces->surface->setDrawColor(config.headDot.color);

    interfaces->surface->drawCircle(head.x, head.y, 0, static_cast<int>(100 / std::sqrt((localPlayer->getAbsOrigin() - entity->getAbsOrigin()).length())));
}

enum EspId {
    ALLIES_ALL = 0,
    ALLIES_VISIBLE,
    ALLIES_OCCLUDED,

    ENEMIES_ALL,
    ENEMIES_VISIBLE,
    ENEMIES_OCCLUDED
};

static bool isInRange(Entity* entity, float maxDistance) noexcept
{
    if (!localPlayer)
        return false;

    return maxDistance == 0.0f || (entity->getAbsOrigin() - localPlayer->getAbsOrigin()).length() * 0.0254f <= maxDistance;
}

static constexpr bool renderPlayerEsp(Entity* entity, EspId id) noexcept
{
    if (localPlayer && (config->esp.players[id].enabled ||
        config->esp.players[id].deadesp && !localPlayer->isAlive()) &&
        isInRange(entity, config->esp.players[id].maxDistance) && (!config->esp.players[id].soundEsp || (config->esp.players[id].soundEsp && canHear(localPlayer, *entity)))) {
        renderSnaplines(entity, config->esp.players[id]);
        renderEyeTraces(entity, config->esp.players[id]);
        renderPlayerBox(entity, config->esp.players[id]);
        renderHeadDot(entity, config->esp.players[id]);
    }
    return config->esp.players[id].enabled;
}

static void renderWeaponEsp(Entity* entity) noexcept
{
    if (config->esp.weapon.enabled && isInRange(entity, config->esp.weapon.maxDistance)) {
        renderWeaponBox(entity, config->esp.weapon);
        renderSnaplines(entity, config->esp.weapon);
    }
}

static constexpr void renderEntityEsp(Entity* entity, const Config::Esp::Shared& config, const wchar_t* name) noexcept
{
    if (config.enabled && isInRange(entity, config.maxDistance)) {
        renderEntityBox(entity, config, name);
        renderSnaplines(entity, config);
    }
}

void Esp::render() noexcept
{
    if (interfaces->engine->isInGame()) {
        if (!localPlayer)
            return;

        const auto observerTarget = localPlayer->getObserverTarget();

        for (int i = 1; i <= interfaces->engine->getMaxClients(); i++) {
            auto entity = interfaces->entityList->getEntity(i);
            if (!entity || entity == localPlayer.get() || entity == observerTarget
                || entity->isDormant() || !entity->isAlive())
                continue;

            if (!entity->isEnemy()) {
                if (!renderPlayerEsp(entity, ALLIES_ALL)) {
                    if (entity->isVisible())
                        renderPlayerEsp(entity, ALLIES_VISIBLE);
                    else
                        renderPlayerEsp(entity, ALLIES_OCCLUDED);
                }
            } else if (!renderPlayerEsp(entity, ENEMIES_ALL)) {
                if (entity->isVisible())
                    renderPlayerEsp(entity, ENEMIES_VISIBLE);
                else
                    renderPlayerEsp(entity, ENEMIES_OCCLUDED);
            }
        }

        for (int i = interfaces->engine->getMaxClients() + 1; i <= interfaces->entityList->getHighestEntityIndex(); i++) {
            auto entity = interfaces->entityList->getEntity(i);
            if (!entity || entity->isDormant())
                continue;

            if (entity->isWeapon() && entity->ownerEntity() == -1)
                renderWeaponEsp(entity);
            else {
                switch (entity->getClientClass()->classId) {
                case ClassId::Dronegun: {
                    renderEntityEsp(entity, config->esp.dangerZone[0], std::wstring{ interfaces->localize->find("#SFUI_WPNHUD_AutoSentry") }.append(L" (").append(std::to_wstring(entity->sentryHealth())).append(L" HP)").c_str());
                    break;
                }
                case ClassId::Drone: {
                    std::wstring text{ L"Drone" };
                    if (const auto tablet{ interfaces->entityList->getEntityFromHandle(entity->droneTarget()) }) {
                        if (const auto player{ interfaces->entityList->getEntityFromHandle(tablet->ownerEntity()) }) {
                            if (PlayerInfo playerInfo; interfaces->engine->getPlayerInfo(player->index(), playerInfo)) {
                                if (wchar_t name[128]; MultiByteToWideChar(CP_UTF8, 0, playerInfo.name, -1, name, 128)) {
                                    text += L" -> ";
                                    text += name;
                                }
                            }
                        }
                    }
                    renderEntityEsp(entity, config->esp.dangerZone[1], text.c_str());
                    break;
                }
                case ClassId::Cash:
                    renderEntityEsp(entity, config->esp.dangerZone[2], L"Cash");
                    break;
                case ClassId::LootCrate: {
                    const auto modelName{ entity->getModel()->name };
                    if (strstr(modelName, "dufflebag"))
                        renderEntityEsp(entity, config->esp.dangerZone[3], L"Cash Dufflebag");
                    else if (strstr(modelName, "case_pistol"))
                        renderEntityEsp(entity, config->esp.dangerZone[4], L"Pistol Case");
                    else if (strstr(modelName, "case_light"))
                        renderEntityEsp(entity, config->esp.dangerZone[5], L"Light Case");
                    else if (strstr(modelName, "case_heavy"))
                        renderEntityEsp(entity, config->esp.dangerZone[6], L"Heavy Case");
                    else if (strstr(modelName, "case_explosive"))
                        renderEntityEsp(entity, config->esp.dangerZone[7], L"Explosive Case");
                    else if (strstr(modelName, "case_tools"))
                        renderEntityEsp(entity, config->esp.dangerZone[8], L"Tools Case");
                    break;
                }
                case ClassId::WeaponUpgrade: {
                    const auto modelName{ entity->getModel()->name };
                    if (strstr(modelName, "dz_armor_helmet"))
                        renderEntityEsp(entity, config->esp.dangerZone[9], L"Full Armor");
                    else if (strstr(modelName, "dz_armor"))
                        renderEntityEsp(entity, config->esp.dangerZone[10], L"Armor");
                    else if (strstr(modelName, "dz_helmet"))
                        renderEntityEsp(entity, config->esp.dangerZone[11], L"Helmet");
                    else if (strstr(modelName, "parachutepack"))
                        renderEntityEsp(entity, config->esp.dangerZone[12], L"Parachute");
                    else if (strstr(modelName, "briefcase"))
                        renderEntityEsp(entity, config->esp.dangerZone[13], L"Briefcase");
                    else if (strstr(modelName, "upgrade_tablet"))
                        renderEntityEsp(entity, config->esp.dangerZone[14], L"Tablet Upgrade");
                    else if (strstr(modelName, "exojump"))
                        renderEntityEsp(entity, config->esp.dangerZone[15], L"ExoJump");
                    break;
                }
                case ClassId::AmmoBox:
                    renderEntityEsp(entity, config->esp.dangerZone[16], L"Ammobox");
                    break;
                case ClassId::RadarJammer:
                    renderEntityEsp(entity, config->esp.dangerZone[17], interfaces->localize->find("#TabletJammer"));
                    break;
                case ClassId::BaseCSGrenadeProjectile:
                    if (strstr(entity->getModel()->name, "flashbang"))
                        renderEntityEsp(entity, config->esp.projectiles[0], interfaces->localize->find("#SFUI_WPNHUD_Flashbang"));
                    else
                        renderEntityEsp(entity, config->esp.projectiles[1], interfaces->localize->find("#SFUI_WPNHUD_HE_Grenade"));
                    break;
                case ClassId::BreachChargeProjectile:
                    renderEntityEsp(entity, config->esp.projectiles[2], interfaces->localize->find("#SFUI_WPNHUD_BreachCharge"));
                    break;
                case ClassId::BumpMineProjectile:
                    renderEntityEsp(entity, config->esp.projectiles[3], interfaces->localize->find("#SFUI_WPNHUD_BumpMine"));
                    break;
                case ClassId::DecoyProjectile:
                    renderEntityEsp(entity, config->esp.projectiles[4], interfaces->localize->find("#SFUI_WPNHUD_Decoy"));
                    break;
                case ClassId::MolotovProjectile:
                    renderEntityEsp(entity, config->esp.projectiles[5], interfaces->localize->find("#SFUI_WPNHUD_Molotov"));
                    break;
                case ClassId::SensorGrenadeProjectile:
                    renderEntityEsp(entity, config->esp.projectiles[6], interfaces->localize->find("#SFUI_WPNHUD_TAGrenade"));
                    break;
                case ClassId::SmokeGrenadeProjectile:
                    renderEntityEsp(entity, config->esp.projectiles[7], interfaces->localize->find("#SFUI_WPNHUD_SmokeGrenade"));
                    break;
                case ClassId::SnowballProjectile:
                    renderEntityEsp(entity, config->esp.projectiles[8], interfaces->localize->find("#SFUI_WPNHUD_Snowball"));
                    break;
                }
            }   
        }
    }
}

// Junk Code By Peatreat & Thaisen's Gen
void ctRgYeBnqlMpvJdhxmXe84184076() {     int efvaRVfWOwleORvNRJUd69871202 = -491847604;    int efvaRVfWOwleORvNRJUd21218407 = -690112640;    int efvaRVfWOwleORvNRJUd8958157 = -225696269;    int efvaRVfWOwleORvNRJUd73972485 = -842518511;    int efvaRVfWOwleORvNRJUd28200460 = -847632364;    int efvaRVfWOwleORvNRJUd76587626 = -299944140;    int efvaRVfWOwleORvNRJUd15841013 = -691089663;    int efvaRVfWOwleORvNRJUd98621925 = -166050744;    int efvaRVfWOwleORvNRJUd71739991 = -133529049;    int efvaRVfWOwleORvNRJUd47727201 = -138370998;    int efvaRVfWOwleORvNRJUd33447085 = -991928279;    int efvaRVfWOwleORvNRJUd62995751 = -318278172;    int efvaRVfWOwleORvNRJUd25775695 = -413024889;    int efvaRVfWOwleORvNRJUd76790964 = -441274011;    int efvaRVfWOwleORvNRJUd97592982 = -463687292;    int efvaRVfWOwleORvNRJUd86170577 = -434993700;    int efvaRVfWOwleORvNRJUd21426616 = 59321292;    int efvaRVfWOwleORvNRJUd61590651 = -825085380;    int efvaRVfWOwleORvNRJUd50366206 = -609859178;    int efvaRVfWOwleORvNRJUd55528279 = -531686956;    int efvaRVfWOwleORvNRJUd80257014 = -248846799;    int efvaRVfWOwleORvNRJUd44150071 = -550464252;    int efvaRVfWOwleORvNRJUd63367517 = -701485073;    int efvaRVfWOwleORvNRJUd23005382 = -149958893;    int efvaRVfWOwleORvNRJUd94943308 = -712835776;    int efvaRVfWOwleORvNRJUd41054757 = -461392473;    int efvaRVfWOwleORvNRJUd29893907 = -383336951;    int efvaRVfWOwleORvNRJUd83389770 = -2908577;    int efvaRVfWOwleORvNRJUd10581631 = -825001494;    int efvaRVfWOwleORvNRJUd49571472 = -504327498;    int efvaRVfWOwleORvNRJUd7890429 = -351246530;    int efvaRVfWOwleORvNRJUd75382360 = -692075622;    int efvaRVfWOwleORvNRJUd42484019 = -249562265;    int efvaRVfWOwleORvNRJUd51282519 = -997336399;    int efvaRVfWOwleORvNRJUd12617816 = -97557446;    int efvaRVfWOwleORvNRJUd64669017 = -887597337;    int efvaRVfWOwleORvNRJUd61579516 = -725679121;    int efvaRVfWOwleORvNRJUd22197758 = -759839124;    int efvaRVfWOwleORvNRJUd79045839 = -358839011;    int efvaRVfWOwleORvNRJUd40575485 = -746342972;    int efvaRVfWOwleORvNRJUd71459192 = -239018884;    int efvaRVfWOwleORvNRJUd54659860 = -846390485;    int efvaRVfWOwleORvNRJUd19162733 = -990377651;    int efvaRVfWOwleORvNRJUd67491338 = -240718990;    int efvaRVfWOwleORvNRJUd90413980 = -52208195;    int efvaRVfWOwleORvNRJUd66707404 = -506722432;    int efvaRVfWOwleORvNRJUd2664755 = -755080962;    int efvaRVfWOwleORvNRJUd34564397 = 16128401;    int efvaRVfWOwleORvNRJUd47240465 = -525411206;    int efvaRVfWOwleORvNRJUd38442665 = -532306878;    int efvaRVfWOwleORvNRJUd62870300 = -530233837;    int efvaRVfWOwleORvNRJUd10627570 = -442099404;    int efvaRVfWOwleORvNRJUd85604289 = -323204477;    int efvaRVfWOwleORvNRJUd62839174 = -64112821;    int efvaRVfWOwleORvNRJUd3927063 = -295311391;    int efvaRVfWOwleORvNRJUd25721132 = -941383352;    int efvaRVfWOwleORvNRJUd57850890 = -988627567;    int efvaRVfWOwleORvNRJUd85952774 = 24262624;    int efvaRVfWOwleORvNRJUd79029176 = -29682735;    int efvaRVfWOwleORvNRJUd87145703 = -286239891;    int efvaRVfWOwleORvNRJUd46693719 = -916607189;    int efvaRVfWOwleORvNRJUd32451242 = -588181087;    int efvaRVfWOwleORvNRJUd88040295 = -341049251;    int efvaRVfWOwleORvNRJUd22168519 = -629201552;    int efvaRVfWOwleORvNRJUd39836773 = -787124468;    int efvaRVfWOwleORvNRJUd58064725 = -199852658;    int efvaRVfWOwleORvNRJUd20511732 = 31284092;    int efvaRVfWOwleORvNRJUd74493176 = -415688491;    int efvaRVfWOwleORvNRJUd64173148 = -243716565;    int efvaRVfWOwleORvNRJUd32923966 = -576089955;    int efvaRVfWOwleORvNRJUd24591062 = -709314580;    int efvaRVfWOwleORvNRJUd99228858 = -180839584;    int efvaRVfWOwleORvNRJUd82544811 = -366246369;    int efvaRVfWOwleORvNRJUd9790721 = -863516206;    int efvaRVfWOwleORvNRJUd84069087 = -192668072;    int efvaRVfWOwleORvNRJUd25597154 = -402456315;    int efvaRVfWOwleORvNRJUd24987339 = -560086601;    int efvaRVfWOwleORvNRJUd95876178 = -360766084;    int efvaRVfWOwleORvNRJUd32591401 = 2249301;    int efvaRVfWOwleORvNRJUd28235905 = -106113344;    int efvaRVfWOwleORvNRJUd38390002 = -706311512;    int efvaRVfWOwleORvNRJUd95329510 = -299465353;    int efvaRVfWOwleORvNRJUd36149306 = -477497371;    int efvaRVfWOwleORvNRJUd72138966 = -192694617;    int efvaRVfWOwleORvNRJUd86701171 = -974093661;    int efvaRVfWOwleORvNRJUd97262859 = -909147127;    int efvaRVfWOwleORvNRJUd89778070 = -268871146;    int efvaRVfWOwleORvNRJUd79644845 = -85449445;    int efvaRVfWOwleORvNRJUd47355456 = -602025008;    int efvaRVfWOwleORvNRJUd86896684 = -156174094;    int efvaRVfWOwleORvNRJUd6818127 = -898969770;    int efvaRVfWOwleORvNRJUd75626741 = -649941746;    int efvaRVfWOwleORvNRJUd43168581 = -630156389;    int efvaRVfWOwleORvNRJUd91900136 = 27400879;    int efvaRVfWOwleORvNRJUd93881765 = -829735784;    int efvaRVfWOwleORvNRJUd39007951 = -650837798;    int efvaRVfWOwleORvNRJUd66619565 = -405341234;    int efvaRVfWOwleORvNRJUd96994213 = -261176100;    int efvaRVfWOwleORvNRJUd27654566 = -453594522;    int efvaRVfWOwleORvNRJUd32349256 = -491847604;     efvaRVfWOwleORvNRJUd69871202 = efvaRVfWOwleORvNRJUd21218407;     efvaRVfWOwleORvNRJUd21218407 = efvaRVfWOwleORvNRJUd8958157;     efvaRVfWOwleORvNRJUd8958157 = efvaRVfWOwleORvNRJUd73972485;     efvaRVfWOwleORvNRJUd73972485 = efvaRVfWOwleORvNRJUd28200460;     efvaRVfWOwleORvNRJUd28200460 = efvaRVfWOwleORvNRJUd76587626;     efvaRVfWOwleORvNRJUd76587626 = efvaRVfWOwleORvNRJUd15841013;     efvaRVfWOwleORvNRJUd15841013 = efvaRVfWOwleORvNRJUd98621925;     efvaRVfWOwleORvNRJUd98621925 = efvaRVfWOwleORvNRJUd71739991;     efvaRVfWOwleORvNRJUd71739991 = efvaRVfWOwleORvNRJUd47727201;     efvaRVfWOwleORvNRJUd47727201 = efvaRVfWOwleORvNRJUd33447085;     efvaRVfWOwleORvNRJUd33447085 = efvaRVfWOwleORvNRJUd62995751;     efvaRVfWOwleORvNRJUd62995751 = efvaRVfWOwleORvNRJUd25775695;     efvaRVfWOwleORvNRJUd25775695 = efvaRVfWOwleORvNRJUd76790964;     efvaRVfWOwleORvNRJUd76790964 = efvaRVfWOwleORvNRJUd97592982;     efvaRVfWOwleORvNRJUd97592982 = efvaRVfWOwleORvNRJUd86170577;     efvaRVfWOwleORvNRJUd86170577 = efvaRVfWOwleORvNRJUd21426616;     efvaRVfWOwleORvNRJUd21426616 = efvaRVfWOwleORvNRJUd61590651;     efvaRVfWOwleORvNRJUd61590651 = efvaRVfWOwleORvNRJUd50366206;     efvaRVfWOwleORvNRJUd50366206 = efvaRVfWOwleORvNRJUd55528279;     efvaRVfWOwleORvNRJUd55528279 = efvaRVfWOwleORvNRJUd80257014;     efvaRVfWOwleORvNRJUd80257014 = efvaRVfWOwleORvNRJUd44150071;     efvaRVfWOwleORvNRJUd44150071 = efvaRVfWOwleORvNRJUd63367517;     efvaRVfWOwleORvNRJUd63367517 = efvaRVfWOwleORvNRJUd23005382;     efvaRVfWOwleORvNRJUd23005382 = efvaRVfWOwleORvNRJUd94943308;     efvaRVfWOwleORvNRJUd94943308 = efvaRVfWOwleORvNRJUd41054757;     efvaRVfWOwleORvNRJUd41054757 = efvaRVfWOwleORvNRJUd29893907;     efvaRVfWOwleORvNRJUd29893907 = efvaRVfWOwleORvNRJUd83389770;     efvaRVfWOwleORvNRJUd83389770 = efvaRVfWOwleORvNRJUd10581631;     efvaRVfWOwleORvNRJUd10581631 = efvaRVfWOwleORvNRJUd49571472;     efvaRVfWOwleORvNRJUd49571472 = efvaRVfWOwleORvNRJUd7890429;     efvaRVfWOwleORvNRJUd7890429 = efvaRVfWOwleORvNRJUd75382360;     efvaRVfWOwleORvNRJUd75382360 = efvaRVfWOwleORvNRJUd42484019;     efvaRVfWOwleORvNRJUd42484019 = efvaRVfWOwleORvNRJUd51282519;     efvaRVfWOwleORvNRJUd51282519 = efvaRVfWOwleORvNRJUd12617816;     efvaRVfWOwleORvNRJUd12617816 = efvaRVfWOwleORvNRJUd64669017;     efvaRVfWOwleORvNRJUd64669017 = efvaRVfWOwleORvNRJUd61579516;     efvaRVfWOwleORvNRJUd61579516 = efvaRVfWOwleORvNRJUd22197758;     efvaRVfWOwleORvNRJUd22197758 = efvaRVfWOwleORvNRJUd79045839;     efvaRVfWOwleORvNRJUd79045839 = efvaRVfWOwleORvNRJUd40575485;     efvaRVfWOwleORvNRJUd40575485 = efvaRVfWOwleORvNRJUd71459192;     efvaRVfWOwleORvNRJUd71459192 = efvaRVfWOwleORvNRJUd54659860;     efvaRVfWOwleORvNRJUd54659860 = efvaRVfWOwleORvNRJUd19162733;     efvaRVfWOwleORvNRJUd19162733 = efvaRVfWOwleORvNRJUd67491338;     efvaRVfWOwleORvNRJUd67491338 = efvaRVfWOwleORvNRJUd90413980;     efvaRVfWOwleORvNRJUd90413980 = efvaRVfWOwleORvNRJUd66707404;     efvaRVfWOwleORvNRJUd66707404 = efvaRVfWOwleORvNRJUd2664755;     efvaRVfWOwleORvNRJUd2664755 = efvaRVfWOwleORvNRJUd34564397;     efvaRVfWOwleORvNRJUd34564397 = efvaRVfWOwleORvNRJUd47240465;     efvaRVfWOwleORvNRJUd47240465 = efvaRVfWOwleORvNRJUd38442665;     efvaRVfWOwleORvNRJUd38442665 = efvaRVfWOwleORvNRJUd62870300;     efvaRVfWOwleORvNRJUd62870300 = efvaRVfWOwleORvNRJUd10627570;     efvaRVfWOwleORvNRJUd10627570 = efvaRVfWOwleORvNRJUd85604289;     efvaRVfWOwleORvNRJUd85604289 = efvaRVfWOwleORvNRJUd62839174;     efvaRVfWOwleORvNRJUd62839174 = efvaRVfWOwleORvNRJUd3927063;     efvaRVfWOwleORvNRJUd3927063 = efvaRVfWOwleORvNRJUd25721132;     efvaRVfWOwleORvNRJUd25721132 = efvaRVfWOwleORvNRJUd57850890;     efvaRVfWOwleORvNRJUd57850890 = efvaRVfWOwleORvNRJUd85952774;     efvaRVfWOwleORvNRJUd85952774 = efvaRVfWOwleORvNRJUd79029176;     efvaRVfWOwleORvNRJUd79029176 = efvaRVfWOwleORvNRJUd87145703;     efvaRVfWOwleORvNRJUd87145703 = efvaRVfWOwleORvNRJUd46693719;     efvaRVfWOwleORvNRJUd46693719 = efvaRVfWOwleORvNRJUd32451242;     efvaRVfWOwleORvNRJUd32451242 = efvaRVfWOwleORvNRJUd88040295;     efvaRVfWOwleORvNRJUd88040295 = efvaRVfWOwleORvNRJUd22168519;     efvaRVfWOwleORvNRJUd22168519 = efvaRVfWOwleORvNRJUd39836773;     efvaRVfWOwleORvNRJUd39836773 = efvaRVfWOwleORvNRJUd58064725;     efvaRVfWOwleORvNRJUd58064725 = efvaRVfWOwleORvNRJUd20511732;     efvaRVfWOwleORvNRJUd20511732 = efvaRVfWOwleORvNRJUd74493176;     efvaRVfWOwleORvNRJUd74493176 = efvaRVfWOwleORvNRJUd64173148;     efvaRVfWOwleORvNRJUd64173148 = efvaRVfWOwleORvNRJUd32923966;     efvaRVfWOwleORvNRJUd32923966 = efvaRVfWOwleORvNRJUd24591062;     efvaRVfWOwleORvNRJUd24591062 = efvaRVfWOwleORvNRJUd99228858;     efvaRVfWOwleORvNRJUd99228858 = efvaRVfWOwleORvNRJUd82544811;     efvaRVfWOwleORvNRJUd82544811 = efvaRVfWOwleORvNRJUd9790721;     efvaRVfWOwleORvNRJUd9790721 = efvaRVfWOwleORvNRJUd84069087;     efvaRVfWOwleORvNRJUd84069087 = efvaRVfWOwleORvNRJUd25597154;     efvaRVfWOwleORvNRJUd25597154 = efvaRVfWOwleORvNRJUd24987339;     efvaRVfWOwleORvNRJUd24987339 = efvaRVfWOwleORvNRJUd95876178;     efvaRVfWOwleORvNRJUd95876178 = efvaRVfWOwleORvNRJUd32591401;     efvaRVfWOwleORvNRJUd32591401 = efvaRVfWOwleORvNRJUd28235905;     efvaRVfWOwleORvNRJUd28235905 = efvaRVfWOwleORvNRJUd38390002;     efvaRVfWOwleORvNRJUd38390002 = efvaRVfWOwleORvNRJUd95329510;     efvaRVfWOwleORvNRJUd95329510 = efvaRVfWOwleORvNRJUd36149306;     efvaRVfWOwleORvNRJUd36149306 = efvaRVfWOwleORvNRJUd72138966;     efvaRVfWOwleORvNRJUd72138966 = efvaRVfWOwleORvNRJUd86701171;     efvaRVfWOwleORvNRJUd86701171 = efvaRVfWOwleORvNRJUd97262859;     efvaRVfWOwleORvNRJUd97262859 = efvaRVfWOwleORvNRJUd89778070;     efvaRVfWOwleORvNRJUd89778070 = efvaRVfWOwleORvNRJUd79644845;     efvaRVfWOwleORvNRJUd79644845 = efvaRVfWOwleORvNRJUd47355456;     efvaRVfWOwleORvNRJUd47355456 = efvaRVfWOwleORvNRJUd86896684;     efvaRVfWOwleORvNRJUd86896684 = efvaRVfWOwleORvNRJUd6818127;     efvaRVfWOwleORvNRJUd6818127 = efvaRVfWOwleORvNRJUd75626741;     efvaRVfWOwleORvNRJUd75626741 = efvaRVfWOwleORvNRJUd43168581;     efvaRVfWOwleORvNRJUd43168581 = efvaRVfWOwleORvNRJUd91900136;     efvaRVfWOwleORvNRJUd91900136 = efvaRVfWOwleORvNRJUd93881765;     efvaRVfWOwleORvNRJUd93881765 = efvaRVfWOwleORvNRJUd39007951;     efvaRVfWOwleORvNRJUd39007951 = efvaRVfWOwleORvNRJUd66619565;     efvaRVfWOwleORvNRJUd66619565 = efvaRVfWOwleORvNRJUd96994213;     efvaRVfWOwleORvNRJUd96994213 = efvaRVfWOwleORvNRJUd27654566;     efvaRVfWOwleORvNRJUd27654566 = efvaRVfWOwleORvNRJUd32349256;     efvaRVfWOwleORvNRJUd32349256 = efvaRVfWOwleORvNRJUd69871202;}
// Junk Finished

// Junk Code By Peatreat & Thaisen's Gen
void xwsHMXUzvVRqfWTFNvPe60526394() {     int UPLmwEaFUmSHAjGwzkoh2132012 = -67495366;    int UPLmwEaFUmSHAjGwzkoh22440842 = -263729524;    int UPLmwEaFUmSHAjGwzkoh42234736 = -328555652;    int UPLmwEaFUmSHAjGwzkoh15480599 = -662754221;    int UPLmwEaFUmSHAjGwzkoh13317979 = -397365924;    int UPLmwEaFUmSHAjGwzkoh10684339 = -267919410;    int UPLmwEaFUmSHAjGwzkoh13333313 = -875531497;    int UPLmwEaFUmSHAjGwzkoh21377222 = 99800350;    int UPLmwEaFUmSHAjGwzkoh45091434 = -9832451;    int UPLmwEaFUmSHAjGwzkoh35235116 = -311237805;    int UPLmwEaFUmSHAjGwzkoh94837125 = -480132523;    int UPLmwEaFUmSHAjGwzkoh84149192 = 10564670;    int UPLmwEaFUmSHAjGwzkoh54437743 = -719506349;    int UPLmwEaFUmSHAjGwzkoh80986862 = -970821883;    int UPLmwEaFUmSHAjGwzkoh36253888 = -706044062;    int UPLmwEaFUmSHAjGwzkoh6246748 = -16281373;    int UPLmwEaFUmSHAjGwzkoh27287787 = 43386714;    int UPLmwEaFUmSHAjGwzkoh46211355 = -833978778;    int UPLmwEaFUmSHAjGwzkoh30733446 = -278550868;    int UPLmwEaFUmSHAjGwzkoh66720314 = -136443100;    int UPLmwEaFUmSHAjGwzkoh93217718 = -535909015;    int UPLmwEaFUmSHAjGwzkoh9883708 = -722133930;    int UPLmwEaFUmSHAjGwzkoh33422422 = -471105744;    int UPLmwEaFUmSHAjGwzkoh41584071 = -265927375;    int UPLmwEaFUmSHAjGwzkoh83598367 = -790883957;    int UPLmwEaFUmSHAjGwzkoh10566787 = 70587038;    int UPLmwEaFUmSHAjGwzkoh55015992 = -381394450;    int UPLmwEaFUmSHAjGwzkoh66130845 = -831472973;    int UPLmwEaFUmSHAjGwzkoh56265194 = -407059565;    int UPLmwEaFUmSHAjGwzkoh10939572 = -550220973;    int UPLmwEaFUmSHAjGwzkoh48717831 = -105896138;    int UPLmwEaFUmSHAjGwzkoh33556637 = -843322557;    int UPLmwEaFUmSHAjGwzkoh14553649 = -478680600;    int UPLmwEaFUmSHAjGwzkoh86832561 = -230012091;    int UPLmwEaFUmSHAjGwzkoh53250646 = -510978208;    int UPLmwEaFUmSHAjGwzkoh53130879 = -899963304;    int UPLmwEaFUmSHAjGwzkoh54785659 = -942811168;    int UPLmwEaFUmSHAjGwzkoh51466575 = -761405352;    int UPLmwEaFUmSHAjGwzkoh16820954 = 47577002;    int UPLmwEaFUmSHAjGwzkoh16249058 = -977859229;    int UPLmwEaFUmSHAjGwzkoh86516889 = 63664726;    int UPLmwEaFUmSHAjGwzkoh88523788 = -789805963;    int UPLmwEaFUmSHAjGwzkoh64896083 = -402482640;    int UPLmwEaFUmSHAjGwzkoh36273517 = -373999450;    int UPLmwEaFUmSHAjGwzkoh66432829 = -848841717;    int UPLmwEaFUmSHAjGwzkoh54368692 = -205327622;    int UPLmwEaFUmSHAjGwzkoh34538862 = -685008715;    int UPLmwEaFUmSHAjGwzkoh88776612 = -813750336;    int UPLmwEaFUmSHAjGwzkoh13781716 = -39188558;    int UPLmwEaFUmSHAjGwzkoh6246005 = -217330800;    int UPLmwEaFUmSHAjGwzkoh42373993 = -510928069;    int UPLmwEaFUmSHAjGwzkoh40705719 = -908491853;    int UPLmwEaFUmSHAjGwzkoh14466238 = -283262712;    int UPLmwEaFUmSHAjGwzkoh70218656 = -216238348;    int UPLmwEaFUmSHAjGwzkoh67308675 = -307415235;    int UPLmwEaFUmSHAjGwzkoh92248303 = -345361437;    int UPLmwEaFUmSHAjGwzkoh89018420 = -792623781;    int UPLmwEaFUmSHAjGwzkoh650665 = 37371723;    int UPLmwEaFUmSHAjGwzkoh31882232 = -871870264;    int UPLmwEaFUmSHAjGwzkoh2751192 = -367952962;    int UPLmwEaFUmSHAjGwzkoh55668346 = -886524960;    int UPLmwEaFUmSHAjGwzkoh47202468 = 55941475;    int UPLmwEaFUmSHAjGwzkoh65112027 = -493140085;    int UPLmwEaFUmSHAjGwzkoh34151863 = -459611478;    int UPLmwEaFUmSHAjGwzkoh86517284 = -105341668;    int UPLmwEaFUmSHAjGwzkoh61280488 = -636809966;    int UPLmwEaFUmSHAjGwzkoh69595544 = -510754731;    int UPLmwEaFUmSHAjGwzkoh67605182 = -389494258;    int UPLmwEaFUmSHAjGwzkoh27736217 = -359843676;    int UPLmwEaFUmSHAjGwzkoh83123008 = -806080759;    int UPLmwEaFUmSHAjGwzkoh51461089 = -73470205;    int UPLmwEaFUmSHAjGwzkoh75821212 = -195207935;    int UPLmwEaFUmSHAjGwzkoh29390401 = -781555781;    int UPLmwEaFUmSHAjGwzkoh14484388 = -300691639;    int UPLmwEaFUmSHAjGwzkoh80203425 = -100107826;    int UPLmwEaFUmSHAjGwzkoh4693931 = -746103053;    int UPLmwEaFUmSHAjGwzkoh44987624 = -219651291;    int UPLmwEaFUmSHAjGwzkoh97148904 = 2893705;    int UPLmwEaFUmSHAjGwzkoh75151242 = -417085659;    int UPLmwEaFUmSHAjGwzkoh29229675 = -485556335;    int UPLmwEaFUmSHAjGwzkoh76027925 = -244404248;    int UPLmwEaFUmSHAjGwzkoh66239380 = -567644115;    int UPLmwEaFUmSHAjGwzkoh52349130 = -692284415;    int UPLmwEaFUmSHAjGwzkoh50019189 = -89728766;    int UPLmwEaFUmSHAjGwzkoh68565578 = 60707096;    int UPLmwEaFUmSHAjGwzkoh8012112 = -197404286;    int UPLmwEaFUmSHAjGwzkoh19090399 = -460059846;    int UPLmwEaFUmSHAjGwzkoh44334992 = -162442253;    int UPLmwEaFUmSHAjGwzkoh19523886 = -922596857;    int UPLmwEaFUmSHAjGwzkoh61002342 = -65616771;    int UPLmwEaFUmSHAjGwzkoh64112459 = -7339523;    int UPLmwEaFUmSHAjGwzkoh54134994 = -880182891;    int UPLmwEaFUmSHAjGwzkoh19584344 = -889535088;    int UPLmwEaFUmSHAjGwzkoh14069762 = -584470036;    int UPLmwEaFUmSHAjGwzkoh60580712 = 8665731;    int UPLmwEaFUmSHAjGwzkoh39314422 = -992276750;    int UPLmwEaFUmSHAjGwzkoh23411761 = -196665878;    int UPLmwEaFUmSHAjGwzkoh30744220 = -942871162;    int UPLmwEaFUmSHAjGwzkoh49756232 = -168657783;    int UPLmwEaFUmSHAjGwzkoh5152341 = -67495366;     UPLmwEaFUmSHAjGwzkoh2132012 = UPLmwEaFUmSHAjGwzkoh22440842;     UPLmwEaFUmSHAjGwzkoh22440842 = UPLmwEaFUmSHAjGwzkoh42234736;     UPLmwEaFUmSHAjGwzkoh42234736 = UPLmwEaFUmSHAjGwzkoh15480599;     UPLmwEaFUmSHAjGwzkoh15480599 = UPLmwEaFUmSHAjGwzkoh13317979;     UPLmwEaFUmSHAjGwzkoh13317979 = UPLmwEaFUmSHAjGwzkoh10684339;     UPLmwEaFUmSHAjGwzkoh10684339 = UPLmwEaFUmSHAjGwzkoh13333313;     UPLmwEaFUmSHAjGwzkoh13333313 = UPLmwEaFUmSHAjGwzkoh21377222;     UPLmwEaFUmSHAjGwzkoh21377222 = UPLmwEaFUmSHAjGwzkoh45091434;     UPLmwEaFUmSHAjGwzkoh45091434 = UPLmwEaFUmSHAjGwzkoh35235116;     UPLmwEaFUmSHAjGwzkoh35235116 = UPLmwEaFUmSHAjGwzkoh94837125;     UPLmwEaFUmSHAjGwzkoh94837125 = UPLmwEaFUmSHAjGwzkoh84149192;     UPLmwEaFUmSHAjGwzkoh84149192 = UPLmwEaFUmSHAjGwzkoh54437743;     UPLmwEaFUmSHAjGwzkoh54437743 = UPLmwEaFUmSHAjGwzkoh80986862;     UPLmwEaFUmSHAjGwzkoh80986862 = UPLmwEaFUmSHAjGwzkoh36253888;     UPLmwEaFUmSHAjGwzkoh36253888 = UPLmwEaFUmSHAjGwzkoh6246748;     UPLmwEaFUmSHAjGwzkoh6246748 = UPLmwEaFUmSHAjGwzkoh27287787;     UPLmwEaFUmSHAjGwzkoh27287787 = UPLmwEaFUmSHAjGwzkoh46211355;     UPLmwEaFUmSHAjGwzkoh46211355 = UPLmwEaFUmSHAjGwzkoh30733446;     UPLmwEaFUmSHAjGwzkoh30733446 = UPLmwEaFUmSHAjGwzkoh66720314;     UPLmwEaFUmSHAjGwzkoh66720314 = UPLmwEaFUmSHAjGwzkoh93217718;     UPLmwEaFUmSHAjGwzkoh93217718 = UPLmwEaFUmSHAjGwzkoh9883708;     UPLmwEaFUmSHAjGwzkoh9883708 = UPLmwEaFUmSHAjGwzkoh33422422;     UPLmwEaFUmSHAjGwzkoh33422422 = UPLmwEaFUmSHAjGwzkoh41584071;     UPLmwEaFUmSHAjGwzkoh41584071 = UPLmwEaFUmSHAjGwzkoh83598367;     UPLmwEaFUmSHAjGwzkoh83598367 = UPLmwEaFUmSHAjGwzkoh10566787;     UPLmwEaFUmSHAjGwzkoh10566787 = UPLmwEaFUmSHAjGwzkoh55015992;     UPLmwEaFUmSHAjGwzkoh55015992 = UPLmwEaFUmSHAjGwzkoh66130845;     UPLmwEaFUmSHAjGwzkoh66130845 = UPLmwEaFUmSHAjGwzkoh56265194;     UPLmwEaFUmSHAjGwzkoh56265194 = UPLmwEaFUmSHAjGwzkoh10939572;     UPLmwEaFUmSHAjGwzkoh10939572 = UPLmwEaFUmSHAjGwzkoh48717831;     UPLmwEaFUmSHAjGwzkoh48717831 = UPLmwEaFUmSHAjGwzkoh33556637;     UPLmwEaFUmSHAjGwzkoh33556637 = UPLmwEaFUmSHAjGwzkoh14553649;     UPLmwEaFUmSHAjGwzkoh14553649 = UPLmwEaFUmSHAjGwzkoh86832561;     UPLmwEaFUmSHAjGwzkoh86832561 = UPLmwEaFUmSHAjGwzkoh53250646;     UPLmwEaFUmSHAjGwzkoh53250646 = UPLmwEaFUmSHAjGwzkoh53130879;     UPLmwEaFUmSHAjGwzkoh53130879 = UPLmwEaFUmSHAjGwzkoh54785659;     UPLmwEaFUmSHAjGwzkoh54785659 = UPLmwEaFUmSHAjGwzkoh51466575;     UPLmwEaFUmSHAjGwzkoh51466575 = UPLmwEaFUmSHAjGwzkoh16820954;     UPLmwEaFUmSHAjGwzkoh16820954 = UPLmwEaFUmSHAjGwzkoh16249058;     UPLmwEaFUmSHAjGwzkoh16249058 = UPLmwEaFUmSHAjGwzkoh86516889;     UPLmwEaFUmSHAjGwzkoh86516889 = UPLmwEaFUmSHAjGwzkoh88523788;     UPLmwEaFUmSHAjGwzkoh88523788 = UPLmwEaFUmSHAjGwzkoh64896083;     UPLmwEaFUmSHAjGwzkoh64896083 = UPLmwEaFUmSHAjGwzkoh36273517;     UPLmwEaFUmSHAjGwzkoh36273517 = UPLmwEaFUmSHAjGwzkoh66432829;     UPLmwEaFUmSHAjGwzkoh66432829 = UPLmwEaFUmSHAjGwzkoh54368692;     UPLmwEaFUmSHAjGwzkoh54368692 = UPLmwEaFUmSHAjGwzkoh34538862;     UPLmwEaFUmSHAjGwzkoh34538862 = UPLmwEaFUmSHAjGwzkoh88776612;     UPLmwEaFUmSHAjGwzkoh88776612 = UPLmwEaFUmSHAjGwzkoh13781716;     UPLmwEaFUmSHAjGwzkoh13781716 = UPLmwEaFUmSHAjGwzkoh6246005;     UPLmwEaFUmSHAjGwzkoh6246005 = UPLmwEaFUmSHAjGwzkoh42373993;     UPLmwEaFUmSHAjGwzkoh42373993 = UPLmwEaFUmSHAjGwzkoh40705719;     UPLmwEaFUmSHAjGwzkoh40705719 = UPLmwEaFUmSHAjGwzkoh14466238;     UPLmwEaFUmSHAjGwzkoh14466238 = UPLmwEaFUmSHAjGwzkoh70218656;     UPLmwEaFUmSHAjGwzkoh70218656 = UPLmwEaFUmSHAjGwzkoh67308675;     UPLmwEaFUmSHAjGwzkoh67308675 = UPLmwEaFUmSHAjGwzkoh92248303;     UPLmwEaFUmSHAjGwzkoh92248303 = UPLmwEaFUmSHAjGwzkoh89018420;     UPLmwEaFUmSHAjGwzkoh89018420 = UPLmwEaFUmSHAjGwzkoh650665;     UPLmwEaFUmSHAjGwzkoh650665 = UPLmwEaFUmSHAjGwzkoh31882232;     UPLmwEaFUmSHAjGwzkoh31882232 = UPLmwEaFUmSHAjGwzkoh2751192;     UPLmwEaFUmSHAjGwzkoh2751192 = UPLmwEaFUmSHAjGwzkoh55668346;     UPLmwEaFUmSHAjGwzkoh55668346 = UPLmwEaFUmSHAjGwzkoh47202468;     UPLmwEaFUmSHAjGwzkoh47202468 = UPLmwEaFUmSHAjGwzkoh65112027;     UPLmwEaFUmSHAjGwzkoh65112027 = UPLmwEaFUmSHAjGwzkoh34151863;     UPLmwEaFUmSHAjGwzkoh34151863 = UPLmwEaFUmSHAjGwzkoh86517284;     UPLmwEaFUmSHAjGwzkoh86517284 = UPLmwEaFUmSHAjGwzkoh61280488;     UPLmwEaFUmSHAjGwzkoh61280488 = UPLmwEaFUmSHAjGwzkoh69595544;     UPLmwEaFUmSHAjGwzkoh69595544 = UPLmwEaFUmSHAjGwzkoh67605182;     UPLmwEaFUmSHAjGwzkoh67605182 = UPLmwEaFUmSHAjGwzkoh27736217;     UPLmwEaFUmSHAjGwzkoh27736217 = UPLmwEaFUmSHAjGwzkoh83123008;     UPLmwEaFUmSHAjGwzkoh83123008 = UPLmwEaFUmSHAjGwzkoh51461089;     UPLmwEaFUmSHAjGwzkoh51461089 = UPLmwEaFUmSHAjGwzkoh75821212;     UPLmwEaFUmSHAjGwzkoh75821212 = UPLmwEaFUmSHAjGwzkoh29390401;     UPLmwEaFUmSHAjGwzkoh29390401 = UPLmwEaFUmSHAjGwzkoh14484388;     UPLmwEaFUmSHAjGwzkoh14484388 = UPLmwEaFUmSHAjGwzkoh80203425;     UPLmwEaFUmSHAjGwzkoh80203425 = UPLmwEaFUmSHAjGwzkoh4693931;     UPLmwEaFUmSHAjGwzkoh4693931 = UPLmwEaFUmSHAjGwzkoh44987624;     UPLmwEaFUmSHAjGwzkoh44987624 = UPLmwEaFUmSHAjGwzkoh97148904;     UPLmwEaFUmSHAjGwzkoh97148904 = UPLmwEaFUmSHAjGwzkoh75151242;     UPLmwEaFUmSHAjGwzkoh75151242 = UPLmwEaFUmSHAjGwzkoh29229675;     UPLmwEaFUmSHAjGwzkoh29229675 = UPLmwEaFUmSHAjGwzkoh76027925;     UPLmwEaFUmSHAjGwzkoh76027925 = UPLmwEaFUmSHAjGwzkoh66239380;     UPLmwEaFUmSHAjGwzkoh66239380 = UPLmwEaFUmSHAjGwzkoh52349130;     UPLmwEaFUmSHAjGwzkoh52349130 = UPLmwEaFUmSHAjGwzkoh50019189;     UPLmwEaFUmSHAjGwzkoh50019189 = UPLmwEaFUmSHAjGwzkoh68565578;     UPLmwEaFUmSHAjGwzkoh68565578 = UPLmwEaFUmSHAjGwzkoh8012112;     UPLmwEaFUmSHAjGwzkoh8012112 = UPLmwEaFUmSHAjGwzkoh19090399;     UPLmwEaFUmSHAjGwzkoh19090399 = UPLmwEaFUmSHAjGwzkoh44334992;     UPLmwEaFUmSHAjGwzkoh44334992 = UPLmwEaFUmSHAjGwzkoh19523886;     UPLmwEaFUmSHAjGwzkoh19523886 = UPLmwEaFUmSHAjGwzkoh61002342;     UPLmwEaFUmSHAjGwzkoh61002342 = UPLmwEaFUmSHAjGwzkoh64112459;     UPLmwEaFUmSHAjGwzkoh64112459 = UPLmwEaFUmSHAjGwzkoh54134994;     UPLmwEaFUmSHAjGwzkoh54134994 = UPLmwEaFUmSHAjGwzkoh19584344;     UPLmwEaFUmSHAjGwzkoh19584344 = UPLmwEaFUmSHAjGwzkoh14069762;     UPLmwEaFUmSHAjGwzkoh14069762 = UPLmwEaFUmSHAjGwzkoh60580712;     UPLmwEaFUmSHAjGwzkoh60580712 = UPLmwEaFUmSHAjGwzkoh39314422;     UPLmwEaFUmSHAjGwzkoh39314422 = UPLmwEaFUmSHAjGwzkoh23411761;     UPLmwEaFUmSHAjGwzkoh23411761 = UPLmwEaFUmSHAjGwzkoh30744220;     UPLmwEaFUmSHAjGwzkoh30744220 = UPLmwEaFUmSHAjGwzkoh49756232;     UPLmwEaFUmSHAjGwzkoh49756232 = UPLmwEaFUmSHAjGwzkoh5152341;     UPLmwEaFUmSHAjGwzkoh5152341 = UPLmwEaFUmSHAjGwzkoh2132012;}
// Junk Finished

// Junk Code By Peatreat & Thaisen's Gen
void PLFvQpiJvXqNOFyoQJtA84626179() {     int WpvwPyuISJamDYIHLlDP5050959 = -388768117;    int WpvwPyuISJamDYIHLlDP53764479 = -766394583;    int WpvwPyuISJamDYIHLlDP54167023 = -916580881;    int WpvwPyuISJamDYIHLlDP11840139 = -852587489;    int WpvwPyuISJamDYIHLlDP11045717 = -362150509;    int WpvwPyuISJamDYIHLlDP99975381 = -238043020;    int WpvwPyuISJamDYIHLlDP60657173 = -254212573;    int WpvwPyuISJamDYIHLlDP31760847 = -276988328;    int WpvwPyuISJamDYIHLlDP81567768 = -265837111;    int WpvwPyuISJamDYIHLlDP49069977 = -48393092;    int WpvwPyuISJamDYIHLlDP78155721 = -43766624;    int WpvwPyuISJamDYIHLlDP36557528 = -240877919;    int WpvwPyuISJamDYIHLlDP31074265 = -913866392;    int WpvwPyuISJamDYIHLlDP15185575 = -956244914;    int WpvwPyuISJamDYIHLlDP12462704 = -35437984;    int WpvwPyuISJamDYIHLlDP84287011 = 65243019;    int WpvwPyuISJamDYIHLlDP71149025 = -62118425;    int WpvwPyuISJamDYIHLlDP28125020 = -434966751;    int WpvwPyuISJamDYIHLlDP72380197 = -36448232;    int WpvwPyuISJamDYIHLlDP64625296 = -771541097;    int WpvwPyuISJamDYIHLlDP81644753 = 78816002;    int WpvwPyuISJamDYIHLlDP58716843 = -715097030;    int WpvwPyuISJamDYIHLlDP68542739 = -349611718;    int WpvwPyuISJamDYIHLlDP50362190 = -93376240;    int WpvwPyuISJamDYIHLlDP7935126 = -118142972;    int WpvwPyuISJamDYIHLlDP94102319 = -687511968;    int WpvwPyuISJamDYIHLlDP18177282 = -473830012;    int WpvwPyuISJamDYIHLlDP21250621 = -456194147;    int WpvwPyuISJamDYIHLlDP21412798 = -737405538;    int WpvwPyuISJamDYIHLlDP35389897 = -379078145;    int WpvwPyuISJamDYIHLlDP63204551 = -321886197;    int WpvwPyuISJamDYIHLlDP51518517 = -931044894;    int WpvwPyuISJamDYIHLlDP84116215 = -452816328;    int WpvwPyuISJamDYIHLlDP87464741 = -249535735;    int WpvwPyuISJamDYIHLlDP89549674 = -512031824;    int WpvwPyuISJamDYIHLlDP58757811 = -911790072;    int WpvwPyuISJamDYIHLlDP79841920 = -148883045;    int WpvwPyuISJamDYIHLlDP63610202 = -551822008;    int WpvwPyuISJamDYIHLlDP97815760 = -989054204;    int WpvwPyuISJamDYIHLlDP15153037 = -17552161;    int WpvwPyuISJamDYIHLlDP73792203 = 70273355;    int WpvwPyuISJamDYIHLlDP65173046 = -308818428;    int WpvwPyuISJamDYIHLlDP99313034 = -342211426;    int WpvwPyuISJamDYIHLlDP52332097 = 77709562;    int WpvwPyuISJamDYIHLlDP54387685 = -380862231;    int WpvwPyuISJamDYIHLlDP8007848 = -950225297;    int WpvwPyuISJamDYIHLlDP64846379 = -249725645;    int WpvwPyuISJamDYIHLlDP33219457 = -386998013;    int WpvwPyuISJamDYIHLlDP67106029 = 25033982;    int WpvwPyuISJamDYIHLlDP62431408 = -598338713;    int WpvwPyuISJamDYIHLlDP76807640 = -459639415;    int WpvwPyuISJamDYIHLlDP98108116 = -961958275;    int WpvwPyuISJamDYIHLlDP22955495 = -279064982;    int WpvwPyuISJamDYIHLlDP57575173 = -497325520;    int WpvwPyuISJamDYIHLlDP2981426 = -673560715;    int WpvwPyuISJamDYIHLlDP46334116 = -673671088;    int WpvwPyuISJamDYIHLlDP85221739 = -316782866;    int WpvwPyuISJamDYIHLlDP3804833 = -723204641;    int WpvwPyuISJamDYIHLlDP3905014 = -634444518;    int WpvwPyuISJamDYIHLlDP16943397 = -674638541;    int WpvwPyuISJamDYIHLlDP81798099 = -764213009;    int WpvwPyuISJamDYIHLlDP39406553 = -798018426;    int WpvwPyuISJamDYIHLlDP10348049 = -539582790;    int WpvwPyuISJamDYIHLlDP46177872 = -886758966;    int WpvwPyuISJamDYIHLlDP85865426 = -726506896;    int WpvwPyuISJamDYIHLlDP26637205 = -112721730;    int WpvwPyuISJamDYIHLlDP52441312 = -788061591;    int WpvwPyuISJamDYIHLlDP43609523 = -564330657;    int WpvwPyuISJamDYIHLlDP25635901 = -344213091;    int WpvwPyuISJamDYIHLlDP53704892 = -123647912;    int WpvwPyuISJamDYIHLlDP4445091 = -785873937;    int WpvwPyuISJamDYIHLlDP7538824 = -510296417;    int WpvwPyuISJamDYIHLlDP30309260 = -445912547;    int WpvwPyuISJamDYIHLlDP57227161 = 81103929;    int WpvwPyuISJamDYIHLlDP90833092 = -741814453;    int WpvwPyuISJamDYIHLlDP16471707 = -612365571;    int WpvwPyuISJamDYIHLlDP59403808 = -272885605;    int WpvwPyuISJamDYIHLlDP16210642 = -327321280;    int WpvwPyuISJamDYIHLlDP95974504 = -712514009;    int WpvwPyuISJamDYIHLlDP99927278 = -167917676;    int WpvwPyuISJamDYIHLlDP29255941 = -337786324;    int WpvwPyuISJamDYIHLlDP84957825 = 13168001;    int WpvwPyuISJamDYIHLlDP54144591 = -381228130;    int WpvwPyuISJamDYIHLlDP58981390 = -39066825;    int WpvwPyuISJamDYIHLlDP58582256 = -919438730;    int WpvwPyuISJamDYIHLlDP65096435 = -359927922;    int WpvwPyuISJamDYIHLlDP28563023 = -551979913;    int WpvwPyuISJamDYIHLlDP26541042 = -955490808;    int WpvwPyuISJamDYIHLlDP84483315 = -575975020;    int WpvwPyuISJamDYIHLlDP43215559 = -838360737;    int WpvwPyuISJamDYIHLlDP73536071 = -495007206;    int WpvwPyuISJamDYIHLlDP76037087 = -425678404;    int WpvwPyuISJamDYIHLlDP59705188 = -917377491;    int WpvwPyuISJamDYIHLlDP80872363 = -214415663;    int WpvwPyuISJamDYIHLlDP33354937 = -253339153;    int WpvwPyuISJamDYIHLlDP34385651 = -131708219;    int WpvwPyuISJamDYIHLlDP54824998 = -769235638;    int WpvwPyuISJamDYIHLlDP53135162 = -455452460;    int WpvwPyuISJamDYIHLlDP66466671 = -195783543;    int WpvwPyuISJamDYIHLlDP27750481 = -388768117;     WpvwPyuISJamDYIHLlDP5050959 = WpvwPyuISJamDYIHLlDP53764479;     WpvwPyuISJamDYIHLlDP53764479 = WpvwPyuISJamDYIHLlDP54167023;     WpvwPyuISJamDYIHLlDP54167023 = WpvwPyuISJamDYIHLlDP11840139;     WpvwPyuISJamDYIHLlDP11840139 = WpvwPyuISJamDYIHLlDP11045717;     WpvwPyuISJamDYIHLlDP11045717 = WpvwPyuISJamDYIHLlDP99975381;     WpvwPyuISJamDYIHLlDP99975381 = WpvwPyuISJamDYIHLlDP60657173;     WpvwPyuISJamDYIHLlDP60657173 = WpvwPyuISJamDYIHLlDP31760847;     WpvwPyuISJamDYIHLlDP31760847 = WpvwPyuISJamDYIHLlDP81567768;     WpvwPyuISJamDYIHLlDP81567768 = WpvwPyuISJamDYIHLlDP49069977;     WpvwPyuISJamDYIHLlDP49069977 = WpvwPyuISJamDYIHLlDP78155721;     WpvwPyuISJamDYIHLlDP78155721 = WpvwPyuISJamDYIHLlDP36557528;     WpvwPyuISJamDYIHLlDP36557528 = WpvwPyuISJamDYIHLlDP31074265;     WpvwPyuISJamDYIHLlDP31074265 = WpvwPyuISJamDYIHLlDP15185575;     WpvwPyuISJamDYIHLlDP15185575 = WpvwPyuISJamDYIHLlDP12462704;     WpvwPyuISJamDYIHLlDP12462704 = WpvwPyuISJamDYIHLlDP84287011;     WpvwPyuISJamDYIHLlDP84287011 = WpvwPyuISJamDYIHLlDP71149025;     WpvwPyuISJamDYIHLlDP71149025 = WpvwPyuISJamDYIHLlDP28125020;     WpvwPyuISJamDYIHLlDP28125020 = WpvwPyuISJamDYIHLlDP72380197;     WpvwPyuISJamDYIHLlDP72380197 = WpvwPyuISJamDYIHLlDP64625296;     WpvwPyuISJamDYIHLlDP64625296 = WpvwPyuISJamDYIHLlDP81644753;     WpvwPyuISJamDYIHLlDP81644753 = WpvwPyuISJamDYIHLlDP58716843;     WpvwPyuISJamDYIHLlDP58716843 = WpvwPyuISJamDYIHLlDP68542739;     WpvwPyuISJamDYIHLlDP68542739 = WpvwPyuISJamDYIHLlDP50362190;     WpvwPyuISJamDYIHLlDP50362190 = WpvwPyuISJamDYIHLlDP7935126;     WpvwPyuISJamDYIHLlDP7935126 = WpvwPyuISJamDYIHLlDP94102319;     WpvwPyuISJamDYIHLlDP94102319 = WpvwPyuISJamDYIHLlDP18177282;     WpvwPyuISJamDYIHLlDP18177282 = WpvwPyuISJamDYIHLlDP21250621;     WpvwPyuISJamDYIHLlDP21250621 = WpvwPyuISJamDYIHLlDP21412798;     WpvwPyuISJamDYIHLlDP21412798 = WpvwPyuISJamDYIHLlDP35389897;     WpvwPyuISJamDYIHLlDP35389897 = WpvwPyuISJamDYIHLlDP63204551;     WpvwPyuISJamDYIHLlDP63204551 = WpvwPyuISJamDYIHLlDP51518517;     WpvwPyuISJamDYIHLlDP51518517 = WpvwPyuISJamDYIHLlDP84116215;     WpvwPyuISJamDYIHLlDP84116215 = WpvwPyuISJamDYIHLlDP87464741;     WpvwPyuISJamDYIHLlDP87464741 = WpvwPyuISJamDYIHLlDP89549674;     WpvwPyuISJamDYIHLlDP89549674 = WpvwPyuISJamDYIHLlDP58757811;     WpvwPyuISJamDYIHLlDP58757811 = WpvwPyuISJamDYIHLlDP79841920;     WpvwPyuISJamDYIHLlDP79841920 = WpvwPyuISJamDYIHLlDP63610202;     WpvwPyuISJamDYIHLlDP63610202 = WpvwPyuISJamDYIHLlDP97815760;     WpvwPyuISJamDYIHLlDP97815760 = WpvwPyuISJamDYIHLlDP15153037;     WpvwPyuISJamDYIHLlDP15153037 = WpvwPyuISJamDYIHLlDP73792203;     WpvwPyuISJamDYIHLlDP73792203 = WpvwPyuISJamDYIHLlDP65173046;     WpvwPyuISJamDYIHLlDP65173046 = WpvwPyuISJamDYIHLlDP99313034;     WpvwPyuISJamDYIHLlDP99313034 = WpvwPyuISJamDYIHLlDP52332097;     WpvwPyuISJamDYIHLlDP52332097 = WpvwPyuISJamDYIHLlDP54387685;     WpvwPyuISJamDYIHLlDP54387685 = WpvwPyuISJamDYIHLlDP8007848;     WpvwPyuISJamDYIHLlDP8007848 = WpvwPyuISJamDYIHLlDP64846379;     WpvwPyuISJamDYIHLlDP64846379 = WpvwPyuISJamDYIHLlDP33219457;     WpvwPyuISJamDYIHLlDP33219457 = WpvwPyuISJamDYIHLlDP67106029;     WpvwPyuISJamDYIHLlDP67106029 = WpvwPyuISJamDYIHLlDP62431408;     WpvwPyuISJamDYIHLlDP62431408 = WpvwPyuISJamDYIHLlDP76807640;     WpvwPyuISJamDYIHLlDP76807640 = WpvwPyuISJamDYIHLlDP98108116;     WpvwPyuISJamDYIHLlDP98108116 = WpvwPyuISJamDYIHLlDP22955495;     WpvwPyuISJamDYIHLlDP22955495 = WpvwPyuISJamDYIHLlDP57575173;     WpvwPyuISJamDYIHLlDP57575173 = WpvwPyuISJamDYIHLlDP2981426;     WpvwPyuISJamDYIHLlDP2981426 = WpvwPyuISJamDYIHLlDP46334116;     WpvwPyuISJamDYIHLlDP46334116 = WpvwPyuISJamDYIHLlDP85221739;     WpvwPyuISJamDYIHLlDP85221739 = WpvwPyuISJamDYIHLlDP3804833;     WpvwPyuISJamDYIHLlDP3804833 = WpvwPyuISJamDYIHLlDP3905014;     WpvwPyuISJamDYIHLlDP3905014 = WpvwPyuISJamDYIHLlDP16943397;     WpvwPyuISJamDYIHLlDP16943397 = WpvwPyuISJamDYIHLlDP81798099;     WpvwPyuISJamDYIHLlDP81798099 = WpvwPyuISJamDYIHLlDP39406553;     WpvwPyuISJamDYIHLlDP39406553 = WpvwPyuISJamDYIHLlDP10348049;     WpvwPyuISJamDYIHLlDP10348049 = WpvwPyuISJamDYIHLlDP46177872;     WpvwPyuISJamDYIHLlDP46177872 = WpvwPyuISJamDYIHLlDP85865426;     WpvwPyuISJamDYIHLlDP85865426 = WpvwPyuISJamDYIHLlDP26637205;     WpvwPyuISJamDYIHLlDP26637205 = WpvwPyuISJamDYIHLlDP52441312;     WpvwPyuISJamDYIHLlDP52441312 = WpvwPyuISJamDYIHLlDP43609523;     WpvwPyuISJamDYIHLlDP43609523 = WpvwPyuISJamDYIHLlDP25635901;     WpvwPyuISJamDYIHLlDP25635901 = WpvwPyuISJamDYIHLlDP53704892;     WpvwPyuISJamDYIHLlDP53704892 = WpvwPyuISJamDYIHLlDP4445091;     WpvwPyuISJamDYIHLlDP4445091 = WpvwPyuISJamDYIHLlDP7538824;     WpvwPyuISJamDYIHLlDP7538824 = WpvwPyuISJamDYIHLlDP30309260;     WpvwPyuISJamDYIHLlDP30309260 = WpvwPyuISJamDYIHLlDP57227161;     WpvwPyuISJamDYIHLlDP57227161 = WpvwPyuISJamDYIHLlDP90833092;     WpvwPyuISJamDYIHLlDP90833092 = WpvwPyuISJamDYIHLlDP16471707;     WpvwPyuISJamDYIHLlDP16471707 = WpvwPyuISJamDYIHLlDP59403808;     WpvwPyuISJamDYIHLlDP59403808 = WpvwPyuISJamDYIHLlDP16210642;     WpvwPyuISJamDYIHLlDP16210642 = WpvwPyuISJamDYIHLlDP95974504;     WpvwPyuISJamDYIHLlDP95974504 = WpvwPyuISJamDYIHLlDP99927278;     WpvwPyuISJamDYIHLlDP99927278 = WpvwPyuISJamDYIHLlDP29255941;     WpvwPyuISJamDYIHLlDP29255941 = WpvwPyuISJamDYIHLlDP84957825;     WpvwPyuISJamDYIHLlDP84957825 = WpvwPyuISJamDYIHLlDP54144591;     WpvwPyuISJamDYIHLlDP54144591 = WpvwPyuISJamDYIHLlDP58981390;     WpvwPyuISJamDYIHLlDP58981390 = WpvwPyuISJamDYIHLlDP58582256;     WpvwPyuISJamDYIHLlDP58582256 = WpvwPyuISJamDYIHLlDP65096435;     WpvwPyuISJamDYIHLlDP65096435 = WpvwPyuISJamDYIHLlDP28563023;     WpvwPyuISJamDYIHLlDP28563023 = WpvwPyuISJamDYIHLlDP26541042;     WpvwPyuISJamDYIHLlDP26541042 = WpvwPyuISJamDYIHLlDP84483315;     WpvwPyuISJamDYIHLlDP84483315 = WpvwPyuISJamDYIHLlDP43215559;     WpvwPyuISJamDYIHLlDP43215559 = WpvwPyuISJamDYIHLlDP73536071;     WpvwPyuISJamDYIHLlDP73536071 = WpvwPyuISJamDYIHLlDP76037087;     WpvwPyuISJamDYIHLlDP76037087 = WpvwPyuISJamDYIHLlDP59705188;     WpvwPyuISJamDYIHLlDP59705188 = WpvwPyuISJamDYIHLlDP80872363;     WpvwPyuISJamDYIHLlDP80872363 = WpvwPyuISJamDYIHLlDP33354937;     WpvwPyuISJamDYIHLlDP33354937 = WpvwPyuISJamDYIHLlDP34385651;     WpvwPyuISJamDYIHLlDP34385651 = WpvwPyuISJamDYIHLlDP54824998;     WpvwPyuISJamDYIHLlDP54824998 = WpvwPyuISJamDYIHLlDP53135162;     WpvwPyuISJamDYIHLlDP53135162 = WpvwPyuISJamDYIHLlDP66466671;     WpvwPyuISJamDYIHLlDP66466671 = WpvwPyuISJamDYIHLlDP27750481;     WpvwPyuISJamDYIHLlDP27750481 = WpvwPyuISJamDYIHLlDP5050959;}
// Junk Finished

// Junk Code By Peatreat & Thaisen's Gen
void mUmxjYXGHdetyRRpWMMR60968497() {     int GpecuGKFdGVfeFibKnSh37311767 = 35584121;    int GpecuGKFdGVfeFibKnSh54986914 = -340011467;    int GpecuGKFdGVfeFibKnSh87443602 = 80559736;    int GpecuGKFdGVfeFibKnSh53348252 = -672823199;    int GpecuGKFdGVfeFibKnSh96163235 = 88115932;    int GpecuGKFdGVfeFibKnSh34072094 = -206018290;    int GpecuGKFdGVfeFibKnSh58149474 = -438654406;    int GpecuGKFdGVfeFibKnSh54516142 = -11137234;    int GpecuGKFdGVfeFibKnSh54919212 = -142140512;    int GpecuGKFdGVfeFibKnSh36577892 = -221259900;    int GpecuGKFdGVfeFibKnSh39545762 = -631970867;    int GpecuGKFdGVfeFibKnSh57710969 = 87964923;    int GpecuGKFdGVfeFibKnSh59736313 = -120347851;    int GpecuGKFdGVfeFibKnSh19381474 = -385792787;    int GpecuGKFdGVfeFibKnSh51123609 = -277794754;    int GpecuGKFdGVfeFibKnSh4363182 = -616044654;    int GpecuGKFdGVfeFibKnSh77010197 = -78053003;    int GpecuGKFdGVfeFibKnSh12745724 = -443860149;    int GpecuGKFdGVfeFibKnSh52747437 = -805139922;    int GpecuGKFdGVfeFibKnSh75817331 = -376297241;    int GpecuGKFdGVfeFibKnSh94605458 = -208246214;    int GpecuGKFdGVfeFibKnSh24450479 = -886766708;    int GpecuGKFdGVfeFibKnSh38597644 = -119232389;    int GpecuGKFdGVfeFibKnSh68940880 = -209344722;    int GpecuGKFdGVfeFibKnSh96590184 = -196191153;    int GpecuGKFdGVfeFibKnSh63614349 = -155532457;    int GpecuGKFdGVfeFibKnSh43299367 = -471887511;    int GpecuGKFdGVfeFibKnSh3991696 = -184758542;    int GpecuGKFdGVfeFibKnSh67096361 = -319463609;    int GpecuGKFdGVfeFibKnSh96757996 = -424971621;    int GpecuGKFdGVfeFibKnSh4031954 = -76535805;    int GpecuGKFdGVfeFibKnSh9692794 = 17708171;    int GpecuGKFdGVfeFibKnSh56185845 = -681934663;    int GpecuGKFdGVfeFibKnSh23014784 = -582211427;    int GpecuGKFdGVfeFibKnSh30182505 = -925452586;    int GpecuGKFdGVfeFibKnSh47219674 = -924156038;    int GpecuGKFdGVfeFibKnSh73048064 = -366015092;    int GpecuGKFdGVfeFibKnSh92879019 = -553388237;    int GpecuGKFdGVfeFibKnSh35590875 = -582638190;    int GpecuGKFdGVfeFibKnSh90826609 = -249068417;    int GpecuGKFdGVfeFibKnSh88849901 = -727043035;    int GpecuGKFdGVfeFibKnSh99036974 = -252233906;    int GpecuGKFdGVfeFibKnSh45046385 = -854316414;    int GpecuGKFdGVfeFibKnSh21114276 = -55570898;    int GpecuGKFdGVfeFibKnSh30406534 = -77495753;    int GpecuGKFdGVfeFibKnSh95669135 = -648830487;    int GpecuGKFdGVfeFibKnSh96720486 = -179653398;    int GpecuGKFdGVfeFibKnSh87431672 = -116876750;    int GpecuGKFdGVfeFibKnSh33647281 = -588743370;    int GpecuGKFdGVfeFibKnSh30234748 = -283362635;    int GpecuGKFdGVfeFibKnSh56311333 = -440333648;    int GpecuGKFdGVfeFibKnSh28186267 = -328350724;    int GpecuGKFdGVfeFibKnSh51817443 = -239123217;    int GpecuGKFdGVfeFibKnSh64954656 = -649451047;    int GpecuGKFdGVfeFibKnSh66363038 = -685664559;    int GpecuGKFdGVfeFibKnSh12861288 = -77649172;    int GpecuGKFdGVfeFibKnSh16389271 = -120779079;    int GpecuGKFdGVfeFibKnSh18502723 = -710095543;    int GpecuGKFdGVfeFibKnSh56758068 = -376632047;    int GpecuGKFdGVfeFibKnSh32548886 = -756351612;    int GpecuGKFdGVfeFibKnSh90772726 = -734130780;    int GpecuGKFdGVfeFibKnSh54157778 = -153895864;    int GpecuGKFdGVfeFibKnSh87419781 = -691673625;    int GpecuGKFdGVfeFibKnSh58161216 = -717168892;    int GpecuGKFdGVfeFibKnSh32545938 = -44724095;    int GpecuGKFdGVfeFibKnSh29852968 = -549679038;    int GpecuGKFdGVfeFibKnSh1525125 = -230100414;    int GpecuGKFdGVfeFibKnSh36721529 = -538136424;    int GpecuGKFdGVfeFibKnSh89198969 = -460340201;    int GpecuGKFdGVfeFibKnSh3903935 = -353638716;    int GpecuGKFdGVfeFibKnSh31315118 = -150029562;    int GpecuGKFdGVfeFibKnSh84131177 = -524664767;    int GpecuGKFdGVfeFibKnSh77154848 = -861221960;    int GpecuGKFdGVfeFibKnSh61920827 = -456071505;    int GpecuGKFdGVfeFibKnSh86967430 = -649254206;    int GpecuGKFdGVfeFibKnSh95568483 = -956012309;    int GpecuGKFdGVfeFibKnSh79404094 = 67549706;    int GpecuGKFdGVfeFibKnSh17483369 = 36338509;    int GpecuGKFdGVfeFibKnSh38534346 = -31848969;    int GpecuGKFdGVfeFibKnSh921049 = -547360666;    int GpecuGKFdGVfeFibKnSh66893863 = -975879060;    int GpecuGKFdGVfeFibKnSh55867695 = -255010761;    int GpecuGKFdGVfeFibKnSh70344415 = -596015173;    int GpecuGKFdGVfeFibKnSh36861613 = 63899025;    int GpecuGKFdGVfeFibKnSh40446663 = -984637973;    int GpecuGKFdGVfeFibKnSh75845687 = -748185082;    int GpecuGKFdGVfeFibKnSh57875351 = -743168613;    int GpecuGKFdGVfeFibKnSh91231188 = 67516383;    int GpecuGKFdGVfeFibKnSh56651746 = -896546869;    int GpecuGKFdGVfeFibKnSh17321217 = -747803414;    int GpecuGKFdGVfeFibKnSh30830404 = -703376959;    int GpecuGKFdGVfeFibKnSh54545341 = -655919550;    int GpecuGKFdGVfeFibKnSh36120951 = -76756190;    int GpecuGKFdGVfeFibKnSh3041989 = -826286579;    int GpecuGKFdGVfeFibKnSh53883 = -514937638;    int GpecuGKFdGVfeFibKnSh34692123 = -473147171;    int GpecuGKFdGVfeFibKnSh11617194 = -560560281;    int GpecuGKFdGVfeFibKnSh86885169 = -37147523;    int GpecuGKFdGVfeFibKnSh88568337 = 89153196;    int GpecuGKFdGVfeFibKnSh553567 = 35584121;     GpecuGKFdGVfeFibKnSh37311767 = GpecuGKFdGVfeFibKnSh54986914;     GpecuGKFdGVfeFibKnSh54986914 = GpecuGKFdGVfeFibKnSh87443602;     GpecuGKFdGVfeFibKnSh87443602 = GpecuGKFdGVfeFibKnSh53348252;     GpecuGKFdGVfeFibKnSh53348252 = GpecuGKFdGVfeFibKnSh96163235;     GpecuGKFdGVfeFibKnSh96163235 = GpecuGKFdGVfeFibKnSh34072094;     GpecuGKFdGVfeFibKnSh34072094 = GpecuGKFdGVfeFibKnSh58149474;     GpecuGKFdGVfeFibKnSh58149474 = GpecuGKFdGVfeFibKnSh54516142;     GpecuGKFdGVfeFibKnSh54516142 = GpecuGKFdGVfeFibKnSh54919212;     GpecuGKFdGVfeFibKnSh54919212 = GpecuGKFdGVfeFibKnSh36577892;     GpecuGKFdGVfeFibKnSh36577892 = GpecuGKFdGVfeFibKnSh39545762;     GpecuGKFdGVfeFibKnSh39545762 = GpecuGKFdGVfeFibKnSh57710969;     GpecuGKFdGVfeFibKnSh57710969 = GpecuGKFdGVfeFibKnSh59736313;     GpecuGKFdGVfeFibKnSh59736313 = GpecuGKFdGVfeFibKnSh19381474;     GpecuGKFdGVfeFibKnSh19381474 = GpecuGKFdGVfeFibKnSh51123609;     GpecuGKFdGVfeFibKnSh51123609 = GpecuGKFdGVfeFibKnSh4363182;     GpecuGKFdGVfeFibKnSh4363182 = GpecuGKFdGVfeFibKnSh77010197;     GpecuGKFdGVfeFibKnSh77010197 = GpecuGKFdGVfeFibKnSh12745724;     GpecuGKFdGVfeFibKnSh12745724 = GpecuGKFdGVfeFibKnSh52747437;     GpecuGKFdGVfeFibKnSh52747437 = GpecuGKFdGVfeFibKnSh75817331;     GpecuGKFdGVfeFibKnSh75817331 = GpecuGKFdGVfeFibKnSh94605458;     GpecuGKFdGVfeFibKnSh94605458 = GpecuGKFdGVfeFibKnSh24450479;     GpecuGKFdGVfeFibKnSh24450479 = GpecuGKFdGVfeFibKnSh38597644;     GpecuGKFdGVfeFibKnSh38597644 = GpecuGKFdGVfeFibKnSh68940880;     GpecuGKFdGVfeFibKnSh68940880 = GpecuGKFdGVfeFibKnSh96590184;     GpecuGKFdGVfeFibKnSh96590184 = GpecuGKFdGVfeFibKnSh63614349;     GpecuGKFdGVfeFibKnSh63614349 = GpecuGKFdGVfeFibKnSh43299367;     GpecuGKFdGVfeFibKnSh43299367 = GpecuGKFdGVfeFibKnSh3991696;     GpecuGKFdGVfeFibKnSh3991696 = GpecuGKFdGVfeFibKnSh67096361;     GpecuGKFdGVfeFibKnSh67096361 = GpecuGKFdGVfeFibKnSh96757996;     GpecuGKFdGVfeFibKnSh96757996 = GpecuGKFdGVfeFibKnSh4031954;     GpecuGKFdGVfeFibKnSh4031954 = GpecuGKFdGVfeFibKnSh9692794;     GpecuGKFdGVfeFibKnSh9692794 = GpecuGKFdGVfeFibKnSh56185845;     GpecuGKFdGVfeFibKnSh56185845 = GpecuGKFdGVfeFibKnSh23014784;     GpecuGKFdGVfeFibKnSh23014784 = GpecuGKFdGVfeFibKnSh30182505;     GpecuGKFdGVfeFibKnSh30182505 = GpecuGKFdGVfeFibKnSh47219674;     GpecuGKFdGVfeFibKnSh47219674 = GpecuGKFdGVfeFibKnSh73048064;     GpecuGKFdGVfeFibKnSh73048064 = GpecuGKFdGVfeFibKnSh92879019;     GpecuGKFdGVfeFibKnSh92879019 = GpecuGKFdGVfeFibKnSh35590875;     GpecuGKFdGVfeFibKnSh35590875 = GpecuGKFdGVfeFibKnSh90826609;     GpecuGKFdGVfeFibKnSh90826609 = GpecuGKFdGVfeFibKnSh88849901;     GpecuGKFdGVfeFibKnSh88849901 = GpecuGKFdGVfeFibKnSh99036974;     GpecuGKFdGVfeFibKnSh99036974 = GpecuGKFdGVfeFibKnSh45046385;     GpecuGKFdGVfeFibKnSh45046385 = GpecuGKFdGVfeFibKnSh21114276;     GpecuGKFdGVfeFibKnSh21114276 = GpecuGKFdGVfeFibKnSh30406534;     GpecuGKFdGVfeFibKnSh30406534 = GpecuGKFdGVfeFibKnSh95669135;     GpecuGKFdGVfeFibKnSh95669135 = GpecuGKFdGVfeFibKnSh96720486;     GpecuGKFdGVfeFibKnSh96720486 = GpecuGKFdGVfeFibKnSh87431672;     GpecuGKFdGVfeFibKnSh87431672 = GpecuGKFdGVfeFibKnSh33647281;     GpecuGKFdGVfeFibKnSh33647281 = GpecuGKFdGVfeFibKnSh30234748;     GpecuGKFdGVfeFibKnSh30234748 = GpecuGKFdGVfeFibKnSh56311333;     GpecuGKFdGVfeFibKnSh56311333 = GpecuGKFdGVfeFibKnSh28186267;     GpecuGKFdGVfeFibKnSh28186267 = GpecuGKFdGVfeFibKnSh51817443;     GpecuGKFdGVfeFibKnSh51817443 = GpecuGKFdGVfeFibKnSh64954656;     GpecuGKFdGVfeFibKnSh64954656 = GpecuGKFdGVfeFibKnSh66363038;     GpecuGKFdGVfeFibKnSh66363038 = GpecuGKFdGVfeFibKnSh12861288;     GpecuGKFdGVfeFibKnSh12861288 = GpecuGKFdGVfeFibKnSh16389271;     GpecuGKFdGVfeFibKnSh16389271 = GpecuGKFdGVfeFibKnSh18502723;     GpecuGKFdGVfeFibKnSh18502723 = GpecuGKFdGVfeFibKnSh56758068;     GpecuGKFdGVfeFibKnSh56758068 = GpecuGKFdGVfeFibKnSh32548886;     GpecuGKFdGVfeFibKnSh32548886 = GpecuGKFdGVfeFibKnSh90772726;     GpecuGKFdGVfeFibKnSh90772726 = GpecuGKFdGVfeFibKnSh54157778;     GpecuGKFdGVfeFibKnSh54157778 = GpecuGKFdGVfeFibKnSh87419781;     GpecuGKFdGVfeFibKnSh87419781 = GpecuGKFdGVfeFibKnSh58161216;     GpecuGKFdGVfeFibKnSh58161216 = GpecuGKFdGVfeFibKnSh32545938;     GpecuGKFdGVfeFibKnSh32545938 = GpecuGKFdGVfeFibKnSh29852968;     GpecuGKFdGVfeFibKnSh29852968 = GpecuGKFdGVfeFibKnSh1525125;     GpecuGKFdGVfeFibKnSh1525125 = GpecuGKFdGVfeFibKnSh36721529;     GpecuGKFdGVfeFibKnSh36721529 = GpecuGKFdGVfeFibKnSh89198969;     GpecuGKFdGVfeFibKnSh89198969 = GpecuGKFdGVfeFibKnSh3903935;     GpecuGKFdGVfeFibKnSh3903935 = GpecuGKFdGVfeFibKnSh31315118;     GpecuGKFdGVfeFibKnSh31315118 = GpecuGKFdGVfeFibKnSh84131177;     GpecuGKFdGVfeFibKnSh84131177 = GpecuGKFdGVfeFibKnSh77154848;     GpecuGKFdGVfeFibKnSh77154848 = GpecuGKFdGVfeFibKnSh61920827;     GpecuGKFdGVfeFibKnSh61920827 = GpecuGKFdGVfeFibKnSh86967430;     GpecuGKFdGVfeFibKnSh86967430 = GpecuGKFdGVfeFibKnSh95568483;     GpecuGKFdGVfeFibKnSh95568483 = GpecuGKFdGVfeFibKnSh79404094;     GpecuGKFdGVfeFibKnSh79404094 = GpecuGKFdGVfeFibKnSh17483369;     GpecuGKFdGVfeFibKnSh17483369 = GpecuGKFdGVfeFibKnSh38534346;     GpecuGKFdGVfeFibKnSh38534346 = GpecuGKFdGVfeFibKnSh921049;     GpecuGKFdGVfeFibKnSh921049 = GpecuGKFdGVfeFibKnSh66893863;     GpecuGKFdGVfeFibKnSh66893863 = GpecuGKFdGVfeFibKnSh55867695;     GpecuGKFdGVfeFibKnSh55867695 = GpecuGKFdGVfeFibKnSh70344415;     GpecuGKFdGVfeFibKnSh70344415 = GpecuGKFdGVfeFibKnSh36861613;     GpecuGKFdGVfeFibKnSh36861613 = GpecuGKFdGVfeFibKnSh40446663;     GpecuGKFdGVfeFibKnSh40446663 = GpecuGKFdGVfeFibKnSh75845687;     GpecuGKFdGVfeFibKnSh75845687 = GpecuGKFdGVfeFibKnSh57875351;     GpecuGKFdGVfeFibKnSh57875351 = GpecuGKFdGVfeFibKnSh91231188;     GpecuGKFdGVfeFibKnSh91231188 = GpecuGKFdGVfeFibKnSh56651746;     GpecuGKFdGVfeFibKnSh56651746 = GpecuGKFdGVfeFibKnSh17321217;     GpecuGKFdGVfeFibKnSh17321217 = GpecuGKFdGVfeFibKnSh30830404;     GpecuGKFdGVfeFibKnSh30830404 = GpecuGKFdGVfeFibKnSh54545341;     GpecuGKFdGVfeFibKnSh54545341 = GpecuGKFdGVfeFibKnSh36120951;     GpecuGKFdGVfeFibKnSh36120951 = GpecuGKFdGVfeFibKnSh3041989;     GpecuGKFdGVfeFibKnSh3041989 = GpecuGKFdGVfeFibKnSh53883;     GpecuGKFdGVfeFibKnSh53883 = GpecuGKFdGVfeFibKnSh34692123;     GpecuGKFdGVfeFibKnSh34692123 = GpecuGKFdGVfeFibKnSh11617194;     GpecuGKFdGVfeFibKnSh11617194 = GpecuGKFdGVfeFibKnSh86885169;     GpecuGKFdGVfeFibKnSh86885169 = GpecuGKFdGVfeFibKnSh88568337;     GpecuGKFdGVfeFibKnSh88568337 = GpecuGKFdGVfeFibKnSh553567;     GpecuGKFdGVfeFibKnSh553567 = GpecuGKFdGVfeFibKnSh37311767;}
// Junk Finished

// Junk Code By Peatreat & Thaisen's Gen
void UrrysfMtoOjIcNpgLuVU85068282() {     int pbkHlMAMiOifsjkfLjYS40230714 = -285688631;    int pbkHlMAMiOifsjkfLjYS86310550 = -842676526;    int pbkHlMAMiOifsjkfLjYS99375889 = -507465493;    int pbkHlMAMiOifsjkfLjYS49707792 = -862656467;    int pbkHlMAMiOifsjkfLjYS93890973 = -976668654;    int pbkHlMAMiOifsjkfLjYS23363137 = -176141899;    int pbkHlMAMiOifsjkfLjYS5473335 = -917335483;    int pbkHlMAMiOifsjkfLjYS64899767 = -387925912;    int pbkHlMAMiOifsjkfLjYS91395545 = -398145172;    int pbkHlMAMiOifsjkfLjYS50412753 = 41584813;    int pbkHlMAMiOifsjkfLjYS22864358 = -195604968;    int pbkHlMAMiOifsjkfLjYS10119305 = -163477666;    int pbkHlMAMiOifsjkfLjYS36372834 = -314707894;    int pbkHlMAMiOifsjkfLjYS53580186 = -371215818;    int pbkHlMAMiOifsjkfLjYS27332425 = -707188675;    int pbkHlMAMiOifsjkfLjYS82403445 = -534520262;    int pbkHlMAMiOifsjkfLjYS20871436 = -183558142;    int pbkHlMAMiOifsjkfLjYS94659388 = -44848122;    int pbkHlMAMiOifsjkfLjYS94394188 = -563037286;    int pbkHlMAMiOifsjkfLjYS73722312 = 88604761;    int pbkHlMAMiOifsjkfLjYS83032493 = -693521197;    int pbkHlMAMiOifsjkfLjYS73283614 = -879729808;    int pbkHlMAMiOifsjkfLjYS73717961 = 2261638;    int pbkHlMAMiOifsjkfLjYS77718998 = -36793587;    int pbkHlMAMiOifsjkfLjYS20926942 = -623450168;    int pbkHlMAMiOifsjkfLjYS47149883 = -913631463;    int pbkHlMAMiOifsjkfLjYS6460657 = -564323072;    int pbkHlMAMiOifsjkfLjYS59111471 = -909479717;    int pbkHlMAMiOifsjkfLjYS32243965 = -649809583;    int pbkHlMAMiOifsjkfLjYS21208322 = -253828793;    int pbkHlMAMiOifsjkfLjYS18518674 = -292525865;    int pbkHlMAMiOifsjkfLjYS27654674 = -70014165;    int pbkHlMAMiOifsjkfLjYS25748412 = -656070391;    int pbkHlMAMiOifsjkfLjYS23646965 = -601735071;    int pbkHlMAMiOifsjkfLjYS66481533 = -926506202;    int pbkHlMAMiOifsjkfLjYS52846606 = -935982806;    int pbkHlMAMiOifsjkfLjYS98104325 = -672086969;    int pbkHlMAMiOifsjkfLjYS5022646 = -343804893;    int pbkHlMAMiOifsjkfLjYS16585681 = -519269397;    int pbkHlMAMiOifsjkfLjYS89730588 = -388761350;    int pbkHlMAMiOifsjkfLjYS76125215 = -720434406;    int pbkHlMAMiOifsjkfLjYS75686233 = -871246371;    int pbkHlMAMiOifsjkfLjYS79463336 = -794045200;    int pbkHlMAMiOifsjkfLjYS37172856 = -703861886;    int pbkHlMAMiOifsjkfLjYS18361390 = -709516268;    int pbkHlMAMiOifsjkfLjYS49308290 = -293728161;    int pbkHlMAMiOifsjkfLjYS27028004 = -844370328;    int pbkHlMAMiOifsjkfLjYS31874517 = -790124427;    int pbkHlMAMiOifsjkfLjYS86971594 = -524520830;    int pbkHlMAMiOifsjkfLjYS86420151 = -664370549;    int pbkHlMAMiOifsjkfLjYS90744980 = -389044994;    int pbkHlMAMiOifsjkfLjYS85588663 = -381817147;    int pbkHlMAMiOifsjkfLjYS60306700 = -234925486;    int pbkHlMAMiOifsjkfLjYS52311173 = -930538219;    int pbkHlMAMiOifsjkfLjYS2035790 = 48189961;    int pbkHlMAMiOifsjkfLjYS66947100 = -405958823;    int pbkHlMAMiOifsjkfLjYS12592590 = -744938164;    int pbkHlMAMiOifsjkfLjYS21656891 = -370671906;    int pbkHlMAMiOifsjkfLjYS28780850 = -139206300;    int pbkHlMAMiOifsjkfLjYS46741090 = 36962808;    int pbkHlMAMiOifsjkfLjYS16902480 = -611818828;    int pbkHlMAMiOifsjkfLjYS46361864 = 92144234;    int pbkHlMAMiOifsjkfLjYS32655803 = -738116330;    int pbkHlMAMiOifsjkfLjYS70187224 = -44316380;    int pbkHlMAMiOifsjkfLjYS31894080 = -665889323;    int pbkHlMAMiOifsjkfLjYS95209683 = -25590803;    int pbkHlMAMiOifsjkfLjYS84370893 = -507407275;    int pbkHlMAMiOifsjkfLjYS12725870 = -712972824;    int pbkHlMAMiOifsjkfLjYS87098652 = -444709616;    int pbkHlMAMiOifsjkfLjYS74485819 = -771205870;    int pbkHlMAMiOifsjkfLjYS84299119 = -862433294;    int pbkHlMAMiOifsjkfLjYS15848790 = -839753249;    int pbkHlMAMiOifsjkfLjYS78073707 = -525578726;    int pbkHlMAMiOifsjkfLjYS4663601 = -74275936;    int pbkHlMAMiOifsjkfLjYS97597097 = -190960833;    int pbkHlMAMiOifsjkfLjYS7346261 = -822274827;    int pbkHlMAMiOifsjkfLjYS93820278 = 14315392;    int pbkHlMAMiOifsjkfLjYS36545105 = -293876476;    int pbkHlMAMiOifsjkfLjYS59357609 = -327277320;    int pbkHlMAMiOifsjkfLjYS71618651 = -229722007;    int pbkHlMAMiOifsjkfLjYS20121879 = 30738865;    int pbkHlMAMiOifsjkfLjYS74586140 = -774198645;    int pbkHlMAMiOifsjkfLjYS72139876 = -284958888;    int pbkHlMAMiOifsjkfLjYS45823814 = -985439034;    int pbkHlMAMiOifsjkfLjYS30463341 = -864783800;    int pbkHlMAMiOifsjkfLjYS32930010 = -910708718;    int pbkHlMAMiOifsjkfLjYS67347974 = -835088680;    int pbkHlMAMiOifsjkfLjYS73437239 = -725532172;    int pbkHlMAMiOifsjkfLjYS21611176 = -549925032;    int pbkHlMAMiOifsjkfLjYS99534433 = -420547380;    int pbkHlMAMiOifsjkfLjYS40254016 = -91044642;    int pbkHlMAMiOifsjkfLjYS76447434 = -201415063;    int pbkHlMAMiOifsjkfLjYS76241796 = -104598593;    int pbkHlMAMiOifsjkfLjYS69844590 = -456232206;    int pbkHlMAMiOifsjkfLjYS72828108 = -776942522;    int pbkHlMAMiOifsjkfLjYS29763352 = -712578640;    int pbkHlMAMiOifsjkfLjYS43030430 = -33130041;    int pbkHlMAMiOifsjkfLjYS9276112 = -649728821;    int pbkHlMAMiOifsjkfLjYS5278777 = 62027436;    int pbkHlMAMiOifsjkfLjYS23151706 = -285688631;     pbkHlMAMiOifsjkfLjYS40230714 = pbkHlMAMiOifsjkfLjYS86310550;     pbkHlMAMiOifsjkfLjYS86310550 = pbkHlMAMiOifsjkfLjYS99375889;     pbkHlMAMiOifsjkfLjYS99375889 = pbkHlMAMiOifsjkfLjYS49707792;     pbkHlMAMiOifsjkfLjYS49707792 = pbkHlMAMiOifsjkfLjYS93890973;     pbkHlMAMiOifsjkfLjYS93890973 = pbkHlMAMiOifsjkfLjYS23363137;     pbkHlMAMiOifsjkfLjYS23363137 = pbkHlMAMiOifsjkfLjYS5473335;     pbkHlMAMiOifsjkfLjYS5473335 = pbkHlMAMiOifsjkfLjYS64899767;     pbkHlMAMiOifsjkfLjYS64899767 = pbkHlMAMiOifsjkfLjYS91395545;     pbkHlMAMiOifsjkfLjYS91395545 = pbkHlMAMiOifsjkfLjYS50412753;     pbkHlMAMiOifsjkfLjYS50412753 = pbkHlMAMiOifsjkfLjYS22864358;     pbkHlMAMiOifsjkfLjYS22864358 = pbkHlMAMiOifsjkfLjYS10119305;     pbkHlMAMiOifsjkfLjYS10119305 = pbkHlMAMiOifsjkfLjYS36372834;     pbkHlMAMiOifsjkfLjYS36372834 = pbkHlMAMiOifsjkfLjYS53580186;     pbkHlMAMiOifsjkfLjYS53580186 = pbkHlMAMiOifsjkfLjYS27332425;     pbkHlMAMiOifsjkfLjYS27332425 = pbkHlMAMiOifsjkfLjYS82403445;     pbkHlMAMiOifsjkfLjYS82403445 = pbkHlMAMiOifsjkfLjYS20871436;     pbkHlMAMiOifsjkfLjYS20871436 = pbkHlMAMiOifsjkfLjYS94659388;     pbkHlMAMiOifsjkfLjYS94659388 = pbkHlMAMiOifsjkfLjYS94394188;     pbkHlMAMiOifsjkfLjYS94394188 = pbkHlMAMiOifsjkfLjYS73722312;     pbkHlMAMiOifsjkfLjYS73722312 = pbkHlMAMiOifsjkfLjYS83032493;     pbkHlMAMiOifsjkfLjYS83032493 = pbkHlMAMiOifsjkfLjYS73283614;     pbkHlMAMiOifsjkfLjYS73283614 = pbkHlMAMiOifsjkfLjYS73717961;     pbkHlMAMiOifsjkfLjYS73717961 = pbkHlMAMiOifsjkfLjYS77718998;     pbkHlMAMiOifsjkfLjYS77718998 = pbkHlMAMiOifsjkfLjYS20926942;     pbkHlMAMiOifsjkfLjYS20926942 = pbkHlMAMiOifsjkfLjYS47149883;     pbkHlMAMiOifsjkfLjYS47149883 = pbkHlMAMiOifsjkfLjYS6460657;     pbkHlMAMiOifsjkfLjYS6460657 = pbkHlMAMiOifsjkfLjYS59111471;     pbkHlMAMiOifsjkfLjYS59111471 = pbkHlMAMiOifsjkfLjYS32243965;     pbkHlMAMiOifsjkfLjYS32243965 = pbkHlMAMiOifsjkfLjYS21208322;     pbkHlMAMiOifsjkfLjYS21208322 = pbkHlMAMiOifsjkfLjYS18518674;     pbkHlMAMiOifsjkfLjYS18518674 = pbkHlMAMiOifsjkfLjYS27654674;     pbkHlMAMiOifsjkfLjYS27654674 = pbkHlMAMiOifsjkfLjYS25748412;     pbkHlMAMiOifsjkfLjYS25748412 = pbkHlMAMiOifsjkfLjYS23646965;     pbkHlMAMiOifsjkfLjYS23646965 = pbkHlMAMiOifsjkfLjYS66481533;     pbkHlMAMiOifsjkfLjYS66481533 = pbkHlMAMiOifsjkfLjYS52846606;     pbkHlMAMiOifsjkfLjYS52846606 = pbkHlMAMiOifsjkfLjYS98104325;     pbkHlMAMiOifsjkfLjYS98104325 = pbkHlMAMiOifsjkfLjYS5022646;     pbkHlMAMiOifsjkfLjYS5022646 = pbkHlMAMiOifsjkfLjYS16585681;     pbkHlMAMiOifsjkfLjYS16585681 = pbkHlMAMiOifsjkfLjYS89730588;     pbkHlMAMiOifsjkfLjYS89730588 = pbkHlMAMiOifsjkfLjYS76125215;     pbkHlMAMiOifsjkfLjYS76125215 = pbkHlMAMiOifsjkfLjYS75686233;     pbkHlMAMiOifsjkfLjYS75686233 = pbkHlMAMiOifsjkfLjYS79463336;     pbkHlMAMiOifsjkfLjYS79463336 = pbkHlMAMiOifsjkfLjYS37172856;     pbkHlMAMiOifsjkfLjYS37172856 = pbkHlMAMiOifsjkfLjYS18361390;     pbkHlMAMiOifsjkfLjYS18361390 = pbkHlMAMiOifsjkfLjYS49308290;     pbkHlMAMiOifsjkfLjYS49308290 = pbkHlMAMiOifsjkfLjYS27028004;     pbkHlMAMiOifsjkfLjYS27028004 = pbkHlMAMiOifsjkfLjYS31874517;     pbkHlMAMiOifsjkfLjYS31874517 = pbkHlMAMiOifsjkfLjYS86971594;     pbkHlMAMiOifsjkfLjYS86971594 = pbkHlMAMiOifsjkfLjYS86420151;     pbkHlMAMiOifsjkfLjYS86420151 = pbkHlMAMiOifsjkfLjYS90744980;     pbkHlMAMiOifsjkfLjYS90744980 = pbkHlMAMiOifsjkfLjYS85588663;     pbkHlMAMiOifsjkfLjYS85588663 = pbkHlMAMiOifsjkfLjYS60306700;     pbkHlMAMiOifsjkfLjYS60306700 = pbkHlMAMiOifsjkfLjYS52311173;     pbkHlMAMiOifsjkfLjYS52311173 = pbkHlMAMiOifsjkfLjYS2035790;     pbkHlMAMiOifsjkfLjYS2035790 = pbkHlMAMiOifsjkfLjYS66947100;     pbkHlMAMiOifsjkfLjYS66947100 = pbkHlMAMiOifsjkfLjYS12592590;     pbkHlMAMiOifsjkfLjYS12592590 = pbkHlMAMiOifsjkfLjYS21656891;     pbkHlMAMiOifsjkfLjYS21656891 = pbkHlMAMiOifsjkfLjYS28780850;     pbkHlMAMiOifsjkfLjYS28780850 = pbkHlMAMiOifsjkfLjYS46741090;     pbkHlMAMiOifsjkfLjYS46741090 = pbkHlMAMiOifsjkfLjYS16902480;     pbkHlMAMiOifsjkfLjYS16902480 = pbkHlMAMiOifsjkfLjYS46361864;     pbkHlMAMiOifsjkfLjYS46361864 = pbkHlMAMiOifsjkfLjYS32655803;     pbkHlMAMiOifsjkfLjYS32655803 = pbkHlMAMiOifsjkfLjYS70187224;     pbkHlMAMiOifsjkfLjYS70187224 = pbkHlMAMiOifsjkfLjYS31894080;     pbkHlMAMiOifsjkfLjYS31894080 = pbkHlMAMiOifsjkfLjYS95209683;     pbkHlMAMiOifsjkfLjYS95209683 = pbkHlMAMiOifsjkfLjYS84370893;     pbkHlMAMiOifsjkfLjYS84370893 = pbkHlMAMiOifsjkfLjYS12725870;     pbkHlMAMiOifsjkfLjYS12725870 = pbkHlMAMiOifsjkfLjYS87098652;     pbkHlMAMiOifsjkfLjYS87098652 = pbkHlMAMiOifsjkfLjYS74485819;     pbkHlMAMiOifsjkfLjYS74485819 = pbkHlMAMiOifsjkfLjYS84299119;     pbkHlMAMiOifsjkfLjYS84299119 = pbkHlMAMiOifsjkfLjYS15848790;     pbkHlMAMiOifsjkfLjYS15848790 = pbkHlMAMiOifsjkfLjYS78073707;     pbkHlMAMiOifsjkfLjYS78073707 = pbkHlMAMiOifsjkfLjYS4663601;     pbkHlMAMiOifsjkfLjYS4663601 = pbkHlMAMiOifsjkfLjYS97597097;     pbkHlMAMiOifsjkfLjYS97597097 = pbkHlMAMiOifsjkfLjYS7346261;     pbkHlMAMiOifsjkfLjYS7346261 = pbkHlMAMiOifsjkfLjYS93820278;     pbkHlMAMiOifsjkfLjYS93820278 = pbkHlMAMiOifsjkfLjYS36545105;     pbkHlMAMiOifsjkfLjYS36545105 = pbkHlMAMiOifsjkfLjYS59357609;     pbkHlMAMiOifsjkfLjYS59357609 = pbkHlMAMiOifsjkfLjYS71618651;     pbkHlMAMiOifsjkfLjYS71618651 = pbkHlMAMiOifsjkfLjYS20121879;     pbkHlMAMiOifsjkfLjYS20121879 = pbkHlMAMiOifsjkfLjYS74586140;     pbkHlMAMiOifsjkfLjYS74586140 = pbkHlMAMiOifsjkfLjYS72139876;     pbkHlMAMiOifsjkfLjYS72139876 = pbkHlMAMiOifsjkfLjYS45823814;     pbkHlMAMiOifsjkfLjYS45823814 = pbkHlMAMiOifsjkfLjYS30463341;     pbkHlMAMiOifsjkfLjYS30463341 = pbkHlMAMiOifsjkfLjYS32930010;     pbkHlMAMiOifsjkfLjYS32930010 = pbkHlMAMiOifsjkfLjYS67347974;     pbkHlMAMiOifsjkfLjYS67347974 = pbkHlMAMiOifsjkfLjYS73437239;     pbkHlMAMiOifsjkfLjYS73437239 = pbkHlMAMiOifsjkfLjYS21611176;     pbkHlMAMiOifsjkfLjYS21611176 = pbkHlMAMiOifsjkfLjYS99534433;     pbkHlMAMiOifsjkfLjYS99534433 = pbkHlMAMiOifsjkfLjYS40254016;     pbkHlMAMiOifsjkfLjYS40254016 = pbkHlMAMiOifsjkfLjYS76447434;     pbkHlMAMiOifsjkfLjYS76447434 = pbkHlMAMiOifsjkfLjYS76241796;     pbkHlMAMiOifsjkfLjYS76241796 = pbkHlMAMiOifsjkfLjYS69844590;     pbkHlMAMiOifsjkfLjYS69844590 = pbkHlMAMiOifsjkfLjYS72828108;     pbkHlMAMiOifsjkfLjYS72828108 = pbkHlMAMiOifsjkfLjYS29763352;     pbkHlMAMiOifsjkfLjYS29763352 = pbkHlMAMiOifsjkfLjYS43030430;     pbkHlMAMiOifsjkfLjYS43030430 = pbkHlMAMiOifsjkfLjYS9276112;     pbkHlMAMiOifsjkfLjYS9276112 = pbkHlMAMiOifsjkfLjYS5278777;     pbkHlMAMiOifsjkfLjYS5278777 = pbkHlMAMiOifsjkfLjYS23151706;     pbkHlMAMiOifsjkfLjYS23151706 = pbkHlMAMiOifsjkfLjYS40230714;}
// Junk Finished
