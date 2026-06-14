#ifndef COMOPNENT_LIFETIME
#define COMOPNENT_LIFETIME
#include <stdbool.h>

struct C_Lifetime
{
    float lifetimeS;
    bool completed;
    float _elapsed;
};

#endif /* COMOPNENT_LIFETIME */