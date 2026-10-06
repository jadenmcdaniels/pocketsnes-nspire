/* The overclock test: the highest CPU speed this calculator runs without
 * errors. */
#ifndef OVERCLOCK_H
#define OVERCLOCK_H

/* Tries 408, 420, ... 504 MHz in turn. Each speed gets two rounds of memory
 * and CPU checks, each after its own clock switch. The test stops at the
 * first speed with an error, a switch that didn't get there, or esc, and the
 * highest speed that passed becomes the "highest tested" CPU speed
 * (config_tested_mhz). A marker file names the speed being tried, so a
 * freeze is noticed at the next start (overclock_check_freeze). The results
 * are shown and written to pocketsnes_overclock.txt.tns. */
void overclock_test_run(void);

/* At startup: if the test froze the calculator, keeps the highest speed that
 * passed before the freeze and says so. Returns 1 if it did. */
int overclock_check_freeze(void);

#endif
