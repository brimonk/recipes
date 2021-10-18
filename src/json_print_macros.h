#ifndef JSON_PRINT_MACROS_H
#define JSON_PRINT_MACROS_H

#define BEGIN()        (used += snprintf(s + used, size - used, "{"))
#define END()          (used += snprintf(s + used, size - used, "}"))
#define ADDSTR(k,v)    (used += snprintf(s + used, size - used, "\"%s\":\"%s\"", (k), (v)))
#define ADDCOM()       (used += snprintf(s + used, size - used, ","))
#define ADDNUM(k,v)    (used += snprintf(s + used, size - used, "\"%s\":%ld", (k), (long)(v)))
#define ARRBEG(k)      (used += snprintf(s + used, size - used, "\"%s\":[", (k)))
#define ARRVAL(fmt,v)  (used += snprintf(s + used, size - used, fmt, (v)))
#define ARREND()       (used += snprintf(s + used, size - used, "]"))

#else
#undef  JSON_PRINT_MACROS_H

#undef BEGIN
#undef END
#undef ADDSTR
#undef ADDCOM
#undef ADDNUM
#undef ARRBEG
#undef ARRVAL
#undef ARREND

#endif
