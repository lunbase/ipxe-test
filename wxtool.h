#ifndef _FUNCTION_H_
#define _FUNCTION_H_

#include <errno.h>
#include <stdarg.h>
#include "functions.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include "lib/pci.h"
#include "pciutils.h"

#include "xgigabit.h"
#include "Gigabit.h"
#include "lspci_wx.h"


const char program_name[] = "wxtool";

#ifdef  SPI_CLK_DIV
#undef  SPI_CLK_DIV
#define SPI_CLK_DIV 3
#endif

#define RES_NUM_MAX     (6)
#define RES_PCI     (RES_NUM_MAX + 1)
#define RES_PCICAP  (RES_NUM_MAX + 2)
#define RES_PCIECAP (RES_NUM_MAX + 3)

static int force;
static struct pci_access *pacc_wx;

static u32 opt_access = 0;


extern int nic_find_init(void);

extern unsigned int cab_read(struct pci_dev *devs, int addr);
extern unsigned int cab_write(struct pci_dev *devs, int addr, int value);
extern unsigned int pci_read_reg(struct pci_dev *devs, int addr);
extern unsigned int pci_write_reg(struct pci_dev *devs,int addr, int value);
extern unsigned int flash_read(struct pci_dev *devs, int addr);
extern int flash_write(struct pci_dev *devs, int addr,int value);
extern int flash_erase(struct pci_dev *devs, int sec_num,int ind);
extern unsigned int mdio_read(struct pci_dev *devs, int addr, int phy_addr);
extern unsigned int mdio_write(struct pci_dev *devs, int addr, int value, int phy_addr);

extern int mac_change(struct pci_dev *devs, long int MAC_ADDR_T);
extern int sn_change(struct pci_dev *devs);
extern void wavetool(struct pci_dev *devs);
extern int sp_set_txeq(struct pci_dev *devs, u32 main, u32 pre, u32 post);

double cal_tmp(double code);
double cal_code(double code);
void show_nic_info(struct pci_dev *devs);
int w_a_check_workaround(struct pci_dev *devs);
extern int dump_image(struct pci_dev *dev);
extern int update_region(struct pci_dev *dev);


int flash_erase(struct pci_dev *devs, int sec_num,int ind)
{
	struct pci_dev    *dev = NULL;
	u8                 status = 0;
	u32                i = 0;

	/* Select Flash Manufacturer */
	opt_m_is_set = 1; // Force to SST
	flash_vendor = FLASH_SST;
	if (flash_vendor != FLASH_WINBOND && 
		flash_vendor != FLASH_SPANISH && 
		flash_vendor != FLASH_SST)
		flash_vendor = FLASH_SST;
	dev = devs; // select current PCI device

	// Check PCIe Memory Space Enable
	status = pci_read_byte(dev, 0x4);
	pci_write_byte(dev, 0x4, (status | 0x2)); // set "Memory Space Enable" field

	if (flash_vendor == FLASH_SST)
	// Note: for SST flash, need to unlock FLASH after power-on
	{
		status = fmgr_usr_cmd_op(dev, 0x6);  // write enable
		usleep(500 * 1000); // 500 ms
		//if (DEBUG_ON || debug_mode) printf("SST flash write enable user command, return status = %0d\n", status);
		status = fmgr_usr_cmd_op(dev, 0x98); // global protection un-lock
		//if (DEBUG_ON || debug_mode) printf("SST flash un-lock protection user command, return status = %0d\n", status);
		usleep(500 * 1000); // 500 ms
	}

	if (flash_vendor == FLASH_SST)
	{
		if(ind == 1){
			for(i = 0; i <= 255; i++)
			{
				write_reg(dev, SPI_CMD_CFG1_ADDR, 0x0103c720);
				flash_erase_sector(dev, i*4*1024);
				usleep(20 * 1000); // 20 ms
				write_reg(dev, SPI_CMD_CFG1_ADDR, 0x0103c7d8);
			}
		}else{
			if(sec_num < 0 || sec_num > 255)
			{
				printf("Invalid sec_num\n");
				return -1;
			}
			write_reg(dev, SPI_CMD_CFG1_ADDR, 0x0103c720);
			//sec_num += 240;
			flash_erase_sector(dev, sec_num*4*1024);
			usleep(20 * 1000); // 20 ms
			write_reg(dev, SPI_CMD_CFG1_ADDR, 0x0103c7d8);
		}
	}

	printf("\n");
	//pci_cleanup(pacc_wx); /* Close everything */
	return 0;
}

int flash_write(struct pci_dev *devs, int addr, int value)
{
	struct pci_dev    *dev = NULL;

	u32                status = 0;
	u32 		read_data = 0;

	dev = devs; // select current PCI device

	// Check PCIe Memory Space Enable
	status = pci_read_byte(dev, 0x4);
	pci_write_byte(dev, 0x4, (status | 0x2)); // set "Memory Space Enable" field

	status = fmgr_usr_cmd_op(dev, 0x6);  // write enable
	status = fmgr_usr_cmd_op(dev, 0x98); // global protection un-lock
	status = flash_write_dword(dev, addr, value);
	if (status)
	{
		printf("\nPCI Utils tool report fatal error:\n");
		printf("ERROR: Program 0x%08x @addr: 0x%08x is failed !!\n", value, addr);
		read_data = flash_read_dword(dev, addr);
		printf("       Read data from Flash is: 0x%08x\n", read_data);
		return status;
	}

	return 0;
}

unsigned int flash_read(struct pci_dev *devs, int addr)
{
	struct pci_dev    *dev = NULL;
	u8                 status = 0;
	u32 value;

	dev = devs; // select current PCI device
	if (DEBUG_ON || debug_mode) printf("read==bus=%0x, dev=%0x, func=%0x\n", dev->bus, dev->dev, dev->func);

	// Check PCIe Memory Space Enable
	status = pci_read_byte(dev, 0x4);
	pci_write_byte(dev, 0x4, (status | 0x2)); // set "Memory Space Enable" field

	value = flash_read_dword(dev, addr);
	printf("addr: %08x - value: %08x\n", addr,value);

	return value;
}

unsigned int pci_write_reg(struct pci_dev *devs, int addr, int value)
{
	struct pci_dev    *dev = NULL;
	u8                 status = 0;

	dev = devs; // select current PCI device
	if (DEBUG_ON || debug_mode) printf("read==bus=%0x, dev=%0x, func=%0x\n", dev->bus, dev->dev, dev->func);

	// Check PCIe Memory Space Enable
	status = pci_read_byte(dev, 0x4);
	pci_write_byte(dev, 0x4, (status | 0x2)); // set "Memory Space Enable" field

	write_reg(dev, addr, value);

	return value;
}

unsigned int pci_read_reg(struct pci_dev *devs, int addr)
{
	struct pci_dev    *dev = NULL;
	u8                 status = 0;
	int value;

	dev = devs; // select current PCI device
	if (DEBUG_ON || debug_mode) printf("read==bus=%0x, dev=%0x, func=%0x\n", dev->bus, dev->dev, dev->func);

	// Check PCIe Memory Space Enable
	status = pci_read_byte(dev, 0x4);
	pci_write_byte(dev, 0x4, (status | 0x2)); // set "Memory Space Enable" field

	value = read_reg(dev, addr);

	printf("addr: %08x - value: %08x\n", addr,value);

	return value;
}

unsigned int cab_read(struct pci_dev *devs, int addr)
{
	struct pci_dev    *dev = NULL;
	u8                 status = 0;
	int value;

	dev = devs; // select current PCI device
	if (DEBUG_ON || debug_mode) printf("read==bus=%0x, dev=%0x, func=%0x\n", dev->bus, dev->dev, dev->func);

	// Check PCIe Memory Space Enable
	status = pci_read_byte(dev, 0x4);
	pci_write_byte(dev, 0x4, (status | 0x2)); // set "Memory Space Enable" field

	usleep(1000);
	value = emnic_flash_read_cab(dev, addr, dev->func);

	printf("addr: %08x - value: %08x\n", addr,value);

	return value;
}

unsigned int cab_write(struct pci_dev *devs, int addr, int value)
{
	struct pci_dev    *dev = NULL;
	u8                 status = 0;

	dev = devs; // select current PCI device
	if (DEBUG_ON || debug_mode) printf("read==bus=%0x, dev=%0x, func=%0x\n", dev->bus, dev->dev, dev->func);

	// Check PCIe Memory Space Enable
	status = pci_read_byte(dev, 0x4);
	pci_write_byte(dev, 0x4, (status | 0x2)); // set "Memory Space Enable" field

	emnic_flash_write_cab(dev, addr, value, dev->func);
	usleep(1000);

	return value;
}

unsigned int mdio_read(struct pci_dev *devs, int addr, int phy_addr)
{
	struct pci_dev    *dev = NULL;
	u8                 status = 0;
	int value;

	dev = devs; // select current PCI device
	if (DEBUG_ON || debug_mode) printf("read==bus=%0x, dev=%0x, func=%0x\n", dev->bus, dev->dev, dev->func);

	// Check PCIe Memory Space Enable
	status = pci_read_byte(dev, 0x4);
	pci_write_byte(dev, 0x4, (status | 0x2)); // set "Memory Space Enable" field

	usleep(1000);
	value = emnic_phy_read_reg_mdi(dev, addr, 0, phy_addr);

	printf("addr: %08x - value: %08x\n", addr,value);

	return value;
}

unsigned int mdio_write(struct pci_dev *devs, int addr, int value, int phy_addr)
{
	struct pci_dev    *dev = NULL;
	u8                 status = 0;

	dev = devs; // select current PCI device
	if (DEBUG_ON || debug_mode) printf("read==bus=%0x, dev=%0x, func=%0x\n", dev->bus, dev->dev, dev->func);

	// Check PCIe Memory Space Enable
	status = pci_read_byte(dev, 0x4);
	pci_write_byte(dev, 0x4, (status | 0x2)); // set "Memory Space Enable" field

	//printf("addr= %x, value = %x, phy_addr = %x\n", addr, value, phy_id);
	emnic_phy_write_reg_mdi(dev, addr, 0, value, phy_addr);
	usleep(1000);

	return value;
}

int nic_find_init(void)
{
	struct pci_access *pacc_wx = NULL;
	struct pci_dev    *dev = NULL;
	struct pci_dev    *devs[FUNC_NUM][MAX_RAPTOR_CARD_NUM];
	u32                dev_num[FUNC_NUM] = {0,0,0,0};
	u16                i = 0,m = 0;
	
	pacc_wx = pci_alloc();		/* Get the pci_access structure */
	pacc_wx->error = die;
	/* Set all options you want -- here we stick with the defaults */
	pci_init(pacc_wx);		/* Initialize the PCI library */
	pci_scan_bus(pacc_wx);		/* We want to get the list of devices */

	for (i=0; i<MAX_RAPTOR_CARD_NUM; i++) {
		for (m=0; m<FUNC_NUM; m++) {
			devs[m/*PCIe Func ID*/][i/*Card ID*/] = NULL;
		}
	}

	for (dev=pacc_wx->devices; dev; dev=dev->next) { /* Iterate over all devices */
		if ((dev->vendor_id == VENDOR_ID || dev->vendor_id == WX_VENDOR_ID) && (dev->device_id >> 4) == 0x010) {
			devs[dev->func][dev_num[dev->func]] = dev; 
			if (DEBUG_ON || debug_mode) printf("read==bus=%0x, dev=%0x, func=%0x\n", dev->bus, dev->dev, dev->func);
			dev_num[dev->func] += 1;
		}
	}
	if (dev_num[0] == 0) {
		printf("\nNo any cards were found! Program is exited!!\n\n");
		return 1;
	}

	dev = devs[0][0]; // select current PCI device

	return 0;
}

static void usage_for_wxtool(int choose)
{
	if (DEBUG_ON || debug_mode) {
		fprintf(stderr,
		"\n"
		"   Usage: ./wxtool -s xx:xx.x [<options>]\n"
		"     Please use '--help' option to get detail information of tool.\n"
		"     Please use '--version' option to check the verision number of tool.\n"
		"\n"
		"Support options:\n"
		"   -s\t\tTo select an pci slot which can use lspci to see\n"
		"	\t./wxtool -s 02:00.0                             \n"
		"   -r\t\tTo read reg value\n"
		"	\t./wxtool -s 02:00.0 -r 0x10000                            \n"
		"   -w\t\tTo write reg value\n"
		"	\t./wxtool -s 02:00.0 -w 0x11000  0x1                           \n"
		"   -f -r\tTo read flash value\n"
		"	\t./wxtool -s 02:00.0 -f -r 0x160                            \n"
		"   -f -w\tTo write flash value\n"
		"	\t./wxtool -s 02:00.0 -f -w 0x160  0xaa                           \n"
		"   -c -r\tTo read cab value\n"
		"	\t./wxtool -s 02:00.0 -c -r 0x500                            \n"
		"   -c -w\tTo write cab value\n"
		"	\t./wxtool -s 02:00.0 -c -w 0x500  0x1                           \n"
		"   -d -r\tTo read access to mdio\n"
		"	\t./wxtool -s 02:00.0 -d -r 0x0[phy_addr] 0x0[addr]                            \n"
		"   -d -w\tTo write access to mdio\n"
		"	\t./wxtool -s 02:00.0 -d -w 0x0[phy_addr] 0x0[addr] 0x1[value]                  \n"
		"   -e\t\tTo erase flash 4k / sector range(1,256)\n"
		"	\t./wxtool -s 02:00.0 -e 240                          \n"
		"   -m\t\tTo change mac\n"
		"	\t./wxtool -s 02:00.0 -m 020203040506                          \n"
		"   -n\t\tTo change sn\n"
		"	\t./wxtool -s 02:00.0 -n 123456789987654321                          \n"
		"   -S -n\tTo change sn that can be any characters no more than 24.\n"
		"	\t./wxtool -s 02:00.0 -S -n anystringless24						   \n"
		"   -W\t\tTo test wave ....\n"
		"	\t./wxtool -s 02:00.0 -W						   \n"
		"   -E\t[SP]\tTo change 10G nic TX_EQ ....\n"
		"	\t./wxtool -s 02:00.0 -E 20 20 20						 \n"
		"   -i\t\tTo show nic info: MAC & SN\n"
		"	\t./wxtool -s 02:00.0 -i | ./wxtool -s 02:00.0 -S -i				\n"
		"   -l\t\tTo lock/unlock flash write enable\n"
		"	\t./wxtool -s 02:00.0 -l 1[0,1]							\n"
		"   -t\t\tTo dump image in nic\n"
		"	\t./wxtool -s 02:00.0 -t					\n"
		"   check\tTo check image version\n"
		"	\t./wxtool check image_name					\n"
		"\n");
	} else {
		fprintf(stderr,
		"\n"
		"   Usage: ./wxtool -s xx:xx.x [<options>]\n"
		"     Please use '--help' option to get detail information of tool.\n"
		"     Please use '--version' option to check the verision number of tool.\n"
		"\n"
		"Support options:\n"
		"   -s\t\tTo select an pci slot which can use lspci to see\n"
		"	\t./wxtool -s 02:00.0                             \n"
		"   -r\t\tTo read reg value\n"
		"	\t./wxtool -s 02:00.0 -r 0x10000                            \n"
		"   -w\t\tTo write reg value\n"
		"	\t./wxtool -s 02:00.0 -w 0x11000  0x1                           \n"
		"   -f -r\tTo read flash value\n"
		"	\t./wxtool -s 02:00.0 -f -r 0x160                            \n"
		"   -f -w\tTo write flash value\n"
		"	\t./wxtool -s 02:00.0 -f -w 0x160  0xaa                           \n"
		"   -c -r\tTo read cab value\n"
		"	\t./wxtool -s 02:00.0 -c -r 0x500                            \n"
		"   -c -w\tTo write cab value\n"
		"	\t./wxtool -s 02:00.0 -c -w 0x500  0x1                           \n"
		"   -d -r\tTo read access to mdio\n"
		"	\t./wxtool -s 02:00.0 -d -r 0x0[phy_addr] 0x0[addr]                            \n"
		"   -d -w\tTo write access to mdio\n"
		"	\t./wxtool -s 02:00.0 -d -w 0x0[phy_addr] 0x0[addr] 0x1[value]                  \n"
		"   -m\t\tTo change mac\n"
		"	\t./wxtool -s 02:00.0 -m 020203040506                          \n"
		"   -n\t\tTo change sn\n"
		"	\t./wxtool -s 02:00.0 -n 123456789987654321                          \n"
		"   -S -n\tTo change sn that can be any characters no more than 24.\n"
		"	\t./wxtool -s 02:00.0 -S -n anystringless24						   \n"
		"   -W\t\tTo test wave ....\n"
		"	\t./wxtool -s 02:00.0 -W						   \n"
		"   -E\t[SP]\tTo change 10G nic TX_EQ ....\n"
		"	\t./wxtool -s 02:00.0 -E 20 20 20						 \n"
		"   -i\t\tTo show nic info: MAC & SN\n"
		"	\t./wxtool -s 02:00.0 -i | ./wxtool -s 02:00.0 -S -i				\n"
		"   -l\t\tTo lock/unlock flash write enable\n"
		"	\t./wxtool -s 02:00.0 -l 1[0,1]							\n"
		"   -t\t\tTo dump image in nic\n"
		"	\t./wxtool -s 02:00.0 -t					\n"
		"   check\tTo check image version\n"
		"	\t./wxtool check image_name					\n"
		"\n");
	}
        if (choose == 1)
                exit(0);
}
static int
parse_options_for_wxtool(int argc, char **argv)
{
	const char opts[] = GENERIC_OPTIONS;
	int i=1;
	u32 verbose = 0;
	u32 demo_mode = 0;

	if (argc <= 2) {
		if (argc <= 1 || !strcmp(argv[1], "--help")) {
			usage_for_wxtool(1);
		} else if (!strcmp(argv[1], "--version")) {
			puts("pciutil version " PCIUTILS_VERSION);
			exit(0);
		}
	}

	while (i < argc && argv[i][0] == '-')
	{
		char *c = argv[i++] + 1;
		char *d = c;
		char *e;
		while (*c)
			switch (*c)
			{
			case 0:
				break;
			case 'v':
				verbose++;
				c++;
				break;
			case 'f':
				force++;
				c++;
				break;
			case 'D':
				demo_mode++;
				c++;
				break;
			default:
				if (e = strchr(opts, *c)) {
					char *arg = NULL;
					c++;
					if (e[1] == ':') {
						if (*c)
							arg = c;
						else if (i < argc)
							arg = argv[i++];
						else
							printf("Option -%c requires an argument", *e);
						c = "";
					}

					if (!parse_generic_option(*e, pacc_wx, arg))
						printf("Unable to parse option -%c", *e);
				} else {
					if (c != d)
						printf("Invalid or misplaced option -%c", *c);
					return i - 1;
				}
			}
	}

	return i;
}

int mac_change(struct pci_dev *devs, long int MAC_ADDR_T)
{
	struct pci_dev    *dev = NULL;
	u32 status = 0;
	u32 nic_flag = -1;
	u32 num[1030] = {0};
	u32 j=0,k=0;
	
	dev = devs; // select current PCI device
	if (DEBUG_ON || debug_mode) printf("read==bus=%0x, dev=%0x, func=%0x\n", dev->bus, dev->dev, dev->func);

	// Check PCIe Memory Space Enable
	status = pci_read_byte(dev, 0x4);
	pci_write_byte(dev, 0x4, (status | 0x2)); // set "Memory Space Enable" field

	if ((dev->vendor_id == VENDOR_ID || dev->vendor_id == WX_VENDOR_ID) && (dev->device_id >> 4) == 0x010) {
		if (DEBUG_ON || debug_mode) printf("read==bus=%0x, dev=%0x, func=%0x\n", dev->bus, dev->dev, dev->func);
		nic_flag = 0;
		if (DEBUG_ON || debug_mode) printf("1G\n");
	} else if ((dev->vendor_id == VENDOR_ID || dev->vendor_id == WX_VENDOR_ID) && (dev->device_id  == 0x2001 ||
			dev->device_id  == 0x1001)) {
		nic_flag = 1;
		if (DEBUG_ON || debug_mode) printf("10G\n");
	} else {
		printf("There is not our nic card.\n");
	}

	if (nic_flag == 0) {
		mac_addr0_dword0 = flash_read_dword(dev, MAC_ADDR0_WORD0_OFFSET_1G);
		mac_addr0_dword1 = flash_read_dword(dev, MAC_ADDR0_WORD1_OFFSET_1G) & 0xffff;
		mac_addr1_dword0 = flash_read_dword(dev, MAC_ADDR1_WORD0_OFFSET_1G);
		mac_addr1_dword1 = flash_read_dword(dev, MAC_ADDR1_WORD1_OFFSET_1G) & 0xffff;
		mac_addr2_dword0 = flash_read_dword(dev, MAC_ADDR2_WORD0_OFFSET_1G);
		mac_addr2_dword1 = flash_read_dword(dev, MAC_ADDR2_WORD1_OFFSET_1G) & 0xffff;
		mac_addr3_dword0 = flash_read_dword(dev, MAC_ADDR3_WORD0_OFFSET_1G);
		mac_addr3_dword1 = flash_read_dword(dev, MAC_ADDR3_WORD1_OFFSET_1G) & 0xffff;

#if 1
		if (dev->func == 0) {
			for (k=0;k<(4096/4);k++) {
				num[k] = flash_read_dword(dev, 0x60000 + k*4);
				//printf("%x--%08x\n",k,num[k]);
			}
			mac_addr0_dword0 = MAC_ADDR_T;
			mac_addr0_dword1 = MAC_ADDR_T >> 32;
			flash_erase(dev, 0x60000/4096, 0);
		} else if (dev->func == 1) {
			for (k=0;k<(4096/4);k++) {
				num[k] = flash_read_dword(dev, 0x68000 + k*4);
				//printf("%x--%08x\n",k,num[k]);
			}
			mac_addr1_dword0 = MAC_ADDR_T;
			mac_addr1_dword1 = MAC_ADDR_T >> 32;
			flash_erase(dev, 0x68000/4096, 0);
		} else if (dev->func == 2) {
			for (k=0;k<(4096/4);k++) {
				num[k] = flash_read_dword(dev, 0x70000 + k*4);
				//printf("%x--%08x\n",k,num[k]);
			}
			mac_addr2_dword0 = MAC_ADDR_T;
			mac_addr2_dword1 = MAC_ADDR_T >> 32;
			flash_erase(dev, 0x70000/4096, 0);
		} else if (dev->func == 3) {
			for (k=0;k<(4096/4);k++) {
				num[k] = flash_read_dword(dev, 0x78000 + k*4);
				//printf("%x--%08x\n",k,num[k]);
			}
			mac_addr3_dword0 = MAC_ADDR_T;
			mac_addr3_dword1 = MAC_ADDR_T >> 32;
			flash_erase(dev, 0x78000/4096, 0);
		}
#endif
		// Re-program MAC address and SN to chip
		if (dev->func == 0) {
			for(j=0;j<4096/4;j++){
				if(!(j*4 == 0xc || j*4 == 0x14 || num[j] == 0xffffffff))
					flash_write_dword(dev, 0x60000 + j*4, num[j]);
			}
			flash_write_dword(dev, MAC_ADDR0_WORD0_OFFSET_1G, mac_addr0_dword0);
			flash_write_dword(dev, MAC_ADDR0_WORD1_OFFSET_1G, (mac_addr0_dword1 | 0x80000000));//lan0
			mac_addr0_dword0  = flash_read_dword(dev, MAC_ADDR0_WORD0_OFFSET_1G);
			mac_addr0_dword1  = flash_read_dword(dev, MAC_ADDR0_WORD1_OFFSET_1G);
			printf("New:MAC Address0 is: 0x%04x%08x\n",(mac_addr0_dword1 & 0xffff), mac_addr0_dword0);
		} else if (dev->func == 1) {
			for(j=0;j<4096/4;j++){
				if(!(j*4 == 0xc || j*4 == 0x14 || num[j] == 0xffffffff))
					flash_write_dword(dev, 0x68000 + j*4, num[j]);
			}
			flash_write_dword(dev, MAC_ADDR1_WORD0_OFFSET_1G, mac_addr1_dword0);
			flash_write_dword(dev, MAC_ADDR1_WORD1_OFFSET_1G, (mac_addr1_dword1 | 0x80000000));//lan1
			mac_addr1_dword0  = flash_read_dword(dev, MAC_ADDR1_WORD0_OFFSET_1G);
			mac_addr1_dword1  = flash_read_dword(dev, MAC_ADDR1_WORD1_OFFSET_1G);
			printf("New:MAC Address1 is: 0x%04x%08x\n",(mac_addr1_dword1 & 0xffff), mac_addr1_dword0);
		} else if (dev->func == 2) {
			for(j=0;j<4096/4;j++){
				if(!(j*4 == 0xc || j*4 == 0x14 || num[j] == 0xffffffff))
					flash_write_dword(dev, 0x70000 + j*4, num[j]);
			}
			flash_write_dword(dev, MAC_ADDR2_WORD0_OFFSET_1G, mac_addr2_dword0);
			flash_write_dword(dev, MAC_ADDR2_WORD1_OFFSET_1G, (mac_addr2_dword1 | 0x80000000));//lan2
			mac_addr2_dword0  = flash_read_dword(dev, MAC_ADDR2_WORD0_OFFSET_1G);
			mac_addr2_dword1  = flash_read_dword(dev, MAC_ADDR2_WORD1_OFFSET_1G);
			printf("New:MAC Address2 is: 0x%04x%08x\n",(mac_addr2_dword1 & 0xffff), mac_addr2_dword0);
		} else if (dev->func == 3) {
			for(j=0;j<4096/4;j++){
				if(!(j*4 == 0xc || j*4 == 0x14 || num[j] == 0xffffffff))
					flash_write_dword(dev, 0x78000 + j*4, num[j]);
			}
			flash_write_dword(dev, MAC_ADDR3_WORD0_OFFSET_1G, mac_addr3_dword0);
			flash_write_dword(dev, MAC_ADDR3_WORD1_OFFSET_1G, (mac_addr3_dword1 | 0x80000000));//lan3
			mac_addr3_dword0  = flash_read_dword(dev, MAC_ADDR3_WORD0_OFFSET_1G);
			mac_addr3_dword1  = flash_read_dword(dev, MAC_ADDR3_WORD1_OFFSET_1G);
			printf("New:MAC Address3 is: 0x%04x%08x\n",(mac_addr3_dword1 & 0xffff), mac_addr3_dword0);
		}
	} else if (nic_flag == 1) {
		mac_addr0_dword0 = flash_read_dword(dev, MAC_ADDR0_WORD0_OFFSET);
		mac_addr0_dword1 = flash_read_dword(dev, MAC_ADDR0_WORD1_OFFSET) & 0xffff;
		mac_addr1_dword0 = flash_read_dword(dev, MAC_ADDR1_WORD0_OFFSET);
		mac_addr1_dword1 = flash_read_dword(dev, MAC_ADDR1_WORD1_OFFSET) & 0xffff;
#if 0
		printf("Old: MAC Address0 is: 0x%04x%08x\n", mac_addr0_dword1, mac_addr0_dword0);
		printf("     MAC Address1 is: 0x%04x%08x\n", mac_addr1_dword1, mac_addr1_dword0);
#endif
		if (dev->func == 0) {
			for (k=0;k<(4096/4);k++) {
				num[k] = flash_read_dword(dev, 0x60000 + k*4);
				//printf("%x--%08x\n",k,num[k]);
			}
			mac_addr0_dword0 = MAC_ADDR_T;
			mac_addr0_dword1 = MAC_ADDR_T >> 32;
			//printf("mac_addr : %08x\n",mac_addr0_dword0 );
			//printf("mac_addr : %04x\n",mac_addr0_dword1 );
			flash_erase(dev, 0x60000/4096, 0);
		} else if (dev->func == 1) {
			for (k=0;k<(4096/4);k++) {
				num[k] = flash_read_dword(dev, 0x70000 + k*4);
				//printf("%x--%08x\n",k,num[k]);
			}
			mac_addr1_dword0 = MAC_ADDR_T;
			mac_addr1_dword1 = MAC_ADDR_T >> 32;
			//printf("mac_addr : %08x\n",mac_addr0_dword0 );
			//printf("mac_addr : %04x\n",mac_addr0_dword1 );
			flash_erase(dev, 0x70000/4096, 0);
		} 

		// Re-program MAC address and SN to chip
		if (dev->func == 0) {
			for(j=0;j<4096/4;j++){
				if(!(j*4 == 0xc || j*4 == 0x14 || num[j] == 0xffffffff))
					flash_write_dword(dev, 0x60000 + j*4, num[j]);
			}
			flash_write_dword(dev, MAC_ADDR0_WORD0_OFFSET, mac_addr0_dword0);
			flash_write_dword(dev, MAC_ADDR0_WORD1_OFFSET, (mac_addr0_dword1 | 0x80000000));//lan0
			mac_addr0_dword0  = flash_read_dword(dev, MAC_ADDR0_WORD0_OFFSET);
			mac_addr0_dword1  = flash_read_dword(dev, MAC_ADDR0_WORD1_OFFSET);

			printf("New:MAC Address0 is: 0x%04x%08x\n",(mac_addr0_dword1 & 0xffff), mac_addr0_dword0);
		} else if (dev->func == 1) {
			for(j=0;j<4096/4;j++){
				if(!(j*4 == 0xc || j*4 == 0x14 || num[j] == 0xffffffff))
					flash_write_dword(dev, 0x70000 + j*4, num[j]);
			}
			flash_write_dword(dev, MAC_ADDR1_WORD0_OFFSET, mac_addr1_dword0);
			flash_write_dword(dev, MAC_ADDR1_WORD1_OFFSET, (mac_addr1_dword1 | 0x80000000));//lan1

			mac_addr1_dword0  = flash_read_dword(dev, MAC_ADDR1_WORD0_OFFSET);
			mac_addr1_dword1  = flash_read_dword(dev, MAC_ADDR1_WORD1_OFFSET);
			printf("New:MAC Address1 is: 0x%04x%08x\n",(mac_addr1_dword1 & 0xffff), mac_addr1_dword0);
		} 

	}
	
	return 0;
}

int sn_change(struct pci_dev *devs)
{
	struct pci_dev    *dev = NULL;
	u32 status = 0;
	u32 i = 0;
	
	dev = devs; // select current PCI device
	// Check PCIe Memory Space Enable
	status = pci_read_byte(dev, 0x4);
	pci_write_byte(dev, 0x4, (status | 0x2)); // set "Memory Space Enable" field
	status = fmgr_usr_cmd_op(dev, 0x6); // write enable
	if (DEBUG_ON || debug_mode) printf("SST flash write enable user command, return status = %0d\n", status);
	status = fmgr_usr_cmd_op(dev, 0x98); // global protection un-lock

	if (opt_s_is_set == 1) {
			memset(serial_num_dword, 0, sizeof(serial_num_dword));
			printf("old SN is ");
			for(i = 0; i < 24; i++) {
				serial_num_dword[23-i] = flash_read_dword(dev, PRODUCT_SERIAL_NUM_OFFSET_1G + 4*(23-i));
				printf("%c", serial_num_dword[23-i]);
			}
			printf("\n");

			memset(serial_num_dword, 0, sizeof(serial_num_dword));
			for(i = 0; i < 24; i++) {
				serial_num_dword[23-i] = (unsigned int)(SN[i]);
			}

			flash_erase(dev, 0xf0000/4096, 0);
			for(i = 0; i < 24; i++) {
				flash_write_dword(dev, PRODUCT_SERIAL_NUM_OFFSET_1G + 4*i, serial_num_dword[i]);
			}

			memset(serial_num_dword, 0, sizeof(serial_num_dword));
			printf("new SN is ");
			for(i = 0; i < 24; i++) {
				serial_num_dword[23-i] = flash_read_dword(dev, PRODUCT_SERIAL_NUM_OFFSET_1G + 4*(23-i));
				printf("%c", serial_num_dword[23-i]);
			}
			printf("\n");
	} else {
		serial_num_dword0 = flash_read_dword(dev, PRODUCT_SERIAL_NUM_OFFSET_1G);
		serial_num_dword1 = flash_read_dword(dev, PRODUCT_SERIAL_NUM_OFFSET_1G + 4);
		serial_num_dword2 = flash_read_dword(dev, PRODUCT_SERIAL_NUM_OFFSET_1G + 8);
		printf("old SN is: %02x%08x%08x\n", (serial_num_dword2 & 0xff), serial_num_dword1, serial_num_dword0);

		memset(str_dword, 0, sizeof(str_dword));
		for (i = 0; i < 2; i++)
			str_dword[i] = SN[i];
		serial_num_dword2 = (unsigned long)strtol(str_dword, NULL, 16) & 0xffffffff;
		for (i = 0; i < 8; i++)
			str_dword[i] = SN[i + 2];
		serial_num_dword1 = (unsigned long)strtol(str_dword, NULL, 16) & 0xffffffff;
		for (i = 0; i < 8; i++)
			str_dword[i] = SN[i + 10];
		serial_num_dword0 = (unsigned long)strtol(str_dword, NULL, 16) & 0xffffffff;
		//printf("SN is: 0x%02x%08x%08x\n", serial_num_dword2, serial_num_dword1, serial_num_dword0);

		flash_erase(dev, 0xf0000/4096, 0);

		flash_write_dword(dev, PRODUCT_SERIAL_NUM_OFFSET_1G, serial_num_dword0);
		flash_write_dword(dev, PRODUCT_SERIAL_NUM_OFFSET_1G + 4, serial_num_dword1);
		flash_write_dword(dev, PRODUCT_SERIAL_NUM_OFFSET_1G + 8, serial_num_dword2);
		serial_num_dword0 = flash_read_dword(dev, PRODUCT_SERIAL_NUM_OFFSET_1G);
		serial_num_dword1 = flash_read_dword(dev, PRODUCT_SERIAL_NUM_OFFSET_1G + 4);
		serial_num_dword2 = flash_read_dword(dev, PRODUCT_SERIAL_NUM_OFFSET_1G + 8);
		printf("new SN is: %02x%08x%08x\n", (serial_num_dword2 & 0xff), serial_num_dword1, serial_num_dword0);
	}

	u8 nic_type = check_nic_type(dev);
	int vpd_offset = 0;
	u32 num[1030] = {0};
	u32 j=0,k=0;
	u32 read_data = 0;
	int flength = 0;
	u8 test2 = 0, test3 = 0;
	u32 chksum = 0;
	u32 chksum_off = 0;

	if (nic_type == EM) {
		vpd_offset = 0x60;
		chksum_off = 0x400;
	}
	else if (nic_type == SP) {
		vpd_offset = 0x500;
		chksum_off = 0x1000;
	} else
		printf("vpd not on\n");

	vpd_sn_change(dev, SN);

	for (k = 0;k<(4096/4);k++) {
		num[k] = flash_read_dword(dev, k*4);
		//printf("%x--%08x\n",k,num[k]);
	}
	//flash_erase(dev, 0x1000/4096, 0);
	write_reg(dev, SPI_CMD_CFG1_ADDR, 0x0103c720);
	for (i = 0; i < 8; i++) {
		flash_erase_sector(dev, i * 128);
		usleep(20 * 1000); // 20 ms
	}
	flash_erase(dev, 0/4096, 0);
	write_reg(dev, SPI_CMD_CFG1_ADDR, 0x0103c7d8);

	//usleep(1000);
	FILE *fd = NULL;
	/* load vpd tent*/
	fd = fopen("vpd_tent", "r");
	if (NULL == fd) {
		printf("ERROR: Can't open vpd_tent File!\n");
		return 0;
	}
	fseek(fd, 0, SEEK_END);
	flength = ftell(fd);
	rewind(fd);

	if (nic_type == EM) {
		for(j=0;j<4096/4;j++){
			if(!((j*4 >= 0x60 && j*4 < 0xc0) || j*4 == 0x15c || num[j] == 0xffffffff))
				flash_write_dword(dev, j*4, num[j]);
		}
	} else if (nic_type == SP) {
		for(j=0;j<4096/4;j++){
			if(!((j*4 >= 0x500 && j*4 < 0x600) || j*4 == 0x15c || num[j] == 0xffffffff))
				flash_write_dword(dev, j*4, num[j]);
		}
	}
	// Program Image file in dword
	for (i = 0; i < flength / 4; i++) {
		fread(&read_data, 4, 1, fd);
		if (read_data != 0xffffffff) {
			status = flash_write_dword(dev, vpd_offset + i * 4, read_data);
			if (status) {
				printf("ERROR: Program 0x%08x @addr: 0x%08x is failed !!\n", read_data, vpd_offset + i * 4);
				read_data = flash_read_dword(dev, vpd_offset + i * 4);
				printf("       Read data from Flash is: 0x%08x\n", read_data);
				return 1;
			}
		}
	}
	fclose(fd);

	for (i = 0; i < chksum_off/2 ; i ++) {
		test2 = flash_read_dword(dev, 2*i);
		test3 = flash_read_dword(dev, 2*i + 1);
		if (2*i == 0x15e) {
			test2 = 0x00;
			test3 = 0x00;
		}
		chksum += (test3 << 8 | test2);
		if (DEBUG_ON || debug_mode)printf("('0x%x', '0x%x')\n", test3, test2);
	}
	chksum = 0xbaba - chksum;
	chksum &= 0xffff;
	status = flash_write_dword(dev, 0x15e, 0xffff0000 | chksum);
	if (DEBUG_ON || debug_mode) printf("chksum : %04x\n", chksum);
	system("rm -rf vpd_tent");

	return 0;
}

int sp_set_txeq(struct pci_dev *devs, u32 main, u32 pre, u32 post)
{
	struct pci_dev    *dev = NULL;
	u32 status = 0;
	u32 nic_flag = -1;
	u32 num[1030] = {0};
	u32 j=0,k=0;
	
	dev = devs; // select current PCI device
	if (DEBUG_ON || debug_mode) printf("read==bus=%0x, dev=%0x, func=%0x\n", dev->bus, dev->dev, dev->func);

	// Check PCIe Memory Space Enable
	status = pci_read_byte(dev, 0x4);
	pci_write_byte(dev, 0x4, (status | 0x2)); // set "Memory Space Enable" field

	if ((dev->vendor_id == VENDOR_ID || dev->vendor_id == WX_VENDOR_ID) && (dev->device_id >> 4) == 0x010) {
		if (DEBUG_ON || debug_mode) printf("read==bus=%0x, dev=%0x, func=%0x\n", dev->bus, dev->dev, dev->func);
		nic_flag = 0;
		if (DEBUG_ON || debug_mode) printf("1G\n");
		printf("1G nic card do not support this operation.\n");
		return 0;
	} else if ((dev->vendor_id == VENDOR_ID || dev->vendor_id == WX_VENDOR_ID) && (dev->device_id  == 0x2001 ||
			dev->device_id  == 0x1001)) {
		nic_flag = 1;
		if (DEBUG_ON || debug_mode) printf("10G\n");
	} else {
		printf("There is not our nic card.\n");
	}

	if (nic_flag == 1) {
		if (dev->func == 0) {
			for (k=0;k<(4096/4);k++) {
				num[k] = flash_read_dword(dev, 0x60000 + k*4);
				//printf("%x--%08x\n",k,num[k]);
			}
			txeq0_ctrl0 = (0x1804 & ~0x3F3F) | main << 8 | pre;
			txeq0_ctrl1 = (0x50 & ~0x7F) | (1 << 6)| post;
			flash_erase(dev, 0x60000/4096, 0);
		} else if (dev->func == 1) {
			for (k=0;k<(4096/4);k++) {
				num[k] = flash_read_dword(dev, 0x70000 + k*4);
				//printf("%x--%08x\n",k,num[k]);
			}
			txeq0_ctrl0 = (0x1804 & ~0x3F3F) | main << 8 | pre;
			txeq0_ctrl1 = (0x50 & ~0x7F) | (1 << 6)| post;
			flash_erase(dev, 0x70000/4096, 0);
		} 

		// Re-program MAC address and SN to chip
		if (dev->func == 0) {
			for(j=0;j<4096/4;j++){
				if(!(j*4 == 0x2c || j*4 == 0x3c || num[j] == 0xffffffff))
					flash_write_dword(dev, 0x60000 + j*4, num[j]);
			}
			flash_write_dword(dev, SP0_TXEQ_CTRL0_OFFSET, txeq0_ctrl0);
			flash_write_dword(dev, SP0_TXEQ_CTRL1_OFFSET, txeq0_ctrl1);//lan0
			txeq0_ctrl0 = 0;
			txeq0_ctrl1 = 0;
			txeq0_ctrl0  = flash_read_dword(dev, SP0_TXEQ_CTRL0_OFFSET);
			txeq0_ctrl1  = flash_read_dword(dev, SP0_TXEQ_CTRL1_OFFSET);

			printf("0x18036 : %04x - 0x18037 : %04x\n", txeq0_ctrl0, txeq0_ctrl1);
		} else if (dev->func == 1) {
			for(j=0;j<4096/4;j++){
				if(!(j*4 == 0x2c || j*4 == 0x3c || num[j] == 0xffffffff))
					flash_write_dword(dev, 0x70000 + j*4, num[j]);
			}
			flash_write_dword(dev, SP1_TXEQ_CTRL0_OFFSET, txeq0_ctrl0);
			flash_write_dword(dev, SP1_TXEQ_CTRL1_OFFSET, txeq0_ctrl1);//lan1
			txeq0_ctrl0 = 0;
			txeq0_ctrl1 = 0;

			txeq0_ctrl0 = flash_read_dword(dev, SP1_TXEQ_CTRL0_OFFSET);
			txeq0_ctrl1 = flash_read_dword(dev, SP1_TXEQ_CTRL1_OFFSET);
			printf("0x18036 : %04x - 0x18037 : %04x\n", txeq0_ctrl0, txeq0_ctrl1);
		} 

	}
	
	return 0;
}


void wavetool(struct pci_dev *devs)
{
	struct pci_dev    *dev = NULL;
	u8                 status = 0;
	int 		way;

	dev = devs; // select current PCI device
	printf("read==bus=%0x, dev=%0x, func=%0x\n", dev->bus, dev->dev, dev->func);

	// Check PCIe Memory Space Enable
	status = pci_read_byte(dev, 0x4);
	pci_write_byte(dev, 0x4, (status | 0x2)); // set "Memory Space Enable" field

back:
	while(1)
		{
		printf("Please Select Wave Form Test Mode:\n");
		printf("  1.1000M Test Mode 1\n");
		printf("  2.1000M Test Mode 2\n");
		printf("  3.1000M Test Mode 4\n");
		printf("  4.100M(MLT-3) Channel A\n");
		printf("  5.100M(MLT-3) Channel B\n");
		printf("  6.10M for Diff.Voltage/TP-IDL/Jitter\n");
		printf("  7.10M for Harmonic(all '1' pattern)\n");
		printf("  8.10M for Harmonic(all '0' pattern)\n");
		printf("  9.Back\n");
		printf("  10.Exit\n");
	
		printf("please input choose number:");
		scanf("%d", &way);
		safe_flush(stdin);

		switch(way)
		{
		case 1:
			printf("  1.1000M Test Mode 1\n");
			do_a(dev);
			break;
		case 2:
			printf("  2.1000M Test Mode 2\n");
			do_b(dev);
			break;
		case 3:
			printf("  3.1000M Test Mode 4\n");
			do_c(dev);
			break;
		case 4:
			printf("  4.100M(MLT-3) Channel A\n");
			do_d(dev);
			break;
		case 5:
			printf("  5.100M(MLT-3) Channel B\n");
			do_e(dev);
			break;
		case 6:
			printf("  6.10M for Diff.Voltage/TP-IDL/Jitter\n");
			do_f(dev);
			break;
		case 7:
			printf("  7.10M for Harmonic(all '1' pattern)\n");
			do_g(dev);
			break;
		case 8:
			printf("  8.10M for Harmonic(all '0' pattern)\n");
			do_h(dev);
			break;
		case 9:
			goto back;
			break;
		case 10:
			pci_cleanup(pacc_wx); /* Close everything */
			exit(0);
			break;
		default:
			printf("Invaild Way!\n");
		}
		way = 0;
	}
}

double cal_tmp(double code)
{
  double tmp_val;

  tmp_val = (-1.6743e-11)*pow(code, (double)4) + (8.1542e-8)*pow(code, (double)3) + (-1.8201e-4)*pow(code, (double)2) + (3.1020e-1)*code + (-4.838e+1);

  return tmp_val;
}

double cal_code(double tmp_val)
{
  double code;

  code = (1.8322e-8)*pow(tmp_val, (double)4) + (2.3430e-5)*pow(tmp_val, (double)3) + (8.7018e-3)*pow(tmp_val, (double)2) + (3.9269e+0)*tmp_val + 1.7204e+2;

  return code;
}

//	tmp_val = -40;//将温顿转换成寄存器的十进制，然后将十进制换成十六进制，即为40℃对应的寄存器的值
//	code = cal_code(tmp_val);
//	printf("temperature = %.2f, code is %.1f\n", tmp_val, code);

void show_nic_info(struct pci_dev *devs)
{
	struct pci_dev	*dev = NULL;
	double code;	// data out code
	double tmp_val;	// temperature value
	u32	pxe_type;
	u32	k = 0;
	u8	type = 0;

	dev = devs; // select current PCI device
	if ((dev->vendor_id == VENDOR_ID || dev->vendor_id == WX_VENDOR_ID) && ((dev->device_id == DEVICE_ID_1001) || (dev->device_id == DEVICE_ID_2001)))
		type = 1;//sp
	 else if ((dev->vendor_id == VENDOR_ID || dev->vendor_id == WX_VENDOR_ID) && ((dev->device_id >> 4 == 0x010)))
		type = 2;//em


	printf("adaptor card [ %02x:%02x.%0x ] info:\n", dev->bus, dev->dev, dev->func);
	if (type == 2) {
		// Re-program MAC address and SN to chip
		if (dev->func == 0) {
			mac_addr0_dword0  = flash_read_dword(dev, MAC_ADDR0_WORD0_OFFSET_1G);
			mac_addr0_dword1  = flash_read_dword(dev, MAC_ADDR0_WORD1_OFFSET_1G);
			printf("MAC Address0 is: 0x%04x%08x\n",(mac_addr0_dword1 & 0xffff), mac_addr0_dword0);
		} else if (dev->func == 1) {
			mac_addr1_dword0  = flash_read_dword(dev, MAC_ADDR1_WORD0_OFFSET_1G);
			mac_addr1_dword1  = flash_read_dword(dev, MAC_ADDR1_WORD1_OFFSET_1G);
			printf("MAC Address1 is: 0x%04x%08x\n",(mac_addr1_dword1 & 0xffff), mac_addr1_dword0);
		} else if (dev->func == 2) {
			mac_addr2_dword0  = flash_read_dword(dev, MAC_ADDR2_WORD0_OFFSET_1G);
			mac_addr2_dword1  = flash_read_dword(dev, MAC_ADDR2_WORD1_OFFSET_1G);
			printf("MAC Address2 is: 0x%04x%08x\n",(mac_addr2_dword1 & 0xffff), mac_addr2_dword0);
		} else if (dev->func == 3) {
			mac_addr3_dword0  = flash_read_dword(dev, MAC_ADDR3_WORD0_OFFSET_1G);
			mac_addr3_dword1  = flash_read_dword(dev, MAC_ADDR3_WORD1_OFFSET_1G);
			printf("MAC Address3 is: 0x%04x%08x\n",(mac_addr3_dword1 & 0xffff), mac_addr3_dword0);
		}
	} else if (type == 1) {
		if (dev->func == 0) {
			mac_addr0_dword0  = flash_read_dword(dev, MAC_ADDR0_WORD0_OFFSET);
			mac_addr0_dword1  = flash_read_dword(dev, MAC_ADDR0_WORD1_OFFSET);
			printf("MAC Address0 is: 0x%04x%08x\n",(mac_addr0_dword1 & 0xffff), mac_addr0_dword0);
		} else if (dev->func == 1) {
			mac_addr1_dword0  = flash_read_dword(dev, MAC_ADDR1_WORD0_OFFSET);
			mac_addr1_dword1  = flash_read_dword(dev, MAC_ADDR1_WORD1_OFFSET);
			printf("MAC Address1 is: 0x%04x%08x\n",(mac_addr1_dword1 & 0xffff), mac_addr1_dword0);
		} 

	}

	if (opt_s_is_set == 1) {
		printf("SN is ");
		for(k = 0; k < 24; k++) {
			serial_num_dword[23-k] = flash_read_dword(dev, PRODUCT_SERIAL_NUM_OFFSET_1G + 4*(23-k));
			printf("%c", serial_num_dword[23-k]);
		}
		printf("\n\n");
	} else {
		serial_num_dword0 = flash_read_dword(dev, PRODUCT_SERIAL_NUM_OFFSET_1G);
		serial_num_dword1 = flash_read_dword(dev, PRODUCT_SERIAL_NUM_OFFSET_1G + 4);
		serial_num_dword2 = flash_read_dword(dev, PRODUCT_SERIAL_NUM_OFFSET_1G + 8);
		if (DEBUG_ON || debug_mode) printf("Store SN: %0x%08x%08x\n", (serial_num_dword2 & 0xff), serial_num_dword1, serial_num_dword0);
		printf("     SN is: %02x%08x%08x\n\n", (serial_num_dword2 & 0xff), serial_num_dword1, serial_num_dword0);
	}
	if (type == 2) {
		//printf("The chip version is %c\n", 0xff & flash_read_dword(dev, 0x100000 - 40));
		printf("The chip version is %c\n", check_chip_version(dev));
		if (dev->func == 0) {
			write_reg(dev, 0x10304, 0x1); 
			code = 0x3ff & read_reg(dev, 0x10308);//寄存器转化成温度值。pcitool -s 01:00.0 -r 0 0x10308读取当前温度值(16进制)，转换成10进制的值为326。
			tmp_val = (code - 216)/4 - 40;
			printf("code=%.3f, temperature is %.2f\n", code, tmp_val);
		}
	} else if (type == 1 && dev->func == 0) {
		write_reg(dev, 0x10304, 0x1); 
		code = 0xfff & read_reg(dev, 0x10308);//寄存器转化成温度值。pcitool -s 01:00.0 -r 0 0x10308读取当前温度值(16进制)，转换成10进制的值为326。
		tmp_val = cal_tmp(code);
		printf("code=%.3f, temperature is %.2f\n", code, tmp_val);
	}
	pxe_type = 0xffff &flash_read_dword(dev, 0xa000a);
	if (pxe_type == 0xaa64)
		printf("Only support arm pxe\n");
}

int w_a_check_workaround(struct pci_dev *devs)
{
	u32 rdata, rdata_0, rdata_1;
	struct pci_dev *dev = devs;
	u16 i = 0;

	//if (dev->device_id != 0x0100)
	//	return 0;
	if (DEBUG_ON || debug_mode) printf("w_a_check_workaround == 1\n");
	rdata = read_reg(dev, 0x10000);
	if (rdata == 0x0 || (rdata == 0xfffffff)) {
		printf("chip is malfunction, all LAN disabled or pcie link is down\n");
		return 0;
	}
	if (DEBUG_ON || debug_mode) printf("w_a_check_workaround == 2\n");
	//read efuse
	cab_write(dev, 0x31c, 0x1);
	usleep(5000);
	rdata = cab_read(dev, 0x31c);
	if (rdata == 0x80000000) {
		rdata_0 = cab_read(dev, 0x328);
		rdata_1 = cab_read(dev, 0x32c);
		printf("rdata_0 : %x - rdata_1 : %x\n", rdata_0, rdata_1);
		if ((((rdata_0 >> 9) & 0x1FF) == ~((rdata_1 >> 23) & 0x1FF)) &&
			(((rdata_0 >> 9) & 0xf) == 0xf)) {
			printf("All lan disabled, and efuse integrity check pass\n");
			printf("write protect, the chip is malfunction\n");    // this chip must be replaced
			return -3;
		} else if ((((rdata_0 >> 9) & 0x1FF) == ~((rdata_1 >> 23) & 0x1FF)) &&
				(((rdata_0 >> 9) & 0xf) != 0)){
			printf("write protect, but some functions is disabled\n"); 
		}
	} else {
		printf("read efuse failed = 1 =\n");   // efuse could not be read
		return -2;
	}

	if (DEBUG_ON || debug_mode) printf("w_a_check_workaround == 3\n");

	for (i = 0;i<32; i++) {
		printf("=========================\n");
		// write efuse with all '1s
		cab_write(dev, 0x320, 0xffffffff);// 0x320 with 0xFFFF_FFFF;
		//cab_write(dev, 0x324, 0xffffffff);// 0x324 with 0xFFFF_FFFF;
		cab_write(dev, 0x31c, 0x2);// 0x31C with 0x2;
		usleep(5000);
		rdata = cab_read(dev, 0x31c); //0x31C to rdata;
		if (rdata == 0x80000000) {  // write successfully
			// re-read efuse, check whether the content is all '1s
			cab_write(dev, 0x31c, 0x1);// 0x31C with 0x1;
			usleep(5000);
			rdata = cab_read(dev, 0x31c); //0x31C to rdata;
			if (rdata == 0x80000000) {
				rdata_0 = cab_read(dev, 0x328);
				//rdata_1 = cab_read(dev, 0x32c);
				if ((rdata_0 != 0xFFFFFFFF)) {
					if ( i != 31) {
						printf("rdata_0 : %x\n", rdata_0);
						printf("=========================\n");
						continue;
					} else {
						//printf("program efuse failed\n");
					}
				}
				break;
			} else {
				printf("read efuse failed = 2 =\n");   // efuse could not be read
				return -2;
			}
		} else {
			printf("program efuse failed\n");
			return -1;
		}
	}

	for (i = 0;i<32; i++) {
		printf("--------------------\n");
		// write efuse with all '1s
		//cab_write(dev, 0x320, 0xffffffff);// 0x320 with 0xFFFF_FFFF;
		cab_write(dev, 0x324, 0xffffffff);// 0x324 with 0xFFFF_FFFF;
		cab_write(dev, 0x31c, 0x2);// 0x31C with 0x2;
		usleep(5000);
		rdata = cab_read(dev, 0x31c); //0x31C to rdata;
		if (rdata == 0x80000000) {  // write successfully
			// re-read efuse, check whether the content is all '1s
			cab_write(dev, 0x31c, 0x1);// 0x31C with 0x1;
			usleep(5000);
			rdata = cab_read(dev, 0x31c); //0x31C to rdata;
			if (rdata == 0x80000000) {
				//rdata_0 = cab_read(dev, 0x328);
				rdata_1 = cab_read(dev, 0x32c);
				//if ((rdata_1 >> 23) != 0x1FF) {
				if ((((rdata_1 >> 23) & (rdata_0 >> 9)) & 0x1ff) == 0) {
					if ( i != 31) {
						printf("rdata_0: %x - rdata_1 : %x\n", rdata_0, rdata_1);
						printf("--------------------\n");
						continue;
					} else {
						printf("program efuse failed\n");
						return -1;
					}
				}
				break;
			} else {
				printf("read efuse failed = 3 =\n");   // efuse could not be read
				return -2;
			}
		} else {
			printf("program efuse failed\n");
			return -1;
		}
	}

	printf("program efuse successed\n");
	return 0;
}

int dump_image(struct pci_dev *dev)
{

	u32 rdata;
	u32 i, status;
	FILE *fp_i = NULL;
	if (DEBUG_ON || debug_mode) printf("===========dump_image============\n");
	if (DEBUG_ON || debug_mode) printf("bus=%0x, dev=%0x, func=%0x\n", dev->bus, dev->dev, dev->func);
	
	if (!(dev->vendor_id == VENDOR_ID || dev->vendor_id == WX_VENDOR_ID)) {
		
		printf("It's not a right slot\n");
		return 0;
	}
		
	// Check PCIe Memory Space Enable
	status = pci_read_byte(dev, 0x4);
	pci_write_byte(dev, 0x4, (status | 0x2)); // set "Memory Space Enable" field
	fp_i = fopen("dump.img", "w+");
	if (NULL == fp_i) {
		printf("ERROR: Can't open IMAGE File %s!\n", "dump.img");
		return 0;
	}

	if (DEBUG_ON || debug_mode) printf("IMAGE File %s!\n", "dump.img");
			// Program Image file in dword
		for (i = 0; i < 0x100000 / 4; i++) {
			rdata = flash_read_dword(dev, i * 4);
			//printf("read_data: %x\n", read_data);
			fwrite(&rdata, 4, 1, fp_i);
			fflush(stdout);
		}


	fclose(fp_i);
	if (DEBUG_ON || debug_mode) printf("===========dump_image============\n");
	printf("===========finish============\n");
	return 0;
}

int update_region(struct pci_dev *dev)
{

	u32 read_data;
	u32 i, status;
	u32 file_length;
	u32 val;
	//struct region f_region = NULL;
	
	FILE *fp = NULL;
	if (DEBUG_ON || debug_mode) printf("bus=%0x, dev=%0x, func=%0x\n", dev->bus, dev->dev, dev->func);
	
	if (!(dev->vendor_id == VENDOR_ID || dev->vendor_id == WX_VENDOR_ID)) {
		
		printf("It's not a right slot\n");
		return 0;
	}
		
	// Check PCIe Memory Space Enable
	status = pci_read_byte(dev, 0x4);
	pci_write_byte(dev, 0x4, (status | 0x2)); // set "Memory Space Enable" field

	//unlock flash lock
	emnic_flash_write_unlock(dev);
	usleep(10000);

	//if f_region.name = "flash":
	val = 0xf0000 - 0xa0000;
	for(i = 0; i < val/(1024*64); i++) {
		flash_erase_sector(dev, 0xa0000 + i * (64 * 1024));
		printf("erase sector===%d\n", i);
	}

	fp = fopen(IMAGE_FILE, "r");
	if (NULL == fp) {
		printf("ERROR: Can't open IMAGE File %s!\n", IMAGE_FILE);
		return 0;
	}
	fseek(fp, 0, SEEK_END);
	file_length = ftell(fp);
	rewind(fp);

	if (DEBUG_ON || debug_mode) printf("IMAGE File %s!\n", IMAGE_FILE);
			// Program Image file in dword
		for (i = 0; i < file_length / 4; i++) {
			fread(&read_data, 4, 1, fp);
			if (read_data != 0xffffffff) {
				status = flash_write_dword(dev, 0xa0000 + i * 4, read_data);
				if (status) {
					printf("ERROR: Program 0x%08x @addr: 0x%08x is failed !!\n", read_data, 0xa0000 + i * 4);
					read_data = flash_read_dword(dev, 0xa0000 + i * 4);
					printf("       Read data from Flash is: 0x%08x\n", read_data);
					return 1;
				}
			}
		}


	fclose(fp);
	printf("===========finish============\n");
	return 0;
}

#endif
