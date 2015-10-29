/* Copyright (c) 2013-2014, LGE Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *     * Neither the name of The Linux Foundation, Inc. nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
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
 *
 */

#include <debug.h>
#include <lge_ftm.h>
#include <lge_target_ftm.h>

int target_ftm_get_firstboot(void)
{
	/* -1 of firstboot value means that
	 * access error or readable check of real value
	 */
	char firstboot = -1;
	int ret;
	ret = ftm_get_item(LGFTM_FIRSTBOOT_FINISHED, &firstboot);

	if (ret < 0)
		return -1;

	return (int)firstboot;
}

int target_ftm_set_firstboot(char firstboot)
{
	int ret;
	ret = ftm_set_item(LGFTM_FIRSTBOOT_FINISHED, &firstboot);

	if (ret < 0)
		return -1;

	return 0;
}

int target_ftm_get_frst(void)
{
	char status;
	int ret;
	ret = ftm_get_item(LGFTM_FRST_STATUS, &status);

	if (ret < 0)
		return -1;

	return (int)status;
}

int target_ftm_set_frst(char status)
{
	int ret;
	ret = ftm_set_item(LGFTM_FRST_STATUS, &status);

	if (ret < 0)
		return -1;

	return 0;
}

int target_ftm_get_qem(void)
{
	char qem = -1;
	int ret;
	ret = ftm_get_item(LGFTM_QEM, &qem);

	if (ret < 0)
		return -1;

	return (int)qem;
}

int target_ftm_get_fakebattery(void)
{
	char status;
	int ret;
	ret = ftm_get_item(LGFTM_FAKE_BATTERY, &status);

	if (ret < 0)
		return -1;

	return (int)status;
}

int target_ftm_set_fakebattery(char status)
{
	int ret;
	if (status)
		status = 1;

	ret = ftm_set_item(LGFTM_FAKE_BATTERY, &status);

	if (ret < 0)
		return -1;

	return 0;
}

int target_ftm_get_uart(void)
{
	char status;
	int ret;
	ret = ftm_get_item(LGFTM_UART, &status);

	if (ret < 0)
		return -1;

	return (int)status;
}

int target_ftm_set_uart(char status)
{
	int ret;

	ret = ftm_set_item(LGFTM_UART, &status);

	if (ret < 0)
		return -1;

	return 0;
}

int target_ftm_get_bootchart()
{
	char status;
	int ret;

	ret = ftm_get_item(LGFTM_BOOTCHART, &status);

	if (ret < 0)
		return -1;

	return (int)status;
}

int target_ftm_set_bootchart(char status)
{
	int ret;

	ret = ftm_set_item(LGFTM_BOOTCHART, &status);

	if (ret < 0)
		return -1;

	return 0;
}
