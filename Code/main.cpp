#include <iostream>
#include <chrono>
#include <string>
#include <algorithm>
#include "Structures/string.hpp"
#include "Memory/freelist_allocator.hpp"
#include "Memory/pool_allocator.hpp"

using namespace std;
using namespace std::chrono;

volatile double prevent_optimization = 0.0; // Zmienna do przechowywania wyników

// Funkcja pomocnicza do pomiaru czasu wykonania (z zabezpieczeniem przed optymalizacją)
template<typename Func>
double measureTime(Func func, int iterations = 100000) {
    auto start = high_resolution_clock::now();
    volatile double sum = 0; // Sztuczna operacja, aby zmusić kompilator do wykonania kodu
    for (int i = 0; i < iterations; i++) {
        sum += func();
    }
    auto end = high_resolution_clock::now();
    prevent_optimization += sum; // Zapisywanie wyniku do zmiennej globalnej
    return duration<double, milli>(end - start).count();
}

int main() {
    PoolAllocator allocator;
    allocator.initialize(10_MiB, 100);

    const char *testString = "BBBBBBBBBBCCCCCCCCCAAAAAAADDDDDDDDDDDD";
    const char *searchString = "AA";
    const char *replaceString = "happy";


    int iterations = 1000000;
    // Pre-inicjalizacja BasicString
    String str1, str2, str3;
    str1.initialize("Hello", allocator.get_allocator_info());
    str2.initialize("World", allocator.get_allocator_info());
    str3.initialize(testString, allocator.get_allocator_info());
    
    // Pre-inicjalizacja std::string
    string std_str1 = "Hello";
    string std_str2 = "World";
    string std_str3 = testString;
    
    // Testy dla BasicString
    double timeBasicCopy = measureTime([&]() -> double {
        String copy;
        copy.copy(str3);
        return copy.get_size();
    }, iterations);
    
    double timeBasicConcat = measureTime([&]() -> double {
        String concat = str1 + str2;
        return concat.get_size();
    }, iterations);
    
    double timeBasicAppend = measureTime([&]() -> double {
        String appendStr;
        appendStr.copy(str1);
        appendStr.append(" World");
        return appendStr.get_size();
    }, iterations);
    
    double timeBasicFind = measureTime([&]() -> double {
        return str3.find(searchString);
    }, iterations);
    
    double timeBasicReplace = measureTime([&]() -> double {
        String replaceStr;
        replaceStr.copy(str3);
        replaceStr.replace(replaceString, searchString);
        return replaceStr.get_size();
    }, iterations);
    
    double timeBasicRemove = measureTime([&]() -> double {
        String removeStr;
        removeStr.copy(str3);
        removeStr.remove('o');
        return removeStr.get_size();
    }, iterations);
    
    double timeBasicPrefix = measureTime([&]() -> double {
        return str3.check_prefix("Lorem");
    }, iterations);
    
    double timeBasicSuffix = measureTime([&]() -> double {
        return str3.check_suffix("elit.");
    }, iterations);
    
    // Testy dla std::string
    double timeStdCopy = measureTime([&]() -> double {
        string copy = std_str3;
        return copy.size();
    }, iterations);
    
    double timeStdConcat = measureTime([&]() -> double {
        string concat = std_str1 + std_str2;
        return concat.size();
    }, iterations);
    
    double timeStdAppend = measureTime([&]() -> double {
        string appendStr = std_str1;
        appendStr += " World";
        return appendStr.size();
    }, iterations);
    
    double timeStdFind = measureTime([&]() -> double {
        return std_str3.find(searchString);
    }, iterations);
    
    double timeStdReplace = measureTime([&]() -> double {
        string replaceStr = std_str3;
        size_t pos = replaceStr.find(searchString);
        if (pos != string::npos) {
            replaceStr.replace(pos, strlen(searchString), replaceString);
        }
        return replaceStr.size();
    }, iterations);
    
    double timeStdRemove = measureTime([&]() -> double {
        string removeStr = std_str3;
        removeStr.erase(remove(removeStr.begin(), removeStr.end(), 'o'), removeStr.end());
        return removeStr.size();
    }, iterations);
    
    double timeStdPrefix = measureTime([&]() -> double {
        return std_str3.rfind("Lorem", 0) == 0;
    }, iterations);
    
    double timeStdSuffix = measureTime([&]() -> double {
        return std_str3.size() >= 5 && std_str3.compare(std_str3.size() - 5, 5, "elit.") == 0;
    }, iterations);
    
    // Wypisanie wyników
    cout << "Performance Comparison (in milliseconds per operation):\n";
    cout << "-----------------------------------------------------\n";
    cout << "Operation                 BasicString    std::string\n";
    cout << "-----------------------------------------------------\n";
    cout << "Copy                      " << timeBasicCopy << "         " << timeStdCopy << endl;
    cout << "Concatenation (+)         " << timeBasicConcat << "         " << timeStdConcat << endl;
    cout << "Append (+=)               " << timeBasicAppend << "         " << timeStdAppend << endl;
    cout << "Find substring            " << timeBasicFind << "         " << timeStdFind << endl;
    cout << "Replace substring         " << timeBasicReplace << "         " << timeStdReplace << endl;
    cout << "Remove characters         " << timeBasicRemove << "         " << timeStdRemove << endl;
    cout << "Check prefix              " << timeBasicPrefix << "         " << timeStdPrefix << endl;
    cout << "Check suffix              " << timeBasicSuffix << "         " << timeStdSuffix << endl;

    allocator.finalize();
    return 0;
}
