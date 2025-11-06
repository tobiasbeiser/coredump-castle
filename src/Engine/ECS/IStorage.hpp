#pragma once

namespace GameEngine::ECS {
class IStorage {
  public:
	IStorage(const IStorage &) = delete;
	IStorage &operator=(const IStorage &) = delete;
	IStorage(const IStorage &&) = delete;
	IStorage &operator=(const IStorage &&) = delete;
	virtual ~IStorage() = default;

  protected:
	IStorage() = default;
};
} // namespace GameEngine::ECS