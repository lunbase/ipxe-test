/*
 *  em_flash_loader -- program external flash, used for Emnic relative cards massproduction
 */

#ifndef _FUNCTIONS_H_
#define _FUNCTIONS_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <arpa/inet.h>

#include "lib/pci.h"
#include "pciutils.h"

#define VENDOR_ID      0x1f64
#define WX_VENDOR_ID   0X8088
#define DEVICE_ID_1001 0x1001
#define DEVICE_ID_2001 0x2001

#define SP		1
#define EM		2

#define MAX_RAPTOR_CARD_NUM 16 // The maximum supported cards number under download

#define DEBUG_ON 0         // Debug information on/off
#define PROGRAM_VERIF_EN 1 // Verify flash program via read-back flash data

// Note: SPI_CLK_DIV set to 0 is used for EM emulation platform only due to emu use 31.25MHz for FMGR

#define SPI_CLK_DIV 3     // 0: 125MHz, 1: 62.5MHz,   2: 41.67Mhz,  3: 31.25MHz, 
			  // 4: 25Mhz,  5: 15.625Mhz, 6: 7.8125Mhz, 7: 3.9Mhz
#define SPI_CLK_DIV_1G 3  // 0: 125MHz, 1: 62.5MHz,   2: 41.67Mhz,  3: 31.25MHz, 
			  // 4: 25Mhz,  5: 15.625Mhz, 6: 7.8125Mhz, 7: 3.9Mhz
#define SPI_CLK_DIV_10G 2 // 0: 125MHz, 1: 62.5MHz,   2: 41.67Mhz,  3: 31.25MHz, 
			  // 4: 25Mhz,  5: 15.625Mhz, 6: 7.8125Mhz, 7: 3.9Mhz. (Can't be larger than 62.5Mhz!)

#define SKIP_FLASH_ERASE 0               // Skip to erase flash before image download, used for Factory MFG only
#define STORE_MAC_ADDR 0                 // Store original MAC address
#define ISSUE_CHIP_RESET_AFTER_UPGRADE 1 // Issue a chip reset to re-enable image download

#define FLASH_WINBOND 0
#define FLASH_SPANISH 1
#define FLASH_SST 2

#define SPI_CMD_ERASE_CHIP 4   // SPI erase chip command
#define SPI_CMD_ERASE_SECTOR 3 // SPI erase sector command
#define SPI_CMD_WRITE_DWORD 0  // SPI write a dword command
#define SPI_CMD_READ_DWORD 1   // SPI read a dword command
#define SPI_CMD_USER_CMD 5     // SPI user command

#define SPI_CLK_CMD_OFFSET 28 // SPI command field offset in Command register
#define SPI_CLK_DIV_OFFSET 25 // SPI clock divide field offset in Command register

#define SPI_TIME_OUT_VALUE 10000
#define SPI_SECTOR_SIZE (64 * 1024)       // FLASH sector size is 64KB
#define SPI_H_CMD_REG_ADDR 0x10104        // SPI Command register address
#define SPI_H_DAT_REG_ADDR 0x10108        // SPI Data register address
#define SPI_H_STA_REG_ADDR 0x1010c        // SPI Status register address
#define SPI_H_USR_CMD_REG_ADDR 0x10110    // SPI User Command register address
#define SPI_CMD_CFG1_ADDR 0x10118         // Flash command configuration register 1
#define MISC_RST_REG_ADDR 0x1000c         // Misc reset register address
#define MGR_FLASH_RELOAD_REG_ADDR 0x101a0 // MGR reload flash read

/*important*/
#define MAC_ADDR0_WORD0_OFFSET_1G 0x006000c // MAC Address for LAN0, stored in external FLASH
#define MAC_ADDR0_WORD1_OFFSET_1G 0x0060014
#define MAC_ADDR1_WORD0_OFFSET_1G 0x006800c // MAC Address for LAN1, stored in external FLASH
#define MAC_ADDR1_WORD1_OFFSET_1G 0x0068014
#define MAC_ADDR2_WORD0_OFFSET_1G 0x007000c // MAC Address for LAN2, stored in external FLASH
#define MAC_ADDR2_WORD1_OFFSET_1G 0x0070014
#define MAC_ADDR3_WORD0_OFFSET_1G 0x007800c // MAC Address for LAN3, stored in external FLASH
#define MAC_ADDR3_WORD1_OFFSET_1G 0x0078014
#define PRODUCT_SERIAL_NUM_OFFSET_1G 0x00f0000 // Product Serial Number, stored in external FLASH last sector

// Internal Macrors (fixed, not to change!!)
#define MAC_ADDR0_WORD0_OFFSET 0x006000c // MAC Address for LAN0, stored in external FLASH
#define MAC_ADDR0_WORD1_OFFSET 0x0060014
#define MAC_ADDR1_WORD0_OFFSET 0x007000c // MAC Address for LAN1, stored in external FLASH
#define MAC_ADDR1_WORD1_OFFSET 0x0070014
#define PRODUCT_SERIAL_NUM_OFFSET 0x00f0000 // Product Serial Number, stored in external FLASH last sector

//TX_EQ offset
#define SP0_TXEQ_CTRL0_OFFSET 0x006002c // txeq, stored in external FLASH
#define SP0_TXEQ_CTRL1_OFFSET 0x006003c
#define SP1_TXEQ_CTRL0_OFFSET 0x007002c // txeq, stored in external FLASH
#define SP1_TXEQ_CTRL1_OFFSET 0x007003c

#define CHECKSUM_CAP_ST_PASS      0x80658383
#define CHECKSUM_CAP_ST_FAIL      0x70657376

#define EMNIC_MSCA                      0x11200
#define EMNIC_MSCA_RA(v)                ((0xFFFF & (v)))
#define EMNIC_MSCA_PA(v)                ((0x1F & (v)) << 16)
#define EMNIC_MSCA_DA(v)                ((0x1F & (v)) << 21)
#define EMNIC_MSCC                      0x11204
#define EMNIC_MSCC_DATA(v)              ((0xFFFF & (v)))
#define EMNIC_MSCC_CMD(v)               ((0x3 & (v)) << 16)
enum EMNIC_MSCA_CMD_value {
	EMNIC_MSCA_CMD_RSV = 0,
	EMNIC_MSCA_CMD_WRITE,
	EMNIC_MSCA_CMD_POST_READ,
	EMNIC_MSCA_CMD_READ,
};
#define EMNIC_MSCC_SADDR                ((0x1U) << 18)
#define EMNIC_MSCC_CR(v)                ((0x8U & (v)) << 19)
#define EMNIC_MSCC_BUSY                 ((0x1U) << 22)
#define EMNIC_MDIO_CLK(v)               ((0x7 & (v)) << 19)

#define FUNC_NUM 4

#define PHY_CONFIG(reg_offset) (0x14000 + ((reg_offset)*4))

static u8 debug_mode;

static u8 opt_f_is_set;
static u8 opt_m_is_set;
static u8 opt_a_is_set;
static u8 opt_u_is_set;
static u8 opt_c_is_set;
static u8 opt_p_is_set;
static u8 opt_k_is_set;
static u8 opt_e_is_set;
static u8 opt_d_is_set;
static u8 opt_t_is_set;
static u8 opt_s_is_set;
static u8 opt_s1_is_set;
static u8 opt_r_is_set;
static u8 opt_i_is_set;
static u8 end;

static u8 g_port_num;

static char IMAGE_FILE[128];
static u8 flash_vendor;
static u8 all_card_num;
static u8 dev_index;
static u8 upgrade_mode;
static struct pci_dev *global_dev;

static char MAC_ADDR[128];
static char SN[128];
static char str_dword[16];
//static u8 chip_v;
static u8 f_chip_v;

static u8 mac_valid;
static u32 mac_addr0_dword0;
static u32 mac_addr0_dword1;
static u32 mac_addr1_dword0;
static u32 mac_addr1_dword1;
static u32 mac_addr2_dword0;
static u32 mac_addr2_dword1;
static u32 mac_addr3_dword0;
static u32 mac_addr3_dword1;

/*
static u32 mac_addr0_dword0_t;
static u32 mac_addr0_dword1_t;
static u32 mac_addr1_dword0_t;
static u32 mac_addr1_dword1_t;
static u32 mac_addr2_dword0_t;
static u32 mac_addr2_dword1_t;
static u32 mac_addr3_dword0_t;
static u32 mac_addr3_dword1_t;
*/

static u32 serial_num_dword0;
static u32 serial_num_dword1;
static u32 serial_num_dword2;
static u32 serial_num_dword[24];

/*
static u32 serial_num_dword0_t;
static u32 serial_num_dword1_t;
static u32 serial_num_dword2_t;
static u32 serial_num_dword_t[24];
*/

static u32 txeq0_ctrl0;
static u32 txeq0_ctrl1;
static u32 txeq1_ctrl0;
static u32 txeq1_ctrl1;

static u8 ffe_main;
static u8 ffe_pre;
static u8 ffe_post;

static u8 don_card_num;


struct wx_phy
{
	char phy_type[20];
	u32 phy_id;
	u8 access_c;
};


struct wx_nic
{
	char chip_type[20];
	struct wx_phy phy;
	u32 cab_0;
	u32 flash_0;
	u32 fw_version;
	u32 fw_init;
	u8  chip_v;
	u16 wol;
	u16 ncsi;
	u16 oprom_arch;
};

struct emnic_hic_hdr2_req
{
	u8 cmd;
	u8 buf_lenh;
	u8 buf_lenl;
	u8 checksum;
};

struct emnic_hic_hdr2_rsp
{
	u8 cmd;
	u8 buf_lenl;
	u8 buf_lenh_status; /* 7-5: high bits of buf_len, 4-0: status */
	u8 checksum;
};

union emnic_hic_hdr2
{
	struct emnic_hic_hdr2_req req;
	struct emnic_hic_hdr2_rsp rsp;
};

struct emnic_hic_read_shadow_ram
{
	union emnic_hic_hdr2 hdr;
	u32 address;
	u16 length;
	u16 pad2;
	u16 data;
	u16 pad3;
};
struct emnic_hic_hdr
{
	u8 cmd;
	u8 buf_len;
	union
	{
		u8 cmd_resv;
		u8 ret_status;
	} cmd_or_resp;
	u8 checksum;
};
struct emnic_hic_read_cab
{
	union emnic_hic_hdr2 hdr;
	union
	{
		u8 d8[252];
		u16 d16[126];
		u32 d32[63];
	} dbuf;
};

extern void write_reg(struct pci_dev *dev, u64 addr, u32 val);
extern u32 read_reg(struct pci_dev *dev, u64 addr);
extern u8 fmgr_cmd_op(struct pci_dev *dev, u32 cmd, u32 cmd_addr);
extern u8 fmgr_usr_cmd_op(struct pci_dev *dev, u32 usr_cmd);
extern u8 flash_erase_chip(struct pci_dev *dev);
extern u8 flash_erase_sector(struct pci_dev *dev, u32 sec_addr);
extern u32 flash_read_dword(struct pci_dev *dev, u32 addr);
extern u8 flash_write_dword(struct pci_dev *dev, u32 addr, u32 dword);
extern void phy_read_reg(struct pci_dev *dev, u32 reg_offset, u32 page, u16 *phy_data);
extern void phy_write_reg(struct pci_dev *dev, u32 reg_offset, u32 page, u16 phy_data);
extern int emnic_flash_write_cab(struct pci_dev *dev, u32 addr, u32 value, u16 lan_id);
extern int emnic_flash_read_cab(struct pci_dev *dev, u32 addr, u16 lan_id);
extern void emnic_release_eeprom_semaphore(struct pci_dev *dev);
extern void emnic_release_swfw_sync(struct pci_dev *dev, u32 mask);
extern u32 emnic_phy_write_reg_mdi(struct pci_dev *dev,
				u32 reg_addr,
				u32 device_type,
				u16 phy_data,
				u32 phy_id);
extern u32 emnic_phy_read_reg_mdi(struct pci_dev *dev,
				u32 reg_addr,
				u32 device_type,
				u16 phy_id);

extern void do_a(struct pci_dev *dev);
extern void do_b(struct pci_dev *dev);
extern void do_c(struct pci_dev *dev);
extern void do_d(struct pci_dev *dev);
extern void do_e(struct pci_dev *dev);
extern void do_f(struct pci_dev *dev);
extern void do_g(struct pci_dev *dev);
extern void do_h(struct pci_dev *dev);
extern inline u32 po32m(struct pci_dev *dev, u32 reg, u32 mask, u32 field, int usecs, int count);
extern int parse_options_for_up_flash(int argc, char **argv);
extern void usage_for_up_flash(int choose);

_Bool emnic_check_mng_access(struct pci_dev *dev);
void emnic_release_swfw_sync(struct pci_dev *dev, u32 mask);
int emnic_host_interface_pass_command(struct pci_dev *dev, u32 *buffer,
				     u32 length, _Bool status);
void wr32m(struct pci_dev *dev, u32 reg, u32 mask, u32 field);
int emnic_get_eeprom_semaphore(struct pci_dev *dev);
void emnic_release_eeprom_semaphore(struct pci_dev *dev);
int emnic_acquire_swfw_sync(struct pci_dev *dev, u32 mask);
extern int emnic_flash_write_unlock(struct pci_dev *dev);
extern int emnic_flash_write_lock(struct pci_dev *dev);
extern int flash_write_unlock(struct pci_dev *dev);
extern int flash_write_lock(struct pci_dev *dev);
extern int emnic_eepromcheck_cap(struct pci_dev *dev);
extern u32 emnic_check_internal_phy_id(struct pci_dev *dev);

extern int check_chip_version(struct pci_dev *dev);
extern int check_image_version(void);
extern int check_image_checksum(void);

extern int check_nic_status(struct pci_dev *dev);
struct device * scan_device(struct pci_dev *p);
extern int wx_nic_int(struct pci_dev *dev);
extern int check_nic_type(struct pci_dev *dev);
extern int check_image_status(void);

struct region {
	u32 id;
	char *name;
	u32 start;
	u32 end;
	u8 store;
};
typedef struct region flash_region;

struct card_items {
	u32 id;
	u32 aux_det;
	char *name; /* outer mac */
	u32 subs_id; /* outer ip*/
	u32 dev_id; /* encaped type */
	u32 port_num; /* encaped ip */
	u32 wol; /* payload proto */
	u32 ncsi; /* payload layer */
	u16 force;
	u32 version;
	char *arch;
	char *pxe_rom;
	char *fw_bin;
};
typedef struct card_items card_items;
#if 1
//typedef struct spnic_dec_ptype spnic_dptype;
card_items cards[256] = {
	{1 ,0,"SF400HXT" ,0x0430,0x0100,4,0,0,0,0,"","",""},
	{2 ,0,"SF200T"   ,0x0201,0x0101,2,0,0,0,0,"","",""},
	{3 ,0,"SF200T-S" ,0x0210,0x0102,2,0,0,0,0,"","",""},
	{4 ,0,"SF400T"   ,0x0401,0x0103,4,0,0,0,0,"","",""},
	{5 ,0,"SF400T-S" ,0x0410,0x0104,4,0,0,0,0,"","",""},
	{6 ,0,"SF200HT"  ,0x0202,0x0105,2,0,0,0,0,"","",""},
	{7 ,0,"SF200HT-S",0x0220,0x0106,2,0,0,0,0,"","",""},
	{8 ,0,"SF400HT"  ,0x0402,0x0107,4,0,0,0,0,"","",""},
	{9 ,0,"SF400HT-S",0x0420,0x0108,4,0,0,0,0,"","",""},
	{10,0,"SF400T-LC",0x0401,0x0109,4,0,0,0,0,"","",""},
	{11,0,"SF100T"   ,0x0101,0x010a,1,0,0,0,0,"","",""},
	{12,0,"SF100HT"  ,0x0102,0x010b,1,0,0,0,0,"","",""},
	{13,0,"SF100HXT" ,0x0130,0x0100,1,0,0,0,0,"","",""},
	{14,0,"SF200HXT" ,0x0230,0x0100,2,0,0,0,0,"","",""},

	{15,0,"SF100F"   ,0x0103,0x010a,1,0,0,0,0,"","",""},
	{16,0,"SF200F"   ,0x0203,0x0101,2,0,0,0,0,"","",""},
	{17,0,"SF400F"   ,0x0403,0x0103,4,0,0,0,0,"","",""},
	{18,0,"SF100HF"  ,0x0103,0x010b,1,0,0,0,0,"","",""},
	{19,0,"SF200HF"  ,0x0203,0x0105,2,0,0,0,0,"","",""},
	{20,0,"SF400HF"  ,0x0403,0x0107,4,0,0,0,0,"","",""},

	{21,0,"SF100F-YT",0x0160,0x010a,1,0,0,0,0,"","",""},
	{22,0,"SF200F-YT",0x0260,0x0101,2,0,0,0,0,"","",""},
	{23,0,"SF400F-YT",0x0460,0x0103,4,0,0,0,0,"","",""},
	{24,0,"SF100HF-YT",0x0160,0x010b,1,0,0,0,0,"","",""},
	{25,0,"SF200HF-YT",0x0260,0x0105,2,0,0,0,0,"","",""},
	{26,0,"SF400HF-YT",0x0460,0x0107,4,0,0,0,0,"","",""},

	{27,0,"SF400F-LY",0x0450,0x0107,4,0,0,0,0,"","",""},
	{28,0,"SF400TF-YT",0x0461,0x0103,4,0,0,0,0,"","",""},
	{29,0,"SF400F-LY-YT",0x0470,0x0107,4,0,0,0,0,"","",""},

	{30,0,"SF400-OCP",0x0440,0x0103,4,0,1,0,0,"","",""},
	{31,0,"SF100F-LR",0x0103,0x0101,1,0,0,0,0,"","",""},
	{32,0,"SF100HF-LR",0x0103,0x0102,2,0,0,0,0,"","",""},
	{33,0,"SF200F-LY",0x0250,0x0107,2,0,0,0,0,"","",""},
	{34,0,"SF100F-LY",0x0150,0x0107,1,0,0,0,0,"","",""},
	{35,0,"SF200F-LY-YT",0x0270,0x0107,2,0,0,0,0,"","",""},
	{36,0,"SF100F-LY-YT",0x0170,0x0107,1,0,0,0,0,"","",""},

	{37,0,"SF200T-LC",0x0201,0x0109,2,0,0,0,0,"","",""},
	{38,0,"SF400HR",0x0480,0x0107,4,0,0,0,0,"","",""},
	{39, 0, "SF400H-1512T", 0x0200, 0x0107, 4, 0, 0, 0, 0, "", "", ""},
	{40, 0, "SF400T-MV", 0x0451, 0x0103, 4, 0, 0, 0, 0, "", "", ""},
	{41, 0, "SF400TF-MV", 0x0452, 0x0103, 4, 0, 0, 0, 0, "", "", ""},
	{42, 0, "SF200T-MV", 0x0251, 0x0101, 2, 0, 0, 0, 0, "", "", ""},
	{43, 0, "SF200TF-MV", 0x0252, 0x0101, 2, 0, 0, 0, 0, "", "", ""},

	{44, 0, "SF400-YT", 0x0460, 0x0100,4,0,0,0,0,"","",""},
	{45, 0, "SF200-YT", 0x0260, 0x0100,2,0,0,0,0,"","",""},
	{46,0,"SF400F-YT-GPIO",0x0462,0x0103,4,0,0,0,0,"","",""},
	{47,0,"SF200F-YT-GPIO",0x0262,0x0101,2,0,0,0,0,"","",""},
	{48,0,"SF400TF-YT-GPIO",0x0464,0x0103,4,0,0,0,0,"","",""},
	{49,0,"SF200TF-YT-GPIO",0x0264,0x0101,2,0,0,0,0,"","",""},
	{50,0,"SF200-OCP",0x0240,0x0101,2,0,1,0,0,"","",""},

	{51,0,"SF200HTF-MV",0x0252,0x0105,2,0,0,0,0,"","",""},
	{52,0,"SF400HTF-MV",0x0452,0x0107,4,0,0,0,0,"","",""},
	{53,0,"SF400-YT-3lan",0x0460,0x0107,3,0,0,0,0,"","",""},
};
#endif


void write_reg(struct pci_dev *dev, u64 addr, u32 val)
{
	pci_write_res_long(dev, 0, addr, val);
}

u32 read_reg(struct pci_dev *dev, u64 addr)
{
	return pci_read_res_long(dev, 0, addr);
}

// cmd_addr is used for some special command:
//   1. to be sector address, when implemented erase sector command
//   2. to be flash address when implemented read, write flash address
u8 fmgr_cmd_op(struct pci_dev *dev, u32 cmd, u32 cmd_addr)
{
	u32 cmd_val = 0;
	u32 time_out = 0;

	cmd_val = (cmd << SPI_CLK_CMD_OFFSET) | (SPI_CLK_DIV << SPI_CLK_DIV_OFFSET) | cmd_addr;
	write_reg(dev, SPI_H_CMD_REG_ADDR, cmd_val);

	while (1)
	{
		if (read_reg(dev, SPI_H_STA_REG_ADDR) & 0x1)
			break;

		if (time_out == SPI_TIME_OUT_VALUE)
			return 1;

		time_out = time_out + 1;
		usleep(10);
	}

	return 0;
}

u8 fmgr_usr_cmd_op(struct pci_dev *dev, u32 usr_cmd)
{
	u8 status = 0;

	write_reg(dev, SPI_H_USR_CMD_REG_ADDR, usr_cmd);
	status = fmgr_cmd_op(dev, SPI_CMD_USER_CMD, 0);

	return status;
}

u8 flash_erase_chip(struct pci_dev *dev)
{
	u8 status = fmgr_cmd_op(dev, SPI_CMD_ERASE_CHIP, 0);
	return status;
}

u8 flash_erase_sector(struct pci_dev *dev, u32 sec_addr)
{
	u8 status = fmgr_cmd_op(dev, SPI_CMD_ERASE_SECTOR, sec_addr);
	return status;
}

u32 flash_read_dword(struct pci_dev *dev, u32 addr)
{
	u8 status = fmgr_cmd_op(dev, SPI_CMD_READ_DWORD, addr);
	if (status)
		return (u32)status;

	return read_reg(dev, SPI_H_DAT_REG_ADDR);
}

u8 flash_write_dword(struct pci_dev *dev, u32 addr, u32 dword)
{
	u8 status = 0;

	write_reg(dev, SPI_H_DAT_REG_ADDR, dword);
	status = fmgr_cmd_op(dev, SPI_CMD_WRITE_DWORD, addr);
	if (status)
		return status;

	if (PROGRAM_VERIF_EN)
	{
		if (dword != flash_read_dword(dev, addr))
		{
			return 1;
		}
	}

	return 0;
}

void safe_flush(FILE *fp)
{
	int ch;
	while( (ch = fgetc(fp)) != EOF && ch != '\n');
}

void wr32m(struct pci_dev *dev, u32 reg, u32 mask, u32 field)
{
	u32 val = 0;
	val = ((val & ~mask) | (field & mask));
	write_reg(dev, reg, val);
}

/* poll register */
#define EMNIC_MDIO_TIMEOUT 1000
#define EMNIC_I2C_TIMEOUT  1000
#define EMNIC_SPI_TIMEOUT  1000
inline u32
po32m(struct pci_dev *dev, u32 reg,
		u32 mask, u32 field, int usecs, int count)
{
	int loop;

	loop = (count ? count : (usecs + 9) / 10);
	usecs = (loop ? (usecs + loop - 1) / loop : 0);

	count = loop;
	do {
		u32 value = read_reg(dev, reg);
		if ((value & mask) == (field & mask)) {
			break;
		}

		if (loop-- <= 0)
			break;

		usleep(usecs);
	} while (1);

	return (count - loop <= count ? 0 : -143);
}

#define rd32a(a, reg, offset) ( \
    read_reg((a), (reg) + ((offset) << 2)))

_Bool emnic_check_mng_access(struct pci_dev *dev)
{
	u32 fwsm;
	fwsm = read_reg(dev, 0x10028);
	fwsm = fwsm & 0x1;
	if (!fwsm)
		return 0;
	return 1;
}

void emnic_release_eeprom_semaphore(struct pci_dev *dev)
{
	if (emnic_check_mng_access(dev))
	{
		//printf("emnic_release_eeprom_semaphore1=04%x: 2c%x\n",read_reg ( dev->regs + 0x1E004 ),read_reg ( dev->regs + 0x1002c ) );
		wr32m(dev, 0x1E004, 0x1, 0);
		wr32m(dev, 0x1002c, 0x1, 0);
		//printf("emnic_release_eeprom_semaphore2=04%x: 2c%x\n",read_reg ( dev->regs + 0x1E004 ),read_reg ( dev->regs + 0x1002c ) );
		read_reg(dev, 0x10000);
		//wmb();
	}
}

int emnic_get_eeprom_semaphore(struct pci_dev *dev)
{
	int status = -1;
	u32 timeout = 2000;
	u32 i;
	u32 swsm;

	/* Get SMBI software semaphore between device drivers first */
	for (i = 0; i < timeout; i++)
	{
		/*
		 * If the SMBI bit is 0 when we read it, then the bit will be
		 * set and we have the semaphore
		 */
		swsm = read_reg(dev, 0x1002c);
		if (!(swsm & 1))
		{
			status = 0;
			break;
		}
		usleep(50);
	}
	if (i == timeout)
	{
		printf("Driver can't access the Eeprom - SMBI Semaphore "
		       "not granted.\n");
		emnic_release_eeprom_semaphore(dev);
		usleep(50);
		/*
		 * one last try
		 * If the SMBI bit is 0 when we read it, then the bit will be
		 * set and we have the semaphore
		 */
		swsm = read_reg(dev, 0x1002c);
		printf("swsm ===%x\n", swsm);
		if (!(swsm & 1))
			status = 0;
	}
	/* Now get the semaphore between SW/FW through the SWESMBI bit */
	if (status == 0)
	{
		for (i = 0; i < timeout; i++)
		{
			if (emnic_check_mng_access(dev))
			{
				/* Set the SW EEPROM semaphore bit to request access */
				wr32m(dev, 0x1E004, 0x1, 0x1);
				/*
				 * If we set the bit successfully then we got
				 * semaphore.
				 */
				swsm = read_reg(dev, 0x1E004);
				if (swsm & 1)
					break;
			}
			usleep(50);
		}

		/*
		 * Release semaphores and return error if SW EEPROM semaphore
		 * was not granted because we don't have access to the EEPROM
		 */
		if (i >= timeout)
		{
			printf("SWESMBI Software EEPROM semaphore not granted.\n");
			emnic_release_eeprom_semaphore(dev);
			status = -2;
		}
	}
	else
	{
		printf("Software semaphore SMBI between device drivers "
		       "not granted.\n");
	}
	//printf("emnic_get_eeprom_semaphore==3\n");
	return status;
}

int emnic_acquire_swfw_sync(struct pci_dev *dev, u32 mask)
{
	u32 gssr = 0;
	u32 swmask = mask;
	u32 fwmask = mask << 16;
	u32 timeout = 200;
	u32 i;

	for (i = 0; i < timeout; i++)
	{
		/*
		 * SW NVM semaphore bit is used for access to all
		 * SW_FW_SYNC bits (not just NVM)
		 */
		if (emnic_get_eeprom_semaphore(dev))
			return -1;

		if (emnic_check_mng_access(dev))
		{
			gssr = read_reg(dev, 0x1E008);
			if (!(gssr & (fwmask | swmask)))
			{
				gssr |= swmask;
				write_reg(dev, gssr, 0x1E008);
				emnic_release_eeprom_semaphore(dev);
				return 0;
			}
			else
			{
				/* Resource is currently in use by FW or SW */
				emnic_release_eeprom_semaphore(dev);
				usleep(5000);
			}
		}
	}

	/* If time expired clear the bits holding the lock and retry */
	if (gssr & (fwmask | swmask))
		emnic_release_swfw_sync(dev, gssr & (fwmask | swmask));

	usleep(5000);
	return -2;
}

void emnic_release_swfw_sync(struct pci_dev *dev, u32 mask)
{
	emnic_get_eeprom_semaphore(dev);
	if (emnic_check_mng_access(dev))
		wr32m(dev, 0x1E008, mask, 0);

	emnic_release_eeprom_semaphore(dev);
}

int emnic_host_interface_pass_command(struct pci_dev *dev, u32 *buffer,
				     u32 length, _Bool return_data)
{
	u32 timeout = 5000;

	u32 hicr, i, bi;
	u32 hdr_size = sizeof(struct emnic_hic_hdr);
	u16 buf_len;
	u32 dword_len;
	u32 status = 0;

	if (length == 0 || length > 256)
	{
		return -1;
	}

	emnic_acquire_swfw_sync(dev, 0x0004);

	if ((length % (sizeof(u32))) != 0)
	{
		status = -32;
		goto rel_out;
	}

	dword_len = length >> 2;
	/*The device driver writes the relevant command block
	***into the ram area.
	***/
	for (i = 0; i < dword_len; i++)
	{
		if (emnic_check_mng_access(dev))
			//wr32a(hw, EMNIC_MNG_MBOX, i, EMNIC_CPU_TO_LE32(buffer[i]));
			write_reg(dev, 0x1E100 + (i << 2), cpu_to_le32(buffer[i]));
		//wr32a(hw, 0x1E100, i, cpu_to_le32(buffer[i]));
		else
		{
			status = -47;
			goto rel_out;
		}
	}
	/* Setting this bit tells the ARC that a new command is pending. */
	if (emnic_check_mng_access(dev))
	{
		//wr32m(hw, EMNIC_MNG_MBOX_CTL, EMNIC_MNG_MBOX_CTL_SWRDY, EMNIC_MNG_MBOX_CTL_SWRDY);
		wr32m(dev, 0x1E044, 0x1, 0x1);
	}
	else
	{
		status = -47;
		goto rel_out;
	}

	for (i = 0; i < timeout; i++)
	{
		if (emnic_check_mng_access(dev))
		{
			hicr = read_reg(dev, 0x1e044);
			if ((hicr & 0x4))
				break;
		}
		//msec_delay(1);
		usleep(1000);
	}

	/* Check command completion */
	if (timeout != 0 && i == timeout)
	{
		printf("Command has failed with no status valid.\n");

		printf("0x1e100===%08x\n", read_reg(dev, 0x1e100));
		printf("0x1e104===%08x\n", read_reg(dev, 0x1e104));
		printf("0x1e108===%08x\n", read_reg(dev, 0x1e108));
		printf("0x1e10c===%08x\n", read_reg(dev, 0x1e10c));
		printf("0x1e044===%08x\n", read_reg(dev, 0x1e044));
		printf("0x10000===%08x\n", read_reg(dev, 0x10000));
#if 0
		printf("write value:\n");
		for (i = 0; i < dword_len; i++)
		{
			printf("%x ", buffer[i]);
		}
		printf("read value:\n");
		for (i = 0; i < dword_len; i++)
		{
			printf("%x ", buf[i]);
		}
#endif
		status = -133;
		goto rel_out;
	}

	if (!return_data)
		goto rel_out;

	/* Calculate length in DWORDs */
	dword_len = hdr_size >> 2;

	/* first pull in the header so we know the buffer length */
	for (bi = 0; bi < dword_len; bi++)
	{
		if (emnic_check_mng_access(dev))
		{
			buffer[bi] = rd32a(dev, 0x1e100,
					   bi);
			le32_to_cpu(&buffer[bi]);
		}
		else
		{
			status = -147;
			goto rel_out;
		}
	}

	/* If there is any thing in data position pull it in */
	buf_len = ((struct emnic_hic_hdr *)buffer)->buf_len;
	if (buf_len == 0)
		goto rel_out;

	if (length < buf_len + hdr_size)
	{
		printf("Buffer not large enough for reply message.\n");
		status = -147;
		goto rel_out;
	}

	/* Calculate length in DWORDs, add 3 for odd lengths */
	dword_len = (buf_len + 3) >> 2;

	/* Pull in the rest of the buffer (bi is where we left off) */
	for (; bi <= dword_len; bi++)
	{
		if (emnic_check_mng_access(dev))
		{
			buffer[bi] = rd32a(dev, 0x1e100,
					   bi);
			le32_to_cpu(&buffer[bi]);
		}
		else
		{
			status = -147;
			goto rel_out;
		}
	}

rel_out:
	emnic_release_swfw_sync(dev, 0x0004);
	return status;
}

int emnic_flash_write_cab(struct pci_dev *dev, u32 addr, u32 value, u16 lan_id)
{
	int status;
	struct emnic_hic_read_cab buffer;

	buffer.hdr.req.cmd = 0xE2;
	buffer.hdr.req.buf_lenh = 0x6;
	buffer.hdr.req.buf_lenl = 0x0;
	buffer.hdr.req.checksum = 0xFF;

	/* convert offset from words to bytes */
	buffer.dbuf.d16[0] = cpu_to_le16(lan_id);
	/* one word */
	//buffer.dbuf.d32[0] = cpu_to_le32(addr);
	//buffer.dbuf.d32[1] = cpu_to_le32(value);
	buffer.dbuf.d32[0] = htonl(addr);
	buffer.dbuf.d32[1] = htonl(value);

	status = emnic_host_interface_pass_command(dev, (u32 *)&buffer,
						  sizeof(buffer), 0);
	if (DEBUG_ON || debug_mode)
		printf("0x1e100 :%08x\n", read_reg(dev, 0x1e100));
	if (DEBUG_ON || debug_mode)
		printf("0x1e104 :%08x\n", read_reg(dev, 0x1e104));
	if (DEBUG_ON || debug_mode)
		printf("0x1e108 :%08x\n", read_reg(dev, 0x1e108));
	if (DEBUG_ON || debug_mode)
		printf("0x1e10c :%08x\n", read_reg(dev, 0x1e10c));

	return status;
}
int emnic_flash_read_cab(struct pci_dev *dev, u32 addr, u16 lan_id)
{
	int status;
	struct emnic_hic_read_cab buffer;
	//u16 *data = NULL;

	buffer.hdr.req.cmd = 0xE1;
	buffer.hdr.req.buf_lenh = 0xaa;
	buffer.hdr.req.buf_lenl = 0;
	buffer.hdr.req.checksum = 0xFF;

	//printf("addr:%04x-lan_id:%x\n", addr, lan_id);
	/* convert offset from words to bytes */
	buffer.dbuf.d16[0] = cpu_to_le16(lan_id);
	/* one word */
	//buffer.dbuf.d32[0] = cpu_to_le32(addr);
	buffer.dbuf.d32[0] = htonl(addr);

	status = emnic_host_interface_pass_command(dev, (u32 *)&buffer,
						  sizeof(buffer), 0);

	if (status)
		return status;
	if (emnic_check_mng_access(dev))
	{
		//*data = (u16)rd32a(dev, 0x1e100, 3);
		if (DEBUG_ON || debug_mode) printf("0x1e100 :%08x\n", read_reg(dev, 0x1e100));
		if (DEBUG_ON || debug_mode) printf("0x1e104 :%08x\n", read_reg(dev, 0x1e104));
		if (DEBUG_ON || debug_mode) printf("0x1e108 :%08x\n", read_reg(dev, 0x1e108));
		if (DEBUG_ON || debug_mode) printf("0x1e10c :%08x\n", read_reg(dev, 0x1e10c));
	}
	else
	{
		status = -147;
		return status;
	}

	return read_reg(dev, 0x1e108);
}
int emnic_flash_write_unlock(struct pci_dev *dev)
{
	int status;
	struct emnic_hic_read_shadow_ram buffer;

	buffer.hdr.req.cmd = 0x40;
	buffer.hdr.req.buf_lenh = 0;
	buffer.hdr.req.buf_lenl = 0;
	buffer.hdr.req.checksum = 0xFF;

	/* convert offset from words to bytes */
	buffer.address = 0;
	/* one word */
	buffer.length = 0;

	status = emnic_host_interface_pass_command(dev, (u32 *)&buffer,
						  sizeof(buffer), 0);
	if (status)
		return status;

	return status;
}

int emnic_flash_write_lock(struct pci_dev *dev)
{
	int status;
	struct emnic_hic_read_shadow_ram buffer;

	buffer.hdr.req.cmd = 0x39;
	buffer.hdr.req.buf_lenh = 0;
	buffer.hdr.req.buf_lenl = 0;
	buffer.hdr.req.checksum = 0xFF;

	/* convert offset from words to bytes */
	buffer.address = 0;
	/* one word */
	buffer.length = 0;

	status = emnic_host_interface_pass_command(dev, (u32 *)&buffer,
						  sizeof(buffer), 0);
	if (status)
		return status;

	return status;
}

int emnic_eepromcheck_cap(struct pci_dev *dev)
{
	int status;
	u32 tmp = 0;
	struct emnic_hic_read_shadow_ram buffer;

	buffer.hdr.req.cmd = 0xE9;
	buffer.hdr.req.buf_lenh = 0;
	buffer.hdr.req.buf_lenl = 0;
	buffer.hdr.req.checksum = 0xFF;

	/* convert offset from words to bytes */
	buffer.address = 0;
	/* one word */
	buffer.length = 0;

	status = emnic_host_interface_pass_command(dev, (u32 *)&buffer,
						  sizeof(buffer), 0);
	if (status)
		return status;

		if (emnic_check_mng_access(dev)){
		tmp = (u32)rd32a(dev, 0x1e100, 1);
		if (tmp == CHECKSUM_CAP_ST_PASS)
			status = 0;
		else
			status = -102;
	}else {
			status = -147;
			return status;
		
	}

	return status;
}

int flash_write_unlock(struct pci_dev *dev)
{
	int status = 0;
	// unlock flash write protect
	emnic_release_eeprom_semaphore(dev);
	status = emnic_flash_write_unlock(dev);
	sleep(1);
#if 0
	write_reg(dev, 0x10114, 0x9f050206);
	write_reg(dev, 0x10194, 0x9f050206);

	status = emnic_flash_write_cab(dev, 0x188, 0, 0);
	sleep(1);
	status = emnic_flash_write_cab(dev, 0x184, 0x60000000, 0);
	sleep(1);
#endif
	return status;
}

int flash_write_lock(struct pci_dev *dev)
{
	int status;

	// lock flash write protect
	emnic_release_eeprom_semaphore(dev);
	status = emnic_flash_write_lock(dev);

	//write_reg(dev, 0x10114, 0x9f050204);
	//write_reg(dev, 0x10194, 0x9f050204);
#if 0
	status = emnic_flash_write_cab(dev, 0x188, 0, 0);
	sleep(1);
	status = emnic_flash_write_cab(dev, 0x184, 0x60000000, 0);
	sleep(1);
#endif
	return status;
}

void phy_read_reg(struct pci_dev *dev, u32 reg_offset, u32 page, u16 *phy_data)
{
	int page_select = 0;

	/* clear input */
	*phy_data = 0;

	if (0 != page)
	{
		/* select page */
		if (0xa42 == page || 0xa43 == page || 0xa46 == page || 0xa47 == page ||
		    0xd04 == page || 0xc80 == page)
		{
			write_reg(dev, PHY_CONFIG(31), page);
			page_select = 1;
		}
	}

	if (reg_offset >= 32)
	{
		printf("input reg offset %d exceed maximum 31.\n", reg_offset);
	}

	*phy_data = 0xFFFF & read_reg(dev, PHY_CONFIG(reg_offset));

	if (page_select)
		write_reg(dev, PHY_CONFIG(31), 0);
}

void phy_write_reg(struct pci_dev *dev, u32 reg_offset, u32 page, u16 phy_data)
{
	int page_select = 0;

	if (0 != page)
	{
		/* select page */
		if (0xa42 == page || 0xa43 == page || 0xa46 == page || 0xa47 == page ||
		    0xd04 == page || 0xc80 == page)
		{
			write_reg(dev, PHY_CONFIG(31), page);
			page_select = 1;
		}
	}

	if (reg_offset >= 32)
	{
		printf("input reg offset %d exceed maximum 31.\n", reg_offset);
	}

	write_reg(dev, PHY_CONFIG(reg_offset), phy_data);

	if (page_select)
		write_reg(dev, PHY_CONFIG(31), 0);
}

void do_a(struct pci_dev *dev)
{
	char k;
	u16 value;
	phy_write_reg(dev, 31, 0x0, 0x0000); //page 0
	phy_write_reg(dev, 9, 0x0, 0x2000);

	phy_read_reg(dev, 9, 0x0, &value);
	//	printf("Mode 1 : 0x%08x\n", value);
	printf("Ready, please test......\n");
	while (1)
	{
		printf("please input 'q' to quit:");
		scanf("%c", &k);
		safe_flush(stdin);
		if (k == 'q')
			break;
	}
	phy_write_reg(dev, 31, 0x0, 0x0000); //page 0
	phy_write_reg(dev, 9, 0x0, 0x0000);
	//phy_read_reg(dev, 9, 0x0, &value);
	//	printf("Mode 1 : 0x%08x\n", value);
}

void do_b(struct pci_dev *dev)
{
	char k;
	//u16 value;
	phy_write_reg(dev, 31, 0x0, 0x0000); //page 0
	phy_write_reg(dev, 9, 0x0, 0x4000);

	//phy_read_reg(dev, 9, 0x0, &value);
	//printf("Mode 2 : 0x%08x\n", value);
	printf("Ready, please test......\n");
	while (1)
	{
		printf("please input 'q' to quit:");
		scanf("%c", &k);
		safe_flush(stdin);
		if (k == 'q')
			break;
	}
	phy_write_reg(dev, 31, 0x0, 0x0000); //page 0
	phy_write_reg(dev, 9, 0x0, 0x0000);
	//	phy_read_reg(dev, 9, 0x0, &value);
	//	printf("Mode 2 : 0x%08x\n", value);
}

void do_c(struct pci_dev *dev)
{
	char k;
	//u16 value;
	phy_write_reg(dev, 31, 0x0, 0x0000); //page 0
	phy_write_reg(dev, 9, 0x0, 0x8000);

	//	phy_read_reg(dev, 9, 0x0, &value);
	//	printf("Mode 2 : 0x%08x\n", value);
	printf("Ready, please test......\n");
	while (1)
	{
		printf("please input 'q' to quit:");
		scanf("%c", &k);
		safe_flush(stdin);
		if (k == 'q')
			break;
	}
	phy_write_reg(dev, 31, 0x0, 0x0000); //page 0
	phy_write_reg(dev, 9, 0x0, 0x0000);
	//phy_read_reg(dev, 9, 0x0, &value);
	//printf("Mode 2 : 0x%08x\n", value);
}

void do_d(struct pci_dev *dev)
{
	char k;
	//	u16 value;
	phy_write_reg(dev, 31, 0x0, 0x0000); //page 0
	phy_write_reg(dev, 24, 0x0, 0x2388);
	phy_write_reg(dev, 9, 0x0, 0x0000);
	phy_write_reg(dev, 0, 0x0, 0x2100);

	//phy_read_reg(dev, 0, 0x0, &value);
	//printf("Mode 4 : 0x%08x\n", value);
	printf("Ready, please test......\n");
	while (1)
	{
		printf("please input 'q' to quit:");
		scanf("%c", &k);
		safe_flush(stdin);
		if (k == 'q')
			break;
	}
	phy_write_reg(dev, 31, 0x0, 0x0000); //page 0
	phy_write_reg(dev, 24, 0x0, 0x2188);
	phy_write_reg(dev, 9, 0x0, 0x0200);
	phy_write_reg(dev, 0, 0x0, 0x9200);
	//	phy_read_reg(dev, 0, 0x0, &value);
	//	printf("Mode 4 : 0x%08x\n", value);
}

void do_e(struct pci_dev *dev)
{
	char k;
	//	u16 value;
	phy_write_reg(dev, 31, 0x0, 0x0000); //page 0
	phy_write_reg(dev, 24, 0x0, 0x2288);
	phy_write_reg(dev, 9, 0x0, 0x0000);
	phy_write_reg(dev, 0, 0x0, 0x2100);

	//	phy_read_reg(dev, 0, 0x0, &value);
	//	printf("Mode 5 : 0x%08x\n", value);
	printf("Ready, please test......\n");
	while (1)
	{
		printf("please input 'q' to quit:");
		scanf("%c", &k);
		safe_flush(stdin);
		if (k == 'q')
			break;
	}
	phy_write_reg(dev, 31, 0x0, 0x0000); //page 0
	phy_write_reg(dev, 24, 0x0, 0x2188);
	phy_write_reg(dev, 9, 0x0, 0x0200);
	phy_write_reg(dev, 0, 0x0, 0x9200);
	//	phy_read_reg(dev, 0, 0x0, &value);
	//	printf("Mode 5 : 0x%08x\n", value);
}

void do_f(struct pci_dev *dev)
{
	char k;
	//	u16 value;
	phy_write_reg(dev, 31, 0x0, 0x0000); //page 0
	phy_write_reg(dev, 9, 0x0, 0x0000);
	phy_write_reg(dev, 4, 0x0, 0x0061);
	phy_write_reg(dev, 25, 0x0, 0x0043); //10Mbps waveforn without EEE
	phy_write_reg(dev, 0, 0x0, 0x9200);  //PHY reset

	phy_write_reg(dev, 31, 0x0, 0x0c80);    //page c80
	phy_write_reg(dev, 18, 0x0c80, 0x0115); //pseudo-random pattern
	phy_write_reg(dev, 16, 0x0c80, 0x5a21); //enable the packet generator to output pseudo-random pattern

	//	phy_read_reg(dev, 16, 0x0c80, &value);
	//	printf("Mode 6 : 0x%08x\n", value);
	printf("Ready, please test......\n");
	while (1)
	{
		printf("please input 'q' to quit:");
		scanf("%c", &k);
		safe_flush(stdin);
		if (k == 'q')
			break;
	}
	phy_write_reg(dev, 31, 0x0, 0x0c80);    //page c80
	phy_write_reg(dev, 16, 0x0c80, 0x5a00); //disable packet generator
	phy_write_reg(dev, 31, 0x0, 0x0000);    //page 0
	phy_write_reg(dev, 4, 0x0, 0x01e1);
	phy_write_reg(dev, 9, 0x0, 0x0200);
	phy_write_reg(dev, 0, 0x0, 0x9200); //PHY reset
	//	phy_read_reg(dev, 0, 0x0, &value);
	//	printf("Mode 6 : 0x%08x\n", value);
}

void do_g(struct pci_dev *dev)
{
	char k;
	//	u16 value;

	phy_write_reg(dev, 31, 0x0, 0x0000); //page 0
	phy_write_reg(dev, 9, 0x0, 0x0000);
	phy_write_reg(dev, 4, 0x0, 0x0061);
	phy_write_reg(dev, 25, 0x0, 0x0043); //10Mbps waveforn without EEE
	phy_write_reg(dev, 0, 0x0, 0x9200);  //PHY reset

	phy_write_reg(dev, 31, 0x0, 0x0c80);    //page c80
	phy_write_reg(dev, 18, 0x0c80, 0x0015); //fixed pattern
	phy_write_reg(dev, 16, 0x0c80, 0xff21); //enable the packet generator to output all "1" pattern

	//	phy_read_reg(dev, 16, 0x0c80, &value);
	//	printf("Mode 6 : 0x%08x\n", value);
	printf("Ready, please test......\n");
	while (1)
	{
		printf("please input 'q' to quit:");
		scanf("%c", &k);
		safe_flush(stdin);
		if (k == 'q')
			break;
	}
	phy_write_reg(dev, 31, 0x0, 0x0c80);    //page c80
	phy_write_reg(dev, 16, 0x0c80, 0x5a00); //disable packet generator
	phy_write_reg(dev, 31, 0x0, 0x0000);    //page 0
	phy_write_reg(dev, 4, 0x0, 0x01e1);
	phy_write_reg(dev, 9, 0x0, 0x0200);
	phy_write_reg(dev, 0, 0x0, 0x9200); //PHY reset
	//	phy_read_reg(dev, 0, 0x0, &value);
	//	printf("Mode 6 : 0x%08x\n", value);
}

void do_h(struct pci_dev *dev)
{
	char k;
	//	u16 value;

	phy_write_reg(dev, 31, 0x0, 0x0000); //page 0
	phy_write_reg(dev, 9, 0x0, 0x0000);
	phy_write_reg(dev, 4, 0x0, 0x0061);
	phy_write_reg(dev, 25, 0x0, 0x0043); //10Mbps waveforn without EEE
	phy_write_reg(dev, 0, 0x0, 0x9200);  //PHY reset

	phy_write_reg(dev, 31, 0x0, 0x0c80);    //page c80
	phy_write_reg(dev, 18, 0x0c80, 0x0015); //fixed pattern
	phy_write_reg(dev, 16, 0x0c80, 0x0021); //enable the packet generator to output all "0" pattern

	//	phy_read_reg(dev, 16, 0x0c80, &value);
	//	printf("Mode 6 : 0x%08x\n", value);
	printf("Ready, please test......\n");
	while (1)
	{
		printf("please input 'q' to quit:");
		scanf("%c", &k);
		safe_flush(stdin);
		if (k == 'q')
			break;
	}
	phy_write_reg(dev, 31, 0x0, 0x0c80);    //page c80
	phy_write_reg(dev, 16, 0x0c80, 0x5a00); //disable packet generator
	phy_write_reg(dev, 31, 0x0, 0x0000);    //page 0
	phy_write_reg(dev, 4, 0x0, 0x01e1);
	phy_write_reg(dev, 9, 0x0, 0x0200);
	phy_write_reg(dev, 0, 0x0, 0x9200); //PHY reset
	//	phy_read_reg(dev, 0, 0x0, &value);
	//	printf("Mode 6 : 0x%08x\n", value);
}

/**
 *  emnic_read_phy_mdi - Reads a value from a specified PHY register without
 *  the SWFW lock
 *  @hw: pointer to hardware structure
 *  @reg_addr: 32 bit address of PHY register to read
 *  @phy_data: Pointer to read data from PHY register
 **/
u32 emnic_phy_read_reg_mdi(struct pci_dev *dev,
							u32 reg_addr,
							u32 device_type,
							u16 phy_addr)
{
	u32 command;
	u32 status = 0;

	//phy_id = 0;
	device_type = 0;
	/* setup and write the address cycle command */
	command = EMNIC_MSCA_RA(reg_addr) |
		EMNIC_MSCA_PA(phy_addr) |
		EMNIC_MSCA_DA(device_type);
	write_reg(dev, EMNIC_MSCA, command);

	command = EMNIC_MSCC_CMD(EMNIC_MSCA_CMD_READ) |
			  EMNIC_MSCC_BUSY |
			  EMNIC_MDIO_CLK(6);
	write_reg(dev, EMNIC_MSCC, command);

	/* wait to complete */
	status = po32m(dev, EMNIC_MSCC,
		EMNIC_MSCC_BUSY, ~EMNIC_MSCC_BUSY,
		EMNIC_MDIO_TIMEOUT, 10);
	if (status != 0) {
		printf("PHY address command did not complete.\n");
		return -111;
	}

	/* read data from MSCC */
	//*phy_data = 0xFFFF & read_reg(dev, EMNIC_MSCC);

	return 0xFFFF & read_reg(dev, EMNIC_MSCC);
}

/**
 *  emnic_write_phy_reg_mdi - Writes a value to specified PHY register
 *  without SWFW lock
 *  @hw: pointer to hardware structure
 *  @reg_addr: 32 bit PHY register to write
 *  @device_type: 5 bit device type
 *  @phy_data: Data to write to the PHY register
 **/
u32 emnic_phy_write_reg_mdi(struct pci_dev *dev,
							u32 reg_addr,
							u32 device_type,
							u16 phy_data,
							u32 phy_addr)
{
	u32 command;
	u32 status = 0;

	//phy_id = 0;
	device_type = 0;
	/* setup and write the address cycle command */
	command = EMNIC_MSCA_RA(reg_addr) |
		EMNIC_MSCA_PA(phy_addr) |
		EMNIC_MSCA_DA(device_type);
	write_reg(dev, EMNIC_MSCA, command);

	command = phy_data | EMNIC_MSCC_CMD(EMNIC_MSCA_CMD_WRITE) |
		  EMNIC_MSCC_BUSY | EMNIC_MDIO_CLK(6);
	write_reg(dev, EMNIC_MSCC, command);

	/* wait to complete */
	status = po32m(dev, EMNIC_MSCC,
		EMNIC_MSCC_BUSY, ~EMNIC_MSCC_BUSY,
		EMNIC_MDIO_TIMEOUT, 10);
	if (status != 0) {
		printf("PHY address command did not complete.\n");
		return -111;
	}

	return 0;
}

void usage_for_up_flash(int choose)
{
	fprintf(stderr,
		"\n"
		"   Usage: ./wxtool -F [<image>] [<options><arguments>]\n"
		"     Used to download image via online PCIe, for more detail operation and error information,\n"
		"     please refer to 'Raptor PCI Utils User Manual.doc'.\n"
		"     Please contact your sales or technology support to check sutiable FLASH model and document support.\n"
		"\n"
		"     Please use '--help' option to get detail information of tool.\n"
		"     Please use '--version' option to check the verision number of tool.\n"
		"\n"
		"Support options:\n"
		"   -F\t\t To select an Image File, default is using image/prd_flash_gloden.img\n"
		"   -M\t[SP]\t To select a Flash manufacturer, [0] Winbond, [1] Spanish, [2] SST. Default is SST.\n"
		"   -A\t\t To upgrade image of all of our devices. Or, program will give user a selection when multiple devices were found.\n"
		"   -U\t\t Force to update MAC Address and Serial Number.\n"
		"   -C\t\t 1.Not to update MAC Address and Serial Number for input;2.Use mac and sn store in image\n"
		"   -K\t\t Auto to choose mode.\n"
		"   -D\t[EM]\t To erase efuse.\n"
		"   -T\t\t To use test mode.\n"
		"   -E\t[SP]\t To store TX_EQ in flash (1.support after 0x2000b version).\n"
		"     \t\t if you want to set different vlaue for two ports ,please use wxtool -s slot -E x x x\n"
		"   -S\t\t SN can use any characters no more than 24.\n"
		"   -I\t\t Ignore to check SSL sign.\n"
		"\n");
	if (choose == 1)
		exit(0);
}

static struct pci_dev **
select_devices_2(struct pci_filter *filt, struct pci_access *pacc)
{
	struct pci_dev *z, **a, **b;
	int cnt = 1;

	for (z = pacc->devices; z; z = z->next)
		if (pci_filter_match(filt, z))
			cnt++;

	if (cnt == 1) {
		return NULL;
	}

	a = b = xmalloc(sizeof(struct device *) * cnt);
	for (z = pacc->devices; z; z = z->next)
		if (pci_filter_match(filt, z)) {
			pci_fill_info(z, 
				PCI_FILL_IDENT | 
				PCI_FILL_CLASS | 
				PCI_FILL_BASES | 
				PCI_FILL_SIZES);
			z->size[0]  = (z->size[0] ? z->size[0] : 0x1000);
			*a++ = z;
		}
	*a = NULL;
	return (a-1);
}

int parse_options_for_up_flash(int argc, char **argv)
{
	int i = 1;

	end = 0;
	opt_f_is_set = 0;
	opt_m_is_set = 0;
	opt_a_is_set = 0;
	opt_u_is_set = 0;
	opt_c_is_set = 0;
	opt_p_is_set = 0;
	opt_d_is_set = 0;
	opt_s_is_set = 0;
	opt_s1_is_set = 0;
	opt_i_is_set = 0;

	flash_vendor = 0;
	all_card_num = 1; // default is to upgrade single card
	dev_index = 255;

	struct pci_access *pacc = NULL;
	pacc = pci_alloc(); /* Get the pci_access structure */
	pacc->error = die;

	/* Set all options you want -- here we stick with the defaults */
	pci_init(pacc);     /* Initialize the PCI library */
	pci_scan_bus(pacc); /* We want to get the list of devices */

	struct pci_filter filter;
	struct pci_dev **selected_devices = NULL;
	pci_filter_init(pacc, &filter);

	if (argc == 2)
	{
		if (strcmp(argv[1], "--help") == 0)
			usage_for_up_flash(1);

		if (strcmp(argv[1], "--version") == 0)
		{
			puts("wx_tool  version: " PCIUTILS_VERSION);
			exit(0);
		}

		if (strcmp(argv[1], "-A") == 0)
		{
			/* It's a good option, no need to die */
		}
		else
		{
			printf("\nERROR: There are unknown options in command line!\n\n");
			usage_for_up_flash(1);
		}
	} else if (argc == 1)
		usage_for_up_flash(1);

	while (i < argc && argv[i][0] == '-' && end == 0)
	{
		char *c = argv[i++] + 1;
		char *d;
		if (!c[0] || strchr("sFMK", c[0])) {
			if(!argv[i] || argv[i][0] == '-') {
				printf("Option -%c requires an argument\n", c[0]);
				return -1;
			}
		}

		while (*c)
			switch (*c)
			{
			case 's':
				d = argv[i++];
				//printf("d= %s\n", d);
				d = pci_filter_parse_slot(&filter, d);
				selected_devices = select_devices_2(&filter, pacc);
				if (!selected_devices) {
					printf("Not found any device\n");
					return -1;
				}
				global_dev = *selected_devices;
				if (global_dev->vendor_id != VENDOR_ID && global_dev->vendor_id != WX_VENDOR_ID) {
					printf("It is not our NIC slot.\n");
					return -1;
				}
				if(DEBUG_ON || debug_mode)printf("vender: %04x, device_id: %04x\n", global_dev->vendor_id, global_dev->device_id);
				//printf("c= %s\n", c);
				opt_s1_is_set = 1;
				c++;
				//printf("c= %s\n", c);
				break;
			case 'F': /* Image File */
				strcpy(IMAGE_FILE, argv[i++]);
				if (DEBUG_ON || debug_mode)
					printf("F option is %s\n", IMAGE_FILE);
				opt_f_is_set = 1;
				c++;
				break;

			case 'M': /* Flash Manufacturers */
				if (strlen(argv[i]) != 1) {
					printf("invalid arguments for option -M\n");
					return -1;
				}
				switch(*argv[i++]) {
				case '0':
					flash_vendor = 0;
					break;
				case '1':
					flash_vendor = 1;
					break;
				case '2':
					flash_vendor = 2;
					break;
				default:
					printf("invalid arguments for option -M\n");
					return -1;
				}
				if (DEBUG_ON || debug_mode)
					printf("M option is %d\n", flash_vendor);
				opt_m_is_set = 1;
				c++;
				break;

			case 'A': /* All PCI Device Upgrading */
				opt_a_is_set = 1;
				//all_card_num = atoi(argv[i++]);
				if (DEBUG_ON || debug_mode)
					printf("A option is %0d\n", all_card_num);
				c++;
				break;

			case 'K': /* choose which mode to upgrade */
				opt_k_is_set = 1;
				if (strlen(argv[i]) != 1) {
					printf("invalid arguments for option -K\n");
					return -1;
				}
				switch(*argv[i++]) {
				case '1':
					upgrade_mode = 1;
					break;
				case '2':
					upgrade_mode = 2;
					break;
				case '3':
					upgrade_mode = 3;
					break;
				case '4':
					upgrade_mode = 4;
					break;
				default:
					printf("invalid arguments for option -K\n");
					return -1;
				}
				if (DEBUG_ON || debug_mode)
					printf("K option is %0d\n", upgrade_mode);
				c++;
				break;

			case 'U': /* Force to update MAC and SN */
				opt_u_is_set = 1;
				c++;
				break;

			case 'S': /* unlimit SN */
				opt_s_is_set = 1;
				c++;
				break;

			case 'T': /* not to check id */
				opt_t_is_set = 1;
				c++;
				break;

			case 'R': /* not to check chip version */
				opt_r_is_set = 1;
				c++;
				break;

			case 'C': /* Force not to update MAC and SN */
				opt_c_is_set = 1;
				c++;
				break;

			case 'P': /* 2 PORTS*/
				opt_p_is_set = 1;
				c++;
				break;

			case 'D': /* erase efuse*/
				opt_d_is_set = 1;
				c++;
				break;

			case 'E': /* choose which mode to upgrade */
				opt_e_is_set = 1;
				//ffe_main = atoi(argv[i++]);
				//ffe_pre = atoi(argv[i++]);
				//ffe_post = atoi(argv[i++]);
				if (DEBUG_ON || debug_mode)
					printf("e option is %0d:%0d:%0d\n", ffe_main, ffe_pre, ffe_post);
				c++;
				break;

			case 'I': /* ignore image checksum */
				opt_i_is_set = 1;
				if (DEBUG_ON || debug_mode)
					printf("i option is set\n");
				c++;
				break;

			default: /* Unknown Options */
				printf("\nERROR: There are unknown options in command line!\n\n");
				usage_for_up_flash(1);
			}
	}

	return 0;
}

#ifndef BIT
#define BIT(nr)         (1UL << (nr))
#endif

int check_chip_version(struct pci_dev *dev)
{
	u32 val = 0;

	val = read_reg(dev, 0x10010);

	return (val & BIT(16)) ? 0x41 : 0x42;  // vers B no need do glb rst

#if 0
	u32 card_v = 0x0;
	u8 rdata_0, rdata_1;
	u32 rdata_2, rdata_3, c_sub_id;
	u8 wol = 0, ncsi = 0;
	u32 access_check = 0;

	if (DEBUG_ON || debug_mode) printf("===========check_chip_version============\n");
	access_check = flash_read_dword(dev, 0);
	if ((access_check != 0x5aa51000) && 
		(access_check != 0xffffffff) &&
		(access_check != 0x5aa54000)) {
		printf("Error vlaue addr 0 : %08x\n", access_check);
		printf("Please check your spi hardware.\n");
		return -2;
	} else if (access_check == 0xffffffff) {
		printf("flash is empty,simply confirm as 'B'\n");
		chip_v = 0x42;
		return chip_v;
	}

	//read image version
	card_v = flash_read_dword(dev, 0x13a) & 0x000f00ff;
	if (DEBUG_ON || debug_mode) printf("check_chip_version=card_v: %x\n", card_v);

	//read subsytem id to check ncsi and wol
	rdata_0 = flash_read_dword(dev, 0x100000 - 36);
	if (DEBUG_ON || debug_mode) printf("check_chip_version=rdata_0: %x\n", rdata_0);
	rdata_1 = flash_read_dword(dev, 0x100000 - 35);
	if (DEBUG_ON || debug_mode) printf("check_chip_version=rdata_1: %x\n", rdata_1);
	c_sub_id = rdata_0 << 8 | rdata_1;
	if (DEBUG_ON || debug_mode) printf("The card's sub_id : %04x\n", c_sub_id);
	if (DEBUG_ON || debug_mode) printf("=1=ncsi : %x - wol : %x\n", ncsi, wol);
	if ((c_sub_id & 0x8000) == 0x8000)
		ncsi = 1;
	if ((c_sub_id & 0x4000) == 0x4000)
		wol = 1;
	if (DEBUG_ON || debug_mode) printf("=2=ncsi : %x - wol : %x\n", ncsi, wol);

	//check card's chip version
	if ((card_v < 0x10015) && (card_v != 0x10012) && (card_v != 0x10013)) {
		chip_v = 0x41;//'A'
	} else if (card_v > 0x10015) {
		chip_v = flash_read_dword(dev, 0x100000 - 40) & 0xff;
	} else if ((card_v == 0x10012) || (card_v == 0x10013) || (card_v == 0x10015)) {
		if (wol == 1 || ncsi == 1) {
			rdata_2 = flash_read_dword(dev, 0xbc) & 0xffffffff;
			if (DEBUG_ON || debug_mode) printf("check_chip_version=rdata_2-bc: %x\n", rdata_2);
			if (rdata_2 == 0x02020202)
				chip_v = 0x41;
			else
				chip_v = 0x42;
		} else {
			rdata_3 = flash_read_dword(dev, 0x3c) & 0xffff;
			if (DEBUG_ON || debug_mode) printf("check_chip_version=rdata_3-3c: %x\n", rdata_3);
			if (rdata_3 == 0x280)
				chip_v = 0x42;
			else
				chip_v = 0x41;
		}
	}
	if (DEBUG_ON || debug_mode) printf("===========check_chip_version============\n");
	return chip_v;
#endif

}

int check_image_version(void)
{
	u32 image_v = 0x0;
	u8 rdata_0, rdata_1, rdata_2;
	u32 rdata_3, rdata_4, f_sub_id;
	u8 wol = 0, ncsi = 0;
	FILE *fp_i = NULL;
	if (DEBUG_ON || debug_mode) printf("===========check_image_version============\n");

	fp_i = fopen(IMAGE_FILE, "r");
	if (NULL == fp_i) {
		printf("ERROR: Can't open IMAGE File %s!\n", IMAGE_FILE);
		return 0;
	}
	if (DEBUG_ON || debug_mode) printf("IMAGE File %s!\n", IMAGE_FILE);

	//read image version
	fseek(fp_i, 0x13a, SEEK_SET);
	//image_v = flash_read_dword(dev, 0x13a) & 0x000f00ff;
	fread(&image_v, 2, 2, fp_i);
	image_v = image_v & 0x000f00ff;
	if (DEBUG_ON || debug_mode) printf("check_image_version=image_v: %x\n", image_v);

	//read subsytem id to check ncsi and wol
	fseek(fp_i, -9 * sizeof(u32), SEEK_END);
	fread(&rdata_0, 1, 1, fp_i);
	fread(&rdata_1, 1, 1, fp_i);
	f_sub_id = rdata_0 << 8 | rdata_1;
	if (DEBUG_ON || debug_mode) printf("The image's sub_id : %04x\n", f_sub_id);
	if (DEBUG_ON || debug_mode) printf("=1=ncsi : %x - wol : %x\n", ncsi, wol);
	if ((f_sub_id & 0x8000) == 0x8000)
		ncsi = 1;
	if ((f_sub_id & 0x4000) == 0x4000)
		wol = 1;
	if (DEBUG_ON || debug_mode) printf("=2=ncsi : %x - wol : %x\n", ncsi, wol);

	fseek(fp_i, -10 * sizeof(u32), SEEK_END);
	fread(&rdata_2, 1, 1, fp_i);
	if (DEBUG_ON || debug_mode) printf("check_image_version=rdata_2-fffdc: %x\n", rdata_2);
	fseek(fp_i, 0xbc, SEEK_SET);
	fread(&rdata_3, 2, 2, fp_i);
	if (DEBUG_ON || debug_mode) printf("check_image_version=rdata_3-bc: %x\n", rdata_3);
	fseek(fp_i, 0x3c, SEEK_SET);
	fread(&rdata_4, 1, 2, fp_i);
	if (DEBUG_ON || debug_mode) printf("check_image_version=rdata_4-3c: %x\n", rdata_4);

	//check card's chip version
	if ((image_v < 0x10015) && (image_v != 0x10012) && (image_v != 0x10013)) {
		f_chip_v = 0x41;//'A'
	} else if (image_v > 0x10015) {
		//f_chip_v = flash_read_dword(dev, 0x100000 - 40);
		f_chip_v = rdata_2 & 0xff;
	} else if ((image_v == 0x10012) || (image_v == 0x10013) || (image_v == 0x10015)) {
		if (wol == 1 || ncsi == 1) {
			//rdata_3 = flash_read_dword(dev, 0xbc);
			rdata_3 = rdata_3 & 0xffffffff;
			if (rdata_3 == 0x02020202)
				f_chip_v = 0x41;
			else
				f_chip_v = 0x42;
		} else {
			//rdata_4 = flash_read_dword(dev, 0x3c);
			rdata_4 = rdata_4 & 0xffff;
			if (rdata_4 == 0x280)
				f_chip_v = 0x42;
			else
				f_chip_v = 0x41;
		}
	}
	fclose(fp_i);
	if (DEBUG_ON || debug_mode) printf("===========check_image_version============\n");
	return f_chip_v;
}

u32 emnic_check_internal_phy_id(struct pci_dev *dev)
{
	u16 phy_id_high = 0;
	u16 phy_id_low = 0;
	u16 phy_id = 0;

	phy_read_reg(dev, 2, 0, &phy_id_high);
	phy_id = phy_id_high << 6;
	phy_read_reg(dev, 3, 0, &phy_id_low);
	phy_id |= (phy_id_low & 0xFFFFFC00U) >> 10;

	if (0x732 != phy_id) {
		printf("internal phy id 0x%x not supported.\n", phy_id);
		return -1;
	} 
	return 0;
}


int check_image_checksum()
{
	char cmd[512] = "";
	char SIG_NAME[512] = "";
	char tmp[128] = "";
	int len = 0;
	int status = -1;

	memset(SIG_NAME, 0, sizeof(SIG_NAME));
	len = strlen(IMAGE_FILE);

	memset(tmp, 0, sizeof(tmp));
	strncpy(tmp, IMAGE_FILE, len - 3);
	sprintf(SIG_NAME, "%ssig", tmp);
	printf("SIG_FILE:%s\n", SIG_NAME);

	printf("\nFILE SHA256 sum:\n");
	memset(cmd, 0, sizeof(cmd));
	sprintf(cmd, "sha256sum %s %s",
			SIG_NAME, IMAGE_FILE);
	status = system(cmd);
	printf("\n");
	if(status)
		return status;
	system("echo '-----BEGIN PUBLIC KEY-----' > key.pub");
	system("echo 'MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAwOCngfqWTYsKUETTbaBk\n32aNdJdmZ6REbBfCx/Y8hSnr8dYhtNRwuo5MJiUKel4Ix29V6Dfh2dr3DiK4ynu6\nkWhnZ50zT58JmiRVYEAi89Dl+a88gssqEPvxvHcARhesaYPSybUhg5ZnK7k7xv3S\nD+d4E2AxVQajxM469HkTOp7dUqL05tEn72aidrELGBBoYEgvRqAPjVD2oEY0ynfL\n5revnjKIasV6UTLSJjKNt/oirp2gDQdsqgCGMSFQPOa11isewWCvWoe26V2hEclJ\nic2Fp+wyhH3GWCTh7sb2PDl2FbLSVVEm3LQLg1hNIQKirI9HzO+y5U11IOiDWmBG\nrwIDAQAB' >> key.pub");
	system("echo '-----END PUBLIC KEY-----' >> key.pub");
	memset(cmd, 0, sizeof(cmd));
	sprintf(cmd, "openssl dgst -sha256 -keyform PEM -verify key.pub -signature %s %s", 
			SIG_NAME, IMAGE_FILE); 
	status = system(cmd);
	system("rm key.pub");

	return status;
}

int check_nic_status(struct pci_dev *dev)
{
	u32 rdata;
	u32 f_rdata;
	if (DEBUG_ON || debug_mode) printf("===========check_image_version============\n");

	//check chip is ok
	rdata = read_reg(dev, 0x10000);
	if (rdata == 0x0) {
		printf("chip is malfunction, all LAN disabled or pcie link is down\n");
		return -1;
	} else if (rdata == 0xffffffff) {
		printf("Pcie link is down or mmap can not access to chip\n");
		return -1;
	}

	//check fw is ready
	rdata = read_reg(dev, 0x10028);
	rdata &= 0x1;
	f_rdata = flash_read_dword(dev, 0x0);
	if (rdata != 0x1) {
		printf("Firmware is not inited\n");
		if (f_rdata == 0xffffffff) {
			printf("Flash is empty\n");
		} else {
			printf("Flash is not empty, fw has something wrong\n");
		}
		return -2;
	}

	if (DEBUG_ON || debug_mode) printf("===========check_image_version============\n");
	return 0;
}

int check_nic_type(struct pci_dev *dev)
{

	if ((dev->vendor_id == VENDOR_ID || dev->vendor_id == WX_VENDOR_ID) && (dev->device_id >> 4) == 0x010) {
		return EM;
	} else if ((dev->vendor_id == VENDOR_ID || dev->vendor_id == WX_VENDOR_ID) && ((dev->device_id) == 0x1001 ||
							(dev->device_id) == 0x2001))
		return SP;

	return 0;
}

int check_image_status(void)
{
	u32 image_v = 0x0;
	FILE *fp_i = NULL;
	int i = 0;
	word sd_id, sv_id, d_id, v_id;
	int nic_type = 0;
	u32 option_rom = 0;
	char *name = NULL, *oprom_arch = NULL;
	u8 wol = 0, ncsi = 0, chip_v = 0;
	struct card_items items;
	u32 oem_en, oem_v1, oem_v2, oem_v3, oem_v4, oem_v5;
	u16 pos = 0;
	u16 data=0, data1=0, data2=0;

	fp_i = fopen(IMAGE_FILE, "r");
	if (NULL == fp_i) {
		printf("ERROR: Can't open IMAGE File %s!\n", IMAGE_FILE);
		return 0;
	}
	if (DEBUG_ON || debug_mode) printf("IMAGE File %s!\n", IMAGE_FILE);

	/*check deive_id and vendor_id*/
	fseek(fp_i, 0xc, SEEK_SET);
	fread(&pos, 2, 1, fp_i);
	while (pos < 0x1000) {
		fseek(fp_i, pos, SEEK_SET);
		fread(&data, 2, 1, fp_i);
		if (data == 0xff00) {
			break;
		}
		pos += 0x10;
		if (pos > 0x1000){
			printf("image error\n");
			goto exit;
		}
	}
	fseek(fp_i, pos+0x4, SEEK_SET);
	fread(&data, 2, 1, fp_i);
	if(data != 0) {
		printf("image error\n");
		goto exit;
	}

	fseek(fp_i, pos+0xc, SEEK_SET);
	fread(&data1, 2, 1, fp_i);
	v_id = data1;
	fread(&data1, 2, 1, fp_i);
	d_id = data1;

	/*check sub_id and sub_vid*/
	pos += 0x10;
	while (pos < 0x1000) {
		fseek(fp_i, pos, SEEK_SET);
		fread(&data, 2, 1, fp_i);
		if (data == 0xff00) {
			break;
		}
		pos += 0x10;
		if (pos > 0x1000) {
			printf("image error\n");
			goto exit;
		}
	}

	fseek(fp_i, pos+0x4, SEEK_SET);
	fread(&data, 2, 1, fp_i);
	if(data != 0x2c) {
		printf("image error\n");
		goto exit;
	}

	fseek(fp_i, pos+0xc, SEEK_SET);
	fread(&data1, 2, 1, fp_i);
	sv_id = data1;
	fread(&data1, 2, 1, fp_i);
	sd_id = data1;

	if ((d_id == 0x1001) || ((d_id & 0xfff0) == 0x2000))
		nic_type = SP;
	else
		nic_type = EM;

	//read image version
	fseek(fp_i, 0x13a, SEEK_SET);
	//image_v = flash_read_dword(dev, 0x13a) & 0x000f00ff;
	fread(&image_v, 2, 2, fp_i);

	fseek(fp_i, 0xa000a, SEEK_SET);
	fread(&option_rom, 2, 2, fp_i);
	option_rom = option_rom & 0xffff;
	if (option_rom == 0xaa64)
		oprom_arch = "arm64";
	else if (option_rom == 0xffff)
		oprom_arch = "no";
	else
		oprom_arch = "arm64/x86";

	fseek(fp_i, 0xff000, SEEK_SET);
	fread(&oem_en, 2, 2, fp_i);
	oem_en = cpu_to_le32(oem_en) & 0xffff;
	if ((oem_en & 0xffff) == 0x55aa)
		oem_en = 1;

	printf("\n");

	//image version
	printf("fw version:\t%08x\n", image_v);

	if (nic_type == EM) {
		//chip version
		chip_v = check_image_version();
		printf("img_v:\t%C\n", chip_v);
	}

	wol = 0x4000 & sd_id ? 1 : 0;
	ncsi = 0x8000 & sd_id ? 1 : 0;

	//wol nsci
	printf("wol:\t%s\n", 0x4000 & sd_id ? "enable":"disable");
	//chip version
	printf("ncsi:\t%s\n", 0x8000 & sd_id ? "enable":"disable");
	//platform
	printf("oprom arch:\t%s\n", oprom_arch);
	//image_name
	for (i = 0;i < 256; i++) {
		items = cards[i];
		if ((items.subs_id & 0xfff) == (sd_id & 0xfff)
			&& items.dev_id == d_id)
			name = items.name;
	}

	if (nic_type == EM) {
		if (v_id != WX_VENDOR_ID || sv_id != WX_VENDOR_ID) {
			printf("VendorId: %04x\n", v_id);
			printf("DeviceId: %04x\n", d_id);
			printf("Subsystem VendorId: %04x\n", sv_id);
			printf("Subsystem DeviceId: %04x\n", sd_id);
		} else if (name != NULL) {
			printf("image_name:\t%s", name);
			if (chip_v == 0x42)
				printf("_B");
			printf("_%x", image_v);
			if (strcmp(oprom_arch, "arm64") == 0)
				printf(".arm64");
			else if (strcmp(oprom_arch, "no") == 0)
				printf(".no_pxe");
			if (wol == 1)
				printf(".wol");
			if (ncsi == 1)
				printf(".ncsi");
			printf(".img\n");
		} else {
			printf("DeviceId: %04x\n", d_id);
			printf("Subsystem DeviceId: %04x\n", sd_id);
		}
	} else if (nic_type == SP) {
		printf("image_name:");
		if (image_v >= 0x2000b) {
			if (d_id == 0x2001)
				printf("RP2000");
			else if (d_id == 0x1001)
				printf("RP1000");
		} else {
			if (d_id == 0x2001)
				printf("rp2000");
			else if (d_id == 0x1001)
				printf("rp1000");
		}
		if (strcmp(oprom_arch, "arm64") == 0)
			printf("[arm64]");
		else if (strcmp(oprom_arch, "no") == 0)
			printf("[arm64/x86]");
		if (wol == 1)
			printf("[wol]");
		if (ncsi == 1)
			printf("[ncsi]");
		printf("\n");
	}
	
	fseek(fp_i, 0xff000, SEEK_SET);
	if (oem_en == 1) {
		fread(&oem_v1, 2, 2, fp_i);
		fread(&oem_v2, 2, 2, fp_i);
		fread(&oem_v3, 2, 2, fp_i);
		fread(&oem_v4, 2, 2, fp_i);
		fread(&oem_v5, 2, 2, fp_i);
		printf("oem conf: [%08x]-[%08x]-[%08x]-[%08x]-[%08x]\n", oem_v1, oem_v2, oem_v3, oem_v4, oem_v5);
	}
exit:
	fclose(fp_i);
	return 0;
}

int wx_nic_int(struct pci_dev *dev)
{
	//word sv_id = 0, sd_id = 0;
	//char classbuf[128], vendbuf[128], devbuf[128], svbuf[128], sdbuf[128];
	//int nic_type = 0;
	//u32 option_rom = 0;
	struct wx_nic* nic = NULL;
	//struct device *d;
	

	//get_subid(d, &sv_id, &sd_id);
	//option_rom = flash_read_dword(dev, 0xa000a) & 0xffff;

	nic->cab_0 = read_reg(dev, 0x10000);
	nic->flash_0 = flash_read_dword(dev, 0x0);
	nic->fw_version = flash_read_dword(dev, 0x13a);
	nic->chip_v = check_chip_version(dev);
	//nic->ncsi = 0x8000 & sd_id ? 1:0;
	//nic->wol = 0x4000 & sd_id ? 1:0;
	//nic->oprom_arch = (option_rom == 0xaa64)? "arm64" : "arm64/x86";

	return 0;
}

int vpd_sn_change_t(struct pci_dev *dev, char *SN)
{
	char id_str[40] = "";
	char pn_str[20] = "";
	char sn_str[32] = "";
	char all_str[256] = " ";
	int id_str_len = 0;
	int pn_str_len = 0;
	int sn_str_len = 0;
	u8 rv_len = 5;
	FILE *fd = NULL;
	int i = 0;
	u8 test = 0;
	u16 test1 = 0;
	int vpd_ro_len_idx = 0;
	int vpd_offset = 0;

	u8 nic_type = check_nic_type(dev);

	if (nic_type == EM)
		vpd_offset = 0x60;
	else if (nic_type == SP)
		vpd_offset = 0x500;
	else
		printf("vpd not on\n");

	//memset(sn_str, 0, sizeof(sn_str));
	printf("\n");
	int len = strlen(SN);
	if (DEBUG_ON || debug_mode) printf("len : %x\n", len);
	strncpy(sn_str, SN, len);
	printf("vpd_sn_change_t\n");

	//fd = fopen("vpd_tent", "w+");
	/*====================== get image vpd info ====================================== */
	fd = fopen(IMAGE_FILE, "r");

	fseek(fd, vpd_offset, SEEK_SET);
#if 0
	for (i = 0; i < 96; i++) {
		fread(&test, 1, 1, fd);
		printf("test=%x : %x\n", i , test);
	}
#endif
	// get id str info 
	fseek(fd, vpd_offset, SEEK_SET);
	fread(&test, 1, 1, fd);
	fread(&id_str_len, 2, 1, fd);
	if (DEBUG_ON || debug_mode) printf("id_str_len : %x\n", id_str_len);
	for (i = 0; i < id_str_len; i ++) {
		fread(&test, 1, 1, fd);
		id_str[i] = (char)test;
	}
	printf("id_str: %s\n", id_str);

	fseek(fd, vpd_offset + 3 + id_str_len, SEEK_SET);
	fread(&test, 1, 1, fd);
	fread(&vpd_ro_len_idx, 2, 1, fd);
	if (DEBUG_ON || debug_mode) printf("vpd_ro_len_idx : %x\n", vpd_ro_len_idx);

	fread(&test1, 2, 1, fd);
	fread(&pn_str_len, 1, 1, fd);
	if (DEBUG_ON || debug_mode) printf("pn_str_len : %x\n", pn_str_len);
	for (i = 0; i < pn_str_len; i ++) {
		fread(&test, 1, 1, fd);
		pn_str[i] = (char)test;
	}
	printf("pn_str: %s\n", pn_str);
	if (DEBUG_ON || debug_mode) printf("sn_str_len : %x\n", len);
	printf("sn_str: %s\n", sn_str);
#if 0
	fseek(fd, vpd_offset + 3 + 3 + 3 + id_str_len + pn_str_len, SEEK_SET);
	fread(&test1, 2, 1, fd);
	fread(&sn_str_len, 1, 1, fd);
	if (DEBUG_ON || debug_mode) printf("sn_str_len : %x\n", sn_str_len);
	for (i = 0; i < sn_str_len; i ++) {
		fread(&test, 1, 1, fd);
		sn_str[i] = (char)test;
	}
	printf("sn_str: %s\n", sn_str);

	fseek(fd, vpd_offset + 3 + 3 + 3 + 3 + id_str_len + pn_str_len + sn_str_len, SEEK_SET);
	fread(&test1, 2, 1, fd);
	fread(&rv_len, 1, 1, fd);
	if (DEBUG_ON || debug_mode) printf("rv_len : %x\n", rv_len);
#endif
	fclose(fd);
	/*====================== get image vpd info ====================================== */

	/*====================== rebuild vpd tent   ====================================== */
	fd = fopen("vpd_tent", "w+");

	// ID Descriptor
	id_str_len = strlen(id_str);
	if (DEBUG_ON || debug_mode) printf("id_str_len : %x\n", id_str_len);

	u8 tmp[2] = {0};
	tmp[0] = 0x82;
	fwrite(&tmp, 1, 1, fd);
	fwrite(&id_str_len, 2, 1, fd);

	for (i = 0; i < id_str_len; i++)
		fwrite(&id_str[i], 1, 1, fd);

	// VPD-READ
	tmp[0] = 0x90;
	fwrite(&tmp, 1, 1, fd);
	vpd_ro_len_idx = ftell(fd);
	if (DEBUG_ON || debug_mode) printf("vpd_ro_len_idx : %x \n", vpd_ro_len_idx);
	tmp[0] = 0x00;
	fwrite(&tmp, 2, 1, fd);

	// PN
	char tmp_str[4] = "PN";
	fwrite(&tmp_str[0], 1, 1, fd);
	fwrite(&tmp_str[1], 1, 1, fd);
	pn_str_len = strlen(pn_str);
	if (DEBUG_ON || debug_mode) printf("pn_str_len : %x\n", pn_str_len);
	fwrite(&pn_str_len, 1, 1, fd);

	for (i = 0; i < pn_str_len; i++)
		fwrite(&pn_str[i], 1, 1, fd);
	// SN
	char sn_sign[4] = "SN";
	fwrite(&sn_sign[0], 1, 1, fd);
	fwrite(&sn_sign[1], 1, 1, fd);
	sn_str_len = strlen(sn_str);
	if (DEBUG_ON || debug_mode) printf("sn_str_len : %x\n", sn_str_len);
	fwrite(&sn_str_len, 1, 1, fd);

	for (i = 0; i < sn_str_len; i++)
		fwrite(&sn_str[i], 1, 1, fd);
	// RV
	char rv_str[2] = "RV";
	fwrite(&rv_str[0], 1, 1, fd);
	fwrite(&rv_str[1], 1, 1, fd);
	fwrite(&rv_len, 1, 1, fd);

	int chksum_idx = ftell(fd);

	tmp[0] = 0x00;
	fwrite(&tmp, 1, 1, fd);

	for (i = 0; i < rv_len - 1; i++)
		fwrite(&tmp, 1, 1, fd);
	int tmp1 = ftell(fd);
	int vpd_ro_len = tmp1 - vpd_ro_len_idx - 2;
	if (DEBUG_ON || debug_mode) printf("vpd_ro_len : %x \n", vpd_ro_len);
	if (DEBUG_ON || debug_mode) printf("len : %x \n", tmp1);
	fseek(fd, vpd_ro_len_idx, SEEK_SET);
	tmp[0] = vpd_ro_len;
	fwrite(&tmp, 2, 1, fd);
	fflush(fd);
	// compute chksum
	int chksum = 0;
	test = 0;
	fseek(fd, 0, SEEK_SET);
	tmp1 = ftell(fd);
	if (DEBUG_ON || debug_mode) printf("len : %x \n", tmp1);

	//chksum_idx = chksum_idx + 2;
	if (DEBUG_ON || debug_mode) printf("chksum_idx : %x \n", chksum_idx);
#if 1
	for (i = 0; i < chksum_idx; i++) {
		fread(&test, 1, 1, fd);
		//printf("test: %x\n", test);
		chksum = test + chksum;
	}
#endif

	if (DEBUG_ON || debug_mode) printf("=1=chksum : %x \n", chksum);
	chksum = chksum & 0xff;
	if (DEBUG_ON || debug_mode) printf("=2=chksum : %x \n", chksum);

	chksum = (~chksum) + 1;
	if (DEBUG_ON || debug_mode) printf("=3=chksum : %x \n", chksum);
	fseek(fd, chksum_idx, SEEK_SET);
	fwrite(&chksum, 1, 1, fd);

	// End
	fseek(fd, 0, SEEK_END);
	tmp[0] = 0x78;
	fwrite(&tmp, 1, 1, fd);
	int end_len = ftell(fd);
	if (end_len % 4) {
		int pad_len = 4 - end_len % 4;
		for (i = 0; i<pad_len; i++) {
			tmp[0] = 0xff;
			fwrite(&tmp, 1, 1, fd);
		}
	}
	fclose(fd);
	/*====================== rebuild vpd tent   ====================================== */

	// End
}

int vpd_sn_change(struct pci_dev *dev, char *SN)
{
	char id_str[40] = "";
	char pn_str[20] = "";
	char sn_str[32] = "";
	char all_str[256] = " ";
	u16 id_str_len = 0;
	u8 pn_str_len = 0;
	int sn_str_len = 0;
	u8 rv_len = 5;
	FILE *fd = NULL;
	int i = 0;
	u8 test = 0;
	u16 test1 = 0;
	u16 vpd_ro_len_idx = 0;
	int vpd_offset = 0;

	u8 nic_type = check_nic_type(dev);

	if (nic_type == EM)
		vpd_offset = 0x60;
	else if (nic_type == SP)
		vpd_offset = 0x500;
	else
		printf("vpd not on\n");

	//memset(sn_str, 0, sizeof(sn_str));
	int len = strlen(SN);
	printf("\n");
	if (DEBUG_ON || debug_mode) printf("len : %x\n", len);

	strncpy(sn_str, SN, len);
	printf("vpd_sn_change\n");
	//fd = fopen("vpd_tent", "w+");
	/*====================== get image vpd info ====================================== */
	//for (i = 0; i < 0x50; i++) {
	//	test = flash_read_dword(dev, vpd_offset + i);
	//	printf("test-%d: %x\n", i, test);
	//}
	// get id str info 
	//fseek(fd, vpd_offset, SEEK_SET);
	test = flash_read_dword(dev, vpd_offset);
	id_str_len = flash_read_dword(dev, vpd_offset + 1);
	//fread(&test, 1, 1, fd);
	//fread(&id_str_len, 2, 1, fd);
	if (DEBUG_ON || debug_mode) printf("id_str_len : %x\n", id_str_len);
	for (i = 0; i < id_str_len; i ++) {
		//fread(&test, 1, 1, fd);
		test = flash_read_dword(dev, vpd_offset + 3 + i);
		id_str[i] = (char)test;
	}
	printf("id_str: %s\n", id_str);

	//fseek(fd, vpd_offset + 3 + id_str_len, SEEK_SET);
	test = flash_read_dword(dev, vpd_offset + 3 + id_str_len);
	//printf("test %x\n", test);
	//fread(&test, 1, 1, fd);
	vpd_ro_len_idx = flash_read_dword(dev, vpd_offset + 3 + id_str_len + 1);
	//fread(&vpd_ro_len_idx, 2, 1, fd);
	if (DEBUG_ON || debug_mode)  printf("vpd_ro_len_idx : %x\n", vpd_ro_len_idx);

	//fread(&test1, 2, 1, fd);
	test1 = flash_read_dword(dev, vpd_offset + 3 + id_str_len + 3);
	//fread(&pn_str_len, 1, 1, fd);
	pn_str_len = flash_read_dword(dev, vpd_offset + 3 + id_str_len + 3 + 2);
	if (DEBUG_ON || debug_mode)  printf("pn_str_len : %x\n", pn_str_len);
	for (i = 0; i < pn_str_len; i ++) {
		//fread(&test, 1, 1, fd);
		test = flash_read_dword(dev, vpd_offset + 3 + id_str_len + 3 + 3 + i);
		pn_str[i] = (char)test;
	}
	printf("pn_str: %s\n", pn_str);
#if 0

	fseek(fd, vpd_offset + 3 + 3 + 3 + id_str_len + pn_str_len, SEEK_SET);
	fread(&test1, 2, 1, fd);
	fread(&sn_str_len, 1, 1, fd);
	if (DEBUG_ON || debug_mode) printf("sn_str_len : %x\n", sn_str_len);
	for (i = 0; i < sn_str_len; i ++) {
		fread(&test, 1, 1, fd);
		sn_str[i] = (char)test;
	}
	printf("sn_str: %s\n", sn_str);

	fseek(fd, vpd_offset + 3 + 3 + 3 + 3 + id_str_len + pn_str_len + sn_str_len, SEEK_SET);
	fread(&test1, 2, 1, fd);
	fread(&rv_len, 1, 1, fd);
	if (DEBUG_ON || debug_mode) printf("rv_len : %x\n", rv_len);
#endif
	/*====================== get image vpd info ====================================== */

	/*====================== rebuild vpd tent   ====================================== */

	printf("sn_str: %s\n", sn_str);
	fd = fopen("vpd_tent", "w+");

	// ID Descriptor
	id_str_len = strlen(id_str);
	if (DEBUG_ON || debug_mode) printf("id_str_len : %x\n", id_str_len);

	u8 tmp[2] = {0};
	tmp[0] = 0x82;
	fwrite(&tmp, 1, 1, fd);
	fwrite(&id_str_len, 2, 1, fd);

	for (i = 0; i < id_str_len; i++)
		fwrite(&id_str[i], 1, 1, fd);

	// VPD-READ
	tmp[0] = 0x90;
	fwrite(&tmp, 1, 1, fd);
	vpd_ro_len_idx = ftell(fd);
	if (DEBUG_ON || debug_mode) printf("vpd_ro_len_idx : %x \n", vpd_ro_len_idx);
	tmp[0] = 0x00;
	fwrite(&tmp, 2, 1, fd);

	// PN
	char tmp_str[4] = "PN";
	fwrite(&tmp_str[0], 1, 1, fd);
	fwrite(&tmp_str[1], 1, 1, fd);
	pn_str_len = strlen(pn_str);
	if (DEBUG_ON || debug_mode) printf("pn_str_len : %x\n", pn_str_len);
	fwrite(&pn_str_len, 1, 1, fd);

	for (i = 0; i < pn_str_len; i++)
		fwrite(&pn_str[i], 1, 1, fd);
	// SN
	char sn_sign[4] = "SN";
	fwrite(&sn_sign[0], 1, 1, fd);
	fwrite(&sn_sign[1], 1, 1, fd);
	sn_str_len = strlen(sn_str);
	if (DEBUG_ON || debug_mode) printf("sn_str_len : %x\n", sn_str_len);
	fwrite(&sn_str_len, 1, 1, fd);

	for (i = 0; i < sn_str_len; i++)
		fwrite(&sn_str[i], 1, 1, fd);
	// RV
	char rv_str[2] = "RV";
	fwrite(&rv_str[0], 1, 1, fd);
	fwrite(&rv_str[1], 1, 1, fd);
	fwrite(&rv_len, 1, 1, fd);

	int chksum_idx = ftell(fd);

	tmp[0] = 0x00;
	fwrite(&tmp, 1, 1, fd);

	for (i = 0; i < rv_len - 1; i++)
		fwrite(&tmp, 1, 1, fd);
	int tmp1 = ftell(fd);
	int vpd_ro_len = tmp1 - vpd_ro_len_idx - 2;
	if (DEBUG_ON || debug_mode) printf("vpd_ro_len : %x \n", vpd_ro_len);
	if (DEBUG_ON || debug_mode) printf("len : %x \n", tmp1);
	fseek(fd, vpd_ro_len_idx, SEEK_SET);
	tmp[0] = vpd_ro_len;
	fwrite(&tmp, 2, 1, fd);
	fflush(fd);
	// compute chksum
	int chksum = 0;
	test = 0;
	fseek(fd, 0, SEEK_SET);
	tmp1 = ftell(fd);
	if (DEBUG_ON || debug_mode) printf("len : %x \n", tmp1);

	//chksum_idx = chksum_idx + 2;
	if (DEBUG_ON || debug_mode) printf("chksum_idx : %x \n", chksum_idx);
#if 1
	for (i = 0; i < chksum_idx; i++) {
		fread(&test, 1, 1, fd);
		//printf("test: %x\n", test);
		chksum = test + chksum;
	}
#endif

	if (DEBUG_ON || debug_mode) printf("=1=chksum : %x \n", chksum);
	chksum = chksum & 0xff;
	if (DEBUG_ON || debug_mode) printf("=2=chksum : %x \n", chksum);

	chksum = (~chksum) + 1;
	if (DEBUG_ON || debug_mode) printf("=3=chksum : %x \n", chksum);
	fseek(fd, chksum_idx, SEEK_SET);
	fwrite(&chksum, 1, 1, fd);

	// End
	fseek(fd, 0, SEEK_END);
	tmp[0] = 0x78;
	fwrite(&tmp, 1, 1, fd);
	int end_len = ftell(fd);
	if (end_len % 4) {
		int pad_len = 4 - end_len % 4;
		for (i = 0; i<pad_len; i++) {
			tmp[0] = 0xff;
			fwrite(&tmp, 1, 1, fd);
		}
	}
	fclose(fd);
	/*====================== rebuild vpd tent   ====================================== */

	// End
}
#endif
