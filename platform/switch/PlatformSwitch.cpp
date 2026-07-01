#ifdef __SWITCH__

#include <switch.h>
#include <unistd.h>
#include <sys/stat.h>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <dirent.h>

extern "C" {
    u32 __nx_applet_type = AppletType_Default;
    void userAppInit(void) {}
    void userAppExit(void) {}
}

namespace PlatformSwitch
{

static bool s_socketsUp = false;
static bool s_romfsUp   = false;

void Init()
{
    if (R_SUCCEEDED(socketInitializeDefault())) {
        s_socketsUp = true;
        nxlinkStdio();
    }

    std::printf("\n=== th06-switch bring-up ===\n");

    Result rc = romfsInit();
    if (R_SUCCEEDED(rc)) {
        s_romfsUp = true;
        std::printf("romfs OK\n");
    }

    if (DIR *d = opendir(".")) {
        std::printf("cwd listing:\n");
        int n = 0;
        struct dirent *e;
        while ((e = readdir(d)) && n < 20) {
            std::printf("  %s\n", e->d_name);
            n++;
        }
        closedir(d);
    }
}

void Shutdown()
{
    if (s_romfsUp)   { romfsExit();  s_romfsUp = false; }
    if (s_socketsUp) { socketExit(); s_socketsUp = false; }
}

bool ShouldKeepRunning()
{
    return appletMainLoop();
}

} // namespace PlatformSwitch

#endif // __SWITCH__
