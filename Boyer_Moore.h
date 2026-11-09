#pragma once
#ifndef BOYER_MOORE_H
#define BOYER_MOORE_H

/*
 * boyer_moore.h
 * Boyer-Moore 문자열 매칭 알고리즘 (Bad Character Rule 기반)
 *
 * 핵심 아이디어:
 *   패턴을 텍스트 앞에서부터가 아니라 "뒤에서부터" 비교한다.
 *   불일치가 나면, 텍스트에서 그 위치에 있던 문자가 패턴 내에서
 *   마지막으로 등장한 위치를 보고 패턴을 통째로 점프시킨다.
 *   (불일치 문자가 패턴에 아예 없으면 패턴 길이만큼 통째로 건너뜀)
 *
 *   KMP는 "텍스트 포인터가 절대 되돌아가지 않는" 방식으로 O(n+m)을
 *   보장하는 반면, Boyer-Moore는 평균적으로 훨씬 많이 건너뛸 수 있어서
 *   실전에서는(특히 패턴이 길고 문자셋이 클 때) KMP보다 빠른 경우가 많다.
 *   다만 최악의 경우(worst case)는 O(n*m)이 될 수 있다.
 */

#include <stddef.h>

#define BM_ALPHABET_SIZE 256  /* ASCII 전체 범위를 가정 */

typedef struct {
    int position;
} BMMatch;

typedef struct {
    int text_length;
    int pattern_length;
    long comparisons;
    int match_count;
    double elapsed_ms;
} BMStats;

/*
 * bm_build_bad_char_table
 *   Bad Character 테이블을 구성한다.
 *   table[c] = 패턴 내에서 문자 c가 마지막으로 등장한 인덱스 (없으면 -1)
 *
 * 매개변수:
 *   pattern      - 대상 패턴
 *   pattern_len  - 패턴 길이
 *   table        - 크기 BM_ALPHABET_SIZE로 호출자가 미리 할당한 배열
 */
void bm_build_bad_char_table(const char* pattern, int pattern_len, int* table);

/*
 * bm_search
 *   텍스트에서 패턴이 나타나는 모든 위치를 찾는다 (Bad Character Rule만 사용).
 *
 * 매개변수는 kmp_search와 동일한 패턴을 따른다.
 *
 * 반환값:
 *   발견된 매칭 개수
 */
int bm_search(const char* text, int text_len,
    const char* pattern, int pattern_len,
    BMMatch* results, int max_results,
    BMStats* stats);

#endif /* BOYER_MOORE_H */