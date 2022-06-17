#pragma once

#include "Interface/Interface.hpp"

namespace LearnVulkan
{
    _Interface_ IModule
    {
    public:
        IModule()               = default;
        virtual ~IModule()      = default;
        IModule(IModule const&) = delete;
        IModule(IModule &&)     = delete;
        IModule& operator=(IModule const&) = delete;
        IModule& operator=(IModule&&) = delete;

        virtual int  initialize() = 0;
        virtual void finalize()   = 0;
        virtual void tick()       = 0;
    };
}  // namespace LearnVulkan