#pragma once

#include <nlohmann/json.hpp>

namespace GameEngine::ECS {

class ISaveStorage {
  public:
	ISaveStorage(const ISaveStorage &) = delete;
	ISaveStorage &operator=(const ISaveStorage &) = delete;
	ISaveStorage(ISaveStorage &&) = delete;
	ISaveStorage &operator=(ISaveStorage &&) = delete;
	virtual ~ISaveStorage() = default;

	virtual void write(const nlohmann::json &data) = 0;
	[[nodiscard]] virtual nlohmann::json read() const = 0;
	[[nodiscard]] virtual std::vector<std::string> getSavesNames() const = 0;
	virtual void setSaveName(std::string saveName) = 0;

  protected:
	ISaveStorage() = default;
};

} // namespace GameEngine::ECS