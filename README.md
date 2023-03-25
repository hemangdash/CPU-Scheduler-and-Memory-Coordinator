# CPU Scheduler

## Project Instruction Updates:

1. Complete the function CPUScheduler() in vcpu_scheduler.c
2. If you are adding extra files, make sure to modify Makefile accordingly.
3. Compile the code using the command `make all`
4. You can run the code by `./vcpu_scheduler <interval>`
5. While submitting, write your algorithm and logic in this Readme.

## Algorithm:

1. First, we get all the active running Virtual Machines (VM's) within `qemu:///system`. We do so by using the function `virConnectListAllDomains()`.
2. We allocate memory for collecting vCPU and pCPU statistics. We also find the number of pCPU's present on the host node.
3. Then, we collect the vCPU statistics of active running VM's. These statistics include the maximum number of vCPU's on a running VM, the identifying number of the vCPU, the CPU time used in nanoseconds and the identifying number of the pCPU that the vCPU is associated to.
4. If there are no new VM's,
    1. We calculate the vCPU usage for all the active running VM's. For this, we first get the vCPU usage in percentage. The formula for doing so is $$\frac{\text{Current vCPU time} - \text{Previous vCPU time}}{\text{Total time in interval}} * 100$$ where total time in interval is expressed in seconds. The pCPU usage is collected as the sum of all vCPU usages for the vCPU's pinned to that pCPU.
    2. We then find the highest usage and lowest usage pCPU.
    3. Finally, we reallocate resources away from the highest usage pCPU to the lowest usage pCPU if the difference between the highest usage and the lowest usage is greater than the accepted difference (20, found by trial and error).
5. If there are new VM's, we allocate some extra space for saving the previous statistics of vCPU's.
6. In the end, we save the current state of VM's to the previous variables. The state includes number of running VM's and the CPU time used. Finally, we free the memories which stored the pCPU and vCPU statistics and proceed with the next iteration of this algorithm.

# Memory Coordinator


## Project Instruction Updates:

1. Complete the function MemoryScheduler() in memory_coordinator.c
2. If you are adding extra files, make sure to modify Makefile accordingly.
3. Compile the code using the command `make all`
4. You can run the code by `./memory_coordinator <interval>`

### Notes:

1. Make sure that the VMs and the host has sufficient memory after any release of memory.
2. Memory should be released gradually. For example, if VM has 300 MB of memory, do not release 200 MB of memory at one-go.
3. Domain should not release memory if it has less than or equal to 100MB of unused memory.
4. Host should not release memory if it has less than or equal to 200MB of unused memory.
5. While submitting, write your algorithm and logic in this Readme.

### Algorithm:

1. We define the starvation threshold (the available memory threshold below which a domain can be considered as starved of memory) to be 150 MB. We define the waste threshod (the available memory threshold above which a domain can be considered to be wasting memory) as 300 MB.
2. First, we get a list of all active domains using `virConnectListAllDomains()`.
3. Then, we find the two relevant domains to our algorithm: the domain which is the most wasteful and the domain which is the most starved. We a for loop to find the relevant domains.
    1. We collect all the memory statistics of each domain.
    2. If the amount of usable memory as seen by the domain is more than the memory of the domain currently stored as the most wasteful, we replace the most wasteful domain.
    3. If the amount of usable memory as seen by the domain is less than the memory of the domain currently stored as the most starved, we replace the most starved domain.
4. If the most starved domain's memory is lesser than or equal to the starvation threshold,
    1. If the most wasteful domain's memory is greater than or equal to the waste threshod, the most wasteful domain's memory is halved and this removed memory is set to the starved domain.
        1. We set the wasteful domain's memory to be $\text{wasteful.memory} - \text{wasteful.memory} / 2$.
        2. We set the starved domain's memory to be $\text{starved.memory} + \text{wasteful.memory} / 2$.
    2. Else, if there is not much waste and a domain is critical with less memory than the starvation threshold, we must assgin memory from the hypervisor until the starved domain is not starved. We set the starved domain's memory to be $\text{starved.memory} + \text{waste threshold}$.
5. Else, if the most wasteful domain's memory is greater than or equal to the waste threshold, this means that no domain needs more memory at this point and hence we give the memory back to the hypervisor. We set the wasteful domain's memory to be $\text{wasteful.memory} - \text{waste threshold}$.

