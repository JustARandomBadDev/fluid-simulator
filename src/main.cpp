#include <exception>
#include <iostream>
#include <stdexcept>
#include <string_view>

#include "app/application.hpp"

int main(int argc, char** argv) {
    try {
        const bool smokeTest = argc == 2 && std::string_view(argv[1]) == "--smoke-test";
        if (argc > 1 && !smokeTest) {
            throw std::runtime_error("usage: fluid-simulator [--smoke-test]");
        }

        fluid::Application application;
        application.run(smokeTest);
    } catch (const std::exception& exception) {
        std::cerr << "fluid-simulator: " << exception.what() << '\n';
        return 1;
    }

    return 0;
}
