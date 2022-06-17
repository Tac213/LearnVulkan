#include "Application/Application.hpp"
#include <cstdlib>

using namespace LearnVulkan;

int main()
{
    const ApplicationConfiguration CONFIG(800, 600, "Learn Vulkan");

    IApplication* application = new Application(CONFIG);

    int result;
    if ((result = application->initialize()) != EXIT_SUCCESS)
    {
        return result;
    }

    while (!application->isQuit())
    {
        application->tick();
    }

    application->finalize();

    return EXIT_SUCCESS;
}
