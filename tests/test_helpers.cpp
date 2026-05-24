#include <gtest/gtest.h>

#include <md/metadata.hpp>

#include <string>

using md::Array;
using md::Object;
using md::Value;

TEST(Helpers, ContainsAndFindPtr_Methods) {
    Object o{{"a", Value{1}}};
    EXPECT_TRUE(o.contains("a"));
    EXPECT_FALSE(o.contains("b"));
    EXPECT_NE(o.find_ptr("a"), nullptr);
    EXPECT_EQ(o.find_ptr("b"), nullptr);
}

TEST(Helpers, ContainsAndFindPtr_FreeFunctions) {
    Object o{{"a", Value{1}}};
    EXPECT_TRUE(md::contains(o, "a"));
    EXPECT_FALSE(md::contains(o, "b"));
    EXPECT_NE(md::find_ptr(o, "a"), nullptr);
    EXPECT_EQ(md::find_ptr(o, "b"), nullptr);
}

TEST(Helpers, Require_ThrowsOnMissing) {
    Object o;
    EXPECT_THROW((void)o.require("k"), md::missing_key_error);
    EXPECT_THROW((void)md::require(o, "k"), md::missing_key_error);
}

TEST(Helpers, Require_Returns) {
    Object o{{"k", Value{42}}};
    EXPECT_EQ(o.require("k").as_int(), 42);
    EXPECT_EQ(md::require(o, "k").as_int(), 42);
}

TEST(Helpers, RequireString) {
    Object o{{"s", Value{"hi"}}, {"n", Value{1}}};
    EXPECT_EQ(o.require_string("s"), "hi");
    EXPECT_THROW((void)o.require_string("n"), md::type_error);
    EXPECT_THROW((void)o.require_string("missing"), md::missing_key_error);

    EXPECT_EQ(md::require_string(o, "s"), "hi");
    EXPECT_THROW((void)md::require_string(o, "n"), md::type_error);
}

TEST(Helpers, RequireArray) {
    Object o{{"a", Value{Array{Value{1}}}}, {"n", Value{1}}};
    EXPECT_EQ(o.require_array("a").size(), 1u);
    EXPECT_THROW((void)o.require_array("n"), md::type_error);
    EXPECT_EQ(md::require_array(o, "a").size(), 1u);
}

TEST(Helpers, RequireObject) {
    Object inner{{"k", Value{1}}};
    Object o{{"o", Value{inner}}, {"n", Value{1}}};
    EXPECT_TRUE(o.require_object("o").contains("k"));
    EXPECT_THROW((void)o.require_object("n"), md::type_error);
    EXPECT_TRUE(md::require_object(o, "o").contains("k"));
}

TEST(Helpers, GetIfFamily) {
    Object o{{"s", Value{"hi"}},
             {"a", Value{Array{Value{1}}}},
             {"o", Value{Object{{"k", Value{1}}}}},
             {"n", Value{42}}};

    EXPECT_NE(o.get_string_if("s"), nullptr);
    EXPECT_EQ(o.get_string_if("n"), nullptr);
    EXPECT_EQ(o.get_string_if("missing"), nullptr);

    EXPECT_NE(o.get_array_if("a"), nullptr);
    EXPECT_EQ(o.get_array_if("n"), nullptr);

    EXPECT_NE(o.get_object_if("o"), nullptr);
    EXPECT_EQ(o.get_object_if("n"), nullptr);

    EXPECT_NE(md::get_string_if(o, "s"), nullptr);
    EXPECT_NE(md::get_array_if(o, "a"), nullptr);
    EXPECT_NE(md::get_object_if(o, "o"), nullptr);
}
