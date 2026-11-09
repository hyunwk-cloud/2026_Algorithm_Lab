/*
 * boyer_moore.c
 * Boyer-Moore 알고리즘 구현부 (Bad Character Rule)
 */

#include "Boyer_Moore.h"
#include <stdlib.h>
#include <time.h>

void bm_build_bad_char_table(const char* pattern, int pattern_len, int* table)
{
    /* 기본값: 패턴에 없는 문자는 -1 */
    for (int c = 0; c < BM_ALPHABET_SIZE; c++) {
        table[c] = -1;
    }

    /* 뒤에서부터가 아니라 앞에서부터 채우되, 같은 문자가 여러 번 나오면
       "마지막으로 등장한 위치"로 계속 덮어써야 하므로 앞->뒤 순서로 순회하면
       자연스럽게 가장 뒤쪽(마지막) 등장 위치가 최종적으로 남는다 */
    for (int i = 0; i < pattern_len; i++) {
        table[(unsigned char)pattern[i]] = i;
    }
}

int bm_search(const char* text, int text_len,
    const char* pattern, int pattern_len,
    BMMatch* results, int max_results,
    BMStats* stats)
{
    clock_t start_time = clock();
    long comparisons = 0;
    int match_count = 0;

    if (pattern_len == 0 || text_len == 0 || pattern_len > text_len) {
        if (stats) {
            stats->text_length = text_len;
            stats->pattern_length = pattern_len;
            stats->comparisons = 0;
            stats->match_count = 0;
            stats->elapsed_ms = 0.0;
        }
        return 0;
    }

    int* bad_char = (int*)malloc(sizeof(int) * BM_ALPHABET_SIZE);
    if (!bad_char) {
        if (stats) {
            stats->text_length = text_len;
            stats->pattern_length = pattern_len;
            stats->comparisons = 0;
            stats->match_count = 0;
            stats->elapsed_ms = 0.0;
        }
        return 0;
    }
    bm_build_bad_char_table(pattern, pattern_len, bad_char);

    int shift = 0;  /* 텍스트 위에서 패턴이 놓인 시작 위치 */

    while (shift <= text_len - pattern_len) {
        int j = pattern_len - 1;  /* 패턴을 뒤에서부터 비교 */

        /* 뒤에서부터 비교하다 불일치가 나거나 전부 일치할 때까지 진행 */
        while (j >= 0) {
            comparisons++;
            if (pattern[j] == text[shift + j]) {
                j--;
            }
            else {
                break;
            }
        }

        if (j < 0) {
            /* 패턴 전체가 일치 -> 매칭 발견 */
            if (match_count < max_results) {
                results[match_count].position = shift;
            }
            match_count++;

            /* 다음 매칭을 찾기 위해 한 칸만 이동 (중첩 매칭도 놓치지 않기 위함) */
            if (shift + pattern_len < text_len) {
                int next_char = (unsigned char)text[shift + pattern_len];
                shift += pattern_len - bad_char[next_char];
                if (shift <= 0) shift = 1;
            }
            else {
                shift++;
            }
        }
        else {
            /* 불일치 지점의 텍스트 문자를 기준으로 건너뛸 거리 계산 */
            int bad_char_shift = j - bad_char[(unsigned char)text[shift + j]];
            shift += (bad_char_shift > 1) ? bad_char_shift : 1;
        }
    }

    free(bad_char);

    clock_t end_time = clock();

    if (stats) {
        stats->text_length = text_len;
        stats->pattern_length = pattern_len;
        stats->comparisons = comparisons;
        stats->match_count = match_count;
        stats->elapsed_ms = ((double)(end_time - start_time) / CLOCKS_PER_SEC) * 1000.0;
    }

    return match_count;
}