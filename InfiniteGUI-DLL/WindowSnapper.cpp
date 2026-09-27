#include "WindowSnapper.h"
#include <cmath>

#include "GameStateDetector.h"
#include "WindowModule.h"
#include "imgui/imgui_internal.h"

SnapResult WindowSnapper::ComputeSnap(
    const ImVec2& pos,
    const ImVec2& size,
    float screenW,
    float screenH,
    float snapDist)
{
    SnapResult r;
    SnapState state = SNAP_NONE;
    r.snappedPos = pos;
    if(GetAsyncKeyState(VK_CONTROL) & 0x8000)
    {
        float right = pos.x + size.x;
        float bottom = pos.y + size.y;

        float cx = pos.x + size.x * 0.5f;
        float cy = pos.y + size.y * 0.5f;

        // 边缘吸附
        if (fabs(pos.x) < snapDist) {
            r.snappedPos.x = 0;
            state = static_cast<SnapState>(state | SNAP_LEFT);
        }
        if (fabs(right - screenW) < snapDist) {
            r.snappedPos.x = screenW - size.x;
            state = static_cast<SnapState>(state | SNAP_RIGHT);
        }
        if (fabs(pos.y) < snapDist) {
            r.snappedPos.y = 0;
            state = static_cast<SnapState>(state | SNAP_TOP);
        }
        if (fabs(bottom - screenH) < snapDist) {
            r.snappedPos.y = screenH - size.y;
            state = static_cast<SnapState>(state | SNAP_BOTTOM);
        }

        // 中心吸附
        if (fabs(cx - screenW * 0.5f) < snapDist) {
            r.snappedPos.x = screenW * 0.5f - size.x * 0.5f;
            state = static_cast<SnapState>(state | SNAP_CENTER_X);
        }
        if (fabs(cy - screenH * 0.5f) < snapDist) {
            r.snappedPos.y = screenH * 0.5f - size.y * 0.5f;
            state = static_cast<SnapState>(state | SNAP_CENTER_Y);
        }

        r.snapState = state;
    }
    if(GetAsyncKeyState(VK_LSHIFT) & 0x8000)
        ComputeSnapWithWindows(size, snapDist, ItemManager::Instance().GetItems(), r);
    return r;
}

void WindowSnapper::ComputeSnapWithWindows(
    const ImVec2& size,
    float snapDist,
    const std::vector<Item*>& items,
    SnapResult& r
    )
{
    float right = r.snappedPos.x + size.x;
    float bottom = r.snappedPos.y + size.y;
    float cx = r.snappedPos.x + size.x * 0.5f;
    float cy = r.snappedPos.y + size.y * 0.5f;

    // 2. 处理窗口间吸附
    float minDistance = snapDist;
    SnapState state = SNAP_NONE;
    ImVec2 windowSnapPos = r.snappedPos;
    bool isWindowNeedHide = false;
    if (GameStateDetector::Instance().IsNeedHide())
        isWindowNeedHide = true; // 隐藏所有窗口
    for (const auto& item : items) {
        if(!item->isEnabled) continue;
        if (auto ren = dynamic_cast<RenderModule*>(item))
        {
            if (!ren->IsRenderGui()) continue;
            if (auto otherWindow = dynamic_cast<WindowModule*>(ren))
            {
                if(isWindowNeedHide)
                    continue;
                // 跳过自己
                if(otherWindow->isMoving == true) continue;
                float otherRight = otherWindow->x + otherWindow->width;
                float otherBottom = otherWindow->y + otherWindow->height;
                float otherCx = otherWindow->x + otherWindow->width * 0.5f;
                float otherCy = otherWindow->y + otherWindow->height * 0.5f;

                // 检查边缘对齐吸附
                // 左边缘对齐其他窗口右边缘
                float distLeftToRight = fabs(r.snappedPos.x - otherRight);
                if (distLeftToRight < minDistance && distLeftToRight < snapDist) {
                    windowSnapPos.x = otherRight;
                    state = static_cast<SnapState>(state | SNAP_OTHER_LEFT);
                }

                // 右边缘对齐其他窗口左边缘
                float distRightToLeft = fabs(right - otherWindow->x);
                if (distRightToLeft < minDistance && distRightToLeft < snapDist) {
                    windowSnapPos.x = otherWindow->x - size.x;
                    state = static_cast<SnapState>(state | SNAP_OTHER_RIGHT);
                }

                // 上边缘对齐其他窗口下边缘
                float distTopToBottom = fabs(r.snappedPos.y - otherBottom);
                if (distTopToBottom < minDistance && distTopToBottom < snapDist) {
                    windowSnapPos.y = otherBottom;
                    state = static_cast<SnapState>(state | SNAP_OTHER_TOP);
                }

                // 下边缘对齐其他窗口上边缘
                float distBottomToTop = fabs(bottom - otherWindow->y);
                if (distBottomToTop < minDistance && distBottomToTop < snapDist) {
                    windowSnapPos.y = otherWindow->y - size.y;
                    state = static_cast<SnapState>(state | SNAP_OTHER_BOTTOM);
                }

                // 中心对齐（可选）
                float distCenterX = fabs(cx - otherCx);
                if (distCenterX < minDistance && distCenterX < snapDist) {
                    windowSnapPos.x = otherWindow->x + (otherWindow->width - size.x) * 0.5f;
                    state = static_cast<SnapState>(state | SNAP_OTHER_CENTER_X);
                }

                float distCenterY = fabs(cy - otherCy);
                if (distCenterY < minDistance && distCenterY < snapDist) {
                    windowSnapPos.y = otherWindow->y + (otherWindow->height - size.y) * 0.5f;
                    state = static_cast<SnapState>(state | SNAP_OTHER_CENTER_Y);
                }

            }

        }
        
    }

    r.snappedPos = windowSnapPos;
    r.snapState = static_cast<SnapState>(r.snapState | state);
}

void WindowSnapper::KeepSnapped(ImVec2& pos, const ImVec2& size, float screenW, float screenH, const SnapState& r)
{
    //保持边缘吸附
    if (r & SNAP_LEFT) {
        pos.x = 0;
    }
    if (r & SNAP_RIGHT) {
        pos.x = screenW - size.x;
    }
    if (r & SNAP_TOP) {
        pos.y = 0;
    }
    if (r & SNAP_BOTTOM) {
        pos.y = screenH - size.y;
    }

    //保持中心吸附
    if (r & SNAP_CENTER_X) {
        pos.x = screenW * 0.5f - size.x * 0.5f;
    }
    if (r & SNAP_CENTER_Y) {
        pos.y = screenH * 0.5f - size.y * 0.5f;
    }
}

void WindowSnapper::DrawGuides(const SnapResult& r, float screenW, float screenH, const ImVec2& size)
{
    auto draw = ImGui::GetForegroundDrawList();
    ImU32 color = IM_COL32(120, 180, 255, 150);
    float lineWidth = 3.0f;
    float lineWidth2 = 1.0f;
    if (GetAsyncKeyState(VK_CONTROL) & 0x8000)
    {
        if (r.snapState & SNAP_LEFT) {
            draw->AddLine(ImVec2(0, 0), ImVec2(0, screenH), color, lineWidth);
        }

        if (r.snapState & SNAP_RIGHT) {
            draw->AddLine(ImVec2(screenW, 0), ImVec2(screenW, screenH), color, lineWidth);
        }

        if (r.snapState & SNAP_TOP) {
            draw->AddLine(ImVec2(0, 0), ImVec2(screenW, 0), color, lineWidth);
        }

        if (r.snapState & SNAP_BOTTOM) {
            draw->AddLine(ImVec2(0, screenH), ImVec2(screenW, screenH), color, lineWidth);
        }

        if (r.snapState & SNAP_CENTER_X) {
            draw->AddLine(ImVec2(screenW * 0.5f, 0), ImVec2(screenW * 0.5f, screenH), color, lineWidth);
        }

        if (r.snapState & SNAP_CENTER_Y) {
            draw->AddLine(ImVec2(0, screenH * 0.5f), ImVec2(screenW, screenH * 0.5f), color, lineWidth);
        }
    }
    if(GetAsyncKeyState(VK_LSHIFT) & 0x8000)
    {
        if (r.snapState & SNAP_OTHER_LEFT) {
            draw->AddLine(ImVec2(r.snappedPos.x, 0), ImVec2(r.snappedPos.x, screenH), color, lineWidth2);
        }
        if (r.snapState & SNAP_OTHER_RIGHT) {
            draw->AddLine(ImVec2(r.snappedPos.x + size.x, 0), ImVec2(r.snappedPos.x + size.x, screenH), color, lineWidth2);
        }
        if (r.snapState & SNAP_OTHER_TOP) {
            draw->AddLine(ImVec2(0, r.snappedPos.y), ImVec2(screenW, r.snappedPos.y), color, lineWidth2);
        }
        if (r.snapState & SNAP_OTHER_BOTTOM) {
            draw->AddLine(ImVec2(0, r.snappedPos.y + size.y), ImVec2(screenW, r.snappedPos.y + size.y), color, lineWidth2);
        }
        if (r.snapState & SNAP_OTHER_CENTER_X) {
            draw->AddLine(ImVec2(r.snappedPos.x + size.x * 0.5f, 0), ImVec2(r.snappedPos.x + size.x * 0.5f, screenH), color, lineWidth2);
        }
        if (r.snapState & SNAP_OTHER_CENTER_Y) {
            draw->AddLine(ImVec2(0, r.snappedPos.y + size.y * 0.5f), ImVec2(screenW, r.snappedPos.y + size.y * 0.5f), color, lineWidth2);
        }
    }
}