#!/usr/bin/env python3
"""
GitHub Issues Analyzer for Azure IoT Hub Device Update
Optimized for GitHub Actions
"""

import requests
import json
import os
import argparse
from datetime import datetime, timedelta
from collections import defaultdict, Counter
from typing import Dict, List

class GitHubIssuesAnalyzer:
    def __init__(self, repo_owner: str, repo_name: str, token: str = None):
        self.repo_owner = repo_owner
        self.repo_name = repo_name
        self.base_url = f"https://api.github.com/repos/{repo_owner}/{repo_name}"
        self.headers = {"Accept": "application/vnd.github.v3+json"}
        if token:
            self.headers["Authorization"] = f"token {token}"
        self.issues = []
        
    def fetch_all_issues(self, state: str = "all", max_pages: int = 10) -> List[Dict]:
        """Fetch issues with pagination limit for Actions."""
        print(f"Fetching {state} issues...")
        issues = []
        
        for page in range(1, max_pages + 1):
            params = {"state": state, "page": page, "per_page": 100}
            response = requests.get(f"{self.base_url}/issues", 
                                  headers=self.headers, params=params)
            
            if response.status_code != 200:
                print(f"Error: {response.status_code}")
                break
                
            page_issues = response.json()
            if not page_issues:
                break
                
            issues.extend(page_issues)
            print(f"  Fetched page {page} ({len(issues)} issues)")
            
        # Filter out PRs
        self.issues = [i for i in issues if "pull_request" not in i]
        return self.issues
    
    def analyze_metrics(self) -> Dict:
        """Generate core metrics."""
        open_issues = [i for i in self.issues if i["state"] == "open"]
        closed_issues = [i for i in self.issues if i["state"] == "closed"]
        
        # Label analysis
        label_counter = Counter()
        for issue in self.issues:
            for label in issue.get("labels", []):
                label_counter[label["name"]] += 1
                
        # Stale issues (90+ days)
        stale_cutoff = datetime.utcnow() - timedelta(days=90)
        stale_issues = []
        for issue in open_issues:
            updated = datetime.strptime(issue["updated_at"], "%Y-%m-%dT%H:%M:%SZ")
            if updated < stale_cutoff:
                stale_issues.append({
                    "number": issue["number"],
                    "title": issue["title"],
                    "url": issue["html_url"],
                    "days_stale": (datetime.utcnow() - updated).days
                })
                
        # Recent activity
        recent_cutoff = datetime.utcnow() - timedelta(days=30)
        recent_opened = sum(1 for i in self.issues 
                          if datetime.strptime(i["created_at"], "%Y-%m-%dT%H:%M:%SZ") > recent_cutoff)
        recent_closed = sum(1 for i in closed_issues 
                          if i.get("closed_at") and 
                          datetime.strptime(i["closed_at"], "%Y-%m-%dT%H:%M:%SZ") > recent_cutoff)
        
        return {
            "total": len(self.issues),
            "open": len(open_issues),
            "closed": len(closed_issues),
            "labels": dict(label_counter.most_common(10)),
            "stale_count": len(stale_issues),
            "stale_issues": stale_issues[:5],  # Top 5
            "recent_opened": recent_opened,
            "recent_closed": recent_closed,
            "unassigned": sum(1 for i in open_issues if not i.get("assignee"))
        }
    
    def generate_report(self, metrics: Dict) -> str:
        """Generate markdown report."""
        report = f"""# 📊 Issue Analysis Report

**Repository:** `{self.repo_owner}/{self.repo_name}`  
**Generated:** {datetime.utcnow().strftime('%Y-%m-%d %H:%M UTC')}  
**Analyzed by:** GitHub Actions  

---

## Executive Summary

| Metric | Value | Status |
|--------|-------|--------|
| Total Issues | {metrics['total']} | - |
| Open Issues | {metrics['open']} | {"🔴" if metrics['open'] > 100 else "🟢"} |
| Closed Issues | {metrics['closed']} | - |
| Close Rate | {metrics['closed']/metrics['total']*100:.1f}% | {"🟢" if metrics['closed']/metrics['total'] > 0.7 else "🟡"} |
| Stale Issues (90+ days) | {metrics['stale_count']} | {"🔴" if metrics['stale_count'] > 20 else "🟢"} |
| Unassigned Open Issues | {metrics['unassigned']} | {"🔴" if metrics['unassigned']/metrics['open'] > 0.5 else "🟢"} |

### 📈 30-Day Activity
- **New Issues:** {metrics['recent_opened']}
- **Closed Issues:** {metrics['recent_closed']}
- **Net Change:** {metrics['recent_closed'] - metrics['recent_opened']} {"📈" if metrics['recent_closed'] > metrics['recent_opened'] else "📉"}

---

## 🏷️ Top Labels

| Label | Count | Percentage |
|-------|-------|------------|
"""
        
        for label, count in metrics['labels'].items():
            percentage = count / metrics['total'] * 100
            report += f"| `{label}` | {count} | {percentage:.1f}% |\n"
            
        report += f"""

---

## ⚠️ Stale Issues

Issues with no activity for 90+ days:

"""
        
        for issue in metrics['stale_issues']:
            report += f"- [#{issue['number']}]({issue['url']}): {issue['title']} ({issue['days_stale']} days)\n"
            
        if metrics['stale_count'] > 5:
            report += f"\n*... and {metrics['stale_count'] - 5} more stale issues*\n"
            
        report += """

---

## 🎯 Recommendations

"""
        
        # Generate actionable recommendations
        recommendations = []
        
        if metrics['stale_count'] > 20:
            recommendations.append("🔴 **High number of stale issues**: Consider reviewing and closing outdated issues")
            
        if metrics['unassigned'] / metrics['open'] > 0.5:
            recommendations.append("🟡 **Many unassigned issues**: Improve triage process to assign owners")
            
        if metrics['recent_closed'] < metrics['recent_opened']:
            recommendations.append("📉 **Negative velocity**: More issues opened than closed recently")
            
        if len(metrics['labels']) < 5:
            recommendations.append("🏷️ **Limited label usage**: Implement comprehensive labeling strategy")
            
        if not recommendations:
            recommendations.append("✅ **Good health**: Issue management appears to be on track!")
            
        for rec in recommendations:
            report += f"- {rec}\n"
            
        report += """

---

## 🔗 Quick Links

- [All Open Issues](https://github.com/{self.repo_owner}/{self.repo_name}/issues?q=is%3Aissue+is%3Aopen)
- [Good First Issues](https://github.com/{self.repo_owner}/{self.repo_name}/issues?q=is%3Aissue+is%3Aopen+label%3A%22good+first+issue%22)
- [Help Wanted](https://github.com/{self.repo_owner}/{self.repo_name}/issues?q=is%3Aissue+is%3Aopen+label%3A%22help+wanted%22)
- [Recently Updated](https://github.com/{self.repo_owner}/{self.repo_name}/issues?q=is%3Aissue+is%3Aopen+sort%3Aupdated-desc)

---

*This report was automatically generated by GitHub Actions*
"""
        
        return report

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--owner', required=True)
    parser.add_argument('--repo', required=True)
    parser.add_argument('--token', default=os.environ.get('GITHUB_TOKEN'))
    parser.add_argument('--output', default='issue_analysis_report.md')
    parser.add_argument('--visualize', action='store_true')
    
    args = parser.parse_args()
    
    analyzer = GitHubIssuesAnalyzer(args.owner, args.repo, args.token)
    analyzer.fetch_all_issues()
    
    metrics = analyzer.analyze_metrics()
    report = analyzer.generate_report(metrics)
    
    with open(args.output, 'w') as f:
        f.write(report)
        
    print(f"Report saved to {args.output}")
    
    # Output summary for Actions
    if os.environ.get('GITHUB_ACTIONS'):
        print(f"::notice::Open Issues: {metrics['open']}")
        print(f"::notice::Stale Issues: {metrics['stale_count']}")
        print(f"::notice::Recent Activity: {metrics['recent_closed'] - metrics['recent_opened']:+d}")

if __name__ == "__main__":
    main()
