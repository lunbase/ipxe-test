/*
 *  em_flash_loader -- program external flash, used for Emnic relative cards massproduction
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "lib/pci.h"
#include "pciutils.h"

#include "xgigabit.h"
#include "Gigabit.h"

const char program_name[] = "upgrade_image_tools";
int main(int argc, char **argv)
{
	int way = 0;

  	struct pci_access *pacc = NULL;
  	struct pci_dev    *dev = NULL;
  	struct pci_dev    *devs [ MAX_RAPTOR_CARD_NUM ];
  	u32               dev_num_1g = 0;
  	u32               dev_num_10g = 0;
  	u32               i = 0;

  	pacc = pci_alloc();       /* Get the pci_access structure */
  	pacc->error = die;

  	pci_init(pacc);       /* Initialize the PCI library */
  	pci_scan_bus(pacc);       /* We want to get the list of devices */

  	parse_options_for_up_flash(argc, argv);
  	
  	for (i=0; i<MAX_RAPTOR_CARD_NUM; i++)
    {
      	devs[i] = NULL;
    }
  	
  	for (dev=pacc->devices; dev; dev=dev->next)  /* Iterate over all devices */
    {
      	if ((dev->vendor_id == VENDOR_ID || dev->vendor_id == WX_VENDOR_ID) && ((dev->device_id == DEVICE_ID_1001) || (dev->device_id == DEVICE_ID_2001)) && (dev->func == 0))
	{
	 	devs[dev_num_1g] = dev; 
	  	dev_num_1g += 1;
	} else if ((dev->vendor_id == VENDOR_ID || dev->vendor_id == WX_VENDOR_ID) && ((dev->device_id >> 4 == 0x010))) {
	 	devs[dev_num_10g] = dev; 
	  	dev_num_10g += 1;
	}
    }

  	if ((dev_num_1g == 0) && (dev_num_10g == 0))
    {
      	printf("\nUpgrade tool report fatal error:\n");
      	printf("ERROR: No any our adaptor cards were found!\n\n");
      	
      	return 1;
    }

	while(1)
	{
		printf("Please Select which kind of NIC to upgrade:\n");
				printf("  1. 1000M_nics_1ports\n");
		printf("  2. 1000M_nics_2ports\n");
		printf("  3. 1000M_nics_4ports\n");
		printf("  4. 10_Gigabit_nics\n");
	       // printf("  3. Port 3\n");
	       // printf("  4. Port 4\n");
	       // printf("  5. Exit\n");
 
		if(!(opt_k_is_set == 1)){
				printf("please input choose number: ");
				scanf("%d", &way);
		getchar();
		}
		if(opt_k_is_set == 1)
			way = upgrade_mode;
		
		switch(way)
		{
			case 1:
				g_port_num = 1;
				Gigabit_nics(void);
				return 0;
			case 2:
				opt_p_is_set = 1;
				g_port_num = 2;
				Gigabit_nics(void);
				return 0;
			case 3:
				g_port_num = 4;
				Gigabit_nics();
				return 0;
			case 4:
				Tgigabit_nics();
				return 0;
		}
	}
	return 0;
}





