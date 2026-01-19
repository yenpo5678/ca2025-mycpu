// w_wad.c - Standard Version (No Zombie Mode)
#include <stdio.h>
#include <string.h>
#include <ctype.h> 

#include "doomdef.h"
#include "m_swap.h"
#include "i_system.h"
#include "z_zone.h"
#include "w_wad.h"

typedef struct {
    int     filepos;
    int     size;
    char    name[8];
} filelump_t;

extern char _binary_doom1_wad_start[];
extern char _binary_doom1_wad_end[];

lumpinfo_t* lumpinfo;
int         numlumps;
void** lumpcache;

void my_strupr(char *s) {
    while (*s) {
        *s = toupper((unsigned char)*s);
        s++;
    }
}

void W_InitMultipleFiles (char** filenames)
{
    printf("[WAD] W_Init: Linked WAD found at %p.\n", _binary_doom1_wad_start);
    int* pHeader = (int*)_binary_doom1_wad_start;
    numlumps = LONG(pHeader[1]);
    int infotableofs = LONG(pHeader[2]);

    lumpinfo = (lumpinfo_t*) Z_Malloc (numlumps * sizeof(lumpinfo_t), PU_STATIC, 0);
    char* fileinfo_ptr = (char*)(_binary_doom1_wad_start + infotableofs);

    for (int i=0 ; i<numlumps ; i++) {
        filelump_t* fileinfo = (filelump_t*)fileinfo_ptr;
        lumpinfo[i].handle = 0;
        lumpinfo[i].position = LONG(fileinfo->filepos);
        lumpinfo[i].size = LONG(fileinfo->size);
        strncpy (lumpinfo[i].name, fileinfo->name, 8);
        fileinfo_ptr += 16; 
    }

    lumpcache = (void**) Z_Malloc (numlumps * sizeof(void*), PU_STATIC, 0);
    memset (lumpcache, 0, numlumps * sizeof(void*));
}

int W_CheckNumForName (char* name)
{
    char name8[9];
    lumpinfo_t* lump_p;
    
    // 正常版：不處理亂碼，因為資料搬運修好後不該有亂碼
    strncpy(name8, name, 8);
    name8[8] = 0; 
    my_strupr (name8);
    int v1 = *(int *)name8;
    int v2 = *(int *)&name8[4];

    lump_p = lumpinfo + numlumps;
    while (lump_p-- != lumpinfo) {
        if ( *(int *)lump_p->name == v1 && *(int *)&lump_p->name[4] == v2 )
            return lump_p - lumpinfo;
    }
    return -1;
}

int W_GetNumForName (char* name)
{
    int i = W_CheckNumForName (name);
    if (i == -1) {
        // 正常版：找不到就報錯，這樣我們才知道真的缺什麼
        I_Error("W_GetNumForName: %s not found!", name);
    }
    return i;
}

int W_LumpLength (int lump) {
    if (lump >= numlumps) I_Error("W_LumpLength: %i >= numlumps", lump);
    return lumpinfo[lump].size;
}

void W_ReadLump (int lump, void* dest) {
    byte* src = (byte*)(_binary_doom1_wad_start + lumpinfo[lump].position);
    memcpy (dest, src, lumpinfo[lump].size);
}

void* W_CacheLumpNum (int lump, int tag) {
    if ((unsigned)lump >= numlumps) I_Error("W_CacheLumpNum: %i >= numlumps", lump);
    if (lumpcache[lump]) return lumpcache[lump];
    Z_Malloc (W_LumpLength (lump), tag, &lumpcache[lump]);
    W_ReadLump (lump, lumpcache[lump]);
    return lumpcache[lump];
}

void* W_CacheLumpName (char* name, int tag) {
    return W_CacheLumpNum (W_GetNumForName (name), tag);
}

void W_Reload (void) {}