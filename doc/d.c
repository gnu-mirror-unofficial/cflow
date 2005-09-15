#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdlib.h>
#include <string.h>

/* Create a full file name from PREFIX and NAME.
   If PREFIX is NULL, return a copy of NAME. */
char *
mkfullname (char *prefix, char *name)
{
  if (prefix)
    {
      char *p = malloc (strlen (prefix) + strlen (name) + 2);
      return strcat (strcat (strcpy (p, prefix), "/"), name);
    }
  return strdup (name);
}

/* Return true if file PREFIX/NAME is a directory.
   PREFIX can be NULL. */
int
isdir (char *prefix, char *name)
{
  struct stat st;
  char *fullname = mkfullname (prefix, name);
  
  if (stat (fullname, &st))
    {
      perror (fullname);
      free (fullname);
      return 0;
    }
  free (fullname);
  return S_ISDIR (st.st_mode);
}

/* Return true if NAME should not be recursed into */
int
ignorent (char *name)
{
  return strcmp (name, ".") == 0
    || strcmp (name, "..") == 0;
}

/* Print contents of the directory PREFIX/NAME.
   Prefix each output line with LEVEL spaces. */
void
printdir (int level, char *prefix, char *name)
{
  DIR *dir;
  struct dirent *ent;
  char *fullname = mkfullname (prefix, name);
  
  dir = opendir (fullname);
  if (!dir)
    {
      perror (fullname);
      exit (1);
    }
  
  while ((ent = readdir (dir)))
    {
      printf ("%*.*s%s", level, level, "", ent->d_name);
      if (ignorent (ent->d_name))
	printf ("\n");
      else if (isdir (fullname, ent->d_name))
	{
	  printf (" contains:\n");
	  printdir (level + 1, fullname, ent->d_name);
	}
      else
	printf ("\n");
    }
  free (fullname);
  closedir (dir);
}

int
main (int argc, char **argv)
{
  if (argc < 2)
    {
      fprintf (stderr, "usage: d DIR [DIR...]\n");
      return 1;
    }

  while (--argc)
    printdir (0, NULL, *++argv);
  
  return 1;
}

	  
    
  
