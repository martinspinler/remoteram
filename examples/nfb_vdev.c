/* SPDX-License-Identifier: BSD-3-Clause
 */

/* INFO: run DPDK application with parameter like:
	./app/dpdk-testpmd --vdev=eth_vdev_nfb,dev=libnfb-ext-grpc.so:grpc:127.0.0.1:18083
*/

#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <linux/sockios.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <net/if.h>
#include <net/if_arp.h>
#include <netinet/ip.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/queue.h>
#include <sys/socket.h>
#include <unistd.h>

#include <rte_alarm.h>
#include <rte_bus.h>
#include <bus_vdev_driver.h>
#include <rte_common.h>
#include <rte_dev.h>
#include <rte_errno.h>
#include <rte_ethdev.h>
#include <rte_ether.h>
#include <rte_hypervisor.h>
#include <rte_kvargs.h>
#include <rte_log.h>
#include <rte_string_fns.h>

#include <dlfcn.h>

#include <ethdev_driver.h>

#include <ethdev_vdev.h>

#include <netcope/info.h>

#include "nfb.h"

#include <remoteram/remoteram.h>

#define VDEV_NFB_DRIVER net_vdev_nfb
#define VDEV_NFB_DRIVER_NAME RTE_STR(VDEV_NFB_DRIVER)
#define VDEV_NFB_DRIVER_NAME_LEN 12
#define VDEV_NFB_ARG_DEV "dev"
#define VDEV_NFB_ARG_MAC "mac"
#define VDEV_NFB_ARG_FORCE "force"
#define VDEV_NFB_ARG_IGNORE "ignore"

static remoteram_t rr;

int nfb_eth_dev_init(struct rte_eth_dev *dev, void *init_data);

static int
vdev_nfb_vdev_probe(struct rte_vdev_device *dev)
{
	static const char *const vdev_netvsc_arg[] = {
		VDEV_NFB_ARG_DEV,
		NULL,
	};

	int i;
	int ret = 0;
	int basename_len;
	char name[RTE_ETH_NAME_MAX_LEN];
	const char *path = NULL;

	struct nc_ifc_info *ifc;
	struct nfb_device *nfb_dev;
	struct rte_eth_dev *eth_dev;
	struct nfb_init_params params;
	struct pmd_internals *p;

	const char *vdev_name = rte_vdev_device_name(dev);
	const char *vdev_args = rte_vdev_device_args(dev);
	struct rte_kvargs *kvargs = rte_kvargs_parse(vdev_args ? vdev_args : "",
						     vdev_netvsc_arg);

	for (i = 0; i != (signed) kvargs->count; ++i) {
		const struct rte_kvargs_pair *pair = &kvargs->pairs[i];

		if (!strcmp(pair->key, VDEV_NFB_ARG_DEV))
			path = pair->value;
	}

	dev->device.devargs = NULL;

	/* FIXME: duplicate path string */
//	rte_kvargs_free(kvargs);

	if (path == NULL)
		return 0;

	dlopen("libremoteram.so", RTLD_NOW);
	rr = remoteram_create_server("127.0.0.1:51112", 1);

	strcpy(name, vdev_name);
	basename_len = strlen(name);

	nfb_dev = nfb_open(path);
	if (nfb_dev == NULL) {
		RTE_LOG(ERR, PMD, "nfb_open(): failed to open '%s'\n", path);
		return -EINVAL;
	}

	params.path = path;

	ret = nc_ifc_map_info_create_ordinary(nfb_dev, &params.map_info);

	params.nfb_id = 0; //comp_dev_info.nfb_id;
	for (i = 0; i < params.map_info.ifc_cnt; i++) {
		ifc = params.ifc_info = &params.map_info.ifc[i];

		/* Skip interfaces which doesn't belong to this PCI device */
		if (
				(ifc->flags & NC_IFC_INFO_FLAG_ACTIVE) == 0)
			continue;

		snprintf(name + basename_len, sizeof(name) - basename_len,
				"_nfb%d_eth%d", params.nfb_id, params.ifc_info->id);

		/* FIXME: replace NULL by function which sets numa node */
		ret = rte_eth_dev_create(&dev->device, name,
				sizeof(struct pmd_internals),
				NULL, NULL,
				nfb_eth_dev_init, &params);

		eth_dev = rte_eth_dev_get_by_name(name);
		if (eth_dev) {
			p = eth_dev->data->dev_private;
			p->eth_dev = eth_dev;
			p->pci_dev = NULL;
			//TAILQ_INSERT_TAIL(&nfb_eth_dev_list, p, eth_dev_list);
		}
	}

	if (ret) {
	}

	nc_map_info_destroy(&params.map_info);
	nfb_close(nfb_dev);

	return 0;
}

static int
vdev_nfb_vdev_remove(__rte_unused struct rte_vdev_device *dev)
{
	remoteram_destroy_server(rr);
	return 0;
}

/** Virtual device descriptor. */
static struct rte_vdev_driver vdev_nfb_vdev = {
	.probe = vdev_nfb_vdev_probe,
	.remove = vdev_nfb_vdev_remove,
};

RTE_PMD_REGISTER_VDEV(VDEV_NFB_DRIVER, vdev_nfb_vdev);
RTE_PMD_REGISTER_ALIAS(VDEV_NFB_DRIVER, eth_vdev_nfb);
RTE_PMD_REGISTER_PARAM_STRING(net_vdev_nfb,
		VDEV_NFB_ARG_DEV "=<string> ");
