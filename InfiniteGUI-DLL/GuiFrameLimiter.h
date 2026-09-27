#include <chrono>
#include <cmath>
#include <windows.h>

class GuiFrameLimiter
{
public:
    static GuiFrameLimiter& Instance()
    {
        static GuiFrameLimiter instance;
        return instance;
    }


    void SetMaxFps(double maxFps)
    {
        interval = std::chrono::duration<double>(1.0 / maxFps);
    }

    bool ShouldUpdate()
    {
        auto now = Clock::now();
        auto elapsed = now - lastTime;
        if (elapsed >= interval)
        {
            auto steps = std::floor(elapsed / interval);

            lastTime += std::chrono::duration_cast<Clock::duration>(
                interval * steps
            );
            return true;
        }
        return false;
    }

    void Init()
    {
        double refreshRate = GetRefreshRateSimple();
        SetMaxFps(refreshRate);
        lastTime = Clock::now();
    }

    static int GetRefreshRateSimple()
    {
        DEVMODE dm = {};
        dm.dmSize = sizeof(dm);
        EnumDisplaySettings(nullptr, ENUM_CURRENT_SETTINGS, &dm);
        return dm.dmDisplayFrequency ? dm.dmDisplayFrequency : 60;
    }

private:
    using Clock = std::chrono::steady_clock;
    Clock::time_point lastTime;
    std::chrono::duration<double> interval;
};

