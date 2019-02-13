#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <stdint.h>
#include <time.h>
#include <unistd.h>
#include <sys/mman.h>
#include <pthread.h>
#include <fcntl.h>
#include <sys/shm.h> 
#include <sys/io.h>
#include <dlfcn.h>
#include <errno.h>
//#include"cache.h"
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <inttypes.h>
#include <stdbool.h>
#include "util.h"

#define ASM_RDTSC
#define ASM_LOAD
#define MEASUREMENTS 100

uint32_t timing[MEASUREMENTS];
uint64_t* trojan_addr[L2_CACHEASSOC];
uint64_t spy_dest, trojan_dest, noise_dest;
uint64_t* eviction_set[L2_CACHEASSOC];
uint64_t* noise_set[L2_CACHEASSOC*2];
int trojan_threads=0;
bool bitflip = false;
bool trojan_flag = false;
bool stop = false;
int tm_index=0;
uint64_t e2,s2;
bool iter_done = false;
bool spy_order=true;
int noise_index=0;

pthread_t trojan_t;
pthread_t spy_t;
pthread_t master_t;
pthread_t noise_t;
int thread_args[4]={1,2,3,4};
pthread_mutex_t count_mutex=PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t count_mutex_2=PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t condition_cond=PTHREAD_COND_INITIALIZER;
pthread_cond_t condition_cond_2=PTHREAD_COND_INITIALIZER;

__attribute__((always_inline)) void spy_kernel(){
    //register uint64_t spy_dest = 0;
    rdtsc(&s2);

    spy_dest += *eviction_set[0];
    spy_dest += *eviction_set[1];
    spy_dest += *eviction_set[2];
    spy_dest += *eviction_set[3];
    spy_dest += *eviction_set[4];
    spy_dest += *eviction_set[5];
    spy_dest += *eviction_set[6];
    spy_dest += *eviction_set[7];
    __asm__ __volatile__("lfence"); 
    
    rdtsc(&e2);
    //printf("S\n");
    //spy_order=!spy_order;
    timing[tm_index++] = e2-s2;
    printf("%d: %d\n", tm_index-1,e2-s2);
}
__attribute__((always_inline)) void noise_kernel(){
         uint64_t temp_e2, temp_s2;
        int id=0;
        //printf("noise\n");
        register uint8_t* gold __asm__("r8");
            
               gold =(uint8_t*) malloc(sizeof(uint8_t)*NUM_PAGES*1024);
            rdtsc(&temp_s2);
            rdtsc(&temp_e2);
            free(gold);
            while (temp_e2-temp_s2<10000){
                 rdtsc(&temp_e2);
            }
        
}
__attribute__((always_inline)) void trojan_kernel(){
//    register uint64_t trojan_dest = 0;
    
    if(bitflip){
        printf("trojan:transmit 1\n");
        trojan_dest += *trojan_addr[0];
        trojan_dest += *trojan_addr[1];
        trojan_dest += *trojan_addr[2];
        trojan_dest += *trojan_addr[3];
        trojan_dest += *trojan_addr[4];
        trojan_dest += *trojan_addr[5];
        trojan_dest += *trojan_addr[6];
        trojan_dest += *trojan_addr[7];
        __asm__ __volatile__("lfence"); 
    }else{
        printf("trojan:transmit 0\n"); 
    }
    //uint32_t i;
    //rdtsc(&s2);
    //for (i=0; i<1000; i++){
    //    static int foo=0;
     //   foo++;
      //  wait(1);
    //}
    //rdtsc(&e2);
    //printf(e2-s2);
    //printf("T\n");
    //else
    //printf("trojan:transmit 0\n");
}

void trojan_thread(){
    int i=0;
    for(i=0; i<MEASUREMENTS; i++){
            pthread_mutex_lock(&count_mutex);
	    if (rand()/(float)RAND_MAX>0.5){
                bitflip = !bitflip;
            }
	    if(bitflip){
		printf("trojan:transmit 1\n");
		trojan_dest += *trojan_addr[0];
		trojan_dest += *trojan_addr[1];
		trojan_dest += *trojan_addr[2];
		trojan_dest += *trojan_addr[3];
		trojan_dest += *trojan_addr[4];
		trojan_dest += *trojan_addr[5];
		trojan_dest += *trojan_addr[6];
		trojan_dest += *trojan_addr[7];
		__asm__ __volatile__("lfence"); 
	    }else{
		printf("trojan:transmit 0\n"); 
	    }
            pthread_mutex_lock(&count_mutex_2);
            trojan_flag=false;
	    pthread_cond_signal(&condition_cond);
            pthread_mutex_unlock(&count_mutex);
            while (!trojan_flag){
                pthread_cond_wait(&condition_cond_2, &count_mutex_2);
            }
            pthread_mutex_unlock(&count_mutex_2);
    }
    //printf("ending T\n");
    return (NULL);
}

void spy_thread(){
    int i=0; 
    for (i=0; i<MEASUREMENTS; i++){
            pthread_mutex_lock(&count_mutex);
            while (trojan_flag){
                pthread_cond_wait(&condition_cond, &count_mutex);
            }
	    rdtsc(&s2);
	    spy_dest += *eviction_set[0];
	    spy_dest += *eviction_set[1];
	    spy_dest += *eviction_set[2];
	    spy_dest += *eviction_set[3];
	    spy_dest += *eviction_set[4];
	    spy_dest += *eviction_set[5];
	    spy_dest += *eviction_set[6];
	    spy_dest += *eviction_set[7];
	    __asm__ __volatile__("lfence");
	    rdtsc(&e2);
	    timing[tm_index++] = e2-s2;
	    //printf("%d: %d\n", tm_index-1,e2-s2);
            pthread_mutex_lock(&count_mutex_2);
            trojan_flag=true;
            pthread_cond_signal(&condition_cond_2);
            pthread_mutex_unlock(&count_mutex);
            pthread_mutex_unlock(&count_mutex_2);
    }
    //printf("ending S\n");
    return (NULL);
}

void noise_thread(){
    while(!stop){
         
	 noise_kernel();
        //break;
    }
}


int main (int argc,char  **argv){
    register uint8_t* gold __asm__("r8");
    uint8_t * gold_shadow;
    uint64_t tmp;
    uint32_t i,j;
    uint32_t count;
    char path_buf[0x100]={};
    uint32_t pfn;

    //8 bits between cache bits and page offset bit boundary
    //PFN: 53-----6------1 + Pageoffset 12---7----1
    //      |     |__________________________|____|
    //                   L2 cache index    block offset
    //page frames sharing the same first 6 bits in PFN would map to 
    //the same L2 cache set
    uint32_t eviction_groups[NUM_GROUPS]={0};
    /*allocate 4MB(1024 pages) to find eviction set to cover L2 eight times
     * this is just heuristics
     */
    uint32_t pfn_set[NUM_PAGES]={0};

    sprintf(path_buf, "/proc/self/pagemap");

    //allocate 8 Mbytes of memory
    gold =(uint8_t*) malloc(sizeof(uint8_t)*NUM_PAGES*4096);
    gold_shadow = gold;
    //*gold_shadow=0;
    for(i=0;i<NUM_PAGES;i++){
        *((uint64_t*)(gold_shadow+i*4096))=1;
    }
    for(i=0;i<NUM_PAGES;i++){
        pfn = read_pagemap(path_buf, gold_shadow+i*4096);
        pfn_set[i] = pfn;
        count = eviction_groups[pfn&0x3F];
        eviction_groups[pfn&0x3F] = count+1;
    }

    //for(i=0;i<NUM_GROUPS;i++){
    //    printf("index%d, %d\n", i, eviction_groups[i]);
    //} 
    int group=0;
    for(i=0;i<NUM_GROUPS;i++){
        tmp =0;
        /*find pfns that would form a eviction set*/
        if(group==0 && eviction_groups[i]>L2_CACHEASSOC*2-1){
            for(j = 0;j < NUM_PAGES;j++){
                if((pfn_set[j]&0x3F) == i){
                    
                    if(tmp<L2_CACHEASSOC){
                        trojan_addr[tmp] = (uint64_t *)(gold_shadow + j * 4096);
                        printf("%d: trojan pfn: %p\n",tmp, pfn_set[j]);}
                    else if (tmp<L2_CACHEASSOC*2){
                        eviction_set[tmp-L2_CACHEASSOC] = (uint64_t*)(gold_shadow + j * 4096);
                        printf("%d: spy pfn: %p\n", tmp-L2_CACHEASSOC,  pfn_set[j]);}
                    else
                        noise_set[tmp-L2_CACHEASSOC*2] = (uint64_t*)(gold_shadow + j * 4096);
                    tmp++;
                    if(tmp > L2_CACHEASSOC*2-1)
                        break;
                }
            } 
            group++;
             continue;
        }
        if (group==1 && eviction_groups[i]>L2_CACHEASSOC*2-1){
           
           for(j = 0;j < NUM_PAGES;j++){
                if((pfn_set[j]&0x3F) == i){
                    printf("pfn: %p\n", pfn_set[j]);
                   
                    noise_set[tmp] = (uint64_t*)(gold_shadow + j * 4096);
                    tmp++;
                    if(tmp > L2_CACHEASSOC*2-1)
                        break;
                }
            }  
            break;
          
        }
    }

    if(!trojan_addr){
        printf("could not find eviction set\n");
    }
    else{
        //printf("trojan_addr at %p\n",trojan_addr);
        //for(i=0;i<L2_CACHEASSOC;i++){
        //    printf("eviction address i:%d at %p\n", i, eviction_set[i]);
        //}
    }

    pthread_create(&trojan_t, NULL,trojan_thread, &thread_args[0]);
    set_affinity(trojan_t, 1, &thread_args[0]);
    pthread_create(&spy_t, NULL,spy_thread, &thread_args[1]);
    set_affinity(spy_t, 2, &thread_args[1]);
    //pthread_create(&noise_t, NULL,noise_thread, &thread_args[2]);
    //set_affinity(noise_t, 3,&thread_args[2]);
    pthread_t master_t = pthread_self();
    set_affinity(master_t, 0, "main thread");

    /*
    ///single thread implementation
    for(i=0;i<MEASUREMENTS;i++){
        trojan_kernel();
        rdtsc(&s2);
        spy_kernel();
        rdtsc(&e2);
        timing[i] = e2 - s2;
    }
    */

    trojan_flag = true;
    //stop=false;
   
    //printf("miemiemie");
    pthread_join(spy_t, NULL);
    pthread_join(trojan_t, NULL);
    //pthread_join(noise_t, NULL);

    for(i=0;i<MEASUREMENTS;i++){
       printf("%d\n", timing[i]);
    }
    m5_exit(0);
    return 0;
}
