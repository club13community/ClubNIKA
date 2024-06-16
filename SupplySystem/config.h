//
// Created by independent-variable on 6/16/2024.
//

#pragma once

/** Resolution of timer. */
#define TIMER_TICK_us	200U
/** For how long socket voltage should be present before switching to it from battery. */
#define SOCKET_DEBOUNCE_s	2U
/** Short over current pulses are allowed(to charge decoupling caps., etc) - this is max. duration of a pulse. */
#define OVER_CURRENT_PULSE_ms	5U
/** For how long after short over current pulse there should be no pulses not to trigger protection;
 * or for how long protection should keep 12V disables after it is triggered. */
#define NO_OVER_CURRENT_TIME_s	5U