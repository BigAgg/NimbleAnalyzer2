#include "themes.h"

#include "imgui.h"

void SetTheme(Themes theme) {
	switch (theme) {
	case Themes::LIGHT:
		ImGui::StyleColorsLight();
		break;
	case Themes::DARK:
		ImGui::StyleColorsDark();
		break;
	case Themes::GOLD_DARK:
		ThemeGoldDark();
		break;
	case Themes::PURPLE_LIGHT:
		ThemePurpleLight();
		break;
	case Themes::PURPLE_DARK:
		ThemePurpleDark();
		break;
	case Themes::GOLD_LIGHT:
	default:
		ThemeGoldLight();
		break;
	}
}

void SetTheme(unsigned int theme) {
	SetTheme((Themes)theme);
}

void ThemeGoldDark() {
	ImGui::StyleColorsDark();
  ImGuiStyle& style = ImGui::GetStyle();
  ImVec4* colors = style.Colors;

  // --- Base palette ---
  ImVec4 bg = ImVec4(0.06f, 0.06f, 0.07f, 1.00f);  // Main background
  ImVec4 bgDark = ImVec4(0.04f, 0.04f, 0.05f, 1.00f);
  ImVec4 bgLight = ImVec4(0.12f, 0.12f, 0.13f, 1.00f);

  ImVec4 text = ImVec4(0.95f, 0.95f, 0.95f, 1.00f);
  ImVec4 textDisabled = ImVec4(0.55f, 0.55f, 0.55f, 1.00f);

  // Yellow-gold accent (base / hover / active)
  ImVec4 goldBase = ImVec4(0.95f, 0.76f, 0.06f, 1.00f);  // ~#F1C40F
  ImVec4 goldHover = ImVec4(0.99f, 0.82f, 0.20f, 1.00f);
  ImVec4 goldActive = ImVec4(0.89f, 0.67f, 0.00f, 1.00f);

  // Text
  colors[ImGuiCol_Text] = text;
  colors[ImGuiCol_TextDisabled] = textDisabled;

  // Backgrounds
  colors[ImGuiCol_WindowBg] = bg;
  colors[ImGuiCol_ChildBg] = bgDark;
  colors[ImGuiCol_PopupBg] = ImVec4(0.08f, 0.08f, 0.09f, 0.98f);

  // Borders
  colors[ImGuiCol_Border] = ImVec4(0.35f, 0.30f, 0.10f, 0.70f); // subtle gold tint
  colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);

  // Frames (input boxes, combo, etc.)
  colors[ImGuiCol_FrameBg] = ImVec4(0.12f, 0.12f, 0.13f, 1.00f);
  colors[ImGuiCol_FrameBgHovered] = ImVec4(0.18f, 0.18f, 0.20f, 1.00f);
  colors[ImGuiCol_FrameBgActive] = ImVec4(0.22f, 0.22f, 0.24f, 1.00f);

  // Title bars
  colors[ImGuiCol_TitleBg] = bgDark;
  colors[ImGuiCol_TitleBgActive] = ImVec4(0.10f, 0.10f, 0.12f, 1.00f);
  colors[ImGuiCol_TitleBgCollapsed] = bgDark;

  // Menus & bars
  colors[ImGuiCol_MenuBarBg] = bgDark;
  colors[ImGuiCol_ScrollbarBg] = bgDark;
  colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.30f, 0.30f, 0.32f, 1.00f);
  colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.38f, 0.38f, 0.40f, 1.00f);
  colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.46f, 0.46f, 0.48f, 1.00f);

  // Checkboxes, radio buttons
  colors[ImGuiCol_CheckMark] = goldBase;

  // Sliders
  colors[ImGuiCol_SliderGrab] = goldBase;
  colors[ImGuiCol_SliderGrabActive] = goldActive;

  // Buttons
  colors[ImGuiCol_Button] = ImVec4(goldBase.x, goldBase.y, goldBase.z, 0.80f);
  colors[ImGuiCol_ButtonHovered] = ImVec4(goldHover.x, goldHover.y, goldHover.z, 0.90f);
  colors[ImGuiCol_ButtonActive] = ImVec4(goldActive.x, goldActive.y, goldActive.z, 1.00f);

  // Headers (tree nodes, collapsing headers, selectable)
  colors[ImGuiCol_Header] = ImVec4(goldBase.x, goldBase.y, goldBase.z, 0.55f);
  colors[ImGuiCol_HeaderHovered] = ImVec4(goldHover.x, goldHover.y, goldHover.z, 0.80f);
  colors[ImGuiCol_HeaderActive] = ImVec4(goldActive.x, goldActive.y, goldActive.z, 0.90f);

  // Separator
  colors[ImGuiCol_Separator] = ImVec4(0.45f, 0.36f, 0.10f, 0.80f);
  colors[ImGuiCol_SeparatorHovered] = ImVec4(goldHover.x, goldHover.y, goldHover.z, 0.78f);
  colors[ImGuiCol_SeparatorActive] = ImVec4(goldActive.x, goldActive.y, goldActive.z, 1.00f);

  // Tabs
  colors[ImGuiCol_Tab] = ImVec4(0.12f, 0.12f, 0.13f, 1.00f);
  colors[ImGuiCol_TabHovered] = ImVec4(goldHover.x, goldHover.y, goldHover.z, 0.85f);
  colors[ImGuiCol_TabActive] = ImVec4(goldBase.x, goldBase.y, goldBase.z, 0.95f);
  colors[ImGuiCol_TabUnfocused] = ImVec4(0.08f, 0.08f, 0.09f, 1.00f);
  colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.10f, 0.10f, 0.11f, 1.00f);

  // Resize grips
  colors[ImGuiCol_ResizeGrip] = ImVec4(goldBase.x, goldBase.y, goldBase.z, 0.20f);
  colors[ImGuiCol_ResizeGripHovered] = ImVec4(goldHover.x, goldHover.y, goldHover.z, 0.70f);
  colors[ImGuiCol_ResizeGripActive] = ImVec4(goldActive.x, goldActive.y, goldActive.z, 0.95f);

  // Selection
  colors[ImGuiCol_TextSelectedBg] = ImVec4(goldBase.x, goldBase.y, goldBase.z, 0.35f);

  // Misc
  colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.70f);
  colors[ImGuiCol_DragDropTarget] = goldHover;
  colors[ImGuiCol_NavHighlight] = goldBase;

  // --- Styling tweaks ---
  style.WindowRounding = 6.0f;
  style.FrameRounding = 4.0f;
  style.PopupRounding = 4.0f;
  style.ScrollbarRounding = 6.0f;
  style.GrabRounding = 4.0f;
  style.TabRounding = 4.0f;

  style.WindowBorderSize = 1.0f;
  style.FrameBorderSize = 1.0f;
}

void ThemeGoldLight() {
  ImGui::StyleColorsLight();
  ImGuiStyle& style = ImGui::GetStyle();
  ImVec4* colors = style.Colors;

  // --- Base palette (light) ---
  ImVec4 bg = ImVec4(0.96f, 0.97f, 0.98f, 1.00f);  // Window background
  ImVec4 bgAlt = ImVec4(0.92f, 0.93f, 0.95f, 1.00f);  // Child/menu
  ImVec4 bgFrame = ImVec4(0.88f, 0.89f, 0.91f, 1.00f);  // Frames / inputs

  ImVec4 text = ImVec4(0.10f, 0.10f, 0.12f, 1.00f);
  ImVec4 textDisabled = ImVec4(0.55f, 0.55f, 0.60f, 1.00f);

  // Same yellow-gold accent family as the dark theme
  ImVec4 goldBase = ImVec4(0.95f, 0.76f, 0.06f, 1.00f);  // ~#F1C40F
  ImVec4 goldHover = ImVec4(0.99f, 0.82f, 0.20f, 1.00f);
  ImVec4 goldActive = ImVec4(0.89f, 0.67f, 0.00f, 1.00f);

  // Text
  colors[ImGuiCol_Text] = text;
  colors[ImGuiCol_TextDisabled] = textDisabled;

  // Backgrounds
  colors[ImGuiCol_WindowBg] = bg;
  colors[ImGuiCol_ChildBg] = bgAlt;
  colors[ImGuiCol_PopupBg] = ImVec4(0.98f, 0.98f, 0.99f, 0.98f);

  // Borders
  colors[ImGuiCol_Border] = ImVec4(0.75f, 0.70f, 0.45f, 0.80f); // subtle warm tint
  colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);

  // Frames (input boxes, combo, etc.)
  colors[ImGuiCol_FrameBg] = bgFrame;
  colors[ImGuiCol_FrameBgHovered] = ImVec4(0.94f, 0.94f, 0.95f, 1.00f);
  colors[ImGuiCol_FrameBgActive] = ImVec4(0.91f, 0.91f, 0.92f, 1.00f);

  // Title bars
  colors[ImGuiCol_TitleBg] = bgAlt;
  colors[ImGuiCol_TitleBgActive] = ImVec4(0.98f, 0.98f, 0.99f, 1.00f);
  colors[ImGuiCol_TitleBgCollapsed] = bgAlt;

  // Menus & bars
  colors[ImGuiCol_MenuBarBg] = bgAlt;
  colors[ImGuiCol_ScrollbarBg] = bgAlt;
  colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.72f, 0.72f, 0.74f, 1.00f);
  colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.64f, 0.64f, 0.66f, 1.00f);
  colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.56f, 0.56f, 0.58f, 1.00f);

  // Checkboxes, radio buttons
  colors[ImGuiCol_CheckMark] = goldActive;

  // Sliders
  colors[ImGuiCol_SliderGrab] = goldBase;
  colors[ImGuiCol_SliderGrabActive] = goldActive;

  // Buttons (gold accent pops nicely on light bg)
  colors[ImGuiCol_Button] = ImVec4(goldBase.x, goldBase.y, goldBase.z, 0.85f);
  colors[ImGuiCol_ButtonHovered] = ImVec4(goldHover.x, goldHover.y, goldHover.z, 0.95f);
  colors[ImGuiCol_ButtonActive] = ImVec4(goldActive.x, goldActive.y, goldActive.z, 1.00f);

  // Headers (tree nodes, collapsing headers, selectable)
  colors[ImGuiCol_Header] = ImVec4(goldBase.x, goldBase.y, goldBase.z, 0.55f);
  colors[ImGuiCol_HeaderHovered] = ImVec4(goldHover.x, goldHover.y, goldHover.z, 0.80f);
  colors[ImGuiCol_HeaderActive] = ImVec4(goldActive.x, goldActive.y, goldActive.z, 0.90f);

  // Separator
  colors[ImGuiCol_Separator] = ImVec4(0.80f, 0.76f, 0.50f, 0.80f);
  colors[ImGuiCol_SeparatorHovered] = ImVec4(goldHover.x, goldHover.y, goldHover.z, 0.90f);
  colors[ImGuiCol_SeparatorActive] = ImVec4(goldActive.x, goldActive.y, goldActive.z, 1.00f);

  // Tabs
  colors[ImGuiCol_Tab] = ImVec4(0.92f, 0.93f, 0.95f, 1.00f);
  colors[ImGuiCol_TabHovered] = ImVec4(goldHover.x, goldHover.y, goldHover.z, 0.90f);
  colors[ImGuiCol_TabActive] = ImVec4(goldBase.x, goldBase.y, goldBase.z, 0.98f);
  colors[ImGuiCol_TabUnfocused] = ImVec4(0.94f, 0.95f, 0.96f, 1.00f);
  colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.96f, 0.96f, 0.97f, 1.00f);

  // Resize grips
  colors[ImGuiCol_ResizeGrip] = ImVec4(goldBase.x, goldBase.y, goldBase.z, 0.30f);
  colors[ImGuiCol_ResizeGripHovered] = ImVec4(goldHover.x, goldHover.y, goldHover.z, 0.85f);
  colors[ImGuiCol_ResizeGripActive] = ImVec4(goldActive.x, goldActive.y, goldActive.z, 1.00f);

  // Selection & navigation
  colors[ImGuiCol_TextSelectedBg] = ImVec4(goldBase.x, goldBase.y, goldBase.z, 0.35f);
  colors[ImGuiCol_DragDropTarget] = goldHover;
  colors[ImGuiCol_NavHighlight] = goldBase;

  // Modals
  colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.10f, 0.10f, 0.10f, 0.35f);

  // --- Styling tweaks (same “shape” as dark theme) ---
  style.WindowRounding = 6.0f;
  style.FrameRounding = 4.0f;
  style.PopupRounding = 4.0f;
  style.ScrollbarRounding = 6.0f;
  style.GrabRounding = 4.0f;
  style.TabRounding = 4.0f;

  style.WindowBorderSize = 1.0f;
  style.FrameBorderSize = 1.0f;
}

void ThemePurpleDark() {
  ImGui::StyleColorsDark();

  ImGuiStyle& style = ImGui::GetStyle();
  ImVec4* colors = style.Colors;

  // --- Base palette (dark) ---
  ImVec4 bg = ImVec4(0.07f, 0.07f, 0.09f, 1.00f);
  ImVec4 bgDark = ImVec4(0.05f, 0.05f, 0.07f, 1.00f);
  ImVec4 bgLight = ImVec4(0.14f, 0.14f, 0.18f, 1.00f);

  ImVec4 text = ImVec4(0.95f, 0.95f, 0.98f, 1.00f);
  ImVec4 textDisabled = ImVec4(0.55f, 0.55f, 0.60f, 1.00f);

  // Purple accent family (~#9B59B6, #AF7AC5)
  ImVec4 purpleBase = ImVec4(0.61f, 0.35f, 0.71f, 1.00f);
  ImVec4 purpleHover = ImVec4(0.69f, 0.45f, 0.80f, 1.00f);
  ImVec4 purpleActive = ImVec4(0.52f, 0.23f, 0.64f, 1.00f);

  // Text
  colors[ImGuiCol_Text] = text;
  colors[ImGuiCol_TextDisabled] = textDisabled;

  // Backgrounds
  colors[ImGuiCol_WindowBg] = bg;
  colors[ImGuiCol_ChildBg] = bgDark;
  colors[ImGuiCol_PopupBg] = ImVec4(0.10f, 0.10f, 0.14f, 0.98f);

  // Borders
  colors[ImGuiCol_Border] = ImVec4(0.35f, 0.25f, 0.45f, 0.70f);
  colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);

  // Frames
  colors[ImGuiCol_FrameBg] = ImVec4(0.13f, 0.13f, 0.17f, 1.00f);
  colors[ImGuiCol_FrameBgHovered] = ImVec4(0.18f, 0.18f, 0.23f, 1.00f);
  colors[ImGuiCol_FrameBgActive] = ImVec4(0.22f, 0.22f, 0.27f, 1.00f);

  // Title bars
  colors[ImGuiCol_TitleBg] = bgDark;
  colors[ImGuiCol_TitleBgActive] = ImVec4(0.12f, 0.10f, 0.18f, 1.00f);
  colors[ImGuiCol_TitleBgCollapsed] = bgDark;

  // Menus & bars
  colors[ImGuiCol_MenuBarBg] = bgDark;
  colors[ImGuiCol_ScrollbarBg] = bgDark;
  colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.30f, 0.30f, 0.38f, 1.00f);
  colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.37f, 0.37f, 0.45f, 1.00f);
  colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.45f, 0.45f, 0.55f, 1.00f);

  // Checkboxes, radio buttons
  colors[ImGuiCol_CheckMark] = purpleBase;

  // Sliders
  colors[ImGuiCol_SliderGrab] = purpleBase;
  colors[ImGuiCol_SliderGrabActive] = purpleActive;

  // Buttons
  colors[ImGuiCol_Button] = ImVec4(purpleBase.x, purpleBase.y, purpleBase.z, 0.85f);
  colors[ImGuiCol_ButtonHovered] = ImVec4(purpleHover.x, purpleHover.y, purpleHover.z, 0.95f);
  colors[ImGuiCol_ButtonActive] = ImVec4(purpleActive.x, purpleActive.y, purpleActive.z, 1.00f);

  // Headers
  colors[ImGuiCol_Header] = ImVec4(purpleBase.x, purpleBase.y, purpleBase.z, 0.65f);
  colors[ImGuiCol_HeaderHovered] = ImVec4(purpleHover.x, purpleHover.y, purpleHover.z, 0.90f);
  colors[ImGuiCol_HeaderActive] = ImVec4(purpleActive.x, purpleActive.y, purpleActive.z, 1.00f);

  // Separator
  colors[ImGuiCol_Separator] = ImVec4(0.40f, 0.30f, 0.55f, 0.80f);
  colors[ImGuiCol_SeparatorHovered] = ImVec4(purpleHover.x, purpleHover.y, purpleHover.z, 0.85f);
  colors[ImGuiCol_SeparatorActive] = ImVec4(purpleActive.x, purpleActive.y, purpleActive.z, 1.00f);

  // Tabs
  colors[ImGuiCol_Tab] = ImVec4(0.12f, 0.10f, 0.18f, 1.00f);
  colors[ImGuiCol_TabHovered] = ImVec4(purpleHover.x, purpleHover.y, purpleHover.z, 0.85f);
  colors[ImGuiCol_TabActive] = ImVec4(purpleBase.x, purpleBase.y, purpleBase.z, 0.95f);
  colors[ImGuiCol_TabUnfocused] = ImVec4(0.08f, 0.08f, 0.10f, 1.00f);
  colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.10f, 0.10f, 0.13f, 1.00f);

  // Resize grips
  colors[ImGuiCol_ResizeGrip] = ImVec4(purpleBase.x, purpleBase.y, purpleBase.z, 0.25f);
  colors[ImGuiCol_ResizeGripHovered] = ImVec4(purpleHover.x, purpleHover.y, purpleHover.z, 0.70f);
  colors[ImGuiCol_ResizeGripActive] = ImVec4(purpleActive.x, purpleActive.y, purpleActive.z, 0.95f);

  // Selection & nav
  colors[ImGuiCol_TextSelectedBg] = ImVec4(purpleBase.x, purpleBase.y, purpleBase.z, 0.35f);
  colors[ImGuiCol_DragDropTarget] = purpleHover;
  colors[ImGuiCol_NavHighlight] = purpleBase;

  // Modals
  colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.70f);

  // --- Styling ---
  style.WindowRounding = 6.0f;
  style.FrameRounding = 4.0f;
  style.PopupRounding = 4.0f;
  style.ScrollbarRounding = 6.0f;
  style.GrabRounding = 4.0f;
  style.TabRounding = 4.0f;

  style.WindowBorderSize = 1.0f;
  style.FrameBorderSize = 1.0f;
}

void ThemePurpleLight() {
  ImGui::StyleColorsLight();

  ImGuiStyle& style = ImGui::GetStyle();
  ImVec4* colors = style.Colors;

  // --- Base palette (light) ---
  ImVec4 bg = ImVec4(0.97f, 0.97f, 0.99f, 1.00f);
  ImVec4 bgAlt = ImVec4(0.94f, 0.94f, 0.98f, 1.00f);
  ImVec4 bgFrame = ImVec4(0.89f, 0.89f, 0.94f, 1.00f);

  ImVec4 text = ImVec4(0.11f, 0.11f, 0.16f, 1.00f);
  ImVec4 textDisabled = ImVec4(0.55f, 0.55f, 0.62f, 1.00f);

  // Same purple accent family
  ImVec4 purpleBase = ImVec4(0.61f, 0.35f, 0.71f, 1.00f);
  ImVec4 purpleHover = ImVec4(0.69f, 0.45f, 0.80f, 1.00f);
  ImVec4 purpleActive = ImVec4(0.52f, 0.23f, 0.64f, 1.00f);

  // Text
  colors[ImGuiCol_Text] = text;
  colors[ImGuiCol_TextDisabled] = textDisabled;

  // Backgrounds
  colors[ImGuiCol_WindowBg] = bg;
  colors[ImGuiCol_ChildBg] = bgAlt;
  colors[ImGuiCol_PopupBg] = ImVec4(0.99f, 0.99f, 1.00f, 0.98f);

  // Borders
  colors[ImGuiCol_Border] = ImVec4(0.44f, 0.40f, 0.58f, 0.90f);
  colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);

  // Frames
  colors[ImGuiCol_FrameBg] = bgFrame;
  colors[ImGuiCol_FrameBgHovered] = ImVec4(0.93f, 0.93f, 0.98f, 1.00f);
  colors[ImGuiCol_FrameBgActive] = ImVec4(0.90f, 0.90f, 0.96f, 1.00f);

  // Title bars
  colors[ImGuiCol_TitleBg] = bgAlt;
  colors[ImGuiCol_TitleBgActive] = ImVec4(0.98f, 0.98f, 1.00f, 1.00f);
  colors[ImGuiCol_TitleBgCollapsed] = bgAlt;

  // Menus & bars
  colors[ImGuiCol_MenuBarBg] = bgAlt;
  colors[ImGuiCol_ScrollbarBg] = bgAlt;
  colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.75f, 0.75f, 0.83f, 1.00f);
  colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.67f, 0.67f, 0.78f, 1.00f);
  colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.60f, 0.60f, 0.72f, 1.00f);

  // Checkboxes, radio buttons
  colors[ImGuiCol_CheckMark] = purpleActive;

  // Sliders
  colors[ImGuiCol_SliderGrab] = purpleBase;
  colors[ImGuiCol_SliderGrabActive] = purpleActive;

  // Buttons
  colors[ImGuiCol_Button] = ImVec4(purpleBase.x, purpleBase.y, purpleBase.z, 0.90f);
  colors[ImGuiCol_ButtonHovered] = ImVec4(purpleHover.x, purpleHover.y, purpleHover.z, 0.98f);
  colors[ImGuiCol_ButtonActive] = ImVec4(purpleActive.x, purpleActive.y, purpleActive.z, 1.00f);

  // Headers
  colors[ImGuiCol_Header] = ImVec4(purpleBase.x, purpleBase.y, purpleBase.z, 0.60f);
  colors[ImGuiCol_HeaderHovered] = ImVec4(purpleHover.x, purpleHover.y, purpleHover.z, 0.90f);
  colors[ImGuiCol_HeaderActive] = ImVec4(purpleActive.x, purpleActive.y, purpleActive.z, 1.00f);

  // Separator
  colors[ImGuiCol_Separator] = ImVec4(0.78f, 0.75f, 0.90f, 0.90f);
  colors[ImGuiCol_SeparatorHovered] = ImVec4(purpleHover.x, purpleHover.y, purpleHover.z, 0.95f);
  colors[ImGuiCol_SeparatorActive] = ImVec4(purpleActive.x, purpleActive.y, purpleActive.z, 1.00f);

  // Tabs
  colors[ImGuiCol_Tab] = ImVec4(0.93f, 0.93f, 0.98f, 1.00f);
  colors[ImGuiCol_TabHovered] = ImVec4(purpleHover.x, purpleHover.y, purpleHover.z, 0.95f);
  colors[ImGuiCol_TabActive] = ImVec4(purpleBase.x, purpleBase.y, purpleBase.z, 0.98f);
  colors[ImGuiCol_TabUnfocused] = ImVec4(0.95f, 0.95f, 0.99f, 1.00f);
  colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.97f, 0.97f, 1.00f, 1.00f);

  // Resize grips
  colors[ImGuiCol_ResizeGrip] = ImVec4(purpleBase.x, purpleBase.y, purpleBase.z, 0.30f);
  colors[ImGuiCol_ResizeGripHovered] = ImVec4(purpleHover.x, purpleHover.y, purpleHover.z, 0.85f);
  colors[ImGuiCol_ResizeGripActive] = ImVec4(purpleActive.x, purpleActive.y, purpleActive.z, 1.00f);

  // Selection & nav
  colors[ImGuiCol_TextSelectedBg] = ImVec4(purpleBase.x, purpleBase.y, purpleBase.z, 0.35f);
  colors[ImGuiCol_DragDropTarget] = purpleHover;
  colors[ImGuiCol_NavHighlight] = purpleBase;

  // Modals
  colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.10f, 0.10f, 0.15f, 0.35f);

  // --- Styling (same shapes as dark) ---
  style.WindowRounding = 6.0f;
  style.FrameRounding = 4.0f;
  style.PopupRounding = 4.0f;
  style.ScrollbarRounding = 6.0f;
  style.GrabRounding = 4.0f;
  style.TabRounding = 4.0f;

  style.WindowBorderSize = 1.0f;
  style.FrameBorderSize = 1.0f;
}

