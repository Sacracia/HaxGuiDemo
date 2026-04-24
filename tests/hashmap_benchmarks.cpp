#include "pch.h"

#include <map>
#include <vector>
#include <numeric>
#include <algorithm>
#include <random>
#include <chrono>
#include <unordered_map>

#include "../haxgui.h"

using namespace Hax;

#define PROFILE_BLOCK(title, ...) { \
    auto start = std::chrono::high_resolution_clock::now(); \
    __VA_ARGS__; \
    auto end = std::chrono::high_resolution_clock::now(); \
    auto dur = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count(); \
    std::cout << "[ BENCHMARK ] " << title << ": " << dur << " ns" << std::endl; \
}

#define PROFILE_SCOPE(title, ...) { \
    auto start = std::chrono::high_resolution_clock::now(); \
    __VA_ARGS__; \
    auto end = std::chrono::high_resolution_clock::now(); \
    auto dur = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count(); \
    std::cout << "[ BENCHMARK ] " << title << ": " << dur << " ns" << std::endl; \
}

class HashMapTest : public ::testing::Test 
{
protected:
    static const int N = 100;
    int keys[N];

    void SetUp() override 
    {
        for (int i = 0; i < N; ++i) keys[i] = i;
        std::mt19937 gen(42);
        std::shuffle(keys, keys + N, gen);
    }
};

TEST_F(HashMapTest, Insert_And_Search_Comparison) 
{
    std::cout << "\n--- Тестирование на N = " << N << " ---" << std::endl;

    PROFILE_SCOPE("Custom HashMap Insert", {
        HashMap<int,int> map;
        map.Reserve(N);
    for (int i = 0; i < N; ++i) map.Insert(keys[i], i);
        });

    PROFILE_SCOPE("std::unordered_map Insert", {
        std::unordered_map<int, int> stdMap;
    stdMap.reserve(N);
    for (int i = 0; i < N; ++i) stdMap[keys[i]] = i;
        });

    HashMap<int,int> myMap;
    myMap.Reserve(N);
    std::unordered_map<int, int> stdMap;
    for (int i = 0; i < N; ++i) {
        myMap.Insert(keys[i], i);
        stdMap[keys[i]] = i;
    }

    volatile int sum = 0;

    // 2. Тест поиска (обращение к состоянию виджетов в каждом кадре)
    PROFILE_SCOPE("Custom HashMap Get", {
        for (int i = 0; i < N; ++i) {
            int* v = myMap.Get(keys[i]);
            if (v) sum += *v;
        }
        });

    PROFILE_SCOPE("std::unordered_map Find", {
        for (int i = 0; i < N; ++i) {
            auto it = stdMap.find(keys[i]);
            if (it != stdMap.end()) sum += it->second;
        }
        });
}