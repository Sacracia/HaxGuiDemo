#include "pch.h"

#include "../hax.h"

using namespace Hax;

struct PlayerState 
{
    int hp;
    int armor;
    bool operator==(const PlayerState& other) const { return hp == other.hp && armor == other.armor; }
};

class MapTest : public ::testing::Test 
{
protected:
    Map<int, PlayerState> playerMap;
};

// 1. Тест базовой вставки и поиска
TEST_F(MapTest, InsertAndGet) 
{
    PlayerState s1 = { 100, 50 };
    playerMap.Insert(1, s1);

    PlayerState* result = playerMap.Get(1);
    ASSERT_NE(result, nullptr);
    EXPECT_EQ(result->hp, 100);
    EXPECT_EQ(result->armor, 50);
}

// 2. Тест обновления существующего значения
TEST_F(MapTest, UpdateExisting) 
{
    playerMap.Insert(1, { 100, 50 });
    playerMap.Insert(1, { 80, 40 }); // Обновление

    EXPECT_EQ(playerMap.Size(), 1);
    EXPECT_EQ(playerMap.Get(1)->hp, 80);
}

// 3. Тест FindOrAdd (самый важный метод)
TEST_F(MapTest, FindOrAddLogic) 
{
    // Должен создать дефолтный объект, если ключа нет
    PlayerState& s = playerMap.FindOrAdd(42);
    EXPECT_EQ(playerMap.Size(), 1);
    EXPECT_EQ(s.hp, 0); // Проверка зануления POD

    s.hp = 99; // Меняем по ссылке
    EXPECT_EQ(playerMap.Get(42)->hp, 99);
}

// 4. Тест удаления
TEST_F(MapTest, EraseElement) 
{
    playerMap.Insert(10, { 100, 100 });
    EXPECT_TRUE(playerMap.Contains(10));

    EXPECT_TRUE(playerMap.Erase(10));
    EXPECT_FALSE(playerMap.Contains(10));
    EXPECT_EQ(playerMap.Size(), 0);
}

// 6. Тест очистки
TEST_F(MapTest, ClearMap) 
{
    playerMap.Insert(1, { 1, 1 });
    playerMap.Clear();
    EXPECT_EQ(playerMap.Size(), 0);
    EXPECT_EQ(playerMap.Get(1), nullptr);
}