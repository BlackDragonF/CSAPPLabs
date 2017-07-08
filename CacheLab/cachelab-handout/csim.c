#include "cachelab.h"

#include "getopt.h"
#include "stdlib.h"
#include "unistd.h"

#include "string.h"

#include "stdio.h"

#include "stdint.h"

/* cache struct definition */
typedef struct CSim_Cache_Entry {
    char valid_bit;
    uint64_t tag_bit;
}CSim_Cache_Entry;

typedef struct CSim_Cache_Set{
    CSim_Cache_Entry * entries;
}CSim_Cache_Set;

typedef struct CSim_Cache {
    /* basic arguments */
    int block_offset;
    int set_number;
    int line_number;
    /* masks and offsets */
    int tag_offset;
    int set_offset;
    uint64_t tag_mask;
    uint64_t set_mask;
    /* cache */
    CSim_Cache_Set * sets;
}CSim_Cache;

typedef struct CSim_Cache_Result {
    int hit;
    int miss;
    int evict;
}CSim_Cache_Result;

/* operation type definition */
typedef enum CSIM_OPERATION_TYPE {
    CSIM_OPERATION_TYPE_NONE,
    CSIM_OPERATION_TYPE_MODIFY,
    CSIM_OPERATION_TYPE_LOAD,
    CSIM_OPERATION_TYPE_STORE,
}CSIM_OPERATION_TYPE;

/* operation result definition */
typedef enum CSIM_OPERATION_RESULT {
    CSIM_OPERATION_RESULT_MISS,
    CSIM_OPERATION_RESULT_HIT,
    CSIM_OPERATION_RESULT_MISS_EVICTION,
}CSIM_OPERATION_RESULT;

/* error definition */
typedef enum CSIM_ERROR {
    CSIM_OK = 0,
    CSIM_ERROR_INVALID_OPTION,
    CSIM_ERROR_MISSING_ARGUMENT,
    CSIM_ERROR_FILE_CANNOT_OPEN,
    CSIM_ERROR_OUT_OF_MEMORY,
}CSIM_ERROR;

#define csim_get_value(number, mask, offset) (((number) & (mask)) >> (offset))

void csim_print_help_info();
void csim_error_missing_argument();
void csim_error_file_cannot_open();
void csim_error_out_of_memory();
CSim_Cache * csim_construct_cache(int set_number, int line_number, int block_offset);
void csim_deconstruct_cache(CSim_Cache ** pcache);
CSim_Cache_Result csim_parse_trace_file(CSim_Cache * cache, FILE * file_pointer, char verbose_flag);

char * program_name = NULL;

void csim_print_help_info() {
    printf("Usage: ./csim [-hv] -s <num> -E <num> -b <num> -t <file>\n");
    printf("Options:\n");
    printf("  -h         Print this help message.\n");
    printf("  -v         Optional verbose flag.\n");
    printf("  -s <num>   Number of set index bits.\n");
    printf("  -E <num>   Number of lines per set.\n");
    printf("  -b <num>   Number of block offset bits.\n");
    printf("  -t <file>  Trace file.\n\n");
    printf("Examples:\n");
    printf("  linux>  ./csim -s 4 -E 1 -b 4 -t traces/yi.trace\n");
    printf("  linux>  ./csim -v -s 8 -E 2 -b 4 -t traces/yi.trace\n");
}

void csim_error_missing_argument() {
    printf("%s: Missing required command line argument\n", program_name);
    csim_print_help_info();
}

void csim_error_file_cannot_open() {
    printf("%s: No such file or directory\n", program_name);
}

void csim_error_out_of_memory() {
    printf("%s: Out of memory\n", program_name);
}

CSim_Cache * csim_construct_cache(int set_number, int line_number, int block_offset) {
    /* memory allocation */
    CSim_Cache * cache = malloc(sizeof(CSim_Cache));
    if (cache == NULL) {
        csim_error_out_of_memory();
        exit(CSIM_ERROR_OUT_OF_MEMORY);
    }
    /* configure basic arguments */
    cache->set_number = set_number;
    cache->line_number = line_number;
    cache->block_offset = block_offset;
    /* caculate masks and offsets */
    cache->tag_mask = ((uint64_t)0xFFFFFFFFFFFFFFFF) << (block_offset + set_number);
    cache->tag_offset = block_offset + set_number;
    cache->set_mask = ((((((uint64_t)0xFFFFFFFFFFFFFFFF) << (64 - cache->tag_offset)) >> (64 - cache->tag_offset)) >> block_offset) << block_offset);
    cache->set_offset = block_offset;
    /* memory allocation for cache */
    cache->sets = calloc((1 << set_number), sizeof(CSim_Cache_Set));
    if (cache->sets == NULL) {
        csim_error_out_of_memory();
        exit(CSIM_ERROR_OUT_OF_MEMORY);
    }
    for (int index = 0 ; index < (1 << set_number) ; ++index) {
        (cache->sets)[index].entries = calloc(line_number, sizeof(CSim_Cache_Entry));
        if ((cache->sets)[index].entries == NULL) {
            csim_error_out_of_memory();
            exit(CSIM_ERROR_OUT_OF_MEMORY);
        }
    }
    return cache;
}

void csim_deconstruct_cache(CSim_Cache ** pcache) {
    CSim_Cache * temp = *pcache;
    int set_number = temp->set_number;
    for (int index = 0 ; index < set_number ; ++index) {
        free((temp->sets)[index].entries);
    }
    free(temp->sets);
    free(temp);
    *pcache = NULL;
}

CSim_Cache_Result csim_parse_trace_file(CSim_Cache * cache, FILE * file_pointer, char verbose_flag) {
    char line[80];
    char * line_pointer = NULL;
    CSIM_OPERATION_TYPE type;
    CSIM_OPERATION_RESULT result;
    int address = 0;
    int set = 0, tag = 0;
    int line_number = cache->line_number;
    CSim_Cache_Result summary = {0, 0, 0};
    while (fgets(line, 80, file_pointer), feof(file_pointer) == 0) {
        line_pointer = line;
        type = CSIM_OPERATION_TYPE_NONE;
        if (*line_pointer++ == ' ') {
            switch(*line_pointer++) {
                case 'L':
                    type = CSIM_OPERATION_TYPE_LOAD;
                    putchar('L');
                    break;
                case 'S':
                    type = CSIM_OPERATION_TYPE_STORE;
                    putchar('S');
                    break;
                case 'M':
                    type = CSIM_OPERATION_TYPE_MODIFY;
                    putchar('M');
                    break;
                default:
                    type = CSIM_OPERATION_TYPE_NONE;
                    break;
            }
            result = CSIM_OPERATION_RESULT_MISS_EVICTION;
            sscanf(line_pointer, "%x", &address);
            while ((*line_pointer) != '\n') {
                putchar(*line_pointer++);
            }
            set = csim_get_value(address, cache->set_mask, cache->set_offset);
            tag = csim_get_value(address, cache->tag_mask, cache->tag_offset);
            CSim_Cache_Entry * pentry = (cache->sets)[set].entries;
            int index;
            CSim_Cache_Entry temp;
            for (index = 0; index < line_number ; ++index) {
                if (!pentry[index].valid_bit) {
                    result = CSIM_OPERATION_RESULT_MISS;
                    break;
                }
                if (pentry[index].tag_bit == tag) {
                    result = CSIM_OPERATION_RESULT_HIT;
                    break;
                }
            }
            switch(result) {
                case CSIM_OPERATION_RESULT_MISS:
                    if (verbose_flag) {
                        printf(" miss");
                    }
                    pentry[index].valid_bit = 1;
                    pentry[index].tag_bit = tag;
                    summary.miss++;
                    break;
                case CSIM_OPERATION_RESULT_MISS_EVICTION:
                    if (verbose_flag) {
                        printf(" miss eviction");
                    }
                    pentry[0].tag_bit = tag;
                    temp = pentry[0];
                    for (index = 0 ; index < line_number - 1 ; ++index) {
                        pentry[index] = pentry[index + 1];
                    }
                    pentry[index] = temp;
                    summary.miss++;
                    summary.evict++;
                    break;
                case CSIM_OPERATION_RESULT_HIT:
                    if (verbose_flag) {
                        printf(" hit");
                    }
                    temp = pentry[index];
                    for ( ; (index < line_number - 1) && (pentry[index + 1].valid_bit) ; ++index) {
                        pentry[index] = pentry[index + 1];
                    }
                    pentry[index] = temp;
                    summary.hit++;
                    break;
            }
            if (type == CSIM_OPERATION_TYPE_MODIFY) {
                if (verbose_flag) {
                    printf(" hit");
                }
                summary.hit++;
            }
            putchar('\n');
        }
    }
    return summary;
}

int main(int argc, char *argv[]) {
    /* variables for argument parsing */
    char h = 0, v = 0, s = 0, E = 0, b = 0, t = 0;
    int opt;
    /* cache arguments */
    int set_number = 0, line_number = 0, block_offset = 0;
    /* cache */
    CSim_Cache * cache = NULL;
    /* file path */
    char file_path[80];
    /* verbose flag */
    char verbose_flag = 0;
    /* summary */
    CSim_Cache_Result summary = {0, 0, 0};
    /* pre-operation */
    program_name = argv[0];
    /* argument parsing */
    while ((opt = getopt(argc, argv, "hvs:E:b:t:")) != -1) {
        switch(opt) {
            case 'h':
                h = 1;
                break;
            case 'v':
                v = 1;
                verbose_flag = 1;
                break;
            case 's':
                set_number = atoi(optarg);
                if (set_number == 0) {
                    csim_error_missing_argument();
                    return CSIM_ERROR_MISSING_ARGUMENT;
                }
                s = 1;
                break;
            case 'E':
                line_number = atoi(optarg);
                if (line_number == 0) {
                    csim_error_missing_argument();
                    return CSIM_ERROR_MISSING_ARGUMENT;
                }
                E = 1;
                break;
            case 'b':
                block_offset = atoi(optarg);
                if (block_offset == 0) {
                    csim_error_missing_argument();
                    return CSIM_ERROR_MISSING_ARGUMENT;
                }
                b = 1;
                break;
            case 't':
                t = 1;
                strcpy(file_path, optarg);
                break;
            default:
                csim_print_help_info();
                return CSIM_ERROR_INVALID_OPTION;
                break;
        }
    }
    if (h == 1) {
        csim_print_help_info();
        return CSIM_OK;
    }
    if (v == 1) {
        verbose_flag = 1;
    }
    if (!(s == 1 && E == 1 && b == 1 && t == 1)) {
        csim_error_missing_argument();
        return CSIM_ERROR_MISSING_ARGUMENT;
    }
    /* file processing */
    FILE * file_pointer = fopen(file_path, "r");
    if (file_pointer == NULL) {
        csim_error_file_cannot_open(file_path);
        return CSIM_ERROR_FILE_CANNOT_OPEN;
    }
    /* cache construction */
    cache = csim_construct_cache(set_number, line_number, block_offset);
    /* trace file parsing */
    summary = csim_parse_trace_file(cache, file_pointer, verbose_flag);
    /* summary */
    printSummary(summary.hit, summary.miss, summary.evict);
    /* post operations */
    csim_deconstruct_cache(&cache);
    fclose(file_pointer);
    return CSIM_OK;
}
