/*
 * Demo of how to use the C interface with cleartool.
 */

#include "childsafe.h"

int
main(int argc, char *argv[])
{
   CHILD *ctcp;			/* cleartool child-handle pointer */
   char line[1024];		/* general-purpose char buffer */

   /* Open (start) the cleartool */
   if ((ctcp = child_open("cleartool", "pwd -h", "Usage: pwd")) == NULL)
      return 1;
   /* Run a cleartool cmd, then get its output and check its exit code. */
   if (child_puts("ls", ctcp) <= 0)
      return 1;
   while (child_gets(line, sizeof(line), ctcp) != NULL)
      (void) fputs(line, stdout);
   if (child_end(ctcp, CP_SHOW_ERR) != 0)
      return 1;

   /* Return actual exit status of cleartool process */
   return child_close(ctcp);
}
