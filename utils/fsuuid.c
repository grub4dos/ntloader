#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>
#include <fsuuid.h>
#ifdef _WIN32
#include <windows.h>
#endif

static int fsuuid_help (const char *prog)
{
  printf ("Usage:\n"
          "%s VOLUME\n"
#ifdef _WIN32
          "-l List all volumes\n"
#endif
          , prog);
  return 0;
}

#ifdef _WIN32
static int fsuuid_list (void)
{
  BOOL rc;
  HANDLE search;
  WCHAR volume[MAX_PATH];
  for (rc = TRUE, search = FindFirstVolumeW (volume, MAX_PATH);
       rc && search != INVALID_HANDLE_VALUE;
       rc = FindNextVolumeW (search, volume, MAX_PATH))
  {
    /* Removing trailing backslash */
    size_t len = wcslen (volume);
    if (len >= 1 && volume[len - 1] == L'\\')
      volume[len - 1] = L'\0';
    printf ("%ls\n", volume);
  }
  if (search != INVALID_HANDLE_VALUE)
    FindVolumeClose (search);
  return 0;
}
#endif

static void fsuuid_get (const char *volume)
{
  char uuid[17] = "";
  union volume_boot_record vbr;
  FILE *fd = fopen (volume, "rb");

  if (fd == NULL)
  {
    fprintf (stderr, "Could not open %s.\n", volume);
    return;
  }

  if (fread (&vbr, sizeof (vbr), 1, fd) != 1)
  {
    fprintf (stderr, "Could not read %s.\n", volume);
    fclose (fd);
    return;
  }

  if (memcmp (vbr.exfat.oem_name, "EXFAT", 5) == 0)
  {
    snprintf (uuid, 17, "%04x-%04x",
              (uint16_t) (vbr.exfat.num_serial >> 16),
              (uint16_t) vbr.exfat.num_serial);
  }
  else if (memcmp (vbr.ntfs.oem_name, "NTFS", 4) == 0)
  {
    snprintf (uuid, 17, "%016llx", (unsigned long long) vbr.ntfs.num_serial);
  }
  else if (memcmp (vbr.fat.version.fat12_or_fat16.fstype, "FAT12", 5) == 0)
  {
    snprintf (uuid, 17, "%04x-%04x",
              (uint16_t) (vbr.fat.version.fat12_or_fat16.num_serial >> 16),
              (uint16_t) vbr.fat.version.fat12_or_fat16.num_serial);
  }
  else if (memcmp (vbr.fat.version.fat12_or_fat16.fstype, "FAT16", 5) == 0)
  {
    snprintf (uuid, 17, "%04x-%04x",
              (uint16_t) (vbr.fat.version.fat12_or_fat16.num_serial >> 16),
              (uint16_t) vbr.fat.version.fat12_or_fat16.num_serial);
  }
  else if (memcmp (vbr.fat.version.fat32.fstype, "FAT32", 5) == 0)
  {
    snprintf (uuid, 17, "%04x-%04x",
              (uint16_t) (vbr.fat.version.fat32.num_serial >> 16),
              (uint16_t) vbr.fat.version.fat32.num_serial);
  }

  if (uuid[0])
    printf ("%s\n", uuid);
  else
    fprintf (stderr, "Unsupported filesystem.\n");

  fclose (fd);
}

int main (int argc, char *argv[])
{
  int i;
  if (argc <= 1)
    return fsuuid_help (argv[0]);
#ifdef _WIN32
  if (argc == 2 && strcasecmp (argv[1], "-l") == 0)
    return fsuuid_list ();
#endif
  for (i = 1; i < argc; i++)
  {
    char *volume = argv[i];
#ifdef _WIN32
    char drive_letter[] = "\\\\.\\c:";
    if (isalpha (argv[i][0])
        && (argv[i][1] == ':' || argv[i][1] == '\0')
        && strlen (argv[i]) <= 3 /* c:\ */)
    {
      drive_letter[4] = argv[i][0];
      volume = drive_letter;
    }
#endif
    fsuuid_get (volume);
  }
  return 0;
}
