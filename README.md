# Communication Detection in Shared Memory (CDSM)

CDSM is a Linux kernel module to detect communication of parallel applications in shared memory, and use that information to migrate threads between cores based on their affinity.

## Installation
Compile and insert the CDSM kernel module:

     $ make
     $ make install
     
## Execution

Rename the binary that you want CDSM to handle so that it ends with ```.x```:

     $ ./app.x
     
Everything else is automatic.

## Publications

The main publications regarding CDSM are:

- Matthias Diener, Eduardo H. M. Cruz, Philippe O. A. Navaux, Anselm Busse, Hans-Ulrich Heiß. “Communication-Aware Process and Thread Mapping Using Online Communication Detection.”, Parallel Computing (PARCO), 2015. http://dx.doi.org/10.1016/j.parco.2015.01.005
- Matthias Diener, Eduardo H. M. Cruz, Philippe O. A. Navaux. “Communication-Based Mapping using Shared Pages.” International Parallel & Distributed Processing Symposium (IPDPS), 2013. http://dx.doi.org/10.1109/IPDPS.2013.57
