#define WHITENOISELEN (0x280*3)

#define WO_LOWPASSES   0
#define WO_TRIANGLE_04 (WO_LOWPASSES+((0xfc+0xfc+0x80*0x1f+0x80+3*0x280)*31))
#define WO_TRIANGLE_08 (WO_TRIANGLE_04+0x04)
#define WO_TRIANGLE_10 (WO_TRIANGLE_08+0x08)
#define WO_TRIANGLE_20 (WO_TRIANGLE_10+0x10)
#define WO_TRIANGLE_40 (WO_TRIANGLE_20+0x20)
#define WO_TRIANGLE_80 (WO_TRIANGLE_40+0x40)
#define WO_SAWTOOTH_04 (WO_TRIANGLE_80+0x80)
#define WO_SAWTOOTH_08 (WO_SAWTOOTH_04+0x04)
#define WO_SAWTOOTH_10 (WO_SAWTOOTH_08+0x08)
#define WO_SAWTOOTH_20 (WO_SAWTOOTH_10+0x10)
#define WO_SAWTOOTH_40 (WO_SAWTOOTH_20+0x20)
#define WO_SAWTOOTH_80 (WO_SAWTOOTH_40+0x40)
#define WO_SQUARES     (WO_SAWTOOTH_80+0x80)
#define WO_WHITENOISE  (WO_SQUARES+(0x80*0x20))
#define WO_HIGHPASSES  (WO_WHITENOISE+WHITENOISELEN)
#define WAVES_SIZE     (WO_HIGHPASSES+((0xfc+0xfc+0x80*0x1f+0x80+3*0x280)*31))

const uint16 lentab[45];

const int16 vib_tab[64];

const uint16 period_tab[61];

const int32 stereopan_left[5];
const int32 stereopan_right[5];

const int16 filter_thing[2790];

int8 waves[WAVES_SIZE];
uint32 panning_left[256], panning_right[256];

void hvl_GenTables( void );
