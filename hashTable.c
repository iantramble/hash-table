//Ian Tramble Project 3B
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "HashTable.h"

struct node //same as tree node
{
	char *key;
	void* data;
	struct node *right, *left;
};

typedef struct node node;

typedef node *nodePTR; //nodePTR is a pointer to a node

struct HashTableObjectTag
{
	int sentinel;
	unsigned int size;
	struct node **hashTable; //pointer array of pointers to nodes
	unsigned int numberofentries;
	unsigned int useFactorNumerator;
	unsigned int largestBucketSize;
	int dynamicBehaviour;
	float expandUseFactor;
	float contractUseFactor;
};

//define hash function
//sums the value of each character in the string and returns the remainder of that divided by range, the size of the hash

unsigned int SimpleStringHash(char *value, unsigned int range)
{
	unsigned int count = 0;
	while (*value != '\0') {
		count+= (unsigned int)(*value);
		value++;
	}
	unsigned int result = count % range;
	return result;
}

int CreateHashTable(HashTablePTR *hashTableHandle, unsigned int initialsize)
{
	//Note that hashTableHandle is a pointer to a pointer to a HashTableObject (i.e. it's a double pointer)
	//allocate memory for HashTableObject
	*hashTableHandle = (HashTableObject *)malloc(sizeof(HashTableObject));

	//check if there was enough space to allocate
	if (*hashTableHandle == NULL) return -1;

	//allocate memory for the actual hashTable
	(*hashTableHandle)->hashTable = calloc(initialsize, sizeof(nodePTR));
	//check if there was enough space
	if ((*hashTableHandle)->hashTable == NULL) return -1;

	//initialize
	(*hashTableHandle)->sentinel = (int)0xDEADBEEF;
	(*hashTableHandle)->size = initialsize;
	(*hashTableHandle)->numberofentries = 0;
	(*hashTableHandle)->largestBucketSize = 0;
	(*hashTableHandle)->dynamicBehaviour = 1;
	(*hashTableHandle)->expandUseFactor = (float)0.7;
	(*hashTableHandle)->contractUseFactor = (float)0.2;
	//(*hashTableHandle)->loadFactor = 0;
	(*hashTableHandle)->useFactorNumerator = 0;
	
	return 0;
}

//DeleteNode function from BINARY TREE
int DeleteNode(nodePTR *root, char *key, void **dataPointer)
{
	if (*root == NULL) return -1;

	if (strcmp(key, (*root)->key) < 0)
		return DeleteNode(&((*root)->left), key, dataPointer);
	else if (strcmp(key, (*root)->key) > 0)
		return DeleteNode(&((*root)->right), key, dataPointer);
	else {
		nodePTR tmp, smallest, prev;
		*dataPointer = (*root)->data;
		if ((*root)->left == NULL && (*root)->right == NULL) {
			free((*root)->key);
			free(*root);
			*root = NULL;
		}
		else if ((*root)->right == NULL) {
			tmp = *root;
			*root = (*root)->left;
			free(tmp->key);
			free(tmp);
			tmp = NULL;
		}
		else if ((*root)->left == NULL) {
			tmp = *root;
			*root = (*root)->right;
			free(tmp->key);
			free(tmp);
			tmp = NULL;
		}
		else { //find smallest to the right and its predecessor
			for (prev = *root, smallest = (*root)->right;
			smallest->left != NULL; prev = smallest,
			smallest = smallest->left);
			
			tmp = smallest;
			(*root)->key = smallest->key;
			if (smallest->data > prev->data)
				prev->right = smallest->right;
			else
				prev->left = smallest->right;
			free(tmp->key);
			free(tmp);
			tmp = NULL;
		}
		return 0;
	}
}

//DestroyHashTable remix ft. DeleteNode
int DestroyHashTable(HashTablePTR *hashTableHandle)
{
	//check if first four bytes of hashTableHandle contain sentinel
	if (*((int *)*hashTableHandle) != 0xDEADBEEF) return -1;

	//first we need to destroy copies of keys
	//then linked nodes
	//then the hashTable itself
	//then the HashTableObject

	char **keysArrayHandle;
	unsigned int keyCount;
	void *dataPointer;
	GetKeys(*hashTableHandle, &keysArrayHandle, &keyCount);

	for (int i = 0; i < (*hashTableHandle)->numberofentries; i++) {
		unsigned int hashindex = SimpleStringHash(keysArrayHandle[i], (*hashTableHandle)->size);

	DeleteNode((&((*hashTableHandle)->hashTable)[hashindex]), keysArrayHandle[i],&dataPointer);
	free(keysArrayHandle[i]);
	}
	
	//Free spree
	free(keysArrayHandle);
	keysArrayHandle = NULL;
	free((*hashTableHandle)->hashTable); //destroy the hashTable!
	free(*hashTableHandle); //free the HashTableObject!
	*hashTableHandle = NULL;
	hashTableHandle = NULL;

	return 0;
} //"I'm free baby. And I'm never coming back. Never!" -HashTableHandle

//PrintInOrder from BINARY TREE
//need it to iterate through all of the leafs in a tree
//currently being used to get all of the keys in GetKeys
void PrintInOrder(nodePTR root, int *entry, char **keysArrayHandle)
{
	if (root == NULL) return;
	char *straddress;
	PrintInOrder(root->left, entry, keysArrayHandle);

	straddress = malloc(sizeof(char) * (strlen(root->key) + 1));
	if (straddress == NULL) return; //check sufficient mem
	strcpy(straddress, root->key);
	keysArrayHandle[*entry] = straddress;
	*entry += 1;
	straddress = NULL;
	PrintInOrder(root->right, entry, keysArrayHandle);
	return;
}

int GetKeys(HashTablePTR hashTable, char ***keysArrayHandle, unsigned int *keyCount)
{
	//keysArrayHandle pointer to pointer to array of char pointers
	if (*((int*)hashTable) != 0xDEADBEEF) return -1;

	nodePTR root;
	int i, entry = 0;
	*keyCount = hashTable->numberofentries;

	//declare array of pointers to char pointers
	*keysArrayHandle = malloc((*keyCount) * sizeof(char *));
	if (*keysArrayHandle == NULL) return -2; //sufficient mem?

	for (i = 0; i < hashTable->size; i++) {
		root = (hashTable->hashTable)[i];
		PrintInOrder(root, &entry, *keysArrayHandle);
	}
	return 0;
}

//function to create a new node with key, right, left, and data
nodePTR createNode(char *key, void *data)
{
	nodePTR newnode = malloc(sizeof(node));
	char *straddress = malloc(sizeof(char) * (strlen(key) + 1));
	if (newnode == NULL || straddress == NULL) return NULL;
	strcpy(straddress, key);
	newnode->key = straddress;
	newnode->data = data;
	newnode->right = NULL;
	newnode->left = NULL;

	return newnode;
}

//insert function for BINARY TREE
void Insert(nodePTR *root, char *key, nodePTR newnode)
{
	if (*root == NULL)
		*root = newnode;
	if (strcmp(key, (*root)->key) < 0)
		Insert(&(*root)->left, key, newnode);
	else if (strcmp(key, (*root)->key) > 0)
		Insert(&((*root)->right), key, newnode);
}

//FindEntry function for BINARY TREE
nodePTR FindItem(nodePTR root, char *key)
{
	if (root == NULL) return NULL;

	nodePTR cur = root;
	while (cur != NULL && strcmp(cur->key, key)) {
		if (strcmp(key, cur->key) < 0)//(data < cur->data)
			cur = cur->left;
		else if (strcmp(key, cur->key) > 0)
			cur = cur->right;
	}

	if (cur == NULL) return NULL;

	return cur;
}

//used to get the largest bucket size
int GetCount(nodePTR root, unsigned int *count)
{
	if (root == NULL) return -1;
	*count += 1;
	GetCount(root->left, count);
	GetCount(root->right, count);
	
	return 0;
}

//used to get the largest bucket size
unsigned int GetLargestBucketSize(HashTablePTR hashTable)
{
	unsigned int count;
	for (int i = 0; i < hashTable->size; i++) {
		count = 0;
		GetCount((hashTable->hashTable)[i], &count);
		if (count > hashTable->largestBucketSize)
			hashTable->largestBucketSize = count;
	}

	return 0;
}

nodePTR *Copy(HashTablePTR *hashTable, unsigned int initialsize)
{
	HashTablePTR newHashTable;
	CreateHashTable(&newHashTable, initialsize);
	newHashTable->dynamicBehaviour = 0;
	char **keysArrayHandle;
	unsigned int keyCount;
	void *dataPointer;
	void *previousDataHandle = NULL; //unused
	GetKeys(*hashTable, &keysArrayHandle, &keyCount);

	
	for (int i = 0; i < (*hashTable)->numberofentries; i++) {
		unsigned int hashindex = SimpleStringHash(keysArrayHandle[i], (*hashTable)->size);
		DeleteNode(&(((*hashTable)->hashTable)[hashindex]), keysArrayHandle[i],&dataPointer);
		InsertEntry(newHashTable, keysArrayHandle[i], dataPointer, &previousDataHandle);
		free(keysArrayHandle[i]);
	}

	free(keysArrayHandle);
	nodePTR *result = newHashTable->hashTable;
	(*hashTable)->size = newHashTable->size;
	(*hashTable)->useFactorNumerator = newHashTable->useFactorNumerator;	
	free(newHashTable);

	return result;
}

//InsertEntry remix ft. Insert, Copy, Macklemore, & Ryan Lewis
int InsertEntry(HashTablePTR hashTable, char *key, void *data, void **previousDataHandle)
{
	int result;
	if (*((int*)hashTable) != 0xDEADBEEF) return -1;
	unsigned int hashindex = SimpleStringHash(key, hashTable->size);
	
	//Check if same key already exists
	//use FindItem to find the largestBucketSize
	nodePTR sameKey = FindItem((hashTable->hashTable)[hashindex], key);
	if (sameKey != NULL) { //the key already exists. don't need to insert anything.
		*previousDataHandle = sameKey->data;
		sameKey->data = data;
		result = 2;
	}
	else { //key does not exist. insert where appropriate.
		nodePTR newnode = createNode(key, data);
		if (newnode == NULL) result = -2; //insufficient memory

		if ((hashTable->hashTable)[hashindex] == NULL) {
			(hashTable->hashTable)[hashindex] = newnode;
			hashTable->numberofentries += 1;
			hashTable->useFactorNumerator += 1;
			result = 0; //cleanly inserted; no collision
		}
		else { //now we're dealing with a hash collisionInsert(&((hashTable->hashTable)[hashindex]), key, newnode);
			hashTable->numberofentries += 1;
			result = 1; //return 1 for hash collision
		}
	}

	//now let's check if we need to resize
	if (hashTable->dynamicBehaviour == 1 && (float)((float)hashTable->useFactorNumerator / (float)hashTable->size) > hashTable->expandUseFactor) {
		nodePTR *tmp = Copy(&hashTable, (hashTable->size) * 2); //just increase size by factor of two because expandUseFactor is checked independently of contractUseFactor
		free(hashTable->hashTable);
		hashTable->hashTable = tmp;
	}

	GetLargestBucketSize(hashTable);
	
	return result; //there was a hash collision
}

int DeleteEntry(HashTablePTR hashTable, char *key, void **dataHandle)
{
	if (*((int*)hashTable) != 0xDEADBEEF) return -1;
	unsigned int hashindex = SimpleStringHash(key, hashTable->size);

	//check if key exists in binary tree of appropriate hash
	nodePTR sameKey = FindItem((hashTable->hashTable)[hashindex], key);
	if (sameKey == NULL)return -2; //key was not found

	//check if key is in first node. If so, decrement hashTable->useNumerator
	if (!strcmp(((hashTable->hashTable)[hashindex])->key, key))
		hashTable->useFactorNumerator -= 1;

	DeleteNode(&((hashTable->hashTable)[hashindex]), key, dataHandle);
	hashTable->numberofentries -= 1;

	//now let's check if we need to resize
	if (hashTable->dynamicBehaviour == 1 && (float)((float)hashTable->useFactorNumerator /(float)hashTable->size) < hashTable->contractUseFactor && hashTable->size > 1) {
		nodePTR *tmp = Copy(&hashTable, (hashTable->size) / 2);
		free(hashTable->hashTable);
		hashTable->hashTable = tmp;
	}		
	GetLargestBucketSize(hashTable);
	
	return 0; //deleted successfully
}

int FindEntry(HashTablePTR hashTable, char *key, void **dataHandle)
{
	if (*((int*)hashTable) != 0xDEADBEEF) return -1;
	unsigned int hashindex = SimpleStringHash(key, hashTable->size);

	nodePTR cur = (hashTable->hashTable)[hashindex];
	
	while (cur != NULL && strcmp(cur->key, key)) {
		if (strcmp(key, cur->key) < 0)//(data < cur->data)
			cur = cur->left;
		else if (strcmp(key, cur->key) > 0)
			cur = cur->right;
	}

	if (cur == NULL) return -2;
	else
		*dataHandle = cur->data;

	return 0;
}

int GetHashTableInfo(HashTablePTR hashTable, HashTableInfo *pHashTableInfo)
{
	if (*((int*)hashTable) != 0xDEADBEEF) return -1;
	
	pHashTableInfo->bucketCount = hashTable->size;
	pHashTableInfo->loadFactor = (float)hashTable->numberofentries / (float)hashTable->size;
	pHashTableInfo->useFactor = (float)((float)hashTable->useFactorNumerator) / (float)(hashTable->size);
	pHashTableInfo->largestBucketSize = hashTable->largestBucketSize;
	pHashTableInfo->dynamicBehaviour = hashTable->dynamicBehaviour;
	pHashTableInfo->expandUseFactor = hashTable->expandUseFactor;
	pHashTableInfo->contractUseFactor = hashTable->contractUseFactor;
	
	return 0;
}