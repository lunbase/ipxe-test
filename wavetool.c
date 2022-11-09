/*
 *  Waveform_tool -- used for Emnic relative cards waveform test
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "lib/pci.h"
#include "pciutils.h"

const char program_name[] = "wavetool";

#define VENDOR_ID                         0x1f64
#define WX_VENDOR_ID                      0x8088
#define MAX_RAPTOR_CARD_NUM               16

#define DEBUG_ON || debug_mode                           0

static u8 opt_f_is_set;
static u8 opt_m_is_set;
static u8 opt_a_is_set;
static u8 opt_u_is_set;
static u8 opt_c_is_set;
static u8 opt_p_is_set;

#define FUNC_NUM                           4

#define PHY_CONFIG(reg_offset)    (0x14000 + ((reg_offset) * 4))
static u8 dev_index;

static void write_reg(struct pci_dev *dev, u64 addr, u32 val)
{
	pci_write_res_long(dev, 0, addr, val);
}

static u32 read_reg(struct pci_dev *dev, u64 addr)
{
	return pci_read_res_long(dev, 0, addr);
}

static void phy_read_reg(struct pci_dev *dev, u32 reg_offset, u32 page, u16 *phy_data)
{
	int page_select = 0;

	/* clear input */
	*phy_data = 0;

	if (0 != page) {
		/* select page */
		if (0xa42 == page || 0xa43 == page || 0xa46 == page || 0xa47 == page ||
			0xd04 == page || 0xc80 == page) {
			write_reg(dev, PHY_CONFIG(31), page);
			page_select = 1;
		}
	}

	if (reg_offset >= 32) {
		printf("input reg offset %d exceed maximum 31.\n", reg_offset);
	}

	*phy_data = 0xFFFF & read_reg(dev, PHY_CONFIG(reg_offset));

	if (page_select)
		write_reg(dev, PHY_CONFIG(31), 0);

}


static void phy_write_reg(struct pci_dev *dev, u32 reg_offset, u32 page, u16 phy_data)
{
	int page_select = 0;

	if (0 != page) {
		/* select page */
		if (0xa42 == page || 0xa43 == page || 0xa46 == page || 0xa47 == page ||
			0xd04 == page || 0xc80 == page) {
			write_reg(dev, PHY_CONFIG(31), page);
			page_select = 1;
		}
	}

	if (reg_offset >= 32) {
		printf("input reg offset %d exceed maximum 31.\n", reg_offset);
	}

	write_reg(dev, PHY_CONFIG(reg_offset), phy_data);

	if (page_select)
		write_reg(dev, PHY_CONFIG(31), 0);

}

static void do_a(struct pci_dev *dev)
{
	char k;
	u16 value;
	phy_write_reg(dev, 31, 0x0, 0x0000);//page 0
	phy_write_reg(dev, 9, 0x0, 0x2000);

	phy_read_reg(dev, 9, 0x0, &value);
//	printf("Mode 1 : 0x%08x\n", value);
	printf("Ready, please test......\n");
	while(1) {
		printf("please input 'q' to quit:");
		scanf("%c", &k);
                getchar();
		if( k == 'q' )
			break;
	}
	phy_write_reg(dev, 31, 0x0, 0x0000);//page 0
	phy_write_reg(dev, 9, 0x0, 0x0000);
	//phy_read_reg(dev, 9, 0x0, &value);
//	printf("Mode 1 : 0x%08x\n", value);
}

static void do_b(struct pci_dev *dev)
{
	char k;
	//u16 value;
	phy_write_reg(dev, 31, 0x0, 0x0000);//page 0
	phy_write_reg(dev, 9, 0x0, 0x4000);

	//phy_read_reg(dev, 9, 0x0, &value);
	//printf("Mode 2 : 0x%08x\n", value);
	printf("Ready, please test......\n");
	while(1) {
		printf("please input 'q' to quit:");
		scanf("%c", &k);
                getchar();
		if( k == 'q' )
			break;
	}
	phy_write_reg(dev, 31, 0x0, 0x0000);//page 0
	phy_write_reg(dev, 9, 0x0, 0x0000);
//	phy_read_reg(dev, 9, 0x0, &value);
//	printf("Mode 2 : 0x%08x\n", value);
}

static void do_c(struct pci_dev *dev)
{
	char k;
	//u16 value;
	phy_write_reg(dev, 31, 0x0, 0x0000);//page 0
	phy_write_reg(dev, 9, 0x0, 0x8000);

//	phy_read_reg(dev, 9, 0x0, &value);
//	printf("Mode 2 : 0x%08x\n", value);
	printf("Ready, please test......\n");
	while(1) {
		printf("please input 'q' to quit:");
		scanf("%c", &k);
                getchar();
		if( k == 'q' )
			break;
	}
	phy_write_reg(dev, 31, 0x0, 0x0000);//page 0
	phy_write_reg(dev, 9, 0x0, 0x0000);
	//phy_read_reg(dev, 9, 0x0, &value);
	//printf("Mode 2 : 0x%08x\n", value);
}

static void do_d(struct pci_dev *dev)
{
	char k;
//	u16 value;
	phy_write_reg(dev, 31, 0x0, 0x0000);//page 0
	phy_write_reg(dev, 24, 0x0, 0x2388);
	phy_write_reg(dev, 9, 0x0, 0x0000);
	phy_write_reg(dev, 0, 0x0, 0x2100);

	//phy_read_reg(dev, 0, 0x0, &value);
	//printf("Mode 4 : 0x%08x\n", value);
	printf("Ready, please test......\n");
	while(1) {
		printf("please input 'q' to quit:");
		scanf("%c", &k);
                getchar();
		if( k == 'q' )
			break;
	}
	phy_write_reg(dev, 31, 0x0, 0x0000);//page 0
	phy_write_reg(dev, 24, 0x0, 0x2188);
	phy_write_reg(dev, 9, 0x0, 0x0200);
	phy_write_reg(dev, 0, 0x0, 0x9200);
//	phy_read_reg(dev, 0, 0x0, &value);
//	printf("Mode 4 : 0x%08x\n", value);
}

static void do_e(struct pci_dev *dev)
{
	char k;
//	u16 value;
	phy_write_reg(dev, 31, 0x0, 0x0000);//page 0
	phy_write_reg(dev, 24, 0x0, 0x2288);
	phy_write_reg(dev, 9, 0x0, 0x0000);
	phy_write_reg(dev, 0, 0x0, 0x2100);

//	phy_read_reg(dev, 0, 0x0, &value);
//	printf("Mode 5 : 0x%08x\n", value);
	printf("Ready, please test......\n");
	while(1) {
		printf("please input 'q' to quit:");
		scanf("%c", &k);
                getchar();
		if( k == 'q' )
			break;
	}
	phy_write_reg(dev, 31, 0x0, 0x0000);//page 0
	phy_write_reg(dev, 24, 0x0, 0x2188);
	phy_write_reg(dev, 9, 0x0, 0x0200);
	phy_write_reg(dev, 0, 0x0, 0x9200);
//	phy_read_reg(dev, 0, 0x0, &value);
//	printf("Mode 5 : 0x%08x\n", value);
}

static void do_f(struct pci_dev *dev)
{
	char k;
//	u16 value;
	phy_write_reg(dev, 31, 0x0, 0x0000);//page 0
	phy_write_reg(dev, 9, 0x0, 0x0000);
	phy_write_reg(dev, 4, 0x0, 0x0061);
	phy_write_reg(dev, 25, 0x0, 0x0043);//10Mbps waveforn without EEE
	phy_write_reg(dev, 0, 0x0, 0x9200);//PHY reset

	phy_write_reg(dev, 31, 0x0, 0x0c80);//page c80
	phy_write_reg(dev, 18, 0x0c80, 0x0115);//pseudo-random pattern
	phy_write_reg(dev, 16, 0x0c80, 0x5a21);//enable the packet generator to output pseudo-random pattern

//	phy_read_reg(dev, 16, 0x0c80, &value);
//	printf("Mode 6 : 0x%08x\n", value);
	printf("Ready, please test......\n");
	while(1) {
		printf("please input 'q' to quit:");
		scanf("%c", &k);
                getchar();
		if( k == 'q' )
			break;
	}
	phy_write_reg(dev, 31, 0x0, 0x0c80);//page c80
	phy_write_reg(dev, 16, 0x0c80, 0x5a00);//disable packet generator
	phy_write_reg(dev, 31, 0x0, 0x0000);//page 0
	phy_write_reg(dev, 4, 0x0, 0x01e1);
	phy_write_reg(dev, 9, 0x0, 0x0200);
	phy_write_reg(dev, 0, 0x0, 0x9200);//PHY reset
//	phy_read_reg(dev, 0, 0x0, &value);
//	printf("Mode 6 : 0x%08x\n", value);
}

static void do_g(struct pci_dev *dev)
{
	char k;
//	u16 value;

	phy_write_reg(dev, 31, 0x0, 0x0000);//page 0
	phy_write_reg(dev, 9, 0x0, 0x0000);
	phy_write_reg(dev, 4, 0x0, 0x0061);
	phy_write_reg(dev, 25, 0x0, 0x0043);//10Mbps waveforn without EEE
	phy_write_reg(dev, 0, 0x0, 0x9200);//PHY reset

	phy_write_reg(dev, 31, 0x0, 0x0c80);//page c80
	phy_write_reg(dev, 18, 0x0c80, 0x0015);//fixed pattern
	phy_write_reg(dev, 16, 0x0c80, 0xff21);//enable the packet generator to output all "1" pattern

//	phy_read_reg(dev, 16, 0x0c80, &value);
//	printf("Mode 6 : 0x%08x\n", value);
	printf("Ready, please test......\n");
	while(1) {
		printf("please input 'q' to quit:");
		scanf("%c", &k);
                getchar();
		if( k == 'q' )
			break;
	}
	phy_write_reg(dev, 31, 0x0, 0x0c80);//page c80
	phy_write_reg(dev, 16, 0x0c80, 0x5a00);//disable packet generator
	phy_write_reg(dev, 31, 0x0, 0x0000);//page 0
	phy_write_reg(dev, 4, 0x0, 0x01e1);
	phy_write_reg(dev, 9, 0x0, 0x0200);
	phy_write_reg(dev, 0, 0x0, 0x9200);//PHY reset 
//	phy_read_reg(dev, 0, 0x0, &value);
//	printf("Mode 6 : 0x%08x\n", value);
}

static void do_h(struct pci_dev *dev)
{
	char k;
//	u16 value;

	phy_write_reg(dev, 31, 0x0, 0x0000);//page 0
	phy_write_reg(dev, 9, 0x0, 0x0000);
	phy_write_reg(dev, 4, 0x0, 0x0061);
	phy_write_reg(dev, 25, 0x0, 0x0043);//10Mbps waveforn without EEE
	phy_write_reg(dev, 0, 0x0, 0x9200);//PHY reset

	phy_write_reg(dev, 31, 0x0, 0x0c80);//page c80
	phy_write_reg(dev, 18, 0x0c80, 0x0015);//fixed pattern
	phy_write_reg(dev, 16, 0x0c80, 0x0021);//enable the packet generator to output all "0" pattern

//	phy_read_reg(dev, 16, 0x0c80, &value);
//	printf("Mode 6 : 0x%08x\n", value);
	printf("Ready, please test......\n");
	while(1) {
		printf("please input 'q' to quit:");
		scanf("%c", &k);
                getchar();
		if( k == 'q' )
			break;
	}
	phy_write_reg(dev, 31, 0x0, 0x0c80);//page c80
	phy_write_reg(dev, 16, 0x0c80, 0x5a00);//disable packet generator
	phy_write_reg(dev, 31, 0x0, 0x0000);//page 0
	phy_write_reg(dev, 4, 0x0, 0x01e1);
	phy_write_reg(dev, 9, 0x0, 0x0200);
	phy_write_reg(dev, 0, 0x0, 0x9200);//PHY reset 
//	phy_read_reg(dev, 0, 0x0, &value);
//	printf("Mode 6 : 0x%08x\n", value);
}

static void usage(void)
{
  fprintf(stderr,
"   Usage: wavetool [<options>]\n"
"\n"
"Support options:\n"
"\n");
  exit(0);
}

static int parse_options(int argc, char **argv)
{
	int i = 1;

	opt_f_is_set = 0;
	opt_m_is_set = 0;
	opt_a_is_set = 0;
	opt_u_is_set = 0;
	opt_c_is_set = 0;
	opt_p_is_set = 0;
  
	dev_index = 255;

	if (argc == 2)
	{
		if (strcmp(argv[1], "--help") == 0)
			usage();

		if (strcmp(argv[1], "--version") == 0)
		{
			puts("upgrade_rocket_image use pciutils version: " PCIUTILS_VERSION);
			exit(0);
		}

		if (strcmp(argv[1], "-A") == 0)
		{
		/* It's a good option, no need to die */
		}else{
		printf("\nERROR: There are unknown options in command line!\n\n");
		usage();
		}
	}

	while (i < argc && argv[i][0] == '-')
	{
		char *c = argv[i++] + 1;

		while (*c)
		switch (*c)
		{
		case 'F': /* Image File */
			//strcpy(IMAGE_FILE, argv[i++]);
			//if (DEBUG_ON || debug_mode) printf("F option is %s\n", IMAGE_FILE);
			opt_f_is_set = 1;
			c++;
			break;

		case 'M': /* Flash Manufacturers */
			//flash_vendor = atoi(argv[i++]);
			//if (DEBUG_ON || debug_mode) printf("M option is %d\n", flash_vendor);
			opt_m_is_set = 1;
			c++;
			break;

		case 'A': /* All PCI Device Upgrading */
			opt_a_is_set = 1;
			//all_card_num = atoi(argv[i++]);
//			if (DEBUG_ON || debug_mode) printf("A option is %0d\n", all_card_num);
			c++;
			break;

		case 'U': /* Force to update MAC and SN */
			opt_u_is_set = 1;
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

		default: /* Unknown Options */
			printf("\nERROR: There are unknown options in command line!\n\n");
			usage();
			}
	}

	return i;
}



int main(int argc, char **argv)
{
    struct pci_access *pacc = NULL;
    struct pci_dev    *dev = NULL;
    struct pci_dev    *devs[FUNC_NUM][MAX_RAPTOR_CARD_NUM];
    u32                dev_num[FUNC_NUM] = {0,0,0,0};
    int 		way = 0;

    //u8                 status = 0;
    u32                i = 0, m = 0;

//	printf("========1========\n");
    pacc = pci_alloc();		/* Get the pci_access structure */
    pacc->error = die;

//	printf("========2========\n");
    /* Set all options you want -- here we stick with the defaults */
    pci_init(pacc);		/* Initialize the PCI library */
    pci_scan_bus(pacc);		/* We want to get the list of devices */

    parse_options(argc, argv);
//	printf("========3========\n");
    //parse_options(argc, argv);

    for (i=0; i<MAX_RAPTOR_CARD_NUM; i++)
    {
        for (m=0; m<FUNC_NUM; m++)
        {
            devs[m/*PCIe Func ID*/][i/*Card ID*/] = NULL;
        }
    }

//	printf("========4========\n");
    for (dev=pacc->devices; dev; dev=dev->next)  /* Iterate over all devices */
    {
       // if (dev->vendor_id == VENDOR_ID && ((dev->device_id == 0x0102) || (dev->device_id == 0x0104)))
        if ((dev->vendor_id == VENDOR_ID || dev->vendor_id == WX_VENDOR_ID) && (dev->device_id >> 4) == 0x010 )
        {
            devs[dev->func][dev_num[dev->func]] = dev; 
            printf("bus=%0d, dev=%0d, func=%0d\n", dev->bus, dev->dev, dev->func);
            dev_num[dev->func] += 1;
	    sleep(0.5);
        }
    }
//	printf("========5========\n");
    if (dev_num[0] == 0)
    {
        printf("\nNo any Rocket cards were found! Program is exited!!\n\n");
        return 0;
    }

    /* Select PCI device */
    if (!opt_a_is_set)
    {
        if (dev_num[0] == 1)
        // To select the unique PCI device
        {
            dev_index = 0;
        }else
        // To select a PCI device when not to upgrade all of device
        {            
            printf("\nMore than one of our networking adaptor cards were found, but without of '-A' option specified. Please select a adaptor to download.\n");
            while (dev_index >= dev_num[0])
            {
              printf("\nPlease select a PCI device before upgrading.\n");
              for (i=0; i<dev_num[0]; i++)
              {
                  if (i == 0)
                    printf(" [ %0d ] %02d:%02d.%0d", i, devs[0][i]->bus, devs[0][i]->dev, devs[0][i]->func);
                  else if (i == (dev_num[0] - 1))
                    printf(" [ %0d ] %02d:%02d.%0d : ", i, devs[0][i]->bus, devs[0][i]->dev, devs[0][i]->func);
                  else
                    printf(", [ %0d ] %02d:%02d.%0d", i, devs[0][i]->bus, devs[0][i]->dev, devs[0][i]->func);
                }
              scanf("%d", (int *)&dev_index);
              printf("\n");
              if (dev_index >= dev_num[0])
                    printf("The PCI selection [ %0d ] is out of range! Please select again!!\n", dev_index);
            }
        }
    }
//	printf("========6========\n");

back:
	while(1)
	{
		printf("Please Select which port to test:\n");
                printf("  1. Port 1\n");
                printf("  2. Port 2\n");
                printf("  3. Port 3\n");
                printf("  4. Port 4\n");
                printf("  5. Exit\n");
 
		printf("please input choose number:");
                scanf("%d", &way);
                getchar();
 
                switch(way)
                {
                case 1:
        		dev = devs[0][0]; // select current PCI device
			printf("0%d:0%d.%d Ethernet controller: Device %04x:%04x\n", dev->bus, dev->dev, dev->func, dev->vendor_id, dev->device_id );
			goto next;
                        break;
                case 2:
        		dev = devs[1][0]; // select current PCI device
			printf("0%d:0%d.%d Ethernet controller: Device %04x:%04x\n", dev->bus, dev->dev, dev->func, dev->vendor_id, dev->device_id );
			goto next;
                        break;
                case 3:
        		dev = devs[2][0]; // select current PCI device
			printf("0%d:0%d.%d Ethernet controller: Device %04x:%04x\n", dev->bus, dev->dev, dev->func, dev->vendor_id, dev->device_id );
			goto next;
                        break;
                case 4:
        		dev = devs[3][0]; // select current PCI device
			printf("0%d:0%d.%d Ethernet controller: Device %04x:%04x\n", dev->bus, dev->dev, dev->func, dev->vendor_id, dev->device_id );
			goto next;
                        break;
                case 5:
			pci_cleanup(pacc); /* Close everything */
                        exit(0);
                        break;
                default:
			printf("Invaild port!\n");
		}
	}
//	printf("========7========\n");

next:
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
                getchar();
 
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
			pci_cleanup(pacc); /* Close everything */
                        exit(0);
                        break;
                default:
			printf("Invaild Way!\n");
		}                                                                                    
          }

	printf("\n");
	pci_cleanup(pacc); /* Close everything */
	sleep(2);
	
	return 0;
}
