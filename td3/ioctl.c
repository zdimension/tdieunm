/*
 * ioctl.c	-- Programme d'essais
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
#include <sys/ioctl.h>

/* Flags pour ioctl */
#define TST_GETDBG   0
#define TST_SETDBG   1
#define TST_GETBLOCK 3
#define TST_SETBLOCK 4

/* Prend deux paramètre qui dovent être des fichiers spéciaux. Place
 * le premier  en mode debug et le second en mode non bloquant 
 */

int main(int argc, char *argv[])
{
  char *f1, *f2;
  int fd1, fd2;
  int res, i;

  if (argc != 3) {
    fprintf(stderr, "Usage: %s device1 device2\n", *argv);
    exit(1);
  }
  
  f1 = argv[1]; f2 = argv[2];

  if ((fd1 = open(f1, O_RDONLY)) == -1) {
    fprintf(stderr, "Je ne peux pas ouvrir `%s'\n", f1);
    exit(1);
  }
 
  if ((fd2 = open(f2, O_RDONLY)) == -1) {
    fprintf(stderr, "Je ne peux pas ouvrir `%s'\n", f2);
    exit(1);
  }

  /* Placer le premier en mode debug */
  if (ioctl(fd1, TST_SETDBG, 1) < 0)
    perror("ioctl fd1");

  /* Tester la valeur du champs debug avec un ioctl */
  if ((res = ioctl(fd1, TST_GETDBG, &i)) < 0)
    perror("ioctl fd1");
  printf("Test ioctl TST_GETDBG res= %d i = %d\n", res, i);

  /* Autoriser les accès concurrents sur le second device */
  if (ioctl(fd2, TST_SETBLOCK, 1) < 0)
    perror("ioctl fd2");

  /* Tester la valeur du mode bloquant  */
  if ((res = ioctl(fd2, TST_GETBLOCK)) < 0)
    perror("ioctl fd2");
  printf("Test ioctl TST_GETBLOCK res= %d\n", res);

  return 0;
}
