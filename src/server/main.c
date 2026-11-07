#include "http_server.h"

int main(void)
{
    return http_server_run(8080) ? 0 : 1;
}

