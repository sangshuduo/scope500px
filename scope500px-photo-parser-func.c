#include <stdio.h>
#include <tidy/tidy.h>
#include <tidy/buffio.h>
#include <curl/curl.h>
#include <glib.h>
 
#define DEBUG	1

#if DEBUG
int verbose = 1;
#define dprintf	printf
#else
int verbose = 0;
#define dprintf
#endif

typedef int bool;
#define true 1
#define false 0

typedef struct {
	gchar *link;
	gchar *icon_url;
	gchar *title;
	gchar *description;
	gchar *creation_date;
	gchar *author;
} result_t;

bool photo_found = false;

/* curl write callback, to fill tidy's input buffer...  */ 
uint write_cb(char *in, uint size, uint nmemb, TidyBuffer *out)
{
	uint r;
	r = size * nmemb;
	tidyBufAppend( out, in, r );
	return(r);
}
 
GSList *addToResults(TidyNode node, GSList *results)
{
	/* walk the attribute list */ 
	TidyNode child = tidyGetChild(node);
	while( child ) {
		ctmbstr name = tidyNodeGetName(child);

		if (strncmp(name, "a", 1) == 0) {
			TidyNode grandson = tidyGetChild(child);
			while (grandson) {
				TidyAttr attr = tidyAttrFirst(grandson);
				while (attr) {
					if (strncmp(tidyAttrName(attr), "src", 3) == 0) {
						result_t *result = (result_t *) malloc( sizeof(result_t));
						bzero((result_t *)result, sizeof(result_t));
						result->icon_url = malloc(strlen(tidyAttrValue(attr)) + 1);
						strcpy(result->icon_url, tidyAttrValue(attr));
						results = g_slist_append(results, result);
						break;
					}
					attr = tidyAttrNext(attr);
				}

				grandson = tidyGetNext(grandson);
			}
		}
		child = tidyGetNext(child);
	}

	return results;
}

bool isPhotoNode(TidyDoc doc, TidyNode node)
{
	ctmbstr name = tidyNodeGetName(node);
	/* if it has a name, then it's an HTML tag ... */ 

	if (name) {
#if DEBUG
		printf( "%s%s ", "<", name);
		TidyAttr attr;
		/* walk the attribute list */ 
		for ( attr=tidyAttrFirst(node); attr; attr=tidyAttrNext(attr) ) {
			printf("%s", tidyAttrName(attr));
			tidyAttrValue(attr)?printf("=\"%s\" ",
			tidyAttrValue(attr)):printf(" ");
		}
		printf( ">\n");
#endif
		if (strncmp(name, "div", 3)==0) {
			TidyAttr attr = tidyAttrFirst(node);
			if ((strncmp(tidyAttrName(attr), "class", 5)==0) 
				&& (strncmp(tidyAttrValue(attr), "photo", 5)==0)) {

				return true;
			}
		}
	}
#if DEBUG
	else {
		/* if it doesn't have a name, then it's probably text, cdata, etc... */ 
		TidyBuffer buf;
		tidyBufInit(&buf);
		tidyNodeGetText(doc, node, &buf);
		printf("%s\n", buf.bp?(char *)buf.bp:"");
		tidyBufFree(&buf);
	}
#endif

	return false;
}

GSList *walkNode(TidyDoc doc, GSList *results)
{
	GQueue *stack = g_queue_new();

	TidyNode current = tidyGetChild(tidyGetRoot(doc));

	while( current || g_queue_get_length(stack) ) {
		while (current) {
			g_queue_push_head(stack, (gpointer)current);

			if (isPhotoNode(doc, current)) {
				results = addToResults(current, results);
				break;
			} else {
				current = tidyGetChild(current);
			}
		}

		if (g_queue_get_length(stack)) {
			current = g_queue_pop_head(stack);
			if (current)
				current = tidyGetNext(current);
		} 
	}

	g_queue_free(stack);

	return results;
}
 
void
result_cleanup(gpointer data)
{
	/* This is specific to a type of result, so you'll
	* need to adapt it to the results sent by your
	* source.
	*/
	result_t *result = (result_t *)data;
	if (result->link) {
		free(result->link);
	}
	if (result->icon_url) {
		free(result->icon_url);
	}
	if (result->title) {
		free(result->title);
	}
	if (result->description) {
		free(result->description);
	}
	if (result->creation_date) {
		free(result->creation_date);
	}
	if (result->author) {
		free(result->author);
	}

	free(result);
}
 
void print_result_info(result_t *result)
{
	puts(result->icon_url);
}

#define BASE_URI "http://500px.com/search?q="

/**
 * @brief get_results Get and parse the results from a search query
 * @param search_term String submitted as the search term
 * @return Search results
 */
GSList *
get_results(const char *search_term)
{
	GSList *results = NULL;
	GString *url = NULL;

	/* Check if an actual search term was submitted, return otherwise */
	if (search_term == NULL) {
		g_warning("get_results: search_term cannot be null");
		return NULL;
	}

	/* Construct the full search query */
	url = g_string_new(BASE_URI);
	g_string_append(url, search_term);
	g_debug("searching %s", url->str);

	CURL *curl;
	char curl_errbuf[CURL_ERROR_SIZE];
	TidyDoc tdoc;
	TidyBuffer docbuf = {0};
	TidyBuffer tidy_errbuf = {0};
	int err, tidy_result;

	curl = curl_easy_init();
	curl_easy_setopt(curl, CURLOPT_URL, url->str);
	curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, curl_errbuf);
	curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
	curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_cb);

	tdoc = tidyCreate();
	tidyOptSetBool(tdoc, TidyForceOutput, yes); /* try harder */ 
	tidyOptSetInt(tdoc, TidyWrapLen, 4096);
	tidySetErrorBuffer( tdoc, &tidy_errbuf );
	tidyBufInit(&docbuf);

	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &docbuf);
	err=curl_easy_perform(curl);

	g_string_free(url, TRUE);

	if ( !err ) {
		tidy_result = tidyParseBuffer(tdoc, &docbuf); /* parse the input */ 

		if ( tidy_result >= 0 ) {
			tidy_result = tidyCleanAndRepair(tdoc); /* fix any problems */ 
			if ( tidy_result >= 0 ) {
				tidy_result = tidyRunDiagnostics(tdoc); /* load tidy error buffer */ 
				if ( tidy_result >= 0 ) {
					results = walkNode(tdoc, results); /* walk the tree */ 
				}
			}
		}
	}
	else
		fprintf(stderr, "%s\n", curl_errbuf);

	/* clean-up */ 
	curl_easy_cleanup(curl);
	tidyBufFree(&docbuf);
	tidyBufFree(&tidy_errbuf);
	tidyRelease(tdoc);

	return results;
}
