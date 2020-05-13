#include "stems.h"
#include <sys/time.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>
#include <mysql/mysql.h>

int main()
{		
    int bodyLength = 0;
    char buf[256];
    char *name;
    int time;
    float value;

    bodyLength = atoi(getenv("CONTENT_LENGTH"));
    if (bodyLength > 0) {
        read(STDIN_FILENO, buf, bodyLength);
    }

    strtok(buf, "=");
    name = strtok(NULL, "&");
    strtok(NULL, "=");
    time = atoi(strtok(NULL, "&"));
    strtok(NULL, "=");
    value = atof(strtok(NULL, "&"));

    fprintf(stderr, "WARNING!! [sensor %s] : %d시에 %.2f값 발생\n", name, time, value);
}