#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include "../src/interpreter/interpreter.h"

int main(int argc, char **argv) {
    char buf[1024];

    while (1) {
        printf("> ");
        fgets(buf, 1023, stdin);
        if (buf[strlen(buf) - 1] == '\n')
            buf[strlen(buf) - 1] = '\0';

        printf("Input: '%s'\n", buf);
        printf("\n");

        intp_value_t val;
        memset(&val, 0, sizeof(intp_value_t));

        uint32_t pos = 0;
        intp_status_t status = intp_evaluate(buf, &pos, &val);
        if (status.code != INTP_STATUS_OK) {
            char err[512];
            intp_format_error(buf, status, err, 512);
            printf("%s\n", err);
            printf("~~~~~~~~~~~~~~~~~~~~\n");
            continue;
        }

        printf("Type: %s (%dbit)\n", intp_data_type_to_string(val.type), val.size * 8);
        switch (val.type) {
            case DATA_TYPE_SIGNED:   printf("Result: %d\n", val.data.int32); break;
            case DATA_TYPE_UNSIGNED: printf("Result: %u\n", val.data.uint32); break;
            case DATA_TYPE_FLOAT:    printf("Result: %f\n", val.data.fl32); break;
            case DATA_TYPE_RAW:
                printf("Result: ");
                for (int i = 0; i < val.size; i++) {
                    printf("%02X ", val.data.raw[i]);
                }
                printf("\n");
                break;
            default: break;
        }

        printf("~~~~~~~~~~~~~~~~~~~~\n");
    }

    return 0;
}
