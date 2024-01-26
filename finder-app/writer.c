#include <stdio.h>
#include <syslog.h>

#define ARG_CNT 3

int main(int argc, char* argv[])
{

    openlog(NULL, LOG_PID, LOG_USER);

    if(argc != ARG_CNT) {
        syslog(LOG_DEBUG, "Invalid number of arguments.");
        return 1;
    }

    const char* file_path = argv[1];
    if (file_path[0] == '\0') {
        syslog(LOG_DEBUG, "file_path is empty.");
        return 1;
    }

    const char* file_content = argv[2];
    if (file_content[0] == '\0') {
        syslog(LOG_DEBUG, "file_content is empty.");
        return 1;
    }

    FILE *f = fopen(file_path, "w");
    if (f == NULL) {
        syslog(LOG_ERR, "Unable to create the file %s.", file_path);
        fclose(f);
        return 1;
    }

    // Write some text to the file
    fprintf(f, "%s", file_content);
    fclose(f);

    return 0;
}
