#include <gtest/gtest.h>
#include <vix/json/json.hpp>
#include <string>

using Vix::json::jget;
using Vix::json::jset;
using Vix::json::Json;

TEST(JPath, SetGet_SimpleObjectKey)
{
    Json j = Json::object();
    EXPECT_TRUE(jset(j, "user.profile.name", std::string("Ada")));
    auto *v = jget(j, "user.profile.name");
    ASSERT_NE(v, nullptr);
    EXPECT_TRUE(v->is_string());
    EXPECT_EQ(*v, "Ada");
}

TEST(JPath, SetGet_ArrayIndex_FillsWithNulls)
{
    Json j = Json::object();
    EXPECT_TRUE(jset(j, "user.langs[2]", std::string("cpp"))); // [null,null,"cpp"]
    const Json *arr = jget(j, "user.langs");
    ASSERT_NE(arr, nullptr);
    ASSERT_TRUE(arr->is_array());
    ASSERT_EQ(arr->size(), 3u);
    EXPECT_TRUE((*arr)[0].is_null());
    EXPECT_TRUE((*arr)[1].is_null());
    EXPECT_EQ((*arr)[2], "cpp");

    // Read a const index
    const Json *v = jget(j, "user.langs[2]");
    ASSERT_NE(v, nullptr);
    EXPECT_EQ(*v, "cpp");
}

TEST(JPath, GetConst_ReturnsNullOnMissingOrWrongType)
{
    Json j = Json::object();
    // Missing key
    EXPECT_EQ(jget(j, "user.id"), nullptr);

    // Bad type (index on an object → nullptr)
    EXPECT_TRUE(jset(j, "meta", Json::object()));
    EXPECT_EQ(jget(j, "meta[0]"), nullptr);
}

TEST(JPath, Tokenize_Throws_OnEmptyIndex)
{
    Json j = Json::object();
    // [] -> empty index → ​​tokenize_path must raise → jset returns false
    EXPECT_FALSE(jset(j, "a[]", 123));
}

TEST(JPath, Set_NegativeIndexReturnsFalse)
{
    Json j = Json::object();
    // [-1] → tokenizer raises → jset catches and returns false
    EXPECT_FALSE(jset(j, "arr[-1]", 7));
}

TEST(JPath, GetConst_ArrayBounds_ReturnsNull)
{
    Json j = Json::object();
    EXPECT_TRUE(jset(j, "a[1]", 42));    // [null,42]
    EXPECT_EQ(jget(j, "a[2]"), nullptr); // out of bounds
}

TEST(JPath, Mutator_CreatesNestedContainers)
{
    Json j = Json::array(); // intentionally not an object at the start
    EXPECT_TRUE(jset(j, "root.child[1].name", std::string("Bob")));
    // The mutator must force the right types along the way
    const Json *v = jget(j, "root.child[1].name");
    ASSERT_NE(v, nullptr);
    EXPECT_EQ(*v, "Bob");
}
