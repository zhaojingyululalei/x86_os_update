#ifndef __GBITMAP_H
#define __GBITMAP_H


void gbitmap_set(char* bitmap, int idx, int value);


void gbitmap_clear(char* bitmap, int idx);

void gbitmap_set_bit(char* bitmap, int idx);


int gbitmap_get(char* bitmap, int idx);

int gbitmap_is_free(char* bitmap, int idx);


int gbitmap_is_used(char* bitmap, int idx);

int gbitmap_find_first_free(char* bitmap, int size);


#endif