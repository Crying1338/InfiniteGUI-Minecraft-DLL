#include "ArrayListModule.h"

#include "Anim.h"
#include "ItemManager.h"
#include "Menu.h"
#include "NotificationItem.h"
#include "GameStateDetector.h"
#include "opengl_hook.h"

#include <algorithm>

// ============================================================
// ArrayListModule · Drip 风格模块列表（默认紫色渐变）
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
	styleMode = Style_Rise;

	// 默认 Rise「MAGIC」紫色渐变 (#4A00E0 -> #8E2DE2)
	gradientStart = ImVec4(0.290f, 0.000f, 0.878f, 1.00f);
	gradientEnd = ImVec4(0.557f, 0.176f, 0.886f, 1.00f);
	useGradient = true;
	gradientAcrossList = true;
	gradientAnimated = false;
	gradientSpeed = 0.05f;
	rainbowAccent = false;

	textGradient = true;
	showSidebar = true;
	entryBackground = false;
	entryBgColor = ImVec4(0.078f, 0.055f, 0.118f, 0.43f);

	barWidth = 220.0f;
	barRounding = 0.0f;
	rowSpacing = 2.0f;

	{
		std::lock_guard<std::mutex> lock(entriesMutex);
		entries.clear();
	}
	lastSignature.clear();
	positionInitialized = false;

	itemStyle.fontSize = 20.0f;

	dirtyState.contentDirty = true;
	dirtyState.animating = true;
}

static bool IsArrayListSkippable(const Item* item, const Item* self)
{
	if (item->type == Hidden) return true;
	if (item == self) return true;
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
		if (IsArrayListSkippable(item, this)) continue;
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
	return gradientStart;
}

void ArrayListModule::Update()
{
	RebuildEntries();

	// 生成签名，检测可见集合变化（与排序无关）
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

	const bool riseStyle = (styleMode == Style_Rise);
	float fontSize = ImGui::GetFontSize();
	float barPaddingX = 10.0f;
	float barHeight = riseStyle ? (fontSize + 6.0f) : (fontSize + 8.0f);
	float barGap = rowSpacing;

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

		// 排序：Drip 风格按宽度（长的在上），或按名称
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

	const int rowCount = (int)layout.size();
	int row = 0;
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

			// ---- 本条目在整列渐变上的取色（Rise 的流动效果）----
			float t = 0.0f;
			if (gradientAcrossList && rowCount > 0)
				t = (float)row / (float)rowCount;
			if (gradientAnimated)
				t = fmodf(t + (float)ImGui::GetTime() * gradientSpeed, 1.0f);

			ImVec4 entryCol;
			if (rainbowAccent)
				entryCol = accent;
			else if (useGradient)
				entryCol = ImLerp(gradientStart, gradientEnd, t);
			else
				entryCol = gradientStart;

			if (riseStyle)
			{
				// ===== Rise Modern：渐变彩色文字 + 右侧细高亮条（+可选深色底）=====
				const float sidebarW = 2.0f;
				const float sidebarH = fontSize * 0.55f;
				const float sidebarGap = 3.0f;
				float rowRight = origin.x + barWidth;
				float textRight = rowRight - sidebarW - sidebarGap;
				float textX = textRight - l.width;
				float textY = y + (barHeight - fontSize) * 0.5f;

				if (entryBackground)
				{
					ImVec4 bg = entryBgColor;
					bg.w *= anim;
					drawList->AddRectFilled(ImVec2(textX - 4.0f, y), ImVec2(rowRight, y + barHeight),
						ImGui::GetColorU32(bg), 3.0f);
				}

				ImVec4 textCol = textGradient ? entryCol : ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
				textCol.w *= anim;
				drawList->AddText(ImVec2(textX + 1.0f, textY + 1.0f),
					ImGui::GetColorU32(ImVec4(0.0f, 0.0f, 0.0f, 0.6f * anim)), l.entry->name.c_str());
				drawList->AddText(ImVec2(textX, textY), ImGui::GetColorU32(textCol), l.entry->name.c_str());

				if (showSidebar)
				{
					ImVec4 sc = entryCol;
					sc.w *= anim;
					float sy = y + (barHeight - sidebarH) * 0.5f;
					drawList->AddRectFilled(ImVec2(rowRight - sidebarW, sy), ImVec2(rowRight, sy + sidebarH),
						ImGui::GetColorU32(sc), 1.0f);
				}
			}
			else
			{
				// ===== 渐变底条 + 白字 =====
				ImVec2 barMin(x0, y);
				ImVec2 barMax(x1, y + barHeight);

				ImVec4 ca(entryCol.x, entryCol.y, entryCol.z, entryCol.w * anim);
				ImVec4 cb = ca;
				if (useGradient && gradientAcrossList && rowCount > 0)
				{
					float t1 = (float)(row + 1) / (float)rowCount;
					if (gradientAnimated)
						t1 = fmodf(t1 + (float)ImGui::GetTime() * gradientSpeed, 1.0f);
					ImVec4 c2 = ImLerp(gradientStart, gradientEnd, t1);
					cb = ImVec4(c2.x, c2.y, c2.z, c2.w * anim);
				}

				if (ca.x == cb.x && ca.y == cb.y && ca.z == cb.z)
					drawList->AddRectFilled(barMin, barMax, ImGui::GetColorU32(ca), barRounding);
				else
					drawList->AddRectFilledMultiColor(barMin, barMax,
						ImGui::GetColorU32(ca), ImGui::GetColorU32(cb),
						ImGui::GetColorU32(cb), ImGui::GetColorU32(ca));

				float textX = alignRight ? (x1 - barPaddingX - l.width) : (x0 + barPaddingX);
				float textY = y + (barHeight - fontSize) * 0.5f;
				drawList->AddText(ImVec2(textX + 1.0f, textY + 1.0f),
					ImGui::GetColorU32(ImVec4(0.0f, 0.0f, 0.0f, 0.55f * anim)),
					l.entry->name.c_str());
				drawList->AddText(ImVec2(textX, textY),
					ImGui::GetColorU32(ImVec4(1.0f, 1.0f, 1.0f, anim)),
					l.entry->name.c_str());
			}
		}

		y += barHeight + barGap;
		row++;
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
	const char* styleNames[] = { u8"渐变底条（Drip）", u8"Rise Modern（彩色渐变文字 + 右侧高亮条）" };
	if (ImGui::Combo(u8"列表风格", &styleMode, styleNames, IM_ARRAYSIZE(styleNames)))
		dirtyState.contentDirty = true;

	// ---- 配色 ----
	ImGui::SetCursorPosX(bigPadding);
	ImGui::SetNextItemWidth(itemWidth);
	if (ImGui::Checkbox(u8"启用渐变", &useGradient)) dirtyState.contentDirty = true;
	ImGui::SameLine();
	ImGui::SetCursorPosX(bigPadding + centerX);
	ImGui::SetNextItemWidth(itemWidth);
	if (ImGui::Checkbox(u8"彩虹", &rainbowAccent)) dirtyState.contentDirty = true;

	if (useGradient && !rainbowAccent)
	{
		ImGui::SetCursorPosX(bigPadding);
		ImGui::SetNextItemWidth(itemWidth);
		if (ImGuiStd::EditColor(u8"渐变起色", gradientStart)) dirtyState.contentDirty = true;
		ImGui::SameLine();
		ImGui::SetCursorPosX(bigPadding + centerX);
		ImGui::SetNextItemWidth(itemWidth);
		if (ImGuiStd::EditColor(u8"渐变止色", gradientEnd)) dirtyState.contentDirty = true;

		ImGui::SetCursorPosX(bigPadding);
		ImGui::SetNextItemWidth(itemWidth);
		if (ImGui::Checkbox(u8"整列渐变", &gradientAcrossList))
			dirtyState.contentDirty = true;
		ImGui::SameLine();
		ImGui::SetCursorPosX(bigPadding + centerX);
		ImGui::SetNextItemWidth(itemWidth);
		if (ImGui::Checkbox(u8"渐变流动", &gradientAnimated))
			dirtyState.contentDirty = true;
	}
	else if (!rainbowAccent)
	{
		ImGui::SetCursorPosX(bigPadding);
		ImGui::SetNextItemWidth(itemWidth);
		if (ImGuiStd::EditColor(u8"底条颜色", gradientStart)) dirtyState.contentDirty = true;
	}

	// ---- Rise 风格专属 ----
	if (styleMode == Style_Rise)
	{
		ImGui::SetCursorPosX(bigPadding);
		ImGui::SetNextItemWidth(itemWidth);
		if (ImGui::Checkbox(u8"彩色渐变文字", &textGradient)) dirtyState.contentDirty = true;
		ImGui::SameLine();
		ImGui::SetCursorPosX(bigPadding + centerX);
		ImGui::SetNextItemWidth(itemWidth);
		if (ImGui::Checkbox(u8"右侧高亮条", &showSidebar)) dirtyState.contentDirty = true;

		ImGui::SetCursorPosX(bigPadding);
		ImGui::SetNextItemWidth(itemWidth);
		if (ImGui::Checkbox(u8"每条深色底", &entryBackground)) dirtyState.contentDirty = true;
		if (entryBackground)
		{
			ImGui::SameLine();
			ImGui::SetCursorPosX(bigPadding + centerX);
			ImGui::SetNextItemWidth(itemWidth);
			if (ImGuiStd::EditColor(u8"底色", entryBgColor)) dirtyState.contentDirty = true;
		}
	}

	ImGui::SetCursorPosX(bigPadding);
	ImGui::SetNextItemWidth(bigItemWidth);
	if (ImGui::SliderFloat(u8"列表宽度", &barWidth, 80.0f, 400.0f, "%.0f"))
		dirtyState.contentDirty = true;

	ImGui::SetCursorPosX(bigPadding);
	ImGui::SetNextItemWidth(itemWidth);
	if (ImGui::SliderFloat(u8"条目间距", &rowSpacing, 0.0f, 12.0f, "%.1f"))
		dirtyState.contentDirty = true;
	ImGui::SameLine();
	ImGui::SetCursorPosX(bigPadding + centerX);
	ImGui::SetNextItemWidth(itemWidth);
	if (styleMode == Style_Bars)
	{
		if (ImGui::SliderFloat(u8"圆角", &barRounding, 0.0f, 8.0f, "%.1f"))
			dirtyState.contentDirty = true;
	}

	ImGui::PushFont(NULL, ImGui::GetFontSize() * 0.8f);
	ImGui::SetCursorPosX(bigPadding);
	ImGuiStd::TextShadow(u8"提示：拖动窗口移动位置，按住 Ctrl/Shift 拖动可吸附屏幕边缘；");
	ImGui::SetCursorPosX(bigPadding);
	ImGuiStd::TextShadow(u8"字体大小与颜色可在下方“窗口设置”里调整。");
	ImGui::PopFont();

	DrawWindowSettings(bigPadding, centerX, itemWidth);
}

void ArrayListModule::Load(const nlohmann::json& j)
{
	LoadItem(j);
	LoadWindow(j);
	if (j.contains("listSide")) listSide = j["listSide"];
	if (j.contains("sortMode")) sortMode = j["sortMode"];
	if (j.contains("styleMode")) styleMode = j["styleMode"];
	if (j.contains("useGradient")) useGradient = j["useGradient"];
	if (j.contains("gradientAcrossList")) gradientAcrossList = j["gradientAcrossList"];
	if (j.contains("gradientAnimated")) gradientAnimated = j["gradientAnimated"];
	if (j.contains("gradientSpeed")) gradientSpeed = j["gradientSpeed"];
	if (j.contains("rainbowAccent")) rainbowAccent = j["rainbowAccent"];
	if (j.contains("textGradient")) textGradient = j["textGradient"];
	if (j.contains("showSidebar")) showSidebar = j["showSidebar"];
	if (j.contains("entryBackground")) entryBackground = j["entryBackground"];
	if (j.contains("barWidth")) barWidth = j["barWidth"];
	if (j.contains("barRounding")) barRounding = j["barRounding"];
	if (j.contains("rowSpacing")) rowSpacing = j["rowSpacing"];
	ImGuiStd::LoadImVec4(j, "gradientStart", gradientStart);
	ImGuiStd::LoadImVec4(j, "gradientEnd", gradientEnd);
	ImGuiStd::LoadImVec4(j, "entryBgColor", entryBgColor);

	// 兼容旧配置：只有 accentColor 时迁移到渐变色
	if (!j.contains("gradientStart") && j.contains("accentColor"))
	{
		ImGuiStd::LoadImVec4(j, "accentColor", gradientStart);
		gradientEnd = gradientStart;
	}
	dirtyState.contentDirty = true;
}

void ArrayListModule::Save(nlohmann::json& j) const
{
	SaveItem(j);
	SaveWindow(j);
	j["listSide"] = listSide;
	j["sortMode"] = sortMode;
	j["styleMode"] = styleMode;
	j["useGradient"] = useGradient;
	j["gradientAcrossList"] = gradientAcrossList;
	j["gradientAnimated"] = gradientAnimated;
	j["gradientSpeed"] = gradientSpeed;
	j["rainbowAccent"] = rainbowAccent;
	j["textGradient"] = textGradient;
	j["showSidebar"] = showSidebar;
	j["entryBackground"] = entryBackground;
	j["barWidth"] = barWidth;
	j["barRounding"] = barRounding;
	j["rowSpacing"] = rowSpacing;
	ImGuiStd::SaveImVec4(j, "gradientStart", gradientStart);
	ImGuiStd::SaveImVec4(j, "gradientEnd", gradientEnd);
	ImGuiStd::SaveImVec4(j, "entryBgColor", entryBgColor);
}
