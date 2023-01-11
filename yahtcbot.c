//#pragma clang diagnostic push
//#pragma ide diagnostic ignored "bugprone-integer-division"
//#pragma ide diagnostic ignored "UnusedValue"
//#pragma ide diagnostic ignored "DanglingPointer"
//#pragma ide diagnostic ignored "OCUnusedMacroInspection"
//#pragma ide diagnostic ignored "OCUnusedGlobalDeclarationInspection"
//#pragma ide diagnostic ignored "cppcoreguidelines-narrowing-conversions"

#include "yahtcbot.h"
void run_tests();  // forward declare unit test runner found in test.c

Slot ACES = 1; Slot TWOS = 2; Slot THREES = 3;
Slot FOURS = 4; Slot FIVES = 5; Slot SIXES = 6;
Slot THREE_OF_A_KIND = 7; Slot FOUR_OF_A_KIND = 8;
Slot FULL_HOUSE = 9; Slot SM_STRAIGHT = 10; 
Slot LG_STRAIGHT = 11; Slot YAHTZEE = 12; Slot CHANCE = 13 ;

int RANGE_IDX_FOR_SELECTION[32] = {0,1,2,3,7,4,8,11,17,5,9,12,20,14,18,23,27,6,10,13,19,15,21,24,28,16,22,25,29,26,30,31} ;
const int SENTINEL=INT_MIN;
u64 tick_limit;
u64 tick_interval;
u64 ticks = 0;
int progress_blocks;

//-------------------------------------------------------------
//  UTILS
//-------------------------------------------------------------

#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))


int min(int a, int b) { return a < b ? a : b; }
int max(int a, int b) { return a > b ? a : b; }

//powerset for a given set of elements
Ints8* powerset(const Ints8 items, size_t* result_count) {
    *result_count = (1 << items.count); //set_size of powerset of n elements is (2**n) w the empty set
    Ints8* result = malloc(*result_count * sizeof(Ints8));
    for (int i = 0; i < *result_count; i++) {  // Run from counter 000..0 to 111..1
        int inner_list_size = 0;
        for (int j = 0; j < items.count; j++) {
            if ((i & (1 << j)) > 0) { 
                result[i].arr[inner_list_size++] = items.arr[j]; 
            }
        }
        result[i].count = inner_list_size;
    }
    return result;
}

u64 factorial(u64 n){ 
    if (n<=1) {return 1;}
    return n * factorial(n-1);
}

// count of arrangements that can be formed from r selections, chosen from n items, 
// where order DOES or DOESNT matter, and WITH or WITHOUT replacement, as specified.
u64 n_take_r(u64 n, u64 r, bool order_matters/*=false*/, bool with_replacement/*=false*/) {  
    if (order_matters){  //  order matters; we're counting "permutations" 
        if (with_replacement) {
            return pow(n,r);
        } else { //  no replacement
            return factorial(n) / factorial(n-r);  //  this = factorial(n) when r=n
        }
    } else { //  we're counting "combinations" where order doesn't matter; there are less of these 
        if (with_replacement) {
            return factorial(n+r-1) / (factorial(r)*factorial(n-1));
        } else { //  no replacement
            return factorial(n) / (factorial(r)*factorial(n-r));
        }
    }
}

u64 powerset_of_size_n_count(int n){ return 1<<n; }

void save_combo_to_results(Ints8 items, Ints8 *indices, int item_count, Ints8* results, int* result_count) {
    //helper function to _get_combos_with_replacement
    results[*result_count].count = item_count;    
    for (int i=0; i<item_count; i++) {
        results[*result_count].arr[i] = items.arr[indices->arr[i]];
    }
    (*result_count)++;
}

void make_combos_with_replacement(Ints8 items, int n, int r, Ints8 *indices, Ints8* results, int* result_count) {
    //helper function to _get_combos_with_replacement
    save_combo_to_results(items, indices, r, results, result_count);
    if (r==0) return; // when supplied r is 0 there's just one emptyset result 
    while(true){
        int i =r;
        while (i > 0) {
            i--;
            if (indices->arr[i] != n-1) break ; //find value of i to use next
        }
        if (indices->arr[i] == n-1) return; 
        int new_index = indices->arr[i]+1;
        for (int j=i; j<r; j++) {
            indices->arr[j] = new_index;
        }
        save_combo_to_results(items, indices, indices->count, results, result_count);
    }
}

Ints8* get_combos_with_replacement(Ints8 items, int r, int* result_count) { 
    //combinations_with_replacement('ABC', 2) --> AA AB AC BB BC CC
    // number items returned:  (n+r-1)! / r! / (n-1)!
    assert(r<=8);
    Ints8 indices = {r, {0,0,0,0,0,0,0,0} };
    int n = items.count;
    u64 result_capacity = n_take_r(n,r,false,true);
    Ints8* results = malloc(result_capacity * sizeof(Ints8));
    *result_count=0;
    make_combos_with_replacement(items, n, r, &indices, results, result_count);
    return results;
}

float distinct_arrangements_for(Ints8 dieval_vec) {
    u64 key_counts[7]={};
    for (int i = 0; i < dieval_vec.count; i++) { 
        int idx = dieval_vec.arr[i];
        key_counts[idx]++; 
    }
    int divisor = 1;
    int non_zero_dievals = 0;
    for (int i = 1; i <= 6; i++) {
        if (key_counts[i] != 0) {
            divisor *= factorial(key_counts[i]);
            non_zero_dievals += key_counts[i];
        }
    }
    return (f32)(factorial(non_zero_dievals) / divisor);
}

int countTrailingZeros(int x) {
     static const int lookup[] = {32, 0, 1,
     26, 2, 23, 27, 0, 3, 16, 24, 30, 28, 11,
     0, 13, 4, 7, 17, 0, 25, 22, 31, 15, 29,
     10, 12, 6, 0, 21, 14, 9, 5, 20, 8, 19, 18};
     return lookup[(-x & x) % 37];
}

void swap(int* a, int* b) { int temp = *a; *a = *b; *b = temp; } //helper to get_unique_perms()

void make_unique_perms(Ints8 items, int start, Ints8 *result, int *counter) {
    //helper to get_unique_perms()
    if (start == items.count-1) { // Found a permutation
        result[*counter] = items; 
        (*counter)++;
    } else {
        for (int i = start; i < items.count; i++) {
            bool dupe = false;// Check if the current element is already present in the permutation
            for (int j = start; j < i; j++) {
                if (items.arr[i] == items.arr[j]) {
                    dupe = true;
                    break;
                }
            }
            if (dupe) continue;
            swap(&items.arr[start], &items.arr[i]);
            make_unique_perms(items, start+1, result, counter);
            swap(&items.arr[start], &items.arr[i]);
        }
    }
}

Ints8* get_unique_perms(Ints8 items, int* result_count) {
    assert(items.count > 0);
    int n = factorial(items.count);// this is too big when there are dupes, but sufficient otherwise
    Ints8* result = malloc(sizeof(Ints8) * n);//map malloced memory onto a locally sized var for easier debugging  
    *result_count = 0;
    make_unique_perms(items, 0, result, result_count);
    return result;
}

// returns a range which corresponds the precomputed dice roll outcome data corresponding to the given selection
Range outcomes_range_for(Selection selection) {
    int idx = RANGE_IDX_FOR_SELECTION[selection];
    Range range = SELECTION_RANGES[idx]; 
    return range;
} 

void print_state_choices_header() { 
    printf("choice_type,choice,dice,rolls_remaining,upper_total,yahtzee_bonus_avail,open_slots,expected_value");
} 

//#=-------------------------------------------------------------
//DieVals
//-------------------------------------------------------------=#

DieVals dievals_empty() { return 0; }

DieVals dievals_init(int a, int b, int c, int d, int e) {
    return (DieVal)(a | b<<3 | c<<6 | d<<9 | e<<12);
}

DieVals dievals_from_arr5(const int dievals[5]) {
    u16 result = 0;
    for (int i = 0; i < 5; i++){ 
        result |= (dievals[i] << (i*3)); 
    } 
    return result;
}

DieVals dievals_from_ints8(Ints8 dievals) {
    u16 result = 0;
    for (int i = 0; i < dievals.count; i++){ result |= (dievals.arr[i] << (i*3)); } 
    return result;
}

DieVals dievals_from_intstar(const int* dievals, int count) {
    u16 result = 0;
    for (int i = 0; i < count; i++){ result |= (dievals[i] << (i*3)); } 
    return result;
}

DieVal dievals_get(const DieVals self, int index) {
    return (self >> (index*3)) & 0b111;
}

//-------------------------------------------------------------
// SLOTS 
// ------------------------------------------------------------

    Slots slots_empty() { return 0; } 

    Slots slots_init_va(int arg_count, ... ) {
        u16 result =0;
        u16 mask = 0; 

        va_list args; //varargs args ceremony
        va_start (args, arg_count); // " " "

        for (int i = 0; i < arg_count; i++){ 
            mask = 0x0001 << (u16)(va_arg(args, int));  // va_arg returns and advances the next arg 
            result |= mask; // force on
        }
        return result;
    }

    Slots slots_from_ints16(Ints16 slots) {
        u16 result = 0;
        u16 mask = 0; 
        for (int i = 0; i < slots.count; i++){ 
            mask = 0x0001 << (u16)(slots.arr[i]);  
            result |= mask; // force on
        }
        return result;
    }

    Slot slots_get(Slots self, int index) {
        u16 bits = self;
        u16 bit_index=0;
        int i = index+1;
        int j=1; //the slots data does not use the rightmost (0th) bit 
        while (j <= i){ 
            bit_index = countTrailingZeros(bits);
            bits = (bits & (~( 1 << bit_index) ));  //unset bit
            j+=1;
        } 
        return (Slot)bit_index;
    }

    bool slots_has(Slots self, Slot slot){ 
        return 0x0000 < (self & (0x0001<<(u16)(slot)));
    } 

    int slots_count(Slots self){ 
        int len = 0;
        for (int i=1; i<=13; i++){ if (slots_has(self, i)) {len++;} }
        return len; 
    } 

    Slots slots_removing(Slots self, Slot slot_to_remove) { 
        u16 mask = ~( 1 << slot_to_remove );
        return (self & mask); //# force off
    } 

    int* ZERO_THRU_63 = (int[64]){0,1,2,3,4,5,6,7,8,9, 10,11,12,13,14,15,16,17,18,19,
            20,21,22,23,24,25,26,27,28,29, 30,31,32,33,34,35,36,37,38,39,
            40,41,42,43,44,45,46,47,48,49, 50,51,52,53,54,55,56,57,58,59, 60,61,62,63};

    Slots used_upper_slots(Slots unused_slots) {
        Slots all_bits_except_unused = ~unused_slots; // "unused" slots are not "previously used", so blank those out
        Slots all_upper_slot_bits = (u16) ((1<<7)-2);  // upper slot bits are those from 2^1 through 2^6 (encoding doesn't use 2^0)
        Slots previously_used_upper_slot_bits = (u16) (all_bits_except_unused & all_upper_slot_bits);
        return previously_used_upper_slot_bits;
    } 

    void slots_powerset(Slots self, Slots* out, int* out_len) { 
        int len = slots_count(self);
        int powerset_len = 1 << len;
        int powerset_index = 0;
        for (int i=0; i<powerset_len; i++){ 
            int j = i;
            int k = 0;
            Slots subset = slots_empty();
            while (j > 0) { 
                if (j & 1) { subset |= (0x0001 << slots_get(self, k)); }
                j >>= 1;
                k += 1;
            } 
            out[powerset_index] = subset;
            powerset_index += 1;
        } 
        *out_len = powerset_len;
    }

    u8 best_upper_total(Slots slots) {  
        u8 sum=0;
        for (int i=1; i<=6; i++) { 
            if (slots_has(slots, i)) { sum+=i; }
        } 
        return sum*5;
    } 

    // a non-exact but fast estimate of relevant_upper_totals
    // ie the unique and relevant "upper bonus total" that could have occurred from the previously used upper slots
    Ints64 useful_upper_totals(Slots unused_slots) { 
        int totals[64]; 
        memcpy(totals, ZERO_THRU_63, 64*sizeof(int)); // init to 0 thru 63
        Slots used_uppers = used_upper_slots(unused_slots);
        bool all_even = true;
        int count = slots_count(used_uppers);
        for (int i=0; i<count; i++){ 
            Slot slot = slots_get(used_uppers, i);
            if (slot % 2 == 1) {all_even = false; break;} 
        }
        if (all_even) { // stub out odd totals if the used uppers are all even 
            for (int i=0; i<64; i++){ 
                if (totals[i]%2==1) totals[i] = SENTINEL;
            } 
        } 

        // filter out the lowish totals that aren't relevant because they can't reach the goal with the upper slots remaining 
        // this filters out a lot of dead state space but means the lookup function must later map extraneous deficits to a default 
        int best_unused_slot_total = best_upper_total(unused_slots);
        // totals = (x for x in totals if x + best_unused_slot_total >=63 || x==0)
        // totals = from x in totals where (x + best_unused_slot_total >=63 || x==0) select x
        Ints64 result = {};
        for (int i=0; i<64; i++){ 
            if (totals[i]!=SENTINEL && totals[i] + best_unused_slot_total >= 63 || totals[i]==0) {
                result.arr[result.count]=totals[i];
                result.count++;
            }
        }
        return result;  
    }

//-------------------------------------------------------------
// INITIALIZERS 
//-------------------------------------------------------------

// this generates the ranges that correspond to the outcomes, within the set of all outcomes, indexed by a give selection 
void cache_selection_ranges() {

    int s = 0;

    Ints8 idxs0to4 = (Ints8){5, {0,1,2,3,4} };
    size_t result_count = 0; 
    Ints8* combos = powerset( idxs0to4, &result_count);

    for(int i=0; i<result_count; i++) {
        int sets_count = n_take_r(6, combos[i].count, false, true); 
        SELECTION_RANGES[i] = (Range){s, s+sets_count}; 
        s += sets_count;
    } 

    free(combos);
}


// for fast access later, this generates an array of dievals in sorted form, 
// along with each's unique "ID" between 0-252, indexed by DieVals data
void cache_sorted_dievals() { 
    
    SORTED_DIEVALS[0] = (DieValsID){}; // first one is for the special wildcard 
    Ints8 one_to_six = {.count=6, .arr={1,2,3,4,5,6} }; 
    int combos_size;
    Ints8* combos = get_combos_with_replacement(one_to_six, 5, &combos_size);
    int perm_count=0;
    for (int i=0; i<combos_size; i++) {
        DieVals dv_sorted = dievals_from_ints8(combos[i]);
        Ints8* ints8_perms = get_unique_perms(combos[i], &perm_count);
        for (int j=0; j<perm_count; j++) {
            Ints8 perm = ints8_perms[j];
            DieVals dv_perm = dievals_from_ints8(perm);
            SORTED_DIEVALS[dv_perm] = (DieValsID){.dievals=dv_sorted, .id=i};
        }
    }
}


//preps the caches of roll outcomes data for every possible 5-die selection, where '0' represents an unselected die """
void cache_roll_outcomes_data() { 

    int i=0; size_t idx_combo_count=0; 
    Ints8* idx_combos = powerset( (Ints8){.count=5,.arr={0,1,2,3,4}}, &idx_combo_count );
    assert(idx_combo_count==32); 
    Ints8 one_thru_six = {6, {1,2,3,4,5,6}}; 

    for (int v=0; v<idx_combo_count; v++) { 
        int dievals_arr[5] = {0,0,0,0,0}; 
        Ints8 idx_combo = idx_combos[v];
        int die_count = idx_combo.count; 
        
        int die_combos_size = 0;
        // combos_with_rep(one_thru_six, 6, die_count, &result, &combos_size); 
        Ints8* die_combos = get_combos_with_replacement(one_thru_six, die_count, &die_combos_size);
 
        for (int w=0; w<die_combos_size; w++) {
            Ints8 die_combo = die_combos[w];
            int mask_vec[5] = {0b111,0b111,0b111,0b111,0b111};
            for(int j=0; j<die_count; j++) {
                int idx = idx_combo.arr[j];
                dievals_arr[idx] = (DieVal)die_combo.arr[j];
                mask_vec[idx]=0;
            }
            int arrangement_count = distinct_arrangements_for(die_combo);
            DieVals dievals = dievals_from_arr5(dievals_arr);
            DieVals mask = dievals_from_arr5(mask_vec);
            OUTCOME_DIEVALS[i] = dievals;
            OUTCOME_MASKS[i] = mask;
            OUTCOME_ARRANGEMENTS[i] = arrangement_count;
            OUTCOMES[i] = (Outcome){ dievals, mask, arrangement_count};
            i+=1;
            assert(i<=1683);
        } 

        free(die_combos);
    } 
    free(idx_combos);
} 

void init_bar_for(GameState game) {
    tick_limit = counts(game);
    tick_interval = (tick_limit) / 100;
    printf("Progress: %d%%\r", 0);
    fflush(stdout);
} 

void tick(){
    ticks++;
    if (ticks % tick_interval == 0) {
        printf("Progress: %d%%\r", (int)(ticks * 100 / tick_limit));
        // printf("█");
        fflush(stdout);
    }
}

void output(GameState state, ChoiceEV choice_ev){ 
    // Uncomment below for more verbose progress output at the expense of speed 
    // print_state_choice(state, choice_ev);
}

void print_state_choice(GameState state, ChoiceEV choice_ev) {
    char Y[] = "Y"; char N[] = "N"; char S[] = "S"; char D[] = "D"; char C[] = ","; char U[] = "_";
    char sb[60];
    memset(sb, 0, sizeof(sb));
    if (state.rolls_remaining == 0) {
        strcat(sb, S); strcat(sb, C);
        char temp[10];
        snprintf(temp, sizeof(temp), "%d", choice_ev.choice); strcat(sb, temp);
    } else {
        strcat(sb, D); strcat(sb, C);
        char temp[10];
        snprintf(temp, sizeof(temp), "%05d", choice_ev.choice); strcat(sb, temp);
    }
    strcat(sb, C);
    char temp[10];
    snprintf(temp, sizeof(temp), "%d", dievals_get(state.sorted_dievals,4)); strcat(sb, temp); 
    snprintf(temp, sizeof(temp), "%d", dievals_get(state.sorted_dievals,3)); strcat(sb, temp); 
    snprintf(temp, sizeof(temp), "%d", dievals_get(state.sorted_dievals,2)); strcat(sb, temp); 
    snprintf(temp, sizeof(temp), "%d", dievals_get(state.sorted_dievals,1)); strcat(sb, temp); 
    snprintf(temp, sizeof(temp), "%d", dievals_get(state.sorted_dievals,0)); strcat(sb, temp); 
    strcat(sb, C);
    snprintf(temp, sizeof(temp), "%d", state.rolls_remaining); strcat(sb, temp); strcat(sb, C);
    snprintf(temp, sizeof(temp), "%d", state.upper_total); strcat(sb, temp); strcat(sb, C);
    if (state.yahtzee_bonus_avail) { 
        strcat(sb, Y); 
    } else { 
        strcat(sb, N); 
    } strcat(sb, C);
    for (int i=0; i<slots_count(state.open_slots); i++){ 
        snprintf(temp, sizeof(temp), "%d", slots_get(state.open_slots,i)); strcat(sb, temp); strcat(sb, U); 
    }
    strcat(sb, C);
    snprintf(temp, sizeof(temp), "%.2f", choice_ev.ev); strcat(sb, temp);
    puts(sb);
}

//-------------------------------------------------------------
//SCORING FNs
//-------------------------------------------------------------

u8 upperbox(const u8 boxnum, const DieVals sorted_dievals) { 
    u8 sum=0;
    for (int i=0; i<5; i++) {
        u8 got_val = dievals_get(sorted_dievals,i);
        if (got_val==boxnum) {
            sum+=boxnum;
        } 
    }
    return sum;
} 

u8 n_of_a_kind(const u8 n, const DieVals sorted_dievals) { 
    u8 inarow=1; u8 maxinarow=1; u8 lastval=100; u8 sum=0;
    for (int i = 0; i < 5; i++) {
        DieVal dieval = dievals_get(sorted_dievals,i);
        if (dieval==lastval && dieval != 0) { inarow += 1; } else { inarow=1; } 
        maxinarow = inarow>maxinarow? inarow : maxinarow;
        lastval = dieval;
        sum+=dieval;
    } 
    if (maxinarow>=n) {return sum;} else {return 0;} 
} 

u8 straight_len(const DieVals sorted_dievals) { 
    u8 inarow =1;
    u8 lastval =254; // stub
    u8 maxinarow =1;
    for (int i = 0; i < 5; i++) {
        DieVal dieval = dievals_get(sorted_dievals,i);
        if (dieval==lastval+1 && dieval != 0){ 
            inarow+=1;
        } else if (dieval != lastval) { 
            inarow=1; 
        }
        maxinarow = inarow>maxinarow? inarow : maxinarow;
        lastval = dieval; 
    } 
    return maxinarow;
} 

u8 score_aces(DieVals sorted_dievals)             { return upperbox(0x1,sorted_dievals);}
u8 score_twos(DieVals sorted_dievals)             { return upperbox(0x2,sorted_dievals);} 
u8 score_threes(DieVals sorted_dievals)           { return upperbox(0x3,sorted_dievals);} 
u8 score_fours(DieVals sorted_dievals)            { return upperbox(0x4,sorted_dievals);} 
u8 score_fives(DieVals sorted_dievals)            { return upperbox(0x5,sorted_dievals);} 
u8 score_sixes(DieVals sorted_dievals)            { return upperbox(0x6,sorted_dievals);} 
    
u8 score_three_of_a_kind(DieVals sorted_dievals)  { return n_of_a_kind(0x3,sorted_dievals);} 
u8 score_four_of_a_kind(DieVals sorted_dievals)   { return n_of_a_kind(0x4,sorted_dievals);} 
u8 score_sm_str8(DieVals sorted_dievals)          { return (straight_len(sorted_dievals)>=4 ? 30 : 0);}
u8 score_lg_str8(DieVals sorted_dievals)          { return (straight_len(sorted_dievals)==5 ? 40 : 0);}

// The official rule is that a Full House is "three of one number and two of another
u8 score_fullhouse(DieVals sorted_dievals) { 
    int valcounts1 = 0; int valcounts2 = 0;
    int j=0;
    for (int i = 0; i < 5; i++) {
        DieVal val = dievals_get(sorted_dievals,i);
        if (val==0) {return 0; }
        if (j==0 || val != dievals_get(sorted_dievals,i-1)) {j+=1;} 
        if (j==1) {valcounts1+=1;} 
        if (j==2) {valcounts2+=1;} 
        if (j==3) {return 0; }
    } 
    if (valcounts1==3 && valcounts2==2 || valcounts2==3 && valcounts1==2) {return 25;} 
    return 0;
} 
    
u8 score_chance(DieVals sorted_dievals) {
    u8 sum=0; int i=5; while (i--) {sum += dievals_get(sorted_dievals,i);}
    return sum; 
}
    
u8 score_yahtzee(DieVals sorted_dievals) { 
    DieVal val_first = dievals_get(sorted_dievals,0);
    DieVal val_last = dievals_get(sorted_dievals,4);
    if (val_first==0) {return 0;}
    return (val_first == val_last ? 50 : 0) ;
}

// reports the score for a set of dice in a given slot w/o regard for exogenous gamestate (bonuses, yahtzee wildcards etc) 
u8 score_slot_with_dice(Slot slot, DieVals sorted_dievals) { 
    if (slot==ACES) {return score_aces(sorted_dievals);}  
    if (slot==TWOS) {return score_twos(sorted_dievals);}  
    if (slot==THREES) {return score_threes(sorted_dievals);}  
    if (slot==FOURS) {return score_fours(sorted_dievals);}  
    if (slot==FIVES) {return score_fives(sorted_dievals);}  
    if (slot==SIXES) {return score_sixes(sorted_dievals);}  
    if (slot==THREE_OF_A_KIND) {return score_three_of_a_kind(sorted_dievals);}  
    if (slot==FOUR_OF_A_KIND) {return score_four_of_a_kind(sorted_dievals);}  
    if (slot==SM_STRAIGHT) {return score_sm_str8(sorted_dievals);}  
    if (slot==LG_STRAIGHT) {return score_lg_str8(sorted_dievals);}  
    if (slot==FULL_HOUSE) {return score_fullhouse(sorted_dievals);}  
    if (slot==CHANCE) {return score_chance(sorted_dievals);}  
    if (slot==YAHTZEE) {return score_yahtzee(sorted_dievals);}  
    assert(0);// shouldn't get here
}

//-------------------------------------------------------------
//GameState
//-------------------------------------------------------------

GameState gamestate_init(DieVals sorted_dievals, Slots open_slots, u8 upper_total, 
                    u8 rolls_remaining, bool yahtzee_bonus_avail) { 
    GameState self = (GameState){};
    u8 dievals_id = SORTED_DIEVALS[sorted_dievals].id; // this is the 8-bit encoding of self.sorted_dievals
    self.id =  (u32)dievals_id;                        // self.id will use 30 bits total...
    self.id |= (u32)(open_slots)               << 7;   // slots.data doesn't use its rightmost bit so we only shift 7 to make room for the 8-bit dieval_id above 
    self.id |= (u32)(upper_total)              << 21;  // make room for 13+8 bit stuff above 
    self.id |= (u32)(rolls_remaining)          << 27;  // make room for the 13+8+6 bit stuff above
    self.id |= (u32)(yahtzee_bonus_avail?1:0)  << 29;  // make room for the 13+8+6+2 bit stuff above
    self.sorted_dievals = sorted_dievals;
    self.open_slots = open_slots;
    self.upper_total = upper_total;
    self.rolls_remaining = rolls_remaining;
    self.yahtzee_bonus_avail = yahtzee_bonus_avail;
    return self;
} 

// calculate relevant counts for gamestate: required lookups and saves
u64 counts(GameState self) { 
    u64 ticks = 0; 
    Slots powerset[8192]; int powerset_len = 0;
    slots_powerset(self.open_slots, powerset, &powerset_len);
    for(int i=1; i<powerset_len; i++) {
        Slots slots = powerset[i];
        bool joker_rules = !slots_has(slots,YAHTZEE); // yahtzees aren't wild whenever yahtzee slot is still available 
        Ints64 totals = useful_upper_totals(slots);
        for(int j=0; j<totals.count; j++) {
            for (int k=0; k<=joker_rules; k++){
//                int subset_len = slots_count(slots);
//                int slot_lookups = (subset_len * subset_len==1? 1 : 2) * 252 ;
                ticks+=1; // this just counts the cost of one pass through the bar.tick call in the dice-choose section of build_cache() loop
            }   
        } 
    }
    return ticks;
} 

u8 min_u8(u8 a, u8 b){return a<b?a:b;}

u8 score_first_slot_in_context(GameState self) { 

    assert(self.open_slots!=0);

    // score slot itself w/o regard to game state 
        Slot slot = slots_get(self.open_slots,0); // first slot in open_slots
        u8 score = score_slot_with_dice(slot, self.sorted_dievals) ;

    // add upper bonus when needed total is reached 
        if (slot<=SIXES && self.upper_total<63){
            u8 new_total = min_u8(self.upper_total+score, 63); 
            if (new_total==63) { // we just reach bonus threshold
                score += 35;   // add the 35 bonus points 
            }
        } 

    // special handling of "joker rules" 
        int just_rolled_yahtzee = score_yahtzee(self.sorted_dievals)==50;
        bool joker_rules_in_play = (slot != YAHTZEE); // joker rules in effect when the yahtzee slot is not open 
        if (just_rolled_yahtzee && joker_rules_in_play){ // standard scoring applies against the yahtzee dice except ... 
            if (slot==FULL_HOUSE) {score=25;}
            if (slot==SM_STRAIGHT){score=30;}
            if (slot==LG_STRAIGHT){score=40;}
        } 

    // # special handling of "extra yahtzee" bonus per rules
        if (just_rolled_yahtzee && self.yahtzee_bonus_avail) {score+=100;}

    return score;
} 

//-------------------------------------------------------------
//    BUILD_CACHE
//-------------------------------------------------------------

// gather up expected values in a multithreaded bottom-up fashion. this is like.. the main thing
void build_ev_cache(GameState game) {

    Range range = outcomes_range_for(0b11111); 
    DieVals placeholder_dievals = (DieVals)0;

    // first handle special case of the most leafy leaf calcs -- where there's one slot left and no rolls remaining
    int len = slots_count(game.open_slots);
    for(int i=0; i<len; i++){
        Slot single_slot = slots_get(game.open_slots, i);
        Slots single_slot_set = slots_init_va(1,single_slot); // set of a single slot 
        bool joker_rules_in_play = (single_slot != YAHTZEE); // joker rules in effect when the yahtzee slot is not open 
        for (int ii=0; ii<=joker_rules_in_play; ii++){ // yahtzee bonus -might- be available when joker rules are in play 
            bool yahtzee_bonus_available = (bool)ii;
            // for upper_total in slot.useful_upper_totals() {
            Ints64 upper_totals = useful_upper_totals(single_slot_set);
            for(int iii=0; iii<upper_totals.count; iii++){
                u8 upper_total = upper_totals.arr[iii]; 
                // for outcome_combo in all_dieval_combos{
                for(int iv=range.start; iv<range.stop; iv++){
                    DieVals outcome_combo = OUTCOMES[iv].dievals;
                    GameState state = gamestate_init(outcome_combo, single_slot_set, upper_total, 0, yahtzee_bonus_available);
                    u8 score = score_first_slot_in_context(state);
                    ChoiceEV choice_ev = (ChoiceEV){single_slot, (f32)score};
                    EV_CACHE[state.id] = choice_ev;
                    output(state, choice_ev);
    } } } } 


    // for each slotset of each length 
    Slots slotspowerset[8192]; int slotspowerset_count=0; // 2^13=8192 is the size of the powerset of all slots
    slots_powerset(game.open_slots, slotspowerset, &slotspowerset_count);    
    for (int i=1; i<slotspowerset_count; i++){//skip empty set
        Slots slots = slotspowerset[i];
        int slots_len = slots_count(slots);
        bool joker_rules_in_play = !slots_has(slots,YAHTZEE); // joker rules might be in effect whenever the yahtzee slot is already filled 

        // for each upper total 
        Ints64 upper_totals = useful_upper_totals(slots); 
        for(int ii=0; ii < upper_totals.count; ii++){
            u8 upper_total = upper_totals.arr[ii];

            // for each yahtzee bonus possibility 
            for(int iii=0; iii<=joker_rules_in_play; iii++){
                bool yahtzee_bonus_available = (bool)iii;

                tick(); // advance the progress bar 

                // for each rolls remaining
                for(int rolls_remaining=0; rolls_remaining<=3; rolls_remaining++){

                    if (rolls_remaining==3) range = (Range){0,1}; 
                    else range = outcomes_range_for(0b11111);

                    // for dieval_combo in dieval_combos { //Threads.@threads :static 
                    for(int iv=range.start; iv<range.stop; iv++){
                        DieVals dieval_combo=OUTCOMES[iv].dievals;
                        process_dieval_combo(
                            rolls_remaining,
                            slots_len,
                            slots,
                            dieval_combo,
                            joker_rules_in_play,
                            yahtzee_bonus_available,
                            upper_total
                        );
    } } } } }

}

void process_dieval_combo(u8 rolls_remaining , int slots_len, Slots slots, DieVals dieval_combo, 
    bool joker_rules_in_play, bool yahtzee_bonus_available, u8 upper_total) { 

    usize threadid = 0; //TODO implement actual threading // threadid = Threads.threadid()

    if (rolls_remaining==0 && slots_len > 1) { // slot selection, but not for already recorded leaf calcs  
                                               // TODO the slots_len > 1 check should be handeled further up right after slots_len is calculated

        //= HANDLE SLOT SELECTION  =//

        ChoiceEV best=(ChoiceEV){0,0};

        // for slot in slots { 
        for(int i=0; i<slots_len; i++) {   

            Slot slot = slots_get(slots,i);

            // joker rules say extra yahtzees must be played in their matching upper slot if it's available
            DieVal first_dieval = dievals_get(dieval_combo,0);
            bool joker_rules_matter = (joker_rules_in_play && score_yahtzee(dieval_combo)>0 && slots_has(slots,first_dieval));
            Slot head_slot = (joker_rules_matter ? first_dieval : slot);

            bool yahtzee_bonus_avail_now = yahtzee_bonus_available;
            u8 upper_total_now = upper_total;
            DieVals dievals_or_placeholder = dieval_combo;
            f32 head_plus_tail_ev = 0.0;
            u8 rolls_remaining_now = 0;

            // find the collective ev for the all the slots with this iteration's slot being first 
            // do this by summing the ev for the first (head) slot with the ev value that we look up for the remaining (tail) slots
            int passes = (slots_len==1 ? 1 : 2); // to do this, we need two passes unless there's only 1 slot left //TODO slots_len is ALWAYS > 1 here!
            for(int ii=1; ii<=passes; ii++) {
                Slots slots_piece = ii==1? slots_init_va(1,head_slot) : slots_removing(slots, head_slot);  // work on the head only, or the set of slots without the head
                u8 relevant_upper_total = (upper_total_now + best_upper_total(slots_piece) >= 63)? upper_total_now : 0;  // only relevant totals are cached
                GameState state_to_get = gamestate_init(
                    dievals_or_placeholder,
                    slots_piece, 
                    relevant_upper_total,
                    rolls_remaining_now, 
                    yahtzee_bonus_avail_now
                );
                ChoiceEV choice_ev = EV_CACHE[state_to_get.id];
                if (ii==1 && slots_len>1) {// prep 2nd pass on relevant 1st pass only..  //TODO don't need slots_len>1 check here
                    // going into tail slots next, we may need to adjust the state based on the head choice
                    if (choice_ev.choice <= SIXES){  // adjust upper total for the next pass 
                        u8 added = fmod(choice_ev.ev , 100); // the modulo 100 here removes any yahtzee bonus from ev since that doesnt' count toward upper bonus total
                        upper_total_now = min(63, upper_total_now + added);
                    } else if (choice_ev.choice==YAHTZEE) {  // adjust yahtzee related state for the next pass
                        if (choice_ev.ev>0.0) {yahtzee_bonus_avail_now=true;}
                    } 
                    rolls_remaining_now=3; // for upcoming tail lookup, we always want the ev for 3 rolls remaining
                    dievals_or_placeholder = (DieVals)0; // for 3 rolls remaining, use "wildcard" representative dievals since dice don't matter when rolling all of them
                }
                head_plus_tail_ev += choice_ev.ev;
            }//for i in passes 

            if (head_plus_tail_ev >= best.ev) { 
                best = (ChoiceEV){slot, head_plus_tail_ev};
            } 
            
            if (joker_rules_matter) break; // if joker-rules-matter we were forced to choose one slot, so we can skip trying the rest  

        }//for slot in slots                               

        GameState state_to_set = gamestate_init(
            dieval_combo,
            slots,
            upper_total, 
            0, //rolls_remaining
            yahtzee_bonus_available
        ); 
        output(state_to_set, best);
        EV_CACHE[state_to_set.id] = best;

    } else if (rolls_remaining > 0) {  
        
    //= HANDLE DICE SELECTION =//    

        u8 next_roll = rolls_remaining-1;
        ChoiceEV best = (ChoiceEV){0, 0.0};
        Ints32 selections = (rolls_remaining==3)? SELECTION_SET_OF_ALL_DICE_ONLY : SELECTION_SET_OF_ALL_POSSIBLE_SELECTIONS;
        
        // HOT LOOP !
        // for each possible selection of dice from this starting dice combo, 
        // we calculate the expected value of rolling that selection, then store the best selection along with its EV 
        for(int i=0; i<selections.count; i++) {
            Selection selection = selections.arr[i];
            f32 avg_ev_for_selection = avg_ev(dieval_combo, selection, slots, upper_total, next_roll,yahtzee_bonus_available, threadid); 
            if (avg_ev_for_selection > best.ev){
                best = (ChoiceEV){selection, avg_ev_for_selection};
            } 
        } 

        GameState state_to_set = gamestate_init( 
            dieval_combo,
            slots, 
            upper_total, 
            rolls_remaining, 
            yahtzee_bonus_available 
        ); 
        output(state_to_set, best);
        EV_CACHE[state_to_set.id] = best;

    }// if rolls_remaining...  
}// process_dieval_combo


// calculates the average EV for a dice selection from a starting dice combo 
// within the context of the other relevant gamestate variables
f32 avg_ev(DieVals start_dievals, Selection selection, Slots slots, u8 upper_total, 
            u8 next_roll, bool yahtzee_bonus_available, usize threadid) { 

    f32 total_ev_for_selection = 0.0 ;
    f32 outcomes_arrangements_count = 0.0;
    Range range = outcomes_range_for(selection);

    GameState game = gamestate_init(
        (DieVals)0,
        slots, 
        upper_total, 
        next_roll, // we'll average all the 'next roll' possibilities (which we'd calclated on the last pass) to get ev for 'this roll' 
        yahtzee_bonus_available 
    );
    usize floor_state_id = game.id ; 
    // from this floor gamestate we can blend in a dievals_id to quickly calc the index we need to access the ev for the complete state 

    // blit all each roll outcome for the given dice selection onto the unrolled start_dievals 
    // and stash results in the NEWVALS_BUFFER 
    for (int i=range.start; i<range.stop; i++) { // we can SIMD this loop but not the next one 
        NEWVALS_BUFFER[threadid][i] = (start_dievals & OUTCOME_MASKS[i]); //make some holes in the dievals for newly rolled die vals 
        NEWVALS_BUFFER[threadid][i] |= OUTCOME_DIEVALS[i]; // fill in the holes with the newly rolled die vals
    } 

    for (usize i=range.start; i<range.stop; i++) { // this loop is a bunch of lookups so doesn't benefit from SIMD
        //= gather sorted =#
            usize newvals_datum = NEWVALS_BUFFER[threadid][i];
            usize sorted_dievals_id  = SORTED_DIEVALS[newvals_datum].id;
        //= gather ev =#
            usize state_to_get_id = floor_state_id | sorted_dievals_id;
            ChoiceEV cache_entry = EV_CACHE[state_to_get_id];
            OUTCOME_EVS_BUFFER[threadid][i] = cache_entry.ev;
    } 

    for (usize i=range.start; i<range.stop; i++) { // this loop is all math so eligble for SIMD optimization
        // we have EVs for each "combination" but we need the average all "permutations" 
        // -- so we mutliply by the number of distinct arrangements for each combo 
        EVS_TIMES_ARRANGEMENTS_BUFFER[threadid][i] = OUTCOME_EVS_BUFFER[threadid][i] * OUTCOME_ARRANGEMENTS[i];
        total_ev_for_selection +=  EVS_TIMES_ARRANGEMENTS_BUFFER[threadid][i];
        outcomes_arrangements_count += OUTCOME_ARRANGEMENTS[i];
    } 

    // this final step gives us the average EV for all permutations of rolled dice 
    return total_ev_for_selection / outcomes_arrangements_count; 

} // avg_ev

void init_caches(){

    CORES = 1; // default to 1 core

    OUTCOME_EVS_BUFFER = malloc(CORES * sizeof(f32*));
    for (int i = 0; i < CORES; i++) { OUTCOME_EVS_BUFFER[i] = malloc(1683 * sizeof(f32)); }

    NEWVALS_BUFFER = malloc(CORES * sizeof(u16*));
    for (int i = 0; i < CORES; i++) { NEWVALS_BUFFER[i] = malloc(1683 * sizeof(DieVals)); }

    EVS_TIMES_ARRANGEMENTS_BUFFER = malloc(CORES * sizeof(f32*));
    for (int i = 0; i < CORES; i++) { EVS_TIMES_ARRANGEMENTS_BUFFER[i] = malloc(1683 * sizeof(f32)); }

    // setup helper values
    cache_selection_ranges(); 
    cache_sorted_dievals(); 
    cache_roll_outcomes_data();

    // selection sets
    SELECTION_SET_OF_ALL_DICE_ONLY = (Ints32){ 1, 0b11111 }; //  selections are bitfields where '1' means roll and '0' means don't roll 
    SELECTION_SET_OF_ALL_POSSIBLE_SELECTIONS = (Ints32){}; // Ints32 type can hold 32 different selections 
    for(int i=0b00000; i<=0b11111; i++) SELECTION_SET_OF_ALL_POSSIBLE_SELECTIONS.arr[i]=i; 
    SELECTION_SET_OF_ALL_POSSIBLE_SELECTIONS.count=32;

    //gignormous cache for holding EVs of all game states
    //    // EV_CACHE = (ChoiceEV(*)[1073741824])malloc(pow(2,30) * sizeof(ChoiceEV)); // 2^30 slots hold all unique game states 
    EV_CACHE = malloc(pow(2,30) * sizeof(ChoiceEV)); // 2^30 slots hold all unique game states 
 
}

//-------------------------------------------------------------
// MAIN 
//-------------------------------------------------------------

int main() {
    
    init_caches();
    //define a particular game state to test
    GameState game = gamestate_init( 
        // dievals_from_arr5( (int[5]) {3,4,4,6,6} ), slots_from_ints16((Ints16){1,{1}}), 0, 1, false //  0.8333 per Swift
        // dievals_from_arr5( (int[5]) {3,4,4,6,6} ), slots_from_ints16((Ints16){3,{4,5,6}}), 0, 2, false// 38.9117 per Swift
        // dievals_from_arr5( (int[5]) {3,4,4,6,6} ), slots_from_ints16((Ints16){8,{1,2,8,9,10,11,12,13}}), 0, 2, false// 137.3749 per Swift
        // dievals_from_arr5( (int[5]) {0,0,0,0,0} ), slots_from_ints16((Ints16){6,{1,2,3,4,5,6,}}), 0, 3, false// 

        // dievals_from_arr5( (int[5]) {3,4,4,6,6} ), slots_from_ints16((Ints16){4,{1,2,3,4}}), 0, 2, false//  Julia 27.0865 == GOT 27.0865 
        // dievals_from_arr5( (int[5]) {3,4,4,6,6} ), slots_from_ints16((Ints16){2,{5,6}}), 0, 2, false //  Julia 28.0668 == GOT 28.0668
        // dievals_from_arr5( (int[5]) {3,4,4,6,6} ), slots_from_ints16((Ints16){3,{4,5,6}}), 0, 2, false //  Julia 38.9117 == GOT 38.9117 
        // dievals_from_arr5( (int[5]) {3,4,4,6,6} ), slots_from_ints16((Ints16){4,{3,4,5,6}}), 0, 2, false//  Julia 49.4368 == GOT 49.4368 
        // dievals_from_arr5( (int[5]) {1,1,1,1,2} ), slots_from_ints16((Ints16){2,{3,4}}), 36, 1, false//  Julia 12.28 == GOT 12.28 
        // dievals_from_arr5( (int[5]) {3,4,4,6,6} ), slots_from_ints16((Ints16){7,{7,8,9,10,11,12,13}}), 0, 2, false//  Julia 141.109 == GOT 141.1090 

        // dievals_from_arr5( (int[5]) {3,4,4,6,6} ), slots_from_ints16((Ints16){5,{2,3,4,5,6}}), 0, 2, false// 61.1906 Julia !=  GOT 61.1906 
        dievals_from_arr5( (int[5]) {3,4,4,6,6} ), slots_from_ints16((Ints16){5,{1,2,3,4,5}}), 0, 2, false// 41.2435 Julia !=  GOT 40.8427
 
        // dievals_from_arr5( (int[5]) {3,4,4,6,6} ), slots_from_ints16((Ints16){6,{1,2,3,4,5,6}}), 0, 2, false// 72.435 per Julia !=  GOT 69.7463
        // dievals_from_arr5( (int[5]) {0,0,0,0,0} ), slots_from_ints16((Ints16){13,{1,2,3,4,5,6,7,8,9,10,11,12,13}}), 0, 3, false // should be 254.5896 got 238.06 :(  
    );  



    // setup progress bar 
    init_bar_for(game);

    // crunch crunch 
    build_ev_cache(game);

    // and the answer is...
    printf("EV of test gamestate : %.4f\n", EV_CACHE[game.id].ev);

    // run_tests();    

}

// D,00031,21111,1,36,N,3_4_,