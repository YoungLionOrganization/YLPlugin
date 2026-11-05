# SPDX-FileCopyrightText: 2026 Cavanshirpro
# SPDX-License-Identifier: LGPL-3.0-only

"""Offline checks of Wiki link portability and publication source inventory."""
import importlib.util
from pathlib import Path
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


if __name__ == '__main__':
    unittest.main()
