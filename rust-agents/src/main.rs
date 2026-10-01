use std::path::PathBuf;
use std::sync::Arc;

use anyhow::{Context, Result};
use dread_rust_agents::{Council, ProblemContext, RelaxedAtomicAgent, TodoMarkerAgent};

#[tokio::main]
async fn main() -> Result<()> {
    let path = std::env::args_os()
        .nth(1)
        .map(PathBuf::from)
        .context("usage: dread-rust-agents <source-file>")?;
    let source = std::fs::read_to_string(&path)
        .with_context(|| format!("failed to read {}", path.display()))?;

    let context = Arc::new(ProblemContext::new(
        path.display().to_string(),
        Arc::<str>::from(source),
    ));
    let council = Council::new();
    council.register_agent(RelaxedAtomicAgent)?;
    council.register_agent(TodoMarkerAgent)?;

    let summary = council.run(context).await?;
    for report in summary.reports {
        println!("{}: {} finding(s)", report.agent, report.findings.len());
        for finding in report.findings {
            let location = finding
                .line
                .map(|line| format!("{}:{}", path.display(), line))
                .unwrap_or_else(|| path.display().to_string());
            println!("  {} [{}] {}", location, finding.rule_id, finding.message);
        }
    }
    println!(
        "Completed {} agents across {} run(s); {} finding(s).",
        summary.metrics.agents_completed, summary.metrics.runs, summary.metrics.findings
    );
    Ok(())
}
