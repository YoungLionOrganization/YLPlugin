# SPDX-FileCopyrightText: 2026 Cavanshirpro
# SPDX-License-Identifier: LGPL-3.0-only

"""Offline checks of Wiki link portability and publication source inventory."""
import importlib.util
from pathlib import Path
import json
import io
from contextlib import redirect_stdout
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location('publish_wiki', ROOT / 'tools/publish_wiki.py')
wiki = importlib.util.module_from_spec(spec)
spec.loader.exec_module(wiki)


class WikiLinks(unittest.TestCase):
    def test_all_pages_and_navigation_resolve(self):
        pages = wiki.render_pages()
        self.assertGreaterEqual(len(pages), 20)
        self.assertIn('/wiki/Getting-Started', pages['Home.md'])
        self.assertIn('/blob/main/docs/specification.md', pages['Home.md'])
        self.assertIn('/blob/main/LICENSE', pages['_Footer.md'])

    def test_anchors_and_external_links_preserved(self):
        page = ROOT / 'docs/wiki/Home.md'
        result = wiki.render_text(page, '[A](Container-and-Profile.md#independent-versions) [B](https://example.org/x)')
        self.assertIn('/wiki/Container-and-Profile#independent-versions', result)
        self.assertIn('[B](https://example.org/x)', result)

    def test_code_examples_not_rewritten(self):
        text = '```text\n[X](not-a-page.md)\n```\n`[Y](not-a-page.md)`\n'
        self.assertEqual(wiki.render_text(ROOT / 'docs/wiki/Home.md', text), text)

    def test_broken_or_escaping_link_rejected(self):
        for target in ('No-Such-Page.md', '../../../../outside.md'):
            with self.assertRaises(ValueError):
                wiki.render_text(ROOT / 'docs/wiki/Home.md', f'[broken]({target})')

    def test_unclosed_fence_rejected(self):
        with self.assertRaises(ValueError):
            wiki.render_text(ROOT / 'docs/wiki/Home.md', '```python\nprint(42)\n')

    def test_publication_preserves_other_pages_and_is_idempotent(self):
        def git(directory, *args):
            return subprocess.check_output(['git', '-C', str(directory), *args], text=True, stderr=subprocess.DEVNULL).strip()
        def commit(directory, message):
            git(directory, '-c', 'user.name=Wiki Test', '-c', 'user.email=wiki-test@example.invalid', 'commit', '-m', message)
        with tempfile.TemporaryDirectory(prefix='ylplugin-wiki-test-') as temporary:
            base = Path(temporary); source = base / 'source'; source.mkdir()
            git(source, 'init', '--initial-branch=main')
            pages = {'Home.md': '# Updated Home\n', '_Sidebar.md': '## Navigation\n', '_Footer.md': 'Footer\n'}
            (source / 'docs/wiki').mkdir(parents=True)
            for name, content in pages.items():
                (source / 'docs/wiki' / name).write_text(content)
            for name in ('LICENSE', 'COPYING'):
                (source / name).write_bytes((ROOT / name).read_bytes())
            git(source, 'add', '.'); commit(source, 'Source')
            remote = base / 'wiki.git'; remote.mkdir()
            git(remote, 'init', '--bare', '--initial-branch=main')
            seed = base / 'seed'
            subprocess.run(['git', 'clone', str(remote), str(seed)], check=True, capture_output=True)
            (seed / 'Home.md').write_text('# Initial Home\n')
            (seed / 'Existing-Notes.md').write_text('Owner notes must stay\n')
            (seed / 'asset.bin').write_bytes(b'\x00\x01\xff')
            git(seed, 'add', '.'); commit(seed, 'Initialize Wiki'); git(seed, 'push', 'origin', 'HEAD')
            previous_root, previous_remote = wiki.ROOT, wiki.WIKI_REMOTE
            try:
                wiki.ROOT, wiki.WIKI_REMOTE = source, str(remote)
                manifest = {'pages': sorted(pages)}
                with redirect_stdout(io.StringIO()):
                    wiki.publish(pages, manifest)
                first = git(remote, 'rev-parse', 'HEAD')
                with redirect_stdout(io.StringIO()):
                    wiki.publish(pages, manifest)
                self.assertEqual(git(remote, 'rev-parse', 'HEAD'), first)
                self.assertEqual(git(remote, 'show', 'HEAD:Existing-Notes.md'), 'Owner notes must stay')
                self.assertEqual(git(remote, 'show', 'HEAD:Home.md'), '# Updated Home')
                self.assertEqual(git(remote, 'show', 'HEAD:LICENSE'), (ROOT / 'LICENSE').read_text().strip())
                recorded = json.loads(git(remote, 'show', 'HEAD:.ylplugin-wiki-source.json'))
                self.assertEqual(recorded['source_commit'], git(source, 'rev-parse', 'HEAD'))
                self.assertEqual(git(remote, 'log', '-1', '--format=%an'), 'Cavanshirpro')
                actual = subprocess.check_output(['git', '-C', str(remote), 'show', 'HEAD:asset.bin'])
                self.assertEqual(actual, b'\x00\x01\xff')
            finally:
                wiki.ROOT, wiki.WIKI_REMOTE = previous_root, previous_remote


if __name__ == '__main__':
    unittest.main()
