#include "shader_manager.h"

ShaderManager& ShaderManager::getInstance() {
	static ShaderManager instance;
	return instance;
}

Shader* ShaderManager::getOrCreate(const char* vertPath, const char* fragPath) {
	std::string key = std::string(vertPath) + "|" + fragPath;
	auto it = mCache.find(key);
	if (it != mCache.end()) {
		return it->second.get();
	}
	auto shader = std::make_unique<Shader>(vertPath, fragPath);
	Shader* raw = shader.get();
	mCache[key] = std::move(shader);
	return raw;
}
