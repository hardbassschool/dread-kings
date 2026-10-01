use std::collections::HashSet;
use std::sync::{Arc, Mutex, RwLock};

use thiserror::Error;
use tokio::sync::mpsc;
use tokio::task::JoinSet;

#[derive(Clone, Debug)]
pub struct ProblemContext {
    pub source_name: String,
    pub source: Arc<str>,
}

impl ProblemContext {
    pub fn new(source_name: impl Into<String>, source: impl Into<Arc<str>>) -> Self {
        Self {
            source_name: source_name.into(),
            source: source.into(),
        }
    }
}

#[derive(Clone, Debug, PartialEq, Eq)]
pub struct Finding {
    pub rule_id: &'static str,
    pub message: String,
    pub line: Option<usize>,
}

#[derive(Clone, Debug, PartialEq, Eq)]
pub struct KingReport {
    pub king: String,
    pub findings: Vec<Finding>,
}

#[derive(Clone, Debug, PartialEq, Eq)]
pub struct KingEvent {
    pub king: String,
    pub finding_count: usize,
}

#[derive(Clone, Debug, Default, PartialEq, Eq)]
pub struct RunMetrics {
    pub runs: u64,
    pub kings_completed: u64,
    pub findings: u64,
}

#[derive(Clone, Debug, PartialEq, Eq)]
pub struct RunSummary {
    pub reports: Vec<KingReport>,
    pub completion_events: Vec<KingEvent>,
    pub metrics: RunMetrics,
}

#[derive(Debug, Error)]
pub enum KingError {
    #[error("king `{king}` failed: {message}")]
    KingFailure {
        king: &'static str,
        message: String,
    },
    #[error("king `{0}` is already registered")]
    DuplicateKing(&'static str),
    #[error("king registry lock is poisoned")]
    RegistryPoisoned,
    #[error("run metrics lock is poisoned")]
    MetricsPoisoned,
    #[error("king task failed: {0}")]
    Join(#[from] tokio::task::JoinError),
    #[error("multiple kings failed ({})", .0.len())]
    Multiple(Vec<KingError>),
}

pub trait DreadKing: Send + Sync + 'static {
    fn name(&self) -> &'static str;
    fn analyze(&self, context: Arc<ProblemContext>) -> Result<KingReport, KingError>;
}

#[derive(Default)]
struct Registry {
    kings: Vec<Arc<dyn DreadKing>>,
    names: HashSet<&'static str>,
}

#[derive(Default)]
pub struct DreadKings {
    registry: RwLock<Registry>,
    metrics: Mutex<RunMetrics>,
}

impl DreadKings {
    pub fn new() -> Self {
        Self::default()
    }

    pub fn register_king<A: DreadKing>(&self, king: A) -> Result<(), KingError> {
        self.register_boxed_king(Box::new(king))
    }

    pub fn register_boxed_king(&self, king: Box<dyn DreadKing>) -> Result<(), KingError> {
        let name = king.name();
        let mut registry = self
            .registry
            .write()
            .map_err(|_| KingError::RegistryPoisoned)?;
        if !registry.names.insert(name) {
            return Err(KingError::DuplicateKing(name));
        }
        let shared_king: Arc<dyn DreadKing> = Arc::from(king);
        registry.kings.push(shared_king);
        Ok(())
    }

    pub fn metrics(&self) -> Result<RunMetrics, KingError> {
        self.metrics
            .lock()
            .map(|metrics| metrics.clone())
            .map_err(|_| KingError::MetricsPoisoned)
    }

    pub async fn run(&self, context: Arc<ProblemContext>) -> Result<RunSummary, KingError> {
        let kings = self
            .registry
            .read()
            .map_err(|_| KingError::RegistryPoisoned)?
            .kings
            .clone();

        let (event_tx, mut event_rx) = mpsc::unbounded_channel();
        let mut tasks = JoinSet::new();
        for king in kings {
            let context = Arc::clone(&context);
            let event_tx = event_tx.clone();
            tasks.spawn_blocking(move || {
                let report = king.analyze(context)?;
                let _ = event_tx.send(KingEvent {
                    king: report.king.clone(),
                    finding_count: report.findings.len(),
                });
                Ok::<KingReport, KingError>(report)
            });
        }
        drop(event_tx);

        let mut reports = Vec::new();
        let mut failures = Vec::new();
        while let Some(result) = tasks.join_next().await {
            match result {
                Ok(Ok(report)) => reports.push(report),
                Ok(Err(error)) => failures.push(error),
                Err(error) => failures.push(KingError::Join(error)),
            }
        }

        let mut completion_events = Vec::new();
        while let Ok(event) = event_rx.try_recv() {
            completion_events.push(event);
        }
        reports.sort_by(|left, right| left.king.cmp(&right.king));
        completion_events.sort_by(|left, right| left.king.cmp(&right.king));

        let findings: u64 = reports
            .iter()
            .map(|report| report.findings.len() as u64)
            .sum();
        let metrics = {
            let mut metrics = self
                .metrics
                .lock()
                .map_err(|_| KingError::MetricsPoisoned)?;
            metrics.runs += 1;
            metrics.kings_completed += reports.len() as u64;
            metrics.findings += findings;
            metrics.clone()
        };

        if !failures.is_empty() {
            return Err(if failures.len() == 1 {
                failures.remove(0)
            } else {
                KingError::Multiple(failures)
            });
        }

        Ok(RunSummary {
            reports,
            completion_events,
            metrics,
        })
    }
}

#[derive(Default)]
pub struct RelaxedAtomicKing;

impl DreadKing for RelaxedAtomicKing {
    fn name(&self) -> &'static str {
        "relaxed-atomic-king-check"
    }

    fn analyze(&self, context: Arc<ProblemContext>) -> Result<KingReport, KingError> {
        let has_threads =
            context.source.contains("std::thread") || context.source.contains("std::jthread");
        let findings = context
            .source
            .lines()
            .enumerate()
            .filter(|(_, line)| {
                has_threads && line.contains(".store") && line.contains("memory_order_relaxed")
            })
            .map(|(index, line)| Finding {
                rule_id: "ISO-CONCURRENCY-RELAXED-RACE",
                message: format!("Relaxed atomic store in threaded source: {}", line.trim()),
                line: Some(index + 1),
            })
            .collect();

        Ok(KingReport {
            king: self.name().to_owned(),
            findings,
        })
    }
}

#[derive(Default)]
pub struct TodoMarkerKing;

impl DreadKing for TodoMarkerKing {
    fn name(&self) -> &'static str {
        "todo-marker-king-check"
    }

    fn analyze(&self, context: Arc<ProblemContext>) -> Result<KingReport, KingError> {
        let findings = context
            .source
            .lines()
            .enumerate()
            .filter(|(_, line)| line.contains("TODO") || line.contains("FIXME"))
            .map(|(index, line)| Finding {
                rule_id: "SOURCE-OPEN-ITEM",
                message: line.trim().to_owned(),
                line: Some(index + 1),
            })
            .collect();

        Ok(KingReport {
            king: self.name().to_owned(),
            findings,
        })
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use std::sync::Barrier;

    #[test]
    fn relaxed_atomic_king_reports_only_threaded_relaxed_stores() {
        let source = "std::thread worker;\nready.store(true, std::memory_order_relaxed);";
        let report = RelaxedAtomicKing
            .analyze(Arc::new(ProblemContext::new("sample.cpp", source)))
            .expect("analysis succeeds");

        assert_eq!(report.findings.len(), 1);
        assert_eq!(report.findings[0].line, Some(2));

        let no_thread_source = "ready.store(true, std::memory_order_relaxed);";
        let clean_report = RelaxedAtomicKing
            .analyze(Arc::new(ProblemContext::new(
                "sample.cpp",
                no_thread_source,
            )))
            .expect("analysis succeeds");
        assert!(clean_report.findings.is_empty());
    }

    #[test]
    fn duplicate_king_names_are_rejected() {
        let dread_kings = DreadKings::new();
        dread_kings
            .register_king(TodoMarkerKing)
            .expect("first registration");

        assert!(matches!(
            dread_kings.register_king(TodoMarkerKing),
            Err(KingError::DuplicateKing("todo-marker-king-check"))
        ));
    }

    struct BarrierKing {
        name: &'static str,
        barrier: Arc<Barrier>,
    }

    impl DreadKing for BarrierKing {
        fn name(&self) -> &'static str {
            self.name
        }

        fn analyze(&self, _context: Arc<ProblemContext>) -> Result<KingReport, KingError> {
            self.barrier.wait();
            Ok(KingReport {
                king: self.name.to_owned(),
                findings: Vec::new(),
            })
        }
    }

    #[tokio::test(flavor = "multi_thread", worker_threads = 2)]
    async fn dread_kings_runs_kings_concurrently_and_tracks_metrics_and_events() {
        let dread_kings = DreadKings::new();
        let barrier = Arc::new(Barrier::new(2));
        dread_kings
            .register_king(BarrierKing {
                name: "first",
                barrier: Arc::clone(&barrier),
            })
            .expect("register first king");
        dread_kings
            .register_king(BarrierKing {
                name: "second",
                barrier,
            })
            .expect("register second king");

        let summary = dread_kings
            .run(Arc::new(ProblemContext::new("sample.cpp", "")))
            .await
            .expect("dread_kings run succeeds");

        assert_eq!(summary.reports.len(), 2);
        assert_eq!(summary.completion_events.len(), 2);
        assert_eq!(summary.metrics.runs, 1);
        assert_eq!(summary.metrics.kings_completed, 2);
        assert_eq!(dread_kings.metrics().expect("metrics lock"), summary.metrics);
    }

    struct FailingKing(&'static str);

    impl DreadKing for FailingKing {
        fn name(&self) -> &'static str {
            self.0
        }

        fn analyze(&self, _context: Arc<ProblemContext>) -> Result<KingReport, KingError> {
            Err(KingError::KingFailure {
                king: self.0,
                message: "test failure".to_owned(),
            })
        }
    }

    #[tokio::test(flavor = "multi_thread", worker_threads = 2)]
    async fn dread_kings_collects_errors_from_all_kings() {
        let dread_kings = DreadKings::new();
        dread_kings
            .register_king(FailingKing("first"))
            .expect("register first");
        dread_kings
            .register_king(FailingKing("second"))
            .expect("register second");

        let result = dread_kings
            .run(Arc::new(ProblemContext::new("sample.cpp", "")))
            .await;

        assert!(matches!(result, Err(KingError::Multiple(errors)) if errors.len() == 2));
        assert_eq!(dread_kings.metrics().expect("metrics lock").runs, 1);
    }
}
