#ifndef _USF_INTERNAL_H_
#define _USF_INTERNAL_H_

#include "osal/preproc.h"

#include "main/main.h"
#include "main/rom.h"

#include "ai/ai_controller.h"
#include "memory/memory.h"
#include "pi/pi_controller.h"

#include "r4300/r4300_core.h"
#include "r4300/ops.h"
#include "r4300/cp0.h"

#include "rdp/rdp_core.h"
#include "ri/rdram.h"
#include "ri/ri_controller.h"
#include "rsp/rsp_core.h"
#include "si/si_controller.h"
#include "vi/vi_controller.h"

#include "rsp_hle/hle.h"

#include <stdio.h>

struct usf_state_helper
{
    size_t offset_to_structure;
};

#ifdef DEBUG_INFO
#include <stdio.h>
#endif

#ifndef INTERUPT_STRUCTS
#define INTERUPT_STRUCTS
#define POOL_CAPACITY 16

struct interrupt_event
{
    int type;
    unsigned int count;
};


struct node
{
    struct interrupt_event data;
    struct node *next;
};

struct pool
{
    struct node nodes[POOL_CAPACITY];
    struct node* stack[POOL_CAPACITY];
    size_t index;
};


struct interrupt_queue
{
    struct pool pool;
    struct node* first;
};
#endif

#ifndef TLB_STRUCTS
#define TLB_STRUCTS
typedef struct _tlb
{
    short mask;
    int vpn2;
    char g;
    unsigned char asid;
    int pfn_even;
    char c_even;
    char d_even;
    char v_even;
    int pfn_odd;
    char c_odd;
    char d_odd;
    char v_odd;
    char r;
    //int check_parity_mask;
    
    unsigned int start_even;
    unsigned int end_even;
    unsigned int phys_even;
    unsigned int start_odd;
    unsigned int end_odd;
    unsigned int phys_odd;
} tlb;
#endif

typedef struct _precomp_instr precomp_instr;
typedef struct _precomp_block precomp_block;

struct usf_state
{
    // r4300/r4300.c
    unsigned int r4300emu/* = 0*/;
    int llbit, rompause;
    int stop;
    int64_t reg[32], hi, lo;
    unsigned int next_interupt;
    precomp_instr *PC;
    unsigned int delay_slot, skip_jump/* = 0*/, last_addr;
    
    cpu_instruction_table current_instruction_table;
    
    // r4300/reset.c
    int reset_hard_job/* = 0*/;
    
    // r4300/cp0.c
    unsigned int g_cp0_regs[CP0_REGS_COUNT];
    
    // r4300/cp1.c
    float *reg_cop1_simple[32];
    double *reg_cop1_double[32];
    int FCR0, FCR31;
    int64_t reg_cop1_fgr_64[32];
    
    int rounding_mode/* = 0x33F*/;
    // These constants won't be written to, but they need to be located within the struct
    int trunc_mode/* = 0xF3F*/, round_mode/* = 0x33F*/,
        ceil_mode/* = 0xB3F*/, floor_mode/* = 0x73F*/;
    
    // r4300/interupt.c
    int interupt_unsafe_state/* = 0*/;
    int SPECIAL_done/* = 0*/;
    
    struct interrupt_queue q;
    
    // r4300/tlb.c
    tlb tlb_e[32];
    
    // r4300/instr_counters.c
#ifdef COUNT_INSTR
    unsigned int instr_count[132];
#endif
    struct io_bus io;

    struct rdram g_rdram;
    
    // main/rom.c
    unsigned char* g_rom/* = NULL*/;
    int g_rom_size/* = 0*/;

    m64p_rom_header   ROM_HEADER;
    rom_params        ROM_PARAMS;
    
    // r4300/cached_interp.c
    char invalid_code[0x100000];
    precomp_block *blocks[0x100000];
    precomp_block *actual;
    
    // r4300/recomp.c
    precomp_instr *dst; // destination structure for the recompiled instruction
    precomp_block *dst_block; // the current block that we are recompiling
    uint32_t src; // the current recompiled instruction
    int no_compiled_jump /* = 0*/; /* use cached interpreter instead of recompiler for jumps */
    
    // function for the latest decoded opcode
    
    const uint32_t* SRC; // currently recompiled instruction in the input stream
    int check_nop; // next instruction is nop ?

    // rsp_hle
    struct hle_t hle;

    // options from file tags
    uint32_t enablecompare, enableFIFOfull;
    
    // options for decoding
    uint32_t enable_hle_audio;

    // XXX enable this for some USF sets, safe to enable always
    int g_disable_tlb_write_exception;
    
    // save state
    unsigned char * save_state;
    unsigned int save_state_size;
    
    // buffering for rendered sample data
    size_t sample_buffer_count;
    int16_t * sample_buffer;

    // audio.c
    // SampleRate is usually guaranteed to stay the same for the duration
    // of a given track, and depends on the game.
    int32_t SampleRate;
    // Audio is rendered in whole Audio Interface DMA transfers, which are
    // then copied directly to the caller's buffer. Any left over samples
    // from the last DMA transfer that fills the caller's buffer will be
    // stored here until the next call to usf_render()
    int16_t samplebuf[16384];
    size_t samples_in_buffer;
    
    void * resampler;
    int16_t samplebuf2[8192];
    size_t samples_in_buffer_2;
    
    // This buffer does not really need to be that large, as it is likely
    // to only accumulate a handlful of error messages, at which point
    // emulation is immediately halted and the messages are returned to
    // the caller.
    const char * last_error;
    char error_message[16384];
    
    uint32_t MemoryState;
    
    // main/main.c
    struct ai_controller g_ai;
    struct pi_controller g_pi;
    struct ri_controller g_ri;
    struct si_controller g_si;
    struct vi_controller g_vi;
    struct r4300_core g_r4300;
    struct rdp_core g_dp;
    struct rsp_core g_sp;
    
    // Compatibility with USF sets dictates that these all remain zero
    int g_delay_si/* = 0*/;
    int g_delay_ai/* = 0*/;
    int g_delay_pi/* = 0*/;
    int g_delay_sp/* = 0*/;
    int g_delay_dp/* = 0*/;
    
    int g_gs_vi_counter/* = 0*/;
    
    // logging
#ifdef DEBUG_INFO
    FILE * debug_log;
#endif
};

void usf_set_audio_format(void *, unsigned int frequency, unsigned int bits);
void usf_push_audio_samples(void *, const void * buffer, size_t size);

#define USF_STATE_HELPER ((usf_state_helper_t *)(state))

#define USF_STATE ((usf_state_t *)(((uint8_t *)(state))+((usf_state_helper_t *)(state))->offset_to_structure))

#endif
