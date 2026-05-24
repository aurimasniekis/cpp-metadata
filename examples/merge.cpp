#include <md/metadata.hpp>

#include <iostream>

int main() {
    md::Object base{
        {"name", "default"},
        {"options", {{"retries", 3}, {"timeout_ms", 1000}}},
        {"tags", md::Array{"a", "b"}},
    };

    const md::Object overlay{
        {"options", {{"timeout_ms", 500}, {"strict", true}}},
        {"tags", md::Array{"x"}},  // arrays are replaced, not merged
        {"description", "overridden"},
    };

    std::cout << "before:  " << base << "\n";
    base.merge(overlay);
    std::cout << "after :  " << base << "\n";

    return 0;
}
