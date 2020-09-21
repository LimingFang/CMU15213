#include "cachelab.h"
#include "unistd.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdbool.h>
#include <getopt.h>

/*
	we should first write the code for getopt from stdin
	then define the struct for cache_line
		construct a struct array[S][E]
		the procedure is:
			- init the cache
			- read the trace file
			- in a loop,read line by line
			- judge the cache hit or miss
			- update the lru_counter


*/
int s,E,b;
char t[100];		//contain the trace file;
int hit = 0;
int miss = 0;
int evictions = 0;

typedef struct{
	int valid_bit;
	long tag;
	int lru_counter;
}cache_line;

void init_cache(cache_line**);
void parse_trace(FILE*,cache_line**,bool);
int check(cache_line**,long);
void update_counter(cache_line**);


int main(int argv,char** argc)
{
	int opt_c;
	bool verbose = false;
	while((opt_c = getopt(argv,argc,"hvs:E:b:t:"))!=-1){
		switch(opt_c){
			case 'h':
				printf("Usage: ./csim-ref [-hv] -s <num> -E <num> -b <num> -t <file>\n");
				printf("Options:\n");
				printf("\t-h		Print this help message.\n");
				printf("\t-v		Optional verbose flag.\n");
				printf("\t-s <num>  Number of set index bits.\n");
				printf("\t-E <num>  Number of lines per set.\n");
				printf("\t-b <num>  Number of block offset bits.\n");
				printf("\t-t <file> Trace file.\n");
				return 0;
			case 's':
				//we need to change the char* to int
				//use atoi
				s = atoi(optarg);
				break;
			case 'E':
				E = atoi(optarg);
				break;
			case 'b':
				b = atoi(optarg);
				break;
			case 't':
				strcpy(t,optarg);
				break;
			case 'v':
				verbose = true;
				break;
			default:
				break;
		}
	}
	//printf("s is %d,E is %d,b is %d,file is %s",s,E,b,t);


	//cache_line struct,we should malloc the space manually
	cache_line** cache = (cache_line**)malloc(sizeof(cache_line*)*pow(2,s));
	if(!cache){
		printf("fail to malloc the cache space\n");
		return 0;
	}
	for(int i=0;i!=pow(2,s);i++){
		cache[i] = (cache_line*)malloc(sizeof(cache_line)*E);
	}
	init_cache(cache);
	FILE* file = fopen(t,"r");
	if(!file)
		return 0;
	parse_trace(file,cache,verbose);
	for(int i=0;i!=pow(2,s)*E;i++)
		free(cache[i]);
    printSummary(hit, miss, evictions);
    return 0;
}	

void init_cache(cache_line** cache){
	for(int i=0;i!=pow(2,s);i++){
		for(int j=0;j!=E;j++){
			cache[i][j].valid_bit = 0;
			cache[i][j].tag = 0;
			cache[i][j].lru_counter = 0;
		}
	}
}

void parse_trace(FILE* file,cache_line** cache,bool verbose){
	char c[100];
	char ins;
	long addr;
	long size;

	int ret;
	while(fgets(c,100,file)){
		sscanf(c," %c %lx %ld",&ins,&addr,&size);
		if(ins=='I')
			continue;
		else if(ins=='M'){
			//M means that we should fetch the data first and then save it back
			ret = check(cache,addr);
			hit++;
		}
		else if(ins=='S'){
			ret = check(cache,addr);
		}
		else{
			//ins == `L`
			ret = check(cache,addr);
		}
		if(verbose)
			switch (ret){
				case 0:
					printf("%c %lx, hit\n",ins,addr);
					break;
				case 1:
					printf("%c %lx, eviction\n",ins,addr);
					break;
				case 2:
					printf("%c %lx, miss\n",ins,addr);
					break;
				default:
					printf("error\n");
			}
	}
}

int check(cache_line** cache,long addr){
	//get the tag\set\offset
	int i;
	int tag = addr>>(s+b);
    int mask = -1;
    mask = ~(mask<<s);
    unsigned int set = (addr>>b)&mask;

	//judge if the cache has the data
	//first we scan lines in certain set,checking their valid_bit and tag
	bool in_cache = false;
	unsigned int least_freq_use_idx = -1;
	unsigned int least_freq_use_counter = 0;

	update_counter(cache);

	for(i=0;i!=E;i++){
		if(cache[set][i].valid_bit&&cache[set][i].lru_counter>least_freq_use_counter)
			least_freq_use_idx = i;
		if(!(cache[set][i].valid_bit)){
			//there exsits an unused cache line,so our data isn't in the cache
			break;
		}
		if(cache[set][i].valid_bit&&cache[set][i].tag==tag){
			in_cache = true;
			break;
		}
	}
	if(in_cache){
		hit++;
		return 0;
	}
	else{
		miss++;
		if(i==E){
			evictions++;
			cache[set][least_freq_use_idx].tag = tag;
			return 1;
		}
		else{
			cache[set][i].valid_bit = 1;
			cache[set][i].tag = tag;
			cache[set][i].lru_counter = 0;
			return 2;
		}
	}
}

void update_counter(cache_line** cache){
	for(int i=0;i!=pow(2,s);i++){
		for(int j=0;j!=E;j++){
			cache[i][j].lru_counter++;
		}
	}
}