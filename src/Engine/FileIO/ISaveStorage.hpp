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

	virtual void write(const nlohmann::json &data, const std::string &fileName) = 0;
	[[nodiscard]] virtual nlohmann::json read(const std::string &fileName) const = 0;

  protected:
	ISaveStorage() = default;
};

} // namespace GameEngine::ECS