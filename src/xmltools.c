#include <libxml/tree.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>
#include <string.h>
#include "common.h"

static int get_string_from_nodes(xmlNodeSetPtr nodes, char* string_ret, size_t max_length) {
	xmlNodePtr cur;
	int size, i;
	size = (nodes) ? nodes->nodeNr : 0;
	
	for (i = 0; i < size; ++i) {
		if(nodes->nodeTab[i]->type == XML_ELEMENT_NODE) {
			cur = nodes->nodeTab[i];
			if (cur->children != NULL) {
				if (cur->children->content != NULL) {
					string_ret[0] = 0; strncat(string_ret, (char*)cur->children->content, max_length - 1);
					return 0;
				}
			}
		}
	}
	return 1;
}


int get_xpath_string(xmlDocPtr doc, const xmlChar* xpathExpr, char* string_ret, size_t max_length) {
	xmlXPathContextPtr xpathCtx; 
	xmlXPathObjectPtr xpathObj; 
	
	/* Create xpath evaluation context */
	xpathCtx = xmlXPathNewContext(doc);
	if(xpathCtx == NULL)
		FAIL("unable to create new XPath context");
	
	/* Evaluate xpath expression */
	xpathObj = xmlXPathEvalExpression(xpathExpr, xpathCtx);
	if (xpathObj == NULL) {
		xmlXPathFreeContext(xpathCtx); 
		FAIL("unable to evaluate xpath expression");
	}

	int rv = get_string_from_nodes(xpathObj->nodesetval, string_ret, max_length);

	/* Cleanup */
	xmlXPathFreeObject(xpathObj);
	xmlXPathFreeContext(xpathCtx); 
	
	return rv;
}
