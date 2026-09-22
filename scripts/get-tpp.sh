#!/usr/bin/env bash

# Sourcing this file (instead of executing it) registers bash tab-completion
# for its own targets and returns, without running the downloader/builder:
#   source ./scripts/get-tpp.sh
if [[ -n "${BASH_SOURCE:-}" && "${BASH_SOURCE[0]}" != "${0}" ]]; then
  _get_tpp_completions() {
    local cur script
    cur="${COMP_WORDS[COMP_CWORD]}"
    script="${BASH_SOURCE[0]}"
    COMPREPLY=($(compgen -W "$("$script" --list-targets)" -- "$cur"))
  }
  complete -F _get_tpp_completions get-tpp "${BASH_SOURCE[0]}"
  return 0
fi

set -euo pipefail

# Temp/build directories registered here are removed on exit (including error
# exits), regardless of which function scope created them.
CLEANUP_DIRS=()
cleanup_registered_dirs() {
  local dir
  for dir in "${CLEANUP_DIRS[@]:-}"; do
    [[ -n "$dir" ]] && rm -rf "$dir"
  done
}
trap cleanup_registered_dirs EXIT

AVAILABLE_TARGETS=(
  tpp
  tpp2cpp
  tpp2java
  tpp2swift
  render-tpp
  tpp-lsp
  vscode-extension
)

DEFAULT_REPO="AnarchoSystems/tpp"

usage() {
  cat <<'EOF'
Usage:
  get-tpp [-o <output>] [-min-version <tag>] [-max-version <tag>] <target> [target...]
  get-tpp [-o <output>] -exact-version <tag> <target> [target...]
  get-tpp [-o <output>] -build-from-source <target> [target...]
  get-tpp [version options] [-o <output>] all

Targets:
  tpp, tpp2cpp, tpp2java, tpp2swift, render-tpp, tpp-lsp, vscode-extension

Notes:
  - -o defaults to the current directory if omitted.
  - Version tags must use the stable vMAJOR.MINOR.PATCH form.
  - -min-version and -max-version are inclusive and may be combined.
  - -exact-version cannot be combined with a minimum or maximum version.
  - Without version options, the latest GitHub release is used.
  - Use a directory with multiple targets.
  - With a single target, -o may be either a directory or a file path.
  - vscode-extension resolves to a .vsix package rather than a binary.
  - If no matching release artifact exists for the current OS/arch, the
    script clones the selected revision and builds the requested targets from
    source.
  - `source ./scripts/get-tpp.sh` (instead of executing it) registers bash
    tab-completion for targets and returns immediately.
EOF
}

die() {
  printf 'get-tpp: %s\n' "$*" >&2
  exit 1
}

is_dir_like() {
  [[ -d "$1" || "$1" == */ ]]
}

normalize_os() {
  case "$(uname -s)" in
    Linux*) printf 'linux' ;;
    Darwin*) printf 'darwin' ;;
    MINGW*|MSYS*|CYGWIN*|Windows_NT) printf 'win32' ;;
    *) printf '%s' "$(uname -s | tr '[:upper:]' '[:lower:]')" ;;
  esac
}

normalize_arch() {
  case "$(uname -m)" in
    x86_64|amd64) printf 'x64' ;;
    arm64|aarch64) printf 'arm64' ;;
    *) printf '%s' "$(uname -m | tr '[:upper:]' '[:lower:]')" ;;
  esac
}

repo_slug() {
  local origin_url
  origin_url=$(git remote get-url origin 2>/dev/null) || return 1
  case "$origin_url" in
    git@github.com:*) printf '%s' "${origin_url#git@github.com:}" | sed 's#\.git$##' ;;
    https://github.com/*) printf '%s' "${origin_url#https://github.com/}" | sed 's#\.git$##' ;;
    http://github.com/*) printf '%s' "${origin_url#http://github.com/}" | sed 's#\.git$##' ;;
    *) return 1 ;;
  esac
}

latest_release_tag() {
  local repo="$1"
  curl -fsSL "https://api.github.com/repos/${repo}/releases/latest" \
    | sed -n 's/^[[:space:]]*"tag_name": "\([^"]*\)".*/\1/p' \
    | head -n1
}

release_tags() {
  local repo="$1"
  curl -fsSL "https://api.github.com/repos/${repo}/releases?per_page=100" \
    | sed -n 's/^[[:space:]]*"tag_name": "\(v[0-9][0-9]*\.[0-9][0-9]*\.[0-9][0-9]*\)".*/\1/p'
}

validate_version_tag() {
  local tag="$1"
  [[ "$tag" =~ ^v(0|[1-9][0-9]*)\.(0|[1-9][0-9]*)\.(0|[1-9][0-9]*)$ ]] \
    || die "invalid version tag '$tag' (expected vMAJOR.MINOR.PATCH)"
}

version_compare() {
  local left="${1#v}"
  local right="${2#v}"
  local left_major left_minor left_patch right_major right_minor right_patch
  IFS=. read -r left_major left_minor left_patch <<< "$left"
  IFS=. read -r right_major right_minor right_patch <<< "$right"

  local index
  local -a left_parts=("$left_major" "$left_minor" "$left_patch")
  local -a right_parts=("$right_major" "$right_minor" "$right_patch")
  for index in 0 1 2; do
    if (( 10#${left_parts[$index]} < 10#${right_parts[$index]} )); then
      printf '%s' -1
      return
    fi
    if (( 10#${left_parts[$index]} > 10#${right_parts[$index]} )); then
      printf '%s' 1
      return
    fi
  done
  printf '%s' 0
}

select_release_tag() {
  local repo="$1"
  local min_version="$2"
  local max_version="$3"
  local selected=""
  local tag

  while IFS= read -r tag; do
    [[ -n "$tag" ]] || continue
    if [[ -n "$min_version" ]] && (( $(version_compare "$tag" "$min_version") < 0 )); then
      continue
    fi
    if [[ -n "$max_version" ]] && (( $(version_compare "$tag" "$max_version") > 0 )); then
      continue
    fi
    if [[ -z "$selected" ]] || (( $(version_compare "$tag" "$selected") > 0 )); then
      selected="$tag"
    fi
  done < <(release_tags "$repo")

  printf '%s' "$selected"
}

release_asset_url() {
  local repo="$1"
  local tag="$2"
  local target="$3"
  local os_name="$4"
  local arch_name="$5"
  printf 'https://github.com/%s/releases/download/%s/%s-%s-%s.tar.gz' "$repo" "$tag" "$target" "$os_name" "$arch_name"
}

RELEASE_JSON_CACHE=""

fetch_release_json() {
  local repo="$1"
  local tag="$2"
  if [[ -z "$RELEASE_JSON_CACHE" ]]; then
    RELEASE_JSON_CACHE=$(curl -fsSL "https://api.github.com/repos/${repo}/releases/tags/${tag}") || return 1
  fi
  printf '%s' "$RELEASE_JSON_CACHE"
}

# vscode-extension is packaged as a single platform-independent .vsix asset,
# so it can't be located via the {target}-{os}-{arch}.tar.gz naming scheme.
vsix_asset_url() {
  local repo="$1"
  local tag="$2"
  local json
  json=$(fetch_release_json "$repo" "$tag") || return 1
  printf '%s' "$json" | sed -n 's/^[[:space:]]*"browser_download_url": "\(.*\.vsix\)".*/\1/p' | head -n1
}

resolve_release_url() {
  local repo="$1"
  local tag="$2"
  local target="$3"
  local os_name="$4"
  local arch_name="$5"
  if [[ "$target" == vscode-extension ]]; then
    vsix_asset_url "$repo" "$tag"
    return
  fi
  release_asset_url "$repo" "$tag" "$target" "$os_name" "$arch_name"
}

validate_targets() {
  local target
  for target in "$@"; do
    local ok=false
    for available in "${AVAILABLE_TARGETS[@]}"; do
      if [[ "$target" == "$available" ]]; then
        ok=true
        break
      fi
    done
    if [[ "$ok" == false ]]; then
      die "unknown target '$target'"
    fi
  done
}

download_release_artifact() {
  local url="$1"
  local destination="$2"
  curl -fL --retry 2 --retry-delay 1 -o "$destination" "$url"
}

extract_single_binary() {
  local archive="$1"
  local destination="$2"
  local temp_dir
  temp_dir=$(mktemp -d)
  CLEANUP_DIRS+=("$temp_dir")
  tar -xzf "$archive" -C "$temp_dir"

  local extracted
  extracted=$(find "$temp_dir" -maxdepth 1 -type f | head -n1)
  [[ -n "$extracted" ]] || die "archive '$archive' did not contain a binary"
  mkdir -p "$(dirname "$destination")"
  cp "$extracted" "$destination"
  chmod +x "$destination"
}

copy_binary_to_dir() {
  local archive="$1"
  local output_dir="$2"
  local temp_dir
  temp_dir=$(mktemp -d)
  CLEANUP_DIRS+=("$temp_dir")
  tar -xzf "$archive" -C "$temp_dir"

  local extracted
  extracted=$(find "$temp_dir" -maxdepth 1 -type f | head -n1)
  [[ -n "$extracted" ]] || die "archive '$archive' did not contain a binary"
  mkdir -p "$output_dir"
  cp "$extracted" "$output_dir/"
  chmod +x "$output_dir/$(basename "$extracted")"
}

# Places a downloaded .vsix (not a tar.gz archive) at the requested destination.
place_vsix() {
  local downloaded_file="$1"
  local source_url="$2"
  local destination="$3"
  local file_name
  file_name=$(basename "$source_url")
  if is_dir_like "$destination"; then
    mkdir -p "$destination"
    cp "$downloaded_file" "$destination/$file_name"
  else
    mkdir -p "$(dirname "$destination")"
    cp "$downloaded_file" "$destination"
  fi
}

build_from_source() {
  local repo="$1"
  local tag="$2"
  local source_dir="$3"

  command -v git >/dev/null 2>&1 || die "git is required to build from source"

  local -a clone_args=(clone --depth 1)
  if [[ -n "$tag" ]]; then
    clone_args+=(--branch "$tag")
  fi
  clone_args+=("https://github.com/${repo}.git" "$source_dir")
  git "${clone_args[@]}"
}

main() {
  local output_path=""
  local build_from_source_only=false
  local min_version=""
  local max_version=""
  local exact_version=""
  local -a requested_targets=()

  while [[ $# -gt 0 ]]; do
    case "$1" in
      -o)
        [[ $# -ge 2 ]] || die "-o requires a path"
        output_path="$2"
        shift 2
        ;;
      -min-version|--min-version)
        [[ $# -ge 2 ]] || die "$1 requires a git tag"
        min_version="$2"
        shift 2
        ;;
      -max-version|--max-version)
        [[ $# -ge 2 ]] || die "$1 requires a git tag"
        max_version="$2"
        shift 2
        ;;
      -exact-version|--exact-version)
        [[ $# -ge 2 ]] || die "$1 requires a git tag"
        exact_version="$2"
        shift 2
        ;;
      -h|--help)
        usage
        exit 0
        ;;
      --list-targets)
        printf '%s\n' "${AVAILABLE_TARGETS[@]}" all
        exit 0
        ;;
      -build-from-source|--build-from-source)
        build_from_source_only=true
        shift
        ;;
      --)
        shift
        break
        ;;
      -*)
        die "unknown option '$1'"
        ;;
      *)
        requested_targets+=("$1")
        shift
        ;;
    esac
  done

  while [[ $# -gt 0 ]]; do
    requested_targets+=("$1")
    shift
  done

  [[ -n "$output_path" ]] || output_path="."
  [[ ${#requested_targets[@]} -gt 0 ]] || die "missing target name"

  [[ -z "$min_version" ]] || validate_version_tag "$min_version"
  [[ -z "$max_version" ]] || validate_version_tag "$max_version"
  [[ -z "$exact_version" ]] || validate_version_tag "$exact_version"
  if [[ -n "$exact_version" && ( -n "$min_version" || -n "$max_version" ) ]]; then
    die "-exact-version cannot be combined with -min-version or -max-version"
  fi
  if [[ -n "$min_version" && -n "$max_version" ]] \
      && (( $(version_compare "$min_version" "$max_version") > 0 )); then
    die "-min-version cannot be greater than -max-version"
  fi

  if [[ ${#requested_targets[@]} -eq 1 && ${requested_targets[0]} == all ]]; then
    requested_targets=("${AVAILABLE_TARGETS[@]}")
  fi

  validate_targets "${requested_targets[@]}"

  local os_name arch_name repo tag
  os_name=$(normalize_os)
  arch_name=$(normalize_arch)
  repo="$DEFAULT_REPO"
  tag=""

  if [[ -n "$exact_version" ]]; then
    tag="$exact_version"
  elif [[ -n "$repo" && ( -n "$min_version" || -n "$max_version" ) ]]; then
    tag=$(select_release_tag "$repo" "$min_version" "$max_version" || true)
  elif [[ -n "$repo" ]]; then
    tag=$(latest_release_tag "$repo" || true)
  fi

  if [[ -z "$tag" && ( -n "$exact_version" || -n "$min_version" || -n "$max_version" ) ]]; then
    die "no published release matches the requested version"
  fi

  local use_release=true
  if [[ "$build_from_source_only" == true ]]; then
    use_release=false
  fi
  local -a release_urls=()
  local target
  if [[ -n "$tag" ]]; then
    for target in "${requested_targets[@]}"; do
      local url
      url=$(resolve_release_url "$repo" "$tag" "$target" "$os_name" "$arch_name")
      if [[ -n "$url" ]] && curl -fsIL "$url" >/dev/null 2>&1; then
        release_urls+=("$url")
      else
        use_release=false
        break
      fi
    done
  else
    use_release=false
  fi

  if [[ "$use_release" == true ]]; then
    local temp_dir
    temp_dir=$(mktemp -d)
    CLEANUP_DIRS+=("$temp_dir")

    if [[ ${#requested_targets[@]} -eq 1 ]]; then
      local archive_path="$temp_dir/${requested_targets[0]}.download"
      download_release_artifact "${release_urls[0]}" "$archive_path"
      if [[ "${requested_targets[0]}" == vscode-extension ]]; then
        place_vsix "$archive_path" "${release_urls[0]}" "$output_path"
      elif is_dir_like "$output_path"; then
        copy_binary_to_dir "$archive_path" "$output_path"
      else
        extract_single_binary "$archive_path" "$output_path"
      fi
      printf 'downloaded %s from %s\n' "${requested_targets[0]}" "$tag"
      return 0
    fi

    is_dir_like "$output_path" || die "multiple targets require -o to point to a directory"
    mkdir -p "$output_path"

    local index=0
    for target in "${requested_targets[@]}"; do
      local archive_path="$temp_dir/${target}.download"
      download_release_artifact "${release_urls[$index]}" "$archive_path"
      if [[ "$target" == vscode-extension ]]; then
        place_vsix "$archive_path" "${release_urls[$index]}" "$output_path"
      else
        copy_binary_to_dir "$archive_path" "$output_path"
      fi
      index=$((index + 1))
    done

    printf 'downloaded %d targets from %s\n' "${#requested_targets[@]}" "$tag"
    return 0
  fi

  printf 'release %s does not provide all requested artifacts for %s/%s; building from source\n' \
    "${tag:-default branch}" "$os_name" "$arch_name" >&2
  local source_dir
  source_dir=$(mktemp -d)
  CLEANUP_DIRS+=("$source_dir")
  build_from_source "$repo" "$tag" "$source_dir"

  # vscode-extension isn't a cmake target; it's packaged separately via npm.
  local -a cmake_targets=()
  local build_vscode_extension=false
  for target in "${requested_targets[@]}"; do
    if [[ "$target" == vscode-extension ]]; then
      build_vscode_extension=true
    else
      cmake_targets+=("$target")
    fi
  done

  local build_dir="" build_output=""
  if [[ ${#cmake_targets[@]} -gt 0 ]]; then
    build_dir=$(mktemp -d)
    CLEANUP_DIRS+=("$build_dir")
    cmake -S "$source_dir" -B "$build_dir" -DCMAKE_BUILD_TYPE=Release
    cmake --build "$build_dir" --config Release --target "${cmake_targets[@]}" --parallel
    build_output="$build_dir/bin"
  fi

  local vsix_path=""
  if [[ "$build_vscode_extension" == true ]]; then
    (cd "$source_dir/vscode-extension" && npm install && npm run package)
    vsix_path=$(find "$source_dir/vscode-extension" -maxdepth 1 -name '*.vsix' -type f | head -n1)
    [[ -n "$vsix_path" ]] || die "built .vsix not found in $source_dir/vscode-extension/"
  fi

  if [[ ${#requested_targets[@]} -eq 1 ]]; then
    if [[ "${requested_targets[0]}" == vscode-extension ]]; then
      place_vsix "$vsix_path" "$vsix_path" "$output_path"
    else
      local binary_name="${requested_targets[0]}"
      if [[ "$os_name" == win32 ]]; then
        binary_name+='.exe'
      fi
      local source_path="$build_output/$binary_name"
      [[ -f "$source_path" ]] || die "built binary not found at $source_path"
      local destination_path
      if is_dir_like "$output_path"; then
        mkdir -p "$output_path"
        destination_path="$output_path/$binary_name"
      else
        mkdir -p "$(dirname "$output_path")"
        destination_path="$output_path"
      fi
      cp "$source_path" "$destination_path"
      chmod +x "$destination_path"
    fi
    return 0
  fi

  is_dir_like "$output_path" || die "multiple targets require -o to point to a directory"
  mkdir -p "$output_path"
  local built_target source_path output_name
  for built_target in "${requested_targets[@]}"; do
    if [[ "$built_target" == vscode-extension ]]; then
      place_vsix "$vsix_path" "$vsix_path" "$output_path"
      continue
    fi
    output_name="$built_target"
    if [[ "$os_name" == win32 ]]; then
      output_name+='.exe'
    fi
    source_path="$build_output/$output_name"
    [[ -f "$source_path" ]] || die "built binary not found at $source_path"
    cp "$source_path" "$output_path/"
    chmod +x "$output_path/$output_name"
  done
}

main "$@"
