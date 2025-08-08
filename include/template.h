#ifndef TEMPLATE_H
#define TEMPLATE_H

typedef struct {
    const char* key;
    const char* value;
} TemplateVar;

char* render_template(const char* source, TemplateVar* vars, int var_count);

#endif
