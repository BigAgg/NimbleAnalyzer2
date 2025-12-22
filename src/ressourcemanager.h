#pragma once
#include <raylib.h>
#include <string>

struct ImageInfo {
	std::string path;
	std::string name;
	Image image;
};

struct TextureInfo {
	std::string path;
	std::string name;
	Texture texture;
};

void UnloadRessources();
void StoreImage(Image image, const std::string& path, const std::string& name);
void StoreImage(const std::string& path, const std::string& name);

void StoreTexture(Texture texture, const std::string& path, const std::string& name);
void StoreTexture(const std::string& path, const std::string& name);

ImageInfo* GetImageInfo(const std::string& name);
Image* GetImage(const std::string& name);

TextureInfo* GetTextureInfo(const std::string& name);
Texture* GetTexture(const std::string& name);

