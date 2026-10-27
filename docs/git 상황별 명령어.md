# ⌨️ git 상황별 명령어

> 8주차 팀 협업(pintos-project2-team)에서 실제로 쓴 명령 위주. 브랜치 규칙은 [[git 브랜치전략]], 커밋 메시지는 [[git 커밋 컨벤션]].

---

## 브랜치 만들기

| 상황 | 명령 |
|---|---|
| 최신 dev 에서 내 이슈 브랜치 따기 | `git fetch origin && git switch -c feature-기능-이름 origin/dev` |
| 공용 기능 브랜치 만들고 올리기 (팀원이 딸 수 있게) | `git fetch origin && git switch -c feature-fork origin/dev && git push -u origin feature-fork` |
| 공용 기능 브랜치에서 내 브랜치 따기 | `git switch -c feature-fork-이름 origin/feature-fork` |
| 다른 브랜치로 옮기기 | `git switch 브랜치` |
| 지금 브랜치 · 바뀐 파일 보기 | `git status` |

> 항상 `fetch` 먼저. `fetch` 는 원격 최신 상태만 받아 오고 내 파일은 안 건드려서 언제 해도 안전하다.
> 브랜치를 바꿔도 커밋 안 한 변경은 그대로 따라온다 → 바꾸기 전에 `git status` 확인.

---

## 커밋 나누기

| 상황 | 명령 |
|---|---|
| 한 파일에서 이번 커밋 줄만 담기 | `git add -p 파일` → 덩어리마다 `y` / `n`, 쪼개기 `s`, 직접 고르기 `e` |
| 이번 커밋에 담긴 것만 보기 | `git diff --staged` |
| 아직 안 담은 변경 보기 | `git diff` |
| 제목 + 본문으로 커밋 | `git commit` (편집기에서 제목 → 빈 줄 → 본문) |
| 한 줄로 쓰기 (`-m` 마다 문단) | `git commit -m "제목" -m "본문"` |
| push 전에 마지막 커밋 고치기 | `git commit --amend` |

**VS Code 로 할 때** (소스 제어 `Ctrl+Shift+G`)
- diff 에서 줄 드래그 → 우클릭 → **Stage Selected Ranges**
- 잘못 담았으면 **Staged Changes** 쪽에서 → **Unstage Selected Ranges**
- 메시지 칸 : 첫 줄 제목, 둘째 줄 빈 줄, 셋째 줄부터 본문 → `Ctrl+Enter`
- "스테이징된 게 없다" 알림에서 **Yes** 를 누르면 전부 한 커밋이 된다 → **Cancel**

> ⚠️ push 한 뒤에는 `--amend` · rebase 금지 (force push 가 필요해진다). 고칠 게 있으면 새 커밋을 더한다.

---

## 올리기 · PR

| 상황 | 명령 |
|---|---|
| 새 브랜치 처음 올리기 | `git push -u origin HEAD` |
| 그다음부터 | `git push` |
| dev 로 PR | `gh pr create --base dev --fill` |
| 공용 기능 브랜치로 PR | `gh pr create --base feature-fork --fill` |
| 올리기 전 내 커밋 훑어보기 | `git log --oneline --stat origin/dev..HEAD` |

> ⚠️ 웹에서 PR 을 만들면 base 가 기본값 `main` 으로 잡힌다. 꼭 바꾼다.
> 커밋 전에 `pull` 할 필요 없다. 내 이슈 브랜치는 나만 쓰니 받아 올 게 없다. 충돌은 PR 에서 `fixed-` 브랜치로 푼다 ([[git 브랜치전략]]).

---

## 잠시 치워 두기 · 되돌리기

| 상황 | 명령 |
|---|---|
| 커밋 안 한 변경을 잠시 치우기 | `git stash` |
| 치운 것 다시 꺼내기 (원래 브랜치에서) | `git stash pop` |
| 치워 둔 목록 | `git stash list` |
| 파일 하나를 마지막 커밋 상태로 | `git restore 파일` ⚠️ |
| 다른 브랜치에 있는 파일만 꺼내 오기 (커밋 X) | `git restore --source=origin/main -- 파일1 파일2` |

> ⚠️ `git restore 파일` 은 그 파일의 **커밋 안 한 변경을 전부** 지운다. 임시 코드 한 줄만 빼려면 그 줄만 지우거나 VS Code diff 에서 그 덩어리만 Revert.
> stash 는 꺼낼 브랜치를 기억해 두기 (예 : load 브랜치용 변경은 load 브랜치에서 `pop`).

---

## 커밋하면 안 되는 파일 숨기기

| 상황 | 방법 |
|---|---|
| 나만 쓰는 도구 · 로그 파일을 `git status` 에서 숨기기 | `.git/info/exclude` 에 경로 한 줄씩 (`printf 'pintos-tui\nscripts/\n.tui/\n' >> .git/info/exclude`) |
| 팀 전체가 숨길 파일 | `.gitignore` (커밋해서 공유) |

> `.git/info/exclude` 는 내 컴퓨터에만 적용되고 저장소에 안 올라간다.

---

## 브랜치 정리

| 상황 | 명령 |
|---|---|
| 원격에서 지워진 브랜치 흔적까지 받아 오기 | `git fetch origin --prune` |
| dev 에 이미 머지된 브랜치 목록 | `git branch --merged origin/dev` |
| 로컬 브랜치 지우기 (머지된 것만) | `git branch -d 브랜치` |
| 원격 브랜치 지우기 | PR 화면 **Delete branch** 버튼, 또는 `git push origin --delete 브랜치` |
| 지운 원격 브랜치 되살리기 | 머지된 PR 화면 **Restore branch** |

> ⚠️ `git branch -D` (대문자) 는 머지 안 된 브랜치도 지운다 → 커밋이 사라질 수 있어 쓰지 않는다.
> 지금 서 있는 브랜치는 지울 수 없다 → 다른 브랜치로 옮긴 뒤 지운다.
> 팀원 브랜치는 각자 지운다. 머지된 PR 이라도 먼저 물어보기 (예 : `merge-dev` — 동호 님).
> `main` · `dev` 는 보호 규칙으로 삭제가 막혀 있다.

---

## 줄 끝(CRLF) 문제 — Windows 에서 받은 저장소를 컨테이너에서 쓸 때

| 증상 | 뜻 |
|---|---|
| `'ash: … syntax error near unexpected token 'fi'` | 스크립트 줄 끝에 `\r` 이 붙어 있다 |
| `/usr/bin/env: 'bash\r': No such file or directory` | 같은 원인 |

| 상황 | 명령 |
|---|---|
| `\r` 이 몇 줄에 있나 세기 (`file` 명령이 없을 때) | `grep -c $'\r' 파일` |
| 줄 끝을 LF 로 되돌리기 (텍스트만) | `grep -Il $'\r' 파일들 \| xargs sed -i 's/\r$//'` |
| 줄 끝 차이를 빼고 실제 변경만 보기 | `git diff --ignore-cr-at-eol --stat` |

> 저장소에 CRLF 로 올라간 파일을 LF 로 바꾸면 `git status` 에 `M` 으로 뜬다 → 커밋하지 않는다. 팀 전체 정리는 따로 합의 후 `chore:` 커밋 + `.gitattributes`.

---

관련: [[git 브랜치전략]], [[git 커밋 컨벤션]], [[도커 상황별 명령어]]
