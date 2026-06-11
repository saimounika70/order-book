import pandas as pd
import matplotlib.pyplot as plt
import matplotlib.ticker as ticker
import numpy as np

df = pd.read_csv('bench_results.csv')

percentiles = ['p50', 'p95', 'p99', 'p999']
labels      = ['p50', 'p95', 'p99', 'p99.9']
x = np.arange(len(percentiles))
width = 0.35

fig, ax = plt.subplots(figsize=(9, 5))
fig.patch.set_facecolor('#0d1117')   # GitHub dark background
ax.set_facecolor('#161b22')

colors = ['#388bfd', '#f85149']  # blue = fast, red = baseline
for i, (_, row) in enumerate(df.iterrows()):
    vals = [row[p] for p in percentiles]
    bars = ax.bar(x + i*width, vals, width, label=row['implementation'],
                  color=colors[i], alpha=0.85, edgecolor='none')
    for bar, val in zip(bars, vals):
        ax.text(bar.get_x() + bar.get_width()/2, bar.get_height() + 50,
                f'{val:.0f}', ha='center', va='bottom',
                fontsize=8, color='#c9d1d9')

ax.set_xlabel('Percentile', color='#c9d1d9', fontsize=11)
ax.set_ylabel('Latency (nanoseconds)', color='#c9d1d9', fontsize=11)
ax.set_title('Order Book Matching Latency\nBaseline (std::map) vs Optimised (flat array)',
             color='#e6edf3', fontsize=13, fontweight='bold')
ax.set_xticks(x + width/2)
ax.set_xticklabels(labels, color='#c9d1d9')
ax.tick_params(colors='#c9d1d9')
ax.yaxis.set_major_formatter(ticker.FuncFormatter(
    lambda v, _: f'{v/1000:.0f}μs' if v >= 1000 else f'{v:.0f}ns'))
ax.legend(facecolor='#21262d', edgecolor='#30363d',
          labelcolor='#c9d1d9', fontsize=10)
ax.spines[:].set_color('#30363d')
ax.grid(axis='y', color='#21262d', linewidth=0.8)

plt.tight_layout()
plt.savefig('latency_benchmark.png', dpi=150,
            facecolor='#0d1117', bbox_inches='tight')
plt.show()
print("Saved latency_benchmark.png")