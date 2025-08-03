#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "pager.h"

#define TABLE_MAX_PAGES 100

struct Pager {
    void* pages[TABLE_MAX_PAGES];
    uint32_t num_pages;
};

Pager* pager_open(const char* filename) {
    Pager* pager = malloc(sizeof(Pager));
    if (!pager) {
        return NULL;
    }
    pager->num_pages = 0;
    for (uint32_t i = 0; i < TABLE_MAX_PAGES; i++) {
        pager->pages[i] = NULL;
    }
    return pager;
}

void pager_close(Pager* pager) {
    for (uint32_t i = 0; i < pager->num_pages; i++) {
        if (pager->pages[i]) {
            free(pager->pages[i]);
        }
    }
    free(pager);
}

void* pager_get_page(Pager* pager, page_num_t page_num) {
    if (page_num >= TABLE_MAX_PAGES) {
        return NULL;
    }

    if (pager->pages[page_num] == NULL) {
        void* page = malloc(PAGE_SIZE);
        if(!page){
            return NULL;
        }
        memset(page, 0, PAGE_SIZE);
        pager->pages[page_num] = page;

        if (page_num >= pager->num_pages) {
            pager->num_pages = page_num + 1;
        }
    }
    return pager->pages[page_num];
}

uint32_t pager_get_num_pages(Pager* pager) {
    return pager->num_pages;
}

void pager_mark_dirty(Pager* pager, page_num_t page_num) {
    // Mock does not need to do anything here
}

void pager_flush_page(Pager* pager, page_num_t page_num) {
    // Mock does not need to do anything here
}