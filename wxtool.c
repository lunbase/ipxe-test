#include "wxtool.h"

extern int upgrade_flash(int argc, char **argv);
extern int parse_options(int argc, char **argv);

static void NONRET PCI_PRINTF(1,2)
	parse_err(const char *msg, ...)
{
	va_list args;
	va_start(args, msg);
	fprintf(stderr, "wxtool: ");
	vfprintf(stderr, msg, args);
	fprintf(stderr, ".\nTry `wxtool --help' for more information.\n");
	exit(1);
}

static struct pci_dev **
select_devices(struct pci_filter *filt)
{
	struct pci_dev *z, **a, **b;
	int cnt = 1;

	for (z = pacc_wx->devices; z; z = z->next)
		if (pci_filter_match(filt, z))
			cnt++;
	if (cnt == 1) {
		return NULL;
	}
	a = b = xmalloc(sizeof(struct device *) * cnt);
	for (z = pacc_wx->devices; z; z = z->next)
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

static int parse_filter(int argc, char **argv, int i,
					struct pci_filter *filter, int *type)
{
	char *c = argv[i++];
	char *d = NULL;
	u32 val;
	u32 val1;
	u32 val2;
	u64 mac;
	u16 ctrl;
	struct pci_dev *dev = NULL;
	char cmd[256];
	int func_num = 0;
	int k;

	if (DEBUG_ON || debug_mode) printf("parse_filter===1\n");
	if (!c[1] || !strchr("swdremnfcdWEilStFZ", c[1]))
		parse_err("Invalid option -%c", c[1]);

	if (DEBUG_ON || debug_mode) printf("parse_filter===2\n");
	if (c[2]) {
		d = (c[2] == '=') ? c+3 : c+2;
	} else if (c[1] == 't') {
		if (argv[i] && argv[i][0]!= '-') {
				d = argv[i++];
		}
		if (argv[i] && strlen(argv[i]) == 1) {
			d = argv[i++];
		}
	} else if (i < argc ){
		//printf("------%x---\n",strchr("fc", c[1]));
		if (!strchr("fcdS", c[1]))
			d = argv[i++];
	} else if (strchr("Wi", c[1])){
		;
	} else
		parse_err("Option -%c requires an argument", c[1]);
	if (DEBUG_ON || debug_mode) printf("parse_filter===3\n");

	if (c[1] != 's' && !global_dev) {
		printf("You need select a device use \"-s\" and \"-s\" should be the first option.\n");
		return 0;
	}

	switch (c[1]) {
	case 's':
		if (d = pci_filter_parse_slot(filter, d))
			parse_err("Unable to parse filter -s %s", d);
		break;
	case 'Z': /* Image File */
		c = d;
		strcpy(IMAGE_FILE, d);
		if (DEBUG_ON || debug_mode)
			printf("F option is %s\n", IMAGE_FILE);
		opt_f_is_set = 1;
		update_region(global_dev);
		break;
	case 'c':
		opt_access = 1;//access to cab
		printf("access to cab\n");
		break;
	case 'S':
		opt_s_is_set = 1;
		break;
	case 'd':
		opt_access = 3;//access to mdio
		printf("access to mdio\n");
		break;
	case 'f':
		opt_access = 2;//access to flash
		printf("access to flash\n");
		break;
	case 'W':
		//opt_access = 2;//access to flash
		printf("wavetool....\n");
		wavetool(global_dev);
		break;
	case 'i':
		printf("show nic info\n");
		show_nic_info(global_dev);
		break;
	case 't':
		if (i > argc) { printf("Please check arguments.\n"); return 0;}
		if (d && strlen(d) != 1) {
			printf("invalid arguments for option -t\n");
			return 0;
		}

		if ( !d ||*d == '1') {
			printf("dump image in nic\n");
			dump_image(global_dev);
		} else if (*d == '0') {
			printf("work around\n");
			w_a_check_workaround(global_dev);
		} else if (*d == '2') {
			printf("========= start ===\n");
			for (dev = pacc_wx->devices; dev; dev = dev->next) { /* Iterate over all devices */
				if ((dev->domain == global_dev->domain) && 
					(dev->bus == global_dev->bus) &&
					(dev->dev == global_dev->dev)) 
					func_num += 1;
			}
			printf("=1=get func_num=%d==\n", func_num);
			printf("=2=chip bus reset==\n");
			write_reg(global_dev, 0x1000c, 0x80000000);
			usleep(1000*1000);

			printf("=3=pci bus reset===\n");
			ctrl = pci_read_word(global_dev, PCI_BRIDGE_CONTROL);
			ctrl = pci_write_word(global_dev, PCI_BRIDGE_CONTROL, ctrl |= PCI_BRIDGE_CTL_BUS_RESET);
			usleep(20*1000);
			printf("domain:%x, bus=%0x, dev=%0x\n", global_dev->domain, global_dev->bus, global_dev->dev);
			printf("=4=remove pci=====\n");
			for(k = 0; k < func_num; k++) {
				memset(cmd, 0, sizeof(cmd));
				sprintf(cmd, "echo '1' > /sys/bus/pci/devices/%04x\\:%02x:%02x.%x/remove", 
						global_dev->domain, global_dev->bus, global_dev->dev, k); 
				//system("echo '1' > /sys/bus/pci/devices/0000\\:01:00.0/remove");
				//system("echo '1' > /sys/bus/pci/devices/0000\\:01:00.1/remove");
				//system("echo '1' > /sys/bus/pci/devices/0000\\:01:00.2/remove");
				//system("echo '1' > /sys/bus/pci/devices/0000\\:01:00.3/remove");
				system(cmd);
			}
			printf("=5=pci rescan =====\n");
			sleep(3);
			system("echo '1' > /sys/bus/pci/rescan");
			printf("domain:%x, bus=%0x, dev=%0x\n", global_dev->domain, global_dev->bus, global_dev->dev);
			usleep(20*1000);
			printf("========= end =====\n");
		} else {
			printf("invalid arguments for option -t\n");
		}
		break;
	case 'r':
		c = d;
		val = strtoul(d, &c, 0);
		//printf("val= %x\n",val);

		if (opt_access == 2)
			flash_read(global_dev, val);
		else if (opt_access == 1)
			cab_read(global_dev, val);
		else if (opt_access == 3) {
			d = argv[i++];
			c = d;
			if (i > argc) { printf("Please check arguments.\n"); return i;}
			val1 = strtoul(d, &c, 0);
			//printf("val1= %x\n",val1);
			mdio_read(global_dev, val1, val);
		} else
			pci_read_reg(global_dev, val);
		break;
	case 'l':
		//printf("val= %x\n",val);
		if (strlen(d) != 1) {
			printf("invalid arguments for option -l\n");
			return 0;
		}
		if (*d == '1') {
			flash_write_lock(global_dev);
		} else if (*d == '0') {
			flash_write_unlock(global_dev);
		} else {
			printf("invalid arguments for option -l\n");
			return 0;
		}
		break;
	case 'e':
		c = d;
		val = strtoul(d, &c, 0);
		//printf("val= %x\n",val);
		if (*argv[i++] == '1') {
			val1 = 1;
		} else {
			val1 = 0;
		}
		//printf("val1= %x\n",val1);
		flash_erase(global_dev, val, val1);
		break;
	case 'm':
		c = d;
		//mac = strtoul(d, &c, 16);
		if (strlen(c) != 12) {
			printf("ERROR: input MAC Address length is illegal! Shall be 12, but length is %d!!\n", (unsigned int)strlen(c));
			return 0;
		}
		// MAC Address HEX format check
		for (i = 0; i < strlen(c); i++) {
			if (!isxdigit(c[i]))
			{
				printf("ERROR: input MAC Address format is illegal! Shall be HEX!!\n");
				return 0;
			}
		}
		mac = strtol(d, &c, 16);
		//printf("mac= %lx\n",mac);
		//mac = d;
		if ((mac >> 40) & 0x1) {
			printf("ERROR: input MAC Address is a multicast or broadcast address! Shall be unicast!!\n");
			return 0;
		} else {
			write_reg(global_dev, 0x10104, 0x70000000);
			val = read_reg(global_dev, 0x10108);
			if (val) {
				printf("ERROR: flash should be unlock first!!\n");
				return 0;
			}
			mac_change(global_dev, mac);
		}
		break;
	case 'n':
		c = d;
		if (opt_s_is_set == 1) {
			if (strlen(c) > 24) {
				printf("ERROR: input SN length is illegal! Shall be less than 24, but length is %d!!\n", (unsigned int)strlen(c));
				printf("\n");
				return i;
			}
		} else {
			// SN length check, 18 ASCI chars
			if (strlen(c) != 18) {
				printf("ERROR: input SN length is illegal! Shall be 18, but length is %d!!\n", (unsigned int)strlen(c));
				printf("\n");
				return 0;
			}

			// SN HEX format check
			for (i = 0; i < strlen(c); i++) {
				if (!isxdigit(c[i])) {
					printf("ERROR: input SN format is illegal! Shall be HEX!!\n");
					return 0;
				}
			}
		}

		write_reg(global_dev, 0x10104, 0x70000000);
		val = read_reg(global_dev, 0x10108);
		if (val) {
			printf("ERROR: flash should be unlock first!!\n");
			return 0;
		}

		sprintf(SN, "%s", c);
		sn_change(global_dev);
		break;
	case 'w':
		c = d;
		val = strtoul(d, &c, 0);
		if (c == d || strlen(c)) {
			parse_err("Unable to parse filter -w %s", d);
			return 0;
		}
		//printf("val= %x\n",val);
		/****************/
		d = argv[i++];
		if (i > argc) { printf("Please check arguments.\n"); return 0;}
		c = d;
		val1 = strtoul(d, &c, 0);
		if (c == d || strlen(c)) {
			parse_err("Unable to parse filter -w %s", d);
			return 0;
		}

		//printf("val= %x\n",val1);
		if (opt_access == 2)
			flash_write(global_dev,val,val1);
		else if (opt_access == 1)
			cab_write(global_dev,val,val1);
		else if (opt_access == 3) {
			/****************/
			d = argv[i++];
			if (i > argc) { printf("Please check arguments.\n"); return 0;}
			c = d;
			val2 = strtoul(d, &c, 0);
			if (c == d || strlen(c)) {
				parse_err("Unable to parse filter -w %s", d);
				return 0;
			}
			//printf("val= %x\n",val2);
			mdio_write(global_dev, val1, val2, val);
		}
		else
			pci_write_reg(global_dev,val,val1);

		break;
	case 'E':
		c = d;
		val = strtoul(d, &c, 0);
		if (c == d || strlen(c)) {
			parse_err("Unable to parse filter -E %s", d);
			return 0;
		}

		//printf("val= %x\n",val);
		d = argv[i++];
		if (i > argc) { printf("Please check arguments.\n"); return i;}
		c = d;
		val1 = strtoul(d, &c, 0);
		//printf("val= %x\n",val1);
		if (c == d || strlen(c)) {
			parse_err("Unable to parse filter -E %s", d);
			return 0;
		}
		d = argv[i++];
		if (i > argc) { printf("Please check arguments.\n"); return i;}
		c = d;
		val2 = strtoul(d, &c, 0);
		if (c == d || strlen(c)) {
			parse_err("Unable to parse filter -E %s", d);
			return 0;
		}

		//printf("val= %x\n",val2);
		if (val > 63 || val1 > 63 || val2 > 63) {
			printf("The arguments of -E need be less than 64.\n");
			return 0;
		}
		sp_set_txeq(global_dev,val,val1,val2);

		break;
	default:
		parse_err("Unknown filter option -%c", c[1]);
	}
	return i;
}

static void parse_ops(int argc, char **argv, int i)
{
	enum { STATE_INIT, STATE_GOT_FILTER, STATE_GOT_OP }
		state = STATE_INIT;
	struct pci_filter filter;
	int type = RES_PCI;
	struct pci_dev **selected_devices = NULL;
	struct pci_dev *dev = NULL;

	if (DEBUG_ON || debug_mode) printf("===parse_ops=1===\n");
	while (state < STATE_GOT_OP) {
		char *c, np[] = "";

		if (i < argc)
			c = argv[i++];
		else
			c = np;
		if (DEBUG_ON || debug_mode) printf("===parse_ops=2===\n");

		if (*c == '-') {
			if (DEBUG_ON || debug_mode) printf("===parse_ops=2-1===\n");
			if (state != STATE_GOT_FILTER) {
				if (DEBUG_ON || debug_mode) printf("===parse_ops=2-2===\n");
				pci_filter_init(pacc_wx, &filter);
			}
			if (DEBUG_ON || debug_mode) printf("===parse_ops=2-3===\n");
			//printf("=i=%x\n",i);
			i = parse_filter(argc, argv, i - 1, &filter, &type);
			if (!i) {
				return;
			}
			if (DEBUG_ON || debug_mode) printf("===parse_ops=2-4===\n");
			state = STATE_GOT_FILTER;
			if (state == STATE_GOT_FILTER)
				selected_devices = select_devices(&filter);
			if (!selected_devices) {
				printf("Not found any device\n");
				return;
			}
			global_dev = *selected_devices;
			dev = global_dev;
			if (dev->vendor_id != VENDOR_ID && dev->vendor_id != WX_VENDOR_ID) {
				printf("It is not our NIC slot.\n");
				return;
			}
				
			//printf("bus=%0d, dev=%0d, func=%0d\n", dev->bus, dev->dev, dev->func);
			if (DEBUG_ON || debug_mode) printf("===parse_ops=3===\n");
		} else if (0) {
		

		} else {
			if (state == STATE_INIT)
				parse_err("Filter specification expected");
			//if (state == STATE_GOT_FILTER)
			//	selected_devices = select_devices(&filter);
			if (!selected_devices[0] && !force)
				fprintf(stderr, "wxtool: Warning: No devices selected for \"%s\".\n", c);
			//parse_op(c, selected_devices, type);
			state = STATE_GOT_OP;
			if (DEBUG_ON || debug_mode) printf("===parse_ops=4===\n");
		}
	}
	if (state == STATE_INIT)
		parse_err("No operation specified");
#if 0
	global_dev = *selected_devices;
	dev = global_dev;

	printf("bus=%0d, dev=%0d, func=%0d\n", dev->bus, dev->dev, dev->func);
#endif
}

int check_upgrade_option()
{
	if (opt_c_is_set && opt_u_is_set) {
		printf("ERROR: Option -C and -U cannot be used at the same time\n");
		return -1;
	}
	return 0;
}

int upgrade_flash(int argc, char **argv)
{
	int way = 0;

	if(parse_options_for_up_flash(argc, argv)) {
		printf("parse option failed\n");
		return 0;
	}

	if (check_upgrade_option()) {
		return 0;
	}

	while (1)
	{
		printf("Please Select which kind of NIC to upgrade:\n");
		printf("  1. 1000M_nics_1ports\n");
		printf("  2. 1000M_nics_2ports\n");
		printf("  3. 1000M_nics_4ports\n");
		printf("  4. 10_Gigabit_nics\n");
		// printf("  3. Port 3\n");
		// printf("  4. Port 4\n");
		// printf("  5. Exit\n");

		if (!(opt_k_is_set == 1))
		{
			printf("please input choose number: ");
			scanf("%d", &way);
			safe_flush(stdin);
		}
		if (opt_k_is_set == 1)
			way = upgrade_mode;

		switch (way)
		{
		case 1:
			g_port_num = 1;
			Gigabit_nics();
			return 0;
		case 2:
			opt_p_is_set = 1;
			g_port_num = 2;
			Gigabit_nics();
			return 0;
		case 3:
			g_port_num = 4;
			Gigabit_nics();
			return 0;
		case 4:
			Tgigabit_nics();
			return 0;
		default:
			printf("Invalid input, please re-enter\n");
			break;
		}
	}
	return 0;
}

int parse_options(int argc, char **argv)
{
	int i = 1;
	FILE *fp = NULL;
	int status = -1;

	debug_mode = 0;
	fp = fopen("DEBUG", "r");
	if (fp != NULL) {
		debug_mode = 1;
		fclose(fp);
	}

	if (argc == 2)
	{
		if (strcmp(argv[1], "show") == 0) {
			show_wx_nic_info(argc, argv);
			return 0;
		}

		if (strcmp(argv[1], "--help") == 0) {
			usage_for_up_flash(0);
			usage_for_wxtool(1);
		}

		if (strcmp(argv[1], "--version") == 0)
		{
			puts("wxtool version: " PCIUTILS_VERSION);
			exit(0);
		}

		printf("\nERROR: There are unknown options in command line!\n\n");
		usage_for_up_flash(0);
		usage_for_wxtool(1);

	} else if (argc == 1) {
		usage_for_up_flash(0);
		usage_for_wxtool(1);
	} else if (argc >= 3) {
		if (strcmp(argv[1], "check") == 0) {
			strcpy(IMAGE_FILE, argv[2]);
			printf("image is %s\n", IMAGE_FILE);
			status = check_image_status();
			status = check_image_checksum();
			//printf("The image chip_v is %c\n", check_image_version());
			return 0;
		} else if (strcmp(argv[1], "show") == 0) {
			show_wx_nic_info(argc, argv);
			return 0;
		}
		char *c = argv[1] + 1;
		//printf("%s\n", argv[1]);
		//printf("%c\n", argv[1][0]);
		//printf("%c\n", *c);
		if (*c == 's')
		{
			i = parse_options_for_wxtool(argc, argv);
			//printf("wxtool ....\n");
		} else if (*c == 'F') {
			//parse_options_for_up_flash(argc, argv);
			upgrade_flash(argc, argv);
			//printf("up_flash...\n");
			return 0;
		}
		//printf("%s\n", argv[1]);
		//printf("%c\n", argv[1][0]);
	}

	return i;
}

int main(int argc, char **argv)
{
	int i;
	global_dev = NULL;

	pacc_wx = pci_alloc();
	pacc_wx->error = die;
	if (DEBUG_ON || debug_mode) printf("===main=1===\n");
	i = parse_options(argc, argv);
	
	pci_init(pacc_wx);
	pci_scan_bus(pacc_wx);
	if (DEBUG_ON || debug_mode) printf("===main=2===\n");
	if (i != 0)
		parse_ops(argc, argv, i);
	pci_cleanup(pacc_wx); /* Close everything */
	if (DEBUG_ON || debug_mode) printf("===main=3===\n");
	return 0;

}

