# 문자열 매칭 알고리즘 기반 IDS 시그니처 탐지 시스템 설계 및 데이터 분석 기반 성능 평가

## 프로젝트 개요
- 과목명: 알고리즘
- 개발 언어: C, Python (Visual Studio 2022, Visual Studio Code)
- 연구 기간: 2026.09 ~ 2026.12 (15주, 1인 프로젝트)

## 주요 기능
- 바이너리 세이프(Binary-safe) 패킷 탐색: `unsigned char` 처리를 통해 널 바이트(`0x00`) 및 확장 아스키가 포함된 네트워크 페이로드 안정적 검사
- KMP 단일 패턴 매칭: 실패 함수(LPS) 기반 포인터 역추적 제거($\mathcal{O}(N+M)$) 및 중첩(Overlapping) 시그니처 연속 탐지
- Boyer-Moore(Horspool) 고속 스킵 매칭: 256바이트 Bad Character 테이블을 활용한 역방향 비교 및 불필요 구간 점프($\mathcal{O}(N/M)$)
- Aho-Corasick DFA 다중 패턴 매칭: 1차원 평면 배열(Flat Array) 및 전이 테이블 완전 압축을 통한 $\mathcal{O}(1)$ 상태 전이·단일 패스 다중 탐색
- 통일된 성능 계측 및 교차 검증: `Stats` 구조체를 통한 문자 비교/전이 횟수, 탐지 인덱스, 소요 시간 일괄 측정 및 무결성 검증
- (예정) 하이브리드 엔진 추가, 대용량 트래픽 벤치마크 및 Python 기반 성능 시각화

## 각 코드 간단 설명
- `kmp.h`, `boyer_moore.h`, `aho_corasick.h`
  - 각 알고리즘의 함수 인터페이스 및 공통 통계 구조체(`Stats`), 오토마톤 노드 구조체(`ACNode`) 정의
- `kmp.c`
  - 패턴의 접두사·접미사 일치 길이를 계산하는 `kmp_build_failure()` 및 텍스트 되돌림 없이 선형 탐색을 수행하는 `kmp_search()` 구현
- 'boyer_moore.c`
  - 불일치 문자의 마지막 등장 위치를 기록하는 Bad Character 전처리 테이블 및 역방향 비교·최소 1칸 전진 안전장치가 적용된 `bm_search()` 구현
- `src/aho_corasick.c`
  - 포인터 대신 4바이트 정수 인덱스를 활용한 캐시 친화적 트라이 구축(`ac_build`) 및 실패 링크를 사전 병합한 DFA 기반 다중 탐색(`ac_search`) 구현
- `src/main.c`
  - 동일한 패킷 페이로드에 대해 3개 알고리즘을 순차 실행하여 탐지 정확도(교차 검증)와 연산 횟수를 비교 출력하는 통합 벤치마크 드라이버
