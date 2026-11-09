#pragma once
#ifndef AHO_CORASICK_H
#define AHO_CORASICK_H

/*
 * aho_corasick.h
 * Aho-Corasick 다중 패턴 매칭 알고리즘
 *
 * 핵심 아이디어:
 *   여러 패턴을 하나의 Trie(트라이)로 통합하고, 각 노드마다 "실패 링크"를
 *   연결한다. 실패 링크는 KMP의 실패함수를 트라이 전체로 확장한 개념으로,
 *   "현재까지 매칭된 접미사와 일치하는, 트라이 상의 다른 가장 긴 접두사"를
 *   가리킨다.
 *
 *   이 덕분에 텍스트를 단 한 번만 스캔해도, 등록된 모든 패턴의 매칭 위치를
 *   동시에 찾아낼 수 있다. 시간복잡도는 O(n + m_total + z)
 *   (n=텍스트 길이, m_total=모든 패턴 길이의 합, z=매칭 결과 수)
 *
 *   KMP/Boyer-Moore를 패턴 개수만큼 반복 실행하면 O(k * (n+m)) 수준인데,
 *   Aho-Corasick은 텍스트 스캔이 k(패턴 개수)와 무관하게 1회로 끝나는 것이
 *   핵심 차이점이다.
 */

#define AC_ALPHABET_SIZE 256   /* ASCII 전체 범위 */
#define AC_MAX_OUTPUTS_PER_NODE 8  /* 한 노드에서 끝나는 패턴 최대 개수 (서로 다른 길이의 패턴이 같은 지점에서 끝날 수 있음) */

 /* 매칭 결과: 어떤 패턴이(pattern_index) 어느 위치에서(position) 끝났는지 */
typedef struct {
    int position;       /* 텍스트 내 매칭이 "끝난" 인덱스 + 1 (매칭 종료 지점) */
    int pattern_index;  /* ac_build에 넘긴 patterns 배열에서의 인덱스 */
} ACMatch;

typedef struct {
    int text_length;
    int total_pattern_length;  /* 등록된 모든 패턴 길이의 합 */
    int pattern_count;
    long comparisons;   /* 텍스트 문자 1개 처리 = 상태전이 1회로 집계 */
    int match_count;
    double elapsed_ms;
} ACStats;

/* 트라이 노드 1개 */
typedef struct {
    int children[AC_ALPHABET_SIZE];  /* 자식 노드 인덱스, 없으면 -1 */
    int fail;                        /* 실패 링크가 가리키는 노드 인덱스 */
    int outputs[AC_MAX_OUTPUTS_PER_NODE];
    int output_count;
} ACNode;

/* Aho-Corasick 오토마타 전체 */
typedef struct {
    ACNode* nodes;
    int node_count;
    int capacity;        /* nodes 배열의 최대 용량 */
    int pattern_count;
} ACAutomaton;

/*
 * ac_build
 *   patterns 배열(문자열 포인터들)로 Trie를 구성하고 실패 링크까지 연결한다.
 *   이 함수 호출 한 번으로 "구축" 단계가 전부 끝나며, 이후 ac_search를
 *   여러 텍스트에 대해 반복 호출할 수 있다.
 *
 * 매개변수:
 *   ac              - 초기화할 오토마타 구조체 (호출자가 선언, 내부 배열은 이 함수가 할당)
 *   patterns        - 패턴 문자열 배열
 *   pattern_lengths - 각 패턴의 길이 배열
 *   pattern_count   - 패턴 개수
 *
 * 반환값:
 *   성공 시 0, 실패(메모리 할당 실패 등) 시 -1
 */
int ac_build(ACAutomaton* ac, const char** patterns, const int* pattern_lengths, int pattern_count);

/*
 * ac_search
 *   텍스트를 한 번 스캔하며 등록된 모든 패턴의 매칭을 찾는다.
 *
 * 매개변수:
 *   ac           - ac_build로 구축된 오토마타
 *   text         - 탐색 대상 텍스트
 *   text_len     - 텍스트 길이
 *   results      - 매칭 결과를 저장할 배열 (호출자가 미리 할당)
 *   max_results  - results 배열의 최대 용량
 *   stats        - 성능 측정 결과 (NULL이면 생략)
 *
 * 반환값:
 *   발견된 매칭 총 개수
 */
int ac_search(ACAutomaton* ac, const char* text, int text_len,
    ACMatch* results, int max_results, ACStats* stats);

/*
 * ac_free
 *   ac_build에서 할당한 내부 메모리를 해제한다.
 */
void ac_free(ACAutomaton* ac);

#endif /* AHO_CORASICK_H */