#include <iostream>
#include <list>
#include <vector>
#include <chrono>
#include <random>
#include <algorithm>
#include <iomanip>

#include "Memory/pool_allocator.hpp"
#include "Structures/list.hpp"

// Różne rozmiary testów
constexpr size_t SMALL_SIZE = 1000;
constexpr size_t MEDIUM_SIZE = 10000;
constexpr size_t LARGE_SIZE = 100000;
constexpr size_t XLARGE_SIZE = 1000000;

// Klasa do bardziej złożonych testów
struct TestObject {
    int value;
    double data[4]; // Symuluje większy obiekt

    TestObject() noexcept : value(0) {
        std::fill(std::begin(data), std::end(data), 0.0);
    }

    TestObject(int v) noexcept : value(v) {
        for (int i = 0; i < 4; ++i) {
            data[i] = v * (i + 1) * 0.1;
        }
    }

    bool operator==(const TestObject &other) const {
        return value == other.value;
    }
};

// Ulepszona funkcja benchmarkowa z większą precyzją
struct BenchmarkResult {
    double avg_time;
    double min_time;
    double max_time;
    double stddev;
};

template<typename Func>
BenchmarkResult benchmark_advanced(Func &&func, size_t repeat = 20) {
    using namespace std::chrono;
    std::vector<double> times;
    times.reserve(repeat);

    // Warm-up
    for (int i = 0; i < 3; ++i) {
        func();
    }

    for (size_t i = 0; i < repeat; ++i) {
        auto start = high_resolution_clock::now();
        func();
        auto end = high_resolution_clock::now();
        times.push_back(duration<double>(end - start).count());
    }

    double sum = 0.0;
    double min_t = times[0];
    double max_t = times[0];

    for (double t : times) {
        sum += t;
        min_t = std::min(min_t, t);
        max_t = std::max(max_t, t);
    }

    double avg = sum / repeat;
    double variance = 0.0;
    for (double t : times) {
        variance += (t - avg) * (t - avg);
    }
    variance /= repeat;

    return { avg, min_t, max_t, std::sqrt(variance) };
}

void print_comparison(const std::string &test_name,
                      const BenchmarkResult &std_result,
                      const BenchmarkResult &custom_result) {
    std::cout << "\n=== " << test_name << " ===\n";
    std::cout << std::fixed << std::setprecision(6);
    std::cout << "std::list   : " << std_result.avg_time << "s (±" << std_result.stddev << ")\n";
    std::cout << "custom List : " << custom_result.avg_time << "s (±" << custom_result.stddev << ")\n";

    double speedup = std_result.avg_time / custom_result.avg_time;
    if (speedup > 1.0) {
        std::cout << "Custom is " << speedup << "x FASTER\n";
    } else {
        std::cout << "Custom is " << (1.0 / speedup) << "x SLOWER\n";
    }
}

// Test konstrukcji i destrukcji
void test_construction_destruction(size_t size) {
    std::cout << "\n--- KONSTRUKCJA/DESTRUKCJA (" << size << " elementów) ---\n";

    auto std_result = benchmark_advanced([size] {
        std::list<int> lst;
        for (size_t i = 0; i < size; ++i) {
            lst.push_back(static_cast<int>(i));
        }
        // Destrukcja automatyczna
    });

    PoolAllocator allocator;
    allocator.initialize(size * sizeof(List<int>::Node), sizeof(List<int>::Node)); // Więcej miejsca na bezpieczeństwo
    auto custom_result = benchmark_advanced([&, size] {
        List<int> lst;
        lst.initialize(allocator.get_allocator_info());
        for (size_t i = 0; i < size; ++i) {
            lst.push_back(static_cast<int>(i));
        }
        lst.finalize();
    });

    allocator.finalize();
    print_comparison("Konstrukcja/Destrukcja", std_result, custom_result);
}

// Test wstawiania na początku
void test_front_insertion(size_t size) {
    std::cout << "\n--- WSTAWIANIE NA POCZĄTKU (" << size << " elementów) ---\n";

    auto std_result = benchmark_advanced([size] {
        std::list<int> lst;
        for (size_t i = 0; i < size; ++i) {
            lst.push_front(static_cast<int>(i));
        }
    });

    PoolAllocator allocator;
    allocator.initialize(size * sizeof(List<int>::Node), sizeof(List<int>::Node));
    auto custom_result = benchmark_advanced([&, size] {
        List<int> lst;
        lst.initialize(allocator.get_allocator_info());
        for (size_t i = 0; i < size; ++i) {
            lst.push_front(static_cast<int>(i));
        }
        lst.finalize();
    });
    allocator.finalize();

    print_comparison("Wstawianie na początku", std_result, custom_result);
}

// Test wstawiania w środku
void test_middle_insertion(size_t size) {
    std::cout << "\n--- WSTAWIANIE W ŚRODKU (" << size << " elementów) ---\n";

    auto std_result = benchmark_advanced([size] {
        std::list<int> lst;
        // Najpierw budujemy bazową listę
        for (size_t i = 0; i < size / 2; ++i) {
            lst.push_back(static_cast<int>(i));
        }

        // Potem wstawiamy w środku
        auto it = lst.begin();
        std::advance(it, size / 4);
        for (size_t i = 0; i < size / 2; ++i) {
            it = lst.insert(it, static_cast<int>(i + 1000));
            ++it; // Przesuwamy iterator żeby nie wstawiać w tym samym miejscu
        }
    });

    PoolAllocator allocator;
    allocator.initialize(size * sizeof(List<int>::Node), sizeof(List<int>::Node));
    auto custom_result = benchmark_advanced([&, size] {
        List<int> lst;
        lst.initialize(allocator.get_allocator_info());

        // Budujemy bazową listę
        for (size_t i = 0; i < size / 2; ++i) {
            lst.push_back(static_cast<int>(i));
        }

        auto it = lst.begin();
        for (size_t i = 0; i < size / 4; ++i)
        {
            ++it;
        }

        for (size_t i = 0; i < size / 2; ++i) 
        {
            lst.push(static_cast<int>(i + 1000), it);
            ++it;
        }

        lst.finalize();
    });
    allocator.finalize();

    print_comparison("Wstawianie w środku", std_result, custom_result);
}

// Test usuwania z końca
void test_back_removal(size_t size) {
    std::cout << "\n--- USUWANIE Z KOŃCA (" << size << " elementów) ---\n";

    auto std_result = benchmark_advanced([size] {
        std::list<int> lst;
        for (size_t i = 0; i < size; ++i) {
            lst.push_back(static_cast<int>(i));
        }
        while (!lst.empty()) {
            lst.pop_back();
        }
    });

    PoolAllocator allocator;
    allocator.initialize(size * sizeof(List<int>::Node), sizeof(List<int>::Node));
    auto custom_result = benchmark_advanced([&, size] {
        List<int> lst;
        lst.initialize(allocator.get_allocator_info());
        for (size_t i = 0; i < size; ++i) {
            lst.push_back(static_cast<int>(i));
        }
        while (!lst.is_empty()) {
            lst.drop_back();
        }
        lst.finalize();
    });
    allocator.finalize();

    print_comparison("Usuwanie z końca", std_result, custom_result);
}

// Test iteracji forward i backward
void test_bidirectional_iteration(size_t size) {
    std::cout << "\n--- ITERACJA DWUKIERUNKOWA (" << size << " elementów) ---\n";

    std::list<int> std_list;
    for (size_t i = 0; i < size; ++i) {
        std_list.push_back(static_cast<int>(i));
    }

    PoolAllocator allocator;
    allocator.initialize(size * sizeof(List<int>::Node), sizeof(List<int>::Node));
    List<int> custom_list;
    custom_list.initialize(allocator.get_allocator_info());
    for (size_t i = 0; i < size; ++i) {
        custom_list.push_back(static_cast<int>(i));
    }

    auto std_result = benchmark_advanced([&std_list] {
        volatile long long sum = 0;
        // Forward
        for (auto it = std_list.begin(); it != std_list.end(); ++it) {
            sum += *it;
        }
        // Backward
        for (auto it = std_list.rbegin(); it != std_list.rend(); ++it) {
            sum += *it;
        }
    });

    auto custom_result = benchmark_advanced([&custom_list] {
        volatile long long sum = 0;
        // Forward
        for (auto it = custom_list.begin(); it != custom_list.end(); ++it) {
            sum += *it;
        }
        // Backward (symulujemy reverse iterator)
        auto it = custom_list.end();
        while (it != custom_list.begin()) {
            --it;
            sum += *it;
        }
    });

    print_comparison("Iteracja dwukierunkowa", std_result, custom_result);

    custom_list.finalize();
    allocator.finalize();
}

// Test dostępu losowego (słabość list)
void test_random_access(size_t size) {
    std::cout << "\n--- DOSTĘP LOSOWY (" << size << " elementów, 1000 dostępów) ---\n";

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<size_t> dis(0, size - 1);

    std::vector<size_t> indices;
    for (int i = 0; i < 1000; ++i) {
        indices.push_back(dis(gen));
    }

    std::list<int> std_list;
    for (size_t i = 0; i < size; ++i) {
        std_list.push_back(static_cast<int>(i));
    }

    PoolAllocator allocator;
    allocator.initialize(size * sizeof(List<int>::Node), sizeof(List<int>::Node));
    List<int> custom_list;
    custom_list.initialize(allocator.get_allocator_info());
    for (size_t i = 0; i < size; ++i) {
        custom_list.push_back(static_cast<int>(i));
    }

    auto std_result = benchmark_advanced([&std_list, &indices] {
        volatile long long sum = 0;
        for (size_t idx : indices) {
            auto it = std_list.begin();
            std::advance(it, idx);
            sum += *it;
        }
    });

    auto custom_result = benchmark_advanced([&custom_list, &indices] {
        volatile long long sum = 0;
        for (size_t idx : indices) {
            sum += custom_list[idx]; // Twoja optymalizacja z połową drogi
        }
    });

    print_comparison("Dostęp losowy", std_result, custom_result);

    custom_list.finalize();
    allocator.finalize();
}

// Test z większymi obiektami
void test_large_objects(size_t size) {
    std::cout << "\n--- DUŻE OBIEKTY (" << size << " elementów) ---\n";

    auto std_result = benchmark_advanced([size] {
        std::list<TestObject> lst;
        for (size_t i = 0; i < size; ++i) {
            lst.push_back(TestObject(static_cast<int>(i)));
        }

        volatile long long sum = 0;
        for (const auto &obj : lst) {
            sum += obj.value;
        }
    });

    PoolAllocator allocator;
    allocator.initialize(size * sizeof(List<TestObject>::Node), sizeof(List<TestObject>::Node)); // Większe obiekty
    auto custom_result = benchmark_advanced([&, size] {
        List<TestObject> lst;
        lst.initialize(allocator.get_allocator_info());
        for (size_t i = 0; i < size; ++i) {
            lst.push_back(TestObject(static_cast<int>(i)));
        }

        volatile long long sum = 0;
        for (auto it = lst.begin(); it != lst.end(); ++it) {
            sum += it->value;
        }

        lst.finalize();
    });
    allocator.finalize();

    print_comparison("Duże obiekty", std_result, custom_result);
}

// Test wyszukiwania
void test_search_performance(size_t size) {
    std::cout << "\n--- WYSZUKIWANIE (" << size << " elementów, 100 wyszukiwań) ---\n";

    std::list<int> std_list;
    for (size_t i = 0; i < size; ++i) {
        std_list.push_back(static_cast<int>(i));
    }

    PoolAllocator allocator;
    allocator.initialize(size * sizeof(List<int>::Node), sizeof(List<int>::Node));
    List<int> custom_list;
    custom_list.initialize(allocator.get_allocator_info());
    for (size_t i = 0; i < size; ++i) {
        custom_list.push_back(static_cast<int>(i));
    }

    std::vector<int> search_values;
    for (int i = 0; i < 100; ++i) {
        search_values.push_back(static_cast<int>(size - 1 - i)); // Szukamy od końca
    }

    auto std_result = benchmark_advanced([&std_list, &search_values] {
        volatile int found_count = 0;
        for (int val : search_values) {
            auto it = std::find(std_list.begin(), std_list.end(), val);
            if (it != std_list.end()) {
                found_count++;
            }
        }
    });

    auto custom_result = benchmark_advanced([&custom_list, &search_values] {
        volatile int found_count = 0;
        for (int val : search_values) {
            if (custom_list.contains(val)) {
                found_count++;
            }
        }
    });

    print_comparison("Wyszukiwanie", std_result, custom_result);

    custom_list.finalize();
    allocator.finalize();
}

// Test fragmentacji pamięci
void test_memory_fragmentation() {
    std::cout << "\n--- TEST FRAGMENTACJI PAMIĘCI ---\n";

    const size_t ops = 10000;

    auto std_result = benchmark_advanced([ops] {
        std::list<int> lst;
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, 2);

        for (size_t i = 0; i < ops; ++i) {
            int op = dis(gen);
            if (op == 0 && lst.size() < 1000) {
                lst.push_back(static_cast<int>(i));
            } else if (op == 1 && lst.size() < 1000) {
                lst.push_front(static_cast<int>(i));
            } else if (!lst.empty()) {
                if (lst.size() % 2 == 0) {
                    lst.pop_back();
                } else {
                    lst.pop_front();
                }
            }
        }
    });

    PoolAllocator allocator;
    allocator.initialize(100000, sizeof(List<int>::Node));
    auto custom_result = benchmark_advanced([&, ops] {
        List<int> lst;
        lst.initialize(allocator.get_allocator_info());

        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, 2);

        for (size_t i = 0; i < ops; ++i) {
            int op = dis(gen);
            if (op == 0 && lst.get_size() < 1000) {
                lst.push_back(static_cast<int>(i));
            } else if (op == 1 && lst.get_size() < 1000) {
                lst.push_front(static_cast<int>(i));
            } else if (!lst.is_empty()) {
                if (lst.get_size() % 2 == 0) {
                    lst.drop_back();
                } else {
                    lst.drop_front();
                }
            }
        }

        lst.finalize();
    });
    allocator.finalize();

    print_comparison("Fragmentacja pamięci", std_result, custom_result);
}

int main() {
    std::cout << "=== ZAAWANSOWANE PORÓWNANIE std::list vs custom List ===\n";

    // Testy z różnymi rozmiarami
    test_construction_destruction(SMALL_SIZE);
    test_construction_destruction(MEDIUM_SIZE);
    test_construction_destruction(LARGE_SIZE);

    test_front_insertion(MEDIUM_SIZE);
    test_middle_insertion(SMALL_SIZE); // Mniejszy rozmiar bo O(n²)

    test_back_removal(MEDIUM_SIZE);
    test_bidirectional_iteration(LARGE_SIZE);

    test_random_access(MEDIUM_SIZE);
    test_large_objects(SMALL_SIZE);
    test_search_performance(MEDIUM_SIZE);

    test_memory_fragmentation();

    std::cout << "\n=== PODSUMOWANIE ===\n";
    std::cout << "1. Pool allocator powinien być szybszy w alokacji/dealokacji\n";
    std::cout << "2. Twoja implementacja ma optymalizację dostępu losowego\n";
    std::cout << "3. Fragmentacja pamięci powinna być mniejsza\n";
    std::cout << "4. Cache locality może być lepsza dzięki pool allocator\n";

    return 0;
}