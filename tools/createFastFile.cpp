#include "progression.hpp"

using namespace Progression;

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cout << "Usage: RESOURCE_FILE_PATH" << std::endl;
        return 0;
    }

    PG::EngineInitialize(PG_ROOT_DIR "configs/offline.toml");

    if (!ResourceManager::createFastFile(argv[1])) {
        LOG_ERR("Unable to create fastfile for resource file: ", argv[1]);
        exit(EXIT_FAILURE);
    }

    PG::EngineQuit();

    return 0;
}
