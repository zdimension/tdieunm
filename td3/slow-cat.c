/*
 * slowcat.c	-- Programme d'essais
 * 
 *           Author: Erick Gallesio
 *    Creation date: 25-Feb-2003 10:52 (eg)
 * Last file update: 25-Feb-2003 14:00 (eg)
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

/* On définit ici volontairement un petit buffer statique pour tester
 * le driver. Petit pour voir comment se comporte le driver quand on
 * lit le buffer en plusisurs foiset statique pour voir si les
 * transferts sont effectués correctement
 */


#define MAX_BUFF 5
char buff[MAX_BUFF]= "";

int main(int argc, char *argv[])
{
  int n, fd;

  if (argc != 2) {
    fprintf(stderr, "Usage: %s fichier\n", *argv);
    exit(1);
  }
    
  fd = open(argv[1], O_RDONLY);
  if (fd < 0) {
    perror(*argv);
    exit(1);
  }

  while ((n = read(fd, buff, MAX_BUFF)) > 0) {
    write(1, buff, n);
    usleep(10000);
  }

  if (n <0 ) {
     perror(*argv);
    exit(1);
  }
  
  close(fd);  
  return 0;
}

