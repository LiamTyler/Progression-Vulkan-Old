#include "progression.h"

using namespace Progression;

int main(int argc, char* argv[]) {
    if (argc != 4) {
        std::cout << "Usage: pathToOBJFile  pathToMaterialDirectory   pathToPGModelOutput" << std::endl;
        return 0;
    }

    std::string fullPathToOBJ = argv[1];
    std::string materialDir   = argv[2];
    std::string pgModelPath   = argv[3];

    if (!ResourceManager::ConvertOBJToPGModel(fullPathToOBJ, materialDir, pgModelPath)) {
        std::cout << "Unable to convert obj to pgModel" << std::endl;
    }

    return 0;
}