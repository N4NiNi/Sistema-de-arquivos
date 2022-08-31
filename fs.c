/*
 * RSFS - Really Simple File System
 *
 * Copyright © 2010,2012,2019 Gustavo Maciel Dias Vieira
 * Copyright © 2010 Rodrigo Rocco Barbieri
 *
 * This file is part of RSFS.
 *
 * RSFS is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <string.h>

#include "disk.h"
#include "fs.h"

#define CLUSTERSIZE 4096
#define FATCLUSTERS 65536
#define DIRENTRIES 128

unsigned short fat[FATCLUSTERS];

typedef struct {
  char used;
  char name[25];
  unsigned short first_block;
  int size;
} dir_entry;

dir_entry dir[DIRENTRIES];


int fs_init() {
  for (unsigned i = 0, j = 0; i < FATCLUSTERS/CLUSTERSIZE * 2; i++){
    bl_read(i, &fat[j]);
    j += CLUSTERSIZE/2;
  }
  bl_read(FATCLUSTERS/CLUSTERSIZE * 2 + 1, dir);
  
  for (unsigned i = 0; i < 32; i++)
    if(fat[i] != 3){
      printf("Imagem criada\n");
      fs_format();
      return 1;
    }
    
  printf("Imagem recuperada\n");
  return 1;
}

int fs_format() {
  unsigned i = 0;
  for (; i < FATCLUSTERS/CLUSTERSIZE * 2; i++)
    fat[i] = 3;
  fat[i] = 4;
  i++;
  for (; i < bl_size(); i++)
    fat[i] = 1;
  for (unsigned i = 0, j = 0; i < FATCLUSTERS/CLUSTERSIZE * 2; i++){
    bl_write(i, &fat[j]);
    j += CLUSTERSIZE/2;
  }
  bl_write(FATCLUSTERS/CLUSTERSIZE * 2 + 1, dir);
  return 1;
}

int fs_free() {
  printf("Função não implementada: fs_free\n");
  return 0;
}

int fs_list(char *buffer, int size) {
  printf("Função não implementada: fs_list\n");
  return 0;
}

int fs_create(char* file_name) {
  unsigned i_dir_entry_livre = -1;
  for (unsigned i = 0; i < DIRENTRIES; i++){
    if(strcmp(dir[i].name, file_name) == 0){
      printf("Arquivo já existente\n");
      return 0;
    }
    if(i_dir_entry_livre == -1 && !dir[i].used)
      i_dir_entry_livre = i;
  }

  for (unsigned i = FATCLUSTERS/CLUSTERSIZE + 1; i < FATCLUSTERS; i++)
    if(fat[i] == 1){
        dir[i_dir_entry_livre].used = 1;
        strcpy(dir[i_dir_entry_livre].name, file_name);
        dir[i_dir_entry_livre].first_block = i;
        dir[i_dir_entry_livre].size = 0;
        fat[i] = 2;
        for (unsigned i = 0, j = 0; i < FATCLUSTERS/CLUSTERSIZE * 2; i++){
          bl_write(i, &fat[j]);
          j += CLUSTERSIZE/2;
        }
        bl_write(FATCLUSTERS/CLUSTERSIZE * 2 + 1, dir);
        return 1;
    }

  return 0;
  }

int fs_remove(char *file_name) {
  printf("Função não implementada: fs_remove\n");
  return 0;
}

int fs_open(char *file_name, int mode) {
  printf("Função não implementada: fs_open\n");
  return -1;
}

int fs_close(int file)  {
  printf("Função não implementada: fs_close\n");
  return 0;
}

int fs_write(char *buffer, int size, int file) {
  printf("Função não implementada: fs_write\n");
  return -1;
}

int fs_read(char *buffer, int size, int file) {
  printf("Função não implementada: fs_read\n");
  return -1;
}

