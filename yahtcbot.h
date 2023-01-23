#include <stdio.h> // printf
#include <unistd.h> // sysconf for thread count
#include <stdbool.h> // bools
#include <stdlib.h> // rand, srand, malloc, free
#include <assert.h> // assert 
#include <string.h> //strcmp, strlen, etc
#include <stdarg.h> // pow, sqrt, 
#include <limits.h> // INT_MIN
#include <pthread.h> // threads
#include <math.h> // pow, sqrt, 
// #include <hwloc.h> // cores count 

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

typedef u16 DieVals ; // 5 dievals, each from 0 to 6, can be encoded in 2 bytes total, each taking 3 bits{ 
typedef u16 Slots ;  // 13 sorted Slots can be positionally encoded in one u16

Slot ACES; Slot TWOS; Slot THREES;
Slot FOURS; Slot FIVES; Slot SIXES;
Slot THREE_OF_A_KIND; Slot FOUR_OF_A_KIND;
Slot FULL_HOUSE; Slot SM_STRAIGHT; 
Slot LG_STRAIGHT; Slot YAHTZEE; Slot CHANCE;

// typedef struct ChoiceEV {
//     Choice choice;
//     f32 ev;
// } ChoiceEV;

// typedef struct DieValsID {  
//     DieVals dievals;
//     u8 id;  // the id is a kind of offset that later helps us fast-index into the EV_CACHE 
//             // it's also an 8-bit handle to the 16-bit DieVals for more compact storage within a 32bit GameState ID
// } DieValsID;

// typedef struct Outcome {  // an outcome represents one way a subset of 5 dievals could turn out after being rolled
//     DieVals dievals; // a set of 5 values where 0 means "unrolled" and other values are 1-6 representing the outcome of the roll
//     DieVals mask; // stores a corresponding mask handy for later blitting this outcome onto outer pre-rolled DieVals to get the post-roll state 
//     f32 arrangements; // how many distinguishable ways can these dievals be arranged (ie swapping identical dievals don't count)
// } Outcome;

typedef struct GameState {
    u32 id; // 30 bits # with the id, 
    //we can store all of below in a sparse array using 2^(8+13+6+2+1) entries = 1_073_741_824 entries = 5.2GB when storing 40bit ResultEVs 
    DieVals sorted_dievals;// 15 bits OR 8 bits once convereted to a DieValID (252 possibilities)
    Slots open_slots;// 13 bits        = 8_192 possibilities
    u8 upper_total;// = 6 bits         = 64    "
    u8 rolls_remaining;// = 2 bits     = 4     "  
    bool yahtzee_bonus_avail;// = 1bit = 2     "
} GameState;

typedef struct ProcessChunkArgs { 
    Slots slots;
    u8 slots_len;
    u8 upper_total;
    u8 rolls_remaining;
    bool joker_rules_in_play;
    Range range_for_thread; 
    int thread_id; 
} ProcessChunkArgs;

f32** OUTCOME_EVS_BUFFER;
DieVals** NEWVALS_BUFFER ;

int RANGE_IDX_FOR_SELECTION[32];
DieVals SORTED_DIEVALS [32767]; 
f32 SORTED_DIEVALS_ID [32767]; 
Range SELECTION_RANGES[32];  
DieVals OUTCOME_DIEVALS[1683]; 
DieVals OUTCOME_MASKS[1683]; 
f32 OUTCOME_ARRANGEMENTS[1683]; // could be a u8 but stored as f32 for faster final hotloop calculation
f32* EV_CACHE; // 2^30 slots hold all unique game state EVs
Choice* CHOICE_CACHE; 

Ints32 SELECTION_SET_OF_ALL_DICE_ONLY; //  selections are bitfields where '1' means roll and '0' means don't roll 
Ints32 SELECTION_SET_OF_ALL_POSSIBLE_SELECTIONS; // Ints32 type can hold 32 different selections 
 

Ints8* powerset(const Ints8 items, size_t *result_count);

u64 factorial(u64 n);

u64 n_take_r(u64 n, u64 r, bool order_matters, bool with_replacement);

void make_combos_with_replacement(Ints8 items, int n, int r, Ints8 *indices, Ints8 *results, int *result_count);

Ints8 *get_combos_with_replacement(Ints8 items, int r, int *result_count);

int min(int a, int b);

int max(int a, int b);

f32 distinct_arrangements_for(Ints8 dieval_vec);

int countTrailingZeros(int x);

void swap(int *a, int *b);

void make_unique_perms(Ints8 items, int start, Ints8 *result, int *counter);

Ints8* get_unique_perms(Ints8 items, int *result_count);

void print_state_choices_header();

DieVals dievals_empty();

DieVals dievals_init(int a, int b, int c, int d, int e);
 
DieVals dievals_from_arr5(const int dievals[5]);

DieVals dievals_from_ints8(Ints8 dievals);

DieVals dievals_from_intstar(const int *dievals, int count);

DieVal dievals_get(const DieVals self, int index);

Slots slots_empty();

Slots slots_init_va(int arg_count, ...);

Slot slots_get(Slots self, int index);

bool slots_has(Slots self, Slot slot);

int slots_count(Slots self);

Slots slots_removing(Slots self, Slot slot_to_remove);

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

f32 avg_ev(u16 start_dievals_data, Selection selection, Slots slots, u8 upper_total, u8 next_roll, bool yahtzee_bonus_available, usize threadid);

void* process_chunk(void* args);

void process_state(GameState state, u8 slots_len, bool joker_rules_in_play, int thread_id);

u64 powerset_of_size_n_count(int n);

void print_state_choice(GameState state, Choice choice, f32 ev, int threadid);
  