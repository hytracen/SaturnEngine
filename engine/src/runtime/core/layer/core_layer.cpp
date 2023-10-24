#include "core_layer.hpp"

namespace saturn {

Layer::Layer(std::string name) : m_layer_name(std::move(name)) {}


void LayerManager::PushOverlay(std::unique_ptr<Layer> layer) {
    layer->OnAttach();
    m_layers.insert(m_layers.begin(), std::move(layer));
}

void LayerManager::PushUnderlay(std::unique_ptr<Layer> layer) {
    layer->OnAttach();
    m_layers.push_back(std::move(layer));
}

}// namespace saturn