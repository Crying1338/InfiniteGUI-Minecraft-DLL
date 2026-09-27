#include "ArrayListModule.h"

#include "Anim.h"
#include "ItemManager.h"
#include "Menu.h"
#include "NotificationItem.h"
#include "GameStateDetector.h"
#include "opengl_hook.h"

#include <algorithm>

// ============================================================
// ArrayListModule · Drip Lite 风格模块列表
// ============================================================

void ArrayListModule::Toggle()
{
}

void ArrayListModule::Reset()
{
	ResetWindow();
	isEnabled = false;

	listSide = Side_Right;
	sortMode = Sort_Width;
	accentColor = ImVec4(0.92f, 0.20f, 0.32f, 1.0f); // Drip 红
	gradientAccent = true;
	rainbowAccent = false;
	barWidth = 220.0f;

	{
		std::lock_guard<std::mutex> lock(entriesMutex);
		entries.clear();
	}
	lastSignature.clear();
	positionInitialized = false;

	itemStyle.bgColor = ImVec4(0.02f, 0.02f, 0.04f, 0.55f);
	itemStyle.fontSize = 20.0f;

	dirtyState.contentDirty = true;
	dirtyState.animating = true;
}

bool IsArrayListSkippable(const Item* item)
{
	if (item->type == Hidden) return true;
	if (item == static_cast<const Item*>(&ArrayListModule::Instance())) return true;
	if (item == static_cast<const Item*>(&Menu::Instance())) return true;             // 菜单本身
	if (item == static_cast<const Item*>(&NotificationItem::Instance())) return true; // 提示弹窗容器
	return false;
}

void ArrayListModule::RebuildEntries()
{
	std::lock_guard<std::mutex> lock(entriesMutex);

	// 标记当前可见项
	for (auto& entry : entries)
		entry.visible = false;

	for (Item* item : ItemManager::Instance().GetItems())
	{
		if (IsArrayListSkippable(item)) continue;
		if (!item->isEnabled) continue;

		bool found = false;
		for (auto& entry : entries)
		{
			if (entry.item == item)
			{
				entry.visible = true;
				found = true;
				break;
			}
		}
		if (!found)
		{
			Entry entry;
			entry.item = item;
			entry.name = item->name;
			entry.anim = 0.0f;
			entry.visible = true;
			entries.push_back(entry);
		}
	}

	// 移除已完全淡出的项
	entries.erase(
		std::remove_if(entries.begin(), entries.end(),
			[](const Entry& e) { return !e.visible && e.anim <= 0.001f; }),
		entries.end());
}

ImVec4 ArrayListModule::GetAccentColor() const
{
	if (rainbowAccent)
		return ImColor::HSV(fmodf((float)ImGui::GetTime() * 0.2f, 1.0f), 1.0f, 1.0f, 1.0f);
	return accentColor;
}

void ArrayListModule::Update()
{
	RebuildEntries();

	// 生成签名，检测变化（与排序无关，排序变化不触发脏标记）
	std::string signature;
	{
		std::lock_guard<std::mutex> lock(entriesMutex);
		std::vector<std::string> names;
		for (const auto& entry : entries)
			if (entry.visible)
				names.push_back(entry.name);
		std::sort(names.begin(), names.end());
		for (const auto& n : names)
		{
			signature += n;
			signature += "|";
		}
	}

	if (signature != lastSignature)
	{
		lastSignature = signature;
		dirtyState.contentDirty = true;
	}
}

void ArrayListModule::RenderGui()
{
	if (!positionInitialized)
	{
		// 默认停靠到屏幕右上角
		float screenW = (float)opengl_hook::screen_size.x;
		x = screenW - barWidth - 30.0f;
		y = 40.0f;
		positionInitialized = true;
	}

	bool anyToDraw = false;
	{
		std::lock_guard<std::mutex> lock(entriesMutex);
		anyToDraw = !entries.empty();
	}
	if (!anyToDraw)
		return; // 无内容时不渲染窗口（淡出动画期间仍有条目）

	WindowModule::RenderGui();
}

void ArrayListModule::HoverSetting()
{
}

void ArrayListModule::DrawContent()
{
	if (closed)
	{
		isEnabled = false;
		closed = false;
	}

	ImGuiIO& io = ImGui::GetIO();
	float dt = std::clamp(io.DeltaTime, 0.0f, 0.05f);

	float fontSize = ImGui::GetFontSize();
	float barPaddingX = 8.0f;
	float barHeight = fontSize + 6.0f;
	float barGap = 2.0f;

	struct LayoutEntry
	{
		const Entry* entry;
		float width;
	};

	std::vector<LayoutEntry> layout;
	{
		std::lock_guard<std::mutex> lock(entriesMutex);
		for (auto& entry : entries)
		{
			// 推进动画
			float target = entry.visible ? 1.0f : 0.0f;
			Anim::SmoothLerp(entry.anim, target, 12.0f, dt);
		}

		for (const auto& entry : entries)
			layout.push_back({ &entry, ImGui::CalcTextSize(entry.name.c_str()).x });

		// 排序：Drip 风格按宽度（长的在上），或按字母
		if (sortMode == Sort_Width)
		{
			std::stable_sort(layout.begin(), layout.end(),
				[](const LayoutEntry& a, const LayoutEntry& b) { return a.width > b.width; });
		}
		else
		{
			std::stable_sort(layout.begin(), layout.end(),
				[](const LayoutEntry& a, const LayoutEntry& b) { return a.entry->name < b.entry->name; });
		}
	}

	// 动画是否进行中
	bool animating = false;
	for (const auto& l : layout)
	{
		if (!Anim::AlmostEqual(l.entry->anim, l.entry->visible ? 1.0f : 0.0f, 0.005f))
			animating = true;
	}
	if (animating || rainbowAccent)
		dirtyState.animating = true;
	else
		dirtyState.animating = false;

	if (layout.empty())
		return;

	float totalHeight = (float)layout.size() * (barHeight + barGap) - barGap;

	// 用 Dummy 撑起窗口大小（窗口自动适应内容）
	ImGui::Dummy(ImVec2(barWidth, totalHeight));

	ImDrawList* drawList = ImGui::GetWindowDrawList();
	ImVec2 windowPos = ImGui::GetWindowPos();
	// 内容区起点（窗口内边距之后），避免 Dummy 尺寸与绘制区域错位
	ImVec2 origin(windowPos.x + ImGui::GetStyle().WindowPadding.x,
		windowPos.y + ImGui::GetStyle().WindowPadding.y);
	bool alignRight = (listSide == Side_Right);
	ImVec4 accent = GetAccentColor();

	float y = origin.y;
	for (const auto& l : layout)
	{
		float anim = l.entry->anim;
		if (anim > 0.005f)
		{
			// 从边缘滑入
			float slideOffset = (1.0f - anim) * barWidth;
			float x0 = alignRight
				? origin.x + (barWidth - l.width) + slideOffset
				: origin.x - slideOffset;
			float x1 = x0 + l.width;

			ImVec2 barMin(x0, y);
			ImVec2 barMax(x1, y + barHeight);

			// 深色半透明背景
			ImU32 bgColor = ImGui::GetColorU32(ImVec4(0.02f, 0.02f, 0.04f, 0.62f * anim));
			drawList->AddRectFilled(barMin, barMax, bgColor, 0.0f);

			// 彩色强调条（贴外侧边缘）
			ImU32 accentU32 = ImGui::GetColorU32(ImVec4(accent.x, accent.y, accent.z, accent.w * anim));
			float stripW = 3.0f;
			if (gradientAccent)
			{
				ImVec4 accentDark(accent.x * 0.45f, accent.y * 0.45f, accent.z * 0.45f, accent.w);
				ImU32 accentDarkU32 = ImGui::GetColorU32(ImVec4(accentDark.x, accentDark.y, accentDark.z, accent.w * anim));
				ImVec2 sMin = alignRight ? ImVec2(x1 - stripW, y) : ImVec2(x0, y);
				ImVec2 sMax = alignRight ? ImVec2(x1, y + barHeight) : ImVec2(x0 + stripW, y + barHeight);
				if (alignRight)
					drawList->AddRectFilledMultiColor(sMin, sMax, accentU32, accentU32, accentDarkU32, accentDarkU32);
				else
					drawList->AddRectFilledMultiColor(sMin, sMax, accentDarkU32, accentDarkU32, accentU32, accentU32);
			}
			else
			{
				ImVec2 sMin = alignRight ? ImVec2(x1 - stripW, y) : ImVec2(x0, y);
				ImVec2 sMax = alignRight ? ImVec2(x1, y + barHeight) : ImVec2(x0 + stripW, y + barHeight);
				drawList->AddRectFilled(sMin, sMax, accentU32, 0.0f);
			}

			// 文字（带阴影）
			float textX = alignRight ? (x1 - stripW - barPaddingX - l.width) : (x0 + stripW + barPaddingX);
			float textY = y + (barHeight - fontSize) * 0.5f;
			ImVec2 textPos(textX, textY);
			drawList->AddText(ImVec2(textPos.x + 1.0f, textPos.y + 1.0f),
				ImGui::GetColorU32(ImVec4(0.0f, 0.0f, 0.0f, 0.85f * anim)),
				l.entry->name.c_str());
			drawList->AddText(textPos,
				ImGui::GetColorU32(ImVec4(1.0f, 1.0f, 1.0f, anim)),
				l.entry->name.c_str());
		}

		y += barHeight + barGap;
	}
}

void ArrayListModule::DrawSettings(const float& bigPadding, const float& centerX, const float& itemWidth)
{
	float bigItemWidth = centerX * 2.0f - bigPadding * 4.0f;

	ImGui::PushFont(NULL, ImGui::GetFontSize() * 0.8f);
	ImGui::BeginDisabled();
	ImGuiStd::TextShadow(u8"ArrayList 设置");
	ImGui::EndDisabled();
	ImGui::PopFont();

	ImGui::SetCursorPosX(bigPadding);
	ImGui::SetNextItemWidth(itemWidth);
	const char* sideNames[] = { u8"右侧贴边", u8"左侧贴边" };
	ImGui::Combo(u8"列表位置", &listSide, sideNames, IM_ARRAYSIZE(sideNames));
	ImGui::SameLine();
	ImGui::SetCursorPosX(bigPadding + centerX);
	ImGui::SetNextItemWidth(itemWidth);
	const char* sortNames[] = { u8"按宽度排序", u8"按名称排序" };
	ImGui::Combo(u8"排序方式", &sortMode, sortNames, IM_ARRAYSIZE(sortNames));

	ImGui::SetCursorPosX(bigPadding);
	ImGui::SetNextItemWidth(bigItemWidth);
	if (ImGui::SliderFloat(u8"条形宽度", &barWidth, 100.0f, 400.0f, "%.0f"))
		dirtyState.contentDirty = true;

	ImGui::SetCursorPosX(bigPadding);
	ImGui::SetNextItemWidth(itemWidth);
	if (ImGuiStd::EditColor(u8"强调颜色", accentColor))
		dirtyState.contentDirty = true;
	ImGui::SameLine();
	if (ImGui::Checkbox(u8"彩虹", &rainbowAccent))
		dirtyState.contentDirty = true;
	ImGui::SameLine();
	ImGui::SetCursorPosX(bigPadding + centerX);
	ImGui::SetNextItemWidth(itemWidth);
	if (ImGui::Checkbox(u8"渐变强调条", &gradientAccent))
		dirtyState.contentDirty = true;

	ImGui::PushFont(opengl_hook::gui.iconFont);
	ImGuiStd::HelpMarker(u8"ArrayList 会显示所有已开启的模块（拖动窗口可以调整位置，按住 Ctrl/Shift 拖动可吸附到屏幕边缘）");
	ImGui::PopFont();

	DrawWindowSettings(bigPadding, centerX, itemWidth);
}

void ArrayListModule::Load(const nlohmann::json& j)
{
	LoadItem(j);
	LoadWindow(j);
	if (j.contains("listSide")) listSide = j["listSide"];
	if (j.contains("sortMode")) sortMode = j["sortMode"];
	if (j.contains("gradientAccent")) gradientAccent = j["gradientAccent"];
	if (j.contains("rainbowAccent")) rainbowAccent = j["rainbowAccent"];
	if (j.contains("barWidth")) barWidth = j["barWidth"];
	ImGuiStd::LoadImVec4(j, "accentColor", accentColor);
}

void ArrayListModule::Save(nlohmann::json& j) const
{
	SaveItem(j);
	SaveWindow(j);
	j["listSide"] = listSide;
	j["sortMode"] = sortMode;
	j["gradientAccent"] = gradientAccent;
	j["rainbowAccent"] = rainbowAccent;
	j["barWidth"] = barWidth;
	ImGuiStd::SaveImVec4(j, "accentColor", accentColor);
}
