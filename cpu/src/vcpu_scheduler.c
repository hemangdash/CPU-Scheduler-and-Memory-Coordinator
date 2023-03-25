#include<stdio.h>
#include<stdlib.h>
#include<libvirt/libvirt.h>
#include<math.h>
#include<string.h>
#include<unistd.h>
#include<limits.h>
#include<signal.h>
#include <float.h>
#define MIN(a,b) ((a)<(b)?a:b)
#define MAX(a,b) ((a)>(b)?a:b)

int is_exit = 0; // DO NOT MODIFY THIS VARIABLE


void CPUScheduler(virConnectPtr conn,int interval);

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

	// Get the total number of pCpus in the host
	signal(SIGINT, signal_callback_handler);

	while(!is_exit)
	// Run the CpuScheduler function that checks the CPU Usage and sets the pin at an interval of "interval" seconds
	{
		CPUScheduler(conn, interval);
		sleep(interval);
	}

	// Closing the connection
	virConnectClose(conn);
	return 0;
}

struct VcpuStats {
	virDomainPtr vm;
	int maxVcpuCount;
	unsigned int *number; // virtual CPU number
	unsigned long long *vCPUTime; // CPU time used, in nanoseconds
	int *cpu; // pCPU number
	double *vCPUUsage;
};

int nRunningVMsPrev = 0;
struct VcpuStats *vCPUStatsPrev;

/* COMPLETE THE IMPLEMENTATION */
void CPUScheduler(virConnectPtr conn, int interval)
{
	// Getting all active running VM's within "qemu:///system"
	virDomainPtr *runningVMs;
	int nRunningVMs = virConnectListAllDomains(
		conn,
		&runningVMs,
		VIR_CONNECT_LIST_DOMAINS_ACTIVE | VIR_CONNECT_LIST_DOMAINS_RUNNING
	);

	struct VcpuStats *vCPUStats = calloc(nRunningVMs, sizeof(struct VcpuStats));
	int ncpuCount = virNodeGetCPUMap(conn, NULL, NULL, 0);
	double *cpuStats = calloc(ncpuCount, sizeof(double));

	// Collecting vCPU statistics of active running VM's
	for (int i = 0; i < nRunningVMs; i++) {
		vCPUStats[i].vm = runningVMs[i];

		vCPUStats[i].maxVcpuCount = virDomainGetMaxVcpus(runningVMs[i]);

		virVcpuInfoPtr vCPUInfo = calloc(vCPUStats[i].maxVcpuCount, sizeof(virVcpuInfo));
		virDomainGetVcpus(runningVMs[i], vCPUInfo, vCPUStats[i].maxVcpuCount, NULL, 0);
		vCPUStats[i].number = calloc(vCPUStats[i].maxVcpuCount, sizeof(int));
		vCPUStats[i].vCPUTime = calloc(vCPUStats[i].maxVcpuCount, sizeof(unsigned long long));
		vCPUStats[i].cpu = calloc(vCPUStats[i].maxVcpuCount, sizeof(unsigned int));

		for (int j = 0; j < vCPUStats[i].maxVcpuCount; j++) {
			vCPUStats[i].number[j] = vCPUInfo[j].number;
			vCPUStats[i].vCPUTime[j] = vCPUInfo[j].cpuTime;
			vCPUStats[i].cpu[j] = vCPUInfo[j].cpu;
		}
		free(vCPUInfo);
	}

	// No new VM's
	if (nRunningVMs == nRunningVMsPrev) {
		// Calculating vCPU usage for active running VM's
		for (int i = 0; i < nRunningVMs; i++) {
			vCPUStats[i].vCPUUsage = calloc(vCPUStats[i].maxVcpuCount, sizeof(double));

			for (int j = 0; j < vCPUStats[i].maxVcpuCount; j++) {
				// Getting vCPU usage in percentage
				double totalTime = (double) (interval * 1000000000);
				double vCPUTime = (double) (vCPUStats[i].vCPUTime[j] - vCPUStatsPrev[i].vCPUTime[j]);
				double vCPUUsage = (vCPUTime / totalTime) * 100.0;
				vCPUStats[i].vCPUUsage[j] = vCPUUsage;

				// Collecting pCPU statistics
				cpuStats[vCPUStats[i].cpu[j]] += vCPUStats[i].vCPUUsage[j];
			}
		}

		// Find highest usage and lowest usage CPU
		double highestUsage = DBL_MIN;
		double lowestUsage = DBL_MAX;
		int highestUsageCpu;
		int lowestUsageCpu;
		double acceptedDiff = 20;

		for (int i = 0; i < ncpuCount; i++) {
			if (cpuStats[i] > highestUsage) {
				highestUsage = cpuStats[i];
				highestUsageCpu = i;
			}
			if (cpuStats[i] < lowestUsage) {
				lowestUsage = cpuStats[i];
				lowestUsageCpu = i;
			}
		}

		// Re-allocate resources away from highest usage pCPU to lowest usage pCPU
		if ((highestUsage - lowestUsage) > acceptedDiff) {
			int cpuMapLen = VIR_CPU_MAPLEN(ncpuCount);
			unsigned char cpuMap = 0x1 << lowestUsageCpu;
			double lowestVcpuUsage = DBL_MAX;
			int lowestUsageVcpu = 0;
			virDomainPtr lowestUsageVcpuVM;

			for (int i = 0; i < nRunningVMs; i++) {
				for (int j = 0; j < vCPUStats[i].maxVcpuCount; j++) {
					if (vCPUStats[i].cpu[j] == highestUsageCpu) {
						if (vCPUStats[i].vCPUUsage[j] < lowestVcpuUsage) {
							lowestVcpuUsage = vCPUStats[i].vCPUUsage[j];
							lowestUsageVcpu = j;
							lowestUsageVcpuVM = vCPUStats[i].vm;
						}
					}
				}
			}

			virDomainPinVcpu(lowestUsageVcpuVM, lowestUsageVcpu, &cpuMap, cpuMapLen);
		}
	} else {
		// New VM's
		vCPUStatsPrev = calloc(nRunningVMs, sizeof(struct VcpuStats));

		for (int i = 0; i < nRunningVMs; i++) {
			int maxVcpuCount = vCPUStats[i].maxVcpuCount;
			vCPUStatsPrev[i].vCPUTime = calloc(maxVcpuCount, sizeof(unsigned long long));
		}
	}

	// Saving current state to previous variables
	nRunningVMsPrev = nRunningVMs;

	for (int i = 0; i < nRunningVMs; i++) {
		for (int j = 0; j < vCPUStats[i].maxVcpuCount; j++) {
			vCPUStatsPrev[i].vCPUTime[j] = vCPUStats[i].vCPUTime[j];
		}
	}

	memset(cpuStats, 0, ncpuCount * sizeof(double));
	for (int i = 0; i < nRunningVMs; i++) {
		free(vCPUStats[i].number);
		free(vCPUStats[i].vCPUTime);
		free(vCPUStats[i].cpu);
		free(vCPUStats[i].vCPUUsage);
	}
	free(vCPUStats);
	free(cpuStats);
}