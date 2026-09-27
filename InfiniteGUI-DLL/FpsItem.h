#include "Item.h"
#include "AffixModule.h"
#include "UpdateModule.h"
#include "WindowModule.h"

struct fps_element {
    ImVec4 color;
};

class FpsItem : public Item, public AffixModule, public UpdateModule, public WindowModule
{
public:
    FpsItem() {
        type = Hud; // 信息项类型
        name = u8"FPS显示";
        description = u8"显示当前帧率";
        icon = u8"\uE022";
        updateIntervalMs = 1000;
        lastUpdateTime = std::chrono::steady_clock::now();
        FpsItem::Reset();
    }

    static FpsItem& Instance() {
        static FpsItem instance;
        return instance;
    }

    void Toggle() override;
    void Reset() override
    {
        ResetAffix();
        ResetWindow();
        isEnabled = false;
        prefix = "[";
        suffix = "FPS]";
        showGuiFPS = false;
        processShowGuiFPS();
        dirtyState.contentDirty = true;
        dirtyState.animating = true;
    }
    void Update() override;
    void HoverSetting() override;
    void DrawContent() override;
    void RenderBeforeGui() override;
    void DrawSettings(const float& bigPadding, const float& centerX, const float& itemWidth) override;
    void Load(const nlohmann::json& j) override;
    void Save(nlohmann::json& j) const override;

    double GetGameDeltaTime() const
    {
        return deltaTime;
    }
    double GetInstantaneousFPS() const
    {
        return 1.0 / deltaTime;
    }

private:
    void processShowGuiFPS()
    {
        renderTask.clear();
        renderTask.gui = true;
        renderTask.before = true;
    }
    // 帧率计数器
    int frameCount = 0;
    float FPS = 0.0f;
    int guiFrameCount = 0;
    float guiFPS = 0.0f;
    bool showGuiFPS = false;
    fps_element color;
    double deltaTime = 0.0;
    using Clock = std::chrono::steady_clock;
    Clock::time_point lastFrameTime;
};