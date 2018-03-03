#ifndef _XMLTOOLS_H_
#define _XMLTOOLS_H_
#include "xmltools.h"
#include <libxml/parser.h>

/** Returns 0 on success */
int get_xpath_string(xmlDocPtr doc, const char* xpathExpr, char* string_ret, size_t max_length);

#endif // _XMLTOOLS_H_
