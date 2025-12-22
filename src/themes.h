#pragma once

enum Themes : unsigned char {
	LIGHT,
	DARK,
	GOLD_LIGHT,
	GOLD_DARK,
	PURPLE_LIGHT,
	PURPLE_DARK,
	DEFAULT = GOLD_LIGHT,
};

void ThemeGoldDark();
void ThemeGoldLight();
void ThemePurpleDark();
void ThemePurpleLight();

void SetTheme(Themes theme);
void SetTheme(unsigned int theme);
