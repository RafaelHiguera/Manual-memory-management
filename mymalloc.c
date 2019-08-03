#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include "mymalloc.h"
#define STANDARD_BLOCK_SIZE 4096
#define STANDARD_ARRAY_SIZE 10

//definitions des structures
typedef struct object object;
struct object {
  unsigned int size;
  unsigned int numberOfReferences;
  void* startPointer;
  unsigned int blockId;
};

typedef struct block block;
struct block {
  void* startPointer;
  void* endPointer;
  void* currentPointer;
  object* objects;
  unsigned int size;
  unsigned int numberOfObjects;
  unsigned int freeSpaces;
};

typedef struct playgroundOfBlocks playgroundOfBlocks;
struct playgroundOfBlocks {
  unsigned int size;
  unsigned int numberOfBlocks;
  block* blocks;
};

//signatures des fonctions
block* createAndSaveBlock(size_t size);

void* saveObject(block* block, size_t sizeOfObject, int indexOfBlock);

size_t getBlockSize(size_t size);

void insertInformationInBlock(object information, block* block);

int updateNumberOfReferences(void* objectId, block* block,  int numberOfReferences);

void freeMemory(block* block, object* object);

void playgroundBlocksAdjustement(int blockId);

void* searchFreeSpaceAndInsert(size_t size);

playgroundOfBlocks playground = {0,0,NULL};

void* mymalloc(size_t size){
  if(size == 0){
    return NULL;
  }

  void* objectPointer =  searchFreeSpaceAndInsert(size);
  if(objectPointer != NULL){
    return objectPointer;
  }

  block* tempPointer = playground.blocks;
  for(int i = 0; i < playground.numberOfBlocks;i++){
    block* tempBlock = (tempPointer + i);
    size_t avaibleSpace = tempBlock->endPointer - tempBlock->currentPointer;
    if(avaibleSpace >= size){
      return saveObject(tempBlock, size, i);
    }
  }

  //Si il n'a pas d'estpace libres entre les objets ni dans le bloc, on crée un
  //nouveau bloc
  block* newBlock = createAndSaveBlock(getBlockSize(size));
  return saveObject(newBlock,size, (playground.numberOfBlocks-1));
}

int refinc(void* ptr){
  if(ptr == NULL){
    return 0;
  }

  //chercher si le pointeur est dans un block
  block* tempPointer = playground.blocks;
  for(int i = 0; i < playground.numberOfBlocks;i++){
    block* tempBlock = (tempPointer + i);
    if(ptr >= tempBlock->startPointer && ptr <= tempBlock->endPointer){
      return updateNumberOfReferences(ptr, tempBlock, 1);
    }
  }
  return 0;
}

void myfree(void *ptr){
  if(ptr == NULL){
    return;
  }

  //chercher si le pointeur est dans un block
  block* tempPointer = playground.blocks;
  for(int i = 0; i < playground.numberOfBlocks;i++){
    block* tempBlock = (tempPointer + i);
    if(ptr >= tempBlock->startPointer && ptr <= tempBlock->endPointer){
      updateNumberOfReferences(ptr, tempBlock, -1);
      return;
    }
  }
}


block* createAndSaveBlock(size_t size){
  if(playground.size == 0){
    playground.blocks = malloc(STANDARD_ARRAY_SIZE * sizeof(block));
    playground.size = STANDARD_ARRAY_SIZE;
  }else if(playground.size == playground.numberOfBlocks){
    playground.size = playground.size * 2;
    playground.blocks = realloc(playground.blocks, playground.size * sizeof(block));
  }
  //creating a new block
  void* blockSpacePointer = malloc(size);
  void* objectsPointer = malloc(STANDARD_ARRAY_SIZE * sizeof(object));
  block newBlock = {blockSpacePointer, blockSpacePointer+size, blockSpacePointer, objectsPointer , STANDARD_ARRAY_SIZE, 0, 0};

  block* blockPointer = playground.blocks + playground.numberOfBlocks;
  *(blockPointer) = newBlock;
  playground.numberOfBlocks++;

  return blockPointer;
}

//dynamiiquement on cherche un block avec une taille plus grande
size_t getBlockSize(size_t size){
  size_t sizeOfBlock = STANDARD_BLOCK_SIZE;
  while(size > sizeOfBlock){
    sizeOfBlock = sizeOfBlock*2;
  }
  return sizeOfBlock;
}


void* saveObject(block* block, size_t sizeOfObject, int indexOfBlock){
  object information = {sizeOfObject, 1, block->currentPointer, indexOfBlock};
  block->currentPointer = block->currentPointer+sizeOfObject;
  insertInformationInBlock(information, block);
  return information.startPointer;
}

void insertInformationInBlock(object information, block* block){
  if(block->size == block->numberOfObjects){
    block->size = block->size * 2;
    block->objects = realloc(block->objects, block->size * sizeof(object));
  }
  *(block->objects + block->numberOfObjects) = information;
  block->numberOfObjects++;
}

int updateNumberOfReferences(void* objectId, block* block, int numberOfReferences){
  object* tempObjectsPointer = block->objects;
  for(int i = 0; i < block->numberOfObjects;i++){
    object* tempObject = (tempObjectsPointer+i);
    if(tempObject->startPointer == objectId){
      if(tempObject->numberOfReferences == 0){
        return 0;
      }
      tempObject->numberOfReferences += numberOfReferences;
      //on vérfie le nombre de références, si c'est 0 on 'efface' l'object
      if(tempObject->numberOfReferences == 0){
        freeMemory(block,tempObject);
      }
      return tempObject->numberOfReferences;
    }
  }
  return 0;
}

void freeMemory(block* block, object* object){
  int blockId = object->blockId;

  //il reste juste un object, celui qu'on va effacer dans le block,
  //donc on va effacer le block au complet
  if(block->numberOfObjects == 1){
    block->numberOfObjects = 0;
    free(block->objects);
    free(block->startPointer);
    playgroundBlocksAdjustement(blockId);
  }else if(block->currentPointer == (object->startPointer + object->size)){
    block->currentPointer = object->startPointer;
    block->numberOfObjects--;
  }else{
    //l'object n'est pas a la fin du block, doncon le laisse dans le block avec le nombre de référence a 0, comme ca
    //par la suite on pourra le remplir avec une autre allocation et economiser les apelle a malloc et free
    block->freeSpaces++;
  }
}

//Fait que si un block du milieu est effacer de decaler de 1 les prochains
void playgroundBlocksAdjustement(int blockId){
  if(blockId+1 != playground.numberOfBlocks){
    for(int i = blockId; i < playground.numberOfBlocks-1; i++){
      *(playground.blocks+i) = *(playground.blocks+i+1);
    }
  }
  playground.numberOfBlocks--;
  //reallocation de l'array dans playground si le nombre est de moins de 25%
  if(playground.size > STANDARD_ARRAY_SIZE && playground.size/4 == playground.numberOfBlocks){
    playground.blocks = realloc(playground.blocks, playground.size/2 * sizeof(block));
  }
}

//cherhce un espace libre dans les blocks
void* searchFreeSpaceAndInsert(size_t size){
  block* tempPointer = playground.blocks;
  for(int i = 0; i < playground.numberOfBlocks;i++){
    block* tempBlock = (tempPointer + i);
    unsigned int freeSpaces = tempBlock->freeSpaces;
    if(freeSpaces > 0){
      for(int j = 0; j < tempBlock->numberOfObjects; j++){
        object* tempObject = (tempBlock->objects + j);
        if(tempObject->numberOfReferences == 0 && tempObject->size >= size){
          if(tempObject->size - size == 0){
            tempObject->numberOfReferences = 1;
            tempBlock->freeSpaces--;
            return tempObject->startPointer;
          }else{
            object newObject = {size, 1, tempObject->startPointer, tempObject->blockId};
            insertInformationInBlock(newObject, tempBlock);
            tempObject->startPointer = tempObject->startPointer+size;
            tempObject->size = tempObject->size - size;
            return newObject.startPointer;
          }
        }else if(tempObject->numberOfReferences == 0){
          freeSpaces--;
        }
        if(freeSpaces == 0){
          break;
        }
      }
    }
  }
  return NULL;
}
