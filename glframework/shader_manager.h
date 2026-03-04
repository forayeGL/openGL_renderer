#pragma once
#include "shader.h"
#include <string>
#include <unordered_map>
#include <memory>

class ShaderManager {
public:
	static ShaderManager& getInstance();

	Shader* getOrCreate(const char* vertPath, const char* fragPath);

	ShaderManager(const ShaderManager&) = delete;
	ShaderManager& operator=(const ShaderManager&) = delete;

private:
	ShaderManager() = default;
	~ShaderManager() = default;

	std::unordered_map<std::string, std::unique_ptr<Shader>> mCache;
};
