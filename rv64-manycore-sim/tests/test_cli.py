"""End-to-end CLI, CSV, latency and multi-hart MMIO acceptance (stdlib only)."""
import csv
import pathlib
import subprocess
import sys
import tempfile

sim, example_dir = sys.argv[1:]
example_dir = pathlib.Path(example_dir)

def run(workload, *args, expected=0):
    result = subprocess.run(
        [sim, str(example_dir / (workload + '.bin')), '--tohost', '0x80001000', *args],
        capture_output=True, text=True, timeout=60)
    if result.returncode != expected:
        raise AssertionError((result.returncode, result.stdout, result.stderr))
    return result

with tempfile.TemporaryDirectory(prefix='manycore-cli-') as folder:
    output = pathlib.Path(folder) / 'results.csv'
    for cores in (1, 2, 4, 8):
        result = run('mmio_bus_smoke', '--cores', str(cores), '--receive-slots', '1',
                     '--memory-queue', '1', '--memory-latency', '100',
                     '--workload', 'mmio,roundtrip', '--csv', str(output))
        assert result.stdout.count('mmio_completed=2') == cores, result.stdout
        assert result.stdout.count('mmio_errors=0') == cores, result.stdout
    with output.open(newline='') as source:
        rows = list(csv.DictReader(source))
    assert len(rows) == 4
    for row, cores in zip(rows, (1, 2, 4, 8)):
        assert None not in row, row
        assert row['workload'] == 'mmio,roundtrip'
        assert row['schema_version'] == '2'
        assert row['success'] == '1'
        assert int(row['mmio_completed']) == cores * 2
        assert int(row['adapter_serialization_wait']) > 0
        assert int(row['max_reserved_messages']) == 1
    latency_csv = pathlib.Path(folder) / 'latency.csv'
    for args in ((), ('--lat-valu', '10'), ('--lat-vtransaction', '3')):
        run('vector_stream_private', '--cores', '1', '--csv', str(latency_csv), *args)
    with latency_csv.open(newline='') as source:
        base, alu, transaction = list(csv.DictReader(source))
    assert int(alu['ticks']) - int(base['ticks']) == 64 * 6
    assert int(transaction['ticks']) - int(base['ticks']) == 512 * 2
    assert int(transaction['vector_issue_wait']) == 512 * 2
    run('vector_stream_private', '--lat-valu', '0', expected=2)
    run('vector_stream_private', '--cores', '-1', expected=2)
    old_csv = pathlib.Path(folder) / 'old.csv'
    old_csv.write_text('old_schema\n', encoding='utf-8')
    run('tohost_smoke', '--csv', str(old_csv), expected=2)
    assert old_csv.read_text() == 'old_schema\n'
print('CLI_CSV_MMIO_LATENCY_PASS')
