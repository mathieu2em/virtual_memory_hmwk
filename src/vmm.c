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


struct clock_entry
{
    bool reference : 1;
    bool modify : 1;
};

static struct clock_entry clock[NUM_PAGES];

void vmm_init (FILE *log)
{
    // Initialise le clock.
    for (int i = 0; i < TLB_NUM_ENTRIES; i++) {
        clock[i].reference    =  0;
        clock[i].modify       =  0;
    }
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
        frame = pt_lookup(page);
        if ( frame < 0) {
            // page_fault
            
        }
    }
    clock[page].reference = true;

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
        frame = pt_lookup(page);
        if (frame < 0) {
            // page miss
        }
    }
    clock[page].reference = true;
    clock[page].modify    = true;

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

/* implementation of second chance's enhanced algorithm. yeah. */
int page_replacement (unsigned int page_number)
{
    // you pass max 2 times upon the structure
    bool passed = false;
    int i = 0;
    while(true) {
        // if you find 1 no ref no modif
        if (!clock[i].reference && (!clock[i].modify || passed)) {
            // on switch le shit
            if (clock[i].modify) {
                pm_backup_page(clock[i].frame_number, clock[i].page_number);
            }
        }
        clock[i].reference = 0;
        i++;
        if (i == TLB_NUM_ENTRIES) {
            passed = true;
        }
        i %= TLB_NUM_ENTRIES;
    }
}

// NE PAS MODIFIER CETTE FONCTION
void vmm_clean (void)
{
    fprintf (stdout, "VM reads : %4u\n", read_count);
    fprintf (stdout, "VM writes: %4u\n", write_count);
}
