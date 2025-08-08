#include "template.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Simple {{var}} replacement. Returns malloc'ed string.
char* render_template(const char* source, TemplateVar* vars, int var_count) {
    size_t output_size = strlen(source) + 1;
    char* output = malloc(output_size);
    if (!output) return NULL;
    strcpy(output, source);

    for (int i = 0; i < var_count; i++) {
        const char* placeholder_fmt = "{{%s}}";
        char placeholder[128];
        snprintf(placeholder, sizeof(placeholder), placeholder_fmt, vars[i].key);

        char* pos = strstr(output, placeholder);
        if (!pos) continue;

        // Compute sizes
        size_t pre_len = pos - output;
        size_t placeholder_len = strlen(placeholder);
        size_t value_len = strlen(vars[i].value);
        size_t rest_len = strlen(pos + placeholder_len);

        // Allocate new output
        size_t new_len = pre_len + value_len + rest_len + 1;
        char* new_output = malloc(new_len);
        if (!new_output) {
            free(output);
            return NULL;
        }

        // Build new string
        strncpy(new_output, output, pre_len);
        strcpy(new_output + pre_len, vars[i].value);
        strcpy(new_output + pre_len + value_len, pos + placeholder_len);

        free(output);
        output = new_output;
    }

    return output;
}
