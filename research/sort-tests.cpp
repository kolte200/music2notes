#include <iostream>
#include <stdint.h>


int arr1[] = {9, 4, 8, 6, 7, 0, 5, -2, 33, 42, 12};
constexpr int arr1_len = sizeof(arr1) / sizeof(arr1[0]);


class MyType {
public:
    float freq;
    float strength;

    inline bool operator>(const MyType& other) {
        return strength > other.strength;
    }
};


MyType arr2[] = {{340, 0.2}, {120, 0.1}, {260, 0.3}};
constexpr int arr2_len = sizeof(arr2) / sizeof(arr2[0]);


template<typename T>
static void print_array(T* arr, size_t len) {
    std::cout << "{";
    if (len > 0) {
        std::cout << arr[0];
        for (int i = 1; i < len; i++)
            std::cout << ", " << arr[i];
    }
    std::cout << "}" << std::endl;
}


static void print_mytype_array(MyType* arr, size_t len) {
    std::cout << "{";
    if (len > 0) {
        std::cout << arr[0].freq;
        for (int i = 1; i < len; i++)
            std::cout << ", " << arr[i].freq;
    }
    std::cout << "}" << std::endl;
}


#include "../src/sort.h"


int main(int argc, char** args) {
    print_array(arr1, arr1_len);
    sort(arr1, arr1_len);
    print_array(arr1, arr1_len);

    print_mytype_array(arr2, arr2_len);
    sort(arr2, arr2_len);
    print_mytype_array(arr2, arr2_len);

    return 0;
}
