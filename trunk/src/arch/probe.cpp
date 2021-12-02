#include "probe.h"
#include "intel.h"
#include "arm.h"

int arch_probe(void)
{
    if (arch_probed)
    {
        return 1;
    }

    arch_intel_probe();
    arch_arm_probe();

    arch_probed = 1;
    
    return 1;
}


int arch_probed = arch_probe();

