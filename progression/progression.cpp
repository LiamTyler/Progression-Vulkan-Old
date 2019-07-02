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
        auto winConfig = conf->get_table("window");
        if (!winConfig) {
            LOG_ERR("Need to specify the 'window' in the config file");
            exit(EXIT_FAILURE);
        }
        WindowCreateInfo winCreate;
		winCreate.title = winConfig->get_as<std::string>("title").value_or("Untitled");
		winCreate.width = winConfig->get_as<int>("width").value_or(1280);
		winCreate.height = winConfig->get_as<int>("height").value_or(720);
		winCreate.visible = winConfig->get_as<bool>("visible").value_or(true);
        initWindowSystem(winCreate);
        Time::Init(conf);
        Input::Init(conf);
        ResourceManager::init();
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
        ResourceManager::shutdown();
        Input::Free();
        Time::Free();
        shutdownWindowSystem();
        Logger::Free();
    }

} // namespace Progression
