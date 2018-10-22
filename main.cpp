#include "ext/cpptoml.h"
#include <iostream>
#include <string>
#include <vector>

using namespace std;

int main() {
    std::cout << std::stod(".6") << endl;
    auto config = cpptoml::parse_file("configs/default.toml");
    auto window = config->get_table("window");
    if (window) {
        int width = *(window->get_as<int>("width"));
        float height = *(window->get_as<float>("height"));
        cout << "w = " << width << ", h = " << height << endl;
        auto e = window->get_as<int>("NA");
        if (e) {
            cout << "there was an e" << endl;
        } else {
            cout << "there was not an e" << endl;
        }
    }

    auto graphics = config->get_table("graphics");
    if (graphics) {
        cout << "there was a graphcis" << endl;
    } else {
        cout << "there was not a graphics" << endl;
    }
    return 0;
}
