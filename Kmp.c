/*
 * kmp.c
 * KMP 알고리즘 구현부
 */

#include "kmp.h"
#include <stdlib.h>
#include <time.h>

void kmp_build_failure(const char* pattern, int pattern_len, int* failure)
{
    /* failure[0]은 항상 0 (길이 1짜리 접두사는 자기 자신과만 겹칠 수 있으므로) */
    failure[0] = 0;

    int len = 0;  /* 현재까지 알아낸 "접두사=접미사" 최대 길이 */
    int i = 1;

    while (i < pattern_len) {
        if (pattern[i] == pattern[len]) {
            /* 일치하면 길이를 늘리고 다음 문자로 */
            len++;
            failure[i] = len;
            i++;
        }
        else if (len > 0) {
            /* 불일치했지만 아직 더 짧은 후보가 있을 수 있으므로
               failure[len-1]을 참고해 len을 줄여본다 (패턴 포인터만 이동,
               텍스트 포인터 개념이 없는 전처리 단계라 여기선 i가 그대로 유지됨) */
            len = failure[len - 1];
        }
        else {
            /* len이 0인데도 불일치하면 이 위치는 접두사-접미사 겹침이 없음 */
            failure[i] = 0;
            i++;
        }
    }
}

int kmp_search(const char* text, int text_len,
    const char* pattern, int pattern_len,
    KMPMatch* results, int max_results,
    KMPStats* stats)
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

    /* 실패함수 전처리 */
    int* failure = (int*)malloc(sizeof(int) * pattern_len);
    if (!failure) {
        if (stats) {
            stats->text_length = text_len;
            stats->pattern_length = pattern_len;
            stats->comparisons = 0;
            stats->match_count = 0;
            stats->elapsed_ms = 0.0;
        }
        return 0;
    }
    kmp_build_failure(pattern, pattern_len, failure);

    int i = 0;  /* 텍스트 인덱스 */
    int j = 0;  /* 패턴 인덱스 */

    while (i < text_len) {
        comparisons++;
        if (text[i] == pattern[j]) {
            i++;
            j++;
            if (j == pattern_len) {
                /* 패턴 전체가 일치 -> 매칭 발견 */
                if (match_count < max_results) {
                    results[match_count].position = i - j;
                }
                match_count++;
                /* 다음 매칭을 계속 찾기 위해 j를 failure 테이블로 되돌림 */
                j = failure[j - 1];
            }
        }
        else if (j > 0) {
            /* 불일치 -> 텍스트는 그대로 두고 패턴 포인터만 점프 (핵심 최적화) */
            j = failure[j - 1];
        }
        else {
            i++;
        }
    }

    free(failure);

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