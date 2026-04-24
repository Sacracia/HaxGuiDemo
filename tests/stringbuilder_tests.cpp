#include "pch.h"

#include "../hax.h"

using namespace Hax;

TEST(StringBuilderTest, CharBasicOperations) 
{
    StringBuilder<32> sb;

    EXPECT_TRUE(sb.Empty());
    EXPECT_STREQ(sb.CStr(), "");

    sb.Append("Hello");
    EXPECT_EQ(sb.Length(), 5);
    EXPECT_STREQ(sb.CStr(), "Hello");

    sb.AppendF(" %s!", "World");
    EXPECT_EQ(sb.Length(), 12);
    EXPECT_STREQ(sb.CStr(), "Hello World!");

    sb.Clear();
    EXPECT_TRUE(sb.Empty());
    EXPECT_EQ(sb.Length(), 0);
    EXPECT_STREQ(sb.CStr(), "");
}

TEST(StringBuilderTest, OverflowProtection) 
{
    constexpr size_t MAX_LEN = 10;
    StringBuilder<MAX_LEN> sb;

    // Заполняем почти до конца
    sb.Append("123456789"); // 9 символов
    EXPECT_EQ(sb.Length(), 9);

    // Добавляем строку, которая не влезет полностью
    sb.Append("ABCDEF"); 
    // Должно добавиться только 'A', итого 10 символов ("123456789A")
    EXPECT_EQ(sb.Size(), MAX_LEN);
    EXPECT_STREQ(sb.CStr(), "123456789A");

    // Проверяем, что AppendF тоже не ломает память при переполнении
    sb.Clear();
    sb.AppendF("%d%d%d", 111, 222, 333); // 9 символов
    sb.AppendF("%s", "Over"); // Должна влезть только 'O'

    EXPECT_EQ(sb.Size(), MAX_LEN);
    EXPECT_STREQ(sb.CStr(), "111222333O");
}

// Тесты для широких строк (wchar_t)
TEST(StringBuilderTest, WCharBasicOperations) 
{
    WStringBuilder<64> sb;

    sb.Append(L"Win");
    sb.AppendF(L" %ls", L"API");

    EXPECT_EQ(sb.View().Size(), 7);
    EXPECT_STREQ((const char*)sb.CStr(), (const char*)L"Win API"); // Сравнение wchar_t через gtest макрос

    // Проверка сложного форматирования
    sb.Clear();
    sb.AppendF(L"Value: %.2f", 3.14159f);
    EXPECT_STREQ((const char*)sb.CStr(), (const char*)L"Value: 3.14");
}

// Тест на корректность StringView
TEST(StringBuilderTest, ViewIntegration) 
{
    StringBuilder<100> sb;
    sb.Append("DataChunk");

    EXPECT_EQ(sb.Size(), 9);
    EXPECT_EQ(sb[0], 'D');
    EXPECT_EQ(sb[8], 'k');
}