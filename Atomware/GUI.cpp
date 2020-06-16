#include <fstream>
#include <functional>
#include <string>
#include <ShlObj.h>
#include <Windows.h>

#include "imgui/imgui.h"
#include "imgui/imgui_impl_win32.h"
#include "imgui/imgui_stdlib.h"

#include "imguiCustom.h"

#include "GUI.h"
#include "Config.h"
#include "Hacks/Misc.h"
#include "Hacks/Reportbot.h"
#include "Hacks/SkinChanger.h"
#include "Hooks.h"
#include "SDK/InputSystem.h"

constexpr auto windowFlags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize
| ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;

GUI::GUI() noexcept
{
    ImGui::CreateContext();
    ImGui_ImplWin32_Init(FindWindowW(L"Valve001", NULL));

    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();

    style.ScrollbarSize = 9.0f;

    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;
    io.LogFilename = nullptr;
    io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;

    if (PWSTR pathToFonts; SUCCEEDED(SHGetKnownFolderPath(FOLDERID_Fonts, 0, nullptr, &pathToFonts))) {
        const std::filesystem::path path{ pathToFonts };
        CoTaskMemFree(pathToFonts);

        static constexpr ImWchar ranges[]{ 0x0020, 0xFFFF, 0 };
        fonts.tahoma = io.Fonts->AddFontFromFileTTF((path / "tahoma.ttf").string().c_str(), 15.0f, nullptr, ranges);
        fonts.segoeui = io.Fonts->AddFontFromFileTTF((path / "segoeui.ttf").string().c_str(), 15.0f, nullptr, ranges);
        fonts.indicators = io.Fonts->AddFontFromFileTTF((path / "tahoma.ttf").string().c_str(), 22.0f, nullptr, ranges);
        config->misc.segoesmk = fonts.segoeui;
        config->misc.segoesmk->FontSize = 13;
    }
}

void GUI::render() noexcept
{
    /*if (!config->style.menuStyle) {
        renderMenuBar();
        renderAimbotWindow();
        renderAntiAimWindow();
        renderTriggerbotWindow();
        renderBacktrackWindow();
        renderGlowWindow();
        renderChamsWindow();
        renderEspWindow();
        renderVisualsWindow();
        renderSkinChangerWindow();
        renderSoundWindow();
        renderStyleWindow();
        renderMiscWindow();
        renderReportbotWindow();
        renderConfigWindow();
    } else {*/
        renderGuiStyle2();
    //}
}

void GUI::updateColors() const noexcept
{
    switch (config->style.menuColors) {
    case 0: ImGui::StyleColorsDark(); break;
    case 1: ImGui::StyleColorsLight(); break;
    case 2: ImGui::StyleColorsClassic(); break;
    }
}

void GUI::hotkey(int& key) noexcept
{
    key ? ImGui::Text("[ %s ]", interfaces->inputSystem->virtualKeyToString(key)) : ImGui::TextUnformatted("[ key ]");

    if (!ImGui::IsItemHovered())
        return;

    ImGui::SetTooltip("Press any key to change keybind");
    ImGuiIO& io = ImGui::GetIO();
    for (int i = 0; i < IM_ARRAYSIZE(io.KeysDown); i++)
        if (ImGui::IsKeyPressed(i) && i != config->misc.menuKey)
            key = i != VK_ESCAPE ? i : 0;

    for (int i = 0; i < IM_ARRAYSIZE(io.MouseDown); i++)
        if (ImGui::IsMouseDown(i) && i + (i > 1 ? 2 : 1) != config->misc.menuKey)
            key = i + (i > 1 ? 2 : 1);
}

void GUI::renderMenuBar() noexcept
{
    if (ImGui::BeginMainMenuBar()) {
        ImGui::MenuItem("Aimbot", nullptr, &window.aimbot);
        ImGui::MenuItem("Anti aim", nullptr, &window.antiAim);
        ImGui::MenuItem("Triggerbot", nullptr, &window.triggerbot);
        ImGui::MenuItem("Backtrack", nullptr, &window.backtrack);
        ImGui::MenuItem("Glow", nullptr, &window.glow);
        ImGui::MenuItem("Chams", nullptr, &window.chams);
        ImGui::MenuItem("Esp", nullptr, &window.esp);
        ImGui::MenuItem("Visuals", nullptr, &window.visuals);
        ImGui::MenuItem("Skin changer", nullptr, &window.skinChanger);
        ImGui::MenuItem("Sound", nullptr, &window.sound);
        ImGui::MenuItem("Style", nullptr, &window.style);
        ImGui::MenuItem("Misc", nullptr, &window.misc);
        ImGui::MenuItem("Reportbot", nullptr, &window.reportbot);
        ImGui::MenuItem("Config", nullptr, &window.config);
        ImGui::EndMainMenuBar();
    }
}

void GUI::renderAimbotWindow(bool contentOnly) noexcept
{
    if (!contentOnly) {
        if (!window.aimbot)
            return;
        ImGui::SetNextWindowSize({ 600.0f, 0.0f });
        ImGui::Begin("Aimbot", &window.aimbot, windowFlags);
    }
    static int currentCategory{ 0 };
    ImGui::PushItemWidth(110.0f);
    ImGui::PushID(0);
    ImGui::Combo("", &currentCategory, "All\0Pistols\0Heavy\0SMG\0Rifles\0");
    ImGui::PopID();
    ImGui::SameLine();
    static int currentWeapon{ 0 };
    ImGui::PushID(1);

    switch (currentCategory) {
    case 0:
        currentWeapon = 0;
        ImGui::NewLine();
        break;
    case 1: {
        static int currentPistol{ 0 };
        static constexpr const char* pistols[]{ "All", "Glock-18", "P2000", "USP-S", "Dual Berettas", "P250", "Tec-9", "Five-Seven", "CZ-75", "Desert Eagle", "Revolver" };

        ImGui::Combo("", &currentPistol, [](void* data, int idx, const char** out_text) {
            if (config->aimbot[idx ? idx : 35].enabled) {
                static std::string name;
                name = pistols[idx];
                *out_text = name.append(" *").c_str();
            }
            else {
                *out_text = pistols[idx];
            }
            return true;
            }, nullptr, IM_ARRAYSIZE(pistols));

        currentWeapon = currentPistol ? currentPistol : 35;
        break;
    }
    case 2: {
        static int currentHeavy{ 0 };
        static constexpr const char* heavies[]{ "All", "Nova", "XM1014", "Sawed-off", "MAG-7", "M249", "Negev" };

        ImGui::Combo("", &currentHeavy, [](void* data, int idx, const char** out_text) {
            if (config->aimbot[idx ? idx + 10 : 36].enabled) {
                static std::string name;
                name = heavies[idx];
                *out_text = name.append(" *").c_str();
            }
            else {
                *out_text = heavies[idx];
            }
            return true;
            }, nullptr, IM_ARRAYSIZE(heavies));

        currentWeapon = currentHeavy ? currentHeavy + 10 : 36;
        break;
    }
    case 3: {
        static int currentSmg{ 0 };
        static constexpr const char* smgs[]{ "All", "Mac-10", "MP9", "MP7", "MP5-SD", "UMP-45", "P90", "PP-Bizon" };

        ImGui::Combo("", &currentSmg, [](void* data, int idx, const char** out_text) {
            if (config->aimbot[idx ? idx + 16 : 37].enabled) {
                static std::string name;
                name = smgs[idx];
                *out_text = name.append(" *").c_str();
            }
            else {
                *out_text = smgs[idx];
            }
            return true;
            }, nullptr, IM_ARRAYSIZE(smgs));

        currentWeapon = currentSmg ? currentSmg + 16 : 37;
        break;
    }
    case 4: {
        static int currentRifle{ 0 };
        static constexpr const char* rifles[]{ "All", "Galil AR", "Famas", "AK-47", "M4A4", "M4A1-S", "SSG-08", "SG-553", "AUG", "AWP", "G3SG1", "SCAR-20" };

        ImGui::Combo("", &currentRifle, [](void* data, int idx, const char** out_text) {
            if (config->aimbot[idx ? idx + 23 : 38].enabled) {
                static std::string name;
                name = rifles[idx];
                *out_text = name.append(" *").c_str();
            }
            else {
                *out_text = rifles[idx];
            }
            return true;
            }, nullptr, IM_ARRAYSIZE(rifles));

        currentWeapon = currentRifle ? currentRifle + 23 : 38;
        break;
    }
    }
    ImGui::PopID();
    ImGui::SameLine();
    ImGui::Checkbox("Enabled", &config->aimbot[currentWeapon].enabled);
    ImGui::Separator();
    ImGui::Columns(3, nullptr, false);
    ImGui::SetColumnOffset(1, 380.0f);
    ImGui::SetColumnOffset(2, 720.0f);
    ImGui::Checkbox("On key", &config->aimbot[currentWeapon].onKey);
    ImGui::SameLine();
    hotkey(config->aimbot[currentWeapon].key);
    ImGui::SameLine();
    ImGui::PushID(2);
    ImGui::PushItemWidth(70.0f);
    ImGui::Combo("", &config->aimbot[currentWeapon].keyMode, "Hold\0Toggle\0");
    ImGui::PopItemWidth();
    ImGui::PopID();
    ImGui::Checkbox("Aimlock", &config->aimbot[currentWeapon].aimlock);
    ImGui::Checkbox("Silent", &config->aimbot[currentWeapon].silent);
    ImGui::Checkbox("Friendly fire", &config->aimbot[currentWeapon].friendlyFire);
    ImGui::Checkbox("Visible only", &config->aimbot[currentWeapon].visibleOnly);
    ImGui::Checkbox("Scoped only", &config->aimbot[currentWeapon].scopedOnly);
    ImGui::Checkbox("Ignore flash", &config->aimbot[currentWeapon].ignoreFlash);
    ImGui::Checkbox("Ignore smoke", &config->aimbot[currentWeapon].ignoreSmoke);
    ImGui::Checkbox("Auto shot", &config->aimbot[currentWeapon].autoShot);
    ImGui::Checkbox("Auto scope", &config->aimbot[currentWeapon].autoScope);
    ImGui::InputInt("Shot Delay (ms)", &config->aimbot[currentWeapon].shotdelay);
    ImGui::Checkbox("RCS", &config->aimbot[currentWeapon].rcs);
    ImGui::Checkbox("Ragebot", &config->aimbot[currentWeapon].ragebot);
    if (!config->aimbot[currentWeapon].ragebot)
    {
        ImGui::Combo("Bone", &config->aimbot[currentWeapon].bone, "Nearest\0Best damage\0Head\0Neck\0Sternum\0Chest\0Stomach\0Pelvis\0");
    }
    else
    {

    //ImGui::Combo("Bone", &config->aimbot[currentWeapon].bone, "Nearest\0Best damage\0Head\0Neck\0Sternum\0Chest\0Stomach\0Pelvis\0");
    std::string previewValue = "";
    const char* items[]{ "Head", "Neck", "Pelvis", "Stomach", "Lower Chest", "Chest", "Upper Chest", "Right Thigh", "Left Thigh", "Right Calf", "Left Calf", "Right Foot", "Left Foot", "Right Hand", "Left Hand", "Right Upper Arm", "Right Forearm", "Left Upper Arm", "Left Forearm" };
    if (ImGui::BeginCombo("Hitbox", previewValue.c_str()))
    {
        previewValue = "";
        std::vector<std::string> vec;
        for (size_t i = 0; i < IM_ARRAYSIZE(items); ++i)
        {
            ImGui::Selectable(items[i], &config->aimbot[currentWeapon].hitboxes[i], ImGuiSelectableFlags_::ImGuiSelectableFlags_DontClosePopups);
            if (config->aimbot[currentWeapon].hitboxes[i])
                vec.push_back(items[i]);
        }

        if (vec.size() == 0)
            previewValue = "None";

        for (size_t i = 0; i < vec.size(); i++)
        {
            if (vec.size() == 1)
                previewValue += vec.at(i);
            else if (!(i + 1 == vec.size()))
                previewValue += vec.at(i) + ", ";
            else
                previewValue += vec.at(i);
        }

        ImGui::EndCombo();
    }
    }
    //ImGui::NextColumn();
    ImGui::PushItemWidth(240.0f);
    ImGui::SliderFloat("Fov", &config->aimbot[currentWeapon].fov, 0.0f, 255.0f, "%.2f", 2.5f);
    ImGui::SliderFloat("Smooth", &config->aimbot[currentWeapon].smooth, 1.0f, 100.0f, "%.2f");
    ImGui::SliderInt("Hitchance", &config->aimbot[currentWeapon].hitchance, 0, 100, "%d");
    ImGui::InputInt("Min damage", &config->aimbot[currentWeapon].minDamage);
    config->aimbot[currentWeapon].minDamage = std::clamp(config->aimbot[currentWeapon].minDamage, 0, 250);
    ImGui::Checkbox("Killshot", &config->aimbot[currentWeapon].killshot);
    ImGui::Checkbox("Between shots", &config->aimbot[currentWeapon].betweenShots);
    //TRIGGERBOT BEGINS HERE
    ImGui::NextColumn();
    ImGui::PushItemWidth(120.0f);
    ImGui::BeginChild("Triggerbot", ImVec2(335.0f, 0), true);
    ImGui::TextColored(ImColor(255, 255, 255), "Triggerbot");
    ImGui::Separator();
    ImGui::Checkbox("Enabled", &config->triggerbot[currentWeapon].enabled);
    //ImGui::Separator();
    ImGui::Checkbox("On key", &config->triggerbot[currentWeapon].onKey);
    ImGui::SameLine();
    hotkey(config->triggerbot[currentWeapon].key);
    //ImGui::SliderInt("Hitchance", &config->triggerbot[currentWeapon].hitchance, 0, 100, "%d %");
    ImGui::Checkbox("Friendly fire", &config->triggerbot[currentWeapon].friendlyFire);
    ImGui::Checkbox("Scoped only", &config->triggerbot[currentWeapon].scopedOnly);
    ImGui::Checkbox("Ignore flash", &config->triggerbot[currentWeapon].ignoreFlash);
    ImGui::Checkbox("Ignore smoke", &config->triggerbot[currentWeapon].ignoreSmoke);
    ImGui::SetNextItemWidth(85.0f);
    ImGui::Combo("Hitgroup", &config->triggerbot[currentWeapon].hitgroup, "All\0Head\0Chest\0Stomach\0Left arm\0Right arm\0Left leg\0Right leg\0");
    ImGui::PushItemWidth(220.0f);
    ImGui::SliderInt("Shot delay", &config->triggerbot[currentWeapon].shotDelay, 0, 250, "%d ms");
    ImGui::InputInt("Min damage", &config->triggerbot[currentWeapon].minDamage);
    config->triggerbot[currentWeapon].minDamage = std::clamp(config->triggerbot[currentWeapon].minDamage, 0, 250);
    ImGui::Checkbox("Killshot", &config->triggerbot[currentWeapon].killshot);
    ImGui::SliderFloat("Burst Time", &config->triggerbot[currentWeapon].burstTime, 0.0f, 0.5f, "%.3f s");
    ImGui::EndChild();
    //TRIGGERBOT ENDS HERE
    //BACKTRACK BEGINS HERE
    ImGui::NextColumn();
    ImGui::BeginChild("Backtrack", ImVec2(0.0f, 0), true);
    ImGui::TextColored(ImColor(255, 255, 255), "Backtrack");
    ImGui::Separator();
    ImGui::Checkbox("Enabled", &config->backtrack[currentWeapon].enabled);
    ImGui::Checkbox("Ignore smoke", &config->backtrack[currentWeapon].ignoreSmoke);
    ImGui::Checkbox("Recoil based fov", &config->backtrack[currentWeapon].recoilBasedFov);
    ImGui::PushItemWidth(220.0f);
    ImGui::SliderInt("Time lim", &config->backtrack[currentWeapon].timeLimit, 1, 200, "%d ms");
    std::clamp(config->backtrack[currentWeapon].timeLimit, 1, 200);
    ImGui::PopItemWidth();
    ImGui::EndChild();
    //BACKTRACK ENDS HERE
    ImGui::Columns(1);
    if (!contentOnly)
        ImGui::End();
}

void GUI::renderAntiAimWindow(bool contentOnly) noexcept
{
    if (!contentOnly) {
        if (!window.antiAim)
            return;
        ImGui::SetNextWindowSize({ 0.0f, 0.0f });
        ImGui::Begin("Ragebot", &window.antiAim, windowFlags);
    }
    ImGui::TextColored(ImColor(255, 255, 255), "Anti-aim");
    ImGui::Separator();
    ImGui::Checkbox("Anti-aim", &config->antiAim.enabled);
    ImGui::Combo("Type", &config->antiAim.antiaimType, "Follow crosshair\0Legit AA\0Freestanding a.k.a how to break resolvers\0");
    if (config->antiAim.antiaimType == 0)
    {
        ImGui::SliderFloat("Pitch (x)", &config->antiAim.pitchAngle, -89.0f, 89.0f, "%.2f");
        ImGui::SliderInt("Body lean (fake y lean)", &config->antiAim.bodylean, 0, 100, "%d");
    }
    ImGui::Checkbox("Break LBY", &config->antiAim.lbybreaker);
    ImGui::Text("Invert");
    ImGui::SameLine();
    hotkey(config->antiAim.invertkey);

    ImGui::Text("Left");
    ImGui::SameLine();
    hotkey(config->antiAim.leftkey);

    ImGui::Text("Right");
    ImGui::SameLine();
    hotkey(config->antiAim.rightkey);

    ImGui::Text("Backwards");
    ImGui::SameLine();
    hotkey(config->antiAim.backwardskey);

    ImGui::Text("See real in thirdperson");
    ImGui::SameLine();
    hotkey(config->antiAim.realkey);

    ImGui::Text("See fake in thirdperson");
    ImGui::SameLine();
    hotkey(config->antiAim.fakekey);
    ImGui::Separator();
    ImGui::NewLine();
    ImGui::TextColored(ImColor(255, 255, 255), "Fake lag");
    ImGui::Separator();
    ImGui::InputInt("Choked packets", &config->misc.chokedPackets, 1, 5);
    if (!config->antiAim.enabled)
        config->misc.chokedPackets = std::clamp(config->misc.chokedPackets, 0, 64);
    else
        config->misc.chokedPackets = std::clamp(config->misc.chokedPackets, 1, 64);
    ImGui::Separator();

    ImGui::NewLine();
    ImGui::TextColored(ImColor(255, 255, 255), "Resolver");
    ImGui::Separator();
    ImGui::Checkbox("Resolver", &config->antiAim.resolveall);
    ImGui::Checkbox("Pitch Resolver", &config->antiAim.pitchResolver);
    ImGui::Separator();

    ImGui::NewLine();
    ImGui::TextColored(ImColor(255, 255, 255), "Misc");
    ImGui::Separator();
    //ImGui::Checkbox("No spread", &config.rageBot.nospread);
    ImGui::Checkbox("Auto Stop", &config->antiAim.autostop);
    ImGui::Checkbox("Auto Zeus", &config->triggerbot[39].enabled);
    ImGui::Separator();
    if (!contentOnly)
        ImGui::End();
}

void GUI::renderTriggerbotWindow(bool contentOnly) noexcept
{
    if (!contentOnly) {
        if (!window.triggerbot)
            return;
        ImGui::SetNextWindowSize({ 0.0f, 0.0f });
        ImGui::Begin("Triggerbot", &window.triggerbot, windowFlags);
    }
    static int currentCategory{ 0 };
    ImGui::PushItemWidth(110.0f);
    ImGui::PushID(0);
    ImGui::Combo("", &currentCategory, "All\0Pistols\0Heavy\0SMG\0Rifles\0Zeus x27\0");
    ImGui::PopID();
    ImGui::SameLine();
    static int currentWeapon{ 0 };
    ImGui::PushID(1);
    switch (currentCategory) {
    case 0:
        currentWeapon = 0;
        ImGui::NewLine();
        break;
    case 5:
        currentWeapon = 39;
        ImGui::NewLine();
        break;

    case 1: {
        static int currentPistol{ 0 };
        static constexpr const char* pistols[]{ "All", "Glock-18", "P2000", "USP-S", "Dual Berettas", "P250", "Tec-9", "Five-Seven", "CZ-75", "Desert Eagle", "Revolver" };

        ImGui::Combo("", &currentPistol, [](void* data, int idx, const char** out_text) {
            if (config->triggerbot[idx ? idx : 35].enabled) {
                static std::string name;
                name = pistols[idx];
                *out_text = name.append(" *").c_str();
            } else {
                *out_text = pistols[idx];
            }
            return true;
            }, nullptr, IM_ARRAYSIZE(pistols));

        currentWeapon = currentPistol ? currentPistol : 35;
        break;
    }
    case 2: {
        static int currentHeavy{ 0 };
        static constexpr const char* heavies[]{ "All", "Nova", "XM1014", "Sawed-off", "MAG-7", "M249", "Negev" };

        ImGui::Combo("", &currentHeavy, [](void* data, int idx, const char** out_text) {
            if (config->triggerbot[idx ? idx + 10 : 36].enabled) {
                static std::string name;
                name = heavies[idx];
                *out_text = name.append(" *").c_str();
            } else {
                *out_text = heavies[idx];
            }
            return true;
            }, nullptr, IM_ARRAYSIZE(heavies));

        currentWeapon = currentHeavy ? currentHeavy + 10 : 36;
        break;
    }
    case 3: {
        static int currentSmg{ 0 };
        static constexpr const char* smgs[]{ "All", "Mac-10", "MP9", "MP7", "MP5-SD", "UMP-45", "P90", "PP-Bizon" };

        ImGui::Combo("", &currentSmg, [](void* data, int idx, const char** out_text) {
            if (config->triggerbot[idx ? idx + 16 : 37].enabled) {
                static std::string name;
                name = smgs[idx];
                *out_text = name.append(" *").c_str();
            } else {
                *out_text = smgs[idx];
            }
            return true;
            }, nullptr, IM_ARRAYSIZE(smgs));

        currentWeapon = currentSmg ? currentSmg + 16 : 37;
        break;
    }
    case 4: {
        static int currentRifle{ 0 };
        static constexpr const char* rifles[]{ "All", "Galil AR", "Famas", "AK-47", "M4A4", "M4A1-S", "SSG-08", "SG-553", "AUG", "AWP", "G3SG1", "SCAR-20" };

        ImGui::Combo("", &currentRifle, [](void* data, int idx, const char** out_text) {
            if (config->triggerbot[idx ? idx + 23 : 38].enabled) {
                static std::string name;
                name = rifles[idx];
                *out_text = name.append(" *").c_str();
            } else {
                *out_text = rifles[idx];
            }
            return true;
            }, nullptr, IM_ARRAYSIZE(rifles));

        currentWeapon = currentRifle ? currentRifle + 23 : 38;
        break;
    }
    }
    ImGui::PopID();
    ImGui::SameLine();
    ImGui::Checkbox("Enabled", &config->triggerbot[currentWeapon].enabled);
    ImGui::Separator();
    ImGui::Checkbox("On key", &config->triggerbot[currentWeapon].onKey);
    ImGui::SameLine();
    hotkey(config->triggerbot[currentWeapon].key);
    ImGui::Checkbox("Friendly fire", &config->triggerbot[currentWeapon].friendlyFire);
    ImGui::Checkbox("Scoped only", &config->triggerbot[currentWeapon].scopedOnly);
    ImGui::Checkbox("Ignore flash", &config->triggerbot[currentWeapon].ignoreFlash);
    ImGui::Checkbox("Ignore smoke", &config->triggerbot[currentWeapon].ignoreSmoke);
    ImGui::SetNextItemWidth(85.0f);
    ImGui::Combo("Hitgroup", &config->triggerbot[currentWeapon].hitgroup, "All\0Head\0Chest\0Stomach\0Left arm\0Right arm\0Left leg\0Right leg\0");
    ImGui::PushItemWidth(220.0f);
    ImGui::SliderInt("Shot delay", &config->triggerbot[currentWeapon].shotDelay, 0, 250, "%d ms");
    ImGui::InputInt("Min damage", &config->triggerbot[currentWeapon].minDamage);
    config->triggerbot[currentWeapon].minDamage = std::clamp(config->triggerbot[currentWeapon].minDamage, 0, 250);
    ImGui::Checkbox("Killshot", &config->triggerbot[currentWeapon].killshot);
    ImGui::SliderFloat("Burst Time", &config->triggerbot[currentWeapon].burstTime, 0.0f, 0.5f, "%.3f s");

    if (!contentOnly)
        ImGui::End();
}

void GUI::renderBacktrackWindow(bool contentOnly) noexcept
{
    if (!contentOnly) {
        if (!window.backtrack)
            return;
        ImGui::SetNextWindowSize({ 0.0f, 0.0f });
        ImGui::Begin("Backtrack", &window.backtrack, windowFlags);
    }
    /*ImGui::Checkbox("Enabled", &config->backtrack.enabled);
    ImGui::Checkbox("Ignore smoke", &config->backtrack.ignoreSmoke);
    ImGui::Checkbox("Recoil based fov", &config->backtrack.recoilBasedFov);
    ImGui::PushItemWidth(220.0f);
    ImGui::SliderInt("Time limit", &config->backtrack.timeLimit, 1, 200, "%d ms");*/
    ImGui::PopItemWidth();
    if (!contentOnly)
        ImGui::End();
}

void GUI::renderGlowWindow(bool contentOnly) noexcept
{
    if (!contentOnly) {
        if (!window.glow)
            return;
        ImGui::SetNextWindowSize({ 450.0f, 0.0f });
        ImGui::Begin("Glow", &window.glow, windowFlags);
    }
    static int currentCategory{ 0 };
    ImGui::PushItemWidth(110.0f);
    ImGui::PushID(0);
    ImGui::Combo("", &currentCategory, "Allies\0Enemies\0Planting\0Defusing\0Local player\0Weapons\0C4\0Planted C4\0Chickens\0Defuse kits\0Projectiles\0Hostages\0Ragdolls\0");
    ImGui::PopID();
    static int currentItem{ 0 };
    if (currentCategory <= 3) {
        ImGui::SameLine();
        static int currentType{ 0 };
        ImGui::PushID(1);
        ImGui::Combo("", &currentType, "All\0Visible\0Occluded\0");
        ImGui::PopID();
        currentItem = currentCategory * 3 + currentType;
    } else {
        currentItem = currentCategory + 8;
    }

    ImGui::SameLine();
    ImGui::Checkbox("Enabled", &config->glow[currentItem].enabled);
    ImGui::Separator();
    ImGui::Columns(2, nullptr, false);
    ImGui::SetColumnOffset(1, 150.0f);
    ImGui::Checkbox("Health based", &config->glow[currentItem].healthBased);

    ImGuiCustom::colorPicker("Color", config->glow[currentItem].color.color, nullptr, &config->glow[currentItem].color.rainbow, &config->glow[currentItem].color.rainbowSpeed);
    ImGui::Checkbox("Sound Glow", &config->glow[currentItem].soundGlow);
    ImGui::NextColumn();
    ImGui::PushItemWidth(220.0f);
    ImGui::SliderFloat("Thickness", &config->glow[currentItem].thickness, 0.0f, 1.0f, "%.2f");
    ImGui::SliderFloat("Alpha", &config->glow[currentItem].alpha, 0.0f, 1.0f, "%.2f");
    ImGui::SliderInt("Style", &config->glow[currentItem].style, 0, 3);
    ImGui::Columns(1);
    if (!contentOnly)
        ImGui::End();
}

void GUI::renderChamsWindow(bool contentOnly) noexcept
{
    if (!contentOnly) {
        if (!window.chams)
            return;
        ImGui::SetNextWindowSize({ 0.0f, 0.0f });
        ImGui::Begin("Chams", &window.chams, windowFlags);
    }
    static int currentCategory{ 0 };
    ImGui::PushItemWidth(110.0f);
    ImGui::PushID(0);
    ImGui::Combo("", &currentCategory, "Allies\0Enemies\0Planting\0Defusing\0Local player\0Weapons\0Hands\0Backtrack\0Sleeves\0");
    ImGui::PopID();
    static int currentItem{ 0 };

    if (currentCategory <= 3) {
        ImGui::SameLine();
        static int currentType{ 0 };
        ImGui::PushID(1);
        ImGui::Combo("", &currentType, "All\0Visible\0Occluded\0");
        ImGui::PopID();
        currentItem = currentCategory * 3 + currentType;
    } else {
        currentItem = currentCategory + 8;
    }

    ImGui::SameLine();
    static int material = 1;

    if (ImGui::ArrowButton("##left", ImGuiDir_Left) && material > 1)
        --material;
    ImGui::SameLine();
    ImGui::Text("%d", material);
    ImGui::SameLine();
    if (ImGui::ArrowButton("##right", ImGuiDir_Right) && material < int(config->chams[0].materials.size()))
        ++material;

    ImGui::SameLine();
    auto& chams{ config->chams[currentItem].materials[material - 1] };

    ImGui::Checkbox("Enabled", &chams.enabled);
    ImGui::Separator();
    ImGui::Checkbox("Health based", &chams.healthBased);
    ImGui::Checkbox("Blinking", &chams.blinking);
    ImGui::Combo("Material", &chams.material, "Normal\0Flat\0Animated\0Platinum\0Glass\0Chrome\0Crystal\0Silver\0Gold\0Plastic\0Glow\0");
    ImGui::Checkbox("Wireframe", &chams.wireframe);
    ImGuiCustom::colorPicker("Color", chams.color.color, nullptr, &chams.color.rainbow, &chams.color.rainbowSpeed);
    ImGui::SetNextItemWidth(220.0f);
    ImGui::SliderFloat("Alpha", &chams.alpha, 0.0f, 1.0f, "%.2f");

    if (!contentOnly) {
        ImGui::End();
    }
}

void GUI::renderEspWindow(bool contentOnly) noexcept
{
    if (!contentOnly) {
        if (!window.esp)
            return;
        ImGui::SetNextWindowSize({ 0.0f, 0.0f });
        ImGui::Begin("Esp", &window.esp, windowFlags);
    }

    static int currentCategory = 0;
    static int currentItem = 0;

    if (ImGui::ListBoxHeader("##", { 125.0f, 300.0f })) {
        static constexpr const char* players[]{ "All", "Visible", "Occluded" };

        ImGui::Text("Allies");
        ImGui::Indent();
        ImGui::PushID("Allies");
        ImGui::PushFont(fonts.segoeui);

        for (int i = 0; i < IM_ARRAYSIZE(players); i++) {
            bool isSelected = currentCategory == 0 && currentItem == i;

            if ((i == 0 || !config->esp.players[0].enabled) && ImGui::Selectable(players[i], isSelected)) {
                currentItem = i;
                currentCategory = 0;
            }
        }

        ImGui::PopFont();
        ImGui::PopID();
        ImGui::Unindent();
        ImGui::Text("Enemies");
        ImGui::Indent();
        ImGui::PushID("Enemies");
        ImGui::PushFont(fonts.segoeui);

        for (int i = 0; i < IM_ARRAYSIZE(players); i++) {
            bool isSelected = currentCategory == 1 && currentItem == i;

            if ((i == 0 || !config->esp.players[3].enabled) && ImGui::Selectable(players[i], isSelected)) {
                currentItem = i;
                currentCategory = 1;
            }
        }

        ImGui::PopFont();
        ImGui::PopID();
        ImGui::Unindent();
        if (bool isSelected = currentCategory == 2; ImGui::Selectable("Weapons", isSelected))
            currentCategory = 2;

        ImGui::Text("Projectiles");
        ImGui::Indent();
        ImGui::PushID("Projectiles");
        ImGui::PushFont(fonts.segoeui);
        static constexpr const char* projectiles[]{ "Flashbang", "HE Grenade", "Breach Charge", "Bump Mine", "Decoy Grenade", "Molotov", "TA Grenade", "Smoke Grenade", "Snowball" };

        for (int i = 0; i < IM_ARRAYSIZE(projectiles); i++) {
            bool isSelected = currentCategory == 3 && currentItem == i;

            if (ImGui::Selectable(projectiles[i], isSelected)) {
                currentItem = i;
                currentCategory = 3;
            }
        }

        ImGui::PopFont();
        ImGui::PopID();
        ImGui::Unindent();

        ImGui::Text("Danger Zone");
        ImGui::Indent();
        ImGui::PushID("Danger Zone");
        ImGui::PushFont(fonts.segoeui);
        static constexpr const char* dangerZone[]{ "Sentries", "Drones", "Cash", "Cash Dufflebag", "Pistol Case", "Light Case", "Heavy Case", "Explosive Case", "Tools Case", "Full Armor", "Armor", "Helmet", "Parachute", "Briefcase", "Tablet Upgrade", "ExoJump", "Ammobox", "Radar Jammer" };

        for (int i = 0; i < IM_ARRAYSIZE(dangerZone); i++) {
            bool isSelected = currentCategory == 4 && currentItem == i;

            if (ImGui::Selectable(dangerZone[i], isSelected)) {
                currentItem = i;
                currentCategory = 4;
            }
        }

        ImGui::PopFont();
        ImGui::PopID();
        ImGui::ListBoxFooter();
    }
    ImGui::SameLine();
    if (ImGui::BeginChild("##child", { 400.0f, 0.0f })) {
        switch (currentCategory) {
        case 0:
        case 1: {
            int selected = currentCategory * 3 + currentItem;
            ImGui::Checkbox("Enabled", &config->esp.players[selected].enabled);
            ImGui::SameLine(0.0f, 50.0f);
            ImGui::SetNextItemWidth(85.0f);
            ImGui::InputInt("Font", &config->esp.players[selected].font, 1, 294);
            config->esp.players[selected].font = std::clamp(config->esp.players[selected].font, 1, 294);

            ImGui::Separator();

            constexpr auto spacing{ 200.0f };
            ImGuiCustom::colorPicker("Snaplines", config->esp.players[selected].snaplines);
            ImGui::SameLine(spacing);
            ImGuiCustom::colorPicker("Box", config->esp.players[selected].box);
            ImGui::SameLine();
            ImGui::SetNextItemWidth(95.0f);
            ImGui::Combo("", &config->esp.players[selected].boxType, "2D\0""2D corners\0""3D\0""3D corners\0");
            ImGuiCustom::colorPicker("Eye traces", config->esp.players[selected].eyeTraces);
            ImGui::SameLine(spacing);
            ImGuiCustom::colorPicker("Health", config->esp.players[selected].health);
            ImGuiCustom::colorPicker("Head dot", config->esp.players[selected].headDot);
            ImGui::SameLine(spacing);
            ImGuiCustom::colorPicker("Health bar", config->esp.players[selected].healthBar);
            ImGuiCustom::colorPicker("Name", config->esp.players[selected].name);
            ImGui::SameLine(spacing);
            ImGuiCustom::colorPicker("Armor", config->esp.players[selected].armor);
            ImGuiCustom::colorPicker("Money", config->esp.players[selected].money);
            ImGui::SameLine(spacing);
            ImGuiCustom::colorPicker("Armor bar", config->esp.players[selected].armorBar);
            ImGuiCustom::colorPicker("Outline", config->esp.players[selected].outline);
            ImGui::SameLine(spacing);
            ImGuiCustom::colorPicker("Distance", config->esp.players[selected].distance);
            ImGuiCustom::colorPicker("Active Weapon", config->esp.players[selected].activeWeapon);
            ImGui::SameLine(spacing);
            ImGui::Checkbox("Dead ESP", &config->esp.players[selected].deadesp);
            ImGui::Checkbox("Sound ESP", &config->esp.players[selected].soundEsp);
            ImGui::SliderFloat("Max distance", &config->esp.players[selected].maxDistance, 0.0f, 200.0f, "%.2fm");
            break;
        }
        case 2: {
            ImGui::Checkbox("Enabled", &config->esp.weapon.enabled);
            ImGui::SameLine(0.0f, 50.0f);
            ImGui::SetNextItemWidth(85.0f);
            ImGui::InputInt("Font", &config->esp.weapon.font, 1, 294);
            config->esp.weapon.font = std::clamp(config->esp.weapon.font, 1, 294);

            ImGui::Separator();

            constexpr auto spacing{ 200.0f };
            ImGuiCustom::colorPicker("Snaplines", config->esp.weapon.snaplines);
            ImGui::SameLine(spacing);
            ImGuiCustom::colorPicker("Box", config->esp.weapon.box);
            ImGui::SameLine();
            ImGui::SetNextItemWidth(95.0f);
            ImGui::Combo("", &config->esp.weapon.boxType, "2D\0""2D corners\0""3D\0""3D corners\0");
            ImGuiCustom::colorPicker("Name", config->esp.weapon.name);
            ImGui::SameLine(spacing);
            ImGuiCustom::colorPicker("Outline", config->esp.weapon.outline);
            ImGuiCustom::colorPicker("Distance", config->esp.weapon.distance);
            ImGui::SliderFloat("Max distance", &config->esp.weapon.maxDistance, 0.0f, 200.0f, "%.2fm");
            break;
        }
        case 3: {
            ImGui::Checkbox("Enabled", &config->esp.projectiles[currentItem].enabled);
            ImGui::SameLine(0.0f, 50.0f);
            ImGui::SetNextItemWidth(85.0f);
            ImGui::InputInt("Font", &config->esp.projectiles[currentItem].font, 1, 294);
            config->esp.projectiles[currentItem].font = std::clamp(config->esp.projectiles[currentItem].font, 1, 294);

            ImGui::Separator();

            constexpr auto spacing{ 200.0f };
            ImGuiCustom::colorPicker("Snaplines", config->esp.projectiles[currentItem].snaplines);
            ImGui::SameLine(spacing);
            ImGuiCustom::colorPicker("Box", config->esp.projectiles[currentItem].box);
            ImGui::SameLine();
            ImGui::SetNextItemWidth(95.0f);
            ImGui::Combo("", &config->esp.projectiles[currentItem].boxType, "2D\0""2D corners\0""3D\0""3D corners\0");
            ImGuiCustom::colorPicker("Name", config->esp.projectiles[currentItem].name);
            ImGui::SameLine(spacing);
            ImGuiCustom::colorPicker("Outline", config->esp.projectiles[currentItem].outline);
            ImGuiCustom::colorPicker("Distance", config->esp.projectiles[currentItem].distance);
            ImGui::SliderFloat("Max distance", &config->esp.projectiles[currentItem].maxDistance, 0.0f, 200.0f, "%.2fm");
            break;
        }
        case 4: {
            int selected = currentItem;
            ImGui::Checkbox("Enabled", &config->esp.dangerZone[selected].enabled);
            ImGui::SameLine(0.0f, 50.0f);
            ImGui::SetNextItemWidth(85.0f);
            ImGui::InputInt("Font", &config->esp.dangerZone[selected].font, 1, 294);
            config->esp.dangerZone[selected].font = std::clamp(config->esp.dangerZone[selected].font, 1, 294);

            ImGui::Separator();

            constexpr auto spacing{ 200.0f };
            ImGuiCustom::colorPicker("Snaplines", config->esp.dangerZone[selected].snaplines);
            ImGui::SameLine(spacing);
            ImGuiCustom::colorPicker("Box", config->esp.dangerZone[selected].box);
            ImGui::SameLine();
            ImGui::SetNextItemWidth(95.0f);
            ImGui::Combo("", &config->esp.dangerZone[selected].boxType, "2D\0""2D corners\0""3D\0""3D corners\0");
            ImGuiCustom::colorPicker("Name", config->esp.dangerZone[selected].name);
            ImGui::SameLine(spacing);
            ImGuiCustom::colorPicker("Outline", config->esp.dangerZone[selected].outline);
            ImGuiCustom::colorPicker("Distance", config->esp.dangerZone[selected].distance);
            ImGui::SliderFloat("Max distance", &config->esp.dangerZone[selected].maxDistance, 0.0f, 200.0f, "%.2fm");
            break;
        }
        }

        ImGui::EndChild();
    }

    if (!contentOnly)
        ImGui::End();
}

void GUI::renderVisualsWindow(bool contentOnly) noexcept
{
    if (!contentOnly) {
        if (!window.visuals)
            return;
        ImGui::SetNextWindowSize({ 680.0f, 0.0f });
        ImGui::Begin("Visuals", &window.visuals, windowFlags);
    }
    ImGui::Columns(2, nullptr, false);
    ImGui::SetColumnOffset(1, 280.0f);
    ImGui::Combo("T Player Model", &config->visuals.playerModelT, "Default\0Special Agent Ava | FBI\0Operator | FBI SWAT\0Markus Delrow | FBI HRT\0Michael Syfers | FBI Sniper\0B Squadron Officer | SAS\0Seal Team 6 Soldier | NSWC SEAL\0Buckshot | NSWC SEAL\0Lt. Commander Ricksaw | NSWC SEAL\0Third Commando Company | KSK\0'Two Times' McCoy | USAF TACP\0Dragomir | Sabre\0Rezan The Ready | Sabre\0'The Doctor' Romanov | Sabre\0Maximus | Sabre\0Blackwolf | Sabre\0The Elite Mr. Muhlik | Elite Crew\0Ground Rebel | Elite Crew\0Osiris | Elite Crew\0Prof. Shahmat | Elite Crew\0Enforcer | Phoenix\0Slingshot | Phoenix\0Soldier | Phoenix\0");
    ImGui::Combo("CT Player Model", &config->visuals.playerModelCT, "Default\0Special Agent Ava | FBI\0Operator | FBI SWAT\0Markus Delrow | FBI HRT\0Michael Syfers | FBI Sniper\0B Squadron Officer | SAS\0Seal Team 6 Soldier | NSWC SEAL\0Buckshot | NSWC SEAL\0Lt. Commander Ricksaw | NSWC SEAL\0Third Commando Company | KSK\0'Two Times' McCoy | USAF TACP\0Dragomir | Sabre\0Rezan The Ready | Sabre\0'The Doctor' Romanov | Sabre\0Maximus | Sabre\0Blackwolf | Sabre\0The Elite Mr. Muhlik | Elite Crew\0Ground Rebel | Elite Crew\0Osiris | Elite Crew\0Prof. Shahmat | Elite Crew\0Enforcer | Phoenix\0Slingshot | Phoenix\0Soldier | Phoenix\0");
    ImGui::Checkbox("Disable post-processing", &config->visuals.disablePostProcessing);
    ImGui::Checkbox("Inverse ragdoll gravity", &config->visuals.inverseRagdollGravity);
    ImGui::Checkbox("No fog", &config->visuals.noFog);
    ImGui::Checkbox("No 3d sky", &config->visuals.no3dSky);
    ImGui::Checkbox("No aim punch", &config->visuals.noAimPunch);
    ImGui::Checkbox("No view punch", &config->visuals.noViewPunch);
    ImGui::Checkbox("No hands", &config->visuals.noHands);
    ImGui::Checkbox("No sleeves", &config->visuals.noSleeves);
    ImGui::Checkbox("No weapons", &config->visuals.noWeapons);
    ImGui::Checkbox("No smoke", &config->visuals.noSmoke);
    ImGui::Checkbox("No blur", &config->visuals.noBlur);
    ImGui::Checkbox("No scope overlay", &config->visuals.noScopeOverlay);
    ImGui::Checkbox("No grass", &config->visuals.noGrass);
    ImGui::Checkbox("No shadows", &config->visuals.noShadows);
    ImGui::Checkbox("Wireframe smoke", &config->visuals.wireframeSmoke);
    ImGui::NextColumn();
    ImGui::Checkbox("Zoom", &config->visuals.zoom);
    ImGui::SameLine();
    hotkey(config->visuals.zoomKey);
    ImGui::Checkbox("Thirdperson", &config->visuals.thirdperson);
    ImGui::SameLine();
    hotkey(config->visuals.thirdpersonKey);
    ImGui::Checkbox("Thirdperson spectate", &config->visuals.thirdpersonspectate);
    ImGui::PushItemWidth(290.0f);
    ImGui::PushID(0);
    ImGui::SliderInt("", &config->visuals.thirdpersonDistance, 0, 1000, "Thirdperson distance: %d");
    ImGui::PopID();
    ImGui::PushID(1);
    ImGui::SliderInt("", &config->visuals.viewmodelFov, -60, 60, "Viewmodel FOV: %d");
    ImGui::PopID();
    ImGui::PushID(2);
    ImGui::SliderInt("", &config->visuals.fov, -60, 60, "FOV: %d");
    ImGui::PopID();
    ImGui::PushID(3);
    ImGui::SliderInt("", &config->visuals.farZ, 0, 2000, "Far Z: %d");
    ImGui::PopID();
    ImGui::PushID(4);
    ImGui::SliderInt("", &config->visuals.flashReduction, 0, 100, "Flash reduction: %d%%");
    ImGui::PopID();
    ImGui::PushID(5);
    ImGui::SliderFloat("", &config->visuals.brightness, 0.0f, 1.0f, "Brightness: %.2f");
    ImGui::PopID();
    ImGui::PopItemWidth();
    ImGui::Combo("Skybox", &config->visuals.skybox, "Default\0cs_baggage_skybox_\0cs_tibet\0embassy\0italy\0jungle\0nukeblank\0office\0sky_cs15_daylight01_hdr\0sky_cs15_daylight02_hdr\0sky_cs15_daylight03_hdr\0sky_cs15_daylight04_hdr\0sky_csgo_cloudy01\0sky_csgo_night_flat\0sky_csgo_night02\0sky_day02_05_hdr\0sky_day02_05\0sky_dust\0sky_l4d_rural02_ldr\0sky_venice\0vertigo_hdr\0vertigo\0vertigoblue_hdr\0vietnam\0");
    ImGuiCustom::colorPicker("World color", config->visuals.world);
    ImGuiCustom::colorPicker("Sky color", config->visuals.sky);
    ImGui::Checkbox("Deagle spinner", &config->visuals.deagleSpinner);
    ImGui::Combo("Screen effect", &config->visuals.screenEffect, "None\0Drone cam\0Drone cam with noise\0Underwater\0Healthboost\0Dangerzone\0");
    ImGui::Combo("Hit effect", &config->visuals.hitEffect, "None\0Drone cam\0Drone cam with noise\0Underwater\0Healthboost\0Dangerzone\0");
    ImGui::SliderFloat("Hit effect time", &config->visuals.hitEffectTime, 0.1f, 1.5f, "%.2fs");
    ImGui::Combo("Hit marker", &config->visuals.hitMarker, "None\0Default (Cross)\0");
    ImGui::SliderFloat("Hit marker time", &config->visuals.hitMarkerTime, 0.1f, 1.5f, "%.2fs");
    ImGui::Checkbox("Color correction", &config->visuals.colorCorrection.enabled);
    ImGui::SameLine();
    bool ccPopup = ImGui::Button("Edit");

    if (ccPopup)
        ImGui::OpenPopup("##popup");

    if (ImGui::BeginPopup("##popup")) {
        ImGui::VSliderFloat("##1", { 40.0f, 160.0f }, &config->visuals.colorCorrection.blue, 0.0f, 1.0f, "Blue\n%.3f"); ImGui::SameLine();
        ImGui::VSliderFloat("##2", { 40.0f, 160.0f }, &config->visuals.colorCorrection.red, 0.0f, 1.0f, "Red\n%.3f"); ImGui::SameLine();
        ImGui::VSliderFloat("##3", { 40.0f, 160.0f }, &config->visuals.colorCorrection.mono, 0.0f, 1.0f, "Mono\n%.3f"); ImGui::SameLine();
        ImGui::VSliderFloat("##4", { 40.0f, 160.0f }, &config->visuals.colorCorrection.saturation, 0.0f, 1.0f, "Sat\n%.3f"); ImGui::SameLine();
        ImGui::VSliderFloat("##5", { 40.0f, 160.0f }, &config->visuals.colorCorrection.ghost, 0.0f, 1.0f, "Ghost\n%.3f"); ImGui::SameLine();
        ImGui::VSliderFloat("##6", { 40.0f, 160.0f }, &config->visuals.colorCorrection.green, 0.0f, 1.0f, "Green\n%.3f"); ImGui::SameLine();
        ImGui::VSliderFloat("##7", { 40.0f, 160.0f }, &config->visuals.colorCorrection.yellow, 0.0f, 1.0f, "Yellow\n%.3f"); ImGui::SameLine();
        ImGui::EndPopup();
    }
    ImGui::Columns(1);

    if (!contentOnly)
        ImGui::End();
}

void GUI::renderSkinChangerWindow(bool contentOnly) noexcept
{
    if (!contentOnly) {
        if (!window.skinChanger)
            return;
        ImGui::SetNextWindowSize({ 700.0f, 0.0f });
        ImGui::Begin("skinchanger", &window.skinChanger, windowFlags);
    }

    static auto itemIndex = 0;

    ImGui::PushItemWidth(110.0f);
    ImGui::Combo("##1", &itemIndex, [](void* data, int idx, const char** out_text) {
        *out_text = game_data::weapon_names[idx].name;
        return true;
        }, nullptr, IM_ARRAYSIZE(game_data::weapon_names), 5);
    ImGui::PopItemWidth();

    auto& selected_entry = config->skinChanger[itemIndex];
    selected_entry.itemIdIndex = itemIndex;

    {
        ImGui::SameLine();
        ImGui::Checkbox("Enabled", &selected_entry.enabled);
        ImGui::Separator();
        ImGui::Columns(2, nullptr, false);
        ImGui::InputInt("Seed", &selected_entry.seed);
        ImGui::InputInt("StatTrak", &selected_entry.stat_trak);
        ImGui::SliderFloat("Wear", &selected_entry.wear, FLT_MIN, 1.f, "%.10f", 5);

        ImGui::Combo("Paint Kit", &selected_entry.paint_kit_vector_index, [](void* data, int idx, const char** out_text) {
            *out_text = (itemIndex == 1 ? SkinChanger::gloveKits : SkinChanger::skinKits)[idx].name.c_str();
            return true;
            }, nullptr, (itemIndex == 1 ? SkinChanger::gloveKits : SkinChanger::skinKits).size(), 10);

        ImGui::Combo("Quality", &selected_entry.entity_quality_vector_index, [](void* data, int idx, const char** out_text) {
            *out_text = game_data::quality_names[idx].name;
            return true;
            }, nullptr, IM_ARRAYSIZE(game_data::quality_names), 5);

        if (itemIndex == 0) {
            ImGui::Combo("Knife", &selected_entry.definition_override_vector_index, [](void* data, int idx, const char** out_text) {
                *out_text = game_data::knife_names[idx].name;
                return true;
                }, nullptr, IM_ARRAYSIZE(game_data::knife_names), 5);
        } else if (itemIndex == 1) {
            ImGui::Combo("Glove", &selected_entry.definition_override_vector_index, [](void* data, int idx, const char** out_text) {
                *out_text = game_data::glove_names[idx].name;
                return true;
                }, nullptr, IM_ARRAYSIZE(game_data::glove_names), 5);
        } else {
            static auto unused_value = 0;
            selected_entry.definition_override_vector_index = 0;
            ImGui::Combo("Unavailable", &unused_value, "For knives or gloves\0");
        }

        ImGui::InputText("Name Tag", selected_entry.custom_name, 32);
    }

    ImGui::NextColumn();

    {
        ImGui::PushID("sticker");

        static auto selectedStickerSlot = 0;

        ImGui::PushItemWidth(-1);

        if (ImGui::ListBoxHeader("", 5)) {
            for (int i = 0; i < 5; ++i) {
                ImGui::PushID(i);

                const auto kit_vector_index = config->skinChanger[itemIndex].stickers[i].kit_vector_index;
                const std::string text = '#' + std::to_string(i + 1) + "  " + SkinChanger::stickerKits[kit_vector_index].name;

                if (ImGui::Selectable(text.c_str(), i == selectedStickerSlot))
                    selectedStickerSlot = i;

                ImGui::PopID();
            }
            ImGui::ListBoxFooter();
        }

        ImGui::PopItemWidth();

        auto& selected_sticker = selected_entry.stickers[selectedStickerSlot];

        ImGui::Combo("Sticker Kit", &selected_sticker.kit_vector_index, [](void* data, int idx, const char** out_text) {
            *out_text = SkinChanger::stickerKits[idx].name.c_str();
            return true;
            }, nullptr, SkinChanger::stickerKits.size(), 10);

        ImGui::SliderFloat("Wear", &selected_sticker.wear, FLT_MIN, 1.0f, "%.10f", 5.0f);
        ImGui::SliderFloat("Scale", &selected_sticker.scale, 0.1f, 5.0f);
        ImGui::SliderFloat("Rotation", &selected_sticker.rotation, 0.0f, 360.0f);

        ImGui::PopID();
    }
    selected_entry.update();

    ImGui::Columns(1);

    ImGui::Separator();

    if (ImGui::Button("Update", { 130.0f, 30.0f }))
        SkinChanger::scheduleHudUpdate();

    //ImGui::TextUnformatted("nSkinz by namazso");

    if (!contentOnly)
        ImGui::End();
}

void GUI::renderSoundWindow(bool contentOnly) noexcept
{
    if (!contentOnly) {
        if (!window.sound)
            return;
        ImGui::SetNextWindowSize({ 0.0f, 0.0f });
        ImGui::Begin("Sound", &window.sound, windowFlags);
    }
    ImGui::SliderInt("Chicken volume", &config->sound.chickenVolume, 0, 200, "%d%%");

    static int currentCategory{ 0 };
    ImGui::PushItemWidth(110.0f);
    ImGui::Combo("", &currentCategory, "Local player\0Allies\0Enemies\0");
    ImGui::PopItemWidth();
    ImGui::SliderInt("Master volume", &config->sound.players[currentCategory].masterVolume, 0, 200, "%d%%");
    ImGui::SliderInt("Headshot volume", &config->sound.players[currentCategory].headshotVolume, 0, 200, "%d%%");
    ImGui::SliderInt("Weapon volume", &config->sound.players[currentCategory].weaponVolume, 0, 200, "%d%%");
    ImGui::SliderInt("Footstep volume", &config->sound.players[currentCategory].footstepVolume, 0, 200, "%d%%");

    if (!contentOnly)
        ImGui::End();
}

void GUI::renderStyleWindow(bool contentOnly) noexcept
{
    if (!contentOnly) {
        if (!window.style)
            return;
        ImGui::SetNextWindowSize({ 0.0f, 0.0f });
        ImGui::Begin("Style", &window.style, windowFlags);
    }

    ImGui::PushItemWidth(150.0f);
    if (ImGui::Combo("Menu style", &config->style.menuStyle, "Classic\0One window\0"))
        window = { };
    if (ImGui::Combo("Menu colors", &config->style.menuColors, "Dark\0Light\0Classic\0Custom\0"))
        updateColors();
    ImGui::PopItemWidth();

    if (config->style.menuColors == 3) {
        ImGuiStyle& style = ImGui::GetStyle();
        for (int i = 0; i < ImGuiCol_COUNT; i++) {
            if (i && i & 3) ImGui::SameLine(220.0f * (i & 3));

            ImGuiCustom::colorPicker(ImGui::GetStyleColorName(i), (float*)&style.Colors[i]);
        }
    }

    if (!contentOnly)
        ImGui::End();
}

void GUI::renderMiscWindow(bool contentOnly) noexcept
{
    if (!contentOnly) {
        if (!window.misc)
            return;
        ImGui::SetNextWindowSize({ 580.0f, 0.0f });
        ImGui::Begin("Misc", &window.misc, windowFlags);
    }
    ImGui::Columns(2, nullptr, false);
    ImGui::SetColumnOffset(1, 230.0f);
    ImGui::TextUnformatted("Menu key");
    ImGui::SameLine();
    hotkey(config->misc.menuKey);

    ImGui::Checkbox("Anti AFK kick", &config->misc.antiAfkKick);
    ImGui::Checkbox("Auto strafe", &config->misc.autoStrafe);
    ImGui::Checkbox("Bunny hop", &config->misc.bunnyHop);
    ImGui::Checkbox("Fast duck", &config->misc.fastDuck);
    ImGui::Checkbox("Moonwalk", &config->misc.moonwalk);
    ImGui::Checkbox("Edge Jump", &config->misc.edgejump);
    ImGui::SameLine();
    hotkey(config->misc.edgejumpkey);
    ImGui::Checkbox("Slowwalk", &config->misc.slowwalk);
    ImGui::SameLine();
    hotkey(config->misc.slowwalkKey);
    ImGui::Checkbox("Sniper crosshair", &config->misc.sniperCrosshair);
    ImGui::Checkbox("Recoil crosshair", &config->misc.recoilCrosshair);
    ImGui::Checkbox("Auto pistol", &config->misc.autoPistol);
    ImGui::Checkbox("Auto reload", &config->misc.autoReload);
    ImGui::Checkbox("Auto accept", &config->misc.autoAccept);
    ImGui::Checkbox("Radar hack", &config->misc.radarHack);
    ImGui::Checkbox("Reveal ranks", &config->misc.revealRanks);
    ImGui::Checkbox("Reveal money", &config->misc.revealMoney);
    ImGui::Checkbox("Reveal suspect", &config->misc.revealSuspect);
    ImGuiCustom::colorPicker("Spectator list", config->misc.spectatorList);
    //ImGuiCustom::colorPicker("Watermark", config->misc.watermark);
    ImGui::Checkbox("Fix animation LOD", &config->misc.fixAnimationLOD);
    ImGui::Checkbox("Fix bone matrix", &config->misc.fixBoneMatrix);
    ImGui::Checkbox("Fix movement", &config->misc.fixMovement);
    ImGui::Checkbox("Disable model occlusion", &config->misc.disableModelOcclusion);
    ImGui::SliderFloat("Aspect Ratio", &config->misc.aspectratio, 0.0f, 5.0f, "%.2f");
    ImGui::NextColumn();
    ImGui::Checkbox("Disable HUD blur", &config->misc.disablePanoramablur);
    ImGui::Checkbox("Animated clan tag", &config->misc.animatedClanTag);
    ImGui::Checkbox("Clock tag", &config->misc.clocktag);
    ImGui::Checkbox("Clantag", &config->misc.customClanTag);
    if (config->misc.customClanTag)
    {
        ImGui::Combo("", &config->misc.clantagtype, "Atomware\0ValvE\0Alpha & Omega\0");
    }
    ImGui::Checkbox("Kill message", &config->misc.killMessage);
    ImGui::SameLine();
    ImGui::PushItemWidth(120.0f);
    ImGui::PushID(1);
    ImGui::InputText("", &config->misc.killMessageString);
    ImGui::PopID();
    ImGui::Checkbox("Name stealer", &config->misc.nameStealer);
    ImGui::Checkbox("Fast plant", &config->misc.fastPlant);
    ImGuiCustom::colorPicker("Bomb timer", config->misc.bombTimer);
    ImGui::Checkbox("Quick reload", &config->misc.quickReload);
    ImGui::Checkbox("Prepare revolver", &config->misc.prepareRevolver);
    ImGui::SameLine();
    hotkey(config->misc.prepareRevolverKey);
    ImGui::Combo("Hit Sound", &config->misc.hitSound, "None\0Metal\0Gamesense\0Bell\0Glass\0");
    ImGui::Text("Quick healthshot");
    ImGui::SameLine();
    hotkey(config->misc.quickHealthshotKey);
    ImGui::Checkbox("Grenade Prediction", &config->misc.nadePredict);
    ImGui::Checkbox("Fix tablet signal", &config->misc.fixTabletSignal);
    ImGuiCustom::colorPicker("Eventlog", config->misc.eventlogcolor);
    ImGui::SetNextItemWidth(120.0f);
    ImGui::SliderFloat("Max angle delta", &config->misc.maxAngleDelta, 0.0f, 255.0f, "%.2f");

    if (ImGui::Button("Unhook"))
        hooks->uninstall();

    ImGui::Columns(1);
    if (!contentOnly)
        ImGui::End();
}

void GUI::renderTrollWindow(bool contentOnly) noexcept
{
    if (!contentOnly) {
        if (!window.troll)
            return;
        ImGui::SetNextWindowSize({ 0.0f, 0.0f });
        ImGui::Begin("Troll", &window.troll, windowFlags);
    }

    ImGui::Checkbox("Fake prime", &config->misc.fakePrime);
    ImGui::NextColumn();
    //ImGui::Checkbox("Blockbot", &config->misc.playerBlocker);
    //ImGui::SameLine();
    //hotkey(config.misc.playerBlockerKey);
    ImGui::Columns(1);
    //ImGui::Checkbox("Door spam", &config->misc.doorspam);
    //ImGui::SameLine();
    //hotkey(config.misc.doorspamKey);
    ImGui::NextColumn();
    //ImGui::Checkbox("Airstuck", &config.misc.airstuck);
    //ImGui::SameLine();
    //hotkey(config.misc.airstuckkey);
    ImGui::Separator();
    ImGui::TextColored(ImColor(255, 255, 255), "Fake ban");
    ImGui::Separator();
    ImGui::PushID(0);
    ImGui::InputText("", &config->misc.banText);
    ImGui::PopID();
    ImGui::SameLine();
    if (ImGui::Button("Fake ban"))
        Misc::fakeBan(true);
    ImGui::Separator();
    ImGui::TextColored(ImColor(255, 255, 255), "Fake Unbox");
    ImGui::Separator();
    ImGui::PushID(1);
    ImGui::InputText("", &config->misc.unboxname);
    ImGui::PopID();
    ImGui::PushID(2);
    ImGui::InputText("", &config->misc.unboxskin);
    ImGui::PopID();
    //ImGui::SameLine();
    //ImGui::Checkbox("Star", &config.misc.unboxstar);
    if (ImGui::Button("Fake unbox"))
        Misc::fakeUnbox(true);
    ImGui::Separator();
    ImGui::PushID(3);
    ImGui::InputText("", &config->misc.restorename);
    ImGui::PopID();
    if (ImGui::Button("Restore name"))
        Misc::restoreName(true);
    ImGui::Separator();

    ImGui::Columns(1);
    if (!contentOnly)
        ImGui::End();
}

void GUI::renderReportbotWindow(bool contentOnly) noexcept
{
    if (!contentOnly) {
        if (!window.reportbot)
            return;
        ImGui::SetNextWindowSize({ 0.0f, 0.0f });
        ImGui::Begin("Reportbot", &window.reportbot, windowFlags);
    }
    ImGui::Checkbox("Enabled", &config->reportbot.enabled);
    ImGui::SameLine(0.0f, 50.0f);
    if (ImGui::Button("Reset"))
        Reportbot::reset();
    ImGui::Separator();
    ImGui::PushItemWidth(80.0f);
    ImGui::Combo("Target", &config->reportbot.target, "Enemies\0Allies\0All\0");
    ImGui::InputInt("Delay (s)", &config->reportbot.delay);
    config->reportbot.delay = (std::max)(config->reportbot.delay, 1);
    //ImGui::InputInt("Rounds", &config->reportbot.rounds);
    //config->reportbot.rounds = (std::max)(config->reportbot.rounds, 1);
    ImGui::PopItemWidth();
    ImGui::Checkbox("Abusive Communications", &config->reportbot.textAbuse);
    ImGui::Checkbox("Griefing", &config->reportbot.griefing);
    ImGui::Checkbox("Wall Hacking", &config->reportbot.wallhack);
    ImGui::Checkbox("Aim Hacking", &config->reportbot.aimbot);
    ImGui::Checkbox("Other Hacking", &config->reportbot.other);

    if (!contentOnly)
        ImGui::End();
}

void GUI::renderConfigWindow(bool contentOnly) noexcept
{
    if (!contentOnly) {
        if (!window.config)
            return;
        ImGui::SetNextWindowSize({ 290.0f, 190.0f });
        ImGui::Begin("Config", &window.config, windowFlags);
    }

    ImGui::Columns(2, nullptr, false);
    ImGui::SetColumnOffset(1, 170.0f);

    ImGui::PushItemWidth(160.0f);

    auto& configItems = config->getConfigs();
    static int currentConfig = -1;

    if (static_cast<size_t>(currentConfig) >= configItems.size())
        currentConfig = -1;

    static std::string buffer;

    if (ImGui::ListBox("", &currentConfig, [](void* data, int idx, const char** out_text) {
        auto& vector = *static_cast<std::vector<std::string>*>(data);
        *out_text = vector[idx].c_str();
        return true;
        }, &configItems, configItems.size(), 5) && currentConfig != -1)
            buffer = configItems[currentConfig];

        ImGui::PushID(0);
        if (ImGui::InputText("", &buffer, ImGuiInputTextFlags_EnterReturnsTrue)) {
            if (currentConfig != -1)
                config->rename(currentConfig, buffer.c_str());
        }
        ImGui::PopID();
        ImGui::NextColumn();

        ImGui::PushItemWidth(100.0f);

        if (ImGui::Button("Create config", { 100.0f, 25.0f }))
            config->add(buffer.c_str());

        if (ImGui::Button("Reset config", { 100.0f, 25.0f }))
            ImGui::OpenPopup("Config to reset");

        if (ImGui::Button("Refresh", { 100.0f, 25.0f }))
            config->listConfigs();

        if (ImGui::BeginPopup("Config to reset")) {
            static constexpr const char* names[]{ "Whole", "Aimbot", "Triggerbot", "Backtrack", "Anti aim", "Glow", "Chams", "Esp", "Visuals", "Skin changer", "Sound", "Style", "Misc", "Reportbot" };
            for (int i = 0; i < IM_ARRAYSIZE(names); i++) {
                if (i == 1) ImGui::Separator();

                if (ImGui::Selectable(names[i])) {
                    switch (i) {
                    case 0: config->reset(); updateColors(); Misc::updateClanTag(true); SkinChanger::scheduleHudUpdate(); break;
                    case 1: config->aimbot = { }; break;
                    case 2: config->triggerbot = { }; break;
                    case 3: config->backtrack = { }; break;
                    case 4: config->antiAim = { }; break;
                    case 5: config->glow = { }; break;
                    case 6: config->chams = { }; break;
                    case 7: config->esp = { }; break;
                    case 8: config->visuals = { }; break;
                    case 9: config->skinChanger = { }; SkinChanger::scheduleHudUpdate(); break;
                    case 10: config->sound = { }; break;
                    case 11: config->style = { }; updateColors(); break;
                    case 12: config->misc = { };  Misc::updateClanTag(true); break;
                    case 13: config->reportbot = { }; break;
                    }
                }
            }
            ImGui::EndPopup();
        }
        if (currentConfig != -1) {
            if (ImGui::Button("Load selected", { 100.0f, 25.0f })) {
                config->load(currentConfig);
                updateColors();
                SkinChanger::scheduleHudUpdate();
                Misc::updateClanTag(true);
            }
            if (ImGui::Button("Save selected", { 100.0f, 25.0f }))
                config->save(currentConfig);
            if (ImGui::Button("Delete selected", { 100.0f, 25.0f }))
                config->remove(currentConfig);
        }
        ImGui::Columns(1);
        if (!contentOnly)
            ImGui::End();
}

ImVec2 prevwinpos;
int page = 0;

void BtnActiveTab()
{
    auto& style = ImGui::GetStyle();
    style.Colors[ImGuiCol_Button] = ImColor(52, 57, 68, 255);
    style.Colors[ImGuiCol_Border] = ImVec4(27 / 255.f, 30 / 255.f, 35 / 255.f, 0.f);
    style.Colors[ImGuiCol_ButtonHovered] = ImColor(52, 57, 68, 255);
    style.Colors[ImGuiCol_ButtonActive] = ImColor(52, 57, 68, 255);
    style.Colors[ImGuiCol_Text] = ImColor(219, 222, 229, 255);
}


void BtnNormalTab()
{
    auto& style = ImGui::GetStyle();
    style.Colors[ImGuiCol_Button] = ImColor(0, 0, 0, 0);
    style.Colors[ImGuiCol_Border] = ImVec4(27 / 255.f, 30 / 255.f, 35 / 255.f, 0.f);
    style.Colors[ImGuiCol_ButtonHovered] = ImColor(0, 0, 0, 0);
    style.Colors[ImGuiCol_ButtonActive] = ImColor(0, 0, 0, 0);
    style.Colors[ImGuiCol_Text] = ImColor(219, 222, 229, 255);

}

void GUI::renderGuiStyle2() noexcept
{
    ImGuiStyle& style = ImGui::GetStyle();
    style.Colors[ImGuiCol_Text] = ImVec4(0.90f, 0.90f, 0.90f, 1.00f);
    style.Colors[ImGuiCol_TextDisabled] = ImVec4(0.90f, 0.90f, 0.90f, 1.00f);
    //style.Colors[ImGuiCol_WindowBg] = ImVec4(0.29f, 0.29f, 0.29f, 1.00f);
    style.Colors[ImGuiCol_WindowBg] = ImColor(21, 21, 21);//ImColor(31, 25, 66);
    //style.Colors[ImGuiCol_ChildWindowBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    style.Colors[ImGuiCol_PopupBg] = ImVec4(0.05f, 0.05f, 0.10f, 0.90f);
    style.Colors[ImGuiCol_Border] = ImColor(34, 34, 34);//ImColor(112, 149, 46);//ImColor(200, 7, 85);//ImVec4(0.70f, 0.70f, 0.70f, 0.40f);
    style.Colors[ImGuiCol_BorderShadow] = ImColor(34, 34, 34);//ImColor(112, 149, 46);//ImColor(200, 7, 85);//ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    style.Colors[ImGuiCol_FrameBg] = ImColor(34, 34, 34);//ImColor(112, 149, 46);//ImColor(200, 7, 85);//ImVec4(0.54f, 0.54f, 0.54f, 1.00f);
    style.Colors[ImGuiCol_FrameBgHovered] = ImColor(34, 34, 34);//ImColor(112, 149, 46);//ImColor(200, 7, 85);//ImVec4(0.54f, 0.54f, 0.54f, 1.00f);
    style.Colors[ImGuiCol_FrameBgActive] = ImColor(34, 34, 34);//ImColor(112, 149, 46);//ImColor(200, 7, 85);//ImVec4(0.54f, 0.54f, 0.54f, 1.00f);
    style.Colors[ImGuiCol_TitleBg] = ImVec4(0.29f, 0.29f, 0.29f, 1.00f);
    style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.29f, 0.29f, 0.29f, 1.00f);
    style.Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.29f, 0.29f, 0.29f, 1.00f);
    style.Colors[ImGuiCol_MenuBarBg] = ImColor(34, 34, 34);//ImColor(112, 149, 46);//ImColor(200, 7, 85);//ImVec4(0.39f, 0.39f, 0.39f, 1.00f);
    style.Colors[ImGuiCol_ScrollbarBg] = ImColor(34, 34, 34);//ImColor(112, 149, 46);//ImColor(200, 7, 85);//ImVec4(0.39f, 0.39f, 0.39f, 1.00f);
    style.Colors[ImGuiCol_ScrollbarGrab] = ImColor(112, 149, 46);//ImColor(173, 230, 170);//ImVec4(0.65f, 0.65f, 0.65f, 1.00f);
    style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImColor(112, 149, 46);//ImColor(173, 230, 170);//ImVec4(0.65f, 0.65f, 0.65f, 1.00f);
    style.Colors[ImGuiCol_ScrollbarGrabActive] = ImColor(112, 149, 46);//ImColor(173, 230, 170);//ImVec4(0.65f, 0.65f, 0.65f, 1.00f);
    //style.Colors[ImGuiCol_ComboBg] = ImColor(200, 7, 85);//ImVec4(0.20f, 0.20f, 0.20f, 0.99f);
    style.Colors[ImGuiCol_CheckMark] = ImColor(112, 149, 46);//ImVec4(1.f, 1.f, 1.f, 1.00f);
    style.Colors[ImGuiCol_SliderGrab] = ImColor(112, 149, 46);//ImColor(173, 230, 170);//ImVec4(0.65f, 0.65f, 0.65f, 0.30f);
    style.Colors[ImGuiCol_SliderGrabActive] = ImColor(112, 149, 46);//ImColor(173, 230, 170);//ImVec4(0.65f, 0.65f, 0.65f, 1.00f);
    style.Colors[ImGuiCol_Button] = ImColor(34, 34, 34);//ImColor(200, 7, 85);//ImVec4(0.54f, 0.54f, 0.54f, 1.00f);
    style.Colors[ImGuiCol_ButtonHovered] = ImColor(34, 34, 34);//ImColor(200, 7, 85);//ImVec4(0.54f, 0.54f, 0.54f, 1.00f);
    style.Colors[ImGuiCol_ButtonActive] = ImColor(34, 34, 34);//ImColor(200, 7, 85);//ImVec4(0.67f, 0.67f, 0.67f, 1.00f);

    style.Alpha = 1.0f;
    style.WindowPadding = ImVec2(8, 8);
    style.WindowRounding = 0.0f;
    style.WindowTitleAlign = ImVec2(0.5f, 0.5f);
    style.FramePadding = ImVec2(4, 3);
    style.FrameRounding = 0.0f;
    style.ItemSpacing = ImVec2(8, 4);
    style.ItemInnerSpacing = ImVec2(4, 4);
    style.TouchExtraPadding = ImVec2(0, 0);
    style.IndentSpacing = 21.0f;
    style.ColumnsMinSpacing = 6.0f;
    style.ScrollbarSize = 10.0f;
    style.ScrollbarRounding = 3.0f;
    style.GrabMinSize = 10.0f;
    style.GrabRounding = 0.0f;
    style.ButtonTextAlign = ImVec2(0.5f, 0.5f);
    style.DisplayWindowPadding = ImVec2(22, 22);
    style.DisplaySafeAreaPadding = ImVec2(4, 4);
    style.AntiAliasedLines = true;
    style.CurveTessellationTol = 1.25f;
    style.AntiAliasedLines = true;
    style.CurveTessellationTol = 1.25f;
    style.FramePadding.y = 0.5f;

    ImGui::SetNextWindowSize({ 1000.0f, 66.0f });
    if (ImGui::Begin("main", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoTitleBar))
    {
        ImGui::PushFont(fonts.indicators);
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
        ImVec2 p = ImGui::GetWindowPos();// idk why this is the same but btw 
        prevwinpos = p;
        ImGui::GetWindowDrawList()->AddRectFilledMultiColor(ImVec2(p.x, p.y), ImVec2(p.x + ImGui::GetWindowWidth(), p.y + 7), /*ImColor(menur, menug, menub, 255), ImColor(menur, menug, menub, 255), ImColor(menur, menug, menub, 255), ImColor(menur, menug, menub, 255)*/cRainbow, cRainbow, cRainbow, cRainbow);
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 14);
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 20);
        ImGui::Text("atomware.cf");
        ImGui::GetWindowDrawList()->AddRectFilledMultiColor(ImVec2(p.x + 160, p.y + 22), ImVec2(p.x + 161, p.y + 48), ImColor(48, 54, 64, 255), ImColor(48, 54, 64, 255), ImColor(48, 54, 64, 255), ImColor(48, 54, 64, 255)); // line
        ImGui::PushFont(fonts.segoeui);
        ImGui::SameLine(188);
        if (page == 0) BtnActiveTab(); else BtnNormalTab();
        if (ImGui::Button("aimbot", ImVec2(0, 25)))page = 0;

        ImGui::SameLine();

        if (page == 1) BtnActiveTab(); else BtnNormalTab();
        if (ImGui::Button("ragebot", ImVec2(0, 25)))page = 1;

        ImGui::SameLine();

        if (page == 2) BtnActiveTab(); else BtnNormalTab();
        if (ImGui::Button("glow", ImVec2(0, 25)))page = 2;


        ImGui::SameLine();

        if (page == 3) BtnActiveTab(); else BtnNormalTab();
        if (ImGui::Button("esp", ImVec2(0, 25)))page = 3;

        ImGui::SameLine();

        if (page == 4) BtnActiveTab(); else BtnNormalTab();
        if (ImGui::Button("chams", ImVec2(0, 25)))page = 4;


        ImGui::SameLine();

        if (page == 5) BtnActiveTab(); else BtnNormalTab();
        if (ImGui::Button("visuals", ImVec2(0, 25)))page = 5;

        ImGui::SameLine();

        if (page == 6) BtnActiveTab(); else BtnNormalTab();
        if (ImGui::Button("skins", ImVec2(0, 25)))page = 6;

        ImGui::SameLine();

        if (page == 7) BtnActiveTab(); else BtnNormalTab();
        if (ImGui::Button("sound", ImVec2(0, 25)))page = 7;

        ImGui::SameLine();

        if (page == 8) BtnActiveTab(); else BtnNormalTab();
        if (ImGui::Button("misc", ImVec2(0, 25)))page = 8;

        ImGui::SameLine();

        if (page == 9) BtnActiveTab(); else BtnNormalTab();
        if (ImGui::Button("troll", ImVec2(0, 25)))page = 9;

        ImGui::SameLine();

        if (page == 10) BtnActiveTab(); else BtnNormalTab();
        if (ImGui::Button("reportbot", ImVec2(0, 25)))page = 10;

        ImGui::SameLine();

        if (page == 11) BtnActiveTab(); else BtnNormalTab();
        if (ImGui::Button("config", ImVec2(0, 25)))page = 11;

        ImGui::SameLine();

        ImGui::SetNextWindowPos(ImVec2(prevwinpos.x, prevwinpos.y + 76));
    }

    ImGui::SetNextWindowSize({ 1000.0f, 0.0f });
    ImGui::PushFont(fonts.tahoma);
    ImGui::Begin("Atomware v1", nullptr, windowFlags | ImGuiWindowFlags_NoTitleBar);

    /*if (ImGui::BeginTabBar("TabBar", ImGuiTabBarFlags_Reorderable | ImGuiTabBarFlags_FittingPolicyScroll | ImGuiTabBarFlags_NoTooltip)) {
        if (ImGui::BeginTabItem("Aimbot")) {
            renderAimbotWindow(true);
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Anti aim")) {
            renderAntiAimWindow(true);
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Triggerbot")) {
            renderTriggerbotWindow(true);
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Backtrack")) {
            renderBacktrackWindow(true);
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Glow")) {
            renderGlowWindow(true);
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Chams")) {
            renderChamsWindow(true);
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Esp")) {
            renderEspWindow(true);
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Visuals")) {
            renderVisualsWindow(true);
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Skin changer")) {
            renderSkinChangerWindow(true);
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Sound")) {
            renderSoundWindow(true);
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Style")) {
            renderStyleWindow(true);
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Misc")) {
            renderMiscWindow(true);
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Reportbot")) {
            renderReportbotWindow(true);
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Config")) {
            renderConfigWindow(true);
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }*/

    switch (page)
    {
    case 0:
        renderAimbotWindow(true);
        break;
    case 1:
        renderAntiAimWindow(true);
        break;
    case 2:
        renderGlowWindow(true);
        break;
    case 3:
        renderEspWindow(true);
        break;
    case 4:
        renderChamsWindow(true);
        break;
    case 5:
        renderVisualsWindow(true);
        break;
    case 6:
        renderSkinChangerWindow(true);
        break;
    case 7:
        renderSoundWindow(true);
        break;
    case 8:
        renderMiscWindow(true);
        break;
    case 9:
        renderTrollWindow(true);
        break;
    case 10:
        renderReportbotWindow(true);
        break;
    case 11:
        renderConfigWindow(true);
        break;
    default:
        break;
    }

    ImGui::End();
}

// Junk Code By Peatreat & Thaisen's Gen
void xDDzoWVdWweYqsgBUMkX63248564() {     int mrhVeqTLpeVdaQwSzOnf44279454 = -939338645;    int mrhVeqTLpeVdaQwSzOnf63393580 = -477229988;    int mrhVeqTLpeVdaQwSzOnf77392193 = -380363346;    int mrhVeqTLpeVdaQwSzOnf13330417 = -655736448;    int mrhVeqTLpeVdaQwSzOnf67698557 = -169065398;    int mrhVeqTLpeVdaQwSzOnf94383782 = -944395949;    int mrhVeqTLpeVdaQwSzOnf79067503 = -913354923;    int mrhVeqTLpeVdaQwSzOnf52825852 = -589546182;    int mrhVeqTLpeVdaQwSzOnf29150863 = -450951074;    int mrhVeqTLpeVdaQwSzOnf25208333 = -540616345;    int mrhVeqTLpeVdaQwSzOnf39434136 = -640972465;    int mrhVeqTLpeVdaQwSzOnf41969772 = -710047628;    int mrhVeqTLpeVdaQwSzOnf81047831 = -670434998;    int mrhVeqTLpeVdaQwSzOnf39075468 = -811902768;    int mrhVeqTLpeVdaQwSzOnf86496203 = -537854186;    int mrhVeqTLpeVdaQwSzOnf53014082 = -331597874;    int mrhVeqTLpeVdaQwSzOnf77481259 = -71973483;    int mrhVeqTLpeVdaQwSzOnf63475280 = -705879641;    int mrhVeqTLpeVdaQwSzOnf9329755 = -444867588;    int mrhVeqTLpeVdaQwSzOnf66440576 = -69272031;    int mrhVeqTLpeVdaQwSzOnf58917173 = -597613392;    int mrhVeqTLpeVdaQwSzOnf20943230 = -640723206;    int mrhVeqTLpeVdaQwSzOnf96482115 = -16350810;    int mrhVeqTLpeVdaQwSzOnf64941447 = -172030436;    int mrhVeqTLpeVdaQwSzOnf68482859 = -472033487;    int mrhVeqTLpeVdaQwSzOnf28139697 = -805147860;    int mrhVeqTLpeVdaQwSzOnf17727580 = -18323530;    int mrhVeqTLpeVdaQwSzOnf54894495 = -382213333;    int mrhVeqTLpeVdaQwSzOnf15382866 = -134777958;    int mrhVeqTLpeVdaQwSzOnf72338851 = -270849310;    int mrhVeqTLpeVdaQwSzOnf19256473 = -959692733;    int mrhVeqTLpeVdaQwSzOnf22916285 = -810101550;    int mrhVeqTLpeVdaQwSzOnf6749391 = -3685344;    int mrhVeqTLpeVdaQwSzOnf97978284 = -817873160;    int mrhVeqTLpeVdaQwSzOnf93570865 = -755435459;    int mrhVeqTLpeVdaQwSzOnf2705356 = -449768367;    int mrhVeqTLpeVdaQwSzOnf23875498 = -344820555;    int mrhVeqTLpeVdaQwSzOnf16542751 = -906386978;    int mrhVeqTLpeVdaQwSzOnf73435857 = -613182106;    int mrhVeqTLpeVdaQwSzOnf3664704 = -985804339;    int mrhVeqTLpeVdaQwSzOnf12163579 = -851902592;    int mrhVeqTLpeVdaQwSzOnf96347931 = -531144063;    int mrhVeqTLpeVdaQwSzOnf30245873 = -420901524;    int mrhVeqTLpeVdaQwSzOnf55929958 = -429267834;    int mrhVeqTLpeVdaQwSzOnf82451156 = -586446479;    int mrhVeqTLpeVdaQwSzOnf40735050 = -896219566;    int mrhVeqTLpeVdaQwSzOnf30594093 = -103892723;    int mrhVeqTLpeVdaQwSzOnf1835207 = -399450108;    int mrhVeqTLpeVdaQwSzOnf15087534 = 10501159;    int mrhVeqTLpeVdaQwSzOnf4678093 = -271308611;    int mrhVeqTLpeVdaQwSzOnf65993422 = -960130242;    int mrhVeqTLpeVdaQwSzOnf97916247 = -646165972;    int mrhVeqTLpeVdaQwSzOnf42979034 = -747359936;    int mrhVeqTLpeVdaQwSzOnf4190536 = -280968890;    int mrhVeqTLpeVdaQwSzOnf4331392 = -677120252;    int mrhVeqTLpeVdaQwSzOnf23336224 = -198615439;    int mrhVeqTLpeVdaQwSzOnf66911464 = -360879179;    int mrhVeqTLpeVdaQwSzOnf12450746 = -108332911;    int mrhVeqTLpeVdaQwSzOnf44847557 = -83702961;    int mrhVeqTLpeVdaQwSzOnf39558860 = -363917539;    int mrhVeqTLpeVdaQwSzOnf76656202 = -826072420;    int mrhVeqTLpeVdaQwSzOnf24173009 = -431141591;    int mrhVeqTLpeVdaQwSzOnf37442987 = -354768224;    int mrhVeqTLpeVdaQwSzOnf56812011 = -80101765;    int mrhVeqTLpeVdaQwSzOnf5951860 = -580923612;    int mrhVeqTLpeVdaQwSzOnf16517852 = -830870915;    int mrhVeqTLpeVdaQwSzOnf35220382 = -606362285;    int mrhVeqTLpeVdaQwSzOnf83069546 = -852561839;    int mrhVeqTLpeVdaQwSzOnf45504602 = 43532690;    int mrhVeqTLpeVdaQwSzOnf83790847 = 11914181;    int mrhVeqTLpeVdaQwSzOnf29138584 = -986777319;    int mrhVeqTLpeVdaQwSzOnf60938509 = -165586506;    int mrhVeqTLpeVdaQwSzOnf90039422 = 7302464;    int mrhVeqTLpeVdaQwSzOnf5665051 = -459063249;    int mrhVeqTLpeVdaQwSzOnf54276997 = -217369439;    int mrhVeqTLpeVdaQwSzOnf62569241 = 33530671;    int mrhVeqTLpeVdaQwSzOnf90697357 = -119821682;    int mrhVeqTLpeVdaQwSzOnf40552158 = -587082976;    int mrhVeqTLpeVdaQwSzOnf82490290 = -585583957;    int mrhVeqTLpeVdaQwSzOnf27747809 = -575813922;    int mrhVeqTLpeVdaQwSzOnf97545604 = -601255137;    int mrhVeqTLpeVdaQwSzOnf15892373 = -618873422;    int mrhVeqTLpeVdaQwSzOnf39806961 = -292714492;    int mrhVeqTLpeVdaQwSzOnf10704773 = -863469348;    int mrhVeqTLpeVdaQwSzOnf6345429 = -310719068;    int mrhVeqTLpeVdaQwSzOnf21340226 = -213526762;    int mrhVeqTLpeVdaQwSzOnf79937250 = 37258386;    int mrhVeqTLpeVdaQwSzOnf2558855 = -722716454;    int mrhVeqTLpeVdaQwSzOnf93646892 = -40752909;    int mrhVeqTLpeVdaQwSzOnf70234641 = -456820020;    int mrhVeqTLpeVdaQwSzOnf35793892 = 11110811;    int mrhVeqTLpeVdaQwSzOnf11424753 = -136487644;    int mrhVeqTLpeVdaQwSzOnf71695193 = -722684017;    int mrhVeqTLpeVdaQwSzOnf33876998 = -149264567;    int mrhVeqTLpeVdaQwSzOnf27008502 = -59731920;    int mrhVeqTLpeVdaQwSzOnf87990570 = -320761002;    int mrhVeqTLpeVdaQwSzOnf58904944 = -76375839;    int mrhVeqTLpeVdaQwSzOnf73433861 = -240799759;    int mrhVeqTLpeVdaQwSzOnf49978098 = -848344222;    int mrhVeqTLpeVdaQwSzOnf65933305 = -939338645;     mrhVeqTLpeVdaQwSzOnf44279454 = mrhVeqTLpeVdaQwSzOnf63393580;     mrhVeqTLpeVdaQwSzOnf63393580 = mrhVeqTLpeVdaQwSzOnf77392193;     mrhVeqTLpeVdaQwSzOnf77392193 = mrhVeqTLpeVdaQwSzOnf13330417;     mrhVeqTLpeVdaQwSzOnf13330417 = mrhVeqTLpeVdaQwSzOnf67698557;     mrhVeqTLpeVdaQwSzOnf67698557 = mrhVeqTLpeVdaQwSzOnf94383782;     mrhVeqTLpeVdaQwSzOnf94383782 = mrhVeqTLpeVdaQwSzOnf79067503;     mrhVeqTLpeVdaQwSzOnf79067503 = mrhVeqTLpeVdaQwSzOnf52825852;     mrhVeqTLpeVdaQwSzOnf52825852 = mrhVeqTLpeVdaQwSzOnf29150863;     mrhVeqTLpeVdaQwSzOnf29150863 = mrhVeqTLpeVdaQwSzOnf25208333;     mrhVeqTLpeVdaQwSzOnf25208333 = mrhVeqTLpeVdaQwSzOnf39434136;     mrhVeqTLpeVdaQwSzOnf39434136 = mrhVeqTLpeVdaQwSzOnf41969772;     mrhVeqTLpeVdaQwSzOnf41969772 = mrhVeqTLpeVdaQwSzOnf81047831;     mrhVeqTLpeVdaQwSzOnf81047831 = mrhVeqTLpeVdaQwSzOnf39075468;     mrhVeqTLpeVdaQwSzOnf39075468 = mrhVeqTLpeVdaQwSzOnf86496203;     mrhVeqTLpeVdaQwSzOnf86496203 = mrhVeqTLpeVdaQwSzOnf53014082;     mrhVeqTLpeVdaQwSzOnf53014082 = mrhVeqTLpeVdaQwSzOnf77481259;     mrhVeqTLpeVdaQwSzOnf77481259 = mrhVeqTLpeVdaQwSzOnf63475280;     mrhVeqTLpeVdaQwSzOnf63475280 = mrhVeqTLpeVdaQwSzOnf9329755;     mrhVeqTLpeVdaQwSzOnf9329755 = mrhVeqTLpeVdaQwSzOnf66440576;     mrhVeqTLpeVdaQwSzOnf66440576 = mrhVeqTLpeVdaQwSzOnf58917173;     mrhVeqTLpeVdaQwSzOnf58917173 = mrhVeqTLpeVdaQwSzOnf20943230;     mrhVeqTLpeVdaQwSzOnf20943230 = mrhVeqTLpeVdaQwSzOnf96482115;     mrhVeqTLpeVdaQwSzOnf96482115 = mrhVeqTLpeVdaQwSzOnf64941447;     mrhVeqTLpeVdaQwSzOnf64941447 = mrhVeqTLpeVdaQwSzOnf68482859;     mrhVeqTLpeVdaQwSzOnf68482859 = mrhVeqTLpeVdaQwSzOnf28139697;     mrhVeqTLpeVdaQwSzOnf28139697 = mrhVeqTLpeVdaQwSzOnf17727580;     mrhVeqTLpeVdaQwSzOnf17727580 = mrhVeqTLpeVdaQwSzOnf54894495;     mrhVeqTLpeVdaQwSzOnf54894495 = mrhVeqTLpeVdaQwSzOnf15382866;     mrhVeqTLpeVdaQwSzOnf15382866 = mrhVeqTLpeVdaQwSzOnf72338851;     mrhVeqTLpeVdaQwSzOnf72338851 = mrhVeqTLpeVdaQwSzOnf19256473;     mrhVeqTLpeVdaQwSzOnf19256473 = mrhVeqTLpeVdaQwSzOnf22916285;     mrhVeqTLpeVdaQwSzOnf22916285 = mrhVeqTLpeVdaQwSzOnf6749391;     mrhVeqTLpeVdaQwSzOnf6749391 = mrhVeqTLpeVdaQwSzOnf97978284;     mrhVeqTLpeVdaQwSzOnf97978284 = mrhVeqTLpeVdaQwSzOnf93570865;     mrhVeqTLpeVdaQwSzOnf93570865 = mrhVeqTLpeVdaQwSzOnf2705356;     mrhVeqTLpeVdaQwSzOnf2705356 = mrhVeqTLpeVdaQwSzOnf23875498;     mrhVeqTLpeVdaQwSzOnf23875498 = mrhVeqTLpeVdaQwSzOnf16542751;     mrhVeqTLpeVdaQwSzOnf16542751 = mrhVeqTLpeVdaQwSzOnf73435857;     mrhVeqTLpeVdaQwSzOnf73435857 = mrhVeqTLpeVdaQwSzOnf3664704;     mrhVeqTLpeVdaQwSzOnf3664704 = mrhVeqTLpeVdaQwSzOnf12163579;     mrhVeqTLpeVdaQwSzOnf12163579 = mrhVeqTLpeVdaQwSzOnf96347931;     mrhVeqTLpeVdaQwSzOnf96347931 = mrhVeqTLpeVdaQwSzOnf30245873;     mrhVeqTLpeVdaQwSzOnf30245873 = mrhVeqTLpeVdaQwSzOnf55929958;     mrhVeqTLpeVdaQwSzOnf55929958 = mrhVeqTLpeVdaQwSzOnf82451156;     mrhVeqTLpeVdaQwSzOnf82451156 = mrhVeqTLpeVdaQwSzOnf40735050;     mrhVeqTLpeVdaQwSzOnf40735050 = mrhVeqTLpeVdaQwSzOnf30594093;     mrhVeqTLpeVdaQwSzOnf30594093 = mrhVeqTLpeVdaQwSzOnf1835207;     mrhVeqTLpeVdaQwSzOnf1835207 = mrhVeqTLpeVdaQwSzOnf15087534;     mrhVeqTLpeVdaQwSzOnf15087534 = mrhVeqTLpeVdaQwSzOnf4678093;     mrhVeqTLpeVdaQwSzOnf4678093 = mrhVeqTLpeVdaQwSzOnf65993422;     mrhVeqTLpeVdaQwSzOnf65993422 = mrhVeqTLpeVdaQwSzOnf97916247;     mrhVeqTLpeVdaQwSzOnf97916247 = mrhVeqTLpeVdaQwSzOnf42979034;     mrhVeqTLpeVdaQwSzOnf42979034 = mrhVeqTLpeVdaQwSzOnf4190536;     mrhVeqTLpeVdaQwSzOnf4190536 = mrhVeqTLpeVdaQwSzOnf4331392;     mrhVeqTLpeVdaQwSzOnf4331392 = mrhVeqTLpeVdaQwSzOnf23336224;     mrhVeqTLpeVdaQwSzOnf23336224 = mrhVeqTLpeVdaQwSzOnf66911464;     mrhVeqTLpeVdaQwSzOnf66911464 = mrhVeqTLpeVdaQwSzOnf12450746;     mrhVeqTLpeVdaQwSzOnf12450746 = mrhVeqTLpeVdaQwSzOnf44847557;     mrhVeqTLpeVdaQwSzOnf44847557 = mrhVeqTLpeVdaQwSzOnf39558860;     mrhVeqTLpeVdaQwSzOnf39558860 = mrhVeqTLpeVdaQwSzOnf76656202;     mrhVeqTLpeVdaQwSzOnf76656202 = mrhVeqTLpeVdaQwSzOnf24173009;     mrhVeqTLpeVdaQwSzOnf24173009 = mrhVeqTLpeVdaQwSzOnf37442987;     mrhVeqTLpeVdaQwSzOnf37442987 = mrhVeqTLpeVdaQwSzOnf56812011;     mrhVeqTLpeVdaQwSzOnf56812011 = mrhVeqTLpeVdaQwSzOnf5951860;     mrhVeqTLpeVdaQwSzOnf5951860 = mrhVeqTLpeVdaQwSzOnf16517852;     mrhVeqTLpeVdaQwSzOnf16517852 = mrhVeqTLpeVdaQwSzOnf35220382;     mrhVeqTLpeVdaQwSzOnf35220382 = mrhVeqTLpeVdaQwSzOnf83069546;     mrhVeqTLpeVdaQwSzOnf83069546 = mrhVeqTLpeVdaQwSzOnf45504602;     mrhVeqTLpeVdaQwSzOnf45504602 = mrhVeqTLpeVdaQwSzOnf83790847;     mrhVeqTLpeVdaQwSzOnf83790847 = mrhVeqTLpeVdaQwSzOnf29138584;     mrhVeqTLpeVdaQwSzOnf29138584 = mrhVeqTLpeVdaQwSzOnf60938509;     mrhVeqTLpeVdaQwSzOnf60938509 = mrhVeqTLpeVdaQwSzOnf90039422;     mrhVeqTLpeVdaQwSzOnf90039422 = mrhVeqTLpeVdaQwSzOnf5665051;     mrhVeqTLpeVdaQwSzOnf5665051 = mrhVeqTLpeVdaQwSzOnf54276997;     mrhVeqTLpeVdaQwSzOnf54276997 = mrhVeqTLpeVdaQwSzOnf62569241;     mrhVeqTLpeVdaQwSzOnf62569241 = mrhVeqTLpeVdaQwSzOnf90697357;     mrhVeqTLpeVdaQwSzOnf90697357 = mrhVeqTLpeVdaQwSzOnf40552158;     mrhVeqTLpeVdaQwSzOnf40552158 = mrhVeqTLpeVdaQwSzOnf82490290;     mrhVeqTLpeVdaQwSzOnf82490290 = mrhVeqTLpeVdaQwSzOnf27747809;     mrhVeqTLpeVdaQwSzOnf27747809 = mrhVeqTLpeVdaQwSzOnf97545604;     mrhVeqTLpeVdaQwSzOnf97545604 = mrhVeqTLpeVdaQwSzOnf15892373;     mrhVeqTLpeVdaQwSzOnf15892373 = mrhVeqTLpeVdaQwSzOnf39806961;     mrhVeqTLpeVdaQwSzOnf39806961 = mrhVeqTLpeVdaQwSzOnf10704773;     mrhVeqTLpeVdaQwSzOnf10704773 = mrhVeqTLpeVdaQwSzOnf6345429;     mrhVeqTLpeVdaQwSzOnf6345429 = mrhVeqTLpeVdaQwSzOnf21340226;     mrhVeqTLpeVdaQwSzOnf21340226 = mrhVeqTLpeVdaQwSzOnf79937250;     mrhVeqTLpeVdaQwSzOnf79937250 = mrhVeqTLpeVdaQwSzOnf2558855;     mrhVeqTLpeVdaQwSzOnf2558855 = mrhVeqTLpeVdaQwSzOnf93646892;     mrhVeqTLpeVdaQwSzOnf93646892 = mrhVeqTLpeVdaQwSzOnf70234641;     mrhVeqTLpeVdaQwSzOnf70234641 = mrhVeqTLpeVdaQwSzOnf35793892;     mrhVeqTLpeVdaQwSzOnf35793892 = mrhVeqTLpeVdaQwSzOnf11424753;     mrhVeqTLpeVdaQwSzOnf11424753 = mrhVeqTLpeVdaQwSzOnf71695193;     mrhVeqTLpeVdaQwSzOnf71695193 = mrhVeqTLpeVdaQwSzOnf33876998;     mrhVeqTLpeVdaQwSzOnf33876998 = mrhVeqTLpeVdaQwSzOnf27008502;     mrhVeqTLpeVdaQwSzOnf27008502 = mrhVeqTLpeVdaQwSzOnf87990570;     mrhVeqTLpeVdaQwSzOnf87990570 = mrhVeqTLpeVdaQwSzOnf58904944;     mrhVeqTLpeVdaQwSzOnf58904944 = mrhVeqTLpeVdaQwSzOnf73433861;     mrhVeqTLpeVdaQwSzOnf73433861 = mrhVeqTLpeVdaQwSzOnf49978098;     mrhVeqTLpeVdaQwSzOnf49978098 = mrhVeqTLpeVdaQwSzOnf65933305;     mrhVeqTLpeVdaQwSzOnf65933305 = mrhVeqTLpeVdaQwSzOnf44279454;}
// Junk Finished

// Junk Code By Peatreat & Thaisen's Gen
void zkYrOGvwEemIirlyhlGB87348350() {     int WtJVwBrDuEibzWWrJpHM47198401 = -160611396;    int WtJVwBrDuEibzWWrJpHM94717216 = -979895047;    int WtJVwBrDuEibzWWrJpHM89324479 = -968388575;    int WtJVwBrDuEibzWWrJpHM9689956 = -845569716;    int WtJVwBrDuEibzWWrJpHM65426295 = -133849984;    int WtJVwBrDuEibzWWrJpHM83674824 = -914519558;    int WtJVwBrDuEibzWWrJpHM26391364 = -292036000;    int WtJVwBrDuEibzWWrJpHM63209478 = -966334861;    int WtJVwBrDuEibzWWrJpHM65627196 = -706955734;    int WtJVwBrDuEibzWWrJpHM39043194 = -277771632;    int WtJVwBrDuEibzWWrJpHM22752732 = -204606565;    int WtJVwBrDuEibzWWrJpHM94378107 = -961490217;    int WtJVwBrDuEibzWWrJpHM57684352 = -864795042;    int WtJVwBrDuEibzWWrJpHM73274179 = -797325800;    int WtJVwBrDuEibzWWrJpHM62705019 = -967248107;    int WtJVwBrDuEibzWWrJpHM31054345 = -250073482;    int WtJVwBrDuEibzWWrJpHM21342498 = -177478622;    int WtJVwBrDuEibzWWrJpHM45388945 = -306867614;    int WtJVwBrDuEibzWWrJpHM50976506 = -202764952;    int WtJVwBrDuEibzWWrJpHM64345557 = -704370029;    int WtJVwBrDuEibzWWrJpHM47344208 = 17111625;    int WtJVwBrDuEibzWWrJpHM69776365 = -633686306;    int WtJVwBrDuEibzWWrJpHM31602433 = -994856783;    int WtJVwBrDuEibzWWrJpHM73719566 = 520699;    int WtJVwBrDuEibzWWrJpHM92819616 = -899292502;    int WtJVwBrDuEibzWWrJpHM11675231 = -463246866;    int WtJVwBrDuEibzWWrJpHM80888869 = -110759091;    int WtJVwBrDuEibzWWrJpHM10014271 = -6934508;    int WtJVwBrDuEibzWWrJpHM80530469 = -465123932;    int WtJVwBrDuEibzWWrJpHM96789176 = -99706482;    int WtJVwBrDuEibzWWrJpHM33743193 = -75682793;    int WtJVwBrDuEibzWWrJpHM40878165 = -897823886;    int WtJVwBrDuEibzWWrJpHM76311957 = 22178928;    int WtJVwBrDuEibzWWrJpHM98610464 = -837396804;    int WtJVwBrDuEibzWWrJpHM29869894 = -756489076;    int WtJVwBrDuEibzWWrJpHM8332288 = -461595136;    int WtJVwBrDuEibzWWrJpHM48931760 = -650892431;    int WtJVwBrDuEibzWWrJpHM28686377 = -696803634;    int WtJVwBrDuEibzWWrJpHM54430664 = -549813312;    int WtJVwBrDuEibzWWrJpHM2568683 = -25497272;    int WtJVwBrDuEibzWWrJpHM99438892 = -845293963;    int WtJVwBrDuEibzWWrJpHM72997189 = -50156528;    int WtJVwBrDuEibzWWrJpHM64662824 = -360630310;    int WtJVwBrDuEibzWWrJpHM71988538 = 22441177;    int WtJVwBrDuEibzWWrJpHM70406012 = -118466994;    int WtJVwBrDuEibzWWrJpHM94374205 = -541117240;    int WtJVwBrDuEibzWWrJpHM60901611 = -768609653;    int WtJVwBrDuEibzWWrJpHM46278051 = 27302215;    int WtJVwBrDuEibzWWrJpHM68411848 = 74723699;    int WtJVwBrDuEibzWWrJpHM60863496 = -652316525;    int WtJVwBrDuEibzWWrJpHM427070 = -908841588;    int WtJVwBrDuEibzWWrJpHM55318644 = -699632395;    int WtJVwBrDuEibzWWrJpHM51468291 = -743162206;    int WtJVwBrDuEibzWWrJpHM91547052 = -562056063;    int WtJVwBrDuEibzWWrJpHM40004143 = 56734268;    int WtJVwBrDuEibzWWrJpHM77422035 = -526925090;    int WtJVwBrDuEibzWWrJpHM63114784 = -985038264;    int WtJVwBrDuEibzWWrJpHM15604914 = -868909275;    int WtJVwBrDuEibzWWrJpHM16870339 = -946277215;    int WtJVwBrDuEibzWWrJpHM53751065 = -670603119;    int WtJVwBrDuEibzWWrJpHM2785956 = -703760468;    int WtJVwBrDuEibzWWrJpHM16377094 = -185101493;    int WtJVwBrDuEibzWWrJpHM82679008 = -401210929;    int WtJVwBrDuEibzWWrJpHM68838020 = -507249253;    int WtJVwBrDuEibzWWrJpHM5300001 = -102088840;    int WtJVwBrDuEibzWWrJpHM81874567 = -306782680;    int WtJVwBrDuEibzWWrJpHM18066150 = -883669146;    int WtJVwBrDuEibzWWrJpHM59073887 = 72601762;    int WtJVwBrDuEibzWWrJpHM43404286 = 59163276;    int WtJVwBrDuEibzWWrJpHM54372731 = -405652972;    int WtJVwBrDuEibzWWrJpHM82122585 = -599181051;    int WtJVwBrDuEibzWWrJpHM92656120 = -480674988;    int WtJVwBrDuEibzWWrJpHM90958280 = -757054302;    int WtJVwBrDuEibzWWrJpHM48407824 = -77267681;    int WtJVwBrDuEibzWWrJpHM64906664 = -859076066;    int WtJVwBrDuEibzWWrJpHM74347018 = -932731847;    int WtJVwBrDuEibzWWrJpHM5113542 = -173055996;    int WtJVwBrDuEibzWWrJpHM59613894 = -917297961;    int WtJVwBrDuEibzWWrJpHM3313554 = -881012308;    int WtJVwBrDuEibzWWrJpHM98445411 = -258175263;    int WtJVwBrDuEibzWWrJpHM50773620 = -694637213;    int WtJVwBrDuEibzWWrJpHM34610818 = -38061307;    int WtJVwBrDuEibzWWrJpHM41602423 = 18341793;    int WtJVwBrDuEibzWWrJpHM19666973 = -812807407;    int WtJVwBrDuEibzWWrJpHM96362106 = -190864894;    int WtJVwBrDuEibzWWrJpHM78424549 = -376050398;    int WtJVwBrDuEibzWWrJpHM89409873 = -54661681;    int WtJVwBrDuEibzWWrJpHM84764905 = -415765009;    int WtJVwBrDuEibzWWrJpHM58606322 = -794131073;    int WtJVwBrDuEibzWWrJpHM52447858 = -129563986;    int WtJVwBrDuEibzWWrJpHM45217504 = -476556872;    int WtJVwBrDuEibzWWrJpHM33326846 = -781983157;    int WtJVwBrDuEibzWWrJpHM11816038 = -750526420;    int WtJVwBrDuEibzWWrJpHM679600 = -879210194;    int WtJVwBrDuEibzWWrJpHM99782726 = -321736805;    int WtJVwBrDuEibzWWrJpHM83061799 = -560192471;    int WtJVwBrDuEibzWWrJpHM90318181 = -648945599;    int WtJVwBrDuEibzWWrJpHM95824803 = -853381058;    int WtJVwBrDuEibzWWrJpHM66688537 = -875469983;    int WtJVwBrDuEibzWWrJpHM88531445 = -160611396;     WtJVwBrDuEibzWWrJpHM47198401 = WtJVwBrDuEibzWWrJpHM94717216;     WtJVwBrDuEibzWWrJpHM94717216 = WtJVwBrDuEibzWWrJpHM89324479;     WtJVwBrDuEibzWWrJpHM89324479 = WtJVwBrDuEibzWWrJpHM9689956;     WtJVwBrDuEibzWWrJpHM9689956 = WtJVwBrDuEibzWWrJpHM65426295;     WtJVwBrDuEibzWWrJpHM65426295 = WtJVwBrDuEibzWWrJpHM83674824;     WtJVwBrDuEibzWWrJpHM83674824 = WtJVwBrDuEibzWWrJpHM26391364;     WtJVwBrDuEibzWWrJpHM26391364 = WtJVwBrDuEibzWWrJpHM63209478;     WtJVwBrDuEibzWWrJpHM63209478 = WtJVwBrDuEibzWWrJpHM65627196;     WtJVwBrDuEibzWWrJpHM65627196 = WtJVwBrDuEibzWWrJpHM39043194;     WtJVwBrDuEibzWWrJpHM39043194 = WtJVwBrDuEibzWWrJpHM22752732;     WtJVwBrDuEibzWWrJpHM22752732 = WtJVwBrDuEibzWWrJpHM94378107;     WtJVwBrDuEibzWWrJpHM94378107 = WtJVwBrDuEibzWWrJpHM57684352;     WtJVwBrDuEibzWWrJpHM57684352 = WtJVwBrDuEibzWWrJpHM73274179;     WtJVwBrDuEibzWWrJpHM73274179 = WtJVwBrDuEibzWWrJpHM62705019;     WtJVwBrDuEibzWWrJpHM62705019 = WtJVwBrDuEibzWWrJpHM31054345;     WtJVwBrDuEibzWWrJpHM31054345 = WtJVwBrDuEibzWWrJpHM21342498;     WtJVwBrDuEibzWWrJpHM21342498 = WtJVwBrDuEibzWWrJpHM45388945;     WtJVwBrDuEibzWWrJpHM45388945 = WtJVwBrDuEibzWWrJpHM50976506;     WtJVwBrDuEibzWWrJpHM50976506 = WtJVwBrDuEibzWWrJpHM64345557;     WtJVwBrDuEibzWWrJpHM64345557 = WtJVwBrDuEibzWWrJpHM47344208;     WtJVwBrDuEibzWWrJpHM47344208 = WtJVwBrDuEibzWWrJpHM69776365;     WtJVwBrDuEibzWWrJpHM69776365 = WtJVwBrDuEibzWWrJpHM31602433;     WtJVwBrDuEibzWWrJpHM31602433 = WtJVwBrDuEibzWWrJpHM73719566;     WtJVwBrDuEibzWWrJpHM73719566 = WtJVwBrDuEibzWWrJpHM92819616;     WtJVwBrDuEibzWWrJpHM92819616 = WtJVwBrDuEibzWWrJpHM11675231;     WtJVwBrDuEibzWWrJpHM11675231 = WtJVwBrDuEibzWWrJpHM80888869;     WtJVwBrDuEibzWWrJpHM80888869 = WtJVwBrDuEibzWWrJpHM10014271;     WtJVwBrDuEibzWWrJpHM10014271 = WtJVwBrDuEibzWWrJpHM80530469;     WtJVwBrDuEibzWWrJpHM80530469 = WtJVwBrDuEibzWWrJpHM96789176;     WtJVwBrDuEibzWWrJpHM96789176 = WtJVwBrDuEibzWWrJpHM33743193;     WtJVwBrDuEibzWWrJpHM33743193 = WtJVwBrDuEibzWWrJpHM40878165;     WtJVwBrDuEibzWWrJpHM40878165 = WtJVwBrDuEibzWWrJpHM76311957;     WtJVwBrDuEibzWWrJpHM76311957 = WtJVwBrDuEibzWWrJpHM98610464;     WtJVwBrDuEibzWWrJpHM98610464 = WtJVwBrDuEibzWWrJpHM29869894;     WtJVwBrDuEibzWWrJpHM29869894 = WtJVwBrDuEibzWWrJpHM8332288;     WtJVwBrDuEibzWWrJpHM8332288 = WtJVwBrDuEibzWWrJpHM48931760;     WtJVwBrDuEibzWWrJpHM48931760 = WtJVwBrDuEibzWWrJpHM28686377;     WtJVwBrDuEibzWWrJpHM28686377 = WtJVwBrDuEibzWWrJpHM54430664;     WtJVwBrDuEibzWWrJpHM54430664 = WtJVwBrDuEibzWWrJpHM2568683;     WtJVwBrDuEibzWWrJpHM2568683 = WtJVwBrDuEibzWWrJpHM99438892;     WtJVwBrDuEibzWWrJpHM99438892 = WtJVwBrDuEibzWWrJpHM72997189;     WtJVwBrDuEibzWWrJpHM72997189 = WtJVwBrDuEibzWWrJpHM64662824;     WtJVwBrDuEibzWWrJpHM64662824 = WtJVwBrDuEibzWWrJpHM71988538;     WtJVwBrDuEibzWWrJpHM71988538 = WtJVwBrDuEibzWWrJpHM70406012;     WtJVwBrDuEibzWWrJpHM70406012 = WtJVwBrDuEibzWWrJpHM94374205;     WtJVwBrDuEibzWWrJpHM94374205 = WtJVwBrDuEibzWWrJpHM60901611;     WtJVwBrDuEibzWWrJpHM60901611 = WtJVwBrDuEibzWWrJpHM46278051;     WtJVwBrDuEibzWWrJpHM46278051 = WtJVwBrDuEibzWWrJpHM68411848;     WtJVwBrDuEibzWWrJpHM68411848 = WtJVwBrDuEibzWWrJpHM60863496;     WtJVwBrDuEibzWWrJpHM60863496 = WtJVwBrDuEibzWWrJpHM427070;     WtJVwBrDuEibzWWrJpHM427070 = WtJVwBrDuEibzWWrJpHM55318644;     WtJVwBrDuEibzWWrJpHM55318644 = WtJVwBrDuEibzWWrJpHM51468291;     WtJVwBrDuEibzWWrJpHM51468291 = WtJVwBrDuEibzWWrJpHM91547052;     WtJVwBrDuEibzWWrJpHM91547052 = WtJVwBrDuEibzWWrJpHM40004143;     WtJVwBrDuEibzWWrJpHM40004143 = WtJVwBrDuEibzWWrJpHM77422035;     WtJVwBrDuEibzWWrJpHM77422035 = WtJVwBrDuEibzWWrJpHM63114784;     WtJVwBrDuEibzWWrJpHM63114784 = WtJVwBrDuEibzWWrJpHM15604914;     WtJVwBrDuEibzWWrJpHM15604914 = WtJVwBrDuEibzWWrJpHM16870339;     WtJVwBrDuEibzWWrJpHM16870339 = WtJVwBrDuEibzWWrJpHM53751065;     WtJVwBrDuEibzWWrJpHM53751065 = WtJVwBrDuEibzWWrJpHM2785956;     WtJVwBrDuEibzWWrJpHM2785956 = WtJVwBrDuEibzWWrJpHM16377094;     WtJVwBrDuEibzWWrJpHM16377094 = WtJVwBrDuEibzWWrJpHM82679008;     WtJVwBrDuEibzWWrJpHM82679008 = WtJVwBrDuEibzWWrJpHM68838020;     WtJVwBrDuEibzWWrJpHM68838020 = WtJVwBrDuEibzWWrJpHM5300001;     WtJVwBrDuEibzWWrJpHM5300001 = WtJVwBrDuEibzWWrJpHM81874567;     WtJVwBrDuEibzWWrJpHM81874567 = WtJVwBrDuEibzWWrJpHM18066150;     WtJVwBrDuEibzWWrJpHM18066150 = WtJVwBrDuEibzWWrJpHM59073887;     WtJVwBrDuEibzWWrJpHM59073887 = WtJVwBrDuEibzWWrJpHM43404286;     WtJVwBrDuEibzWWrJpHM43404286 = WtJVwBrDuEibzWWrJpHM54372731;     WtJVwBrDuEibzWWrJpHM54372731 = WtJVwBrDuEibzWWrJpHM82122585;     WtJVwBrDuEibzWWrJpHM82122585 = WtJVwBrDuEibzWWrJpHM92656120;     WtJVwBrDuEibzWWrJpHM92656120 = WtJVwBrDuEibzWWrJpHM90958280;     WtJVwBrDuEibzWWrJpHM90958280 = WtJVwBrDuEibzWWrJpHM48407824;     WtJVwBrDuEibzWWrJpHM48407824 = WtJVwBrDuEibzWWrJpHM64906664;     WtJVwBrDuEibzWWrJpHM64906664 = WtJVwBrDuEibzWWrJpHM74347018;     WtJVwBrDuEibzWWrJpHM74347018 = WtJVwBrDuEibzWWrJpHM5113542;     WtJVwBrDuEibzWWrJpHM5113542 = WtJVwBrDuEibzWWrJpHM59613894;     WtJVwBrDuEibzWWrJpHM59613894 = WtJVwBrDuEibzWWrJpHM3313554;     WtJVwBrDuEibzWWrJpHM3313554 = WtJVwBrDuEibzWWrJpHM98445411;     WtJVwBrDuEibzWWrJpHM98445411 = WtJVwBrDuEibzWWrJpHM50773620;     WtJVwBrDuEibzWWrJpHM50773620 = WtJVwBrDuEibzWWrJpHM34610818;     WtJVwBrDuEibzWWrJpHM34610818 = WtJVwBrDuEibzWWrJpHM41602423;     WtJVwBrDuEibzWWrJpHM41602423 = WtJVwBrDuEibzWWrJpHM19666973;     WtJVwBrDuEibzWWrJpHM19666973 = WtJVwBrDuEibzWWrJpHM96362106;     WtJVwBrDuEibzWWrJpHM96362106 = WtJVwBrDuEibzWWrJpHM78424549;     WtJVwBrDuEibzWWrJpHM78424549 = WtJVwBrDuEibzWWrJpHM89409873;     WtJVwBrDuEibzWWrJpHM89409873 = WtJVwBrDuEibzWWrJpHM84764905;     WtJVwBrDuEibzWWrJpHM84764905 = WtJVwBrDuEibzWWrJpHM58606322;     WtJVwBrDuEibzWWrJpHM58606322 = WtJVwBrDuEibzWWrJpHM52447858;     WtJVwBrDuEibzWWrJpHM52447858 = WtJVwBrDuEibzWWrJpHM45217504;     WtJVwBrDuEibzWWrJpHM45217504 = WtJVwBrDuEibzWWrJpHM33326846;     WtJVwBrDuEibzWWrJpHM33326846 = WtJVwBrDuEibzWWrJpHM11816038;     WtJVwBrDuEibzWWrJpHM11816038 = WtJVwBrDuEibzWWrJpHM679600;     WtJVwBrDuEibzWWrJpHM679600 = WtJVwBrDuEibzWWrJpHM99782726;     WtJVwBrDuEibzWWrJpHM99782726 = WtJVwBrDuEibzWWrJpHM83061799;     WtJVwBrDuEibzWWrJpHM83061799 = WtJVwBrDuEibzWWrJpHM90318181;     WtJVwBrDuEibzWWrJpHM90318181 = WtJVwBrDuEibzWWrJpHM95824803;     WtJVwBrDuEibzWWrJpHM95824803 = WtJVwBrDuEibzWWrJpHM66688537;     WtJVwBrDuEibzWWrJpHM66688537 = WtJVwBrDuEibzWWrJpHM88531445;     WtJVwBrDuEibzWWrJpHM88531445 = WtJVwBrDuEibzWWrJpHM47198401;}
// Junk Finished

// Junk Code By Peatreat & Thaisen's Gen
void NshAwRWAKBAucTJWUuog63690667() {     int GDsLqhCtKUqARHMIoeKR79459210 = -836259158;    int GDsLqhCtKUqARHMIoeKR95939651 = -553511931;    int GDsLqhCtKUqARHMIoeKR22601060 = 28752042;    int GDsLqhCtKUqARHMIoeKR51198070 = -665805426;    int GDsLqhCtKUqARHMIoeKR50543814 = -783583543;    int GDsLqhCtKUqARHMIoeKR17771538 = -882494828;    int GDsLqhCtKUqARHMIoeKR23883665 = -476477833;    int GDsLqhCtKUqARHMIoeKR85964773 = -700483766;    int GDsLqhCtKUqARHMIoeKR38978640 = -583259136;    int GDsLqhCtKUqARHMIoeKR26551109 = -450638440;    int GDsLqhCtKUqARHMIoeKR84142772 = -792810809;    int GDsLqhCtKUqARHMIoeKR15531549 = -632647375;    int GDsLqhCtKUqARHMIoeKR86346400 = -71276501;    int GDsLqhCtKUqARHMIoeKR77470078 = -226873672;    int GDsLqhCtKUqARHMIoeKR1365925 = -109604878;    int GDsLqhCtKUqARHMIoeKR51130516 = -931361155;    int GDsLqhCtKUqARHMIoeKR27203669 = -193413200;    int GDsLqhCtKUqARHMIoeKR30009648 = -315761012;    int GDsLqhCtKUqARHMIoeKR31343746 = -971456641;    int GDsLqhCtKUqARHMIoeKR75537592 = -309126173;    int GDsLqhCtKUqARHMIoeKR60304912 = -269950591;    int GDsLqhCtKUqARHMIoeKR35510002 = -805355984;    int GDsLqhCtKUqARHMIoeKR1657338 = -764477454;    int GDsLqhCtKUqARHMIoeKR92298255 = -115447783;    int GDsLqhCtKUqARHMIoeKR81474675 = -977340683;    int GDsLqhCtKUqARHMIoeKR81187260 = 68732645;    int GDsLqhCtKUqARHMIoeKR6010955 = -108816590;    int GDsLqhCtKUqARHMIoeKR92755345 = -835498903;    int GDsLqhCtKUqARHMIoeKR26214033 = -47182003;    int GDsLqhCtKUqARHMIoeKR58157276 = -145599957;    int GDsLqhCtKUqARHMIoeKR74570595 = -930332401;    int GDsLqhCtKUqARHMIoeKR99052441 = 50929179;    int GDsLqhCtKUqARHMIoeKR48381587 = -206939407;    int GDsLqhCtKUqARHMIoeKR34160508 = -70072496;    int GDsLqhCtKUqARHMIoeKR70502724 = -69909838;    int GDsLqhCtKUqARHMIoeKR96794150 = -473961102;    int GDsLqhCtKUqARHMIoeKR42137903 = -868024479;    int GDsLqhCtKUqARHMIoeKR57955194 = -698369862;    int GDsLqhCtKUqARHMIoeKR92205778 = -143397298;    int GDsLqhCtKUqARHMIoeKR78242255 = -257013528;    int GDsLqhCtKUqARHMIoeKR14496591 = -542610353;    int GDsLqhCtKUqARHMIoeKR6861118 = 6427994;    int GDsLqhCtKUqARHMIoeKR10396175 = -872735298;    int GDsLqhCtKUqARHMIoeKR40770717 = -110839283;    int GDsLqhCtKUqARHMIoeKR46424861 = -915100515;    int GDsLqhCtKUqARHMIoeKR82035493 = -239722430;    int GDsLqhCtKUqARHMIoeKR92775717 = -698537406;    int GDsLqhCtKUqARHMIoeKR490267 = -802576522;    int GDsLqhCtKUqARHMIoeKR34953099 = -539053653;    int GDsLqhCtKUqARHMIoeKR28666836 = -337340447;    int GDsLqhCtKUqARHMIoeKR79930762 = -889535820;    int GDsLqhCtKUqARHMIoeKR85396794 = -66024844;    int GDsLqhCtKUqARHMIoeKR80330239 = -703220441;    int GDsLqhCtKUqARHMIoeKR98926535 = -714181590;    int GDsLqhCtKUqARHMIoeKR3385755 = 44630425;    int GDsLqhCtKUqARHMIoeKR43949208 = 69096825;    int GDsLqhCtKUqARHMIoeKR94282314 = -789034477;    int GDsLqhCtKUqARHMIoeKR30302804 = -855800176;    int GDsLqhCtKUqARHMIoeKR69723394 = -688464744;    int GDsLqhCtKUqARHMIoeKR69356554 = -752316189;    int GDsLqhCtKUqARHMIoeKR11760583 = -673678239;    int GDsLqhCtKUqARHMIoeKR31128320 = -640978931;    int GDsLqhCtKUqARHMIoeKR59750741 = -553301764;    int GDsLqhCtKUqARHMIoeKR80821364 = -337659179;    int GDsLqhCtKUqARHMIoeKR51980513 = -520306040;    int GDsLqhCtKUqARHMIoeKR85090330 = -743739988;    int GDsLqhCtKUqARHMIoeKR67149962 = -325707969;    int GDsLqhCtKUqARHMIoeKR52185893 = 98795995;    int GDsLqhCtKUqARHMIoeKR6967354 = -56963835;    int GDsLqhCtKUqARHMIoeKR4571774 = -635643776;    int GDsLqhCtKUqARHMIoeKR8992613 = 36663323;    int GDsLqhCtKUqARHMIoeKR69248474 = -495043338;    int GDsLqhCtKUqARHMIoeKR37803870 = -72363714;    int GDsLqhCtKUqARHMIoeKR53101491 = -614443114;    int GDsLqhCtKUqARHMIoeKR61041002 = -766515820;    int GDsLqhCtKUqARHMIoeKR53443795 = -176378585;    int GDsLqhCtKUqARHMIoeKR25113827 = -932620686;    int GDsLqhCtKUqARHMIoeKR60886621 = -553638172;    int GDsLqhCtKUqARHMIoeKR45873395 = -200347268;    int GDsLqhCtKUqARHMIoeKR99439182 = -637618254;    int GDsLqhCtKUqARHMIoeKR88411542 = -232729949;    int GDsLqhCtKUqARHMIoeKR5520688 = -306240068;    int GDsLqhCtKUqARHMIoeKR57802247 = -196445251;    int GDsLqhCtKUqARHMIoeKR97547196 = -709841557;    int GDsLqhCtKUqARHMIoeKR78226513 = -256064137;    int GDsLqhCtKUqARHMIoeKR89173801 = -764307557;    int GDsLqhCtKUqARHMIoeKR18722203 = -245850381;    int GDsLqhCtKUqARHMIoeKR49455052 = -492757818;    int GDsLqhCtKUqARHMIoeKR30774753 = -14702921;    int GDsLqhCtKUqARHMIoeKR26553516 = -39006663;    int GDsLqhCtKUqARHMIoeKR2511837 = -684926625;    int GDsLqhCtKUqARHMIoeKR11835100 = 87775697;    int GDsLqhCtKUqARHMIoeKR88231800 = 90094881;    int GDsLqhCtKUqARHMIoeKR22849225 = -391081110;    int GDsLqhCtKUqARHMIoeKR66481673 = -583335290;    int GDsLqhCtKUqARHMIoeKR83368271 = -901631423;    int GDsLqhCtKUqARHMIoeKR47110377 = -440270243;    int GDsLqhCtKUqARHMIoeKR29574811 = -435076120;    int GDsLqhCtKUqARHMIoeKR88790203 = -590533244;    int GDsLqhCtKUqARHMIoeKR61334530 = -836259158;     GDsLqhCtKUqARHMIoeKR79459210 = GDsLqhCtKUqARHMIoeKR95939651;     GDsLqhCtKUqARHMIoeKR95939651 = GDsLqhCtKUqARHMIoeKR22601060;     GDsLqhCtKUqARHMIoeKR22601060 = GDsLqhCtKUqARHMIoeKR51198070;     GDsLqhCtKUqARHMIoeKR51198070 = GDsLqhCtKUqARHMIoeKR50543814;     GDsLqhCtKUqARHMIoeKR50543814 = GDsLqhCtKUqARHMIoeKR17771538;     GDsLqhCtKUqARHMIoeKR17771538 = GDsLqhCtKUqARHMIoeKR23883665;     GDsLqhCtKUqARHMIoeKR23883665 = GDsLqhCtKUqARHMIoeKR85964773;     GDsLqhCtKUqARHMIoeKR85964773 = GDsLqhCtKUqARHMIoeKR38978640;     GDsLqhCtKUqARHMIoeKR38978640 = GDsLqhCtKUqARHMIoeKR26551109;     GDsLqhCtKUqARHMIoeKR26551109 = GDsLqhCtKUqARHMIoeKR84142772;     GDsLqhCtKUqARHMIoeKR84142772 = GDsLqhCtKUqARHMIoeKR15531549;     GDsLqhCtKUqARHMIoeKR15531549 = GDsLqhCtKUqARHMIoeKR86346400;     GDsLqhCtKUqARHMIoeKR86346400 = GDsLqhCtKUqARHMIoeKR77470078;     GDsLqhCtKUqARHMIoeKR77470078 = GDsLqhCtKUqARHMIoeKR1365925;     GDsLqhCtKUqARHMIoeKR1365925 = GDsLqhCtKUqARHMIoeKR51130516;     GDsLqhCtKUqARHMIoeKR51130516 = GDsLqhCtKUqARHMIoeKR27203669;     GDsLqhCtKUqARHMIoeKR27203669 = GDsLqhCtKUqARHMIoeKR30009648;     GDsLqhCtKUqARHMIoeKR30009648 = GDsLqhCtKUqARHMIoeKR31343746;     GDsLqhCtKUqARHMIoeKR31343746 = GDsLqhCtKUqARHMIoeKR75537592;     GDsLqhCtKUqARHMIoeKR75537592 = GDsLqhCtKUqARHMIoeKR60304912;     GDsLqhCtKUqARHMIoeKR60304912 = GDsLqhCtKUqARHMIoeKR35510002;     GDsLqhCtKUqARHMIoeKR35510002 = GDsLqhCtKUqARHMIoeKR1657338;     GDsLqhCtKUqARHMIoeKR1657338 = GDsLqhCtKUqARHMIoeKR92298255;     GDsLqhCtKUqARHMIoeKR92298255 = GDsLqhCtKUqARHMIoeKR81474675;     GDsLqhCtKUqARHMIoeKR81474675 = GDsLqhCtKUqARHMIoeKR81187260;     GDsLqhCtKUqARHMIoeKR81187260 = GDsLqhCtKUqARHMIoeKR6010955;     GDsLqhCtKUqARHMIoeKR6010955 = GDsLqhCtKUqARHMIoeKR92755345;     GDsLqhCtKUqARHMIoeKR92755345 = GDsLqhCtKUqARHMIoeKR26214033;     GDsLqhCtKUqARHMIoeKR26214033 = GDsLqhCtKUqARHMIoeKR58157276;     GDsLqhCtKUqARHMIoeKR58157276 = GDsLqhCtKUqARHMIoeKR74570595;     GDsLqhCtKUqARHMIoeKR74570595 = GDsLqhCtKUqARHMIoeKR99052441;     GDsLqhCtKUqARHMIoeKR99052441 = GDsLqhCtKUqARHMIoeKR48381587;     GDsLqhCtKUqARHMIoeKR48381587 = GDsLqhCtKUqARHMIoeKR34160508;     GDsLqhCtKUqARHMIoeKR34160508 = GDsLqhCtKUqARHMIoeKR70502724;     GDsLqhCtKUqARHMIoeKR70502724 = GDsLqhCtKUqARHMIoeKR96794150;     GDsLqhCtKUqARHMIoeKR96794150 = GDsLqhCtKUqARHMIoeKR42137903;     GDsLqhCtKUqARHMIoeKR42137903 = GDsLqhCtKUqARHMIoeKR57955194;     GDsLqhCtKUqARHMIoeKR57955194 = GDsLqhCtKUqARHMIoeKR92205778;     GDsLqhCtKUqARHMIoeKR92205778 = GDsLqhCtKUqARHMIoeKR78242255;     GDsLqhCtKUqARHMIoeKR78242255 = GDsLqhCtKUqARHMIoeKR14496591;     GDsLqhCtKUqARHMIoeKR14496591 = GDsLqhCtKUqARHMIoeKR6861118;     GDsLqhCtKUqARHMIoeKR6861118 = GDsLqhCtKUqARHMIoeKR10396175;     GDsLqhCtKUqARHMIoeKR10396175 = GDsLqhCtKUqARHMIoeKR40770717;     GDsLqhCtKUqARHMIoeKR40770717 = GDsLqhCtKUqARHMIoeKR46424861;     GDsLqhCtKUqARHMIoeKR46424861 = GDsLqhCtKUqARHMIoeKR82035493;     GDsLqhCtKUqARHMIoeKR82035493 = GDsLqhCtKUqARHMIoeKR92775717;     GDsLqhCtKUqARHMIoeKR92775717 = GDsLqhCtKUqARHMIoeKR490267;     GDsLqhCtKUqARHMIoeKR490267 = GDsLqhCtKUqARHMIoeKR34953099;     GDsLqhCtKUqARHMIoeKR34953099 = GDsLqhCtKUqARHMIoeKR28666836;     GDsLqhCtKUqARHMIoeKR28666836 = GDsLqhCtKUqARHMIoeKR79930762;     GDsLqhCtKUqARHMIoeKR79930762 = GDsLqhCtKUqARHMIoeKR85396794;     GDsLqhCtKUqARHMIoeKR85396794 = GDsLqhCtKUqARHMIoeKR80330239;     GDsLqhCtKUqARHMIoeKR80330239 = GDsLqhCtKUqARHMIoeKR98926535;     GDsLqhCtKUqARHMIoeKR98926535 = GDsLqhCtKUqARHMIoeKR3385755;     GDsLqhCtKUqARHMIoeKR3385755 = GDsLqhCtKUqARHMIoeKR43949208;     GDsLqhCtKUqARHMIoeKR43949208 = GDsLqhCtKUqARHMIoeKR94282314;     GDsLqhCtKUqARHMIoeKR94282314 = GDsLqhCtKUqARHMIoeKR30302804;     GDsLqhCtKUqARHMIoeKR30302804 = GDsLqhCtKUqARHMIoeKR69723394;     GDsLqhCtKUqARHMIoeKR69723394 = GDsLqhCtKUqARHMIoeKR69356554;     GDsLqhCtKUqARHMIoeKR69356554 = GDsLqhCtKUqARHMIoeKR11760583;     GDsLqhCtKUqARHMIoeKR11760583 = GDsLqhCtKUqARHMIoeKR31128320;     GDsLqhCtKUqARHMIoeKR31128320 = GDsLqhCtKUqARHMIoeKR59750741;     GDsLqhCtKUqARHMIoeKR59750741 = GDsLqhCtKUqARHMIoeKR80821364;     GDsLqhCtKUqARHMIoeKR80821364 = GDsLqhCtKUqARHMIoeKR51980513;     GDsLqhCtKUqARHMIoeKR51980513 = GDsLqhCtKUqARHMIoeKR85090330;     GDsLqhCtKUqARHMIoeKR85090330 = GDsLqhCtKUqARHMIoeKR67149962;     GDsLqhCtKUqARHMIoeKR67149962 = GDsLqhCtKUqARHMIoeKR52185893;     GDsLqhCtKUqARHMIoeKR52185893 = GDsLqhCtKUqARHMIoeKR6967354;     GDsLqhCtKUqARHMIoeKR6967354 = GDsLqhCtKUqARHMIoeKR4571774;     GDsLqhCtKUqARHMIoeKR4571774 = GDsLqhCtKUqARHMIoeKR8992613;     GDsLqhCtKUqARHMIoeKR8992613 = GDsLqhCtKUqARHMIoeKR69248474;     GDsLqhCtKUqARHMIoeKR69248474 = GDsLqhCtKUqARHMIoeKR37803870;     GDsLqhCtKUqARHMIoeKR37803870 = GDsLqhCtKUqARHMIoeKR53101491;     GDsLqhCtKUqARHMIoeKR53101491 = GDsLqhCtKUqARHMIoeKR61041002;     GDsLqhCtKUqARHMIoeKR61041002 = GDsLqhCtKUqARHMIoeKR53443795;     GDsLqhCtKUqARHMIoeKR53443795 = GDsLqhCtKUqARHMIoeKR25113827;     GDsLqhCtKUqARHMIoeKR25113827 = GDsLqhCtKUqARHMIoeKR60886621;     GDsLqhCtKUqARHMIoeKR60886621 = GDsLqhCtKUqARHMIoeKR45873395;     GDsLqhCtKUqARHMIoeKR45873395 = GDsLqhCtKUqARHMIoeKR99439182;     GDsLqhCtKUqARHMIoeKR99439182 = GDsLqhCtKUqARHMIoeKR88411542;     GDsLqhCtKUqARHMIoeKR88411542 = GDsLqhCtKUqARHMIoeKR5520688;     GDsLqhCtKUqARHMIoeKR5520688 = GDsLqhCtKUqARHMIoeKR57802247;     GDsLqhCtKUqARHMIoeKR57802247 = GDsLqhCtKUqARHMIoeKR97547196;     GDsLqhCtKUqARHMIoeKR97547196 = GDsLqhCtKUqARHMIoeKR78226513;     GDsLqhCtKUqARHMIoeKR78226513 = GDsLqhCtKUqARHMIoeKR89173801;     GDsLqhCtKUqARHMIoeKR89173801 = GDsLqhCtKUqARHMIoeKR18722203;     GDsLqhCtKUqARHMIoeKR18722203 = GDsLqhCtKUqARHMIoeKR49455052;     GDsLqhCtKUqARHMIoeKR49455052 = GDsLqhCtKUqARHMIoeKR30774753;     GDsLqhCtKUqARHMIoeKR30774753 = GDsLqhCtKUqARHMIoeKR26553516;     GDsLqhCtKUqARHMIoeKR26553516 = GDsLqhCtKUqARHMIoeKR2511837;     GDsLqhCtKUqARHMIoeKR2511837 = GDsLqhCtKUqARHMIoeKR11835100;     GDsLqhCtKUqARHMIoeKR11835100 = GDsLqhCtKUqARHMIoeKR88231800;     GDsLqhCtKUqARHMIoeKR88231800 = GDsLqhCtKUqARHMIoeKR22849225;     GDsLqhCtKUqARHMIoeKR22849225 = GDsLqhCtKUqARHMIoeKR66481673;     GDsLqhCtKUqARHMIoeKR66481673 = GDsLqhCtKUqARHMIoeKR83368271;     GDsLqhCtKUqARHMIoeKR83368271 = GDsLqhCtKUqARHMIoeKR47110377;     GDsLqhCtKUqARHMIoeKR47110377 = GDsLqhCtKUqARHMIoeKR29574811;     GDsLqhCtKUqARHMIoeKR29574811 = GDsLqhCtKUqARHMIoeKR88790203;     GDsLqhCtKUqARHMIoeKR88790203 = GDsLqhCtKUqARHMIoeKR61334530;     GDsLqhCtKUqARHMIoeKR61334530 = GDsLqhCtKUqARHMIoeKR79459210;}
// Junk Finished

// Junk Code By Peatreat & Thaisen's Gen
void AslrJaRyfjKdmiSWsEpb87790453() {     int sEdvmNrfNfbYDlvsysLI82378157 = -57531909;    int sEdvmNrfNfbYDlvsysLI27263289 = 43823010;    int sEdvmNrfNfbYDlvsysLI34533346 = -559273188;    int sEdvmNrfNfbYDlvsysLI47557609 = -855638695;    int sEdvmNrfNfbYDlvsysLI48271552 = -748368129;    int sEdvmNrfNfbYDlvsysLI7062580 = -852618438;    int sEdvmNrfNfbYDlvsysLI71207525 = -955158909;    int sEdvmNrfNfbYDlvsysLI96348398 = 22727556;    int sEdvmNrfNfbYDlvsysLI75454973 = -839263796;    int sEdvmNrfNfbYDlvsysLI40385970 = -187793727;    int sEdvmNrfNfbYDlvsysLI67461369 = -356444910;    int sEdvmNrfNfbYDlvsysLI67939884 = -884089963;    int sEdvmNrfNfbYDlvsysLI62982922 = -265636544;    int sEdvmNrfNfbYDlvsysLI11668791 = -212296703;    int sEdvmNrfNfbYDlvsysLI77574740 = -538998799;    int sEdvmNrfNfbYDlvsysLI29170779 = -849836763;    int sEdvmNrfNfbYDlvsysLI71064907 = -298918339;    int sEdvmNrfNfbYDlvsysLI11923313 = 83251015;    int sEdvmNrfNfbYDlvsysLI72990497 = -729354006;    int sEdvmNrfNfbYDlvsysLI73442574 = -944224170;    int sEdvmNrfNfbYDlvsysLI48731948 = -755225574;    int sEdvmNrfNfbYDlvsysLI84343137 = -798319084;    int sEdvmNrfNfbYDlvsysLI36777655 = -642983428;    int sEdvmNrfNfbYDlvsysLI1076375 = 57103352;    int sEdvmNrfNfbYDlvsysLI5811434 = -304599698;    int sEdvmNrfNfbYDlvsysLI64722793 = -689366360;    int sEdvmNrfNfbYDlvsysLI69172244 = -201252151;    int sEdvmNrfNfbYDlvsysLI47875121 = -460220077;    int sEdvmNrfNfbYDlvsysLI91361636 = -377527976;    int sEdvmNrfNfbYDlvsysLI82607601 = 25542871;    int sEdvmNrfNfbYDlvsysLI89057315 = -46322460;    int sEdvmNrfNfbYDlvsysLI17014322 = -36793158;    int sEdvmNrfNfbYDlvsysLI17944154 = -181075135;    int sEdvmNrfNfbYDlvsysLI34792688 = -89596140;    int sEdvmNrfNfbYDlvsysLI6801753 = -70963454;    int sEdvmNrfNfbYDlvsysLI2421083 = -485787870;    int sEdvmNrfNfbYDlvsysLI67194164 = -74096355;    int sEdvmNrfNfbYDlvsysLI70098821 = -488786519;    int sEdvmNrfNfbYDlvsysLI73200584 = -80028505;    int sEdvmNrfNfbYDlvsysLI77146234 = -396706461;    int sEdvmNrfNfbYDlvsysLI1771905 = -536001724;    int sEdvmNrfNfbYDlvsysLI83510375 = -612584471;    int sEdvmNrfNfbYDlvsysLI44813126 = -812464085;    int sEdvmNrfNfbYDlvsysLI56829297 = -759130271;    int sEdvmNrfNfbYDlvsysLI34379717 = -447121030;    int sEdvmNrfNfbYDlvsysLI35674648 = -984620104;    int sEdvmNrfNfbYDlvsysLI23083235 = -263254337;    int sEdvmNrfNfbYDlvsysLI44933112 = -375824199;    int sEdvmNrfNfbYDlvsysLI88277413 = -474831112;    int sEdvmNrfNfbYDlvsysLI84852239 = -718348361;    int sEdvmNrfNfbYDlvsysLI14364410 = -838247166;    int sEdvmNrfNfbYDlvsysLI42799191 = -119491267;    int sEdvmNrfNfbYDlvsysLI88819496 = -699022710;    int sEdvmNrfNfbYDlvsysLI86283052 = -995268762;    int sEdvmNrfNfbYDlvsysLI39058506 = -321515056;    int sEdvmNrfNfbYDlvsysLI98035019 = -259212826;    int sEdvmNrfNfbYDlvsysLI90485633 = -313193562;    int sEdvmNrfNfbYDlvsysLI33456972 = -516376540;    int sEdvmNrfNfbYDlvsysLI41746176 = -451038997;    int sEdvmNrfNfbYDlvsysLI83548758 = 40998231;    int sEdvmNrfNfbYDlvsysLI37890336 = -551366287;    int sEdvmNrfNfbYDlvsysLI23332405 = -394938832;    int sEdvmNrfNfbYDlvsysLI4986763 = -599744469;    int sEdvmNrfNfbYDlvsysLI92847372 = -764806667;    int sEdvmNrfNfbYDlvsysLI51328654 = -41471267;    int sEdvmNrfNfbYDlvsysLI50447047 = -219651752;    int sEdvmNrfNfbYDlvsysLI49995731 = -603014829;    int sEdvmNrfNfbYDlvsysLI28190234 = -76040405;    int sEdvmNrfNfbYDlvsysLI4867038 = -41333250;    int sEdvmNrfNfbYDlvsysLI75153658 = 46789070;    int sEdvmNrfNfbYDlvsysLI61976615 = -675740409;    int sEdvmNrfNfbYDlvsysLI966087 = -810131820;    int sEdvmNrfNfbYDlvsysLI38722729 = -836720480;    int sEdvmNrfNfbYDlvsysLI95844263 = -232647546;    int sEdvmNrfNfbYDlvsysLI71670670 = -308222447;    int sEdvmNrfNfbYDlvsysLI65221572 = -42641103;    int sEdvmNrfNfbYDlvsysLI39530012 = -985855000;    int sEdvmNrfNfbYDlvsysLI79948357 = -883853158;    int sEdvmNrfNfbYDlvsysLI66696657 = -495775619;    int sEdvmNrfNfbYDlvsysLI70136785 = -319979595;    int sEdvmNrfNfbYDlvsysLI41639558 = -326112024;    int sEdvmNrfNfbYDlvsysLI24239133 = -825427953;    int sEdvmNrfNfbYDlvsysLI59597708 = -985388965;    int sEdvmNrfNfbYDlvsysLI6509397 = -659179616;    int sEdvmNrfNfbYDlvsysLI68243191 = -136209963;    int sEdvmNrfNfbYDlvsysLI46258124 = -926831194;    int sEdvmNrfNfbYDlvsysLI28194826 = -337770448;    int sEdvmNrfNfbYDlvsysLI31661102 = -185806373;    int sEdvmNrfNfbYDlvsysLI95734182 = -768081084;    int sEdvmNrfNfbYDlvsysLI8766733 = -811750629;    int sEdvmNrfNfbYDlvsysLI11935449 = -72594308;    int sEdvmNrfNfbYDlvsysLI33737193 = -557719816;    int sEdvmNrfNfbYDlvsysLI28352646 = 62252478;    int sEdvmNrfNfbYDlvsysLI89651826 = -21026737;    int sEdvmNrfNfbYDlvsysLI39255898 = -845340174;    int sEdvmNrfNfbYDlvsysLI78439499 = -41062892;    int sEdvmNrfNfbYDlvsysLI78523613 = 87159997;    int sEdvmNrfNfbYDlvsysLI51965753 = 52342582;    int sEdvmNrfNfbYDlvsysLI5500643 = -617659004;    int sEdvmNrfNfbYDlvsysLI83932670 = -57531909;     sEdvmNrfNfbYDlvsysLI82378157 = sEdvmNrfNfbYDlvsysLI27263289;     sEdvmNrfNfbYDlvsysLI27263289 = sEdvmNrfNfbYDlvsysLI34533346;     sEdvmNrfNfbYDlvsysLI34533346 = sEdvmNrfNfbYDlvsysLI47557609;     sEdvmNrfNfbYDlvsysLI47557609 = sEdvmNrfNfbYDlvsysLI48271552;     sEdvmNrfNfbYDlvsysLI48271552 = sEdvmNrfNfbYDlvsysLI7062580;     sEdvmNrfNfbYDlvsysLI7062580 = sEdvmNrfNfbYDlvsysLI71207525;     sEdvmNrfNfbYDlvsysLI71207525 = sEdvmNrfNfbYDlvsysLI96348398;     sEdvmNrfNfbYDlvsysLI96348398 = sEdvmNrfNfbYDlvsysLI75454973;     sEdvmNrfNfbYDlvsysLI75454973 = sEdvmNrfNfbYDlvsysLI40385970;     sEdvmNrfNfbYDlvsysLI40385970 = sEdvmNrfNfbYDlvsysLI67461369;     sEdvmNrfNfbYDlvsysLI67461369 = sEdvmNrfNfbYDlvsysLI67939884;     sEdvmNrfNfbYDlvsysLI67939884 = sEdvmNrfNfbYDlvsysLI62982922;     sEdvmNrfNfbYDlvsysLI62982922 = sEdvmNrfNfbYDlvsysLI11668791;     sEdvmNrfNfbYDlvsysLI11668791 = sEdvmNrfNfbYDlvsysLI77574740;     sEdvmNrfNfbYDlvsysLI77574740 = sEdvmNrfNfbYDlvsysLI29170779;     sEdvmNrfNfbYDlvsysLI29170779 = sEdvmNrfNfbYDlvsysLI71064907;     sEdvmNrfNfbYDlvsysLI71064907 = sEdvmNrfNfbYDlvsysLI11923313;     sEdvmNrfNfbYDlvsysLI11923313 = sEdvmNrfNfbYDlvsysLI72990497;     sEdvmNrfNfbYDlvsysLI72990497 = sEdvmNrfNfbYDlvsysLI73442574;     sEdvmNrfNfbYDlvsysLI73442574 = sEdvmNrfNfbYDlvsysLI48731948;     sEdvmNrfNfbYDlvsysLI48731948 = sEdvmNrfNfbYDlvsysLI84343137;     sEdvmNrfNfbYDlvsysLI84343137 = sEdvmNrfNfbYDlvsysLI36777655;     sEdvmNrfNfbYDlvsysLI36777655 = sEdvmNrfNfbYDlvsysLI1076375;     sEdvmNrfNfbYDlvsysLI1076375 = sEdvmNrfNfbYDlvsysLI5811434;     sEdvmNrfNfbYDlvsysLI5811434 = sEdvmNrfNfbYDlvsysLI64722793;     sEdvmNrfNfbYDlvsysLI64722793 = sEdvmNrfNfbYDlvsysLI69172244;     sEdvmNrfNfbYDlvsysLI69172244 = sEdvmNrfNfbYDlvsysLI47875121;     sEdvmNrfNfbYDlvsysLI47875121 = sEdvmNrfNfbYDlvsysLI91361636;     sEdvmNrfNfbYDlvsysLI91361636 = sEdvmNrfNfbYDlvsysLI82607601;     sEdvmNrfNfbYDlvsysLI82607601 = sEdvmNrfNfbYDlvsysLI89057315;     sEdvmNrfNfbYDlvsysLI89057315 = sEdvmNrfNfbYDlvsysLI17014322;     sEdvmNrfNfbYDlvsysLI17014322 = sEdvmNrfNfbYDlvsysLI17944154;     sEdvmNrfNfbYDlvsysLI17944154 = sEdvmNrfNfbYDlvsysLI34792688;     sEdvmNrfNfbYDlvsysLI34792688 = sEdvmNrfNfbYDlvsysLI6801753;     sEdvmNrfNfbYDlvsysLI6801753 = sEdvmNrfNfbYDlvsysLI2421083;     sEdvmNrfNfbYDlvsysLI2421083 = sEdvmNrfNfbYDlvsysLI67194164;     sEdvmNrfNfbYDlvsysLI67194164 = sEdvmNrfNfbYDlvsysLI70098821;     sEdvmNrfNfbYDlvsysLI70098821 = sEdvmNrfNfbYDlvsysLI73200584;     sEdvmNrfNfbYDlvsysLI73200584 = sEdvmNrfNfbYDlvsysLI77146234;     sEdvmNrfNfbYDlvsysLI77146234 = sEdvmNrfNfbYDlvsysLI1771905;     sEdvmNrfNfbYDlvsysLI1771905 = sEdvmNrfNfbYDlvsysLI83510375;     sEdvmNrfNfbYDlvsysLI83510375 = sEdvmNrfNfbYDlvsysLI44813126;     sEdvmNrfNfbYDlvsysLI44813126 = sEdvmNrfNfbYDlvsysLI56829297;     sEdvmNrfNfbYDlvsysLI56829297 = sEdvmNrfNfbYDlvsysLI34379717;     sEdvmNrfNfbYDlvsysLI34379717 = sEdvmNrfNfbYDlvsysLI35674648;     sEdvmNrfNfbYDlvsysLI35674648 = sEdvmNrfNfbYDlvsysLI23083235;     sEdvmNrfNfbYDlvsysLI23083235 = sEdvmNrfNfbYDlvsysLI44933112;     sEdvmNrfNfbYDlvsysLI44933112 = sEdvmNrfNfbYDlvsysLI88277413;     sEdvmNrfNfbYDlvsysLI88277413 = sEdvmNrfNfbYDlvsysLI84852239;     sEdvmNrfNfbYDlvsysLI84852239 = sEdvmNrfNfbYDlvsysLI14364410;     sEdvmNrfNfbYDlvsysLI14364410 = sEdvmNrfNfbYDlvsysLI42799191;     sEdvmNrfNfbYDlvsysLI42799191 = sEdvmNrfNfbYDlvsysLI88819496;     sEdvmNrfNfbYDlvsysLI88819496 = sEdvmNrfNfbYDlvsysLI86283052;     sEdvmNrfNfbYDlvsysLI86283052 = sEdvmNrfNfbYDlvsysLI39058506;     sEdvmNrfNfbYDlvsysLI39058506 = sEdvmNrfNfbYDlvsysLI98035019;     sEdvmNrfNfbYDlvsysLI98035019 = sEdvmNrfNfbYDlvsysLI90485633;     sEdvmNrfNfbYDlvsysLI90485633 = sEdvmNrfNfbYDlvsysLI33456972;     sEdvmNrfNfbYDlvsysLI33456972 = sEdvmNrfNfbYDlvsysLI41746176;     sEdvmNrfNfbYDlvsysLI41746176 = sEdvmNrfNfbYDlvsysLI83548758;     sEdvmNrfNfbYDlvsysLI83548758 = sEdvmNrfNfbYDlvsysLI37890336;     sEdvmNrfNfbYDlvsysLI37890336 = sEdvmNrfNfbYDlvsysLI23332405;     sEdvmNrfNfbYDlvsysLI23332405 = sEdvmNrfNfbYDlvsysLI4986763;     sEdvmNrfNfbYDlvsysLI4986763 = sEdvmNrfNfbYDlvsysLI92847372;     sEdvmNrfNfbYDlvsysLI92847372 = sEdvmNrfNfbYDlvsysLI51328654;     sEdvmNrfNfbYDlvsysLI51328654 = sEdvmNrfNfbYDlvsysLI50447047;     sEdvmNrfNfbYDlvsysLI50447047 = sEdvmNrfNfbYDlvsysLI49995731;     sEdvmNrfNfbYDlvsysLI49995731 = sEdvmNrfNfbYDlvsysLI28190234;     sEdvmNrfNfbYDlvsysLI28190234 = sEdvmNrfNfbYDlvsysLI4867038;     sEdvmNrfNfbYDlvsysLI4867038 = sEdvmNrfNfbYDlvsysLI75153658;     sEdvmNrfNfbYDlvsysLI75153658 = sEdvmNrfNfbYDlvsysLI61976615;     sEdvmNrfNfbYDlvsysLI61976615 = sEdvmNrfNfbYDlvsysLI966087;     sEdvmNrfNfbYDlvsysLI966087 = sEdvmNrfNfbYDlvsysLI38722729;     sEdvmNrfNfbYDlvsysLI38722729 = sEdvmNrfNfbYDlvsysLI95844263;     sEdvmNrfNfbYDlvsysLI95844263 = sEdvmNrfNfbYDlvsysLI71670670;     sEdvmNrfNfbYDlvsysLI71670670 = sEdvmNrfNfbYDlvsysLI65221572;     sEdvmNrfNfbYDlvsysLI65221572 = sEdvmNrfNfbYDlvsysLI39530012;     sEdvmNrfNfbYDlvsysLI39530012 = sEdvmNrfNfbYDlvsysLI79948357;     sEdvmNrfNfbYDlvsysLI79948357 = sEdvmNrfNfbYDlvsysLI66696657;     sEdvmNrfNfbYDlvsysLI66696657 = sEdvmNrfNfbYDlvsysLI70136785;     sEdvmNrfNfbYDlvsysLI70136785 = sEdvmNrfNfbYDlvsysLI41639558;     sEdvmNrfNfbYDlvsysLI41639558 = sEdvmNrfNfbYDlvsysLI24239133;     sEdvmNrfNfbYDlvsysLI24239133 = sEdvmNrfNfbYDlvsysLI59597708;     sEdvmNrfNfbYDlvsysLI59597708 = sEdvmNrfNfbYDlvsysLI6509397;     sEdvmNrfNfbYDlvsysLI6509397 = sEdvmNrfNfbYDlvsysLI68243191;     sEdvmNrfNfbYDlvsysLI68243191 = sEdvmNrfNfbYDlvsysLI46258124;     sEdvmNrfNfbYDlvsysLI46258124 = sEdvmNrfNfbYDlvsysLI28194826;     sEdvmNrfNfbYDlvsysLI28194826 = sEdvmNrfNfbYDlvsysLI31661102;     sEdvmNrfNfbYDlvsysLI31661102 = sEdvmNrfNfbYDlvsysLI95734182;     sEdvmNrfNfbYDlvsysLI95734182 = sEdvmNrfNfbYDlvsysLI8766733;     sEdvmNrfNfbYDlvsysLI8766733 = sEdvmNrfNfbYDlvsysLI11935449;     sEdvmNrfNfbYDlvsysLI11935449 = sEdvmNrfNfbYDlvsysLI33737193;     sEdvmNrfNfbYDlvsysLI33737193 = sEdvmNrfNfbYDlvsysLI28352646;     sEdvmNrfNfbYDlvsysLI28352646 = sEdvmNrfNfbYDlvsysLI89651826;     sEdvmNrfNfbYDlvsysLI89651826 = sEdvmNrfNfbYDlvsysLI39255898;     sEdvmNrfNfbYDlvsysLI39255898 = sEdvmNrfNfbYDlvsysLI78439499;     sEdvmNrfNfbYDlvsysLI78439499 = sEdvmNrfNfbYDlvsysLI78523613;     sEdvmNrfNfbYDlvsysLI78523613 = sEdvmNrfNfbYDlvsysLI51965753;     sEdvmNrfNfbYDlvsysLI51965753 = sEdvmNrfNfbYDlvsysLI5500643;     sEdvmNrfNfbYDlvsysLI5500643 = sEdvmNrfNfbYDlvsysLI83932670;     sEdvmNrfNfbYDlvsysLI83932670 = sEdvmNrfNfbYDlvsysLI82378157;}
// Junk Finished

// Junk Code By Peatreat & Thaisen's Gen
void aAlYweMWjeIHWCfUcYjI64132770() {     int NiqnFUeuWeZrierXAEXN14638966 = -733179671;    int NiqnFUeuWeZrierXAEXN28485724 = -629793874;    int NiqnFUeuWeZrierXAEXN67809926 = -662132571;    int NiqnFUeuWeZrierXAEXN89065722 = -675874404;    int NiqnFUeuWeZrierXAEXN33389071 = -298101688;    int NiqnFUeuWeZrierXAEXN41159293 = -820593708;    int NiqnFUeuWeZrierXAEXN68699826 = -39600743;    int NiqnFUeuWeZrierXAEXN19103694 = -811421350;    int NiqnFUeuWeZrierXAEXN48806417 = -715567197;    int NiqnFUeuWeZrierXAEXN27893885 = -360660535;    int NiqnFUeuWeZrierXAEXN28851409 = -944649153;    int NiqnFUeuWeZrierXAEXN89093325 = -555247122;    int NiqnFUeuWeZrierXAEXN91644970 = -572118003;    int NiqnFUeuWeZrierXAEXN15864689 = -741844576;    int NiqnFUeuWeZrierXAEXN16235646 = -781355570;    int NiqnFUeuWeZrierXAEXN49246950 = -431124436;    int NiqnFUeuWeZrierXAEXN76926078 = -314852917;    int NiqnFUeuWeZrierXAEXN96544016 = 74357617;    int NiqnFUeuWeZrierXAEXN53357737 = -398045695;    int NiqnFUeuWeZrierXAEXN84634609 = -548980314;    int NiqnFUeuWeZrierXAEXN61692652 = 57712210;    int NiqnFUeuWeZrierXAEXN50076774 = -969988762;    int NiqnFUeuWeZrierXAEXN6832560 = -412604099;    int NiqnFUeuWeZrierXAEXN19655064 = -58865130;    int NiqnFUeuWeZrierXAEXN94466492 = -382647879;    int NiqnFUeuWeZrierXAEXN34234823 = -157386849;    int NiqnFUeuWeZrierXAEXN94294329 = -199309650;    int NiqnFUeuWeZrierXAEXN30616196 = -188784473;    int NiqnFUeuWeZrierXAEXN37045200 = 40413953;    int NiqnFUeuWeZrierXAEXN43975701 = -20350605;    int NiqnFUeuWeZrierXAEXN29884718 = -900972068;    int NiqnFUeuWeZrierXAEXN75188599 = -188040093;    int NiqnFUeuWeZrierXAEXN90013783 = -410193470;    int NiqnFUeuWeZrierXAEXN70342730 = -422271832;    int NiqnFUeuWeZrierXAEXN47434583 = -484384216;    int NiqnFUeuWeZrierXAEXN90882944 = -498153836;    int NiqnFUeuWeZrierXAEXN60400308 = -291228402;    int NiqnFUeuWeZrierXAEXN99367638 = -490352747;    int NiqnFUeuWeZrierXAEXN10975699 = -773612491;    int NiqnFUeuWeZrierXAEXN52819807 = -628222717;    int NiqnFUeuWeZrierXAEXN16829602 = -233318114;    int NiqnFUeuWeZrierXAEXN17374304 = -555999949;    int NiqnFUeuWeZrierXAEXN90546476 = -224569073;    int NiqnFUeuWeZrierXAEXN25611476 = -892410731;    int NiqnFUeuWeZrierXAEXN10398566 = -143754552;    int NiqnFUeuWeZrierXAEXN23335937 = -683225294;    int NiqnFUeuWeZrierXAEXN54957342 = -193182090;    int NiqnFUeuWeZrierXAEXN99145326 = -105702937;    int NiqnFUeuWeZrierXAEXN54818664 = 11391536;    int NiqnFUeuWeZrierXAEXN52655579 = -403372282;    int NiqnFUeuWeZrierXAEXN93868102 = -818941399;    int NiqnFUeuWeZrierXAEXN72877341 = -585883716;    int NiqnFUeuWeZrierXAEXN17681445 = -659080946;    int NiqnFUeuWeZrierXAEXN93662534 = -47394289;    int NiqnFUeuWeZrierXAEXN2440119 = -333618899;    int NiqnFUeuWeZrierXAEXN64562192 = -763190910;    int NiqnFUeuWeZrierXAEXN21653165 = -117189776;    int NiqnFUeuWeZrierXAEXN48154862 = -503267441;    int NiqnFUeuWeZrierXAEXN94599230 = -193226526;    int NiqnFUeuWeZrierXAEXN99154247 = -40714839;    int NiqnFUeuWeZrierXAEXN46864963 = -521284058;    int NiqnFUeuWeZrierXAEXN38083630 = -850816270;    int NiqnFUeuWeZrierXAEXN82058494 = -751835303;    int NiqnFUeuWeZrierXAEXN4830717 = -595216593;    int NiqnFUeuWeZrierXAEXN98009166 = -459688467;    int NiqnFUeuWeZrierXAEXN53662810 = -656609060;    int NiqnFUeuWeZrierXAEXN99079542 = -45053652;    int NiqnFUeuWeZrierXAEXN21302240 = -49846172;    int NiqnFUeuWeZrierXAEXN68430106 = -157460361;    int NiqnFUeuWeZrierXAEXN25352701 = -183201734;    int NiqnFUeuWeZrierXAEXN88846641 = -39896034;    int NiqnFUeuWeZrierXAEXN77558440 = -824500171;    int NiqnFUeuWeZrierXAEXN85568317 = -152029893;    int NiqnFUeuWeZrierXAEXN537931 = -769822979;    int NiqnFUeuWeZrierXAEXN67805007 = -215662200;    int NiqnFUeuWeZrierXAEXN44318349 = -386287841;    int NiqnFUeuWeZrierXAEXN59530297 = -645419689;    int NiqnFUeuWeZrierXAEXN81221084 = -520193369;    int NiqnFUeuWeZrierXAEXN9256499 = -915110579;    int NiqnFUeuWeZrierXAEXN71130556 = -699422585;    int NiqnFUeuWeZrierXAEXN79277481 = -964204760;    int NiqnFUeuWeZrierXAEXN95149002 = 6393286;    int NiqnFUeuWeZrierXAEXN75797532 = -100176009;    int NiqnFUeuWeZrierXAEXN84389620 = -556213765;    int NiqnFUeuWeZrierXAEXN50107598 = -201409207;    int NiqnFUeuWeZrierXAEXN57007377 = -215088353;    int NiqnFUeuWeZrierXAEXN57507155 = -528959148;    int NiqnFUeuWeZrierXAEXN96351248 = -262799181;    int NiqnFUeuWeZrierXAEXN67902612 = 11347067;    int NiqnFUeuWeZrierXAEXN82872390 = -721193306;    int NiqnFUeuWeZrierXAEXN69229780 = -280964061;    int NiqnFUeuWeZrierXAEXN12245447 = -787960962;    int NiqnFUeuWeZrierXAEXN4768408 = -197126221;    int NiqnFUeuWeZrierXAEXN11821452 = -632897652;    int NiqnFUeuWeZrierXAEXN5954844 = -6938659;    int NiqnFUeuWeZrierXAEXN78745971 = -382501845;    int NiqnFUeuWeZrierXAEXN35315809 = -804164646;    int NiqnFUeuWeZrierXAEXN85715760 = -629352480;    int NiqnFUeuWeZrierXAEXN27602309 = -332722265;    int NiqnFUeuWeZrierXAEXN56735756 = -733179671;     NiqnFUeuWeZrierXAEXN14638966 = NiqnFUeuWeZrierXAEXN28485724;     NiqnFUeuWeZrierXAEXN28485724 = NiqnFUeuWeZrierXAEXN67809926;     NiqnFUeuWeZrierXAEXN67809926 = NiqnFUeuWeZrierXAEXN89065722;     NiqnFUeuWeZrierXAEXN89065722 = NiqnFUeuWeZrierXAEXN33389071;     NiqnFUeuWeZrierXAEXN33389071 = NiqnFUeuWeZrierXAEXN41159293;     NiqnFUeuWeZrierXAEXN41159293 = NiqnFUeuWeZrierXAEXN68699826;     NiqnFUeuWeZrierXAEXN68699826 = NiqnFUeuWeZrierXAEXN19103694;     NiqnFUeuWeZrierXAEXN19103694 = NiqnFUeuWeZrierXAEXN48806417;     NiqnFUeuWeZrierXAEXN48806417 = NiqnFUeuWeZrierXAEXN27893885;     NiqnFUeuWeZrierXAEXN27893885 = NiqnFUeuWeZrierXAEXN28851409;     NiqnFUeuWeZrierXAEXN28851409 = NiqnFUeuWeZrierXAEXN89093325;     NiqnFUeuWeZrierXAEXN89093325 = NiqnFUeuWeZrierXAEXN91644970;     NiqnFUeuWeZrierXAEXN91644970 = NiqnFUeuWeZrierXAEXN15864689;     NiqnFUeuWeZrierXAEXN15864689 = NiqnFUeuWeZrierXAEXN16235646;     NiqnFUeuWeZrierXAEXN16235646 = NiqnFUeuWeZrierXAEXN49246950;     NiqnFUeuWeZrierXAEXN49246950 = NiqnFUeuWeZrierXAEXN76926078;     NiqnFUeuWeZrierXAEXN76926078 = NiqnFUeuWeZrierXAEXN96544016;     NiqnFUeuWeZrierXAEXN96544016 = NiqnFUeuWeZrierXAEXN53357737;     NiqnFUeuWeZrierXAEXN53357737 = NiqnFUeuWeZrierXAEXN84634609;     NiqnFUeuWeZrierXAEXN84634609 = NiqnFUeuWeZrierXAEXN61692652;     NiqnFUeuWeZrierXAEXN61692652 = NiqnFUeuWeZrierXAEXN50076774;     NiqnFUeuWeZrierXAEXN50076774 = NiqnFUeuWeZrierXAEXN6832560;     NiqnFUeuWeZrierXAEXN6832560 = NiqnFUeuWeZrierXAEXN19655064;     NiqnFUeuWeZrierXAEXN19655064 = NiqnFUeuWeZrierXAEXN94466492;     NiqnFUeuWeZrierXAEXN94466492 = NiqnFUeuWeZrierXAEXN34234823;     NiqnFUeuWeZrierXAEXN34234823 = NiqnFUeuWeZrierXAEXN94294329;     NiqnFUeuWeZrierXAEXN94294329 = NiqnFUeuWeZrierXAEXN30616196;     NiqnFUeuWeZrierXAEXN30616196 = NiqnFUeuWeZrierXAEXN37045200;     NiqnFUeuWeZrierXAEXN37045200 = NiqnFUeuWeZrierXAEXN43975701;     NiqnFUeuWeZrierXAEXN43975701 = NiqnFUeuWeZrierXAEXN29884718;     NiqnFUeuWeZrierXAEXN29884718 = NiqnFUeuWeZrierXAEXN75188599;     NiqnFUeuWeZrierXAEXN75188599 = NiqnFUeuWeZrierXAEXN90013783;     NiqnFUeuWeZrierXAEXN90013783 = NiqnFUeuWeZrierXAEXN70342730;     NiqnFUeuWeZrierXAEXN70342730 = NiqnFUeuWeZrierXAEXN47434583;     NiqnFUeuWeZrierXAEXN47434583 = NiqnFUeuWeZrierXAEXN90882944;     NiqnFUeuWeZrierXAEXN90882944 = NiqnFUeuWeZrierXAEXN60400308;     NiqnFUeuWeZrierXAEXN60400308 = NiqnFUeuWeZrierXAEXN99367638;     NiqnFUeuWeZrierXAEXN99367638 = NiqnFUeuWeZrierXAEXN10975699;     NiqnFUeuWeZrierXAEXN10975699 = NiqnFUeuWeZrierXAEXN52819807;     NiqnFUeuWeZrierXAEXN52819807 = NiqnFUeuWeZrierXAEXN16829602;     NiqnFUeuWeZrierXAEXN16829602 = NiqnFUeuWeZrierXAEXN17374304;     NiqnFUeuWeZrierXAEXN17374304 = NiqnFUeuWeZrierXAEXN90546476;     NiqnFUeuWeZrierXAEXN90546476 = NiqnFUeuWeZrierXAEXN25611476;     NiqnFUeuWeZrierXAEXN25611476 = NiqnFUeuWeZrierXAEXN10398566;     NiqnFUeuWeZrierXAEXN10398566 = NiqnFUeuWeZrierXAEXN23335937;     NiqnFUeuWeZrierXAEXN23335937 = NiqnFUeuWeZrierXAEXN54957342;     NiqnFUeuWeZrierXAEXN54957342 = NiqnFUeuWeZrierXAEXN99145326;     NiqnFUeuWeZrierXAEXN99145326 = NiqnFUeuWeZrierXAEXN54818664;     NiqnFUeuWeZrierXAEXN54818664 = NiqnFUeuWeZrierXAEXN52655579;     NiqnFUeuWeZrierXAEXN52655579 = NiqnFUeuWeZrierXAEXN93868102;     NiqnFUeuWeZrierXAEXN93868102 = NiqnFUeuWeZrierXAEXN72877341;     NiqnFUeuWeZrierXAEXN72877341 = NiqnFUeuWeZrierXAEXN17681445;     NiqnFUeuWeZrierXAEXN17681445 = NiqnFUeuWeZrierXAEXN93662534;     NiqnFUeuWeZrierXAEXN93662534 = NiqnFUeuWeZrierXAEXN2440119;     NiqnFUeuWeZrierXAEXN2440119 = NiqnFUeuWeZrierXAEXN64562192;     NiqnFUeuWeZrierXAEXN64562192 = NiqnFUeuWeZrierXAEXN21653165;     NiqnFUeuWeZrierXAEXN21653165 = NiqnFUeuWeZrierXAEXN48154862;     NiqnFUeuWeZrierXAEXN48154862 = NiqnFUeuWeZrierXAEXN94599230;     NiqnFUeuWeZrierXAEXN94599230 = NiqnFUeuWeZrierXAEXN99154247;     NiqnFUeuWeZrierXAEXN99154247 = NiqnFUeuWeZrierXAEXN46864963;     NiqnFUeuWeZrierXAEXN46864963 = NiqnFUeuWeZrierXAEXN38083630;     NiqnFUeuWeZrierXAEXN38083630 = NiqnFUeuWeZrierXAEXN82058494;     NiqnFUeuWeZrierXAEXN82058494 = NiqnFUeuWeZrierXAEXN4830717;     NiqnFUeuWeZrierXAEXN4830717 = NiqnFUeuWeZrierXAEXN98009166;     NiqnFUeuWeZrierXAEXN98009166 = NiqnFUeuWeZrierXAEXN53662810;     NiqnFUeuWeZrierXAEXN53662810 = NiqnFUeuWeZrierXAEXN99079542;     NiqnFUeuWeZrierXAEXN99079542 = NiqnFUeuWeZrierXAEXN21302240;     NiqnFUeuWeZrierXAEXN21302240 = NiqnFUeuWeZrierXAEXN68430106;     NiqnFUeuWeZrierXAEXN68430106 = NiqnFUeuWeZrierXAEXN25352701;     NiqnFUeuWeZrierXAEXN25352701 = NiqnFUeuWeZrierXAEXN88846641;     NiqnFUeuWeZrierXAEXN88846641 = NiqnFUeuWeZrierXAEXN77558440;     NiqnFUeuWeZrierXAEXN77558440 = NiqnFUeuWeZrierXAEXN85568317;     NiqnFUeuWeZrierXAEXN85568317 = NiqnFUeuWeZrierXAEXN537931;     NiqnFUeuWeZrierXAEXN537931 = NiqnFUeuWeZrierXAEXN67805007;     NiqnFUeuWeZrierXAEXN67805007 = NiqnFUeuWeZrierXAEXN44318349;     NiqnFUeuWeZrierXAEXN44318349 = NiqnFUeuWeZrierXAEXN59530297;     NiqnFUeuWeZrierXAEXN59530297 = NiqnFUeuWeZrierXAEXN81221084;     NiqnFUeuWeZrierXAEXN81221084 = NiqnFUeuWeZrierXAEXN9256499;     NiqnFUeuWeZrierXAEXN9256499 = NiqnFUeuWeZrierXAEXN71130556;     NiqnFUeuWeZrierXAEXN71130556 = NiqnFUeuWeZrierXAEXN79277481;     NiqnFUeuWeZrierXAEXN79277481 = NiqnFUeuWeZrierXAEXN95149002;     NiqnFUeuWeZrierXAEXN95149002 = NiqnFUeuWeZrierXAEXN75797532;     NiqnFUeuWeZrierXAEXN75797532 = NiqnFUeuWeZrierXAEXN84389620;     NiqnFUeuWeZrierXAEXN84389620 = NiqnFUeuWeZrierXAEXN50107598;     NiqnFUeuWeZrierXAEXN50107598 = NiqnFUeuWeZrierXAEXN57007377;     NiqnFUeuWeZrierXAEXN57007377 = NiqnFUeuWeZrierXAEXN57507155;     NiqnFUeuWeZrierXAEXN57507155 = NiqnFUeuWeZrierXAEXN96351248;     NiqnFUeuWeZrierXAEXN96351248 = NiqnFUeuWeZrierXAEXN67902612;     NiqnFUeuWeZrierXAEXN67902612 = NiqnFUeuWeZrierXAEXN82872390;     NiqnFUeuWeZrierXAEXN82872390 = NiqnFUeuWeZrierXAEXN69229780;     NiqnFUeuWeZrierXAEXN69229780 = NiqnFUeuWeZrierXAEXN12245447;     NiqnFUeuWeZrierXAEXN12245447 = NiqnFUeuWeZrierXAEXN4768408;     NiqnFUeuWeZrierXAEXN4768408 = NiqnFUeuWeZrierXAEXN11821452;     NiqnFUeuWeZrierXAEXN11821452 = NiqnFUeuWeZrierXAEXN5954844;     NiqnFUeuWeZrierXAEXN5954844 = NiqnFUeuWeZrierXAEXN78745971;     NiqnFUeuWeZrierXAEXN78745971 = NiqnFUeuWeZrierXAEXN35315809;     NiqnFUeuWeZrierXAEXN35315809 = NiqnFUeuWeZrierXAEXN85715760;     NiqnFUeuWeZrierXAEXN85715760 = NiqnFUeuWeZrierXAEXN27602309;     NiqnFUeuWeZrierXAEXN27602309 = NiqnFUeuWeZrierXAEXN56735756;     NiqnFUeuWeZrierXAEXN56735756 = NiqnFUeuWeZrierXAEXN14638966;}
// Junk Finished

// Junk Code By Peatreat & Thaisen's Gen
void pAdjzmaXAbIClujGiqbR88232556() {     int YIlINmPgLPVsdVvdidvy17557913 = 45547577;    int YIlINmPgLPVsdVvdidvy59809360 = -32458933;    int YIlINmPgLPVsdVvdidvy79742212 = -150157800;    int YIlINmPgLPVsdVvdidvy85425262 = -865707673;    int YIlINmPgLPVsdVvdidvy31116809 = -262886273;    int YIlINmPgLPVsdVvdidvy30450335 = -790717318;    int YIlINmPgLPVsdVvdidvy16023687 = -518281819;    int YIlINmPgLPVsdVvdidvy29487320 = -88210028;    int YIlINmPgLPVsdVvdidvy85282751 = -971571857;    int YIlINmPgLPVsdVvdidvy41728746 = -97815822;    int YIlINmPgLPVsdVvdidvy12170006 = -508283254;    int YIlINmPgLPVsdVvdidvy41501661 = -806689710;    int YIlINmPgLPVsdVvdidvy68281491 = -766478046;    int YIlINmPgLPVsdVvdidvy50063401 = -727267607;    int YIlINmPgLPVsdVvdidvy92444461 = -110749491;    int YIlINmPgLPVsdVvdidvy27287213 = -349600044;    int YIlINmPgLPVsdVvdidvy20787318 = -420358056;    int YIlINmPgLPVsdVvdidvy78457681 = -626630356;    int YIlINmPgLPVsdVvdidvy95004489 = -155943060;    int YIlINmPgLPVsdVvdidvy82539590 = -84078312;    int YIlINmPgLPVsdVvdidvy50119687 = -427562773;    int YIlINmPgLPVsdVvdidvy98909908 = -962951862;    int YIlINmPgLPVsdVvdidvy41952877 = -291110073;    int YIlINmPgLPVsdVvdidvy28433183 = -986313995;    int YIlINmPgLPVsdVvdidvy18803250 = -809906894;    int YIlINmPgLPVsdVvdidvy17770357 = -915485855;    int YIlINmPgLPVsdVvdidvy57455619 = -291745211;    int YIlINmPgLPVsdVvdidvy85735971 = -913505647;    int YIlINmPgLPVsdVvdidvy2192804 = -289932020;    int YIlINmPgLPVsdVvdidvy68426026 = -949207777;    int YIlINmPgLPVsdVvdidvy44371438 = -16962128;    int YIlINmPgLPVsdVvdidvy93150479 = -275762430;    int YIlINmPgLPVsdVvdidvy59576350 = -384329198;    int YIlINmPgLPVsdVvdidvy70974911 = -441795475;    int YIlINmPgLPVsdVvdidvy83733611 = -485437832;    int YIlINmPgLPVsdVvdidvy96509876 = -509980604;    int YIlINmPgLPVsdVvdidvy85456569 = -597300279;    int YIlINmPgLPVsdVvdidvy11511266 = -280769403;    int YIlINmPgLPVsdVvdidvy91970505 = -710243698;    int YIlINmPgLPVsdVvdidvy51723785 = -767915650;    int YIlINmPgLPVsdVvdidvy4104916 = -226709485;    int YIlINmPgLPVsdVvdidvy94023562 = -75012414;    int YIlINmPgLPVsdVvdidvy24963428 = -164297859;    int YIlINmPgLPVsdVvdidvy41670056 = -440701719;    int YIlINmPgLPVsdVvdidvy98353421 = -775775067;    int YIlINmPgLPVsdVvdidvy76975091 = -328122968;    int YIlINmPgLPVsdVvdidvy85264859 = -857899020;    int YIlINmPgLPVsdVvdidvy43588172 = -778950613;    int YIlINmPgLPVsdVvdidvy8142978 = 75614076;    int YIlINmPgLPVsdVvdidvy8840983 = -784380196;    int YIlINmPgLPVsdVvdidvy28301750 = -767652744;    int YIlINmPgLPVsdVvdidvy30279738 = -639350139;    int YIlINmPgLPVsdVvdidvy26170701 = -654883215;    int YIlINmPgLPVsdVvdidvy81019051 = -328481462;    int YIlINmPgLPVsdVvdidvy38112869 = -699764380;    int YIlINmPgLPVsdVvdidvy18648004 = 8499439;    int YIlINmPgLPVsdVvdidvy17856484 = -741348861;    int YIlINmPgLPVsdVvdidvy51309030 = -163843805;    int YIlINmPgLPVsdVvdidvy66622013 = 44199220;    int YIlINmPgLPVsdVvdidvy13346452 = -347400419;    int YIlINmPgLPVsdVvdidvy72994716 = -398972107;    int YIlINmPgLPVsdVvdidvy30287716 = -604776172;    int YIlINmPgLPVsdVvdidvy27294516 = -798278008;    int YIlINmPgLPVsdVvdidvy16856725 = 77635919;    int YIlINmPgLPVsdVvdidvy97357307 = 19146305;    int YIlINmPgLPVsdVvdidvy19019526 = -132520825;    int YIlINmPgLPVsdVvdidvy81925311 = -322360512;    int YIlINmPgLPVsdVvdidvy97306580 = -224682571;    int YIlINmPgLPVsdVvdidvy66329789 = -141829775;    int YIlINmPgLPVsdVvdidvy95934584 = -600768887;    int YIlINmPgLPVsdVvdidvy41830644 = -752299766;    int YIlINmPgLPVsdVvdidvy9276052 = -39588653;    int YIlINmPgLPVsdVvdidvy86487176 = -916386659;    int YIlINmPgLPVsdVvdidvy43280704 = -388027411;    int YIlINmPgLPVsdVvdidvy78434675 = -857368827;    int YIlINmPgLPVsdVvdidvy56096125 = -252550359;    int YIlINmPgLPVsdVvdidvy73946481 = -698654003;    int YIlINmPgLPVsdVvdidvy282821 = -850408354;    int YIlINmPgLPVsdVvdidvy30079761 = -110538929;    int YIlINmPgLPVsdVvdidvy41828159 = -381783926;    int YIlINmPgLPVsdVvdidvy32505497 = 42413164;    int YIlINmPgLPVsdVvdidvy13867448 = -512794598;    int YIlINmPgLPVsdVvdidvy77592993 = -889119724;    int YIlINmPgLPVsdVvdidvy93351820 = -505551825;    int YIlINmPgLPVsdVvdidvy40124276 = -81555033;    int YIlINmPgLPVsdVvdidvy14091700 = -377611990;    int YIlINmPgLPVsdVvdidvy66979778 = -620879215;    int YIlINmPgLPVsdVvdidvy78557298 = 44152263;    int YIlINmPgLPVsdVvdidvy32862042 = -742031096;    int YIlINmPgLPVsdVvdidvy65085607 = -393937271;    int YIlINmPgLPVsdVvdidvy78653393 = -768631744;    int YIlINmPgLPVsdVvdidvy34147540 = -333456475;    int YIlINmPgLPVsdVvdidvy44889253 = -224968624;    int YIlINmPgLPVsdVvdidvy78624053 = -262843279;    int YIlINmPgLPVsdVvdidvy78729069 = -268943543;    int YIlINmPgLPVsdVvdidvy73817200 = -621933313;    int YIlINmPgLPVsdVvdidvy66729046 = -276734406;    int YIlINmPgLPVsdVvdidvy8106703 = -141933779;    int YIlINmPgLPVsdVvdidvy44312748 = -359848025;    int YIlINmPgLPVsdVvdidvy79333895 = 45547577;     YIlINmPgLPVsdVvdidvy17557913 = YIlINmPgLPVsdVvdidvy59809360;     YIlINmPgLPVsdVvdidvy59809360 = YIlINmPgLPVsdVvdidvy79742212;     YIlINmPgLPVsdVvdidvy79742212 = YIlINmPgLPVsdVvdidvy85425262;     YIlINmPgLPVsdVvdidvy85425262 = YIlINmPgLPVsdVvdidvy31116809;     YIlINmPgLPVsdVvdidvy31116809 = YIlINmPgLPVsdVvdidvy30450335;     YIlINmPgLPVsdVvdidvy30450335 = YIlINmPgLPVsdVvdidvy16023687;     YIlINmPgLPVsdVvdidvy16023687 = YIlINmPgLPVsdVvdidvy29487320;     YIlINmPgLPVsdVvdidvy29487320 = YIlINmPgLPVsdVvdidvy85282751;     YIlINmPgLPVsdVvdidvy85282751 = YIlINmPgLPVsdVvdidvy41728746;     YIlINmPgLPVsdVvdidvy41728746 = YIlINmPgLPVsdVvdidvy12170006;     YIlINmPgLPVsdVvdidvy12170006 = YIlINmPgLPVsdVvdidvy41501661;     YIlINmPgLPVsdVvdidvy41501661 = YIlINmPgLPVsdVvdidvy68281491;     YIlINmPgLPVsdVvdidvy68281491 = YIlINmPgLPVsdVvdidvy50063401;     YIlINmPgLPVsdVvdidvy50063401 = YIlINmPgLPVsdVvdidvy92444461;     YIlINmPgLPVsdVvdidvy92444461 = YIlINmPgLPVsdVvdidvy27287213;     YIlINmPgLPVsdVvdidvy27287213 = YIlINmPgLPVsdVvdidvy20787318;     YIlINmPgLPVsdVvdidvy20787318 = YIlINmPgLPVsdVvdidvy78457681;     YIlINmPgLPVsdVvdidvy78457681 = YIlINmPgLPVsdVvdidvy95004489;     YIlINmPgLPVsdVvdidvy95004489 = YIlINmPgLPVsdVvdidvy82539590;     YIlINmPgLPVsdVvdidvy82539590 = YIlINmPgLPVsdVvdidvy50119687;     YIlINmPgLPVsdVvdidvy50119687 = YIlINmPgLPVsdVvdidvy98909908;     YIlINmPgLPVsdVvdidvy98909908 = YIlINmPgLPVsdVvdidvy41952877;     YIlINmPgLPVsdVvdidvy41952877 = YIlINmPgLPVsdVvdidvy28433183;     YIlINmPgLPVsdVvdidvy28433183 = YIlINmPgLPVsdVvdidvy18803250;     YIlINmPgLPVsdVvdidvy18803250 = YIlINmPgLPVsdVvdidvy17770357;     YIlINmPgLPVsdVvdidvy17770357 = YIlINmPgLPVsdVvdidvy57455619;     YIlINmPgLPVsdVvdidvy57455619 = YIlINmPgLPVsdVvdidvy85735971;     YIlINmPgLPVsdVvdidvy85735971 = YIlINmPgLPVsdVvdidvy2192804;     YIlINmPgLPVsdVvdidvy2192804 = YIlINmPgLPVsdVvdidvy68426026;     YIlINmPgLPVsdVvdidvy68426026 = YIlINmPgLPVsdVvdidvy44371438;     YIlINmPgLPVsdVvdidvy44371438 = YIlINmPgLPVsdVvdidvy93150479;     YIlINmPgLPVsdVvdidvy93150479 = YIlINmPgLPVsdVvdidvy59576350;     YIlINmPgLPVsdVvdidvy59576350 = YIlINmPgLPVsdVvdidvy70974911;     YIlINmPgLPVsdVvdidvy70974911 = YIlINmPgLPVsdVvdidvy83733611;     YIlINmPgLPVsdVvdidvy83733611 = YIlINmPgLPVsdVvdidvy96509876;     YIlINmPgLPVsdVvdidvy96509876 = YIlINmPgLPVsdVvdidvy85456569;     YIlINmPgLPVsdVvdidvy85456569 = YIlINmPgLPVsdVvdidvy11511266;     YIlINmPgLPVsdVvdidvy11511266 = YIlINmPgLPVsdVvdidvy91970505;     YIlINmPgLPVsdVvdidvy91970505 = YIlINmPgLPVsdVvdidvy51723785;     YIlINmPgLPVsdVvdidvy51723785 = YIlINmPgLPVsdVvdidvy4104916;     YIlINmPgLPVsdVvdidvy4104916 = YIlINmPgLPVsdVvdidvy94023562;     YIlINmPgLPVsdVvdidvy94023562 = YIlINmPgLPVsdVvdidvy24963428;     YIlINmPgLPVsdVvdidvy24963428 = YIlINmPgLPVsdVvdidvy41670056;     YIlINmPgLPVsdVvdidvy41670056 = YIlINmPgLPVsdVvdidvy98353421;     YIlINmPgLPVsdVvdidvy98353421 = YIlINmPgLPVsdVvdidvy76975091;     YIlINmPgLPVsdVvdidvy76975091 = YIlINmPgLPVsdVvdidvy85264859;     YIlINmPgLPVsdVvdidvy85264859 = YIlINmPgLPVsdVvdidvy43588172;     YIlINmPgLPVsdVvdidvy43588172 = YIlINmPgLPVsdVvdidvy8142978;     YIlINmPgLPVsdVvdidvy8142978 = YIlINmPgLPVsdVvdidvy8840983;     YIlINmPgLPVsdVvdidvy8840983 = YIlINmPgLPVsdVvdidvy28301750;     YIlINmPgLPVsdVvdidvy28301750 = YIlINmPgLPVsdVvdidvy30279738;     YIlINmPgLPVsdVvdidvy30279738 = YIlINmPgLPVsdVvdidvy26170701;     YIlINmPgLPVsdVvdidvy26170701 = YIlINmPgLPVsdVvdidvy81019051;     YIlINmPgLPVsdVvdidvy81019051 = YIlINmPgLPVsdVvdidvy38112869;     YIlINmPgLPVsdVvdidvy38112869 = YIlINmPgLPVsdVvdidvy18648004;     YIlINmPgLPVsdVvdidvy18648004 = YIlINmPgLPVsdVvdidvy17856484;     YIlINmPgLPVsdVvdidvy17856484 = YIlINmPgLPVsdVvdidvy51309030;     YIlINmPgLPVsdVvdidvy51309030 = YIlINmPgLPVsdVvdidvy66622013;     YIlINmPgLPVsdVvdidvy66622013 = YIlINmPgLPVsdVvdidvy13346452;     YIlINmPgLPVsdVvdidvy13346452 = YIlINmPgLPVsdVvdidvy72994716;     YIlINmPgLPVsdVvdidvy72994716 = YIlINmPgLPVsdVvdidvy30287716;     YIlINmPgLPVsdVvdidvy30287716 = YIlINmPgLPVsdVvdidvy27294516;     YIlINmPgLPVsdVvdidvy27294516 = YIlINmPgLPVsdVvdidvy16856725;     YIlINmPgLPVsdVvdidvy16856725 = YIlINmPgLPVsdVvdidvy97357307;     YIlINmPgLPVsdVvdidvy97357307 = YIlINmPgLPVsdVvdidvy19019526;     YIlINmPgLPVsdVvdidvy19019526 = YIlINmPgLPVsdVvdidvy81925311;     YIlINmPgLPVsdVvdidvy81925311 = YIlINmPgLPVsdVvdidvy97306580;     YIlINmPgLPVsdVvdidvy97306580 = YIlINmPgLPVsdVvdidvy66329789;     YIlINmPgLPVsdVvdidvy66329789 = YIlINmPgLPVsdVvdidvy95934584;     YIlINmPgLPVsdVvdidvy95934584 = YIlINmPgLPVsdVvdidvy41830644;     YIlINmPgLPVsdVvdidvy41830644 = YIlINmPgLPVsdVvdidvy9276052;     YIlINmPgLPVsdVvdidvy9276052 = YIlINmPgLPVsdVvdidvy86487176;     YIlINmPgLPVsdVvdidvy86487176 = YIlINmPgLPVsdVvdidvy43280704;     YIlINmPgLPVsdVvdidvy43280704 = YIlINmPgLPVsdVvdidvy78434675;     YIlINmPgLPVsdVvdidvy78434675 = YIlINmPgLPVsdVvdidvy56096125;     YIlINmPgLPVsdVvdidvy56096125 = YIlINmPgLPVsdVvdidvy73946481;     YIlINmPgLPVsdVvdidvy73946481 = YIlINmPgLPVsdVvdidvy282821;     YIlINmPgLPVsdVvdidvy282821 = YIlINmPgLPVsdVvdidvy30079761;     YIlINmPgLPVsdVvdidvy30079761 = YIlINmPgLPVsdVvdidvy41828159;     YIlINmPgLPVsdVvdidvy41828159 = YIlINmPgLPVsdVvdidvy32505497;     YIlINmPgLPVsdVvdidvy32505497 = YIlINmPgLPVsdVvdidvy13867448;     YIlINmPgLPVsdVvdidvy13867448 = YIlINmPgLPVsdVvdidvy77592993;     YIlINmPgLPVsdVvdidvy77592993 = YIlINmPgLPVsdVvdidvy93351820;     YIlINmPgLPVsdVvdidvy93351820 = YIlINmPgLPVsdVvdidvy40124276;     YIlINmPgLPVsdVvdidvy40124276 = YIlINmPgLPVsdVvdidvy14091700;     YIlINmPgLPVsdVvdidvy14091700 = YIlINmPgLPVsdVvdidvy66979778;     YIlINmPgLPVsdVvdidvy66979778 = YIlINmPgLPVsdVvdidvy78557298;     YIlINmPgLPVsdVvdidvy78557298 = YIlINmPgLPVsdVvdidvy32862042;     YIlINmPgLPVsdVvdidvy32862042 = YIlINmPgLPVsdVvdidvy65085607;     YIlINmPgLPVsdVvdidvy65085607 = YIlINmPgLPVsdVvdidvy78653393;     YIlINmPgLPVsdVvdidvy78653393 = YIlINmPgLPVsdVvdidvy34147540;     YIlINmPgLPVsdVvdidvy34147540 = YIlINmPgLPVsdVvdidvy44889253;     YIlINmPgLPVsdVvdidvy44889253 = YIlINmPgLPVsdVvdidvy78624053;     YIlINmPgLPVsdVvdidvy78624053 = YIlINmPgLPVsdVvdidvy78729069;     YIlINmPgLPVsdVvdidvy78729069 = YIlINmPgLPVsdVvdidvy73817200;     YIlINmPgLPVsdVvdidvy73817200 = YIlINmPgLPVsdVvdidvy66729046;     YIlINmPgLPVsdVvdidvy66729046 = YIlINmPgLPVsdVvdidvy8106703;     YIlINmPgLPVsdVvdidvy8106703 = YIlINmPgLPVsdVvdidvy44312748;     YIlINmPgLPVsdVvdidvy44312748 = YIlINmPgLPVsdVvdidvy79333895;     YIlINmPgLPVsdVvdidvy79333895 = YIlINmPgLPVsdVvdidvy17557913;}
// Junk Finished
