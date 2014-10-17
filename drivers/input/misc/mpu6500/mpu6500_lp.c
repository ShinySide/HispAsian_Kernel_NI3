#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/input.h>
#include <linux/input-polldev.h>
#include <linux/miscdevice.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/pm.h>
#include <linux/pm_runtime.h>
#include <linux/irq.h>
#include <linux/gpio.h>

#include <linux/mpu6500_input.h>
#include "mpu6500_lp.h"
#include "mpu6500_dmpkey.h"

#define CFG_FIFO_ON_EVENT       (2694)
#define CFG_ORIENT_IRQ_1        (2533)
#define CFG_DISPLAY_ORIENT_INT  (1706)
#define D_0_22                  (22+512)
#define D_1_74                  (256 + 74)
#define D_1_232                 (256 + 232)
#define D_1_250                 (256 + 250)
#define CFG_6                   (2756)
#define CFG_15                  (2731)
#define CFG_27                  (2745)

#define FCFG_1                  (1062)
#define FCFG_2                  (1066)

#define FCFG_3                  (1110)
#define FCFG_7                  (1076)

#define DMP_CODE_SIZE 3065

static const unsigned char dmpMemory[DMP_CODE_SIZE] = {
	/* bank # 0 */
	0x00, 0x00, 0x70, 0x00, 0x00, 0x00, 0x00, 0x24, 0x00, 0x00, 0x00, 0x02,
	    0x00, 0x03, 0x00, 0x00,
	0x00, 0x65, 0x00, 0x54, 0xff, 0xef, 0x00, 0x00, 0xfa, 0x80, 0x00, 0x0b,
	    0x12, 0x82, 0x00, 0x01,
	0x03, 0x0c, 0x30, 0xc3, 0x0e, 0x8c, 0x8c, 0xe9, 0x14, 0xd5, 0x40, 0x02,
	    0x13, 0x71, 0x0f, 0x8e,
	0x38, 0x83, 0xf8, 0x83, 0x30, 0x00, 0xf8, 0x83, 0x25, 0x8e, 0xf8, 0x83,
	    0x30, 0x00, 0xf8, 0x83,
	0x00, 0x00, 0x00, 0x01, 0x0f, 0xfe, 0xa9, 0xd6, 0x24, 0x00, 0x04, 0x00,
	    0x1a, 0x82, 0x79, 0xa1,
	0xff, 0xff, 0x00, 0x00, 0x00, 0x14, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00,
	    0x00, 0x12, 0x00, 0x02,
	0x00, 0x3e, 0x03, 0x30, 0x40, 0x00, 0x00, 0x00, 0x02, 0xca, 0xe3, 0x09,
	    0x3e, 0x80, 0x00, 0x00,
	0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00,
	    0x60, 0x00, 0x00, 0x00,
	0x00, 0x0c, 0x00, 0x00, 0x00, 0x0c, 0x18, 0x6e, 0x00, 0x00, 0x06, 0x92,
	    0x0a, 0x16, 0xc0, 0xdf,
	0xff, 0xff, 0x02, 0x56, 0xfd, 0x8c, 0xd3, 0x77, 0xff, 0xe1, 0xc4, 0x96,
	    0xe0, 0xc5, 0xbe, 0xaa,
	0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0x0b, 0x2b, 0x00, 0x00, 0x16, 0x57,
	    0x00, 0x00, 0x03, 0x59,
	0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1d, 0xfa, 0x00, 0x02, 0x6c, 0x1d,
	    0x00, 0x00, 0x00, 0x00,
	0x3f, 0xff, 0xdf, 0xeb, 0x00, 0x3e, 0xb3, 0xb6, 0x00, 0x0d, 0x22, 0x78,
	    0x00, 0x00, 0x2f, 0x3c,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x19, 0x42, 0xb5, 0x00, 0x00, 0x39, 0xa2,
	    0x00, 0x00, 0xb3, 0x65,
	0x3f, 0x84, 0x05, 0x99, 0xf2, 0xbc, 0x3a, 0x39, 0xf1, 0x53, 0x7a, 0x76,
	    0xd9, 0x0e, 0x9f, 0xc8,
	0x00, 0x00, 0x00, 0x00, 0xf2, 0xbc, 0x3a, 0x39, 0xf1, 0x53, 0x7a, 0x76,
	    0xd9, 0x0e, 0x9f, 0xc8,
	/* bank # 1 */
	0x10, 0x00, 0x00, 0x00, 0x10, 0x00, 0xfa, 0x92, 0x10, 0x00, 0x22, 0x5e,
	    0x00, 0x0d, 0x22, 0x9f,
	0x00, 0x01, 0x00, 0x00, 0x00, 0x32, 0x00, 0x00, 0xff, 0x46, 0x00, 0x00,
	    0x63, 0xd4, 0x00, 0x00,
	0x10, 0x00, 0x00, 0x00, 0x04, 0xd6, 0x00, 0x00, 0x04, 0xcc, 0x00, 0x00,
	    0x04, 0xcc, 0x00, 0x00,
	0x00, 0x00, 0x10, 0x72, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x00,
	    0x00, 0x00, 0x00, 0x00,
	0x00, 0x06, 0x00, 0x02, 0x00, 0x05, 0x00, 0x07, 0x00, 0x00, 0x00, 0x00,
	    0x00, 0x64, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05, 0x00, 0x05, 0x00, 0x64,
	    0x00, 0x20, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x00,
	    0x00, 0x00, 0x03, 0x00,
	0x00, 0x00, 0x00, 0x32, 0xf8, 0x98, 0x00, 0x00, 0xff, 0x65, 0x00, 0x00,
	    0x83, 0x0f, 0x00, 0x00,
	0xff, 0x9b, 0xfc, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	    0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	    0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	    0x00, 0x00, 0x10, 0x00,
	0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0xb2, 0x6a,
	    0x00, 0x02, 0x00, 0x00,
	0x00, 0x01, 0xfb, 0x83, 0x00, 0x68, 0x00, 0x00, 0x00, 0xd9, 0xfc, 0x00,
	    0x7c, 0xf1, 0xff, 0x83,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x65, 0x00, 0x00, 0x00, 0x64, 0x03, 0xe8,
	    0x00, 0x64, 0x00, 0x28,
	0x00, 0x00, 0x00, 0x25, 0x00, 0x00, 0x00, 0x00, 0x16, 0xa0, 0x00, 0x00,
	    0x00, 0x00, 0x10, 0x00,
	0x00, 0x00, 0x10, 0x00, 0x00, 0x2f, 0x00, 0x00, 0x00, 0x00, 0x01, 0xf4,
	    0x00, 0x00, 0x10, 0x00,
	/* bank # 2 */
	0x00, 0x28, 0x00, 0x00, 0xff, 0xff, 0x45, 0x81, 0xff, 0xff, 0xfa, 0x72,
	    0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x01, 0x00, 0x44, 0x00, 0x05, 0x00, 0x01, 0x00, 0x00,
	    0x00, 0x01, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0x00,
	    0x00, 0x00, 0x00, 0x14,
	0x00, 0x00, 0x25, 0x4d, 0x00, 0x2f, 0x70, 0x6d, 0x00, 0x00, 0x05, 0xae,
	    0x00, 0x0c, 0x02, 0xd0,
	0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00,
	    0x00, 0x00, 0x00, 0x00,
	0x00, 0x1b, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	    0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	    0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	    0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	    0x00, 0x00, 0x00, 0x00,
	0xc0, 0x00, 0x00, 0x01, 0x3f, 0xff, 0xff, 0xff, 0x40, 0x00, 0x00, 0x00,
	    0x30, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	    0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	    0x00, 0x47, 0x78, 0xa2,
	0x00, 0x1b, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	    0x00, 0x0e, 0x00, 0x0e,
	0x00, 0x00, 0x0a, 0xc7, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x32,
	    0xff, 0xff, 0xff, 0x9c,
	0x00, 0x00, 0x0b, 0x2b, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x01,
	    0x00, 0x00, 0x00, 0x64,
	0xff, 0xe5, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	    0x00, 0x00, 0x00, 0x00,
	/* bank # 3 */
	0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00,
	    0x00, 0x00, 0x00, 0x00,
	0x00, 0x01, 0x80, 0x00, 0x00, 0x01, 0x80, 0x00, 0x00, 0x01, 0x80, 0x00,
	    0x00, 0x24, 0x26, 0xd3,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0x00, 0x10,
	    0x00, 0x96, 0x00, 0x3c,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	    0x00, 0x00, 0x00, 0x00,
	0x0c, 0x0a, 0x4e, 0x68, 0xcd, 0xcf, 0x77, 0x09, 0x50, 0x16, 0x67, 0x59,
	    0xc6, 0x19, 0xce, 0x82,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	    0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x17, 0xd7, 0x84, 0x00,
	    0x03, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc7, 0x93, 0x8f, 0x9d,
	    0x1e, 0x1b, 0x1c, 0x19,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	    0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x03, 0x18, 0x85,
	    0x00, 0x00, 0x40, 0x00,
	0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00,
	    0x00, 0x00, 0x00, 0x00,
	0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	    0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	    0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	    0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x67, 0x7d, 0xdf, 0x7e, 0x72, 0x90, 0x2e, 0x55,
	    0x4c, 0xf6, 0xe6, 0x88,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	    0x00, 0x00, 0x00, 0x00,

	/* bank # 4 */
	0xd8, 0xdc, 0xb4, 0xb8, 0xb0, 0xd8, 0xb9, 0xab, 0xf3, 0xf8, 0xfa, 0xb3,
	    0xb7, 0xbb, 0x8e, 0x9e,
	0xae, 0xf1, 0x32, 0xf5, 0x1b, 0xf1, 0xb4, 0xb8, 0xb0, 0x80, 0x97, 0xf1,
	    0xa9, 0xdf, 0xdf, 0xdf,
	0xaa, 0xdf, 0xdf, 0xdf, 0xf2, 0xaa, 0xc5, 0xcd, 0xc7, 0xa9, 0x0c, 0xc9,
	    0x2c, 0x97, 0x97, 0x97,
	0x97, 0xf1, 0xa9, 0x89, 0x26, 0x46, 0x66, 0xb0, 0xb4, 0xba, 0x80, 0xac,
	    0xde, 0xf2, 0xca, 0xf1,
	0xb2, 0x8c, 0x02, 0xa9, 0xb6, 0x98, 0x00, 0x89, 0x0e, 0x16, 0x1e, 0xb8,
	    0xa9, 0xb4, 0x99, 0x2c,
	0x54, 0x7c, 0xb0, 0x8a, 0xa8, 0x96, 0x36, 0x56, 0x76, 0xf1, 0xba, 0xa3,
	    0xb4, 0xb2, 0x80, 0xc0,
	0xb8, 0xa8, 0x97, 0x11, 0xb2, 0x83, 0x98, 0xba, 0xa3, 0xf0, 0x24, 0x08,
	    0x44, 0x10, 0x64, 0x18,
	0xb2, 0xb9, 0xb4, 0x98, 0x83, 0xf1, 0xa3, 0x29, 0x55, 0x7d, 0xba, 0xb5,
	    0xb1, 0xa3, 0x83, 0x93,
	0xf0, 0x00, 0x28, 0x50, 0xf5, 0xb2, 0xb6, 0xaa, 0x83, 0x93, 0x28, 0x54,
	    0x7c, 0xb9, 0xf1, 0xa3,
	0x82, 0x93, 0x61, 0xba, 0xa2, 0xda, 0xde, 0xdf, 0xdb, 0x8b, 0x9a, 0xb9,
	    0xae, 0xf5, 0x60, 0x68,
	0x70, 0xf1, 0xda, 0xba, 0xa2, 0xdf, 0xd9, 0xa2, 0xfa, 0xb9, 0xa3, 0x82,
	    0x92, 0xdb, 0x31, 0xba,
	0xa2, 0xd9, 0xf8, 0xdf, 0xa4, 0x83, 0xc2, 0xc5, 0xc7, 0x85, 0xc1, 0xb8,
	    0xa2, 0xdf, 0xdf, 0xdf,
	0xba, 0xa0, 0xdf, 0xdf, 0xdf, 0xd8, 0xd8, 0xf1, 0xb8, 0xa8, 0xb3, 0x8d,
	    0xb4, 0x98, 0x0d, 0x35,
	0x5d, 0xb8, 0xaa, 0x98, 0xb0, 0x87, 0x2d, 0x35, 0x3d, 0xb2, 0xb6, 0xba,
	    0xaf, 0x8c, 0x96, 0x19,
	0x8f, 0x9f, 0xa7, 0x0e, 0x16, 0x1e, 0xb4, 0x9a, 0xb8, 0xaa, 0x87, 0x2c,
	    0x54, 0x7c, 0xd8, 0xb8,
	0xb4, 0xb0, 0xf1, 0x99, 0x82, 0xa8, 0x2d, 0x55, 0x7d, 0x98, 0xa8, 0x0e,
	    0x16, 0x1e, 0xa2, 0x2c,
	/* bank # 5 */
	0x54, 0x7c, 0xa8, 0x92, 0xf5, 0x2c, 0x54, 0x88, 0x98, 0xf1, 0x35, 0xd9,
	    0xf4, 0x18, 0xd8, 0xf1,
	0xa2, 0xd0, 0xf8, 0xf9, 0xa8, 0x84, 0xd9, 0xc7, 0xdf, 0xf8, 0xf8, 0x83,
	    0xc5, 0xda, 0xdf, 0x69,
	0xdf, 0x83, 0xc1, 0xd8, 0xf4, 0x01, 0x14, 0xf1, 0xa8, 0x82, 0x4e, 0xa8,
	    0x84, 0xf3, 0x11, 0xd1,
	0x82, 0xf5, 0xd9, 0x92, 0x28, 0x97, 0x88, 0xf1, 0x09, 0xf4, 0x1c, 0x1c,
	    0xd8, 0x84, 0xa8, 0xf3,
	0xc0, 0xf9, 0xd1, 0xd9, 0x97, 0x82, 0xf1, 0x29, 0xf4, 0x0d, 0xd8, 0xf3,
	    0xf9, 0xf9, 0xd1, 0xd9,
	0x82, 0xf4, 0xc2, 0x03, 0xd8, 0xde, 0xdf, 0x1a, 0xd8, 0xf1, 0xa2, 0xfa,
	    0xf9, 0xa8, 0x84, 0x98,
	0xd9, 0xc7, 0xdf, 0xf8, 0xf8, 0xf8, 0x83, 0xc7, 0xda, 0xdf, 0x69, 0xdf,
	    0xf8, 0x83, 0xc3, 0xd8,
	0xf4, 0x01, 0x14, 0xf1, 0x98, 0xa8, 0x82, 0x2e, 0xa8, 0x84, 0xf3, 0x11,
	    0xd1, 0x82, 0xf5, 0xd9,
	0x92, 0x50, 0x97, 0x88, 0xf1, 0x09, 0xf4, 0x1c, 0xd8, 0x84, 0xa8, 0xf3,
	    0xc0, 0xf8, 0xf9, 0xd1,
	0xd9, 0x97, 0x82, 0xf1, 0x49, 0xf4, 0x0d, 0xd8, 0xf3, 0xf9, 0xf9, 0xd1,
	    0xd9, 0x82, 0xf4, 0xc4,
	0x03, 0xd8, 0xde, 0xdf, 0xd8, 0xf1, 0xad, 0x88, 0x98, 0xcc, 0xa8, 0x09,
	    0xf9, 0xd9, 0x82, 0x92,
	0xa4, 0xf0, 0x2c, 0x50, 0x78, 0xa8, 0x82, 0xf5, 0x7c, 0xf1, 0x88, 0x3a,
	    0xcf, 0x94, 0x4a, 0x6e,
	0x98, 0xdb, 0x69, 0x31, 0xd9, 0x84, 0xc4, 0xcd, 0xfc, 0xdb, 0x6d, 0xd9,
	    0xa8, 0xfc, 0xdb, 0x25,
	0xda, 0xad, 0xf2, 0xde, 0xf9, 0xd8, 0xf2, 0xa5, 0xf8, 0x8d, 0x94, 0xd1,
	    0xda, 0xf4, 0x19, 0xd8,
	0xa8, 0xf2, 0x05, 0xd1, 0xa4, 0xda, 0xc0, 0xa5, 0xf7, 0xde, 0xf9, 0xd8,
	    0xa5, 0xf8, 0x85, 0x95,
	0x18, 0xdf, 0xf1, 0xad, 0x8e, 0xc3, 0xc5, 0xc7, 0xd8, 0xf2, 0xa5, 0xf8,
	    0xd1, 0xd9, 0xf1, 0xad,
	/* bank # 6 */
	0x8f, 0xc3, 0xc5, 0xc7, 0xd8, 0xa5, 0xf2, 0xf9, 0xf9, 0xa8, 0x8d, 0x9d,
	    0xf0, 0x28, 0x50, 0x78,
	0x84, 0x98, 0xf1, 0x26, 0x84, 0x95, 0x2d, 0x88, 0x98, 0x11, 0x52, 0x87,
	    0x12, 0x12, 0x88, 0x31,
	0xf9, 0xd9, 0xf1, 0x8d, 0x9d, 0x7a, 0xf5, 0x7c, 0xf1, 0x88, 0x7a, 0x98,
	    0x45, 0x8e, 0x0e, 0xf5,
	0x82, 0x92, 0x7c, 0x88, 0xf1, 0x5a, 0x98, 0x5a, 0x82, 0x92, 0x7e, 0x88,
	    0x94, 0x69, 0x98, 0x1e,
	0x11, 0x08, 0xd0, 0xf5, 0x04, 0xf1, 0x1e, 0x97, 0x02, 0x02, 0x98, 0x36,
	    0x25, 0xdb, 0xf9, 0xd9,
	0xf0, 0x8d, 0x92, 0xa8, 0x49, 0x30, 0x2c, 0x50, 0x8e, 0xc9, 0x88, 0x98,
	    0x34, 0xf5, 0x04, 0xf1,
	0x61, 0xdb, 0xf9, 0xd9, 0xf2, 0xa5, 0xf8, 0xdb, 0xf9, 0xd9, 0xf3, 0xfa,
	    0xd8, 0xf2, 0xa5, 0xf8,
	0xf9, 0xd9, 0xf1, 0xaf, 0x8e, 0xc3, 0xc5, 0xc7, 0xae, 0x82, 0xc3, 0xc5,
	    0xc7, 0xd8, 0xa4, 0xf2,
	0xf8, 0xd1, 0xa5, 0xf3, 0xd9, 0xde, 0xf9, 0xdf, 0xd8, 0xa4, 0xf2, 0xf9,
	    0xa5, 0xf8, 0xf8, 0xd1,
	0xda, 0xf9, 0xf9, 0xf4, 0x18, 0xd8, 0xf2, 0xf9, 0xf9, 0xf3, 0xfb, 0xf9,
	    0xd1, 0xda, 0xf4, 0x1d,
	0xd9, 0xf3, 0xa4, 0x84, 0xc8, 0xa8, 0x9f, 0x01, 0xdb, 0xd1, 0xd8, 0xf4,
	    0x10, 0x1c, 0xd8, 0xbb,
	0xaf, 0xd0, 0xf2, 0xde, 0xf8, 0xf8, 0xf8, 0xf8, 0xf8, 0xf8, 0xf8, 0xf8,
	    0xb8, 0xd8, 0xf3, 0xaf,
	0x84, 0xc0, 0xa5, 0xfa, 0xf8, 0xf2, 0x85, 0xa5, 0xc6, 0xd8, 0xd8, 0xf2,
	    0xf9, 0xf6, 0xb5, 0xb9,
	0xb0, 0x8a, 0x95, 0xa3, 0xde, 0x3c, 0xa3, 0xd9, 0xf8, 0xd8, 0x5c, 0xa3,
	    0xd9, 0xf8, 0xd8, 0x7c,
	0xa3, 0xd9, 0xf8, 0xd8, 0xf8, 0xf9, 0xd1, 0xa5, 0xd9, 0xdf, 0xda, 0xfa,
	    0xd8, 0xb1, 0x85, 0x30,
	0xf7, 0xd9, 0xde, 0xd8, 0xf8, 0x30, 0xad, 0xda, 0xde, 0xd8, 0xf2, 0xb4,
	    0x8c, 0x99, 0xa3, 0x2d,
	/* bank # 7 */
	0x55, 0x7d, 0xa0, 0x83, 0xdf, 0xdf, 0xdf, 0xb5, 0x91, 0xa0, 0xf6, 0x29,
	    0xd9, 0xfb, 0xd8, 0xa0,
	0xfc, 0x29, 0xd9, 0xfa, 0xd8, 0xa0, 0xd0, 0x51, 0xd9, 0xf8, 0xd8, 0xfc,
	    0x51, 0xd9, 0xf9, 0xd8,
	0x79, 0xd9, 0xfb, 0xd8, 0xa0, 0xd0, 0xfc, 0x79, 0xd9, 0xfa, 0xd8, 0xa1,
	    0xf9, 0xf9, 0xf9, 0xf9,
	0xf9, 0xa0, 0xda, 0xdf, 0xdf, 0xdf, 0xd8, 0xa1, 0xf8, 0xf8, 0xf8, 0xf8,
	    0xf8, 0xac, 0xde, 0xf8,
	0xad, 0xde, 0x83, 0x93, 0xac, 0x2c, 0x54, 0x7c, 0xf1, 0xa8, 0xdf, 0xdf,
	    0xdf, 0xf6, 0x9d, 0x2c,
	0xda, 0xa0, 0xdf, 0xd9, 0xfa, 0xdb, 0x2d, 0xf8, 0xd8, 0xa8, 0x50, 0xda,
	    0xa0, 0xd0, 0xde, 0xd9,
	0xd0, 0xf8, 0xf8, 0xf8, 0xdb, 0x55, 0xf8, 0xd8, 0xa8, 0x78, 0xda, 0xa0,
	    0xd0, 0xdf, 0xd9, 0xd0,
	0xfa, 0xf8, 0xf8, 0xf8, 0xf8, 0xdb, 0x7d, 0xf8, 0xd8, 0x9c, 0xa8, 0x8c,
	    0xf5, 0x30, 0xdb, 0x38,
	0xd9, 0xd0, 0xde, 0xdf, 0xa0, 0xd0, 0xde, 0xdf, 0xd8, 0xa8, 0x48, 0xdb,
	    0x58, 0xd9, 0xdf, 0xd0,
	0xde, 0xa0, 0xdf, 0xd0, 0xde, 0xd8, 0xa8, 0x68, 0xdb, 0x70, 0xd9, 0xdf,
	    0xdf, 0xa0, 0xdf, 0xdf,
	0xd8, 0xf1, 0xa8, 0x88, 0x90, 0x2c, 0x54, 0x7c, 0x98, 0xa8, 0xd0, 0x5c,
	    0x38, 0xd1, 0xda, 0xf2,
	0xae, 0x8c, 0xdf, 0xf9, 0xd8, 0xb0, 0x87, 0xa8, 0xc1, 0xc1, 0xb1, 0x88,
	    0xa8, 0xc6, 0xf9, 0xf9,
	0xda, 0x36, 0xd8, 0xa8, 0xf9, 0xda, 0x36, 0xd8, 0xa8, 0xf9, 0xda, 0x36,
	    0xd8, 0xa8, 0xf9, 0xda,
	0x36, 0xd8, 0xa8, 0xf9, 0xda, 0x36, 0xd8, 0xf7, 0x8d, 0x9d, 0xad, 0xf8,
	    0x18, 0xda, 0xf2, 0xae,
	0xdf, 0xd8, 0xf7, 0xad, 0xfa, 0x30, 0xd9, 0xa4, 0xde, 0xf9, 0xd8, 0xf2,
	    0xae, 0xde, 0xfa, 0xf9,
	0x83, 0xa7, 0xd9, 0xc3, 0xc5, 0xc7, 0xf1, 0x88, 0x9b, 0xa7, 0x7a, 0xad,
	    0xf7, 0xde, 0xdf, 0xa4,
	/* bank # 8 */
	0xf8, 0x84, 0x94, 0x08, 0xa7, 0x97, 0xf3, 0x00, 0xae, 0xf2, 0x98, 0x19,
	    0xa4, 0x88, 0xc6, 0xa3,
	0x94, 0x88, 0xf6, 0x32, 0xdf, 0xf2, 0x83, 0x93, 0xdb, 0x09, 0xd9, 0xf2,
	    0xaa, 0xdf, 0xd8, 0xd8,
	0xae, 0xf8, 0xf9, 0xd1, 0xda, 0xf3, 0xa4, 0xde, 0xa7, 0xf1, 0x88, 0x9b,
	    0x7a, 0xd8, 0xf3, 0x84,
	0x94, 0xae, 0x19, 0xf9, 0xda, 0xaa, 0xf1, 0xdf, 0xd8, 0xa8, 0x81, 0xc0,
	    0xc3, 0xc5, 0xc7, 0xa3,
	0x92, 0x83, 0xf6, 0x28, 0xad, 0xde, 0xd9, 0xf8, 0xd8, 0xa3, 0x50, 0xad,
	    0xd9, 0xf8, 0xd8, 0xa3,
	0x78, 0xad, 0xd9, 0xf8, 0xd8, 0xf8, 0xf9, 0xd1, 0xa1, 0xda, 0xde, 0xc3,
	    0xc5, 0xc7, 0xd8, 0xa1,
	0x81, 0x94, 0xf8, 0x18, 0xf2, 0xb0, 0x89, 0xac, 0xc3, 0xc5, 0xc7, 0xf1,
	    0xd8, 0xb8, 0xb4, 0xb0,
	0x97, 0x86, 0xa8, 0x31, 0x9b, 0x06, 0x99, 0x07, 0xab, 0x97, 0x28, 0x88,
	    0x9b, 0xf0, 0x0c, 0x20,
	0x14, 0x40, 0xb0, 0xb4, 0xb8, 0xf0, 0xa8, 0x8a, 0x9a, 0x28, 0x50, 0x78,
	    0xb7, 0x9b, 0xa8, 0x29,
	0x51, 0x79, 0x24, 0x70, 0x59, 0x44, 0x69, 0x38, 0x64, 0x48, 0x31, 0xf1,
	    0xbb, 0xab, 0x88, 0x00,
	0x2c, 0x54, 0x7c, 0xf0, 0xb3, 0x8b, 0xb8, 0xa8, 0x04, 0x28, 0x50, 0x78,
	    0xf1, 0xb0, 0x88, 0xb4,
	0x97, 0x26, 0xa8, 0x59, 0x98, 0xbb, 0xab, 0xb3, 0x8b, 0x02, 0x26, 0x46,
	    0x66, 0xb0, 0xb8, 0xf0,
	0x8a, 0x9c, 0xa8, 0x29, 0x51, 0x79, 0x8b, 0x29, 0x51, 0x79, 0x8a, 0x24,
	    0x70, 0x59, 0x8b, 0x20,
	0x58, 0x71, 0x8a, 0x44, 0x69, 0x38, 0x8b, 0x39, 0x40, 0x68, 0x8a, 0x64,
	    0x48, 0x31, 0x8b, 0x30,
	0x49, 0x60, 0x88, 0xf1, 0xac, 0x00, 0x2c, 0x54, 0x7c, 0xf0, 0x8c, 0xa8,
	    0x04, 0x28, 0x50, 0x78,
	0xf1, 0x88, 0x97, 0x26, 0xa8, 0x59, 0x98, 0xac, 0x8c, 0x02, 0x26, 0x46,
	    0x66, 0xf0, 0x89, 0x9c,
	/* bank # 9 */
	0xa8, 0x29, 0x51, 0x79, 0x24, 0x70, 0x59, 0x44, 0x69, 0x38, 0x64, 0x48,
	    0x31, 0xa9, 0x88, 0x09,
	0x20, 0x59, 0x70, 0xab, 0x11, 0x38, 0x40, 0x69, 0xa8, 0x19, 0x31, 0x48,
	    0x60, 0x8c, 0xa8, 0x3c,
	0x41, 0x5c, 0x20, 0x7c, 0x00, 0xf1, 0x87, 0x98, 0x19, 0x86, 0xa8, 0x6e,
	    0x76, 0x7e, 0xa9, 0x99,
	0x88, 0x2d, 0x55, 0x7d, 0xf5, 0xb0, 0xb4, 0xb8, 0x88, 0x98, 0xad, 0x2c,
	    0x54, 0x7c, 0xb5, 0xb9,
	0x9e, 0xa3, 0xdf, 0xdf, 0xdf, 0xf6, 0xa3, 0x30, 0xd9, 0xfa, 0xdb, 0x35,
	    0xf8, 0xd8, 0xa3, 0x50,
	0xd0, 0xd9, 0xf8, 0xdb, 0x55, 0xf8, 0xd8, 0xa3, 0x70, 0xd0, 0xd9, 0xfa,
	    0xdb, 0x75, 0xf8, 0xd8,
	0xa3, 0xb4, 0x8d, 0x9d, 0x30, 0xdb, 0x38, 0xd9, 0xd0, 0xde, 0xdf, 0xd8,
	    0xa3, 0x48, 0xdb, 0x58,
	0xd9, 0xdf, 0xd0, 0xde, 0xd8, 0xa3, 0x68, 0xdb, 0x70, 0xd9, 0xdf, 0xdf,
	    0xd8, 0xf1, 0xa3, 0xb1,
	0xb5, 0x82, 0xc0, 0x83, 0x93, 0x0e, 0x0a, 0x0a, 0x16, 0x12, 0x1e, 0x58,
	    0x38, 0x1a, 0xa3, 0x8f,
	0x7c, 0x83, 0x0e, 0xa3, 0x12, 0x86, 0x61, 0xd1, 0xd9, 0xc7, 0xc7, 0xd8,
	    0xa3, 0x6d, 0xd1, 0xd9,
	0xc7, 0xd8, 0xa3, 0x71, 0xd1, 0xa6, 0xd9, 0xc5, 0xda, 0xdf, 0xd8, 0x9f,
	    0x83, 0xf3, 0xa3, 0x65,
	0xd1, 0xaf, 0xc6, 0xd9, 0xfa, 0xda, 0xdf, 0xd8, 0xa3, 0x8f, 0xdf, 0x1d,
	    0xd1, 0xa3, 0xd9, 0xfa,
	0xd8, 0xa3, 0x31, 0xda, 0xfa, 0xd9, 0xaf, 0xdf, 0xd8, 0xa3, 0xfa, 0xf9,
	    0xd1, 0xd9, 0xaf, 0xd0,
	0x96, 0xc1, 0xae, 0xd0, 0x0c, 0x8e, 0x94, 0xa3, 0xf7, 0x72, 0xdf, 0xf3,
	    0x83, 0x93, 0xdb, 0x09,
	0xd9, 0xf2, 0xaa, 0xd0, 0xde, 0xd8, 0xd8, 0xd8, 0xf2, 0xab, 0xf8, 0xf9,
	    0xd9, 0xb0, 0x87, 0xc4,
	0xaa, 0xf1, 0xdf, 0xdf, 0xbb, 0xaf, 0xdf, 0xdf, 0xb9, 0xd8, 0xb1, 0xf1,
	    0xa3, 0x97, 0x8e, 0x60,
	/* bank # 10 */
	0xdf, 0xb0, 0x84, 0xf2, 0xc8, 0x93, 0x85, 0xf1, 0x4a, 0xb1, 0x83, 0xa3,
	    0x08, 0xb5, 0x83, 0x9a,
	0x08, 0x10, 0xb7, 0x9f, 0x10, 0xb5, 0xd8, 0xf1, 0xb0, 0xba, 0xae, 0xb0,
	    0x8a, 0xc2, 0xb2, 0xb6,
	0x8e, 0x9e, 0xf1, 0xfb, 0xd9, 0xf4, 0x1d, 0xd8, 0xf9, 0xd9, 0x0c, 0xf1,
	    0xd8, 0xf8, 0xf8, 0xad,
	0x61, 0xd9, 0xae, 0xfb, 0xd8, 0xf4, 0x0c, 0xf1, 0xd8, 0xf8, 0xf8, 0xad,
	    0x19, 0xd9, 0xae, 0xfb,
	0xdf, 0xd8, 0xf4, 0x16, 0xf1, 0xd8, 0xf8, 0xad, 0x8d, 0x61, 0xd9, 0xf4,
	    0xf4, 0xac, 0xf5, 0x9c,
	0x9c, 0x8d, 0xdf, 0x2b, 0xba, 0xb6, 0xae, 0xfa, 0xf8, 0xf4, 0x0b, 0xd8,
	    0xf1, 0xae, 0xd0, 0xf8,
	0xad, 0x51, 0xda, 0xae, 0xfa, 0xf8, 0xf1, 0xd8, 0xb9, 0xb1, 0xb6, 0xa3,
	    0x83, 0x9c, 0x08, 0xb9,
	0xb1, 0x83, 0x9a, 0xb5, 0xaa, 0xc0, 0xfd, 0x30, 0x83, 0xb7, 0x9f, 0x10,
	    0xb5, 0x8b, 0x93, 0xf2,
	0x02, 0x02, 0xd1, 0xab, 0xda, 0xde, 0xd8, 0xd8, 0xb1, 0xb9, 0xf3, 0x8b,
	    0xa3, 0x91, 0xb6, 0x09,
	0xb4, 0xd9, 0xab, 0xde, 0xb0, 0x87, 0x9c, 0xb9, 0xa3, 0xdd, 0xf1, 0xb3,
	    0x8b, 0x8b, 0x8b, 0x8b,
	0x8b, 0xb0, 0x87, 0xa3, 0xa3, 0xa3, 0xa3, 0xb4, 0x90, 0x80, 0xf2, 0xa3,
	    0xa3, 0xa3, 0xa3, 0xa3,
	0xa3, 0xa3, 0xa3, 0xa3, 0xa3, 0xf1, 0x87, 0xb5, 0x9a, 0xa3, 0xf3, 0x9b,
	    0xa3, 0xa3, 0xdc, 0xba,
	0xac, 0xdf, 0xb9, 0xa3, 0xa3, 0xa3, 0xa3, 0xa3, 0xa3, 0xa3, 0xa3, 0xa3,
	    0xa3, 0xa3, 0xa3, 0xa3,
	0xa3, 0xa3, 0xa3, 0xd8, 0xd8, 0xd8, 0xbb, 0xb3, 0xb7, 0xf1, 0xaa, 0xf9,
	    0xda, 0xff, 0xd9, 0x80,
	0x9a, 0xaa, 0x28, 0xb4, 0x80, 0x98, 0xa7, 0x20, 0xb7, 0x97, 0x87, 0xa8,
	    0x66, 0x88, 0xf0, 0x79,
	0x51, 0xf1, 0x90, 0x2c, 0x87, 0x0c, 0xa7, 0x81, 0x97, 0x62, 0x93, 0xf0,
	    0x71, 0x71, 0x60, 0x85,
	/* bank # 11 */
	0x94, 0x01, 0x29, 0x51, 0x79, 0x90, 0xa5, 0xf1, 0x28, 0x4c, 0x6c, 0x87,
	    0x0c, 0x95, 0x18, 0x85,
	0x78, 0xa3, 0x83, 0x90, 0x28, 0x4c, 0x6c, 0x88, 0x6c, 0xd8, 0xf3, 0xa2,
	    0x82, 0x00, 0xf2, 0x10,
	0xa8, 0x92, 0x19, 0x80, 0xa2, 0xf2, 0xd9, 0x26, 0xd8, 0xf1, 0x88, 0xa8,
	    0x4d, 0xd9, 0x48, 0xd8,
	0x96, 0xa8, 0x39, 0x80, 0xd9, 0x3c, 0xd8, 0x95, 0x80, 0xa8, 0x39, 0xa6,
	    0x86, 0x98, 0xd9, 0x2c,
	0xda, 0x87, 0xa7, 0x2c, 0xd8, 0xa8, 0x89, 0x95, 0x19, 0xa9, 0x80, 0xd9,
	    0x38, 0xd8, 0xa8, 0x89,
	0x39, 0xa9, 0x80, 0xda, 0x3c, 0xd8, 0xa8, 0x2e, 0xa8, 0x39, 0x90, 0xd9,
	    0x0c, 0xd8, 0xa8, 0x95,
	0x31, 0x98, 0xd9, 0x0c, 0xd8, 0xa8, 0x09, 0xd9, 0xff, 0xd8, 0x01, 0xda,
	    0xff, 0xd8, 0x95, 0x39,
	0xa9, 0xda, 0x26, 0xff, 0xd8, 0x90, 0xa8, 0x0d, 0x89, 0x99, 0xa8, 0x10,
	    0x80, 0x98, 0x21, 0xda,
	0x2e, 0xd8, 0x89, 0x99, 0xa8, 0x31, 0x80, 0xda, 0x2e, 0xd8, 0xa8, 0x86,
	    0x96, 0x31, 0x80, 0xda,
	0x2e, 0xd8, 0xa8, 0x87, 0x31, 0x80, 0xda, 0x2e, 0xd8, 0xa8, 0x82, 0x92,
	    0xf3, 0x41, 0x80, 0xf1,
	0xd9, 0x2e, 0xd8, 0xa8, 0x82, 0xf3, 0x19, 0x80, 0xf1, 0xd9, 0x2e, 0xd8,
	    0x82, 0xac, 0xf3, 0xc0,
	0xa2, 0x80, 0x22, 0xf1, 0xa6, 0x2e, 0xa7, 0x2e, 0xa9, 0x22, 0x98, 0xa8,
	    0x29, 0xda, 0xac, 0xde,
	0xff, 0xd8, 0xa2, 0xf2, 0x2a, 0xf1, 0xa9, 0x2e, 0x82, 0x92, 0xa8, 0xf2,
	    0x31, 0x80, 0xa6, 0x96,
	0xf1, 0xd9, 0x00, 0xac, 0x8c, 0x9c, 0x0c, 0x30, 0xac, 0xde, 0xd0, 0xde,
	    0xff, 0xd8, 0x8c, 0x9c,
	0xac, 0xd0, 0x10, 0xac, 0xde, 0x80, 0x92, 0xa2, 0xf2, 0x4c, 0x82, 0xa8,
	    0xf1, 0xca, 0xf2, 0x35,
	0xf1, 0x96, 0x88, 0xa6, 0xd9, 0x00, 0xd8, 0xf1, 0xff
};

#define LOG_RESULT_LOCATION(x) {\
	printk(KERN_ERR "%s:%s:%d result=%d\n",\
	__FILE__, __func__, __LINE__, x);\
}


#define CHECK_RESULT(x) {\
	result = x;\
	if (unlikely(result))\
		LOG_RESULT_LOCATION(result);\
}


static int mpu6500_lp_load_firmware(struct i2c_client *i2c_client)
{

	int bank, write_size;
	int result;
	unsigned short memaddr;

	int size = DMP_CODE_SIZE;
	const unsigned char *data = (unsigned char *)dmpMemory;

	/* Write and verify memory */
	for (bank = 0; size > 0;
		bank++, size -= write_size, data += write_size) {
		if (size > MPU_MEM_BANK_SIZE)
			write_size = MPU_MEM_BANK_SIZE;
		else
			write_size = size;

		memaddr = ((bank << 8) | 0x00);

		result =
		    mpu6500_i2c_memory_write(i2c_client, memaddr, write_size,
					     data);
		if (result)
			return result;
	}
	return 0;
}

static int mpu6500_lp_verify_firmware(struct i2c_client *i2c_client)
{
	int bank, write_size;
	int result;
	unsigned short memaddr;
	unsigned char firmware[MPU_MEM_BANK_SIZE];

	int size = DMP_CODE_SIZE;
	const unsigned char *data = (unsigned char *)dmpMemory;

	/* Write and verify memory */
	for (bank = 0; size > 0;
		bank++, size -= write_size, data += write_size) {
		if (size > MPU_MEM_BANK_SIZE)
			write_size = MPU_MEM_BANK_SIZE;
		else
			write_size = size;

		memaddr = ((bank << 8) | 0x00);
		result = mpu6500_i2c_memory_read(i2c_client,
						 memaddr, write_size, firmware);
		if (result)
			return result;
		if (0 != memcmp(firmware, data, write_size))
			return -EINVAL;
	}
	return 0;
}

int mpu6500_lp_set_interrupt_on_gesture_event(struct i2c_client *i2c_client,
					      bool on)
{
	unsigned char result;
	const unsigned char regs_on[] = { DINADA, DINADA, DINAB1, DINAB9,
		DINAF3, DINA8B, DINAA3, DINA91,
		DINAB6, DINADA, DINAB4, DINADA
	};
	const unsigned char regs_off[] = { 0xd8, 0xd8, 0xb1, 0xb9, 0xf3, 0x8b,
		0xa3, 0x91, 0xb6, 0x09, 0xb4, 0xd9
	};
	/*For some reason DINAC4 is defined as 0xb8,
	   but DINBC4 is not defined. */
	const unsigned char regs_end[] = { DINAFE, DINAF2, DINAAB, 0xc4,
		DINAAA, DINAF1, DINADF, DINADF
	};
	if (on)
		/*Sets the DMP to send an interrupt and put a FIFO packet
		   in the FIFO if and only if a tap/orientation event
		   just occurred */
		result =
		    mpu6500_i2c_memory_write(i2c_client, CFG_FIFO_ON_EVENT,
					     ARRAY_SIZE(regs_on), regs_on);
	else
		/*Sets the DMP to send an interrupt and put a FIFO packet
		   in the FIFO at the rate specified by the FIFO div.
		   see inv_set_fifo_div in hw_setup.c to set the FIFO div. */
		result =
		    mpu6500_i2c_memory_write(i2c_client, CFG_FIFO_ON_EVENT,
					     ARRAY_SIZE(regs_off), regs_off);
	if (result)
		return result;

	result =
	    mpu6500_i2c_memory_write(i2c_client, CFG_6, ARRAY_SIZE(regs_end),
				     regs_end);
	return result;
}

static int mpu6500_lp_send_sensor_data(struct i2c_client *i2c_client,
				       unsigned short elements)
{
	int result;
	unsigned char regs[] = { DINAA0 + 3, DINAA0 + 3, DINAA0 + 3,
		DINAA0 + 3, DINAA0 + 3, DINAA0 + 3,
		DINAA0 + 3, DINAA0 + 3, DINAA0 + 3,
		DINAA0 + 3
	};

	if (elements & MPU6500_ELEMENT_1)
		regs[0] = DINACA;
	if (elements & MPU6500_ELEMENT_2)
		regs[4] = DINBC4;
	if (elements & MPU6500_ELEMENT_3)
		regs[5] = DINACC;
	if (elements & MPU6500_ELEMENT_4)
		regs[6] = DINBC6;
	if ((elements & MPU6500_ELEMENT_5) || (elements & MPU6500_ELEMENT_6) ||
	    (elements & MPU6500_ELEMENT_7)) {
		regs[1] = DINBC0;
		regs[2] = DINAC8;
		regs[3] = DINBC2;
	}
	result =
	    mpu6500_i2c_memory_write(i2c_client, CFG_15, ARRAY_SIZE(regs),
				     regs);
	return result;
}

static int mpu6500_lp_send_interrupt_word(struct i2c_client *i2c_client)
{
	const unsigned char regs[] = { DINA20 };
	unsigned char result;

	result =
	    mpu6500_i2c_memory_write(i2c_client, CFG_27, ARRAY_SIZE(regs),
				     regs);
	return result;
}

static int mpu6500_lp_set_orient_interrupt_dmp(struct i2c_client *i2c_client,
					       bool on)
{
	/*Turn on the display orientation interrupt in the DMP */
	int result;
	unsigned char regs[] = { 0xd8 };

	if (on)
		regs[0] = 0xd9;
	result =
	    mpu6500_i2c_memory_write(i2c_client, CFG_DISPLAY_ORIENT_INT, 1,
				     regs);
	return result;
}

static int mpu6500_lp_set_orientation_interrupt_dmp(struct i2c_client
						    *i2c_client, bool on)
{
	int result;
	unsigned char regs[2];
	if (on) {
		regs[0] = DINBF8;
		regs[1] = DINBF8;
	} else {
		regs[0] = DINAD8;
		regs[1] = DINAD8;
	}
	result =
	    mpu6500_i2c_memory_write(i2c_client, CFG_ORIENT_IRQ_1,
				     ARRAY_SIZE(regs), regs);
	if (result)
		return result;
	return result;

}

static int mpu6500_lp_set_orientation_dmp(struct i2c_client *i2c_client,
					  int orientation)
{
	/*Set a mask in the DMP determining what orientations
	   will trigger interrupts */
	unsigned char regs[4];
	unsigned char result;

	/* check if any spurious bit other the ones expected are set */
	if (orientation &
	    (~(MPU6500_ORIENTATION_ALL | MPU6500_ORIENTATION_FLIP)))
		return -EINVAL;

	regs[0] = (unsigned char)orientation;
	result = mpu6500_i2c_memory_write(i2c_client, D_1_74, 1, regs);
	return result;
}

static int mpu6500_lp_set_orientation_thresh_dmp(struct i2c_client *i2c_client,
						 int angle)
{
	/*Set an angle threshold in the DMP determining
	   when orientations change */
	unsigned char *regs;
	unsigned char result;
	unsigned int out;
	unsigned int d;
	const unsigned int threshold[] = { 138952416, 268435455, 379625062,
		464943848, 518577479, 536870912
	};
	/* The real calculation is
	 * threshold = (long)((1 << 29) * sin((angle * M_PI) / 180.));
	 * Here we have to use table lookup*/
	d = angle / DMP_ANGLE_SCALE;
	d -= 1;
	if (d >= ARRAY_SIZE(threshold))
		return -EPERM;
	out = cpu_to_be32p(&threshold[d]);
	regs = (unsigned char *)&out;

	result =
	    mpu6500_i2c_memory_write(i2c_client, D_1_232, sizeof(out), regs);
	return result;
}

static int mpu6500_lp_set_orientation_time_dmp(struct i2c_client *i2c_client,
					       unsigned int time,
					       unsigned char sample_divider)
{
	/*Determines the stability time required before a
	   new orientation can be adopted */
	unsigned short dmpTime;
	unsigned char data[2];
	unsigned char sampleDivider;
	unsigned char result;
	/* First check if we are allowed to call this function here */
	sampleDivider = sample_divider;
	sampleDivider++;
	/* 60 ms minimum time added */
	dmpTime = ((time) / sampleDivider);
	data[0] = dmpTime >> 8;
	data[1] = dmpTime & 0xFF;
	result = mpu6500_i2c_memory_write(i2c_client, D_1_250, 2, data);

	return result;
}

static int mpu6500_lp_set_fifo_div(struct i2c_client *i2c_client,
				   unsigned short fifo_rate)
{
	unsigned char regs[2];
	int result = 0;
	/*For some reason DINAC4 is defined as 0xb8, but DINBC4 is not */
	const unsigned char regs_end[12] = { DINAFE, DINAF2, DINAAB, 0xc4,
		DINAAA, DINAF1, DINADF, DINADF,
		0xbb, 0xaf, DINADF, DINADF
	};

	regs[0] = (unsigned char)((fifo_rate >> 8) & 0xff);
	regs[1] = (unsigned char)(fifo_rate & 0xff);
	result =
	    mpu6500_i2c_memory_write(i2c_client, D_0_22, ARRAY_SIZE(regs),
				     regs);
	if (result)
		return result;

	/*Modify the FIFO handler to reset the tap/orient interrupt flags */
	/* each time the FIFO handler runs */
	result =
	    mpu6500_i2c_memory_write(i2c_client, CFG_6, ARRAY_SIZE(regs_end),
				     regs_end);

	return result;
}

static unsigned short mpu6500_lp_row_to_scale(const signed char *row)
{
	unsigned short b;

	if (row[0] > 0)
		b = 0;
	else if (row[0] < 0)
		b = 4;
	else if (row[1] > 0)
		b = 1;
	else if (row[1] < 0)
		b = 5;
	else if (row[2] > 0)
		b = 2;
	else if (row[2] < 0)
		b = 6;
	else
		b = 7;

	return b;
}

static unsigned short mpu6500_lp_orientation_to_scalar(__s8 orientation[9])
{
	unsigned short scalar;

	scalar = mpu6500_lp_row_to_scale(orientation);
	scalar |= mpu6500_lp_row_to_scale(orientation + 3) << 3;
	scalar |= mpu6500_lp_row_to_scale(orientation + 6) << 6;

	return scalar;
}

static int mpu6500_lp_gyro_dmp_cal(struct i2c_client *i2c_client,
				   __s8 orientation[9])
{
	int inv_gyro_orient;
	unsigned char regs[3];
	int result;

	unsigned char tmpD = DINA4C;
	unsigned char tmpE = DINACD;
	unsigned char tmpF = DINA6C;

	inv_gyro_orient = mpu6500_lp_orientation_to_scalar(orientation);

	if ((inv_gyro_orient & 3) == 0)
		regs[0] = tmpD;
	else if ((inv_gyro_orient & 3) == 1)
		regs[0] = tmpE;
	else if ((inv_gyro_orient & 3) == 2)
		regs[0] = tmpF;
	if ((inv_gyro_orient & 0x18) == 0)
		regs[1] = tmpD;
	else if ((inv_gyro_orient & 0x18) == 0x8)
		regs[1] = tmpE;
	else if ((inv_gyro_orient & 0x18) == 0x10)
		regs[1] = tmpF;
	if ((inv_gyro_orient & 0xc0) == 0)
		regs[2] = tmpD;
	else if ((inv_gyro_orient & 0xc0) == 0x40)
		regs[2] = tmpE;
	else if ((inv_gyro_orient & 0xc0) == 0x80)
		regs[2] = tmpF;

	result = mpu6500_i2c_memory_write(i2c_client, FCFG_1, 3, regs);
	if (result)
		return result;

	if (inv_gyro_orient & 4)
		regs[0] = DINA36 | 1;
	else
		regs[0] = DINA36;
	if (inv_gyro_orient & 0x20)
		regs[1] = DINA56 | 1;
	else
		regs[1] = DINA56;
	if (inv_gyro_orient & 0x100)
		regs[2] = DINA76 | 1;
	else
		regs[2] = DINA76;

	result =
	    mpu6500_i2c_memory_write(i2c_client, FCFG_3, ARRAY_SIZE(regs),
				     regs);

	return result;

}

static int mpu6500_lp_accel_dmp_cal(struct i2c_client *i2c_client,
				    __s8 orientation[9])
{
	int inv_accel_orient;
	int result;
	unsigned char regs[3];
	const unsigned char tmp[3] = { DINA0C, DINAC9, DINA2C };
	inv_accel_orient = mpu6500_lp_orientation_to_scalar(orientation);

	regs[0] = tmp[inv_accel_orient & 3];
	regs[1] = tmp[(inv_accel_orient >> 3) & 3];
	regs[2] = tmp[(inv_accel_orient >> 6) & 3];
	result = mpu6500_i2c_memory_write(i2c_client, FCFG_2, 3, regs);
	if (result)
		return result;

	regs[0] = DINA26;
	regs[1] = DINA46;
	regs[2] = DINA66;
	if (inv_accel_orient & 4)
		regs[0] |= 1;
	if (inv_accel_orient & 0x20)
		regs[1] |= 1;
	if (inv_accel_orient & 0x100)
		regs[2] |= 1;
	result =
	    mpu6500_i2c_memory_write(i2c_client, FCFG_7, ARRAY_SIZE(regs),
				     regs);

	return result;

}

int mpu6500_lp_init_dmp(struct i2c_client *i2c_client, __s8 orientation[9])
{
	int result = 0;

	CHECK_RESULT(mpu6500_i2c_write_single_reg
		     (i2c_client, MPUREG_PWR_MGMT_1, 0x0));
#if 0
	CHECK_RESULT(mpu6500_i2c_write_single_reg
		     (i2c_client, MPUREG_INT_PIN_CFG,
		      data->int_pin_cfg | BIT_BYPASS_EN));
#endif

	CHECK_RESULT(mpu6500_lp_load_firmware(i2c_client));
	CHECK_RESULT(mpu6500_lp_verify_firmware(i2c_client));

	CHECK_RESULT(mpu6500_i2c_write_single_reg
		     (i2c_client, MPUREG_DMP_CFG_1, DMP_START_ADDR >> 8));
	CHECK_RESULT(mpu6500_i2c_write_single_reg
		     (i2c_client, MPUREG_DMP_CFG_2, DMP_START_ADDR & 0xff));

	CHECK_RESULT(mpu6500_lp_send_sensor_data
		     (i2c_client, MPU6500_GYRO_ACC_MASK));
	CHECK_RESULT(mpu6500_lp_send_interrupt_word(i2c_client));

	CHECK_RESULT(mpu6500_lp_set_orient_interrupt_dmp(i2c_client, true));
#if 0
	CHECK_RESULT(mpu6500_lp_set_orientation_interrupt_dmp
		     (i2c_client, true));
	CHECK_RESULT(mpu6500_lp_set_orientation_dmp
		     (i2c_client, 0x40 | MPU6500_ORIENTATION_XY));
	CHECK_RESULT(mpu6500_lp_set_orientation_thresh_dmp
		     (i2c_client, DMP_ORIENTATION_ANGLE));
	CHECK_RESULT(mpu6500_lp_set_orientation_time_dmp
		     (i2c_client, DMP_ORIENTATION_TIME, 4));
#endif
	CHECK_RESULT(mpu6500_lp_accel_dmp_cal(i2c_client, orientation));
	CHECK_RESULT(mpu6500_lp_gyro_dmp_cal(i2c_client, orientation));

	return result;
}

int mpu6500_lp_activate_screen_orientation(struct i2c_client *i2c_client,
					   bool enable, int current_delay)
{
	int result = 0;

	if (enable) {
		if (!result)
			result =
			    mpu6500_lp_set_delay(i2c_client, current_delay);

		if (!result)
			result =
			    mpu6500_i2c_write_single_reg(i2c_client,
							 MPUREG_USER_CTRL,
							 BIT_DMP_RST |
							 BIT_FIFO_RST);

		if (!result)
			result =
			    mpu6500_i2c_write_single_reg(i2c_client,
							 MPUREG_USER_CTRL,
							 BIT_DMP_EN |
							 BIT_FIFO_EN);

	} else {
		if (!result)
			result =
			    mpu6500_i2c_write_single_reg(i2c_client,
							 MPUREG_USER_CTRL, 0x0);
	}

	return result;
}

int mpu6500_lp_set_delay(struct i2c_client *i2c_client, int current_delay)
{
	int result = 0;
	unsigned short divider = 0;
	unsigned short fifo_divider = 0;

	divider = (short)(current_delay / 1000) - 1;

	if (divider > 4) {
		fifo_divider = (unsigned short)((current_delay / 1000) / 5) - 1;
	} else {
		fifo_divider = 0;
	}

	result = mpu6500_lp_set_fifo_div(i2c_client, fifo_divider);

	return result;
}
