#pragma once

struct UserCmd;
struct Vector;

namespace AntiAim {
    void run(UserCmd*, const Vector&, const Vector&, bool&) noexcept;
    void indicators() noexcept;
    void Resolver() noexcept;
    void FakeDuck(UserCmd* cmd, bool& sendPacket) noexcept;
    void Resolverp100();
}
