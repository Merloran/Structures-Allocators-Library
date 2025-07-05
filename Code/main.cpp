#include <iostream>
#include <chrono>
#include <unordered_map>
#include <vector>
#include <random>
#include <string>
#include <iomanip>

#include "Memory/freelist_allocator.hpp"
#include "Memory/pool_allocator.hpp"
#include "Memory/stack_allocator.hpp"
#include "Structures/hash_map.hpp"
#include "Structures/string.hpp"

// Klasa do mierzenia czasu
class Timer {
private:
    std::chrono::high_resolution_clock::time_point start_time;

public:
    void start() {
        start_time = std::chrono::high_resolution_clock::now();
    }

    double elapsed_ms() const {
        auto end_time = std::chrono::high_resolution_clock::now();
        return std::chrono::duration<double, std::milli>(end_time - start_time).count();
    }
};

// Klasa do testów wydajnościowych
class HashMapBenchmark {
private:
    static constexpr size_t BENCHMARK_ITERATIONS = 10;

    std::mt19937 rng;
    std::uniform_int_distribution<int> int_dist;
    std::uniform_int_distribution<size_t> string_length_dist;

    std::vector<int> test_keys_int;
    std::vector<std::string> test_keys_string;
    std::vector<String> test_keys_custom_string;
    std::vector<int> test_values;

    // Alokatory specjalne dla różnych testów
    FreeListAllocator string_allocator;
    PoolAllocator int_node_allocator;
    PoolAllocator string_node_allocator;
    StackAllocator bucket_allocator;

public:
    HashMapBenchmark() : rng(42), int_dist(0, 1000000), string_length_dist(5, 20) {}

    void initialize_allocators(size_t max_elements) {
        // Alokator dla stringów
        string_allocator.initialize(max_elements * 50); // Szacowana średnia długość stringa * max elementów

        // Alokator dla node'ów int->int (rozmiar bloku = rozmiar Node)
        int_node_allocator.initialize(max_elements * sizeof(HashMap<int, int>::Node), sizeof(HashMap<int, int>::Node));

        // Alokator dla node'ów String->int (rozmiar bloku = rozmiar Node)
        string_node_allocator.initialize(max_elements * sizeof(HashMap<String, int>::Node), sizeof(HashMap<String, int>::Node));

        // Alokator dla bucketów
        bucket_allocator.initialize(max_elements * sizeof(void *) * 4); // Zapas na rehashing
    }

    void finalize_allocators() {
        string_allocator.finalize();
        int_node_allocator.finalize();
        string_node_allocator.finalize();
        bucket_allocator.finalize();
    }

    void generate_test_data(size_t count) {
        test_keys_int.clear();
        test_keys_string.clear();
        test_keys_custom_string.clear();
        test_values.clear();

        test_keys_int.reserve(count);
        test_keys_string.reserve(count);
        test_keys_custom_string.reserve(count);
        test_values.reserve(count);

        for (size_t i = 0; i < count; ++i) {
            test_keys_int.push_back(int_dist(rng));
            test_values.push_back(int_dist(rng));

            // Generowanie losowego std::string
            size_t length = string_length_dist(rng);
            std::string str;
            str.reserve(length);
            for (size_t j = 0; j < length; ++j) {
                str += static_cast<char>('a' + (rng() % 26));
            }
            test_keys_string.push_back(str);

            // Generowanie losowego własnego String
            String custom_str;
            custom_str.initialize(str.c_str(), string_allocator.get_allocator_info());
            test_keys_custom_string.push_back(std::move(custom_str));
        }
    }

    void cleanup_custom_strings() {
        for (auto &str : test_keys_custom_string) {
            str.finalize();
        }
        test_keys_custom_string.clear();
    }

    // Benchmarki dla std::unordered_map
    template<typename MapType>
    double benchmark_insertion(MapType &map, const std::vector<typename MapType::key_type> &keys) {
        Timer timer;
        timer.start();

        for (size_t i = 0; i < keys.size(); ++i) {
            map[keys[i]] = test_values[i];
        }

        return timer.elapsed_ms();
    }

    template<typename MapType>
    double benchmark_lookup(const MapType &map, const std::vector<typename MapType::key_type> &keys) {
        Timer timer;
        timer.start();

        volatile int sink = 0;
        for (const auto &key : keys) {
            auto it = map.find(key);
            if (it != map.end()) {
                sink += it->second;
            }
        }

        return timer.elapsed_ms();
    }

    template<typename MapType>
    double benchmark_deletion(MapType &map, const std::vector<typename MapType::key_type> &keys) {
        Timer timer;
        timer.start();

        for (const auto &key : keys) {
            map.erase(key);
        }

        return timer.elapsed_ms();
    }

    // Benchmarki dla własnej HashMap<int, int>
    double benchmark_insertion_custom_int(HashMap<int, int> &map, const std::vector<int> &keys) {
        Timer timer;
        timer.start();

        for (size_t i = 0; i < keys.size(); ++i) {
            map[keys[i]] = test_values[i];
        }

        return timer.elapsed_ms();
    }

    double benchmark_lookup_custom_int(const HashMap<int, int> &map, const std::vector<int> &keys) {
        Timer timer;
        timer.start();

        volatile int sink = 0;
        for (const auto &key : keys) {
            auto it = map.find(key);
            if (it != map.end()) {
                sink += *it;
            }
        }

        return timer.elapsed_ms();
    }

    double benchmark_deletion_custom_int(HashMap<int, int> &map, const std::vector<int> &keys) {
        Timer timer;
        timer.start();

        for (const auto &key : keys) {
            map.remove(key);
        }

        return timer.elapsed_ms();
    }

    // Benchmarki dla własnej HashMap<String, int>
    double benchmark_insertion_custom_string(HashMap<String, int> &map, const std::vector<String> &keys) {
        Timer timer;
        timer.start();

        for (size_t i = 0; i < keys.size(); ++i) {
            map[keys[i]] = test_values[i];
        }

        return timer.elapsed_ms();
    }

    double benchmark_lookup_custom_string(const HashMap<String, int> &map, const std::vector<String> &keys) {
        Timer timer;
        timer.start();

        volatile int sink = 0;
        for (const auto &key : keys) {
            auto it = map.find(key);
            if (it != map.end()) {
                sink += *it;
            }
        }

        return timer.elapsed_ms();
    }

    double benchmark_deletion_custom_string(HashMap<String, int> &map, const std::vector<String> &keys) {
        Timer timer;
        timer.start();

        for (const auto &key : keys) {
            map.remove(key);
        }

        return timer.elapsed_ms();
    }

    void run_int_benchmark(size_t data_size) {
        std::cout << "\n=== Benchmark dla int -> int (rozmiar: " << data_size << ") ===\n";

        generate_test_data(data_size);

        double custom_insert_time = 0;
        double custom_lookup_time = 0;
        double custom_delete_time = 0;

        double std_insert_time = 0;
        double std_lookup_time = 0;
        double std_delete_time = 0;

        for (size_t iter = 0; iter < BENCHMARK_ITERATIONS; ++iter) {
            // Test własnej HashMap z dedykowanymi alokatorami
            HashMap<int, int> custom_map;
            custom_map.initialize(data_size,
                                  int_node_allocator.get_allocator_info(),
                                  bucket_allocator.get_allocator_info());

            custom_insert_time += benchmark_insertion_custom_int(custom_map, test_keys_int);
            custom_lookup_time += benchmark_lookup_custom_int(custom_map, test_keys_int);
            custom_delete_time += benchmark_deletion_custom_int(custom_map, test_keys_int);

            custom_map.finalize();

            // Test std::unordered_map
            std::unordered_map<int, int> std_map;
            std_map.reserve(data_size);

            std_insert_time += benchmark_insertion(std_map, test_keys_int);
            std_lookup_time += benchmark_lookup(std_map, test_keys_int);
            std_delete_time += benchmark_deletion(std_map, test_keys_int);
        }

        // Średnie czasy
        custom_insert_time /= BENCHMARK_ITERATIONS;
        custom_lookup_time /= BENCHMARK_ITERATIONS;
        custom_delete_time /= BENCHMARK_ITERATIONS;

        std_insert_time /= BENCHMARK_ITERATIONS;
        std_lookup_time /= BENCHMARK_ITERATIONS;
        std_delete_time /= BENCHMARK_ITERATIONS;

        // Wyświetlenie wyników
        std::cout << std::fixed << std::setprecision(2);
        std::cout << "Operacja       | Własna HashMap | std::unordered_map | Przyspieszenie\n";
        std::cout << "---------------|----------------|--------------------|--------------\n";
        std::cout << "Wstawianie     | " << std::setw(10) << custom_insert_time << " ms | "
            << std::setw(14) << std_insert_time << " ms | "
            << std::setw(10) << (custom_insert_time > 0 ? std_insert_time / custom_insert_time : 0.0) << "x\n";
        std::cout << "Wyszukiwanie   | " << std::setw(10) << custom_lookup_time << " ms | "
            << std::setw(14) << std_lookup_time << " ms | "
            << std::setw(10) << (custom_lookup_time > 0 ? std_lookup_time / custom_lookup_time : 0.0) << "x\n";
        std::cout << "Usuwanie       | " << std::setw(10) << custom_delete_time << " ms | "
            << std::setw(14) << std_delete_time << " ms | "
            << std::setw(10) << (custom_delete_time > 0 ? std_delete_time / custom_delete_time : 0.0) << "x\n";

        double total_custom = custom_insert_time + custom_lookup_time + custom_delete_time;
        double total_std = std_insert_time + std_lookup_time + std_delete_time;

        std::cout << "CAŁKOWITY CZAS | " << std::setw(10) << total_custom << " ms | "
            << std::setw(14) << total_std << " ms | "
            << std::setw(10) << (total_custom > 0 ? total_std / total_custom : 0.0) << "x\n";

        std::cout << "Alokatory:\n";
        std::cout << "  Node allocator (Pool): blok " << sizeof(HashMap<int, int>::Node) << " bajtów\n";
        std::cout << "  Bucket allocator (Stack): " << bucket_allocator.get_capacity() << " bajtów\n";
    }

    void run_string_benchmark(size_t data_size) {
        std::cout << "\n=== Benchmark dla String -> int (rozmiar: " << data_size << ") ===\n";

        generate_test_data(data_size);

        double custom_insert_time = 0;
        double custom_lookup_time = 0;
        double custom_delete_time = 0;

        double std_insert_time = 0;
        double std_lookup_time = 0;
        double std_delete_time = 0;

        for (size_t iter = 0; iter < BENCHMARK_ITERATIONS; ++iter) {
            // Test własnej HashMap<String, int>
            HashMap<String, int> custom_map;
            custom_map.initialize(data_size,
                                  string_node_allocator.get_allocator_info(),
                                  bucket_allocator.get_allocator_info());

            custom_insert_time += benchmark_insertion_custom_string(custom_map, test_keys_custom_string);
            custom_lookup_time += benchmark_lookup_custom_string(custom_map, test_keys_custom_string);
            custom_delete_time += benchmark_deletion_custom_string(custom_map, test_keys_custom_string);

            custom_map.finalize();

            // Test std::unordered_map<std::string, int>
            std::unordered_map<std::string, int> std_map;
            std_map.reserve(data_size);

            std_insert_time += benchmark_insertion(std_map, test_keys_string);
            std_lookup_time += benchmark_lookup(std_map, test_keys_string);
            std_delete_time += benchmark_deletion(std_map, test_keys_string);
        }

        // Średnie czasy
        custom_insert_time /= BENCHMARK_ITERATIONS;
        custom_lookup_time /= BENCHMARK_ITERATIONS;
        custom_delete_time /= BENCHMARK_ITERATIONS;

        std_insert_time /= BENCHMARK_ITERATIONS;
        std_lookup_time /= BENCHMARK_ITERATIONS;
        std_delete_time /= BENCHMARK_ITERATIONS;

        // Wyświetlenie wyników
        std::cout << std::fixed << std::setprecision(2);
        std::cout << "Operacja       | Własna HashMap | std::unordered_map | Przyspieszenie\n";
        std::cout << "---------------|----------------|--------------------|--------------\n";
        std::cout << "Wstawianie     | " << std::setw(10) << custom_insert_time << " ms | "
            << std::setw(14) << std_insert_time << " ms | "
            << std::setw(10) << (custom_insert_time > 0 ? std_insert_time / custom_insert_time : 0.0) << "x\n";
        std::cout << "Wyszukiwanie   | " << std::setw(10) << custom_lookup_time << " ms | "
            << std::setw(14) << std_lookup_time << " ms | "
            << std::setw(10) << (custom_lookup_time > 0 ? std_lookup_time / custom_lookup_time : 0.0) << "x\n";
        std::cout << "Usuwanie       | " << std::setw(10) << custom_delete_time << " ms | "
            << std::setw(14) << std_delete_time << " ms | "
            << std::setw(10) << (custom_delete_time > 0 ? std_delete_time / custom_delete_time : 0.0) << "x\n";

        double total_custom = custom_insert_time + custom_lookup_time + custom_delete_time;
        double total_std = std_insert_time + std_lookup_time + std_delete_time;

        std::cout << "CAŁKOWITY CZAS | " << std::setw(10) << total_custom << " ms | "
            << std::setw(14) << total_std << " ms | "
            << std::setw(10) << (total_custom > 0 ? total_std / total_custom : 0.0) << "x\n";

        std::cout << "Alokatory:\n";
        std::cout << "  String allocator (FreeList): " << string_allocator.get_capacity() << " bajtów\n";
        std::cout << "  Node allocator (Pool): blok " << sizeof(HashMap<String, int>::Node) << " bajtów\n";
        std::cout << "  Bucket allocator (Stack): " << bucket_allocator.get_capacity() << " bajtów\n";

        cleanup_custom_strings();
    }

    void run_memory_usage_test(size_t data_size) {
        std::cout << "\n=== Test zużycia pamięci (rozmiar: " << data_size << ") ===\n";

        generate_test_data(data_size);

        std::cout << "Rozmiary struktur:\n";
        std::cout << "  HashMap<int, int>::Node: " << sizeof(HashMap<int, int>::Node) << " bajtów\n";
        std::cout << "  HashMap<String, int>::Node: " << sizeof(HashMap<String, int>::Node) << " bajtów\n";
        std::cout << "  std::pair<const int, int>: " << sizeof(std::pair<const int, int>) << " bajtów\n";
        std::cout << "  std::pair<const std::string, int>: " << sizeof(std::pair<const std::string, int>) << " bajtów\n";

        // Test HashMap<int, int>
        HashMap<int, int> custom_int_map;
        custom_int_map.initialize(data_size,
                                  int_node_allocator.get_allocator_info(),
                                  bucket_allocator.get_allocator_info());

        for (size_t i = 0; i < data_size && i < test_keys_int.size(); ++i) {
            custom_int_map[test_keys_int[i]] = test_values[i];
        }

        std::cout << "\nHashMap<int, int>:\n";
        std::cout << "  Rozmiar: " << custom_int_map.get_size() << " elementów\n";
        std::cout << "  Pojemność: " << custom_int_map.get_capacity() << " buckets\n";
        std::cout << "  Load factor: " << custom_int_map.get_load_factor() << "\n";

        // Test HashMap<String, int>
        HashMap<String, int> custom_string_map;
        custom_string_map.initialize(data_size,
                                     string_node_allocator.get_allocator_info(),
                                     bucket_allocator.get_allocator_info());

        for (size_t i = 0; i < data_size && i < test_keys_custom_string.size(); ++i) {
            custom_string_map[test_keys_custom_string[i]] = test_values[i];
        }

        std::cout << "\nHashMap<String, int>:\n";
        std::cout << "  Rozmiar: " << custom_string_map.get_size() << " elementów\n";
        std::cout << "  Pojemność: " << custom_string_map.get_capacity() << " buckets\n";
        std::cout << "  Load factor: " << custom_string_map.get_load_factor() << "\n";

        // Test std::unordered_map
        std::unordered_map<int, int> std_int_map;
        std::unordered_map<std::string, int> std_string_map;

        for (size_t i = 0; i < data_size && i < test_keys_int.size(); ++i) {
            std_int_map[test_keys_int[i]] = test_values[i];
            std_string_map[test_keys_string[i]] = test_values[i];
        }

        std::cout << "\nstd::unordered_map<int, int>:\n";
        std::cout << "  Rozmiar: " << std_int_map.size() << " elementów\n";
        std::cout << "  Bucket count: " << std_int_map.bucket_count() << "\n";
        std::cout << "  Load factor: " << std_int_map.load_factor() << "\n";

        std::cout << "\nstd::unordered_map<std::string, int>:\n";
        std::cout << "  Rozmiar: " << std_string_map.size() << " elementów\n";
        std::cout << "  Bucket count: " << std_string_map.bucket_count() << "\n";
        std::cout << "  Load factor: " << std_string_map.load_factor() << "\n";

        custom_int_map.finalize();
        custom_string_map.finalize();
        cleanup_custom_strings();
    }

    void run_collision_test() {
        std::cout << "\n=== Test kolizji hash'y ===\n";

        // Generowanie kluczy, które mogą powodować kolizje
        std::vector<int> collision_keys;
        for (int i = 0; i < 1000; ++i) {
            collision_keys.push_back(i * 32); // Klucze o regularnym odstępie
        }

        // Reset alokatorów dla małego testu
        PoolAllocator small_node_allocator;
        StackAllocator small_bucket_allocator;
        small_node_allocator.initialize(1000 * sizeof(HashMap<int, int>::Node), sizeof(HashMap<int, int>::Node));
        small_bucket_allocator.initialize(4000 * sizeof(void *));

        HashMap<int, int> custom_map;
        custom_map.initialize(32,
                              small_node_allocator.get_allocator_info(),
                              small_bucket_allocator.get_allocator_info());

        Timer timer;
        timer.start();

        for (size_t i = 0; i < collision_keys.size(); ++i) {
            custom_map[collision_keys[i]] = static_cast<int>(i);
        }

        double custom_time = timer.elapsed_ms();

        std::unordered_map<int, int> std_map;
        std_map.reserve(32);

        timer.start();

        for (size_t i = 0; i < collision_keys.size(); ++i) {
            std_map[collision_keys[i]] = static_cast<int>(i);
        }

        double std_time = timer.elapsed_ms();

        std::cout << "Test z potencjalnymi kolizjami (1000 elementów, początkowa pojemność 32):\n";
        std::cout << "Własna HashMap:     " << std::fixed << std::setprecision(2) << custom_time << " ms\n";
        std::cout << "std::unordered_map: " << std_time << " ms\n";
        std::cout << "Przyspieszenie:     " << (custom_time > 0 ? std_time / custom_time : 0.0) << "x\n";

        std::cout << "\nStatystyki po teście:\n";
        std::cout << "Własna HashMap - rozmiar: " << custom_map.get_size()
            << ", pojemność: " << custom_map.get_capacity()
            << ", load factor: " << custom_map.get_load_factor() << "\n";
        std::cout << "std::unordered_map - rozmiar: " << std_map.size()
            << ", bucket count: " << std_map.bucket_count()
            << ", load factor: " << std_map.load_factor() << "\n";

        custom_map.finalize();
        small_node_allocator.finalize();
        small_bucket_allocator.finalize();
    }

    void run_allocator_efficiency_test(size_t data_size) {
        std::cout << "\n=== Test efektywności alokatorów (rozmiar: " << data_size << ") ===\n";

        generate_test_data(data_size);

        // Test z różnymi konfiguracjami alokatorów
        std::cout << "Konfiguracje alokatorów:\n";
        std::cout << "1. Pool (node) + Stack (bucket) - zoptymalizowane\n";
        std::cout << "2. FreeList (node) + FreeList (bucket) - ogólne\n";

        // Konfiguracja 1: Pool + Stack (zoptymalizowana)
        PoolAllocator opt_node_allocator;
        StackAllocator opt_bucket_allocator;
        opt_node_allocator.initialize(data_size * sizeof(HashMap<int, int>::Node), sizeof(HashMap<int, int>::Node));
        opt_bucket_allocator.initialize(data_size * sizeof(void *) * 4);

        HashMap<int, int> opt_map;
        opt_map.initialize(data_size,
                           opt_node_allocator.get_allocator_info(),
                           opt_bucket_allocator.get_allocator_info());

        Timer timer;
        timer.start();
        for (size_t i = 0; i < data_size && i < test_keys_int.size(); ++i) {
            opt_map[test_keys_int[i]] = test_values[i];
        }
        double opt_time = timer.elapsed_ms();

        // Konfiguracja 2: FreeList + FreeList (ogólna)
        FreeListAllocator gen_node_allocator;
        FreeListAllocator gen_bucket_allocator;
        gen_node_allocator.initialize(data_size * sizeof(HashMap<int, int>::Node) * 8);
        gen_bucket_allocator.initialize(data_size * sizeof(void *) * 8);

        HashMap<int, int> gen_map;
        gen_map.initialize(data_size,
                           gen_node_allocator.get_allocator_info(),
                           gen_bucket_allocator.get_allocator_info());

        timer.start();
        for (size_t i = 0; i < data_size && i < test_keys_int.size(); ++i) {
            gen_map[test_keys_int[i]] = test_values[i];
        }
        double gen_time = timer.elapsed_ms();

        std::cout << std::fixed << std::setprecision(2);
        std::cout << "Wyniki wstawiania " << data_size << " elementów:\n";
        std::cout << "Pool + Stack:     " << opt_time << " ms\n";
        std::cout << "FreeList + FreeList: " << gen_time << " ms\n";
        std::cout << "Przyspieszenie:   " << (opt_time > 0 ? gen_time / opt_time : 0.0) << "x\n";

        opt_map.finalize();
        gen_map.finalize();

        opt_node_allocator.finalize();
        opt_bucket_allocator.finalize();
        gen_node_allocator.finalize();
        gen_bucket_allocator.finalize();
    }
};

int main() {
    std::cout << "=== ZAAWANSOWANE TESTY WYDAJNOŚCIOWE HASHMAP ===\n";
    std::cout << "Porównanie własnej implementacji HashMap z std::unordered_map\n";
    std::cout << "z dedykowanymi alokatorami dla każdego typu\n";

    HashMapBenchmark benchmark;

    // Maksymalny rozmiar dla inicjalizacji alokatorów
    const size_t max_test_size = 1000000;
    benchmark.initialize_allocators(max_test_size);

    try {
        // Testy dla różnych rozmiarów danych
        std::vector<size_t> test_sizes = { 1000, 10000, 100000, 1000000 };

        for (size_t size : test_sizes) {
            benchmark.run_int_benchmark(size);
        }

        // Test dla stringów
        std::vector<size_t> string_test_sizes = { 1000, 10000, 100000 };
        for (size_t size : string_test_sizes) {
            benchmark.run_string_benchmark(size);
        }

        // Test zużycia pamięci
        benchmark.run_memory_usage_test(10000);

        // Test kolizji
        benchmark.run_collision_test();

        // Test efektywności alokatorów
        benchmark.run_allocator_efficiency_test(50000);

        std::cout << "\n=== KONIEC TESTÓW ===\n";
    } catch (const std::exception &e) {
        std::cerr << "Błąd podczas testów: " << e.what() << std::endl;
    }

    benchmark.finalize_allocators();
    return 0;
}