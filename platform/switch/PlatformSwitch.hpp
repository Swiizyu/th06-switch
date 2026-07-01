#pragma once

#ifdef __SWITCH__
namespace PlatformSwitch
{
    void Init();
    void Shutdown();
    bool ShouldKeepRunning();
}
#endif
