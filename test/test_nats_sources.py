#!/usr/bin/env python3
# Copyright (C) 2026 Qore Technologies, s.r.o.
# SPDX-License-Identifier: MIT
"""Check pinned NATS source preparation without modifying downloaded sources."""
import argparse
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
SOURCE = None
FILES = ('msg.c', 'url.c', 'util.c', 'glib/glib_last_error.c')


class NatsSourcesTest(unittest.TestCase):
    def setUp(self):
        temporary = tempfile.TemporaryDirectory(prefix='nats const sources ')
        self.addCleanup(temporary.cleanup)
        self.root = Path(temporary.name)
        self.source = self.root / 'upstream'
        for file in FILES:
            target = self.source / 'src' / file
            target.parent.mkdir(parents=True, exist_ok=True)
            shutil.copyfile(SOURCE / 'src' / file, target)
        self.project = self.root / 'project'
        self.project.mkdir()
        self.build = self.root / 'build'
        sources = ' '.join(f'"{self.source}/src/{file}"' for file in FILES)
        native = self.project / 'native'
        native.mkdir()
        (native / 'CMakeLists.txt').write_text(f'add_library(nats_static STATIC {sources})\n')
        (self.project / 'CMakeLists.txt').write_text(f'''
cmake_minimum_required(VERSION 3.21...3.31)
project(NatsSourceTest C)
add_subdirectory(native)
include("{ROOT}/cmake/PrepareNatsSources.cmake")
qore_prepare_nats_sources("{self.source}")
get_target_property(prepared nats_static SOURCES)
file(WRITE "${{CMAKE_BINARY_DIR}}/sources.txt" "${{prepared}}")
''')

    def configure(self):
        return subprocess.run(['cmake', '-S', str(self.project), '-B', str(self.build)],
                              text=True, capture_output=True, timeout=30)

    def test_private_copies_preserve_upstream_and_reconfigure_without_rewrites(self):
        original = {file: (self.source / 'src' / file).read_bytes() for file in FILES}
        result = self.configure()
        self.assertEqual(0, result.returncode, result.stdout + result.stderr)
        self.assertEqual('', result.stderr)
        copies = [self.build / 'nats-sources' / file for file in FILES]
        compiled = (self.build / 'sources.txt').read_text().split(';')
        self.assertEqual({str(path) for path in copies}, set(compiled))
        for file, copy in zip(FILES, copies):
            self.assertEqual(original[file], (self.source / 'src' / file).read_bytes())
            self.assertNotEqual(original[file], copy.read_bytes())
        times = [copy.stat().st_mtime_ns for copy in copies]
        result = self.configure()
        self.assertEqual(0, result.returncode, result.stdout + result.stderr)
        self.assertEqual('', result.stderr)
        self.assertEqual(times, [copy.stat().st_mtime_ns for copy in copies])

    def test_changed_upstream_source_is_rejected(self):
        path = self.source / 'src/msg.c'
        path.write_text(path.read_text() + '\n/* changed upstream */\n')
        result = self.configure()
        self.assertNotEqual(0, result.returncode)
        self.assertIn('source changed: msg.c', result.stderr)

    def test_missing_target_source_is_rejected(self):
        path = self.project / 'native/CMakeLists.txt'
        path.write_text(path.read_text().replace(f'"{self.source}/src/url.c"', ''))
        result = self.configure()
        self.assertNotEqual(0, result.returncode)
        self.assertIn('does not compile expected source:', result.stderr)
        self.assertIn('url.c', result.stderr)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source-dir', type=Path, required=True)
    args, tests = parser.parse_known_args()
    SOURCE = args.source_dir.resolve()
    unittest.main(argv=[__file__, *tests])
