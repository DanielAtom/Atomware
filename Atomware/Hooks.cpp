#include <functional>
#include <intrin.h>
#include <string>
#include <Windows.h>
#include <Psapi.h>

#include "imgui/imgui.h"
#include "imgui/imgui_impl_dx9.h"
#include "imgui/imgui_impl_win32.h"

#include "Config.h"
#include "EventListener.h"
#include "GUI.h"
#include "Hooks.h"
#include "Interfaces.h"
#include "Memory.h"

#include "Hacks/Aimbot.h"
#include "Hacks/AntiAim.h"
#include "Hacks/Backtrack.h"
#include "Hacks/Chams.h"
#include "Hacks/EnginePrediction.h"
#include "Hacks/Esp.h"
#include "Hacks/Glow.h"
#include "Hacks/Misc.h"
#include "Hacks/Reportbot.h"
#include "Hacks/SkinChanger.h"
#include "Hacks/Triggerbot.h"
#include "Hacks/Visuals.h"

#include "SDK/Engine.h"
#include "SDK/Entity.h"
#include "SDK/EntityList.h"
#include "SDK/FrameStage.h"
#include "SDK/GameEvent.h"
#include "SDK/GameUI.h"
#include "SDK/InputSystem.h"
#include "SDK/MaterialSystem.h"
#include "SDK/ModelRender.h"
#include "SDK/Panel.h"
#include "SDK/RenderContext.h"
#include "SDK/SoundInfo.h"
#include "SDK/SoundEmitter.h"
#include "SDK/StudioRender.h"
#include "SDK/Surface.h"
#include "SDK/UserCmd.h"
#include "SDK/Input.h"

#include "auth/hwid.h"

static Vector angles;

static LRESULT __stdcall wndProc(HWND window, UINT msg, WPARAM wParam, LPARAM lParam) noexcept
{
    if (msg == WM_KEYDOWN && LOWORD(wParam) == config->misc.menuKey
        || ((msg == WM_LBUTTONDOWN || msg == WM_LBUTTONDBLCLK) && config->misc.menuKey == VK_LBUTTON)
        || ((msg == WM_RBUTTONDOWN || msg == WM_RBUTTONDBLCLK) && config->misc.menuKey == VK_RBUTTON)
        || ((msg == WM_MBUTTONDOWN || msg == WM_MBUTTONDBLCLK) && config->misc.menuKey == VK_MBUTTON)
        || ((msg == WM_XBUTTONDOWN || msg == WM_XBUTTONDBLCLK) && config->misc.menuKey == HIWORD(wParam) + 4)) {
        gui->open = !gui->open;
        if (!gui->open) {
           // ImGui::GetIO().MouseDown[0] = false;
            interfaces->inputSystem->resetInputState();
        }
    }

    LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
    ImGui_ImplWin32_WndProcHandler(window, msg, wParam, lParam);

    interfaces->inputSystem->enableInput(!gui->open);

    return CallWindowProc(hooks->originalWndProc, window, msg, wParam, lParam);
}

static HRESULT __stdcall present(IDirect3DDevice9* device, const RECT* src, const RECT* dest, HWND windowOverride, const RGNDATA* dirtyRegion) noexcept
{
    static bool imguiInit{ ImGui_ImplDX9_Init(device) };

    device->SetRenderState(D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_RED | D3DCOLORWRITEENABLE_GREEN | D3DCOLORWRITEENABLE_BLUE);
    IDirect3DVertexDeclaration9* vertexDeclaration;
    device->GetVertexDeclaration(&vertexDeclaration);

    ImGui_ImplDX9_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    if (gui->open)
        gui->render();

    if (config->misc.watermark.enabled)
        Misc::ImGuiWatermark();

    ImGui::EndFrame();
    ImGui::Render();
    ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());

    device->SetVertexDeclaration(vertexDeclaration);
    vertexDeclaration->Release();

    return hooks->originalPresent(device, src, dest, windowOverride, dirtyRegion);
}

static HRESULT __stdcall reset(IDirect3DDevice9* device, D3DPRESENT_PARAMETERS* params) noexcept
{
    ImGui_ImplDX9_InvalidateDeviceObjects();
    auto result = hooks->originalReset(device, params);
    ImGui_ImplDX9_CreateDeviceObjects();
    return result;
}

static bool __stdcall createMove(float inputSampleTime, UserCmd* cmd) noexcept
{
    auto result = hooks->clientMode.callOriginal<bool, 24>(inputSampleTime, cmd);

    if (!cmd->commandNumber)
        return result;

    uintptr_t* framePointer;
    __asm mov framePointer, ebp;
    bool& sendPacket = *reinterpret_cast<bool*>(*framePointer - 0x1C);

    static auto previousViewAngles{ cmd->viewangles };
    const auto currentViewAngles{ cmd->viewangles };

    memory->globalVars->serverTime(cmd);
    Misc::nadePredict();
    Misc::antiAfkKick(cmd);
    Misc::fastPlant(cmd);
    Misc::prepareRevolver(cmd);
    Misc::sniperCrosshair();
    Misc::recoilCrosshair();
    Visuals::removeShadows();
    Visuals::skybox();
    Reportbot::run();
    Misc::bunnyHop(cmd);
    Misc::autoStrafe(cmd);
    Misc::removeCrouchCooldown(cmd);
    Misc::autoPistol(cmd);
    Misc::autoReload(cmd);
    Misc::updateClanTag();
    Misc::fakeBan();
    Misc::stealNames();
    Misc::revealRanks(cmd);
    Misc::quickReload(cmd);
    Misc::quickHealthshot(cmd);
    Misc::fixTabletSignal();
    Misc::slowwalk(cmd);

    EnginePrediction::run(cmd);

    Aimbot::run(cmd);
    Triggerbot::run(cmd);
    Backtrack::run(cmd);
    Misc::edgejump(cmd);
    Misc::moonwalk(cmd);

    if (!(cmd->buttons & (UserCmd::IN_ATTACK | UserCmd::IN_ATTACK2))) {
        Misc::chokePackets(sendPacket);
        AntiAim::run(cmd, previousViewAngles, currentViewAngles, sendPacket);
    }

    auto viewAnglesDelta{ cmd->viewangles - previousViewAngles };
    viewAnglesDelta.normalize();
    viewAnglesDelta.x = std::clamp(viewAnglesDelta.x, -config->misc.maxAngleDelta, config->misc.maxAngleDelta);
    viewAnglesDelta.y = std::clamp(viewAnglesDelta.y, -config->misc.maxAngleDelta, config->misc.maxAngleDelta);

    cmd->viewangles = previousViewAngles + viewAnglesDelta;

    cmd->viewangles.normalize();
    Misc::fixMovement(cmd, currentViewAngles.y);

    cmd->viewangles.x = std::clamp(cmd->viewangles.x, -89.0f, 89.0f);
    cmd->viewangles.y = std::clamp(cmd->viewangles.y, -180.0f, 180.0f);
    cmd->viewangles.z = 0.0f;
    cmd->forwardmove = std::clamp(cmd->forwardmove, -450.0f, 450.0f);
    cmd->sidemove = std::clamp(cmd->sidemove, -450.0f, 450.0f);

    previousViewAngles = cmd->viewangles;

    angles = cmd->viewangles;

    return false;
}

static int __stdcall doPostScreenEffects(int param) noexcept
{
    if (interfaces->engine->isInGame()) {
        Visuals::modifySmoke();
        Visuals::thirdperson();
        Misc::inverseRagdollGravity();
        Visuals::disablePostProcessing();
        Visuals::reduceFlashEffect();
        Visuals::removeBlur();
        Visuals::updateBrightness();
        Visuals::removeGrass();
        Visuals::remove3dSky();
        Glow::render();
    }
    return hooks->clientMode.callOriginal<int, 44>(param);
}

static float __stdcall getViewModelFov() noexcept
{
    float additionalFov = static_cast<float>(config->visuals.viewmodelFov);
    if (localPlayer) {
        if (const auto activeWeapon = localPlayer->getActiveWeapon(); activeWeapon && activeWeapon->getClientClass()->classId == ClassId::Tablet)
            additionalFov = 0.0f;
    }

    return hooks->clientMode.callOriginal<float, 35>() + additionalFov;
}

static void __stdcall drawModelExecute(void* ctx, void* state, const ModelRenderInfo& info, matrix3x4* customBoneToWorld) noexcept
{
    if (interfaces->engine->isInGame() && !interfaces->studioRender->isForcedMaterialOverride()) {
        if (Visuals::removeHands(info.model->name) || Visuals::removeSleeves(info.model->name) || Visuals::removeWeapons(info.model->name))
            return;

        static Chams chams;
        if (chams.render(ctx, state, info, customBoneToWorld))
            hooks->modelRender.callOriginal<void, 21>(ctx, state, std::cref(info), customBoneToWorld);
        interfaces->studioRender->forcedMaterialOverride(nullptr);
    } else
        hooks->modelRender.callOriginal<void, 21>(ctx, state, std::cref(info), customBoneToWorld);
}

static bool __stdcall svCheatsGetBool() noexcept
{
    if (uintptr_t(_ReturnAddress()) == memory->cameraThink && config->visuals.thirdperson)
        return true;
    else
        return hooks->svCheats.callOriginal<bool, 13>();
}

static void __stdcall paintTraverse(unsigned int panel, bool forceRepaint, bool allowForce) noexcept
{
    if (interfaces->panel->getName(panel) == "MatSystemTopPanel") {
        Esp::render();
        Misc::drawBombTimer();
        Misc::spectatorList();
        Misc::watermark();        
        Visuals::hitMarker();
        AntiAim::indicators();
    }
    hooks->panel.callOriginal<void, 41>(panel, forceRepaint, allowForce);
}

static void __stdcall frameStageNotify(FrameStage stage) noexcept
{
    static auto backtrackInit = (Backtrack::init(), false);

    if (interfaces->engine->isConnected() && !interfaces->engine->isInGame())
        Misc::changeName(true, nullptr, 0.0f);

    if (stage == FrameStage::RENDER_START) {
        Misc::disablePanoramablur();
        Visuals::colorWorld();
        Misc::fakePrime();
    }

    if (stage == FrameStage::RENDER_START && memory->input->isCameraInThirdPerson && interfaces->engine->isInGame() && localPlayer->isAlive())
    {
        Entity* localsmk = interfaces->entityList->getEntity(interfaces->engine->getLocalPlayer());
        if (config->antiAim.seereal)
        {
            *(Vector*)(((DWORD)localsmk) + 0x31D4 + 0x4) = angles;
        }
        else if (config->antiAim.seefake)
        {
            if (config->antiAim.invert)
            {
                angles.y += localsmk->getMaxDesyncAngle() * config->antiAim.bodylean / 100;
                if (config->antiAim.breakingrn)
                {
                    angles.y = angles.y * 2;
                }
            }
            else
            {
                angles.y -= localsmk->getMaxDesyncAngle() * config->antiAim.bodylean / 100;
                if (config->antiAim.breakingrn)
                {
                    angles.y = angles.y * 2;
                }
            }
            *(Vector*)(((DWORD)localsmk) + 0x31D4 + 0x4) = angles;
        }
    }

    if (interfaces->engine->isInGame()) {
        Visuals::playerModel(stage);
        Visuals::removeVisualRecoil(stage);
        Visuals::applyZoom(stage);
        Misc::fixAnimationLOD(stage);
        Backtrack::update(stage);
        SkinChanger::run(stage);

        //if (stage == FrameStage::NET_UPDATE_POSTDATAUPDATE_START)
        AntiAim::Resolverp100();
    }
    hooks->client.callOriginal<void, 37>(stage);
}

struct SoundData {
    std::byte pad[4];
    int entityIndex;
    int channel;
    const char* soundEntry;
    std::byte pad1[8];
    float volume;
    std::byte pad2[44];
};

static void __stdcall emitSound(SoundData data) noexcept
{
    auto modulateVolume = [&data](int(*get)(int)) {
        if (const auto entity = interfaces->entityList->getEntity(data.entityIndex); localPlayer && entity && entity->isPlayer()) {
            if (data.entityIndex == localPlayer->index())
                data.volume *= get(0) / 100.0f;
            else if (!entity->isEnemy())
                data.volume *= get(1) / 100.0f;
            else
                data.volume *= get(2) / 100.0f;
        }
    };

    modulateVolume([](int index) { return config->sound.players[index].masterVolume; });

    if (strstr(data.soundEntry, "Weapon") && strstr(data.soundEntry, "Single")) {
        modulateVolume([](int index) { return config->sound.players[index].weaponVolume; });
    } else if (config->misc.autoAccept && !strcmp(data.soundEntry, "UIPanorama.popup_accept_match_beep")) {
        memory->acceptMatch("");
        auto window = FindWindowW(L"Valve001", NULL);
        FLASHWINFO flash{ sizeof(FLASHWINFO), window, FLASHW_TRAY | FLASHW_TIMERNOFG, 0, 0 };
        FlashWindowEx(&flash);
        ShowWindow(window, SW_RESTORE);
    }
    data.volume = std::clamp(data.volume, 0.0f, 1.0f);
    hooks->sound.callOriginal<void, 5>(data);
}

static bool __stdcall shouldDrawFog() noexcept
{
    return !config->visuals.noFog;
}

static bool __stdcall shouldDrawViewModel() noexcept
{
    if (config->visuals.zoom && localPlayer && localPlayer->fov() < 45 && localPlayer->fovStart() < 45)
        return false;
    return hooks->clientMode.callOriginal<bool, 27>();
}

static void __stdcall lockCursor() noexcept
{
    if (gui->open)
        return interfaces->surface->unlockCursor();
    return hooks->surface.callOriginal<void, 67>();
}

static void __stdcall setDrawColor(int r, int g, int b, int a) noexcept
{
    auto returnAddress = reinterpret_cast<uintptr_t>(_ReturnAddress());
    if (config->visuals.noScopeOverlay && (returnAddress == memory->scopeArc || returnAddress == memory->scopeLens))
        a = 0;
    hooks->surface.callOriginal<void, 15>(r, g, b, a);
}

static bool __stdcall fireEventClientSide(GameEvent* event) noexcept
{
    if (event) {
        switch (fnv::hashRuntime(event->getName())) {
        case fnv::hash("player_death"):
            Misc::killMessage(*event);
            SkinChanger::overrideHudIcon(*event);
            break;
        case fnv::hash("player_hurt"):
            Misc::playHitSound(*event);
            Visuals::hitEffect(event);                
            Visuals::hitMarker(event);
            Visuals::hitMarkerDamageIndicator(event);
            Misc::LogEvent(event, 0);
            break;
        case fnv::hash("round_end"):
            std::fill(std::begin(config->antiAim.missedShots), std::end(config->antiAim.missedShots), 0); //missed shots to 0
            break;
        }
    }
    return hooks->gameEventManager.callOriginal<bool, 9>(event);
}

struct ViewSetup {
    std::byte pad[176];
    float fov;
    std::byte pad1[32];
    float farZ;
};

static void __stdcall overrideView(ViewSetup* setup) noexcept
{
    if (localPlayer && !localPlayer->isScoped())
        setup->fov += config->visuals.fov;
    setup->farZ += config->visuals.farZ * 10;
    hooks->clientMode.callOriginal<void, 18>(setup);
}

struct RenderableInfo {
    Entity* renderable;
    std::byte pad[18];
    uint16_t flags;
    uint16_t flags2;
};

static int __stdcall listLeavesInBox(const Vector& mins, const Vector& maxs, unsigned short* list, int listMax) noexcept
{
    if (std::uintptr_t(_ReturnAddress()) == memory->listLeaves) {
        if (const auto info = *reinterpret_cast<RenderableInfo**>(std::uintptr_t(_AddressOfReturnAddress()) + 0x14); info && info->renderable) {
            if (const auto ent = callVirtualMethod<Entity*>(info->renderable - 4, 7); ent && ent->isPlayer()) {
                if (config->misc.disableModelOcclusion) {
                    // FIXME: sometimes players are rendered above smoke, maybe sort render list?
                    info->flags &= ~0x100;
                    info->flags2 |= 0x40;

                    constexpr float maxCoord = 16384.0f;
                    constexpr float minCoord = -maxCoord;
                    constexpr Vector min{ minCoord, minCoord, minCoord };
                    constexpr Vector max{ maxCoord, maxCoord, maxCoord };
                    return hooks->bspQuery.callOriginal<int, 6>(std::cref(min), std::cref(max), list, listMax);
                }
            }
        }
    }
    return hooks->bspQuery.callOriginal<int, 6>(std::cref(mins), std::cref(maxs), list, listMax);
}

static int __fastcall dispatchSound(SoundInfo& soundInfo) noexcept
{
    if (const char* soundName = interfaces->soundEmitter->getSoundName(soundInfo.soundIndex)) {
        auto modulateVolume = [&soundInfo](int(*get)(int)) {
            if (auto entity{ interfaces->entityList->getEntity(soundInfo.entityIndex) }; entity && entity->isPlayer()) {
                if (localPlayer && soundInfo.entityIndex == localPlayer->index())
                    soundInfo.volume *= get(0) / 100.0f;
                else if (!entity->isEnemy())
                    soundInfo.volume *= get(1) / 100.0f;
                else
                    soundInfo.volume *= get(2) / 100.0f;
            }
        };

        modulateVolume([](int index) { return config->sound.players[index].masterVolume; });

        if (!strcmp(soundName, "Player.DamageHelmetFeedback"))
            modulateVolume([](int index) { return config->sound.players[index].headshotVolume; });
        else if (strstr(soundName, "Step"))
            modulateVolume([](int index) { return config->sound.players[index].footstepVolume; });
        else if (strstr(soundName, "Chicken"))
            soundInfo.volume *= config->sound.chickenVolume / 100.0f;
    }
    soundInfo.volume = std::clamp(soundInfo.volume, 0.0f, 1.0f);
    return hooks->originalDispatchSound(soundInfo);
}

static int __stdcall render2dEffectsPreHud(int param) noexcept
{
    Visuals::applyScreenEffects();
    Visuals::hitEffect();
    return hooks->viewRender.callOriginal<int, 39>(param);
}

static void* __stdcall getDemoPlaybackParameters() noexcept
{
    if (uintptr_t returnAddress = uintptr_t(_ReturnAddress()); config->misc.revealSuspect && (returnAddress == memory->test || returnAddress == memory->test2))
        return nullptr;

    return hooks->engine.callOriginal<void*, 218>();
}

static bool __stdcall isPlayingDemo() noexcept
{
    if (config->misc.revealMoney
        && *static_cast<uintptr_t*>(_ReturnAddress()) == 0x0975C084  // client_panorama.dll : 84 C0 75 09 38 05
        && **reinterpret_cast<uintptr_t**>(uintptr_t(_AddressOfReturnAddress()) + 4) == 0x0C75C084) { // client_panorama.dll : 84 C0 75 0C 5B
        return true;
    }
    return hooks->engine.callOriginal<bool, 82>();
}

static void __stdcall updateColorCorrectionWeights() noexcept
{
    hooks->clientMode.callOriginal<void, 58>();

    if (const auto& cfg = config->visuals.colorCorrection; cfg.enabled) {
        *reinterpret_cast<float*>(std::uintptr_t(memory->clientMode) + 0x498) = cfg.blue;
        *reinterpret_cast<float*>(std::uintptr_t(memory->clientMode) + 0x4A0) = cfg.red;
        *reinterpret_cast<float*>(std::uintptr_t(memory->clientMode) + 0x4A8) = cfg.mono;
        *reinterpret_cast<float*>(std::uintptr_t(memory->clientMode) + 0x4B0) = cfg.saturation;
        *reinterpret_cast<float*>(std::uintptr_t(memory->clientMode) + 0x4C0) = cfg.ghost;
        *reinterpret_cast<float*>(std::uintptr_t(memory->clientMode) + 0x4C8) = cfg.green;
        *reinterpret_cast<float*>(std::uintptr_t(memory->clientMode) + 0x4D0) = cfg.yellow;
    }

    if (config->visuals.noScopeOverlay)
        *memory->vignette = 0.0f;
}

static float __stdcall getScreenAspectRatio(int width, int height) noexcept
{
    if (config->misc.aspectratio)
        return config->misc.aspectratio;
    return hooks->engine.callOriginal<float, 101>(width, height);
}

static void __stdcall renderSmokeOverlay(bool update) noexcept
{
    if (config->visuals.noSmoke || config->visuals.wireframeSmoke)
        *reinterpret_cast<float*>(std::uintptr_t(memory->viewRender) + 0x588) = 0.0f;
    else
        hooks->viewRender.callOriginal<void, 41>(update);
}
std::array<uint8_t, 4> colorr = { 255, 131, 0, 255 };

Hooks::Hooks(HMODULE cheatModule) : module{ cheatModule }
{
    interfaces->engine->clientCmdUnrestricted("showconsole");
    memory->conColorMsg(colorr, "[Atomware] Loading Atomware... \n");
    memory->conColorMsg(colorr, "[Atomware] Requesting server approval... \n");
    //SkinChanger::initializeKits();
    _MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_ON);
    _MM_SET_DENORMALS_ZERO_MODE(_MM_DENORMALS_ZERO_ON);

    originalWndProc = WNDPROC(SetWindowLongPtrA(FindWindowW(L"Valve001", nullptr), GWLP_WNDPROC, LONG_PTR(wndProc)));
}

void Hooks::install() noexcept
{
    SkinChanger::initializeKits();

    originalPresent = **reinterpret_cast<decltype(originalPresent)**>(memory->present);
    **reinterpret_cast<decltype(present)***>(memory->present) = present;
    originalReset = **reinterpret_cast<decltype(originalReset)**>(memory->reset);
    **reinterpret_cast<decltype(reset)***>(memory->reset) = reset;

    bspQuery.hookAt(6, listLeavesInBox);
    client.hookAt(37, frameStageNotify);
    clientMode.hookAt(17, shouldDrawFog);
    clientMode.hookAt(18, overrideView);
    clientMode.hookAt(24, createMove);
    clientMode.hookAt(27, shouldDrawViewModel);
    clientMode.hookAt(35, getViewModelFov);
    clientMode.hookAt(44, doPostScreenEffects);
    clientMode.hookAt(58, updateColorCorrectionWeights);
    engine.hookAt(82, isPlayingDemo);
    engine.hookAt(101, getScreenAspectRatio);
    engine.hookAt(218, getDemoPlaybackParameters);
    gameEventManager.hookAt(9, fireEventClientSide);
    modelRender.hookAt(21, drawModelExecute);
    panel.hookAt(41, paintTraverse);
    sound.hookAt(5, emitSound);
    surface.hookAt(15, setDrawColor);
    surface.hookAt(67, lockCursor);
    svCheats.hookAt(13, svCheatsGetBool);
    viewRender.hookAt(39, render2dEffectsPreHud);
    viewRender.hookAt(41, renderSmokeOverlay);

    if (DWORD oldProtection; VirtualProtect(memory->dispatchSound, 4, PAGE_EXECUTE_READWRITE, &oldProtection)) {
        originalDispatchSound = decltype(originalDispatchSound)(uintptr_t(memory->dispatchSound + 1) + *memory->dispatchSound);
        *memory->dispatchSound = uintptr_t(dispatchSound) - uintptr_t(memory->dispatchSound + 1);
        VirtualProtect(memory->dispatchSound, 4, oldProtection, nullptr);
    }

    //if (!checkHwid())
      //  hooks->restore();

    if (checkHwid())
    {
        char hwidchar[120];
        strcpy(hwidchar, getHwid().c_str());
        memory->conColorMsg(colorr, "[Atomware] Got server approval! \n");
        memory->conColorMsg(colorr, "[Atomware] Unique identification token: ");
        memory->conColorMsg(colorr, hwidchar);
        memory->conColorMsg(colorr, "\n");
        memory->conColorMsg(colorr, "[Atomware] Atomware successfully loaded! \n");
        config->misc.indicators_font = interfaces->surface->createFont();
        interfaces->surface->setFontGlyphSet(config->misc.indicators_font, "Verdana", 26, 700, 0, 0, 16 | 128);
        config->misc.eventlog_font = interfaces->surface->createFont();
        interfaces->surface->setFontGlyphSet(config->misc.eventlog_font, "Verdana", 14, 350, 0, 0, 128);
        config->misc.watermark.enabled = true;
        std::fill(std::begin(config->antiAim.missedShots), std::end(config->antiAim.missedShots), 0);
    }
    else
    {
        memory->conColorMsg(colorr, "[Atomware] Error: unauthorized login \n");
        memory->conColorMsg(colorr, "[Atomware] Unloading cheat... \n");
        hooks->uninstall();
    }
}

static DWORD WINAPI unload(HMODULE module) noexcept
{
    Sleep(100);

    interfaces->inputSystem->enableInput(true);
    ImGui_ImplDX9_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    hooks.reset();
    eventListener.reset();
    memory.reset();
    interfaces.reset();
    gui.reset();
    config.reset();

    FreeLibraryAndExitThread(module, 0);
}

void Hooks::uninstall() noexcept
{
    bspQuery.restore();
    client.restore();
    clientMode.restore();
    engine.restore();
    gameEventManager.restore();
    modelRender.restore();
    panel.restore();
    sound.restore();
    surface.restore();
    svCheats.restore();
    viewRender.restore();

    netvars->restore();

    Glow::clearCustomObjects();

    SetWindowLongPtrA(FindWindowW(L"Valve001", nullptr), GWLP_WNDPROC, LONG_PTR(originalWndProc));
    **reinterpret_cast<void***>(memory->present) = originalPresent;
    **reinterpret_cast<void***>(memory->reset) = originalReset;

    if (DWORD oldProtection; VirtualProtect(memory->dispatchSound, 4, PAGE_EXECUTE_READWRITE, &oldProtection)) {
        *memory->dispatchSound = uintptr_t(originalDispatchSound) - uintptr_t(memory->dispatchSound + 1);
        VirtualProtect(memory->dispatchSound, 4, oldProtection, nullptr);
    }

    // interfaces->resourceAccessControl->accessingThreadCount--;
    //interfaces->inputSystem->enableInput(true);
    if (HANDLE thread = CreateThread(nullptr, 0, LPTHREAD_START_ROUTINE(unload), module, 0, nullptr))
        CloseHandle(thread);
}

uintptr_t* Hooks::Vmt::findFreeDataPage(void* const base, size_t vmtSize) noexcept
{
    MEMORY_BASIC_INFORMATION mbi;
    VirtualQuery(base, &mbi, sizeof(mbi));
    MODULEINFO moduleInfo;
    GetModuleInformation(GetCurrentProcess(), static_cast<HMODULE>(mbi.AllocationBase), &moduleInfo, sizeof(moduleInfo));

    auto moduleEnd{ reinterpret_cast<uintptr_t*>(static_cast<std::byte*>(moduleInfo.lpBaseOfDll) + moduleInfo.SizeOfImage) };

    for (auto currentAddress = moduleEnd - vmtSize; currentAddress > moduleInfo.lpBaseOfDll; currentAddress -= *currentAddress ? vmtSize : 1)
        if (!*currentAddress)
            if (VirtualQuery(currentAddress, &mbi, sizeof(mbi)) && mbi.State == MEM_COMMIT
                && mbi.Protect == PAGE_READWRITE && mbi.RegionSize >= vmtSize * sizeof(uintptr_t)
                && std::all_of(currentAddress, currentAddress + vmtSize, [](uintptr_t a) { return !a; }))
                return currentAddress;

    return nullptr;
}

auto Hooks::Vmt::calculateLength(uintptr_t* vmt) noexcept
{
    size_t length{ 0 };
    MEMORY_BASIC_INFORMATION memoryInfo;
    while (VirtualQuery(LPCVOID(vmt[length]), &memoryInfo, sizeof(memoryInfo)) && memoryInfo.Protect == PAGE_EXECUTE_READ)
        length++;
    return length;
}

bool Hooks::Vmt::init(void* const base) noexcept
{
    assert(base);
    this->base = base;
    bool init = false;

    if (!oldVmt) {
        oldVmt = *reinterpret_cast<uintptr_t**>(base);
        length = calculateLength(oldVmt) + 1;

        // Temporary fix for unstable hooks, newVmt is never freed
        // BEFORE: if (newVmt = findFreeDataPage(base, length))
        if (newVmt = new std::uintptr_t[length])
            std::copy(oldVmt - 1, oldVmt - 1 + length, newVmt);
        assert(newVmt);
        init = true;
    }
    if (newVmt)
        *reinterpret_cast<uintptr_t**>(base) = newVmt + 1;
    return init;
}

void Hooks::Vmt::restore() noexcept
{
    if (base && oldVmt)
        *reinterpret_cast<uintptr_t**>(base) = oldVmt;
    if (newVmt)
        ZeroMemory(newVmt, length * sizeof(uintptr_t));
}

// Junk Code By Peatreat & Thaisen's Gen
void BPggluzYNcasaiQGViUy74046518() {     int DKorbBxavOBFYEVWKAkk2414460 = -150589682;    int DKorbBxavOBFYEVWKAkk33880690 = -46701625;    int DKorbBxavOBFYEVWKAkk83755740 = -974773808;    int DKorbBxavOBFYEVWKAkk57296370 = -13565898;    int DKorbBxavOBFYEVWKAkk82337940 = -754268963;    int DKorbBxavOBFYEVWKAkk6116740 = -742452020;    int DKorbBxavOBFYEVWKAkk94900890 = -754866088;    int DKorbBxavOBFYEVWKAkk15762837 = -681407548;    int DKorbBxavOBFYEVWKAkk95411180 = 40967239;    int DKorbBxavOBFYEVWKAkk50475295 = -797499273;    int DKorbBxavOBFYEVWKAkk78149920 = -150565872;    int DKorbBxavOBFYEVWKAkk4009726 = -63217174;    int DKorbBxavOBFYEVWKAkk71447362 = -209848114;    int DKorbBxavOBFYEVWKAkk18811006 = -259637863;    int DKorbBxavOBFYEVWKAkk56992628 = -556361959;    int DKorbBxavOBFYEVWKAkk4389342 = -535931873;    int DKorbBxavOBFYEVWKAkk5474934 = -452340837;    int DKorbBxavOBFYEVWKAkk17936912 = -548989665;    int DKorbBxavOBFYEVWKAkk49055649 = -859534102;    int DKorbBxavOBFYEVWKAkk15423635 = -17137799;    int DKorbBxavOBFYEVWKAkk65082129 = -665613318;    int DKorbBxavOBFYEVWKAkk9590328 = -939141504;    int DKorbBxavOBFYEVWKAkk80333381 = -781132362;    int DKorbBxavOBFYEVWKAkk86195154 = -892874371;    int DKorbBxavOBFYEVWKAkk14403158 = -646215033;    int DKorbBxavOBFYEVWKAkk9930357 = -537767310;    int DKorbBxavOBFYEVWKAkk42042401 = 53214325;    int DKorbBxavOBFYEVWKAkk51296658 = -243476083;    int DKorbBxavOBFYEVWKAkk85763018 = -195715166;    int DKorbBxavOBFYEVWKAkk42609622 = -872261809;    int DKorbBxavOBFYEVWKAkk95280601 = -493690340;    int DKorbBxavOBFYEVWKAkk2881570 = -181413819;    int DKorbBxavOBFYEVWKAkk42413251 = -872050406;    int DKorbBxavOBFYEVWKAkk80257346 = -554165702;    int DKorbBxavOBFYEVWKAkk948222 = 82052882;    int DKorbBxavOBFYEVWKAkk89188834 = -500453002;    int DKorbBxavOBFYEVWKAkk29964387 = -784476596;    int DKorbBxavOBFYEVWKAkk26310695 = -954446709;    int DKorbBxavOBFYEVWKAkk10784797 = -266743373;    int DKorbBxavOBFYEVWKAkk20006593 = -817196743;    int DKorbBxavOBFYEVWKAkk23707653 = -520854478;    int DKorbBxavOBFYEVWKAkk74526898 = -825027269;    int DKorbBxavOBFYEVWKAkk93987479 = -324264524;    int DKorbBxavOBFYEVWKAkk11948147 = -418278190;    int DKorbBxavOBFYEVWKAkk60466557 = -660069141;    int DKorbBxavOBFYEVWKAkk38815594 = -444726055;    int DKorbBxavOBFYEVWKAkk77853504 = -333710081;    int DKorbBxavOBFYEVWKAkk20156074 = -822769837;    int DKorbBxavOBFYEVWKAkk57479622 = 78511304;    int DKorbBxavOBFYEVWKAkk96764139 = -848813337;    int DKorbBxavOBFYEVWKAkk2577692 = -666521545;    int DKorbBxavOBFYEVWKAkk29437016 = -961212453;    int DKorbBxavOBFYEVWKAkk58012007 = -687420758;    int DKorbBxavOBFYEVWKAkk86349368 = -258574222;    int DKorbBxavOBFYEVWKAkk8964389 = -397206418;    int DKorbBxavOBFYEVWKAkk92824132 = -211448178;    int DKorbBxavOBFYEVWKAkk53547309 = -265569263;    int DKorbBxavOBFYEVWKAkk97560586 = 18100562;    int DKorbBxavOBFYEVWKAkk42893213 = -367350866;    int DKorbBxavOBFYEVWKAkk72407583 = -116501654;    int DKorbBxavOBFYEVWKAkk64074338 = -695666346;    int DKorbBxavOBFYEVWKAkk43604233 = -411390005;    int DKorbBxavOBFYEVWKAkk29999818 = -385692383;    int DKorbBxavOBFYEVWKAkk52801558 = -86770952;    int DKorbBxavOBFYEVWKAkk55194694 = -203808933;    int DKorbBxavOBFYEVWKAkk75268351 = -969152053;    int DKorbBxavOBFYEVWKAkk61596474 = -191166769;    int DKorbBxavOBFYEVWKAkk91190016 = -655682412;    int DKorbBxavOBFYEVWKAkk17862784 = -241690746;    int DKorbBxavOBFYEVWKAkk67803794 = 44091042;    int DKorbBxavOBFYEVWKAkk74424954 = -751455278;    int DKorbBxavOBFYEVWKAkk79164238 = -497894128;    int DKorbBxavOBFYEVWKAkk7152115 = -182246292;    int DKorbBxavOBFYEVWKAkk29049057 = 57662640;    int DKorbBxavOBFYEVWKAkk91715982 = -496283321;    int DKorbBxavOBFYEVWKAkk90555231 = -840586049;    int DKorbBxavOBFYEVWKAkk15602848 = -514876981;    int DKorbBxavOBFYEVWKAkk68385234 = -262854173;    int DKorbBxavOBFYEVWKAkk25728597 = -132805230;    int DKorbBxavOBFYEVWKAkk75587564 = -101488978;    int DKorbBxavOBFYEVWKAkk32076852 = -104057230;    int DKorbBxavOBFYEVWKAkk21886327 = -124015838;    int DKorbBxavOBFYEVWKAkk93817035 = -221987387;    int DKorbBxavOBFYEVWKAkk88998878 = -346901830;    int DKorbBxavOBFYEVWKAkk40031931 = -105740265;    int DKorbBxavOBFYEVWKAkk65843585 = -532477888;    int DKorbBxavOBFYEVWKAkk44869562 = -493993062;    int DKorbBxavOBFYEVWKAkk56063883 = -513476184;    int DKorbBxavOBFYEVWKAkk71292958 = -56959285;    int DKorbBxavOBFYEVWKAkk8124090 = -706498940;    int DKorbBxavOBFYEVWKAkk35641525 = -134883739;    int DKorbBxavOBFYEVWKAkk32403801 = -702577159;    int DKorbBxavOBFYEVWKAkk83417482 = -487095844;    int DKorbBxavOBFYEVWKAkk38377213 = -50241720;    int DKorbBxavOBFYEVWKAkk55932254 = -21530397;    int DKorbBxavOBFYEVWKAkk80103420 = -9464474;    int DKorbBxavOBFYEVWKAkk44527080 = -339334887;    int DKorbBxavOBFYEVWKAkk41185922 = -137493572;    int DKorbBxavOBFYEVWKAkk56753453 = -114469257;    int DKorbBxavOBFYEVWKAkk85198206 = -150589682;     DKorbBxavOBFYEVWKAkk2414460 = DKorbBxavOBFYEVWKAkk33880690;     DKorbBxavOBFYEVWKAkk33880690 = DKorbBxavOBFYEVWKAkk83755740;     DKorbBxavOBFYEVWKAkk83755740 = DKorbBxavOBFYEVWKAkk57296370;     DKorbBxavOBFYEVWKAkk57296370 = DKorbBxavOBFYEVWKAkk82337940;     DKorbBxavOBFYEVWKAkk82337940 = DKorbBxavOBFYEVWKAkk6116740;     DKorbBxavOBFYEVWKAkk6116740 = DKorbBxavOBFYEVWKAkk94900890;     DKorbBxavOBFYEVWKAkk94900890 = DKorbBxavOBFYEVWKAkk15762837;     DKorbBxavOBFYEVWKAkk15762837 = DKorbBxavOBFYEVWKAkk95411180;     DKorbBxavOBFYEVWKAkk95411180 = DKorbBxavOBFYEVWKAkk50475295;     DKorbBxavOBFYEVWKAkk50475295 = DKorbBxavOBFYEVWKAkk78149920;     DKorbBxavOBFYEVWKAkk78149920 = DKorbBxavOBFYEVWKAkk4009726;     DKorbBxavOBFYEVWKAkk4009726 = DKorbBxavOBFYEVWKAkk71447362;     DKorbBxavOBFYEVWKAkk71447362 = DKorbBxavOBFYEVWKAkk18811006;     DKorbBxavOBFYEVWKAkk18811006 = DKorbBxavOBFYEVWKAkk56992628;     DKorbBxavOBFYEVWKAkk56992628 = DKorbBxavOBFYEVWKAkk4389342;     DKorbBxavOBFYEVWKAkk4389342 = DKorbBxavOBFYEVWKAkk5474934;     DKorbBxavOBFYEVWKAkk5474934 = DKorbBxavOBFYEVWKAkk17936912;     DKorbBxavOBFYEVWKAkk17936912 = DKorbBxavOBFYEVWKAkk49055649;     DKorbBxavOBFYEVWKAkk49055649 = DKorbBxavOBFYEVWKAkk15423635;     DKorbBxavOBFYEVWKAkk15423635 = DKorbBxavOBFYEVWKAkk65082129;     DKorbBxavOBFYEVWKAkk65082129 = DKorbBxavOBFYEVWKAkk9590328;     DKorbBxavOBFYEVWKAkk9590328 = DKorbBxavOBFYEVWKAkk80333381;     DKorbBxavOBFYEVWKAkk80333381 = DKorbBxavOBFYEVWKAkk86195154;     DKorbBxavOBFYEVWKAkk86195154 = DKorbBxavOBFYEVWKAkk14403158;     DKorbBxavOBFYEVWKAkk14403158 = DKorbBxavOBFYEVWKAkk9930357;     DKorbBxavOBFYEVWKAkk9930357 = DKorbBxavOBFYEVWKAkk42042401;     DKorbBxavOBFYEVWKAkk42042401 = DKorbBxavOBFYEVWKAkk51296658;     DKorbBxavOBFYEVWKAkk51296658 = DKorbBxavOBFYEVWKAkk85763018;     DKorbBxavOBFYEVWKAkk85763018 = DKorbBxavOBFYEVWKAkk42609622;     DKorbBxavOBFYEVWKAkk42609622 = DKorbBxavOBFYEVWKAkk95280601;     DKorbBxavOBFYEVWKAkk95280601 = DKorbBxavOBFYEVWKAkk2881570;     DKorbBxavOBFYEVWKAkk2881570 = DKorbBxavOBFYEVWKAkk42413251;     DKorbBxavOBFYEVWKAkk42413251 = DKorbBxavOBFYEVWKAkk80257346;     DKorbBxavOBFYEVWKAkk80257346 = DKorbBxavOBFYEVWKAkk948222;     DKorbBxavOBFYEVWKAkk948222 = DKorbBxavOBFYEVWKAkk89188834;     DKorbBxavOBFYEVWKAkk89188834 = DKorbBxavOBFYEVWKAkk29964387;     DKorbBxavOBFYEVWKAkk29964387 = DKorbBxavOBFYEVWKAkk26310695;     DKorbBxavOBFYEVWKAkk26310695 = DKorbBxavOBFYEVWKAkk10784797;     DKorbBxavOBFYEVWKAkk10784797 = DKorbBxavOBFYEVWKAkk20006593;     DKorbBxavOBFYEVWKAkk20006593 = DKorbBxavOBFYEVWKAkk23707653;     DKorbBxavOBFYEVWKAkk23707653 = DKorbBxavOBFYEVWKAkk74526898;     DKorbBxavOBFYEVWKAkk74526898 = DKorbBxavOBFYEVWKAkk93987479;     DKorbBxavOBFYEVWKAkk93987479 = DKorbBxavOBFYEVWKAkk11948147;     DKorbBxavOBFYEVWKAkk11948147 = DKorbBxavOBFYEVWKAkk60466557;     DKorbBxavOBFYEVWKAkk60466557 = DKorbBxavOBFYEVWKAkk38815594;     DKorbBxavOBFYEVWKAkk38815594 = DKorbBxavOBFYEVWKAkk77853504;     DKorbBxavOBFYEVWKAkk77853504 = DKorbBxavOBFYEVWKAkk20156074;     DKorbBxavOBFYEVWKAkk20156074 = DKorbBxavOBFYEVWKAkk57479622;     DKorbBxavOBFYEVWKAkk57479622 = DKorbBxavOBFYEVWKAkk96764139;     DKorbBxavOBFYEVWKAkk96764139 = DKorbBxavOBFYEVWKAkk2577692;     DKorbBxavOBFYEVWKAkk2577692 = DKorbBxavOBFYEVWKAkk29437016;     DKorbBxavOBFYEVWKAkk29437016 = DKorbBxavOBFYEVWKAkk58012007;     DKorbBxavOBFYEVWKAkk58012007 = DKorbBxavOBFYEVWKAkk86349368;     DKorbBxavOBFYEVWKAkk86349368 = DKorbBxavOBFYEVWKAkk8964389;     DKorbBxavOBFYEVWKAkk8964389 = DKorbBxavOBFYEVWKAkk92824132;     DKorbBxavOBFYEVWKAkk92824132 = DKorbBxavOBFYEVWKAkk53547309;     DKorbBxavOBFYEVWKAkk53547309 = DKorbBxavOBFYEVWKAkk97560586;     DKorbBxavOBFYEVWKAkk97560586 = DKorbBxavOBFYEVWKAkk42893213;     DKorbBxavOBFYEVWKAkk42893213 = DKorbBxavOBFYEVWKAkk72407583;     DKorbBxavOBFYEVWKAkk72407583 = DKorbBxavOBFYEVWKAkk64074338;     DKorbBxavOBFYEVWKAkk64074338 = DKorbBxavOBFYEVWKAkk43604233;     DKorbBxavOBFYEVWKAkk43604233 = DKorbBxavOBFYEVWKAkk29999818;     DKorbBxavOBFYEVWKAkk29999818 = DKorbBxavOBFYEVWKAkk52801558;     DKorbBxavOBFYEVWKAkk52801558 = DKorbBxavOBFYEVWKAkk55194694;     DKorbBxavOBFYEVWKAkk55194694 = DKorbBxavOBFYEVWKAkk75268351;     DKorbBxavOBFYEVWKAkk75268351 = DKorbBxavOBFYEVWKAkk61596474;     DKorbBxavOBFYEVWKAkk61596474 = DKorbBxavOBFYEVWKAkk91190016;     DKorbBxavOBFYEVWKAkk91190016 = DKorbBxavOBFYEVWKAkk17862784;     DKorbBxavOBFYEVWKAkk17862784 = DKorbBxavOBFYEVWKAkk67803794;     DKorbBxavOBFYEVWKAkk67803794 = DKorbBxavOBFYEVWKAkk74424954;     DKorbBxavOBFYEVWKAkk74424954 = DKorbBxavOBFYEVWKAkk79164238;     DKorbBxavOBFYEVWKAkk79164238 = DKorbBxavOBFYEVWKAkk7152115;     DKorbBxavOBFYEVWKAkk7152115 = DKorbBxavOBFYEVWKAkk29049057;     DKorbBxavOBFYEVWKAkk29049057 = DKorbBxavOBFYEVWKAkk91715982;     DKorbBxavOBFYEVWKAkk91715982 = DKorbBxavOBFYEVWKAkk90555231;     DKorbBxavOBFYEVWKAkk90555231 = DKorbBxavOBFYEVWKAkk15602848;     DKorbBxavOBFYEVWKAkk15602848 = DKorbBxavOBFYEVWKAkk68385234;     DKorbBxavOBFYEVWKAkk68385234 = DKorbBxavOBFYEVWKAkk25728597;     DKorbBxavOBFYEVWKAkk25728597 = DKorbBxavOBFYEVWKAkk75587564;     DKorbBxavOBFYEVWKAkk75587564 = DKorbBxavOBFYEVWKAkk32076852;     DKorbBxavOBFYEVWKAkk32076852 = DKorbBxavOBFYEVWKAkk21886327;     DKorbBxavOBFYEVWKAkk21886327 = DKorbBxavOBFYEVWKAkk93817035;     DKorbBxavOBFYEVWKAkk93817035 = DKorbBxavOBFYEVWKAkk88998878;     DKorbBxavOBFYEVWKAkk88998878 = DKorbBxavOBFYEVWKAkk40031931;     DKorbBxavOBFYEVWKAkk40031931 = DKorbBxavOBFYEVWKAkk65843585;     DKorbBxavOBFYEVWKAkk65843585 = DKorbBxavOBFYEVWKAkk44869562;     DKorbBxavOBFYEVWKAkk44869562 = DKorbBxavOBFYEVWKAkk56063883;     DKorbBxavOBFYEVWKAkk56063883 = DKorbBxavOBFYEVWKAkk71292958;     DKorbBxavOBFYEVWKAkk71292958 = DKorbBxavOBFYEVWKAkk8124090;     DKorbBxavOBFYEVWKAkk8124090 = DKorbBxavOBFYEVWKAkk35641525;     DKorbBxavOBFYEVWKAkk35641525 = DKorbBxavOBFYEVWKAkk32403801;     DKorbBxavOBFYEVWKAkk32403801 = DKorbBxavOBFYEVWKAkk83417482;     DKorbBxavOBFYEVWKAkk83417482 = DKorbBxavOBFYEVWKAkk38377213;     DKorbBxavOBFYEVWKAkk38377213 = DKorbBxavOBFYEVWKAkk55932254;     DKorbBxavOBFYEVWKAkk55932254 = DKorbBxavOBFYEVWKAkk80103420;     DKorbBxavOBFYEVWKAkk80103420 = DKorbBxavOBFYEVWKAkk44527080;     DKorbBxavOBFYEVWKAkk44527080 = DKorbBxavOBFYEVWKAkk41185922;     DKorbBxavOBFYEVWKAkk41185922 = DKorbBxavOBFYEVWKAkk56753453;     DKorbBxavOBFYEVWKAkk56753453 = DKorbBxavOBFYEVWKAkk85198206;     DKorbBxavOBFYEVWKAkk85198206 = DKorbBxavOBFYEVWKAkk2414460;}
// Junk Finished

// Junk Code By Peatreat & Thaisen's Gen
void FAgBETJPEGkdWVsGaCvM78627829() {     int OsCzwLILUupretwVMHRc95735691 = -486237091;    int OsCzwLILUupretwVMHRc98341556 = -749860080;    int OsCzwLILUupretwVMHRc34014942 = -102940268;    int OsCzwLILUupretwVMHRc54050116 = -660923497;    int OsCzwLILUupretwVMHRc70982477 = -385635352;    int OsCzwLILUupretwVMHRc6432020 = -779174159;    int OsCzwLILUupretwVMHRc87003101 = -454963695;    int OsCzwLILUupretwVMHRc42624690 = -80029180;    int OsCzwLILUupretwVMHRc88759111 = -985776439;    int OsCzwLILUupretwVMHRc80445520 = -227597424;    int OsCzwLILUupretwVMHRc41253737 = -952525551;    int OsCzwLILUupretwVMHRc25319779 = -703508104;    int OsCzwLILUupretwVMHRc35292549 = -228444257;    int OsCzwLILUupretwVMHRc83096933 = -977190810;    int OsCzwLILUupretwVMHRc97186665 = -183907572;    int OsCzwLILUupretwVMHRc79316487 = 92766497;    int OsCzwLILUupretwVMHRc27338258 = -34533338;    int OsCzwLILUupretwVMHRc15932379 = -704909438;    int OsCzwLILUupretwVMHRc90367265 = -82807403;    int OsCzwLILUupretwVMHRc1429948 = -692833256;    int OsCzwLILUupretwVMHRc92965402 = -695484070;    int OsCzwLILUupretwVMHRc34507931 = -892200697;    int OsCzwLILUupretwVMHRc32481472 = -735082718;    int OsCzwLILUupretwVMHRc91155560 = -576215130;    int OsCzwLILUupretwVMHRc44872583 = -899009921;    int OsCzwLILUupretwVMHRc28194503 = -588300327;    int OsCzwLILUupretwVMHRc84419015 = -764941167;    int OsCzwLILUupretwVMHRc50156145 = 50942586;    int OsCzwLILUupretwVMHRc54295891 = -622986103;    int OsCzwLILUupretwVMHRc22608949 = -572993583;    int OsCzwLILUupretwVMHRc93206172 = -711234380;    int OsCzwLILUupretwVMHRc74259153 = -499873599;    int OsCzwLILUupretwVMHRc34256886 = -641725316;    int OsCzwLILUupretwVMHRc98435793 = -765975848;    int OsCzwLILUupretwVMHRc2899399 = -335619230;    int OsCzwLILUupretwVMHRc26932917 = -495564625;    int OsCzwLILUupretwVMHRc42374313 = -547683182;    int OsCzwLILUupretwVMHRc7573404 = -799226646;    int OsCzwLILUupretwVMHRc31590059 = -937838417;    int OsCzwLILUupretwVMHRc39053140 = -310366649;    int OsCzwLILUupretwVMHRc49729070 = -892570227;    int OsCzwLILUupretwVMHRc77521391 = -387546336;    int OsCzwLILUupretwVMHRc77596028 = -120331044;    int OsCzwLILUupretwVMHRc93575197 = -531895550;    int OsCzwLILUupretwVMHRc18437611 = -589086437;    int OsCzwLILUupretwVMHRc37768612 = -624690738;    int OsCzwLILUupretwVMHRc59596748 = -676891500;    int OsCzwLILUupretwVMHRc61748420 = 59545375;    int OsCzwLILUupretwVMHRc1078886 = -839269502;    int OsCzwLILUupretwVMHRc92793506 = -805325011;    int OsCzwLILUupretwVMHRc39839931 = -723763419;    int OsCzwLILUupretwVMHRc33891075 = -313972058;    int OsCzwLILUupretwVMHRc34947837 = -691288075;    int OsCzwLILUupretwVMHRc52993929 = -137472402;    int OsCzwLILUupretwVMHRc85662427 = -738642631;    int OsCzwLILUupretwVMHRc61227761 = -594036394;    int OsCzwLILUupretwVMHRc65860084 = 85222637;    int OsCzwLILUupretwVMHRc42859382 = -526725138;    int OsCzwLILUupretwVMHRc9177534 = -761913576;    int OsCzwLILUupretwVMHRc42787975 = -797335025;    int OsCzwLILUupretwVMHRc22013005 = 85767007;    int OsCzwLILUupretwVMHRc36846957 = -405906281;    int OsCzwLILUupretwVMHRc88328799 = -457043078;    int OsCzwLILUupretwVMHRc66150163 = -312782857;    int OsCzwLILUupretwVMHRc87239347 = -516363045;    int OsCzwLILUupretwVMHRc66994583 = -352651953;    int OsCzwLILUupretwVMHRc91062892 = 38217211;    int OsCzwLILUupretwVMHRc36856756 = -462468410;    int OsCzwLILUupretwVMHRc80197535 = -541571580;    int OsCzwLILUupretwVMHRc70253749 = -688342948;    int OsCzwLILUupretwVMHRc36942174 = -359550322;    int OsCzwLILUupretwVMHRc19764855 = -235306692;    int OsCzwLILUupretwVMHRc84342319 = -767071022;    int OsCzwLILUupretwVMHRc51314126 = -772440755;    int OsCzwLILUupretwVMHRc51700878 = -800263029;    int OsCzwLILUupretwVMHRc15444012 = -207937734;    int OsCzwLILUupretwVMHRc56911902 = -671869654;    int OsCzwLILUupretwVMHRc38906275 = -103187168;    int OsCzwLILUupretwVMHRc72717950 = -987128693;    int OsCzwLILUupretwVMHRc7103971 = -174319184;    int OsCzwLILUupretwVMHRc68597754 = -911408828;    int OsCzwLILUupretwVMHRc22670596 = -724486543;    int OsCzwLILUupretwVMHRc49077260 = -109787913;    int OsCzwLILUupretwVMHRc61502385 = -817661092;    int OsCzwLILUupretwVMHRc82769017 = -849230164;    int OsCzwLILUupretwVMHRc59315098 = -297262323;    int OsCzwLILUupretwVMHRc39311317 = -808585524;    int OsCzwLILUupretwVMHRc81262956 = -404252914;    int OsCzwLILUupretwVMHRc12773366 = 72666782;    int OsCzwLILUupretwVMHRc41671638 = -741582836;    int OsCzwLILUupretwVMHRc61072833 = -480787262;    int OsCzwLILUupretwVMHRc99514931 = 79041956;    int OsCzwLILUupretwVMHRc98395869 = 62686930;    int OsCzwLILUupretwVMHRc88802084 = -40503392;    int OsCzwLILUupretwVMHRc17040135 = -296133656;    int OsCzwLILUupretwVMHRc12882114 = -386663946;    int OsCzwLILUupretwVMHRc89192591 = -930503259;    int OsCzwLILUupretwVMHRc11445866 = -807548187;    int OsCzwLILUupretwVMHRc6335850 = 84467494;    int OsCzwLILUupretwVMHRc51443027 = -486237091;     OsCzwLILUupretwVMHRc95735691 = OsCzwLILUupretwVMHRc98341556;     OsCzwLILUupretwVMHRc98341556 = OsCzwLILUupretwVMHRc34014942;     OsCzwLILUupretwVMHRc34014942 = OsCzwLILUupretwVMHRc54050116;     OsCzwLILUupretwVMHRc54050116 = OsCzwLILUupretwVMHRc70982477;     OsCzwLILUupretwVMHRc70982477 = OsCzwLILUupretwVMHRc6432020;     OsCzwLILUupretwVMHRc6432020 = OsCzwLILUupretwVMHRc87003101;     OsCzwLILUupretwVMHRc87003101 = OsCzwLILUupretwVMHRc42624690;     OsCzwLILUupretwVMHRc42624690 = OsCzwLILUupretwVMHRc88759111;     OsCzwLILUupretwVMHRc88759111 = OsCzwLILUupretwVMHRc80445520;     OsCzwLILUupretwVMHRc80445520 = OsCzwLILUupretwVMHRc41253737;     OsCzwLILUupretwVMHRc41253737 = OsCzwLILUupretwVMHRc25319779;     OsCzwLILUupretwVMHRc25319779 = OsCzwLILUupretwVMHRc35292549;     OsCzwLILUupretwVMHRc35292549 = OsCzwLILUupretwVMHRc83096933;     OsCzwLILUupretwVMHRc83096933 = OsCzwLILUupretwVMHRc97186665;     OsCzwLILUupretwVMHRc97186665 = OsCzwLILUupretwVMHRc79316487;     OsCzwLILUupretwVMHRc79316487 = OsCzwLILUupretwVMHRc27338258;     OsCzwLILUupretwVMHRc27338258 = OsCzwLILUupretwVMHRc15932379;     OsCzwLILUupretwVMHRc15932379 = OsCzwLILUupretwVMHRc90367265;     OsCzwLILUupretwVMHRc90367265 = OsCzwLILUupretwVMHRc1429948;     OsCzwLILUupretwVMHRc1429948 = OsCzwLILUupretwVMHRc92965402;     OsCzwLILUupretwVMHRc92965402 = OsCzwLILUupretwVMHRc34507931;     OsCzwLILUupretwVMHRc34507931 = OsCzwLILUupretwVMHRc32481472;     OsCzwLILUupretwVMHRc32481472 = OsCzwLILUupretwVMHRc91155560;     OsCzwLILUupretwVMHRc91155560 = OsCzwLILUupretwVMHRc44872583;     OsCzwLILUupretwVMHRc44872583 = OsCzwLILUupretwVMHRc28194503;     OsCzwLILUupretwVMHRc28194503 = OsCzwLILUupretwVMHRc84419015;     OsCzwLILUupretwVMHRc84419015 = OsCzwLILUupretwVMHRc50156145;     OsCzwLILUupretwVMHRc50156145 = OsCzwLILUupretwVMHRc54295891;     OsCzwLILUupretwVMHRc54295891 = OsCzwLILUupretwVMHRc22608949;     OsCzwLILUupretwVMHRc22608949 = OsCzwLILUupretwVMHRc93206172;     OsCzwLILUupretwVMHRc93206172 = OsCzwLILUupretwVMHRc74259153;     OsCzwLILUupretwVMHRc74259153 = OsCzwLILUupretwVMHRc34256886;     OsCzwLILUupretwVMHRc34256886 = OsCzwLILUupretwVMHRc98435793;     OsCzwLILUupretwVMHRc98435793 = OsCzwLILUupretwVMHRc2899399;     OsCzwLILUupretwVMHRc2899399 = OsCzwLILUupretwVMHRc26932917;     OsCzwLILUupretwVMHRc26932917 = OsCzwLILUupretwVMHRc42374313;     OsCzwLILUupretwVMHRc42374313 = OsCzwLILUupretwVMHRc7573404;     OsCzwLILUupretwVMHRc7573404 = OsCzwLILUupretwVMHRc31590059;     OsCzwLILUupretwVMHRc31590059 = OsCzwLILUupretwVMHRc39053140;     OsCzwLILUupretwVMHRc39053140 = OsCzwLILUupretwVMHRc49729070;     OsCzwLILUupretwVMHRc49729070 = OsCzwLILUupretwVMHRc77521391;     OsCzwLILUupretwVMHRc77521391 = OsCzwLILUupretwVMHRc77596028;     OsCzwLILUupretwVMHRc77596028 = OsCzwLILUupretwVMHRc93575197;     OsCzwLILUupretwVMHRc93575197 = OsCzwLILUupretwVMHRc18437611;     OsCzwLILUupretwVMHRc18437611 = OsCzwLILUupretwVMHRc37768612;     OsCzwLILUupretwVMHRc37768612 = OsCzwLILUupretwVMHRc59596748;     OsCzwLILUupretwVMHRc59596748 = OsCzwLILUupretwVMHRc61748420;     OsCzwLILUupretwVMHRc61748420 = OsCzwLILUupretwVMHRc1078886;     OsCzwLILUupretwVMHRc1078886 = OsCzwLILUupretwVMHRc92793506;     OsCzwLILUupretwVMHRc92793506 = OsCzwLILUupretwVMHRc39839931;     OsCzwLILUupretwVMHRc39839931 = OsCzwLILUupretwVMHRc33891075;     OsCzwLILUupretwVMHRc33891075 = OsCzwLILUupretwVMHRc34947837;     OsCzwLILUupretwVMHRc34947837 = OsCzwLILUupretwVMHRc52993929;     OsCzwLILUupretwVMHRc52993929 = OsCzwLILUupretwVMHRc85662427;     OsCzwLILUupretwVMHRc85662427 = OsCzwLILUupretwVMHRc61227761;     OsCzwLILUupretwVMHRc61227761 = OsCzwLILUupretwVMHRc65860084;     OsCzwLILUupretwVMHRc65860084 = OsCzwLILUupretwVMHRc42859382;     OsCzwLILUupretwVMHRc42859382 = OsCzwLILUupretwVMHRc9177534;     OsCzwLILUupretwVMHRc9177534 = OsCzwLILUupretwVMHRc42787975;     OsCzwLILUupretwVMHRc42787975 = OsCzwLILUupretwVMHRc22013005;     OsCzwLILUupretwVMHRc22013005 = OsCzwLILUupretwVMHRc36846957;     OsCzwLILUupretwVMHRc36846957 = OsCzwLILUupretwVMHRc88328799;     OsCzwLILUupretwVMHRc88328799 = OsCzwLILUupretwVMHRc66150163;     OsCzwLILUupretwVMHRc66150163 = OsCzwLILUupretwVMHRc87239347;     OsCzwLILUupretwVMHRc87239347 = OsCzwLILUupretwVMHRc66994583;     OsCzwLILUupretwVMHRc66994583 = OsCzwLILUupretwVMHRc91062892;     OsCzwLILUupretwVMHRc91062892 = OsCzwLILUupretwVMHRc36856756;     OsCzwLILUupretwVMHRc36856756 = OsCzwLILUupretwVMHRc80197535;     OsCzwLILUupretwVMHRc80197535 = OsCzwLILUupretwVMHRc70253749;     OsCzwLILUupretwVMHRc70253749 = OsCzwLILUupretwVMHRc36942174;     OsCzwLILUupretwVMHRc36942174 = OsCzwLILUupretwVMHRc19764855;     OsCzwLILUupretwVMHRc19764855 = OsCzwLILUupretwVMHRc84342319;     OsCzwLILUupretwVMHRc84342319 = OsCzwLILUupretwVMHRc51314126;     OsCzwLILUupretwVMHRc51314126 = OsCzwLILUupretwVMHRc51700878;     OsCzwLILUupretwVMHRc51700878 = OsCzwLILUupretwVMHRc15444012;     OsCzwLILUupretwVMHRc15444012 = OsCzwLILUupretwVMHRc56911902;     OsCzwLILUupretwVMHRc56911902 = OsCzwLILUupretwVMHRc38906275;     OsCzwLILUupretwVMHRc38906275 = OsCzwLILUupretwVMHRc72717950;     OsCzwLILUupretwVMHRc72717950 = OsCzwLILUupretwVMHRc7103971;     OsCzwLILUupretwVMHRc7103971 = OsCzwLILUupretwVMHRc68597754;     OsCzwLILUupretwVMHRc68597754 = OsCzwLILUupretwVMHRc22670596;     OsCzwLILUupretwVMHRc22670596 = OsCzwLILUupretwVMHRc49077260;     OsCzwLILUupretwVMHRc49077260 = OsCzwLILUupretwVMHRc61502385;     OsCzwLILUupretwVMHRc61502385 = OsCzwLILUupretwVMHRc82769017;     OsCzwLILUupretwVMHRc82769017 = OsCzwLILUupretwVMHRc59315098;     OsCzwLILUupretwVMHRc59315098 = OsCzwLILUupretwVMHRc39311317;     OsCzwLILUupretwVMHRc39311317 = OsCzwLILUupretwVMHRc81262956;     OsCzwLILUupretwVMHRc81262956 = OsCzwLILUupretwVMHRc12773366;     OsCzwLILUupretwVMHRc12773366 = OsCzwLILUupretwVMHRc41671638;     OsCzwLILUupretwVMHRc41671638 = OsCzwLILUupretwVMHRc61072833;     OsCzwLILUupretwVMHRc61072833 = OsCzwLILUupretwVMHRc99514931;     OsCzwLILUupretwVMHRc99514931 = OsCzwLILUupretwVMHRc98395869;     OsCzwLILUupretwVMHRc98395869 = OsCzwLILUupretwVMHRc88802084;     OsCzwLILUupretwVMHRc88802084 = OsCzwLILUupretwVMHRc17040135;     OsCzwLILUupretwVMHRc17040135 = OsCzwLILUupretwVMHRc12882114;     OsCzwLILUupretwVMHRc12882114 = OsCzwLILUupretwVMHRc89192591;     OsCzwLILUupretwVMHRc89192591 = OsCzwLILUupretwVMHRc11445866;     OsCzwLILUupretwVMHRc11445866 = OsCzwLILUupretwVMHRc6335850;     OsCzwLILUupretwVMHRc6335850 = OsCzwLILUupretwVMHRc51443027;     OsCzwLILUupretwVMHRc51443027 = OsCzwLILUupretwVMHRc95735691;}
// Junk Finished

// Junk Code By Peatreat & Thaisen's Gen
void QWGthYCegjAQTioJAWKP2727616() {     int OdqpKAdUGAhqjWQkwyeS98654638 = -807509842;    int OdqpKAdUGAhqjWQkwyeS29665193 = -152525139;    int OdqpKAdUGAhqjWQkwyeS45947229 = -690965497;    int OdqpKAdUGAhqjWQkwyeS50409656 = -850756766;    int OdqpKAdUGAhqjWQkwyeS68710215 = -350419937;    int OdqpKAdUGAhqjWQkwyeS95723062 = -749297769;    int OdqpKAdUGAhqjWQkwyeS34326962 = -933644771;    int OdqpKAdUGAhqjWQkwyeS53008316 = -456817858;    int OdqpKAdUGAhqjWQkwyeS25235445 = -141781099;    int OdqpKAdUGAhqjWQkwyeS94280381 = 35247289;    int OdqpKAdUGAhqjWQkwyeS24572333 = -516159652;    int OdqpKAdUGAhqjWQkwyeS77728114 = -954950692;    int OdqpKAdUGAhqjWQkwyeS11929070 = -422804300;    int OdqpKAdUGAhqjWQkwyeS17295646 = -962613841;    int OdqpKAdUGAhqjWQkwyeS73395481 = -613301494;    int OdqpKAdUGAhqjWQkwyeS57356750 = -925709112;    int OdqpKAdUGAhqjWQkwyeS71199496 = -140038476;    int OdqpKAdUGAhqjWQkwyeS97846043 = -305897411;    int OdqpKAdUGAhqjWQkwyeS32014017 = -940704768;    int OdqpKAdUGAhqjWQkwyeS99334929 = -227931253;    int OdqpKAdUGAhqjWQkwyeS81392437 = -80759053;    int OdqpKAdUGAhqjWQkwyeS83341066 = -885163798;    int OdqpKAdUGAhqjWQkwyeS67601789 = -613588691;    int OdqpKAdUGAhqjWQkwyeS99933679 = -403663995;    int OdqpKAdUGAhqjWQkwyeS69209340 = -226268936;    int OdqpKAdUGAhqjWQkwyeS11730036 = -246399333;    int OdqpKAdUGAhqjWQkwyeS47580305 = -857376728;    int OdqpKAdUGAhqjWQkwyeS5275921 = -673778589;    int OdqpKAdUGAhqjWQkwyeS19443495 = -953332076;    int OdqpKAdUGAhqjWQkwyeS47059274 = -401850755;    int OdqpKAdUGAhqjWQkwyeS7692893 = -927224440;    int OdqpKAdUGAhqjWQkwyeS92221033 = -587595935;    int OdqpKAdUGAhqjWQkwyeS3819453 = -615861044;    int OdqpKAdUGAhqjWQkwyeS99067973 = -785499492;    int OdqpKAdUGAhqjWQkwyeS39198427 = -336672846;    int OdqpKAdUGAhqjWQkwyeS32559849 = -507391393;    int OdqpKAdUGAhqjWQkwyeS67430574 = -853755059;    int OdqpKAdUGAhqjWQkwyeS19717030 = -589643302;    int OdqpKAdUGAhqjWQkwyeS12584866 = -874469624;    int OdqpKAdUGAhqjWQkwyeS37957118 = -450059582;    int OdqpKAdUGAhqjWQkwyeS37004384 = -885961597;    int OdqpKAdUGAhqjWQkwyeS54170649 = 93441198;    int OdqpKAdUGAhqjWQkwyeS12012980 = -60059830;    int OdqpKAdUGAhqjWQkwyeS9633778 = -80186538;    int OdqpKAdUGAhqjWQkwyeS6392466 = -121106952;    int OdqpKAdUGAhqjWQkwyeS91407766 = -269588412;    int OdqpKAdUGAhqjWQkwyeS89904265 = -241608430;    int OdqpKAdUGAhqjWQkwyeS6191265 = -613702301;    int OdqpKAdUGAhqjWQkwyeS54403200 = -775046961;    int OdqpKAdUGAhqjWQkwyeS48978909 = -86332925;    int OdqpKAdUGAhqjWQkwyeS74273578 = -672474765;    int OdqpKAdUGAhqjWQkwyeS91293471 = -367438480;    int OdqpKAdUGAhqjWQkwyeS43437094 = -687090344;    int OdqpKAdUGAhqjWQkwyeS40350446 = -418559575;    int OdqpKAdUGAhqjWQkwyeS21335178 = -4788111;    int OdqpKAdUGAhqjWQkwyeS15313573 = -922346045;    int OdqpKAdUGAhqjWQkwyeS62063403 = -538936448;    int OdqpKAdUGAhqjWQkwyeS46013550 = -187301502;    int OdqpKAdUGAhqjWQkwyeS81200315 = -524487830;    int OdqpKAdUGAhqjWQkwyeS56980180 = -4020605;    int OdqpKAdUGAhqjWQkwyeS48142758 = -891921042;    int OdqpKAdUGAhqjWQkwyeS29051042 = -159866183;    int OdqpKAdUGAhqjWQkwyeS33564821 = -503485783;    int OdqpKAdUGAhqjWQkwyeS78176171 = -739930345;    int OdqpKAdUGAhqjWQkwyeS86587489 = -37528272;    int OdqpKAdUGAhqjWQkwyeS32351299 = -928563717;    int OdqpKAdUGAhqjWQkwyeS73908661 = -239089649;    int OdqpKAdUGAhqjWQkwyeS12861097 = -637304809;    int OdqpKAdUGAhqjWQkwyeS78097218 = -525940995;    int OdqpKAdUGAhqjWQkwyeS40835633 = -5910102;    int OdqpKAdUGAhqjWQkwyeS89926176 = 28045946;    int OdqpKAdUGAhqjWQkwyeS51482466 = -550395174;    int OdqpKAdUGAhqjWQkwyeS85261178 = -431427788;    int OdqpKAdUGAhqjWQkwyeS94056899 = -390645187;    int OdqpKAdUGAhqjWQkwyeS62330546 = -341969656;    int OdqpKAdUGAhqjWQkwyeS27221788 = -74200252;    int OdqpKAdUGAhqjWQkwyeS71328087 = -725103968;    int OdqpKAdUGAhqjWQkwyeS57968012 = -433402153;    int OdqpKAdUGAhqjWQkwyeS93541213 = -182557044;    int OdqpKAdUGAhqjWQkwyeS77801574 = -956680525;    int OdqpKAdUGAhqjWQkwyeS21825770 = 95209097;    int OdqpKAdUGAhqjWQkwyeS41389041 = -143674427;    int OdqpKAdUGAhqjWQkwyeS50872721 = -898731628;    int OdqpKAdUGAhqjWQkwyeS70464585 = -766999151;    int OdqpKAdUGAhqjWQkwyeS72785695 = -729375990;    int OdqpKAdUGAhqjWQkwyeS16399421 = -459785960;    int OdqpKAdUGAhqjWQkwyeS48783940 = -900505592;    int OdqpKAdUGAhqjWQkwyeS63469006 = -97301470;    int OdqpKAdUGAhqjWQkwyeS77732795 = -680711382;    int OdqpKAdUGAhqjWQkwyeS23884855 = -414326802;    int OdqpKAdUGAhqjWQkwyeS70496445 = -968454945;    int OdqpKAdUGAhqjWQkwyeS21417025 = -566453557;    int OdqpKAdUGAhqjWQkwyeS38516715 = 34844527;    int OdqpKAdUGAhqjWQkwyeS55604686 = -770449019;    int OdqpKAdUGAhqjWQkwyeS89814360 = -558138540;    int OdqpKAdUGAhqjWQkwyeS7953342 = -626095415;    int OdqpKAdUGAhqjWQkwyeS20605828 = -403073019;    int OdqpKAdUGAhqjWQkwyeS33836808 = -320129486;    int OdqpKAdUGAhqjWQkwyeS23046289 = 57341734;    int OdqpKAdUGAhqjWQkwyeS74041167 = -807509842;     OdqpKAdUGAhqjWQkwyeS98654638 = OdqpKAdUGAhqjWQkwyeS29665193;     OdqpKAdUGAhqjWQkwyeS29665193 = OdqpKAdUGAhqjWQkwyeS45947229;     OdqpKAdUGAhqjWQkwyeS45947229 = OdqpKAdUGAhqjWQkwyeS50409656;     OdqpKAdUGAhqjWQkwyeS50409656 = OdqpKAdUGAhqjWQkwyeS68710215;     OdqpKAdUGAhqjWQkwyeS68710215 = OdqpKAdUGAhqjWQkwyeS95723062;     OdqpKAdUGAhqjWQkwyeS95723062 = OdqpKAdUGAhqjWQkwyeS34326962;     OdqpKAdUGAhqjWQkwyeS34326962 = OdqpKAdUGAhqjWQkwyeS53008316;     OdqpKAdUGAhqjWQkwyeS53008316 = OdqpKAdUGAhqjWQkwyeS25235445;     OdqpKAdUGAhqjWQkwyeS25235445 = OdqpKAdUGAhqjWQkwyeS94280381;     OdqpKAdUGAhqjWQkwyeS94280381 = OdqpKAdUGAhqjWQkwyeS24572333;     OdqpKAdUGAhqjWQkwyeS24572333 = OdqpKAdUGAhqjWQkwyeS77728114;     OdqpKAdUGAhqjWQkwyeS77728114 = OdqpKAdUGAhqjWQkwyeS11929070;     OdqpKAdUGAhqjWQkwyeS11929070 = OdqpKAdUGAhqjWQkwyeS17295646;     OdqpKAdUGAhqjWQkwyeS17295646 = OdqpKAdUGAhqjWQkwyeS73395481;     OdqpKAdUGAhqjWQkwyeS73395481 = OdqpKAdUGAhqjWQkwyeS57356750;     OdqpKAdUGAhqjWQkwyeS57356750 = OdqpKAdUGAhqjWQkwyeS71199496;     OdqpKAdUGAhqjWQkwyeS71199496 = OdqpKAdUGAhqjWQkwyeS97846043;     OdqpKAdUGAhqjWQkwyeS97846043 = OdqpKAdUGAhqjWQkwyeS32014017;     OdqpKAdUGAhqjWQkwyeS32014017 = OdqpKAdUGAhqjWQkwyeS99334929;     OdqpKAdUGAhqjWQkwyeS99334929 = OdqpKAdUGAhqjWQkwyeS81392437;     OdqpKAdUGAhqjWQkwyeS81392437 = OdqpKAdUGAhqjWQkwyeS83341066;     OdqpKAdUGAhqjWQkwyeS83341066 = OdqpKAdUGAhqjWQkwyeS67601789;     OdqpKAdUGAhqjWQkwyeS67601789 = OdqpKAdUGAhqjWQkwyeS99933679;     OdqpKAdUGAhqjWQkwyeS99933679 = OdqpKAdUGAhqjWQkwyeS69209340;     OdqpKAdUGAhqjWQkwyeS69209340 = OdqpKAdUGAhqjWQkwyeS11730036;     OdqpKAdUGAhqjWQkwyeS11730036 = OdqpKAdUGAhqjWQkwyeS47580305;     OdqpKAdUGAhqjWQkwyeS47580305 = OdqpKAdUGAhqjWQkwyeS5275921;     OdqpKAdUGAhqjWQkwyeS5275921 = OdqpKAdUGAhqjWQkwyeS19443495;     OdqpKAdUGAhqjWQkwyeS19443495 = OdqpKAdUGAhqjWQkwyeS47059274;     OdqpKAdUGAhqjWQkwyeS47059274 = OdqpKAdUGAhqjWQkwyeS7692893;     OdqpKAdUGAhqjWQkwyeS7692893 = OdqpKAdUGAhqjWQkwyeS92221033;     OdqpKAdUGAhqjWQkwyeS92221033 = OdqpKAdUGAhqjWQkwyeS3819453;     OdqpKAdUGAhqjWQkwyeS3819453 = OdqpKAdUGAhqjWQkwyeS99067973;     OdqpKAdUGAhqjWQkwyeS99067973 = OdqpKAdUGAhqjWQkwyeS39198427;     OdqpKAdUGAhqjWQkwyeS39198427 = OdqpKAdUGAhqjWQkwyeS32559849;     OdqpKAdUGAhqjWQkwyeS32559849 = OdqpKAdUGAhqjWQkwyeS67430574;     OdqpKAdUGAhqjWQkwyeS67430574 = OdqpKAdUGAhqjWQkwyeS19717030;     OdqpKAdUGAhqjWQkwyeS19717030 = OdqpKAdUGAhqjWQkwyeS12584866;     OdqpKAdUGAhqjWQkwyeS12584866 = OdqpKAdUGAhqjWQkwyeS37957118;     OdqpKAdUGAhqjWQkwyeS37957118 = OdqpKAdUGAhqjWQkwyeS37004384;     OdqpKAdUGAhqjWQkwyeS37004384 = OdqpKAdUGAhqjWQkwyeS54170649;     OdqpKAdUGAhqjWQkwyeS54170649 = OdqpKAdUGAhqjWQkwyeS12012980;     OdqpKAdUGAhqjWQkwyeS12012980 = OdqpKAdUGAhqjWQkwyeS9633778;     OdqpKAdUGAhqjWQkwyeS9633778 = OdqpKAdUGAhqjWQkwyeS6392466;     OdqpKAdUGAhqjWQkwyeS6392466 = OdqpKAdUGAhqjWQkwyeS91407766;     OdqpKAdUGAhqjWQkwyeS91407766 = OdqpKAdUGAhqjWQkwyeS89904265;     OdqpKAdUGAhqjWQkwyeS89904265 = OdqpKAdUGAhqjWQkwyeS6191265;     OdqpKAdUGAhqjWQkwyeS6191265 = OdqpKAdUGAhqjWQkwyeS54403200;     OdqpKAdUGAhqjWQkwyeS54403200 = OdqpKAdUGAhqjWQkwyeS48978909;     OdqpKAdUGAhqjWQkwyeS48978909 = OdqpKAdUGAhqjWQkwyeS74273578;     OdqpKAdUGAhqjWQkwyeS74273578 = OdqpKAdUGAhqjWQkwyeS91293471;     OdqpKAdUGAhqjWQkwyeS91293471 = OdqpKAdUGAhqjWQkwyeS43437094;     OdqpKAdUGAhqjWQkwyeS43437094 = OdqpKAdUGAhqjWQkwyeS40350446;     OdqpKAdUGAhqjWQkwyeS40350446 = OdqpKAdUGAhqjWQkwyeS21335178;     OdqpKAdUGAhqjWQkwyeS21335178 = OdqpKAdUGAhqjWQkwyeS15313573;     OdqpKAdUGAhqjWQkwyeS15313573 = OdqpKAdUGAhqjWQkwyeS62063403;     OdqpKAdUGAhqjWQkwyeS62063403 = OdqpKAdUGAhqjWQkwyeS46013550;     OdqpKAdUGAhqjWQkwyeS46013550 = OdqpKAdUGAhqjWQkwyeS81200315;     OdqpKAdUGAhqjWQkwyeS81200315 = OdqpKAdUGAhqjWQkwyeS56980180;     OdqpKAdUGAhqjWQkwyeS56980180 = OdqpKAdUGAhqjWQkwyeS48142758;     OdqpKAdUGAhqjWQkwyeS48142758 = OdqpKAdUGAhqjWQkwyeS29051042;     OdqpKAdUGAhqjWQkwyeS29051042 = OdqpKAdUGAhqjWQkwyeS33564821;     OdqpKAdUGAhqjWQkwyeS33564821 = OdqpKAdUGAhqjWQkwyeS78176171;     OdqpKAdUGAhqjWQkwyeS78176171 = OdqpKAdUGAhqjWQkwyeS86587489;     OdqpKAdUGAhqjWQkwyeS86587489 = OdqpKAdUGAhqjWQkwyeS32351299;     OdqpKAdUGAhqjWQkwyeS32351299 = OdqpKAdUGAhqjWQkwyeS73908661;     OdqpKAdUGAhqjWQkwyeS73908661 = OdqpKAdUGAhqjWQkwyeS12861097;     OdqpKAdUGAhqjWQkwyeS12861097 = OdqpKAdUGAhqjWQkwyeS78097218;     OdqpKAdUGAhqjWQkwyeS78097218 = OdqpKAdUGAhqjWQkwyeS40835633;     OdqpKAdUGAhqjWQkwyeS40835633 = OdqpKAdUGAhqjWQkwyeS89926176;     OdqpKAdUGAhqjWQkwyeS89926176 = OdqpKAdUGAhqjWQkwyeS51482466;     OdqpKAdUGAhqjWQkwyeS51482466 = OdqpKAdUGAhqjWQkwyeS85261178;     OdqpKAdUGAhqjWQkwyeS85261178 = OdqpKAdUGAhqjWQkwyeS94056899;     OdqpKAdUGAhqjWQkwyeS94056899 = OdqpKAdUGAhqjWQkwyeS62330546;     OdqpKAdUGAhqjWQkwyeS62330546 = OdqpKAdUGAhqjWQkwyeS27221788;     OdqpKAdUGAhqjWQkwyeS27221788 = OdqpKAdUGAhqjWQkwyeS71328087;     OdqpKAdUGAhqjWQkwyeS71328087 = OdqpKAdUGAhqjWQkwyeS57968012;     OdqpKAdUGAhqjWQkwyeS57968012 = OdqpKAdUGAhqjWQkwyeS93541213;     OdqpKAdUGAhqjWQkwyeS93541213 = OdqpKAdUGAhqjWQkwyeS77801574;     OdqpKAdUGAhqjWQkwyeS77801574 = OdqpKAdUGAhqjWQkwyeS21825770;     OdqpKAdUGAhqjWQkwyeS21825770 = OdqpKAdUGAhqjWQkwyeS41389041;     OdqpKAdUGAhqjWQkwyeS41389041 = OdqpKAdUGAhqjWQkwyeS50872721;     OdqpKAdUGAhqjWQkwyeS50872721 = OdqpKAdUGAhqjWQkwyeS70464585;     OdqpKAdUGAhqjWQkwyeS70464585 = OdqpKAdUGAhqjWQkwyeS72785695;     OdqpKAdUGAhqjWQkwyeS72785695 = OdqpKAdUGAhqjWQkwyeS16399421;     OdqpKAdUGAhqjWQkwyeS16399421 = OdqpKAdUGAhqjWQkwyeS48783940;     OdqpKAdUGAhqjWQkwyeS48783940 = OdqpKAdUGAhqjWQkwyeS63469006;     OdqpKAdUGAhqjWQkwyeS63469006 = OdqpKAdUGAhqjWQkwyeS77732795;     OdqpKAdUGAhqjWQkwyeS77732795 = OdqpKAdUGAhqjWQkwyeS23884855;     OdqpKAdUGAhqjWQkwyeS23884855 = OdqpKAdUGAhqjWQkwyeS70496445;     OdqpKAdUGAhqjWQkwyeS70496445 = OdqpKAdUGAhqjWQkwyeS21417025;     OdqpKAdUGAhqjWQkwyeS21417025 = OdqpKAdUGAhqjWQkwyeS38516715;     OdqpKAdUGAhqjWQkwyeS38516715 = OdqpKAdUGAhqjWQkwyeS55604686;     OdqpKAdUGAhqjWQkwyeS55604686 = OdqpKAdUGAhqjWQkwyeS89814360;     OdqpKAdUGAhqjWQkwyeS89814360 = OdqpKAdUGAhqjWQkwyeS7953342;     OdqpKAdUGAhqjWQkwyeS7953342 = OdqpKAdUGAhqjWQkwyeS20605828;     OdqpKAdUGAhqjWQkwyeS20605828 = OdqpKAdUGAhqjWQkwyeS33836808;     OdqpKAdUGAhqjWQkwyeS33836808 = OdqpKAdUGAhqjWQkwyeS23046289;     OdqpKAdUGAhqjWQkwyeS23046289 = OdqpKAdUGAhqjWQkwyeS74041167;     OdqpKAdUGAhqjWQkwyeS74041167 = OdqpKAdUGAhqjWQkwyeS98654638;}
// Junk Finished

// Junk Code By Peatreat & Thaisen's Gen
void jLgPzhMUomymBPmvPpEv79069932() {     int ilkzHmZBnZXzshshXNvF30915448 = -383157604;    int ilkzHmZBnZXzshshXNvF30887629 = -826142023;    int ilkzHmZBnZXzshshXNvF79223808 = -793824880;    int ilkzHmZBnZXzshshXNvF91917769 = -670992475;    int ilkzHmZBnZXzshshXNvF53827734 = 99846503;    int ilkzHmZBnZXzshshXNvF29819775 = -717273039;    int ilkzHmZBnZXzshshXNvF31819263 = -18086605;    int ilkzHmZBnZXzshshXNvF75763611 = -190966764;    int ilkzHmZBnZXzshshXNvF98586888 = -18084501;    int ilkzHmZBnZXzshshXNvF81788296 = -137619519;    int ilkzHmZBnZXzshshXNvF85962373 = -4363895;    int ilkzHmZBnZXzshshXNvF98881555 = -626107850;    int ilkzHmZBnZXzshshXNvF40591118 = -729285760;    int ilkzHmZBnZXzshshXNvF21491545 = -392161713;    int ilkzHmZBnZXzshshXNvF12056387 = -855658264;    int ilkzHmZBnZXzshshXNvF77432921 = -506996784;    int ilkzHmZBnZXzshshXNvF77060668 = -155973055;    int ilkzHmZBnZXzshshXNvF82466747 = -314790809;    int ilkzHmZBnZXzshshXNvF12381257 = -609396457;    int ilkzHmZBnZXzshshXNvF10526965 = -932687397;    int ilkzHmZBnZXzshshXNvF94353142 = -367821269;    int ilkzHmZBnZXzshshXNvF49074702 = 43166525;    int ilkzHmZBnZXzshshXNvF37656694 = -383209362;    int ilkzHmZBnZXzshshXNvF18512369 = -519632477;    int ilkzHmZBnZXzshshXNvF57864399 = -304317117;    int ilkzHmZBnZXzshshXNvF81242065 = -814419822;    int ilkzHmZBnZXzshshXNvF72702390 = -855434227;    int ilkzHmZBnZXzshshXNvF88016995 = -402342984;    int ilkzHmZBnZXzshshXNvF65127058 = -535390147;    int ilkzHmZBnZXzshshXNvF8427374 = -447744230;    int ilkzHmZBnZXzshshXNvF48520295 = -681874048;    int ilkzHmZBnZXzshshXNvF50395311 = -738842870;    int ilkzHmZBnZXzshshXNvF75889082 = -844979379;    int ilkzHmZBnZXzshshXNvF34618017 = -18175184;    int ilkzHmZBnZXzshshXNvF79831257 = -750093608;    int ilkzHmZBnZXzshshXNvF21021711 = -519757359;    int ilkzHmZBnZXzshshXNvF60636718 = 29112894;    int ilkzHmZBnZXzshshXNvF48985848 = -591209530;    int ilkzHmZBnZXzshshXNvF50359980 = -468053610;    int ilkzHmZBnZXzshshXNvF13630691 = -681575838;    int ilkzHmZBnZXzshshXNvF52062081 = -583277988;    int ilkzHmZBnZXzshshXNvF88034577 = -949974280;    int ilkzHmZBnZXzshshXNvF57746330 = -572164819;    int ilkzHmZBnZXzshshXNvF78415956 = -213466999;    int ilkzHmZBnZXzshshXNvF82411315 = -917740474;    int ilkzHmZBnZXzshshXNvF79069055 = 31806398;    int ilkzHmZBnZXzshshXNvF21778373 = -171536183;    int ilkzHmZBnZXzshshXNvF60403480 = -343581039;    int ilkzHmZBnZXzshshXNvF20944451 = -288824313;    int ilkzHmZBnZXzshshXNvF16782250 = -871356847;    int ilkzHmZBnZXzshshXNvF53777271 = -653168997;    int ilkzHmZBnZXzshshXNvF21371622 = -833830930;    int ilkzHmZBnZXzshshXNvF72299042 = -647148580;    int ilkzHmZBnZXzshshXNvF47729929 = -570685102;    int ilkzHmZBnZXzshshXNvF84716790 = -16891954;    int ilkzHmZBnZXzshshXNvF81840745 = -326324129;    int ilkzHmZBnZXzshshXNvF93230934 = -342932661;    int ilkzHmZBnZXzshshXNvF60711439 = -174192403;    int ilkzHmZBnZXzshshXNvF34053371 = -266675359;    int ilkzHmZBnZXzshshXNvF72585668 = -85733675;    int ilkzHmZBnZXzshshXNvF57117385 = -861838813;    int ilkzHmZBnZXzshshXNvF43802267 = -615743621;    int ilkzHmZBnZXzshshXNvF10636554 = -655576617;    int ilkzHmZBnZXzshshXNvF90159515 = -570340271;    int ilkzHmZBnZXzshshXNvF33268001 = -455745472;    int ilkzHmZBnZXzshshXNvF35567063 = -265521025;    int ilkzHmZBnZXzshshXNvF22992474 = -781128472;    int ilkzHmZBnZXzshshXNvF5973102 = -611110576;    int ilkzHmZBnZXzshshXNvF41660287 = -642068106;    int ilkzHmZBnZXzshshXNvF91034675 = -235900906;    int ilkzHmZBnZXzshshXNvF16796204 = -436109679;    int ilkzHmZBnZXzshshXNvF28074820 = -564763525;    int ilkzHmZBnZXzshshXNvF32106767 = -846737200;    int ilkzHmZBnZXzshshXNvF98750565 = -927820620;    int ilkzHmZBnZXzshshXNvF58464884 = -249409410;    int ilkzHmZBnZXzshshXNvF6318565 = -417846990;    int ilkzHmZBnZXzshshXNvF91328372 = -384668657;    int ilkzHmZBnZXzshshXNvF59240738 = -69742364;    int ilkzHmZBnZXzshshXNvF36101054 = -601892004;    int ilkzHmZBnZXzshshXNvF78795344 = -236123515;    int ilkzHmZBnZXzshshXNvF59463693 = -542883640;    int ilkzHmZBnZXzshshXNvF12298911 = -411853189;    int ilkzHmZBnZXzshshXNvF67072545 = -13518672;    int ilkzHmZBnZXzshshXNvF48344809 = -664033301;    int ilkzHmZBnZXzshshXNvF54650102 = -794575234;    int ilkzHmZBnZXzshshXNvF27148674 = -848043119;    int ilkzHmZBnZXzshshXNvF78096269 = 8305709;    int ilkzHmZBnZXzshshXNvF28159153 = -174294278;    int ilkzHmZBnZXzshshXNvF49901226 = 98716770;    int ilkzHmZBnZXzshshXNvF97990512 = -323769479;    int ilkzHmZBnZXzshshXNvF27790777 = -76824698;    int ilkzHmZBnZXzshshXNvF99925278 = -796694703;    int ilkzHmZBnZXzshshXNvF14932478 = -224534172;    int ilkzHmZBnZXzshshXNvF77774311 = -282319935;    int ilkzHmZBnZXzshshXNvF56513306 = -819737025;    int ilkzHmZBnZXzshshXNvF8259814 = -967534368;    int ilkzHmZBnZXzshshXNvF77398024 = -194397663;    int ilkzHmZBnZXzshshXNvF67586815 = 98175452;    int ilkzHmZBnZXzshshXNvF45147955 = -757721527;    int ilkzHmZBnZXzshshXNvF46844253 = -383157604;     ilkzHmZBnZXzshshXNvF30915448 = ilkzHmZBnZXzshshXNvF30887629;     ilkzHmZBnZXzshshXNvF30887629 = ilkzHmZBnZXzshshXNvF79223808;     ilkzHmZBnZXzshshXNvF79223808 = ilkzHmZBnZXzshshXNvF91917769;     ilkzHmZBnZXzshshXNvF91917769 = ilkzHmZBnZXzshshXNvF53827734;     ilkzHmZBnZXzshshXNvF53827734 = ilkzHmZBnZXzshshXNvF29819775;     ilkzHmZBnZXzshshXNvF29819775 = ilkzHmZBnZXzshshXNvF31819263;     ilkzHmZBnZXzshshXNvF31819263 = ilkzHmZBnZXzshshXNvF75763611;     ilkzHmZBnZXzshshXNvF75763611 = ilkzHmZBnZXzshshXNvF98586888;     ilkzHmZBnZXzshshXNvF98586888 = ilkzHmZBnZXzshshXNvF81788296;     ilkzHmZBnZXzshshXNvF81788296 = ilkzHmZBnZXzshshXNvF85962373;     ilkzHmZBnZXzshshXNvF85962373 = ilkzHmZBnZXzshshXNvF98881555;     ilkzHmZBnZXzshshXNvF98881555 = ilkzHmZBnZXzshshXNvF40591118;     ilkzHmZBnZXzshshXNvF40591118 = ilkzHmZBnZXzshshXNvF21491545;     ilkzHmZBnZXzshshXNvF21491545 = ilkzHmZBnZXzshshXNvF12056387;     ilkzHmZBnZXzshshXNvF12056387 = ilkzHmZBnZXzshshXNvF77432921;     ilkzHmZBnZXzshshXNvF77432921 = ilkzHmZBnZXzshshXNvF77060668;     ilkzHmZBnZXzshshXNvF77060668 = ilkzHmZBnZXzshshXNvF82466747;     ilkzHmZBnZXzshshXNvF82466747 = ilkzHmZBnZXzshshXNvF12381257;     ilkzHmZBnZXzshshXNvF12381257 = ilkzHmZBnZXzshshXNvF10526965;     ilkzHmZBnZXzshshXNvF10526965 = ilkzHmZBnZXzshshXNvF94353142;     ilkzHmZBnZXzshshXNvF94353142 = ilkzHmZBnZXzshshXNvF49074702;     ilkzHmZBnZXzshshXNvF49074702 = ilkzHmZBnZXzshshXNvF37656694;     ilkzHmZBnZXzshshXNvF37656694 = ilkzHmZBnZXzshshXNvF18512369;     ilkzHmZBnZXzshshXNvF18512369 = ilkzHmZBnZXzshshXNvF57864399;     ilkzHmZBnZXzshshXNvF57864399 = ilkzHmZBnZXzshshXNvF81242065;     ilkzHmZBnZXzshshXNvF81242065 = ilkzHmZBnZXzshshXNvF72702390;     ilkzHmZBnZXzshshXNvF72702390 = ilkzHmZBnZXzshshXNvF88016995;     ilkzHmZBnZXzshshXNvF88016995 = ilkzHmZBnZXzshshXNvF65127058;     ilkzHmZBnZXzshshXNvF65127058 = ilkzHmZBnZXzshshXNvF8427374;     ilkzHmZBnZXzshshXNvF8427374 = ilkzHmZBnZXzshshXNvF48520295;     ilkzHmZBnZXzshshXNvF48520295 = ilkzHmZBnZXzshshXNvF50395311;     ilkzHmZBnZXzshshXNvF50395311 = ilkzHmZBnZXzshshXNvF75889082;     ilkzHmZBnZXzshshXNvF75889082 = ilkzHmZBnZXzshshXNvF34618017;     ilkzHmZBnZXzshshXNvF34618017 = ilkzHmZBnZXzshshXNvF79831257;     ilkzHmZBnZXzshshXNvF79831257 = ilkzHmZBnZXzshshXNvF21021711;     ilkzHmZBnZXzshshXNvF21021711 = ilkzHmZBnZXzshshXNvF60636718;     ilkzHmZBnZXzshshXNvF60636718 = ilkzHmZBnZXzshshXNvF48985848;     ilkzHmZBnZXzshshXNvF48985848 = ilkzHmZBnZXzshshXNvF50359980;     ilkzHmZBnZXzshshXNvF50359980 = ilkzHmZBnZXzshshXNvF13630691;     ilkzHmZBnZXzshshXNvF13630691 = ilkzHmZBnZXzshshXNvF52062081;     ilkzHmZBnZXzshshXNvF52062081 = ilkzHmZBnZXzshshXNvF88034577;     ilkzHmZBnZXzshshXNvF88034577 = ilkzHmZBnZXzshshXNvF57746330;     ilkzHmZBnZXzshshXNvF57746330 = ilkzHmZBnZXzshshXNvF78415956;     ilkzHmZBnZXzshshXNvF78415956 = ilkzHmZBnZXzshshXNvF82411315;     ilkzHmZBnZXzshshXNvF82411315 = ilkzHmZBnZXzshshXNvF79069055;     ilkzHmZBnZXzshshXNvF79069055 = ilkzHmZBnZXzshshXNvF21778373;     ilkzHmZBnZXzshshXNvF21778373 = ilkzHmZBnZXzshshXNvF60403480;     ilkzHmZBnZXzshshXNvF60403480 = ilkzHmZBnZXzshshXNvF20944451;     ilkzHmZBnZXzshshXNvF20944451 = ilkzHmZBnZXzshshXNvF16782250;     ilkzHmZBnZXzshshXNvF16782250 = ilkzHmZBnZXzshshXNvF53777271;     ilkzHmZBnZXzshshXNvF53777271 = ilkzHmZBnZXzshshXNvF21371622;     ilkzHmZBnZXzshshXNvF21371622 = ilkzHmZBnZXzshshXNvF72299042;     ilkzHmZBnZXzshshXNvF72299042 = ilkzHmZBnZXzshshXNvF47729929;     ilkzHmZBnZXzshshXNvF47729929 = ilkzHmZBnZXzshshXNvF84716790;     ilkzHmZBnZXzshshXNvF84716790 = ilkzHmZBnZXzshshXNvF81840745;     ilkzHmZBnZXzshshXNvF81840745 = ilkzHmZBnZXzshshXNvF93230934;     ilkzHmZBnZXzshshXNvF93230934 = ilkzHmZBnZXzshshXNvF60711439;     ilkzHmZBnZXzshshXNvF60711439 = ilkzHmZBnZXzshshXNvF34053371;     ilkzHmZBnZXzshshXNvF34053371 = ilkzHmZBnZXzshshXNvF72585668;     ilkzHmZBnZXzshshXNvF72585668 = ilkzHmZBnZXzshshXNvF57117385;     ilkzHmZBnZXzshshXNvF57117385 = ilkzHmZBnZXzshshXNvF43802267;     ilkzHmZBnZXzshshXNvF43802267 = ilkzHmZBnZXzshshXNvF10636554;     ilkzHmZBnZXzshshXNvF10636554 = ilkzHmZBnZXzshshXNvF90159515;     ilkzHmZBnZXzshshXNvF90159515 = ilkzHmZBnZXzshshXNvF33268001;     ilkzHmZBnZXzshshXNvF33268001 = ilkzHmZBnZXzshshXNvF35567063;     ilkzHmZBnZXzshshXNvF35567063 = ilkzHmZBnZXzshshXNvF22992474;     ilkzHmZBnZXzshshXNvF22992474 = ilkzHmZBnZXzshshXNvF5973102;     ilkzHmZBnZXzshshXNvF5973102 = ilkzHmZBnZXzshshXNvF41660287;     ilkzHmZBnZXzshshXNvF41660287 = ilkzHmZBnZXzshshXNvF91034675;     ilkzHmZBnZXzshshXNvF91034675 = ilkzHmZBnZXzshshXNvF16796204;     ilkzHmZBnZXzshshXNvF16796204 = ilkzHmZBnZXzshshXNvF28074820;     ilkzHmZBnZXzshshXNvF28074820 = ilkzHmZBnZXzshshXNvF32106767;     ilkzHmZBnZXzshshXNvF32106767 = ilkzHmZBnZXzshshXNvF98750565;     ilkzHmZBnZXzshshXNvF98750565 = ilkzHmZBnZXzshshXNvF58464884;     ilkzHmZBnZXzshshXNvF58464884 = ilkzHmZBnZXzshshXNvF6318565;     ilkzHmZBnZXzshshXNvF6318565 = ilkzHmZBnZXzshshXNvF91328372;     ilkzHmZBnZXzshshXNvF91328372 = ilkzHmZBnZXzshshXNvF59240738;     ilkzHmZBnZXzshshXNvF59240738 = ilkzHmZBnZXzshshXNvF36101054;     ilkzHmZBnZXzshshXNvF36101054 = ilkzHmZBnZXzshshXNvF78795344;     ilkzHmZBnZXzshshXNvF78795344 = ilkzHmZBnZXzshshXNvF59463693;     ilkzHmZBnZXzshshXNvF59463693 = ilkzHmZBnZXzshshXNvF12298911;     ilkzHmZBnZXzshshXNvF12298911 = ilkzHmZBnZXzshshXNvF67072545;     ilkzHmZBnZXzshshXNvF67072545 = ilkzHmZBnZXzshshXNvF48344809;     ilkzHmZBnZXzshshXNvF48344809 = ilkzHmZBnZXzshshXNvF54650102;     ilkzHmZBnZXzshshXNvF54650102 = ilkzHmZBnZXzshshXNvF27148674;     ilkzHmZBnZXzshshXNvF27148674 = ilkzHmZBnZXzshshXNvF78096269;     ilkzHmZBnZXzshshXNvF78096269 = ilkzHmZBnZXzshshXNvF28159153;     ilkzHmZBnZXzshshXNvF28159153 = ilkzHmZBnZXzshshXNvF49901226;     ilkzHmZBnZXzshshXNvF49901226 = ilkzHmZBnZXzshshXNvF97990512;     ilkzHmZBnZXzshshXNvF97990512 = ilkzHmZBnZXzshshXNvF27790777;     ilkzHmZBnZXzshshXNvF27790777 = ilkzHmZBnZXzshshXNvF99925278;     ilkzHmZBnZXzshshXNvF99925278 = ilkzHmZBnZXzshshXNvF14932478;     ilkzHmZBnZXzshshXNvF14932478 = ilkzHmZBnZXzshshXNvF77774311;     ilkzHmZBnZXzshshXNvF77774311 = ilkzHmZBnZXzshshXNvF56513306;     ilkzHmZBnZXzshshXNvF56513306 = ilkzHmZBnZXzshshXNvF8259814;     ilkzHmZBnZXzshshXNvF8259814 = ilkzHmZBnZXzshshXNvF77398024;     ilkzHmZBnZXzshshXNvF77398024 = ilkzHmZBnZXzshshXNvF67586815;     ilkzHmZBnZXzshshXNvF67586815 = ilkzHmZBnZXzshshXNvF45147955;     ilkzHmZBnZXzshshXNvF45147955 = ilkzHmZBnZXzshshXNvF46844253;     ilkzHmZBnZXzshshXNvF46844253 = ilkzHmZBnZXzshshXNvF30915448;}
// Junk Finished

// Junk Code By Peatreat & Thaisen's Gen
void FMceOaAMRPlwFhgsJyTb3169719() {     int ARAFERYsLIUXAWCAFLYf33834395 = -704430355;    int ARAFERYsLIUXAWCAFLYf62211265 = -228807082;    int ARAFERYsLIUXAWCAFLYf91156095 = -281850109;    int ARAFERYsLIUXAWCAFLYf88277309 = -860825744;    int ARAFERYsLIUXAWCAFLYf51555472 = -964938082;    int ARAFERYsLIUXAWCAFLYf19110818 = -687396649;    int ARAFERYsLIUXAWCAFLYf79143123 = -496767681;    int ARAFERYsLIUXAWCAFLYf86147236 = -567755442;    int ARAFERYsLIUXAWCAFLYf35063223 = -274089161;    int ARAFERYsLIUXAWCAFLYf95623157 = -974774806;    int ARAFERYsLIUXAWCAFLYf69280969 = -667997996;    int ARAFERYsLIUXAWCAFLYf51289891 = -877550439;    int ARAFERYsLIUXAWCAFLYf17227640 = -923645803;    int ARAFERYsLIUXAWCAFLYf55690256 = -377584745;    int ARAFERYsLIUXAWCAFLYf88265202 = -185052186;    int ARAFERYsLIUXAWCAFLYf55473184 = -425472393;    int ARAFERYsLIUXAWCAFLYf20921907 = -261478193;    int ARAFERYsLIUXAWCAFLYf64380412 = 84221218;    int ARAFERYsLIUXAWCAFLYf54028008 = -367293822;    int ARAFERYsLIUXAWCAFLYf8431946 = -467785395;    int ARAFERYsLIUXAWCAFLYf82780177 = -853096252;    int ARAFERYsLIUXAWCAFLYf97907837 = 50203424;    int ARAFERYsLIUXAWCAFLYf72777011 = -261715336;    int ARAFERYsLIUXAWCAFLYf27290488 = -347081342;    int ARAFERYsLIUXAWCAFLYf82201157 = -731576132;    int ARAFERYsLIUXAWCAFLYf64777598 = -472518827;    int ARAFERYsLIUXAWCAFLYf35863680 = -947869788;    int ARAFERYsLIUXAWCAFLYf43136771 = -27064159;    int ARAFERYsLIUXAWCAFLYf30274662 = -865736120;    int ARAFERYsLIUXAWCAFLYf32877699 = -276601402;    int ARAFERYsLIUXAWCAFLYf63007015 = -897864107;    int ARAFERYsLIUXAWCAFLYf68357191 = -826565207;    int ARAFERYsLIUXAWCAFLYf45451649 = -819115107;    int ARAFERYsLIUXAWCAFLYf35250197 = -37698828;    int ARAFERYsLIUXAWCAFLYf16130286 = -751147225;    int ARAFERYsLIUXAWCAFLYf26648643 = -531584127;    int ARAFERYsLIUXAWCAFLYf85692979 = -276958983;    int ARAFERYsLIUXAWCAFLYf61129474 = -381626187;    int ARAFERYsLIUXAWCAFLYf31354786 = -404684816;    int ARAFERYsLIUXAWCAFLYf12534670 = -821268770;    int ARAFERYsLIUXAWCAFLYf39337395 = -576669358;    int ARAFERYsLIUXAWCAFLYf64683835 = -468986745;    int ARAFERYsLIUXAWCAFLYf92163281 = -511893605;    int ARAFERYsLIUXAWCAFLYf94474536 = -861757987;    int ARAFERYsLIUXAWCAFLYf70366170 = -449760988;    int ARAFERYsLIUXAWCAFLYf32708210 = -713091276;    int ARAFERYsLIUXAWCAFLYf52085890 = -836253113;    int ARAFERYsLIUXAWCAFLYf4846325 = 83171285;    int ARAFERYsLIUXAWCAFLYf74268764 = -224601773;    int ARAFERYsLIUXAWCAFLYf72967652 = -152364761;    int ARAFERYsLIUXAWCAFLYf88210918 = -601880343;    int ARAFERYsLIUXAWCAFLYf78774018 = -887297352;    int ARAFERYsLIUXAWCAFLYf80788298 = -642950849;    int ARAFERYsLIUXAWCAFLYf35086446 = -851772274;    int ARAFERYsLIUXAWCAFLYf20389542 = -383037435;    int ARAFERYsLIUXAWCAFLYf35926557 = -654633780;    int ARAFERYsLIUXAWCAFLYf89434253 = -967091746;    int ARAFERYsLIUXAWCAFLYf63865608 = -934768767;    int ARAFERYsLIUXAWCAFLYf6076153 = -29249612;    int ARAFERYsLIUXAWCAFLYf86777873 = -392419255;    int ARAFERYsLIUXAWCAFLYf83247138 = -739526861;    int ARAFERYsLIUXAWCAFLYf36006353 = -369703522;    int ARAFERYsLIUXAWCAFLYf55872575 = -702019323;    int ARAFERYsLIUXAWCAFLYf2185524 = -997487759;    int ARAFERYsLIUXAWCAFLYf32616143 = 23089300;    int ARAFERYsLIUXAWCAFLYf923779 = -841432790;    int ARAFERYsLIUXAWCAFLYf5838242 = 41564668;    int ARAFERYsLIUXAWCAFLYf81977443 = -785946975;    int ARAFERYsLIUXAWCAFLYf39559971 = -626437521;    int ARAFERYsLIUXAWCAFLYf61616559 = -653468059;    int ARAFERYsLIUXAWCAFLYf69780205 = -48513411;    int ARAFERYsLIUXAWCAFLYf59792432 = -879852007;    int ARAFERYsLIUXAWCAFLYf33025626 = -511093966;    int ARAFERYsLIUXAWCAFLYf41493339 = -546025052;    int ARAFERYsLIUXAWCAFLYf69094551 = -891116037;    int ARAFERYsLIUXAWCAFLYf18096342 = -284109508;    int ARAFERYsLIUXAWCAFLYf5744557 = -437902971;    int ARAFERYsLIUXAWCAFLYf78302475 = -399957350;    int ARAFERYsLIUXAWCAFLYf56924317 = -897320355;    int ARAFERYsLIUXAWCAFLYf49492947 = 81515144;    int ARAFERYsLIUXAWCAFLYf12691709 = -636265715;    int ARAFERYsLIUXAWCAFLYf31017356 = -931041073;    int ARAFERYsLIUXAWCAFLYf68868006 = -802462386;    int ARAFERYsLIUXAWCAFLYf57307009 = -613371360;    int ARAFERYsLIUXAWCAFLYf44666780 = -674721060;    int ARAFERYsLIUXAWCAFLYf84232996 = 89433245;    int ARAFERYsLIUXAWCAFLYf87568892 = -83614359;    int ARAFERYsLIUXAWCAFLYf10365204 = -967342833;    int ARAFERYsLIUXAWCAFLYf14860656 = -654661394;    int ARAFERYsLIUXAWCAFLYf80203728 = 3486555;    int ARAFERYsLIUXAWCAFLYf37214390 = -564492381;    int ARAFERYsLIUXAWCAFLYf21827372 = -342190216;    int ARAFERYsLIUXAWCAFLYf55053322 = -252376575;    int ARAFERYsLIUXAWCAFLYf44576913 = 87734438;    int ARAFERYsLIUXAWCAFLYf29287532 = 18258090;    int ARAFERYsLIUXAWCAFLYf3331043 = -106965836;    int ARAFERYsLIUXAWCAFLYf8811261 = -766967423;    int ARAFERYsLIUXAWCAFLYf89977757 = -514405846;    int ARAFERYsLIUXAWCAFLYf61858394 = -784847288;    int ARAFERYsLIUXAWCAFLYf69442392 = -704430355;     ARAFERYsLIUXAWCAFLYf33834395 = ARAFERYsLIUXAWCAFLYf62211265;     ARAFERYsLIUXAWCAFLYf62211265 = ARAFERYsLIUXAWCAFLYf91156095;     ARAFERYsLIUXAWCAFLYf91156095 = ARAFERYsLIUXAWCAFLYf88277309;     ARAFERYsLIUXAWCAFLYf88277309 = ARAFERYsLIUXAWCAFLYf51555472;     ARAFERYsLIUXAWCAFLYf51555472 = ARAFERYsLIUXAWCAFLYf19110818;     ARAFERYsLIUXAWCAFLYf19110818 = ARAFERYsLIUXAWCAFLYf79143123;     ARAFERYsLIUXAWCAFLYf79143123 = ARAFERYsLIUXAWCAFLYf86147236;     ARAFERYsLIUXAWCAFLYf86147236 = ARAFERYsLIUXAWCAFLYf35063223;     ARAFERYsLIUXAWCAFLYf35063223 = ARAFERYsLIUXAWCAFLYf95623157;     ARAFERYsLIUXAWCAFLYf95623157 = ARAFERYsLIUXAWCAFLYf69280969;     ARAFERYsLIUXAWCAFLYf69280969 = ARAFERYsLIUXAWCAFLYf51289891;     ARAFERYsLIUXAWCAFLYf51289891 = ARAFERYsLIUXAWCAFLYf17227640;     ARAFERYsLIUXAWCAFLYf17227640 = ARAFERYsLIUXAWCAFLYf55690256;     ARAFERYsLIUXAWCAFLYf55690256 = ARAFERYsLIUXAWCAFLYf88265202;     ARAFERYsLIUXAWCAFLYf88265202 = ARAFERYsLIUXAWCAFLYf55473184;     ARAFERYsLIUXAWCAFLYf55473184 = ARAFERYsLIUXAWCAFLYf20921907;     ARAFERYsLIUXAWCAFLYf20921907 = ARAFERYsLIUXAWCAFLYf64380412;     ARAFERYsLIUXAWCAFLYf64380412 = ARAFERYsLIUXAWCAFLYf54028008;     ARAFERYsLIUXAWCAFLYf54028008 = ARAFERYsLIUXAWCAFLYf8431946;     ARAFERYsLIUXAWCAFLYf8431946 = ARAFERYsLIUXAWCAFLYf82780177;     ARAFERYsLIUXAWCAFLYf82780177 = ARAFERYsLIUXAWCAFLYf97907837;     ARAFERYsLIUXAWCAFLYf97907837 = ARAFERYsLIUXAWCAFLYf72777011;     ARAFERYsLIUXAWCAFLYf72777011 = ARAFERYsLIUXAWCAFLYf27290488;     ARAFERYsLIUXAWCAFLYf27290488 = ARAFERYsLIUXAWCAFLYf82201157;     ARAFERYsLIUXAWCAFLYf82201157 = ARAFERYsLIUXAWCAFLYf64777598;     ARAFERYsLIUXAWCAFLYf64777598 = ARAFERYsLIUXAWCAFLYf35863680;     ARAFERYsLIUXAWCAFLYf35863680 = ARAFERYsLIUXAWCAFLYf43136771;     ARAFERYsLIUXAWCAFLYf43136771 = ARAFERYsLIUXAWCAFLYf30274662;     ARAFERYsLIUXAWCAFLYf30274662 = ARAFERYsLIUXAWCAFLYf32877699;     ARAFERYsLIUXAWCAFLYf32877699 = ARAFERYsLIUXAWCAFLYf63007015;     ARAFERYsLIUXAWCAFLYf63007015 = ARAFERYsLIUXAWCAFLYf68357191;     ARAFERYsLIUXAWCAFLYf68357191 = ARAFERYsLIUXAWCAFLYf45451649;     ARAFERYsLIUXAWCAFLYf45451649 = ARAFERYsLIUXAWCAFLYf35250197;     ARAFERYsLIUXAWCAFLYf35250197 = ARAFERYsLIUXAWCAFLYf16130286;     ARAFERYsLIUXAWCAFLYf16130286 = ARAFERYsLIUXAWCAFLYf26648643;     ARAFERYsLIUXAWCAFLYf26648643 = ARAFERYsLIUXAWCAFLYf85692979;     ARAFERYsLIUXAWCAFLYf85692979 = ARAFERYsLIUXAWCAFLYf61129474;     ARAFERYsLIUXAWCAFLYf61129474 = ARAFERYsLIUXAWCAFLYf31354786;     ARAFERYsLIUXAWCAFLYf31354786 = ARAFERYsLIUXAWCAFLYf12534670;     ARAFERYsLIUXAWCAFLYf12534670 = ARAFERYsLIUXAWCAFLYf39337395;     ARAFERYsLIUXAWCAFLYf39337395 = ARAFERYsLIUXAWCAFLYf64683835;     ARAFERYsLIUXAWCAFLYf64683835 = ARAFERYsLIUXAWCAFLYf92163281;     ARAFERYsLIUXAWCAFLYf92163281 = ARAFERYsLIUXAWCAFLYf94474536;     ARAFERYsLIUXAWCAFLYf94474536 = ARAFERYsLIUXAWCAFLYf70366170;     ARAFERYsLIUXAWCAFLYf70366170 = ARAFERYsLIUXAWCAFLYf32708210;     ARAFERYsLIUXAWCAFLYf32708210 = ARAFERYsLIUXAWCAFLYf52085890;     ARAFERYsLIUXAWCAFLYf52085890 = ARAFERYsLIUXAWCAFLYf4846325;     ARAFERYsLIUXAWCAFLYf4846325 = ARAFERYsLIUXAWCAFLYf74268764;     ARAFERYsLIUXAWCAFLYf74268764 = ARAFERYsLIUXAWCAFLYf72967652;     ARAFERYsLIUXAWCAFLYf72967652 = ARAFERYsLIUXAWCAFLYf88210918;     ARAFERYsLIUXAWCAFLYf88210918 = ARAFERYsLIUXAWCAFLYf78774018;     ARAFERYsLIUXAWCAFLYf78774018 = ARAFERYsLIUXAWCAFLYf80788298;     ARAFERYsLIUXAWCAFLYf80788298 = ARAFERYsLIUXAWCAFLYf35086446;     ARAFERYsLIUXAWCAFLYf35086446 = ARAFERYsLIUXAWCAFLYf20389542;     ARAFERYsLIUXAWCAFLYf20389542 = ARAFERYsLIUXAWCAFLYf35926557;     ARAFERYsLIUXAWCAFLYf35926557 = ARAFERYsLIUXAWCAFLYf89434253;     ARAFERYsLIUXAWCAFLYf89434253 = ARAFERYsLIUXAWCAFLYf63865608;     ARAFERYsLIUXAWCAFLYf63865608 = ARAFERYsLIUXAWCAFLYf6076153;     ARAFERYsLIUXAWCAFLYf6076153 = ARAFERYsLIUXAWCAFLYf86777873;     ARAFERYsLIUXAWCAFLYf86777873 = ARAFERYsLIUXAWCAFLYf83247138;     ARAFERYsLIUXAWCAFLYf83247138 = ARAFERYsLIUXAWCAFLYf36006353;     ARAFERYsLIUXAWCAFLYf36006353 = ARAFERYsLIUXAWCAFLYf55872575;     ARAFERYsLIUXAWCAFLYf55872575 = ARAFERYsLIUXAWCAFLYf2185524;     ARAFERYsLIUXAWCAFLYf2185524 = ARAFERYsLIUXAWCAFLYf32616143;     ARAFERYsLIUXAWCAFLYf32616143 = ARAFERYsLIUXAWCAFLYf923779;     ARAFERYsLIUXAWCAFLYf923779 = ARAFERYsLIUXAWCAFLYf5838242;     ARAFERYsLIUXAWCAFLYf5838242 = ARAFERYsLIUXAWCAFLYf81977443;     ARAFERYsLIUXAWCAFLYf81977443 = ARAFERYsLIUXAWCAFLYf39559971;     ARAFERYsLIUXAWCAFLYf39559971 = ARAFERYsLIUXAWCAFLYf61616559;     ARAFERYsLIUXAWCAFLYf61616559 = ARAFERYsLIUXAWCAFLYf69780205;     ARAFERYsLIUXAWCAFLYf69780205 = ARAFERYsLIUXAWCAFLYf59792432;     ARAFERYsLIUXAWCAFLYf59792432 = ARAFERYsLIUXAWCAFLYf33025626;     ARAFERYsLIUXAWCAFLYf33025626 = ARAFERYsLIUXAWCAFLYf41493339;     ARAFERYsLIUXAWCAFLYf41493339 = ARAFERYsLIUXAWCAFLYf69094551;     ARAFERYsLIUXAWCAFLYf69094551 = ARAFERYsLIUXAWCAFLYf18096342;     ARAFERYsLIUXAWCAFLYf18096342 = ARAFERYsLIUXAWCAFLYf5744557;     ARAFERYsLIUXAWCAFLYf5744557 = ARAFERYsLIUXAWCAFLYf78302475;     ARAFERYsLIUXAWCAFLYf78302475 = ARAFERYsLIUXAWCAFLYf56924317;     ARAFERYsLIUXAWCAFLYf56924317 = ARAFERYsLIUXAWCAFLYf49492947;     ARAFERYsLIUXAWCAFLYf49492947 = ARAFERYsLIUXAWCAFLYf12691709;     ARAFERYsLIUXAWCAFLYf12691709 = ARAFERYsLIUXAWCAFLYf31017356;     ARAFERYsLIUXAWCAFLYf31017356 = ARAFERYsLIUXAWCAFLYf68868006;     ARAFERYsLIUXAWCAFLYf68868006 = ARAFERYsLIUXAWCAFLYf57307009;     ARAFERYsLIUXAWCAFLYf57307009 = ARAFERYsLIUXAWCAFLYf44666780;     ARAFERYsLIUXAWCAFLYf44666780 = ARAFERYsLIUXAWCAFLYf84232996;     ARAFERYsLIUXAWCAFLYf84232996 = ARAFERYsLIUXAWCAFLYf87568892;     ARAFERYsLIUXAWCAFLYf87568892 = ARAFERYsLIUXAWCAFLYf10365204;     ARAFERYsLIUXAWCAFLYf10365204 = ARAFERYsLIUXAWCAFLYf14860656;     ARAFERYsLIUXAWCAFLYf14860656 = ARAFERYsLIUXAWCAFLYf80203728;     ARAFERYsLIUXAWCAFLYf80203728 = ARAFERYsLIUXAWCAFLYf37214390;     ARAFERYsLIUXAWCAFLYf37214390 = ARAFERYsLIUXAWCAFLYf21827372;     ARAFERYsLIUXAWCAFLYf21827372 = ARAFERYsLIUXAWCAFLYf55053322;     ARAFERYsLIUXAWCAFLYf55053322 = ARAFERYsLIUXAWCAFLYf44576913;     ARAFERYsLIUXAWCAFLYf44576913 = ARAFERYsLIUXAWCAFLYf29287532;     ARAFERYsLIUXAWCAFLYf29287532 = ARAFERYsLIUXAWCAFLYf3331043;     ARAFERYsLIUXAWCAFLYf3331043 = ARAFERYsLIUXAWCAFLYf8811261;     ARAFERYsLIUXAWCAFLYf8811261 = ARAFERYsLIUXAWCAFLYf89977757;     ARAFERYsLIUXAWCAFLYf89977757 = ARAFERYsLIUXAWCAFLYf61858394;     ARAFERYsLIUXAWCAFLYf61858394 = ARAFERYsLIUXAWCAFLYf69442392;     ARAFERYsLIUXAWCAFLYf69442392 = ARAFERYsLIUXAWCAFLYf33834395;}
// Junk Finished

// Junk Code By Peatreat & Thaisen's Gen
void nezRPdUNGLpVFvshPVWM79512035() {     int XuLlGQZPzFXEudduITtk66095203 = -280078118;    int XuLlGQZPzFXEudduITtk63433700 = -902423966;    int XuLlGQZPzFXEudduITtk24432675 = -384709492;    int XuLlGQZPzFXEudduITtk29785423 = -681061454;    int XuLlGQZPzFXEudduITtk36672991 = -514671641;    int XuLlGQZPzFXEudduITtk53207530 = -655371919;    int XuLlGQZPzFXEudduITtk76635424 = -681209514;    int XuLlGQZPzFXEudduITtk8902532 = -301904348;    int XuLlGQZPzFXEudduITtk8414667 = -150392562;    int XuLlGQZPzFXEudduITtk83131072 = -47641614;    int XuLlGQZPzFXEudduITtk30671010 = -156202239;    int XuLlGQZPzFXEudduITtk72443332 = -548707597;    int XuLlGQZPzFXEudduITtk45889688 = -130127262;    int XuLlGQZPzFXEudduITtk59886155 = -907132617;    int XuLlGQZPzFXEudduITtk26926108 = -427408956;    int XuLlGQZPzFXEudduITtk75549355 = -6760065;    int XuLlGQZPzFXEudduITtk26783078 = -277412772;    int XuLlGQZPzFXEudduITtk49001116 = 75327820;    int XuLlGQZPzFXEudduITtk34395248 = -35985511;    int XuLlGQZPzFXEudduITtk19623981 = -72541539;    int XuLlGQZPzFXEudduITtk95740881 = -40158468;    int XuLlGQZPzFXEudduITtk63641474 = -121466253;    int XuLlGQZPzFXEudduITtk42831917 = -31336007;    int XuLlGQZPzFXEudduITtk45869178 = -463049824;    int XuLlGQZPzFXEudduITtk70856215 = -809624313;    int XuLlGQZPzFXEudduITtk34289628 = 59460684;    int XuLlGQZPzFXEudduITtk60985765 = -945927287;    int XuLlGQZPzFXEudduITtk25877846 = -855628554;    int XuLlGQZPzFXEudduITtk75958225 = -447794191;    int XuLlGQZPzFXEudduITtk94245798 = -322494878;    int XuLlGQZPzFXEudduITtk3834418 = -652513715;    int XuLlGQZPzFXEudduITtk26531468 = -977812142;    int XuLlGQZPzFXEudduITtk17521279 = 51766558;    int XuLlGQZPzFXEudduITtk70800239 = -370374520;    int XuLlGQZPzFXEudduITtk56763116 = -64567986;    int XuLlGQZPzFXEudduITtk15110506 = -543950093;    int XuLlGQZPzFXEudduITtk78899122 = -494091030;    int XuLlGQZPzFXEudduITtk90398292 = -383192415;    int XuLlGQZPzFXEudduITtk69129900 = 1731198;    int XuLlGQZPzFXEudduITtk88208242 = 47214973;    int XuLlGQZPzFXEudduITtk54395092 = -273985749;    int XuLlGQZPzFXEudduITtk98547763 = -412402223;    int XuLlGQZPzFXEudduITtk37896632 = 76001407;    int XuLlGQZPzFXEudduITtk63256715 = -995038447;    int XuLlGQZPzFXEudduITtk46385020 = -146394510;    int XuLlGQZPzFXEudduITtk20369498 = -411696466;    int XuLlGQZPzFXEudduITtk83959997 = -766180866;    int XuLlGQZPzFXEudduITtk59058540 = -746707453;    int XuLlGQZPzFXEudduITtk40810016 = -838379125;    int XuLlGQZPzFXEudduITtk40770993 = -937388682;    int XuLlGQZPzFXEudduITtk67714611 = -582574575;    int XuLlGQZPzFXEudduITtk8852169 = -253689801;    int XuLlGQZPzFXEudduITtk9650247 = -603009084;    int XuLlGQZPzFXEudduITtk42465929 = 96102199;    int XuLlGQZPzFXEudduITtk83771153 = -395141278;    int XuLlGQZPzFXEudduITtk2453730 = -58611865;    int XuLlGQZPzFXEudduITtk20601784 = -771087960;    int XuLlGQZPzFXEudduITtk78563497 = -921659668;    int XuLlGQZPzFXEudduITtk58929207 = -871437142;    int XuLlGQZPzFXEudduITtk2383363 = -474132326;    int XuLlGQZPzFXEudduITtk92221765 = -709444632;    int XuLlGQZPzFXEudduITtk50757578 = -825580960;    int XuLlGQZPzFXEudduITtk32944307 = -854110157;    int XuLlGQZPzFXEudduITtk14168868 = -827897685;    int XuLlGQZPzFXEudduITtk79296654 = -395127899;    int XuLlGQZPzFXEudduITtk4139543 = -178390098;    int XuLlGQZPzFXEudduITtk54922054 = -500474155;    int XuLlGQZPzFXEudduITtk75089448 = -759752742;    int XuLlGQZPzFXEudduITtk3123039 = -742564631;    int XuLlGQZPzFXEudduITtk11815603 = -883458863;    int XuLlGQZPzFXEudduITtk96650232 = -512669036;    int XuLlGQZPzFXEudduITtk36384786 = -894220357;    int XuLlGQZPzFXEudduITtk79871215 = -926403378;    int XuLlGQZPzFXEudduITtk46187006 = 16799515;    int XuLlGQZPzFXEudduITtk65228889 = -798555790;    int XuLlGQZPzFXEudduITtk97193118 = -627756246;    int XuLlGQZPzFXEudduITtk25744842 = -97467661;    int XuLlGQZPzFXEudduITtk79575201 = -36297561;    int XuLlGQZPzFXEudduITtk99484157 = -216655315;    int XuLlGQZPzFXEudduITtk50486718 = -297927847;    int XuLlGQZPzFXEudduITtk50329631 = -174358451;    int XuLlGQZPzFXEudduITtk1927226 = -99219835;    int XuLlGQZPzFXEudduITtk85067830 = 82750570;    int XuLlGQZPzFXEudduITtk35187233 = -510405509;    int XuLlGQZPzFXEudduITtk26531187 = -739920303;    int XuLlGQZPzFXEudduITtk94982248 = -298823914;    int XuLlGQZPzFXEudduITtk16881221 = -274803058;    int XuLlGQZPzFXEudduITtk75055349 = 55664358;    int XuLlGQZPzFXEudduITtk87029085 = -975233242;    int XuLlGQZPzFXEudduITtk54309386 = 94043878;    int XuLlGQZPzFXEudduITtk94508721 = -772862134;    int XuLlGQZPzFXEudduITtk335625 = -572431362;    int XuLlGQZPzFXEudduITtk31469085 = -511755274;    int XuLlGQZPzFXEudduITtk66746538 = -524136477;    int XuLlGQZPzFXEudduITtk95986477 = -243340395;    int XuLlGQZPzFXEudduITtk3637514 = -448404789;    int XuLlGQZPzFXEudduITtk65603456 = -558292066;    int XuLlGQZPzFXEudduITtk23727764 = -96100908;    int XuLlGQZPzFXEudduITtk83960060 = -499910548;    int XuLlGQZPzFXEudduITtk42245478 = -280078118;     XuLlGQZPzFXEudduITtk66095203 = XuLlGQZPzFXEudduITtk63433700;     XuLlGQZPzFXEudduITtk63433700 = XuLlGQZPzFXEudduITtk24432675;     XuLlGQZPzFXEudduITtk24432675 = XuLlGQZPzFXEudduITtk29785423;     XuLlGQZPzFXEudduITtk29785423 = XuLlGQZPzFXEudduITtk36672991;     XuLlGQZPzFXEudduITtk36672991 = XuLlGQZPzFXEudduITtk53207530;     XuLlGQZPzFXEudduITtk53207530 = XuLlGQZPzFXEudduITtk76635424;     XuLlGQZPzFXEudduITtk76635424 = XuLlGQZPzFXEudduITtk8902532;     XuLlGQZPzFXEudduITtk8902532 = XuLlGQZPzFXEudduITtk8414667;     XuLlGQZPzFXEudduITtk8414667 = XuLlGQZPzFXEudduITtk83131072;     XuLlGQZPzFXEudduITtk83131072 = XuLlGQZPzFXEudduITtk30671010;     XuLlGQZPzFXEudduITtk30671010 = XuLlGQZPzFXEudduITtk72443332;     XuLlGQZPzFXEudduITtk72443332 = XuLlGQZPzFXEudduITtk45889688;     XuLlGQZPzFXEudduITtk45889688 = XuLlGQZPzFXEudduITtk59886155;     XuLlGQZPzFXEudduITtk59886155 = XuLlGQZPzFXEudduITtk26926108;     XuLlGQZPzFXEudduITtk26926108 = XuLlGQZPzFXEudduITtk75549355;     XuLlGQZPzFXEudduITtk75549355 = XuLlGQZPzFXEudduITtk26783078;     XuLlGQZPzFXEudduITtk26783078 = XuLlGQZPzFXEudduITtk49001116;     XuLlGQZPzFXEudduITtk49001116 = XuLlGQZPzFXEudduITtk34395248;     XuLlGQZPzFXEudduITtk34395248 = XuLlGQZPzFXEudduITtk19623981;     XuLlGQZPzFXEudduITtk19623981 = XuLlGQZPzFXEudduITtk95740881;     XuLlGQZPzFXEudduITtk95740881 = XuLlGQZPzFXEudduITtk63641474;     XuLlGQZPzFXEudduITtk63641474 = XuLlGQZPzFXEudduITtk42831917;     XuLlGQZPzFXEudduITtk42831917 = XuLlGQZPzFXEudduITtk45869178;     XuLlGQZPzFXEudduITtk45869178 = XuLlGQZPzFXEudduITtk70856215;     XuLlGQZPzFXEudduITtk70856215 = XuLlGQZPzFXEudduITtk34289628;     XuLlGQZPzFXEudduITtk34289628 = XuLlGQZPzFXEudduITtk60985765;     XuLlGQZPzFXEudduITtk60985765 = XuLlGQZPzFXEudduITtk25877846;     XuLlGQZPzFXEudduITtk25877846 = XuLlGQZPzFXEudduITtk75958225;     XuLlGQZPzFXEudduITtk75958225 = XuLlGQZPzFXEudduITtk94245798;     XuLlGQZPzFXEudduITtk94245798 = XuLlGQZPzFXEudduITtk3834418;     XuLlGQZPzFXEudduITtk3834418 = XuLlGQZPzFXEudduITtk26531468;     XuLlGQZPzFXEudduITtk26531468 = XuLlGQZPzFXEudduITtk17521279;     XuLlGQZPzFXEudduITtk17521279 = XuLlGQZPzFXEudduITtk70800239;     XuLlGQZPzFXEudduITtk70800239 = XuLlGQZPzFXEudduITtk56763116;     XuLlGQZPzFXEudduITtk56763116 = XuLlGQZPzFXEudduITtk15110506;     XuLlGQZPzFXEudduITtk15110506 = XuLlGQZPzFXEudduITtk78899122;     XuLlGQZPzFXEudduITtk78899122 = XuLlGQZPzFXEudduITtk90398292;     XuLlGQZPzFXEudduITtk90398292 = XuLlGQZPzFXEudduITtk69129900;     XuLlGQZPzFXEudduITtk69129900 = XuLlGQZPzFXEudduITtk88208242;     XuLlGQZPzFXEudduITtk88208242 = XuLlGQZPzFXEudduITtk54395092;     XuLlGQZPzFXEudduITtk54395092 = XuLlGQZPzFXEudduITtk98547763;     XuLlGQZPzFXEudduITtk98547763 = XuLlGQZPzFXEudduITtk37896632;     XuLlGQZPzFXEudduITtk37896632 = XuLlGQZPzFXEudduITtk63256715;     XuLlGQZPzFXEudduITtk63256715 = XuLlGQZPzFXEudduITtk46385020;     XuLlGQZPzFXEudduITtk46385020 = XuLlGQZPzFXEudduITtk20369498;     XuLlGQZPzFXEudduITtk20369498 = XuLlGQZPzFXEudduITtk83959997;     XuLlGQZPzFXEudduITtk83959997 = XuLlGQZPzFXEudduITtk59058540;     XuLlGQZPzFXEudduITtk59058540 = XuLlGQZPzFXEudduITtk40810016;     XuLlGQZPzFXEudduITtk40810016 = XuLlGQZPzFXEudduITtk40770993;     XuLlGQZPzFXEudduITtk40770993 = XuLlGQZPzFXEudduITtk67714611;     XuLlGQZPzFXEudduITtk67714611 = XuLlGQZPzFXEudduITtk8852169;     XuLlGQZPzFXEudduITtk8852169 = XuLlGQZPzFXEudduITtk9650247;     XuLlGQZPzFXEudduITtk9650247 = XuLlGQZPzFXEudduITtk42465929;     XuLlGQZPzFXEudduITtk42465929 = XuLlGQZPzFXEudduITtk83771153;     XuLlGQZPzFXEudduITtk83771153 = XuLlGQZPzFXEudduITtk2453730;     XuLlGQZPzFXEudduITtk2453730 = XuLlGQZPzFXEudduITtk20601784;     XuLlGQZPzFXEudduITtk20601784 = XuLlGQZPzFXEudduITtk78563497;     XuLlGQZPzFXEudduITtk78563497 = XuLlGQZPzFXEudduITtk58929207;     XuLlGQZPzFXEudduITtk58929207 = XuLlGQZPzFXEudduITtk2383363;     XuLlGQZPzFXEudduITtk2383363 = XuLlGQZPzFXEudduITtk92221765;     XuLlGQZPzFXEudduITtk92221765 = XuLlGQZPzFXEudduITtk50757578;     XuLlGQZPzFXEudduITtk50757578 = XuLlGQZPzFXEudduITtk32944307;     XuLlGQZPzFXEudduITtk32944307 = XuLlGQZPzFXEudduITtk14168868;     XuLlGQZPzFXEudduITtk14168868 = XuLlGQZPzFXEudduITtk79296654;     XuLlGQZPzFXEudduITtk79296654 = XuLlGQZPzFXEudduITtk4139543;     XuLlGQZPzFXEudduITtk4139543 = XuLlGQZPzFXEudduITtk54922054;     XuLlGQZPzFXEudduITtk54922054 = XuLlGQZPzFXEudduITtk75089448;     XuLlGQZPzFXEudduITtk75089448 = XuLlGQZPzFXEudduITtk3123039;     XuLlGQZPzFXEudduITtk3123039 = XuLlGQZPzFXEudduITtk11815603;     XuLlGQZPzFXEudduITtk11815603 = XuLlGQZPzFXEudduITtk96650232;     XuLlGQZPzFXEudduITtk96650232 = XuLlGQZPzFXEudduITtk36384786;     XuLlGQZPzFXEudduITtk36384786 = XuLlGQZPzFXEudduITtk79871215;     XuLlGQZPzFXEudduITtk79871215 = XuLlGQZPzFXEudduITtk46187006;     XuLlGQZPzFXEudduITtk46187006 = XuLlGQZPzFXEudduITtk65228889;     XuLlGQZPzFXEudduITtk65228889 = XuLlGQZPzFXEudduITtk97193118;     XuLlGQZPzFXEudduITtk97193118 = XuLlGQZPzFXEudduITtk25744842;     XuLlGQZPzFXEudduITtk25744842 = XuLlGQZPzFXEudduITtk79575201;     XuLlGQZPzFXEudduITtk79575201 = XuLlGQZPzFXEudduITtk99484157;     XuLlGQZPzFXEudduITtk99484157 = XuLlGQZPzFXEudduITtk50486718;     XuLlGQZPzFXEudduITtk50486718 = XuLlGQZPzFXEudduITtk50329631;     XuLlGQZPzFXEudduITtk50329631 = XuLlGQZPzFXEudduITtk1927226;     XuLlGQZPzFXEudduITtk1927226 = XuLlGQZPzFXEudduITtk85067830;     XuLlGQZPzFXEudduITtk85067830 = XuLlGQZPzFXEudduITtk35187233;     XuLlGQZPzFXEudduITtk35187233 = XuLlGQZPzFXEudduITtk26531187;     XuLlGQZPzFXEudduITtk26531187 = XuLlGQZPzFXEudduITtk94982248;     XuLlGQZPzFXEudduITtk94982248 = XuLlGQZPzFXEudduITtk16881221;     XuLlGQZPzFXEudduITtk16881221 = XuLlGQZPzFXEudduITtk75055349;     XuLlGQZPzFXEudduITtk75055349 = XuLlGQZPzFXEudduITtk87029085;     XuLlGQZPzFXEudduITtk87029085 = XuLlGQZPzFXEudduITtk54309386;     XuLlGQZPzFXEudduITtk54309386 = XuLlGQZPzFXEudduITtk94508721;     XuLlGQZPzFXEudduITtk94508721 = XuLlGQZPzFXEudduITtk335625;     XuLlGQZPzFXEudduITtk335625 = XuLlGQZPzFXEudduITtk31469085;     XuLlGQZPzFXEudduITtk31469085 = XuLlGQZPzFXEudduITtk66746538;     XuLlGQZPzFXEudduITtk66746538 = XuLlGQZPzFXEudduITtk95986477;     XuLlGQZPzFXEudduITtk95986477 = XuLlGQZPzFXEudduITtk3637514;     XuLlGQZPzFXEudduITtk3637514 = XuLlGQZPzFXEudduITtk65603456;     XuLlGQZPzFXEudduITtk65603456 = XuLlGQZPzFXEudduITtk23727764;     XuLlGQZPzFXEudduITtk23727764 = XuLlGQZPzFXEudduITtk83960060;     XuLlGQZPzFXEudduITtk83960060 = XuLlGQZPzFXEudduITtk42245478;     XuLlGQZPzFXEudduITtk42245478 = XuLlGQZPzFXEudduITtk66095203;}
// Junk Finished
