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

}  // namespace internal
}  // namespace rgb_matrix

#endif  // RGBMATRIX_SPWM_PANEL_REGISTERS_H
