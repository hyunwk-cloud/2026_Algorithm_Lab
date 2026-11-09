/*
 * main.c
 * 문자열 매칭 기반 IDS 시그니처 탐지 - 사용자 입력 중심 메뉴 드라이버
 *
 * 사용 흐름:
 *   1) 텍스트 입력  2) 패턴 입력(쉼표로 여러 개 가능)
 *   3) 알고리즘 선택 실행(KMP / Boyer-Moore / Aho-Corasick / 전체 비교)
 *
 * 내장 샘플 데이터는 없다. 입력한 텍스트/패턴만 처리한다.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "Kmp.h"
#include "Boyer_Moore.h"
#include "Aho_Corasick.h"

#define MAX_TEXT_LEN    4096
#define MAX_PATTERNS    32
#define MAX_PATTERN_BUF 1024
#define MAX_RESULTS     512   /* 패턴 1개당 저장할 최대 매칭 개수 */
#define MAX_SHOW        15    /* 화면에 출력할 최대 위치 개수 */

 /* ---------- 현재 입력 상태 ---------- */

static char g_text[MAX_TEXT_LEN];
static int  g_text_len = 0;

static char        g_pattern_buf[MAX_PATTERN_BUF];
static const char* g_patterns[MAX_PATTERNS];
static int         g_pattern_lengths[MAX_PATTERNS];
static int         g_pattern_count = 0;

/* 알고리즘 1회 실행 결과 요약 */
typedef struct {
    long   comparisons;
    double elapsed_ms;
    int    matches;
} Summary;

/* ---------- 입력 유틸 ---------- */

/* 한 줄 읽기. 개행 제거, 버퍼보다 긴 입력은 남은 부분을 버린다. 실패 시 0 */
static int read_line(char* buf, int size)
{
    if (!fgets(buf, size, stdin)) return 0;

    int len = (int)strlen(buf);
    if (len > 0 && buf[len - 1] == '\n') {
        buf[--len] = '\0';
        if (len > 0 && buf[len - 1] == '\r') buf[--len] = '\0';
    }
    else {
        int ch;
        while ((ch = getchar()) != '\n' && ch != EOF) {}
        printf("(입력이 너무 길어 %d자까지만 사용합니다)\n", size - 1);
    }
    return 1;
}

static void input_text(void)
{
    printf("\n탐지 대상 텍스트를 입력하세요 (최대 %d자)\n> ", MAX_TEXT_LEN - 1);
    if (!read_line(g_text, MAX_TEXT_LEN)) return;
    g_text_len = (int)strlen(g_text);
    printf("텍스트 %d자 저장됨\n\n", g_text_len);
}

/* 쉼표로 구분된 패턴 입력. 쉼표 앞뒤 공백도 패턴의 일부로 취급한다. */
static void input_patterns(void)
{
    printf("\n찾을 패턴을 입력하세요 (쉼표로 구분, 최대 %d개)\n", MAX_PATTERNS);
    printf("예) OR 1=1--,<script>,UNION SELECT\n> ");
    if (!read_line(g_pattern_buf, MAX_PATTERN_BUF)) return;

    g_pattern_count = 0;
    char* p = g_pattern_buf;
    while (*p != '\0' && g_pattern_count < MAX_PATTERNS) {
        char* start = p;
        while (*p != ',' && *p != '\0') p++;
        int len = (int)(p - start);
        if (*p == ',') { *p = '\0'; p++; }
        if (len > 0) {
            g_patterns[g_pattern_count] = start;
            g_pattern_lengths[g_pattern_count] = len;
            g_pattern_count++;
        }
    }

    printf("패턴 %d개 저장됨\n", g_pattern_count);
    for (int i = 0; i < g_pattern_count; i++) {
        printf("  [%d] \"%s\"\n", i + 1, g_patterns[i]);
    }
    printf("\n");
}

static void show_current(void)
{
    printf("\n--- 현재 입력 ---\n");
    if (g_text_len > 0) printf("텍스트(%d자): \"%s\"\n", g_text_len, g_text);
    else                printf("텍스트: (없음)\n");

    if (g_pattern_count > 0) {
        for (int i = 0; i < g_pattern_count; i++) {
            printf("패턴 [%d]: \"%s\"\n", i + 1, g_patterns[i]);
        }
    }
    else {
        printf("패턴: (없음)\n");
    }
    printf("\n");
}

/* 실행 전에 텍스트/패턴이 입력되어 있는지 확인 */
static int require_ready(void)
{
    if (g_text_len == 0) {
        printf("\n먼저 1번으로 텍스트를 입력하세요.\n\n");
        return 0;
    }
    if (g_pattern_count == 0) {
        printf("\n먼저 2번으로 패턴을 입력하세요.\n\n");
        return 0;
    }
    return 1;
}

/* ---------- 결과 출력 유틸 ---------- */

/* 패턴 하나의 결과 출력. pos[]는 매칭 시작 위치 */
static void print_pattern_result(const char* pattern, int total, const int* pos, int stored)
{
    printf("  패턴 \"%s\" -> %d건", pattern, total);
    if (total == 0) {
        printf("\n");
        return;
    }
    printf("  위치: ");
    int show = stored < MAX_SHOW ? stored : MAX_SHOW;
    for (int k = 0; k < show; k++) {
        printf("%d%s", pos[k], (k + 1 < show) ? ", " : "");
    }
    if (total > show) printf(" ... (외 %d건)", total - show);
    printf("\n");
}

/* ---------- 알고리즘 실행 ---------- */

static Summary do_kmp(void)
{
    Summary sum = { 0, 0.0, 0 };
    printf("\n[KMP] 패턴 %d개를 하나씩 순차 실행\n", g_pattern_count);

    for (int i = 0; i < g_pattern_count; i++) {
        KMPMatch r[MAX_RESULTS];
        KMPStats s;
        int pos[MAX_RESULTS];

        int found = kmp_search(g_text, g_text_len, g_patterns[i], g_pattern_lengths[i],
            r, MAX_RESULTS, &s);
        int stored = found < MAX_RESULTS ? found : MAX_RESULTS;
        for (int k = 0; k < stored; k++) pos[k] = r[k].position;

        print_pattern_result(g_patterns[i], found, pos, stored);
        sum.comparisons += s.comparisons;
        sum.elapsed_ms += s.elapsed_ms;
        sum.matches += found;
    }

    printf("  => 총 매칭 %d건, 총 비교횟수 %ld, 총 시간 %.4fms\n\n",
        sum.matches, sum.comparisons, sum.elapsed_ms);
    return sum;
}

static Summary do_bm(void)
{
    Summary sum = { 0, 0.0, 0 };
    printf("\n[Boyer-Moore] 패턴 %d개를 하나씩 순차 실행\n", g_pattern_count);

    for (int i = 0; i < g_pattern_count; i++) {
        BMMatch r[MAX_RESULTS];
        BMStats s;
        int pos[MAX_RESULTS];

        int found = bm_search(g_text, g_text_len, g_patterns[i], g_pattern_lengths[i],
            r, MAX_RESULTS, &s);
        int stored = found < MAX_RESULTS ? found : MAX_RESULTS;
        for (int k = 0; k < stored; k++) pos[k] = r[k].position;

        print_pattern_result(g_patterns[i], found, pos, stored);
        sum.comparisons += s.comparisons;
        sum.elapsed_ms += s.elapsed_ms;
        sum.matches += found;
    }

    printf("  => 총 매칭 %d건, 총 비교횟수 %ld, 총 시간 %.4fms\n\n",
        sum.matches, sum.comparisons, sum.elapsed_ms);
    return sum;
}

static Summary do_ac(void)
{
    Summary sum = { 0, 0.0, 0 };
    printf("\n[Aho-Corasick] 패턴 %d개를 텍스트 1회 스캔으로 동시 탐지\n", g_pattern_count);

    ACAutomaton ac;
    if (ac_build(&ac, g_patterns, g_pattern_lengths, g_pattern_count) != 0) {
        printf("  오토마타 구축 실패\n\n");
        return sum;
    }

    ACMatch r[MAX_RESULTS];
    ACStats s;
    int found = ac_search(&ac, g_text, g_text_len, r, MAX_RESULTS, &s);
    int stored = found < MAX_RESULTS ? found : MAX_RESULTS;

    /* 패턴별로 묶어서 출력. AC는 "끝 위치"를 주므로 시작 위치로 변환 */
    for (int i = 0; i < g_pattern_count; i++) {
        int pos[MAX_RESULTS];
        int cnt = 0;
        for (int k = 0; k < stored; k++) {
            if (r[k].pattern_index == i) {
                pos[cnt++] = r[k].position - g_pattern_lengths[i];
            }
        }
        print_pattern_result(g_patterns[i], cnt, pos, cnt);
    }
    if (found > MAX_RESULTS) {
        printf("  (매칭이 너무 많아 일부만 표시됨)\n");
    }

    sum.comparisons = s.comparisons;
    sum.elapsed_ms = s.elapsed_ms;
    sum.matches = found;

    printf("  => 총 매칭 %d건, 상태전이 횟수 %ld, 시간 %.4fms\n\n",
        sum.matches, sum.comparisons, sum.elapsed_ms);

    ac_free(&ac);
    return sum;
}

static void do_all(void)
{
    Summary k = do_kmp();
    Summary b = do_bm();
    Summary a = do_ac();

    printf("===== 전체 비교 요약 =====\n");
    printf("%-14s | %8s | %12s | %10s\n", "알고리즘", "총 매칭", "비교/전이횟수", "시간(ms)");
    printf("---------------+----------+--------------+-----------\n");
    printf("%-14s | %8d | %12ld | %10.4f\n", "KMP", k.matches, k.comparisons, k.elapsed_ms);
    printf("%-14s | %8d | %12ld | %10.4f\n", "Boyer-Moore", b.matches, b.comparisons, b.elapsed_ms);
    printf("%-14s | %8d | %12ld | %10.4f\n", "Aho-Corasick", a.matches, a.comparisons, a.elapsed_ms);

    if (k.matches == b.matches && b.matches == a.matches) {
        printf("\n세 알고리즘의 매칭 결과가 일치합니다.\n\n");
    }
    else {
        printf("\n[주의] 알고리즘 간 매칭 개수가 다릅니다. 구현 확인이 필요합니다.\n\n");
    }
}

/* ---------- 메뉴 ---------- */

static void print_menu(void)
{
    printf("========================================\n");
    printf(" 문자열 매칭 기반 IDS 시그니처 탐지 시스템\n");
    printf("========================================\n");
    printf(" 현재: 텍스트 %d자 / 패턴 %d개\n", g_text_len, g_pattern_count);
    printf("----------------------------------------\n");
    printf("1. 텍스트 입력\n");
    printf("2. 패턴 입력 (쉼표로 여러 개)\n");
    printf("3. KMP 실행\n");
    printf("4. Boyer-Moore 실행\n");
    printf("5. Aho-Corasick 실행\n");
    printf("6. 전체 알고리즘 비교 실행\n");
    printf("7. 현재 입력 확인\n");
    printf("8. 종료\n");
    printf("========================================\n");
    printf("선택 > ");
}

int main(void)
{
    char line[16];

    while (1) {
        print_menu();
        if (!read_line(line, sizeof(line))) break;
        int choice = atoi(line);

        if (choice == 1) {
            input_text();
        }
        else if (choice == 2) {
            input_patterns();
        }
        else if (choice == 3) {
            if (require_ready()) do_kmp();
        }
        else if (choice == 4) {
            if (require_ready()) do_bm();
        }
        else if (choice == 5) {
            if (require_ready()) do_ac();
        }
        else if (choice == 6) {
            if (require_ready()) do_all();
        }
        else if (choice == 7) {
            show_current();
        }
        else if (choice == 8) {
            printf("종료합니다.\n");
            break;
        }
        else {
            printf("잘못된 입력입니다. 다시 선택해주세요.\n\n");
        }
    }

    return 0;
}