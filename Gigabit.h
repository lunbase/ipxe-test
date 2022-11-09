#ifndef _GIGABIT_H_
#define _GIGABIT_H_
#include "functions.h"

#ifdef SPI_CLK_DIV
#undef SPI_CLK_DIV
#define SPI_CLK_DIV 3
#endif

extern int Gigabit_nics(void);

int Gigabit_nics(void)
{
	struct pci_access *pacc = NULL;
	struct pci_dev *dev = NULL;
	struct pci_dev *devs[FUNC_NUM][MAX_RAPTOR_CARD_NUM];
	u32 dev_num[FUNC_NUM] = {0, 0, 0, 0};
	u32 active_dev_num = 0;

	FILE *fp = NULL;
	long int file_length = 0;
	u32 sector_num = 0;

	u32 read_data = 0;
	u32 upgrade_read_data = 0;
	u8 status = 0;
	u8 skip = 0;
	u32 i = 0, j = 0, m = 0, k = 0, n = 0;
	u32 num[1030] = {0};
	int chip_v1,image_v1;

	u32 chksum = 0;
	u8 test3 = 0;
	u8 test2 = 0;

	pacc = pci_alloc(); /* Get the pci_access structure */
	pacc->error = die;

	/* Set all options you want -- here we stick with the defaults */
	pci_init(pacc);     /* Initialize the PCI library */
	pci_scan_bus(pacc); /* We want to get the list of devices */

	for (i = 0; i < MAX_RAPTOR_CARD_NUM; i++) {
		for (m = 0; m < FUNC_NUM; m++) {
			devs[m /*PCIe Func ID*/][i /*Card ID*/] = NULL;
		}
	}

	for (dev = pacc->devices; dev; dev = dev->next) { /* Iterate over all devices */
		if ((dev->vendor_id == VENDOR_ID || dev->vendor_id == WX_VENDOR_ID) && (dev->device_id >> 4) == 0x010) {
			devs[dev->func][dev_num[dev->func]] = dev;
			if (DEBUG_ON || debug_mode) printf("bus=%0x, dev=%0x, func=%0x\n", dev->bus, dev->dev, dev->func);
			dev_num[dev->func] += 1;
		}
	}
	printf("Please wait %d Rocket cards flash upgrading ....\n\n", dev_num[0]);
	if (dev_num[0] == 0) {
		printf("\nNo any Rocket cards were found! Program is exited!!\n\n");
		return 0;
	}

	/* Read Image File */
	if (!opt_f_is_set)
		strcpy(IMAGE_FILE, "image/prd_flash_golden.img");

	fp = fopen(IMAGE_FILE, "r");
	if (NULL == fp) {
		printf("ERROR: Can't open IMAGE File %s!\n", IMAGE_FILE);
		return 0;
	}

	if (!opt_i_is_set) {
		if (check_image_checksum()) {
			printf("Image verify failed...\n");
			printf("Please check your image\n");
			return -1;
		}
	}

	fseek(fp, 0, SEEK_END);
	file_length = ftell(fp);
	sector_num = file_length / SPI_SECTOR_SIZE;
	if (DEBUG_ON || debug_mode)
		printf("image file length is %0ld bytes, sector number is %0d.\n", file_length, sector_num);
	rewind(fp);

	if (opt_s1_is_set != 1) {
		/* Select PCI device */
		if (!opt_a_is_set) {
			if (dev_num[0] == 1) { // To select the unique PCI device
				dev_index = 0;
			} else { // To select a PCI device when not to upgrade all of device
				printf("\nMore than one of our adaptor cards were found, but without of '-A' option specified. Please select a adaptor to download.\n");
				while (dev_index >= dev_num[0]) {
					printf("\nPlease select a PCI device before upgrading.\n");
					for (i = 0; i < dev_num[0]; i++) {
						if (i == 0)
							printf(" [ %0x ] %02x:%02x.%0x", i, devs[0][i]->bus, devs[0][i]->dev, devs[0][i]->func);
						else if (i == (dev_num[0] - 1))
							printf(" [ %0x ] %02x:%02x.%0x : ", i, devs[0][i]->bus, devs[0][i]->dev, devs[0][i]->func);
						else
							printf(", [ %0x ] %02x:%02x.%0x", i, devs[0][i]->bus, devs[0][i]->dev, devs[0][i]->func);
					}
					printf("\nPlease select slot index: ");
					scanf("%d", (int *)&dev_index);
					safe_flush(stdin);
					printf("\n");
					if (dev_index >= dev_num[0])
						printf("The PCI selection is out of range! Please select again!!\n");
				}
			}
			active_dev_num = 1;
		} else {
			active_dev_num = dev_num[0];
		}
	} else {
		dev_index =0;
		dev_num[0] = 1;
		active_dev_num = 1;
	}
	/* Select Flash Manufacturer */
	opt_m_is_set = 1; // Force to SST
	flash_vendor = FLASH_SST;
	if (!opt_m_is_set) {
		printf("\nPlease select a flash chip kind before upgrading, nothing input will select to default Winbond flash.\n");
		printf(" [ 0 ] Winbond, [ 1 ] Spanish, [ 2 ] SST, [ others ] Winbond :  ");
		scanf("%d", (int *)&flash_vendor);
		printf("\n");
	}
	if (flash_vendor != FLASH_WINBOND && flash_vendor != FLASH_SPANISH && flash_vendor != FLASH_SST)
		flash_vendor = FLASH_SST;
	if (DEBUG_ON || debug_mode)
		printf("Select [ %0d ] flash for upgrading.\n", flash_vendor);

	if (DEBUG_ON || debug_mode) printf("dev_num : %x - active_dev_num : %x\n", dev_num[0], active_dev_num);
	/* Select PCI Device */
	for (j = 0; j < dev_num[0]; j++) {
		if (opt_s1_is_set == 1)
			dev = global_dev;
		else {
			if (!opt_a_is_set && dev_index != j)
				continue;
			dev = devs[0][j]; // select current PCI device
		}

		if (!((dev->vendor_id == VENDOR_ID || dev->vendor_id == WX_VENDOR_ID) && (dev->device_id >> 4) == 0x010)) {
			if (DEBUG_ON || debug_mode) printf("bus=%0x, dev=%0x, func=%0x\n", dev->bus, dev->dev, dev->func);
			printf("It's not a right slot\n");
			return 0;
		}

		// Check PCIe Memory Space Enable
		status = pci_read_byte(dev, 0x4);
		pci_write_byte(dev, 0x4, (status | 0x2)); // set "Memory Space Enable" field

		//check chip/fw mbox cmd is ok
		status = check_nic_status(dev);

		/* check whether has been upgraded or not */
		upgrade_read_data = read_reg(dev, 0x10200);
		if (upgrade_read_data & 0x80000000) {
			printf("The Following networking adaptor cards has been upgraded:\n");
			printf("[ No.%x ] %02x:%02x.%0x\n", j, dev->bus, dev->dev, dev->func);
			printf("Please reboot your machine and retry \n");
			continue;
		}

		if (!(opt_r_is_set == 1)) {
			printf("Checking chip/image version .......\n");
			chip_v1 = check_chip_version(dev);
			if (chip_v1 == -2) printf("Check the hardware(flash)\n");
			image_v1 = check_image_version();
			printf("The image chip_v is %c\n", image_v1);
			printf("The nic chip_v is %c\n", chip_v1);
			if (chip_v1 != image_v1)
			{
				printf("====The Gigabit image is not match the Gigabit card (chip version)====\n");
				printf("====Please check your image====\n");
				continue;
			}
		}

		if (!(opt_t_is_set == 1)) {
			printf("Checking sub_id .......\n");
			u16 sub_id;
			u16 s_sub_id = 0;
			u8 data1, data2, data3, data4;
			fseek(fp, -9 * sizeof(u32), SEEK_END);

			data3 = flash_read_dword(dev, 0x100000 - 36);
			if (DEBUG_ON || debug_mode) printf("%02x ", data3);
			data4 = flash_read_dword(dev, 0x100000 - 35);
			if (DEBUG_ON || debug_mode) printf("%02x ", data4);
			s_sub_id = data3 << 8 | data4;
			printf("The card's sub_id : %04x\n", s_sub_id);

			/*check sub_id*/
			fread(&data1, 1, 1, fp);
			if (DEBUG_ON || debug_mode) printf("%02x ", data1);
			fread(&data2, 1, 1, fp);
			if (DEBUG_ON || debug_mode) printf("%02x ", data2);
			sub_id = data1 << 8 | data2;
			printf("The image's sub_id : %04x\n", sub_id);
			if ((sub_id & 0xffff) == (s_sub_id & 0xffff)) {
				if ((data1 & g_port_num) != g_port_num) {
					printf("Port nums is not right\n");
					continue;
				}
				printf("It is a right image\n");
			} else if (s_sub_id == 0xffff) {
				printf("update anyway\n");
			} else {
				printf("====The Gigabit image is not match the Gigabit card====\n");
				printf("====Please check your image====\n");
				continue;
			}

			printf("Checking dev_id .......\n");
			u16 dev_id;
			u8 data5, data6;

			/*check dev_id*/
			fread(&data5, 1, 1, fp);
			if (DEBUG_ON || debug_mode) printf("%02x ", data1);
			fread(&data6, 1, 1, fp);
			if (DEBUG_ON || debug_mode) printf("%02x ", data2);
			dev_id = data5 << 8 | data6;
			printf("The image's dev_id : %04x\n", dev_id);
			printf("The card's dev_id : %04x\n", dev->device_id);
			if (!(dev->device_id == dev_id) && !(dev_id == 0xffff)) {
				printf("====The Gigabit image is not match the Gigabit card====\n");
				printf("====Please check your image====\n");
				continue;
			}
			rewind(fp);
		}
		rewind(fp);

		//if not 0 mbox will not ok
		if (status) {
			write_reg(dev, 0x10104, 0x70000000);
			write_reg(dev, 0x10108, 0x0);
		} else
			flash_write_unlock(dev);

		mac_addr0_dword0 = flash_read_dword(dev, MAC_ADDR0_WORD0_OFFSET_1G);
		mac_addr0_dword1 = flash_read_dword(dev, MAC_ADDR0_WORD1_OFFSET_1G) & 0xffff;
		mac_addr1_dword0 = flash_read_dword(dev, MAC_ADDR1_WORD0_OFFSET_1G);
		mac_addr1_dword1 = flash_read_dword(dev, MAC_ADDR1_WORD1_OFFSET_1G) & 0xffff;
		mac_addr2_dword0 = flash_read_dword(dev, MAC_ADDR2_WORD0_OFFSET_1G);
		mac_addr2_dword1 = flash_read_dword(dev, MAC_ADDR2_WORD1_OFFSET_1G) & 0xffff;
		mac_addr3_dword0 = flash_read_dword(dev, MAC_ADDR3_WORD0_OFFSET_1G);
		mac_addr3_dword1 = flash_read_dword(dev, MAC_ADDR3_WORD1_OFFSET_1G) & 0xffff;

		if (!opt_c_is_set) {
			mac_valid = (((mac_addr0_dword0 == 0x03040506) && (mac_addr0_dword1 == 0x0202)) || ((mac_addr0_dword0 == 0xffffffff) && (mac_addr0_dword1 == 0xffff))) ? 0 : 1;
			if (mac_valid == 0)
				opt_u_is_set = 1;
		}
		if (DEBUG_ON || debug_mode) printf("mac_valid: %02x\n", mac_valid);
		printf("Start to download No.%x adaptor card [ %02x:%02x.%0x ]:\n", j, dev->bus, dev->dev, dev->func);
		printf("Old: MAC Address0 is: %04x%08x\n", mac_addr0_dword1, mac_addr0_dword0);
		if (!(g_port_num == 1)) {
			printf("     MAC Address1 is: %04x%08x\n", mac_addr1_dword1, mac_addr1_dword0);
			if (!(g_port_num == 2)) {
				printf("     MAC Address2 is: %04x%08x\n", mac_addr2_dword1, mac_addr2_dword0);
				printf("     MAC Address3 is: %04x%08x\n", mac_addr3_dword1, mac_addr3_dword0);
			}
		}

		if (opt_s_is_set == 1) {
			printf("     SN is ");
			for(k = 0; k < 24; k++) {
				serial_num_dword[23-k] = flash_read_dword(dev, PRODUCT_SERIAL_NUM_OFFSET_1G + 4*(23-k));
				printf("%c", serial_num_dword[23-k]);
			}
			for(k = 0; k < 24; k++) {
				SN[k] = serial_num_dword[23-k];
				//printf("%c", SN[k]);
			}
			if (!opt_u_is_set)
				vpd_sn_change_t(dev, SN);
			printf("\n");
		} else {
			serial_num_dword0 = flash_read_dword(dev, PRODUCT_SERIAL_NUM_OFFSET_1G);
			serial_num_dword1 = flash_read_dword(dev, PRODUCT_SERIAL_NUM_OFFSET_1G + 4);
			serial_num_dword2 = flash_read_dword(dev, PRODUCT_SERIAL_NUM_OFFSET_1G + 8);
			printf("     SN is: %02x%08x%08x\n", (serial_num_dword2 & 0xff), serial_num_dword1, serial_num_dword0);
			memset(SN, 0, sizeof(SN));
			sprintf(SN,"%02x%08x%08x",(serial_num_dword2 & 0xff), serial_num_dword1, serial_num_dword0);
			if (!opt_u_is_set)
				vpd_sn_change_t(dev, SN);
		}

		//restore calibration
		for (k = 0; k < (4096 / 4); k++) {
			num[k] = flash_read_dword(dev, 0xfe000 + k * 4);
		}

		// Scan MAC Address
		status = opt_u_is_set;
		while (status) {
			memset(MAC_ADDR, 0, sizeof(MAC_ADDR));
			printf("Please type in New MAC Address: ");
			scanf("%s", (char *)MAC_ADDR);

			status = 0;
			// MAC Address length check, 12 ASCI chars, 6 bytes
			if (strlen(MAC_ADDR) != 12) {
				printf("ERROR: input MAC Address length is illegal! Shall be 12, but length is %d!!\n", (unsigned int)strlen(MAC_ADDR));
				status = 1;
				printf("\n");
				continue;
			}
			// MAC Address HEX format check
			for (i = 0; i < strlen(MAC_ADDR); i++) {
				if (!isxdigit(MAC_ADDR[i])) {
					printf("ERROR: input MAC Address format is illegal! Shall be HEX!!\n");
					status = 1;
					break;
				}
			}
			if (status) {
				printf("\n");
				continue; // Need re-enter MAC Address when error was detected!
			}

			memset(str_dword, 0, sizeof(str_dword));
			for (i = 0; i < 4; i++)
				str_dword[i] = MAC_ADDR[i];
			mac_addr0_dword1 = (unsigned long)strtol(str_dword, NULL, 16) & 0xffffffff;
			// MAC Address unicast check
			if (mac_addr0_dword1 & 0x100) /* check MAC Address bit[40] */ {
				printf("ERROR: input MAC Address is a multicast or broadcast address! Shall be unicast!!\n");
				status = 1;
			}
			if (status) {
				printf("\n");
				continue; // Need re-enter MAC Address when error was detected!
			}

			for (i = 0; i < 8; i++)
				str_dword[i] = MAC_ADDR[i + 4];
			mac_addr0_dword0 = (unsigned long)strtol(str_dword, NULL, 16) & 0xffffffff;

			if (mac_addr0_dword0 == 0xffffffff) {
				mac_addr1_dword0 = 0;
				mac_addr1_dword1 = mac_addr0_dword1 + 1;
				mac_addr2_dword0 = 0;
				mac_addr2_dword1 = mac_addr0_dword1 + 2;
				mac_addr3_dword0 = 0;
				mac_addr3_dword1 = mac_addr0_dword1 + 3;
			} else {
				mac_addr1_dword0 = mac_addr0_dword0 + 1;
				mac_addr1_dword1 = mac_addr0_dword1;
				mac_addr2_dword0 = mac_addr0_dword0 + 2;
				mac_addr2_dword1 = mac_addr0_dword1;
				mac_addr3_dword0 = mac_addr0_dword0 + 3;
				mac_addr3_dword1 = mac_addr0_dword1;
			}
			if (DEBUG_ON || debug_mode) {
				printf("MAC Address0 is: 0x%04x%08x\n", mac_addr0_dword1, mac_addr0_dword0);
				printf("MAC Address1 is: 0x%04x%08x\n", mac_addr1_dword1, mac_addr1_dword0);
				printf("MAC Address2 is: 0x%04x%08x\n", mac_addr2_dword1, mac_addr2_dword0);
				printf("MAC Address3 is: 0x%04x%08x\n", mac_addr3_dword1, mac_addr3_dword0);
			}
		}

		// Scan SN
		status = opt_u_is_set;
		while (status) {
			memset(SN, 0, sizeof(SN));
			printf("Please type in SN: ");
			scanf("%s", (char *)SN);

			status = 0;
			if (opt_s_is_set == 1) {
				// SN length check, 24 ASCI chars
				if (strlen(SN) > 24) {
					printf("ERROR: input SN length is illegal! Shall be less than 24, but length is %d!!\n", (unsigned int)strlen(SN));
					status = 1;
					printf("\n");
					continue;
				}

				if (status) {
					printf("\n");
					continue;
				}
				vpd_sn_change_t(dev, SN);

				memset(str_dword, 0, sizeof(str_dword));
				for(k = 0; k < 24; k++) {
					serial_num_dword[23-k] = (unsigned int)(SN[k]);
				}
			} else {
				// SN length check, 18 ASCI chars
				if (strlen(SN) != 18) {
					printf("ERROR: input SN length is illegal! Shall be 18, but length is %d!!\n", (unsigned int)strlen(SN));
					status = 1;
					printf("\n");
					continue;
				}

				// SN HEX format check
				for (i = 0; i < strlen(SN); i++) {
					if (!isxdigit(SN[i])) {
						printf("ERROR: input SN format is illegal! Shall be HEX!!\n");
						status = 1;
						break;
					}
				}
				if (status) {
					printf("\n");
					continue;
				}
				vpd_sn_change_t(dev, SN);

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

				if (DEBUG_ON || debug_mode) {
					printf("SN is: 0x%02x%08x%08x\n", (serial_num_dword2 & 0xff), serial_num_dword1, serial_num_dword0);
				}
			}
		}

		// Note: for SST flash, need to unlock FLASH after power-on
		if (flash_vendor == FLASH_SST) {
			status = fmgr_usr_cmd_op(dev, 0x6); // write enable
			if (DEBUG_ON || debug_mode) printf("SST flash write enable user command, return status = %0d\n", status);
			status = fmgr_usr_cmd_op(dev, 0x98); // global protection un-lock
			if (DEBUG_ON || debug_mode) printf("SST flash un-lock protection user command, return status = %0d\n", status);
			sleep(1); // 1 s
		}

		// Note: for Spanish FLASH, first 8 sectors (4KB) in sector0 (64KB) need to use a special erase command (4K sector erase)
		if ((flash_vendor == FLASH_SPANISH) && (SKIP_FLASH_ERASE == 0)) {
			write_reg(dev, SPI_CMD_CFG1_ADDR, 0x0103c720);
			for (i = 0; i < 8; i++) {
				flash_erase_sector(dev, i * 128);
				usleep(20 * 1000); // 20 ms
			}
			write_reg(dev, SPI_CMD_CFG1_ADDR, 0x0103c7d8);
		}

		// Winbond Flash, erase chip command is okay, but erase sector doestn't work
		if (SKIP_FLASH_ERASE == 0) {
			if (flash_vendor == FLASH_WINBOND) {
				status = flash_erase_chip(dev);
				if (DEBUG_ON || debug_mode) printf("Erase chip command, return status = %0d\n", status);
				sleep(1); // 1 s
			} else {
				for (i = 0; i < sector_num; i++) {
					status = flash_erase_sector(dev, i * SPI_SECTOR_SIZE);
					if (DEBUG_ON || debug_mode)
						printf("Erase sector[%2d] command, return status = %0d\n", i, status);
					usleep(50 * 1000); // 50 ms
				}
			}
			/*fix : the last sector can not be erase*/
			status = flash_erase_sector(dev, 127 * 8 * 1024);
			sleep(1);
			if (DEBUG_ON || debug_mode) printf("Erase chip is finished.\n\n");
		}

		// Program Image file in dword
		for (i = 0; i < file_length / 4; i++) {
			fread(&read_data, 4, 1, fp);
			if (!opt_c_is_set) {
				skip = ((i * 4 == MAC_ADDR0_WORD0_OFFSET_1G) || (i * 4 == MAC_ADDR0_WORD1_OFFSET_1G) ||
					(i * 4 == MAC_ADDR1_WORD0_OFFSET_1G) || (i * 4 == MAC_ADDR1_WORD1_OFFSET_1G) ||
					(i * 4 == MAC_ADDR2_WORD0_OFFSET_1G) || (i * 4 == MAC_ADDR2_WORD1_OFFSET_1G) ||
					(i * 4 == MAC_ADDR3_WORD0_OFFSET_1G) || (i * 4 == MAC_ADDR3_WORD1_OFFSET_1G) ||
					(i * 4 >= PRODUCT_SERIAL_NUM_OFFSET_1G && i * 4 <= PRODUCT_SERIAL_NUM_OFFSET_1G + 92)||
					(i * 4 >= 0x60 && i*4 < 0xc0)||
					(i* 4 == 0x15c));
			}
			if (read_data != 0xffffffff && !skip) {
				status = flash_write_dword(dev, i * 4, read_data);
				if (status) {
					printf("ERROR: Program 0x%08x @addr: 0x%08x is failed !!\n", read_data, i * 4);
					read_data = flash_read_dword(dev, i * 4);
					printf("       Read data from Flash is: 0x%08x\n", read_data);
					return 1;
				}
			}
			if (i == 0) {
				printf("Start to upgrade PCI [ %02x:%02x.%0x ] flash .... complete   0%%", dev->bus, dev->dev, dev->func);
			} else if (i % 1024 == 0) {
				printf("\b\b\b\b%3d%%", (int)(i * 4 * 100 / file_length));
			}
			fflush(stdout);
		}
		printf("\b\b\b\b%3d%%", (int)(i * 4 * 100 / file_length));
		fflush(stdout);
		printf("\n");

		if (!opt_c_is_set)
		{
			// Re-program MAC address and SN to chip
			flash_write_dword(dev, MAC_ADDR0_WORD0_OFFSET_1G, mac_addr0_dword0);
			flash_write_dword(dev, MAC_ADDR0_WORD1_OFFSET_1G, (mac_addr0_dword1 | 0x80000000)); //lan0
			flash_write_dword(dev, MAC_ADDR1_WORD0_OFFSET_1G, mac_addr1_dword0);
			flash_write_dword(dev, MAC_ADDR1_WORD1_OFFSET_1G, (mac_addr1_dword1 | 0x80000000)); //lan1
			flash_write_dword(dev, MAC_ADDR2_WORD0_OFFSET_1G, mac_addr2_dword0);
			flash_write_dword(dev, MAC_ADDR2_WORD1_OFFSET_1G, (mac_addr2_dword1 | 0x80000000)); //lan2
			flash_write_dword(dev, MAC_ADDR3_WORD0_OFFSET_1G, mac_addr3_dword0);
			flash_write_dword(dev, MAC_ADDR3_WORD1_OFFSET_1G, (mac_addr3_dword1 | 0x80000000)); //lan3

			if (opt_s_is_set == 1) {
				for (k = 0; k < 24; k++) {
					flash_write_dword(dev, PRODUCT_SERIAL_NUM_OFFSET_1G + 4*k, serial_num_dword[k]);
				}
			} else {
				flash_write_dword(dev, PRODUCT_SERIAL_NUM_OFFSET_1G, serial_num_dword0);
				flash_write_dword(dev, PRODUCT_SERIAL_NUM_OFFSET_1G + 4, serial_num_dword1);
				flash_write_dword(dev, PRODUCT_SERIAL_NUM_OFFSET_1G + 8, serial_num_dword2);
			}
		}

		if (!(opt_d_is_set == 1)) {
			for (n = 0; n < 4096 / 4; n++) {
				if (!(num[n] == 0xffffffff))
					flash_write_dword(dev, 0xfe000 + n * 4, num[n]);
			}
		}

		//recheck mac and sn
		mac_addr0_dword0 = flash_read_dword(dev, MAC_ADDR0_WORD0_OFFSET_1G);
		mac_addr0_dword1 = flash_read_dword(dev, MAC_ADDR0_WORD1_OFFSET_1G);
		if (DEBUG_ON || debug_mode) printf("Store MAC0 address: 0x%04x%08x\n", (mac_addr0_dword1 & 0xffff), mac_addr0_dword0);
		if (!(mac_addr0_dword1 & 0x80000000))
			printf("WARNING: MAC Address0 is not active yet, program will active it but please check the address value either!\n");
		mac_addr1_dword0 = flash_read_dword(dev, MAC_ADDR1_WORD0_OFFSET_1G);
		mac_addr1_dword1 = flash_read_dword(dev, MAC_ADDR1_WORD1_OFFSET_1G);
		if (DEBUG_ON || debug_mode) printf("Store MAC1 address: 0x%04x%08x\n", (mac_addr1_dword1 & 0xffff), mac_addr1_dword0);
		if (!(mac_addr1_dword1 & 0x80000000))
			printf("WARNING: MAC Address1 is not active yet, program will active it but please check the address value either!\n");

		mac_addr2_dword0 = flash_read_dword(dev, MAC_ADDR2_WORD0_OFFSET_1G);
		mac_addr2_dword1 = flash_read_dword(dev, MAC_ADDR2_WORD1_OFFSET_1G);
		if (DEBUG_ON || debug_mode) printf("Store MAC2 address: 0x%04x%08x\n", (mac_addr2_dword1 & 0xffff), mac_addr2_dword0);
		if (!(mac_addr1_dword1 & 0x80000000))
			printf("WARNING: MAC Address2 is not active yet, program will active it but please check the address value either!\n");
		mac_addr3_dword0 = flash_read_dword(dev, MAC_ADDR3_WORD0_OFFSET_1G);
		mac_addr3_dword1 = flash_read_dword(dev, MAC_ADDR3_WORD1_OFFSET_1G);
		if (DEBUG_ON || debug_mode) printf("Store MAC3 address: 0x%04x%08x\n", (mac_addr3_dword1 & 0xffff), mac_addr3_dword0);
		if (!(mac_addr1_dword1 & 0x80000000))
			printf("WARNING: MAC Address3 is not active yet, program will active it but please check the address value either!\n");

		printf("New: MAC Address0 is: %04x%08x\n", (mac_addr0_dword1 & 0xffff), mac_addr0_dword0);
		if (!(g_port_num == 1)) {
			printf("     MAC Address1 is: %04x%08x\n", (mac_addr1_dword1 & 0xffff), mac_addr1_dword0);
			if (!(g_port_num == 2)) {
				printf("     MAC Address2 is: %04x%08x\n", (mac_addr2_dword1 & 0xffff), mac_addr2_dword0);
				printf("     MAC Address3 is: %04x%08x\n", (mac_addr3_dword1 & 0xffff), mac_addr3_dword0);
			}
		}

		if (opt_s_is_set == 1) {
			printf("New SN is ");
			for(k = 0; k < 24; k++) {
				serial_num_dword[23-k] = flash_read_dword(dev, PRODUCT_SERIAL_NUM_OFFSET_1G + 4*(23-k));
				printf("%c", serial_num_dword[23-k]);
			}
			printf("\n");
		} else {
			serial_num_dword0 = flash_read_dword(dev, PRODUCT_SERIAL_NUM_OFFSET_1G);
			serial_num_dword1 = flash_read_dword(dev, PRODUCT_SERIAL_NUM_OFFSET_1G + 4);
			serial_num_dword2 = flash_read_dword(dev, PRODUCT_SERIAL_NUM_OFFSET_1G + 8);
			if (DEBUG_ON || debug_mode) printf("Store SN: %0x%08x%08x\n", (serial_num_dword2 & 0xff), serial_num_dword1, serial_num_dword0);
			printf("     SN is: %02x%08x%08x\n\n", (serial_num_dword2 & 0xff), serial_num_dword1, serial_num_dword0);
		}

		if (!opt_c_is_set) {
			/* load vpd tent*/
			FILE *fd = NULL;
			fd = fopen("vpd_tent", "r");
			if (NULL == fd) {
				printf("ERROR: Can't open vpd_tent File!\n");
				return 0;
			}
			fseek(fd, 0, SEEK_END);
			int flength = ftell(fd);
			rewind(fd);

			// Program Image file in dword
			for (i = 0; i < flength / 4; i++) {
				fread(&read_data, 4, 1, fd);
				if (read_data != 0xffffffff) {
					status = flash_write_dword(dev, 0x60 + i * 4, read_data);
					if (status) {
						printf("ERROR: Program 0x%08x @addr: 0x%08x is failed !!\n", read_data, 0x60 + i * 4);
						read_data = flash_read_dword(dev, 0x60 + i * 4);
						printf("       Read data from Flash is: 0x%08x\n", read_data);
						return 1;
					}
				}
			}

			rewind(fd); // Note: need rewind fp after an upgrading, or fread will not get anything
			fclose(fd);

			test2 = 0;
			test3 = 0;
			chksum = 0;
			for (i = 0; i < 0x400/2 ; i ++) {
				test2 = flash_read_dword(dev, 2*i);
				test3 = flash_read_dword(dev, 2*i + 1);
				if (2*i == 0x15e) {
					test2 = 0x00;
					test3 = 0x00;
				}
				chksum += (test3 << 8 | test2);
				if (DEBUG_ON || debug_mode)printf("('0x%x', '0x%x')\n",test3, test2);
			}
			chksum = 0xbaba - chksum;
			chksum &= 0xffff;
			status = flash_write_dword(dev, 0x15e, 0xffff0000 | chksum);
			if (DEBUG_ON || debug_mode) printf("chksum : %04x\n", chksum);
		}
		system("rm -rf vpd_tent");

		upgrade_read_data |= 0x80000000;
		write_reg(dev, 0x10200, upgrade_read_data);

		don_card_num += 1;
	}

	printf("\n");
	fclose(fp);
	pci_cleanup(pacc); /* Close everything */

	if (don_card_num == 0) {
		printf("Only %0d cards are upgraded for %0d cards!!\n\n", don_card_num, all_card_num);
		return 1;
	} else {
		printf("[ ^_^ ] [ QAQ ] Emnic PCI Utils upgrading is succeeded! %0d cards are upgraded!!\n\n", don_card_num);
		return 0;
	}
}
#endif
