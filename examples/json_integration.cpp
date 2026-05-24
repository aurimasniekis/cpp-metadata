#include <nlohmann/json.hpp>

#include <md/metadata.hpp>

#include <iostream>

int main() {
    md::Object m{
        {"name", "sensor"},
        {"enabled", true},
        {"readings", md::Array{1.0, 2.5, 3.75}},
    };

    nlohmann::json j = md::to_json(m);
    std::cout << "to nlohmann::json: " << j.dump() << "\n";

    // Round-trip back into md::Value via the ADL hook.
    auto back = j.get<md::Value>();
    std::cout << "back to metadata:  " << back << "\n";

    return 0;
}
