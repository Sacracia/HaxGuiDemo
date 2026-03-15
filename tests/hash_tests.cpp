#include "pch.h"

#include "../hax.h"

using namespace Hax;

TEST(HashTest, ConstexprSafety) 
{
    static_assert(GetTypeId<int>() != GetTypeId<float>(), "Collision between int and float!");
    static_assert(GetTypeId<int>() == GetTypeId<int>(), "Deterministic failure for int!");

    static constexpr size_t h1 = Hash(L"UnityObject");
    static constexpr size_t h2 = Hash(L"UnityObject");
    static_assert(h1 == h2, "Hash must be deterministic");
}

class HashRuntimeTest : public ::testing::Test 
{
protected:
    Allocator& alloc = g_GlobalAlloc;
};

TEST_F(HashRuntimeTest, StringConsistency) 
{
    const wchar_t* raw = L"SDK_Test_String";
    String str(raw, alloc);
    StringView view(raw);

    // Все три вида представления одной строки должны давать идентичный хеш
    EXPECT_EQ(Hash(raw), Hash(str));
    EXPECT_EQ(Hash(str), Hash(view));
    EXPECT_EQ(Hash(raw), Hash(view));
}

TEST_F(HashRuntimeTest, CaseSensitivity) 
{
    EXPECT_NE(Hash(L"Player"), Hash(L"player"));
}

TEST_F(HashRuntimeTest, TriCoStructures) 
{
    struct MyState 
    {
        int id;
        float health;
    };
    // Убеждаемся, что концепт TriCo работает
    static_assert(TriCo<MyState>);

    MyState s1{ 42, 100.0f };
    MyState s2{ 42, 100.0f };
    MyState s3{ 43, 100.0f };

    EXPECT_EQ(Hash(s1), Hash(s2));
    EXPECT_NE(Hash(s1), Hash(s3));
}