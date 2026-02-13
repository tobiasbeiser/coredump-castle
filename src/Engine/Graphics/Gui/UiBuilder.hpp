#pragma once
#include "Base/Widget.hpp"
#include "LayoutEngine.hpp"
#include <concepts>
#include <memory>
#include <vector>

namespace Ui {

template <typename T>
concept IsWidget = std::derived_from<T, Widget>;

template <IsWidget RootType>
class UiBuilder {
  public:
	enum class LayoutMode { Linear, Stack, Grid };

	static UiBuilder create(LayoutMode mode = LayoutMode::Linear)
	{
		UiBuilder builder;
		builder.mode = mode;
		return builder;
	}

	UiBuilder &withGrid(std::vector<LayoutEngine::Track> cols, std::vector<LayoutEngine::Track> rows, float gap = 0.0f)
	{
		mode = LayoutMode::Grid;
		gridCfg.colDefs = std::move(cols);
		gridCfg.rowDefs = std::move(rows);
		gridCfg.gap = gap;
		return *this;
	}

	UiBuilder &withStack()
	{
		mode = LayoutMode::Stack;
		return *this;
	}

	UiBuilder &withLinear(const LayoutEngine::Orientation orientation, const float spacing = 0.0f)
	{
		linearCfg.orientation = orientation;
		linearCfg.spacing = spacing;
		return *this;
	}

	template <IsWidget T>
	UiBuilder &add(T &&widget, LayoutEngine::LayoutProps props = {})
	{
		std::unique_ptr<Widget> ptr = std::make_unique<T>(std::forward<T>(widget));
		children.push_back({std::make_unique<WidgetNode>(std::move(ptr)), props});
		return *this;
	}

	UiBuilder &add(std::unique_ptr<Widget> widget, LayoutEngine::LayoutProps props = {})
	{
		if (widget)
			children.push_back({std::make_unique<WidgetNode>(std::move(widget)), props});
		return *this;
	}

	template <typename T>
	UiBuilder &add(UiBuilder<T> &&subBuilder, LayoutEngine::LayoutProps props = {})
	{
		children.push_back({std::make_unique<SubBuilderNode<T>>(std::move(subBuilder)), props});
		return *this;
	}

	template <typename T>
	UiBuilder &add(UiBuilder<T> &subBuilder, const LayoutEngine::LayoutProps props = {})
	{
		return add(std::move(subBuilder), props);
	}

	std::unique_ptr<RootType> build(const UIContext *ctx, SDL_FRect targetRect)
	{
		auto root = std::make_unique<RootType>();
		root->setBounds(targetRect);

		SDL_FRect localRect = {0.0f, 0.0f, targetRect.w, targetRect.h};

		std::vector<LayoutEngine::LayoutItem> solveItems;
		solveItems.reserve(children.size());

		for (auto &entry : children) {
			LayoutEngine::LayoutItem item;
			static_cast<LayoutEngine::LayoutProps &>(item) = entry.props;

			glm::vec2 naturalSize = entry.node->getIdealSize(ctx);
			item.idealSize = {std::max(naturalSize.x, entry.props.minSize.x),
			                  std::max(naturalSize.y, entry.props.minSize.y)};

			solveItems.push_back(item);
		}

		if (mode == LayoutMode::Grid)
			LayoutEngine::solveGrid(solveItems, localRect, gridCfg);
		else if (mode == LayoutMode::Stack)
			LayoutEngine::solveStack(solveItems, localRect);
		else
			LayoutEngine::solveLinear(solveItems, localRect, linearCfg);

		if constexpr (requires { root->adoptChildren(std::vector<std::unique_ptr<Widget>>{}); }) {
			std::vector<std::unique_ptr<Widget>> actualChildren;
			actualChildren.reserve(children.size());

			for (size_t i = 0; i < children.size(); ++i) {
				if (std::unique_ptr<Widget> builtWidget =
				        children[i].node->build(ctx, solveItems[i].calculatedBounds)) {
					actualChildren.push_back(std::move(builtWidget));
				}
			}
			root->adoptChildren(std::move(actualChildren));
		}

		return root;
	}

  private:
	struct INode {
		virtual ~INode() = default;
		virtual glm::vec2 getIdealSize(const UIContext *ctx) = 0;
		virtual std::unique_ptr<Widget> build(const UIContext *ctx, SDL_FRect targetRect) = 0;
	};

	struct WidgetNode : INode {
		std::unique_ptr<Widget> widget;

		explicit WidgetNode(std::unique_ptr<Widget> w) : widget(std::move(w)) {}

		glm::vec2 getIdealSize(const UIContext *ctx) override
		{
			return (ctx && widget) ? widget->getIdealContentSize(*ctx) : glm::vec2{0, 0};
		}

		std::unique_ptr<Widget> build(const UIContext *ctx, SDL_FRect targetRect) override
		{
			if (widget)
				widget->setBounds(targetRect);
			return std::move(widget);
		}
	};

	template <typename SubT>
	struct SubBuilderNode : INode {
		UiBuilder<SubT> builder;

		explicit SubBuilderNode(UiBuilder<SubT> &&b) : builder(std::move(b)) {}

		glm::vec2 getIdealSize(const UIContext *ctx) override { return {0, 0}; }

		std::unique_ptr<Widget> build(const UIContext *ctx, SDL_FRect targetRect) override
		{
			return builder.build(ctx, targetRect);
		}
	};

	struct Entry {
		std::unique_ptr<INode> node;
		LayoutEngine::LayoutProps props;
	};

	LayoutMode mode = LayoutMode::Linear;
	std::vector<Entry> children;

	LayoutEngine::GridConfig gridCfg;
	LayoutEngine::LinearConfig linearCfg;
};
} // namespace Ui