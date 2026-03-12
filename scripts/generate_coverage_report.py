#!/usr/bin/env python3
"""Parse Cobertura.xml and generate coverage-report.md"""

import xml.etree.ElementTree as ET
import os
import sys
from collections import defaultdict

# Resolve paths relative to the repo root (parent of scripts/)
script_dir = os.path.dirname(os.path.abspath(__file__))
root_dir = os.path.dirname(script_dir)

# Accept optional arguments: <xml_path> [<output_path>]
xml_path = sys.argv[1] if len(sys.argv) > 1 else os.path.join(root_dir, 'out', 'coverage', 'Cobertura.xml')
output_path = sys.argv[2] if len(sys.argv) > 2 else os.path.join(root_dir, 'out', 'coverage', 'coverage-report.md')

# Parse the Cobertura XML
tree = ET.parse(xml_path)
root = tree.getroot()

# Get overall stats from root attributes
overall_lines_covered = int(root.get('lines-covered', 0))
overall_lines_valid = int(root.get('lines-valid', 0))
overall_branches_covered = int(root.get('branches-covered', 0))
overall_branches_valid = int(root.get('branches-valid', 0))

# Collect all files with their line data
files_data = {}

for package in root.findall('.//package'):
    for cls in package.findall('.//class'):
        filename = cls.get('filename', '')
        if not filename.startswith('src/'):
            continue
        lines_elem = cls.find('lines')
        if lines_elem is None:
            continue
        total_lines = 0
        hit_lines = 0
        for line in lines_elem.findall('line'):
            total_lines += 1
            if int(line.get('hits', 0)) > 0:
                hit_lines += 1
        methods_elem = cls.find('methods')
        total_methods = 0
        covered_methods = 0
        if methods_elem is not None:
            for method in methods_elem.findall('method'):
                total_methods += 1
                if float(method.get('line-rate', 0)) > 0:
                    covered_methods += 1
        coverage_pct = (hit_lines / total_lines * 100) if total_lines > 0 else 0.0
        if filename in files_data:
            if coverage_pct > files_data[filename]['coverage_pct']:
                files_data[filename] = {'hit_lines': hit_lines, 'total_lines': total_lines, 'coverage_pct': coverage_pct, 'total_methods': total_methods, 'covered_methods': covered_methods}
        else:
            files_data[filename] = {'hit_lines': hit_lines, 'total_lines': total_lines, 'coverage_pct': coverage_pct, 'total_methods': total_methods, 'covered_methods': covered_methods}

def get_subfolder(filepath):
    parts = filepath.split('/')
    return '/src/' + parts[1] if len(parts) >= 2 else '/src/unknown'

subfolders = defaultdict(lambda: {'files': [], 'total_hit_lines': 0, 'total_lines': 0, 'total_methods': 0, 'covered_methods': 0})
for filename, data in sorted(files_data.items()):
    sf = get_subfolder(filename)
    subfolders[sf]['files'].append((filename, data))
    subfolders[sf]['total_hit_lines'] += data['hit_lines']
    subfolders[sf]['total_lines'] += data['total_lines']
    subfolders[sf]['total_methods'] += data['total_methods']
    subfolders[sf]['covered_methods'] += data['covered_methods']

total_functions = sum(sf['total_methods'] for sf in subfolders.values())
covered_functions = sum(sf['covered_methods'] for sf in subfolders.values())
total_metric_a = sum(1 for d in files_data.values() if d['coverage_pct'] == 0.0)
total_metric_b = sum(1 for d in files_data.values() if 0.0 < d['coverage_pct'] < 85.0)
total_metric_c = sum(1 for d in files_data.values() if d['coverage_pct'] >= 85.0)

L = []
L.append('# Code Coverage Report\n\n## Summary\n')
line_cov_pct = (overall_lines_covered / overall_lines_valid * 100) if overall_lines_valid > 0 else 0.0
branch_cov_pct = (overall_branches_covered / overall_branches_valid * 100) if overall_branches_valid > 0 else 0.0
func_cov_pct = (covered_functions / total_functions * 100) if total_functions > 0 else 0.0
L.append(f'- **Line Coverage:** {line_cov_pct:.2f}% ({overall_lines_covered}/{overall_lines_valid} lines)')
L.append(f'- **Branch Coverage:** {branch_cov_pct:.2f}% ({overall_branches_covered}/{overall_branches_valid} branches)')
L.append(f'- **Function Coverage:** {func_cov_pct:.2f}% ({covered_functions}/{total_functions} functions)')
L.append(f'\n- **Metric A (0% coverage):** {total_metric_a} files')
L.append(f'- **Metric B (>0% and <85% coverage):** {total_metric_b} files')
L.append(f'- **Metric C (>=85% coverage):** {total_metric_c} files')
L.append(f'- **Total files:** {len(files_data)}\n')
L.append('### Coverage by Subfolder\n')
L.append('| Subfolder | Total Files | Files at 0% | Files >0% & <85% | Files >=85% | Hit/Total Lines | Coverage % |')
L.append('|-----------|-------------|-------------|-------------------|-------------|-----------------|------------|')
for sf_name in sorted(subfolders.keys()):
    sf = subfolders[sf_name]; sf_files = sf['files']
    a = sum(1 for _, d in sf_files if d['coverage_pct'] == 0.0)
    b = sum(1 for _, d in sf_files if 0.0 < d['coverage_pct'] < 85.0)
    c = sum(1 for _, d in sf_files if d['coverage_pct'] >= 85.0)
    p = (sf['total_hit_lines'] / sf['total_lines'] * 100) if sf['total_lines'] > 0 else 0.0
    L.append(f"| `{sf_name}` | {len(sf_files)} | {a} | {b} | {c} | {sf['total_hit_lines']}/{sf['total_lines']} | {p:.2f}% |")
L.append('\n---\n\n## Details\n')
for sf_name in sorted(subfolders.keys()):
    sf = subfolders[sf_name]; sf_files = sf['files']
    a = sum(1 for _, d in sf_files if d['coverage_pct'] == 0.0)
    b = sum(1 for _, d in sf_files if 0.0 < d['coverage_pct'] < 85.0)
    c = sum(1 for _, d in sf_files if d['coverage_pct'] >= 85.0)
    p = (sf['total_hit_lines'] / sf['total_lines'] * 100) if sf['total_lines'] > 0 else 0.0
    L.append(f'### `{sf_name}`\n')
    L.append(f'- **Total files:** {len(sf_files)}')
    L.append(f'- **Files at 0%:** {a}')
    L.append(f'- **Files >0% & <85%:** {b}')
    L.append(f'- **Files >=85%:** {c}')
    L.append(f"- **Hit/Total lines:** {sf['total_hit_lines']}/{sf['total_lines']}")
    L.append(f'- **Subfolder coverage:** {p:.2f}%\n')
    L.append('| File | Hit/Total Lines | Coverage % | Flags |')
    L.append('|------|-----------------|------------|-------|')
    for fn, d in sorted(sf_files, key=lambda x: x[0]):
        flag = '\U0001f534 0%' if d['coverage_pct'] == 0.0 else ('\U0001f7e1 <85%' if d['coverage_pct'] < 85.0 else '\U0001f7e2 >=85%')
        L.append(f"| `{fn}` | {d['hit_lines']}/{d['total_lines']} | {d['coverage_pct']:.2f}% | {flag} |")
    L.append('')

with open(output_path, 'w') as f:
    f.write('\n'.join(L))
print(f"Done. Files={len(files_data)} Subfolders={len(subfolders)} A={total_metric_a} B={total_metric_b} C={total_metric_c}")
