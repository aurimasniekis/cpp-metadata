#include <gtest/gtest.h>

#include <md/metadata.hpp>

#include <string_view>

#include <parcel/parcel.h>

using md::Array;
using md::ArrayCell;
using md::Object;
using md::ObjectCell;
using md::Value;
using md::ValueCell;

namespace {

parcel::ParcelRegistry make_registry() {
    parcel::ParcelRegistry r;
    md::register_cells(r);
    return r;
}

}  // namespace

TEST(Parcel, KindIds) {
    EXPECT_EQ(ValueCell::kind_id, std::string_view{"md:v"});
    EXPECT_EQ(ObjectCell::kind_id, std::string_view{"md:o"});
    EXPECT_EQ(ArrayCell::kind_id, std::string_view{"md:a"});
}

TEST(Parcel, ValueRoundTrip) {
    const auto registry = make_registry();
    const ValueCell cell{Value{42}};
    const auto j = cell.to_json();
    EXPECT_EQ(j.at("k").get<std::string>(), "md:v");

    const auto restored = registry.cell_from_json(j);
    const auto* typed = dynamic_cast<ValueCell*>(restored.get());
    ASSERT_NE(typed, nullptr);
    EXPECT_TRUE(typed->value.is_int());
    EXPECT_EQ(typed->value.as_int(), 42);
}

TEST(Parcel, ObjectRoundTrip) {
    auto registry = make_registry();
    const Object payload{
        {"name", "sensor"},
        {"count", 7},
        {"readings", Array{1.0, 2.5, 3.75}},
    };
    const ObjectCell cell{payload};
    const auto j = cell.to_json();
    EXPECT_EQ(j.at("k").get<std::string>(), "md:o");

    const auto restored = registry.cell_from_json(j);
    const auto* typed = dynamic_cast<ObjectCell*>(restored.get());
    ASSERT_NE(typed, nullptr);
    EXPECT_EQ(typed->value, payload);
}

TEST(Parcel, ArrayRoundTrip) {
    auto registry = make_registry();
    const Array payload{Value{1}, Value{"x"}, Value{true}};
    const ArrayCell cell{payload};
    const auto j = cell.to_json();
    EXPECT_EQ(j.at("k").get<std::string>(), "md:a");

    const auto restored = registry.cell_from_json(j);
    const auto* typed = dynamic_cast<ArrayCell*>(restored.get());
    ASSERT_NE(typed, nullptr);
    ASSERT_EQ(typed->value.size(), payload.size());
    EXPECT_EQ(typed->value, payload);
}

TEST(Parcel, KindMismatchRejected) {
    auto registry = make_registry();
    parcel::json_t wrong = {{"k", "md:o"}, {"v", parcel::json_t::object({{"k", 1}})}};
    // Feeding an object envelope into ValueCell::from_json must fail the kind check.
    EXPECT_THROW((void)ValueCell::from_json(wrong, registry), parcel::ParcelException);
}

TEST(Parcel, DescriptorCarriesIconAndColor) {
    const auto vd = ValueCell::descriptor()->meta();
    EXPECT_TRUE(vd.icon.has_value());
    EXPECT_TRUE(vd.color.has_value());
    EXPECT_EQ(*vd.icon, "mdi:variable");

    const auto od = ObjectCell::descriptor()->meta();
    EXPECT_EQ(*od.icon, "mdi:code-braces");
    EXPECT_EQ(*od.color, "#FFA000");

    const auto ad = ArrayCell::descriptor()->meta();
    EXPECT_EQ(*ad.icon, "mdi:code-brackets");
    EXPECT_EQ(*ad.color, "#43A047");
}

TEST(Parcel, BraceInitPassthrough) {
    // ArrayCell takes the same {…} shorthand as md::Array.
    const ArrayCell a{"a", "b", "c"};
    ASSERT_EQ(a.value.size(), 3u);
    EXPECT_EQ(a.value[0].as_string(), "a");
    EXPECT_EQ(a.value[2].as_string(), "c");

    // ObjectCell takes the same {{key, value}, ...} shorthand as md::Object.
    const ObjectCell o{{"name", "x"}, {"count", 7}};
    EXPECT_EQ(o.value.size(), 2u);
    EXPECT_EQ(o.value.at("name").as_string(), "x");
    EXPECT_EQ(o.value.at("count").as_int(), 7);
}

TEST(Parcel, DefaultCellInference) {
    // PARCEL_DEFAULT_CELL specializations let parcel::cell(...) pick the wrapper.
    const parcel::cell_t v_handle = parcel::cell(Value{1});
    const parcel::cell_t o_handle = parcel::cell(Object{{"k", 1}});
    const parcel::cell_t a_handle = parcel::cell(Array{Value{1}});

    EXPECT_EQ(v_handle->kind(), std::string_view{"md:v"});
    EXPECT_EQ(o_handle->kind(), std::string_view{"md:o"});
    EXPECT_EQ(a_handle->kind(), std::string_view{"md:a"});
}
