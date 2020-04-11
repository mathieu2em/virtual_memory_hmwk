#include <fcntl.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include "conf.h"
#include "common.h"
#include "vmm.h"
#include "tlb.h"
#include "pt.h"
#include "pm.h"

static unsigned int read_count = 0;
static unsigned int write_count = 0;
static FILE* vmm_log;

void vmm_init (FILE *log)
{
  // Initialise le fichier de journal.
  vmm_log = log;
}


// NE PAS MODIFIER CETTE FONCTION
static void vmm_log_command (FILE *out, const char *command,
                             unsigned int laddress, /* Logical address. */
		             unsigned int page,
                             unsigned int frame,
                             unsigned int offset,
                             unsigned int paddress, /* Physical address.  */
		             char c) /* Caractère lu ou écrit.  */
{
  if (out)
    fprintf (out, "%s[%c]@%05d: p=%d, o=%d, f=%d pa=%d\n", command, c, laddress,
	     page, offset, frame, paddress);
}

/* Effectue une lecture à l'adresse logique `laddress`.  */
char vmm_read (unsigned int laddress)
{
  char c = '!';
  read_count++;
  /* ¡ TODO: COMPLÉTER ! */
  // calculer le num de page et le num de offset
  unsigned int page = laddress/PAGE_FRAME_SIZE;
  unsigned int offset = laddress%PAGE_FRAME_SIZE;

  int frame = tlb_lookup(page, false);
  if (frame < 0) {
      puts("TLB_MISS");
      // call le shit de tlb miss TODO
  }
  int paddress = frame * PAGE_FRAME_SIZE + offset;

  c = pm_read(paddress);

  // TODO: Fournir les arguments manquants.
  vmm_log_command (stdout, "READING",
                   laddress,
                   page,
                   frame,
                   offset,
                   paddress,
                   c);
  return c;
}

/* Effectue une écriture à l'adresse logique `laddress`.  */
void vmm_write (unsigned int laddress, char c)
{
  write_count++;
  /* ¡ TODO: COMPLÉTER ! */
  // calculer le num de page et le num de offset
  unsigned int page = laddress / PAGE_FRAME_SIZE;
  unsigned int offset = laddress % PAGE_FRAME_SIZE;

  int frame = tlb_lookup(page, true);
  if (frame == -2) {
      error ("trying to write in readonly page (%d)\n", page);
  } else if (frame < 0) {
      // TODO tlb_miss
  }
  int paddress = frame * PAGE_FRAME_SIZE + offset;

  pm_write(paddress, c);

  vmm_log_command (stdout, "WRITING",
                   laddress,
                   page,
                   frame,
                   offset,
                   paddress,
                   c);
}

// NE PAS MODIFIER CETTE FONCTION
void vmm_clean (void)
{
  fprintf (stdout, "VM reads : %4u\n", read_count);
  fprintf (stdout, "VM writes: %4u\n", write_count);
}
