/* PocketSNES clock test for the TI-Nspire CX II: which way of changing the
 * CPU clock works on this calculator and OS.
 *
 * The CPU clock is 12 MHz times the multiplier in bits 24-29 of the power
 * controller's register 0x90140030 (see frontend/platform_nspire.cpp). This
 * raises it by two steps (24 MHz, which every CX II tried so far takes) in
 * several ways, and after each one measures the clock it really runs at by
 * timing a loop of known length with the 32768 Hz timer. Then it puts the
 * clock back. The results are shown and written to
 * pocketsnes_clocktest.txt.tns next to this program.
 *
 * The ways, in this order:
 *   D  NoverII's: interrupts on (TCT_Local_Control_Interrupts(0)), write,
 *      then the Ndless SDK's msleep(1) of the time (December 2020), which
 *      idles the CPU until the second timer's interrupt, then interrupts
 *      back as they were. NoverII is known to work, at least on OS 5.2.
 *   A  write with interrupts off, then busy-wait 2 ms
 *   B  ... then idle the CPU 1 ms, interrupts still off (PocketSNES's way
 *      since 2026-10-05)
 *   C  ... then clear the power controller's interrupt flag (0x90140024,
 *      as Firebird has the OS do) and idle 1 ms again
 * The clock is put back with D's way after D, and after C. */
#include <os.h>
#include <libndls.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define REG(address) (*(volatile uint32_t *) (address))
#define CLOCK        REG(0x90140030)
#define PMU_FLAGS    REG(0x90140024)
#define MULTIPLIER(value) ((int) (((value) >> 24) & 0x3F))

/* The first timer measures (32768 Hz, free running), the second wakes the
 * CPU from idle (Ndless's msleep timer, interrupt 19). */
#define T1(offset) REG(0x900C0000 + (offset))
#define T2(offset) REG(0x900D0000 + (offset))
#define VIC(offset) REG(0xDC000000 + (offset))
#define TIMER_IRQ (1u << 19)

static char report[4096];
static size_t report_len;

static void say(const char *format, ...) __attribute__ ((format (printf, 1, 2)));
static void say(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    int n = vsnprintf(report + report_len, sizeof(report) - report_len, format, args);
    va_end(args);
    if (n > 0)
        report_len += (size_t) n < sizeof(report) - report_len ? (size_t) n : sizeof(report) - report_len - 1;
}

static uint32_t interrupts_off(void)
{
    uint32_t old, off;
    __asm__ volatile ("mrs %0, cpsr\n\torr %1, %0, #0xC0\n\tmsr cpsr_c, %1"
                      : "=r" (old), "=r" (off) : : "memory");
    return old;
}

static void interrupts_restore(uint32_t old)
{
    __asm__ volatile ("msr cpsr_c, %0" : : "r" (old) : "memory");
}

static uint32_t ticks(void)
{
    return ~T1(0x04);   /* counts down */
}

/* 16 million cycles (SUBS 1 + taken branch 3, from the instruction cache),
 * timed with interrupts off. */
static uint32_t measure_mhz(void)
{
    uint32_t loops = 4000000;
    uint32_t cpsr = interrupts_off();
    uint32_t start = ticks();
    __asm__ volatile ("1: subs %0, %0, #1\n\tbne 1b" : "+r" (loops) : : "cc");
    uint32_t elapsed = ticks() - start;
    interrupts_restore(cpsr);
    if (!elapsed)
        return 0;
    return (uint32_t) ((16000000ull * 32768 + elapsed * 500000ull) / elapsed / 1000000);
}

static void busy_wait(uint32_t timer_ticks)
{
    uint32_t start = ticks();
    while (ticks() - start < timer_ticks)
        ;
}

/* PocketSNES's idle: the CPU waits for the second timer's interrupt, the
 * only one let through, with interrupts off in the CPU. */
static void idle_interrupts_off(uint32_t timer_ticks)
{
    uint32_t cpsr = interrupts_off();
    uint32_t control = T2(0x08), load = T2(0x00), enabled = VIC(0x10);
    T2(0x08) = 0;
    T2(0x0C) = 1;
    T2(0x00) = timer_ticks;
    T2(0x08) = 0x63;
    T2(0x08) = 0xE3;
    VIC(0x14) = ~TIMER_IRQ;
    VIC(0x10) = TIMER_IRQ;
    while (!(T2(0x10) & 1))
        __asm__ volatile ("mcr p15, 0, %0, c7, c10, 4\n\tmcr p15, 0, %0, c7, c0, 4" : : "r" (0) : "memory");
    VIC(0x14) = 0xFFFFFFFF;
    VIC(0x10) = enabled;
    T2(0x08) = 0;
    T2(0x0C) = 1;
    T2(0x08) = control & 0x7F;
    T2(0x00) = load;
    T2(0x08) = control;
    interrupts_restore(cpsr);
}

/* The Ndless SDK's idle() and msleep() as NoverII has them (from its
 * binary): the timer counts down once, the CPU idles until an interrupt,
 * and with interrupts on the OS handles it. */
static void old_idle(void)
{
    uint32_t enabled = VIC(0x10);
    VIC(0x14) = ~TIMER_IRQ;
    __asm__ volatile ("mcr p15, 0, %0, c7, c0, 4" : : "r" (0) : "memory");
    T2(0x0C) = 1;
    VIC(0x14) = 0xFFFFFFFF;
    VIC(0x10) = enabled;
}

static void old_msleep(unsigned ms)
{
    uint32_t control = T2(0x08), load = T2(0x00);
    T2(0x08) = 0;
    T2(0x08) = 0x63;
    T2(0x08) = 0xE3;
    T2(0x00) = ms * 32;
    for (int i = 0; i < 100000 && T2(0x04) != 0; i++)
        old_idle();
    T2(0x08) = 0;
    T2(0x08) = control & 0x7F;
    T2(0x00) = load;
    T2(0x08) = control;
}

static void noverii_write(uint32_t value)
{
    int mask = TCT_Local_Control_Interrupts(0);
    CLOCK = value;
    old_msleep(1);
    TCT_Local_Control_Interrupts(mask);
}

static void state(void)
{
    say(" [clock %08lx flags %08lx 20:%08lx 810:%08lx; irq raw %08lx enabled %08lx]\n",
        (unsigned long) CLOCK, (unsigned long) PMU_FLAGS, (unsigned long) REG(0x90140020),
        (unsigned long) REG(0x90140810), (unsigned long) VIC(0x08), (unsigned long) VIC(0x10));
}

int main(int argc, char **argv)
{
    (void) argc;
    if (!is_cx2)
    {
        show_msgbox("Clock test", "This test is for the TI-Nspire CX II.");
        return 0;
    }

    uint32_t saved_load = T1(0x00), saved_control = T1(0x08), saved_clock = T1(0x80);
    T1(0x08) = 0;
    T1(0x80) = 0xA;            /* 32768 Hz */
    T1(0x00) = 0xFFFFFFFF;
    T1(0x08) = 0x82;           /* on, free running, 32-bit, no interrupt */

    uint32_t original = CLOCK;
    int m = MULTIPLIER(original);
    uint32_t cpsr;
    __asm__ volatile ("mrs %0, cpsr" : "=r" (cpsr));
    say("PocketSNES clock test. Clock register %08lx: x%d = %d MHz. cpsr %08lx\n",
        (unsigned long) original, m, m * 12, (unsigned long) cpsr);
    say("Start: %lu MHz", (unsigned long) measure_mhz());
    state();

    if (m < 20 || m > 36)
    {
        say("Multiplier outside 20..36, nothing tried.\n");
    }
    else
    {
        uint32_t target = ((original & ~(0x3Fu << 24)) | (uint32_t) (m + 2) << 24 | 1) & ~(1u << 4);
        say("Target %08lx: x%d = %d MHz\n", (unsigned long) target, m + 2, (m + 2) * 12);

        noverii_write(target);
        say("D NoverII's way: %lu MHz", (unsigned long) measure_mhz());
        state();
        noverii_write(original);
        say("  back: %lu MHz", (unsigned long) measure_mhz());
        state();

        uint32_t irq = interrupts_off();
        CLOCK = target;
        busy_wait(66);
        interrupts_restore(irq);
        say("A write, busy 2 ms: %lu MHz", (unsigned long) measure_mhz());
        state();
        idle_interrupts_off(33);
        say("B + idle 1 ms: %lu MHz", (unsigned long) measure_mhz());
        state();
        uint32_t flags = PMU_FLAGS;
        if (flags)
            PMU_FLAGS = flags;
        idle_interrupts_off(33);
        say("C + clear flags %08lx, idle 1 ms: %lu MHz", (unsigned long) flags, (unsigned long) measure_mhz());
        state();
        noverii_write(original);
        say("  back: %lu MHz", (unsigned long) measure_mhz());
        state();
    }

    T1(0x08) = 0;
    T1(0x80) = saved_clock;
    T1(0x00) = saved_load;
    T1(0x08) = saved_control;

    char path[300];
    snprintf(path, sizeof(path), "%s", argv[0]);
    char *slash = strrchr(path, '/');
    snprintf(slash ? slash + 1 : path, sizeof(path) - (slash ? (size_t) (slash + 1 - path) : 0),
             "pocketsnes_clocktest.txt.tns");
    FILE *f = fopen(path, "w");
    if (f)
    {
        fputs(report, f);
        fclose(f);
    }

    /* The message box: the lines without the register details. */
    char summary[1024];
    size_t len = 0;
    for (const char *p = report; *p && len < sizeof(summary) - 1; p++)
    {
        if (*p == '[')
            while (*p && *p != ']')
                p++;
        else if (*p != ']')
            summary[len++] = *p;
    }
    summary[len] = 0;
    show_msgbox("Clock test (also in pocketsnes_clocktest.txt)", summary);
    return 0;
}
