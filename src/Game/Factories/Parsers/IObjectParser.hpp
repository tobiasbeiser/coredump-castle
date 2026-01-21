#pragma once
#include "tileson.hpp"
#include <glm/vec2.hpp>

class IObjectParser {
  protected:
	IObjectParser() = default;

  public:
	IObjectParser(const IObjectParser &) = delete;
	IObjectParser &operator=(const IObjectParser &) = delete;
	IObjectParser(const IObjectParser &&) = delete;
	IObjectParser &operator=(const IObjectParser &&) = delete;
	virtual ~IObjectParser() = default;

	virtual bool canParse(tson::Object &obj) const = 0;

	virtual void parse(tson::Object &obj, const glm::vec2 &levelPos, int layerIndex) const = 0;
};