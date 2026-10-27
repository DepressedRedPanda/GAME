#!/usr/bin/env python3
"""Check combined-project linkage and reflected-source structure, without Unreal."""
from pathlib import Path
import json
import re
import sys

ROOT = Path(__file__).resolve().parent.parent
errors = []
project = json.loads((ROOT / 'HeavyMettle.uproject').read_text())
modules = {entry['Name'] for entry in project['Modules']}
expected = {'Hollowreach', 'VeilOfAsh', 'SharedArt'}
if modules != expected or project['EngineAssociation'] != '5.8':
    errors.append('Wrong engine association or runtime module set')
if len(list(ROOT.glob('*.uproject'))) != 1:
    errors.append('Expected one root project descriptor')
for target in ('HeavyMettle', 'HeavyMettleEditor'):
    text = (ROOT / 'Source' / (target + '.Target.cs')).read_text()
    linked = set(re.findall(r'ExtraModuleNames.Add\("([^"]+)"\)', text))
    if linked != expected:
        errors.append(target + ': does not link all three modules')

cpp = sorted((ROOT / 'Source').rglob('*.cpp'))
headers = sorted((ROOT / 'Source').rglob('*.h'))
primary = [p for p in cpp if 'IMPLEMENT_PRIMARY_GAME_MODULE(' in p.read_text()]
if primary != [ROOT / 'Source/Hollowreach/Hollowreach.cpp']:
    errors.append('Expected exactly one primary module: Hollowreach')

dependencies = {}
for module in sorted(modules):
    rules = ROOT / 'Source' / module / (module + '.Build.cs')
    if not rules.exists():
        errors.append(module + ': missing module build rules')
        continue
    text = rules.read_text()
    dependencies[module] = set(re.findall(r'"([^"]+)"', text)) & modules

def visit(module, chain):
    if module in chain:
        errors.append('Module dependency cycle: ' + ' -> '.join(chain + [module]))
        return
    for dependency in dependencies.get(module, set()):
        visit(dependency, chain + [module])

for module in modules:
    visit(module, [])

reflected_names = {}
known_headers = {p.name for p in headers}
for header in headers:
    text = header.read_text()
    includes = re.findall(r'^\s*#include\s+"([^"]+)"', text, re.M)
    if re.search(r'\bGENERATED_BODY\s*\(', text):
        generated = header.stem + '.generated.h'
        if not includes or includes[-1] != generated:
            errors.append(str(header.relative_to(ROOT)) + ': generated include missing or not last')
    for match in re.finditer(r'U(?:CLASS|STRUCT|ENUM)\([^)]*\)\s*(?:class|struct|enum\s+class)\s+(?:\w+_API\s+)?(\w+)', text):
        name = match.group(1)
        if name in reflected_names:
            errors.append('Duplicate reflected type: ' + name)
        reflected_names[name] = header

for source in cpp + headers:
    text = source.read_text()
    for include in re.findall(r'^\s*#include\s+"([^"]+)"', text, re.M):
        name = Path(include).name
        if name.startswith(('HR', 'Ash', 'SharedArt')) and not name.endswith('.generated.h') and name not in known_headers:
            errors.append(str(source.relative_to(ROOT)) + ': unresolved project header ' + include)
    # Remove comments and quoted strings/characters in one pass (comments inside strings stay strings).
    stripped = re.sub(r'"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'|/\*.*?\*/|//[^\n]*', '', text, flags=re.S)
    balance = 0
    for char in stripped:
        balance += (char == '{') - (char == '}')
        if balance < 0:
            break
    if balance != 0:
        errors.append(str(source.relative_to(ROOT)) + ': unbalanced braces')
    if re.search(r'\n[ \t]*;[ \t]*\n[ \t]*\{', stripped):
        errors.append(str(source.relative_to(ROOT)) + ': stray semicolon before function body')

engine_config = (ROOT / 'Config/DefaultEngine.ini').read_text()
if 'game=VeilOfAsh' in engine_config or 'GameModeClassAliases' in engine_config:
    errors.append('An alternate campaign mode is still configured')
if (ROOT / 'Source/VeilOfAsh/AshGameMode.cpp').exists():
    errors.append('Legacy gameplay is compiled instead of ported into the campaign')
schema_files = {p.name for p in (ROOT / 'Source/VeilOfAsh').iterdir() if p.is_file()}
if schema_files != {'AshSave.h', 'AshTypes.h', 'VeilOfAsh.cpp', 'VeilOfAsh.Build.cs'}:
    errors.append('Veil runtime module must contain only legacy save-schema compatibility')
save_source = (ROOT / 'Source/Hollowreach/Systems/HRSaveSubsystem.cpp').read_text()
save_header = (ROOT / 'Source/Hollowreach/Systems/HRSaveSubsystem.h').read_text()
if 'Hollowreach_Unified_Slot0' not in save_header:
    errors.append('Canonical unified slot missing')
if not (ROOT / 'Source/Hollowreach/Core/HRCampaignRules.h').exists():
    errors.append('Shared campaign rule implementation missing')

manifest = json.loads((ROOT / 'docs/assets/free_asset_manifest.json').read_text())
for item in manifest['assets']:
    if item['status'] != 'candidate_not_imported':
        errors.append(item['name'] + ': an unimported asset cannot be marked integrated')
    if item['required_paid_purchase']:
        errors.append(item['name'] + ': paid purchase unexpectedly required')

print('Modules:', ', '.join(sorted(modules)))
print('Source: {} .cpp, {} .h; {} reflected types'.format(len(cpp), len(headers), len(reflected_names)))
print('Free asset candidates:', len(manifest['assets']))
for error in errors:
    print('FAIL:', error)
print('Consistency errors:', len(errors))
print('These checks do not run UHT, compile C++, load assets, or test gameplay.')
sys.exit(1 if errors else 0)
