#include <stdio.h>

/*
 * TST_PG.C
 *
 * Little test program of "PG" command for FBB BBS software.
 *
 * (C) F6FBB 1991.
 *
 * FBB software 5.14 and up.
 *
 *
 * This program echoes to the user what he types
 * or executes a BBS command preceded by "CMD"
 * until "BYE" is received
 */


main(int argc, char **argv)
{
  int  i;
  int  level = atoi(argv[2]);        /* Get level from argument list   */

                                     /* and transform it to integer    */
  if (level == 0) {                  /* Is level equal to 0 ?          */
                                     /* This is the first call         */
    printf("Hello %s, type BYE when you want to stop !\n", argv[1]);
    return(1);                       /* program will be called again   */
  }
  else {
    strupr(argv[5]);                 /* Capitalise the first word      */
    if (strcmp(argv[5], "BYE") == 0) {       /* is BYE received ?      */
      printf("Ok, bye-bye\n");
      return(0);                     /* Yes, go on BBS                 */
    }
    else if (strcmp(argv[5], "CMD") == 0) {  /* is CMD received ?      */
      for (i = 6 ; i < argc ; i++)   /* List line arguments            */
      printf("%s ", argv[i]);        /* sent by user                   */
      putchar('\n');
      for (i = 6 ; i < argc ; i++)   /* List line arguments            */
        printf("%s ", argv[i]);      /* sent by user                   */
      putchar('\n');
      return(4);                     /* Yes, send command              */
    }
    else {
      printf("You told me : ");      /* These are other lines          */
      for (i = 5 ; i < argc ; i++)   /* List line arguments            */
      printf("%s ", argv[i]);        /* sent by user                   */
      putchar('\n');
      return(1);                     /* No, call again program         */
    }
  }
}