#include "ressourcemanager.h"
#include <vector>
#include <unordered_map>

static std::unordered_map<std::string, ImageInfo> images;
static std::unordered_map<std::string, TextureInfo> textures;

void UnloadRessources() {
	for (auto& ii : images) {
		UnloadImage(ii.second.image);
		ii.second.name.clear();
		ii.second.path.clear();
	}
	images.clear();
	for (auto& ti : textures) {
		UnloadTexture(ti.second.texture);
		ti.second.name.clear();
		ti.second.path.clear();
	}
	textures.clear();
}

void StoreImage(Image image, const std::string& path, const std::string& name) {
	ImageInfo ii;
	ii.image = image;
	ii.path = path;
	ii.name = name;
	images[name] = ii;
}

void StoreImage(const std::string& path, const std::string& name) {
	ImageInfo ii;
	ii.image = LoadImage(path.c_str());
	ii.path = path;
	ii.name = name;
	images[name] = ii;
}

void StoreTexture(Texture texture, const std::string& path, const std::string& name) {
	TextureInfo ti;
	ti.texture = texture;
	ti.path = path;
	ti.name = name;
	textures[name] = ti;
}

void StoreTexture(const std::string& path, const std::string& name) {
	TextureInfo ti;
	ti.texture = LoadTexture(path.c_str());
	ti.path = path;
	ti.name = name;
	textures[name] = ti;
}

ImageInfo* GetImageInfo(const std::string& name) {
	auto it = images.find(name);
	if (it != images.end()) {
		return &it->second;
	}
	return nullptr;
}

Image* GetImage(const std::string& name) {
	auto it = images.find(name);
	if (it != images.end()) {
		return &it->second.image;
	}
	return nullptr;
}

TextureInfo* GetTextureInfo(const std::string& name) {
	auto it = textures.find(name);
	if (it != textures.end()) {
		return &it->second;
	}
	return nullptr;
}

Texture* GetTexture(const std::string& name) {
	auto it = textures.find(name);
	if (it != textures.end()) {
		return &it->second.texture;
	}
	return nullptr;
}
