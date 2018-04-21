#include "controller.h"

#define LEDCODES_LEN	(sizeof(LED_decode_tab1)/sizeof(LED_decode_tab1[0]))
const led_bitmap *ledCodes = LED_decode_tab1;

/**
 * Source for the transpose algorithm:
   http://www.hackersdelight.org/hdcodetxt/transpose8.c.txt
 */
void transpose8rS64(unsigned char* A, unsigned char* B) {
	unsigned long long x = 0, t;
	int i;

	for (i = 0; i <= 7; i++)	// Load 8 bytes from the input
		x = x << 8 | A[i];	// array and pack them into x.

	t = (x ^ (x >> 7)) & 0x00AA00AA00AA00AALL;
	x = x ^ t ^ (t << 7);
	t = (x ^ (x >> 14)) & 0x0000CCCC0000CCCCLL;
	x = x ^ t ^ (t << 14);
	t = (x ^ (x >> 28)) & 0x00000000F0F0F0F0LL;
	x = x ^ t ^ (t << 28);

	memcpy(B, &x, sizeof(x));	// Store result into output array B.
}

static unsigned char char_to_mask(unsigned char ch)
{
	unsigned int index = 0;
	for (index = 0; index < LEDCODES_LEN; index++) {
		if (ledCodes[index].character == ch) {
			return ledCodes[index].bitmap;
		}
	}

	return 0;
}

size_t seg7_write_display_data(const struct fd628_display_data *data, unsigned short *raw_wdata, size_t sz)
{
	size_t i, len;
	char buffer[8];
	size_t status = sizeof(*data);

	if (sz < sizeof(unsigned char[7]))
		return 0;

	memset(raw_wdata, 0, sz);
	switch (data->mode) {
	case DISPLAY_MODE_CLOCK:
	case DISPLAY_MODE_PLAYBACK_TIME:
		raw_wdata[0] = data->colon_on ? ledDots[LED_DOT_SEC] : 0;
		if (data->mode == DISPLAY_MODE_PLAYBACK_TIME && data->time_date.hours == 0) {
			raw_wdata[1] = char_to_mask(data->time_date.minutes / 10);
			raw_wdata[2] = char_to_mask(data->time_date.minutes % 10);
			raw_wdata[3] = char_to_mask(data->time_date.seconds / 10);
			raw_wdata[4] = char_to_mask(data->time_date.seconds % 10);
		} else {
			raw_wdata[1] = char_to_mask(data->time_date.hours / 10);
			raw_wdata[2] = char_to_mask(data->time_date.hours % 10);
			raw_wdata[3] = char_to_mask(data->time_date.minutes / 10);
			raw_wdata[4] = char_to_mask(data->time_date.minutes % 10);
		}
		break;
	case DISPLAY_MODE_CHANNEL:
		len = scnprintf(buffer, sizeof(buffer), "%*d", 4, data->channel_data.channel % 10000);
		for (i = 0; i < len; i++)
			raw_wdata[i + 1] = char_to_mask(buffer[i]);
		break;
	case DISPLAY_MODE_TITLE:
		break;
	case DISPLAY_MODE_TEMPERATURE:
		len = scnprintf(buffer, sizeof(buffer), "%d%c", data->temperature % 1000, 0xB0); // ascii 176 = degree
		for (i = 0; i < len; i++)
			raw_wdata[i + 1] = char_to_mask(buffer[i]);
		break;
	default:
		status = 0;
		break;
	}

	return status;
}
