#include <stdio.h>
#include <glib.h>
#include "scope500px-photo-parser-func.h"
 
int main(int argc, char **argv )
{
	GSList *results = NULL;

	int err;
	if ( argc == 2) {
		results = get_results(argv[1]);

		if (results) {
			g_slist_foreach(results, (GFunc)print_result_info, NULL);
			g_slist_free_full(results, (GDestroyNotify) result_cleanup);

			err = 0;
		} else 
			err = -1;

		return err;
	}
	else
		printf( "usage: %s <url>\n", argv[0] );

	return 0;
}
