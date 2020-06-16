#include <fstream>
#include <ShlObj.h>

#include "json/json.h"

#include "Config.h"

Config::Config(const char* name) noexcept
{
    if (PWSTR pathToDocuments; SUCCEEDED(SHGetKnownFolderPath(FOLDERID_Documents, 0, nullptr, &pathToDocuments))) {
        path = pathToDocuments;
        path /= name;
        CoTaskMemFree(pathToDocuments);
    }

    listConfigs();
}

void Config::load(size_t id) noexcept
{
    Json::Value json;

    if (std::ifstream in{ path / (const char8_t*)configs[id].c_str() }; in.good())
        in >> json;
    else
        return;

    for (size_t i = 0; i < aimbot.size(); i++) {
        const auto& aimbotJson = json["Aimbot"][i];
        auto& aimbotConfig = aimbot[i];

        if (aimbotJson.isMember("Enabled")) aimbotConfig.enabled = aimbotJson["Enabled"].asBool();
        if (aimbotJson.isMember("On key")) aimbotConfig.onKey = aimbotJson["On key"].asBool();
        if (aimbotJson.isMember("Key")) aimbotConfig.key = aimbotJson["Key"].asInt();
        if (aimbotJson.isMember("Key mode")) aimbotConfig.keyMode = aimbotJson["Key mode"].asInt();
        if (aimbotJson.isMember("Aimlock")) aimbotConfig.aimlock = aimbotJson["Aimlock"].asBool();
        if (aimbotJson.isMember("Silent")) aimbotConfig.silent = aimbotJson["Silent"].asBool();
        if (aimbotJson.isMember("Friendly fire")) aimbotConfig.friendlyFire = aimbotJson["Friendly fire"].asBool();
        if (aimbotJson.isMember("Visible only")) aimbotConfig.visibleOnly = aimbotJson["Visible only"].asBool();
        if (aimbotJson.isMember("Scoped only")) aimbotConfig.scopedOnly = aimbotJson["Scoped only"].asBool();
        if (aimbotJson.isMember("Ignore flash")) aimbotConfig.ignoreFlash = aimbotJson["Ignore flash"].asBool();
        if (aimbotJson.isMember("Ignore smoke")) aimbotConfig.ignoreSmoke = aimbotJson["Ignore smoke"].asBool();
        if (aimbotJson.isMember("Auto shot")) aimbotConfig.autoShot = aimbotJson["Auto shot"].asBool();
        if (aimbotJson.isMember("Auto scope")) aimbotConfig.autoScope = aimbotJson["Auto scope"].asBool();
        if (aimbotJson.isMember("Fov")) aimbotConfig.fov = aimbotJson["Fov"].asFloat();
        if (aimbotJson.isMember("Smooth")) aimbotConfig.smooth = aimbotJson["Smooth"].asFloat();
        if (aimbotJson.isMember("Bone")) aimbotConfig.bone = aimbotJson["Bone"].asInt();
        if (aimbotJson.isMember("Max aim inaccuracy")) aimbotConfig.maxAimInaccuracy = aimbotJson["Max aim inaccuracy"].asFloat();
        if (aimbotJson.isMember("Max shot inaccuracy")) aimbotConfig.maxShotInaccuracy = aimbotJson["Max shot inaccuracy"].asFloat();
        if (aimbotJson.isMember("Hitchance")) aimbotConfig.hitchance = aimbotJson["Hitchance"].asInt();
        if (aimbotJson.isMember("Min damage")) aimbotConfig.minDamage = aimbotJson["Min damage"].asInt();
        if (aimbotJson.isMember("Killshot")) aimbotConfig.killshot = aimbotJson["Killshot"].asBool();
        if (aimbotJson.isMember("Between shots")) aimbotConfig.betweenShots = aimbotJson["Between shots"].asBool();
        if (aimbotJson.isMember("RCS")) aimbotConfig.rcs = aimbotJson["RCS"].asBool();
        if (aimbotJson.isMember("Shot Delay")) aimbotConfig.shotdelay = aimbotJson["Shot Delay"].asInt();
        for (int i = 0; i < 19; ++i)
        {
            if (aimbotJson.isMember("Hitbox" + std::to_string(i))) aimbotConfig.hitboxes[i] = aimbotJson["Hitbox" + std::to_string(i)].asBool();
        }
    }

    for (size_t i = 0; i < triggerbot.size(); i++) {
        const auto& triggerbotJson = json["Triggerbot"][i];
        auto& triggerbotConfig = triggerbot[i];

        if (triggerbotJson.isMember("Enabled")) triggerbotConfig.enabled = triggerbotJson["Enabled"].asBool();
        if (triggerbotJson.isMember("On key")) triggerbotConfig.onKey = triggerbotJson["On key"].asBool();
        if (triggerbotJson.isMember("Key")) triggerbotConfig.key = triggerbotJson["Key"].asInt();
        if (triggerbotJson.isMember("Friendly fire")) triggerbotConfig.friendlyFire = triggerbotJson["Friendly fire"].asBool();
        if (triggerbotJson.isMember("Scoped only")) triggerbotConfig.scopedOnly = triggerbotJson["Scoped only"].asBool();
        if (triggerbotJson.isMember("Ignore flash")) triggerbotConfig.ignoreFlash = triggerbotJson["Ignore flash"].asBool();
        if (triggerbotJson.isMember("Ignore smoke")) triggerbotConfig.ignoreSmoke = triggerbotJson["Ignore smoke"].asBool();
        if (triggerbotJson.isMember("Hitgroup")) triggerbotConfig.hitgroup = triggerbotJson["Hitgroup"].asInt();
        if (triggerbotJson.isMember("Shot delay")) triggerbotConfig.shotDelay = triggerbotJson["Shot delay"].asInt();
        if (triggerbotJson.isMember("Min damage")) triggerbotConfig.minDamage = triggerbotJson["Min damage"].asInt();
        if (triggerbotJson.isMember("Killshot")) triggerbotConfig.killshot = triggerbotJson["Killshot"].asBool();
        if (triggerbotJson.isMember("Burst Time")) triggerbotConfig.burstTime = triggerbotJson["Burst Time"].asFloat();
    }

    /*{
        const auto& backtrackJson = json["Backtrack"];
        if (backtrackJson.isMember("Enabled")) backtrack.enabled = backtrackJson["Enabled"].asBool();
        if (backtrackJson.isMember("Ignore smoke")) backtrack.ignoreSmoke = backtrackJson["Ignore smoke"].asBool();
        if (backtrackJson.isMember("Recoil based fov")) backtrack.recoilBasedFov = backtrackJson["Recoil based fov"].asBool();
        if (backtrackJson.isMember("Time limit")) backtrack.timeLimit = backtrackJson["Time limit"].asInt();
    }*/

    for (size_t i = 0; i < backtrack.size(); i++) {
        const auto& backtrackJson = json["Backtrackm"][i];
        auto& backtrackConfig = backtrack[i];
        if (backtrackJson.isMember("Enabled")) backtrackConfig.enabled = backtrackJson["Enabled"].asBool();
        if (backtrackJson.isMember("Ignore smoke")) backtrackConfig.ignoreSmoke = backtrackJson["Ignore smoke"].asBool();
        if (backtrackJson.isMember("Recoil based fov")) backtrackConfig.recoilBasedFov = backtrackJson["Recoil based fov"].asBool();
        if (backtrackJson.isMember("Time limit")) backtrackConfig.timeLimit = backtrackJson["Time limit"].asInt();
    }

    {
        const auto& antiAimJson = json["Anti aim"];
        if (antiAimJson.isMember("Enabled")) antiAim.enabled = antiAimJson["Enabled"].asBool();
        if (antiAimJson.isMember("Antiaim Type")) antiAim.antiaimType = antiAimJson["Antiaim Type"].asInt();
        if (antiAimJson.isMember("Pitch angle")) antiAim.pitchAngle = antiAimJson["Pitch angle"].asFloat();
        if (antiAimJson.isMember("Invert key")) antiAim.invertkey = antiAimJson["Invert key"].asInt();
        if (antiAimJson.isMember("LBY Breaker")) antiAim.lbybreaker = antiAimJson["LBY Breaker"].asBool();
        if (antiAimJson.isMember("Back key")) antiAim.backwardskey = antiAimJson["Back key"].asInt();
        if (antiAimJson.isMember("Right key")) antiAim.rightkey = antiAimJson["Right key"].asInt();
        if (antiAimJson.isMember("Left key")) antiAim.leftkey = antiAimJson["Left key"].asInt();
        //if (antiAimJson.isMember("Fakeduck")) antiAim.fakeduck = antiAimJson["Fakeduck"].asBool();
        //if (antiAimJson.isMember("Fakeduck key")) antiAim.fakeduckkey = antiAimJson["Fakeduck key"].asInt();
        if (antiAimJson.isMember("Resolver")) antiAim.resolveall = antiAimJson["Resolver"].asBool();
        if (antiAimJson.isMember("Pitch resolver")) antiAim.pitchResolver = antiAimJson["Pitch resolver"].asBool();
        if (antiAimJson.isMember("Auto stop")) antiAim.autostop = antiAimJson["Auto stop"].asBool();
        if (antiAimJson.isMember("Auto zeus")) antiAim.autozeus = antiAimJson["Auto zeus"].asBool();
        if (antiAimJson.isMember("Body lean")) antiAim.bodylean = antiAimJson["Body lean"].asInt();
        if (antiAimJson.isMember("Real key")) antiAim.realkey = antiAimJson["Real key"].asInt();
        if (antiAimJson.isMember("Fake key")) antiAim.fakekey = antiAimJson["Fake key"].asInt();
    }

    for (size_t i = 0; i < glow.size(); i++) {
        const auto& glowJson = json["glow"][i];
        auto& glowConfig = glow[i];

        if (glowJson.isMember("Enabled")) glowConfig.enabled = glowJson["Enabled"].asBool();
        if (glowJson.isMember("healthBased")) glowConfig.healthBased = glowJson["healthBased"].asBool();
        if (glowJson.isMember("Sound glow")) glowConfig.soundGlow = glowJson["Sound glow"].asBool();
        if (glowJson.isMember("thickness")) glowConfig.thickness = glowJson["thickness"].asFloat();
        if (glowJson.isMember("alpha")) glowConfig.alpha = glowJson["alpha"].asFloat();
        if (glowJson.isMember("style")) glowConfig.style = glowJson["style"].asInt();
        if (glowJson.isMember("Color")) {
            const auto& colorJson = glowJson["Color"];
            auto& colorConfig = glowConfig.color;

            if (colorJson.isMember("Color")) {
                colorConfig.color[0] = colorJson["Color"][0].asFloat();
                colorConfig.color[1] = colorJson["Color"][1].asFloat();
                colorConfig.color[2] = colorJson["Color"][2].asFloat();
            }

            if (colorJson.isMember("Rainbow")) colorConfig.rainbow = colorJson["Rainbow"].asBool();
            if (colorJson.isMember("Rainbow speed")) colorConfig.rainbowSpeed = colorJson["Rainbow speed"].asFloat();
        }
    }

    for (size_t i = 0; i < chams.size(); i++) {
        const auto& chamsJson = json["Chams"][i];
        auto& chamsConfig = chams[i];

        for (size_t j = 0; j < chams[0].materials.size(); j++) {
            const auto& materialsJson = chamsJson[j];
            auto& materialsConfig = chams[i].materials[j];

            if (materialsJson.isMember("Enabled")) materialsConfig.enabled = materialsJson["Enabled"].asBool();
            if (materialsJson.isMember("Health based")) materialsConfig.healthBased = materialsJson["Health based"].asBool();
            if (materialsJson.isMember("Blinking")) materialsConfig.blinking = materialsJson["Blinking"].asBool();
            if (materialsJson.isMember("Material")) materialsConfig.material = materialsJson["Material"].asInt();
            if (materialsJson.isMember("Wireframe")) materialsConfig.wireframe = materialsJson["Wireframe"].asBool();
            if (materialsJson.isMember("Color")) {
                const auto& colorJson = materialsJson["Color"];
                auto& colorConfig = materialsConfig.color;

                if (colorJson.isMember("Color")) {
                    colorConfig.color[0] = colorJson["Color"][0].asFloat();
                    colorConfig.color[1] = colorJson["Color"][1].asFloat();
                    colorConfig.color[2] = colorJson["Color"][2].asFloat();
                }

                if (colorJson.isMember("Rainbow")) colorConfig.rainbow = colorJson["Rainbow"].asBool();
                if (colorJson.isMember("Rainbow speed")) colorConfig.rainbowSpeed = colorJson["Rainbow speed"].asFloat();
            }
            if (materialsJson.isMember("Alpha")) materialsConfig.alpha = materialsJson["Alpha"].asFloat();
        }
    }

    for (size_t i = 0; i < esp.players.size(); i++) {
        const auto& espJson = json["Esp"]["Players"][i];
        auto& espConfig = esp.players[i];
        
        if (espJson.isMember("Enabled")) espConfig.enabled = espJson["Enabled"].asBool();
        if (espJson.isMember("Font")) espConfig.font = espJson["Font"].asInt();

        if (espJson.isMember("Snaplines")) {
            const auto& snaplinesJson = espJson["Snaplines"];
            auto& snaplinesConfig = espConfig.snaplines;

            if (snaplinesJson.isMember("Enabled")) snaplinesConfig.enabled = snaplinesJson["Enabled"].asBool();

            if (snaplinesJson.isMember("Color")) {
                snaplinesConfig.color[0] = snaplinesJson["Color"][0].asFloat();
                snaplinesConfig.color[1] = snaplinesJson["Color"][1].asFloat();
                snaplinesConfig.color[2] = snaplinesJson["Color"][2].asFloat();
            }

            if (snaplinesJson.isMember("Rainbow")) snaplinesConfig.rainbow = snaplinesJson["Rainbow"].asBool();
            if (snaplinesJson.isMember("Rainbow speed")) snaplinesConfig.rainbowSpeed = snaplinesJson["Rainbow speed"].asFloat();
        }

        if (espJson.isMember("Eye traces")) {
            const auto& eyeTracesJson = espJson["Eye traces"];
            auto& eyeTracesConfig = espConfig.eyeTraces;

            if (eyeTracesJson.isMember("Enabled")) eyeTracesConfig.enabled = eyeTracesJson["Enabled"].asBool();

            if (eyeTracesJson.isMember("Color")) {
                eyeTracesConfig.color[0] = eyeTracesJson["Color"][0].asFloat();
                eyeTracesConfig.color[1] = eyeTracesJson["Color"][1].asFloat();
                eyeTracesConfig.color[2] = eyeTracesJson["Color"][2].asFloat();
            }

            if (eyeTracesJson.isMember("Rainbow")) eyeTracesConfig.rainbow = eyeTracesJson["Rainbow"].asBool();
            if (eyeTracesJson.isMember("Rainbow speed")) eyeTracesConfig.rainbowSpeed = eyeTracesJson["Rainbow speed"].asFloat();
        }

        if (espJson.isMember("Box")) {
            const auto& boxJson = espJson["Box"];
            auto& boxConfig = espConfig.box;

            if (boxJson.isMember("Enabled")) boxConfig.enabled = boxJson["Enabled"].asBool();

            if (boxJson.isMember("Color")) {
                boxConfig.color[0] = boxJson["Color"][0].asFloat();
                boxConfig.color[1] = boxJson["Color"][1].asFloat();
                boxConfig.color[2] = boxJson["Color"][2].asFloat();
            }

            if (boxJson.isMember("Rainbow")) boxConfig.rainbow = boxJson["Rainbow"].asBool();
            if (boxJson.isMember("Rainbow speed")) boxConfig.rainbowSpeed = boxJson["Rainbow speed"].asFloat();
        }

        if (espJson.isMember("Box type")) espConfig.boxType = espJson["Box type"].asInt();

        if (espJson.isMember("Name")) {
            const auto& nameJson = espJson["Name"];
            auto& nameConfig = espConfig.name;

            if (nameJson.isMember("Enabled")) nameConfig.enabled = nameJson["Enabled"].asBool();

            if (nameJson.isMember("Color")) {
                nameConfig.color[0] = nameJson["Color"][0].asFloat();
                nameConfig.color[1] = nameJson["Color"][1].asFloat();
                nameConfig.color[2] = nameJson["Color"][2].asFloat();
            }

            if (nameJson.isMember("Rainbow")) nameConfig.rainbow = nameJson["Rainbow"].asBool();
            if (nameJson.isMember("Rainbow speed")) nameConfig.rainbowSpeed = nameJson["Rainbow speed"].asFloat();
        }

        if (espJson.isMember("Health")) {
            const auto& healthJson = espJson["Health"];
            auto& healthConfig = espConfig.health;

            if (healthJson.isMember("Enabled")) healthConfig.enabled = healthJson["Enabled"].asBool();

            if (healthJson.isMember("Color")) {
                healthConfig.color[0] = healthJson["Color"][0].asFloat();
                healthConfig.color[1] = healthJson["Color"][1].asFloat();
                healthConfig.color[2] = healthJson["Color"][2].asFloat();
            }

            if (healthJson.isMember("Rainbow")) healthConfig.rainbow = healthJson["Rainbow"].asBool();
            if (healthJson.isMember("Rainbow speed")) healthConfig.rainbowSpeed = healthJson["Rainbow speed"].asFloat();
        }

        if (espJson.isMember("Health bar")) {
            const auto& healthBarJson = espJson["Health bar"];
            auto& healthBarConfig = espConfig.healthBar;

            if (healthBarJson.isMember("Enabled")) healthBarConfig.enabled = healthBarJson["Enabled"].asBool();

            if (healthBarJson.isMember("Color")) {
                healthBarConfig.color[0] = healthBarJson["Color"][0].asFloat();
                healthBarConfig.color[1] = healthBarJson["Color"][1].asFloat();
                healthBarConfig.color[2] = healthBarJson["Color"][2].asFloat();
            }

            if (healthBarJson.isMember("Rainbow")) healthBarConfig.rainbow = healthBarJson["Rainbow"].asBool();
            if (healthBarJson.isMember("Rainbow speed")) healthBarConfig.rainbowSpeed = healthBarJson["Rainbow speed"].asFloat();
        }

        if (espJson.isMember("Armor")) {
            const auto& armorJson = espJson["Armor"];
            auto& armorConfig = espConfig.armor;

            if (armorJson.isMember("Enabled")) armorConfig.enabled = armorJson["Enabled"].asBool();

            if (armorJson.isMember("Color")) {
                armorConfig.color[0] = armorJson["Color"][0].asFloat();
                armorConfig.color[1] = armorJson["Color"][1].asFloat();
                armorConfig.color[2] = armorJson["Color"][2].asFloat();
            }

            if (armorJson.isMember("Rainbow")) armorConfig.rainbow = armorJson["Rainbow"].asBool();
            if (armorJson.isMember("Rainbow speed")) armorConfig.rainbowSpeed = armorJson["Rainbow speed"].asFloat();
        }

        if (espJson.isMember("Armor bar")) {
            const auto& armorBarJson = espJson["Armor bar"];
            auto& armorBarConfig = espConfig.armorBar;

            if (armorBarJson.isMember("Enabled")) armorBarConfig.enabled = armorBarJson["Enabled"].asBool();

            if (armorBarJson.isMember("Color")) {
                armorBarConfig.color[0] = armorBarJson["Color"][0].asFloat();
                armorBarConfig.color[1] = armorBarJson["Color"][1].asFloat();
                armorBarConfig.color[2] = armorBarJson["Color"][2].asFloat();
            }

            if (armorBarJson.isMember("Rainbow")) armorBarConfig.rainbow = armorBarJson["Rainbow"].asBool();
            if (armorBarJson.isMember("Rainbow speed")) armorBarConfig.rainbowSpeed = armorBarJson["Rainbow speed"].asFloat();
        }

        if (espJson.isMember("Money")) {
            const auto& moneyJson = espJson["Money"];
            auto& moneyConfig = espConfig.money;

            if (moneyJson.isMember("Enabled")) moneyConfig.enabled = moneyJson["Enabled"].asBool();

            if (moneyJson.isMember("Color")) {
                moneyConfig.color[0] = moneyJson["Color"][0].asFloat();
                moneyConfig.color[1] = moneyJson["Color"][1].asFloat();
                moneyConfig.color[2] = moneyJson["Color"][2].asFloat();
            }

            if (moneyJson.isMember("Rainbow")) moneyConfig.rainbow = moneyJson["Rainbow"].asBool();
            if (moneyJson.isMember("Rainbow speed")) moneyConfig.rainbowSpeed = moneyJson["Rainbow speed"].asFloat();
        }

        if (espJson.isMember("Head dot")) {
            const auto& headDotJson = espJson["Head dot"];
            auto& headDotConfig = espConfig.headDot;

            if (headDotJson.isMember("Enabled")) headDotConfig.enabled = headDotJson["Enabled"].asBool();

            if (headDotJson.isMember("Color")) {
                headDotConfig.color[0] = headDotJson["Color"][0].asFloat();
                headDotConfig.color[1] = headDotJson["Color"][1].asFloat();
                headDotConfig.color[2] = headDotJson["Color"][2].asFloat();
            }

            if (headDotJson.isMember("Rainbow")) headDotConfig.rainbow = headDotJson["Rainbow"].asBool();
            if (headDotJson.isMember("Rainbow speed")) headDotConfig.rainbowSpeed = headDotJson["Rainbow speed"].asFloat();
        }

        if (espJson.isMember("Active weapon")) {
            const auto& activeWeaponJson = espJson["Active weapon"];
            auto& activeWeaponConfig = espConfig.activeWeapon;

            if (activeWeaponJson.isMember("Enabled")) activeWeaponConfig.enabled = activeWeaponJson["Enabled"].asBool();

            if (activeWeaponJson.isMember("Color")) {
                activeWeaponConfig.color[0] = activeWeaponJson["Color"][0].asFloat();
                activeWeaponConfig.color[1] = activeWeaponJson["Color"][1].asFloat();
                activeWeaponConfig.color[2] = activeWeaponJson["Color"][2].asFloat();
            }

            if (activeWeaponJson.isMember("Rainbow")) activeWeaponConfig.rainbow = activeWeaponJson["Rainbow"].asBool();
            if (activeWeaponJson.isMember("Rainbow speed")) activeWeaponConfig.rainbowSpeed = activeWeaponJson["Rainbow speed"].asFloat();
        }

        if (espJson.isMember("Outline")) {
            const auto& outlineJson = espJson["Outline"];
            auto& outlineConfig = espConfig.outline;

            if (outlineJson.isMember("Enabled")) outlineConfig.enabled = outlineJson["Enabled"].asBool();

            if (outlineJson.isMember("Color")) {
                outlineConfig.color[0] = outlineJson["Color"][0].asFloat();
                outlineConfig.color[1] = outlineJson["Color"][1].asFloat();
                outlineConfig.color[2] = outlineJson["Color"][2].asFloat();
            }

            if (outlineJson.isMember("Rainbow")) outlineConfig.rainbow = outlineJson["Rainbow"].asBool();
            if (outlineJson.isMember("Rainbow speed")) outlineConfig.rainbowSpeed = outlineJson["Rainbow speed"].asFloat();
        }

        if (espJson.isMember("Distance")) {
            const auto& distanceJson = espJson["Distance"];
            auto& distanceConfig = espConfig.distance;

            if (distanceJson.isMember("Enabled")) distanceConfig.enabled = distanceJson["Enabled"].asBool();

            if (distanceJson.isMember("Color")) {
                distanceConfig.color[0] = distanceJson["Color"][0].asFloat();
                distanceConfig.color[1] = distanceJson["Color"][1].asFloat();
                distanceConfig.color[2] = distanceJson["Color"][2].asFloat();
            }

            if (distanceJson.isMember("Rainbow")) distanceConfig.rainbow = distanceJson["Rainbow"].asBool();
            if (distanceJson.isMember("Rainbow speed")) distanceConfig.rainbowSpeed = distanceJson["Rainbow speed"].asFloat();
        }
        
        if (espJson.isMember("Dead ESP")) espConfig.deadesp = espJson["Dead ESP"].asBool();
        if (espJson.isMember("Sound esp")) espConfig.soundEsp = espJson["Sound esp"].asBool();
        if (espJson.isMember("Max distance")) espConfig.maxDistance = espJson["Max distance"].asFloat();
    }

    {
        const auto& espJson = json["Esp"]["Weapons"];
        auto& espConfig = esp.weapon;

        if (espJson.isMember("Enabled")) espConfig.enabled = espJson["Enabled"].asBool();
        if (espJson.isMember("Font")) espConfig.font = espJson["Font"].asInt();
        if (espJson.isMember("Snaplines")) {
            const auto& snaplinesJson = espJson["Snaplines"];
            auto& snaplinesConfig = espConfig.snaplines;

            if (snaplinesJson.isMember("Enabled")) snaplinesConfig.enabled = snaplinesJson["Enabled"].asBool();

            if (snaplinesJson.isMember("Color")) {
                snaplinesConfig.color[0] = snaplinesJson["Color"][0].asFloat();
                snaplinesConfig.color[1] = snaplinesJson["Color"][1].asFloat();
                snaplinesConfig.color[2] = snaplinesJson["Color"][2].asFloat();
            }

            if (snaplinesJson.isMember("Rainbow")) snaplinesConfig.rainbow = snaplinesJson["Rainbow"].asBool();
            if (snaplinesJson.isMember("Rainbow speed")) snaplinesConfig.rainbowSpeed = snaplinesJson["Rainbow speed"].asFloat();
        }

        if (espJson.isMember("Box")) {
            const auto& boxJson = espJson["Box"];
            auto& boxConfig = espConfig.box;

            if (boxJson.isMember("Enabled")) boxConfig.enabled = boxJson["Enabled"].asBool();

            if (boxJson.isMember("Color")) {
                boxConfig.color[0] = boxJson["Color"][0].asFloat();
                boxConfig.color[1] = boxJson["Color"][1].asFloat();
                boxConfig.color[2] = boxJson["Color"][2].asFloat();
            }

            if (boxJson.isMember("Rainbow")) boxConfig.rainbow = boxJson["Rainbow"].asBool();
            if (boxJson.isMember("Rainbow speed")) boxConfig.rainbowSpeed = boxJson["Rainbow speed"].asFloat();
        }

        if (espJson.isMember("Box type")) espConfig.boxType = espJson["Box type"].asInt();

        if (espJson.isMember("Outline")) {
            const auto& outlineJson = espJson["Outline"];
            auto& outlineConfig = espConfig.outline;

            if (outlineJson.isMember("Enabled")) outlineConfig.enabled = outlineJson["Enabled"].asBool();

            if (outlineJson.isMember("Color")) {
                outlineConfig.color[0] = outlineJson["Color"][0].asFloat();
                outlineConfig.color[1] = outlineJson["Color"][1].asFloat();
                outlineConfig.color[2] = outlineJson["Color"][2].asFloat();
            }

            if (outlineJson.isMember("Rainbow")) outlineConfig.rainbow = outlineJson["Rainbow"].asBool();
            if (outlineJson.isMember("Rainbow speed")) outlineConfig.rainbowSpeed = outlineJson["Rainbow speed"].asFloat();
        }

        if (espJson.isMember("Name")) {
            const auto& nameJson = espJson["Name"];
            auto& nameConfig = espConfig.name;

            if (nameJson.isMember("Enabled")) nameConfig.enabled = nameJson["Enabled"].asBool();

            if (nameJson.isMember("Color")) {
                nameConfig.color[0] = nameJson["Color"][0].asFloat();
                nameConfig.color[1] = nameJson["Color"][1].asFloat();
                nameConfig.color[2] = nameJson["Color"][2].asFloat();
            }

            if (nameJson.isMember("Rainbow")) nameConfig.rainbow = nameJson["Rainbow"].asBool();
            if (nameJson.isMember("Rainbow speed")) nameConfig.rainbowSpeed = nameJson["Rainbow speed"].asFloat();
        }

        if (espJson.isMember("Distance")) {
            const auto& distanceJson = espJson["Distance"];
            auto& distanceConfig = espConfig.distance;

            if (distanceJson.isMember("Enabled")) distanceConfig.enabled = distanceJson["Enabled"].asBool();

            if (distanceJson.isMember("Color")) {
                distanceConfig.color[0] = distanceJson["Color"][0].asFloat();
                distanceConfig.color[1] = distanceJson["Color"][1].asFloat();
                distanceConfig.color[2] = distanceJson["Color"][2].asFloat();
            }

            if (distanceJson.isMember("Rainbow")) distanceConfig.rainbow = distanceJson["Rainbow"].asBool();
            if (distanceJson.isMember("Rainbow speed")) distanceConfig.rainbowSpeed = distanceJson["Rainbow speed"].asFloat();
        }

        if (espJson.isMember("Max distance")) espConfig.maxDistance = espJson["Max distance"].asFloat();
    }

    for (size_t i = 0; i < esp.dangerZone.size(); i++) {
        const auto& espJson = json["Esp"]["Danger Zone"][i];
        auto& espConfig = esp.dangerZone[i];

        if (espJson.isMember("Enabled")) espConfig.enabled = espJson["Enabled"].asBool();
        if (espJson.isMember("Font")) espConfig.font = espJson["Font"].asInt();
        if (espJson.isMember("Snaplines")) {
            const auto& snaplinesJson = espJson["Snaplines"];
            auto& snaplinesConfig = espConfig.snaplines;

            if (snaplinesJson.isMember("Enabled")) snaplinesConfig.enabled = snaplinesJson["Enabled"].asBool();

            if (snaplinesJson.isMember("Color")) {
                snaplinesConfig.color[0] = snaplinesJson["Color"][0].asFloat();
                snaplinesConfig.color[1] = snaplinesJson["Color"][1].asFloat();
                snaplinesConfig.color[2] = snaplinesJson["Color"][2].asFloat();
            }

            if (snaplinesJson.isMember("Rainbow")) snaplinesConfig.rainbow = snaplinesJson["Rainbow"].asBool();
            if (snaplinesJson.isMember("Rainbow speed")) snaplinesConfig.rainbowSpeed = snaplinesJson["Rainbow speed"].asFloat();
        }
        
        if (espJson.isMember("Box")) {
            const auto& boxJson = espJson["Box"];
            auto& boxConfig = espConfig.box;

            if (boxJson.isMember("Enabled")) boxConfig.enabled = boxJson["Enabled"].asBool();

            if (boxJson.isMember("Color")) {
                boxConfig.color[0] = boxJson["Color"][0].asFloat();
                boxConfig.color[1] = boxJson["Color"][1].asFloat();
                boxConfig.color[2] = boxJson["Color"][2].asFloat();
            }

            if (boxJson.isMember("Rainbow")) boxConfig.rainbow = boxJson["Rainbow"].asBool();
            if (boxJson.isMember("Rainbow speed")) boxConfig.rainbowSpeed = boxJson["Rainbow speed"].asFloat();
        }

        if (espJson.isMember("Box type")) espConfig.boxType = espJson["Box type"].asInt();

        if (espJson.isMember("Outline")) {
            const auto& outlineJson = espJson["Outline"];
            auto& outlineConfig = espConfig.outline;

            if (outlineJson.isMember("Enabled")) outlineConfig.enabled = outlineJson["Enabled"].asBool();

            if (outlineJson.isMember("Color")) {
                outlineConfig.color[0] = outlineJson["Color"][0].asFloat();
                outlineConfig.color[1] = outlineJson["Color"][1].asFloat();
                outlineConfig.color[2] = outlineJson["Color"][2].asFloat();
            }

            if (outlineJson.isMember("Rainbow")) outlineConfig.rainbow = outlineJson["Rainbow"].asBool();
            if (outlineJson.isMember("Rainbow speed")) outlineConfig.rainbowSpeed = outlineJson["Rainbow speed"].asFloat();
        }

        if (espJson.isMember("Name")) {
            const auto& nameJson = espJson["Name"];
            auto& nameConfig = espConfig.name;

            if (nameJson.isMember("Enabled")) nameConfig.enabled = nameJson["Enabled"].asBool();

            if (nameJson.isMember("Color")) {
                nameConfig.color[0] = nameJson["Color"][0].asFloat();
                nameConfig.color[1] = nameJson["Color"][1].asFloat();
                nameConfig.color[2] = nameJson["Color"][2].asFloat();
            }

            if (nameJson.isMember("Rainbow")) nameConfig.rainbow = nameJson["Rainbow"].asBool();
            if (nameJson.isMember("Rainbow speed")) nameConfig.rainbowSpeed = nameJson["Rainbow speed"].asFloat();
        }

        if (espJson.isMember("Distance")) {
            const auto& distanceJson = espJson["Distance"];
            auto& distanceConfig = espConfig.distance;

            if (distanceJson.isMember("Enabled")) distanceConfig.enabled = distanceJson["Enabled"].asBool();

            if (distanceJson.isMember("Color")) {
                distanceConfig.color[0] = distanceJson["Color"][0].asFloat();
                distanceConfig.color[1] = distanceJson["Color"][1].asFloat();
                distanceConfig.color[2] = distanceJson["Color"][2].asFloat();
            }

            if (distanceJson.isMember("Rainbow")) distanceConfig.rainbow = distanceJson["Rainbow"].asBool();
            if (distanceJson.isMember("Rainbow speed")) distanceConfig.rainbowSpeed = distanceJson["Rainbow speed"].asFloat();
        }

        if (espJson.isMember("Max distance")) espConfig.maxDistance = espJson["Max distance"].asFloat();
    }

    for (size_t i = 0; i < esp.projectiles.size(); i++) {
        const auto& espJson = json["Esp"]["Projectiles"][i];
        auto& espConfig = esp.projectiles[i];

        if (espJson.isMember("Enabled")) espConfig.enabled = espJson["Enabled"].asBool();
        if (espJson.isMember("Font")) espConfig.font = espJson["Font"].asInt();
        if (espJson.isMember("Snaplines")) {
            const auto& snaplinesJson = espJson["Snaplines"];
            auto& snaplinesConfig = espConfig.snaplines;

            if (snaplinesJson.isMember("Enabled")) snaplinesConfig.enabled = snaplinesJson["Enabled"].asBool();

            if (snaplinesJson.isMember("Color")) {
                snaplinesConfig.color[0] = snaplinesJson["Color"][0].asFloat();
                snaplinesConfig.color[1] = snaplinesJson["Color"][1].asFloat();
                snaplinesConfig.color[2] = snaplinesJson["Color"][2].asFloat();
            }

            if (snaplinesJson.isMember("Rainbow")) snaplinesConfig.rainbow = snaplinesJson["Rainbow"].asBool();
            if (snaplinesJson.isMember("Rainbow speed")) snaplinesConfig.rainbowSpeed = snaplinesJson["Rainbow speed"].asFloat();
        }
        if (espJson.isMember("Box")) {
            const auto& boxJson = espJson["Box"];
            auto& boxConfig = espConfig.box;

            if (boxJson.isMember("Enabled")) boxConfig.enabled = boxJson["Enabled"].asBool();

            if (boxJson.isMember("Color")) {
                boxConfig.color[0] = boxJson["Color"][0].asFloat();
                boxConfig.color[1] = boxJson["Color"][1].asFloat();
                boxConfig.color[2] = boxJson["Color"][2].asFloat();
            }

            if (boxJson.isMember("Rainbow")) boxConfig.rainbow = boxJson["Rainbow"].asBool();
            if (boxJson.isMember("Rainbow speed")) boxConfig.rainbowSpeed = boxJson["Rainbow speed"].asFloat();
        }

        if (espJson.isMember("Box type")) espConfig.boxType = espJson["Box type"].asInt();

        if (espJson.isMember("Outline")) {
            const auto& outlineJson = espJson["Outline"];
            auto& outlineConfig = espConfig.outline;

            if (outlineJson.isMember("Enabled")) outlineConfig.enabled = outlineJson["Enabled"].asBool();

            if (outlineJson.isMember("Color")) {
                outlineConfig.color[0] = outlineJson["Color"][0].asFloat();
                outlineConfig.color[1] = outlineJson["Color"][1].asFloat();
                outlineConfig.color[2] = outlineJson["Color"][2].asFloat();
            }

            if (outlineJson.isMember("Rainbow")) outlineConfig.rainbow = outlineJson["Rainbow"].asBool();
            if (outlineJson.isMember("Rainbow speed")) outlineConfig.rainbowSpeed = outlineJson["Rainbow speed"].asFloat();
        }

        if (espJson.isMember("Name")) {
            const auto& nameJson = espJson["Name"];
            auto& nameConfig = espConfig.name;

            if (nameJson.isMember("Enabled")) nameConfig.enabled = nameJson["Enabled"].asBool();

            if (nameJson.isMember("Color")) {
                nameConfig.color[0] = nameJson["Color"][0].asFloat();
                nameConfig.color[1] = nameJson["Color"][1].asFloat();
                nameConfig.color[2] = nameJson["Color"][2].asFloat();
            }

            if (nameJson.isMember("Rainbow")) nameConfig.rainbow = nameJson["Rainbow"].asBool();
            if (nameJson.isMember("Rainbow speed")) nameConfig.rainbowSpeed = nameJson["Rainbow speed"].asFloat();
        }

        if (espJson.isMember("Distance")) {
            const auto& distanceJson = espJson["Distance"];
            auto& distanceConfig = espConfig.distance;

            if (distanceJson.isMember("Enabled")) distanceConfig.enabled = distanceJson["Enabled"].asBool();

            if (distanceJson.isMember("Color")) {
                distanceConfig.color[0] = distanceJson["Color"][0].asFloat();
                distanceConfig.color[1] = distanceJson["Color"][1].asFloat();
                distanceConfig.color[2] = distanceJson["Color"][2].asFloat();
            }

            if (distanceJson.isMember("Rainbow")) distanceConfig.rainbow = distanceJson["Rainbow"].asBool();
            if (distanceJson.isMember("Rainbow speed")) distanceConfig.rainbowSpeed = distanceJson["Rainbow speed"].asFloat();
        }

        if (espJson.isMember("Max distance")) espConfig.maxDistance = espJson["Max distance"].asFloat();
    }

    {
        const auto& visualsJson = json["visuals"];
        if (visualsJson.isMember("disablePostProcessing")) visuals.disablePostProcessing = visualsJson["disablePostProcessing"].asBool();
        if (visualsJson.isMember("inverseRagdollGravity")) visuals.inverseRagdollGravity = visualsJson["inverseRagdollGravity"].asBool();
        if (visualsJson.isMember("noFog")) visuals.noFog = visualsJson["noFog"].asBool();
        if (visualsJson.isMember("no3dSky")) visuals.no3dSky = visualsJson["no3dSky"].asBool();
        if (visualsJson.isMember("No aim punch")) visuals.noAimPunch = visualsJson["No aim punch"].asBool();
        if (visualsJson.isMember("No view punch")) visuals.noViewPunch = visualsJson["No view punch"].asBool();
        if (visualsJson.isMember("noHands")) visuals.noHands = visualsJson["noHands"].asBool();
        if (visualsJson.isMember("noSleeves")) visuals.noSleeves = visualsJson["noSleeves"].asBool();
        if (visualsJson.isMember("noWeapons")) visuals.noWeapons = visualsJson["noWeapons"].asBool();
        if (visualsJson.isMember("noSmoke")) visuals.noSmoke = visualsJson["noSmoke"].asBool();
        if (visualsJson.isMember("noBlur")) visuals.noBlur = visualsJson["noBlur"].asBool();
        if (visualsJson.isMember("noScopeOverlay")) visuals.noScopeOverlay = visualsJson["noScopeOverlay"].asBool();
        if (visualsJson.isMember("noGrass")) visuals.noGrass = visualsJson["noGrass"].asBool();
        if (visualsJson.isMember("noShadows")) visuals.noShadows = visualsJson["noShadows"].asBool();
        if (visualsJson.isMember("wireframeSmoke")) visuals.wireframeSmoke = visualsJson["wireframeSmoke"].asBool();
        if (visualsJson.isMember("Zoom")) visuals.zoom = visualsJson["Zoom"].asBool();
        if (visualsJson.isMember("Zoom key")) visuals.zoomKey = visualsJson["Zoom key"].asInt();
        if (visualsJson.isMember("thirdperson")) visuals.thirdperson = visualsJson["thirdperson"].asBool();
        if (visualsJson.isMember("thirdpersonSpectate")) visuals.thirdpersonspectate = visualsJson["thirdpersonSpectate"].asBool();
        if (visualsJson.isMember("thirdpersonKey")) visuals.thirdpersonKey = visualsJson["thirdpersonKey"].asInt();
        if (visualsJson.isMember("thirdpersonDistance")) visuals.thirdpersonDistance = visualsJson["thirdpersonDistance"].asInt();
        if (visualsJson.isMember("viewmodelFov")) visuals.viewmodelFov = visualsJson["viewmodelFov"].asInt();
        if (visualsJson.isMember("Fov")) visuals.fov = visualsJson["Fov"].asInt();
        if (visualsJson.isMember("farZ")) visuals.farZ = visualsJson["farZ"].asInt();
        if (visualsJson.isMember("flashReduction")) visuals.flashReduction = visualsJson["flashReduction"].asInt();
        if (visualsJson.isMember("brightness")) visuals.brightness = visualsJson["brightness"].asFloat();
        if (visualsJson.isMember("skybox")) visuals.skybox = visualsJson["skybox"].asInt();
        if (visualsJson.isMember("World")) {
            const auto& worldJson = visualsJson["World"];

            if (worldJson.isMember("Enabled")) visuals.world.enabled = worldJson["Enabled"].asBool();

            if (worldJson.isMember("Color")) {
                visuals.world.color[0] = worldJson["Color"][0].asFloat();
                visuals.world.color[1] = worldJson["Color"][1].asFloat();
                visuals.world.color[2] = worldJson["Color"][2].asFloat();
            }
            if (worldJson.isMember("Rainbow")) visuals.world.rainbow = worldJson["Rainbow"].asBool();
            if (worldJson.isMember("Rainbow speed")) visuals.world.rainbowSpeed = worldJson["Rainbow speed"].asFloat();
        }
        if (visualsJson.isMember("Sky")) {
            const auto& skyJson = visualsJson["Sky"];

            if (skyJson.isMember("Enabled")) visuals.sky.enabled = skyJson["Enabled"].asBool();

            if (skyJson.isMember("Color")) {
                visuals.sky.color[0] = skyJson["Color"][0].asFloat();
                visuals.sky.color[1] = skyJson["Color"][1].asFloat();
                visuals.sky.color[2] = skyJson["Color"][2].asFloat();
            }
            if (skyJson.isMember("Rainbow")) visuals.sky.rainbow = skyJson["Rainbow"].asBool();
            if (skyJson.isMember("Rainbow speed")) visuals.sky.rainbowSpeed = skyJson["Rainbow speed"].asFloat();
        }
        if (visualsJson.isMember("Deagle spinner")) visuals.deagleSpinner = visualsJson["Deagle spinner"].asBool();
        if (visualsJson.isMember("Screen effect")) visuals.screenEffect = visualsJson["Screen effect"].asInt();
        if (visualsJson.isMember("Hit effect")) visuals.hitEffect = visualsJson["Hit effect"].asInt();
        if (visualsJson.isMember("Hit effect time")) visuals.hitEffectTime = visualsJson["Hit effect time"].asFloat();
        if (visualsJson.isMember("Hit marker")) visuals.hitMarker = visualsJson["Hit marker"].asInt();
        if (visualsJson.isMember("Hit marker time")) visuals.hitMarkerTime = visualsJson["Hit marker time"].asFloat();
        if (visualsJson.isMember("Playermodel T")) visuals.playerModelT = visualsJson["Playermodel T"].asInt();
        if (visualsJson.isMember("Playermodel CT")) visuals.playerModelCT = visualsJson["Playermodel CT"].asInt();

        if (visualsJson.isMember("Color correction")) {
            const auto& cc = visualsJson["Color correction"];

            if (cc.isMember("Enabled")) visuals.colorCorrection.enabled = cc["Enabled"].asBool();
            if (cc.isMember("Blue")) visuals.colorCorrection.blue = cc["Blue"].asFloat();
            if (cc.isMember("Red")) visuals.colorCorrection.red = cc["Red"].asFloat();
            if (cc.isMember("Mono")) visuals.colorCorrection.mono = cc["Mono"].asFloat();
            if (cc.isMember("Saturation")) visuals.colorCorrection.saturation = cc["Saturation"].asFloat();
            if (cc.isMember("Ghost")) visuals.colorCorrection.ghost = cc["Ghost"].asFloat();
            if (cc.isMember("Green")) visuals.colorCorrection.green = cc["Green"].asFloat();
            if (cc.isMember("Yellow")) visuals.colorCorrection.yellow = cc["Yellow"].asFloat();
        }
    }

    for (size_t i = 0; i < skinChanger.size(); i++) {
        const auto& skinChangerJson = json["skinChanger"][i];
        auto& skinChangerConfig = skinChanger[i];

        if (skinChangerJson.isMember("Enabled")) skinChangerConfig.enabled = skinChangerJson["Enabled"].asBool();
        if (skinChangerJson.isMember("definition_vector_index")) skinChangerConfig.itemIdIndex = skinChangerJson["definition_vector_index"].asInt();
        if (skinChangerJson.isMember("definition_index")) skinChangerConfig.itemId = skinChangerJson["definition_index"].asInt();
        if (skinChangerJson.isMember("entity_quality_vector_index")) skinChangerConfig.entity_quality_vector_index = skinChangerJson["entity_quality_vector_index"].asInt();
        if (skinChangerJson.isMember("entity_quality_index")) skinChangerConfig.quality = skinChangerJson["entity_quality_index"].asInt();
        if (skinChangerJson.isMember("paint_kit_vector_index")) skinChangerConfig.paint_kit_vector_index = skinChangerJson["paint_kit_vector_index"].asInt();
        if (skinChangerJson.isMember("paint_kit_index")) skinChangerConfig.paintKit = skinChangerJson["paint_kit_index"].asInt();
        if (skinChangerJson.isMember("definition_override_vector_index")) skinChangerConfig.definition_override_vector_index = skinChangerJson["definition_override_vector_index"].asInt();
        if (skinChangerJson.isMember("definition_override_index")) skinChangerConfig.definition_override_index = skinChangerJson["definition_override_index"].asInt();
        if (skinChangerJson.isMember("seed")) skinChangerConfig.seed = skinChangerJson["seed"].asInt();
        if (skinChangerJson.isMember("stat_trak")) skinChangerConfig.stat_trak = skinChangerJson["stat_trak"].asInt();
        if (skinChangerJson.isMember("wear")) skinChangerConfig.wear = skinChangerJson["wear"].asFloat();
        if (skinChangerJson.isMember("custom_name")) strcpy_s(skinChangerConfig.custom_name, sizeof(skinChangerConfig.custom_name), skinChangerJson["custom_name"].asCString());

        if (skinChangerJson.isMember("stickers")) {
            for (size_t j = 0; j < skinChangerConfig.stickers.size(); j++) {
                const auto& stickerJson = skinChangerJson["stickers"][j];
                auto& stickerConfig = skinChangerConfig.stickers[j];

                if (stickerJson.isMember("kit")) stickerConfig.kit = stickerJson["kit"].asInt();
                if (stickerJson.isMember("kit_vector_index")) stickerConfig.kit_vector_index = stickerJson["kit_vector_index"].asInt();
                if (stickerJson.isMember("wear")) stickerConfig.wear = stickerJson["wear"].asFloat();
                if (stickerJson.isMember("scale")) stickerConfig.scale = stickerJson["scale"].asFloat();
                if (stickerJson.isMember("rotation")) stickerConfig.rotation = stickerJson["rotation"].asFloat();
            }
        }
    }

    {
        const auto& soundJson = json["Sound"];

        if (soundJson.isMember("Chicken volume")) sound.chickenVolume = soundJson["Chicken volume"].asInt();

        if (soundJson.isMember("Players")) {
            for (size_t i = 0; i < sound.players.size(); i++) {
                const auto& playerJson = soundJson["Players"][i];
                auto& playerConfig = sound.players[i];

                if (playerJson.isMember("Master volume")) playerConfig.masterVolume = playerJson["Master volume"].asInt();
                if (playerJson.isMember("Headshot volume")) playerConfig.headshotVolume = playerJson["Headshot volume"].asInt();
                if (playerJson.isMember("Weapon volume")) playerConfig.weaponVolume = playerJson["Weapon volume"].asInt();
                if (playerJson.isMember("Footstep volume")) playerConfig.footstepVolume = playerJson["Footstep volume"].asInt();
            }
        }
    }


    {
        const auto& styleJson = json["Style"];

        if (styleJson.isMember("Menu style")) style.menuStyle = styleJson["Menu style"].asInt();
        if (styleJson.isMember("Menu colors")) style.menuColors = styleJson["Menu colors"].asInt();

        if (styleJson.isMember("Colors")) {
            const auto& colorsJson = styleJson["Colors"];

            ImGuiStyle& style = ImGui::GetStyle();

            for (int i = 0; i < ImGuiCol_COUNT; i++) {
                if (const char* name = ImGui::GetStyleColorName(i); colorsJson.isMember(name)) {
                    const auto& colorJson = styleJson["Colors"][name];
                    style.Colors[i].x = colorJson[0].asFloat();
                    style.Colors[i].y = colorJson[1].asFloat();
                    style.Colors[i].z = colorJson[2].asFloat();
                    style.Colors[i].w = colorJson[3].asFloat();
                }
            }
        }
    }

    {
        const auto& miscJson = json["Misc"];

        if (miscJson.isMember("Menu key")) misc.menuKey = miscJson["Menu key"].asInt();
        if (miscJson.isMember("Anti AFK kick")) misc.antiAfkKick = miscJson["Anti AFK kick"].asBool();
        if (miscJson.isMember("Auto strafe")) misc.autoStrafe = miscJson["Auto strafe"].asBool();
        if (miscJson.isMember("Bunny hop")) misc.bunnyHop = miscJson["Bunny hop"].asBool();
        if (miscJson.isMember("Custom clan tag")) misc.customClanTag = miscJson["Custom clan tag"].asBool();
        if (miscJson.isMember("Clock tag")) misc.clocktag = miscJson["Clock tag"].asBool();
        if (miscJson.isMember("Clan tag")) misc.clanTag = miscJson["Clan tag"].asString();
        if (miscJson.isMember("Animated clan tag")) misc.animatedClanTag = miscJson["Animated clan tag"].asBool();
        if (miscJson.isMember("Fast duck")) misc.fastDuck = miscJson["Fast duck"].asBool();
        if (miscJson.isMember("Moonwalk")) misc.moonwalk = miscJson["Moonwalk"].asBool();
        if (miscJson.isMember("Edge Jump")) misc.edgejump = miscJson["Edge Jump"].asBool();
        if (miscJson.isMember("Edge Jump Key")) misc.edgejumpkey = miscJson["Edge Jump Key"].asInt();
        if (miscJson.isMember("Slowwalk")) misc.slowwalk = miscJson["Slowwalk"].asBool();
        if (miscJson.isMember("Slowwalk key")) misc.slowwalkKey = miscJson["Slowwalk key"].asInt();
        if (miscJson.isMember("Sniper crosshair")) misc.sniperCrosshair = miscJson["Sniper crosshair"].asBool();
        if (miscJson.isMember("Recoil crosshair")) misc.recoilCrosshair = miscJson["Recoil crosshair"].asBool();
        if (miscJson.isMember("Auto pistol")) misc.autoPistol = miscJson["Auto pistol"].asBool();
        if (miscJson.isMember("Auto reload")) misc.autoReload = miscJson["Auto reload"].asBool();
        if (miscJson.isMember("Auto accept")) misc.autoAccept = miscJson["Auto accept"].asBool();
        if (miscJson.isMember("Radar hack")) misc.radarHack = miscJson["Radar hack"].asBool();
        if (miscJson.isMember("Reveal ranks")) misc.revealRanks = miscJson["Reveal ranks"].asBool();
        if (miscJson.isMember("Reveal money")) misc.revealMoney = miscJson["Reveal money"].asBool();
        if (miscJson.isMember("Reveal suspect")) misc.revealSuspect = miscJson["Reveal suspect"].asBool();

        if (const auto& spectatorList{ miscJson["Spectator list"] }; spectatorList.isObject()) {
            if (const auto& enabled{ spectatorList["Enabled"] }; enabled.isBool())
                misc.spectatorList.enabled = enabled.asBool();

            if (const auto& color{ spectatorList["Color"] }; color.isArray()) {
                misc.spectatorList.color[0] = color[0].asFloat();
                misc.spectatorList.color[1] = color[1].asFloat();
                misc.spectatorList.color[2] = color[2].asFloat();
            }
            if (const auto& rainbow{ spectatorList["Rainbow"] }; rainbow.isBool())
                misc.spectatorList.rainbow = rainbow.asBool();

            if (const auto& rainbowSpeed{ spectatorList["Rainbow speed"] }; rainbowSpeed.isDouble())
                misc.spectatorList.rainbowSpeed = rainbowSpeed.asFloat();
        }

        if (const auto& watermark{ miscJson["Watermark"] }; watermark.isObject()) {
            if (const auto& enabled{ watermark["Enabled"] }; enabled.isBool())
                misc.watermark.enabled = enabled.asBool();

            if (const auto& color{ watermark["Color"] }; color.isArray()) {
                misc.watermark.color[0] = color[0].asFloat();
                misc.watermark.color[1] = color[1].asFloat();
                misc.watermark.color[2] = color[2].asFloat();
            }
            if (const auto& rainbow{ watermark["Rainbow"] }; rainbow.isBool())
                misc.watermark.rainbow = rainbow.asBool();

            if (const auto& rainbowSpeed{ watermark["Rainbow speed"] }; rainbowSpeed.isDouble())
                misc.watermark.rainbowSpeed = rainbowSpeed.asFloat();
        }

        if (miscJson.isMember("Fix animation LOD")) misc.fixAnimationLOD = miscJson["Fix animation LOD"].asBool();
        if (miscJson.isMember("Fix bone matrix")) misc.fixBoneMatrix = miscJson["Fix bone matrix"].asBool();
        if (miscJson.isMember("Fix movement")) misc.fixMovement = miscJson["Fix movement"].asBool();
        if (miscJson.isMember("Disable model occlusion")) misc.disableModelOcclusion = miscJson["Disable model occlusion"].asBool();
        if (miscJson.isMember("Aspect Ratio")) misc.aspectratio = miscJson["Aspect Ratio"].asFloat();
        if (miscJson.isMember("Kill message")) misc.killMessage = miscJson["Kill message"].asBool();
        if (miscJson.isMember("Kill message string")) misc.killMessageString = miscJson["Kill message string"].asString();
        if (miscJson.isMember("Name stealer"))  misc.nameStealer = miscJson["Name stealer"].asBool();
        if (miscJson.isMember("Disable HUD blur"))  misc.disablePanoramablur = miscJson["Disable HUD blur"].asBool();
        if (miscJson.isMember("Ban color")) misc.banColor = miscJson["Ban color"].asInt();
        if (miscJson.isMember("Ban text")) misc.banText = miscJson["Ban text"].asString();
        if (miscJson.isMember("Fast plant")) misc.fastPlant = miscJson["Fast plant"].asBool();

        if (const auto& bombTimer{ miscJson["Bomb timer"] }; bombTimer.isObject()) {
            if (const auto& enabled{ bombTimer["Enabled"] }; enabled.isBool())
                misc.bombTimer.enabled = enabled.asBool();

            if (const auto& color{ bombTimer["Color"] }; color.isArray()) {
                misc.bombTimer.color[0] = color[0].asFloat();
                misc.bombTimer.color[1] = color[1].asFloat();
                misc.bombTimer.color[2] = color[2].asFloat();
            }
            if (const auto& rainbow{ bombTimer["Rainbow"] }; rainbow.isBool())
                misc.bombTimer.rainbow = rainbow.asBool();

            if (const auto& rainbowSpeed{ bombTimer["Rainbow speed"] }; rainbowSpeed.isDouble())
                misc.bombTimer.rainbowSpeed = rainbowSpeed.asFloat();
        }

        if (miscJson.isMember("Quick reload")) misc.quickReload = miscJson["Quick reload"].asBool();
        if (miscJson.isMember("Prepare revolver")) misc.prepareRevolver = miscJson["Prepare revolver"].asBool();
        if (miscJson.isMember("Prepare revolver key")) misc.prepareRevolverKey = miscJson["Prepare revolver key"].asInt();
        if (miscJson.isMember("Hit sound")) misc.hitSound = miscJson["Hit sound"].asInt();
        if (miscJson.isMember("Choked packets")) misc.chokedPackets = miscJson["Choked packets"].asInt();
        if (miscJson.isMember("Choked packets key")) misc.chokedPacketsKey = miscJson["Choked packets key"].asInt();
        if (miscJson.isMember("Quick healthshot key")) misc.quickHealthshotKey = miscJson["Quick healthshot key"].asInt();
        if (miscJson.isMember("Grenade predict")) misc.nadePredict = miscJson["Grenade predict"].asBool();
        if (miscJson.isMember("Fix tablet signal")) misc.fixTabletSignal = miscJson["Fix tablet signal"].asBool();
        if (miscJson.isMember("Max angle delta")) misc.maxAngleDelta = miscJson["Max angle delta"].asFloat();
        if (miscJson.isMember("Fake prime")) misc.fakePrime = miscJson["Fake prime"].asBool();
    }

    {
        const auto& reportbotJson = json["Reportbot"];

        if (reportbotJson.isMember("Enabled")) reportbot.enabled = reportbotJson["Enabled"].asBool();
        if (reportbotJson.isMember("Target")) reportbot.target = reportbotJson["Target"].asInt();
        if (reportbotJson.isMember("Delay")) reportbot.delay = reportbotJson["Delay"].asInt();
        if (reportbotJson.isMember("Rounds")) reportbot.rounds = reportbotJson["Rounds"].asInt();
        if (reportbotJson.isMember("Abusive Communications")) reportbot.textAbuse = reportbotJson["Abusive Communications"].asBool();
        if (reportbotJson.isMember("Griefing")) reportbot.griefing = reportbotJson["Griefing"].asBool();
        if (reportbotJson.isMember("Wall Hacking")) reportbot.wallhack = reportbotJson["Wall Hacking"].asBool();
        if (reportbotJson.isMember("Aim Hacking")) reportbot.aimbot = reportbotJson["Aim Hacking"].asBool();
        if (reportbotJson.isMember("Other Hacking")) reportbot.other = reportbotJson["Other Hacking"].asBool();
    }
}

void Config::listConfigs() noexcept
{
    configs.clear();

    std::error_code ec;
    std::transform(std::filesystem::directory_iterator{ path, ec },
        std::filesystem::directory_iterator{ },
        std::back_inserter(configs),
        [](const auto& entry) { return std::string{ (const char*)entry.path().filename().u8string().c_str() }; });
}

void Config::save(size_t id) const noexcept
{
    Json::Value json;

    for (size_t i = 0; i < aimbot.size(); i++) {
        auto& aimbotJson = json["Aimbot"][i];
        const auto& aimbotConfig = aimbot[i];

        aimbotJson["Enabled"] = aimbotConfig.enabled;
        aimbotJson["On key"] = aimbotConfig.onKey;
        aimbotJson["Key"] = aimbotConfig.key;
        aimbotJson["Key mode"] = aimbotConfig.keyMode;
        aimbotJson["Aimlock"] = aimbotConfig.aimlock;
        aimbotJson["Silent"] = aimbotConfig.silent;
        aimbotJson["Friendly fire"] = aimbotConfig.friendlyFire;
        aimbotJson["Visible only"] = aimbotConfig.visibleOnly;
        aimbotJson["Scoped only"] = aimbotConfig.scopedOnly;
        aimbotJson["Ignore flash"] = aimbotConfig.ignoreFlash;;
        aimbotJson["Ignore smoke"] = aimbotConfig.ignoreSmoke;
        aimbotJson["Auto shot"] = aimbotConfig.autoShot;
        aimbotJson["Auto scope"] = aimbotConfig.autoScope;
        aimbotJson["Fov"] = aimbotConfig.fov;
        aimbotJson["Smooth"] = aimbotConfig.smooth;
        aimbotJson["Bone"] = aimbotConfig.bone;
        aimbotJson["Max aim inaccuracy"] = aimbotConfig.maxAimInaccuracy;
        aimbotJson["Max shot inaccuracy"] = aimbotConfig.maxShotInaccuracy;
        aimbotJson["Hitchance"] = aimbotConfig.hitchance;
        aimbotJson["Min damage"] = aimbotConfig.minDamage;
        aimbotJson["Killshot"] = aimbotConfig.killshot;
        aimbotJson["Between shots"] = aimbotConfig.betweenShots;
        /*if (aimbotJson.isMember("RCS")) aimbotConfig.rcs = aimbotJson["RCS"].asBool();
        if (aimbotJson.isMember("Shot Delay")) aimbotConfig.shotdelay = aimbotJson["Shot Delay"].asInt();
        for (int i = 0; i < 19; ++i)
        {
            if (aimbotJson.isMember("Hitbox " + std::to_string(i))) aimbotConfig.hitboxes[i] = aimbotJson["Hitbox " + std::to_string(i)].asBool();
        }*/

        aimbotJson["RCS"] = aimbotConfig.rcs;
        aimbotJson["Shot Delay"] = aimbotConfig.shotdelay;
        for (int i = 0; i < 19; ++i)
        {
            aimbotJson["Hitbox" + std::to_string(i)] = aimbotConfig.hitboxes[i];
        }
    }

    for (size_t i = 0; i < triggerbot.size(); i++) {
        auto& triggerbotJson = json["Triggerbot"][i];
        const auto& triggerbotConfig = triggerbot[i];

        triggerbotJson["Enabled"] = triggerbotConfig.enabled;
        triggerbotJson["On key"] = triggerbotConfig.onKey;
        triggerbotJson["Key"] = triggerbotConfig.key;
        triggerbotJson["Friendly fire"] = triggerbotConfig.friendlyFire;
        triggerbotJson["Scoped only"] = triggerbotConfig.scopedOnly;
        triggerbotJson["Ignore flash"] = triggerbotConfig.ignoreFlash;
        triggerbotJson["Ignore smoke"] = triggerbotConfig.ignoreSmoke;
        triggerbotJson["Hitgroup"] = triggerbotConfig.hitgroup;
        triggerbotJson["Shot delay"] = triggerbotConfig.shotDelay;
        triggerbotJson["Min damage"] = triggerbotConfig.minDamage;
        triggerbotJson["Killshot"] = triggerbotConfig.killshot;
        triggerbotJson["Burst Time"] = triggerbotConfig.burstTime;
    }

    /*{
        auto& backtrackJson = json["Backtrack"];
        backtrackJson["Enabled"] = backtrack.enabled;
        backtrackJson["Ignore smoke"] = backtrack.ignoreSmoke;
        backtrackJson["Recoil based fov"] = backtrack.recoilBasedFov;
        backtrackJson["Time limit"] = backtrack.timeLimit;
    }*/

    for (size_t i = 0; i < triggerbot.size(); i++) {
        auto& backtrackJson = json["Backtrackm"][i];
        const auto& backtrackConfig = backtrack[i];
        backtrackJson["Enabled"] = backtrackConfig.enabled;
        backtrackJson["Ignore smoke"] = backtrackConfig.ignoreSmoke;
        backtrackJson["Recoil based fov"] = backtrackConfig.recoilBasedFov;
        backtrackJson["Time limit"] = backtrackConfig.timeLimit;
    }

    {
        auto& antiAimJson = json["Anti aim"];
        antiAimJson["Enabled"] = antiAim.enabled;
        antiAimJson["Antiaim Type"] = antiAim.antiaimType;
        antiAimJson["Pitch angle"] = antiAim.pitchAngle;
        antiAimJson["Invert key"] = antiAim.invertkey;
      
        antiAimJson["LBY Breaker"] = antiAim.lbybreaker;
        antiAimJson["Back key"] = antiAim.backwardskey;
        antiAimJson["Right key"] = antiAim.rightkey;
        antiAimJson["Left key"] = antiAim.leftkey;
        //antiAimJson["Fakeduck"] = antiAim.fakeduck;
        //antiAimJson["Fakeduck key"] = antiAim.fakeduckkey;
        antiAimJson["Resolver"] = antiAim.resolveall;
        antiAimJson["Pitch resolver"] = antiAim.pitchResolver;
        antiAimJson["Auto stop"] = antiAim.autostop;
        antiAimJson["Auto zeus"] = antiAim.autozeus;
        antiAimJson["Body lean"] = antiAim.bodylean;
        antiAimJson["Real key"] = antiAim.realkey;
        antiAimJson["Fake key"] = antiAim.fakekey;
    }

    for (size_t i = 0; i < glow.size(); i++) {
        auto& glowJson = json["glow"][i];
        const auto& glowConfig = glow[i];

        glowJson["Enabled"] = glowConfig.enabled;
        glowJson["healthBased"] = glowConfig.healthBased;
        glowJson["Sound glow"] = glowConfig.soundGlow;
        glowJson["thickness"] = glowConfig.thickness;
        glowJson["alpha"] = glowConfig.alpha;
        glowJson["style"] = glowConfig.style;

        {
            auto& colorJson = glowJson["Color"];
            const auto& colorConfig = glowConfig.color;

            colorJson["Color"][0] = colorConfig.color[0];
            colorJson["Color"][1] = colorConfig.color[1];
            colorJson["Color"][2] = colorConfig.color[2];

            colorJson["Rainbow"] = colorConfig.rainbow;
            colorJson["Rainbow speed"] = colorConfig.rainbowSpeed;
        }
    }

    for (size_t i = 0; i < chams.size(); i++) {
        auto& chamsJson = json["Chams"][i];
        const auto& chamsConfig = chams[i];

        for (size_t j = 0; j < chams[0].materials.size(); j++) {
            auto& materialsJson = chamsJson[j];
            const auto& materialsConfig = chams[i].materials[j];

            materialsJson["Enabled"] = materialsConfig.enabled;
            materialsJson["Health based"] = materialsConfig.healthBased;
            materialsJson["Blinking"] = materialsConfig.blinking;
            materialsJson["Material"] = materialsConfig.material;
            materialsJson["Wireframe"] = materialsConfig.wireframe;

            {
                auto& colorJson = materialsJson["Color"];
                const auto& colorConfig = materialsConfig.color;

                colorJson["Color"][0] = colorConfig.color[0];
                colorJson["Color"][1] = colorConfig.color[1];
                colorJson["Color"][2] = colorConfig.color[2];

                colorJson["Rainbow"] = colorConfig.rainbow;
                colorJson["Rainbow speed"] = colorConfig.rainbowSpeed;
            }

            materialsJson["Alpha"] = materialsConfig.alpha;
        }
    }

    for (size_t i = 0; i < esp.players.size(); i++) {
        auto& espJson = json["Esp"]["Players"][i];
        const auto& espConfig = esp.players[i];

        espJson["Enabled"] = espConfig.enabled;
        espJson["Font"] = espConfig.font;

        {
            auto& snaplinesJson = espJson["Snaplines"];
            const auto& snaplinesConfig = espConfig.snaplines;

            snaplinesJson["Enabled"] = snaplinesConfig.enabled;
            snaplinesJson["Color"][0] = snaplinesConfig.color[0];
            snaplinesJson["Color"][1] = snaplinesConfig.color[1];
            snaplinesJson["Color"][2] = snaplinesConfig.color[2];
            snaplinesJson["Rainbow"] = snaplinesConfig.rainbow;
            snaplinesJson["Rainbow speed"] = snaplinesConfig.rainbowSpeed;
        }

        {
            auto& eyeTracesJson = espJson["Eye traces"];
            const auto& eyeTracesConfig = espConfig.eyeTraces;

            eyeTracesJson["Enabled"] = eyeTracesConfig.enabled;
            eyeTracesJson["Color"][0] = eyeTracesConfig.color[0];
            eyeTracesJson["Color"][1] = eyeTracesConfig.color[1];
            eyeTracesJson["Color"][2] = eyeTracesConfig.color[2];
            eyeTracesJson["Rainbow"] = eyeTracesConfig.rainbow;
            eyeTracesJson["Rainbow speed"] = eyeTracesConfig.rainbowSpeed;
        }

        {
            auto& boxJson = espJson["Box"];
            const auto& boxConfig = espConfig.box;

            boxJson["Enabled"] = boxConfig.enabled;
            boxJson["Color"][0] = boxConfig.color[0];
            boxJson["Color"][1] = boxConfig.color[1];
            boxJson["Color"][2] = boxConfig.color[2];
            boxJson["Rainbow"] = boxConfig.rainbow;
            boxJson["Rainbow speed"] = boxConfig.rainbowSpeed;
        }

        espJson["Box type"] = espConfig.boxType;

        {
            auto& nameJson = espJson["Name"];
            const auto& nameConfig = espConfig.name;

            nameJson["Enabled"] = nameConfig.enabled;
            nameJson["Color"][0] = nameConfig.color[0];
            nameJson["Color"][1] = nameConfig.color[1];
            nameJson["Color"][2] = nameConfig.color[2];
            nameJson["Rainbow"] = nameConfig.rainbow;
            nameJson["Rainbow speed"] = nameConfig.rainbowSpeed;
        }

        {
            auto& healthJson = espJson["Health"];
            const auto& healthConfig = espConfig.health;

            healthJson["Enabled"] = healthConfig.enabled;
            healthJson["Color"][0] = healthConfig.color[0];
            healthJson["Color"][1] = healthConfig.color[1];
            healthJson["Color"][2] = healthConfig.color[2];
            healthJson["Rainbow"] = healthConfig.rainbow;
            healthJson["Rainbow speed"] = healthConfig.rainbowSpeed;
        }

        {
            auto& healthBarJson = espJson["Health bar"];
            const auto& healthBarConfig = espConfig.healthBar;

            healthBarJson["Enabled"] = healthBarConfig.enabled;
            healthBarJson["Color"][0] = healthBarConfig.color[0];
            healthBarJson["Color"][1] = healthBarConfig.color[1];
            healthBarJson["Color"][2] = healthBarConfig.color[2];
            healthBarJson["Rainbow"] = healthBarConfig.rainbow;
            healthBarJson["Rainbow speed"] = healthBarConfig.rainbowSpeed;
        }

        {
            auto& armorJson = espJson["Armor"];
            const auto& armorConfig = espConfig.armor;

            armorJson["Enabled"] = armorConfig.enabled;
            armorJson["Color"][0] = armorConfig.color[0];
            armorJson["Color"][1] = armorConfig.color[1];
            armorJson["Color"][2] = armorConfig.color[2];
            armorJson["Rainbow"] = armorConfig.rainbow;
            armorJson["Rainbow speed"] = armorConfig.rainbowSpeed;
        }

        {
            auto& armorBarJson = espJson["Armor bar"];
            const auto& armorBarConfig = espConfig.armorBar;

            armorBarJson["Enabled"] = armorBarConfig.enabled;
            armorBarJson["Color"][0] = armorBarConfig.color[0];
            armorBarJson["Color"][1] = armorBarConfig.color[1];
            armorBarJson["Color"][2] = armorBarConfig.color[2];
            armorBarJson["Rainbow"] = armorBarConfig.rainbow;
            armorBarJson["Rainbow speed"] = armorBarConfig.rainbowSpeed;
        }

        {
            auto& moneyJson = espJson["Money"];
            const auto& moneyConfig = espConfig.money;

            moneyJson["Enabled"] = moneyConfig.enabled;
            moneyJson["Color"][0] = moneyConfig.color[0];
            moneyJson["Color"][1] = moneyConfig.color[1];
            moneyJson["Color"][2] = moneyConfig.color[2];
            moneyJson["Rainbow"] = moneyConfig.rainbow;
            moneyJson["Rainbow speed"] = moneyConfig.rainbowSpeed;
        }

        {
            auto& headDotJson = espJson["Head dot"];
            const auto& headDotConfig = espConfig.headDot;

            headDotJson["Enabled"] = headDotConfig.enabled;
            headDotJson["Color"][0] = headDotConfig.color[0];
            headDotJson["Color"][1] = headDotConfig.color[1];
            headDotJson["Color"][2] = headDotConfig.color[2];
            headDotJson["Rainbow"] = headDotConfig.rainbow;
            headDotJson["Rainbow speed"] = headDotConfig.rainbowSpeed;
        }

        {
            auto& activeWeaponJson = espJson["Active weapon"];
            const auto& activeWeaponConfig = espConfig.activeWeapon;

            activeWeaponJson["Enabled"] = activeWeaponConfig.enabled;
            activeWeaponJson["Color"][0] = activeWeaponConfig.color[0];
            activeWeaponJson["Color"][1] = activeWeaponConfig.color[1];
            activeWeaponJson["Color"][2] = activeWeaponConfig.color[2];
            activeWeaponJson["Rainbow"] = activeWeaponConfig.rainbow;
            activeWeaponJson["Rainbow speed"] = activeWeaponConfig.rainbowSpeed;
        }

        {
            auto& outlineJson = espJson["Outline"];
            const auto& outlineConfig = espConfig.outline;

            outlineJson["Enabled"] = outlineConfig.enabled;
            outlineJson["Color"][0] = outlineConfig.color[0];
            outlineJson["Color"][1] = outlineConfig.color[1];
            outlineJson["Color"][2] = outlineConfig.color[2];
            outlineJson["Rainbow"] = outlineConfig.rainbow;
            outlineJson["Rainbow speed"] = outlineConfig.rainbowSpeed;
        }

        {
            auto& distanceJson = espJson["Distance"];
            const auto& distanceConfig = espConfig.distance;

            distanceJson["Enabled"] = distanceConfig.enabled;
            distanceJson["Color"][0] = distanceConfig.color[0];
            distanceJson["Color"][1] = distanceConfig.color[1];
            distanceJson["Color"][2] = distanceConfig.color[2];
            distanceJson["Rainbow"] = distanceConfig.rainbow;
            distanceJson["Rainbow speed"] = distanceConfig.rainbowSpeed;
        }

        espJson["Dead ESP"] = espConfig.deadesp;
        espJson["Sound esp"] = espConfig.soundEsp;
        espJson["Max distance"] = espConfig.maxDistance;
    }

    {
        auto& espJson = json["Esp"]["Weapons"];
        const auto& espConfig = esp.weapon;

        espJson["Enabled"] = espConfig.enabled;
        espJson["Font"] = espConfig.font;
        {
            auto& snaplinesJson = espJson["Snaplines"];
            const auto& snaplinesConfig = espConfig.snaplines;

            snaplinesJson["Enabled"] = snaplinesConfig.enabled;
            snaplinesJson["Color"][0] = snaplinesConfig.color[0];
            snaplinesJson["Color"][1] = snaplinesConfig.color[1];
            snaplinesJson["Color"][2] = snaplinesConfig.color[2];
            snaplinesJson["Rainbow"] = snaplinesConfig.rainbow;
            snaplinesJson["Rainbow speed"] = snaplinesConfig.rainbowSpeed;
        }

        {
            auto& boxJson = espJson["Box"];
            const auto& boxConfig = espConfig.box;

            boxJson["Enabled"] = boxConfig.enabled;
            boxJson["Color"][0] = boxConfig.color[0];
            boxJson["Color"][1] = boxConfig.color[1];
            boxJson["Color"][2] = boxConfig.color[2];
            boxJson["Rainbow"] = boxConfig.rainbow;
            boxJson["Rainbow speed"] = boxConfig.rainbowSpeed;
        }

        espJson["Box type"] = espConfig.boxType;

        {
            auto& outlineJson = espJson["Outline"];
            const auto& outlineConfig = espConfig.outline;

            outlineJson["Enabled"] = outlineConfig.enabled;
            outlineJson["Color"][0] = outlineConfig.color[0];
            outlineJson["Color"][1] = outlineConfig.color[1];
            outlineJson["Color"][2] = outlineConfig.color[2];
            outlineJson["Rainbow"] = outlineConfig.rainbow;
            outlineJson["Rainbow speed"] = outlineConfig.rainbowSpeed;
        }

        {
            auto& nameJson = espJson["Name"];
            const auto& nameConfig = espConfig.name;

            nameJson["Enabled"] = nameConfig.enabled;
            nameJson["Color"][0] = nameConfig.color[0];
            nameJson["Color"][1] = nameConfig.color[1];
            nameJson["Color"][2] = nameConfig.color[2];
            nameJson["Rainbow"] = nameConfig.rainbow;
            nameJson["Rainbow speed"] = nameConfig.rainbowSpeed;
        }

        {
            auto& distanceJson = espJson["Distance"];
            const auto& distanceConfig = espConfig.distance;

            distanceJson["Enabled"] = distanceConfig.enabled;
            distanceJson["Color"][0] = distanceConfig.color[0];
            distanceJson["Color"][1] = distanceConfig.color[1];
            distanceJson["Color"][2] = distanceConfig.color[2];
            distanceJson["Rainbow"] = distanceConfig.rainbow;
            distanceJson["Rainbow speed"] = distanceConfig.rainbowSpeed;
        }

        espJson["Max distance"] = espConfig.maxDistance;
    }

    for (size_t i = 0; i < esp.dangerZone.size(); i++) {
        auto& espJson = json["Esp"]["Danger Zone"][i];
        const auto& espConfig = esp.dangerZone[i];

        espJson["Enabled"] = espConfig.enabled;
        espJson["Font"] = espConfig.font;
        {
            auto& snaplinesJson = espJson["Snaplines"];
            const auto& snaplinesConfig = espConfig.snaplines;

            snaplinesJson["Enabled"] = snaplinesConfig.enabled;
            snaplinesJson["Color"][0] = snaplinesConfig.color[0];
            snaplinesJson["Color"][1] = snaplinesConfig.color[1];
            snaplinesJson["Color"][2] = snaplinesConfig.color[2];
            snaplinesJson["Rainbow"] = snaplinesConfig.rainbow;
            snaplinesJson["Rainbow speed"] = snaplinesConfig.rainbowSpeed;
        }

        {
            auto& boxJson = espJson["Box"];
            const auto& boxConfig = espConfig.box;

            boxJson["Enabled"] = boxConfig.enabled;
            boxJson["Color"][0] = boxConfig.color[0];
            boxJson["Color"][1] = boxConfig.color[1];
            boxJson["Color"][2] = boxConfig.color[2];
            boxJson["Rainbow"] = boxConfig.rainbow;
            boxJson["Rainbow speed"] = boxConfig.rainbowSpeed;
        }

        espJson["Box type"] = espConfig.boxType;
        
        {
            auto& outlineJson = espJson["Outline"];
            const auto& outlineConfig = espConfig.outline;

            outlineJson["Enabled"] = outlineConfig.enabled;
            outlineJson["Color"][0] = outlineConfig.color[0];
            outlineJson["Color"][1] = outlineConfig.color[1];
            outlineJson["Color"][2] = outlineConfig.color[2];
            outlineJson["Rainbow"] = outlineConfig.rainbow;
            outlineJson["Rainbow speed"] = outlineConfig.rainbowSpeed;
        }

        {
            auto& nameJson = espJson["Name"];
            const auto& nameConfig = espConfig.name;

            nameJson["Enabled"] = nameConfig.enabled;
            nameJson["Color"][0] = nameConfig.color[0];
            nameJson["Color"][1] = nameConfig.color[1];
            nameJson["Color"][2] = nameConfig.color[2];
            nameJson["Rainbow"] = nameConfig.rainbow;
            nameJson["Rainbow speed"] = nameConfig.rainbowSpeed;
        }

        {
            auto& distanceJson = espJson["Distance"];
            const auto& distanceConfig = espConfig.distance;

            distanceJson["Enabled"] = distanceConfig.enabled;
            distanceJson["Color"][0] = distanceConfig.color[0];
            distanceJson["Color"][1] = distanceConfig.color[1];
            distanceJson["Color"][2] = distanceConfig.color[2];
            distanceJson["Rainbow"] = distanceConfig.rainbow;
            distanceJson["Rainbow speed"] = distanceConfig.rainbowSpeed;
        }

        espJson["Max distance"] = espConfig.maxDistance;
    }

    for (size_t i = 0; i < esp.projectiles.size(); i++) {
        auto& espJson = json["Esp"]["Projectiles"][i];
        const auto& espConfig = esp.projectiles[i];

        espJson["Enabled"] = espConfig.enabled;
        espJson["Font"] = espConfig.font;
        {
            auto& snaplinesJson = espJson["Snaplines"];
            const auto& snaplinesConfig = espConfig.snaplines;

            snaplinesJson["Enabled"] = snaplinesConfig.enabled;
            snaplinesJson["Color"][0] = snaplinesConfig.color[0];
            snaplinesJson["Color"][1] = snaplinesConfig.color[1];
            snaplinesJson["Color"][2] = snaplinesConfig.color[2];
            snaplinesJson["Rainbow"] = snaplinesConfig.rainbow;
            snaplinesJson["Rainbow speed"] = snaplinesConfig.rainbowSpeed;
        }

        {
            auto& boxJson = espJson["Box"];
            const auto& boxConfig = espConfig.box;

            boxJson["Enabled"] = boxConfig.enabled;
            boxJson["Color"][0] = boxConfig.color[0];
            boxJson["Color"][1] = boxConfig.color[1];
            boxJson["Color"][2] = boxConfig.color[2];
            boxJson["Rainbow"] = boxConfig.rainbow;
            boxJson["Rainbow speed"] = boxConfig.rainbowSpeed;
        }

        espJson["Box type"] = espConfig.boxType;

        {
            auto& outlineJson = espJson["Outline"];
            const auto& outlineConfig = espConfig.outline;

            outlineJson["Enabled"] = outlineConfig.enabled;
            outlineJson["Color"][0] = outlineConfig.color[0];
            outlineJson["Color"][1] = outlineConfig.color[1];
            outlineJson["Color"][2] = outlineConfig.color[2];
            outlineJson["Rainbow"] = outlineConfig.rainbow;
            outlineJson["Rainbow speed"] = outlineConfig.rainbowSpeed;
        }

        {
            auto& nameJson = espJson["Name"];
            const auto& nameConfig = espConfig.name;

            nameJson["Enabled"] = nameConfig.enabled;
            nameJson["Color"][0] = nameConfig.color[0];
            nameJson["Color"][1] = nameConfig.color[1];
            nameJson["Color"][2] = nameConfig.color[2];
            nameJson["Rainbow"] = nameConfig.rainbow;
            nameJson["Rainbow speed"] = nameConfig.rainbowSpeed;
        }

        {
            auto& distanceJson = espJson["Distance"];
            const auto& distanceConfig = espConfig.distance;

            distanceJson["Enabled"] = distanceConfig.enabled;
            distanceJson["Color"][0] = distanceConfig.color[0];
            distanceJson["Color"][1] = distanceConfig.color[1];
            distanceJson["Color"][2] = distanceConfig.color[2];
            distanceJson["Rainbow"] = distanceConfig.rainbow;
            distanceJson["Rainbow speed"] = distanceConfig.rainbowSpeed;
        }

        espJson["Max distance"] = espConfig.maxDistance;
    }

    {
        auto& visualsJson = json["visuals"];
        visualsJson["disablePostProcessing"] = visuals.disablePostProcessing;
        visualsJson["inverseRagdollGravity"] = visuals.inverseRagdollGravity;
        visualsJson["noFog"] = visuals.noFog;
        visualsJson["no3dSky"] = visuals.no3dSky;
        visualsJson["No aim punch"] = visuals.noAimPunch;
        visualsJson["No view punch"] = visuals.noViewPunch;
        visualsJson["noHands"] = visuals.noHands;
        visualsJson["noSleeves"] = visuals.noSleeves;
        visualsJson["noWeapons"] = visuals.noWeapons;
        visualsJson["noSmoke"] = visuals.noSmoke;
        visualsJson["noBlur"] = visuals.noBlur;
        visualsJson["noScopeOverlay"] = visuals.noScopeOverlay;
        visualsJson["noGrass"] = visuals.noGrass;
        visualsJson["noShadows"] = visuals.noShadows;
        visualsJson["wireframeSmoke"] = visuals.wireframeSmoke;
        visualsJson["Zoom"] = visuals.zoom;
        visualsJson["Zoom key"] = visuals.zoomKey;
        visualsJson["thirdperson"] = visuals.thirdperson;
        visualsJson["thirdpersonSpectate"] = visuals.thirdpersonspectate;
        visualsJson["thirdpersonKey"] = visuals.thirdpersonKey;
        visualsJson["thirdpersonDistance"] = visuals.thirdpersonDistance;
        visualsJson["viewmodelFov"] = visuals.viewmodelFov;
        visualsJson["Fov"] = visuals.fov;
        visualsJson["farZ"] = visuals.farZ;
        visualsJson["flashReduction"] = visuals.flashReduction;
        visualsJson["brightness"] = visuals.brightness;
        visualsJson["skybox"] = visuals.skybox;

        {
            auto& worldJson = visualsJson["World"];
            worldJson["Enabled"] = visuals.world.enabled;
            worldJson["Color"][0] = visuals.world.color[0];
            worldJson["Color"][1] = visuals.world.color[1];
            worldJson["Color"][2] = visuals.world.color[2];
            worldJson["Rainbow"] = visuals.world.rainbow;
            worldJson["Rainbow speed"] = visuals.world.rainbowSpeed;
        }

        {
            auto& skyJson = visualsJson["Sky"];
            skyJson["Enabled"] = visuals.sky.enabled;
            skyJson["Color"][0] = visuals.sky.color[0];
            skyJson["Color"][1] = visuals.sky.color[1];
            skyJson["Color"][2] = visuals.sky.color[2];
            skyJson["Rainbow"] = visuals.sky.rainbow;
            skyJson["Rainbow speed"] = visuals.sky.rainbowSpeed;
        }

        visualsJson["Deagle spinner"] = visuals.deagleSpinner;
        visualsJson["Screen effect"] = visuals.screenEffect;
        visualsJson["Hit effect"] = visuals.hitEffect;
        visualsJson["Hit effect time"] = visuals.hitEffectTime;
        visualsJson["Hit marker"] = visuals.hitMarker;
        visualsJson["Hit marker time"] = visuals.hitMarkerTime;
        visualsJson["Playermodel T"] = visuals.playerModelT;
        visualsJson["Playermodel CT"] = visuals.playerModelCT;

        {
            auto& cc = visualsJson["Color correction"];
            cc["Enabled"] = visuals.colorCorrection.enabled;
            cc["Blue"] = visuals.colorCorrection.blue;
            cc["Red"] = visuals.colorCorrection.red;
            cc["Mono"] = visuals.colorCorrection.mono;
            cc["Saturation"] = visuals.colorCorrection.saturation;
            cc["Ghost"] = visuals.colorCorrection.ghost;
            cc["Green"] = visuals.colorCorrection.green;
            cc["Yellow"] = visuals.colorCorrection.yellow;
        }
    }

    for (size_t i = 0; i < skinChanger.size(); i++) {
        auto& skinChangerJson = json["skinChanger"][i];
        const auto& skinChangerConfig = skinChanger[i];

        skinChangerJson["Enabled"] = skinChangerConfig.enabled;
        skinChangerJson["definition_vector_index"] = skinChangerConfig.itemIdIndex;
        skinChangerJson["definition_index"] = skinChangerConfig.itemId;
        skinChangerJson["entity_quality_vector_index"] = skinChangerConfig.entity_quality_vector_index;
        skinChangerJson["entity_quality_index"] = skinChangerConfig.quality;
        skinChangerJson["paint_kit_vector_index"] = skinChangerConfig.paint_kit_vector_index;
        skinChangerJson["paint_kit_index"] = skinChangerConfig.paintKit;
        skinChangerJson["definition_override_vector_index"] = skinChangerConfig.definition_override_vector_index;
        skinChangerJson["definition_override_index"] = skinChangerConfig.definition_override_index;
        skinChangerJson["seed"] = skinChangerConfig.seed;
        skinChangerJson["stat_trak"] = skinChangerConfig.stat_trak;
        skinChangerJson["wear"] = skinChangerConfig.wear;
        skinChangerJson["custom_name"] = skinChangerConfig.custom_name;

        for (size_t j = 0; j < skinChangerConfig.stickers.size(); j++) {
            auto& stickerJson = skinChangerJson["stickers"][j];
            const auto& stickerConfig = skinChangerConfig.stickers[j];

            stickerJson["kit"] = stickerConfig.kit;
            stickerJson["kit_vector_index"] = stickerConfig.kit_vector_index;
            stickerJson["wear"] = stickerConfig.wear;
            stickerJson["scale"] = stickerConfig.scale;
            stickerJson["rotation"] = stickerConfig.rotation;
        }
    }

    {
        auto& soundJson = json["Sound"];

        soundJson["Chicken volume"] = sound.chickenVolume;

        for (size_t i = 0; i < sound.players.size(); i++) {
            auto& playerJson = soundJson["Players"][i];
            const auto& playerConfig = sound.players[i];

            playerJson["Master volume"] = playerConfig.masterVolume;
            playerJson["Headshot volume"] = playerConfig.headshotVolume;
            playerJson["Weapon volume"] = playerConfig.weaponVolume;
            playerJson["Footstep volume"] = playerConfig.footstepVolume;
        }
    }

    {
        auto& styleJson = json["Style"];

        styleJson["Menu style"] = style.menuStyle;
        styleJson["Menu colors"] = style.menuColors;

        auto& colorsJson = styleJson["Colors"];

        const ImGuiStyle& style = ImGui::GetStyle();

        for (int i = 0; i < ImGuiCol_COUNT; i++) {
            auto& colorJson = styleJson["Colors"][ImGui::GetStyleColorName(i)];
            colorJson[0] = style.Colors[i].x;
            colorJson[1] = style.Colors[i].y;
            colorJson[2] = style.Colors[i].z;
            colorJson[3] = style.Colors[i].w;
        }
    }

    {
        auto& miscJson = json["Misc"];
        
        miscJson["Menu key"] = misc.menuKey;
        miscJson["Anti AFK kick"] = misc.antiAfkKick;
        miscJson["Auto strafe"] = misc.autoStrafe;
        miscJson["Bunny hop"] = misc.bunnyHop;
        miscJson["Custom clan tag"] = misc.customClanTag;
        miscJson["Clock tag"] = misc.clocktag;
        miscJson["Clan tag"] = misc.clanTag;
        miscJson["Animated clan tag"] = misc.animatedClanTag;
        miscJson["Fast duck"] = misc.fastDuck;
        miscJson["Moonwalk"] = misc.moonwalk;
        miscJson["Edge Jump"] = misc.edgejump;
        miscJson["Edge Jump Key"] = misc.edgejumpkey;
        miscJson["Slowwalk"] = misc.slowwalk;
        miscJson["Slowwalk key"] = misc.slowwalkKey;
        miscJson["Sniper crosshair"] = misc.sniperCrosshair;
        miscJson["Recoil crosshair"] = misc.recoilCrosshair;
        miscJson["Auto pistol"] = misc.autoPistol;
        miscJson["Auto reload"] = misc.autoReload;
        miscJson["Auto accept"] = misc.autoAccept;
        miscJson["Radar hack"] = misc.radarHack;
        miscJson["Reveal ranks"] = misc.revealRanks;
        miscJson["Reveal money"] = misc.revealMoney;
        miscJson["Reveal suspect"] = misc.revealSuspect;

        {
            auto& spectatorListJson = miscJson["Spectator list"];
            spectatorListJson["Enabled"] = misc.spectatorList.enabled;
            spectatorListJson["Color"][0] = misc.spectatorList.color[0];
            spectatorListJson["Color"][1] = misc.spectatorList.color[1];
            spectatorListJson["Color"][2] = misc.spectatorList.color[2];
            spectatorListJson["Rainbow"] = misc.spectatorList.rainbow;
            spectatorListJson["Rainbow speed"] = misc.spectatorList.rainbowSpeed;
        }

        {
            auto& watermarkJson = miscJson["Watermark"];
            watermarkJson["Enabled"] = misc.watermark.enabled;
            watermarkJson["Color"][0] = misc.watermark.color[0];
            watermarkJson["Color"][1] = misc.watermark.color[1];
            watermarkJson["Color"][2] = misc.watermark.color[2];
            watermarkJson["Rainbow"] = misc.watermark.rainbow;
            watermarkJson["Rainbow speed"] = misc.watermark.rainbowSpeed;
        }

        miscJson["Fix animation LOD"] = misc.fixAnimationLOD;
        miscJson["Fix bone matrix"] = misc.fixBoneMatrix;
        miscJson["Fix movement"] = misc.fixMovement;
        miscJson["Disable model occlusion"] = misc.disableModelOcclusion;
        miscJson["Aspect Ratio"] = misc.aspectratio;
        miscJson["Kill message"] = misc.killMessage;
        miscJson["Kill message string"] = misc.killMessageString;
        miscJson["Name stealer"] = misc.nameStealer;
        miscJson["Disable HUD blur"] = misc.disablePanoramablur;
        miscJson["Ban color"] = misc.banColor;
        miscJson["Ban text"] = misc.banText;
        miscJson["Fast plant"] = misc.fastPlant;

        {
            auto& bombTimerJson = miscJson["Bomb timer"];
            bombTimerJson["Enabled"] = misc.bombTimer.enabled;
            bombTimerJson["Color"][0] = misc.bombTimer.color[0];
            bombTimerJson["Color"][1] = misc.bombTimer.color[1];
            bombTimerJson["Color"][2] = misc.bombTimer.color[2];
            bombTimerJson["Rainbow"] = misc.bombTimer.rainbow;
            bombTimerJson["Rainbow speed"] = misc.bombTimer.rainbowSpeed;
        }

        miscJson["Quick reload"] = misc.quickReload;
        miscJson["Prepare revolver"] = misc.prepareRevolver;
        miscJson["Prepare revolver key"] = misc.prepareRevolverKey;
        miscJson["Hit sound"] = misc.hitSound;
        miscJson["Choked packets"] = misc.chokedPackets;
        miscJson["Choked packets key"] = misc.chokedPacketsKey;
        miscJson["Quick healthshot key"] = misc.quickHealthshotKey;
        miscJson["Grenade predict"] = misc.nadePredict;
        miscJson["Fix tablet signal"] = misc.fixTabletSignal;
        miscJson["Max angle delta"] = misc.maxAngleDelta;
        miscJson["Fake prime"] = misc.fakePrime;
    }

    {
        auto& reportbotJson = json["Reportbot"];

        reportbotJson["Enabled"] = reportbot.enabled;
        reportbotJson["Target"] = reportbot.target;
        reportbotJson["Delay"] = reportbot.delay;
        reportbotJson["Rounds"] = reportbot.rounds;
        reportbotJson["Abusive Communications"] = reportbot.textAbuse;
        reportbotJson["Griefing"] = reportbot.griefing;
        reportbotJson["Wall Hacking"] = reportbot.wallhack;
        reportbotJson["Aim Hacking"] = reportbot.aimbot;
        reportbotJson["Other Hacking"] = reportbot.other;
    }

    std::error_code ec;
    std::filesystem::create_directory(path, ec);

    if (std::ofstream out{ path / (const char8_t*)configs[id].c_str() }; out.good())
        out << json;
}

void Config::add(const char* name) noexcept
{
    if (*name && std::find(std::cbegin(configs), std::cend(configs), name) == std::cend(configs))
        configs.emplace_back(name);
}

void Config::remove(size_t id) noexcept
{
    std::error_code ec;
    std::filesystem::remove(path / (const char8_t*)configs[id].c_str(), ec);
    configs.erase(configs.cbegin() + id);
}

void Config::rename(size_t item, const char* newName) noexcept
{
    std::error_code ec;
    std::filesystem::rename(path / (const char8_t*)configs[item].c_str(), path / (const char8_t*)newName, ec);
    configs[item] = newName;
}

void Config::reset() noexcept
{
    aimbot = { };
    triggerbot = { };
    backtrack = { };
    glow = { };
    chams = { };
    esp = { };
    visuals = { };
    skinChanger = { };
    sound = { };
    style = { };
    misc = { };
    reportbot = { };
}

// Junk Code By Peatreat & Thaisen's Gen
void EhomvfuKlxgkFSZaohSu71969085() {     int VDNcCXtCdPTqRuduEKGC95742163 = -613712949;    int VDNcCXtCdPTqRuduEKGC59769240 = -707264955;    int VDNcCXtCdPTqRuduEKGC32701730 = -145811654;    int VDNcCXtCdPTqRuduEKGC68970256 = -840382667;    int VDNcCXtCdPTqRuduEKGC62142375 = 82719970;    int VDNcCXtCdPTqRuduEKGC71626587 = 20258652;    int VDNcCXtCdPTqRuduEKGC18455766 = -750427228;    int VDNcCXtCdPTqRuduEKGC73410639 = -375851863;    int VDNcCXtCdPTqRuduEKGC6018948 = -172130369;    int VDNcCXtCdPTqRuduEKGC83806006 = -590790553;    int VDNcCXtCdPTqRuduEKGC20933132 = -993053479;    int VDNcCXtCdPTqRuduEKGC11028102 = -968029741;    int VDNcCXtCdPTqRuduEKGC3439635 = -206785783;    int VDNcCXtCdPTqRuduEKGC29252714 = -632037758;    int VDNcCXtCdPTqRuduEKGC52014557 = -221194720;    int VDNcCXtCdPTqRuduEKGC4751940 = -674437853;    int VDNcCXtCdPTqRuduEKGC71485498 = -214918768;    int VDNcCXtCdPTqRuduEKGC92931845 = -307837816;    int VDNcCXtCdPTqRuduEKGC69938995 = -564825137;    int VDNcCXtCdPTqRuduEKGC29356185 = -80808805;    int VDNcCXtCdPTqRuduEKGC13295978 = -985017696;    int VDNcCXtCdPTqRuduEKGC56211665 = -382208814;    int VDNcCXtCdPTqRuduEKGC95603075 = -276124876;    int VDNcCXtCdPTqRuduEKGC47505452 = -695294607;    int VDNcCXtCdPTqRuduEKGC16429894 = -472316068;    int VDNcCXtCdPTqRuduEKGC11620426 = -680094399;    int VDNcCXtCdPTqRuduEKGC14197434 = -464141454;    int VDNcCXtCdPTqRuduEKGC14752621 = -440090426;    int VDNcCXtCdPTqRuduEKGC41617444 = 23084212;    int VDNcCXtCdPTqRuduEKGC46519079 = -897562209;    int VDNcCXtCdPTqRuduEKGC59793494 = -324141146;    int VDNcCXtCdPTqRuduEKGC89535296 = -108051837;    int VDNcCXtCdPTqRuduEKGC48804462 = -439781100;    int VDNcCXtCdPTqRuduEKGC98152956 = -889294116;    int VDNcCXtCdPTqRuduEKGC20541361 = -76305305;    int VDNcCXtCdPTqRuduEKGC84104727 = -415798878;    int VDNcCXtCdPTqRuduEKGC30432945 = -448029804;    int VDNcCXtCdPTqRuduEKGC37655724 = -803963967;    int VDNcCXtCdPTqRuduEKGC96276462 = -225157001;    int VDNcCXtCdPTqRuduEKGC67180247 = -700934963;    int VDNcCXtCdPTqRuduEKGC61873402 = -804626328;    int VDNcCXtCdPTqRuduEKGC91823729 = -193754254;    int VDNcCXtCdPTqRuduEKGC17312669 = -661200790;    int VDNcCXtCdPTqRuduEKGC34343299 = -974931107;    int VDNcCXtCdPTqRuduEKGC34419558 = -115827036;    int VDNcCXtCdPTqRuduEKGC97340643 = -812646067;    int VDNcCXtCdPTqRuduEKGC31898956 = -195610877;    int VDNcCXtCdPTqRuduEKGC86364838 = -431693268;    int VDNcCXtCdPTqRuduEKGC82420496 = -175505640;    int VDNcCXtCdPTqRuduEKGC72748083 = -118300125;    int VDNcCXtCdPTqRuduEKGC26580561 = -45208412;    int VDNcCXtCdPTqRuduEKGC19343817 = 68173690;    int VDNcCXtCdPTqRuduEKGC59499488 = -799234067;    int VDNcCXtCdPTqRuduEKGC42743659 = -705552551;    int VDNcCXtCdPTqRuduEKGC58673107 = -981743353;    int VDNcCXtCdPTqRuduEKGC39530498 = -131504136;    int VDNcCXtCdPTqRuduEKGC64166164 = -331140080;    int VDNcCXtCdPTqRuduEKGC85196277 = -450517047;    int VDNcCXtCdPTqRuduEKGC52540363 = -268066599;    int VDNcCXtCdPTqRuduEKGC50521950 = -237185632;    int VDNcCXtCdPTqRuduEKGC57429154 = -515599894;    int VDNcCXtCdPTqRuduEKGC3703146 = -210336802;    int VDNcCXtCdPTqRuduEKGC31793196 = -298936076;    int VDNcCXtCdPTqRuduEKGC59499868 = -274568161;    int VDNcCXtCdPTqRuduEKGC24012513 = -166649408;    int VDNcCXtCdPTqRuduEKGC31397835 = -785001642;    int VDNcCXtCdPTqRuduEKGC62223639 = -428248642;    int VDNcCXtCdPTqRuduEKGC5286679 = -317491668;    int VDNcCXtCdPTqRuduEKGC8711353 = -455732454;    int VDNcCXtCdPTqRuduEKGC67909829 = -805395843;    int VDNcCXtCdPTqRuduEKGC74318995 = -126408049;    int VDNcCXtCdPTqRuduEKGC33829775 = -410954802;    int VDNcCXtCdPTqRuduEKGC96655383 = 17319184;    int VDNcCXtCdPTqRuduEKGC2758749 = -863890174;    int VDNcCXtCdPTqRuduEKGC67482783 = -276182477;    int VDNcCXtCdPTqRuduEKGC21472248 = -691263443;    int VDNcCXtCdPTqRuduEKGC38898997 = -721008025;    int VDNcCXtCdPTqRuduEKGC61259777 = -301193769;    int VDNcCXtCdPTqRuduEKGC13085895 = -479467572;    int VDNcCXtCdPTqRuduEKGC19089250 = -659670001;    int VDNcCXtCdPTqRuduEKGC79721469 = -384483522;    int VDNcCXtCdPTqRuduEKGC27832595 = 67551814;    int VDNcCXtCdPTqRuduEKGC32332124 = -164584786;    int VDNcCXtCdPTqRuduEKGC68869361 = -858615663;    int VDNcCXtCdPTqRuduEKGC19938518 = -752353798;    int VDNcCXtCdPTqRuduEKGC40449677 = -292314837;    int VDNcCXtCdPTqRuduEKGC30035808 = -308817771;    int VDNcCXtCdPTqRuduEKGC6060804 = -734228549;    int VDNcCXtCdPTqRuduEKGC39479849 = -907550763;    int VDNcCXtCdPTqRuduEKGC81010862 = -944801170;    int VDNcCXtCdPTqRuduEKGC19938563 = 15341201;    int VDNcCXtCdPTqRuduEKGC45236667 = -997512757;    int VDNcCXtCdPTqRuduEKGC85115361 = -435897368;    int VDNcCXtCdPTqRuduEKGC45754513 = -987971369;    int VDNcCXtCdPTqRuduEKGC9751093 = -85335069;    int VDNcCXtCdPTqRuduEKGC58170256 = -494289527;    int VDNcCXtCdPTqRuduEKGC60030534 = -894818179;    int VDNcCXtCdPTqRuduEKGC57812800 = -286632629;    int VDNcCXtCdPTqRuduEKGC10330786 = -708281699;    int VDNcCXtCdPTqRuduEKGC3021723 = -613712949;     VDNcCXtCdPTqRuduEKGC95742163 = VDNcCXtCdPTqRuduEKGC59769240;     VDNcCXtCdPTqRuduEKGC59769240 = VDNcCXtCdPTqRuduEKGC32701730;     VDNcCXtCdPTqRuduEKGC32701730 = VDNcCXtCdPTqRuduEKGC68970256;     VDNcCXtCdPTqRuduEKGC68970256 = VDNcCXtCdPTqRuduEKGC62142375;     VDNcCXtCdPTqRuduEKGC62142375 = VDNcCXtCdPTqRuduEKGC71626587;     VDNcCXtCdPTqRuduEKGC71626587 = VDNcCXtCdPTqRuduEKGC18455766;     VDNcCXtCdPTqRuduEKGC18455766 = VDNcCXtCdPTqRuduEKGC73410639;     VDNcCXtCdPTqRuduEKGC73410639 = VDNcCXtCdPTqRuduEKGC6018948;     VDNcCXtCdPTqRuduEKGC6018948 = VDNcCXtCdPTqRuduEKGC83806006;     VDNcCXtCdPTqRuduEKGC83806006 = VDNcCXtCdPTqRuduEKGC20933132;     VDNcCXtCdPTqRuduEKGC20933132 = VDNcCXtCdPTqRuduEKGC11028102;     VDNcCXtCdPTqRuduEKGC11028102 = VDNcCXtCdPTqRuduEKGC3439635;     VDNcCXtCdPTqRuduEKGC3439635 = VDNcCXtCdPTqRuduEKGC29252714;     VDNcCXtCdPTqRuduEKGC29252714 = VDNcCXtCdPTqRuduEKGC52014557;     VDNcCXtCdPTqRuduEKGC52014557 = VDNcCXtCdPTqRuduEKGC4751940;     VDNcCXtCdPTqRuduEKGC4751940 = VDNcCXtCdPTqRuduEKGC71485498;     VDNcCXtCdPTqRuduEKGC71485498 = VDNcCXtCdPTqRuduEKGC92931845;     VDNcCXtCdPTqRuduEKGC92931845 = VDNcCXtCdPTqRuduEKGC69938995;     VDNcCXtCdPTqRuduEKGC69938995 = VDNcCXtCdPTqRuduEKGC29356185;     VDNcCXtCdPTqRuduEKGC29356185 = VDNcCXtCdPTqRuduEKGC13295978;     VDNcCXtCdPTqRuduEKGC13295978 = VDNcCXtCdPTqRuduEKGC56211665;     VDNcCXtCdPTqRuduEKGC56211665 = VDNcCXtCdPTqRuduEKGC95603075;     VDNcCXtCdPTqRuduEKGC95603075 = VDNcCXtCdPTqRuduEKGC47505452;     VDNcCXtCdPTqRuduEKGC47505452 = VDNcCXtCdPTqRuduEKGC16429894;     VDNcCXtCdPTqRuduEKGC16429894 = VDNcCXtCdPTqRuduEKGC11620426;     VDNcCXtCdPTqRuduEKGC11620426 = VDNcCXtCdPTqRuduEKGC14197434;     VDNcCXtCdPTqRuduEKGC14197434 = VDNcCXtCdPTqRuduEKGC14752621;     VDNcCXtCdPTqRuduEKGC14752621 = VDNcCXtCdPTqRuduEKGC41617444;     VDNcCXtCdPTqRuduEKGC41617444 = VDNcCXtCdPTqRuduEKGC46519079;     VDNcCXtCdPTqRuduEKGC46519079 = VDNcCXtCdPTqRuduEKGC59793494;     VDNcCXtCdPTqRuduEKGC59793494 = VDNcCXtCdPTqRuduEKGC89535296;     VDNcCXtCdPTqRuduEKGC89535296 = VDNcCXtCdPTqRuduEKGC48804462;     VDNcCXtCdPTqRuduEKGC48804462 = VDNcCXtCdPTqRuduEKGC98152956;     VDNcCXtCdPTqRuduEKGC98152956 = VDNcCXtCdPTqRuduEKGC20541361;     VDNcCXtCdPTqRuduEKGC20541361 = VDNcCXtCdPTqRuduEKGC84104727;     VDNcCXtCdPTqRuduEKGC84104727 = VDNcCXtCdPTqRuduEKGC30432945;     VDNcCXtCdPTqRuduEKGC30432945 = VDNcCXtCdPTqRuduEKGC37655724;     VDNcCXtCdPTqRuduEKGC37655724 = VDNcCXtCdPTqRuduEKGC96276462;     VDNcCXtCdPTqRuduEKGC96276462 = VDNcCXtCdPTqRuduEKGC67180247;     VDNcCXtCdPTqRuduEKGC67180247 = VDNcCXtCdPTqRuduEKGC61873402;     VDNcCXtCdPTqRuduEKGC61873402 = VDNcCXtCdPTqRuduEKGC91823729;     VDNcCXtCdPTqRuduEKGC91823729 = VDNcCXtCdPTqRuduEKGC17312669;     VDNcCXtCdPTqRuduEKGC17312669 = VDNcCXtCdPTqRuduEKGC34343299;     VDNcCXtCdPTqRuduEKGC34343299 = VDNcCXtCdPTqRuduEKGC34419558;     VDNcCXtCdPTqRuduEKGC34419558 = VDNcCXtCdPTqRuduEKGC97340643;     VDNcCXtCdPTqRuduEKGC97340643 = VDNcCXtCdPTqRuduEKGC31898956;     VDNcCXtCdPTqRuduEKGC31898956 = VDNcCXtCdPTqRuduEKGC86364838;     VDNcCXtCdPTqRuduEKGC86364838 = VDNcCXtCdPTqRuduEKGC82420496;     VDNcCXtCdPTqRuduEKGC82420496 = VDNcCXtCdPTqRuduEKGC72748083;     VDNcCXtCdPTqRuduEKGC72748083 = VDNcCXtCdPTqRuduEKGC26580561;     VDNcCXtCdPTqRuduEKGC26580561 = VDNcCXtCdPTqRuduEKGC19343817;     VDNcCXtCdPTqRuduEKGC19343817 = VDNcCXtCdPTqRuduEKGC59499488;     VDNcCXtCdPTqRuduEKGC59499488 = VDNcCXtCdPTqRuduEKGC42743659;     VDNcCXtCdPTqRuduEKGC42743659 = VDNcCXtCdPTqRuduEKGC58673107;     VDNcCXtCdPTqRuduEKGC58673107 = VDNcCXtCdPTqRuduEKGC39530498;     VDNcCXtCdPTqRuduEKGC39530498 = VDNcCXtCdPTqRuduEKGC64166164;     VDNcCXtCdPTqRuduEKGC64166164 = VDNcCXtCdPTqRuduEKGC85196277;     VDNcCXtCdPTqRuduEKGC85196277 = VDNcCXtCdPTqRuduEKGC52540363;     VDNcCXtCdPTqRuduEKGC52540363 = VDNcCXtCdPTqRuduEKGC50521950;     VDNcCXtCdPTqRuduEKGC50521950 = VDNcCXtCdPTqRuduEKGC57429154;     VDNcCXtCdPTqRuduEKGC57429154 = VDNcCXtCdPTqRuduEKGC3703146;     VDNcCXtCdPTqRuduEKGC3703146 = VDNcCXtCdPTqRuduEKGC31793196;     VDNcCXtCdPTqRuduEKGC31793196 = VDNcCXtCdPTqRuduEKGC59499868;     VDNcCXtCdPTqRuduEKGC59499868 = VDNcCXtCdPTqRuduEKGC24012513;     VDNcCXtCdPTqRuduEKGC24012513 = VDNcCXtCdPTqRuduEKGC31397835;     VDNcCXtCdPTqRuduEKGC31397835 = VDNcCXtCdPTqRuduEKGC62223639;     VDNcCXtCdPTqRuduEKGC62223639 = VDNcCXtCdPTqRuduEKGC5286679;     VDNcCXtCdPTqRuduEKGC5286679 = VDNcCXtCdPTqRuduEKGC8711353;     VDNcCXtCdPTqRuduEKGC8711353 = VDNcCXtCdPTqRuduEKGC67909829;     VDNcCXtCdPTqRuduEKGC67909829 = VDNcCXtCdPTqRuduEKGC74318995;     VDNcCXtCdPTqRuduEKGC74318995 = VDNcCXtCdPTqRuduEKGC33829775;     VDNcCXtCdPTqRuduEKGC33829775 = VDNcCXtCdPTqRuduEKGC96655383;     VDNcCXtCdPTqRuduEKGC96655383 = VDNcCXtCdPTqRuduEKGC2758749;     VDNcCXtCdPTqRuduEKGC2758749 = VDNcCXtCdPTqRuduEKGC67482783;     VDNcCXtCdPTqRuduEKGC67482783 = VDNcCXtCdPTqRuduEKGC21472248;     VDNcCXtCdPTqRuduEKGC21472248 = VDNcCXtCdPTqRuduEKGC38898997;     VDNcCXtCdPTqRuduEKGC38898997 = VDNcCXtCdPTqRuduEKGC61259777;     VDNcCXtCdPTqRuduEKGC61259777 = VDNcCXtCdPTqRuduEKGC13085895;     VDNcCXtCdPTqRuduEKGC13085895 = VDNcCXtCdPTqRuduEKGC19089250;     VDNcCXtCdPTqRuduEKGC19089250 = VDNcCXtCdPTqRuduEKGC79721469;     VDNcCXtCdPTqRuduEKGC79721469 = VDNcCXtCdPTqRuduEKGC27832595;     VDNcCXtCdPTqRuduEKGC27832595 = VDNcCXtCdPTqRuduEKGC32332124;     VDNcCXtCdPTqRuduEKGC32332124 = VDNcCXtCdPTqRuduEKGC68869361;     VDNcCXtCdPTqRuduEKGC68869361 = VDNcCXtCdPTqRuduEKGC19938518;     VDNcCXtCdPTqRuduEKGC19938518 = VDNcCXtCdPTqRuduEKGC40449677;     VDNcCXtCdPTqRuduEKGC40449677 = VDNcCXtCdPTqRuduEKGC30035808;     VDNcCXtCdPTqRuduEKGC30035808 = VDNcCXtCdPTqRuduEKGC6060804;     VDNcCXtCdPTqRuduEKGC6060804 = VDNcCXtCdPTqRuduEKGC39479849;     VDNcCXtCdPTqRuduEKGC39479849 = VDNcCXtCdPTqRuduEKGC81010862;     VDNcCXtCdPTqRuduEKGC81010862 = VDNcCXtCdPTqRuduEKGC19938563;     VDNcCXtCdPTqRuduEKGC19938563 = VDNcCXtCdPTqRuduEKGC45236667;     VDNcCXtCdPTqRuduEKGC45236667 = VDNcCXtCdPTqRuduEKGC85115361;     VDNcCXtCdPTqRuduEKGC85115361 = VDNcCXtCdPTqRuduEKGC45754513;     VDNcCXtCdPTqRuduEKGC45754513 = VDNcCXtCdPTqRuduEKGC9751093;     VDNcCXtCdPTqRuduEKGC9751093 = VDNcCXtCdPTqRuduEKGC58170256;     VDNcCXtCdPTqRuduEKGC58170256 = VDNcCXtCdPTqRuduEKGC60030534;     VDNcCXtCdPTqRuduEKGC60030534 = VDNcCXtCdPTqRuduEKGC57812800;     VDNcCXtCdPTqRuduEKGC57812800 = VDNcCXtCdPTqRuduEKGC10330786;     VDNcCXtCdPTqRuduEKGC10330786 = VDNcCXtCdPTqRuduEKGC3021723;     VDNcCXtCdPTqRuduEKGC3021723 = VDNcCXtCdPTqRuduEKGC95742163;}
// Junk Finished

// Junk Code By Peatreat & Thaisen's Gen
void gOHZCTepMYtytzVxEWlW48311402() {     int bhhVSAwkgqcrHpFRPcWO28002972 = -189360712;    int bhhVSAwkgqcrHpFRPcWO60991675 = -280881839;    int bhhVSAwkgqcrHpFRPcWO65978310 = -248671037;    int bhhVSAwkgqcrHpFRPcWO10478370 = -660618377;    int bhhVSAwkgqcrHpFRPcWO47259894 = -567013590;    int bhhVSAwkgqcrHpFRPcWO5723300 = 52283382;    int bhhVSAwkgqcrHpFRPcWO15948067 = -934869061;    int bhhVSAwkgqcrHpFRPcWO96165935 = -110000768;    int bhhVSAwkgqcrHpFRPcWO79370390 = -48433771;    int bhhVSAwkgqcrHpFRPcWO71313921 = -763657361;    int bhhVSAwkgqcrHpFRPcWO82323172 = -481257723;    int bhhVSAwkgqcrHpFRPcWO32181543 = -639186899;    int bhhVSAwkgqcrHpFRPcWO32101683 = -513267242;    int bhhVSAwkgqcrHpFRPcWO33448612 = -61585631;    int bhhVSAwkgqcrHpFRPcWO90675462 = -463551491;    int bhhVSAwkgqcrHpFRPcWO24828111 = -255725525;    int bhhVSAwkgqcrHpFRPcWO77346670 = -230853346;    int bhhVSAwkgqcrHpFRPcWO77552549 = -316731215;    int bhhVSAwkgqcrHpFRPcWO50306235 = -233516826;    int bhhVSAwkgqcrHpFRPcWO40548220 = -785564948;    int bhhVSAwkgqcrHpFRPcWO26256683 = -172079913;    int bhhVSAwkgqcrHpFRPcWO21945301 = -553878492;    int bhhVSAwkgqcrHpFRPcWO65657981 = -45745547;    int bhhVSAwkgqcrHpFRPcWO66084142 = -811263089;    int bhhVSAwkgqcrHpFRPcWO5084952 = -550364249;    int bhhVSAwkgqcrHpFRPcWO81132455 = -148114888;    int bhhVSAwkgqcrHpFRPcWO39319519 = -462198953;    int bhhVSAwkgqcrHpFRPcWO97493695 = -168654821;    int bhhVSAwkgqcrHpFRPcWO87301007 = -658973859;    int bhhVSAwkgqcrHpFRPcWO7887179 = -943455684;    int bhhVSAwkgqcrHpFRPcWO620897 = -78790754;    int bhhVSAwkgqcrHpFRPcWO47709573 = -259298772;    int bhhVSAwkgqcrHpFRPcWO20874092 = -668899435;    int bhhVSAwkgqcrHpFRPcWO33702999 = -121969808;    int bhhVSAwkgqcrHpFRPcWO61174191 = -489726067;    int bhhVSAwkgqcrHpFRPcWO72566589 = -428164845;    int bhhVSAwkgqcrHpFRPcWO23639089 = -665161851;    int bhhVSAwkgqcrHpFRPcWO66924541 = -805530195;    int bhhVSAwkgqcrHpFRPcWO34051577 = -918740987;    int bhhVSAwkgqcrHpFRPcWO42853820 = -932451219;    int bhhVSAwkgqcrHpFRPcWO76931099 = -501942719;    int bhhVSAwkgqcrHpFRPcWO25687658 = -137169732;    int bhhVSAwkgqcrHpFRPcWO63046019 = -73305778;    int bhhVSAwkgqcrHpFRPcWO3125477 = -8211567;    int bhhVSAwkgqcrHpFRPcWO10438408 = -912460557;    int bhhVSAwkgqcrHpFRPcWO85001931 = -511251257;    int bhhVSAwkgqcrHpFRPcWO63773062 = -125538630;    int bhhVSAwkgqcrHpFRPcWO40577054 = -161572006;    int bhhVSAwkgqcrHpFRPcWO48961747 = -789282992;    int bhhVSAwkgqcrHpFRPcWO40551423 = -903324047;    int bhhVSAwkgqcrHpFRPcWO6084255 = -25902644;    int bhhVSAwkgqcrHpFRPcWO49421967 = -398218759;    int bhhVSAwkgqcrHpFRPcWO88361436 = -759292302;    int bhhVSAwkgqcrHpFRPcWO50123142 = -857678078;    int bhhVSAwkgqcrHpFRPcWO22054720 = -993847196;    int bhhVSAwkgqcrHpFRPcWO6057671 = -635482220;    int bhhVSAwkgqcrHpFRPcWO95333694 = -135136293;    int bhhVSAwkgqcrHpFRPcWO99894167 = -437407949;    int bhhVSAwkgqcrHpFRPcWO5393418 = -10254128;    int bhhVSAwkgqcrHpFRPcWO66127439 = -318898703;    int bhhVSAwkgqcrHpFRPcWO66403781 = -485517665;    int bhhVSAwkgqcrHpFRPcWO18454372 = -666214240;    int bhhVSAwkgqcrHpFRPcWO8864929 = -451026910;    int bhhVSAwkgqcrHpFRPcWO71483212 = -104978087;    int bhhVSAwkgqcrHpFRPcWO70693025 = -584866607;    int bhhVSAwkgqcrHpFRPcWO34613599 = -121958951;    int bhhVSAwkgqcrHpFRPcWO11307451 = -970287465;    int bhhVSAwkgqcrHpFRPcWO98398684 = -291297435;    int bhhVSAwkgqcrHpFRPcWO72274421 = -571859564;    int bhhVSAwkgqcrHpFRPcWO18108873 = 64613353;    int bhhVSAwkgqcrHpFRPcWO1189022 = -590563674;    int bhhVSAwkgqcrHpFRPcWO10422129 = -425323152;    int bhhVSAwkgqcrHpFRPcWO43500973 = -397990228;    int bhhVSAwkgqcrHpFRPcWO7452416 = -301065607;    int bhhVSAwkgqcrHpFRPcWO63617121 = -183622230;    int bhhVSAwkgqcrHpFRPcWO569025 = 65089819;    int bhhVSAwkgqcrHpFRPcWO58899282 = -380572714;    int bhhVSAwkgqcrHpFRPcWO62532504 = 62466020;    int bhhVSAwkgqcrHpFRPcWO55645735 = -898802532;    int bhhVSAwkgqcrHpFRPcWO20083021 = 60887008;    int bhhVSAwkgqcrHpFRPcWO17359393 = 77423742;    int bhhVSAwkgqcrHpFRPcWO98742464 = -200626947;    int bhhVSAwkgqcrHpFRPcWO48531948 = -379371830;    int bhhVSAwkgqcrHpFRPcWO46749584 = -755649813;    int bhhVSAwkgqcrHpFRPcWO1802925 = -817553041;    int bhhVSAwkgqcrHpFRPcWO51198929 = -680571996;    int bhhVSAwkgqcrHpFRPcWO59348136 = -500006471;    int bhhVSAwkgqcrHpFRPcWO70750950 = -811221357;    int bhhVSAwkgqcrHpFRPcWO11648280 = -128122612;    int bhhVSAwkgqcrHpFRPcWO55116520 = -854243847;    int bhhVSAwkgqcrHpFRPcWO77232895 = -193028552;    int bhhVSAwkgqcrHpFRPcWO23744921 = -127753903;    int bhhVSAwkgqcrHpFRPcWO61531124 = -695276067;    int bhhVSAwkgqcrHpFRPcWO67924138 = -499842285;    int bhhVSAwkgqcrHpFRPcWO76450039 = -346933554;    int bhhVSAwkgqcrHpFRPcWO58476728 = -835728479;    int bhhVSAwkgqcrHpFRPcWO16822730 = -686142822;    int bhhVSAwkgqcrHpFRPcWO91562806 = -968327692;    int bhhVSAwkgqcrHpFRPcWO32432452 = -423344960;    int bhhVSAwkgqcrHpFRPcWO75824808 = -189360712;     bhhVSAwkgqcrHpFRPcWO28002972 = bhhVSAwkgqcrHpFRPcWO60991675;     bhhVSAwkgqcrHpFRPcWO60991675 = bhhVSAwkgqcrHpFRPcWO65978310;     bhhVSAwkgqcrHpFRPcWO65978310 = bhhVSAwkgqcrHpFRPcWO10478370;     bhhVSAwkgqcrHpFRPcWO10478370 = bhhVSAwkgqcrHpFRPcWO47259894;     bhhVSAwkgqcrHpFRPcWO47259894 = bhhVSAwkgqcrHpFRPcWO5723300;     bhhVSAwkgqcrHpFRPcWO5723300 = bhhVSAwkgqcrHpFRPcWO15948067;     bhhVSAwkgqcrHpFRPcWO15948067 = bhhVSAwkgqcrHpFRPcWO96165935;     bhhVSAwkgqcrHpFRPcWO96165935 = bhhVSAwkgqcrHpFRPcWO79370390;     bhhVSAwkgqcrHpFRPcWO79370390 = bhhVSAwkgqcrHpFRPcWO71313921;     bhhVSAwkgqcrHpFRPcWO71313921 = bhhVSAwkgqcrHpFRPcWO82323172;     bhhVSAwkgqcrHpFRPcWO82323172 = bhhVSAwkgqcrHpFRPcWO32181543;     bhhVSAwkgqcrHpFRPcWO32181543 = bhhVSAwkgqcrHpFRPcWO32101683;     bhhVSAwkgqcrHpFRPcWO32101683 = bhhVSAwkgqcrHpFRPcWO33448612;     bhhVSAwkgqcrHpFRPcWO33448612 = bhhVSAwkgqcrHpFRPcWO90675462;     bhhVSAwkgqcrHpFRPcWO90675462 = bhhVSAwkgqcrHpFRPcWO24828111;     bhhVSAwkgqcrHpFRPcWO24828111 = bhhVSAwkgqcrHpFRPcWO77346670;     bhhVSAwkgqcrHpFRPcWO77346670 = bhhVSAwkgqcrHpFRPcWO77552549;     bhhVSAwkgqcrHpFRPcWO77552549 = bhhVSAwkgqcrHpFRPcWO50306235;     bhhVSAwkgqcrHpFRPcWO50306235 = bhhVSAwkgqcrHpFRPcWO40548220;     bhhVSAwkgqcrHpFRPcWO40548220 = bhhVSAwkgqcrHpFRPcWO26256683;     bhhVSAwkgqcrHpFRPcWO26256683 = bhhVSAwkgqcrHpFRPcWO21945301;     bhhVSAwkgqcrHpFRPcWO21945301 = bhhVSAwkgqcrHpFRPcWO65657981;     bhhVSAwkgqcrHpFRPcWO65657981 = bhhVSAwkgqcrHpFRPcWO66084142;     bhhVSAwkgqcrHpFRPcWO66084142 = bhhVSAwkgqcrHpFRPcWO5084952;     bhhVSAwkgqcrHpFRPcWO5084952 = bhhVSAwkgqcrHpFRPcWO81132455;     bhhVSAwkgqcrHpFRPcWO81132455 = bhhVSAwkgqcrHpFRPcWO39319519;     bhhVSAwkgqcrHpFRPcWO39319519 = bhhVSAwkgqcrHpFRPcWO97493695;     bhhVSAwkgqcrHpFRPcWO97493695 = bhhVSAwkgqcrHpFRPcWO87301007;     bhhVSAwkgqcrHpFRPcWO87301007 = bhhVSAwkgqcrHpFRPcWO7887179;     bhhVSAwkgqcrHpFRPcWO7887179 = bhhVSAwkgqcrHpFRPcWO620897;     bhhVSAwkgqcrHpFRPcWO620897 = bhhVSAwkgqcrHpFRPcWO47709573;     bhhVSAwkgqcrHpFRPcWO47709573 = bhhVSAwkgqcrHpFRPcWO20874092;     bhhVSAwkgqcrHpFRPcWO20874092 = bhhVSAwkgqcrHpFRPcWO33702999;     bhhVSAwkgqcrHpFRPcWO33702999 = bhhVSAwkgqcrHpFRPcWO61174191;     bhhVSAwkgqcrHpFRPcWO61174191 = bhhVSAwkgqcrHpFRPcWO72566589;     bhhVSAwkgqcrHpFRPcWO72566589 = bhhVSAwkgqcrHpFRPcWO23639089;     bhhVSAwkgqcrHpFRPcWO23639089 = bhhVSAwkgqcrHpFRPcWO66924541;     bhhVSAwkgqcrHpFRPcWO66924541 = bhhVSAwkgqcrHpFRPcWO34051577;     bhhVSAwkgqcrHpFRPcWO34051577 = bhhVSAwkgqcrHpFRPcWO42853820;     bhhVSAwkgqcrHpFRPcWO42853820 = bhhVSAwkgqcrHpFRPcWO76931099;     bhhVSAwkgqcrHpFRPcWO76931099 = bhhVSAwkgqcrHpFRPcWO25687658;     bhhVSAwkgqcrHpFRPcWO25687658 = bhhVSAwkgqcrHpFRPcWO63046019;     bhhVSAwkgqcrHpFRPcWO63046019 = bhhVSAwkgqcrHpFRPcWO3125477;     bhhVSAwkgqcrHpFRPcWO3125477 = bhhVSAwkgqcrHpFRPcWO10438408;     bhhVSAwkgqcrHpFRPcWO10438408 = bhhVSAwkgqcrHpFRPcWO85001931;     bhhVSAwkgqcrHpFRPcWO85001931 = bhhVSAwkgqcrHpFRPcWO63773062;     bhhVSAwkgqcrHpFRPcWO63773062 = bhhVSAwkgqcrHpFRPcWO40577054;     bhhVSAwkgqcrHpFRPcWO40577054 = bhhVSAwkgqcrHpFRPcWO48961747;     bhhVSAwkgqcrHpFRPcWO48961747 = bhhVSAwkgqcrHpFRPcWO40551423;     bhhVSAwkgqcrHpFRPcWO40551423 = bhhVSAwkgqcrHpFRPcWO6084255;     bhhVSAwkgqcrHpFRPcWO6084255 = bhhVSAwkgqcrHpFRPcWO49421967;     bhhVSAwkgqcrHpFRPcWO49421967 = bhhVSAwkgqcrHpFRPcWO88361436;     bhhVSAwkgqcrHpFRPcWO88361436 = bhhVSAwkgqcrHpFRPcWO50123142;     bhhVSAwkgqcrHpFRPcWO50123142 = bhhVSAwkgqcrHpFRPcWO22054720;     bhhVSAwkgqcrHpFRPcWO22054720 = bhhVSAwkgqcrHpFRPcWO6057671;     bhhVSAwkgqcrHpFRPcWO6057671 = bhhVSAwkgqcrHpFRPcWO95333694;     bhhVSAwkgqcrHpFRPcWO95333694 = bhhVSAwkgqcrHpFRPcWO99894167;     bhhVSAwkgqcrHpFRPcWO99894167 = bhhVSAwkgqcrHpFRPcWO5393418;     bhhVSAwkgqcrHpFRPcWO5393418 = bhhVSAwkgqcrHpFRPcWO66127439;     bhhVSAwkgqcrHpFRPcWO66127439 = bhhVSAwkgqcrHpFRPcWO66403781;     bhhVSAwkgqcrHpFRPcWO66403781 = bhhVSAwkgqcrHpFRPcWO18454372;     bhhVSAwkgqcrHpFRPcWO18454372 = bhhVSAwkgqcrHpFRPcWO8864929;     bhhVSAwkgqcrHpFRPcWO8864929 = bhhVSAwkgqcrHpFRPcWO71483212;     bhhVSAwkgqcrHpFRPcWO71483212 = bhhVSAwkgqcrHpFRPcWO70693025;     bhhVSAwkgqcrHpFRPcWO70693025 = bhhVSAwkgqcrHpFRPcWO34613599;     bhhVSAwkgqcrHpFRPcWO34613599 = bhhVSAwkgqcrHpFRPcWO11307451;     bhhVSAwkgqcrHpFRPcWO11307451 = bhhVSAwkgqcrHpFRPcWO98398684;     bhhVSAwkgqcrHpFRPcWO98398684 = bhhVSAwkgqcrHpFRPcWO72274421;     bhhVSAwkgqcrHpFRPcWO72274421 = bhhVSAwkgqcrHpFRPcWO18108873;     bhhVSAwkgqcrHpFRPcWO18108873 = bhhVSAwkgqcrHpFRPcWO1189022;     bhhVSAwkgqcrHpFRPcWO1189022 = bhhVSAwkgqcrHpFRPcWO10422129;     bhhVSAwkgqcrHpFRPcWO10422129 = bhhVSAwkgqcrHpFRPcWO43500973;     bhhVSAwkgqcrHpFRPcWO43500973 = bhhVSAwkgqcrHpFRPcWO7452416;     bhhVSAwkgqcrHpFRPcWO7452416 = bhhVSAwkgqcrHpFRPcWO63617121;     bhhVSAwkgqcrHpFRPcWO63617121 = bhhVSAwkgqcrHpFRPcWO569025;     bhhVSAwkgqcrHpFRPcWO569025 = bhhVSAwkgqcrHpFRPcWO58899282;     bhhVSAwkgqcrHpFRPcWO58899282 = bhhVSAwkgqcrHpFRPcWO62532504;     bhhVSAwkgqcrHpFRPcWO62532504 = bhhVSAwkgqcrHpFRPcWO55645735;     bhhVSAwkgqcrHpFRPcWO55645735 = bhhVSAwkgqcrHpFRPcWO20083021;     bhhVSAwkgqcrHpFRPcWO20083021 = bhhVSAwkgqcrHpFRPcWO17359393;     bhhVSAwkgqcrHpFRPcWO17359393 = bhhVSAwkgqcrHpFRPcWO98742464;     bhhVSAwkgqcrHpFRPcWO98742464 = bhhVSAwkgqcrHpFRPcWO48531948;     bhhVSAwkgqcrHpFRPcWO48531948 = bhhVSAwkgqcrHpFRPcWO46749584;     bhhVSAwkgqcrHpFRPcWO46749584 = bhhVSAwkgqcrHpFRPcWO1802925;     bhhVSAwkgqcrHpFRPcWO1802925 = bhhVSAwkgqcrHpFRPcWO51198929;     bhhVSAwkgqcrHpFRPcWO51198929 = bhhVSAwkgqcrHpFRPcWO59348136;     bhhVSAwkgqcrHpFRPcWO59348136 = bhhVSAwkgqcrHpFRPcWO70750950;     bhhVSAwkgqcrHpFRPcWO70750950 = bhhVSAwkgqcrHpFRPcWO11648280;     bhhVSAwkgqcrHpFRPcWO11648280 = bhhVSAwkgqcrHpFRPcWO55116520;     bhhVSAwkgqcrHpFRPcWO55116520 = bhhVSAwkgqcrHpFRPcWO77232895;     bhhVSAwkgqcrHpFRPcWO77232895 = bhhVSAwkgqcrHpFRPcWO23744921;     bhhVSAwkgqcrHpFRPcWO23744921 = bhhVSAwkgqcrHpFRPcWO61531124;     bhhVSAwkgqcrHpFRPcWO61531124 = bhhVSAwkgqcrHpFRPcWO67924138;     bhhVSAwkgqcrHpFRPcWO67924138 = bhhVSAwkgqcrHpFRPcWO76450039;     bhhVSAwkgqcrHpFRPcWO76450039 = bhhVSAwkgqcrHpFRPcWO58476728;     bhhVSAwkgqcrHpFRPcWO58476728 = bhhVSAwkgqcrHpFRPcWO16822730;     bhhVSAwkgqcrHpFRPcWO16822730 = bhhVSAwkgqcrHpFRPcWO91562806;     bhhVSAwkgqcrHpFRPcWO91562806 = bhhVSAwkgqcrHpFRPcWO32432452;     bhhVSAwkgqcrHpFRPcWO32432452 = bhhVSAwkgqcrHpFRPcWO75824808;     bhhVSAwkgqcrHpFRPcWO75824808 = bhhVSAwkgqcrHpFRPcWO28002972;}
// Junk Finished

// Junk Code By Peatreat & Thaisen's Gen
void szCWUdUntJMAjPqCIgCa72411188() {     int BWfHinWRviSfyqFVLHPv30921919 = -510633463;    int BWfHinWRviSfyqFVLHPv92315311 = -783546898;    int BWfHinWRviSfyqFVLHPv77910596 = -836696266;    int BWfHinWRviSfyqFVLHPv6837909 = -850451645;    int BWfHinWRviSfyqFVLHPv44987632 = -531798175;    int BWfHinWRviSfyqFVLHPv95014342 = 82159773;    int BWfHinWRviSfyqFVLHPv63271927 = -313550138;    int BWfHinWRviSfyqFVLHPv6549561 = -486789447;    int BWfHinWRviSfyqFVLHPv15846725 = -304438431;    int BWfHinWRviSfyqFVLHPv85148782 = -500812648;    int BWfHinWRviSfyqFVLHPv65641768 = -44891823;    int BWfHinWRviSfyqFVLHPv84589878 = -890629488;    int BWfHinWRviSfyqFVLHPv8738205 = -707627285;    int BWfHinWRviSfyqFVLHPv67647324 = -47008662;    int BWfHinWRviSfyqFVLHPv66884278 = -892945412;    int BWfHinWRviSfyqFVLHPv2868374 = -174201134;    int BWfHinWRviSfyqFVLHPv21207909 = -336358485;    int BWfHinWRviSfyqFVLHPv59466214 = 82280813;    int BWfHinWRviSfyqFVLHPv91952986 = 8585810;    int BWfHinWRviSfyqFVLHPv38453202 = -320662946;    int BWfHinWRviSfyqFVLHPv14683718 = -657354895;    int BWfHinWRviSfyqFVLHPv70778436 = -546841592;    int BWfHinWRviSfyqFVLHPv778298 = 75748480;    int BWfHinWRviSfyqFVLHPv74862261 = -638711954;    int BWfHinWRviSfyqFVLHPv29421710 = -977623264;    int BWfHinWRviSfyqFVLHPv64667988 = -906213893;    int BWfHinWRviSfyqFVLHPv2480809 = -554634514;    int BWfHinWRviSfyqFVLHPv52613471 = -893375996;    int BWfHinWRviSfyqFVLHPv52448611 = -989319832;    int BWfHinWRviSfyqFVLHPv32337504 = -772312856;    int BWfHinWRviSfyqFVLHPv15107617 = -294780813;    int BWfHinWRviSfyqFVLHPv65671453 = -347021109;    int BWfHinWRviSfyqFVLHPv90436658 = -643035163;    int BWfHinWRviSfyqFVLHPv34335179 = -141493451;    int BWfHinWRviSfyqFVLHPv97473219 = -490779683;    int BWfHinWRviSfyqFVLHPv78193521 = -439991613;    int BWfHinWRviSfyqFVLHPv48695350 = -971233728;    int BWfHinWRviSfyqFVLHPv79068168 = -595946851;    int BWfHinWRviSfyqFVLHPv15046384 = -855372194;    int BWfHinWRviSfyqFVLHPv41757798 = 27855848;    int BWfHinWRviSfyqFVLHPv64206413 = -495334089;    int BWfHinWRviSfyqFVLHPv2336917 = -756182197;    int BWfHinWRviSfyqFVLHPv97462970 = -13034564;    int BWfHinWRviSfyqFVLHPv19184058 = -656502555;    int BWfHinWRviSfyqFVLHPv98393262 = -444481072;    int BWfHinWRviSfyqFVLHPv38641087 = -156148931;    int BWfHinWRviSfyqFVLHPv94080580 = -790255560;    int BWfHinWRviSfyqFVLHPv85019898 = -834819683;    int BWfHinWRviSfyqFVLHPv2286062 = -725060452;    int BWfHinWRviSfyqFVLHPv96736826 = -184331960;    int BWfHinWRviSfyqFVLHPv40517902 = 25386010;    int BWfHinWRviSfyqFVLHPv6824364 = -451685181;    int BWfHinWRviSfyqFVLHPv96850693 = -755094571;    int BWfHinWRviSfyqFVLHPv37479658 = -38765250;    int BWfHinWRviSfyqFVLHPv57727470 = -259992677;    int BWfHinWRviSfyqFVLHPv60143482 = -963791871;    int BWfHinWRviSfyqFVLHPv91537014 = -759295378;    int BWfHinWRviSfyqFVLHPv3048336 = -97984312;    int BWfHinWRviSfyqFVLHPv77416199 = -872828382;    int BWfHinWRviSfyqFVLHPv80319643 = -625584282;    int BWfHinWRviSfyqFVLHPv92533534 = -363205714;    int BWfHinWRviSfyqFVLHPv10658457 = -420174142;    int BWfHinWRviSfyqFVLHPv54100950 = -497469615;    int BWfHinWRviSfyqFVLHPv83509221 = -532125575;    int BWfHinWRviSfyqFVLHPv70041166 = -106031835;    int BWfHinWRviSfyqFVLHPv99970314 = -697870715;    int BWfHinWRviSfyqFVLHPv94153219 = -147594325;    int BWfHinWRviSfyqFVLHPv74403025 = -466133834;    int BWfHinWRviSfyqFVLHPv70174105 = -556228979;    int BWfHinWRviSfyqFVLHPv88690756 = -352953800;    int BWfHinWRviSfyqFVLHPv54173024 = -202967407;    int BWfHinWRviSfyqFVLHPv42139740 = -740411634;    int BWfHinWRviSfyqFVLHPv44419831 = -62346994;    int BWfHinWRviSfyqFVLHPv50195189 = 80729961;    int BWfHinWRviSfyqFVLHPv74246788 = -825328857;    int BWfHinWRviSfyqFVLHPv12346802 = -901172699;    int BWfHinWRviSfyqFVLHPv73315466 = -433807028;    int BWfHinWRviSfyqFVLHPv81594240 = -267748966;    int BWfHinWRviSfyqFVLHPv76468998 = -94230883;    int BWfHinWRviSfyqFVLHPv90780623 = -721474333;    int BWfHinWRviSfyqFVLHPv70587408 = -15958333;    int BWfHinWRviSfyqFVLHPv17460910 = -719814832;    int BWfHinWRviSfyqFVLHPv50327409 = -68315545;    int BWfHinWRviSfyqFVLHPv55711785 = -704987872;    int BWfHinWRviSfyqFVLHPv91819602 = -697698867;    int BWfHinWRviSfyqFVLHPv8283253 = -843095632;    int BWfHinWRviSfyqFVLHPv68820760 = -591926538;    int BWfHinWRviSfyqFVLHPv52957000 = -504269913;    int BWfHinWRviSfyqFVLHPv76607709 = -881500775;    int BWfHinWRviSfyqFVLHPv37329737 = -526987813;    int BWfHinWRviSfyqFVLHPv86656507 = -680696235;    int BWfHinWRviSfyqFVLHPv45647014 = -773249416;    int BWfHinWRviSfyqFVLHPv1651969 = -723118470;    int BWfHinWRviSfyqFVLHPv34726740 = -129787912;    int BWfHinWRviSfyqFVLHPv49224264 = -608938438;    int BWfHinWRviSfyqFVLHPv53547957 = 24840052;    int BWfHinWRviSfyqFVLHPv48235967 = -158712583;    int BWfHinWRviSfyqFVLHPv13953749 = -480908990;    int BWfHinWRviSfyqFVLHPv49142891 = -450470720;    int BWfHinWRviSfyqFVLHPv98422948 = -510633463;     BWfHinWRviSfyqFVLHPv30921919 = BWfHinWRviSfyqFVLHPv92315311;     BWfHinWRviSfyqFVLHPv92315311 = BWfHinWRviSfyqFVLHPv77910596;     BWfHinWRviSfyqFVLHPv77910596 = BWfHinWRviSfyqFVLHPv6837909;     BWfHinWRviSfyqFVLHPv6837909 = BWfHinWRviSfyqFVLHPv44987632;     BWfHinWRviSfyqFVLHPv44987632 = BWfHinWRviSfyqFVLHPv95014342;     BWfHinWRviSfyqFVLHPv95014342 = BWfHinWRviSfyqFVLHPv63271927;     BWfHinWRviSfyqFVLHPv63271927 = BWfHinWRviSfyqFVLHPv6549561;     BWfHinWRviSfyqFVLHPv6549561 = BWfHinWRviSfyqFVLHPv15846725;     BWfHinWRviSfyqFVLHPv15846725 = BWfHinWRviSfyqFVLHPv85148782;     BWfHinWRviSfyqFVLHPv85148782 = BWfHinWRviSfyqFVLHPv65641768;     BWfHinWRviSfyqFVLHPv65641768 = BWfHinWRviSfyqFVLHPv84589878;     BWfHinWRviSfyqFVLHPv84589878 = BWfHinWRviSfyqFVLHPv8738205;     BWfHinWRviSfyqFVLHPv8738205 = BWfHinWRviSfyqFVLHPv67647324;     BWfHinWRviSfyqFVLHPv67647324 = BWfHinWRviSfyqFVLHPv66884278;     BWfHinWRviSfyqFVLHPv66884278 = BWfHinWRviSfyqFVLHPv2868374;     BWfHinWRviSfyqFVLHPv2868374 = BWfHinWRviSfyqFVLHPv21207909;     BWfHinWRviSfyqFVLHPv21207909 = BWfHinWRviSfyqFVLHPv59466214;     BWfHinWRviSfyqFVLHPv59466214 = BWfHinWRviSfyqFVLHPv91952986;     BWfHinWRviSfyqFVLHPv91952986 = BWfHinWRviSfyqFVLHPv38453202;     BWfHinWRviSfyqFVLHPv38453202 = BWfHinWRviSfyqFVLHPv14683718;     BWfHinWRviSfyqFVLHPv14683718 = BWfHinWRviSfyqFVLHPv70778436;     BWfHinWRviSfyqFVLHPv70778436 = BWfHinWRviSfyqFVLHPv778298;     BWfHinWRviSfyqFVLHPv778298 = BWfHinWRviSfyqFVLHPv74862261;     BWfHinWRviSfyqFVLHPv74862261 = BWfHinWRviSfyqFVLHPv29421710;     BWfHinWRviSfyqFVLHPv29421710 = BWfHinWRviSfyqFVLHPv64667988;     BWfHinWRviSfyqFVLHPv64667988 = BWfHinWRviSfyqFVLHPv2480809;     BWfHinWRviSfyqFVLHPv2480809 = BWfHinWRviSfyqFVLHPv52613471;     BWfHinWRviSfyqFVLHPv52613471 = BWfHinWRviSfyqFVLHPv52448611;     BWfHinWRviSfyqFVLHPv52448611 = BWfHinWRviSfyqFVLHPv32337504;     BWfHinWRviSfyqFVLHPv32337504 = BWfHinWRviSfyqFVLHPv15107617;     BWfHinWRviSfyqFVLHPv15107617 = BWfHinWRviSfyqFVLHPv65671453;     BWfHinWRviSfyqFVLHPv65671453 = BWfHinWRviSfyqFVLHPv90436658;     BWfHinWRviSfyqFVLHPv90436658 = BWfHinWRviSfyqFVLHPv34335179;     BWfHinWRviSfyqFVLHPv34335179 = BWfHinWRviSfyqFVLHPv97473219;     BWfHinWRviSfyqFVLHPv97473219 = BWfHinWRviSfyqFVLHPv78193521;     BWfHinWRviSfyqFVLHPv78193521 = BWfHinWRviSfyqFVLHPv48695350;     BWfHinWRviSfyqFVLHPv48695350 = BWfHinWRviSfyqFVLHPv79068168;     BWfHinWRviSfyqFVLHPv79068168 = BWfHinWRviSfyqFVLHPv15046384;     BWfHinWRviSfyqFVLHPv15046384 = BWfHinWRviSfyqFVLHPv41757798;     BWfHinWRviSfyqFVLHPv41757798 = BWfHinWRviSfyqFVLHPv64206413;     BWfHinWRviSfyqFVLHPv64206413 = BWfHinWRviSfyqFVLHPv2336917;     BWfHinWRviSfyqFVLHPv2336917 = BWfHinWRviSfyqFVLHPv97462970;     BWfHinWRviSfyqFVLHPv97462970 = BWfHinWRviSfyqFVLHPv19184058;     BWfHinWRviSfyqFVLHPv19184058 = BWfHinWRviSfyqFVLHPv98393262;     BWfHinWRviSfyqFVLHPv98393262 = BWfHinWRviSfyqFVLHPv38641087;     BWfHinWRviSfyqFVLHPv38641087 = BWfHinWRviSfyqFVLHPv94080580;     BWfHinWRviSfyqFVLHPv94080580 = BWfHinWRviSfyqFVLHPv85019898;     BWfHinWRviSfyqFVLHPv85019898 = BWfHinWRviSfyqFVLHPv2286062;     BWfHinWRviSfyqFVLHPv2286062 = BWfHinWRviSfyqFVLHPv96736826;     BWfHinWRviSfyqFVLHPv96736826 = BWfHinWRviSfyqFVLHPv40517902;     BWfHinWRviSfyqFVLHPv40517902 = BWfHinWRviSfyqFVLHPv6824364;     BWfHinWRviSfyqFVLHPv6824364 = BWfHinWRviSfyqFVLHPv96850693;     BWfHinWRviSfyqFVLHPv96850693 = BWfHinWRviSfyqFVLHPv37479658;     BWfHinWRviSfyqFVLHPv37479658 = BWfHinWRviSfyqFVLHPv57727470;     BWfHinWRviSfyqFVLHPv57727470 = BWfHinWRviSfyqFVLHPv60143482;     BWfHinWRviSfyqFVLHPv60143482 = BWfHinWRviSfyqFVLHPv91537014;     BWfHinWRviSfyqFVLHPv91537014 = BWfHinWRviSfyqFVLHPv3048336;     BWfHinWRviSfyqFVLHPv3048336 = BWfHinWRviSfyqFVLHPv77416199;     BWfHinWRviSfyqFVLHPv77416199 = BWfHinWRviSfyqFVLHPv80319643;     BWfHinWRviSfyqFVLHPv80319643 = BWfHinWRviSfyqFVLHPv92533534;     BWfHinWRviSfyqFVLHPv92533534 = BWfHinWRviSfyqFVLHPv10658457;     BWfHinWRviSfyqFVLHPv10658457 = BWfHinWRviSfyqFVLHPv54100950;     BWfHinWRviSfyqFVLHPv54100950 = BWfHinWRviSfyqFVLHPv83509221;     BWfHinWRviSfyqFVLHPv83509221 = BWfHinWRviSfyqFVLHPv70041166;     BWfHinWRviSfyqFVLHPv70041166 = BWfHinWRviSfyqFVLHPv99970314;     BWfHinWRviSfyqFVLHPv99970314 = BWfHinWRviSfyqFVLHPv94153219;     BWfHinWRviSfyqFVLHPv94153219 = BWfHinWRviSfyqFVLHPv74403025;     BWfHinWRviSfyqFVLHPv74403025 = BWfHinWRviSfyqFVLHPv70174105;     BWfHinWRviSfyqFVLHPv70174105 = BWfHinWRviSfyqFVLHPv88690756;     BWfHinWRviSfyqFVLHPv88690756 = BWfHinWRviSfyqFVLHPv54173024;     BWfHinWRviSfyqFVLHPv54173024 = BWfHinWRviSfyqFVLHPv42139740;     BWfHinWRviSfyqFVLHPv42139740 = BWfHinWRviSfyqFVLHPv44419831;     BWfHinWRviSfyqFVLHPv44419831 = BWfHinWRviSfyqFVLHPv50195189;     BWfHinWRviSfyqFVLHPv50195189 = BWfHinWRviSfyqFVLHPv74246788;     BWfHinWRviSfyqFVLHPv74246788 = BWfHinWRviSfyqFVLHPv12346802;     BWfHinWRviSfyqFVLHPv12346802 = BWfHinWRviSfyqFVLHPv73315466;     BWfHinWRviSfyqFVLHPv73315466 = BWfHinWRviSfyqFVLHPv81594240;     BWfHinWRviSfyqFVLHPv81594240 = BWfHinWRviSfyqFVLHPv76468998;     BWfHinWRviSfyqFVLHPv76468998 = BWfHinWRviSfyqFVLHPv90780623;     BWfHinWRviSfyqFVLHPv90780623 = BWfHinWRviSfyqFVLHPv70587408;     BWfHinWRviSfyqFVLHPv70587408 = BWfHinWRviSfyqFVLHPv17460910;     BWfHinWRviSfyqFVLHPv17460910 = BWfHinWRviSfyqFVLHPv50327409;     BWfHinWRviSfyqFVLHPv50327409 = BWfHinWRviSfyqFVLHPv55711785;     BWfHinWRviSfyqFVLHPv55711785 = BWfHinWRviSfyqFVLHPv91819602;     BWfHinWRviSfyqFVLHPv91819602 = BWfHinWRviSfyqFVLHPv8283253;     BWfHinWRviSfyqFVLHPv8283253 = BWfHinWRviSfyqFVLHPv68820760;     BWfHinWRviSfyqFVLHPv68820760 = BWfHinWRviSfyqFVLHPv52957000;     BWfHinWRviSfyqFVLHPv52957000 = BWfHinWRviSfyqFVLHPv76607709;     BWfHinWRviSfyqFVLHPv76607709 = BWfHinWRviSfyqFVLHPv37329737;     BWfHinWRviSfyqFVLHPv37329737 = BWfHinWRviSfyqFVLHPv86656507;     BWfHinWRviSfyqFVLHPv86656507 = BWfHinWRviSfyqFVLHPv45647014;     BWfHinWRviSfyqFVLHPv45647014 = BWfHinWRviSfyqFVLHPv1651969;     BWfHinWRviSfyqFVLHPv1651969 = BWfHinWRviSfyqFVLHPv34726740;     BWfHinWRviSfyqFVLHPv34726740 = BWfHinWRviSfyqFVLHPv49224264;     BWfHinWRviSfyqFVLHPv49224264 = BWfHinWRviSfyqFVLHPv53547957;     BWfHinWRviSfyqFVLHPv53547957 = BWfHinWRviSfyqFVLHPv48235967;     BWfHinWRviSfyqFVLHPv48235967 = BWfHinWRviSfyqFVLHPv13953749;     BWfHinWRviSfyqFVLHPv13953749 = BWfHinWRviSfyqFVLHPv49142891;     BWfHinWRviSfyqFVLHPv49142891 = BWfHinWRviSfyqFVLHPv98422948;     BWfHinWRviSfyqFVLHPv98422948 = BWfHinWRviSfyqFVLHPv30921919;}
// Junk Finished

// Junk Code By Peatreat & Thaisen's Gen
void GcZXbCQbGwNvmWWRPuRv48753505() {     int lkkViAJMyKKzDGJrqqqf63182728 = -86281225;    int lkkViAJMyKKzDGJrqqqf93537747 = -357163782;    int lkkViAJMyKKzDGJrqqqf11187177 = -939555649;    int lkkViAJMyKKzDGJrqqqf48346023 = -670687355;    int lkkViAJMyKKzDGJrqqqf30105151 = -81531735;    int lkkViAJMyKKzDGJrqqqf29111055 = -985815497;    int lkkViAJMyKKzDGJrqqqf60764228 = -497991971;    int lkkViAJMyKKzDGJrqqqf29304856 = -220938352;    int lkkViAJMyKKzDGJrqqqf89198168 = -180741832;    int lkkViAJMyKKzDGJrqqqf72656697 = -673679456;    int lkkViAJMyKKzDGJrqqqf27031809 = -633096067;    int lkkViAJMyKKzDGJrqqqf5743320 = -561786646;    int lkkViAJMyKKzDGJrqqqf37400253 = 85891256;    int lkkViAJMyKKzDGJrqqqf71843223 = -576556535;    int lkkViAJMyKKzDGJrqqqf5545183 = -35302183;    int lkkViAJMyKKzDGJrqqqf22944545 = -855488806;    int lkkViAJMyKKzDGJrqqqf27069080 = -352293063;    int lkkViAJMyKKzDGJrqqqf44086918 = 73387414;    int lkkViAJMyKKzDGJrqqqf72320226 = -760105880;    int lkkViAJMyKKzDGJrqqqf49645237 = 74580910;    int lkkViAJMyKKzDGJrqqqf27644423 = -944417111;    int lkkViAJMyKKzDGJrqqqf36512073 = -718511270;    int lkkViAJMyKKzDGJrqqqf70833203 = -793872191;    int lkkViAJMyKKzDGJrqqqf93440950 = -754680436;    int lkkViAJMyKKzDGJrqqqf18076769 = 44328556;    int lkkViAJMyKKzDGJrqqqf34180018 = -374234382;    int lkkViAJMyKKzDGJrqqqf27602894 = -552692013;    int lkkViAJMyKKzDGJrqqqf35354546 = -621940391;    int lkkViAJMyKKzDGJrqqqf98132174 = -571377903;    int lkkViAJMyKKzDGJrqqqf93705603 = -818206332;    int lkkViAJMyKKzDGJrqqqf55935018 = -49430421;    int lkkViAJMyKKzDGJrqqqf23845730 = -498268044;    int lkkViAJMyKKzDGJrqqqf62506288 = -872153498;    int lkkViAJMyKKzDGJrqqqf69885222 = -474169144;    int lkkViAJMyKKzDGJrqqqf38106050 = -904200445;    int lkkViAJMyKKzDGJrqqqf66655384 = -452357579;    int lkkViAJMyKKzDGJrqqqf41901493 = -88365775;    int lkkViAJMyKKzDGJrqqqf8336986 = -597513079;    int lkkViAJMyKKzDGJrqqqf52821497 = -448956180;    int lkkViAJMyKKzDGJrqqqf17431372 = -203660408;    int lkkViAJMyKKzDGJrqqqf79264111 = -192650480;    int lkkViAJMyKKzDGJrqqqf36200844 = -699597675;    int lkkViAJMyKKzDGJrqqqf43196321 = -525139553;    int lkkViAJMyKKzDGJrqqqf87966235 = -789783015;    int lkkViAJMyKKzDGJrqqqf74412112 = -141114594;    int lkkViAJMyKKzDGJrqqqf26302375 = -954754121;    int lkkViAJMyKKzDGJrqqqf25954687 = -720183313;    int lkkViAJMyKKzDGJrqqqf39232114 = -564698420;    int lkkViAJMyKKzDGJrqqqf68827312 = -238837804;    int lkkViAJMyKKzDGJrqqqf64540166 = -969355882;    int lkkViAJMyKKzDGJrqqqf20021595 = 44691778;    int lkkViAJMyKKzDGJrqqqf36902514 = -918077630;    int lkkViAJMyKKzDGJrqqqf25712642 = -715152807;    int lkkViAJMyKKzDGJrqqqf44859141 = -190890777;    int lkkViAJMyKKzDGJrqqqf21109083 = -272096520;    int lkkViAJMyKKzDGJrqqqf26670655 = -367769956;    int lkkViAJMyKKzDGJrqqqf22704545 = -563291592;    int lkkViAJMyKKzDGJrqqqf17746226 = -84875214;    int lkkViAJMyKKzDGJrqqqf30269254 = -615015911;    int lkkViAJMyKKzDGJrqqqf95925132 = -707297353;    int lkkViAJMyKKzDGJrqqqf1508161 = -333123485;    int lkkViAJMyKKzDGJrqqqf25409682 = -876051580;    int lkkViAJMyKKzDGJrqqqf31172682 = -649560450;    int lkkViAJMyKKzDGJrqqqf95492565 = -362535501;    int lkkViAJMyKKzDGJrqqqf16721679 = -524249035;    int lkkViAJMyKKzDGJrqqqf3186079 = -34828023;    int lkkViAJMyKKzDGJrqqqf43237032 = -689633148;    int lkkViAJMyKKzDGJrqqqf67515031 = -439939601;    int lkkViAJMyKKzDGJrqqqf33737173 = -672356090;    int lkkViAJMyKKzDGJrqqqf38889799 = -582944604;    int lkkViAJMyKKzDGJrqqqf81043051 = -667123032;    int lkkViAJMyKKzDGJrqqqf18732094 = -754779984;    int lkkViAJMyKKzDGJrqqqf91265420 = -477656407;    int lkkViAJMyKKzDGJrqqqf54888855 = -456445473;    int lkkViAJMyKKzDGJrqqqf70381126 = -732768610;    int lkkViAJMyKKzDGJrqqqf91443578 = -144819437;    int lkkViAJMyKKzDGJrqqqf93315751 = -93371718;    int lkkViAJMyKKzDGJrqqqf82866967 = 95910823;    int lkkViAJMyKKzDGJrqqqf19028839 = -513565843;    int lkkViAJMyKKzDGJrqqqf91774393 = -917323;    int lkkViAJMyKKzDGJrqqqf8225331 = -654051070;    int lkkViAJMyKKzDGJrqqqf88370779 = -987993593;    int lkkViAJMyKKzDGJrqqqf66527233 = -283102588;    int lkkViAJMyKKzDGJrqqqf33592008 = -602022021;    int lkkViAJMyKKzDGJrqqqf73684008 = -762898110;    int lkkViAJMyKKzDGJrqqqf19032505 = -131352792;    int lkkViAJMyKKzDGJrqqqf98133088 = -783115238;    int lkkViAJMyKKzDGJrqqqf17647147 = -581262721;    int lkkViAJMyKKzDGJrqqqf48776139 = -102072624;    int lkkViAJMyKKzDGJrqqqf11435395 = -436430490;    int lkkViAJMyKKzDGJrqqqf43950839 = -889065988;    int lkkViAJMyKKzDGJrqqqf24155268 = 96509438;    int lkkViAJMyKKzDGJrqqqf78067731 = -982497169;    int lkkViAJMyKKzDGJrqqqf56896365 = -741658827;    int lkkViAJMyKKzDGJrqqqf15923211 = -870536923;    int lkkViAJMyKKzDGJrqqqf53854429 = -316598900;    int lkkViAJMyKKzDGJrqqqf5028163 = 49962774;    int lkkViAJMyKKzDGJrqqqf47703756 = -62604052;    int lkkViAJMyKKzDGJrqqqf71244557 = -165533981;    int lkkViAJMyKKzDGJrqqqf71226033 = -86281225;     lkkViAJMyKKzDGJrqqqf63182728 = lkkViAJMyKKzDGJrqqqf93537747;     lkkViAJMyKKzDGJrqqqf93537747 = lkkViAJMyKKzDGJrqqqf11187177;     lkkViAJMyKKzDGJrqqqf11187177 = lkkViAJMyKKzDGJrqqqf48346023;     lkkViAJMyKKzDGJrqqqf48346023 = lkkViAJMyKKzDGJrqqqf30105151;     lkkViAJMyKKzDGJrqqqf30105151 = lkkViAJMyKKzDGJrqqqf29111055;     lkkViAJMyKKzDGJrqqqf29111055 = lkkViAJMyKKzDGJrqqqf60764228;     lkkViAJMyKKzDGJrqqqf60764228 = lkkViAJMyKKzDGJrqqqf29304856;     lkkViAJMyKKzDGJrqqqf29304856 = lkkViAJMyKKzDGJrqqqf89198168;     lkkViAJMyKKzDGJrqqqf89198168 = lkkViAJMyKKzDGJrqqqf72656697;     lkkViAJMyKKzDGJrqqqf72656697 = lkkViAJMyKKzDGJrqqqf27031809;     lkkViAJMyKKzDGJrqqqf27031809 = lkkViAJMyKKzDGJrqqqf5743320;     lkkViAJMyKKzDGJrqqqf5743320 = lkkViAJMyKKzDGJrqqqf37400253;     lkkViAJMyKKzDGJrqqqf37400253 = lkkViAJMyKKzDGJrqqqf71843223;     lkkViAJMyKKzDGJrqqqf71843223 = lkkViAJMyKKzDGJrqqqf5545183;     lkkViAJMyKKzDGJrqqqf5545183 = lkkViAJMyKKzDGJrqqqf22944545;     lkkViAJMyKKzDGJrqqqf22944545 = lkkViAJMyKKzDGJrqqqf27069080;     lkkViAJMyKKzDGJrqqqf27069080 = lkkViAJMyKKzDGJrqqqf44086918;     lkkViAJMyKKzDGJrqqqf44086918 = lkkViAJMyKKzDGJrqqqf72320226;     lkkViAJMyKKzDGJrqqqf72320226 = lkkViAJMyKKzDGJrqqqf49645237;     lkkViAJMyKKzDGJrqqqf49645237 = lkkViAJMyKKzDGJrqqqf27644423;     lkkViAJMyKKzDGJrqqqf27644423 = lkkViAJMyKKzDGJrqqqf36512073;     lkkViAJMyKKzDGJrqqqf36512073 = lkkViAJMyKKzDGJrqqqf70833203;     lkkViAJMyKKzDGJrqqqf70833203 = lkkViAJMyKKzDGJrqqqf93440950;     lkkViAJMyKKzDGJrqqqf93440950 = lkkViAJMyKKzDGJrqqqf18076769;     lkkViAJMyKKzDGJrqqqf18076769 = lkkViAJMyKKzDGJrqqqf34180018;     lkkViAJMyKKzDGJrqqqf34180018 = lkkViAJMyKKzDGJrqqqf27602894;     lkkViAJMyKKzDGJrqqqf27602894 = lkkViAJMyKKzDGJrqqqf35354546;     lkkViAJMyKKzDGJrqqqf35354546 = lkkViAJMyKKzDGJrqqqf98132174;     lkkViAJMyKKzDGJrqqqf98132174 = lkkViAJMyKKzDGJrqqqf93705603;     lkkViAJMyKKzDGJrqqqf93705603 = lkkViAJMyKKzDGJrqqqf55935018;     lkkViAJMyKKzDGJrqqqf55935018 = lkkViAJMyKKzDGJrqqqf23845730;     lkkViAJMyKKzDGJrqqqf23845730 = lkkViAJMyKKzDGJrqqqf62506288;     lkkViAJMyKKzDGJrqqqf62506288 = lkkViAJMyKKzDGJrqqqf69885222;     lkkViAJMyKKzDGJrqqqf69885222 = lkkViAJMyKKzDGJrqqqf38106050;     lkkViAJMyKKzDGJrqqqf38106050 = lkkViAJMyKKzDGJrqqqf66655384;     lkkViAJMyKKzDGJrqqqf66655384 = lkkViAJMyKKzDGJrqqqf41901493;     lkkViAJMyKKzDGJrqqqf41901493 = lkkViAJMyKKzDGJrqqqf8336986;     lkkViAJMyKKzDGJrqqqf8336986 = lkkViAJMyKKzDGJrqqqf52821497;     lkkViAJMyKKzDGJrqqqf52821497 = lkkViAJMyKKzDGJrqqqf17431372;     lkkViAJMyKKzDGJrqqqf17431372 = lkkViAJMyKKzDGJrqqqf79264111;     lkkViAJMyKKzDGJrqqqf79264111 = lkkViAJMyKKzDGJrqqqf36200844;     lkkViAJMyKKzDGJrqqqf36200844 = lkkViAJMyKKzDGJrqqqf43196321;     lkkViAJMyKKzDGJrqqqf43196321 = lkkViAJMyKKzDGJrqqqf87966235;     lkkViAJMyKKzDGJrqqqf87966235 = lkkViAJMyKKzDGJrqqqf74412112;     lkkViAJMyKKzDGJrqqqf74412112 = lkkViAJMyKKzDGJrqqqf26302375;     lkkViAJMyKKzDGJrqqqf26302375 = lkkViAJMyKKzDGJrqqqf25954687;     lkkViAJMyKKzDGJrqqqf25954687 = lkkViAJMyKKzDGJrqqqf39232114;     lkkViAJMyKKzDGJrqqqf39232114 = lkkViAJMyKKzDGJrqqqf68827312;     lkkViAJMyKKzDGJrqqqf68827312 = lkkViAJMyKKzDGJrqqqf64540166;     lkkViAJMyKKzDGJrqqqf64540166 = lkkViAJMyKKzDGJrqqqf20021595;     lkkViAJMyKKzDGJrqqqf20021595 = lkkViAJMyKKzDGJrqqqf36902514;     lkkViAJMyKKzDGJrqqqf36902514 = lkkViAJMyKKzDGJrqqqf25712642;     lkkViAJMyKKzDGJrqqqf25712642 = lkkViAJMyKKzDGJrqqqf44859141;     lkkViAJMyKKzDGJrqqqf44859141 = lkkViAJMyKKzDGJrqqqf21109083;     lkkViAJMyKKzDGJrqqqf21109083 = lkkViAJMyKKzDGJrqqqf26670655;     lkkViAJMyKKzDGJrqqqf26670655 = lkkViAJMyKKzDGJrqqqf22704545;     lkkViAJMyKKzDGJrqqqf22704545 = lkkViAJMyKKzDGJrqqqf17746226;     lkkViAJMyKKzDGJrqqqf17746226 = lkkViAJMyKKzDGJrqqqf30269254;     lkkViAJMyKKzDGJrqqqf30269254 = lkkViAJMyKKzDGJrqqqf95925132;     lkkViAJMyKKzDGJrqqqf95925132 = lkkViAJMyKKzDGJrqqqf1508161;     lkkViAJMyKKzDGJrqqqf1508161 = lkkViAJMyKKzDGJrqqqf25409682;     lkkViAJMyKKzDGJrqqqf25409682 = lkkViAJMyKKzDGJrqqqf31172682;     lkkViAJMyKKzDGJrqqqf31172682 = lkkViAJMyKKzDGJrqqqf95492565;     lkkViAJMyKKzDGJrqqqf95492565 = lkkViAJMyKKzDGJrqqqf16721679;     lkkViAJMyKKzDGJrqqqf16721679 = lkkViAJMyKKzDGJrqqqf3186079;     lkkViAJMyKKzDGJrqqqf3186079 = lkkViAJMyKKzDGJrqqqf43237032;     lkkViAJMyKKzDGJrqqqf43237032 = lkkViAJMyKKzDGJrqqqf67515031;     lkkViAJMyKKzDGJrqqqf67515031 = lkkViAJMyKKzDGJrqqqf33737173;     lkkViAJMyKKzDGJrqqqf33737173 = lkkViAJMyKKzDGJrqqqf38889799;     lkkViAJMyKKzDGJrqqqf38889799 = lkkViAJMyKKzDGJrqqqf81043051;     lkkViAJMyKKzDGJrqqqf81043051 = lkkViAJMyKKzDGJrqqqf18732094;     lkkViAJMyKKzDGJrqqqf18732094 = lkkViAJMyKKzDGJrqqqf91265420;     lkkViAJMyKKzDGJrqqqf91265420 = lkkViAJMyKKzDGJrqqqf54888855;     lkkViAJMyKKzDGJrqqqf54888855 = lkkViAJMyKKzDGJrqqqf70381126;     lkkViAJMyKKzDGJrqqqf70381126 = lkkViAJMyKKzDGJrqqqf91443578;     lkkViAJMyKKzDGJrqqqf91443578 = lkkViAJMyKKzDGJrqqqf93315751;     lkkViAJMyKKzDGJrqqqf93315751 = lkkViAJMyKKzDGJrqqqf82866967;     lkkViAJMyKKzDGJrqqqf82866967 = lkkViAJMyKKzDGJrqqqf19028839;     lkkViAJMyKKzDGJrqqqf19028839 = lkkViAJMyKKzDGJrqqqf91774393;     lkkViAJMyKKzDGJrqqqf91774393 = lkkViAJMyKKzDGJrqqqf8225331;     lkkViAJMyKKzDGJrqqqf8225331 = lkkViAJMyKKzDGJrqqqf88370779;     lkkViAJMyKKzDGJrqqqf88370779 = lkkViAJMyKKzDGJrqqqf66527233;     lkkViAJMyKKzDGJrqqqf66527233 = lkkViAJMyKKzDGJrqqqf33592008;     lkkViAJMyKKzDGJrqqqf33592008 = lkkViAJMyKKzDGJrqqqf73684008;     lkkViAJMyKKzDGJrqqqf73684008 = lkkViAJMyKKzDGJrqqqf19032505;     lkkViAJMyKKzDGJrqqqf19032505 = lkkViAJMyKKzDGJrqqqf98133088;     lkkViAJMyKKzDGJrqqqf98133088 = lkkViAJMyKKzDGJrqqqf17647147;     lkkViAJMyKKzDGJrqqqf17647147 = lkkViAJMyKKzDGJrqqqf48776139;     lkkViAJMyKKzDGJrqqqf48776139 = lkkViAJMyKKzDGJrqqqf11435395;     lkkViAJMyKKzDGJrqqqf11435395 = lkkViAJMyKKzDGJrqqqf43950839;     lkkViAJMyKKzDGJrqqqf43950839 = lkkViAJMyKKzDGJrqqqf24155268;     lkkViAJMyKKzDGJrqqqf24155268 = lkkViAJMyKKzDGJrqqqf78067731;     lkkViAJMyKKzDGJrqqqf78067731 = lkkViAJMyKKzDGJrqqqf56896365;     lkkViAJMyKKzDGJrqqqf56896365 = lkkViAJMyKKzDGJrqqqf15923211;     lkkViAJMyKKzDGJrqqqf15923211 = lkkViAJMyKKzDGJrqqqf53854429;     lkkViAJMyKKzDGJrqqqf53854429 = lkkViAJMyKKzDGJrqqqf5028163;     lkkViAJMyKKzDGJrqqqf5028163 = lkkViAJMyKKzDGJrqqqf47703756;     lkkViAJMyKKzDGJrqqqf47703756 = lkkViAJMyKKzDGJrqqqf71244557;     lkkViAJMyKKzDGJrqqqf71244557 = lkkViAJMyKKzDGJrqqqf71226033;     lkkViAJMyKKzDGJrqqqf71226033 = lkkViAJMyKKzDGJrqqqf63182728;}
// Junk Finished

// Junk Code By Peatreat & Thaisen's Gen
void HGscawuzJmrurGrufYXR72853291() {     int XmdUsRiMIWhXPwkNUoqt66101675 = -407553976;    int XmdUsRiMIWhXPwkNUoqt24861384 = -859828841;    int XmdUsRiMIWhXPwkNUoqt23119463 = -427580878;    int XmdUsRiMIWhXPwkNUoqt44705562 = -860520624;    int XmdUsRiMIWhXPwkNUoqt27832889 = -46316320;    int XmdUsRiMIWhXPwkNUoqt18402098 = -955939107;    int XmdUsRiMIWhXPwkNUoqt8088089 = -976673047;    int XmdUsRiMIWhXPwkNUoqt39688481 = -597727030;    int XmdUsRiMIWhXPwkNUoqt25674502 = -436746492;    int XmdUsRiMIWhXPwkNUoqt86491558 = -410834743;    int XmdUsRiMIWhXPwkNUoqt10350405 = -196730167;    int XmdUsRiMIWhXPwkNUoqt58151655 = -813229235;    int XmdUsRiMIWhXPwkNUoqt14036774 = -108468787;    int XmdUsRiMIWhXPwkNUoqt6041935 = -561979566;    int XmdUsRiMIWhXPwkNUoqt81753998 = -464696104;    int XmdUsRiMIWhXPwkNUoqt984808 = -773964415;    int XmdUsRiMIWhXPwkNUoqt70930318 = -457798202;    int XmdUsRiMIWhXPwkNUoqt26000583 = -627600559;    int XmdUsRiMIWhXPwkNUoqt13966979 = -518003244;    int XmdUsRiMIWhXPwkNUoqt47550218 = -560517087;    int XmdUsRiMIWhXPwkNUoqt16071458 = -329692094;    int XmdUsRiMIWhXPwkNUoqt85345208 = -711474370;    int XmdUsRiMIWhXPwkNUoqt5953520 = -672378165;    int XmdUsRiMIWhXPwkNUoqt2219070 = -582129301;    int XmdUsRiMIWhXPwkNUoqt42413526 = -382930460;    int XmdUsRiMIWhXPwkNUoqt17715551 = -32333388;    int XmdUsRiMIWhXPwkNUoqt90764183 = -645127574;    int XmdUsRiMIWhXPwkNUoqt90474321 = -246661566;    int XmdUsRiMIWhXPwkNUoqt63279778 = -901723876;    int XmdUsRiMIWhXPwkNUoqt18155929 = -647063504;    int XmdUsRiMIWhXPwkNUoqt70421738 = -265420481;    int XmdUsRiMIWhXPwkNUoqt41807611 = -585990381;    int XmdUsRiMIWhXPwkNUoqt32068855 = -846289226;    int XmdUsRiMIWhXPwkNUoqt70517402 = -493692787;    int XmdUsRiMIWhXPwkNUoqt74405078 = -905254062;    int XmdUsRiMIWhXPwkNUoqt72282316 = -464184347;    int XmdUsRiMIWhXPwkNUoqt66957755 = -394437652;    int XmdUsRiMIWhXPwkNUoqt20480613 = -387929736;    int XmdUsRiMIWhXPwkNUoqt33816304 = -385587386;    int XmdUsRiMIWhXPwkNUoqt16335350 = -343353340;    int XmdUsRiMIWhXPwkNUoqt66539425 = -186041850;    int XmdUsRiMIWhXPwkNUoqt12850103 = -218610140;    int XmdUsRiMIWhXPwkNUoqt77613272 = -464868339;    int XmdUsRiMIWhXPwkNUoqt4024817 = -338074003;    int XmdUsRiMIWhXPwkNUoqt62366967 = -773135108;    int XmdUsRiMIWhXPwkNUoqt79941529 = -599651796;    int XmdUsRiMIWhXPwkNUoqt56262204 = -284900243;    int XmdUsRiMIWhXPwkNUoqt83674958 = -137946097;    int XmdUsRiMIWhXPwkNUoqt22151627 = -174615264;    int XmdUsRiMIWhXPwkNUoqt20725570 = -250363796;    int XmdUsRiMIWhXPwkNUoqt54455242 = 95980432;    int XmdUsRiMIWhXPwkNUoqt94304911 = -971544053;    int XmdUsRiMIWhXPwkNUoqt34201899 = -710955076;    int XmdUsRiMIWhXPwkNUoqt32215658 = -471977950;    int XmdUsRiMIWhXPwkNUoqt56781833 = -638242001;    int XmdUsRiMIWhXPwkNUoqt80756466 = -696079607;    int XmdUsRiMIWhXPwkNUoqt18907864 = -87450677;    int XmdUsRiMIWhXPwkNUoqt20900394 = -845451577;    int XmdUsRiMIWhXPwkNUoqt2292037 = -377590165;    int XmdUsRiMIWhXPwkNUoqt10117338 = 86017067;    int XmdUsRiMIWhXPwkNUoqt27637914 = -210811533;    int XmdUsRiMIWhXPwkNUoqt17613768 = -630011482;    int XmdUsRiMIWhXPwkNUoqt76408703 = -696003155;    int XmdUsRiMIWhXPwkNUoqt7518574 = -789682989;    int XmdUsRiMIWhXPwkNUoqt16069820 = -45414262;    int XmdUsRiMIWhXPwkNUoqt68542794 = -610739787;    int XmdUsRiMIWhXPwkNUoqt26082800 = -966940009;    int XmdUsRiMIWhXPwkNUoqt43519372 = -614776001;    int XmdUsRiMIWhXPwkNUoqt31636857 = -656725505;    int XmdUsRiMIWhXPwkNUoqt9471683 = 99488242;    int XmdUsRiMIWhXPwkNUoqt34027053 = -279526764;    int XmdUsRiMIWhXPwkNUoqt50449706 = 30131533;    int XmdUsRiMIWhXPwkNUoqt92184278 = -142013173;    int XmdUsRiMIWhXPwkNUoqt97631628 = -74649905;    int XmdUsRiMIWhXPwkNUoqt81010793 = -274475237;    int XmdUsRiMIWhXPwkNUoqt3221356 = -11081955;    int XmdUsRiMIWhXPwkNUoqt7731937 = -146606032;    int XmdUsRiMIWhXPwkNUoqt1928704 = -234304162;    int XmdUsRiMIWhXPwkNUoqt39852102 = -808994194;    int XmdUsRiMIWhXPwkNUoqt62471997 = -783278664;    int XmdUsRiMIWhXPwkNUoqt61453346 = -747433145;    int XmdUsRiMIWhXPwkNUoqt7089225 = -407181478;    int XmdUsRiMIWhXPwkNUoqt68322695 = 27953697;    int XmdUsRiMIWhXPwkNUoqt42554209 = -551360081;    int XmdUsRiMIWhXPwkNUoqt63700687 = -643043936;    int XmdUsRiMIWhXPwkNUoqt76116827 = -293876428;    int XmdUsRiMIWhXPwkNUoqt7605712 = -875035305;    int XmdUsRiMIWhXPwkNUoqt99853197 = -274311277;    int XmdUsRiMIWhXPwkNUoqt13735569 = -855450787;    int XmdUsRiMIWhXPwkNUoqt93648611 = -109174455;    int XmdUsRiMIWhXPwkNUoqt53374452 = -276733671;    int XmdUsRiMIWhXPwkNUoqt46057361 = -548986075;    int XmdUsRiMIWhXPwkNUoqt18188577 = 89660428;    int XmdUsRiMIWhXPwkNUoqt23698967 = -371604454;    int XmdUsRiMIWhXPwkNUoqt88697435 = -32541808;    int XmdUsRiMIWhXPwkNUoqt48925657 = -556030369;    int XmdUsRiMIWhXPwkNUoqt36441399 = -522606986;    int XmdUsRiMIWhXPwkNUoqt70094698 = -675185350;    int XmdUsRiMIWhXPwkNUoqt87954996 = -192659742;    int XmdUsRiMIWhXPwkNUoqt93824173 = -407553976;     XmdUsRiMIWhXPwkNUoqt66101675 = XmdUsRiMIWhXPwkNUoqt24861384;     XmdUsRiMIWhXPwkNUoqt24861384 = XmdUsRiMIWhXPwkNUoqt23119463;     XmdUsRiMIWhXPwkNUoqt23119463 = XmdUsRiMIWhXPwkNUoqt44705562;     XmdUsRiMIWhXPwkNUoqt44705562 = XmdUsRiMIWhXPwkNUoqt27832889;     XmdUsRiMIWhXPwkNUoqt27832889 = XmdUsRiMIWhXPwkNUoqt18402098;     XmdUsRiMIWhXPwkNUoqt18402098 = XmdUsRiMIWhXPwkNUoqt8088089;     XmdUsRiMIWhXPwkNUoqt8088089 = XmdUsRiMIWhXPwkNUoqt39688481;     XmdUsRiMIWhXPwkNUoqt39688481 = XmdUsRiMIWhXPwkNUoqt25674502;     XmdUsRiMIWhXPwkNUoqt25674502 = XmdUsRiMIWhXPwkNUoqt86491558;     XmdUsRiMIWhXPwkNUoqt86491558 = XmdUsRiMIWhXPwkNUoqt10350405;     XmdUsRiMIWhXPwkNUoqt10350405 = XmdUsRiMIWhXPwkNUoqt58151655;     XmdUsRiMIWhXPwkNUoqt58151655 = XmdUsRiMIWhXPwkNUoqt14036774;     XmdUsRiMIWhXPwkNUoqt14036774 = XmdUsRiMIWhXPwkNUoqt6041935;     XmdUsRiMIWhXPwkNUoqt6041935 = XmdUsRiMIWhXPwkNUoqt81753998;     XmdUsRiMIWhXPwkNUoqt81753998 = XmdUsRiMIWhXPwkNUoqt984808;     XmdUsRiMIWhXPwkNUoqt984808 = XmdUsRiMIWhXPwkNUoqt70930318;     XmdUsRiMIWhXPwkNUoqt70930318 = XmdUsRiMIWhXPwkNUoqt26000583;     XmdUsRiMIWhXPwkNUoqt26000583 = XmdUsRiMIWhXPwkNUoqt13966979;     XmdUsRiMIWhXPwkNUoqt13966979 = XmdUsRiMIWhXPwkNUoqt47550218;     XmdUsRiMIWhXPwkNUoqt47550218 = XmdUsRiMIWhXPwkNUoqt16071458;     XmdUsRiMIWhXPwkNUoqt16071458 = XmdUsRiMIWhXPwkNUoqt85345208;     XmdUsRiMIWhXPwkNUoqt85345208 = XmdUsRiMIWhXPwkNUoqt5953520;     XmdUsRiMIWhXPwkNUoqt5953520 = XmdUsRiMIWhXPwkNUoqt2219070;     XmdUsRiMIWhXPwkNUoqt2219070 = XmdUsRiMIWhXPwkNUoqt42413526;     XmdUsRiMIWhXPwkNUoqt42413526 = XmdUsRiMIWhXPwkNUoqt17715551;     XmdUsRiMIWhXPwkNUoqt17715551 = XmdUsRiMIWhXPwkNUoqt90764183;     XmdUsRiMIWhXPwkNUoqt90764183 = XmdUsRiMIWhXPwkNUoqt90474321;     XmdUsRiMIWhXPwkNUoqt90474321 = XmdUsRiMIWhXPwkNUoqt63279778;     XmdUsRiMIWhXPwkNUoqt63279778 = XmdUsRiMIWhXPwkNUoqt18155929;     XmdUsRiMIWhXPwkNUoqt18155929 = XmdUsRiMIWhXPwkNUoqt70421738;     XmdUsRiMIWhXPwkNUoqt70421738 = XmdUsRiMIWhXPwkNUoqt41807611;     XmdUsRiMIWhXPwkNUoqt41807611 = XmdUsRiMIWhXPwkNUoqt32068855;     XmdUsRiMIWhXPwkNUoqt32068855 = XmdUsRiMIWhXPwkNUoqt70517402;     XmdUsRiMIWhXPwkNUoqt70517402 = XmdUsRiMIWhXPwkNUoqt74405078;     XmdUsRiMIWhXPwkNUoqt74405078 = XmdUsRiMIWhXPwkNUoqt72282316;     XmdUsRiMIWhXPwkNUoqt72282316 = XmdUsRiMIWhXPwkNUoqt66957755;     XmdUsRiMIWhXPwkNUoqt66957755 = XmdUsRiMIWhXPwkNUoqt20480613;     XmdUsRiMIWhXPwkNUoqt20480613 = XmdUsRiMIWhXPwkNUoqt33816304;     XmdUsRiMIWhXPwkNUoqt33816304 = XmdUsRiMIWhXPwkNUoqt16335350;     XmdUsRiMIWhXPwkNUoqt16335350 = XmdUsRiMIWhXPwkNUoqt66539425;     XmdUsRiMIWhXPwkNUoqt66539425 = XmdUsRiMIWhXPwkNUoqt12850103;     XmdUsRiMIWhXPwkNUoqt12850103 = XmdUsRiMIWhXPwkNUoqt77613272;     XmdUsRiMIWhXPwkNUoqt77613272 = XmdUsRiMIWhXPwkNUoqt4024817;     XmdUsRiMIWhXPwkNUoqt4024817 = XmdUsRiMIWhXPwkNUoqt62366967;     XmdUsRiMIWhXPwkNUoqt62366967 = XmdUsRiMIWhXPwkNUoqt79941529;     XmdUsRiMIWhXPwkNUoqt79941529 = XmdUsRiMIWhXPwkNUoqt56262204;     XmdUsRiMIWhXPwkNUoqt56262204 = XmdUsRiMIWhXPwkNUoqt83674958;     XmdUsRiMIWhXPwkNUoqt83674958 = XmdUsRiMIWhXPwkNUoqt22151627;     XmdUsRiMIWhXPwkNUoqt22151627 = XmdUsRiMIWhXPwkNUoqt20725570;     XmdUsRiMIWhXPwkNUoqt20725570 = XmdUsRiMIWhXPwkNUoqt54455242;     XmdUsRiMIWhXPwkNUoqt54455242 = XmdUsRiMIWhXPwkNUoqt94304911;     XmdUsRiMIWhXPwkNUoqt94304911 = XmdUsRiMIWhXPwkNUoqt34201899;     XmdUsRiMIWhXPwkNUoqt34201899 = XmdUsRiMIWhXPwkNUoqt32215658;     XmdUsRiMIWhXPwkNUoqt32215658 = XmdUsRiMIWhXPwkNUoqt56781833;     XmdUsRiMIWhXPwkNUoqt56781833 = XmdUsRiMIWhXPwkNUoqt80756466;     XmdUsRiMIWhXPwkNUoqt80756466 = XmdUsRiMIWhXPwkNUoqt18907864;     XmdUsRiMIWhXPwkNUoqt18907864 = XmdUsRiMIWhXPwkNUoqt20900394;     XmdUsRiMIWhXPwkNUoqt20900394 = XmdUsRiMIWhXPwkNUoqt2292037;     XmdUsRiMIWhXPwkNUoqt2292037 = XmdUsRiMIWhXPwkNUoqt10117338;     XmdUsRiMIWhXPwkNUoqt10117338 = XmdUsRiMIWhXPwkNUoqt27637914;     XmdUsRiMIWhXPwkNUoqt27637914 = XmdUsRiMIWhXPwkNUoqt17613768;     XmdUsRiMIWhXPwkNUoqt17613768 = XmdUsRiMIWhXPwkNUoqt76408703;     XmdUsRiMIWhXPwkNUoqt76408703 = XmdUsRiMIWhXPwkNUoqt7518574;     XmdUsRiMIWhXPwkNUoqt7518574 = XmdUsRiMIWhXPwkNUoqt16069820;     XmdUsRiMIWhXPwkNUoqt16069820 = XmdUsRiMIWhXPwkNUoqt68542794;     XmdUsRiMIWhXPwkNUoqt68542794 = XmdUsRiMIWhXPwkNUoqt26082800;     XmdUsRiMIWhXPwkNUoqt26082800 = XmdUsRiMIWhXPwkNUoqt43519372;     XmdUsRiMIWhXPwkNUoqt43519372 = XmdUsRiMIWhXPwkNUoqt31636857;     XmdUsRiMIWhXPwkNUoqt31636857 = XmdUsRiMIWhXPwkNUoqt9471683;     XmdUsRiMIWhXPwkNUoqt9471683 = XmdUsRiMIWhXPwkNUoqt34027053;     XmdUsRiMIWhXPwkNUoqt34027053 = XmdUsRiMIWhXPwkNUoqt50449706;     XmdUsRiMIWhXPwkNUoqt50449706 = XmdUsRiMIWhXPwkNUoqt92184278;     XmdUsRiMIWhXPwkNUoqt92184278 = XmdUsRiMIWhXPwkNUoqt97631628;     XmdUsRiMIWhXPwkNUoqt97631628 = XmdUsRiMIWhXPwkNUoqt81010793;     XmdUsRiMIWhXPwkNUoqt81010793 = XmdUsRiMIWhXPwkNUoqt3221356;     XmdUsRiMIWhXPwkNUoqt3221356 = XmdUsRiMIWhXPwkNUoqt7731937;     XmdUsRiMIWhXPwkNUoqt7731937 = XmdUsRiMIWhXPwkNUoqt1928704;     XmdUsRiMIWhXPwkNUoqt1928704 = XmdUsRiMIWhXPwkNUoqt39852102;     XmdUsRiMIWhXPwkNUoqt39852102 = XmdUsRiMIWhXPwkNUoqt62471997;     XmdUsRiMIWhXPwkNUoqt62471997 = XmdUsRiMIWhXPwkNUoqt61453346;     XmdUsRiMIWhXPwkNUoqt61453346 = XmdUsRiMIWhXPwkNUoqt7089225;     XmdUsRiMIWhXPwkNUoqt7089225 = XmdUsRiMIWhXPwkNUoqt68322695;     XmdUsRiMIWhXPwkNUoqt68322695 = XmdUsRiMIWhXPwkNUoqt42554209;     XmdUsRiMIWhXPwkNUoqt42554209 = XmdUsRiMIWhXPwkNUoqt63700687;     XmdUsRiMIWhXPwkNUoqt63700687 = XmdUsRiMIWhXPwkNUoqt76116827;     XmdUsRiMIWhXPwkNUoqt76116827 = XmdUsRiMIWhXPwkNUoqt7605712;     XmdUsRiMIWhXPwkNUoqt7605712 = XmdUsRiMIWhXPwkNUoqt99853197;     XmdUsRiMIWhXPwkNUoqt99853197 = XmdUsRiMIWhXPwkNUoqt13735569;     XmdUsRiMIWhXPwkNUoqt13735569 = XmdUsRiMIWhXPwkNUoqt93648611;     XmdUsRiMIWhXPwkNUoqt93648611 = XmdUsRiMIWhXPwkNUoqt53374452;     XmdUsRiMIWhXPwkNUoqt53374452 = XmdUsRiMIWhXPwkNUoqt46057361;     XmdUsRiMIWhXPwkNUoqt46057361 = XmdUsRiMIWhXPwkNUoqt18188577;     XmdUsRiMIWhXPwkNUoqt18188577 = XmdUsRiMIWhXPwkNUoqt23698967;     XmdUsRiMIWhXPwkNUoqt23698967 = XmdUsRiMIWhXPwkNUoqt88697435;     XmdUsRiMIWhXPwkNUoqt88697435 = XmdUsRiMIWhXPwkNUoqt48925657;     XmdUsRiMIWhXPwkNUoqt48925657 = XmdUsRiMIWhXPwkNUoqt36441399;     XmdUsRiMIWhXPwkNUoqt36441399 = XmdUsRiMIWhXPwkNUoqt70094698;     XmdUsRiMIWhXPwkNUoqt70094698 = XmdUsRiMIWhXPwkNUoqt87954996;     XmdUsRiMIWhXPwkNUoqt87954996 = XmdUsRiMIWhXPwkNUoqt93824173;     XmdUsRiMIWhXPwkNUoqt93824173 = XmdUsRiMIWhXPwkNUoqt66101675;}
// Junk Finished

// Junk Code By Peatreat & Thaisen's Gen
void SwlMfaIefssXVdnhZqml49195608() {     int eMNukOBAtXfFIpRYMhMQ98362483 = 16798262;    int eMNukOBAtXfFIpRYMhMQ26083820 = -433445726;    int eMNukOBAtXfFIpRYMhMQ56396043 = -530440261;    int eMNukOBAtXfFIpRYMhMQ86213676 = -680756333;    int eMNukOBAtXfFIpRYMhMQ12950407 = -696049880;    int eMNukOBAtXfFIpRYMhMQ52498810 = -923914377;    int eMNukOBAtXfFIpRYMhMQ5580389 = -61114881;    int eMNukOBAtXfFIpRYMhMQ62443777 = -331875936;    int eMNukOBAtXfFIpRYMhMQ99025945 = -313049894;    int eMNukOBAtXfFIpRYMhMQ73999472 = -583701550;    int eMNukOBAtXfFIpRYMhMQ71740445 = -784934411;    int eMNukOBAtXfFIpRYMhMQ79305096 = -484386393;    int eMNukOBAtXfFIpRYMhMQ42698822 = -414950247;    int eMNukOBAtXfFIpRYMhMQ10237834 = 8472562;    int eMNukOBAtXfFIpRYMhMQ20414904 = -707052875;    int eMNukOBAtXfFIpRYMhMQ21060979 = -355252087;    int eMNukOBAtXfFIpRYMhMQ76791489 = -473732780;    int eMNukOBAtXfFIpRYMhMQ10621287 = -636493957;    int eMNukOBAtXfFIpRYMhMQ94334218 = -186694934;    int eMNukOBAtXfFIpRYMhMQ58742253 = -165273231;    int eMNukOBAtXfFIpRYMhMQ29032163 = -616754310;    int eMNukOBAtXfFIpRYMhMQ51078845 = -883144048;    int eMNukOBAtXfFIpRYMhMQ76008425 = -441998836;    int eMNukOBAtXfFIpRYMhMQ20797759 = -698097783;    int eMNukOBAtXfFIpRYMhMQ31068585 = -460978640;    int eMNukOBAtXfFIpRYMhMQ87227580 = -600353877;    int eMNukOBAtXfFIpRYMhMQ15886269 = -643185073;    int eMNukOBAtXfFIpRYMhMQ73215396 = 24774039;    int eMNukOBAtXfFIpRYMhMQ8963341 = -483781947;    int eMNukOBAtXfFIpRYMhMQ79524028 = -692956979;    int eMNukOBAtXfFIpRYMhMQ11249141 = -20070089;    int eMNukOBAtXfFIpRYMhMQ99981887 = -737237316;    int eMNukOBAtXfFIpRYMhMQ4138485 = 24592438;    int eMNukOBAtXfFIpRYMhMQ6067445 = -826368479;    int eMNukOBAtXfFIpRYMhMQ15037909 = -218674823;    int eMNukOBAtXfFIpRYMhMQ60744178 = -476550314;    int eMNukOBAtXfFIpRYMhMQ60163898 = -611569699;    int eMNukOBAtXfFIpRYMhMQ49749430 = -389495964;    int eMNukOBAtXfFIpRYMhMQ71591418 = 20828628;    int eMNukOBAtXfFIpRYMhMQ92008922 = -574869597;    int eMNukOBAtXfFIpRYMhMQ81597122 = -983358241;    int eMNukOBAtXfFIpRYMhMQ46714030 = -162025618;    int eMNukOBAtXfFIpRYMhMQ23346623 = -976973327;    int eMNukOBAtXfFIpRYMhMQ72806994 = -471354464;    int eMNukOBAtXfFIpRYMhMQ38385817 = -469768630;    int eMNukOBAtXfFIpRYMhMQ67602818 = -298256986;    int eMNukOBAtXfFIpRYMhMQ88136311 = -214827996;    int eMNukOBAtXfFIpRYMhMQ37887174 = -967824834;    int eMNukOBAtXfFIpRYMhMQ88692877 = -788392615;    int eMNukOBAtXfFIpRYMhMQ88528909 = 64612282;    int eMNukOBAtXfFIpRYMhMQ33958935 = -984713800;    int eMNukOBAtXfFIpRYMhMQ24383061 = -337936502;    int eMNukOBAtXfFIpRYMhMQ63063847 = -671013311;    int eMNukOBAtXfFIpRYMhMQ39595141 = -624103477;    int eMNukOBAtXfFIpRYMhMQ20163446 = -650345844;    int eMNukOBAtXfFIpRYMhMQ47283639 = -100057691;    int eMNukOBAtXfFIpRYMhMQ50075394 = -991446890;    int eMNukOBAtXfFIpRYMhMQ35598284 = -832342479;    int eMNukOBAtXfFIpRYMhMQ55145091 = -119777694;    int eMNukOBAtXfFIpRYMhMQ25722826 = 4303997;    int eMNukOBAtXfFIpRYMhMQ36612541 = -180729304;    int eMNukOBAtXfFIpRYMhMQ32364993 = 14111080;    int eMNukOBAtXfFIpRYMhMQ53480436 = -848093989;    int eMNukOBAtXfFIpRYMhMQ19501918 = -620092915;    int eMNukOBAtXfFIpRYMhMQ62750332 = -463631462;    int eMNukOBAtXfFIpRYMhMQ71758558 = 52302904;    int eMNukOBAtXfFIpRYMhMQ75166612 = -408978832;    int eMNukOBAtXfFIpRYMhMQ36631378 = -588581768;    int eMNukOBAtXfFIpRYMhMQ95199925 = -772852615;    int eMNukOBAtXfFIpRYMhMQ59670726 = -130502562;    int eMNukOBAtXfFIpRYMhMQ60897080 = -743682389;    int eMNukOBAtXfFIpRYMhMQ27042060 = 15763183;    int eMNukOBAtXfFIpRYMhMQ39029868 = -557322585;    int eMNukOBAtXfFIpRYMhMQ2325296 = -611825338;    int eMNukOBAtXfFIpRYMhMQ77145131 = -181914991;    int eMNukOBAtXfFIpRYMhMQ82318132 = -354728693;    int eMNukOBAtXfFIpRYMhMQ27732222 = -906170721;    int eMNukOBAtXfFIpRYMhMQ3201431 = -970644373;    int eMNukOBAtXfFIpRYMhMQ82411942 = -128329154;    int eMNukOBAtXfFIpRYMhMQ63465767 = -62721655;    int eMNukOBAtXfFIpRYMhMQ99091269 = -285525881;    int eMNukOBAtXfFIpRYMhMQ77999094 = -675360239;    int eMNukOBAtXfFIpRYMhMQ84522518 = -186833346;    int eMNukOBAtXfFIpRYMhMQ20434432 = -448394230;    int eMNukOBAtXfFIpRYMhMQ45565093 = -708243180;    int eMNukOBAtXfFIpRYMhMQ86866080 = -682133587;    int eMNukOBAtXfFIpRYMhMQ36918041 = 33775995;    int eMNukOBAtXfFIpRYMhMQ64543343 = -351304085;    int eMNukOBAtXfFIpRYMhMQ85903998 = -76022636;    int eMNukOBAtXfFIpRYMhMQ67754269 = -18617133;    int eMNukOBAtXfFIpRYMhMQ10668784 = -485103424;    int eMNukOBAtXfFIpRYMhMQ24565615 = -779227221;    int eMNukOBAtXfFIpRYMhMQ94604338 = -169718271;    int eMNukOBAtXfFIpRYMhMQ45868592 = -983475370;    int eMNukOBAtXfFIpRYMhMQ55396381 = -294140293;    int eMNukOBAtXfFIpRYMhMQ49232129 = -897469322;    int eMNukOBAtXfFIpRYMhMQ93233594 = -313931630;    int eMNukOBAtXfFIpRYMhMQ3844706 = -256880413;    int eMNukOBAtXfFIpRYMhMQ10056663 = 92276998;    int eMNukOBAtXfFIpRYMhMQ66627259 = 16798262;     eMNukOBAtXfFIpRYMhMQ98362483 = eMNukOBAtXfFIpRYMhMQ26083820;     eMNukOBAtXfFIpRYMhMQ26083820 = eMNukOBAtXfFIpRYMhMQ56396043;     eMNukOBAtXfFIpRYMhMQ56396043 = eMNukOBAtXfFIpRYMhMQ86213676;     eMNukOBAtXfFIpRYMhMQ86213676 = eMNukOBAtXfFIpRYMhMQ12950407;     eMNukOBAtXfFIpRYMhMQ12950407 = eMNukOBAtXfFIpRYMhMQ52498810;     eMNukOBAtXfFIpRYMhMQ52498810 = eMNukOBAtXfFIpRYMhMQ5580389;     eMNukOBAtXfFIpRYMhMQ5580389 = eMNukOBAtXfFIpRYMhMQ62443777;     eMNukOBAtXfFIpRYMhMQ62443777 = eMNukOBAtXfFIpRYMhMQ99025945;     eMNukOBAtXfFIpRYMhMQ99025945 = eMNukOBAtXfFIpRYMhMQ73999472;     eMNukOBAtXfFIpRYMhMQ73999472 = eMNukOBAtXfFIpRYMhMQ71740445;     eMNukOBAtXfFIpRYMhMQ71740445 = eMNukOBAtXfFIpRYMhMQ79305096;     eMNukOBAtXfFIpRYMhMQ79305096 = eMNukOBAtXfFIpRYMhMQ42698822;     eMNukOBAtXfFIpRYMhMQ42698822 = eMNukOBAtXfFIpRYMhMQ10237834;     eMNukOBAtXfFIpRYMhMQ10237834 = eMNukOBAtXfFIpRYMhMQ20414904;     eMNukOBAtXfFIpRYMhMQ20414904 = eMNukOBAtXfFIpRYMhMQ21060979;     eMNukOBAtXfFIpRYMhMQ21060979 = eMNukOBAtXfFIpRYMhMQ76791489;     eMNukOBAtXfFIpRYMhMQ76791489 = eMNukOBAtXfFIpRYMhMQ10621287;     eMNukOBAtXfFIpRYMhMQ10621287 = eMNukOBAtXfFIpRYMhMQ94334218;     eMNukOBAtXfFIpRYMhMQ94334218 = eMNukOBAtXfFIpRYMhMQ58742253;     eMNukOBAtXfFIpRYMhMQ58742253 = eMNukOBAtXfFIpRYMhMQ29032163;     eMNukOBAtXfFIpRYMhMQ29032163 = eMNukOBAtXfFIpRYMhMQ51078845;     eMNukOBAtXfFIpRYMhMQ51078845 = eMNukOBAtXfFIpRYMhMQ76008425;     eMNukOBAtXfFIpRYMhMQ76008425 = eMNukOBAtXfFIpRYMhMQ20797759;     eMNukOBAtXfFIpRYMhMQ20797759 = eMNukOBAtXfFIpRYMhMQ31068585;     eMNukOBAtXfFIpRYMhMQ31068585 = eMNukOBAtXfFIpRYMhMQ87227580;     eMNukOBAtXfFIpRYMhMQ87227580 = eMNukOBAtXfFIpRYMhMQ15886269;     eMNukOBAtXfFIpRYMhMQ15886269 = eMNukOBAtXfFIpRYMhMQ73215396;     eMNukOBAtXfFIpRYMhMQ73215396 = eMNukOBAtXfFIpRYMhMQ8963341;     eMNukOBAtXfFIpRYMhMQ8963341 = eMNukOBAtXfFIpRYMhMQ79524028;     eMNukOBAtXfFIpRYMhMQ79524028 = eMNukOBAtXfFIpRYMhMQ11249141;     eMNukOBAtXfFIpRYMhMQ11249141 = eMNukOBAtXfFIpRYMhMQ99981887;     eMNukOBAtXfFIpRYMhMQ99981887 = eMNukOBAtXfFIpRYMhMQ4138485;     eMNukOBAtXfFIpRYMhMQ4138485 = eMNukOBAtXfFIpRYMhMQ6067445;     eMNukOBAtXfFIpRYMhMQ6067445 = eMNukOBAtXfFIpRYMhMQ15037909;     eMNukOBAtXfFIpRYMhMQ15037909 = eMNukOBAtXfFIpRYMhMQ60744178;     eMNukOBAtXfFIpRYMhMQ60744178 = eMNukOBAtXfFIpRYMhMQ60163898;     eMNukOBAtXfFIpRYMhMQ60163898 = eMNukOBAtXfFIpRYMhMQ49749430;     eMNukOBAtXfFIpRYMhMQ49749430 = eMNukOBAtXfFIpRYMhMQ71591418;     eMNukOBAtXfFIpRYMhMQ71591418 = eMNukOBAtXfFIpRYMhMQ92008922;     eMNukOBAtXfFIpRYMhMQ92008922 = eMNukOBAtXfFIpRYMhMQ81597122;     eMNukOBAtXfFIpRYMhMQ81597122 = eMNukOBAtXfFIpRYMhMQ46714030;     eMNukOBAtXfFIpRYMhMQ46714030 = eMNukOBAtXfFIpRYMhMQ23346623;     eMNukOBAtXfFIpRYMhMQ23346623 = eMNukOBAtXfFIpRYMhMQ72806994;     eMNukOBAtXfFIpRYMhMQ72806994 = eMNukOBAtXfFIpRYMhMQ38385817;     eMNukOBAtXfFIpRYMhMQ38385817 = eMNukOBAtXfFIpRYMhMQ67602818;     eMNukOBAtXfFIpRYMhMQ67602818 = eMNukOBAtXfFIpRYMhMQ88136311;     eMNukOBAtXfFIpRYMhMQ88136311 = eMNukOBAtXfFIpRYMhMQ37887174;     eMNukOBAtXfFIpRYMhMQ37887174 = eMNukOBAtXfFIpRYMhMQ88692877;     eMNukOBAtXfFIpRYMhMQ88692877 = eMNukOBAtXfFIpRYMhMQ88528909;     eMNukOBAtXfFIpRYMhMQ88528909 = eMNukOBAtXfFIpRYMhMQ33958935;     eMNukOBAtXfFIpRYMhMQ33958935 = eMNukOBAtXfFIpRYMhMQ24383061;     eMNukOBAtXfFIpRYMhMQ24383061 = eMNukOBAtXfFIpRYMhMQ63063847;     eMNukOBAtXfFIpRYMhMQ63063847 = eMNukOBAtXfFIpRYMhMQ39595141;     eMNukOBAtXfFIpRYMhMQ39595141 = eMNukOBAtXfFIpRYMhMQ20163446;     eMNukOBAtXfFIpRYMhMQ20163446 = eMNukOBAtXfFIpRYMhMQ47283639;     eMNukOBAtXfFIpRYMhMQ47283639 = eMNukOBAtXfFIpRYMhMQ50075394;     eMNukOBAtXfFIpRYMhMQ50075394 = eMNukOBAtXfFIpRYMhMQ35598284;     eMNukOBAtXfFIpRYMhMQ35598284 = eMNukOBAtXfFIpRYMhMQ55145091;     eMNukOBAtXfFIpRYMhMQ55145091 = eMNukOBAtXfFIpRYMhMQ25722826;     eMNukOBAtXfFIpRYMhMQ25722826 = eMNukOBAtXfFIpRYMhMQ36612541;     eMNukOBAtXfFIpRYMhMQ36612541 = eMNukOBAtXfFIpRYMhMQ32364993;     eMNukOBAtXfFIpRYMhMQ32364993 = eMNukOBAtXfFIpRYMhMQ53480436;     eMNukOBAtXfFIpRYMhMQ53480436 = eMNukOBAtXfFIpRYMhMQ19501918;     eMNukOBAtXfFIpRYMhMQ19501918 = eMNukOBAtXfFIpRYMhMQ62750332;     eMNukOBAtXfFIpRYMhMQ62750332 = eMNukOBAtXfFIpRYMhMQ71758558;     eMNukOBAtXfFIpRYMhMQ71758558 = eMNukOBAtXfFIpRYMhMQ75166612;     eMNukOBAtXfFIpRYMhMQ75166612 = eMNukOBAtXfFIpRYMhMQ36631378;     eMNukOBAtXfFIpRYMhMQ36631378 = eMNukOBAtXfFIpRYMhMQ95199925;     eMNukOBAtXfFIpRYMhMQ95199925 = eMNukOBAtXfFIpRYMhMQ59670726;     eMNukOBAtXfFIpRYMhMQ59670726 = eMNukOBAtXfFIpRYMhMQ60897080;     eMNukOBAtXfFIpRYMhMQ60897080 = eMNukOBAtXfFIpRYMhMQ27042060;     eMNukOBAtXfFIpRYMhMQ27042060 = eMNukOBAtXfFIpRYMhMQ39029868;     eMNukOBAtXfFIpRYMhMQ39029868 = eMNukOBAtXfFIpRYMhMQ2325296;     eMNukOBAtXfFIpRYMhMQ2325296 = eMNukOBAtXfFIpRYMhMQ77145131;     eMNukOBAtXfFIpRYMhMQ77145131 = eMNukOBAtXfFIpRYMhMQ82318132;     eMNukOBAtXfFIpRYMhMQ82318132 = eMNukOBAtXfFIpRYMhMQ27732222;     eMNukOBAtXfFIpRYMhMQ27732222 = eMNukOBAtXfFIpRYMhMQ3201431;     eMNukOBAtXfFIpRYMhMQ3201431 = eMNukOBAtXfFIpRYMhMQ82411942;     eMNukOBAtXfFIpRYMhMQ82411942 = eMNukOBAtXfFIpRYMhMQ63465767;     eMNukOBAtXfFIpRYMhMQ63465767 = eMNukOBAtXfFIpRYMhMQ99091269;     eMNukOBAtXfFIpRYMhMQ99091269 = eMNukOBAtXfFIpRYMhMQ77999094;     eMNukOBAtXfFIpRYMhMQ77999094 = eMNukOBAtXfFIpRYMhMQ84522518;     eMNukOBAtXfFIpRYMhMQ84522518 = eMNukOBAtXfFIpRYMhMQ20434432;     eMNukOBAtXfFIpRYMhMQ20434432 = eMNukOBAtXfFIpRYMhMQ45565093;     eMNukOBAtXfFIpRYMhMQ45565093 = eMNukOBAtXfFIpRYMhMQ86866080;     eMNukOBAtXfFIpRYMhMQ86866080 = eMNukOBAtXfFIpRYMhMQ36918041;     eMNukOBAtXfFIpRYMhMQ36918041 = eMNukOBAtXfFIpRYMhMQ64543343;     eMNukOBAtXfFIpRYMhMQ64543343 = eMNukOBAtXfFIpRYMhMQ85903998;     eMNukOBAtXfFIpRYMhMQ85903998 = eMNukOBAtXfFIpRYMhMQ67754269;     eMNukOBAtXfFIpRYMhMQ67754269 = eMNukOBAtXfFIpRYMhMQ10668784;     eMNukOBAtXfFIpRYMhMQ10668784 = eMNukOBAtXfFIpRYMhMQ24565615;     eMNukOBAtXfFIpRYMhMQ24565615 = eMNukOBAtXfFIpRYMhMQ94604338;     eMNukOBAtXfFIpRYMhMQ94604338 = eMNukOBAtXfFIpRYMhMQ45868592;     eMNukOBAtXfFIpRYMhMQ45868592 = eMNukOBAtXfFIpRYMhMQ55396381;     eMNukOBAtXfFIpRYMhMQ55396381 = eMNukOBAtXfFIpRYMhMQ49232129;     eMNukOBAtXfFIpRYMhMQ49232129 = eMNukOBAtXfFIpRYMhMQ93233594;     eMNukOBAtXfFIpRYMhMQ93233594 = eMNukOBAtXfFIpRYMhMQ3844706;     eMNukOBAtXfFIpRYMhMQ3844706 = eMNukOBAtXfFIpRYMhMQ10056663;     eMNukOBAtXfFIpRYMhMQ10056663 = eMNukOBAtXfFIpRYMhMQ66627259;     eMNukOBAtXfFIpRYMhMQ66627259 = eMNukOBAtXfFIpRYMhMQ98362483;}
// Junk Finished
