/*
 * Copyright (c) 2008 Travis Geiselbrecht
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files
 * (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#include <debug.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <lib/console.h>
#if PLRTEST_ENABLE
#include "plr_type.h"
#include "plr_hooking.h"
#include "mmc_wrapper.h"
//#define PLR_DEBUG
extern int plr_main(int argc, char *argv[]);
extern void plr_print_log(void);
extern int plr_print_log_sector(uint start_sector, uint sector_count);
#endif

static cmd_block *command_list = NULL;

/* a linear array of statically defined command blocks,
   defined in the linker script.
 */
extern cmd_block __commands_start;
extern cmd_block __commands_end;

static int cmd_help(int argc, const cmd_args *argv);
static int cmd_test(int argc, const cmd_args *argv);
#if PLRTEST_ENABLE
#if 0
static int cmd_plrtest(int argc, const cmd_args *argv);
static int cmd_plrpacked(int argc, const cmd_args *argv);
static int cmd_plrmain(int argc, const cmd_args *argv);
#endif
static int cmd_plrlog(int argc, const cmd_args *argv);
#endif

STATIC_COMMAND_START
	{ "help", "this list", &cmd_help },
	{ "test", "test the command processor", &cmd_test },
#if PLRTEST_ENABLE
#if 0
	/* READ/WRITE/ERASE */
	{ "plrtest", "plrtest test", &cmd_plrtest },
	{ "plr", "plrtest shortcut", &cmd_plrtest },
	/* PACKED CMD */
	{ "plrpacked", "plr packed cmd test", &cmd_plrpacked },
	{ "pp", "plr packed cmd shortcut", &cmd_plrpacked },
	{ "plrmain", "main func for plr test", &cmd_plrmain },
#endif
	/* PLR LOG */
	{ "plrlog", "print logs for plr test", &cmd_plrlog },
	{ "tr", "test report", &cmd_plrlog },

#endif
STATIC_COMMAND_END(help);

int console_init(void)
{
	printf("console_init: entry\n");

	/* add all the statically defined commands to the list */
	cmd_block *block;
	for (block = &__commands_start; block != &__commands_end; block++) {
		console_register_commands(block);
	}

	return 0;
}

static const cmd *match_command(const char *command)
{
	cmd_block *block;
	size_t i;

	for (block = command_list; block != NULL; block = block->next) {
		const cmd *curr_cmd = block->list;
		for (i = 0; i < block->count; i++) {
			if (strcmp(command, curr_cmd[i].cmd_str) == 0) {
				return &curr_cmd[i];
			}
		}
	}

	return NULL;
}

#if PLRTEST_ENABLE
int read_line(char *buffer, int len)
#else
static int read_line(char *buffer, int len)
#endif
{
	int pos = 0;
	int escape_level = 0;

	for (;;) {
		char c;

		/* loop until we get a char */
		if (getc(&c) < 0)
			continue;

//		printf("c = 0x%hhx\n", c);

		if (escape_level == 0) {
			switch (c) {
				case '\r':
				case '\n':
					putc(c);
					goto done;

				case 0x7f: // backspace or delete
				case 0x8:
					if (pos > 0) {
						pos--;
						puts("\x1b[1D"); // move to the left one
						putc(' ');
						puts("\x1b[1D"); // move to the left one
					}
					break;

				case 0x1b: // escape
					escape_level++;
					break;

				default:
					buffer[pos++] = c;
					putc(c);
			}
		} else if (escape_level == 1) {
			// inside an escape, look for '['
			if (c == '[') {
				escape_level++;
			} else {
				// we didn't get it, abort
				escape_level = 0;
			}
		} else { // escape_level > 1
			switch (c) {
				case 67: // right arrow
					buffer[pos++] = ' ';
					putc(' ');
					break;
				case 68: // left arrow
					if (pos > 0) {
						pos--;
						puts("\x1b[1D"); // move to the left one
						putc(' ');
						puts("\x1b[1D"); // move to the left one
					}
					break;
				case 65: // up arrow
				case 66: // down arrow
					// XXX do history here
					break;
				default:
					break;
			}
			escape_level = 0;
		}

		/* end of line. */
		if (pos == (len - 1)) {
			puts("\nerror: line too long\n");
			pos = 0;
			goto done;
		}
	}

done:
//	printf("returning pos %d\n", pos);

	buffer[pos] = 0;
	return pos;
}

static int tokenize_command(char *buffer, cmd_args *args, int arg_count)
{
	int pos;
	int arg;
	bool finished;
	enum {
		INITIAL = 0,
		IN_SPACE,
		IN_TOKEN
	} state;

	pos = 0;
	arg = 0;
	state = INITIAL;
	finished = false;

	for (;;) {
		char c = buffer[pos];

		if (c == '\0')
			finished = true;

//		printf("c 0x%hhx state %d arg %d pos %d\n", c, state, arg, pos);

		switch (state) {
			case INITIAL:
				if (isspace(c)) {
					state = IN_SPACE;
				} else {
					state = IN_TOKEN;
					args[arg].str = &buffer[pos];
				}
				break;
			case IN_TOKEN:
				if (finished) {
					arg++;
					goto done;
				}
				if (isspace(c)) {
					arg++;
					buffer[pos] = 0;
					/* are we out of tokens? */
					if (arg == arg_count)
						goto done;
					state = IN_SPACE;
				}
				pos++;
				break;
			case IN_SPACE:
				if (finished)
					goto done;
				if (!isspace(c)) {
					state = IN_TOKEN;
					args[arg].str = &buffer[pos];
				}
				pos++;
				break;
		}
	}

done:
	return arg;
}

static void convert_args(int argc, cmd_args *argv)
{
	int i;

	for (i = 0; i < argc; i++) {
		argv[i].u = atoui(argv[i].str);
		argv[i].i = atoi(argv[i].str);
	}
}

static void console_loop(void)
{
	cmd_args args[16];
	char buffer[256];

	printf("entering main console loop\n");

	for (;;) {
		puts("] ");

		int len = read_line(buffer, sizeof(buffer));
		if (len == 0)
			continue;

//		printf("line = '%s'\n", buffer);

		/* tokenize the line */
		int argc = tokenize_command(buffer, args, 16);
		if (argc < 0) {
			printf("syntax error\n");
			continue;
		} else if (argc == 0) {
			continue;
		}

//		printf("after tokenize: argc %d\n", argc);
//		for (int i = 0; i < argc; i++)
//			printf("%d: '%s'\n", i, args[i].str);

		/* convert the args */
		convert_args(argc, args);

		/* try to match the command */
		const cmd *command = match_command(args[0].str);
		if (!command) {
			printf("command not found\n");
			continue;
		}

		command->cmd_callback(argc, args);

		// XXX do something with the result
	}
}


void console_start(void)
{
	printf("console_start: entry\n");
	plr_main(1, NULL);
	console_loop();
}

int console_run_command(const char *string)
{
	const cmd *command;

	ASSERT(string != NULL);

	command = match_command(string);
	if (!command)
		return -1;

	int result = command->cmd_callback(0, NULL);

	return result;
}

void console_register_commands(cmd_block *block)
{
	ASSERT(block);
	ASSERT(block->next == NULL);

	block->next = command_list;
	command_list = block;
}

static int cmd_help(int argc, const cmd_args *argv)
{

	printf("command list:\n");

	cmd_block *block;
	size_t i;

	for (block = command_list; block != NULL; block = block->next) {
		const cmd *curr_cmd = block->list;
		for (i = 0; i < block->count; i++) {
			printf("\t%-16s: %s\n", curr_cmd[i].cmd_str, curr_cmd[i].help_str ? curr_cmd[i].help_str : "");
		}
	}

	return 0;
}

static int cmd_test(int argc, const cmd_args *argv)
{
	int i;

	printf("argc %d, argv %p\n", argc, argv);
	for (i = 0; i < argc; i++)
		printf("\t%d: str '%s', i %d, u %#x\n", i, argv[i].str, argv[i].i, argv[i].u);

	return 0;
}

#if PLRTEST_ENABLE
#if 0
static int cmd_plrtest(int argc, const cmd_args *argv)
{
	uint32_t len;
	bool enable;
	void* data;
	int32_t dev_num;

	if (argc == 1) {
		printf("Type this : plrtest help\n");
		return 0;
	}

	/* help */
	if(!strcmp("help", argv[1].str)){
		printf("\n\tCommand Formats are as belows(dev_num:0=SDCARD, 1=eMMC\n");
		printf("========================================================================\n");
		printf("READ               -> plrtest read dev_num start_sector block_num\n");
		printf("WRITE              -> plrtest write dev_num data start_sector block_num\n");
		printf("ERASE              -> plrtest erase dev_num start_sector block_num\n");
		printf("CACHE ON(1)/OFF(0) -> plrtest cache dev_num enable\n");
		printf("NAME               -> plrtest name dev_num\n");
		printf("TEST TIMER TICK-> plrtest timer\n");
	}
	/* read */
	else if(!strcmp("read", argv[1].str)){
		if (argc != 5) {
			printf("Not enough arguments. We need 5 arguments (include CMD)\n");
			printf("plrtest read device_num start_sector block_num\n");
		} else {
			dev_num = argv[2].i;
			if(dev_num == SDCARD_DEV_NUM || dev_num == EMMC_DEV_NUM){
				len = argv[4].i;
				data = malloc(len * SECTOR_SIZE);
				memset(data, 1, len * SECTOR_SIZE);

				do_read(dev_num, (uchar*)data, argv[3].u, len);
				dprintf(INFO,ESC_COLOR_GREEN "[%s]data = 0x%x\n" ESC_COLOR_NORMAL,__func__, *(uint32_t *)data);
#ifdef PLR_DEBUG
				dprintf(INFO,"\tcmd : %s, dev_num : %d, start_sector : %u, len : %u\n", argv[1].str, dev_num, argv[3].u, len);
#endif
				free(data);
			} else {
				printf("please input # 0(SD Card) or 1(eMMC)\n");
			}
		}
	}
	/* write */
	else if(!strcmp("write", argv[1].str)){
		if (argc != 6) {
			printf("Not enough arguments. We need 6 arguments (include CMD)\n");
			printf("plrtest write device_num data start_sector block_num\n");
		} else {
			dev_num = argv[2].i;
			if(dev_num == SDCARD_DEV_NUM || dev_num == EMMC_DEV_NUM){
				do_write(dev_num, (uchar*)argv[3].str, argv[4].u, argv[5].u);
#ifdef PLR_DEBUG
				dprintf(INFO,"\tcmd : %s, dev_num : %d, data : %s, start_sector : %u, len : %d\n", argv[1].str, dev_num, argv[3].str, argv[4].u, argv[5].u);
#endif
			} else {
				printf("please input # 0(SD Card) or 1(eMMC)\n");
			}
		}
	}
	/* erase */
	else if(!strcmp("erase", argv[1].str)){
		if (argc != 5) {
			printf("Not enough arguments. We need 5 arguments (include CMD)\n");
			printf("plrtest erase device_num start_sector block_num\n");
		} else {
			dev_num = argv[2].i;
			if(dev_num == SDCARD_DEV_NUM || dev_num == EMMC_DEV_NUM){
				do_erase(dev_num, argv[3].u, argv[4].u);
#ifdef PLR_DEBUG
				dprintf(INFO,"\tcmd : %s, dev_num : %d, start_sector : %u, len : %u\n", argv[1].str, dev_num, argv[3].u, argv[4].u);
#endif
			} else {
				printf("please input # 0(SD Card) or 1(eMMC)\n");
			}
		}
	}
	/* cache enable*/
	else if(!strcmp("cache", argv[1].str)){
		if (argc != 4) {
			printf("Not enough arguments. We need 4 arguments (include CMD)\n");
			printf("plrtest cache device_num enable\n");
		} else {
			dev_num = argv[2].i;
			if (dev_num != EMMC_DEV_NUM){
				printf("Only eMMC(device_num = 1) cache can be controlled!! \n");
				return -1;
			} else {
				enable = argv[3].i;
				do_cache_ctrl(dev_num, enable);
#ifdef PLR_DEBUG
				if (enable == 0) {
					dprintf(INFO,"\tcmd : %s, dev_num : %d, enable : %d\n", argv[1].str, dev_num, enable);
				}
#endif
			}
		}
	}
	/* name */
	else if(!strcmp("name", argv[1].str)){
		if (argc != 3) {
			printf("Not enough arguments. We need 3 arguments (include CMD)\n");
			printf("plrtest name device_num\n");
		}
		else {
			dev_num = argv[2].i;
			if (dev_num != EMMC_DEV_NUM){
				printf("Only eMMC(device_num = 1) cache can be controlled!! \n");
				return -1;
			} else {
				data = malloc(sizeof(char)*6);
				memset(data, 0, (sizeof(char)*6));

				get_mmc_product_name(dev_num, data);
				dprintf(INFO,ESC_COLOR_GREEN "[%s] Product Name = %s\n" ESC_COLOR_NORMAL,__func__, (char *)data);
#ifdef PLR_DEBUG
				dprintf(INFO,"\tcmd : %s, dev_num : %d\n", argv[1].str, dev_num);
#endif
				free(data);
			}
		}
	}
	/* test timer */
	else if(!strcmp("timer", argv[1].str)){
		uint32_t tick = get_tick_count();
		printf("current timer tick is %lu\n", tick);
		mdelay(300U);	// sleep 300ms
		printf("after 300ms sleep, tick count is increased as %lu\n", get_tick_count()-tick);
		reset_tick_count(1);
		mdelay(300U);  // sleep 300ms
		printf("reset_tick_count is also working as well! tick after reset(1) is %lu\n", get_tick_count());
		reset_tick_count(0);
	}
	else
		printf("Type this : plrtest help\n");

	return 0;
}

/* PACKED CMD */
static uint32_t packed_buf_offset = 0;
static void* packed_buf = NULL;
static int cmd_plrpacked_add(int dev_num, ulong start, lbaint_t blkcnt, void* src, int rw)
{
	int ret = 0;

retry:
	ret = do_packed_add_list(dev_num, start, blkcnt, src+packed_buf_offset, rw);
	switch(ret) {
		case MMC_PACKED_ADD :
			packed_buf_offset += (blkcnt * SECTOR_SIZE);
			ret = 0;
			break;
		case MMC_PACKED_PASS :
			ret = 0;
			break;
		case MMC_PACKED_COUNT_FULL :
		case MMC_PACKED_BLOCKS_FULL :
			ret = do_packed_send_list(dev_num);
			packed_buf_offset = 0;
			break;
		case MMC_PACKED_BLOCKS_OVER :
		case MMC_PACKED_CHANGE :
			ret = do_packed_send_list(dev_num);
			packed_buf_offset = 0;
			goto retry;
			break;
		default :
			break;
	}

	return ret;
}

static void print_mmc_packed_list(struct mmc_packed* packed)
{
	struct mmc_req *req;
	int i = 0;
	int sum_blkcnt = 0;

	printf("====PACKED_LIST====(struct mmc_packed  *packed = %lx\n", packed);
	if(!list_is_empty(&packed->packed_list)) {
		list_for_every_entry(&packed->packed_list, req, struct mmc_req, req_list) {
			printf("[%d]req=%lx, cmd_flags=%x, start=%lu, blkcnt=%lu\n",
					i++, req, req->cmd_flags, req->start, req->blkcnt);
			sum_blkcnt += req->blkcnt;
		}
	} else {
		printf("packed list is empty\n");
	}
#ifdef PLR_DEBUG
	/* print packed_buf */
	if(packed_buf) {
		for(i = 0; i < sum_blkcnt; ++i) {
			printf("packed_buf[%d] = 0x%lx\n", i, *(uint32_t*)(packed_buf+(i*SECTOR_SIZE)));
		}
	}
#endif
	return;
}

static int cmd_plrpacked(int argc, const cmd_args *argv)
{
	const int dev_num = EMMC_DEV_NUM;	// packed cmd always works on eMMC

	if (argc == 1) {
		printf("Type this : plrpacked help\n");
		return 0;
	}

	// check packed_buf
	if(!packed_buf) {
		packed_buf = do_packed_create_buff(dev_num, packed_buf);
	}

	/* help */
	if(!strcmp("help", argv[1].str)){
		printf("\n\tCommand Formats are as belows\n");
		printf("========================================================================\n");
		printf("ADD READ CMD    -> plrpacked add read start_sector block_num\n");
		printf("ADD WRITE CMD   -> plrpacked add write data start_sector block_num\n");
		printf("LIST CMD        -> plrpacked list\n");
		printf("SEND CMD        -> plrpacked send\n");
	}
	/* add */
	else if(!strcmp("add", argv[1].str)){
		/* read */
		if(!strcmp("read", argv[2].str)) {
			if (argc != 5) {
				printf("Not enough arguments. We need 5 arguments (include CMD)\n");
				printf("plrpacked add read start_sector block_num\n");
			}
			cmd_plrpacked_add(dev_num, argv[3].u, argv[4].u, packed_buf, READ);
		}
		/* write */
		else if(!strcmp("write", argv[2].str)) {
			if (argc != 6) {
				printf("Not enough arguments. We need 6 arguments (include CMD)\n");
				printf("plrpacked add write data start_sector block_num\n");
			}
			strncpy(packed_buf+packed_buf_offset, argv[3].str, SECTOR_SIZE*argv[5].u);
			cmd_plrpacked_add(dev_num, argv[4].u, argv[5].u, packed_buf, WRITE);
		}
	}
	/* list */
	else if(!strcmp("list", argv[1].str)){
		struct mmc_packed* packed = plr_get_packed_info(dev_num);
		print_mmc_packed_list(packed);
	}
	/* send */
	else if(!strcmp("send", argv[1].str)){
		do_packed_send_list(dev_num);
		packed_buf_offset = 0;
	}
	else
		printf("Type this : plrpacked help\n");

	return 0;
}

static int cmd_plrmain(int argc, const cmd_args *argv)
{
	if (argc > 1) {
		printf("help:\nargc have to be 1 for plr test\n");
		return -1;
	}

	return plr_main(1, (char**)&argv->str);
}
#endif

extern int test_report(char *args[], uint argc);
static int cmd_plrlog(int argc, const cmd_args *argv)
{
	return test_report((char**)&argv->str, argc);
}
#endif
