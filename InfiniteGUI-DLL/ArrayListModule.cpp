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

	// 默认紫色渐变
	gradientStart = ImVec4(0.36f, 0.09f, 0.92f, 0.90f);
	gradientEnd = ImVec4(0.78f, 0.38f, 1.00f, 0.90f);
	useGradient = true;
	gradientAcrossList = false;
	rainbowAccent = false;

	barWidth = 220.0f;
	barRounding = 0.0f;

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

	float fontSize = ImGui::GetFontSize();
	float barPaddingX = 10.0f;
	float barHeight = fontSize + 8.0f;
	float barGap = 3.0f;

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

			ImVec2 barMin(x0, y);
			ImVec2 barMax(x1, y + barHeight);

			// ---- 底条配色 ----
			ImVec4 colA, colB;
			if (rainbowAccent)
			{
				colA = colB = accent;
			}
			else if (useGradient)
			{
				if (gradientAcrossList && rowCount > 0)
				{
					float t0 = (float)row / (float)rowCount;
					float t1 = (float)(row + 1) / (float)rowCount;
					colA = ImLerp(gradientStart, gradientEnd, t0);
					colB = ImLerp(gradientStart, gradientEnd, t1);
				}
				else
				{
					colA = gradientStart;
					colB = gradientEnd;
				}
			}
			else
			{
				colA = gradientStart;
				colB = gradientEnd;
			}

			ImVec4 ca(colA.x, colA.y, colA.z, colA.w * anim);
			ImVec4 cb(colB.x, colB.y, colB.z, colB.w * anim);

			if (ca.x == cb.x && ca.y == cb.y && ca.z == cb.z)
			{
				// 纯色条
				drawList->AddRectFilled(barMin, barMax, ImGui::GetColorU32(ca), barRounding);
			}
			else
			{
				// 水平渐变条（左上 -> 右上 -> 右下 -> 左下）
				drawList->AddRectFilledMultiColor(barMin, barMax,
					ImGui::GetColorU32(ca), ImGui::GetColorU32(cb),
					ImGui::GetColorU32(cb), ImGui::GetColorU32(ca));
			}

			// ---- 文字（白色 + 阴影）----
			float textX = alignRight ? (x1 - barPaddingX - l.width) : (x0 + barPaddingX);
			float textY = y + (barHeight - fontSize) * 0.5f;
			drawList->AddText(ImVec2(textX + 1.0f, textY + 1.0f),
				ImGui::GetColorU32(ImVec4(0.0f, 0.0f, 0.0f, 0.55f * anim)),
				l.entry->name.c_str());
			drawList->AddText(ImVec2(textX, textY),
				ImGui::GetColorU32(ImVec4(1.0f, 1.0f, 1.0f, anim)),
				l.entry->name.c_str());
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

	// ---- 配色 ----
	ImGui::SetCursorPosX(bigPadding);
	ImGui::SetNextItemWidth(itemWidth);
	if (ImGui::Checkbox(u8"渐变底条", &useGradient)) dirtyState.contentDirty = true;
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
		ImGui::SetNextItemWidth(bigItemWidth);
		if (ImGui::Checkbox(u8"整列渐变（每根条取整段渐变的一段）", &gradientAcrossList))
			dirtyState.contentDirty = true;
	}
	else if (!rainbowAccent)
	{
		ImGui::SetCursorPosX(bigPadding);
		ImGui::SetNextItemWidth(itemWidth);
		if (ImGuiStd::EditColor(u8"底条颜色", gradientStart)) dirtyState.contentDirty = true;
	}

	ImGui::SetCursorPosX(bigPadding);
	ImGui::SetNextItemWidth(bigItemWidth);
	if (ImGui::SliderFloat(u8"条形宽度", &barWidth, 100.0f, 400.0f, "%.0f"))
		dirtyState.contentDirty = true;

	ImGui::SetCursorPosX(bigPadding);
	ImGui::SetNextItemWidth(bigItemWidth);
	if (ImGui::SliderFloat(u8"圆角", &barRounding, 0.0f, 8.0f, "%.1f"))
		dirtyState.contentDirty = true;

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
	if (j.contains("useGradient")) useGradient = j["useGradient"];
	if (j.contains("gradientAcrossList")) gradientAcrossList = j["gradientAcrossList"];
	if (j.contains("rainbowAccent")) rainbowAccent = j["rainbowAccent"];
	if (j.contains("barWidth")) barWidth = j["barWidth"];
	if (j.contains("barRounding")) barRounding = j["barRounding"];
	ImGuiStd::LoadImVec4(j, "gradientStart", gradientStart);
	ImGuiStd::LoadImVec4(j, "gradientEnd", gradientEnd);

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
	j["useGradient"] = useGradient;
	j["gradientAcrossList"] = gradientAcrossList;
	j["rainbowAccent"] = rainbowAccent;
	j["barWidth"] = barWidth;
	j["barRounding"] = barRounding;
	ImGuiStd::SaveImVec4(j, "gradientStart", gradientStart);
	ImGuiStd::SaveImVec4(j, "gradientEnd", gradientEnd);
}
