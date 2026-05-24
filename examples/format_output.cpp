#include <md/metadata.hpp>

#include <format>
#include <iostream>

int main() {
    md::Value v = md::Object{
        {"name", "radio"},
        {"power_dbm", -10.5},
        {"channels", md::Array{1, 2, 3}},
    };

    std::cout << "via std::format: " << std::format("{}", v) << "\n";
    std::cout << "via operator<<:  " << v << "\n";

    return 0;
}
