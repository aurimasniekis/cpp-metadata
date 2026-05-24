#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

#include <md/metadata.hpp>

#include <cstdint>
#include <string>

using md::Array;
using md::Object;
using md::Value;

TEST(Json, RoundTripNull) {
    Value src{};
    nlohmann::json j = md::to_json(src);
    EXPECT_TRUE(j.is_null());
    Value back = md::from_json(j);
    EXPECT_TRUE(back.is_null());
}

TEST(Json, RoundTripBool) {
    Value src{true};
    nlohmann::json j = md::to_json(src);
    EXPECT_TRUE(j.is_boolean());
    Value back = md::from_json(j);
    EXPECT_TRUE(back.is_bool());
    EXPECT_EQ(back.as_bool(), true);
}

TEST(Json, IntegerRouting) {
    Value pos{std::int64_t{42}};
    Value neg{std::int64_t{-7}};
    Value big{std::uint64_t{1ULL << 40}};

    nlohmann::json jp = md::to_json(pos);
    nlohmann::json jn = md::to_json(neg);
    nlohmann::json jb = md::to_json(big);

    Value pp = md::from_json(jp);
    Value nn = md::from_json(jn);
    Value bb = md::from_json(jb);

    // nlohmann routes non-negative integers to unsigned by default; the negative
    // one must come back as int64. The big unsigned must come back as uint64.
    EXPECT_TRUE(nn.is_int());
    EXPECT_EQ(nn.as_int(), -7);

    EXPECT_TRUE(bb.is_uint());
    EXPECT_EQ(bb.as_uint(), (1ULL << 40));

    // pp is allowed to be uint or int — nlohmann picks based on parser path.
    EXPECT_TRUE(pp.is_int() || pp.is_uint());
}

TEST(Json, RoundTripDouble) {
    Value src{3.5};
    nlohmann::json j = md::to_json(src);
    EXPECT_TRUE(j.is_number_float());
    Value back = md::from_json(j);
    EXPECT_TRUE(back.is_double());
    EXPECT_DOUBLE_EQ(back.as_double(), 3.5);
}

TEST(Json, FloatRoundTripsThroughDouble) {
    // nlohmann's parser maps all floating-point JSON to its number_float
    // (double) bucket — there's no way to recover that a value was originally
    // a float. We accept that: float → JSON → double.
    Value src{1.5F};
    nlohmann::json j = md::to_json(src);
    EXPECT_TRUE(j.is_number_float());
    Value back = md::from_json(j);
    EXPECT_TRUE(back.is_double());
    EXPECT_DOUBLE_EQ(back.as_double(), 1.5);
}

TEST(Json, RoundTripString) {
    Value src{"hello"};
    nlohmann::json j = md::to_json(src);
    EXPECT_TRUE(j.is_string());
    Value back = md::from_json(j);
    EXPECT_EQ(back.as_string(), "hello");
}

TEST(Json, RoundTripArray) {
    Value src{Array{Value{1}, Value{"x"}, Value{true}}};
    nlohmann::json j = md::to_json(src);
    EXPECT_TRUE(j.is_array());
    EXPECT_EQ(j.size(), 3u);
    Value back = md::from_json(j);
    EXPECT_TRUE(back.is_array());
    EXPECT_EQ(back.as_array().size(), 3u);
}

TEST(Json, RoundTripObject) {
    Object src{{"a", Value{1}}, {"b", Value{"x"}}};
    nlohmann::json j = md::to_json(src);
    EXPECT_TRUE(j.is_object());
    EXPECT_EQ(j.size(), 2u);
    Value back = md::from_json(j);
    EXPECT_TRUE(back.is_object());
    const Object& o = back.as_object();
    EXPECT_TRUE(o.contains("a"));
    EXPECT_TRUE(o.contains("b"));
}

TEST(Json, NestedRoundTrip) {
    Value src{Object{{"a", Value{Array{Value{1}, Value{Object{{"b", Value{2}}}}}}}}};
    nlohmann::json j = md::to_json(src);
    Value back = md::from_json(j);
    EXPECT_EQ(src, back);
}

TEST(Json, AdlHooks) {
    // The to_json / from_json ADL hooks let nlohmann auto-pick them up.
    Value src{Object{{"k", Value{1}}}};
    nlohmann::json j = src;      // uses to_json(json&, const Value&)
    auto back = j.get<Value>();  // uses from_json(const json&, Value&)
    EXPECT_EQ(src, back);
}

TEST(Json, FromNonObjectIntoObjectThrows) {
    nlohmann::json arr = nlohmann::json::array({1, 2});
    Object o;
    EXPECT_THROW(md::from_json(arr, o), md::type_error);
}
