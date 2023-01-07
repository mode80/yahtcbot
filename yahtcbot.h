#include <stdio.h> // printf
#include <unistd.h> // sysconf for thread count
#include <stdbool.h> // bools
#include <stdlib.h> // rand, srand, malloc, free
#include <assert.h> // assert 
#include <string.h> //strcmp, strlen, etc
#include <math.h> // pow, sqrt, 
#include <stdarg.h> // pow, sqrt, 
#include <limits.h> // INT_MIN
// #include <pthread.h> // threads

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef size_t usize;
typedef float f32;
typedef double f64; // lazy Rust-like abbreviations

typedef u8 Selection; 
typedef u8 Choice;
typedef u8 DieVal;
typedef u8 Slot;

typedef struct { size_t count; int arr[8]; } Ints8;
typedef struct { size_t count; int arr[16]; } Ints16;
typedef struct { size_t count; int arr[32]; } Ints32;
typedef struct { size_t count; int arr[64]; } Ints64;
typedef struct { size_t count; int arr[128]; } Ints128;


const int SENTINEL; 
typedef struct { int start; int stop; } Range;
int cores;

typedef u16 DieVals ; // 5 dievals, each from 0 to 6, can be encoded in 2 bytes total, each taking 3 bits{ 
typedef u16 Slots ;  // 13 sorted Slots can be positionally encoded in one u16

Slot ACES; Slot TWOS; Slot THREES;
Slot FOURS; Slot FIVES; Slot SIXES;
Slot THREE_OF_A_KIND; Slot FOUR_OF_A_KIND;
Slot FULL_HOUSE; Slot SM_STRAIGHT; 
Slot LG_STRAIGHT; Slot YAHTZEE; Slot CHANCE;

typedef struct ChoiceEV {
    Choice choice;
    f32 ev;
} ChoiceEV;

typedef struct DieValsID {  
    DieVals dievals;
    u8 id;  // the id is a kind of offset that later helps us fast-index into the ev_cache 
            // it's also an 8-bit handle to the 16-bit DieVals for more compact storage within a 32bit GameState ID
} DieValsID;

typedef struct Outcome { 
    DieVals dievals;
    DieVals mask; // stores a pre-made mask for blitting this outcome onto a GameState.DieVals.data u16 later
    f32 arrangements; // how many indistinguishable ways can these dievals be arranged (ie swapping identical dievals don't count)
} Outcome;

typedef struct GameState {
    u32 id; // 30 bits # with the id, 
    //we can store all of below in a sparse array using 2^(8+13+6+2+1) entries = 1_073_741_824 entries = 5.2GB when storing 40bit ResultEVs 
    DieVals sorted_dievals;// 15 bits OR 8 bits once convereted to a DieValID (252 possibilities)
    Slots open_slots;// 13 bits        = 8_192 possibilities
    u8 upper_total;// = 6 bits         = 64    "
    u8 rolls_remaining;// = 2 bits     = 4     "  
    bool yahtzee_bonus_avail;// = 1bit = 2     "
} GameState;

int RANGE_IDX_FOR_SELECTION[32];
DieValsID SORTED_DIEVALS [32767]; //new DieValsID[32767];
Range SELECTION_RANGES[32];  //new Range[32];  
Outcome OUTCOMES[1683]; //new Outcome[1683]    
u16 OUTCOME_DIEVALS_DATA[1683]; //new u16[1683]  //these 3 arrays mirror that in OUTCOMES but are contiguous and faster to access
u16 OUTCOME_MASK_DATA[1683]; // new u16[1683] 
u16 OUTCOME_ARRANGEMENTS[1683]; //new f32[1683] 
ChoiceEV* ev_cache; //= malloc(pow_2_30 * sizeof(ChoiceEV); // 2^30 slots hold all unique game states 



Ints8 *powerset(const Ints8 items, size_t *result_count);

u64 factorial(u64 n);

u64 n_take_r(u64 n, u64 r, bool order_matters, bool with_replacement);

void make_combos_with_replacement(Ints8 items, int n, int r, Ints8 *indices, Ints8 *results, int *result_count);

Ints8 *get_combos_with_replacement(Ints8 items, int r, int *result_count);

float distinct_arrangements_for(Ints8 dieval_vec);

int countTrailingZeros(int x);

void swap(int *a, int *b);

void make_unique_perms(Ints8 items, int start, Ints8 *result, int *counter);

Ints8 *get_unique_perms(Ints8 items, int *result_count);

void print_state_choices_header();

DieVals dievals_empty();

DieVals dievals_init(int a, int b, int c, int d, int e);
 
DieVals dievals_from_arr5(int dievals[5]);

DieVals dievals_from_ints8(Ints8 dievals);

DieVals dievals_from_intstar(int *dievals, int count);

DieVal dievals_get(const DieVals self, int index);

Slots slots_empty();

Slots slots_init_va(int arg_count, ...);

Slot slots_get(Slots self, int index);

bool slots_has(Slots self, Slot slot);

int slots_len(Slots self);

Slots removing(Slots self, Slot slot_to_remove);

Slots used_upper_slots(Slots self);

void slots_powerset(Slots self, Slots *out, int *out_len);

u8 best_upper_total(Slots slots);

Ints64 useful_upper_totals(Slots self);

void cache_selection_ranges();

void cache_sorted_dievals();

void cache_roll_outcomes_data();

u8 upperbox(const u8 boxnum, const DieVals sorted_dievals);

u8 n_of_a_kind(const u8 n, const DieVals sorted_dievals);

u8 straight_len(const DieVals sorted_dievals);

u8 score_fours(DieVals sorted_dievals);

u8 score_fives(DieVals sorted_dievals);

u8 score_sixes(DieVals sorted_dievals);

u8 score_three_of_a_kind(DieVals sorted_dievals);

u8 score_four_of_a_kind(DieVals sorted_dievals);

u8 score_lg_str8(DieVals sorted_dievals);

u8 score_fullhouse(DieVals sorted_dievals);

u8 score_yahtzee(DieVals sorted_dievals);

u8 score_slot_with_dice(Slot slot, DieVals sorted_dievals);

GameState gamestate_init(DieVals sorted_dievals, Slots open_slots, u8 upper_total, u8 rolls_remaining, bool yahtzee_bonus_avail);

u64 counts(GameState self);

u8 score_first_slot_in_context(GameState self);
