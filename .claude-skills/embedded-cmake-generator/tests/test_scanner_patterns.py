"""Tests for scanner pattern matching and exclusion helpers.

Covers ``_match_simple_pattern``, ``_should_exclude_path``,
``_should_exclude_file``, and ``_should_exclude_dir_name``
from ``embedded_cmake.scanner``.
"""
from __future__ import annotations

from embedded_cmake.models import ScanConfig
from embedded_cmake.scanner import (
    _match_simple_pattern,
    _should_exclude_dir_name,
    _should_exclude_file,
    _should_exclude_path,
)


# ---------------------------------------------------------------------------
# _match_simple_pattern
# ---------------------------------------------------------------------------

class TestMatchSimplePattern:
    """Exhaustive coverage of the simple glob pattern matcher."""

    def test_wildcard_matches_everything(self):
        """``*`` alone matches any string."""
        assert _match_simple_pattern("anything", "*") is True
        assert _match_simple_pattern("", "*") is True

    def test_wildcard_contains(self):
        """``*text*`` matches when text is contained anywhere."""
        assert _match_simple_pattern("foobar", "*ob*") is True
        assert _match_simple_pattern("foobar", "*x*") is False

    def test_wildcard_suffix(self):
        """``*suffix`` matches when string ends with suffix."""
        assert _match_simple_pattern("foobar", "*bar") is True
        assert _match_simple_pattern("foobar", "*foo") is False

    def test_wildcard_prefix(self):
        """``prefix*`` matches when string starts with prefix."""
        assert _match_simple_pattern("foobar", "foo*") is True
        assert _match_simple_pattern("foobar", "bar*") is False

    def test_question_mark(self):
        """``?`` matches any single character via fnmatch."""
        assert _match_simple_pattern("foo", "fo?") is True
        assert _match_simple_pattern("foo", "f??") is True
        assert _match_simple_pattern("foo", "????") is False

    def test_exact_match(self):
        """Exact string comparison when no wildcard chars present."""
        assert _match_simple_pattern("foo", "foo") is True
        assert _match_simple_pattern("foo", "bar") is False

    def test_empty_pattern_does_not_match_nonempty(self):
        """Empty pattern only matches via fnmatch or exact."""
        assert _match_simple_pattern("foo", "") is False

    def test_empty_string_matches_wildcard(self):
        """Empty string still matches ``*``."""
        assert _match_simple_pattern("", "*") is True

    def test_dot_in_name(self):
        """Dots are matched literally."""
        assert _match_simple_pattern("file.c", "*.c") is True
        assert _match_simple_pattern("file.cpp", "*.c") is False

    def test_path_separator_not_special(self):
        """Path separator is treated as a regular character."""
        assert _match_simple_pattern("a/b/c", "*b*") is True

    def test_multiple_wildcards(self):
        """Multiple wildcard chars are handled."""
        # Simple matcher only handles wildcards at start/end;
        # middle wildcards like ``a*c`` fall through to exact match.
        assert _match_simple_pattern("a_b_c", "*_c") is True   # suffix wildcard
        assert _match_simple_pattern("aXbYc", "a*") is True    # prefix wildcard

    def test_leading_wildcard_with_period(self):
        """``*.ext`` style pattern."""
        assert _match_simple_pattern("build", "*uild") is True
        assert _match_simple_pattern("build", "*ld") is True


# ---------------------------------------------------------------------------
# _should_exclude_dir_name
# ---------------------------------------------------------------------------

class TestShouldExcludeDirName:
    """Verify directory name exclusion logic."""

    def test_exact_match_excludes(self):
        """Exact match in exclude_dirs excludes the dir name."""
        config = ScanConfig(exclude_dirs=["build", ".git"])
        assert _should_exclude_dir_name("build", config) is True

    def test_no_match_does_not_exclude(self):
        """Name not in exclude_dirs is not excluded."""
        config = ScanConfig(exclude_dirs=["build"])
        assert _should_exclude_dir_name("src", config) is False

    def test_pattern_match_excludes(self):
        """Matching exclude_dir_pattern excludes the dir name."""
        config = ScanConfig(exclude_dir_patterns=["*Temp*"])
        assert _should_exclude_dir_name("Template", config) is True

    def test_pattern_no_match(self):
        """Non-matching pattern does not exclude."""
        config = ScanConfig(exclude_dir_patterns=["*Temp*"])
        assert _should_exclude_dir_name("Sources", config) is False


# ---------------------------------------------------------------------------
# _should_exclude_file
# ---------------------------------------------------------------------------

class TestShouldExcludeFile:
    """Verify file name exclusion logic."""

    def test_exact_match_excludes(self):
        """Exact match in exclude_files excludes the file."""
        config = ScanConfig(exclude_files=["syscalls.c"])
        assert _should_exclude_file("syscalls.c", config) is True

    def test_no_match_does_not_exclude(self):
        """File not in exclude_files is not excluded."""
        config = ScanConfig(exclude_files=["syscalls.c"])
        assert _should_exclude_file("main.c", config) is False

    def test_pattern_match_excludes(self):
        """Matching exclude_file_pattern excludes the file."""
        config = ScanConfig(exclude_file_patterns=["*template*"])
        assert _should_exclude_file("some_template.c", config) is True

    def test_pattern_no_match(self):
        """Non-matching pattern does not exclude."""
        config = ScanConfig(exclude_file_patterns=["*template*"])
        assert _should_exclude_file("main.c", config) is False


# ---------------------------------------------------------------------------
# _should_exclude_path
# ---------------------------------------------------------------------------

class TestShouldExcludePath:
    """Verify full path exclusion logic (checks each path component)."""

    def test_exclude_path_with_build_dir(self):
        """Path containing a build/ directory is excluded."""
        config = ScanConfig(exclude_dirs=["build"])
        assert _should_exclude_path("/project/build/release", config) is True

    def test_exclude_path_with_git_dir(self):
        """Path containing .git is excluded."""
        config = ScanConfig(exclude_dirs=[".git"])
        assert _should_exclude_path("/project/.git/objects", config) is True

    def test_clean_path_not_excluded(self):
        """Path with no excluded components is not excluded."""
        config = ScanConfig(exclude_dirs=["build"])
        assert _should_exclude_path("/project/src/main", config) is False

    def test_exclude_with_pattern(self):
        """Path containing a component matching a pattern is excluded."""
        config = ScanConfig(exclude_dir_patterns=["*Temp*"])
        assert _should_exclude_path("/project/Templates/foo", config) is True

    def test_root_path_check(self):
        """The root directory itself is checked against exclude lists."""
        config = ScanConfig(exclude_dirs=["build"])
        assert _should_exclude_path("build", config) is True
