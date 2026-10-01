use std::path::PathBuf;
use std::sync::Arc;

use anyhow::{Context, Result};
use dread_kings_rust::{DreadKings, ProblemContext, RelaxedAtomicKing, TodoMarkerKing};

#[tokio::main]
async fn main() -> Result<()> {
    let path = std::env::args_os()
        .nth(1)
        .map(PathBuf::from)
        .context("usage: dread-kings-rust <source-file>")?;
    let source = std::fs::read_to_string(&path)
        .with_context(|| format!("failed to read {}", path.display()))?;

    let context = Arc::new(ProblemContext::new(
        path.display().to_string(),
        Arc::<str>::from(source),
    ));
    let dread_kings = DreadKings::new();
    dread_kings.register_king(RelaxedAtomicKing)?;
    dread_kings.register_king(TodoMarkerKing)?;

    let summary = dread_kings.run(context).await?;
    for report in summary.reports {
        println!("{}: {} finding(s)", report.king, report.findings.len());
        for finding in report.findings {
            let location = finding
                .line
                .map(|line| format!("{}:{}", path.display(), line))
                .unwrap_or_else(|| path.display().to_string());
            println!("  {} [{}] {}", location, finding.rule_id, finding.message);
        }
    }
    println!(
        "Completed {} kings across {} run(s); {} finding(s).",
        summary.metrics.kings_completed, summary.metrics.runs, summary.metrics.findings
    );
    Ok(())
}
