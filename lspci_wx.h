/*
 *	The PCI Utilities -- List All PCI Devices
 *
 *	Copyright (c) 1997--2010 Martin Mares <mj@ucw.cz>
 *
 *	Can be freely distributed and used under the terms of the GNU GPL.
 */

#define PCIUTILS_LSPCI
#include "pciutils.h"
#include "functions.h"

/*
 *  If we aren't being compiled by GCC, use xmalloc() instead of alloca().
 *  This increases our memory footprint, but only slightly since we don't
 *  use alloca() much.
 */
#if defined (__FreeBSD__) || defined (__NetBSD__) || defined (__OpenBSD__) || defined (__DragonFly__)
/* alloca() is defined in stdlib.h */
#elif defined(__GNUC__) && !defined(PCI_OS_WINDOWS)
#include <alloca.h>
#else
#undef alloca
#define alloca xmalloc
#endif

/*** Options ***/

extern int verbose;
extern struct pci_filter filter;
extern char *opt_pcimap;

/*** PCI devices and access to their config space ***/

struct device {
	struct device *next;
	struct pci_dev *dev;
	unsigned int config_cached, config_bufsize;
	byte *config;				/* Cached configuration space data */
	byte *present;			/* Maps which configuration bytes are present */
};

extern struct device *first_dev;
struct pci_access *pacc;

extern struct device *scan_device(struct pci_dev *p);
extern void show_device(struct device *d);

extern int config_fetch(struct device *d, unsigned int pos, unsigned int len);
extern u32 get_conf_long(struct device *d, unsigned int pos);
extern word get_conf_word(struct device *d, unsigned int pos);
extern byte get_conf_byte(struct device *d, unsigned int pos);

extern void get_subid(struct device *d, word *subvp, word *subdp);
extern void show_wx_info(struct device *d);

extern int show_wx_nic_info(int argc, char **argv);
extern u32 self_test(struct device *d);
extern void self_test1(struct device *d);

/* Useful macros for decoding of bits and bit fields */

#define FLAG(x,y) ((x & y) ? '+' : '-')
#define BITS(x,at,width) (((x) >> (at)) & ((1 << (width)) - 1))
#define TABLE(tab,x,buf) ((x) < sizeof(tab)/sizeof((tab)[0]) ? (tab)[x] : (sprintf((buf), "??%d", (x)), (buf)))

/* ls-vpd.c */

extern void cap_vpd(struct device *d);

/* ls-caps.c */

extern void show_caps(struct device *d, int where);

/* ls-ecaps.c */

extern void show_ext_caps(struct device *d);

/* ls-caps-vendor.c */

extern void show_vendor_caps(struct device *d, int where, int cap);

/* ls-kernel.c */

extern void show_kernel_machine(struct device *d UNUSED);
extern void show_kernel(struct device *d UNUSED);
extern void show_kernel_cleanup(void);

/* ls-tree.c */

void show_forest(void);

/* ls-map.c */

void map_the_bus(void);

static char *
find_driver(struct device *d, char *buf);

/* Options */

int verbose;				/* Show detailed information */
static int opt_hex;			/* Show contents of config space as hexadecimal numbers */
struct pci_filter filter;		/* Device filter */
static int opt_tree;			/* Show bus tree */
static int opt_machine;			/* Generate machine-readable output */
static int opt_map_mode;		/* Bus mapping mode enabled */
static int opt_domains;			/* Show domain numbers (0=disabled, 1=auto-detected, 2=requested) */
static int opt_kernel;			/* Show kernel drivers */
static int opt_query_dns;		/* Query the DNS (0=disabled, 1=enabled, 2=refresh cache) */
static int opt_query_all;		/* Query the DNS for all entries */
char *opt_pcimap;			/* Override path to Linux modules.pcimap */
static int opt_wx_info;
static int opt_self_test;		/*Do self test*/

static int test_lvl;

static char options[] = "nvbxs:d:tmgp:qkMDQiT:" GENERIC_OPTIONS ;

static char help_msg[] =
"Usage: wxtool show [<switches>]\n"
"\n"
"Basic display modes:\n"
"-mm\t\tProduce machine-readable output (single -m for an obsolete format)\n"
"-t\t\tShow bus tree\n"
"\n"
"Display options:\n"
"-v\t\tBe verbose (-vv for very verbose)\n"
#ifdef PCI_OS_LINUX
"-k\t\tShow kernel drivers handling each device\n"
#endif
"-x\t\tShow hex-dump of the standard part of the config space\n"
"-xxx\t\tShow hex-dump of the whole config space (dangerous; root only)\n"
"-xxxx\t\tShow hex-dump of the 4096-byte extended config space (root only)\n"
"-b\t\tBus-centric view (addresses and IRQ's as seen by the bus)\n"
"-D\t\tAlways show domain numbers\n"
"\n"
"Resolving of device ID's to names:\n"
"-n\t\tShow numeric ID's\n"
"-nn\t\tShow both textual and numeric ID's (names & numbers)\n"
#ifdef PCI_USE_DNS
"-q\t\tQuery the PCI ID database for unknown ID's via DNS\n"
"-qq\t\tAs above, but re-query locally cached entries\n"
"-Q\t\tQuery the PCI ID database for all ID's via DNS\n"
#endif
"\n"
"Use for wx nic self test\n"
"-i\t\tCheck chip status and show some info\n"
"-T [0-3](level)\tself-test level\n"
"\t\tlevel >=1 need driver load"
"\n"
"Selection of devices:\n"
"-s [[[[<domain>]:]<bus>]:][<slot>][.[<func>]]\tShow only devices in selected slots\n"
"-d [<vendor>]:[<device>][:<class>]\t\tShow only devices with specified ID's\n"
"\n"
//"Other options:\n"
//"-i <file>\tUse specified ID database instead of %s\n"
//#ifdef PCI_OS_LINUX
//"-p <file>\tLook up kernel modules in a given file instead of default modules.pcimap\n"
//#endif
//"-M\t\tEnable `bus mapping' mode (dangerous; root only)\n"
//"\n"
//"PCI access options:\n"
//GENERIC_HELP
;

/*** Our view of the PCI bus ***/

struct device *first_dev;
static int seen_errors;



static char *
find_driver(struct device *d, char *buf)
{
  struct pci_dev *dev = d->dev;
  char name[1024], *drv, *base;
  int n;

  if (dev->access->method != PCI_ACCESS_SYS_BUS_PCI)
    return NULL;

  base = pci_get_param(dev->access, "sysfs.path");
  if (!base || !base[0])
    return NULL;

  n = snprintf(name, sizeof(name), "%s/devices/%04x:%02x:%02x.%d/driver",
	       base, dev->domain, dev->bus, dev->dev, dev->func);
  if (n < 0 || n >= (int)sizeof(name))
    die("show_driver: sysfs device name too long, why?");

  n = readlink(name, buf, 1024);
  if (n < 0)
    return NULL;
  if (n >= 1024)
    return "<name-too-long>";
  buf[n] = 0;

  if (drv = strrchr(buf, '/'))
    return drv+1;
  else
    return buf;
}


int
config_fetch(struct device *d, unsigned int pos, unsigned int len)
{
	unsigned int end = pos+len;
	int result;

	while (pos < d->config_bufsize && len && d->present[pos])
		pos++, len--;
	while (pos+len <= d->config_bufsize && len && d->present[pos+len-1])
		len--;
	if (!len)
		return 1;

	if (end > d->config_bufsize)
		{
			int orig_size = d->config_bufsize;
			while (end > d->config_bufsize)
	d->config_bufsize *= 2;
			d->config = xrealloc(d->config, d->config_bufsize);
			d->present = xrealloc(d->present, d->config_bufsize);
			memset(d->present + orig_size, 0, d->config_bufsize - orig_size);
		}
	result = pci_read_block(d->dev, pos, d->config + pos, len);
	if (result)
		memset(d->present + pos, 1, len);
	return result;
}


struct device *
scan_device(struct pci_dev *p)
{
	struct device *d;

	if (p->domain && !opt_domains)
		opt_domains = 1;
  if (!pci_filter_match(&filter, p))
   return NULL;
	d = xmalloc(sizeof(struct device));
	memset(d, 0, sizeof(*d));
	d->dev = p;
	d->config_cached = d->config_bufsize = 64;
	d->config = xmalloc(64);
	d->present = xmalloc(64);
	memset(d->present, 1, 64);
	if (!pci_read_block(p, 0, d->config, 64))
		{
			fprintf(stderr, "lspci: Unable to read the standard configuration space header of device %04x:%02x:%02x.%d\n",
				p->domain, p->bus, p->dev, p->func);
			seen_errors++;
			return NULL;
		}
	if ((d->config[PCI_HEADER_TYPE] & 0x7f) == PCI_HEADER_TYPE_CARDBUS)
		{
			/* For cardbus bridges, we need to fetch 64 bytes more to get the
 *        * full standard header... */
			if (config_fetch(d, 64, 64))
	d->config_cached += 64;
		}
	pci_setup_cache(p, d->config, d->config_cached);
	pci_fill_info(p, PCI_FILL_IDENT | PCI_FILL_CLASS);
	return d;
}

static void
scan_devices(void)
{
	struct device *d;
	struct pci_dev *p;

	pci_scan_bus(pacc);
	for (p=pacc->devices; p; p=p->next)
		if (d = scan_device(p))
			{
	d->next = first_dev;
	first_dev = d;
			}
}

/*** Config space accesses ***/

static void
check_conf_range(struct device *d, unsigned int pos, unsigned int len)
{
	while (len)
		if (!d->present[pos])
			die("Internal bug: Accessing non-read configuration byte at position %x", pos);
		else
			pos++, len--;
}

byte
get_conf_byte(struct device *d, unsigned int pos)
{
	check_conf_range(d, pos, 1);
	return d->config[pos];
}

word
get_conf_word(struct device *d, unsigned int pos)
{
	check_conf_range(d, pos, 2);
	return d->config[pos] | (d->config[pos+1] << 8);
}

u32
get_conf_long(struct device *d, unsigned int pos)
{
	check_conf_range(d, pos, 4);
	return d->config[pos] |
		(d->config[pos+1] << 8) |
		(d->config[pos+2] << 16) |
		(d->config[pos+3] << 24);
}

/*** Sorting ***/

static int
compare_them(const void *A, const void *B)
{
	const struct pci_dev *a = (*(const struct device **)A)->dev;
	const struct pci_dev *b = (*(const struct device **)B)->dev;

	if (a->domain < b->domain)
		return -1;
	if (a->domain > b->domain)
		return 1;
	if (a->bus < b->bus)
		return -1;
	if (a->bus > b->bus)
		return 1;
	if (a->dev < b->dev)
		return -1;
	if (a->dev > b->dev)
		return 1;
	if (a->func < b->func)
		return -1;
	if (a->func > b->func)
		return 1;
	return 0;
}

static void
sort_them(void)
{
	struct device **index, **h, **last_dev;
	int cnt;
	struct device *d;

	cnt = 0;
	for (d=first_dev; d; d=d->next)
		cnt++;
	h = index = alloca(sizeof(struct device *) * cnt);
	for (d=first_dev; d; d=d->next)
		*h++ = d;
	qsort(index, cnt, sizeof(struct device *), compare_them);
	last_dev = &first_dev;
	h = index;
	while (cnt--)
		{
			*last_dev = *h;
			last_dev = &(*h)->next;
			h++;
		}
	*last_dev = NULL;
}

/*** Normal output ***/

static void
show_slot_name(struct device *d)
{
	struct pci_dev *p = d->dev;

	if (!opt_machine ? opt_domains : (p->domain || opt_domains >= 2))
		printf("%04x:", p->domain);
	printf("%02x:%02x.%d", p->bus, p->dev, p->func);
}

void
get_subid(struct device *d, word *subvp, word *subdp)
{
	byte htype = get_conf_byte(d, PCI_HEADER_TYPE) & 0x7f;

	if (htype == PCI_HEADER_TYPE_NORMAL)
		{
			*subvp = get_conf_word(d, PCI_SUBSYSTEM_VENDOR_ID);
			*subdp = get_conf_word(d, PCI_SUBSYSTEM_ID);
		}
	else if (htype == PCI_HEADER_TYPE_CARDBUS && d->config_cached >= 128)
		{
			*subvp = get_conf_word(d, PCI_CB_SUBSYSTEM_VENDOR_ID);
			*subdp = get_conf_word(d, PCI_CB_SUBSYSTEM_ID);
		}
	else
		*subvp = *subdp = 0xffff;
}

static void
show_terse(struct device *d)
{
	int c;
	struct pci_dev *p = d->dev;
	char classbuf[128], devbuf[128];

	show_slot_name(d);
	printf(" %s: %s",
	 pci_lookup_name(pacc, classbuf, sizeof(classbuf),
			 PCI_LOOKUP_CLASS,
			 p->device_class),
	 pci_lookup_name(pacc, devbuf, sizeof(devbuf),
			 PCI_LOOKUP_VENDOR | PCI_LOOKUP_DEVICE,
			 p->vendor_id, p->device_id));
	if (c = get_conf_byte(d, PCI_REVISION_ID))
		printf(" (rev %02x)", c);
	if (verbose)
		{
			char *x;
			c = get_conf_byte(d, PCI_CLASS_PROG);
			x = pci_lookup_name(pacc, devbuf, sizeof(devbuf),
				PCI_LOOKUP_PROGIF | PCI_LOOKUP_NO_NUMBERS,
				p->device_class, c);
			if (c || x)
	{
		printf(" (prog-if %02x", c);
		if (x)
			printf(" [%s]", x);
		putchar(')');
	}
		}
	putchar('\n');

	if (verbose || opt_kernel)
		{
			word subsys_v, subsys_d;
			char ssnamebuf[256];

			if (p->label)
				printf("\tDeviceName: %s", p->label);
			get_subid(d, &subsys_v, &subsys_d);
			if (subsys_v && subsys_v != 0xffff)
	printf("\tSubsystem: %s\n",
		pci_lookup_name(pacc, ssnamebuf, sizeof(ssnamebuf),
			PCI_LOOKUP_SUBSYSTEM | PCI_LOOKUP_VENDOR | PCI_LOOKUP_DEVICE,
			p->vendor_id, p->device_id, subsys_v, subsys_d));
		}
}

/*** Verbose output ***/

static void
show_size(u64 x)
{
	static const char suffix[][2] = { "", "K", "M", "G", "T" };
	unsigned i;
	if (!x)
		return;
	for (i = 0; i < (sizeof(suffix) / sizeof(*suffix) - 1); i++) {
		if (x % 1024)
			break;
		x /= 1024;
	}
	printf(" [size=%u%s]", (unsigned)x, suffix[i]);
}

static void
show_range(char *prefix, u64 base, u64 limit, int is_64bit)
{
	if (base > limit)
		{
			if (!verbose)
	return;
			else if (verbose < 3)
	{
		printf("%s: None\n", prefix);
		return;
	}
		}

	printf("%s: ", prefix);
	if (is_64bit)
		printf("%016" PCI_U64_FMT_X "-%016" PCI_U64_FMT_X, base, limit);
	else
		printf("%08x-%08x", (unsigned) base, (unsigned) limit);
	if (base <= limit)
		show_size(limit - base + 1);
	else
		printf(" [empty]");
	putchar('\n');
}

static void
show_bases(struct device *d, int cnt)
{
	struct pci_dev *p = d->dev;
	word cmd = get_conf_word(d, PCI_COMMAND);
	int i;
	int virtual = 0;

	for (i=0; i<cnt; i++)
		{
			pciaddr_t pos = p->base_addr[i];
			pciaddr_t len = (p->known_fields & PCI_FILL_SIZES) ? p->size[i] : 0;
			pciaddr_t ioflg = (p->known_fields & PCI_FILL_IO_FLAGS) ? p->flags[i] : 0;
			u32 flg = get_conf_long(d, PCI_BASE_ADDRESS_0 + 4*i);
			if (flg == 0xffffffff)
	flg = 0;
			if (!pos && !flg && !len)
	continue;
			if (verbose > 1)
	printf("\tRegion %d: ", i);
			else
	putchar('\t');
			if (ioflg & PCI_IORESOURCE_PCI_EA_BEI)
		printf("[enhanced] ");
			else if (pos && !flg)	/* Reported by the OS, but not by the device */
	{
		printf("[virtual] ");
		flg = pos;
		virtual = 1;
	}
			if (flg & PCI_BASE_ADDRESS_SPACE_IO)
	{
		pciaddr_t a = pos & PCI_BASE_ADDRESS_IO_MASK;
		printf("I/O ports at ");
		if (a || (cmd & PCI_COMMAND_IO))
			printf(PCIADDR_PORT_FMT, a);
		else if (flg & PCI_BASE_ADDRESS_IO_MASK)
			printf("<ignored>");
		else
			printf("<unassigned>");
		if (!virtual && !(cmd & PCI_COMMAND_IO))
			printf(" [disabled]");
	}
			else
	{
		int t = flg & PCI_BASE_ADDRESS_MEM_TYPE_MASK;
		pciaddr_t a = pos & PCI_ADDR_MEM_MASK;
		int done = 0;
		u32 z = 0;

		printf("Memory at ");
		if (t == PCI_BASE_ADDRESS_MEM_TYPE_64)
			{
				if (i >= cnt - 1)
		{
			printf("<invalid-64bit-slot>");
			done = 1;
		}
				else
		{
			i++;
			z = get_conf_long(d, PCI_BASE_ADDRESS_0 + 4*i);
		}
			}
		if (!done)
			{
				if (a)
		printf(PCIADDR_T_FMT, a);
				else
		printf(((flg & PCI_BASE_ADDRESS_MEM_MASK) || z) ? "<ignored>" : "<unassigned>");
			}
		printf(" (%s, %sprefetchable)",
		 (t == PCI_BASE_ADDRESS_MEM_TYPE_32) ? "32-bit" :
		 (t == PCI_BASE_ADDRESS_MEM_TYPE_64) ? "64-bit" :
		 (t == PCI_BASE_ADDRESS_MEM_TYPE_1M) ? "low-1M" : "type 3",
		 (flg & PCI_BASE_ADDRESS_MEM_PREFETCH) ? "" : "non-");
		if (!virtual && !(cmd & PCI_COMMAND_MEMORY))
			printf(" [disabled]");
	}
			show_size(len);
			putchar('\n');
		}
}

static void
show_rom(struct device *d, int reg)
{
	struct pci_dev *p = d->dev;
	pciaddr_t rom = p->rom_base_addr;
	pciaddr_t len = (p->known_fields & PCI_FILL_SIZES) ? p->rom_size : 0;
	pciaddr_t ioflg = (p->known_fields & PCI_FILL_IO_FLAGS) ? p->rom_flags : 0;
	u32 flg = get_conf_long(d, reg);
	word cmd = get_conf_word(d, PCI_COMMAND);
	int virtual = 0;

	if (!rom && !flg && !len)
		return;
	putchar('\t');
	if (ioflg & PCI_IORESOURCE_PCI_EA_BEI)
			printf("[enhanced] ");
	else if ((rom & PCI_ROM_ADDRESS_MASK) && !(flg & PCI_ROM_ADDRESS_MASK))
		{
			printf("[virtual] ");
			flg = rom;
			virtual = 1;
		}
	printf("Expansion ROM at ");
	if (rom & PCI_ROM_ADDRESS_MASK)
		printf(PCIADDR_T_FMT, rom & PCI_ROM_ADDRESS_MASK);
	else if (flg & PCI_ROM_ADDRESS_MASK)
		printf("<ignored>");
	else
		printf("<unassigned>");
	if (!(flg & PCI_ROM_ADDRESS_ENABLE))
		printf(" [disabled]");
	else if (!virtual && !(cmd & PCI_COMMAND_MEMORY))
		printf(" [disabled by cmd]");
	show_size(len);
	putchar('\n');
}

static void
show_htype0(struct device *d)
{
	show_bases(d, 6);
	show_rom(d, PCI_ROM_ADDRESS);
	show_caps(d, PCI_CAPABILITY_LIST);
}

static void
show_htype1(struct device *d)
{
	u32 io_base = get_conf_byte(d, PCI_IO_BASE);
	u32 io_limit = get_conf_byte(d, PCI_IO_LIMIT);
	u32 io_type = io_base & PCI_IO_RANGE_TYPE_MASK;
	u32 mem_base = get_conf_word(d, PCI_MEMORY_BASE);
	u32 mem_limit = get_conf_word(d, PCI_MEMORY_LIMIT);
	u32 mem_type = mem_base & PCI_MEMORY_RANGE_TYPE_MASK;
	u32 pref_base = get_conf_word(d, PCI_PREF_MEMORY_BASE);
	u32 pref_limit = get_conf_word(d, PCI_PREF_MEMORY_LIMIT);
	u32 pref_type = pref_base & PCI_PREF_RANGE_TYPE_MASK;
	word sec_stat = get_conf_word(d, PCI_SEC_STATUS);
	word brc = get_conf_word(d, PCI_BRIDGE_CONTROL);

	show_bases(d, 2);
	printf("\tBus: primary=%02x, secondary=%02x, subordinate=%02x, sec-latency=%d\n",
	 get_conf_byte(d, PCI_PRIMARY_BUS),
	 get_conf_byte(d, PCI_SECONDARY_BUS),
	 get_conf_byte(d, PCI_SUBORDINATE_BUS),
	 get_conf_byte(d, PCI_SEC_LATENCY_TIMER));

	if (io_type != (io_limit & PCI_IO_RANGE_TYPE_MASK) ||
			(io_type != PCI_IO_RANGE_TYPE_16 && io_type != PCI_IO_RANGE_TYPE_32))
		printf("\t!!! Unknown I/O range types %x/%x\n", io_base, io_limit);
	else
		{
			io_base = (io_base & PCI_IO_RANGE_MASK) << 8;
			io_limit = (io_limit & PCI_IO_RANGE_MASK) << 8;
			if (io_type == PCI_IO_RANGE_TYPE_32)
	{
		io_base |= (get_conf_word(d, PCI_IO_BASE_UPPER16) << 16);
		io_limit |= (get_conf_word(d, PCI_IO_LIMIT_UPPER16) << 16);
	}
			show_range("\tI/O behind bridge", io_base, io_limit+0xfff, 0);
		}

	if (mem_type != (mem_limit & PCI_MEMORY_RANGE_TYPE_MASK) ||
			mem_type)
		printf("\t!!! Unknown memory range types %x/%x\n", mem_base, mem_limit);
	else
		{
			mem_base = (mem_base & PCI_MEMORY_RANGE_MASK) << 16;
			mem_limit = (mem_limit & PCI_MEMORY_RANGE_MASK) << 16;
			show_range("\tMemory behind bridge", mem_base, mem_limit + 0xfffff, 0);
		}

	if (pref_type != (pref_limit & PCI_PREF_RANGE_TYPE_MASK) ||
			(pref_type != PCI_PREF_RANGE_TYPE_32 && pref_type != PCI_PREF_RANGE_TYPE_64))
		printf("\t!!! Unknown prefetchable memory range types %x/%x\n", pref_base, pref_limit);
	else
		{
			u64 pref_base_64 = (pref_base & PCI_PREF_RANGE_MASK) << 16;
			u64 pref_limit_64 = (pref_limit & PCI_PREF_RANGE_MASK) << 16;
			if (pref_type == PCI_PREF_RANGE_TYPE_64)
	{
		pref_base_64 |= (u64) get_conf_long(d, PCI_PREF_BASE_UPPER32) << 32;
		pref_limit_64 |= (u64) get_conf_long(d, PCI_PREF_LIMIT_UPPER32) << 32;
	}
			show_range("\tPrefetchable memory behind bridge", pref_base_64, pref_limit_64 + 0xfffff, (pref_type == PCI_PREF_RANGE_TYPE_64));
		}

	if (verbose > 1)
		printf("\tSecondary status: 66MHz%c FastB2B%c ParErr%c DEVSEL=%s >TAbort%c <TAbort%c <MAbort%c <SERR%c <PERR%c\n",
			 FLAG(sec_stat, PCI_STATUS_66MHZ),
			 FLAG(sec_stat, PCI_STATUS_FAST_BACK),
			 FLAG(sec_stat, PCI_STATUS_PARITY),
			 ((sec_stat & PCI_STATUS_DEVSEL_MASK) == PCI_STATUS_DEVSEL_SLOW) ? "slow" :
			 ((sec_stat & PCI_STATUS_DEVSEL_MASK) == PCI_STATUS_DEVSEL_MEDIUM) ? "medium" :
			 ((sec_stat & PCI_STATUS_DEVSEL_MASK) == PCI_STATUS_DEVSEL_FAST) ? "fast" : "??",
			 FLAG(sec_stat, PCI_STATUS_SIG_TARGET_ABORT),
			 FLAG(sec_stat, PCI_STATUS_REC_TARGET_ABORT),
			 FLAG(sec_stat, PCI_STATUS_REC_MASTER_ABORT),
			 FLAG(sec_stat, PCI_STATUS_SIG_SYSTEM_ERROR),
			 FLAG(sec_stat, PCI_STATUS_DETECTED_PARITY));

	show_rom(d, PCI_ROM_ADDRESS1);

	if (verbose > 1)
		{
			printf("\tBridgeCtl: Parity%c SERR%c NoISA%c VGA%c MAbort%c >Reset%c FastB2B%c\n",
	FLAG(brc, PCI_BRIDGE_CTL_PARITY),
	FLAG(brc, PCI_BRIDGE_CTL_SERR),
	FLAG(brc, PCI_BRIDGE_CTL_NO_ISA),
	FLAG(brc, PCI_BRIDGE_CTL_VGA),
	FLAG(brc, PCI_BRIDGE_CTL_MASTER_ABORT),
	FLAG(brc, PCI_BRIDGE_CTL_BUS_RESET),
	FLAG(brc, PCI_BRIDGE_CTL_FAST_BACK));
			printf("\t\tPriDiscTmr%c SecDiscTmr%c DiscTmrStat%c DiscTmrSERREn%c\n",
	FLAG(brc, PCI_BRIDGE_CTL_PRI_DISCARD_TIMER),
	FLAG(brc, PCI_BRIDGE_CTL_SEC_DISCARD_TIMER),
	FLAG(brc, PCI_BRIDGE_CTL_DISCARD_TIMER_STATUS),
	FLAG(brc, PCI_BRIDGE_CTL_DISCARD_TIMER_SERR_EN));
		}

	show_caps(d, PCI_CAPABILITY_LIST);
}

static void
show_htype2(struct device *d)
{
	int i;
	word cmd = get_conf_word(d, PCI_COMMAND);
	word brc = get_conf_word(d, PCI_CB_BRIDGE_CONTROL);
	word exca;
	int verb = verbose > 2;

	show_bases(d, 1);
	printf("\tBus: primary=%02x, secondary=%02x, subordinate=%02x, sec-latency=%d\n",
	 get_conf_byte(d, PCI_CB_PRIMARY_BUS),
	 get_conf_byte(d, PCI_CB_CARD_BUS),
	 get_conf_byte(d, PCI_CB_SUBORDINATE_BUS),
	 get_conf_byte(d, PCI_CB_LATENCY_TIMER));
	for (i=0; i<2; i++)
		{
			int p = 8*i;
			u32 base = get_conf_long(d, PCI_CB_MEMORY_BASE_0 + p);
			u32 limit = get_conf_long(d, PCI_CB_MEMORY_LIMIT_0 + p);
			limit = limit + 0xfff;
			if (base <= limit || verb)
	printf("\tMemory window %d: %08x-%08x%s%s\n", i, base, limit,
				 (cmd & PCI_COMMAND_MEMORY) ? "" : " [disabled]",
				 (brc & (PCI_CB_BRIDGE_CTL_PREFETCH_MEM0 << i)) ? " (prefetchable)" : "");
		}
	for (i=0; i<2; i++)
		{
			int p = 8*i;
			u32 base = get_conf_long(d, PCI_CB_IO_BASE_0 + p);
			u32 limit = get_conf_long(d, PCI_CB_IO_LIMIT_0 + p);
			if (!(base & PCI_IO_RANGE_TYPE_32))
	{
		base &= 0xffff;
		limit &= 0xffff;
	}
			base &= PCI_CB_IO_RANGE_MASK;
			limit = (limit & PCI_CB_IO_RANGE_MASK) + 3;
			if (base <= limit || verb)
	printf("\tI/O window %d: %08x-%08x%s\n", i, base, limit,
				 (cmd & PCI_COMMAND_IO) ? "" : " [disabled]");
		}

	if (get_conf_word(d, PCI_CB_SEC_STATUS) & PCI_STATUS_SIG_SYSTEM_ERROR)
		printf("\tSecondary status: SERR\n");
	if (verbose > 1)
		printf("\tBridgeCtl: Parity%c SERR%c ISA%c VGA%c MAbort%c >Reset%c 16bInt%c PostWrite%c\n",
		 FLAG(brc, PCI_CB_BRIDGE_CTL_PARITY),
		 FLAG(brc, PCI_CB_BRIDGE_CTL_SERR),
		 FLAG(brc, PCI_CB_BRIDGE_CTL_ISA),
		 FLAG(brc, PCI_CB_BRIDGE_CTL_VGA),
		 FLAG(brc, PCI_CB_BRIDGE_CTL_MASTER_ABORT),
		 FLAG(brc, PCI_CB_BRIDGE_CTL_CB_RESET),
		 FLAG(brc, PCI_CB_BRIDGE_CTL_16BIT_INT),
		 FLAG(brc, PCI_CB_BRIDGE_CTL_POST_WRITES));

	if (d->config_cached < 128)
		{
			printf("\t<access denied to the rest>\n");
			return;
		}

	exca = get_conf_word(d, PCI_CB_LEGACY_MODE_BASE);
	if (exca)
		printf("\t16-bit legacy interface ports at %04x\n", exca);
	show_caps(d, PCI_CB_CAPABILITY_LIST);
}

static void
show_verbose(struct device *d)
{
	struct pci_dev *p = d->dev;
	word status = get_conf_word(d, PCI_STATUS);
	word cmd = get_conf_word(d, PCI_COMMAND);
	word class = p->device_class;
	byte bist = get_conf_byte(d, PCI_BIST);
	byte htype = get_conf_byte(d, PCI_HEADER_TYPE) & 0x7f;
	byte latency = get_conf_byte(d, PCI_LATENCY_TIMER);
	byte cache_line = get_conf_byte(d, PCI_CACHE_LINE_SIZE);
	byte max_lat, min_gnt;
	byte int_pin = get_conf_byte(d, PCI_INTERRUPT_PIN);
	unsigned int irq;

	show_terse(d);

	pci_fill_info(p, PCI_FILL_IRQ | PCI_FILL_BASES | PCI_FILL_ROM_BASE | PCI_FILL_SIZES |
		PCI_FILL_PHYS_SLOT | PCI_FILL_LABEL | PCI_FILL_NUMA_NODE);
	irq = p->irq;

	switch (htype)
		{
		case PCI_HEADER_TYPE_NORMAL:
			if (class == PCI_CLASS_BRIDGE_PCI)
	printf("\t!!! Invalid class %04x for header type %02x\n", class, htype);
			max_lat = get_conf_byte(d, PCI_MAX_LAT);
			min_gnt = get_conf_byte(d, PCI_MIN_GNT);
			break;
		case PCI_HEADER_TYPE_BRIDGE:
			if ((class >> 8) != PCI_BASE_CLASS_BRIDGE)
	printf("\t!!! Invalid class %04x for header type %02x\n", class, htype);
			min_gnt = max_lat = 0;
			break;
		case PCI_HEADER_TYPE_CARDBUS:
			if ((class >> 8) != PCI_BASE_CLASS_BRIDGE)
	printf("\t!!! Invalid class %04x for header type %02x\n", class, htype);
			min_gnt = max_lat = 0;
			break;
		default:
			printf("\t!!! Unknown header type %02x\n", htype);
			return;
		}

	if (p->phy_slot)
		printf("\tPhysical Slot: %s\n", p->phy_slot);

	if (verbose > 1)
		{
			printf("\tControl: I/O%c Mem%c BusMaster%c SpecCycle%c MemWINV%c VGASnoop%c ParErr%c Stepping%c SERR%c FastB2B%c DisINTx%c\n",
			 FLAG(cmd, PCI_COMMAND_IO),
			 FLAG(cmd, PCI_COMMAND_MEMORY),
			 FLAG(cmd, PCI_COMMAND_MASTER),
			 FLAG(cmd, PCI_COMMAND_SPECIAL),
			 FLAG(cmd, PCI_COMMAND_INVALIDATE),
			 FLAG(cmd, PCI_COMMAND_VGA_PALETTE),
			 FLAG(cmd, PCI_COMMAND_PARITY),
			 FLAG(cmd, PCI_COMMAND_WAIT),
			 FLAG(cmd, PCI_COMMAND_SERR),
			 FLAG(cmd, PCI_COMMAND_FAST_BACK),
			 FLAG(cmd, PCI_COMMAND_DISABLE_INTx));
			printf("\tStatus: Cap%c 66MHz%c UDF%c FastB2B%c ParErr%c DEVSEL=%s >TAbort%c <TAbort%c <MAbort%c >SERR%c <PERR%c INTx%c\n",
			 FLAG(status, PCI_STATUS_CAP_LIST),
			 FLAG(status, PCI_STATUS_66MHZ),
			 FLAG(status, PCI_STATUS_UDF),
			 FLAG(status, PCI_STATUS_FAST_BACK),
			 FLAG(status, PCI_STATUS_PARITY),
			 ((status & PCI_STATUS_DEVSEL_MASK) == PCI_STATUS_DEVSEL_SLOW) ? "slow" :
			 ((status & PCI_STATUS_DEVSEL_MASK) == PCI_STATUS_DEVSEL_MEDIUM) ? "medium" :
			 ((status & PCI_STATUS_DEVSEL_MASK) == PCI_STATUS_DEVSEL_FAST) ? "fast" : "??",
			 FLAG(status, PCI_STATUS_SIG_TARGET_ABORT),
			 FLAG(status, PCI_STATUS_REC_TARGET_ABORT),
			 FLAG(status, PCI_STATUS_REC_MASTER_ABORT),
			 FLAG(status, PCI_STATUS_SIG_SYSTEM_ERROR),
			 FLAG(status, PCI_STATUS_DETECTED_PARITY),
			 FLAG(status, PCI_STATUS_INTx));
			if (cmd & PCI_COMMAND_MASTER)
	{
		printf("\tLatency: %d", latency);
		if (min_gnt || max_lat)
			{
				printf(" (");
				if (min_gnt)
		printf("%dns min", min_gnt*250);
				if (min_gnt && max_lat)
		printf(", ");
				if (max_lat)
		printf("%dns max", max_lat*250);
				putchar(')');
			}
		if (cache_line)
			printf(", Cache Line Size: %d bytes", cache_line * 4);
		putchar('\n');
	}
			if (int_pin || irq)
	printf("\tInterrupt: pin %c routed to IRQ " PCIIRQ_FMT "\n",
				 (int_pin ? 'A' + int_pin - 1 : '?'), irq);
			if (p->numa_node != -1)
	printf("\tNUMA node: %d\n", p->numa_node);
		}
	else
		{
			printf("\tFlags: ");
			if (cmd & PCI_COMMAND_MASTER)
	printf("bus master, ");
			if (cmd & PCI_COMMAND_VGA_PALETTE)
	printf("VGA palette snoop, ");
			if (cmd & PCI_COMMAND_WAIT)
	printf("stepping, ");
			if (cmd & PCI_COMMAND_FAST_BACK)
	printf("fast Back2Back, ");
			if (status & PCI_STATUS_66MHZ)
	printf("66MHz, ");
			if (status & PCI_STATUS_UDF)
	printf("user-definable features, ");
			printf("%s devsel",
			 ((status & PCI_STATUS_DEVSEL_MASK) == PCI_STATUS_DEVSEL_SLOW) ? "slow" :
			 ((status & PCI_STATUS_DEVSEL_MASK) == PCI_STATUS_DEVSEL_MEDIUM) ? "medium" :
			 ((status & PCI_STATUS_DEVSEL_MASK) == PCI_STATUS_DEVSEL_FAST) ? "fast" : "??");
			if (cmd & PCI_COMMAND_MASTER)
	printf(", latency %d", latency);
			if (irq)
	printf(", IRQ " PCIIRQ_FMT, irq);
			if (p->numa_node != -1)
	printf(", NUMA node %d", p->numa_node);
			putchar('\n');
		}

	if (bist & PCI_BIST_CAPABLE)
		{
			if (bist & PCI_BIST_START)
	printf("\tBIST is running\n");
			else
	printf("\tBIST result: %02x\n", bist & PCI_BIST_CODE_MASK);
		}

	switch (htype)
		{
		case PCI_HEADER_TYPE_NORMAL:
			show_htype0(d);
			break;
		case PCI_HEADER_TYPE_BRIDGE:
			show_htype1(d);
			break;
		case PCI_HEADER_TYPE_CARDBUS:
			show_htype2(d);
			break;
		}
}

/*** Machine-readable dumps ***/

static void
show_hex_dump(struct device *d)
{
	unsigned int i, cnt;

	cnt = d->config_cached;
	if (opt_hex >= 3 && config_fetch(d, cnt, 256-cnt))
		{
			cnt = 256;
			if (opt_hex >= 4 && config_fetch(d, 256, 4096-256))
	cnt = 4096;
		}

	for (i=0; i<cnt; i++)
		{
			if (! (i & 15))
	printf("%02x:", i);
			printf(" %02x", get_conf_byte(d, i));
			if ((i & 15) == 15)
	putchar('\n');
		}
}

static void
print_shell_escaped(char *c)
{
	printf(" \"");
	while (*c)
		{
			if (*c == '"' || *c == '\\')
	putchar('\\');
			putchar(*c++);
		}
	putchar('"');
}

static void
show_machine(struct device *d)
{
	struct pci_dev *p = d->dev;
	int c;
	word sv_id, sd_id;
	char classbuf[128], vendbuf[128], devbuf[128], svbuf[128], sdbuf[128];

	get_subid(d, &sv_id, &sd_id);

	if (verbose)
		{
			pci_fill_info(p, PCI_FILL_PHYS_SLOT | PCI_FILL_NUMA_NODE);
			printf((opt_machine >= 2) ? "Slot:\t" : "Device:\t");
			show_slot_name(d);
			putchar('\n');
			printf("Class:\t%s\n",
			 pci_lookup_name(pacc, classbuf, sizeof(classbuf), PCI_LOOKUP_CLASS, p->device_class));
			printf("Vendor:\t%s\n",
			 pci_lookup_name(pacc, vendbuf, sizeof(vendbuf), PCI_LOOKUP_VENDOR, p->vendor_id, p->device_id));
			printf("Device:\t%s\n",
			 pci_lookup_name(pacc, devbuf, sizeof(devbuf), PCI_LOOKUP_DEVICE, p->vendor_id, p->device_id));
			if (sv_id && sv_id != 0xffff)
	{
		printf("SVendor:\t%s\n",
		 pci_lookup_name(pacc, svbuf, sizeof(svbuf), PCI_LOOKUP_SUBSYSTEM | PCI_LOOKUP_VENDOR, sv_id));
		printf("SDevice:\t%s\n",
		 pci_lookup_name(pacc, sdbuf, sizeof(sdbuf), PCI_LOOKUP_SUBSYSTEM | PCI_LOOKUP_DEVICE, p->vendor_id, p->device_id, sv_id, sd_id));
	}
			if (p->phy_slot)
	printf("PhySlot:\t%s\n", p->phy_slot);
			if (c = get_conf_byte(d, PCI_REVISION_ID))
	printf("Rev:\t%02x\n", c);
			if (c = get_conf_byte(d, PCI_CLASS_PROG))
	printf("ProgIf:\t%02x\n", c);
			if (opt_kernel)
	show_kernel_machine(d);
			if (p->numa_node != -1)
	printf("NUMANode:\t%d\n", p->numa_node);
		}
	else
		{
			show_slot_name(d);
			print_shell_escaped(pci_lookup_name(pacc, classbuf, sizeof(classbuf), PCI_LOOKUP_CLASS, p->device_class));
			print_shell_escaped(pci_lookup_name(pacc, vendbuf, sizeof(vendbuf), PCI_LOOKUP_VENDOR, p->vendor_id, p->device_id));
			print_shell_escaped(pci_lookup_name(pacc, devbuf, sizeof(devbuf), PCI_LOOKUP_DEVICE, p->vendor_id, p->device_id));
			if (c = get_conf_byte(d, PCI_REVISION_ID))
	printf(" -r%02x", c);
			if (c = get_conf_byte(d, PCI_CLASS_PROG))
	printf(" -p%02x", c);
			if (sv_id && sv_id != 0xffff)
	{
		print_shell_escaped(pci_lookup_name(pacc, svbuf, sizeof(svbuf), PCI_LOOKUP_SUBSYSTEM | PCI_LOOKUP_VENDOR, sv_id));
		print_shell_escaped(pci_lookup_name(pacc, sdbuf, sizeof(sdbuf), PCI_LOOKUP_SUBSYSTEM | PCI_LOOKUP_DEVICE, p->vendor_id, p->device_id, sv_id, sd_id));
	}
			else
	printf(" \"\" \"\"");
			putchar('\n');
		}
}

/*** Main show function ***/
void show_wx_info(struct device *d)
{
	struct pci_dev *pdev = d->dev;
	int i = 0;
	word sv_id, sd_id, v_id, d_id;
	int nic_type = 0;
	u32 option_rom = 0, version = 0, version_c;
	char *name = NULL, *oprom_arch = NULL;
	u8 wol = 0, ncsi = 0, chip_v = 0;
	struct card_items items;
	u32 cab0_0, flash0;
	u32 RGMII_mod;
	u32 flash_bypass;
	u32 oem_en, tmp, oem_v1, oem_v2, oem_v3, oem_v4, oem_v5;
	u16 status = 0;

	// Check PCIe Memory Space Enable
	status = pci_read_byte(pdev, 0x4);
	pci_write_byte(pdev, 0x4, (status | 0x2)); // set "Memory Space Enable" field
	cab0_0 = read_reg(pdev, 0x10000);
	flash0 = flash_read_dword(pdev, 0x0);
	flash_bypass = read_reg(pdev, 0x1010C) & 80000000;
	v_id = pdev->vendor_id;
	d_id = pdev->device_id;
	get_subid(d, &sv_id, &sd_id);
	nic_type = check_nic_type(pdev); 
	option_rom = flash_read_dword(pdev, 0xa000a) & 0xffff;
	if (option_rom == 0xaa64)
		oprom_arch = "arm64";
	else if (option_rom == 0xffff)
		oprom_arch = "no";
	else
		oprom_arch = "arm64/x86";

	version = flash_read_dword(pdev, 0x13a);
	version_c = version & 0xf00ff;
	chip_v = check_chip_version(pdev);

	tmp = flash_read_dword(pdev, 0xff000);
	oem_en = cpu_to_le32(tmp);
	if ((oem_en & 0xffff) == 0x55aa)
		oem_en = 1;

	printf("\n");
	if (cab0_0 == 0x0 || (cab0_0 == 0xfffffff)) {
		printf("chip is malfunction, all LAN disabled or pcie link is down\n");
		printf("chip status: -1\n");
	} else {
		printf("chip status: ok\n");
	}
	if ((flash0 != 0x5aa51000) && 
		(flash0 != 0xffffffff) &&
		(flash0 != 0x5aa54000)) {
		printf("flash status: -1\n");
	} else if (flash0 == 0xffffffff && (cab0_0 == 0x0 || (cab0_0 == 0xfffffff))) {
		printf("flash: none\n");
	} else if (flash_bypass) {
		printf("flash bypass is set, status: -1\n");
	} else {
		printf("flash status: ok\n");
	}

	//0x10000 access
	printf("Cab0 0:\t%08x\n", cab0_0);
	//spi 0 access
	printf("Flash 0:\t%08x\n", flash0);
	//image version
	printf("fw version:\t%08x\n", version);
	//fw ready bit
	printf("fw init:\t%08x\n", read_reg(pdev, 0x10028));
	if (nic_type == EM) {
		//chip version
		printf("chip_v:\t%C\n", chip_v);
	}
	//wol nsci
	printf("wol:\t%s\n", 0x4000 & sd_id ? "enable":"disable");
	//chip version
	printf("ncsi:\t%s\n", 0x8000 & sd_id ? "enable":"disable");
	//platform
	printf("oprom arch:\t%s\n", oprom_arch);
	//image_name
	for (i = 0;i < 256; i++) {
		items = cards[i];
		if ((items.subs_id == sd_id) && (items.dev_id == d_id)) {
			name = items.name;
			break;
		}
	}

	// Re-program MAC address and SN to chip
	if (nic_type == SP) {
		if (pdev->func == 0) {
			txeq0_ctrl0 = 0;
			txeq0_ctrl1 = 0;
			txeq0_ctrl0 = flash_read_dword(pdev, SP0_TXEQ_CTRL0_OFFSET);
			txeq0_ctrl1 = flash_read_dword(pdev, SP0_TXEQ_CTRL1_OFFSET);
			//printf("lan0 - 0x18036 : %04x - 0x18037 : %04x\n", txeq0_ctrl0, txeq0_ctrl1);
			ffe_pre = txeq0_ctrl0 & 0x3f;
			ffe_main  = (txeq0_ctrl0 >> 8) &0x3f;
			ffe_post = txeq0_ctrl1 & 0x3f;
			//printf("lan0 : %d - %d - %d\n", ffe_main, ffe_pre, ffe_post);
			txeq1_ctrl0 = 0;
			txeq1_ctrl1 = 0;
		} else if (pdev->func == 1) {
			txeq1_ctrl0 = flash_read_dword(pdev, SP1_TXEQ_CTRL0_OFFSET);
			txeq1_ctrl1 = flash_read_dword(pdev, SP1_TXEQ_CTRL1_OFFSET);
			//printf("lan1 - 0x18036 : %04x - 0x18037 : %04x\n", txeq1_ctrl0, txeq1_ctrl1);
			ffe_pre = txeq1_ctrl0 & 0x3f;
			ffe_main  = (txeq1_ctrl0 >> 8) &0x3f;
			ffe_post = txeq1_ctrl1 & 0x3f;
			//printf("lan1 : main: %d - pre: %d - post: %d\n", ffe_main, ffe_pre, ffe_post);
		}
		printf("TX_EQ: \t%d - %d - %d\n", ffe_main, ffe_pre, ffe_post);
	} else if (nic_type == EM) {
		RGMII_mod = read_reg(pdev, 0x14404) & 0x80;
		if (RGMII_mod) {
			printf("phy : External\n");
		} else {
			printf("phy : Internal\n");
		}
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
			printf("_%x", version);
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
		if (version_c >= 0x2000b) {
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

	if (oem_en == 1) {
		oem_v1 = flash_read_dword(pdev, 0xff000);
		oem_v2 = flash_read_dword(pdev, 0xff004);
		oem_v3 = flash_read_dword(pdev, 0xff008);
		oem_v4 = flash_read_dword(pdev, 0xff00c);
		oem_v5 = flash_read_dword(pdev, 0xff010);
		printf("oem conf: [%08x]-[%08x]-[%08x]-[%08x]-[%08x]\n",
				 oem_v1, oem_v2, oem_v3, oem_v4, oem_v5);
	}
}

/*** Main show function ***/
void self_test1(struct device *d)
{
	struct pci_dev *pdev = d->dev;
	word sv_id, sd_id;
	word d_id;
	int nic_type = 0;
	u32 option_rom = 0;
	//u32 version = 0, version_c;
	//char *name = NULL;
	char *oprom_arch = NULL;
	u8 chip_v = 0;
	u32 cab0_0, flash0;
	u32 oem_en, tmp, oem_v1, oem_v2, oem_v3, oem_v4, oem_v5;
	u16 status = 0;
	u8 ret = 0;

	// Check PCIe Memory Space Enable
	status = pci_read_byte(pdev, 0x4);
	pci_write_byte(pdev, 0x4, (status | 0x2)); // set "Memory Space Enable" field

	cab0_0 = read_reg(pdev, 0x10000);
	flash0 = flash_read_dword(pdev, 0x0);

	d_id = pdev->device_id;
	get_subid(d, &sv_id, &sd_id);

	nic_type = check_nic_type(pdev); 

	option_rom = flash_read_dword(pdev, 0xa000a) & 0xffff;
	if (option_rom == 0xaa64)
		oprom_arch = "arm64";
	else if (option_rom == 0xffff)
		oprom_arch = "no";
	else
		oprom_arch = "arm64/x86";

	//version = flash_read_dword(pdev, 0x13a);
	//version_c = version & 0xf00ff;
	chip_v = check_chip_version(pdev);

	tmp = flash_read_dword(pdev, 0xff000);
	oem_en = cpu_to_le32(tmp);
	if ((oem_en & 0xffff) == 0x55aa)
		oem_en = 1;

	/*******************************************************/
	printf("========== 1.check pcie link status =======\n");
	show_machine(d);
	printf("========== 1.check pcie link status =======\n\n");

	/*******************************************************/

	printf("========== 2.check chip status ============\n");
	if (cab0_0 == 0x0 || (cab0_0 == 0xfffffff)) {
		printf("chip is malfunction, all LAN disabled or pcie link is down\n");
		printf("chip status: \t-1\n");
		ret = -1;
	} else
		printf("chip status: \tok\n");
	//0x10000 access
	printf("Cab0 0:\t%08x\n", cab0_0);
	if (nic_type == EM) {
		if (DEBUG_ON || debug_mode) printf("lan enable : %d-%d-%d-%d\n", 
					(int)((cab0_0 & BIT(28)) >> 28),
					(int)((cab0_0 & BIT(29)) >> 29),
					(int)((cab0_0 & BIT(30)) >> 30),
					(int)((cab0_0 & BIT(31)) >> 31));
	} else if (nic_type == SP) {
		printf("lan enable : %d-%d\n", (int)((cab0_0 & BIT(30)) >> 30),
						(int)((cab0_0 & BIT(31)) >> 31));
	} else
		ret = -1;
	if (nic_type == EM) {
		//chip version
		printf("chip_v:\t%C\n", chip_v);
	}
	printf("========== 2.check chip status ============\n");
	printf("\n");

	/*******************************************************/

	//spi 0 access
	printf("========== 3.check flash status ===========\n");
	if ((flash0 != 0x5aa51000) && 
		(flash0 != 0xffffffff) &&
		(flash0 != 0x5aa54000)) {
		printf("flash status: \t-1\n");
		ret = -1;
	} else if (flash0 == 0xffffffff && (cab0_0 == 0x0 || (cab0_0 == 0xfffffff))) {
		printf("flash: \tNA\n");
		ret = -1;
	} else
		printf("flash status: \tok\n");
	printf("Flash 0:\t%08x\n", flash0);
	printf("========== 3.check flash status ===========\n\n");
	/*******************************************************/

	printf("========== 4.check fw status ==============\n");
	//image version
	//printf("fw version:\t%08x\n", version);
	//fw ready bit
	printf("fw init:\t%08x\n", read_reg(pdev, 0x10028));
	printf("========== 4.check fw status ==============\n\n");
	/*******************************************************/

	printf("========== 5.check mbox status ============\n");
	//image version

	printf("========== 5.check mbox status ============\n");
	printf("\n");
	/*******************************************************/

	printf("========== 6.check function ===============\n");
	//wol nsci
	printf("wol:\t%s\n", 0x4000 & sd_id ? "enable":"disable");
	//chip version
	printf("ncsi:\t%s\n", 0x8000 & sd_id ? "enable":"disable");
	if (oem_en == 1) {
		oem_v1 = flash_read_dword(pdev, 0xff000);
		oem_v2 = flash_read_dword(pdev, 0xff004);
		oem_v3 = flash_read_dword(pdev, 0xff008);
		oem_v4 = flash_read_dword(pdev, 0xff00c);
		oem_v5 = flash_read_dword(pdev, 0xff010);
		printf("oem conf: [%08x]-[%08x]-[%08x]-[%08x]-[%08x]\n",
				 oem_v1, oem_v2, oem_v3, oem_v4, oem_v5);
	}
	printf("========== 6.check function ===============\n\n");
	/*******************************************************/

	printf("========== 7.check mbox status ============\n");
	printf("========== 7.check mbox status ============\n\n");
	/*******************************************************/

	printf("========== 8.check option rom =============\n");
	printf("oprom arch:\t%s\n", oprom_arch);
	printf("========== 8.check option rom =============\n\n");
	/*******************************************************/

	printf("========== 9.check sw(driver) status ======\n");


	printf("========== 9.check sw(driver) status ======\n\n");
	/*******************************************************/


	printf("========== 10.check phy status ============\n");

	printf("========== 10.check phy status ============\n\n");

	/*******************************************************/
}

/*** Main show function ***/
u32 self_test(struct device *d)
{
	struct pci_dev *pdev = d->dev;
	word sv_id, sd_id;
	word d_id;
	int nic_type = 0;
	//u32 version = 0, version_c;
	u32 cab0_0, flash0;
	u16 status = 0;
	u8 ret = 0;

	u8 chip_s = 0;
	u8 pcie_s = 0;
	u8 flash_s = 0;
	u8 fw_s = 0;
	u8 mbox_s = 0;
	//u8 func_s = 0;
	//u8 pxe_s = 0;
	u8 sw_s = 0;
	u8 phy_s = 0;
	char buf[1024];
	const char *driver;
	int mac[2];

	// Check PCIe Memory Space Enable
	status = pci_read_byte(pdev, 0x4);
	pci_write_byte(pdev, 0x4, (status | 0x2)); // set "Memory Space Enable" field

	cab0_0 = read_reg(pdev, 0x10000);
	flash0 = flash_read_dword(pdev, 0x0);

	if (pci_read_word(pdev, 0x0) != 0xffff)
		pcie_s = 1;

	d_id = pdev->device_id;
	get_subid(d, &sv_id, &sd_id);

	nic_type = check_nic_type(pdev);

	/*********************show id**********************************/
	show_machine(d);

	if (nic_type == EM) {
		// Re-program MAC address and SN to chip
		mac[0]  = flash_read_dword(pdev, 0x8000 * (pdev->func) + MAC_ADDR0_WORD0_OFFSET_1G);
		mac[1]  = flash_read_dword(pdev, 0x8000 * (pdev->func) + MAC_ADDR0_WORD0_OFFSET_1G + 8);
		printf(" MAC : 0x%04x%08x\n",(mac[1] & 0xffff), mac[0]);
	} else if (nic_type == SP) {
		mac[0]  = flash_read_dword(pdev, 0x10000 * (pdev->func) + MAC_ADDR0_WORD0_OFFSET);
		mac[1]  = flash_read_dword(pdev, 0x10000 * (pdev->func) + MAC_ADDR0_WORD0_OFFSET + 8);
		printf(" MAC : 0x%04x%08x\n",(mac[1] & 0xffff), mac[0]);

	}

	/*********************chip status**********************************/
	if (cab0_0 == 0x0 || (cab0_0 == 0xfffffff)) {
		if (DEBUG_ON || debug_mode) printf("chip is malfunction, all LAN disabled or pcie link is down\n");
		if (DEBUG_ON || debug_mode) printf("chip status: \t-1\n");
		chip_s = -1;
	} else {
		if (DEBUG_ON || debug_mode) printf("chip status: \tok\n");
		chip_s = 1;
	}
	//0x10000 access
	if (DEBUG_ON || debug_mode) printf("Cab0 0:\t%08x\n", cab0_0);
	if (nic_type == EM) {
		if (DEBUG_ON || debug_mode) printf("lan enable : %d-%d-%d-%d\n", 
					(int)((cab0_0 & BIT(28)) >> 28),
					(int)((cab0_0 & BIT(29)) >> 29),
					(int)((cab0_0 & BIT(30)) >> 30),
					(int)((cab0_0 & BIT(31)) >> 31));
	} else if (nic_type == SP) {
		if (DEBUG_ON || debug_mode) printf("lan enable : %d-%d\n", (int)((cab0_0 & BIT(30)) >> 30),
						(int)((cab0_0 & BIT(31)) >> 31));
	} else
		chip_s = -1;

	/***********************flash status********************************/
	if ((flash0 != 0x5aa51000) && 
		(flash0 != 0xffffffff) &&
		(flash0 != 0x5aa54000)) {
		if (DEBUG_ON || debug_mode) printf("flash status: \t-1\n");
		flash_s = -1;
	} else if (flash0 == 0xffffffff && (cab0_0 == 0x0 || (cab0_0 == 0xfffffff))) {
		if (DEBUG_ON || debug_mode) printf("flash: \tNA\n");
		flash_s = -1;
	} else {
		if (DEBUG_ON || debug_mode) printf("flash status: \tok\n");
		flash_s = 1;
	}

	/***********************fw init********************************/
	//fw ready bit
	status = read_reg(pdev, 0x10028);
	if (status & 0x1 == 0x1)
		fw_s = 1;

	/***********************sw init********************************/
	if (driver = find_driver(d, buf))
		sw_s = 1;

	/***********************mbox init********************************/
	if (nic_type == EM) {
		if (emnic_eepromcheck_cap(pdev) == 0)
			mbox_s = 1;
	} else if (nic_type == SP) {
		if ((u32)emnic_flash_read_cab(pdev, 0x0, 0x0) == cab0_0)
			mbox_s = 1;
	}

	if (nic_type == EM) {
		if (emnic_check_internal_phy_id(pdev) == 0)
			phy_s = 1;
	} else if (nic_type == SP) {
		phy_s = 1;
	}
	ret = pcie_s & chip_s & flash_s & fw_s & mbox_s;
	printf(" 1.check pcie \tstatus : \t\t[%s]\n", pcie_s ? "PASS":"FAIL");
	/*******************************************************/
	printf(" 2.check chip \tstatus : \t\t[%s]\n", chip_s ? "PASS":"FAIL");
	/*******************************************************/
	printf(" 3.check flash \tstatus : \t\t[%s]\n", flash_s ? "PASS":"FAIL");
	/*******************************************************/
	printf(" 4.check fw \tstatus : \t\t[%s]\n", fw_s ? "PASS":"FAIL");
	/*******************************************************/
	printf(" 5.check mbox \tstatus : \t\t[%s]\n", mbox_s ? "PASS":"FAIL");
	/*******************************************************/
	//printf(" 6.check function : %s\n", func_s ? "PASS":"FAIL");
	/*******************************************************/
	//printf(" 7.check option rom : %s\n", pxe_s ? "PASS":"FAIL");
	if (test_lvl >= 1) {
		/*******************************************************/
		printf(" 6.check sw \tstatus : \t\t[%s]\n", sw_s ? "PASS":"FAIL");
		/*******************************************************/
		printf(" 7.check phy \tstatus : \t\t[%s]\n", phy_s ? "PASS":"FAIL");
		ret &= sw_s & phy_s;
	}
	printf("\n");
	printf(" Self-test result : \t\t\t[%s]\n", ret ? "PASS":"FAIL");
	/*******************************************************/
	printf("\n");
	return !ret;
}

void show_device(struct device *d)
{
	if (opt_self_test)
		self_test(d);
	else {
	if (opt_machine)
		show_machine(d);
	else {
		if (verbose)
			show_verbose(d);
		else
			show_terse(d);
		if (opt_kernel || verbose)
			show_kernel(d);
	}
	if (opt_hex)
		show_hex_dump(d);
	if (opt_wx_info)
		show_wx_info(d);

	}
	if (verbose || opt_hex)
		putchar('\n');
}

static void show(void)
{
	struct device *d;

	for (d=first_dev; d; d=d->next) {
		if (d->dev->vendor_id == VENDOR_ID || d->dev->vendor_id == WX_VENDOR_ID)
			show_device(d);
	}
}

/* Main */
int show_wx_nic_info(int argc, char **argv)
{
	int i;
	char *msg;

	//work around
	argv++;
	argc--;

	if (argc == 2 && !strcmp(argv[1], "--version"))
	{
		puts("wxtool show version " PCIUTILS_VERSION);
		return 0;
	}

	if (argc >= 2 && !strcmp(argv[1], "--help"))
	{
		fprintf(stderr, help_msg);
		return 0;
	}

	pacc = pci_alloc();
	pacc->error = die;
	pci_filter_init(pacc, &filter);

	while ((i = getopt(argc, argv, options)) != -1)
	switch (i)
	{
	case 'n':
		pacc->numeric_ids++;
		break;
	case 'v':
		verbose++;
		break;
	case 'i':
		opt_wx_info++;
		break;
	case 'T':
		pacc->numeric_ids++;
		opt_machine++;
		///verbose++;
		//opt_wx_info++;
		//opt_kernel++;
		opt_self_test++;

		if(strlen(optarg) != 1) {
			printf("invalid arguments for option -T\n");
			goto clean;
		}
		if (*optarg == '0') {
			test_lvl = 0;
		} else if (*optarg == '1') {
			test_lvl = 1;
		} else if (*optarg == '2') {
			test_lvl = 2;
		} else if (*optarg == '3') {
			test_lvl = 3;
		} else {
			printf("invalid arguments for option -T\n");
			goto clean;
		}
		break;
	case 'b':
		pacc->buscentric = 1;
		break;
	case 's':
		if (msg = pci_filter_parse_slot(&filter, optarg))
			die("-s: %s", msg);
		break;
	case 'd':
		if (msg = pci_filter_parse_id(&filter, optarg))
			die("-d: %s", msg);
		break;
	case 'x':
		opt_hex++;
		break;
	case 't':
		opt_tree++;
		break;
	case 'm':
		opt_machine++;
		break;
	case 'p':
		opt_pcimap = optarg;
		break;
#ifdef PCI_OS_LINUX
	case 'k':
		opt_kernel++;
		break;
#endif
	case 'M':
		opt_map_mode++;
		break;
	case 'D':
		opt_domains = 2;
		break;
#ifdef PCI_USE_DNS
	case 'q':
		opt_query_dns++;
		break;
	case 'Q':
		opt_query_all = 1;
		break;
#else
	case 'q':
	case 'Q':
		die("DNS queries are not available in this version");
#endif
	default:
		if (parse_generic_option(i, pacc, optarg))
			break;
bad:
	fprintf(stderr, help_msg, pacc->id_file_name);
	return 1;
	}
	if (optind < argc)
		goto bad;

	if (opt_query_dns)
	{
		pacc->id_lookup_mode |= PCI_LOOKUP_NETWORK;
		if (opt_query_dns > 1)
		pacc->id_lookup_mode |= PCI_LOOKUP_REFRESH_CACHE;
	}
	if (opt_query_all)
		pacc->id_lookup_mode |= PCI_LOOKUP_NETWORK | PCI_LOOKUP_SKIP_LOCAL;

	pci_init(pacc);
	if (opt_map_mode)
		map_the_bus();
	else {
		scan_devices();
		sort_them();
		if (opt_tree)
			show_forest();
		else
			show();
	}
clean:
	show_kernel_cleanup();
	pci_cleanup(pacc);

	return (seen_errors ? 2 : 0);
}

