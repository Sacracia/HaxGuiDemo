#include "pch.h"

#include <map>
#include <vector>
#include <numeric>
#include <algorithm>
#include <random>
#include <chrono>

#include "../haxgui.h"

using namespace Hax;

class MapPerfTest : public ::testing::Test {
protected:
    const int N = 1000;
    const int Iterations = 1000;
    std::vector<int> keys;

    void SetUp() override {
        std::mt19937 gen(42);
        for (int i = 0; i < N; ++i) keys.push_back(i);
        std::shuffle(keys.begin(), keys.end(), gen);
    }
};

TEST_F(MapPerfTest, Benchmark_HaxMap_Insert) {
    Map<int, int> myMap;
    myMap.Reserve(N);
    for (int k : keys) {
        myMap.Insert(k, k);
    }
}

TEST_F(MapPerfTest, Benchmark_StdMap_Insert) {
    std::map<int, int> stdMap;
    for (int k : keys) {
        stdMap[k] = k;
    }
}

TEST_F(MapPerfTest, PureSearch_HaxMap) {
    Map<int, int> myMap;
    myMap.Reserve(N);
    for (int k : keys) myMap.Insert(k, k);

    volatile int sum = 0;
    for (int i = 0; i < Iterations; ++i) {
        for (int k : keys) {
            int* v = myMap.Get(k);
            if (v) sum += *v;
        }
    }
}

TEST_F(MapPerfTest, PureSearch_StdMap) {
    std::map<int, int> stdMap;
    for (int k : keys) stdMap[k] = k;

    volatile int sum = 0;
    for (int i = 0; i < Iterations; ++i) {
        for (int k : keys) {
            auto it = stdMap.find(k);
            if (it != stdMap.end()) sum += it->second;
        }
    }
}

TEST_F(MapPerfTest, Benchmark_HaxMap_Erase) {
    Map<int, int> myMap;
    for (int k : keys) myMap.Insert(k, k);

    for (int k : keys) {
        myMap.Erase(k);
    }
}

TEST_F(MapPerfTest, Benchmark_StdMap_Erase) {
    std::map<int, int> stdMap;
    for (int k : keys) stdMap[k] = k;

    for (int k : keys) {
        stdMap.erase(k);
    }
}