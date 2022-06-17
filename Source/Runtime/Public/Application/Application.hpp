#pragma once

#include "Configuration.hpp"
#include "Interface/IApplication.hpp"
#include "Interface/Interface.hpp"

namespace LearnVulkan
{
    class Application : _implements_ IApplication
    {
    public:
        explicit Application(const ApplicationConfiguration&);
        Application() = delete;
        virtual int  initialize() override;
        virtual void finalize() override;
        virtual void tick() override;

        bool isQuit() override;

    protected:
        bool mbQuit;

        const ApplicationConfiguration& mConfig;

        uint32_t mVulkanExtensionCount = 0;

        virtual void initWindow() override;
        virtual void initVulkan() override;
    };
}  // namespace LearnVulkan