#include "progression.hpp"

namespace PG = Progression;

namespace Progression {

    bool EngineShutdown = false;

    void EngineInitialize(std::string config_name) {
        if (config_name == "")
            config_name = PG_ROOT_DIR "configs/default.toml";

        auto conf = config::Config(config_name);
        if (!conf) {
            LOG_ERR("Failed to load the config file: ", config_name);
            exit(EXIT_FAILURE);
        }

        srand(time(NULL));
        Logger::Init(conf);
        random::setSeed();
        initWindowSystem();
        Time::Init(conf);
        Input::Init(conf);
        // ResourceManager::Init(conf);
        // RenderSystem::Init(conf);
#if PG_AUDIO_ENABLED
        AudioSystem::Init(conf);
#endif
    }

    void EngineQuit() {
#if PG_AUDIO_ENABLED
        AudioSystem::Free();
#endif
        // RenderSystem::Free();
        // ResourceManager::Free();
        Input::Free();
        Time::Free();
        shutdownWindowSystem();
        Logger::Free();
    }

} // namespace Progression
