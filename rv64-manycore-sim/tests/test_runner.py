"""INI inheritance, overrides, runnable presets and output preservation."""
import configparser
import csv
import json
import pathlib
import subprocess
import sys
import tempfile

root = pathlib.Path(sys.argv[1])
build = pathlib.Path(sys.argv[2])

def invoke(*args, expected=0):
    result = subprocess.run([sys.executable, str(root / 'run.py'), '--build-dir', str(build), *args],
                            cwd=tempfile.gettempdir(), capture_output=True, text=True, timeout=60)
    assert result.returncode == expected, (result.stdout, result.stderr)
    return result

dry = invoke('mmio', '--cores', '2', '--dry-run').stdout
assert '--cores 2' in dry and '--memory-latency 100' in dry and '--receive-slots 1' in dry
assert len(invoke('sweep', '--dry-run').stdout.splitlines()) == 12
invoke('sweep', '--cores', '2', expected=2)
invoke('private', '--cores', '0', expected=2)
with tempfile.TemporaryDirectory(prefix='manycore-runner-') as folder:
    config = configparser.ConfigParser()
    config.read(str(root / 'run.ini'), encoding='utf-8')
    config['common']['lat-valu'] = '7'
    config['private']['memory-latency'] = '11'
    path = pathlib.Path(folder) / 'custom.ini'
    with path.open('w', encoding='utf-8') as target:
        config.write(target)
    result = invoke('private', '--config', str(path), '--dry-run').stdout
    assert '--lat-valu 7' in result and '--memory-latency 11' in result
    config['private']['typo'] = '99'
    with path.open('w', encoding='utf-8') as target:
        config.write(target)
    invoke('private', '--config', str(path), '--dry-run', expected=2)

output_dirs = []
for _ in range(2):
    result = invoke('all', '--cores', '2')
    assert result.stdout.count('：PASS ') == 4
    csv_line = next(line for line in result.stdout.splitlines() if line.startswith('CSV：'))
    path = pathlib.Path(csv_line.split('：', 1)[1])
    output_dirs.append(path.parent)
    with path.open(newline='') as source:
        rows = list(csv.DictReader(source))
    assert len(rows) == 4 and all(row['success'] == '1' for row in rows)
    assert [row['workload'] for row in rows] == ['private', 'hotspot', 'compute', 'mmio']
    assert all(row['cores'] == '2' for row in rows)
    snapshot = json.loads((path.parent / 'configuration.json').read_text(encoding='utf-8'))
    assert len(snapshot['jobs']) == 4
    assert len(list(path.parent.glob('*.log'))) == 4
assert output_dirs[0] != output_dirs[1]
assert all((path / 'results.csv').is_file() for path in output_dirs)
print('RUNNER_INI_PRESETS_PASS')
