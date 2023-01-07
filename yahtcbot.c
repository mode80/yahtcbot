#include "yahtcbot.h"
void run_tests();  // forward declare unit test runner found in test.c

Slot ACES = 1; Slot TWOS = 2; Slot THREES = 3;
Slot FOURS = 4; Slot FIVES = 5; Slot SIXES = 6;
Slot THREE_OF_A_KIND = 7; Slot FOUR_OF_A_KIND = 8;
Slot FULL_HOUSE = 9; Slot SM_STRAIGHT = 10; 
Slot LG_STRAIGHT = 11; Slot YAHTZEE = 12; Slot CHANCE = 13 ;

int RANGE_IDX_FOR_SELECTION[32] = {0,1,2,3,7,4,8,11,17,5,9,12,20,14,18,23,27,6,10,13,19,15,21,24,28,16,22,25,29,26,30,31} ;
const int SENTINEL=INT_MIN;


//-------------------------------------------------------------
//  UTILS
//-------------------------------------------------------------

#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

// typedef struct { size_t count; ints8 arr[32]; } ints8s;

//powerset for a given set of elements
ints8* powerset(const ints8 items, size_t* result_count) {
    *result_count = (1 << items.count); //set_size of powerset of n elements is (2**n) w the empty set
    ints8* result = malloc(*result_count * sizeof(ints8));
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

void save_combo_to_results(ints8 items, ints8 *indices, int item_count, ints8* results, int* result_count) {
    //helper function to _get_combos_with_replacement
    results[*result_count].count = item_count;    
    for (int i=0; i<item_count; i++) {
        results[*result_count].arr[i] = items.arr[indices->arr[i]];
    }
    (*result_count)++;
}

void make_combos_with_replacement(ints8 items, int n, int r, ints8 *indices, ints8* results, int* result_count) {
    //helper function to _get_combos_with_replacement
    save_combo_to_results(items, indices, r, results, result_count);
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

ints8* get_combos_with_replacement(ints8 items, int r, int* result_count) { 
    //combinations_with_replacement('ABC', 2) --> AA AB AC BB BC CC"
    // number items returned:  (n+r-1)! / r! / (n-1)!
    assert(r<=8);
    ints8 indices = {r, {0,0,0,0,0,0,0,0} };
    int n = items.count;
    ints8* results = malloc(n_take_r(n,r,false,true) * sizeof(ints8));
    *result_count=0;
    make_combos_with_replacement(items, n, r, &indices, results, result_count);
    return results;
}

float distinct_arrangements_for(ints8 dieval_vec) {
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
    // free(key_counts);
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

void make_unique_perms(ints8 items, int start, ints8 *result, int *counter) {
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

ints8* get_unique_perms(ints8 items, int* result_count) {
    assert(items.count > 0);
    int n = factorial(items.count);// this is too big when there are dupes, but sufficient otherwise
    ints8* result = malloc(sizeof(ints8) * n);//map malloced memory onto a locally sized var for easier debugging  
    *result_count = 0;
    make_unique_perms(items, 0, result, result_count);
    return result;
}

void print_state_choices_header() { 
    printf("choice_type,choice,dice,rolls_remaining,upper_total,yahtzee_bonus_avail,open_slots,expected_value");
} 

// // should produce one line of output kinda like ...
// // D,01111,65411,2,31,Y,1_3_4_6_7_8_11_,119.23471
// // S,13,66641,0,11,Y,3_4_5_9_10_13_,113.45208
// func print_state_choice(_ state :GameState , _ choice_ev: ChoiceEV ) { 
//     let Y="Y"; let N="N"; let S="S"; let D="D"; let C=","; // TODO hoist these to constants
//     var sb:String=""; sb.reserveCapacity(60)
//     if (state.rolls_remaining==0){ // for slot choice 
//         sb += (S); sb+=(C);
//         sb += (choice_ev.choice.description); // for dice choice 
//     } else {
//         sb+=(D); sb+=(C);
//         sb+=("00000"+choice_ev.choice.description).suffix(5)
//     }
//     sb+=(C);
//     sb+=(state.sorted_dievals.description); sb+=(C);
//     sb+=(state.rolls_remaining.description); sb+=(C);
//     sb+=(state.upper_total.description); sb+=(C);
//     sb+=(state.yahtzee_bonus_avail ? Y : N); sb+=(C);
//     sb+=(state.open_slots.description); sb+=(C);
//     sb+=(choice_ev.ev.description);
//     // sb+=(C); sb+=(state.id.description);
//     print(sb);
// } 

//#=-------------------------------------------------------------
//DieVals
//-------------------------------------------------------------=#

DieVals dievals_empty() { return 0; }

DieVals dievals_init(DieVal dievals[5]) {
    u16 result = 0;
    for (int i = 0; i < 5; i++){ 
        result |= (dievals[i] << (i*3));
    } 
    return result;
}

DieVals dievals_from_5ints(int dievals[5]) {
    u16 result = 0;
    for (int i = 0; i < 5; i++){ 
        result |= (dievals[i] << (i*3));
    } 
    return result;
}

DieVals dievals_from_ints8(ints8 dievals) {
    u16 result = 0;
    for (int i = 0; i < dievals.count; i++){ 
        result |= (dievals.arr[i] << (i*3));
    } 
    return result;
}

DieVals dievals_from_intstar(int* dievals, int count) {
    u16 result = 0;
    for (int i = 0; i < count; i++){ 
        result |= (dievals[i] << (i*3));
    } 
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

    int slots_len(Slots self){ 
        int len = 0;
        for (int i=1; i<=13; i++){ if (slots_has(self, i)) {len++;} }
        return len; 
    } 

    Slots removing(Slots self, Slot slot_to_remove) { 
        u16 mask = ~( 1 << slot_to_remove );
        return (self & mask); //# force off
    } 

    int* zero_thru_63 = (int[64]){0,1,2,3,4,5,6,7,8,9, 10,11,12,13,14,15,16,17,18,19,
            20,21,22,23,24,25,26,27,28,29, 30,31,32,33,34,35,36,37,38,39,
            40,41,42,43,44,45,46,47,48,49, 50,51,52,53,54,55,56,57,58,59, 60,61,62,63};

    Slots used_upper_slots(Slots self) {
        Slots all_bits_except_unused_uppers = ~self; // "unused" slots are not "previously used", so blank those out
        Slots all_upper_slot_bits = (u16) ((1<<7)-2);  // upper slot bits are those from 2^1 through 2^6 (encoding doesn't use 2^0)
        Slots previously_used_upper_slot_bits = (u16) (all_bits_except_unused_uppers & all_upper_slot_bits);
        return previously_used_upper_slot_bits;
    } 

    void slots_powerset(Slots self, Slots* out, int* out_len) { 
        int len = slots_len(self);
        int* powerset = zero_thru_63;
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
    ints64 useful_upper_totals(Slots self) { 
        int* totals = zero_thru_63;
        Slots used_uppers = used_upper_slots(self);
        bool all_even = true;
        for (int i=1; i<=13; i++){ 
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
        int best_unused_slot_total = best_upper_total(self);
        // totals = (x for x in totals if x + best_unused_slot_total >=63 || x==0)
        // totals = from x in totals where (x + best_unused_slot_total >=63 || x==0) select x
        ints64 result = {};
        int j=0;
        for (int i=0; i<64; i++){ 
            if (totals[i]!=SENTINEL && totals[i] + best_unused_slot_total >= 63 || totals[i]==0) {
                result.arr[j]=totals[i];
                j++;
            }
        } 
        return result;  
    }

// returns a range which corresponds the precomputed dice roll outcome data corresponding to the given selection
Range outcome_range_for(Selection selection){
    int idx = RANGE_IDX_FOR_SELECTION[selection];
    Range range = SELECTION_RANGES[idx]; 
    return range;
} 


//-------------------------------------------------------------
// INITIALIZERS 
//-------------------------------------------------------------

// this generates the ranges that correspond to the outcomes, within the set of all outcomes, indexed by a give selection 
void cache_selection_ranges() {

    int s = 0;

    ints8 idxs0to4 = (ints8){5, {0,1,2,3,4} };
    size_t result_count = 0; 
    ints8* combos = powerset( idxs0to4, &result_count);

    for(int i=0; i<result_count; i++) {
        int sets_count = n_take_r(6, combos[i].count, false, true) ;
        SELECTION_RANGES[i] = (Range){s, s+sets_count}; 
        s += sets_count;
    } 

    free(combos);
}


// for fast access later, this generates an array of dievals in sorted form, 
// along with each's unique "ID" between 0-252, indexed by DieVals data
void cache_sorted_dievals() { 
    
    SORTED_DIEVALS[0] = (DieValsID){}; // first one is for the special wildcard 
    ints8 one_to_six = {.count=6, .arr={1,2,3,4,5,6} }; 
    int combos_size;
    ints8* combos = get_combos_with_replacement(one_to_six, 5, &combos_size);
    int perm_count=0;
    for (int i=0; i<combos_size; i++) {
        DieVals dv_sorted = dievals_from_ints8(combos[i]);
        ints8* ints8_perms = get_unique_perms(combos[i], &perm_count);
        for (int j=0; j<perm_count; j++) {
            ints8 perm = ints8_perms[j];
            DieVals dv_perm = dievals_from_ints8(perm);
            SORTED_DIEVALS[dv_perm] = (DieValsID){.dievals=dv_sorted, .id=i};
        }
    }
}


//preps the caches of roll outcomes data for every possible 5-die selection, where '0' represents an unselected die """
void cache_roll_outcomes_data() { 

    int i=0; size_t idx_combo_count=0; 
    ints8* idx_combos = powerset( (ints8){.count=5,.arr={0,1,2,3,4}}, &idx_combo_count );
    assert(idx_combo_count==32); 
    ints8 one_thru_six = {6, {1,2,3,4,5,6}}; 

    for (int v=1; v<idx_combo_count; v++) { //start at 1 because the 0th is the empty set
        DieVal dievals_arr[5] = {0,0,0,0,0}; 
        ints8 idx_combo = idx_combos[v];
        int die_count = idx_combo.count; 
        
        int die_combos_size = 0;
        // combos_with_rep(one_thru_six, 6, die_count, &result, &combos_size); 
        ints8* die_combos = get_combos_with_replacement(one_thru_six, die_count, &die_combos_size);
 
        for (int w=0; w<die_combos_size; w++) {
            ints8 die_combo = die_combos[w];
            u8 mask_vec[5] = {0b111,0b111,0b111,0b111,0b111};
            for(int j=0; j<die_count; j++) {
                int idx = idx_combo.arr[j];
                dievals_arr[idx] = (DieVal)die_combo.arr[j];
                mask_vec[idx]=0;
            }
            int arrangement_count = distinct_arrangements_for(die_combo);
            DieVals dievals = dievals_init(dievals_arr);
            DieVals mask = dievals_init(mask_vec);
            OUTCOME_DIEVALS_DATA[i] = dievals;
            OUTCOME_MASK_DATA[i] = mask;
            OUTCOME_ARRANGEMENTS[i] = arrangement_count;
            OUTCOMES[i] = (Outcome){ dievals, mask, arrangement_count};
            i+=1;
            assert(i<=1683);
        } 

        free(die_combos);
    } 
    free(idx_combos);
} 

/*

func init_bar_for(_ game :GameState) {
    bar = Tqdm(total:game.counts())
} 

func output(state :GameState, choice_ev :ChoiceEV ){ 
    // Uncomment below for more verbose progress output at the expense of speed 
    // print_state_choice(state, choice_ev)
} 

*/

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
    if (val_first==0) {return 0;} ; 
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
    u8 dievals_id = SORTED_DIEVALS[sorted_dievals].id;  // this is the 8-bit encoding of self.sorted_dievals
    self.id =  (u32)dievals_id;                         // self.id will use 30 bits total...
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
    Slots powerset[64]; int powerset_len = 0;
    slots_powerset(self.open_slots, powerset, &powerset_len);
    for(int i=0; i<powerset_len; i++) {
        Slots slots = powerset[i];
        bool joker_rules = slots_has(slots,YAHTZEE); // yahtzees aren't wild whenever yahtzee slot is still available 
        ints64 totals = useful_upper_totals(slots);
        for(int j=0; j<totals.count; j++) {
            for (int k=0; i<=joker_rules?1:0; k++){
                int subset_len = slots_len(slots);
                int slot_lookups = (subset_len * subset_len==1? 1 : 2) * 252 ;
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
// MAIN 
//-------------------------------------------------------------

int main() {
// commented out for now so we can build with main() living the test.c file that #includes this one
    ChoiceEV* ev_cache = malloc(pow_2_30 * sizeof(ChoiceEV)); // 2^30 slots hold all unique game states 

    run_tests();    

}

