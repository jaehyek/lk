/*
 * Copyright (c) 2013  ElixirFlash Technology Co., Ltd.
 *              http://www.elixirflash.com/
 *
 * Powerloss Test for U-boot
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */


#include "plr_common.h"
#include "plr_protocol.h"
#include "plr_tftp.h"
#include "plr_lk_wrapper.h"
#include "plr_deviceio.h"

#define PLRTOKEN  "plrtoken "
#define PLRACK  "plrack "

#define CONFIG_SYS_CBSIZE 256

extern int readline (const char *const prompt);
extern char	console_buffer[CONFIG_SYS_CBSIZE + 1];
extern int g_prepare_option;
extern struct tftp_state g_tftp_state;

const char* receive_token_string[MAX_RECIEVE_TOKEN] = {
	"success",
	"fail",
	"reset",
};

const char* send_token_string[MAX_SEND_TOKEN] = {
	"test_case",
	"prepare_done",
	"test_info",
	"veri_start",

	"recv_write_info",
	"recv_random_seed",

	"crash_log_start",
	"crash_log_end",
	"write_start",

	"send_write_info",
	"send_random_seed",


	"init_info",
	"pl_config",
	"write_config",
	"init_start",
	"reset",
	"env_error",
	"boot_done",
	"boot_cnt",
	"veri_done",
	"write_done",
	"init_done",
	"setloopcnt",
	"setaddress",
	"setplpos",
	"setstatistics",
	"test_finish",
#ifdef PLR_TFTP_FW_UP
	"utftp_state",
	"utftp_setting",
	"utftp_applied",
	"ufile_path",
	"udnload_start",
	"udnload_done",
	"ktftp_setting",
	"kfile_path",
	"kdnload_start",
	"kdnload_done",
	"uupload_start",
	"uupload_done",
	"eupload_start",
	"eupload_done",
	"ext_sw_reset_wait"
#endif
};

const int ACK_PARAM_COUNT[MAX_SEND_TOKEN] = {
	1,	// test_case
	1,	// prepare_done
	22,	// test_info
	0,	// veri_start
	4, 	// recv_write_info
	1,	// recv_random_seed
	0,	// crash_log_start
	0,	// crash_log_end
	0,	// write_start
	0,	// send_write_info
	0, 	// send_random_seed
	5,	// init_info
	-1,	// pl_config : variadic
	-1,	// write_config : variadic
	0,	// init_start
	0,	// reset
	0,	// env_error
	2,	// boot_done
	0,	// boot_cnt
	0,	// veri_done
	0,	// write_done
	0,	// init_done
	0,	// setloopcnt
	0,	// setaddress
	0,	// setplpos
	0,	// setstatistics
	0, 	// test_finish
#ifdef PLR_TFTP_FW_UP
	1,	// utftp_state
	5,	// utftp_setting
	0,	// utftp_applied
	1,	// ufile_path
	0,	// udnload_start
	1,	// udnload_done
	4,	// ktftp_setting
	1,	// kfile_path
	0,	// kdnload_start
	0,	// kdnload_done
#endif
	0,	// uupload_start
	1,	// uupload_done
	0,	// eupload_start
	1,	// eupload_done
};

// pre-generated crc table using polynomiar 0xD5
static unsigned char crc_table[256] = {
	0x0, 0xD5, 0x7F, 0xAA, 0xFE, 0x2B, 0x81, 0x54, 0x29, 0xFC, 0x56, 0x83, 0xD7, 0x2, 0xA8, 0x7D,
	0x52, 0x87, 0x2D, 0xF8, 0xAC, 0x79, 0xD3, 0x6, 0x7B, 0xAE, 0x4, 0xD1, 0x85, 0x50, 0xFA, 0x2F,
	0xA4, 0x71, 0xDB, 0xE, 0x5A, 0x8F, 0x25, 0xF0, 0x8D, 0x58, 0xF2, 0x27, 0x73, 0xA6, 0xC, 0xD9,
	0xF6, 0x23, 0x89, 0x5C, 0x8, 0xDD, 0x77, 0xA2, 0xDF, 0xA, 0xA0, 0x75, 0x21, 0xF4, 0x5E, 0x8B,
	0x9D, 0x48, 0xE2, 0x37, 0x63, 0xB6, 0x1C, 0xC9, 0xB4, 0x61, 0xCB, 0x1E, 0x4A, 0x9F, 0x35, 0xE0,
	0xCF, 0x1A, 0xB0, 0x65, 0x31, 0xE4, 0x4E, 0x9B, 0xE6, 0x33, 0x99, 0x4C, 0x18, 0xCD, 0x67, 0xB2,
	0x39, 0xEC, 0x46, 0x93, 0xC7, 0x12, 0xB8, 0x6D, 0x10, 0xC5, 0x6F, 0xBA, 0xEE, 0x3B, 0x91, 0x44,
	0x6B, 0xBE, 0x14, 0xC1, 0x95, 0x40, 0xEA, 0x3F, 0x42, 0x97, 0x3D, 0xE8, 0xBC, 0x69, 0xC3, 0x16,
	0xEF, 0x3A, 0x90, 0x45, 0x11, 0xC4, 0x6E, 0xBB, 0xC6, 0x13, 0xB9, 0x6C, 0x38, 0xED, 0x47, 0x92,
	0xBD, 0x68, 0xC2, 0x17, 0x43, 0x96, 0x3C, 0xE9, 0x94, 0x41, 0xEB, 0x3E, 0x6A, 0xBF, 0x15, 0xC0,
	0x4B, 0x9E, 0x34, 0xE1, 0xB5, 0x60, 0xCA, 0x1F, 0x62, 0xB7, 0x1D, 0xC8, 0x9C, 0x49, 0xE3, 0x36,
	0x19, 0xCC, 0x66, 0xB3, 0xE7, 0x32, 0x98, 0x4D, 0x30, 0xE5, 0x4F, 0x9A, 0xCE, 0x1B, 0xB1, 0x64,
	0x72, 0xA7, 0xD, 0xD8, 0x8C, 0x59, 0xF3, 0x26, 0x5B, 0x8E, 0x24, 0xF1, 0xA5, 0x70, 0xDA, 0xF,
	0x20, 0xF5, 0x5F, 0x8A, 0xDE, 0xB, 0xA1, 0x74, 0x9, 0xDC, 0x76, 0xA3, 0xF7, 0x22, 0x88, 0x5D,
	0xD6, 0x3, 0xA9, 0x7C, 0x28, 0xFD, 0x57, 0x82, 0xFF, 0x2A, 0x80, 0x55, 0x1, 0xD4, 0x7E, 0xAB,
	0x84, 0x51, 0xFB, 0x2E, 0x7A, 0xAF, 0x5, 0xD0, 0xAD, 0x78, 0xD2, 0x7, 0x53, 0x86, 0x2C, 0xF9
};

#define ACK_PARAM_TOTAL_COUNT(token)	((token >= MAX_SEND_TOKEN) ? 0 : ACK_PARAM_COUNT[token])

typedef struct _protocol_param_{
	int num_of_params;
	char **params;
} PROTOCOL_PARAM, *PPROTOCOL_PARAM;

/* ------------------------------------------------
* Static Function
* ------------------------------------------------*/

/*
 *	author : edward
 *	date : 2013.10.30
 *	function : crc8()
 *	parameter :
 		IN arr(unsigned char *) - crc 값을 확인할 unsigned char 배열 포인터
 		IN len(int) - arr 배열의 길이
 *	return : unsigned char
 *		crc 값
 */
static uchar crc8(uchar *arr, int len)
{
	uchar crc = 0;
	int i;
	if(arr != NULL && len > 0)
	{
		for(i = 0; i < len; ++i)
			crc = crc_table[crc ^ arr[i]];
	}

	return crc;
}

/*
 *	author : edward
 *	date : 2013.08.23
 *	function : _split_params()
 *	parameter :
 		IN str(char*) - protocol ack 의 파라미터 부분 문자열 포인터
 		IN/OUT p(PPROTOCOL_PARARM) - 분리한 파라미터에 대한 구조체
 *	return : int
 *		0 이면 성공, 0 이외의 값은 모두 실패
 *	description :
 *		protocol ack 의 파라미터를 분리하고, 필요한 메모리를 할당하여 PROTOCOL_PARAM 구조체에 저장
 */
static int _split_params(char* str, PPROTOCOL_PARAM p)
{
	int index = 0;
	char *token = NULL, *last = NULL, *buf = NULL;

	if(!str || !p)	return -1;

	// 복사본을 사용하여 strtok 를 호출하도록 함
	buf = (char*)strdup(str);

	// "$1/$2/$3/.../$n/"

	p->num_of_params = 0;
	while(buf[index])
	{
		if(buf[index] == '/') ++(p->num_of_params);
		++index;
	}

	if(buf[index - 1] != '/') ++(p->num_of_params);

	if(!(p->num_of_params)){
		free(buf);
		return -1;
	}

	p->params = (char**)malloc(sizeof(char*) * p->num_of_params);
	token = strtok(buf, "/");
	for(index = 0; index < p->num_of_params && token; ++index)
	{
		p->params[index] = strdup(token);
		last = token;
		token = strtok(NULL, "/");
	}
	if(index < p->num_of_params)
	{
		// move pointer[last] next of '/'
		p->params[p->num_of_params - 1] = strdup(++last);
	}

	free(buf);

	return 0;
}

/*
 *	author : edward
 *	date : 2013.08.23
 *	function : _release_params()
 *	parameter :
 *		IN p(PPROTOCOL_PARARM) - protocol parsing 이 완료된 PROTOCOL_PARAM 구조체의 포인터
 *	return : void
 *	description :
 *		PROTOCOL_PARAM 구조체가 가리키는 동적 할당된 메모리를 모두 해제
 */

static void _release_params(PPROTOCOL_PARAM p)
{
	if(p->params){
		int i = 0;
		for(; i < p->num_of_params; ++i)
		{
			if(p->params[i]){
				free(p->params[i]);
				p->params[i] = NULL;
			}
		}

		free(p->params);
		p->params = NULL;
		p->num_of_params = 0;
	}
}


static char check_valid_string( char* buf, const char *cmp_str)
{
	 int i = 0;

	 uint len = strlen(cmp_str);

	 for (i=0; i<len; i++) {
		 if (buf[i] != cmp_str[i])
			 return 0;
	 }

	 if(strlen(buf) > len + 1){
		strcpy(buf, &buf[len + 1]);
	 }
	 else{
	 	buf[0] = '\0';
	 }

	 return 1;
}

static int readline_plrack_compare( char *buf )
{
	int len = readline(NULL);

	if (!strncmp(console_buffer, PLRACK, 7)) {
 		strcpy(buf, &console_buffer[7]);
 		return len - strlen(PLRACK);
	}
	else
 		return -1;
}


/* test name format : Bx-xxxx-xx
 * [type]-[major version]-[minor version]
 */
void set_test_name_parsing(uchar *test_name)
{
	int count = 0;
	char *sp = NULL;
	char tok_buf[16] = {0};
	char tok_array[3][16];

	memset(tok_array, 0, sizeof(tok_array));


	sprintf (tok_buf, "%s", test_name);
	// plr_debug ("%s\n", tok_buf);

	sp = (char*)strtok(tok_buf, "-");

	// parsing params
	while (sp != NULL) {
		sprintf (tok_array[count++], "%s", sp);
		// plr_debug ("===== param %d = %s\n", count - 1, tok_array[count-1]);
		sp = (char*)strtok(NULL, "-");
	}

	if (count < 1)
		return;

	/* save test type */
	g_plr_state.test_type1 = tok_array[0][0];
	g_plr_state.test_type2 = tok_array[0][1];
	g_plr_state.test_type3 = tok_array[0][2];
	g_plr_state.test_type4 = tok_array[0][3];
	g_plr_state.test_num = strtoud(tok_array[1]);
	g_plr_state.test_minor = strtoud(tok_array[2]);
}

/*
 *	author : edward
 *	date : 2013.08.23
 *	function : _release_params()
 *	parameter :
 *		IN sent_token(e_send_token) - Board 에서 전송한 token
 *		IN p(PPROTOCOL_PARARM) - 전송한 token 에 대한 PC 의 ack 의 파라미터에 대한 PROTOCOL_PARAM 구조체
 *	return : void
 *	description :
 *		PROTOCOL_PARAM 구조체의 파라미터들을 전송한 token 에 따라 parsing 하여
 *		이후 사용을 위해 g_plr_state / g_tftp_state / g_prepare_option 등의 전역 변수에 저장하고
 *		_release_params() 을 호출하여 메모리를 해제
 */

static void set_recv_param_to_state(e_send_token sent_token, PPROTOCOL_PARAM p)
{
	if (p->num_of_params == 0)
		return;

	switch(sent_token){
		case PLRTOKEN_BOOT_DONE:
			g_tftp_state.option = strtoud(p->params[0]);
			// edward. 2013.09.16	receive date information ( format : MMDDHHmmYY.ss )
			memset(g_plr_state.date_info, 0, 14);
			strcpy((char *)g_plr_state.date_info, p->params[1]);
			break;

		case PLRTOKEN_TEST_CASE:
			memset(g_plr_state.test_name, 0, 16);
			strcpy((char *)g_plr_state.test_name, p->params[0]);
			set_test_name_parsing(g_plr_state.test_name);
			break;

		case PLRTOKEN_PREPARE_DONE:
			g_prepare_option = strtoud(p->params[0]);
			break;

		case PLRTOKEN_RECV_WRITE_INFO:
			g_plr_state.write_info.lsn 				= strtoud(p->params[0]);
			g_plr_state.write_info.request_sectors	= strtoud(p->params[1]);
			g_plr_state.write_info.loop_count 		= strtoud(p->params[2]);
			g_plr_state.write_info.boot_count 		= strtoud(p->params[3]);
			break;

		case PLRTOKEN_RECV_RANDOM_SEED:
			g_plr_state.write_info.random_seed 	= strtoud(p->params[0]);
			break;

		case PLRTOKEN_TEST_INFO:
			memset(g_plr_state.test_name, 0, 16);
			strcpy((char *)g_plr_state.test_name, p->params[0]);
			set_test_name_parsing(g_plr_state.test_name);
			g_plr_state.internal_poweroff_type = (bool)strtoud(p->params[1]);
			g_plr_state.test_start_sector = strtoud(p->params[2]);
			g_plr_state.test_sector_length = strtoud(p->params[3]);
			g_plr_state.b_cache_enable = (bool)strtoud(p->params[4]);
			g_plr_state.cache_flush_cycle = strtoud(p->params[5]);
			g_plr_state.b_packed_enable = (bool)strtoud(p->params[6]);
			g_plr_state.packed_flush_cycle = strtoud(p->params[7]);
			g_plr_state.boot_cnt = strtoud(p->params[8]);
			g_plr_state.loop_cnt = strtoud(p->params[9]);
			g_plr_state.checked_addr = strtoud(p->params[10]);
			g_plr_state.poweroff_pos = strtoud(p->params[11]);
			g_plr_state.last_flush_pos = strtoud(p->params[12]);
			g_plr_state.finish.type = strtoud(p->params[13]);
			g_plr_state.finish.value = strtoud(p->params[14]);
			g_plr_state.filled_data.type = (uchar)strtoud(p->params[15]);
			g_plr_state.filled_data.value = (uchar)strtoud(p->params[16]);

			switch	(g_plr_state.filled_data.type) {
				case 0 :
					g_plr_state.filled_data.value = 0xAAAAAAAA;
					break;
				case 1 :
					g_plr_state.filled_data.value = 0x55555555;
					break;
				case 2 :
					g_plr_state.filled_data.value = well512_rand();
					break;
				case 3 :
					plr_debug("filled data[%x]\n", g_plr_state.filled_data.value);
					memset(&(g_plr_state.filled_data.value), g_plr_state.filled_data.value, 4);
					break;
				default :
					break;
			}
			g_plr_state.chunk_size.type = strtoud(p->params[17]);
			g_plr_state.chunk_size.ratio_of_4kb = strtoud(p->params[18]);
			g_plr_state.chunk_size.ratio_of_8kb_64kb = strtoud(p->params[19]);
			g_plr_state.chunk_size.ratio_of_64kb_over = strtoud(p->params[20]);
			g_plr_state.ratio_of_filled = strtoud(p->params[21]);
			break;

		case PLRTOKEN_INIT_INFO:
			g_plr_state.test_start_sector = strtoud(p->params[0]);
			g_plr_state.test_sector_length = strtoud(p->params[1]);
			g_plr_state.filled_data.type = (uchar)strtoud(p->params[2]);
			g_plr_state.filled_data.value = (uchar)strtoud(p->params[3]);
			g_plr_state.ratio_of_filled = strtoud(p->params[4]);
			break;

		case PLRTOKEN_POWERLOSS_CONFIG :
			g_plr_state.pl_info.pl_time_min = strtoud(p->params[0]);
			g_plr_state.pl_info.pl_time_max = strtoud(p->params[1]);
			plr_debug("min[%d], max[%d]\n",
				g_plr_state.pl_info.pl_time_min, g_plr_state.pl_info.pl_time_max);
			break;

#ifdef PLR_TFTP_FW_UP
		case PLRTOKEN_UTFTP_STATE:
			g_tftp_state.state = strtoud(p->params[0]);
			break;

		case PLRTOKEN_UTFTP_SETTING:
			memset(g_tftp_state.client_ip, 0, 16);
			strcpy(g_tftp_state.client_ip, p->params[0]);
			memset(g_tftp_state.server_ip, 0, 16);
			strcpy(g_tftp_state.server_ip, p->params[1]);
			memset(g_tftp_state.gateway_ip, 0, 16);
			strcpy(g_tftp_state.gateway_ip, p->params[2]);
			memset(g_tftp_state.netmask, 0, 16);
			strcpy(g_tftp_state.netmask, p->params[3]);
			memset(g_tftp_state.mac_addr, 0, 18);
			strcpy(g_tftp_state.mac_addr, p->params[4]);
			break;

		case PLRTOKEN_UFILE_PATH:
		case PLRTOKEN_KFILE_PATH:
			memset(g_tftp_state.file_path, 0, 128);
			strcpy(g_tftp_state.file_path, p->params[0]);
			break;

		case PLRTOKEN_KTFTP_SETTING:
			memset(g_tftp_state.client_ip, 0, 16);
			strcpy(g_tftp_state.client_ip, p->params[0]);
			memset(g_tftp_state.server_ip, 0, 16);
			strcpy(g_tftp_state.server_ip, p->params[1]);
			memset(g_tftp_state.netmask, 0, 16);
			strcpy(g_tftp_state.netmask, p->params[2]);
			memset(g_tftp_state.mac_addr, 0, 18);
			strcpy(g_tftp_state.mac_addr, p->params[3]);
			break;

		case PLRTOKEN_UDNLOAD_DONE:
			g_tftp_state.option = strtoud(p->params[0]);
			break;
#endif

		case PLRTOKEN_UUPLOAD_DONE:
		case PLRTOKEN_EUPLOAD_DONE:
			g_tftp_state.option = strtoud(p->params[0]);
			break;

		default:
			break;
	}

	_release_params(p);
}

/*
 *	author : edward
 *	date : 2013.08.23
 *	function : _recv_acks()
 *	parameter :
 *		IN token(e_send_token) - Board 에서 전송한 token
 *		OUT buf(char*) - success ack 의 파라미터에 대한 문자열
 *	return : e_receive_token
 *		수신한 ack 의 종류 ( success, fail ) 에 대한 열거형
 *	description :
 *		ack 를 수신할 때까지 무한 루프 수행 및 success ack 수신 후 protocol 파라미터 부분 문자열을 OUT 파라미터(buf) 에 복사한 후 반환
 *		fail ack 를 수신한 경우 caller 함수에서 token 을 재전송하도록 반환
 */
static e_receive_token _recv_acks( e_send_token token, char *buf )
{
	int len = 0;
	int i;

	while (1)	{
		len = readline_plrack_compare(buf);

		if (len < 0)
			continue;

		for(i = 0; i < MAX_RECIEVE_TOKEN; i++){
			if (check_valid_string(buf, receive_token_string[i]))
				goto __OUT;
		}
	}

__OUT:
		return i;
}

/*
 *	author : edward
 *	date : 2013.08.23
 *	function : recv_param_error_check()
 *	parameter :
 *		IN token(e_send_token) - Board 에서 전송한 token
 *		IN/OUT p(PPROTOCOL_PARARM) - 전송한 token 에 대한 PC 의 ack 의 파라미터를 분리하여 저장할 PROTOCOL_PARAM 구조체 포인터
 *	return : int
 *		parameter 가 유효한 경우 0 을 반환하고,
 *		그렇지 않은 경우 0 이외의 값을 반환함
 *	description :
 *		parameter 분리 및 유효성 체크
 */

static int recv_param_error_check( e_send_token token, char *buf, PPROTOCOL_PARAM p)
{
	char *index = NULL, *param_start = NULL;
	int num_of_params = 0;
	int recv_data_len = 0;
	int check_length = 0;
	char recv_buf[PLRTOKEN_BUFFER_SIZE] = {0};
	unsigned char crc_recv, crc_calc;
	int i = 0;

	/* get params & set params array */
	num_of_params = ACK_PARAM_TOTAL_COUNT(token);

	if (num_of_params == 0) {
		return 0;
	}

	// backup receive buffer
	sprintf (recv_buf, "%s", buf);
	//plr_debug ("receive param buff : %s\n", recv_buf);

	/* 0. CRC check */
	index = strtok(recv_buf, "/");

	if (is_numeric(index)) {
		crc_recv = strtoud(index);
		index += strlen(index) + 1;
		crc_calc = crc8((uchar *)index, strlen(index));
		if(crc_recv != crc_calc) {
			plr_err ("CRC is not correct!! crc param[%d] calculated crc[%d]\n", crc_recv, crc_calc);
			return -1;
		}
	}
	else {
		plr_err ("CRC param is not numeric!! receive data %s\n", buf);
		return -1;
	}

	/* 1. token string length */
	index = strtok(NULL, "/");

	if (is_numeric(index)) {
		recv_data_len = strtoud(index);
	}
	else {
		plr_err ("Token length param is not numeric!! receive data %s\n", buf);
		return -1;
	}

	/* 2. checking receive data length */
	param_start = index + strlen(index) + 1;
	check_length = strlen(param_start);
	if (recv_data_len != check_length) {
		plr_err ("Token length not matched!! length param %d, receive data length : %d\n", check_length, recv_data_len);
		return -1;
	}

	if(_split_params(param_start, p))	return -1;

	//plr_debug ("receive param count : %d\n", count);

	/* 3. checking receive param count */
	if (p->num_of_params < num_of_params) {
		plr_err ("param count not matched!! Need param count %d, receive param count : %d\n", num_of_params, p->num_of_params);
		goto __ERROR;
	}

	switch(token){
		case PLRTOKEN_BOOT_DONE:
		case PLRTOKEN_PREPARE_DONE:
#ifdef PLR_TFTP_FW_UP
		case PLRTOKEN_UTFTP_STATE:
		case PLRTOKEN_UDNLOAD_DONE:
#endif
			if(!is_numeric(p->params[0])){
				plr_err ("invalid string for numeric convsersion, receive param[0] : %s\n", p->params[0]);
				goto __ERROR;
			}
			break;

		case PLRTOKEN_TEST_INFO:
			{
				for(i = 1; i < p->num_of_params; i++){
					if(!is_numeric(p->params[i])){
						plr_err ("invalid string for numeric convsersion, receive param[%d] : %s\n", i, p->params[i]);
						goto __ERROR;
					}
				}

#if 0	// joys,2015.10.14
				if(strtoud(p->params[2]) + strtoud(p->params[3]) > g_plr_state.total_device_sector) {
					plr_err ("Test area is not correct : start sector [%s] length [%s]\n", p->params[2], p->params[3]);
					goto __ERROR;
				}
#endif
				if(strtoud(p->params[4]) && strtoud(p->params[5]) > 1024){
					plr_err ("cache flush cycle value must be between 0 and 1000, receive param : %s\n", p->params[5]);
					goto __ERROR;
				}

				if(strtoud(p->params[6]) && strtoud(p->params[7]) > 1024){
					plr_err ("packed flush cycle value must be between 0 and 1000, receive param : %s\n", p->params[7]);
					goto __ERROR;
				}
			}
			break;

		case PLRTOKEN_POWERLOSS_CONFIG:
		case PLRTOKEN_WRITE_CONFIG:
			{
				for(i = 0; i < p->num_of_params; i++){
					if(!is_numeric(p->params[i])){
						plr_err ("invalid string for numeric convsersion, receive param[%d] : %s\n", i, p->params[i]);
						goto __ERROR;
					}
				}
			}
			break;

		case PLRTOKEN_INIT_INFO:
			{
				g_plr_state.total_device_sector = get_dd_total_sector_count();
				for(i = 0; i < num_of_params; i++){
					if(!is_numeric(p->params[i])){
						plr_err ("invalid string for numeric convsersion, receive param[%d] : %s\n", i, p->params[i]);
						goto __ERROR;
					}
				}

#if 0	// joys,2015.10.14
				if(strtoud(p->params[0]) + strtoud(p->params[1]) > g_plr_state.total_device_sector) {
					printf("start[%s], length[%s], total sector[%d]\n",  p->params[0], p->params[1], g_plr_state.total_device_sector);
					plr_err ("Test area is not correct : start sector [%s] length [%s]\n", p->params[0], p->params[1]);
					goto __ERROR;
				}
#endif
				if(strtoud(p->params[4]) > 100) {
					plr_err ("filled ratio is not correct (0 ~ 100) [%s]\n", p->params[4]);
					goto __ERROR;
				}

			}
			break;

		case PLRTOKEN_UUPLOAD_DONE:
		case PLRTOKEN_EUPLOAD_DONE:
			if(!is_numeric(p->params[0])){
				plr_err ("invalid string for numeric convsersion, receive param[%d] : %s\n", 0, p->params[0]);
				goto __ERROR;
			}
			break;
		default:
			break;
	}

	return 0;

__ERROR:
	_release_params(p);

	return -1;
}

/*
 *	author : edward
 *	date : 2013.08.23
 *	function : do_recv_acks()
 *	parameter :
 *		IN token(e_send_token) - Board 에서 전송한 token
 *		IN/OUT p(PPROTOCOL_PARARM) - 전송한 token 에 대한 PC 의 ack 의 파라미터를 분리하여 저장할 PROTOCOL_PARAM 구조체 포인터
 *	return : int
 *		success ack 를 수신하고 parameter 가 유효한 경우 0 을 반환하고,
 *		그렇지 않은 경우 0 이외의 값을 반환함
 *	description :
 *		ack 를 수신하고 success ack 인 경우 parameter 분리 및 유효성 체크
 */

static int do_recv_acks( e_send_token token, PPROTOCOL_PARAM p )
{
	char buf[PLRTOKEN_BUFFER_SIZE] = {0,};

	//plr_debug("Waiting ack for %s ....\n", send_token_string[token]);

	e_receive_token tok = _recv_acks(token, buf);
	if(tok != PLRACK_SUCCESS)
	{
		// 2014.02.07	edward	intervening reset for hc board
		if(tok == PLRACK_RESET)	run_command("reset", 0);

		return 1;
	}

	return recv_param_error_check(token, buf, p);
}

/* ------------------------------------------------
* External Function
* ------------------------------------------------*/

int wait_getc(char c)
{
   do {
      readline(NULL);
      if (console_buffer[0] == c)
         return 1;
      else
         return 0;
   }while(1);
}

void send_token( e_send_token token, char *buf )
{
	int data_len = 0;
	PROTOCOL_PARAM param = {0};
	uchar crc = 0;
	char temp[255] = {0};

	// joys,2015.07.08
	// Clear pevious console garbage data
/*
	char t;
//	int i = 0;
	while(!getc(&t)) {
//		printf("%d%c", ++i, t);
	}
//	printf("%c\n", t);
*/
	if(buf)
	{
		data_len = strlen(buf);
		sprintf(temp, "%d/%s", data_len, buf);
		crc = crc8((uchar *)temp, strlen(temp));
	}

	while(1) {
		if (data_len) {
			printf("plrtoken %s %d/%d/%s\n", send_token_string[token], crc, data_len, buf);
		}
		else {
			printf("plrtoken %s\n", send_token_string[token]);
		}

		// setloopcnt, setaddress 는 ack 수신하지 않음
		if (token == PLRTOKEN_SET_LOOPCNT || token == PLRTOKEN_SET_ADDRESS)
			break;

		//plr_debug("Waiting ack %s....\n", send_token_string[token]);

		if(!do_recv_acks(token, &param)){
			set_recv_param_to_state(token, &param);
			break;
		}
	}
}

void send_token_param( e_send_token token, uint param )
{
	char send_buff[128] = {0};

	sprintf (send_buff, "%u/", param);
	send_token(token, send_buff);
}
