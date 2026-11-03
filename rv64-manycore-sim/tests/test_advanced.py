#!/usr/bin/env python3
import csv
import pathlib
import subprocess
import sys
import tempfile

exe, examples = pathlib.Path(sys.argv[1]), pathlib.Path(sys.argv[2])
with tempfile.TemporaryDirectory(prefix='bus-advanced-') as directory:
    root = pathlib.Path(directory)
    counter = 0
    def run(program, *extra):
        global counter
        counter += 1
        output = root / ('case-{}.csv'.format(counter))
        cmd = [str(exe), str(examples/(program+'.bin')), '--cores', '1', '--tohost', '0x80001000',
               '--max-ticks', '2000000', '--csv', str(output), *extra]
        result = subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
        assert result.returncode == 0, result.stdout
        return next(csv.DictReader(output.open()))
    base = run('vector_stream_private')
    a = root/'v.ini'; a.write_text('vadd.vi = { base=2, lanes=8 }\n')
    b = root/'v2.ini'; b.write_text('vadd.vi = { base=8, lanes=8 }\n')
    ra = run('vector_stream_private', '--timing-table', str(a))
    rb = run('vector_stream_private', '--timing-table', str(b))
    assert int(rb['ticks'])-int(ra['ticks']) == 64*6
    a.write_text('vle32.v = 5\n')
    load = run('vector_stream_private', '--timing-table', str(a))
    assert int(load['ticks'])-int(base['ticks']) == 64*4
    args = ('--interconnect','shared','--mmio-depth','4','--memory-latency','100')
    fa = run('csr_bus_smoke', *args, '--lat-fence', '1')
    fb = run('csr_bus_smoke', *args, '--lat-fence', '9')
    assert int(fb['ticks'])-int(fa['ticks']) == 8
    assert int(fa['mmio_commands']) == int(fa['mmio_completed']) == 5
    assert int(fa['mmio_errors']) == 0
    assert 0 <= float(fa['hart_memory_wait_fraction']) <= 1
    assert float(fa['mmio_p99_latency']) >= float(fa['mmio_mean_latency'])
    for invalid in ('addi = -1\n', 'addi = 0\n', 'unknown.foo = 1\n',
                    'addi = 1\naddi = 2\n', 'vadd.vv = { base=2, lanes=0 }\n'):
        a.write_text(invalid)
        result = subprocess.run([str(exe), str(examples/'vector_stream_private.bin'),
                                 '--timing-table',str(a)], stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        assert result.returncode == 2, invalid
print('ADVANCED_TIMING_CSR_FENCE_PASS')
