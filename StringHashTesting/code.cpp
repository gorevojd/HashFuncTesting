#include <iostream>
#include <Windows.h>
#include <time.h>

using namespace std;

typedef int b32;
typedef unsigned int u32;
typedef unsigned long long u64;

typedef u32 (*hash_func_prot)(char* Text);

LARGE_INTEGER PerformanceFreqLI;
float OneOverPerformanceFreq;

#define SCOUNT ((sizeof(Symbols) / sizeof(char)) - 1)
char Symbols[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz1234567890";

int RandomStrCount;
char** RandomStrArray;
int HashTableSize;
int* HashTable;

inline int StringLength(char* Text) {
	int Res = 0;

	char* At = Text;
	while (*At) {
		Res++;

		At++;
	}

	return(Res);
}

u32 StringHashFNV(char* Name) {
	u32 Result = 2166136261;

	char* At = Name;
	while (*At) {

		Result *= 16777619;
		Result ^= *At;

		At++;
	}

	return(Result);
}

u32 StringHashMy(char* Name) {
	u32 Result = 369123691;

	char* At = Name;
	while (*At) {
		char AtChar = *At;

		Result *= 369123539;
		Result ^= (AtChar * 4000000531);

		At++;
	}

	return(Result);
}

static inline u32 murmur_32_scramble(u32 k) {
	k *= 0xcc9e2d51;
	k = (k << 15) | (k >> 17);
	k *= 0x1b873593;
	return k;
}

static u32 murmur3_32(uint8_t* key, size_t len, u32 seed)
{
	u32 h = seed;
	u32 k;
	/* Read in groups of 4. */
	for (size_t i = len >> 2; i; i--) {
		// Here is a source of differing results across endiannesses.
		// A swap here has no effects on hash properties though.
		k = *((u32*)key);
		key += sizeof(u32);
		h ^= murmur_32_scramble(k);
		h = (h << 13) | (h >> 19);
		h = h * 5 + 0xe6546b64;
	}
	/* Read the rest. */
	k = 0;
	for (size_t i = len & 3; i; i--) {
		k <<= 8;
		k |= key[i - 1];
	}
	// A swap is *not* necessary here because the preceeding loop already
	// places the low bytes in the low places according to whatever endianess
	// we use. Swaps only apply when the memory is copied in a chunk.
	h ^= murmur_32_scramble(k);
	/* Finalize. */
	h ^= len;
	h ^= h >> 16;
	h *= 0x85ebca6b;
	h ^= h >> 13;
	h *= 0xc2b2ae35;
	h ^= h >> 16;
	return h;
}

#define mmix(h,k) { k *= m; k ^= k >> r; k *= m; h *= m; h ^= k; }

u32 MurmurHash2A(uint8_t* data, int len, unsigned int seed)
{
	u32 m = 0x5bd1e995;
	const int r = 24;
	u32 l = len;

	u32 h = seed;
	u32 k;

	while (len >= 4)
	{
		k = *(u32*)data;

		mmix(h, k);

		data += 4;
		len -= 4;
	}

	u32 t = 0;

	switch (len)
	{
		case 3: t ^= data[2] << 16;
		case 2: t ^= data[1] << 8;
		case 1: t ^= data[0];
	};

	mmix(h, t);
	mmix(h, l);

	h ^= h >> 13;
	h *= m;
	h ^= h >> 15;

	return h;
}

u32 StringHashMurMur(char* Name) {
	int Len = StringLength(Name);
	u32 Result = murmur3_32((uint8_t*)Name, Len, 1234);

	return(Result);
}

u32 StringHashMurMur2(char* Name) {
	int Len = StringLength(Name);
	u32 Result = MurmurHash2A((uint8_t*)Name, Len, 1234);

	return(Result);
}

static void GenerateRandomString(char* Str, int Size) {
	int Scount = SCOUNT;
	for (int i = 0; i < Size - 1; i++) {
		char RandC = Symbols[rand() % Scount];
		Str[i] = RandC;
	}
	Str[Size - 1] = 0;
}

static void ClearHashTable(int* Table, int Size) {
	for (int i = 0; i < Size; i++) {
		Table[i] = -1;
	}
}

void TestAlgo(
	char* AlgoName,
	hash_func_prot HashFunc) 
{
	ClearHashTable(HashTable, HashTableSize);

	LARGE_INTEGER BeginClockLI;
	QueryPerformanceCounter(&BeginClockLI);

	int TotalCollisionCount = 0;;

	//NOTE(dima): Calculating hash and fake inserting
	for (int i = 0; i < RandomStrCount; i++) {
		u32 Hash = HashFunc(RandomStrArray[i]);
		u32 Key = Hash % HashTableSize;

		HashTable[Key]++;
	}

	//NOTE(dima): Calculating total collision count
	for (int i = 0; i < HashTableSize; i++) {
		if (HashTable[i] >= 0) {
			TotalCollisionCount += HashTable[i];
		}
	}

	LARGE_INTEGER EndClockLI;
	QueryPerformanceCounter(&EndClockLI);
	u64 ClocksElapsed = EndClockLI.QuadPart - BeginClockLI.QuadPart;
	float DeltaTime = (float)ClocksElapsed * OneOverPerformanceFreq;

	char Buf[256];
	sprintf(Buf, "Algo: %s; Total collisions: %d; Time: %.4fsec", AlgoName, TotalCollisionCount, DeltaTime);
	printf("%s\n", Buf);
}

int main(int ArgsCount, char** Args) {
	srand((unsigned int)time(0));

	QueryPerformanceFrequency(&PerformanceFreqLI);
	OneOverPerformanceFreq = 1.0f / (float)PerformanceFreqLI.QuadPart;

	//NOTE(dima): Generating random strings
#define RANDOM_STRING_SIZE 128
	RandomStrCount = 100000;
	RandomStrArray = (char**)malloc(sizeof(char*) * RandomStrCount);
	char* RandomStrBlock = (char*)malloc(RandomStrCount * sizeof(char) * RANDOM_STRING_SIZE);
	for (int RandomStrIndex = 0;
		RandomStrIndex < RandomStrCount;
		RandomStrIndex++)
	{
		char** Array = &RandomStrArray[RandomStrIndex];
		*Array = RandomStrBlock + RandomStrIndex * RANDOM_STRING_SIZE;

		GenerateRandomString(*Array, RANDOM_STRING_SIZE);
	}

	//NOTE(dima): Generating hash table
	/*
		Hash table will contain only ints that count
		collision count on corresponding keys;
	*/
	HashTableSize = 100000;
	HashTable = (int*)malloc(sizeof(int) * HashTableSize);
	ClearHashTable(HashTable, HashTableSize);


	//NOTE(dima): Testing
	printf("Table size: %d\n", HashTableSize);
	printf("Random strings count: %d\n", RandomStrCount);
	printf("String len: %d\n", RANDOM_STRING_SIZE - 1);

	TestAlgo("FNV", StringHashFNV);
	TestAlgo("My", StringHashMy);
	TestAlgo("MurMur", StringHashMurMur);
	TestAlgo("MurMur2", StringHashMurMur2);

	//NOTE(dima): Freing
	free(HashTable);
	free(RandomStrBlock);
	free(RandomStrArray);

	system("pause");
	return(0);
}