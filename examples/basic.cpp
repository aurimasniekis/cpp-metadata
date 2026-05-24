#include <md/metadata.hpp>

#include <iostream>

int main() {
    md::Metadata m;
    m["name"] = "sensor-7";
    m["enabled"] = true;
    m["count"] = 42;
    m["weight"] = 3.14;
    m["tags"] = {"alpha", "beta"};  // braced list → Array

    std::cout << m << "\n";

    std::cout << "name=" << m.require_string("name") << "\n";
    std::cout << "count=" << m["count"].as_int() << "\n";
    std::cout << "weight=" << m["weight"].as_double() << "\n";

    return 0;
}
