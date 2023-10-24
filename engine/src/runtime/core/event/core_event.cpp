#include "core_event.hpp"


namespace saturn {

void EventDispatcher::Dispatch(const Event &event) {
    if (m_observers.find(event.Type()) == m_observers.end()) return;
    for (const auto &observer: m_observers.at(event.Type())) {
        observer(event);
    }
}

}// namespace saturn