/**
 * stepper.cpp - stepper motor driver: executes motion plans using stepper motors
 * Marlin Firmware
 *
 * Derived from Grbl
 * Copyright (c) 2009-2011 Simen Svale Skogsrud
 *
 * Grbl is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Grbl is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Grbl.  If not, see <http://www.gnu.org/licenses/>.
 */

/* The timer calculations of this module informed by the 'RepRap cartesian firmware' by Zack Smith
   and Philipp Tiefenbacher. */

#include "base.h"
#include "Marlin_main.h"

#if ENABLED(AUTO_BED_LEVELING_FEATURE)
  #include "vector_3.h"
#endif
#include "planner.h"
#include "stepper_indirection.h"
#if MB(ALLIGATOR)
  #include "external_dac.h"
#endif
#include "stepper.h"
#include "temperature.h"
#include "ultralcd.h"
#include "nextion_lcd.h"
#include "language.h"
#if ENABLED(SDSUPPORT)
  #include "cardreader.h"
#endif
#if HAS(DIGIPOTSS)
  #include <SPI.h>
#endif

//===========================================================================
//============================= public variables ============================
//===========================================================================
block_t* current_block;  // A pointer to the block currently being traced


//===========================================================================
//============================= private variables ===========================
//===========================================================================
//static makes it impossible to be called from outside of this file by extern.!

// Variables used by The Stepper Driver Interrupt
static unsigned char out_bits = 0;        // The next stepping-bits to be output
static unsigned int cleaning_buffer_counter;

#if ENABLED(Z_DUAL_ENDSTOPS)
  static bool performing_homing = false,
              locked_z_motor = false,
              locked_z2_motor = false;
#endif

// Counter variables for the Bresenham line tracer
static long counter_x, counter_y, counter_z, counter_e;
volatile static unsigned long step_events_completed; // The number of step events executed in the current block

#if ENABLED(ADVANCE)
  static long advance_rate, advance, final_advance = 0;
  static long old_advance = 0;
  static long e_steps[4];
#endif

static long acceleration_time, deceleration_time;
//static unsigned long accelerate_until, decelerate_after, acceleration_rate, initial_rate, final_rate, nominal_rate;
static unsigned long acc_step_rate; // needed for deceleration start point
static char step_loops;
static unsigned long OCR1A_nominal;
static unsigned short step_loops_nominal;

volatile long endstops_trigsteps[3] = { 0 };
volatile long endstops_stepsTotal, endstops_stepsDone;
static volatile char endstop_hit_bits = 0; // use X_MIN, Y_MIN, Z_MIN and Z_PROBE as BIT value

#if ENABLED(Z_DUAL_ENDSTOPS) || ENABLED(NPR2)
  static uint16_t
#else
  static byte
#endif
  old_endstop_bits = 0; // use X_MIN, X_MAX... Z_MAX, Z_PROBE, Z2_MIN, Z2_MAX, E_MIN

#if ENABLED(ABORT_ON_ENDSTOP_HIT_FEATURE_ENABLED)
  bool abort_on_endstop_hit = false;
#endif

#if PIN_EXISTS(MOTOR_CURRENT_PWM_XY)
  int motor_current_setting[3] = DEFAULT_PWM_MOTOR_CURRENT;
#endif

static bool check_endstops = true;

volatile long count_position[NUM_AXIS] = { 0 };
volatile signed char count_direction[NUM_AXIS] = { 1, 1, 1, 1 };


//===========================================================================
//================================ functions ================================
//===========================================================================

#if ENABLED(DUAL_X_CARRIAGE)
  #define X_APPLY_DIR(v,ALWAYS) \
    if (extruder_duplication_enabled || ALWAYS) { \
      X_DIR_WRITE(v); \
      X2_DIR_WRITE(v); \
    } \
    else { \
      if (current_block->active_driver) X2_DIR_WRITE(v); else X_DIR_WRITE(v); \
    }
  #define X_APPLY_STEP(v,ALWAYS) \
    if (extruder_duplication_enabled || ALWAYS) { \
      X_STEP_WRITE(v); \
      X2_STEP_WRITE(v); \
    } \
    else { \
      if (current_block->active_driver != 0) X2_STEP_WRITE(v); else X_STEP_WRITE(v); \
    }
#else
  #define X_APPLY_DIR(v,Q) X_DIR_WRITE(v)
  #define X_APPLY_STEP(v,Q) X_STEP_WRITE(v)
#endif

#if ENABLED(Y_DUAL_STEPPER_DRIVERS)
  #define Y_APPLY_DIR(v,Q) { Y_DIR_WRITE(v); Y2_DIR_WRITE((v) != INVERT_Y2_VS_Y_DIR); }
  #define Y_APPLY_STEP(v,Q) { Y_STEP_WRITE(v); Y2_STEP_WRITE(v); }
#else
  #define Y_APPLY_DIR(v,Q) Y_DIR_WRITE(v)
  #define Y_APPLY_STEP(v,Q) Y_STEP_WRITE(v)
#endif

#if ENABLED(Z_DUAL_STEPPER_DRIVERS)
  #define Z_APPLY_DIR(v,Q) { Z_DIR_WRITE(v); Z2_DIR_WRITE(v); }
  #if ENABLED(Z_DUAL_ENDSTOPS)
    #define Z_APPLY_STEP(v,Q) \
    if (performing_homing) { \
      if (Z_HOME_DIR > 0) {\
        if (!(TEST(old_endstop_bits, Z_MAX) && (count_direction[Z_AXIS] > 0)) && !locked_z_motor) Z_STEP_WRITE(v); \
        if (!(TEST(old_endstop_bits, Z2_MAX) && (count_direction[Z_AXIS] > 0)) && !locked_z2_motor) Z2_STEP_WRITE(v); \
      } \
      else { \
        if (!(TEST(old_endstop_bits, Z_MIN) && (count_direction[Z_AXIS] < 0)) && !locked_z_motor) Z_STEP_WRITE(v); \
        if (!(TEST(old_endstop_bits, Z2_MIN) && (count_direction[Z_AXIS] < 0)) && !locked_z2_motor) Z2_STEP_WRITE(v); \
      } \
    } \
    else { \
      Z_STEP_WRITE(v); \
      Z2_STEP_WRITE(v); \
    }
  #else
    #define Z_APPLY_STEP(v,Q) { Z_STEP_WRITE(v); Z2_STEP_WRITE(v); }
  #endif
#else
  #define Z_APPLY_DIR(v,Q) Z_DIR_WRITE(v)
  #define Z_APPLY_STEP(v,Q) Z_STEP_WRITE(v)
#endif

#define E_APPLY_STEP(v,Q) E_STEP_WRITE(v)

// intRes = intIn1 * intIn2 >> 16
#define MultiU16X8toH16(intRes, charIn1, intIn2)   intRes = ((charIn1) * (intIn2)) >> 16

// intRes = longIn1 * longIn2 >> 24
#define MultiU32X32toH32(intRes, longIn1, longIn2) intRes = ((uint64_t)longIn1 * longIn2 + 0x80000000) >> 32

void endstops_hit_on_purpose() {
  endstop_hit_bits = 0;
}

void checkHitEndstops() {
  if (endstop_hit_bits) {
    ECHO_SM(DB, MSG_ENDSTOPS_HIT);
    if (endstop_hit_bits & BIT(X_MIN)) {
      ECHO_MV(MSG_ENDSTOP_X, (float)endstops_trigsteps[X_AXIS] / axis_steps_per_unit[X_AXIS]);
      LCD_MESSAGEPGM(MSG_ENDSTOPS_HIT MSG_ENDSTOP_XS);
    }
    if (endstop_hit_bits & BIT(Y_MIN)) {
      ECHO_MV(MSG_ENDSTOP_Y, (float)endstops_trigsteps[Y_AXIS] / axis_steps_per_unit[Y_AXIS]);
      LCD_MESSAGEPGM(MSG_ENDSTOPS_HIT MSG_ENDSTOP_YS);
    }
    if (endstop_hit_bits & BIT(Z_MIN)) {
      ECHO_MV(MSG_ENDSTOP_Z, (float)endstops_trigsteps[Z_AXIS] / axis_steps_per_unit[Z_AXIS]);
      LCD_MESSAGEPGM(MSG_ENDSTOPS_HIT MSG_ENDSTOP_ZS);
    }
    #if ENABLED(Z_PROBE_ENDSTOP)
      if (endstop_hit_bits & BIT(Z_PROBE)) {
        ECHO_MV(MSG_ENDSTOP_ZPS, (float)endstops_trigsteps[Z_AXIS] / axis_steps_per_unit[Z_AXIS]);
        LCD_MESSAGEPGM(MSG_ENDSTOPS_HIT MSG_ENDSTOP_ZPS);
      }
    #endif
    #if ENABLED(NPR2)
      if (endstop_hit_bits & BIT(E_MIN)) {
        ECHO_MV(MSG_ENDSTOP_E, (float)endstops_trigsteps[E_AXIS] / axis_steps_per_unit[E_AXIS]);
        LCD_MESSAGEPGM(MSG_ENDSTOPS_HIT MSG_ENDSTOP_ES);
      }
    #endif
    ECHO_E;

    endstops_hit_on_purpose();

    #if ENABLED(ABORT_ON_ENDSTOP_HIT_FEATURE_ENABLED) && ENABLED(SDSUPPORT)
      if (abort_on_endstop_hit) {
        card.sdprinting = false;
        card.closeFile();
        quickStop();
        disable_all_heaters(); // switch off all heaters.
      }
    #endif
  }
}

void enable_endstops(bool check) {
  if (debugLevel & DEBUG_INFO) {
    ECHO_SM(DB, "setup_for_endstop_move > enable_endstops");
    if (check) ECHO_EM("(true)");
    else ECHO_EM("(false)");
  }
  check_endstops = check;
}

// Check endstops
inline void update_endstops() {

  #if ENABLED(Z_DUAL_ENDSTOPS) || ENABLED(NPR2)
    uint16_t
  #else
    byte
  #endif
      current_endstop_bits = 0;

  #define _ENDSTOP_PIN(AXIS, MINMAX) AXIS ##_## MINMAX ##_PIN
  #define _ENDSTOP_INVERTING(AXIS, MINMAX) AXIS ##_## MINMAX ##_ENDSTOP_INVERTING
  #define _AXIS(AXIS) AXIS ##_AXIS
  #define _ENDSTOP_HIT(AXIS) endstop_hit_bits |= BIT(_ENDSTOP(AXIS, MIN))
  #define _ENDSTOP(AXIS, MINMAX) AXIS ##_## MINMAX

  // SET_ENDSTOP_BIT: set the current endstop bits for an endstop to its status
  #define SET_ENDSTOP_BIT(AXIS, MINMAX) SET_BIT(current_endstop_bits, _ENDSTOP(AXIS, MINMAX), (READ(_ENDSTOP_PIN(AXIS, MINMAX)) != _ENDSTOP_INVERTING(AXIS, MINMAX)))
  // COPY_BIT: copy the value of COPY_BIT to BIT in bits
  #define COPY_BIT(bits, COPY_BIT, BIT) SET_BIT(bits, BIT, TEST(bits, COPY_BIT))
  // TEST_ENDSTOP: test the old and the current status of an endstop
  #define TEST_ENDSTOP(ENDSTOP) (TEST(current_endstop_bits, ENDSTOP) && TEST(old_endstop_bits, ENDSTOP))

  #define UPDATE_ENDSTOP(AXIS,MINMAX) \
    SET_ENDSTOP_BIT(AXIS, MINMAX); \
    if (TEST_ENDSTOP(_ENDSTOP(AXIS, MINMAX))  && (current_block->steps[_AXIS(AXIS)] > 0)) { \
      endstops_trigsteps[_AXIS(AXIS)] = count_position[_AXIS(AXIS)]; \
      _ENDSTOP_HIT(AXIS); \
      step_events_completed = current_block->step_event_count; \
    }

  #if MECH(COREXY)
    // Head direction in -X axis for CoreXY bots.
    // If DeltaX == -DeltaY, the movement is only in Y axis
    if ((current_block->steps[A_AXIS] != current_block->steps[B_AXIS]) || (TEST(out_bits, A_AXIS) == TEST(out_bits, B_AXIS))) {
      if (TEST(out_bits, X_HEAD))
  #elif MECH(COREXZ)
    // Head direction in -X axis for CoreXZ bots.
    // If DeltaX == -DeltaZ, the movement is only in Z axis
    if ((current_block->steps[A_AXIS] != current_block->steps[C_AXIS]) || (TEST(out_bits, A_AXIS) == TEST(out_bits, C_AXIS))) {
      if (TEST(out_bits, X_HEAD))
  #else
    if (TEST(out_bits, X_AXIS))   // stepping along -X axis (regular Cartesian bot)
  #endif
      { // -direction
        #if ENABLED(DUAL_X_CARRIAGE)
          // with 2 x-carriages, endstops are only checked in the homing direction for the active extruder
          if ((current_block->active_extruder == 0 && X_HOME_DIR == -1) || (current_block->active_extruder != 0 && X2_HOME_DIR == -1))
        #endif
          {
            #if HAS(X_MIN)
              UPDATE_ENDSTOP(X, MIN);
            #endif
          }
      }
      else { // +direction
        #if ENABLED(DUAL_X_CARRIAGE)
          // with 2 x-carriages, endstops are only checked in the homing direction for the active extruder
          if ((current_block->active_extruder == 0 && X_HOME_DIR == 1) || (current_block->active_extruder != 0 && X2_HOME_DIR == 1))
        #endif
          {
            #if HAS(X_MAX)
              UPDATE_ENDSTOP(X, MAX);
            #endif
          }
      }
  #if MECH(COREXY) || MECH(COREXZ)
    }
  #endif

  #if MECH(COREXY)
    // Head direction in -Y axis for CoreXY bots.
    // If DeltaX == DeltaY, the movement is only in X axis
    if ((current_block->steps[A_AXIS] != current_block->steps[B_AXIS]) || (TEST(out_bits, A_AXIS) != TEST(out_bits, B_AXIS))) {
      if (TEST(out_bits, Y_HEAD))
  #else
      if (TEST(out_bits, Y_AXIS))   // -direction
  #endif
      { // -direction
        #if HAS(Y_MIN)
          UPDATE_ENDSTOP(Y, MIN);
        #endif
      }
      else { // +direction
        #if HAS(Y_MAX)
          UPDATE_ENDSTOP(Y, MAX);
        #endif
      }
  #if MECH(COREXY)
    }
  #endif

  #if MECH(COREXZ)
    // Head direction in -Z axis for CoreXZ bots.
    // If DeltaX == DeltaZ, the movement is only in X axis
    if ((current_block->steps[A_AXIS] != current_block->steps[C_AXIS]) || (TEST(out_bits, A_AXIS) != TEST(out_bits, C_AXIS))) {
      if (TEST(out_bits, Z_HEAD))
  #else
      if (TEST(out_bits, Z_AXIS))
  #endif
      { // z -direction
        #if HAS(Z_MIN)

          #if ENABLED(Z_DUAL_ENDSTOPS)
            SET_ENDSTOP_BIT(Z, MIN);
            #if HAS(Z2_MIN)
              SET_ENDSTOP_BIT(Z2, MIN);
            #else
              COPY_BIT(current_endstop_bits, Z_MIN, Z2_MIN);
            #endif

            byte z_test = TEST_ENDSTOP(Z_MIN) | (TEST_ENDSTOP(Z2_MIN) << 1); // bit 0 for Z, bit 1 for Z2

            if (z_test && current_block->steps[Z_AXIS] > 0) { // z_test = Z_MIN || Z2_MIN
              endstops_trigsteps[Z_AXIS] = count_position[Z_AXIS];
              endstop_hit_bits |= BIT(Z_MIN);
              if (!performing_homing || (z_test == 0x3))  //if not performing home or if both endstops were trigged during homing...
                step_events_completed = current_block->step_event_count;
            }
          #else // !Z_DUAL_ENDSTOPS

            UPDATE_ENDSTOP(Z, MIN);

          #endif // !Z_DUAL_ENDSTOPS
        #endif // Z_MIN_PIN

        #if ENABLED(Z_PROBE_ENDSTOP)
          UPDATE_ENDSTOP(Z, PROBE);

          if (TEST_ENDSTOP(Z_PROBE)) {
            endstops_trigsteps[Z_AXIS] = count_position[Z_AXIS];
            endstop_hit_bits |= BIT(Z_PROBE);
          }
        #endif
      }
      else { // z +direction
        #if HAS(Z_MAX)

          #if ENABLED(Z_DUAL_ENDSTOPS)

            SET_ENDSTOP_BIT(Z, MAX);
            #if HAS(Z2_MAX)
              SET_ENDSTOP_BIT(Z2, MAX);
            #else
              COPY_BIT(current_endstop_bits, Z_MAX, Z2_MAX);
            #endif

            byte z_test = TEST_ENDSTOP(Z_MAX) | (TEST_ENDSTOP(Z2_MAX) << 1); // bit 0 for Z, bit 1 for Z2

            if (z_test && current_block->steps[Z_AXIS] > 0) {  // t_test = Z_MAX || Z2_MAX
              endstops_trigsteps[Z_AXIS] = count_position[Z_AXIS];
              endstop_hit_bits |= BIT(Z_MIN);
              if (!performing_homing || (z_test == 0x3))  //if not performing home or if both endstops were trigged during homing...
                step_events_completed = current_block->step_event_count;
            }

          #else // !Z_DUAL_ENDSTOPS

            UPDATE_ENDSTOP(Z, MAX);

          #endif // !Z_DUAL_ENDSTOPS
        #endif // Z_MAX_PIN
      }
  #if MECH(COREXZ)
    }
  #endif
  #if ENABLED(NPR2)
    UPDATE_ENDSTOP(E, MIN);
  #endif
  old_endstop_bits = current_endstop_bits;
}

//         __________________________
//        /|                        |\     _________________         ^
//       / |                        | \   /|               |\        |
//      /  |                        |  \ / |               | \       s
//     /   |                        |   |  |               |  \      p
//    /    |                        |   |  |               |   \     e
//   +-----+------------------------+---+--+---------------+----+    e
//   |               BLOCK 1            |      BLOCK 2          |    d
//
//                           time ----->
//
//  The trapezoid is the shape the speed curve over time. It starts at block->initial_rate, accelerates
//  first block->accelerate_until step_events_completed, then keeps going at constant speed until
//  step_events_completed reaches block->decelerate_after after which it decelerates until the trapezoid generator is reset.
//  The slope of acceleration is calculated using v = u + at where t is the accumulated timer values of the steps so far.

void st_wake_up() {
  //  TCNT1 = 0;
  ENABLE_STEPPER_DRIVER_INTERRUPT();
}

FORCE_INLINE unsigned long calc_timer(unsigned long step_rate) {
  unsigned long timer;
  if (step_rate > MAX_STEP_FREQUENCY) step_rate = MAX_STEP_FREQUENCY;

  #if ENABLED(ENABLE_HIGH_SPEED_STEPPING)
    if(step_rate > (2 * DOUBLE_STEP_FREQUENCY)) { // If steprate > 2*DOUBLE_STEP_FREQUENCY >> step 4 times
      step_rate = (step_rate >> 2);
      step_loops = 4;
    }
    else if(step_rate > DOUBLE_STEP_FREQUENCY) { // If steprate > DOUBLE_STEP_FREQUENCY >> step 2 times
      step_rate = (step_rate >> 1);
      step_loops = 2;
    }
    else
  #endif
  {
    step_loops = 1;
  }

  timer = HAL_TIMER_RATE / step_rate;

  return timer;
}

/**
 * Set the stepper direction of each axis
 *
 *   X_AXIS=A_AXIS and Y_AXIS=B_AXIS for COREXY
 *   X_AXIS=A_AXIS and Z_AXIS=C_AXIS for COREXZ
 */
void set_stepper_direction(bool onlye) {

  if (!onlye) {
    if (TEST(out_bits, X_AXIS)) { // A_AXIS
      X_APPLY_DIR(INVERT_X_DIR, 0);
      count_direction[X_AXIS] = -1;
    }
    else {
      X_APPLY_DIR(!INVERT_X_DIR, 0);
      count_direction[X_AXIS] = 1;
    }

    if (TEST(out_bits, Y_AXIS)) { // B_AXIS
      Y_APPLY_DIR(INVERT_Y_DIR, 0);
      count_direction[Y_AXIS] = -1;
    }
    else {
      Y_APPLY_DIR(!INVERT_Y_DIR, 0);
      count_direction[Y_AXIS] = 1;
    }

    if (TEST(out_bits, Z_AXIS)) { // C_AXIS
      Z_APPLY_DIR(INVERT_Z_DIR, 0);
      count_direction[Z_AXIS] = -1;
    }
    else {
      Z_APPLY_DIR(!INVERT_Z_DIR, 0);
      count_direction[Z_AXIS] = 1;
    }
  }

  #if DISABLED(ADVANCE) && ENABLED(DONDOLO)
    if (TEST(out_bits, E_AXIS)) {
      switch(active_extruder) {
        case 0:
          REV_E_DIR(); break;
        case 1:
          NORM_E_DIR(); break;
      }
      count_direction[E_AXIS] = -1;
    }
    else {
      switch(active_extruder) {
        case 0:
          NORM_E_DIR(); break;
        case 1:
          REV_E_DIR(); break;
      }
      count_direction[E_AXIS] = 1;
    }
  #elif DISABLED(ADVANCE)
    if (TEST(out_bits, E_AXIS)) {
      REV_E_DIR();
      count_direction[E_AXIS] = -1;
    }
    else {
      NORM_E_DIR();
      count_direction[E_AXIS] = 1;
    }
  #endif // !ADVANCE
}

// Initializes the trapezoid generator from the current block. Called whenever a new
// block begins.

TcChannel *stepperChannel = (STEP_TIMER_COUNTER->TC_CHANNEL + STEP_TIMER_CHANNEL);

FORCE_INLINE
void HAL_timer_stepper_count(uint32_t count) {

  uint32_t counter_value = stepperChannel->TC_CV + 42;  // we need time for other stuff!
  //if(count < 105) count = 105;
  stepperChannel->TC_RC = (counter_value <= count) ? count : counter_value;
}

FORCE_INLINE void trapezoid_generator_reset() {

  if (current_block->direction_bits != out_bits) {
    out_bits = current_block->direction_bits;
    set_stepper_direction();
  }

  #if ENABLED(ADVANCE)
    advance = current_block->initial_advance;
    final_advance = current_block->final_advance;
    // Do E steps + advance steps
    e_steps[current_block->active_driver] += ((advance >>8) - old_advance);
    old_advance = advance >>8;
  #endif
  deceleration_time = 0;
  // step_rate to timer interval
  OCR1A_nominal = calc_timer(current_block->nominal_rate);
  // make a note of the number of step loops required at nominal speed
  step_loops_nominal = step_loops;
  acc_step_rate = current_block->initial_rate;
  acceleration_time = calc_timer(acc_step_rate);
  //HAL_timer_stepper_count(acceleration_time);
}

// "The Stepper Driver Interrupt" - This timer interrupt is the workhorse.
// It pops blocks from the block_buffer and executes them by pulsing the stepper pins appropriately.

HAL_STEP_TIMER_ISR {

  //STEP_TIMER_COUNTER->TC_CHANNEL[STEP_TIMER_CHANNEL].TC_SR;
  stepperChannel->TC_SR;
  //stepperChannel->TC_RC = 1000000;

  if (cleaning_buffer_counter) {
    current_block = NULL;
    plan_discard_current_block();
    #if ENABLED(SD_FINISHED_RELEASECOMMAND)
      if ((cleaning_buffer_counter == 1) && (SD_FINISHED_STEPPERRELEASE)) enqueuecommands_P(PSTR(SD_FINISHED_RELEASECOMMAND));
    #endif
    cleaning_buffer_counter--;
    HAL_timer_stepper_count(HAL_TIMER_RATE / 200); //5ms wait
    return;
  }

  // If there is no current block, attempt to pop one from the buffer
  if (!current_block) {
    // Anything in the buffer?
    current_block = plan_get_current_block();
    if (current_block) {
      current_block->busy = true;
      trapezoid_generator_reset();
      counter_x = -(current_block->step_event_count >> 1);
      counter_y = counter_z = counter_e = counter_x;
      step_events_completed = 0;

      #if ENABLED(Z_LATE_ENABLE)
        if (current_block->steps[Z_AXIS] > 0) {
          enable_z();
          HAL_timer_set_count (STEP_TIMER_COUNTER, STEP_TIMER_CHANNEL, HAL_TIMER_RATE / 1000); //1ms wait
          return;
        }
      #endif

      // #if ENABLED(ADVANCE)
      //   e_steps[current_block->active_driver] = 0;
      // #endif
    }
    else {
      HAL_timer_stepper_count(HAL_TIMER_RATE / 1000); // 1kHz
    }
  }

  if (current_block != NULL) {

    // Update endstops state, if enabled
    if (check_endstops) update_endstops();

    #define _COUNTER(axis) counter_## axis
    #define _APPLY_STEP(AXIS) AXIS ##_APPLY_STEP
  	#define _INVERT_STEP_PIN(AXIS) INVERT_## AXIS ##_STEP_PIN

    #define STEP_START(axis, AXIS) \
      _COUNTER(axis) += current_block->steps[_AXIS(AXIS)]; \
      if (_COUNTER(axis) > 0) { \
      _APPLY_STEP(AXIS)(!_INVERT_STEP_PIN(AXIS),0); \
      _COUNTER(axis) -= current_block->step_event_count; \
      count_position[_AXIS(AXIS)] += count_direction[_AXIS(AXIS)]; }

    #define STEP_END(axis, AXIS) _APPLY_STEP(AXIS)(_INVERT_STEP_PIN(AXIS),0)

    #if ENABLED(ENABLE_HIGH_SPEED_STEPPING)
      // Take multiple steps per interrupt (For high speed moves)
      for (int8_t i = 0; i < step_loops; i++) {

        #if ENABLED(ADVANCE)
          counter_e += current_block->steps[E_AXIS];
          if (counter_e > 0) {
            counter_e -= current_block->step_event_count;
            e_steps[current_block->active_driver] += TEST(out_bits, E_AXIS) ? -1 : 1;
          }
        #endif //ADVANCE

        STEP_START(x,X);
        STEP_START(y,Y);
        STEP_START(z,Z);
        #ifndef ADVANCE
          STEP_START(e,E);
        #endif

        STEP_END(x, X);
        STEP_END(y, Y);
        STEP_END(z, Z);
        #if DISABLED(ADVANCE)
          STEP_END(e, E);
        #endif

        step_events_completed++;
        if (step_events_completed >= current_block->step_event_count) break;
      }
    #else
      STEP_START(x,X);
      STEP_START(y,Y);
      STEP_START(z,Z);
      #if DISABLED(ADVANCE)
        STEP_START(e,E);
      #endif
      step_events_completed++;
    #endif

    // Calculate new timer value
    unsigned long timer;
    unsigned long step_rate;
    if (step_events_completed <= (unsigned long)current_block->accelerate_until) {

      MultiU32X32toH32(acc_step_rate, acceleration_time, current_block->acceleration_rate);
      acc_step_rate += current_block->initial_rate;

      // upper limit
      if (acc_step_rate > current_block->nominal_rate)
        acc_step_rate = current_block->nominal_rate;

      // step_rate to timer interval
      timer = calc_timer(acc_step_rate);
      acceleration_time += timer;
      #if ENABLED(ADVANCE)
        for (int8_t i = 0; i < step_loops; i++) {
          advance += advance_rate;
        }
        // if (advance > current_block->advance) advance = current_block->advance;
        // Do E steps + advance steps
        e_steps[current_block->active_driver] += ((advance >> 8) - old_advance);
        old_advance = advance >> 8;

      #endif // ADVANCE
    }
    else if (step_events_completed > (unsigned long)current_block->decelerate_after) {
      MultiU32X32toH32(step_rate, deceleration_time, current_block->acceleration_rate);

      if (step_rate > acc_step_rate) { // Check step_rate stays positive
        step_rate = current_block->final_rate;
      }
      else {
        step_rate = acc_step_rate - step_rate; // Decelerate from acceleration end point.
      }

      // lower limit
      if (step_rate < current_block->final_rate)
        step_rate = current_block->final_rate;

      // step_rate to timer interval
      timer = calc_timer(step_rate);
      deceleration_time += timer;
      #if ENABLED(ADVANCE)
        for (int8_t i = 0; i < step_loops; i++) {
          advance -= advance_rate;
        }
        if (advance < final_advance) advance = final_advance;
        // Do E steps + advance steps
        e_steps[current_block->active_driver] += ((advance >> 8) - old_advance);
        old_advance = advance >> 8;
      #endif //ADVANCE
    }
    else {
      timer = OCR1A_nominal;
      // ensure we're running at the correct step rate, even if we just came off an acceleration
      step_loops = step_loops_nominal;
    }
    #if DISABLED(ENABLE_HIGH_SPEED_STEPPING)
      STEP_END(x, X);
      STEP_END(y, Y);
      STEP_END(z, Z);
      #ifndef ADVANCE
        STEP_END(e, E);
      #endif
    #endif

    HAL_timer_stepper_count(timer);

    // If current block is finished, reset pointer
    if (step_events_completed >= current_block->step_event_count) {
      current_block = NULL;
      plan_discard_current_block();
    }
  }
}

#if ENABLED(ADVANCE)
  unsigned char old_OCR0A;
  // Timer interrupt for E. e_steps is set in the main routine;
  // Timer 0 is shared with millies
  ISR(TIMER0_COMPA_vect) {
    old_OCR0A += 52; // ~10kHz interrupt (250000 / 26 = 9615kHz)
    OCR0A = old_OCR0A;
    // Set E direction (Depends on E direction + advance)
    for (unsigned char i = 0; i < 4; i++) {
      if (e_steps[0] != 0) {
        E0_STEP_WRITE(INVERT_E_STEP_PIN);
        if (e_steps[0] < 0) {
          #if ENABLED(DONDOLO)
            if (active_extruder == 0)
              E0_DIR_WRITE(INVERT_E0_DIR);
            else
              E0_DIR_WRITE(!INVERT_E0_DIR);
          #else
            E0_DIR_WRITE(INVERT_E0_DIR);
          #endif
          e_steps[0]++;
          E0_STEP_WRITE(!INVERT_E_STEP_PIN);
        }
        else if (e_steps[0] > 0) {
          #if ENABLED(DONDOLO)
            if (active_extruder == 0)
              E0_DIR_WRITE(!INVERT_E0_DIR);
            else
              E0_DIR_WRITE(INVERT_E0_DIR);
          #else
            E0_DIR_WRITE(!INVERT_E0_DIR);
          #endif
          e_steps[0]--;
          E0_STEP_WRITE(!INVERT_E_STEP_PIN);
        }
      }
      #if DRIVER_EXTRUDERS > 1
        if (e_steps[1] != 0) {
          E1_STEP_WRITE(INVERT_E_STEP_PIN);
          if (e_steps[1] < 0) {
            E1_DIR_WRITE(INVERT_E1_DIR);
            e_steps[1]++;
            E1_STEP_WRITE(!INVERT_E_STEP_PIN);
          }
          else if (e_steps[1] > 0) {
            E1_DIR_WRITE(!INVERT_E1_DIR);
            e_steps[1]--;
            E1_STEP_WRITE(!INVERT_E_STEP_PIN);
          }
        }
      #endif
      #if DRIVER_EXTRUDERS > 2
        if (e_steps[2] != 0) {
          E2_STEP_WRITE(INVERT_E_STEP_PIN);
          if (e_steps[2] < 0) {
            E2_DIR_WRITE(INVERT_E2_DIR);
            e_steps[2]++;
            E2_STEP_WRITE(!INVERT_E_STEP_PIN);
          }
          else if (e_steps[2] > 0) {
            E2_DIR_WRITE(!INVERT_E2_DIR);
            e_steps[2]--;
            E2_STEP_WRITE(!INVERT_E_STEP_PIN);
          }
        }
      #endif
      #if DRIVER_EXTRUDERS > 3
        if (e_steps[3] != 0) {
          E3_STEP_WRITE(INVERT_E_STEP_PIN);
          if (e_steps[3] < 0) {
            E3_DIR_WRITE(INVERT_E3_DIR);
            e_steps[3]++;
            E3_STEP_WRITE(!INVERT_E_STEP_PIN);
          }
          else if (e_steps[3] > 0) {
            E3_DIR_WRITE(!INVERT_E3_DIR);
            e_steps[3]--;
            E3_STEP_WRITE(!INVERT_E_STEP_PIN);
          }
        }
      #endif
    }
  }
#endif // ADVANCE

void st_init() {
  digipot_init(); //Initialize Digipot Motor Current
  microstep_init(); //Initialize Microstepping Pins

  // initialise TMC Steppers
  #if ENABLED(HAVE_TMCDRIVER)
    tmc_init();
  #endif
    // initialise L6470 Steppers
  #if ENABLED(HAVE_L6470DRIVER)
    L6470_init();
  #endif

  // Initialize Dir Pins
  #if HAS(X_DIR)
    X_DIR_INIT;
  #endif
  #if HAS(X2_DIR)
    X2_DIR_INIT;
  #endif
  #if HAS(Y_DIR)
    Y_DIR_INIT;
    #if ENABLED(Y_DUAL_STEPPER_DRIVERS) && HAS(Y2_DIR)
      Y2_DIR_INIT;
    #endif
  #endif
  #if HAS(Z_DIR)
    Z_DIR_INIT;
    #if ENABLED(Z_DUAL_STEPPER_DRIVERS) && HAS(Z2_DIR)
      Z2_DIR_INIT;
    #endif
  #endif
  #if HAS(E0_DIR)
    E0_DIR_INIT;
  #endif
  #if HAS(E1_DIR)
    E1_DIR_INIT;
  #endif
  #if HAS(E2_DIR)
    E2_DIR_INIT;
  #endif
  #if HAS(E3_DIR)
    E3_DIR_INIT;
  #endif

  //Initialize Enable Pins - steppers default to disabled.

  #if HAS(X_ENABLE)
    X_ENABLE_INIT;
    if (!X_ENABLE_ON) X_ENABLE_WRITE(HIGH);
  #endif
  #if HAS(X2_ENABLE)
    X2_ENABLE_INIT;
    if (!X_ENABLE_ON) X2_ENABLE_WRITE(HIGH);
  #endif
  #if HAS(Y_ENABLE)
    Y_ENABLE_INIT;
    if (!Y_ENABLE_ON) Y_ENABLE_WRITE(HIGH);

  #if ENABLED(Y_DUAL_STEPPER_DRIVERS) && HAS(Y2_ENABLE)
    Y2_ENABLE_INIT;
    if (!Y_ENABLE_ON) Y2_ENABLE_WRITE(HIGH);
  #endif
  #endif
  #if HAS(Z_ENABLE)
    Z_ENABLE_INIT;
    if (!Z_ENABLE_ON) Z_ENABLE_WRITE(HIGH);

    #if ENABLED(Z_DUAL_STEPPER_DRIVERS) && HAS(Z2_ENABLE)
      Z2_ENABLE_INIT;
      if (!Z_ENABLE_ON) Z2_ENABLE_WRITE(HIGH);
    #endif
  #endif
  #if HAS(E0_ENABLE)
    E0_ENABLE_INIT;
    if (!E_ENABLE_ON) E0_ENABLE_WRITE(HIGH);
  #endif
  #if HAS(E1_ENABLE)
    E1_ENABLE_INIT;
    if (!E_ENABLE_ON) E1_ENABLE_WRITE(HIGH);
  #endif
  #if HAS(E2_ENABLE)
    E2_ENABLE_INIT;
    if (!E_ENABLE_ON) E2_ENABLE_WRITE(HIGH);
  #endif
  #if HAS(E3_ENABLE)
    E3_ENABLE_INIT;
    if (!E_ENABLE_ON) E3_ENABLE_WRITE(HIGH);
  #endif

  //Choice E0-E1 or E0-E2 or E1-E3 pin
  #if ENABLED(MKR4) && HAS(E0E1)
    OUT_WRITE_RELE(E0E1_CHOICE_PIN, LOW);
  #endif
  #if ENABLED(MKR4) && HAS(E0E2)
    OUT_WRITE_RELE(E0E2_CHOICE_PIN, LOW);
  #endif
  #if ENABLED(MKR4) && HAS(E0E3)
    OUT_WRITE_RELE(E0E3_CHOICE_PIN, LOW);
  #endif
  #if ENABLED(MKR4) && HAS(E1E3)
    OUT_WRITE_RELE(E1E3_CHOICE_PIN, LOW);
  #endif

  //endstops and pullups

  #if HAS(X_MIN)
    SET_INPUT(X_MIN_PIN);
    #if ENABLED(ENDSTOPPULLUP_XMIN)
      PULLUP(X_MIN_PIN, HIGH);
    #endif
  #endif

  #if HAS(Y_MIN)
    SET_INPUT(Y_MIN_PIN);
    #if ENABLED(ENDSTOPPULLUP_YMIN)
      PULLUP(Y_MIN_PIN ,HIGH);
    #endif
  #endif

  #if HAS(Z_MIN)
    SET_INPUT(Z_MIN_PIN);
    #if ENABLED(ENDSTOPPULLUP_ZMIN)
      PULLUP(Z_MIN_PIN, HIGH);
    #endif
  #endif

  #if HAS(Z2_MIN)
    SET_INPUT(Z2_MIN_PIN);
    #if ENABLED(ENDSTOPPULLUP_Z2MIN)
      PULLUP(Z2_MIN_PIN,HIGH);
    #endif
  #endif

  #if HAS(E_MIN)
    SET_INPUT(E_MIN_PIN);
    #if ENABLED(ENDSTOPPULLUP_EMIN)
      PULLUP(E_MIN_PIN, HIGH);
    #endif
  #endif

  #if HAS(X_MAX)
    SET_INPUT(X_MAX_PIN);
    #if ENABLED(ENDSTOPPULLUP_XMAX)
      PULLUP(X_MAX_PIN, HIGH);
    #endif
  #endif

  #if HAS(Y_MAX)
    SET_INPUT(Y_MAX_PIN);
    #if ENABLED(ENDSTOPPULLUP_YMAX)
      PULLUP(Y_MAX_PIN, HIGH);
    #endif
  #endif

  #if HAS(Z_MAX)
    SET_INPUT(Z_MAX_PIN);
    #if ENABLED(ENDSTOPPULLUP_ZMAX)
      PULLUP(Z_MAX_PIN, HIGH);
    #endif
  #endif

  #if HAS(Z2_MAX)
    SET_INPUT(Z2_MAX_PIN);
    #if ENABLED(ENDSTOPPULLUP_Z2MAX)
      PULLUP(Z2_MAX_PIN, HIGH);
    #endif
  #endif

  #if HAS(Z_PROBE) // Check for Z_PROBE_ENDSTOP so we don't pull a pin high unless it's to be used.
    SET_INPUT(Z_PROBE_PIN);
    #if ENABLED(ENDSTOPPULLUP_ZPROBE)
      PULLUP(Z_PROBE_PIN, HIGH);
    #endif
  #endif

  #define _STEP_INIT(AXIS) AXIS ##_STEP_INIT
  #define _WRITE_STEP(AXIS, HIGHLOW) AXIS ##_STEP_WRITE(HIGHLOW)
  #define _DISABLE(axis) disable_## axis()

  #define AXIS_INIT(axis, AXIS, PIN) \
    _STEP_INIT(AXIS); \
    _WRITE_STEP(AXIS, _INVERT_STEP_PIN(PIN)); \
    _DISABLE(axis)

  #define E_AXIS_INIT(NUM) AXIS_INIT(e## NUM, E## NUM, E)

  // Initialize Step Pins
  #if HAS(X_STEP)
    AXIS_INIT(x, X, X);
  #endif
  #if HAS(X2_STEP)
    AXIS_INIT(x, X2, X);
  #endif
  #if HAS(Y_STEP)
    #if ENABLED(Y_DUAL_STEPPER_DRIVERS) && HAS(Y2_STEP)
      Y2_STEP_INIT;
      Y2_STEP_WRITE(INVERT_Y_STEP_PIN);
    #endif
    AXIS_INIT(y, Y, Y);
  #endif
  #if HAS(Z_STEP)
    #if ENABLED(Z_DUAL_STEPPER_DRIVERS) && HAS(Z2_STEP)
      Z2_STEP_INIT;
      Z2_STEP_WRITE(INVERT_Z_STEP_PIN);
    #endif
    AXIS_INIT(z, Z, Z);
  #endif
  #if HAS(E0_STEP)
    E_AXIS_INIT(0);
  #endif
  #if HAS(E1_STEP)
    E_AXIS_INIT(1);
  #endif
  #if HAS(E2_STEP)
    E_AXIS_INIT(2);
  #endif
  #if HAS(E3_STEP)
    E_AXIS_INIT(3);
  #endif

  HAL_step_timer_start();
  ENABLE_STEPPER_DRIVER_INTERRUPT();

  enable_endstops(true); // Start with endstops active. After homing they can be disabled
  sei();

  set_stepper_direction(); // Init directions to out_bits = 0
}


/**
 * Block until all buffered steps are executed
 */
void st_synchronize() { while (blocks_queued()) idle(); }

void st_set_position(const long& x, const long& y, const long& z, const long& e) {
  CRITICAL_SECTION_START;
  count_position[X_AXIS] = x;
  count_position[Y_AXIS] = y;
  count_position[Z_AXIS] = z;
  count_position[E_AXIS] = e;
  CRITICAL_SECTION_END;
}

void st_set_e_position(const long& e) {
  CRITICAL_SECTION_START;
  count_position[E_AXIS] = e;
  CRITICAL_SECTION_END;
}

long st_get_position(uint8_t axis) {
  long count_pos;
  CRITICAL_SECTION_START;
  count_pos = count_position[axis];
  CRITICAL_SECTION_END;
  return count_pos;
}

float st_get_position_mm(AxisEnum axis) { return st_get_position(axis) / axis_steps_per_unit[axis]; }

void enable_all_steppers() {
  enable_x();
  enable_y();
  enable_z();
  enable_e0();
  enable_e1();
  enable_e2();
  enable_e3();
}

void disable_all_steppers() {
  disable_x();
  disable_y();
  disable_z();
  disable_e0();
  disable_e1();
  disable_e2();
  disable_e3();
}

void finishAndDisableSteppers() {
  st_synchronize();
  disable_all_steppers();
}

void quickStop() {
  cleaning_buffer_counter = 5000;
  DISABLE_STEPPER_DRIVER_INTERRUPT();
  while (blocks_queued()) plan_discard_current_block();
  current_block = NULL;
  ENABLE_STEPPER_DRIVER_INTERRUPT();
}

#if ENABLED(NPR2)
  void colorstep(long csteps,const bool direction) {
    enable_e1();
    //setup new step
    WRITE(E1_DIR_PIN,(INVERT_E1_DIR)^direction);
    //perform step
    for(long i=0; i<=csteps; i++){
      WRITE(E1_STEP_PIN, !INVERT_E_STEP_PIN);
      delayMicroseconds(COLOR_SLOWRATE);
      WRITE(E1_STEP_PIN, INVERT_E_STEP_PIN);
      delayMicroseconds(COLOR_SLOWRATE);
    }
  }  
#endif //NPR2

#if ENABLED(BABYSTEPPING)

  // MUST ONLY BE CALLED BY AN ISR,
  // No other ISR should ever interrupt this!
  void babystep(const uint8_t axis, const bool direction) {

    #define _ENABLE(axis) enable_## axis()
    #define _READ_DIR(AXIS) AXIS ##_DIR_READ
    #define _INVERT_DIR(AXIS) INVERT_## AXIS ##_DIR
    #define _APPLY_DIR(AXIS, INVERT) AXIS ##_APPLY_DIR(INVERT, true)

    #define BABYSTEP_AXIS(axis, AXIS, INVERT) { \
        _ENABLE(axis); \
        uint8_t old_pin = _READ_DIR(AXIS); \
        _APPLY_DIR(AXIS, _INVERT_DIR(AXIS)^direction^INVERT); \
        _APPLY_STEP(AXIS)(!_INVERT_STEP_PIN(AXIS), true); \
        delayMicroseconds(2); \
        _APPLY_STEP(AXIS)(_INVERT_STEP_PIN(AXIS), true); \
        _APPLY_DIR(AXIS, old_pin); \
      }

    switch (axis) {

      case X_AXIS:
        BABYSTEP_AXIS(x, X, false);
        break;

      case Y_AXIS:
        BABYSTEP_AXIS(y, Y, false);
        break;

      case Z_AXIS: {

        #if !MECH(DELTA)

          BABYSTEP_AXIS(z, Z, BABYSTEP_INVERT_Z);

        #else // DELTA

          bool z_direction = direction ^ BABYSTEP_INVERT_Z;

          enable_x();
          enable_y();
          enable_z();
          uint8_t old_x_dir_pin = X_DIR_READ,
                  old_y_dir_pin = Y_DIR_READ,
                  old_z_dir_pin = Z_DIR_READ;
          //setup new step
          X_DIR_WRITE(INVERT_X_DIR ^ z_direction);
          Y_DIR_WRITE(INVERT_Y_DIR ^ z_direction);
          Z_DIR_WRITE(INVERT_Z_DIR ^ z_direction);
          // perform step
          X_STEP_WRITE(!INVERT_X_STEP_PIN);
          Y_STEP_WRITE(!INVERT_Y_STEP_PIN);
          Z_STEP_WRITE(!INVERT_Z_STEP_PIN);
          _delay_us(1U);
          X_STEP_WRITE(INVERT_X_STEP_PIN);
          Y_STEP_WRITE(INVERT_Y_STEP_PIN);
          Z_STEP_WRITE(INVERT_Z_STEP_PIN);
          //get old pin state back.
          X_DIR_WRITE(old_x_dir_pin);
          Y_DIR_WRITE(old_y_dir_pin);
          Z_DIR_WRITE(old_z_dir_pin);

        #endif

      } break;

      default: break;
    }
  }

#endif //BABYSTEPPING

// From Arduino DigitalPotControl example
void digitalPotWrite(int address, int value) {
  #if HAS(DIGIPOTSS)
    digitalWrite(DIGIPOTSS_PIN, LOW); // take the SS pin low to select the chip
    SPI.transfer(address); //  send in the address and value via SPI:
    SPI.transfer(value);
    digitalWrite(DIGIPOTSS_PIN, HIGH); // take the SS pin high to de-select the chip:
    //delay(10);
  #else
    UNUSED(address);
    UNUSED(value);
  #endif
}

// Initialize Digipot Motor Current
void digipot_init() {
  #if HAS(DIGIPOTSS)
    const uint8_t digipot_motor_current[] = DIGIPOT_MOTOR_CURRENT;

    SPI.begin();
    pinMode(DIGIPOTSS_PIN, OUTPUT);
    for (int i = 0; i <= 4; i++) {
      //digitalPotWrite(digipot_ch[i], digipot_motor_current[i]);
      digipot_current(i, digipot_motor_current[i]);
    }
  #endif
  #if HAS(MOTOR_CURRENT_PWM_XY)
    pinMode(MOTOR_CURRENT_PWM_XY_PIN, OUTPUT);
    pinMode(MOTOR_CURRENT_PWM_Z_PIN, OUTPUT);
    pinMode(MOTOR_CURRENT_PWM_E_PIN, OUTPUT);
    digipot_current(0, motor_current_setting[0]);
    digipot_current(1, motor_current_setting[1]);
    digipot_current(2, motor_current_setting[2]);
    //Set timer5 to 31khz so the PWM of the motor power is as constant as possible. (removes a buzzing noise)
    TCCR5B = (TCCR5B & ~(_BV(CS50) | _BV(CS51) | _BV(CS52))) | _BV(CS50);
  #endif

  #if MB(ALLIGATOR)
    const float motor_current[] = MOTOR_CURRENT;
    unsigned int digipot_motor = 0;
    for (uint8_t i = 0; i < 3 + DRIVER_EXTRUDERS; i++) {
      digipot_motor = 255 * (motor_current[i] / 2.5);
      ExternalDac::setValue(i, digipot_motor);
    }
  #endif//MB(ALLIGATOR)
}

void digipot_current(uint8_t driver, int current) {
  #if HAS(DIGIPOTSS)
    const uint8_t digipot_ch[] = DIGIPOT_CHANNELS;
    digitalPotWrite(digipot_ch[driver], current);
  #elif HAS(MOTOR_CURRENT_PWM_XY)
    switch (driver) {
      case 0: analogWrite(MOTOR_CURRENT_PWM_XY_PIN, 255L * current / MOTOR_CURRENT_PWM_RANGE); break;
      case 1: analogWrite(MOTOR_CURRENT_PWM_Z_PIN, 255L * current / MOTOR_CURRENT_PWM_RANGE); break;
      case 2: analogWrite(MOTOR_CURRENT_PWM_E_PIN, 255L * current / MOTOR_CURRENT_PWM_RANGE); break;
    }
  #else
    UNUSED(driver);
    UNUSED(current);
  #endif
}

void microstep_init() {
  #if HAS(MICROSTEPS_E1)
    pinMode(E1_MS1_PIN, OUTPUT);
    pinMode(E1_MS2_PIN, OUTPUT);
  #endif

  #if HAS(MICROSTEPS)
    pinMode(X_MS1_PIN, OUTPUT);
    pinMode(X_MS2_PIN, OUTPUT);
    pinMode(Y_MS1_PIN, OUTPUT);
    pinMode(Y_MS2_PIN, OUTPUT);
    pinMode(Z_MS1_PIN, OUTPUT);
    pinMode(Z_MS2_PIN, OUTPUT);
    pinMode(E0_MS1_PIN, OUTPUT);
    pinMode(E0_MS2_PIN, OUTPUT);
    const uint8_t microstep_modes[] = MICROSTEP_MODES;
    for (uint16_t i = 0; i < COUNT(microstep_modes); i++)
      microstep_mode(i, microstep_modes[i]);
  #endif
}

void microstep_ms(uint8_t driver, int8_t ms1, int8_t ms2) {
  if (ms1 >= 0) switch (driver) {
    case 0: digitalWrite(X_MS1_PIN, ms1); break;
    case 1: digitalWrite(Y_MS1_PIN, ms1); break;
    case 2: digitalWrite(Z_MS1_PIN, ms1); break;
    case 3: digitalWrite(E0_MS1_PIN, ms1); break;
    #if HAS(MICROSTEPS_E1)
      case 4: digitalWrite(E1_MS1_PIN, ms1); break;
    #endif
  }
  if (ms2 >= 0) switch (driver) {
    case 0: digitalWrite(X_MS2_PIN, ms2); break;
    case 1: digitalWrite(Y_MS2_PIN, ms2); break;
    case 2: digitalWrite(Z_MS2_PIN, ms2); break;
    case 3: digitalWrite(E0_MS2_PIN, ms2); break;
    #if PIN_EXISTS(E1_MS2)
      case 4: digitalWrite(E1_MS2_PIN, ms2); break;
    #endif
  }
}

void microstep_mode(uint8_t driver, uint8_t stepping_mode) {
  switch (stepping_mode) {
    case 1: microstep_ms(driver,  MICROSTEP1); break;
    case 2: microstep_ms(driver,  MICROSTEP2); break;
    case 4: microstep_ms(driver,  MICROSTEP4); break;
    case 8: microstep_ms(driver,  MICROSTEP8); break;
    case 16: microstep_ms(driver, MICROSTEP16); break;
    #if MB(ALLIGATOR)
      case 32: microstep_ms(driver, MICROSTEP32); break;
    #endif
  }
}

void microstep_readings() {
  ECHO_SM(DB, MSG_MICROSTEP_MS1_MS2);
  ECHO_M(MSG_MICROSTEP_X);
  ECHO_V(digitalRead(X_MS1_PIN));
  ECHO_EV(digitalRead(X_MS2_PIN));
  ECHO_SM(DB, MSG_MICROSTEP_Y);
  ECHO_V(digitalRead(Y_MS1_PIN));
  ECHO_EV(digitalRead(Y_MS2_PIN));
  ECHO_SM(DB, MSG_MICROSTEP_Z);
  ECHO_V(digitalRead(Z_MS1_PIN));
  ECHO_EV(digitalRead(Z_MS2_PIN));
  ECHO_SM(DB, MSG_MICROSTEP_E0);
  ECHO_V(digitalRead(E0_MS1_PIN));
  ECHO_EV(digitalRead(E0_MS2_PIN));
  #if HAS(MICROSTEPS_E1)
    ECHO_SM(DB, MSG_MICROSTEP_E1);
    ECHO_V(digitalRead(E1_MS1_PIN));
    ECHO_EV(digitalRead(E1_MS2_PIN));
  #endif
}

#if ENABLED(Z_DUAL_ENDSTOPS)
  void In_Homing_Process(bool state) { performing_homing = state; }
  void Lock_z_motor(bool state) { locked_z_motor = state; }
  void Lock_z2_motor(bool state) { locked_z2_motor = state; }
#endif
