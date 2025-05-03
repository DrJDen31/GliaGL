#include "glia.h"
#include "neuron.h"

int main()
{
    Glia *glia = new Glia();

    while (1)
    {
        glia->step();
    }
}