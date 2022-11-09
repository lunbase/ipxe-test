/*******************************************************************************
  PCI Tools -- access PCI registers

  This program is free software; you can redistribute it and/or modify it
  under the terms and conditions of the GNU General Public License,
  version 2, as published by the Free Software Foundation.

  This program is distributed in the hope it will be useful, but WITHOUT
  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
  more details.

  You should have received a copy of the GNU General Public License along with
  this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA.

  The full GNU General Public License is included in this distribution in
  the file called "COPYING".

*******************************************************************************/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <errno.h>

#define PCIUTILS_SETPCI
#include "pciutils.h"

static int force;                       /* Don't complain if no devices match */
static int verbose;                     /* Verbosity level */
static int demo_mode;                   /* Only show */

const char program_name[] = "pcitool";
#define PCITOOL_VERSION "1.0.7"

#define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))

static struct pci_access *pacc;

struct value {
	u32 value;
	u32 mask;
};

struct reg_info {
	int type; /* 0-5: bar; 6-n: conf */
	u32 addr;
	u32 offset;
	u32 width;
	u32 count;
	const char *name;
};

struct op {
	struct op *next;
	struct pci_dev **dev_vector;

	struct reg_info reg;
	char format[32];
	u32 num_values;
	struct value values[0];
};

static struct op *first_op, **last_op = &first_op;
static u32 max_values[] = { 0, 0xff, 0xffff, 0, 0xffffffff };

#define RES_NUM_MAX     (6)
#define RES_PCI     (RES_NUM_MAX + 1)
#define RES_PCICAP  (RES_NUM_MAX + 2)
#define RES_PCIECAP (RES_NUM_MAX + 3)

struct reg_table {
	const struct reg_info *table;
	int size;
};

#define PCI_DEAD_REG 0xbeedbeedU

static const struct reg_info pci_agent_conf_regs[] = {
	/*** PCI configuration ***/
	{ RES_PCI,     0x000000, 0, 2, 1,  "VENDOR_ID" },
	{ RES_PCI,     0x000002, 0, 2, 1,  "DEVICE_ID" },
	{ RES_PCI,     0x000004, 0, 2, 1,  "COMMAND" },
	{ RES_PCI,     0x000006, 0, 2, 1,  "STATUS" },
	{ RES_PCI,     0x000008, 0, 1, 1,  "REVISION" },
	{ RES_PCI,     0x000009, 0, 1, 1,  "CLASS_PROG" },
	{ RES_PCI,     0x00000a, 0, 2, 1,  "CLASS_DEVICE" },
	{ RES_PCI,     0x00000c, 0, 1, 1,  "CACHE_LINE_SIZE" },
	{ RES_PCI,     0x00000d, 0, 1, 1,  "LATENCY_TIMER" },
	{ RES_PCI,     0x00000e, 0, 1, 1,  "HEADER_TYPE" },
	{ RES_PCI,     0x00000f, 0, 1, 1,  "BIST" },
	{ RES_PCI,     0x000010, 0, 4, 1,  "BASE_ADDRESS_0" },
	{ RES_PCI,     0x000014, 0, 4, 1,  "BASE_ADDRESS_1" },
	{ RES_PCI,     0x000018, 0, 4, 1,  "BASE_ADDRESS_2" },
	{ RES_PCI,     0x00001c, 0, 4, 1,  "BASE_ADDRESS_3" },
	{ RES_PCI,     0x000020, 0, 4, 1,  "BASE_ADDRESS_4" },
	{ RES_PCI,     0x000024, 0, 4, 1,  "BASE_ADDRESS_5" },
	{ RES_PCI,     0x000028, 0, 4, 1,  "CARDBUS_CIS" },
	{ RES_PCI,     0x00002c, 0, 2, 1,  "SUBSYSTEM_VENDOR_ID" },
	{ RES_PCI,     0x00002e, 0, 2, 1,  "SUBSYSTEM_ID" },
	{ RES_PCI,     0x000030, 0, 4, 1,  "ROM_ADDRESS" },
	{ RES_PCI,     0x000034, 0, 1, 1,  "CAP_PTR" },
	{ RES_PCI,     0x000035, 0, 1, 1,  "RESERVED" },
	{ RES_PCI,     0x000036, 0, 2, 1,  "RESERVED" },
	{ RES_PCI,     0x000038, 0, 4, 1,  "RESERVED" },
	{ RES_PCI,     0x00003c, 0, 1, 1,  "INTERRUPT_LINE" },
	{ RES_PCI,     0x00003d, 0, 1, 1,  "INTERRUPT_PIN" },
	{ RES_PCI,     0x00003e, 0, 1, 1,  "MIN_GNT" },
	{ RES_PCI,     0x00003f, 0, 1, 1,  "MAX_LAT" },
	/*** PCI capabilities ***/
	/* addr = PCI_CAP_NORMAL << 16 | PCI_CAP_ID */
	{ RES_PCICAP,  0x010001, 0, 4, 2,  "CAP_PM" },
	{ RES_PCICAP,  0x010002, 0, 4, 1,  "CAP_AGP" },
	{ RES_PCICAP,  0x010003, 0, 4, 1,  "CAP_VPD" },
	{ RES_PCICAP,  0x010004, 0, 4, 1,  "CAP_SLOTID" },
	{ RES_PCICAP,  0x010005, 0, 4, 1,  "CAP_MSI" },
	{ RES_PCICAP,  0x010006, 0, 4, 1,  "CAP_CHSWP" },
	{ RES_PCICAP,  0x010007, 0, 4, 1,  "CAP_PCIX" },
	{ RES_PCICAP,  0x010008, 0, 4, 1,  "CAP_HT" },
	{ RES_PCICAP,  0x010009, 0, 4, 1,  "CAP_VNDR" },
	{ RES_PCICAP,  0x01000a, 0, 4, 1,  "CAP_DBG" },
	{ RES_PCICAP,  0x01000b, 0, 4, 1,  "CAP_CCRC" },
	{ RES_PCICAP,  0x01000c, 0, 4, 1,  "CAP_HOTPLUG" },
	{ RES_PCICAP,  0x01000d, 0, 4, 1,  "CAP_SSVID" },
	{ RES_PCICAP,  0x01000e, 0, 4, 1,  "CAP_AGP3" },
	{ RES_PCICAP,  0x01000f, 0, 4, 1,  "CAP_SECURE" },
	{ RES_PCICAP,  0x010010, 0, 4, 15, "CAP_EXP" },
	{ RES_PCICAP,  0x010011, 0, 4, 1,  "CAP_MSIX" },
	{ RES_PCICAP,  0x010012, 0, 4, 1,  "CAP_SATA" },
	{ RES_PCICAP,  0x010013, 0, 4, 1,  "CAP_AF" },
	/*** PCIe extended capabilities ***/
	/* addr = PCI_CAP_EXTENDED << 16 | PCI_CAP_ID */
	{ RES_PCIECAP, 0x020001, 0, 4, 1,  "ECAP_AER" },
	{ RES_PCIECAP, 0x020002, 0, 4, 1,  "ECAP_VC" },
	{ RES_PCIECAP, 0x020003, 0, 4, 1,  "ECAP_DSN" },
	{ RES_PCIECAP, 0x020004, 0, 4, 1,  "ECAP_PB" },
	{ RES_PCIECAP, 0x020005, 0, 4, 1,  "ECAP_RCLINK" },
	{ RES_PCIECAP, 0x020006, 0, 4, 1,  "ECAP_RCILINK" },
	{ RES_PCIECAP, 0x020007, 0, 4, 1,  "ECAP_RCECOLL" },
	{ RES_PCIECAP, 0x020008, 0, 4, 1,  "ECAP_MFVC" },
	{ RES_PCIECAP, 0x02000a, 0, 4, 1,  "ECAP_RBCB" },
	{ RES_PCIECAP, 0x02000b, 0, 4, 1,  "ECAP_VNDR" },
	{ RES_PCIECAP, 0x02000d, 0, 4, 1,  "ECAP_ACS" },
	{ RES_PCIECAP, 0x02000e, 0, 4, 1,  "ECAP_ARI" },
	{ RES_PCIECAP, 0x02000f, 0, 4, 1,  "ECAP_ATS" },
	{ RES_PCIECAP, 0x020010, 0, 4, 1,  "ECAP_SRIOV" },
	{ RES_PCIECAP, 0x02001d, 0, 4, 1,  "ECAP_DPC" },
	{ 0, 0, 0, 0, 0, NULL }
};

const struct reg_table default_rtables[] = {
	{ pci_agent_conf_regs, ARRAY_SIZE(pci_agent_conf_regs) - 1},
	{ NULL, 0},
};

static const struct reg_info x520_bar0_regs[] = {
	{ 0, 0x00000, 0, 4, 1,  "CTRL" },
	{ 0, 0x00008, 0, 4, 1,  "STATUS" },
	{ 0, 0x00018, 0, 4, 1,  "CTRL_EXT" },
	{ 0, 0x00020, 0, 4, 1,  "ESDP" },
	{ 0, 0x00028, 0, 4, 1,  "EODSDP" },
	{ 0, 0, 0, 0, 0, NULL }
};

const struct reg_table x520_rtables[] = {
	{ pci_agent_conf_regs, ARRAY_SIZE(pci_agent_conf_regs) - 1 },
	{ x520_bar0_regs,     ARRAY_SIZE(x520_bar0_regs) - 1     },
	{ NULL, 0},
};

static const struct reg_info xl710_bar0_regs[] = {
	{ 0, 0x088080, 0, 4, 1,  "PFINT_GPIO_ENA" },
	{ 0, 0, 0, 0, 0, NULL }
};

const struct reg_table xl710_rtables[] = {
	{ pci_agent_conf_regs, ARRAY_SIZE(pci_agent_conf_regs) - 1 },
	{ xl710_bar0_regs,     ARRAY_SIZE(xl710_bar0_regs) - 1     },
	{ NULL, 0},
};

static const struct reg_info sp1001_bar0_regs[] = {
//    SP REG
//    Global
	{ 0, 0x10000, 0, 4, 1,  "MIS_PWR" },
	{ 0, 0x10004, 0, 4, 1,  "MIS_CTL" },
	{ 0, 0x1000C, 0, 4, 1,  "MIS_RST" },
	{ 0, 0x10028, 0, 4, 1,  "MIS_ST" },
	{ 0, 0x10030, 0, 4, 1,  "MIS_RST_ST" },
//    Port
	{ 0, 0x14400, 0, 4, 1,  "CFG_PORT_CTL" },
	{ 0, 0x14404, 0, 4, 1,  "CFG_PORT_ST" },
	{ 0, 0x14404, 0, 4, 1,  "CFG_PORT_ST" },
//    TDMA
	{ 0, 0x18000, 0, 4, 1,  "TDM_CTL" },
	{ 0, 0x18020, 0, 4, 1,  "TDM_PB_THRE0" },
	{ 0, 0x18040, 0, 4, 1,  "TDM_LLQ0" },
	{ 0, 0x18050, 0, 4, 1,  "TDM_ETYPE_LB_L" },
	{ 0, 0x18054, 0, 4, 1,  "TDM_ETYPE_LB_H" },
	{ 0, 0x18058, 0, 4, 1,  "TDM_EYTPE_AS_L" },
	{ 0, 0x1805C, 0, 4, 1,  "TDM_EYTPE_AS_H" },
	{ 0, 0x18060, 0, 4, 1,  "TDM_MAC_AS_L" },
	{ 0, 0x18064, 0, 4, 1,  "TDM_MAC_AS_H" },
	{ 0, 0x18070, 0, 4, 1,  "TDM_VLAN_AS_L" },
	{ 0, 0x18074, 0, 4, 1,  "TDM_VLAN_AS_H" },
	{ 0, 0x18100, 0, 4, 1,  "TDM_VLAN_INS0" },

	{ 0, 0x01008, 0, 4, 1,  "PX_RR_WP0"},
	{ 0, 0x0100C, 0, 4, 1,  "PX_RR_RP0"},
	{ 0, 0x01014, 0, 4, 1,  "PX_GPRC0"},
	{ 0, 0x01020, 0, 4, 1,  "PX_MPRC0"},
	{ 0, 0x03008, 0, 4, 1,  "PX_TR_WP0"},
	{ 0, 0x0300C, 0, 4, 1,  "PX_TR_RP0"},
	{ 0, 0x03014, 0, 4, 1,  "PX_GPTC0"},

	{ 0, 0, 0, 0, 0, NULL }
};

static const struct reg_info sp1001_bar4_regs[] = {
	{ 4, 0x000000, 0, 4, 1,  "MSIX_VECTOR" },
	{ 0, 0, 0, 0, 0, NULL }
};

const struct reg_table sp1001_rtables[] = {
	{ pci_agent_conf_regs, ARRAY_SIZE(pci_agent_conf_regs) - 1 },
	{ sp1001_bar0_regs,    ARRAY_SIZE(sp1001_bar0_regs) - 1 },
	{ sp1001_bar4_regs,    ARRAY_SIZE(sp1001_bar4_regs) - 1 },
	{ NULL, 0},
};


static const struct reg_info sp1000_bar0_regs[] = {
	{ 0, 0x00000, 0, 4, 1,  "VXRXMEMWRAP" },
	{ 0, 0x00004, 0, 4, 1,  "VXSTATUS" },
	{ 0, 0x00008, 0, 4, 1,  "VXCTRL" },
	{ 0, 0x00100, 0, 4, 1,  "VXICR" },
	{ 0, 0x00104, 0, 4, 1,  "VXICS" },
	{ 0, 0x00108, 0, 4, 1,  "VXIMS" },
	{ 0, 0x0010C, 0, 4, 1,  "VXIMC" },
	{ 0, 0x00200, 0, 4, 2,  "VXITR" },
	{ 0, 0x00240, 0, 4, 4,  "VXIVAR" },
	{ 0, 0x00260, 0, 4, 1,  "VXIVAR_MISC" },

	{ 0, 0x01000, 0, 4, 8,  "VXRDBAL" },
	{ 0, 0x01004, 0, 4, 8,  "VXRDBAH" },
	{ 0, 0x01008, 0, 4, 8,  "VXRDH" },
	{ 0, 0x0100C, 0, 4, 8,  "VXRDT" },
	{ 0, 0x01010, 0, 4, 8,  "VXRXDCTL" },

	{ 0, 0x03000, 0, 4, 8,  "VXTDBAL" },
	{ 0, 0x03004, 0, 4, 8,  "VXTDBAH" },
	{ 0, 0x03008, 0, 4, 8,  "VXTDH" },
	{ 0, 0x0300C, 0, 4, 8,  "VXTDT"},
	{ 0, 0x03010, 0, 4, 8,  "VXTXDCTL"},

	{ 0, 0x01014, 0, 4, 1,  "VXGPRC"},
	{ 0, 0x03014, 0, 4, 1,  "VXGPTC"},
	{ 0, 0x01018, 0, 4, 1,  "VXGORC_LSB"},
	{ 0, 0x0101C, 0, 4, 1,  "VXGORC_MSB"},
	{ 0, 0x03018, 0, 4, 1,  "VXGOTC_LSB"},
	{ 0, 0x0301C, 0, 4, 1,  "VXGOTC_MSB"},
	{ 0, 0x01020, 0, 4, 1,  "VXMPRC"},

	{ 0, 0x00080, 0, 4, 10, "VXRSSRK"},
	{ 0, 0x000C0, 0, 4, 16, "VXRETA"},
	{ 0, 0x0007C, 0, 4, 1,  "VXMRQC"},

	{ 0, 0x00C00, 0, 4, 1,  "VXMBMEM"},
	{ 0, 0x01020, 0, 4, 1,  "VXMPRC"},
	{ 0, 0x01020, 0, 4, 1,  "VXMPRC"},
	{ 0, 0x01020, 0, 4, 1,  "VXMPRC"},
	{ 0, 0x01020, 0, 4, 1,  "VXMPRC"},
	{ 0, 0x01020, 0, 4, 1,  "VXMPRC"},
	{ 0, 0x01020, 0, 4, 1,  "VXMPRC"},

	{ 0, 0, 0, 0, 0, NULL }
};

static const struct reg_info sp1000_bar4_regs[] = {
	{ 4, 0x000000, 0, 4, 1,  "MSIX_VECTOR" },
	{ 0, 0, 0, 0, 0, NULL }
};

const struct reg_table sp1000_rtables[] = {
	{ pci_agent_conf_regs, ARRAY_SIZE(pci_agent_conf_regs) - 1 },
	{ sp1000_bar0_regs,    ARRAY_SIZE(sp1000_bar0_regs) - 1 },
	{ sp1000_bar4_regs,    ARRAY_SIZE(sp1000_bar4_regs) - 1 },
	{ NULL, 0},
};

static const struct reg_table *get_dev_rtables(struct pci_dev *dev)
{
	struct pci_filter filter;

	memset(&filter, -1, sizeof(filter));

	/* intel x520 */
	filter.vendor = PCI_VENDOR_ID_INTEL;
	filter.device = 0x10fb;
	if (pci_filter_match(&filter, dev))
		return x520_rtables;

	/* intel xl710 */
	filter.vendor = PCI_VENDOR_ID_INTEL;
	filter.device = 0x1572;
	if (pci_filter_match(&filter, dev))
		return xl710_rtables;

	/* pf */
	filter.vendor = PCI_VENDOR_ID_NETIC;
	filter.device = 0x1001;
	if (pci_filter_match(&filter, dev))
		return sp1001_rtables;

	/* vf */
	filter.vendor = PCI_VENDOR_ID_NETIC;
	filter.device = 0x1000;
	if (pci_filter_match(&filter, dev))
		return sp1000_rtables;

	return default_rtables;
}

static int get_reg_address(struct pci_dev *dev, const struct reg_info *reg,
				u32 *address)
{
	struct pci_cap *pcap;
	pciaddr_t res_size;
	u32 addr = reg->addr;

	if (reg->type > RES_PCI) {
		u16 cap_type = reg->addr >> 16;
		u16 cap_id = reg->addr & 0xffff;
		pcap = pci_find_cap(dev, cap_id, cap_type);
		if (!pcap)
			return -1;
		addr = pcap->addr;
	}
	addr += reg->offset;

	if (reg->type < RES_NUM_MAX)
		res_size = dev->size[reg->type];
	else
		res_size = 0x1000; /* default pci config space */

	if (addr & (reg->width - 1))
		return -2;
	if (addr + reg->width > res_size)
		return -3;

	if (address)
		*address = addr;

	return 0;
}

static int read_reg(struct pci_dev *dev, const struct reg_info *reg, u32 *value)
{
	u32 val;
	u32 addr;
	int status = 0;

	if (status = get_reg_address(dev, reg, &addr)) {
		return status;
	}

	switch (reg->width) {
	case 1:
		val = reg->type >= RES_PCI ? pci_read_byte(dev, addr)
			: pci_read_res_byte(dev, reg->type, addr);
		break;
	case 2:
		val = reg->type >= RES_PCI ? pci_read_word(dev, addr)
			: pci_read_res_word(dev, reg->type, addr);
		break;
	default:
		val = reg->type >= RES_PCI ? pci_read_long(dev, addr)
			: pci_read_res_long(dev, reg->type, addr);
		break;
	}

	*value = val;
	return status;
}

static int write_reg(struct pci_dev *dev, const struct reg_info *reg, u32 val)
{
	int status = 0;
	u32 addr;

	if (demo_mode)
		return status;

	if (status = get_reg_address(dev, reg, &addr)) {
		return status;
	}

	switch (reg->width)
	{
	case 1:
		status = reg->type >= RES_PCI ? pci_write_byte(dev, addr, val)
			: pci_write_res_byte(dev, reg->type, addr, val);
		break;
	case 2:
		status = reg->type >= RES_PCI ? pci_write_word(dev, addr, val)
			: pci_write_res_word(dev, reg->type, addr, val);
		break;
	default:
		status = reg->type >= RES_PCI ? pci_write_long(dev, addr, val)
			: pci_write_res_long(dev, reg->type, addr, val);
		break;
	}

	return status;
}

static const struct reg_info *
find_reg_name(struct pci_dev *dev, char *name)
{
	const struct reg_table *rtables = get_dev_rtables(dev);
	if (!rtables)
		return NULL;

	while (rtables->table) {
		const struct reg_info *reg;
		for (reg = rtables->table; reg->name; reg++)
			if (!strcasecmp(reg->name, name))
				return reg;
		rtables++;
	}
	return NULL;
}

static const struct reg_info *
find_reg_seq(struct pci_dev *dev, int seq)
{
	const struct reg_info *reg = NULL;
	const struct reg_table *rtables = get_dev_rtables(dev);
	if (!rtables)
		return reg;

	while (rtables->table) {
		if (seq < 0) {
			break;
		} else if (seq < rtables->size) {
			reg = &rtables->table[seq];
			break;
		}

		seq -= rtables->size;
		rtables++;
	}

	return reg;
}

static void
print_reg(struct pci_dev *dev, const struct reg_info *reg)
{
	char value[12];
	u32 val = PCI_DEAD_REG;

	int status = read_reg(dev, reg, &val);
	if (status)
		sprintf(value, "0x--------");
	else if (reg->width == 1)
		sprintf(value, "0x------%02x", val);
	else if (reg->width == 2)
		sprintf(value, "0x----%04x", val);
	else
		sprintf(value, "0x%08x", val);
	printf("%c 0x%06x %c %8s %s\n",
		"012345-*CE"[reg->type],
		reg->addr,
		"-BW?D???L"[reg->width],
		value,
		reg->name);
}

static void
print_region(struct pci_dev *dev, const struct reg_info *reg)
{
	u32 addr;
	u32 i;
	char buff[64] = "";

	if (get_reg_address(dev, reg, &addr))
		return;

	for (i = 0; i < reg->count; i++) {
		int offset = i * reg->width;
		struct reg_info reg_cap = {
			(reg->type > RES_PCI ? RES_PCI : reg->type),
			addr + offset, 0, reg->width, 1, buff};
		sprintf(buff, "%s+%02d", reg->name, offset);
		print_reg(dev, &reg_cap);
	}
}

#if 0 /* defined but not used */
static void
print_array(struct pci_dev *dev, const struct reg_info *reg)
{
	u32 addr;
	u32 i;
	char buff[64] = "";

	if (get_reg_address(dev, reg, &addr))
		return;

	for (i = 0; i < reg->length; i++) {
		int offset = i * reg->width;
		struct reg_info reg_cap =
			{ reg->type, addr, 0, reg->width, 1, buff };

		if (write_reg(dev, reg, i))
			break;

		sprintf(buff, "%s+%02d", reg->name, offset);
		print_reg(dev, &reg_cap);
	}
}
#endif /* defined but not used */

static void
dump_all_regs(struct op *op, struct pci_dev *dev)
{
	const struct reg_info *reg;
	u32 i;

	printf("R ADDR     W VALUE      NAME\n");
	for (i = 0; reg = find_reg_seq(dev, i); i++) {
		if (op->reg.type < RES_NUM_MAX && op->reg.type != reg->type)
			continue;

		if (reg->count > 1)
			print_region(dev, reg);
		else
			print_reg(dev, reg);
	}
}

static struct pci_dev **
select_devices(struct pci_filter *filt)
{
	struct pci_dev *z, **a, **b;
	int cnt = 1;

	for (z = pacc->devices; z; z = z->next)
		if (pci_filter_match(filt, z))
			cnt++;
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
	return b;
}

static void
exec_op(struct op *op, struct pci_dev *dev)
{
	u32 i, x, y;
	char slot[16];
	struct reg_info *reg = &op->reg;

	sprintf(slot, "%04x:%02x:%02x.%x", dev->domain, dev->bus, dev->dev, dev->func);
	if (get_reg_address(dev, reg, NULL))
		die("%s: access %s 0x%05x failed", slot, reg->name, reg->addr);

	if (op->num_values) {
		for (i = 0; i < op->num_values; i++) {
			if ((op->values[i].mask & max_values[op->reg.width])
				== max_values[op->reg.width]) {
				x = op->values[i].value;
			} else {
				read_reg(dev, reg, &y);
				x = (y & ~op->values[i].mask) | op->values[i].value;
			}
			write_reg(dev, reg, x);
		}
	} else if (op->reg.count) {
		read_reg(dev, reg, &x);
		printf(op->format, x);
	} else {
		dump_all_regs(op, dev);
	}
}

static void
execute(struct op *op)
{
	struct pci_dev **vec = NULL;
	struct pci_dev **pdev, *dev;
	struct op *oops;

	while (op)
	{
		pdev = vec = op->dev_vector;
		while (dev = *pdev++)
			for (oops=op; oops && oops->dev_vector == vec; oops=oops->next)
				exec_op(oops, dev);
		while (op && op->dev_vector == vec)
			op = op->next;
	}
}

static void
scan_ops(struct op *op)
{
	if (demo_mode)
		return;
	while (op)
	{
		if (op->num_values)
			pacc->writeable = 1;
		op = op->next;
	}
}

static void NONRET
usage(void)
{
	fprintf(stderr,
		"Usage: pcitool [<options>] (<device>+ <reg>[=<values>]*)*\n"
		"\n"
		"General options:\n"
		"-f\t\tDon't complain if there's nothing to do\n"
		"-v\t\tBe verbose\n"
		"-D\t\tList changes, don't commit them\n"
		"\n"
		"PCI access options:\n"
		GENERIC_HELP
		"\n"
		"Setting commands:\n"
		"<device>:\t-s [[[<domain>]:][<bus>]:][<slot>][.[<func>]]\n"
		"\t\t-d [<vendor>]:[<device>]\n"
		"\t\t-r [<resource0-6>]\n"
		"<reg>:\t\t<base>[+<offset>][.(B|W|D|L)]\n"
		"<base>:\t\t<address>\n"
		"\t\t<named-register>\n"
		"\t\t[E]CAP_<capability-name>\n"
		"\t\t[E]CAP<capability-number>\n"
		"<values>:\t<number>[,<number>...]\n"
		"<value>:\t<number>\n"
		"\t\t<number>:<mask>\n");
	exit(0);
}

static void NONRET PCI_PRINTF(1,2)
	parse_err(const char *msg, ...)
{
	va_list args;
	va_start(args, msg);
	fprintf(stderr, "pcitool: ");
	vfprintf(stderr, msg, args);
	fprintf(stderr, ".\nTry `pcitool --help' for more information.\n");
	exit(1);
}

static int
parse_options(int argc, char **argv)
{
	const char opts[] = GENERIC_OPTIONS;
	int i=1;

	if (argc <= 2) {
		if (argc <= 1 || !strcmp(argv[1], "--help")) {
			usage();
		} else if (!strcmp(argv[1], "--version")) {
			puts("pciutil version " PCIUTILS_VERSION);
			puts("pcitool version " PCITOOL_VERSION);
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
							parse_err("Option -%c requires an argument", *e);
						c = "";
					}

					if (!parse_generic_option(*e, pacc, arg))
						parse_err("Unable to parse option -%c", *e);
				} else {
					if (c != d)
						parse_err("Invalid or misplaced option -%c", *c);
					return i - 1;
				}
			}
	}

	return i;
}

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

static int parse_x32(char *c, char **stopp, u32 *resp)
{
	char *stop;
	unsigned long int l, base;

	if (!*c)
		return -1;

	if (!strncmp(c, "0b", 2))
		base = 2;
	else if (!strncmp(c, "0O", 2))
		base = 8;
	else if (!strncmp(c, "0x", 2))
		base = 16;
	else
		base = 10;

	errno = 0;
	l = strtoul(c, &stop, base);
	if (errno)
		return -1;

	if ((l & ~0U) != l)
		return -1;
	*resp = l;
	if (*stop) {
		if (stopp)
			*stopp = stop;
		return 0;
	} else {
		if (stopp)
			*stopp = NULL;
		return 1;
	}
}

static const struct reg_info *
find_register(struct op *op, char *name)
{
	const struct reg_info *reg;
	struct pci_dev **pdev, *dev;

	pdev = op->dev_vector;
	while (dev = *pdev++) {
		reg = find_reg_name(dev, name);
		if (reg)
			break;
	}

	return reg;
}

static void parse_register(struct op *op, char *base)
{
	const struct reg_info *r;
	u32 cap;

	op->reg.width = 4; /* default width */
	if (0 < parse_x32(base, NULL, &op->reg.addr)) {
		op->reg.type = op->reg.type;
		op->reg.count = 1;
		op->reg.name= "RES";
	} else if (r = find_register(op, base)) {
		op->reg.type = r->type;
		op->reg.addr = r->addr;
		op->reg.width = r->width;
		op->reg.count = r->count;
		op->reg.name = r->name;
	} else if (!strncasecmp(base, "CAP", 3)) {
		if (parse_x32(base+3, NULL, &cap) > 0 && cap < 0x100) {
			op->reg.type = RES_PCICAP;
			op->reg.addr = (PCI_CAP_NORMAL << 16) | (cap & 0xFFFF);
		}
		op->reg.count = 1;
		op->reg.name= "CAP";
	} else if (!strncasecmp(base, "ECAP", 4)) {
		if (parse_x32(base+4, NULL, &cap) > 0 && cap < 0x1000) {
			op->reg.type = RES_PCIECAP;
			op->reg.addr = (PCI_CAP_EXTENDED << 16) | (cap & 0xFFFF);
		}
		op->reg.count = 1;
		op->reg.name= "ECAP";
	}
}

static void parse_op(char *c, struct pci_dev **selected_devices, int type)
{
	char *base, *offset, *width, *value;
	char *e, *f;
	int n, j;
	struct op *op;

	/* Split the argument */
	base = xstrdup(c);
	if (value = strchr(base, '='))
		*value++ = '\0';
	if (width = strchr(base, '.'))
		*width++ = '\0';
	if (offset = strchr(base, '+'))
		*offset++ = '\0';

	/* Look for setting of values and count how many */
	n = 0;
	if (value) {
		if (!*value)
			parse_err("Missing value");
		n++;
		for (e = value; *e; e++)
			if (*e == ',')
				n++;
	}

	/* Allocate the operation */
	op = xmalloc(sizeof(struct op) + n*sizeof(struct value));
	memset(op, 0, sizeof(*op));
	op->dev_vector = selected_devices;
	op->num_values = n;
	op->reg.type = type;

	/* Find the register */
	parse_register(op, base);

	/* What is the width suffix */
	if (width) {
		switch (*width & 0xdf) {
		case 'B':
			op->reg.width = 1;
			sprintf(op->format, "%%0%dx\n", op->reg.width * 2);
			break;
		case 'W':
			op->reg.width = 2;
			sprintf(op->format, "%%0%dx\n", op->reg.width * 2);
			break;
		case '\0':
		case 'D':
			op->reg.width = 4;
			sprintf(op->format, "%%0%dlx\n", op->reg.width * 2);
			break;
		case 'L':
			op->reg.width = 8;
			sprintf(op->format, "%%0%dllx\n", op->reg.width * 2);
			break;
		default:
			parse_err("Invalid width \"%c\"", *width);
		}
	} else {
		sprintf(op->format, "%%0%dlx\n", op->reg.width * 2);
	}
	//sprintf(op->format, "%%0%dllx\n", op->reg.width * 2);

	/* Add offset */
	if (offset) {
		u32 off;
		if (parse_x32(offset, NULL, &off) <= 0)
			parse_err("Invalid offset \"%s\"", offset);
		op->reg.offset += off;
	}

	/* Parse the values */
	for (j = 0; j < n; j++) {
		u32 ll, lim;
		e = strchr(value, ',');
		if (e)
			*e++ = 0;
		if (parse_x32(value, &f, &ll) < 0 || f && *f != ':')
			parse_err("Invalid value \"%s\"", value);
		lim = max_values[op->reg.width];
		if (ll > lim && ll < ~0U - lim)
			parse_err("Value \"%s\" is out of range", value);
		op->values[j].value = ll;
		if (f && *f == ':') {
			if (parse_x32(f+1, NULL, &ll) <= 0)
				parse_err("Invalid mask \"%s\"", f+1);
			if (ll > lim && ll < ~0U - lim)
				parse_err("Mask \"%s\" is out of range", f+1);
			op->values[j].mask = ll;
			op->values[j].value &= ll;
		} else {
			op->values[j].mask = ~0U;
		}
		value = e;
	}

	*last_op = op;
	last_op = &op->next;
	op->next = NULL;
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

int
main(int argc, char **argv)
{
	int i;

	pacc = pci_alloc();
	pacc->error = die;
	i = parse_options(argc, argv);

	pci_init(pacc);
	pci_scan_bus(pacc);

	parse_ops(argc, argv, i);
	scan_ops(first_op);
	execute(first_op);

	return 0;
}
