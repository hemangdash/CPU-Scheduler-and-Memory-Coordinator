#include<stdio.h>
#include<stdlib.h>
#include<libvirt/libvirt.h>
#include<math.h>
#include<string.h>
#include<unistd.h>
#include<limits.h>
#include<signal.h>
#define MIN(a,b) ((a)<(b)?a:b)
#define MAX(a,b) ((a)>(b)?a:b)

int is_exit = 0; // DO NOT MODIFY THE VARIABLE

void MemoryScheduler(virConnectPtr conn,int interval);

/*
DO NOT CHANGE THE FOLLOWING FUNCTION
*/
void signal_callback_handler()
{
	printf("Caught Signal");
	is_exit = 1;
}

/*
DO NOT CHANGE THE FOLLOWING FUNCTION
*/
int main(int argc, char *argv[])
{
	virConnectPtr conn;

	if(argc != 2)
	{
		printf("Incorrect number of arguments\n");
		return 0;
	}

	// Gets the interval passes as a command line argument and sets it as the STATS_PERIOD for collection of balloon memory statistics of the domains
	int interval = atoi(argv[1]);
	
	conn = virConnectOpen("qemu:///system");
	if(conn == NULL)
	{
		fprintf(stderr, "Failed to open connection\n");
		return 1;
	}

	signal(SIGINT, signal_callback_handler);

	while(!is_exit)
	{
		// Calls the MemoryScheduler function after every 'interval' seconds
		MemoryScheduler(conn, interval);
		sleep(interval);
	}

	// Close the connection
	virConnectClose(conn);
	return 0;
}

/*
COMPLETE THE IMPLEMENTATION
*/
static const int STARVATION_THRESHOLD = 150 * 1024;

static const int WASTE_THRESHOLD = 300 * 1024;

typedef struct HostMemory {
	virDomainPtr host;
	long memory;
} HostMemory_t;

typedef struct HostsList {
	virDomainPtr *hosts; // pointer to array of libvirt hosts
	int count; // number of hosts in the *hosts array
} HostsList_t;

HostsList_t activeHosts(virConnectPtr conn) {
	unsigned int fl = VIR_CONNECT_LIST_DOMAINS_ACTIVE | VIR_CONNECT_LIST_DOMAINS_RUNNING;
	virDomainPtr *hosts;

	int num_hosts = virConnectListAllDomains(conn, &hosts, fl);
	if (num_hosts <= 0) {
		fprintf(stderr, "activeHosts(): Failed to list all hosts\n");
		exit(1);
	}

	HostsList_t *list = (HostsList_t *) malloc(sizeof(HostsList_t));

	list->count = num_hosts;
	list->hosts = hosts;

	return *list;
}

void MemoryScheduler(virConnectPtr conn, int interval)
{
	HostsList_t list = activeHosts(conn);
	if (list.count <= 0) {
		fprintf(stderr, "MemoryScheduler(): No active hosts available\n");
		exit(1);
	}

	HostMemory_t mostWasteful;
	HostMemory_t mostStarved;

	mostWasteful.memory = 0;
	mostStarved.memory = 0;

	for (int i = 0; i < list.count; i++) {
		virDomainMemoryStatStruct memStats[VIR_DOMAIN_MEMORY_STAT_NR];
		unsigned int nr_stats;
		unsigned int fl = VIR_DOMAIN_AFFECT_CURRENT;

		unsigned int period_enabled = virDomainSetMemoryStatsPeriod(list.hosts[i], 1, fl);
		if (period_enabled < 0) {
			fprintf(stderr, "MemoryScheduler(): Could not change the period of collecting balloon\n");
			exit(1);
		}

		nr_stats = virDomainMemoryStats(list.hosts[i], memStats, VIR_DOMAIN_MEMORY_STAT_NR, 0);
		if (nr_stats == -1) {
			fprintf(stderr, "MemoryScheduler(): Could not collect host memory stats\n");
			exit(1);
		}

		if (memStats[VIR_DOMAIN_MEMORY_STAT_AVAILABLE].val > mostWasteful.memory) {
			mostWasteful.host = list.hosts[i];
			mostWasteful.memory = memStats[VIR_DOMAIN_MEMORY_STAT_AVAILABLE].val;
		}

		if (mostStarved.memory == 0 || memStats[VIR_DOMAIN_MEMORY_STAT_AVAILABLE].val < mostStarved.memory) {
			mostStarved.host = list.hosts[i];
			mostStarved.memory = memStats[VIR_DOMAIN_MEMORY_STAT_AVAILABLE].val;
		}
	}

	if (mostStarved.memory <= STARVATION_THRESHOLD) {
		if (mostWasteful.memory < WASTE_THRESHOLD) {
			virDomainSetMemory(mostStarved.host, mostStarved.memory + WASTE_THRESHOLD);
		} else {
			virDomainSetMemory(mostStarved.host, mostStarved.memory + mostWasteful.memory / 2);
			virDomainSetMemory(mostWasteful.host, mostWasteful.memory - mostWasteful.memory / 2);
		}
	} else if (mostWasteful.memory >= WASTE_THRESHOLD) {
		virDomainSetMemory(mostWasteful.host, mostWasteful.memory - WASTE_THRESHOLD);
	}
	sleep(interval);
}