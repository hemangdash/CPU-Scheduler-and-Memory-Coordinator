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