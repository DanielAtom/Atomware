#pragma once

#include "VirtualMethod.h"

class GameUI {
public:
    /*constexpr void messageBox(const char* title, const char* text) noexcept
    {
        callVirtualMethod<void, const char*, const char*, bool, bool, const char*, const char*, const char*, const char*, void*>(this, 20, title, text, true, false, nullptr, nullptr, nullptr, nullptr, nullptr);
    }*/

    VIRTUAL_METHOD(void, createCommandMsgBox, 20, (const char* title, const char* message, bool showOk = true, bool showCancel = false, const char* okCommand = nullptr, const char* cancelCommand = nullptr, const char* closedCommand = nullptr, const char* legend = nullptr, const char* unknown = nullptr), (this, title, message, showOk, showCancel, okCommand, cancelCommand, closedCommand, legend, unknown))
};