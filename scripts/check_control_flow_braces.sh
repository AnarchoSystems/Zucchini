#!/usr/bin/env bash
set -euo pipefail

# Enforce braces around control-flow bodies.
# Supported checks:
# - switch case/default labels must be followed by a block body
# - if / else / else if / for / while / do bodies must start with '{'

scope="${TPP_CASE_BRACE_SCOPE:-all}"

collect_all_files() {
    git ls-files '*.[ch]' '*.cc' '*.cpp' '*.cxx' '*.hh' '*.hpp' '*.hxx'
}

collect_changed_files_for_refspec() {
    local refspec="$1"
    local output
    if output="$(git diff --name-only --diff-filter=ACMR "$refspec" -- '*.[ch]' '*.cc' '*.cpp' '*.cxx' '*.hh' '*.hpp' '*.hxx' 2>/dev/null)"; then
        if [[ -n "$output" ]]; then
            printf '%s\n' "$output"
        fi
        return 0
    fi
    return 1
}

collect_changed_files() {
    if [[ -n "${GITHUB_BASE_REF:-}" ]]; then
        git fetch --no-tags --depth=1 origin "$GITHUB_BASE_REF" >/dev/null 2>&1 || true
        if git rev-parse --verify "origin/$GITHUB_BASE_REF" >/dev/null 2>&1; then
            if collect_changed_files_for_refspec "origin/$GITHUB_BASE_REF...HEAD"; then
                return
            fi
            if collect_changed_files_for_refspec "origin/$GITHUB_BASE_REF..HEAD"; then
                return
            fi
        fi
    fi

    if git rev-parse --verify HEAD~1 >/dev/null 2>&1; then
        collect_changed_files_for_refspec "HEAD~1..HEAD"
        return
    fi

    if git rev-parse --verify HEAD >/dev/null 2>&1; then
        collect_changed_files_for_refspec "HEAD"
        return
    fi

    collect_all_files
}

populate_files() {
    local mode="$1"
    files=()
    while IFS= read -r filepath; do
        if [[ -n "$filepath" ]]; then
            files+=("$filepath")
        fi
    done < <(if [[ "$mode" == "changed" ]]; then collect_changed_files; else collect_all_files; fi)
}

if [[ $# -gt 0 ]]; then
    files=()
    for filepath in "$@"; do
        if [[ -n "$filepath" ]]; then
            files+=("$filepath")
        fi
    done
else
    populate_files "$scope"
fi

if [[ ${#files[@]} -eq 0 ]]; then
    echo "control-flow brace enforcement skipped (no matching files)"
    exit 0
fi

violations=0

for file in "${files[@]}"; do
    if [[ ! -f "$file" ]]; then
        continue
    fi

    awk -v file="$file" '
BEGIN {
    waiting_for_brace = 0;
    pending_line = 0;
    pending_kind = "";
    file_failed = 0;
}
{
    if (waiting_for_brace) {
        if (pending_kind == "case/default" &&
            $0 ~ /^[[:space:]]*(case[[:space:]].*|default)[[:space:]]*:[[:space:]]*$/) {
            pending_line = NR;
            next;
        }
        if ($0 ~ /^[[:space:]]*$/) {
            next;
        }
        if ($0 ~ /^[[:space:]]*\/\//) {
            next;
        }
        if ($0 ~ /^[[:space:]]*\/\*/) {
            next;
        }
        if ($0 ~ /^[[:space:]]*\{/) {
            waiting_for_brace = 0;
            pending_kind = "";
            next;
        }

        printf "%s:%d: %s body must start with '\''{\''\n", file, pending_line, pending_kind;
        waiting_for_brace = 0;
        pending_kind = "";
        file_failed = 1;
    }

    if ($0 ~ /^[[:space:]]*else[[:space:]]+if[[:space:]]*\(.*\)[[:space:]]*$/) {
        waiting_for_brace = 1;
        pending_line = NR;
        pending_kind = "else if";
        next;
    }

    if ($0 ~ /^[[:space:]]*else[[:space:]]*$/) {
        waiting_for_brace = 1;
        pending_line = NR;
        pending_kind = "else";
        next;
    }

    if ($0 ~ /^[[:space:]]*if[[:space:]]*\(.*\)[[:space:]]*$/) {
        waiting_for_brace = 1;
        pending_line = NR;
        pending_kind = "if";
        next;
    }

    if ($0 ~ /^[[:space:]]*for[[:space:]]*\(.*\)[[:space:]]*$/) {
        waiting_for_brace = 1;
        pending_line = NR;
        pending_kind = "for";
        next;
    }

    if ($0 ~ /^[[:space:]]*while[[:space:]]*\(.*\)[[:space:]]*$/) {
        waiting_for_brace = 1;
        pending_line = NR;
        pending_kind = "while";
        next;
    }

    if ($0 ~ /^[[:space:]]*do[[:space:]]*$/) {
        waiting_for_brace = 1;
        pending_line = NR;
        pending_kind = "do";
        next;
    }

    if ($0 ~ /^[[:space:]]*(case[[:space:]].*|default)[[:space:]]*:[[:space:]]*$/) {
        waiting_for_brace = 1;
        pending_line = NR;
        pending_kind = "case/default";
    }
}
END {
    if (file_failed) {
        exit 2;
    }
}
' "$file" || violations=1
done

if [[ $violations -ne 0 ]]; then
    echo "control-flow brace enforcement failed"
    exit 1
fi

echo "control-flow brace enforcement passed"
