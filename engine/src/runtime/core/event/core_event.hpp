#pragma once

#include <engine_pch.hpp>

namespace saturn {

enum class EventType {
    KeyPress,
    KeyHold,
    KeyRelease
};


class Event {
public:
    
    [[nodiscard]] virtual auto Type() const -> EventType = 0;
};

class Event1 : public Event {
    public:
    [[nodiscard]] auto Type() const  -> EventType override {
        return EventType::KeyHold;
    } 
};

/**
 * ParamIsEvent必须是一个参数包含Event类型的函数
 */
template<typename T>
concept ParamIsEvent = requires(const Event &e, T fuc) {
    fuc(e);
};

/**
 * 事件分发器
 */
class EventDispatcher {
public:
    
    template<ParamIsEvent T>
    void Register(EventType event_type, T observer) {
        m_observers[event_type].push_back(observer);
    }

    void Dispatch(const Event &event);

private:
    std::unordered_map<EventType, std::vector<std::function<bool(const Event &e)>>> m_observers;
};

}// namespace saturn
