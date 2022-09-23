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
#define FILE_OPEN_R 2
#define FILE_OPEN_W 3

unsigned short fat[FATCLUSTERS];

typedef struct {
  char used;
  char name[25];
  unsigned short first_block;
  int size;
} dir_entry;

dir_entry dir[DIRENTRIES];

void salvaFAT();

int fs_init() {
  /* Lendo a fat no disco */
  for (unsigned i = 0, j = 0; i < FATCLUSTERS/CLUSTERSIZE * 2; i++){
    bl_read(i, (char *)&fat[j]);
    j += CLUSTERSIZE/2;
  }
  /* Lendo as entradas do diretório no disco */
  bl_read(FATCLUSTERS/CLUSTERSIZE * 2 + 1, (char *)dir);
  
  /* Se os primeiros campos da fat identificar a própria fat,
  então já tem uma fat salva. */
  for (unsigned i = 0; i < FATCLUSTERS/CLUSTERSIZE * 2; i++)
    if(fat[i] != 3){
      /* Se não houver fat salva, a inicializa */
      printf("Imagem criada\n");
      fs_format();
      return 1;
    }
  printf("Imagem recuperada\n");
  return 1;
}

int fs_format() {
  unsigned i = 0;
  /* Marcando os setores ocupados pela fat no disco na própria fat */
  for (; i < FATCLUSTERS/CLUSTERSIZE * 2; i++)
    fat[i] = 3;
  /* Marcando os setores ocupados pelas entradas do diretório no disco */
  fat[i] = 4;
  i++;
  /* Marcando os setores livres do disco */
  for (; i < bl_size(); i++)
    fat[i] = 1;
  /* Inicializando o diretório */
  for (unsigned i = 0; i < DIRENTRIES; i++)
    dir[i].used = 0;

  /* Salvando a fat e as entradas do diretório no disco */
  for (unsigned i = 0, j = 0; i < FATCLUSTERS/CLUSTERSIZE * 2; i++){
    bl_write(i, (char *)&fat[j]);
    j += CLUSTERSIZE/2;
  }
  bl_write(FATCLUSTERS/CLUSTERSIZE * 2 + 1, (char *)dir);
  return 1;
}

int fs_free() {
  unsigned i = FATCLUSTERS/CLUSTERSIZE * 2 + 1; //i começa a partir dos blocos que armazenam os arquivos

  /* Busca sequencial na fat somando os blocos livres */
  unsigned setores_livres = 0;
  for (; i < bl_size(); i++)
    if(fat[i] == 1)
      setores_livres++;
  
  /* Retorna o tamanho dos blocos livres em bytes */
  return(setores_livres * SECTORSIZE);
}

int fs_list(char *buffer, int size) {
  buffer[0] = '\0'; //inicializa buffer
  /* Percorre diretório */
  for (unsigned i = 0; i < DIRENTRIES; i++)
    if(dir[i].used){ //se entrada do diretório em uso
      /* Formata a string e a coloca em buffer */
      char aux[100];
      aux[0] = '\0';
      strcat(aux, dir[i].name);
      strcat(aux, "\t\t");
      char size[20];
      sprintf(size, "%d", dir[i].size);
      strcat(aux, size);
      strcat(aux, "\n");
      strcat(buffer, aux);
    }
  return 1;
}

int fs_create(char* file_name) {
  unsigned i_dir_entry_livre = -1;
  /* Busca sequencial no diretório verificando se arquivo existe */
  for (unsigned i = 0; i < DIRENTRIES; i++){
    if(dir[i].used && strcmp(dir[i].name, file_name) == 0){
      printf("Arquivo já existente\n");
      return -1;
    }
    if(i_dir_entry_livre == -1 && !dir[i].used)
      i_dir_entry_livre = i; //primeira entrada do diretório livre
  }

  /* Percorre a fat procurando por setor livre */
  for (unsigned i = FATCLUSTERS/CLUSTERSIZE + 1; i < FATCLUSTERS; i++)
    if(fat[i] == 1){ //se livre
        dir[i_dir_entry_livre].used = 1; //marca a entrada como usada
        strcpy(dir[i_dir_entry_livre].name, file_name);
        dir[i_dir_entry_livre].first_block = i; //marca o bloco livre encontrado
        dir[i_dir_entry_livre].size = 0;
        fat[i] = 2; //marca bloco como "Último Agrupamento de Arquivo"
        
        /* Salvando a fat e as entradas do diretório no disco */
        for (unsigned i = 0, j = 0; i < FATCLUSTERS/CLUSTERSIZE * 2; i++){
          bl_write(i, (char *)&fat[j]);
          j += CLUSTERSIZE/2;
        }
        bl_write(FATCLUSTERS/CLUSTERSIZE * 2 + 1, (char *)dir);
        return i_dir_entry_livre;
    }

  return -1; //não há setores livres
  }

int fs_remove(char *file_name) {
  /* Busca sequencial no diretório verificando se arquivo existe */
  for (unsigned i = 0; i < DIRENTRIES; i++)
    if(dir[i].used && strcmp(dir[i].name, file_name) == 0){ //se existe
      dir[i].used = 0; //desmarca o campo "em uso"
      fat[dir[i].first_block] = 1; //marca o bloco como "Agrupamento livre"
      
      /* Salvando a fat e as entradas do diretório no disco */
      for (unsigned i = 0, j = 0; i < FATCLUSTERS/CLUSTERSIZE * 2; i++){
        bl_write(i, (char *)&fat[j]);
        j += CLUSTERSIZE/2;
      }
      bl_write(FATCLUSTERS/CLUSTERSIZE * 2 + 1, (char *)dir);
      return 1;
    }
  printf("Arquivo não encontrado\n");
  return 0;
}

int fs_open(char *file_name, int mode) {
  unsigned i;
  for (i = 0; i < DIRENTRIES; i++)
    if (dir[i].used && strcmp(dir[i].name, file_name) == 0)
      break;
  if(i == DIRENTRIES && mode == FS_R){
    printf("Arquivo não encontrado\n");
    return -1;
  }

  if(mode == FS_R){
    dir[i].used = FILE_OPEN_R;
    return i;
  }
  if (mode == FS_W){
    if(i != DIRENTRIES){
      printf("Removido arquivo\n");
      fs_remove(file_name);
    }
    i = fs_create(file_name);
    if(i == -1)
      return -1;
    dir[i].used = FILE_OPEN_W;
    bl_write(FATCLUSTERS/CLUSTERSIZE * 2 + 1, (char *)dir);
    return i;
  }
  return -1;
}

int fs_close(int file)  {
  if(dir[file].used == FILE_OPEN_R || dir[file].used == FILE_OPEN_W){
    dir[file].used = 1;
    bl_write(FATCLUSTERS/CLUSTERSIZE * 2 + 1, (char *)dir);
    return 1;
  }
  else
    return -1;
}

int fs_write(char *buffer, int size, int file) {
  if (dir[file].used == FILE_OPEN_R || !dir[file].used)
    return -1;
  
  /* Encontrar o ultimo bloco do arquivo */
  unsigned i_fat = dir[file].first_block;
  for(; fat[i_fat] != 2; i_fat = fat[i_fat]);  
  char arq_data[CLUSTERSIZE] = {};
  if(!bl_read(i_fat, arq_data))
    return -1;

  /* Ler o buffer e ir salvando */
  unsigned setor = FATCLUSTERS/CLUSTERSIZE * 2 + 1; //inicia na primeira posiçao de dados da fat
  unsigned bytes_escritos = 0;
  for (unsigned i = 0; i < size; i++){
    arq_data[dir[file].size % CLUSTERSIZE] = buffer[i];
    bytes_escritos++;
    dir[file].size++;

    //se arq_data já tem o tamanho de um setor, ou já foi salvo os "size" bytes em arq_ata
    if(dir[file].size % CLUSTERSIZE == 0 || i + 1 == size){
        printf("Escrevendo no setor %d  ", i_fat);
        puts(buffer);
        if(!bl_write(i_fat, arq_data)){
          printf("Algum problema\n");
          salvaFAT();
          return bytes_escritos;
        }
        arq_data[0] = '\0';
        printf("Sucesso\n");
        if(i + 1 == size){ //se acabou o buffer
          salvaFAT();
          return bytes_escritos;
        }

        //senão, necessário mais um setor
        for (; setor < bl_size(); setor++){
          printf("Procurando espaço livre. Analisando o setor %d\n", setor);
          if(fat[setor] == 1)
            break;
        }
        fat[setor] = 2;
        fat[i_fat] = setor;
        i_fat = setor; 
    }
  }
  return 0;
}

int fs_read(char *buffer, int size, int file) {
  buffer[0] = '\0';
  if (dir[file].used == FILE_OPEN_W || !dir[file].used)
    return -1;
  
  unsigned i_fat = dir[file].first_block;
  unsigned bytes_lidos = 0;

  char setor_data[CLUSTERSIZE];
  setor_data[0] = '\0';
  while (1){
    printf("Lendo o setor %d  ", i_fat);
    if(!bl_read(i_fat, setor_data)){
      printf("Erro\n");
      return bytes_lidos;
    }
    printf("Sucesso\n");
    bytes_lidos += CLUSTERSIZE;
    strncpy(buffer, setor_data, size);
    buffer[size%CLUSTERSIZE] = '\0';
    puts(setor_data);
    if (bytes_lidos >= size){
      printf("Foi lido tudo size = %d | bytes_lidos = %d\n", size, bytes_lidos);
      return 0;
    }
    if(fat[i_fat] == 2){
      printf("FIM DO ARQUIVO\n");
      return bytes_lidos;
    }
    i_fat = fat[i_fat];
  }
  return bytes_lidos;
}

void salvaFAT(){
  /* Salvando a fat e as entradas do diretório no disco */
  for (unsigned i = 0, j = 0; i < FATCLUSTERS/CLUSTERSIZE * 2; i++){
    bl_write(i, (char *)&fat[j]);
    j += CLUSTERSIZE/2;
  }
  bl_write(FATCLUSTERS/CLUSTERSIZE * 2 + 1, (char *)dir);
}

