/*
 * aho_corasick.c
 * Aho-Corasick 알고리즘 구현부
 *
 * 구축 단계에서 "goto 함수"까지 완전히 채워두는 방식(모든 노드의
 * children[]이 -1로 남지 않고 실패 링크를 따라간 결과로 채워짐)을 사용한다.
 * 이렇게 하면 탐색 단계에서는 실패 링크를 따라 되돌아가는 반복문 없이
 * 매 문자마다 배열 조회 1번(cur = node[cur].children[c])으로 끝나서,
 * 탐색이 순수 O(n)이 된다.
 */

#include "aho_corasick.h"
#include <stdlib.h>
#include <time.h>

 /* 새 노드 하나를 초기화 (children 전부 -1, fail=0, output 없음) */
static void ac_init_node(ACNode* node)
{
    for (int c = 0; c < AC_ALPHABET_SIZE; c++) {
        node->children[c] = -1;
    }
    node->fail = 0;
    node->output_count = 0;
}

/* node의 output 목록에 pattern_index를 추가 (용량 초과 시 무시) */
static void ac_add_output(ACNode* node, int pattern_index)
{
    if (node->output_count < AC_MAX_OUTPUTS_PER_NODE) {
        node->outputs[node->output_count++] = pattern_index;
    }
}

int ac_build(ACAutomaton* ac, const char** patterns, const int* pattern_lengths, int pattern_count)
{
    /* 용량 = 모든 패턴 길이의 합 + 1(루트) */
    int capacity = 1;
    for (int i = 0; i < pattern_count; i++) {
        capacity += pattern_lengths[i];
    }

    ac->nodes = (ACNode*)malloc(sizeof(ACNode) * capacity);
    if (!ac->nodes) return -1;

    ac->capacity = capacity;
    ac->pattern_count = pattern_count;
    ac->node_count = 1;  /* 0번 노드 = 루트 */
    ac_init_node(&ac->nodes[0]);

    /* 1단계: 모든 패턴을 Trie에 삽입 */
    for (int p = 0; p < pattern_count; p++) {
        int cur = 0;
        for (int i = 0; i < pattern_lengths[p]; i++) {
            unsigned char c = (unsigned char)patterns[p][i];
            if (ac->nodes[cur].children[c] == -1) {
                int new_node = ac->node_count++;
                ac_init_node(&ac->nodes[new_node]);
                ac->nodes[cur].children[c] = new_node;
            }
            cur = ac->nodes[cur].children[c];
        }
        ac_add_output(&ac->nodes[cur], p);
    }

    /* 2단계: BFS로 실패 링크 + goto 함수 완성 */
    int* queue = (int*)malloc(sizeof(int) * ac->node_count);
    if (!queue) {
        free(ac->nodes);
        return -1;
    }
    int qhead = 0, qtail = 0;

    /* 루트의 직계 자식들: 실패 링크는 전부 루트, 없는 문자는 루트를 가리키게 함 */
    for (int c = 0; c < AC_ALPHABET_SIZE; c++) {
        int child = ac->nodes[0].children[c];
        if (child == -1) {
            ac->nodes[0].children[c] = 0;  /* 루트에서 못 가면 그냥 루트에 머문다 */
        }
        else {
            ac->nodes[child].fail = 0;
            queue[qtail++] = child;
        }
    }

    while (qhead < qtail) {
        int u = queue[qhead++];
        for (int c = 0; c < AC_ALPHABET_SIZE; c++) {
            int v = ac->nodes[u].children[c];
            if (v != -1) {
                /* 실패 링크 = "부모의 실패 링크가 가리키는 노드"에서 같은 문자로 간 곳 */
                int f = ac->nodes[u].fail;
                ac->nodes[v].fail = ac->nodes[f].children[c];

                /* 실패 링크가 가리키는 노드의 output을 그대로 상속
                   (더 짧은 패턴이 현재 패턴의 접미사로 숨어있는 경우를 놓치지 않기 위함) */
                ACNode* fail_node = &ac->nodes[ac->nodes[v].fail];
                for (int k = 0; k < fail_node->output_count; k++) {
                    ac_add_output(&ac->nodes[v], fail_node->outputs[k]);
                }

                queue[qtail++] = v;
            }
            else {
                /* goto 함수 완성: 이 노드에 해당 문자로 가는 길이 없으면
                   실패 링크를 따라갔을 때의 전이를 그대로 복사해둔다.
                   탐색 단계에서 반복 없이 1번의 배열 조회로 끝나게 하는 핵심. */
                ac->nodes[u].children[c] = ac->nodes[ac->nodes[u].fail].children[c];
            }
        }
    }

    free(queue);
    return 0;
}

int ac_search(ACAutomaton* ac, const char* text, int text_len,
    ACMatch* results, int max_results, ACStats* stats)
{
    clock_t start_time = clock();
    long comparisons = 0;
    int match_count = 0;

    int cur = 0;
    for (int i = 0; i < text_len; i++) {
        unsigned char c = (unsigned char)text[i];
        cur = ac->nodes[cur].children[c];  /* goto 함수 1회 조회 = 상태전이 1회 */
        comparisons++;

        if (ac->nodes[cur].output_count > 0) {
            for (int k = 0; k < ac->nodes[cur].output_count; k++) {
                if (match_count < max_results) {
                    results[match_count].position = i + 1; /* 매칭이 끝난 지점(+1) */
                    results[match_count].pattern_index = ac->nodes[cur].outputs[k];
                }
                match_count++;
            }
        }
    }

    clock_t end_time = clock();

    if (stats) {
        int total_pattern_len = 0;
        /* total_pattern_length는 ac_build 때 알 수 있지만 여기선 호출자가
           필요시 별도로 채우도록 0으로 둠 (main에서 직접 세팅) */
        stats->text_length = text_len;
        stats->total_pattern_length = total_pattern_len;
        stats->pattern_count = ac->pattern_count;
        stats->comparisons = comparisons;
        stats->match_count = match_count;
        stats->elapsed_ms = ((double)(end_time - start_time) / CLOCKS_PER_SEC) * 1000.0;
    }

    return match_count;
}

void ac_free(ACAutomaton* ac)
{
    if (ac->nodes) {
        free(ac->nodes);
        ac->nodes = NULL;
    }
    ac->node_count = 0;
    ac->capacity = 0;
}