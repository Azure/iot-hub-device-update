# Issue Analysis Tools

This directory contains automated tools for analyzing GitHub issues in the Azure IoT Hub Device Update repository.

## 🚀 Quick Start for Maintainers

### Option 1: Run via GitHub Actions (Recommended)

1. Go to the [Actions tab](../../actions)
2. Select "Issue Analysis Report" workflow
3. Click "Run workflow"
4. View results in the workflow run

### Option 2: Run Locally

```bash
# Clone the repo
git clone https://github.com/Azure/iot-hub-device-update.git
cd iot-hub-device-update

# Set your GitHub token (optional but recommended)
export GITHUB_TOKEN="your_github_personal_access_token"

# Run the analysis
.github/tools/run_analysis.sh

# With visualizations (requires matplotlib)
.github/tools/run_analysis.sh --visualize
```

## 📊 What the Analysis Provides

- **Issue Statistics**: Open/closed counts, close rates
- **Activity Metrics**: 30-day trends, velocity
- **Label Analysis**: Most used labels, unlabeled issues
- **Stale Issues**: Issues with no activity for 90+ days
- **Assignment Status**: Unassigned issue tracking
- **Actionable Recommendations**: Based on metrics

## 🔧 Configuration

### GitHub Action Schedule

The action runs:
- Weekly on Mondays at 9 AM UTC
- On manual trigger
- When issues are opened/closed

To change the schedule, edit `.github/workflows/issue-analysis.yml`:

```yaml
schedule:
  - cron: '0 9 * * 1'  # Modify this line
```

### Analysis Parameters

Edit these in `analyze_issues.py`:
- `STALE_DAYS = 90` - Days before issue is considered stale
- `MAX_PAGES = 10` - Maximum pages to fetch (100 issues/page)
- `TOP_LABELS = 10` - Number of top labels to show

## 📈 Understanding the Metrics

### Status Indicators

- 🟢 **Green**: Healthy metric
- 🟡 **Yellow**: Needs attention
- 🔴 **Red**: Requires immediate action

### Key Metrics

1. **Close Rate**: Percentage of issues that have been closed
   - Healthy: > 70%
   - Concerning: < 50%

2. **Stale Issues**: Issues with no activity for 90+ days
   - Healthy: < 10
   - Concerning: > 20

3. **Assignment Rate**: Percentage of open issues with assignees
   - Healthy: > 80%
   - Concerning: < 50%

4. **Velocity**: Net change in issues over 30 days
   - Positive: More issues closed than opened
   - Negative: Backlog is growing

## 🛠️ Troubleshooting

### Common Issues

1. **Rate Limit Exceeded**
   ```
   Error: 403
   ```
   Solution: Set `GITHUB_TOKEN` environment variable

2. **Python Not Found**
   ```
   Error: Python 3 is required
   ```
   Solution: Install Python 3.7 or higher

3. **Permission Denied**
   ```
   Permission denied: run_analysis.sh
   ```
   Solution: `chmod +x .github/tools/run_analysis.sh`

## 📝 Customization

### Adding New Metrics

1. Edit `analyze_issues.py`
2. Add metric calculation in `analyze_metrics()`
3. Add display in `generate_report()`

Example:
```python
# In analyze_metrics()
metrics['bugs'] = sum(1 for i in self.issues 
                     if any(l['name'] == 'bug' for l in i.get('labels', [])))

# In generate_report()
report += f"| Bug Issues | {metrics['bugs']} | - |\n"
```

### Custom Reports

Create specialized reports by extending the analyzer:

```python
class CustomAnalyzer(GitHubIssuesAnalyzer):
    def generate_security_report(self):
        # Custom security-focused analysis
        pass
```

## 🤝 Contributing

To improve these tools:

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Test locally
5. Submit a pull request

### Ideas for Improvement

- [ ] Add PR analysis
- [ ] Create trend graphs
- [ ] Add email notifications
- [ ] Export to different formats (JSON, CSV)
- [ ] Add more visualizations
- [ ] Create issue predictions

## 📄 License

These tools are part of the Azure IoT Hub Device Update project and follow the same license terms.
