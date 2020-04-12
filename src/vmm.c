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
    int page_number;
};

static struct clock_entry clock[NUM_FRAMES];

void vmm_init (FILE *log)
{
    // Initialise le clock.
    for (int i = 0; i < NUM_FRAMES; i++) {
        clock[i].reference    =  0;
        clock[i].page_number  = -1;
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

/* implementation of second chance's enhanced algorithm. yeah. */
int page_replacement (unsigned int page_number)
{
    bool passed = false; // flag for second encounter
    int i = 0;
    while(true) {
        if (!clock[i].reference && (clock[i].page_number < 0 ||
                                    !pt_readonly_p(clock[i].page_number) ||
                                    passed)) {
            // found a victim

            if (clock[i].page_number >= 0) {
                pt_unset_entry(clock[i].page_number);
                if (pt_readonly_p(clock[i].page_number)) {
                    pm_backup_page(i, clock[i].page_number);
                }
            }

            pt_set_entry(page_number, i);
            clock[i].page_number = page_number;
            return i;
        }

        clock[i].reference = 0; // reset reference for second encounter

        i++;
        if (i == NUM_FRAMES)
            passed = true;
        i %= NUM_FRAMES;
    }
}

/* Effectue une lecture à l'adresse logique `laddress`.  */
char vmm_read (unsigned int laddress)
{
    char c;
    read_count++;

    unsigned int paddress;
    unsigned int page   = laddress / PAGE_FRAME_SIZE;
    unsigned int offset = laddress % PAGE_FRAME_SIZE;

    int frame = tlb_lookup(page, false);
    if (frame < 0) {
        // tlb miss
        frame = pt_lookup(page);
        if (frame < 0) {
            // page fault
            frame = page_replacement(page);
            pm_download_page(page, frame);
            pt_set_readonly(page, true);
        }
        tlb_add_entry(page, frame, pt_readonly_p(page));
    }
    clock[frame].reference = true;

    paddress = frame * PAGE_FRAME_SIZE + offset;

    c = pm_read(paddress);

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

    unsigned int paddress;
    unsigned int page   = laddress / PAGE_FRAME_SIZE;
    unsigned int offset = laddress % PAGE_FRAME_SIZE;

    int frame = tlb_lookup(page, true);
    if (frame < 0) {
        // tlb miss
        frame = pt_lookup(page);
        if (frame < 0) {
            // page fault
            frame = page_replacement(page);
            pm_download_page(page, frame);
            pt_set_readonly(page, false);
        }
        tlb_add_entry(page, frame, pt_readonly_p(page));
    }

    clock[frame].reference = true;
    pt_set_readonly(page, false);

    paddress = frame * PAGE_FRAME_SIZE + offset;

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
