#ifndef KMP_H
#define KMP_H

/*
 * kmp.h
 * KMP(Knuth-Morris-Pratt) 문자열 매칭 알고리즘
 *
 * 핵심 아이디어:
 *   패턴 내부의 "접두사이면서 접미사인 부분"을 미리 계산해두면(실패함수),
 *   텍스트와 비교하다 불일치가 나도 이미 비교한 정보를 버리지 않고
 *   패턴 포인터만 다음 위치로 점프시킬 수 있다. 이 덕분에 텍스트 포인터는
 *   절대 뒤로 돌아가지 않고, 전체 시간복잡도는 O(n + m)이 된다.
 *   (n = 텍스트 길이, m = 패턴 길이)
 */

#include <stddef.h>

 /* 매칭 결과 하나(패턴이 발견된 위치)를 담는 구조체 */
typedef struct {
    int position;   /* 텍스트 내 매칭 시작 인덱스 (0-based) */
} KMPMatch;

/* 성능 측정 결과를 담는 구조체 */
typedef struct {
    int text_length;
    int pattern_length;
    long comparisons;      /* 문자 비교 횟수 */
    int match_count;       /* 발견된 매칭 개수 */
    double elapsed_ms;     /* 처리 시간(ms) */
} KMPStats;

/*
 * kmp_build_failure
 *   패턴에 대한 실패함수(부분일치 테이블)를 계산한다.
 *   failure[i] = pattern[0..i] 구간에서, 접두사이면서 접미사인
 *                가장 긴 부분문자열의 길이
 *
 * 매개변수:
 *   pattern      - 대상 패턴 문자열
 *   pattern_len  - 패턴 길이
 *   failure      - 결과를 저장할 배열 (호출자가 pattern_len 크기로 할당)
 */
void kmp_build_failure(const char* pattern, int pattern_len, int* failure);

/*
 * kmp_search
 *   텍스트에서 패턴이 나타나는 모든 위치를 찾는다.
 *
 * 매개변수:
 *   text         - 탐색 대상 텍스트
 *   text_len     - 텍스트 길이
 *   pattern      - 찾을 패턴
 *   pattern_len  - 패턴 길이
 *   results      - 매칭 결과를 저장할 배열 (호출자가 미리 할당, 충분한 크기 필요)
 *   max_results  - results 배열의 최대 용량
 *   stats        - 성능 측정 결과를 저장할 구조체 (NULL이면 측정 생략)
 *
 * 반환값:
 *   발견된 매칭 개수 (results에 실제로 채워진 개수)
 */
int kmp_search(const char* text, int text_len,
    const char* pattern, int pattern_len,
    KMPMatch* results, int max_results,
    KMPStats* stats);

#endif /* KMP_H */