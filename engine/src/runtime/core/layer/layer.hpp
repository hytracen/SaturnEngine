#pragma once

#include <runtime/core/event/event.hpp>

#include <engine_pch.hpp>

namespace saturn {
class Layer {
public:
    explicit Layer(std::string name = "DefaultLayer");
    Layer(Layer&& layer) = default;
    auto operator=(Layer&& layer) -> Layer& = default;
    virtual ~Layer() = default;

    virtual void OnAttach() = 0;                // 此层被添加时执行
    virtual void OnDetach() = 0;                // 此层被移除时执行
    virtual void Update() = 0;                  // 当前层更新
    virtual auto ProcessEvent(Event &event) -> std::optional<Event> = 0;// 当前层处理事件，如果该事件被当前层处理，则不会传递到下一层

    [[nodiscard]] auto GetName() const -> const std::string & { return m_layer_name; }

protected:
    std::string m_layer_name;
};

class LayerManager {
public:
    LayerManager();
    LayerManager(const LayerManager& manager) = delete;
    auto operator=(const LayerManager& manager) -> LayerManager& = delete;

    /**
     * 将Layer推入到最上层（在渲染上覆盖其它层）
     */
    void PushOverlay(std::unique_ptr<Layer> layer);
    
    /**
     * 将Layer推入到最下层（在渲染上被其他层覆盖）
     */
    void PushUnderlay(std::unique_ptr<Layer> layer);

private:
    std::vector<std::unique_ptr<Layer>> m_layers{};

};


}// namespace saturn
