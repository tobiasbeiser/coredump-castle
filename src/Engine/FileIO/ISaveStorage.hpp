#pragma once

#include <nlohmann/json_fwd.hpp>
using json = nlohmann::json;

namespace GameEngine::ECS {

class ISaveStorage {
  public:
	ISaveStorage(const ISaveStorage &) = delete;
	ISaveStorage &operator=(const ISaveStorage &) = delete;
	ISaveStorage(ISaveStorage &&) = delete;
	ISaveStorage &operator=(ISaveStorage &&) = delete;
	virtual ~ISaveStorage() = default;

	virtual void write(const json &data) = 0;
	[[nodiscard]] virtual json read() const = 0;

  protected:
	ISaveStorage() = default;
};

} // namespace GameEngine::ECS