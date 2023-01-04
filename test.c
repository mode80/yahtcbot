#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include "yahtcbot.c" 

// Definition of the "ints8" struct and the "powerset" function
// typedef struct { size_t count; int arr[8]; } ints8;
// ints8* powerset(const ints8 items, size_t* result_count);

void test_powerset() {
    // Test input
    ints8 items = {3, {1, 2, 3}};
    size_t expected_result_count = 8;
    ints8 expected_result[8] = {
        {0, {}},
        {1, {1}},
        {1, {2}},
        {2, {1, 2}},
        {1, {3}},
        {2, {1, 3}},
        {2, {2, 3}},
        {3, {1, 2, 3}}
    };

    // Call the "powerset" function
    size_t result_count;
    ints8* result = powerset(items, &result_count);

    // Check the result
    assert (result_count == expected_result_count);
    
    for (int i = 0; i < result_count; i++) {
        assert (result[i].count == expected_result[i].count) ;
        for (int j = 0; j < result[i].count; j++) {
            assert (result[i].arr[j] == expected_result[i].arr[j]) ;
        }
    }
}

void test_intbuf_new() {
    // Test 1: Check that intbuf_new returns a non-NULL pointer
    intbuf* buf = intbuf_new(10);
    assert(buf != NULL);
    intbuf_destroy(buf);

    // Test 2: Check that the capacity of the intbuf is set correctly
    buf = intbuf_new(15);
    assert(buf->_cap == 15);
    intbuf_destroy(buf);
}

void test_intbuf_recap() {
    // Test 1: Check that intbuf_recap changes the capacity of the intbuf
    intbuf* buf = intbuf_new(10);
    buf = intbuf_recap(buf, 15);
    assert(buf->_cap == 15);
    intbuf_destroy(buf);

    // Test 2: Check that intbuf_recap preserves the existing elements of the intbuf
    int arr[10] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    buf = intbuf_from_arr(arr, 10);
    buf = intbuf_recap(buf, 20);
    assert(buf->_cap == 20);
    for (size_t i = 0; i < 10; i++) {
        assert(intbuf_get(buf, i) == arr[i]);
    }
    intbuf_destroy(buf);
}

void test_intbuf_get() {
    // Test 1: Check that intbuf_get returns the correct value
    int arr[10] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    intbuf* buf = intbuf_from_arr(arr, 10);
    assert(intbuf_get(buf, 0) == 1);
    assert(intbuf_get(buf, 9) == 10);
    intbuf_destroy(buf);

    // Test 2: Check that intbuf_get returns the correct value after changing the value of an element
    buf = intbuf_from_arr(arr, 10);
    intbuf_set(buf, 0, 100);
    assert(intbuf_get(buf, 0) == 100);
    intbuf_destroy(buf);
}

void test_intbuf_set() {
    // Test 1: Check that intbuf_set changes the value of an element
    int arr[10] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    intbuf* buf = intbuf_from_arr(arr, 10);
    intbuf_set(buf, 0, 100);
    assert(intbuf_get(buf, 0) == 100);
    intbuf_destroy(buf);
}

void test_intbuf_from_arr() {
    // Test 1: Check that intbuf_from_arr creates an intbuf with the correct capacity
    int arr[10] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    intbuf* buf = intbuf_from_arr(arr, 10);
    assert(buf->_cap == 10);

    // Test 2: Check that intbuf_from_arr initializes the elements of the intbuf correctly
    for (size_t i = 0; i < 10; i++) {
        assert(intbuf_get(buf, i) == arr[i]);
    }
    intbuf_destroy(buf);
}

int main() {

    test_powerset();

    test_intbuf_new();
    test_intbuf_recap();
    test_intbuf_get();
    test_intbuf_set();
    test_intbuf_from_arr();
 
    printf("tests passed.\n");
    return 0;
}
