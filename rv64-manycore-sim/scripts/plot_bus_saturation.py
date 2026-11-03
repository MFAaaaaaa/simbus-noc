#!/usr/bin/env python3
"""Plot raw bus_saturation results, with explicit overload/censoring caveats."""
import csv
import json
import pathlib
import sys
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt

source = pathlib.Path(sys.argv[1])
rows = list(csv.DictReader(source.open()))
out = source.parent
selected = [r for r in rows if r['master']=='0' and r['profile']=='balanced' and r['policy']=='rr']
x = [float(r['observed_offered']) for r in selected]
for key, label, filename in [('throughput','Completed requests / tick','throughput'),
                              ('mean_latency','Completed-request mean latency (ticks)','latency')]:
    fig, ax = plt.subplots(figsize=(7,4.5))
    ax.plot(x, [float(r[key]) for r in selected], 'o-', label='Round-robin, balanced sources')
    if key=='throughput':
        ax.axhline(1/8, color='darkred', linestyle='--', label='Capacity: 1 / (2 x 4) = 0.125')
    else:
        ax.plot(x, [float(r['p99_latency']) for r in selected], 's--', label='Completed-request p99')
    ax.axvline(.125, color='gray', linestyle=':', label='Nominal bus saturation point')
    ax.set(xlabel='Offered requests / tick (including rejected/dropped offers)', ylabel=label)
    fig.text(.5,.015,'20,000 observation ticks. Above capacity: unfinished requests are censored, no steady-state latency claim.',ha='center',fontsize=7)
    ax.grid(alpha=.25); ax.legend();fig.tight_layout(rect=(0,.04,1,1))
    fig.savefig(out/(filename+'.png'), dpi=180);fig.savefig(out/(filename+'.svg'));plt.close(fig)
fig, ax = plt.subplots(figsize=(7,4.5))
for policy, offset in [('rr',-.18),('fixed',.18)]:
    rs = [r for r in rows if r['policy']==policy and r['profile']=='hot' and r['offered_load']=='0.4']
    ax.bar([int(r['master'])+offset for r in rs], [int(r['master_completed'])/20000 for r in rs], width=.36, label=policy)
ax.set(xlabel='Master ID', ylabel='Completed requests / observation tick', xticks=range(4))
ax.set_title('Overload: master 0 offers 80% of total traffic, aggregate load 0.4')
ax.legend();fig.tight_layout();fig.savefig(out/'fairness.png',dpi=180);fig.savefig(out/'fairness.svg');plt.close(fig)
assert all(int(r['memory_queue_full'])==0 for r in rows), 'Memory queues saturated'
assert max(float(r['memory_utilization']) for r in rows)<.2, 'Memory is not sufficiently fast'
assert max(float(r['throughput']) for r in selected)>.12, 'No bus plateau observed'
assert all(float(r['throughput'])<.126 for r in selected), 'Capacity bound violated'
rr_hot=[r for r in rows if r['profile']=='hot' and r['offered_load']=='0.4' and r['policy']=='rr']
fixed_hot=[r for r in rows if r['profile']=='hot' and r['offered_load']=='0.4' and r['policy']=='fixed']
assert all(int(r['master_completed'])>0 for r in rr_hot), 'RR master starvation'
assert all(int(r['master_completed'])==0 for r in fixed_hot if r['master']!='0'), 'Priority starvation demonstration missing'
summary = {'capacity_requests_per_tick':.125, 'max_memory_utilization':max(float(r['memory_utilization']) for r in rows),
           'measurement_ticks':20000, 'warmup_ticks':2000,
           'latency_scope':'Completed during observation window, includes external source backlog. Overload has censored unfinished requests; no steady-state latency claim.',
           'grant_scope':'Bus grant histograms and starvation counts cover warmup + observation.',
           'censored_max':max(int(r['censored']) for r in rows), 'drops_max':max(int(r['drops']) for r in rows),
           'fairness_hot': [r for r in rows if r['profile']=='hot' and r['offered_load']=='0.4']}
(out/'summary.json').write_text(json.dumps(summary,indent=2),encoding='utf-8')
print('PLOTS_PASS', json.dumps({k:v for k,v in summary.items() if k!='fairness_hot'}))
