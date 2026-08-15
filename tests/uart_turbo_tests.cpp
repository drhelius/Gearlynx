#include <cstdio>
#include <cstdlib>
#include "gearlynx.h"
#include "mikey.h"
#include "mikey_defines.h"

bool g_mcp_stdio_mode = false;

static int s_publish_count = 0;
static u64 s_publish_start = 0;
static u32 s_publish_bit_cycles = 0;
static u16 s_publish_bits = 0;
static int s_sync_count = 0;
static bool s_sync_valid = true;

static void PublishFrame(u64 start_cycle, u32 bit_cycles, u16 bits, void*)
{
    s_publish_count++;
    s_publish_start = start_cycle;
    s_publish_bit_cycles = bit_cycles;
    s_publish_bits = bits;
}

static bool SampleLine(u64, void*)
{
    return true;
}

static void SynchronizeTurbo(u64 cycle, void*)
{
    if ((cycle & (COMLYNX_TURBO_SYNC_CYCLES - 1)) != 0)
        s_sync_valid = false;
    s_sync_count++;
}

static void Check(bool condition, const char* message)
{
    if (!condition)
    {
        fprintf(stderr, "FAIL: %s\n", message);
        exit(1);
    }
}

int main()
{
    GearlynxCore core;
    core.Init();
    Mikey* mikey = core.GetMikey();

    mikey->Reset(true);
    mikey->Write<true>(MIKEY_TIM4CTLA, 0x08);
    mikey->Write<true>(MIKEY_TIM4BKUP, 0x00);
    Check(mikey->GetUartBitCycles() == 128, "backup zero uses 125000 baud");
    Check(mikey->GetComLynxSyncCycles() == 64,
        "125000 baud synchronizes every half bit");
    Check(mikey->GetComLynxPromiseCycles() == 128,
        "125000 baud promises one bit");

    mikey->Write<true>(MIKEY_TIM4BKUP, 0x01);
    Check(mikey->GetUartBitCycles() == 256, "backup one uses 62500 baud");
    Check(mikey->GetComLynxSyncCycles() == COMLYNX_MAX_SYNC_CYCLES,
        "62500 baud retains normal synchronization");
    Check(mikey->GetComLynxPromiseCycles() == COMLYNX_MAX_PROMISE_CYCLES,
        "62500 baud retains normal promise");

    mikey->Reset(true);
    mikey->Write<true>(MIKEY_TIM4CTLA, 0x00);
    mikey->Write<true>(MIKEY_TIM4BKUP, 0x67);
    mikey->Write<true>(MIKEY_TIM4CNT, 0x67);
    mikey->Write<true>(MIKEY_SERCTL, 0x0D);
    mikey->Write<true>(MIKEY_SERDAT, 0x2A);
    mikey->Clock(GLYNX_UART_TURBO_BIT_CYCLES * 16);
    Check(!mikey->GetState()->uart.rx_ready,
        "stopped Timer 4 does not advance normal UART");

    mikey->Reset(true);
    mikey->Write<true>(MIKEY_TIM4CTLA, 0x00);
    mikey->Write<true>(MIKEY_TIM4BKUP, 0x67);
    mikey->Write<true>(MIKEY_TIM4CNT, 0x67);
    mikey->Write<true>(MIKEY_SERCTL, 0x0D);
    mikey->Write<true>(MIKEY_MTEST0, GLYNX_MTEST0_UART_TURBO);
    core.SetComLynxCallbacks(PublishFrame, NULL, NULL, NULL, NULL);
    core.SetComLynxTurboCallbacks(SampleLine, SynchronizeTurbo, NULL);
    core.SetComLynxCableConnected(true);
    mikey->Write<true>(MIKEY_SERDAT, 0x2A);
    u64 publish_cycle = mikey->GetComLynxCycle();

    Check(s_publish_count == 1, "turbo waveform is published at SERDAT write");
    Check(s_publish_start > publish_cycle &&
        s_publish_start <= publish_cycle + GLYNX_UART_TURBO_BIT_CYCLES * 2,
        "turbo waveform starts within the next two clocks");
    Check(s_publish_bit_cycles == GLYNX_UART_TURBO_BIT_CYCLES,
        "published turbo waveform uses one microsecond bits");
    Check(((s_publish_bits >> 1) & 0xFF) == 0x2A,
        "published turbo waveform preserves data");

    int ticks = 0;
    while (!mikey->GetState()->uart.rx_ready && ticks < 16)
    {
        mikey->Clock(GLYNX_UART_TURBO_BIT_CYCLES);
        ticks++;
    }

    Check(mikey->GetState()->uart.rx_ready, "turbo UART receives self echo");
    Check(mikey->GetState()->uart.rx_data == 0x2A, "turbo UART preserves data");
    Check(ticks == 12, "turbo UART uses one microsecond bit clock");
    Check(s_sync_count > 0 && s_sync_valid,
        "linked turbo UART synchronizes at exact eight-cycle boundaries");

    printf("UART turbo tests passed\n");
    return 0;
}