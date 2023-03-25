
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