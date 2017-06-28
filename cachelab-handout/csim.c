#include "cachelab.h"
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct cache_line {
	int v_bit;
	long tag;
	int counter;
};
int main(int argc, char** argv)
{
	int opt,s,E,b;
	int S;
	int h,m,e;
	char *t;
	int v_flag = 0; // arguments have v or not
	FILE * pfile;
	char identifier;
	long address;
	int size;
	struct cache_line* cache;
	int s_index, i_tag; 
	int replace_i; // the replaced block's index
	int r_flag; // resolve or not
	h = 0;
	m = 0;
	e = 0;
	 /* looping over arguments */
	 while(-1 != (opt = getopt(argc, argv, "vs:E:b:t:"))){
	 /* determine which argument it’s processing */
		 switch(opt) {
			 case 'v':
			 v_flag = 1;
			 break;
			 case 's':
			 s = atoi(optarg);
			 S = 1<<s;
			 break;
			 case 'E':
			 E = atoi(optarg);
			 break;
			 case 'b':
			 b = atoi(optarg);
			 break;
			 case 't':
			 t = optarg;
			 break;
			 default:
			 printf("wrong argument\n");
			 break;
		 }
	 } 
	cache = (struct cache_line*)malloc(sizeof(struct cache_line)*S*E);
	for (int i = 0; i < S; ++i)
	{
		for (int j = 0; j < E; ++j)
		{
			cache[i*E+j].v_bit = 0;
		}
	}
	pfile = fopen(t,"r");
	while(fscanf(pfile," %c %lx,%d", &identifier, &address, &size)>0) {
		if(v_flag && identifier!='I') printf("%c %lx,%d", identifier, address, size);
		i_tag = address>>(b+s);
		s_index = ((~((-1)<<(b+s)))&address)>>b;
		switch(identifier) {
			case 'L': 
			case 'S':
			case 'M':
				replace_i = 0;
				r_flag = 0;
				for (int k = 0; k < E; ++k)
				{
					if(cache[s_index*E+k].v_bit == 0) {
						cache[s_index*E+k].v_bit = 1;
						cache[s_index*E+k].tag = i_tag;
						cache[s_index*E+k].counter = 0;
						if(v_flag)
						printf(" miss");
						m++;
						r_flag = 1;
						break;
					}else {
						cache[s_index*E+k].counter++;
						if(cache[s_index*E+k].tag == i_tag) {
							cache[s_index*E+k].counter = 0;
							if(v_flag)
							printf(" hit");
							h++;
							r_flag = 1;
							for (k=k+1; k < E; ++k) cache[s_index*E+k].counter++;
							break;
						}
					}
				}
				if (r_flag == 0) {
					for (int k = 0; k < E; ++k)
					{
						if(cache[s_index*E+k].counter >= cache[s_index*E+replace_i].counter)
							replace_i = k;
					}
					if(v_flag)
					printf(" miss eviction");
					m++;e++;
					cache[s_index*E+replace_i].tag = i_tag;
					cache[s_index*E+replace_i].counter = 0;
				}
		}
		if (identifier == 'M')
		{
			if(v_flag)
			printf(" hit");
			h++;
		}
		if (identifier != 'I'&& v_flag)
			printf("\n");
	}
	fclose(pfile);
    printSummary(h, m, e);
    return 0;
}
