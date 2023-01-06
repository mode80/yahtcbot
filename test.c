#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include "yahtcbot.c" 

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

void test_n_take_r() {
    assert(n_take_r(5, 2, true, true) == 25);  // 5^2 permutations with replacement
    assert(n_take_r(5, 2, true, false) == 20);  // 5! / (5-2)! permutations without replacement
    assert(n_take_r(5, 2, false, true) == 15);  // (5+2-1)! / (2! * (5-1)!) combinations with replacement
    assert(n_take_r(5, 2, false, false) == 10);  // 5! / (2! * (5-2)!) combinations without replacement
}

typedef struct ints8_6{ int arr[6]; } ints8_6;

void test_get_combos_with_replacement() {
    int take_count = 2;
    int return_size;
    ints8 items = {3, {1, 2, 3} };
    ints8 (*combos)[6] = (ints8(*)[6])get_combos_with_replacement(items, 2, &return_size); //TODO understand this black magic

    // Check number of combinations
    assert(return_size == n_take_r(3,2,false,true));

    // Check that all combinations are present
    int expected_combinations[][2] = {{1, 1}, {1, 2}, {1, 3}, {2, 2}, {2, 3}, {3, 3}};
    for (int i = 0; i < return_size; i++) {
        ints8 combo = (*combos)[i];
        bool found = false;
        for (int j = 0; j < 6; j++) {
            if (memcmp(combo.arr, expected_combinations[j], sizeof(int)*take_count) == 0) {
                found = true;
                break;
            }
        }
        assert(found);
    }

    free(combos);
}

void test_distinct_arrangements_for() {
    // Test 1: Test with a list of unique values
    ints8 dieval_vec1 = {5,{1, 2, 3, 4, 5}};
    float expected_result1 = 120.0;
    float result1 = distinct_arrangements_for(dieval_vec1);
    assert(result1 == expected_result1);

    // Test 2: Test with a list of all the same value
    ints8 dieval_vec2 = {5,{1, 1, 1, 1, 1}};
    float expected_result2 = 1.0;
    float result2 = distinct_arrangements_for(dieval_vec2);
    assert(result2 == expected_result2);

    // Test 3: Test with a list of mixed values
    ints8 dieval_vec3 = {4,{1, 2, 2, 3}};
    float expected_result3 = 12.0;
    float result3 = distinct_arrangements_for(dieval_vec3);
    assert(result3 == expected_result3);
}

void test_uniquePermsOf5Ints() {
    // Test 1: Test with a list of unique values
    int array1[] = {1, 2, 3, 4, 5};
    int return_size1;
    int** result1 = uniquePermsOf5Ints(array1, &return_size1);
    assert(return_size1 == 120);
    for (int i = 0; i < return_size1; i++) {
        int* perm = result1[i];
        assert(perm[0] != perm[1] && perm[1] != perm[2] && perm[2] != perm[3] && perm[3] != perm[4] && perm[4] != perm[0]);
    }

    // Test 2: Test with a list of all the same value
    int array2[] = {1, 1, 1, 1, 1};
    int return_size2;
    int** result2 = uniquePermsOf5Ints(array2, &return_size2);
    assert(return_size2 == 1);
    for (int i = 0; i < return_size2; i++) {
        int* perm = result2[i];
        assert(perm[0] == perm[1] && perm[1] == perm[2] && perm[2] == perm[3] && perm[3] == perm[4] && perm[4] == perm[0]);
    }

    // Test 3: Test with a list of mixed values
    int array3[] = {1, 2, 2, 3, 3};
    int return_size3;
    int** result3 = uniquePermsOf5Ints(array3, &return_size3);
    assert(return_size3 == 30);
    for (int i = 0; i < return_size3; i++) {
        int* perm = result3[i];
        int count1 = 0;
        int count2 = 0;
        int count3 = 0;
        for (int j = 0; j < 5; j++) {
            if (perm[j] == 1) count1++;
            else if (perm[j] == 2) count2++;
            else if (perm[j] == 3) count3++;
        }
        assert(count1 == 1 && count2 == 2 && count3 == 2);
    }

}

void test_dievals_functions() {
    // Test 1: Test dievals_empty
    DieVals empty = dievals_empty();
    assert(empty == 0);

    // Test 2: Test dievals_init
    DieVal dievals1[5] = {1, 2, 3, 4, 5};
    DieVals result1 = dievals_init(dievals1);
    assert(result1 == ((1 << 0) | (2 << 3) | (3 << 6) | (4 << 9) | (5 << 12)));

    // Test 3: Test dievals_init_w_ints
    int dievals2[5] = {6, 5, 4, 3, 2};
    DieVals result2 = dievals_init_w_ints(dievals2);
    assert(result2 == ((6 << 0) | (5 << 3) | (4 << 6) | (3 << 9) | (2 << 12)));

    // Test 4: Test dievals_get
    DieVals test_val = ((6 << 0) | (5 << 3) | (4 << 6) | (3 << 9) | (2 << 12));
    assert(dievals_get(test_val, 0) == 6);
    assert(dievals_get(test_val, 1) == 5);
    assert(dievals_get(test_val, 2) == 4);
    assert(dievals_get(test_val, 3) == 3);
    assert(dievals_get(test_val, 4) == 2);
}

void test_slots_functions() {
    // Test 1: Test slots_empty
    Slots empty = slots_empty();
    assert(empty == 0);

    // Test 2: Test slots_init_va
    Slots test_val = slots_init_va(3, 1, 3, 5);
    assert(test_val == ((1 << 1) | (1 << 3) | (1 << 5)));

    // Test 3: Test slots_get
    assert(slots_get(test_val, 0) == 1);
    assert(slots_get(test_val, 1) == 3);
    assert(slots_get(test_val, 2) == 5);

    // Test 4: Test slots_has
    assert(slots_has(test_val, 1) == true);
    assert(slots_has(test_val, 2) == false);
    assert(slots_has(test_val, 3) == true);
    assert(slots_has(test_val, 4) == false);
    assert(slots_has(test_val, 5) == true);

    // Test 5: Test slots_len
    assert(slots_len(test_val) == 3);

    // Test 6: Test removing
    Slots result2 = removing(test_val, 3);
    assert(result2 == ((1 << 1) | (1 << 5)));

    // Test 7: Test used_upper_slots
    Slots all_bits = 0xFFFF;
    Slots used_uppers = used_upper_slots(all_bits);
    assert(used_uppers == 0); 
    used_uppers = used_upper_slots(test_val);
    assert(used_uppers == ((1 << 2) | (1 << 4) | (1 << 6))); 

 
    // Test 8: Test best_upper_total
    Slots upper_bits = ((1 << 1) | (1 << 2) | (1 << 3) | (1 << 4) | (1 << 5) | (1 << 6));
    assert(best_upper_total(upper_bits) == 0x69);

    // Test 9: Test useful_upper_totals
    Slots used_upper_slots = ((1 << 1) | (1 << 2) | (1 << 3) | (1 << 4) | (1 << 5) | (1 << 6));
    ints64 result3 = useful_upper_totals(used_upper_slots);
    ints64 expected_output = {32,{0,2,4,6,8,10,12,14,16,18,20,22,24,26,28,30,32,34,36,38,40,42,44,46,48,50,52,54,56,58,60,62}};
    for (int i = 0; i < 64; i++) {
        assert(result3.arr[i] == expected_output.arr[i]);
    }
}


int main() {

    test_powerset();

    test_intbuf_new();
    test_intbuf_recap();
    test_intbuf_get();
    test_intbuf_set();
    test_intbuf_from_arr();

    test_n_take_r();

    test_get_combos_with_replacement();
    test_distinct_arrangements_for();  
    test_uniquePermsOf5Ints();
    test_dievals_functions();
    test_slots_functions();

    printf("Tests PASSED\n");
    return 0;
}
