#include <child.h>

/*
 * This is a contrived example using the ksh as a coprocess. Normally the ksh
 * wouldn't be a great choice for a coprocess because it would be just as
 * easy to write a script, which would achieve the same goal of running the
 * shell only once for an unlimited number of commands. But we use ksh as an
 * example because it's a familiar program, and to demonstrate that this
 * doesn't work just with cleartool.
 */

int
main(int argc, char *argv[])
{
   CHILD *kchp;			/* ksh child-handle pointer */
   char line[1024];		/* general-purpose char buffer */

   /* Open (start) the shell process */
   if ((kchp = child_open("ksh", "print ++EOT++", "++EOT++")) == NULL)
      return 1;
   /* Run an ls cmd, then get its output and check its exit code. */
   if (child_puts("ls -lrt /var", kchp) <= 0)
      return 1;
   while (child_gets(line, sizeof(line), kchp) != NULL)
      (void) fputs(line, stdout);
   if (child_end(kchp, CP_SHOW_ERR) != 0)
      return 1;

   /*
    * Now attempt to remove a file and report whether it was there or not
    * Dump any resulting stdout.
    */
   if (child_puts("rm /tmp/test-file", kchp) == EOF)
      return 1;
   while (child_gets(line, sizeof(line), kchp) != NULL);
   if (child_end(kchp, CP_NO_SHOW_ERR) == 0)
      (void) printf("\n/tmp/test-file was removed!\n");
   else
      (void) printf("\n/tmp/test-file wasn't there!\n");
   /* Return actual exit status of ksh process */
   return child_close(kchp);
}
