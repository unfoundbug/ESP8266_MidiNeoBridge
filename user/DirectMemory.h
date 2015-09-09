#include "os_type.h"
#define PIN_OUT (*(uint32_t *)0x60000300)
#define PIN_OUT_SET (*(uint32_t *)0x60000304)
#define PIN_OUT_CLEAR (*(uint32_t *)0x60000308)

#define PIN_DIR (*(uint32_t *)0x6000030C)
#define PIN_DIR_OUTPUT (*(uint32_t *)0x60000310)
#define PIN_DIR_INPUT (*(uint32_t *)0x60000314)
