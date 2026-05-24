#include <md/metadata.hpp>

#include <iostream>

#include <parcel/parcel.h>

int main() {
    parcel::ParcelRegistry registry;
    md::register_cells(registry);  // registers md:v, md:o, md:a in one call

    // A md::Object — brace-init passthrough lets us skip the md::Object{} wrapper.
    md::ObjectCell cell = {
        {"name", "sensor"},
        {"enabled", true},
        {"readings", md::Array{1.0, 2.5, 3.75}},
    };
    const auto wire = cell.to_json();
    std::cout << "wire: " << wire.dump() << "\n";

    // Round-trip through the registry.
    const parcel::cell_t restored = registry.cell_from_json(wire);
    std::cout << "restored kind: " << restored->kind() << "\n";

    const auto* typed = dynamic_cast<md::ObjectCell*>(restored.get());
    std::cout << "restored payload: " << typed->value << "\n";

    // Same for a bare md::Value (a heterogeneous wrapper).
    const md::ValueCell value_cell = 42;
    const auto value_wire = value_cell.to_json();
    const auto value_back = registry.cell_from_json(value_wire);
    std::cout << "value cell: " << value_back->kind() << " = " << value_back->to_string() << "\n";

    // And an md::Array — brace-init passthrough lets us skip the md::Array{} wrapper.
    const md::ArrayCell array_cell = {"a", "b", "c"};
    const auto array_wire = array_cell.to_json();
    const auto array_back = registry.cell_from_json(array_wire);
    std::cout << "array cell: " << array_back->kind() << " = " << array_back->to_string() << "\n";

    return 0;
}
