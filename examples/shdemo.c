#include "childsafe.h"

/*
 * This is a contrived example using the sh as a coprocess. Normally the sh
 * wouldn't be a great choice for a coprocess because it would be just as
 * easy to write a script which would achieve the same goal of running the
 * shell only once for an unlimited number of commands. But we use it as an
 * example because it's a familiar program, and to demonstrate that this
 * doesn't work just with cleartool.
 */

int
main(int argc, char *argv[])
{
   CHILD *schp;			/* sh child-handle pointer */
   char line[1024];		/* general-purpose char buffer */

   /* Open (start) the shell process */
   if ((schp = child_open("sh", "echo ++EOT++", "++EOT++", "exit")) == NULL)
      return 1;
   /* Run an ls cmd, then get its output and check its exit code. */
   if (child_puts("ls -lrt /var", schp) <= 0)
      return 1;
   while (child_gets(line, sizeof(line), schp) != NULL)
      (void) fputs(line, stdout);
   if (child_end(schp, CP_SHOW_ERR) != 0)
      return 1;

   /*
    * Now attempt to remove a file and report whether it was there or not
    * Dump any resulting stdout.
    */
   if (child_puts("rm /tmp/test-file", schp) == EOF)
      return 1;
   while (child_gets(line, sizeof(line), schp) != NULL);
   if (child_end(schp, CP_NO_SHOW_ERR) == 0)
      (void) printf("\n/tmp/test-file was removed!\n");
   else
      (void) printf("\n/tmp/test-file wasn't there!\n");
   /* Return actual exit status of sh process */
   return child_close(schp);
}
