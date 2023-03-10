#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include "yahtcbot.h" 

extern Slot ACES; extern Slot TWOS; extern Slot THREES;
extern Slot FOURS; extern Slot FIVES; extern Slot SIXES;
extern Slot THREE_OF_A_KIND; extern Slot FOUR_OF_A_KIND;
extern Slot FULL_HOUSE; extern Slot SM_STRAIGHT; 
extern Slot LG_STRAIGHT; extern Slot YAHTZEE; extern Slot CHANCE;

extern DieVals SORTED_DIEVALS [32767]; 
extern f32 SORTED_DIEVALS_ID [32767]; 
extern Range SELECTION_RANGES[32];
extern DieVals OUTCOME_DIEVALS[1683]; 
extern DieVals OUTCOME_MASKS[1683]; 
extern f32 OUTCOME_ARRANGEMENTS[1683]; // could be a u8 but stored as f32 for faster final hotloop calculation

void test_powerset() {
    // Test input
    Ints8 items = {3, {1, 2, 3}};
    size_t expected_result_count = 8;
    Ints8 expected_result[8] = {
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
    Ints8* result = powerset(items, &result_count);

    // Check the result
    assert (result_count == expected_result_count);
    
    for (int i = 0; i < result_count; i++) {
        assert (result[i].count == expected_result[i].count) ;
        for (int j = 0; j < result[i].count; j++) {
            assert (result[i].arr[j] == expected_result[i].arr[j]) ;
        }
    }
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
    Ints8 items = {3, {1, 2, 3} };
    Ints8 (*combos)[6] = (Ints8(*)[6])get_combos_with_replacement(items, 2, &return_size); //TODO understand this black magic

    // Check number of combinations
    assert(return_size == n_take_r(3,2,false,true));

    // Check that all combinations are present
    int expected_combinations[][2] = {{1, 1}, {1, 2}, {1, 3}, {2, 2}, {2, 3}, {3, 3}};
    for (int i = 0; i < return_size; i++) {
        Ints8 combo = (*combos)[i];
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
    Ints8 dieval_vec1 = {5,{1, 2, 3, 4, 5}};
    float expected_result1 = 120.0;
    float result1 = distinct_arrangements_for(dieval_vec1);
    assert(result1 == expected_result1);

    // Test 2: Test with a list of all the same value
    Ints8 dieval_vec2 = {5,{1, 1, 1, 1, 1}};
    float expected_result2 = 1.0;
    float result2 = distinct_arrangements_for(dieval_vec2);
    assert(result2 == expected_result2);

    // Test 3: Test with a list of mixed values
    Ints8 dieval_vec3 = {4,{1, 2, 2, 3}};
    float expected_result3 = 12.0;
    float result3 = distinct_arrangements_for(dieval_vec3);
    assert(result3 == expected_result3);
}

void test_dievals_functions() {
    // Test 1: Test dievals_empty
    DieVals empty = dievals_empty();
    assert(empty == 0);

    // Test 2: Test dievals_from_intstar
    int* dievals1 = malloc(5*sizeof(int));
    memcpy(dievals1, (int[]){1, 2, 3, 4, 5}, sizeof(int)*5 );
    DieVals result1 = dievals_from_intstar(dievals1,5);
    assert(result1 == ((1 << 0) | (2 << 3) | (3 << 6) | (4 << 9) | (5 << 12)));

    // Test 3: Test dievals_init_w_ints
    int dievals2[5] = {6, 5, 4, 3, 2};
    DieVals result2 = dievals_from_arr5(dievals2);
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

    // Test 5: Test slots_count
    assert(slots_count(test_val) == 3);

    // Test 6: Test slots_removing
    Slots result2 = slots_removing(test_val, 3);
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
    Ints64 result3 = useful_upper_totals(used_upper_slots);
    Ints64 expected_output = {32,{0,2,4,6,8,10,12,14,16,18,20,22,24,26,28,30,32,34,36,38,40,42,44,46,48,50,52,54,56,58,60,62}};
    for (int i = 0; i < 64; i++) {
        assert(result3.arr[i] == expected_output.arr[i]);
    }
    Slots used_upper_slots2 = ((1 << 3) | (1 << 4) | (1 << 5));
    Ints64 result4 = useful_upper_totals(used_upper_slots2);
    Ints64 expected_output2 = {62, {0,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63}};
    for (int i = 0; i < 64; i++) {
        assert(result4.arr[i] == expected_output2.arr[i]);
    }
}

void test_get_unique_perms() {
    // Test with an array of unique elements
    Ints8 items1 = { .arr = {1, 2, 3}, .count = 3 };
    int result_count1;
    Ints8 *result1 = get_unique_perms(items1, &result_count1);
    assert(result_count1 == 6);
    free(result1);

    // Test with an array containing duplicate elements
    Ints8 items2 = { .arr = {1, 2, 2}, .count = 3 };
    int result_count2;
    Ints8 *result2 = get_unique_perms(items2, &result_count2);
    assert(result_count2 == 3);
    free(result2);

    // Test with 4 elements and a dupe 
    Ints8 items = { .arr={1, 2, 2, 3}, .count=4 };
    int result_count;
    Ints8* result = get_unique_perms(items, &result_count);
    assert(result_count == 12); // 4! / (2! * 1!) =12 
    free(result);
}

int compare_ints(const void* a, const void* b) {
    int x = *(int*)a; int y = *(int*)b;
    if (x < y) { return -1; } else if (x > y) { return 1; } else { return 0; }
}

void test_cache_sorted_dievals() {
    cache_sorted_dievals();

    // Check that the first element in SORTED_DIEVALS is the special wildcard
    assert(SORTED_DIEVALS_ID[0] == 0);
    assert(SORTED_DIEVALS[0] == 0);

    // Check that all other elements in SORTED_DIEVALS are set correctly
    for (int i = 1; i < 32767; i++) {
        // Check that permutation dv_id.dievals is equal to i when i is treated as a DieVals and sorted
        Ints8 dv_ints_to_sort = { .count = 5 };
        for (int j=0; j<5; j++){ dv_ints_to_sort.arr[j] = dievals_get(i, j); }        
        qsort(dv_ints_to_sort.arr, 5, sizeof(int), compare_ints);
        DieVal sorted_dievals = dievals_from_ints8(dv_ints_to_sort);
        assert(SORTED_DIEVALS[i] = sorted_dievals);

    }

    // Check that cache is able to lookup the correct sorted dievals for a given DieVals
    DieVals unsorted_dievals = dievals_from_arr5((int[5]){3, 2, 1, 5, 4});
    DieVals expected_sorted = dievals_from_arr5((int[5]){1, 2, 3, 4, 5}); 

    DieVals dievals_looked_up = SORTED_DIEVALS[unsorted_dievals];
    bool is_same = (dievals_looked_up == expected_sorted);
    assert(is_same);
}

void test_score_slot_with_dice() {
    DieVals dice = dievals_from_arr5((int[5]){1, 1, 1, 1, 1});
    assert(score_slot_with_dice(ACES, dice) == 5);

    dice = dievals_from_arr5((int[5]){3, 4, 4, 4, 4});
    assert(score_slot_with_dice(FOURS, dice) == 16);

    dice = dievals_from_arr5((int[5]){2, 2, 2, 4, 4});
    assert(score_slot_with_dice(THREE_OF_A_KIND, dice) == 14);

    dice = dievals_from_arr5((int[5]){1, 1, 1, 1, 5});
    assert(score_slot_with_dice(FOUR_OF_A_KIND, dice) == 9);
 
    dice = dievals_from_arr5((int[5]){3, 3, 6, 6, 6});
    assert(score_slot_with_dice(FULL_HOUSE, dice) == 25);
       
    dice = dievals_from_arr5((int[5]){2, 3, 4, 5, 5});
    assert(score_slot_with_dice(SM_STRAIGHT, dice) == 30);

    dice = dievals_from_arr5((int[5]){1, 2, 3, 4, 5});
    assert(score_slot_with_dice(LG_STRAIGHT, dice) == 40);

    dice = dievals_from_arr5((int[5]){6, 6, 6, 6, 6});
    assert(score_slot_with_dice(YAHTZEE, dice) == 50);

    dice = dievals_from_arr5((int[5]){1, 2, 3, 4, 5});
    assert(score_slot_with_dice(CHANCE, dice) == 15);
}

void test_cache_roll_outcomes_data() { //TODO could be better

    cache_roll_outcomes_data();

    assert(sizeof(OUTCOME_DIEVALS) / sizeof(OUTCOME_DIEVALS[0]) == 1683);

    //compare to known good values from Swift implementation
    assert(OUTCOME_DIEVALS[1682] == dievals_from_arr5((int[5]){6,6,6,6,6}));
    assert(OUTCOME_MASKS[1682] == dievals_from_arr5((int[5]){0,0,0,0,0}));
    assert(OUTCOME_ARRANGEMENTS[1682] == 1); 

    assert(OUTCOME_DIEVALS[0] == dievals_from_arr5((int[5]){0,0,0,0,0}));
    assert(OUTCOME_MASKS[0] == dievals_from_arr5((int[5]){7,7,7,7,7}));
    assert(OUTCOME_ARRANGEMENTS[0] == 1); 

    assert(OUTCOME_DIEVALS[1000] == dievals_from_arr5((int[5]){1,1,0,1,4}));
    assert(OUTCOME_MASKS[1000] == dievals_from_arr5((int[5]){0,0,7,0,0}));
    assert(OUTCOME_ARRANGEMENTS[1000] == 4); 

}

void test_slots_powerset() {
    Slots slots = slots_init_va(3, ACES, TWOS, FOURS);
    Slots powerset[64];
    int powerset_len;
    slots_powerset(slots, powerset, &powerset_len);
    // printf("Powerset of slots: \n");
    for (int i = 0; i < powerset_len; i++) {
        for(int j=0; j<slots_count(powerset[i]); j++){
            // printf("%d ", slots_get(powerset[i],j));
        }
        // printf("\n");
    }
}

void test_score_first_slot_in_context(){
    DieVals dievals = dievals_from_arr5((int[5]){1, 1, 1, 2, 3});
    Slots open_slots = slots_init_va(3, 1, 2, 4);
    GameState game = gamestate_init(dievals, open_slots, 0, 3, false);
    u8 score = score_first_slot_in_context(game);
    assert(score == 3);
}


void run_tests() {

    test_powerset();

    test_n_take_r();

    test_get_combos_with_replacement();
    test_distinct_arrangements_for();  
    test_dievals_functions();
    test_slots_functions();
    test_get_unique_perms(); 

    test_cache_sorted_dievals();
    test_score_slot_with_dice();  
    test_cache_roll_outcomes_data();     
    test_slots_powerset();

    test_score_first_slot_in_context();

    printf("Tests PASSED\n");
}

