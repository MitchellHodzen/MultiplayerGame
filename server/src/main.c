#include <stdio.h>
#include <enet/enet.h>

int main(int argc, char* args[])
{
    if (enet_initialize () != 0)
    {
        fprintf (stderr, "An error occurred while initializing ENet.\n");
        return 1;
    }
    printf("hello world!\n");

    enet_deinitialize();
    return 0;
}