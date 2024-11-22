#include "cachelab.h"
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <stdbool.h>
#include <stdint.h>

bool arg_v;
int arg_s, arg_E, arg_b;
char* arg_t;
int hits, misses, evictions;
int tmp_hits, tmp_misses, tmp_evictions;
void print_parse_args() {
    // 打印解析结果
    printf("Parsed Arguments:\n");
    printf("v = %d, s = %d, E = %d, b = %d, tracefile = %s\n", arg_v, arg_s, arg_E, arg_b, arg_t);
}
void parse_args(int argc, char *argv[]) {
    int opt;
    // 使用 getopt 函数解析命令行参数
    while ((opt = getopt(argc, argv, "hvs:E:b:t:")) != -1) {
        switch (opt) {
            case 'h':
                // 打印帮助信息
                printf("Usage: %s [-hv] -s <s> -E <E> -b <b> -t <tracefile>\n", argv[0]);
                exit(0);
            case 'v':
                arg_v = true;
                break;
            case 's':
                arg_s = atoi(optarg);  // 解析 -s 参数值
                break;
            case 'E':
                arg_E = atoi(optarg);  // 解析 -E 参数值
                break;
            case 'b':
                arg_b = atoi(optarg);  // 解析 -b 参数值
                break;
            case 't':
                arg_t = optarg;  // 解析 -t 参数值
                break;
            case '?':
                // 当遇到无效参数时
                printf("Invalid option or missing argument\n");
                exit(1);
            default:
                break;
        }
    }
}

struct Block {
    uint32_t tag;
    struct Block* nxt;
};
struct Set {
    struct Block* head;
    uint32_t len;
};
struct Set* cache;

void lru_set(struct Set* set, uint32_t tag) {
    struct Block* cur = set->head->nxt;
    struct Block* pre = set->head;
    // 这里要保证结构体被初始化为全 0
    while(cur != NULL) {
        if(cur->tag == tag) {
            pre->nxt = cur->nxt;
            cur->nxt = NULL;
            break;
        }
        pre = cur;
        cur = cur->nxt;
    }
    // int hits, misses, evictions;
    if(cur == NULL) {
        // miss 了
        tmp_misses++;
        if(set->len == arg_E) {
            tmp_evictions++;
            cur = pre;
            pre = set->head;
            while(pre->nxt != cur) {
                pre = pre->nxt;
            }
            pre->nxt = NULL;
        } else {
            cur = (struct Block*)calloc(1, sizeof(struct Block));
            ++set->len;
        }
    } else {
        tmp_hits++;
    }
    cur->tag = tag;
    cur->nxt = set->head->nxt;
    set->head->nxt = cur;
}

void unpack_address(unsigned long addr, uint32_t* tag, uint32_t* set_index, uint32_t* block_offset) {
    *block_offset = ((1<<arg_b) - 1) & addr;
    addr>>=arg_b;
    *set_index = ((1<<arg_s) - 1) & addr;
    addr>>=arg_s;
    *tag = addr;
}

void exec_line(char op, unsigned long addr, int siz) {
    uint32_t tag, set_index, block_offset;
    unpack_address(addr, &tag, &set_index, &block_offset);
//    printf("[]DEBUG: tag=%d set_index=%d block_offset=%d\n", tag, set_index, block_offset);
    switch (op) {
        case 'I':
            break;
        case 'L':
            lru_set(cache+set_index, tag);
            break;
        case 'S':
            lru_set(cache+set_index, tag);
            break;
        case 'M':
            lru_set(cache+set_index, tag);
            lru_set(cache+set_index, tag);
            break;
        default:
            printf("Unknown operation: %c\n", op);
            exit(1);
            break;
    }
    if(arg_v) {
        printSummary(tmp_hits, tmp_misses, tmp_evictions);
        printf("\n");
    }
    hits += tmp_hits;
    misses += tmp_misses;
    evictions += tmp_evictions;
    tmp_hits = tmp_misses = tmp_evictions = 0;
}

void exec_file() {
    // 创建 cache 变量
    cache = (struct Set*)calloc(1<<arg_s, sizeof(struct Set));
    for(int i=0;i<(1<<arg_s);++i) {
        cache[i].head = (struct Block*)calloc(1, sizeof(struct Block));
    }
    // 打开文件
    FILE *file;
    char line[256];
    file = fopen(arg_t, "r");
    if (file == NULL) {
        perror("Error opening file");
        exit(EXIT_FAILURE);
    }
    char op;
    unsigned long addr;
    int siz;
    // 逐行读取文件内容
    while (fgets(line, sizeof(line), file)) {
        if(arg_v) {
            printf("%s", line);
        }
        // 跳过空行
        if (line[0] == '\n') {
            continue;
        }
        // 解析每行数据
        if (line[0] == ' ') {
            // 格式: " M 0x7fff5fc0,8" (注意前置空格)
            sscanf(line, " %c %lx,%d", &op, &addr, &siz);
        } else {
            // 格式: "I 0x4004c8,4"
            sscanf(line, "%c %lx,%d", &op, &addr, &siz);
        }
        // 打印解析结果
        // printf("Operation: %c, Address: 0x%lx, Size: %d\n", op, addr, siz);
        exec_line(op, addr, siz);
    }
    fclose(file);
}


int main(int argc, char *argv[]) {
    // 解析命令行参数
    parse_args(argc, argv);
    // 模拟 tracefile 文件中的操作
    exec_file();
    printSummary(hits, misses, evictions);
    return 0;
}
// ./csim -s 4 -E 2 -b 3 -t ./traces/yi2.trace