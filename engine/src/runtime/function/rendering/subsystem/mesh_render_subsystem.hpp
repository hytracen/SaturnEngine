#pragma once

#include <engine_pch.hpp>

#include <runtime/function/rendering/pipeline.hpp>

namespace saturn {

namespace rendering {

class MeshRenderSubsystem {
public:
    MeshRenderSubsystem();
    ~MeshRenderSubsystem();

    void Render();

private:
    std::shared_ptr<Device> m_device;
    std::unique_ptr<Pipeline> m_pipeline;

};

}  // namespace rendering

}  // namespace saturn