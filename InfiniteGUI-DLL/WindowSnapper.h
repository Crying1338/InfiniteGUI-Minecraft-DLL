#pragma once
#include "imgui/imgui.h"
#include <intrin.h>
#include <Windows.h>
#include <stdint.h>
#include <algorithm>
#include "ItemManager.h"
enum SnapState : uint32_t {
    SNAP_NONE = 0,

    // 边
    SNAP_LEFT = 1 << 0,
    SNAP_RIGHT = 1 << 1,
    SNAP_TOP = 1 << 2,
    SNAP_BOTTOM = 1 << 3,

    // 中心
    SNAP_CENTER_X = 1 << 4,
    SNAP_CENTER_Y = 1 << 5,

    // 边
    SNAP_OTHER_LEFT = 1 << 6,
    SNAP_OTHER_RIGHT = 1 << 7,
    SNAP_OTHER_TOP = 1 << 8,
    SNAP_OTHER_BOTTOM = 1 << 9,

    // 中心
    SNAP_OTHER_CENTER_X = 1 << 10,
    SNAP_OTHER_CENTER_Y = 1 << 11,
};

struct SnapResult {
    ImVec2 snappedPos;
    SnapState snapState = SNAP_NONE;
};

class WindowSnapper
{
public:
    static SnapResult ComputeSnap(
        const ImVec2& pos,
        const ImVec2& size,
        float screenW,
        float screenH,
        float snapDist);
    static void ComputeSnapWithWindows(const ImVec2& size,
                                       float snapDist, const std::vector<Item*>& items, SnapResult& r);

    static void DrawGuides(const SnapResult& r, float screenW, float screenH, const ImVec2& size);
    static void KeepSnapped(ImVec2& pos, const ImVec2& size, float screenW, float screenH, const SnapState& r);
};