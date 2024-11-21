#include "cachelab.h"
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <stdbool.h>

bool arg_v;
int arg_s, arg_E, arg_b;
char* arg_t;
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
    bool valid;
    uint32_t Tag;
    char* data;
};
struct Set {
    Block* block;
};
struct Cache {
    Set* set;
};

void exec_line(char op, unsigned long addr, int siz) {
    switch (op) {
        case 'I':
            printf("Instruction load at address 0x%lx, size %d\n", addr, siz);
        break;
        case 'L':
            printf("Data load at address 0x%lx, size %d\n", addr, siz);
        break;
        case 'S':
            printf("Data store at address 0x%lx, size %d\n", addr, siz);
        break;
        case 'M':
            printf("Data modify at address 0x%lx, size %d\n", addr, siz);
        break;
        default:
            printf("Unknown operation: %c\n", op);
        exit(1);
        break;
    }
}

void exec_file() {
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
        printf("Operation: %c, Address: 0x%lx, Size: %d\n", op, addr, siz);
        exec_line(op, addr, siz);
    }
    fclose(file);
}


int main(int argc, char *argv[]) {
    // 解析命令行参数
    parse_args(argc, argv);
    print_parse_args();
    // 模拟 tracefile 文件中的操作
    exec_file();
    printSummary(0, 0, 0);
    return 0;
}
// ./csim -s 4 -E 2 -b 3 -t ./traces/yi2.trace