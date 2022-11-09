#ifndef _XGIGABIT_H_
#define _XGIGABIT_H_
#include "functions.h"

#ifdef SPI_CLK_DIV
#undef SPI_CLK_DIV
#define SPI_CLK_DIV 2
#endif

extern int Tgigabit_nics(void);

int Tgigabit_nics(void)
{
	struct pci_access *pacc = NULL;
	struct pci_dev *dev = NULL;
	struct pci_dev *devs[MAX_RAPTOR_CARD_NUM];
	u32 dev_num = 0;
	u32 active_dev_num = 0;
	u32 card_v, image_v;

	FILE *fp = NULL;
	long int file_length = 0;
	u32 sector_num = 0;

	u32 read_data = 0;
	u32 upgrade_read_data = 0;
	u8 status = 0;
	u8 skip = 0;
	u8 dl_err = 0; // flash download error
	u32 i = 0;
	u32 j = 0, k = 0;

	u32 chksum = 0;
	u8 test3 = 0;
	u8 test2 = 0;

	pacc = pci_alloc(); /* Get the pci_access structure */
	pacc->error = die;

	/* Set all options you want -- here we stick with the defaults */
	pci_init(pacc);     /* Initialize the PCI library */
	pci_scan_bus(pacc); /* We want to get the list of devices */

	for (i = 0; i < MAX_RAPTOR_CARD_NUM; i++)
	{
		devs[i] = NULL;
	}

	for (dev = pacc->devices; dev; dev = dev->next) /* Iterate over all devices */
	{
		if ((dev->vendor_id == VENDOR_ID || dev->vendor_id == WX_VENDOR_ID) && ((dev->device_id == DEVICE_ID_1001) || (dev->device_id == DEVICE_ID_2001)) && (dev->func == 0))
		{
			devs[dev_num] = dev;
			if (DEBUG_ON || debug_mode) printf("bus=%0x, dev=%0x, func=%0x\n", dev->bus, dev->dev, dev->func);
			dev_num += 1;
		}
	}
	if (dev_num == 0)
	{
		printf("\nRaptor PCI Utils tool report fatal error:\n");
		printf("ERROR: No any our adaptor cards were found!\n\n");
		return 1;
	}
#if 0
	printf("----opt_f_is_set----%d-%d-%d-%d-%d-%d-%d\n",opt_f_is_set,
						opt_m_is_set,
						opt_a_is_set,
						opt_u_is_set,
						opt_c_is_set,
						opt_p_is_set,
						opt_d_is_set
						);
#endif
	/* Read Image File */
	if (!opt_f_is_set)
		strcpy(IMAGE_FILE, "image/prd_flash_golden.img");

	fp = fopen(IMAGE_FILE, "r");
	if (NULL == fp)
	{
		printf("\nRaptor PCI Utils tool report fatal error:\n");
		printf("ERROR: Can't open image file, %s!\n\n", IMAGE_FILE);
		return 1;
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

	u32 check_t = 0;

	/*check sub_id*/
	fseek(fp, 80 * sizeof(u8), SEEK_SET);
	fread(&check_t, 2, 1, fp);
	//printf("%04x ", check_t);
	if (check_t != 0xffff)
	{
		printf("==It is not a 10Gigabit nic image.==\n");
		printf("==Please check your image==\n");
		return 0;
	}
	rewind(fp);

	if (opt_s1_is_set != 1) {
		/* Select PCI device */
		if (!opt_a_is_set) {
			// To select the unique PCI device
			if (dev_num == 1) {
				dev_index = 0;
			} else { // To select a PCI device when not to download all of device
				printf("\nMore than one of our networking adaptor cards were found, but without of '-A' option specified. Please select a adaptor to download.\n");
				while (dev_index >= dev_num) {
					for (i = 0; i < dev_num; i++) {
						if (i == 0)
							printf(" [ %0d ] %02x:%02x.%02x", i, devs[i]->bus, devs[i]->dev, devs[i]->func);
						else if (i == (dev_num - 1))
							printf(" [ %0d ] %02x:%02x.%02x : ", i, devs[i]->bus, devs[i]->dev, devs[i]->func);
						else
							printf(", [ %0d ] %02x:%02x.%02x", i, devs[i]->bus, devs[i]->dev, devs[i]->func);
					}
					printf("\nPlease select slot index: ");
					scanf("%d", (int *)&dev_index);
					safe_flush(stdin);
					printf("\n");
					if (dev_index >= dev_num)
						printf("Your selection is out of range! Please select again!!\n");
				}
			}
			active_dev_num = 1;
		} else
			active_dev_num = dev_num;

	} else {
		dev_index =0;
		dev_num = 1;
		active_dev_num = 1;
	}

	/* Select Flash Manufacturer */
	if (!opt_m_is_set) {
		flash_vendor = FLASH_SST;
	}

	if (flash_vendor != FLASH_WINBOND && flash_vendor != FLASH_SPANISH && flash_vendor != FLASH_SST)
		flash_vendor = FLASH_WINBOND;
	if (DEBUG_ON || debug_mode)
		printf("Select [ %0d ] flash for downloading.\n", flash_vendor);

	/* Select PCI Device */
	printf("\nRaptor PCI Utils tool is started.\n");
	printf("We will download %d in %0d cards depends on the configuration.\n\n", dev_num, active_dev_num);

	for (j = 0; j < dev_num; j++) {
		if (opt_s1_is_set == 1)
			dev = global_dev;
		else {
			if (!opt_a_is_set && dev_index != j)
				continue;
			dev = devs[j]; // select current PCI device
		}

		if (!((dev->vendor_id == VENDOR_ID || dev->vendor_id == WX_VENDOR_ID) && 
			((dev->device_id == DEVICE_ID_1001) || (dev->device_id == DEVICE_ID_2001)))) {
			if (DEBUG_ON || debug_mode) printf("bus=%0d, dev=%0d, func=%0d\n", dev->bus, dev->dev, dev->func);
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
			printf("The Following  adaptor cards has been upgraded:\n");
			printf("[ No.%x ] %02x:%02x.%0x\n", j, dev->bus, dev->dev, dev->func);
			printf("Please reboot your machine and retry \n");
			continue;
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
			if ((sub_id & 0xfff) == (s_sub_id & 0xfff)) {
				printf("It is a right image\n");
			} else if (s_sub_id == 0xffff) {
				printf("update anyway\n");
			} else {
				printf("====The 10Gigabit image is not match the 10Gigabit card====\n");
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
			if (!((dev->device_id & 0xfff0) == (dev_id & 0xfff0)) && !(dev_id == 0xffff)) {
				printf("====The 10Gigabit image is not match the 10Gigabit card====\n");
				printf("====Please check your image====\n");
				continue;
			}
			rewind(fp);
		}
		rewind(fp);

		//read nic version
		card_v = flash_read_dword(dev, 0x13a) & 0x000f00ff;
		if (DEBUG_ON || debug_mode) printf("check_chip_version=card_v: %x\n", card_v);
		//read image version
		fseek(fp, 0x13a, SEEK_SET);
		//image_v = flash_read_dword(dev, 0x13a) & 0x000f00ff;
		fread(&image_v, 2, 2, fp);
		image_v = image_v & 0x000f00ff;
		if (DEBUG_ON || debug_mode) printf("check_image_version=image_v: %x\n", image_v);
		rewind(fp);

		if ((card_v >= 0x2000b) && (image_v >= 0x2000b))
			opt_e_is_set = opt_e_is_set;
		else
			opt_e_is_set = 0;

		//if not 0 mbox will not ok
		if (status) {
			write_reg(dev, 0x10104, 0x70000000);
			write_reg(dev, 0x10108, 0x0);
		} else
			flash_write_unlock(dev);

		//record something import
		mac_addr0_dword0 = flash_read_dword(dev, MAC_ADDR0_WORD0_OFFSET);
		mac_addr0_dword1 = flash_read_dword(dev, MAC_ADDR0_WORD1_OFFSET) & 0xffff;
		mac_addr1_dword0 = flash_read_dword(dev, MAC_ADDR1_WORD0_OFFSET);
		mac_addr1_dword1 = flash_read_dword(dev, MAC_ADDR1_WORD1_OFFSET) & 0xffff;
		txeq0_ctrl0 = flash_read_dword(dev, SP0_TXEQ_CTRL0_OFFSET);
		txeq0_ctrl1 = flash_read_dword(dev, SP0_TXEQ_CTRL1_OFFSET);
		txeq1_ctrl0 = flash_read_dword(dev, SP1_TXEQ_CTRL0_OFFSET);
		txeq1_ctrl1 = flash_read_dword(dev, SP1_TXEQ_CTRL1_OFFSET);

		//check mac is vaild
		if (!opt_c_is_set) {
			mac_valid = (((mac_addr0_dword0 == 0x03040506) && (mac_addr0_dword1 == 0x0202)) || ((mac_addr0_dword0 == 0xffffffff) && (mac_addr0_dword1 == 0xffff))) ? 0 : 1;
			if (mac_valid == 0)
				opt_u_is_set = 1;
		}
		printf("Start to download No.%x adaptor card [ %02x:%02x.%0x ]:\n", j, dev->bus, dev->dev, dev->func);
		printf("Old: MAC Address0 is: %04x%08x\n", mac_addr0_dword1, mac_addr0_dword0);
		printf("     MAC Address1 is: %04x%08x\n", mac_addr1_dword1, mac_addr1_dword0);

		if (opt_s_is_set == 1) {
			printf("Old: SN is ");
			for(k = 0; k < 24; k++) {
				serial_num_dword[23-k] = flash_read_dword(dev, PRODUCT_SERIAL_NUM_OFFSET + 4*(23-k));
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
			serial_num_dword0 = flash_read_dword(dev, PRODUCT_SERIAL_NUM_OFFSET);
			serial_num_dword1 = flash_read_dword(dev, PRODUCT_SERIAL_NUM_OFFSET + 4);
			serial_num_dword2 = flash_read_dword(dev, PRODUCT_SERIAL_NUM_OFFSET + 8);
			printf("     SN is: %02x%08x%08x\n", (serial_num_dword2 & 0xff), serial_num_dword1, serial_num_dword0);
			memset(SN, 0, sizeof(SN));
			sprintf(SN,"%02x%08x%08x",(serial_num_dword2 & 0xff), serial_num_dword1, serial_num_dword0);
			if (!opt_u_is_set)
				vpd_sn_change_t(dev, SN);
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
				if (!isxdigit(MAC_ADDR[i]))
				{
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
			} else {
				mac_addr1_dword0 = mac_addr0_dword0 + 1;
				mac_addr1_dword1 = mac_addr0_dword1;
			}
			if (DEBUG_ON || debug_mode) {
				printf("MAC Address0 is: 0x%04x%08x\n", mac_addr0_dword1, mac_addr0_dword0);
				printf("MAC Address1 is: 0x%04x%08x\n", mac_addr1_dword1, mac_addr1_dword0);
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

		if (flash_vendor == FLASH_SST)
		// Note: for SST flash, need to unlock FLASH after power-on
		{
			status = fmgr_usr_cmd_op(dev, 0x6); // write enable
			if (DEBUG_ON || debug_mode)
				printf("SST flash write enable user command, return status = %0d\n", status);
			usleep(500 * 1000); // 500 ms
			status = fmgr_usr_cmd_op(dev, 0x98); // global protection un-lock
			if (DEBUG_ON || debug_mode)
				printf("SST flash un-lock protection user command, return status = %0d\n", status);
			usleep(500 * 1000); // 500 ms
		}

		if (flash_vendor == FLASH_SPANISH)
		// Note: for Spanish FLASH, first 8 sectors (4KB) in sector0 (64KB) need to use a special erase command (4K sector erase)
		{
			write_reg(dev, SPI_CMD_CFG1_ADDR, 0x0103c720);
			for (i = 0; i < 8; i++)
			{
				flash_erase_sector(dev, i * 128);
				usleep(200 * 1000); // 200 ms
			}
			write_reg(dev, SPI_CMD_CFG1_ADDR, 0x0103c7d8);
		}

		// Winbond Flash, erase chip command is okay, but erase sector doestn't work
		status = flash_erase_chip(dev);
		if (DEBUG_ON || debug_mode)
			printf("Erase chip command, return status = %0d\n", status);
		sleep(2); // 2 s
		for (i = 0; i < sector_num; i++) {
			status = flash_erase_sector(dev, i * SPI_SECTOR_SIZE);
			if (DEBUG_ON || debug_mode)
				printf("Erase sector[%2d] command, return status = %0d\n", i, status);
			usleep(200 * 1000); // 200 ms
		}
		/*fix : the last sector can not be erase*/
		status = flash_erase_sector(dev, 127 * 8 * 1024);
		sleep(1);
		if (DEBUG_ON || debug_mode)
			printf("Erase chip is finished.\n\n");

		// Program Image file in dword
		dl_err = 0;
		for (i = 0; i < file_length / 4; i++) {
			fread(&read_data, 4, 1, fp);
			if (!opt_c_is_set) {
				skip = ((i * 4 == MAC_ADDR0_WORD0_OFFSET) || (i * 4 == MAC_ADDR0_WORD1_OFFSET) ||
					(i * 4 == MAC_ADDR1_WORD0_OFFSET) || (i * 4 == MAC_ADDR1_WORD1_OFFSET) ||
					(i * 4 >= PRODUCT_SERIAL_NUM_OFFSET && i * 4 <= PRODUCT_SERIAL_NUM_OFFSET + 92) ||
					(i * 4 >= 0x500 && i*4 < 0x600) ||
					(i* 4 == 0x15c));
				if (opt_e_is_set == 1)
				skip |= (i * 4 == SP0_TXEQ_CTRL0_OFFSET) || (i * 4 == SP0_TXEQ_CTRL1_OFFSET) ||
					(i * 4 == SP1_TXEQ_CTRL0_OFFSET) || (i * 4 == SP1_TXEQ_CTRL1_OFFSET);
			}
			if (read_data != 0xffffffff && !skip) {
				status = flash_write_dword(dev, i * 4, read_data);
				if (status) {
					printf("\nRaptor PCI Utils tool report fatal error:\n");
					printf("ERROR: Program 0x%08x @addr: 0x%08x is failed !!\n", read_data, i * 4);
					read_data = flash_read_dword(dev, i * 4);
					printf("       Read data from Flash is: 0x%08x\n", read_data);
					printf("Download is failed!\n\n");
					dl_err = 1;
					break;
				}
			}
			if (i == 0) {
				printf("Start to download image to adaptor ...... complete   0%%");
			} else if (i % 1024 == 0) {
				printf("\b\b\b\b%3d%%", (int)(i * 4 * 100 / file_length));
			}
			fflush(stdout);
		}
		if (status == 0) {
			printf("\b\b\b\b%3d%%", (int)(i * 4 * 100 / file_length));
			fflush(stdout);
			printf("\n");

			if (!opt_c_is_set)
			{
				// Re-program MAC address and SN to chip
				flash_write_dword(dev, MAC_ADDR0_WORD0_OFFSET, mac_addr0_dword0);
				flash_write_dword(dev, MAC_ADDR0_WORD1_OFFSET, (mac_addr0_dword1 | 0x80000000));
				flash_write_dword(dev, MAC_ADDR1_WORD0_OFFSET, mac_addr1_dword0);
				flash_write_dword(dev, MAC_ADDR1_WORD1_OFFSET, (mac_addr1_dword1 | 0x80000000));

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

			if (opt_e_is_set == 1) {
				flash_write_dword(dev, SP0_TXEQ_CTRL0_OFFSET, txeq0_ctrl0);
				flash_write_dword(dev, SP0_TXEQ_CTRL1_OFFSET, txeq0_ctrl1); //lan1
				flash_write_dword(dev, SP1_TXEQ_CTRL0_OFFSET, txeq1_ctrl0);
				flash_write_dword(dev, SP1_TXEQ_CTRL1_OFFSET, txeq1_ctrl1); //lan1

				txeq0_ctrl0 = 0;
				txeq0_ctrl1 = 0;
				txeq0_ctrl0 = flash_read_dword(dev, SP0_TXEQ_CTRL0_OFFSET);
				txeq0_ctrl1 = flash_read_dword(dev, SP0_TXEQ_CTRL1_OFFSET);
				printf("lan0 - 0x18036 : %04x - 0x18037 : %04x\n", txeq0_ctrl0, txeq0_ctrl1);
				ffe_pre = txeq0_ctrl0 & 0x3f;
				ffe_main  = (txeq0_ctrl0 >> 8) &0x3f;
				ffe_post = txeq0_ctrl1 & 0x3f;
				printf("lan0 : main: %d - pre: %d - post: %d\n", ffe_main, ffe_pre, ffe_post);
				txeq1_ctrl0 = 0;
				txeq1_ctrl1 = 0;
				txeq1_ctrl0 = flash_read_dword(dev, SP1_TXEQ_CTRL0_OFFSET);
				txeq1_ctrl1 = flash_read_dword(dev, SP1_TXEQ_CTRL1_OFFSET);
				printf("lan1 - 0x18036 : %04x - 0x18037 : %04x\n", txeq1_ctrl0, txeq1_ctrl1);
				ffe_pre = txeq1_ctrl0 & 0x3f;
				ffe_main  = (txeq1_ctrl0 >> 8) &0x3f;
				ffe_post = txeq1_ctrl1 & 0x3f;
				printf("lan1 : main: %d - pre: %d - post: %d\n", ffe_main, ffe_pre, ffe_post);
			} else {
				txeq0_ctrl0 = 0;
				txeq0_ctrl1 = 0;
				txeq0_ctrl0 = flash_read_dword(dev, SP0_TXEQ_CTRL0_OFFSET);
				txeq0_ctrl1 = flash_read_dword(dev, SP0_TXEQ_CTRL1_OFFSET);
				printf("lan0 - 0x18036 : %04x - 0x18037 : %04x\n", txeq0_ctrl0, txeq0_ctrl1);
				ffe_pre = txeq0_ctrl0 & 0x3f;
				ffe_main  = (txeq0_ctrl0 >> 8) &0x3f;
				ffe_post = txeq0_ctrl1 & 0x3f;
				printf("lan0 : main: %d - pre: %d - post: %d\n", ffe_main, ffe_pre, ffe_post);
				txeq1_ctrl0 = 0;
				txeq1_ctrl1 = 0;
				txeq1_ctrl0 = flash_read_dword(dev, SP1_TXEQ_CTRL0_OFFSET);
				txeq1_ctrl1 = flash_read_dword(dev, SP1_TXEQ_CTRL1_OFFSET);
				printf("lan1 - 0x18036 : %04x - 0x18037 : %04x\n", txeq1_ctrl0, txeq1_ctrl1);
				ffe_pre = txeq1_ctrl0 & 0x3f;
				ffe_main  = (txeq1_ctrl0 >> 8) &0x3f;
				ffe_post = txeq1_ctrl1 & 0x3f;
				printf("lan1 : main: %d - pre: %d - post: %d\n", ffe_main, ffe_pre, ffe_post);
			}
		}

		mac_addr0_dword0 = flash_read_dword(dev, MAC_ADDR0_WORD0_OFFSET);
		mac_addr0_dword1 = flash_read_dword(dev, MAC_ADDR0_WORD1_OFFSET) & 0xffff;
		mac_addr1_dword0 = flash_read_dword(dev, MAC_ADDR1_WORD0_OFFSET);
		mac_addr1_dword1 = flash_read_dword(dev, MAC_ADDR1_WORD1_OFFSET) & 0xffff;
		printf("New: MAC Address0 is: 0x%04x%08x\n", mac_addr0_dword1, mac_addr0_dword0);
		printf("     MAC Address1 is: 0x%04x%08x\n", mac_addr1_dword1, mac_addr1_dword0);
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

		rewind(fp); // Note: need rewind fp after an upgrading, or fread will not get anything

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
					status = flash_write_dword(dev, 0x500 + i * 4, read_data);
					if (status) {
						printf("ERROR: Program 0x%08x @addr: 0x%08x is failed !!\n", read_data, 0x500 + i * 4);
						read_data = flash_read_dword(dev, 0x500 + i * 4);
						printf("       Read data from Flash is: 0x%08x\n", read_data);
						return 1;
					}
				}
			}

			rewind(fd); // Note: need rewind fp after an upgrading, or fread will not get anything
			fclose(fd);

			rewind(fp);
			chksum = 0;
			for (i = 0; i < 0x1000/2 ; i ++) {
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

		if (dl_err == 0) {
			upgrade_read_data |= 0x80000000;
			write_reg(dev, 0x10200, upgrade_read_data);
			don_card_num += 1;
		}
	}

	printf("\n");
	fclose(fp);
	pci_cleanup(pacc); /* Close everything */

	if (don_card_num == 0) {
		printf("[ERROR] Raptor PCI Utils upgrading is failed! Only %0d cards are upgraded for %0d cards!!\n\n", don_card_num, all_card_num);
		return 1;
	} else {
		printf("[ ^_^ ] Raptor PCI Utils upgrading is succeeded! %0d cards are upgraded!!\n\n", don_card_num);
		return 0;
	}
}
#endif