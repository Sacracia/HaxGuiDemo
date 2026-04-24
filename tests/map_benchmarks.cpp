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
    std::cout << "[ BENCHMARK ] " << title << ": " << dur << " ns total" << std::endl; \
}

class MapBenchmark : public ::testing::Test {
protected:
    static const int N = 100; 
    int keys[N];

    void SetUp() override {
        for (int i = 0; i < N; ++i) keys[i] = i;
        std::mt19937 gen(42);
        std::shuffle(keys, keys + N, gen);
    }
};

TEST_F(MapBenchmark, Insertion_Comparison) {
    std::cout << "\n--- Insertion (N=" << N << ") ---" << std::endl;

    // 1. Твой Map (HaxMap)
    PROFILE_BLOCK("HaxMap (Reserve + Insert)", {
        Map<int, int> hax;
    hax.Reserve(N);
    for (int i = 0; i < N; ++i) hax.Insert(keys[i], i);
        });

    // 3. std::map
    PROFILE_BLOCK("std::map (Insert)", {
        std::map<int, int> s_map;
    for (int i = 0; i < N; ++i) s_map[keys[i]] = i;
        });
}

TEST_F(MapBenchmark, Search_Comparison) {
    // Подготовка данных вне замера
    Map<int, int> hax; hax.Reserve(N);
    std::unordered_map<int, int> u_map;
    std::map<int, int> s_map;

    for (int i = 0; i < N; ++i) {
        hax.Insert(keys[i], i);
        u_map[keys[i]] = i;
        s_map[keys[i]] = i;
    }

    volatile int sum = 0;
    std::cout << "\n--- Search Latency (1000 iterations over N=" << N << ") ---" << std::endl;

    PROFILE_BLOCK("HaxMap Search", {
            for (int i = 0; i < N; ++i) {
                int* v = hax.Get(keys[i]);
                if (v) sum += *v;
            }
        });

    PROFILE_BLOCK("std::map Search", {
            for (int i = 0; i < N; ++i) {
                auto res = s_map.find(keys[i]);
                if (res != s_map.end()) sum += res->second;
            }
        });
}