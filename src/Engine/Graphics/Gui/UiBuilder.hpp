#pragma once
#include <vector>
#include <memory>
#include <concepts>
#include "Base/Widget.hpp"
#include "Layouts/Container.hpp"
#include "LayoutEngine.hpp"

namespace Ui {

// Configuration for a specific child in the layout
struct ChildLayoutConfig {
    // For Grid
    int row = 0;
    int col = 0;
    int spanX = 1;
    int spanY = 1;
    
    // For Linear/Stack
    float weight = 0.0f; 

    // Alignment
    LayoutEngine::Alignment hAlign = LayoutEngine::Alignment::Center;
    LayoutEngine::Alignment vAlign = LayoutEngine::Alignment::Center;
};

// Builder
template <typename RootType = Container>
class UiBuilder {
public:
    enum class LayoutMode { Linear, Stack, Grid };

    static UiBuilder<RootType> create(LayoutMode mode = LayoutMode::Linear) {
        UiBuilder<RootType> b;
        b.mode = mode;
        return b;
    }

    // --- Configuration ---
    
    // Grid Config
    UiBuilder& withGrid(int rows, int cols, float gap = 0.0f) {
        mode = LayoutMode::Grid;
        gridCfg.rows = rows;
        gridCfg.columns = cols;
        gridCfg.gap = gap;
        return *this;
    }

    // Stack Config (just mode)
    UiBuilder& withStack() {
        mode = LayoutMode::Stack;
        return *this;
    }

    // Linear Config
    UiBuilder& withLinear(LayoutEngine::Orientation orient, float spacing = 0.0f) {
        mode = LayoutMode::Linear;
        linearCfg.orientation = orient;
        linearCfg.spacing = spacing;
        return *this;
    }

    // --- Adding Children ---

    // 1. Add direct Widget
    template <typename T>
    UiBuilder& add(T&& widget, ChildLayoutConfig cfg = {}) {
        auto ptr = std::make_unique<T>(std::move(widget));
        children.push_back({std::move(ptr), cfg});
        return *this;
    }

    // 2. Add Nested Builder (Recursion!)
    // This builds the sub-tree using a temporary rect (0,0), 
    // but the parent LayoutEngine will overwrite the bounds later.
    template <typename T>
    UiBuilder& add(UiBuilder<T>&& subBuilder, ChildLayoutConfig cfg = {}) {
        // We temporarily build it with size 0. 
        // The parent solve() will set the correct bounds later.
        std::unique_ptr<Widget> subTree = subBuilder.build(nullptr, {0,0,0,0});
        children.push_back({std::move(subTree), cfg});
        return *this;
    }

    // --- Build ---

    std::unique_ptr<RootType> build(const UIContext* ctx, SDL_FRect targetRect) {
        auto root = std::make_unique<RootType>();
        root->setBounds(targetRect);

        // 1. Convert Children to LayoutItems
        std::vector<LayoutEngine::LayoutItem> items;
        items.reserve(children.size());

        for (auto& [ptr, cfg] : children) {
            glm::vec2 ideal = {0,0};
            if (ctx) ideal = ptr->getIdealContentSize(*ctx);
            
            items.push_back({
                ptr.get(),
                cfg.row, cfg.col, cfg.spanX, cfg.spanY,
                cfg.hAlign, cfg.vAlign,
                ideal
            });
        }

        // 2. Solve Layout (BAKING)
        if (mode == LayoutMode::Stack) {
            LayoutEngine::solveStack(items, targetRect);
        } else if (mode == LayoutMode::Grid) {
            LayoutEngine::solveGrid(items, targetRect, gridCfg);
        } else {
            // Adapt Linear params
            // Note: Linear solver needs updating to match the unified LayoutItem struct
            // or we map it here. For brevity, assuming solveLinear updated to use LayoutItem.
             LayoutEngine::solveLinear(items, targetRect, linearCfg);
        }

        // 3. Adopt
        std::vector<std::unique_ptr<Widget>> finalChildren;
        for (auto& c : children) finalChildren.push_back(std::move(c.widget));
        root->adoptChildren(std::move(finalChildren));

        return root;
    }

private:
    struct Entry {
        std::unique_ptr<Widget> widget;
        ChildLayoutConfig config;
    };

    LayoutMode mode = LayoutMode::Linear;
    std::vector<Entry> children;
    
    LayoutEngine::GridConfig gridCfg;
    LayoutEngine::LinearConfig linearCfg;
};

} // namespace Ui