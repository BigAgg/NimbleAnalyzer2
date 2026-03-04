#pragma once

enum Themes : unsigned char {
	LIGHT,
	DARK,
	GOLD_LIGHT,
	GOLD_DARK,
	PURPLE_LIGHT,
	PURPLE_DARK,
	GIRLY_PINK,
	DEFAULT = GOLD_LIGHT,
};

void ThemeGoldDark();
void ThemeGoldLight();
void ThemePurpleDark();
void ThemePurpleLight();
void ThemeGirlyPink();

void SetTheme(Themes theme);
void SetTheme(unsigned int theme);
