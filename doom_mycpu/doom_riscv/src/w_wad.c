// w_wad.c (Bare Metal Version)
// Fixed: Renamed strupr to my_strupr to avoid conflict with Picolibc

#include <stdio.h>
#include <string.h>
#include <ctype.h> // for toupper

#include "doomdef.h"
#include "m_swap.h"
#include "i_system.h"
#include "z_zone.h"
#include "w_wad.h"

// =========================================================
// WAD Data Structures
// =========================================================
typedef struct
{
    int     filepos;
    int     size;
    char    name[8];
} filelump_t;

// =========================================================
// WAD Data Linked via objcopy
// =========================================================
extern char _binary_doom1_wad_start[];
extern char _binary_doom1_wad_end[];

// Globals
lumpinfo_t* lumpinfo;
int         numlumps;
void** lumpcache;

// [修正] 改名為 my_strupr，避免跟 Picolibc 的 strupr 衝突
void my_strupr(char *s) {
    while (*s) {
        *s = toupper((unsigned char)*s);
        s++;
    }
}

//
// W_InitMultipleFiles
//
void W_InitMultipleFiles (char** filenames)
{
    int size;
    
    // 計算 WAD 大小
    size = _binary_doom1_wad_end - _binary_doom1_wad_start;
    
    printf("[WAD] W_Init: Linked WAD found at %p, size %d bytes.\n", 
           _binary_doom1_wad_start, size);

    // 檢查檔頭
    if (strncmp(_binary_doom1_wad_start, "IWAD", 4) != 0 &&
        strncmp(_binary_doom1_wad_start, "PWAD", 4) != 0)
    {
        I_Error("W_Init: Invalid WAD header!");
    }

    // 讀取 Lump 數量
    int* pHeader = (int*)_binary_doom1_wad_start;
    numlumps = LONG(pHeader[1]);
    int infotableofs = LONG(pHeader[2]);

    printf("[WAD] NumLumps: %d, TableOffset: %d\n", numlumps, infotableofs);

    // 分配 LumpInfo 表格記憶體
    lumpinfo = (lumpinfo_t*) Z_Malloc (numlumps * sizeof(lumpinfo_t), PU_STATIC, 0);
    
    // 指向 WAD 中的目錄表
    filelump_t* fileinfo = (filelump_t*)(_binary_doom1_wad_start + infotableofs);

    // 解析目錄表
    for (int i=0 ; i<numlumps ; i++)
    {
        lumpinfo[i].handle = 0; 
        lumpinfo[i].position = LONG(fileinfo[i].filepos);
        lumpinfo[i].size = LONG(fileinfo[i].size);
        
        strncpy (lumpinfo[i].name, fileinfo[i].name, 8);
        
        fileinfo++;
    }

    // 初始化 Cache
    lumpcache = (void**) Z_Malloc (numlumps * sizeof(void*), PU_STATIC, 0);
    memset (lumpcache, 0, numlumps * sizeof(void*));
    
    printf("[WAD] W_Init: Loaded %d lumps successfully.\n", numlumps);
}

//
// W_CheckNumForName
//
int W_CheckNumForName (char* name)
{
    char name8[9];
    int  v1, v2;
    lumpinfo_t* lump_p;

    // 標準化名稱
    name8[8] = 0;
    strncpy(name8, name, 8);
    name8[8] = 0; 
    
    // [修正] 呼叫改名後的函式
    my_strupr (name8);

    v1 = *(int *)name8;
    v2 = *(int *)&name8[4];

    // 倒著搜尋
    lump_p = lumpinfo + numlumps;

    while (lump_p-- != lumpinfo)
    {
        if ( *(int *)lump_p->name == v1 &&
             *(int *)&lump_p->name[4] == v2 )
        {
            return lump_p - lumpinfo;
        }
    }

    return -1;
}

int W_GetNumForName (char* name)
{
    int i = W_CheckNumForName (name);
    if (i == -1)
        I_Error ("W_GetNumForName: %s not found!", name);
    return i;
}

int W_LumpLength (int lump)
{
    if (lump >= numlumps) I_Error ("W_LumpLength: %i >= numlumps", lump);
    return lumpinfo[lump].size;
}

void W_ReadLump (int lump, void* dest)
{
    byte* src = (byte*)(_binary_doom1_wad_start + lumpinfo[lump].position);
    memcpy (dest, src, lumpinfo[lump].size);
}

void* W_CacheLumpNum (int lump, int tag)
{
    if ((unsigned)lump >= numlumps) I_Error ("W_CacheLumpNum: %i >= numlumps", lump);
    
    if (lumpcache[lump]) return lumpcache[lump];

    Z_Malloc (W_LumpLength (lump), tag, &lumpcache[lump]);
    W_ReadLump (lump, lumpcache[lump]);

    return lumpcache[lump];
}

void* W_CacheLumpName (char* name, int tag)
{
    return W_CacheLumpNum (W_GetNumForName (name), tag);
}