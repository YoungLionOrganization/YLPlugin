# SPDX-FileCopyrightText: 2026 Cavanshirpro
# SPDX-License-Identifier: LGPL-3.0-only

"""Validate/render versioned Wiki pages; explicitly publish to an initialized Wiki."""
from pathlib import Path
import argparse
import hashlib
import json
import re
import subprocess
import tempfile
from urllib.parse import quote, unquote, urlsplit

ROOT = Path(__file__).resolve().parents[1]
REPOSITORY = 'https://github.com/YoungLionOrganization/YLPlugin'
WIKI_REMOTE = REPOSITORY + '.wiki.git'
LINK = re.compile(r'(?<!!)\[([^\]]+)\]\(([^)]+)\)')


def render_text(source, text, root=ROOT):
    wiki = root / 'docs/wiki'

    def replace(match):
        label, destination = match.groups()
        parsed = urlsplit(destination)
        if parsed.scheme or parsed.netloc or not parsed.path:
            return match.group(0)
        target = (source.parent / unquote(parsed.path)).resolve()
        if not target.is_relative_to(root.resolve()) or not target.is_file():
            raise ValueError(f'{source.name}: missing/outside-repository link {destination}')
        fragment = '#' + parsed.fragment if parsed.fragment else ''
        if target.parent == wiki.resolve() and target.suffix == '.md':
            url = REPOSITORY + '/wiki/' + quote(target.stem)
        else:
            url = REPOSITORY + '/blob/main/' + quote(target.relative_to(root.resolve()).as_posix())
        return f'[{label}]({url}{fragment})'

    output, fence = [], None
    for line in text.splitlines(keepends=True):
        marker = re.match(r'^\s*(`{3,}|~{3,})', line)
        if marker:
            current = marker.group(1)
            if fence is None:
                fence = current
            elif current[0] == fence[0] and len(current) >= len(fence):
                fence = None
            output.append(line)
        elif fence:
            output.append(line)
        else:
            # Inline code is explanatory text, not a Markdown link to rewrite.
            parts = re.split(r'(`+[^`]*`+)', line)
            output.append(''.join(part if part.startswith('`') else LINK.sub(replace, part) for part in parts))
    if fence:
        raise ValueError(f'{source.name}: unclosed Markdown code fence')
    return ''.join(output)


def render_pages(root=ROOT):
    source = root / 'docs/wiki'
    pages = sorted(source.glob('*.md'))
    required = {'Home.md', '_Sidebar.md', '_Footer.md'}
    if not required.issubset({page.name for page in pages}):
        raise ValueError('Wiki needs Home.md, _Sidebar.md and _Footer.md')
    output = {}
    for page in pages:
        if not re.fullmatch(r'[A-Za-z0-9_-]+\.md', page.name):
            raise ValueError(f'Unsupported Wiki page filename: {page.name}')
        text = page.read_text(encoding='utf-8')
        if not text.strip() or (page.name != '_Footer.md' and not text.lstrip().startswith('#')):
            raise ValueError(f'{page.name}: a page heading is required')
        output[page.name] = render_text(page, text, root)
    return output


def publish(pages, manifest):
    source_paths = [f'docs/wiki/{name}' for name in pages]
    tracked = set(subprocess.check_output(['git', 'ls-files', '--', 'docs/wiki'], cwd=ROOT, text=True).splitlines())
    if not set(source_paths).issubset(tracked):
        raise ValueError('Commit all Wiki sources before publishing')
    subprocess.run(['git', 'diff', '--exit-code', 'HEAD', '--', 'docs/wiki', 'tools/publish_wiki.py', 'LICENSE', 'COPYING'], cwd=ROOT, check=True)
    manifest['source_commit'] = subprocess.check_output(['git', 'rev-parse', 'HEAD'], cwd=ROOT, text=True).strip()
    with tempfile.TemporaryDirectory(prefix='ylplugin-wiki-') as temporary:
        checkout = Path(temporary) / 'wiki'
        subprocess.run(['git', 'clone', '--', WIKI_REMOTE, str(checkout)], check=True)
        for name, content in pages.items():
            (checkout / name).write_text(content, encoding='utf-8', newline='\n')
        for name in ('LICENSE', 'COPYING'):
            (checkout / name).write_bytes((ROOT / name).read_bytes())
        record = '.ylplugin-wiki-source.json'
        (checkout / record).write_text(json.dumps(manifest, indent=2) + '\n', encoding='utf-8', newline='\n')
        # Stage managed pages, license texts and manifest; preserve other content.
        subprocess.run(['git', 'add', '--', *sorted(pages), 'LICENSE', 'COPYING', record], cwd=checkout, check=True)
        changed = subprocess.run(['git', 'diff', '--cached', '--quiet'], cwd=checkout)
        if changed.returncode == 0:
            print('Wiki already matches the committed source')
            return
        if changed.returncode != 1:
            changed.check_returncode()
        subprocess.run(['git', '-c', 'user.name=Cavanshirpro', '-c',
                        'user.email=176774890+Cavanshirpro@users.noreply.github.com',
                        'commit', '-m', 'Publish detailed English YLPlugin Wiki'], cwd=checkout, check=True)
        subprocess.run(['git', 'push', 'origin', 'HEAD'], cwd=checkout, check=True)
        print('Published:', REPOSITORY + '/wiki')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--check', action='store_true', help='validate sources/links without writing or publishing')
    parser.add_argument('--output', type=Path, default=ROOT / 'build/wiki-preview')
    parser.add_argument('--publish', action='store_true', help='commit/push managed pages to the existing Wiki Git repository')
    args = parser.parse_args()
    if args.check and args.publish:
        parser.error('--check and --publish are separate operations')
    pages = render_pages()
    manifest = {'repository': REPOSITORY, 'license': 'LGPL-3.0-only', 'license_files': {
        name: hashlib.sha256((ROOT / name).read_bytes()).hexdigest() for name in ('LICENSE', 'COPYING')}, 'pages': {
        name: {'source_sha256': hashlib.sha256((ROOT / 'docs/wiki' / name).read_bytes()).hexdigest(),
               'rendered_sha256': hashlib.sha256(content.encode()).hexdigest()}
        for name, content in pages.items()}}
    if not args.check:
        args.output.mkdir(parents=True, exist_ok=True)
        for name, content in pages.items():
            (args.output / name).write_text(content, encoding='utf-8', newline='\n')
        for name in ('LICENSE', 'COPYING'):
            (args.output / name).write_bytes((ROOT / name).read_bytes())
        (args.output / 'wiki-manifest.json').write_text(json.dumps(manifest, indent=2) + '\n', encoding='utf-8', newline='\n')
    print(f'Validated {len(pages)} English Wiki Markdown files (including navigation)')
    if args.publish:
        try:
            publish(pages, manifest)
        except subprocess.CalledProcessError as error:
            parser.exit(1, 'Wiki Git operation failed. Confirm Wikis are enabled, an initial Home page '
                        'has been saved, and the current Git connection can access the separate Wiki repository.\n'
                        f'Git exited with status {error.returncode}; no force push was attempted.\n')


if __name__ == '__main__':
    main()
