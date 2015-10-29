/* Copyright (c) 2014, The Linux Foundation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above
 *     copyright notice, this list of conditions and the following
 *     disclaimer in the documentation and/or other materials provided
 *     with the distribution.
 *   * Neither the name of The Linux Foundation, Inc. nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef __DEV_PMIC_VIB_VIBRATOR_H
#define __DEV_PMIC_VIB_VIBRATOR_H

#ifndef QPNP_HAPTIC_SUPPORT
#define QPNP_VIB_EN_CTL             0x1c046
#define QPNP_VIB_VTG_CTL            0x1c041
#define QPNP_VIB_VTG_SET_MASK       0x1F
#define QPNP_VIB_DEFAULT_VTG_LVL    22

#else /* QPNP_HAPTIC_SUPPORT */

#define QPNP_HAP_EN_CTL_REG				0x3C046
#define QPNP_HAP_ACT_TYPE_REG			0x3C04C
#define QPNP_HAP_WAV_SHAPE_REG			0x3C04D
#define QPNP_HAP_PLAY_MODE_REG			0x3C04E
#define QPNP_HAP_LRA_AUTO_RES_REG		0x3C04F
#define QPNP_HAP_VMAX_REG				0x3C051
#define QPNP_HAP_ILIM_REG				0x3C052
#define QPNP_HAP_SC_DEB_REG				0x3C053
#define QPNP_HAP_RATE_CFG1_REG			0x3C054
#define QPNP_HAP_RATE_CFG2_REG			0x3C055
#define QPNP_HAP_INT_PWM_REG			0x3C056
#define QPNP_HAP_EXT_PWM_REG			0x3C057
#define QPNP_HAP_PWM_CAP_REG			0x3C058
#define QPNP_HAP_SC_CLR_REG				0x3C059
#define QPNP_HAP_BRAKE_REG				0x3C05C
#define QPNP_HAP_WAV_REP_REG			0x3C05E
#define QPNP_HAP_WAV_S_REG_BASE			0x3C060
#define QPNP_HAP_PLAY_REG				0x3C070

#define QPNP_HAP_ACT_TYPE_MASK		0xFE
#define QPNP_HAP_LRA			0x0
#define QPNP_HAP_ERM			0x1
#define QPNP_HAP_AUTO_RES_MODE_MASK	0x8F
#define QPNP_HAP_AUTO_RES_MODE_SHIFT	4
#define QPNP_HAP_LRA_HIGH_Z_MASK	0xF3
#define QPNP_HAP_LRA_HIGH_Z_SHIFT	2
#define QPNP_HAP_LRA_RES_CAL_PER_MASK	0xFC
#define QPNP_HAP_RES_CAL_PERIOD_MIN	4
#define QPNP_HAP_RES_CAL_PERIOD_MAX	32
#define QPNP_HAP_PLAY_MODE_MASK		0xCF
#define QPNP_HAP_PLAY_MODE_SHFT		4
#define QPNP_HAP_VMAX_MASK		0xC1
#define QPNP_HAP_VMAX_SHIFT		1
#define QPNP_HAP_VMAX_MIN_MV		116
#define QPNP_HAP_VMAX_MAX_MV		3596
#define QPNP_HAP_ILIM_MASK		0xFE
#define QPNP_HAP_ILIM_MIN_MV		400
#define QPNP_HAP_ILIM_MAX_MV		800
#define QPNP_HAP_SC_DEB_MASK		0xF8
#define QPNP_HAP_SC_DEB_SUB		2
#define QPNP_HAP_SC_DEB_CYCLES_MIN	0
#define QPNP_HAP_DEF_SC_DEB_CYCLES	8
#define QPNP_HAP_SC_DEB_CYCLES_MAX	32
#define QPNP_HAP_SC_CLR			1
#define QPNP_HAP_INT_PWM_MASK		0xFC
#define QPNP_HAP_INT_PWM_FREQ_253_KHZ	253
#define QPNP_HAP_INT_PWM_FREQ_505_KHZ	505
#define QPNP_HAP_INT_PWM_FREQ_739_KHZ	739
#define QPNP_HAP_INT_PWM_FREQ_1076_KHZ	1076
#define QPNP_HAP_WAV_SHAPE_MASK		0xFE
#define QPNP_HAP_RATE_CFG1_MASK		0xFF
#define QPNP_HAP_RATE_CFG2_MASK		0xF0
#define QPNP_HAP_RATE_CFG2_SHFT		8
#define QPNP_HAP_RATE_CFG_STEP_US	5
#define QPNP_HAP_WAV_PLAY_RATE_US_MIN	0
#define QPNP_HAP_DEF_WAVE_PLAY_RATE_US	5715
#define QPNP_HAP_WAV_PLAY_RATE_US_MAX	20475
#define QPNP_HAP_WAV_REP_MASK		0x8F
#define QPNP_HAP_WAV_S_REP_MASK		0xFC
#define QPNP_HAP_WAV_REP_SHFT		4
#define QPNP_HAP_WAV_REP_MIN		1
#define QPNP_HAP_WAV_REP_MAX		128
#define QPNP_HAP_WAV_S_REP_MIN		1
#define QPNP_HAP_WAV_S_REP_MAX		8
#define QPNP_HAP_BRAKE_PAT_MASK		0x3
#define QPNP_HAP_ILIM_MIN_MA		400
#define QPNP_HAP_ILIM_MAX_MA		800
#define QPNP_HAP_EXT_PWM_MASK		0xFC
#define QPNP_HAP_EXT_PWM_FREQ_25_KHZ	25
#define QPNP_HAP_EXT_PWM_FREQ_50_KHZ	50
#define QPNP_HAP_EXT_PWM_FREQ_75_KHZ	75
#define QPNP_HAP_EXT_PWM_FREQ_100_KHZ	100
#define QPNP_HAP_WAV_SINE		0
#define QPNP_HAP_WAV_SQUARE		1
#define QPNP_HAP_WAV_SAMP_LEN		8
#define QPNP_HAP_WAV_SAMP_MAX		0x7E
#define QPNP_HAP_BRAKE_PAT_LEN		4
#define QPNP_HAP_PLAY_EN		0x80
#define QPNP_HAP_EN			0x80

#define QPNP_HAP_TIMEOUT_MS_MAX		15000
#define QPNP_HAP_STR_SIZE		20

extern void pm8x41_reg_write_mask(uint32_t addr, uint8_t mask, uint8_t val);

#endif /* QPNP_HAPTIC_SUPPORT */

void pm_vib_turn_on(void);
void pm_vib_turn_off(void);

#endif/* __DEV_PMIC_VIB_VIBRATOR_H */
