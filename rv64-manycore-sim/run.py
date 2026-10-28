#!/usr/bin/env python3
"""从单一INI配置运行裸机场景；仅依赖Python 3标准库。"""
import argparse
import configparser
import datetime
import json
import pathlib
import shlex
import subprocess
import sys
import tempfile

ROOT = pathlib.Path(__file__).resolve().parent
SCENARIOS = ('private', 'hotspot', 'compute', 'mmio')
OPTIONS = (
    'cores', 'tohost', 'link-width', 'route-latency', 'router-buffer',
    'receive-slots', 'memory-latency', 'memory-queue', 'max-ticks',
    'lat-alu', 'lat-mul', 'lat-div', 'lat-branch', 'lat-csr', 'lat-memory',
    'lat-fence', 'lat-vconfig', 'lat-valu', 'lat-vtransaction',
)


def positive(text):
    value = int(text)
    if value <= 0:
        raise argparse.ArgumentTypeError('核心数量必须大于0')
    return value


def load_jobs(args):
    parser = configparser.ConfigParser(interpolation=None)
    with args.config.open(encoding='utf-8-sig') as source:
        parser.read_file(source)
    if parser.defaults():
        raise ValueError('请将公共参数放入[common]，不使用[DEFAULT]')
    sections = {'run', 'common', 'sweep', *SCENARIOS}
    if set(parser.sections()) != sections:
        raise ValueError('配置必须包含且仅包含这些段：' + ', '.join(sorted(sections)))
    allowed = {'run': {'build_dir', 'timeout_seconds'}, 'common': set(OPTIONS),
               'sweep': {'cores'}}
    allowed.update({name: set(OPTIONS) | {'program'} for name in SCENARIOS})
    for section in parser.sections():
        unknown = set(parser[section]) - allowed[section]
        if unknown:
            raise ValueError('未知配置项 [{}]: {}'.format(section, ', '.join(sorted(unknown))))
    if set(parser['common']) != set(OPTIONS):
        raise ValueError('[common]需保留全部公共参数')
    timeout = int(parser['run']['timeout_seconds'])
    if timeout <= 0:
        raise ValueError('timeout_seconds必须大于0')
    build = pathlib.Path(args.build_dir or parser['run']['build_dir'])
    if not build.is_absolute():
        build = ROOT / build
    build = build.resolve()
    names = SCENARIOS if args.scene == 'all' else SCENARIOS[:3] if args.scene == 'sweep' else (args.scene,)
    sizes = None
    if args.scene == 'sweep':
        sizes = [int(v.strip()) for v in parser['sweep']['cores'].split(',')]
        if not sizes or any(v <= 0 for v in sizes) or len(set(sizes)) != len(sizes):
            raise ValueError('[sweep]核心数必须为不重复的正整数')
    jobs = []
    for name in names:
        values = dict(parser['common'])
        values.update(parser[name])
        program = values.pop('program')
        if pathlib.Path(program).name != program or not program.endswith('.bin'):
            raise ValueError('program必须是build/examples下的.bin文件名')
        for core_count in sizes or [int(values['cores'], 0)]:
            options = dict(values)
            options['cores'] = str(args.cores if args.cores is not None else core_count)
            for key, value in options.items():
                number = int(value, 0)
                if number < 0 or (number == 0 and key not in ('receive-slots', 'tohost')):
                    raise ValueError('{}: {}数值无效'.format(name, key))
                if key not in ('tohost', 'max-ticks', 'memory-queue') and number > 0xffffffff:
                    raise ValueError('{}超出uint32范围'.format(key))
                if number > 0xffffffffffffffff:
                    raise ValueError('{}超出uint64范围'.format(key))
            jobs.append({'scene': name, 'program': program, 'options': options})
    return build, timeout, jobs


def command(build, job, csv_path=None):
    cmd = [str(build / 'rv64_manycore_sim'), str(build / 'examples' / job['program'])]
    for key in OPTIONS:
        cmd.extend(('--' + key, job['options'][key]))
    cmd.extend(('--workload', job['scene']))
    if csv_path is not None:
        cmd.extend(('--csv', str(csv_path)))
    return cmd


def main():
    args_parser = argparse.ArgumentParser(description='用run.ini统一配置众核拥塞实验')
    args_parser.add_argument('scene', nargs='?', default='private',
                             choices=(*SCENARIOS, 'all', 'sweep'),
                             help='默认private；all运行四个场景；sweep扫描三类RVV负载')
    args_parser.add_argument('--config', type=pathlib.Path, default=ROOT / 'run.ini')
    args_parser.add_argument('--cores', type=positive, help='临时覆盖核心数')
    args_parser.add_argument('--build-dir', help='临时指定已有构建目录')
    args_parser.add_argument('--dry-run', action='store_true', help='仅打印最终命令，不启动模拟器或写文件')
    args = args_parser.parse_args()
    try:
        if args.scene == 'sweep' and args.cores is not None:
            raise ValueError('sweep的核心数在run.ini的[sweep]中设置，不与--cores同时使用')
        build, timeout, jobs = load_jobs(args)
        if args.dry_run:
            for job in jobs:
                print(' '.join(shlex.quote(part) for part in command(build, job)))
            return 0
        for path in [build / 'rv64_manycore_sim', *(build / 'examples' / j['program'] for j in jobs)]:
            if not path.is_file():
                raise ValueError('找不到{}；请先按README构建程序和裸机示例'.format(path))
        results = build / 'results'
        results.mkdir(parents=True, exist_ok=True)
        stamp = datetime.datetime.now().strftime('%Y%m%d-%H%M%S-')
        output = pathlib.Path(tempfile.mkdtemp(prefix=stamp, dir=str(results)))
        csv_path = output / 'results.csv'
        (output / 'configuration.json').write_text(
            json.dumps({'source_config': str(args.config.resolve()), 'timeout_seconds': timeout,
                        'build_dir': str(build), 'jobs': jobs}, ensure_ascii=False, indent=2),
            encoding='utf-8')
        print('结果目录：{}'.format(output), flush=True)
        for index, job in enumerate(jobs, 1):
            cores = job['options']['cores']
            cmd = command(build, job, csv_path)
            log_path = output / '{:02d}-{}-{}cores.log'.format(index, job['scene'], cores)
            try:
                completed = subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                                           text=True, timeout=timeout)
                log_path.write_text(completed.stdout, encoding='utf-8')
            except subprocess.TimeoutExpired as error:
                partial = error.stdout or b''
                if isinstance(partial, bytes):
                    partial = partial.decode('utf-8', errors='replace')
                log_path.write_text(partial + '\nHOST_TIMEOUT\n', encoding='utf-8')
                print('{} {}核：超时；日志 {}'.format(job['scene'], cores, log_path), file=sys.stderr)
                return 1
            summary = next((line for line in completed.stdout.splitlines() if line.startswith('completed=')), '')
            if completed.returncode != 0:
                print('{} {}核：FAIL {}；日志 {}'.format(job['scene'], cores, summary, log_path), file=sys.stderr)
                return 1
            print('{} {}核：PASS {}'.format(job['scene'], cores, summary), flush=True)
        print('CSV：{}'.format(csv_path))
        return 0
    except (OSError, ValueError, KeyError, configparser.Error) as error:
        print('运行配置错误：{}'.format(error), file=sys.stderr)
        return 2


if __name__ == '__main__':
    sys.exit(main())
