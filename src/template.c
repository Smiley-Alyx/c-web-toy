#include "template.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Simple {{var}} replacement. Returns malloc'ed string.
char* render_template(const char* source, TemplateVar* vars, int var_count) {
    size_t output_size = strlen(source) + 1;
    char* output = (char*)malloc(output_size);
    if (!output) return NULL;
    strcpy(output, source);

    for (int i = 0; i < var_count; i++) {
        char placeholder[128];
        snprintf(placeholder, sizeof(placeholder), "{{%s}}", vars[i].key);

        // Replace ALL occurrences of the placeholder (loop until no more matches)
        for (;;) {
            char* pos = strstr(output, placeholder);
            if (!pos) break;

            size_t pre_len = (size_t)(pos - output);
            size_t placeholder_len = strlen(placeholder);
            size_t value_len = strlen(vars[i].value);
            size_t rest_len = strlen(pos + placeholder_len);

            size_t new_len = pre_len + value_len + rest_len + 1;
            char* new_output = (char*)malloc(new_len);
            if (!new_output) { free(output); return NULL; }

            memcpy(new_output, output, pre_len);
            memcpy(new_output + pre_len, vars[i].value, value_len);
            memcpy(new_output + pre_len + value_len, pos + placeholder_len, rest_len + 1); // +1 for '\0'

            free(output);
            output = new_output;
        }
    }
    return output;
}

// Existing function: render_template() should already be here
char* render_template_file(const char* filename, TemplateVar* vars, int var_count) {
    char path[256];
    snprintf(path, sizeof(path), "templates/%s", filename);

    FILE* f = fopen(path, "rb");
    if (!f) return NULL;

    if (fseek(f, 0, SEEK_END) != 0) { fclose(f); return NULL; }
    long size = ftell(f);
    if (size < 0) { fclose(f); return NULL; }
    rewind(f);

    char* buffer = (char*)malloc((size_t)size + 1);
    if (!buffer) { fclose(f); return NULL; }

    size_t readn = fread(buffer, 1, (size_t)size, f);
    fclose(f);
    buffer[readn] = '\0';

    char* rendered = render_template(buffer, vars, var_count);
    free(buffer);
    return rendered;
}
