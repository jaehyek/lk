/* Copyright (c) 2014-2015, The Linux Foundation. All rights reserved.
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

#include <bits.h>
#include <pm8x41_hw.h>
#include <pm_vib.h>
#include <pm8x41_hw.h>

#define QPNP_VIB_EN    BIT(7)

#ifndef QPNP_HAPTIC_SUPPORT
/* Turn on vibrator */
void pm_vib_turn_on(void)
{
	uint8_t val;

	val = pm8x41_reg_read(QPNP_VIB_VTG_CTL);
	val &= ~QPNP_VIB_VTG_SET_MASK;
	val |= (QPNP_VIB_DEFAULT_VTG_LVL & QPNP_VIB_VTG_SET_MASK);
	pm8x41_reg_write(QPNP_VIB_VTG_CTL, val);

	val = pm8x41_reg_read(QPNP_VIB_EN_CTL);
	val |= QPNP_VIB_EN;
	pm8x41_reg_write(QPNP_VIB_EN_CTL, val);
}

/* Turn off vibrator */
void pm_vib_turn_off(void)
{
	uint8_t val;

	val = pm8x41_reg_read(QPNP_VIB_EN_CTL);
	val &= ~QPNP_VIB_EN;
	pm8x41_reg_write(QPNP_VIB_EN_CTL, val);
}

#else /* QPNP_HAPTIC_SUPPORT */

/* Turn on vibrator */
void pm_vib_turn_on(void)
{
	/* Configure the ACTUATOR TYPE register */
	pm8x41_reg_write_mask(QPNP_HAP_ACT_TYPE_REG, QPNP_HAP_ACT_TYPE_MASK, 0X01);

	/* Configure the PLAY MODE register */
	pm8x41_reg_write_mask(QPNP_HAP_PLAY_MODE_REG, QPNP_HAP_PLAY_MODE_MASK, 0x00 << 4);

	/* Configure the VMAX register */
	pm8x41_reg_write_mask(QPNP_HAP_VMAX_REG, QPNP_HAP_VMAX_MASK, 17 << 1);

	/* Configure the ILIM register */
	pm8x41_reg_write_mask(QPNP_HAP_ILIM_REG, QPNP_HAP_ILIM_MASK, 0x1);

	/* Configure the INTERNAL_PWM register */
	pm8x41_reg_write_mask(QPNP_HAP_INT_PWM_REG, QPNP_HAP_INT_PWM_MASK, 0x01);

	/* Configure the WAVE SHAPE register */
	pm8x41_reg_write_mask(QPNP_HAP_WAV_SHAPE_REG, QPNP_HAP_WAV_SHAPE_MASK, QPNP_HAP_WAV_SQUARE);

	/* Cache play register */
	pm8x41_reg_write(QPNP_HAP_PLAY_REG, QPNP_HAP_PLAY_EN);

	/* Enable register on */
	pm8x41_reg_write(QPNP_HAP_EN_CTL_REG, QPNP_HAP_EN);
}

/* Turn off vibrator */
void pm_vib_turn_off(void)
{
	uint8_t val;

	val = pm8x41_reg_read(QPNP_HAP_EN_CTL_REG);
	val &= ~QPNP_VIB_EN;
	pm8x41_reg_write(QPNP_HAP_EN_CTL_REG, val);
}

#endif
