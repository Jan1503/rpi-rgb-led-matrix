// -*- mode: c++; c-basic-offset: 2; indent-tabs-mode: nil; -*-
#ifndef RGBMATRIX_SPWM_PANEL_REGISTERS_H
#define RGBMATRIX_SPWM_PANEL_REGISTERS_H

#include "spwm-panel-config.h"

namespace rgb_matrix {
namespace internal {

SPWM_Config spwm_create_fm6373_config(
    const SPWM_Panel_Settings &spwm_settings, int spwm_columns,
    int spwm_row_address_type, int spwm_register_config);
SPWM_Config spwm_create_icnd1065l_config(
    const SPWM_Panel_Settings &spwm_settings, int spwm_columns,
    int spwm_row_address_type, int spwm_register_config);
SPWM_Config spwm_create_sm16380sh_config(
    const SPWM_Panel_Settings &spwm_settings, int spwm_columns,
    int spwm_row_address_type, int spwm_register_config);
SPWM_Config spwm_create_fm6363_config(
    const SPWM_Panel_Settings &spwm_settings, int spwm_columns,
    int spwm_row_address_type, int spwm_register_config);
SPWM_Config spwm_create_fm6353_config(
    const SPWM_Panel_Settings &spwm_settings, int spwm_columns,
    int spwm_row_address_type, int spwm_register_config);

// Diagnostic live-tuning hook: set runtime overrides for the five FM6363 fixed
// control words (register index 1..5 -> words[0..4]). A value < 0 keeps the
// built-in / env default for that register. Applied by spwm_create_fm6363_config
// on the next config (re)build. Used by the SPWM_FM6363_REG_FILE hot-reload so
// the ghost/pre-charge/current bits can be swept against live content without a
// restart.
void spwm_set_fm6363_register_overrides(const int words[5]);

}  // namespace internal
}  // namespace rgb_matrix

#endif  // RGBMATRIX_SPWM_PANEL_REGISTERS_H
