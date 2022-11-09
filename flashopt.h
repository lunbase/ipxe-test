#include "functions.h"
//const char program_name[] = "upgrade_rocket_image";

#ifdef  SPI_CLK_DIV
#undef  SPI_CLK_DIV
#define SPI_CLK_DIV 3
#endif

#define RES_NUM_MAX     (6)
#define RES_PCI     (RES_NUM_MAX + 1)
#define RES_PCICAP  (RES_NUM_MAX + 2)
#define RES_PCIECAP (RES_NUM_MAX + 3)

static int force;
static struct pci_access *pacc;

int nic_find_init(struct pci_dev *devs);

unsigned int flash_read(int addr);
int flash_write(int addr,int value);
int flash_erase(int sec_num,int ind);

int nic_find_init(struct pci_dev *devs);

static int parse_filter(int argc, char **argv, int i,
		                        struct pci_filter *filter, int *type)
{
	char *c = argv[i++];
	char *d;
	u32 val;

	if (!c[1] || !strchr("sdr", c[1]))
		parse_err("Invalid option -%c", c[1]);

	
	if (c[2])
		d = (c[2] == '=') ? c+3 : c+2;
	else if (i < argc)
		d = argv[i++];
	else
		parse_err("Option -%c requires an argument", c[1]);

	switch (c[1]) {
	case 's':
		if (d = pci_filter_parse_slot(filter, d))
			parse_err("Unable to parse filter -s %s", d);

		break;
	case 'd':
		if (d = pci_filter_parse_id(filter, d))
			parse_err("Unable to parse filter -d %s", d);
		break;
	case 'r':
		c = d;
		val = strtoul(d, &c, 0);
		if (c == d)
			parse_err("Unable to parse filter -r %s", d);
		else if (val > 6)
			parse_err("resource number belong to [0-5]");
		else
			*type = val;
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

	while (state < STATE_GOT_OP) {
		char *c, np[] = "";

		if (i < argc)
			c = argv[i++];
		else
			c = np;

		if (*c == '-') {
			if (state != STATE_GOT_FILTER)
				pci_filter_init(pacc, &filter);
			i = parse_filter(argc, argv, i - 1, &filter, &type);
			state = STATE_GOT_FILTER;
		} else {
			if (state == STATE_INIT)
				parse_err("Filter specification expected");
			if (state == STATE_GOT_FILTER)
				selected_devices = select_devices(&filter);
			if (!selected_devices[0] && !force)
				fprintf(stderr, "pcitool: Warning: No devices selected for \"%s\".\n", c);
			parse_op(c, selected_devices, type);
			state = STATE_GOT_OP;
		}
	}
	if (state == STATE_INIT)
		parse_err("No operation specified");
}

int flash_erase(int sec_num,int ind)
{
	struct pci_access *pacc = NULL;
	struct pci_dev    *dev = NULL;
    	struct pci_dev    *devs[FUNC_NUM][MAX_RAPTOR_CARD_NUM];
    	u32                dev_num[FUNC_NUM] = {0,0,0,0};
	u32 active_dev_num = 0;
    	u8                 status = 0;
    	u32                i = 0, m = 0;

	pacc = pci_alloc();		/* Get the pci_access structure */
	pacc->error = die;

	/* Set all options you want -- here we stick with the defaults */
	pci_init(pacc);		/* Initialize the PCI library */
	pci_scan_bus(pacc);		/* We want to get the list of devices */

	for (i=0; i<MAX_RAPTOR_CARD_NUM; i++)
	{
		for (m=0; m<FUNC_NUM; m++)
        	{
            		devs[m/*PCIe Func ID*/][i/*Card ID*/] = NULL;
        	}
    	}

	for (dev=pacc->devices; dev; dev=dev->next)  /* Iterate over all devices */
	{
		if ((dev->vendor_id == VENDOR_ID || dev->vendor_id == WX_VENDOR_ID) && (dev->device_id >> 4) == 0x010)
		{
			devs[dev->func][dev_num[dev->func]] = dev; 
			//printf("bus=%0d, dev=%0d, func=%0d\n", dev->bus, dev->dev, dev->func);
			dev_num[dev->func] += 1;
		}
    	}
    	if (dev_num[0] == 0)
    	{
		printf("\nNo any cards were found! Program is exited!!\n\n");
		return 0;
	}
    	active_dev_num = dev_num[0];

    	/* Select Flash Manufacturer */
    	opt_m_is_set = 1; // Force to SST
    	flash_vendor = FLASH_SST;
    	if (flash_vendor != FLASH_WINBOND && 
		flash_vendor != FLASH_SPANISH && 
		flash_vendor != FLASH_SST)
        flash_vendor = FLASH_SST;
        dev = devs[0][0]; // select current PCI device

        // Check PCIe Memory Space Enable
        status = pci_read_byte(dev, 0x4);
        pci_write_byte(dev, 0x4, (status | 0x2)); // set "Memory Space Enable" field

 	if (flash_vendor == FLASH_SST)
	// Note: for SST flash, need to unlock FLASH after power-on
	{
		status = fmgr_usr_cmd_op(dev, 0x6);  // write enable
		//if (DEBUG_ON || debug_mode) printf("SST flash write enable user command, return status = %0d\n", status);
		status = fmgr_usr_cmd_op(dev, 0x98); // global protection un-lock
		//if (DEBUG_ON || debug_mode) printf("SST flash un-lock protection user command, return status = %0d\n", status);
		sleep(1); // 1 s
	}
    
	if (flash_vendor == FLASH_SST)
	// Note: for Spanish FLASH, first 8 sectors (4KB) in sector0 (64KB) need to use a special erase command (4K sector erase)
	{
		if(ind == 1){
			for(i=1;i<=255;i++)
			{
				write_reg(dev, SPI_CMD_CFG1_ADDR, 0x0103c720);
				flash_erase_sector(dev, i*4*1024);
				usleep(20 * 1000); // 20 ms
				write_reg(dev, SPI_CMD_CFG1_ADDR, 0x0103c7d8);
			}
    		}else{
			if(sec_num < 1 || sec_num > 255)
			{
				printf("Invalid sec_num\n");
				return -1;
			}
              		write_reg(dev, SPI_CMD_CFG1_ADDR, 0x0103c720);
			sec_num += 240;
              		flash_erase_sector(dev, sec_num*4*1024);
              		usleep(20 * 1000); // 20 ms
              		write_reg(dev, SPI_CMD_CFG1_ADDR, 0x0103c7d8);
    		}
        }

	printf("\n");
	pci_cleanup(pacc); /* Close everything */
	return 0;
}

int flash_write(int addr, int value)
{
	struct pci_access *pacc = NULL;
    	struct pci_dev    *dev = NULL;
    	struct pci_dev    *devs[FUNC_NUM][MAX_RAPTOR_CARD_NUM];
    	u32                dev_num[FUNC_NUM] = {0,0,0,0};
	u32				   active_dev_num = 0;

	u32                i = 0, status = 0, m = 0;
      	u32 read_data;

    	pacc = pci_alloc();		/* Get the pci_access structure */
    	pacc->error = die;

    	/* Set all options you want -- here we stick with the defaults */
    	pci_init(pacc);		/* Initialize the PCI library */
    	pci_scan_bus(pacc);		/* We want to get the list of devices */

    	for (i=0; i<MAX_RAPTOR_CARD_NUM; i++){
		for (m=0; m<FUNC_NUM; m++){
			devs[m/*PCIe Func ID*/][i/*Card ID*/] = NULL;
		}
	}

	for (dev=pacc->devices; dev; dev=dev->next) { /* Iterate over all devices */
		if ((dev->vendor_id == VENDOR_ID || dev->vendor_id == WX_VENDOR_ID) && (dev->device_id >> 4) == 0x010) { 
			devs[dev->func][dev_num[dev->func]] = dev; 
	     		//printf("bus=%0d, dev=%0d, func=%0d\n", dev->bus, dev->dev, dev->func)
			dev_num[dev->func] += 1;
		}
       	}
	//printf("Please wait %d Rocket cards flash upgrading ....\n\n", dev_num[0]);
	if (dev_num[0] == 0){
        	printf("\nNo any Rocket cards were found! Program is exited!!\n\n");
        	return 0;
	}
	active_dev_num = dev_num[0];
	dev = devs[0][0]; // select current PCI device

	// Check PCIe Memory Space Enable
	status = pci_read_byte(dev, 0x4);
	pci_write_byte(dev, 0x4, (status | 0x2)); // set "Memory Space Enable" field

	status = flash_write_dword(dev, addr, value);
	if (status)
	{
		printf("\nPCI Utils tool report fatal error:\n");
		printf("ERROR: Program 0x%08x @addr: 0x%08x is failed !!\n", read_data, addr);
		read_data = flash_read_dword(dev, addr);
		printf("       Read data from Flash is: 0x%08x\n", read_data);
		return status;
	}

	pci_cleanup(pacc); /* Close everything */
	return 0;
}

unsigned int flash_read(int addr)
{
	struct pci_access *pacc = NULL;
    	struct pci_dev    *dev = NULL;
    	struct pci_dev    *devs[FUNC_NUM][MAX_RAPTOR_CARD_NUM];
    	u32                dev_num[FUNC_NUM] = {0,0,0,0};
    	u32	active_dev_num = 0;
	u32                sector_num = 0;
	u8                 status = 0;
	u32                i = 0, m = 0;
	u32 value;

	pacc = pci_alloc();		/* Get the pci_access structure */
	pacc->error = die;
	/* Set all options you want -- here we stick with the defaults */
	pci_init(pacc);		/* Initialize the PCI library */
	pci_scan_bus(pacc);		/* We want to get the list of devices */

	for (i=0; i<MAX_RAPTOR_CARD_NUM; i++) {
		for (m=0; m<FUNC_NUM; m++) {
			devs[m/*PCIe Func ID*/][i/*Card ID*/] = NULL;
		}
	}

	for (dev=pacc->devices; dev; dev=dev->next) { /* Iterate over all devices */
		if ((dev->vendor_id == VENDOR_ID || dev->vendor_id == WX_VENDOR_ID) && (dev->device_id >> 4) == 0x010) {
			devs[dev->func][dev_num[dev->func]] = dev; 
	    		//printf("bus=%0d, dev=%0d, func=%0d\n", dev->bus, dev->dev, dev->func);
			dev_num[dev->func] += 1;
		}
	}
	//printf("Please wait %d Rocket cards flash upgrading ....\n\n", dev_num[0]);
	if (dev_num[0] == 0) {
		printf("\nNo any Rocket cards were found! Program is exited!!\n\n");
		return 0;
	}

    	active_dev_num = dev_num[0];

    	dev = devs[0][0]; // select current PCI device

    	// Check PCIe Memory Space Enable
	status = pci_read_byte(dev, 0x4);
    	pci_write_byte(dev, 0x4, (status | 0x2)); // set "Memory Space Enable" field
    	value = flash_read_dword(dev, addr);
	printf("addr: %x - value: %x\n", addr,value);

    	pci_cleanup(pacc); /* Close everything */
    	return value;
}

int nic_find_init(struct pci_dev *devs_t)
{
	struct pci_access *pacc = NULL;
    	struct pci_dev    *dev = NULL;
    	struct pci_dev    *devs[FUNC_NUM][MAX_RAPTOR_CARD_NUM];
    	u32                dev_num[FUNC_NUM] = {0,0,0,0};
	u8                 status = 0;
	u32                i = 0,m = 0;

	pacc = pci_alloc();		/* Get the pci_access structure */
	pacc->error = die;
	/* Set all options you want -- here we stick with the defaults */
	pci_init(pacc);		/* Initialize the PCI library */
	pci_scan_bus(pacc);		/* We want to get the list of devices */

	for (i=0; i<MAX_RAPTOR_CARD_NUM; i++) {
		for (m=0; m<FUNC_NUM; m++) {
			devs[m/*PCIe Func ID*/][i/*Card ID*/] = NULL;
		}
	}

	for (dev=pacc->devices; dev; dev=dev->next) { /* Iterate over all devices */
		if ((dev->vendor_id == VENDOR_ID || dev->vendor_id == WX_VENDOR_ID) && (dev->device_id >> 4) == 0x010) {
			devs[dev->func][dev_num[dev->func]] = dev; 
	    		//printf("bus=%0d, dev=%0d, func=%0d\n", dev->bus, dev->dev, dev->func);
			dev_num[dev->func] += 1;
		}
	}
	//printf("Please wait %d cards flash upgrading ....\n\n", dev_num[0]);
	if (dev_num[0] == 0) {
		printf("\nNo any cards were found! Program is exited!!\n\n");
		return 1;
	}

    	dev = devs[0][0]; // select current PCI device

    	return 0;
}

int main(int argc, char **argv)
{
	int i;
	pacc = pci_alloc();
	pacc->error = die;
	i = parse_options(argc, argv);
	
	pci_init(pacc);
	pci_scan_bus(pacc);

	parse_ops(argc, argv, i);

	return 0;

}

