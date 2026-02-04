/****************************************************************************
 * apps/netutils/iptlite/iptlite_main.c
 * iptlite networking application
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "../../../nuttx/net/devif/devif.h"
#include <nuttx/config.h>

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

/****************************************************************************
 * Private Functions
 ****************************************************************************/

void listall_rules(void)
{
  int rules_counter = nflite_get_rules_counter();
  char** table = nflite_listall();

  printf("%3s %10s %16s %16s %9s %9s\n", \
  "ID", "RULE", "SRC IPADDR", "DEST IPADDR", "SRC PORT", "DEST PORT");

  for (int i = 0; i < rules_counter; i++)
    {
      for (int j = 0; j < RULE_INFO_MAX_SIZE; j++)
        {
          printf("%c", table[i][j]);
        }

      printf("\n");
    }
}

void add_rule(int rule, char * srcip, char * destip, char * srcprt, \
char * destprt)
{
  in_addr_t srcipaddr, destipaddr;
  in_port_t srcport, destport;
  bool rule_added;

  inet_pton(AF_INET, srcip, &srcipaddr);
  inet_pton(AF_INET, destip, &destipaddr);
  srcport = htons(strtoul(srcprt, NULL, 10));
  destport = htons(strtoul(destprt, NULL, 10));

  rule_added = nflite_addrule(
    rule, srcipaddr, destipaddr, srcport, destport);

  printf("rule_added? %s\n", rule_added ? "true" : "false");
}

/****************************************************************************
 * iptlite_main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  int rule;

  if (argc < 2)
    {
      printf("Not enough arguments!\n");
      return -1;
    }

  if (strcmp(argv[1], "DROP") == 0 && argc == 6)
    {
      rule = 0;
      add_rule(rule, argv[2], argv[3], argv[4], argv[5]);
    }
  else if (strcmp(argv[1], "FLUSHALL") == 0 && argc == 2)
    {
      rule = 1;
      nflite_flushall();
    }
  else if (strcmp(argv[1], "LISTALL") == 0 && argc == 2)
    {
      rule = 2;
      listall_rules();
    }
  else
    {
      printf("Invalid command! Verify command pattern.\n");
      return -1;
    }

  return 0;
}
