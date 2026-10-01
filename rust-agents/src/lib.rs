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
pub struct AgentReport {
    pub agent: String,
    pub findings: Vec<Finding>,
}

#[derive(Clone, Debug, PartialEq, Eq)]
pub struct AgentEvent {
    pub agent: String,
    pub finding_count: usize,
}

#[derive(Clone, Debug, Default, PartialEq, Eq)]
pub struct RunMetrics {
    pub runs: u64,
    pub agents_completed: u64,
    pub findings: u64,
}

#[derive(Clone, Debug, PartialEq, Eq)]
pub struct RunSummary {
    pub reports: Vec<AgentReport>,
    pub completion_events: Vec<AgentEvent>,
    pub metrics: RunMetrics,
}

#[derive(Debug, Error)]
pub enum AgentError {
    #[error("agent `{agent}` failed: {message}")]
    AgentFailure {
        agent: &'static str,
        message: String,
    },
    #[error("agent `{0}` is already registered")]
    DuplicateAgent(&'static str),
    #[error("agent registry lock is poisoned")]
    RegistryPoisoned,
    #[error("run metrics lock is poisoned")]
    MetricsPoisoned,
    #[error("agent task failed: {0}")]
    Join(#[from] tokio::task::JoinError),
    #[error("multiple agents failed ({})", .0.len())]
    Multiple(Vec<AgentError>),
}

pub trait Agent: Send + Sync + 'static {
    fn name(&self) -> &'static str;
    fn analyze(&self, context: Arc<ProblemContext>) -> Result<AgentReport, AgentError>;
}

#[derive(Default)]
struct Registry {
    agents: Vec<Arc<dyn Agent>>,
    names: HashSet<&'static str>,
}

#[derive(Default)]
pub struct Council {
    registry: RwLock<Registry>,
    metrics: Mutex<RunMetrics>,
}

impl Council {
    pub fn new() -> Self {
        Self::default()
    }

    pub fn register_agent<A: Agent>(&self, agent: A) -> Result<(), AgentError> {
        self.register_boxed(Box::new(agent))
    }

    pub fn register_boxed(&self, agent: Box<dyn Agent>) -> Result<(), AgentError> {
        let name = agent.name();
        let mut registry = self
            .registry
            .write()
            .map_err(|_| AgentError::RegistryPoisoned)?;
        if !registry.names.insert(name) {
            return Err(AgentError::DuplicateAgent(name));
        }
        let shared_agent: Arc<dyn Agent> = Arc::from(agent);
        registry.agents.push(shared_agent);
        Ok(())
    }

    pub fn metrics(&self) -> Result<RunMetrics, AgentError> {
        self.metrics
            .lock()
            .map(|metrics| metrics.clone())
            .map_err(|_| AgentError::MetricsPoisoned)
    }

    pub async fn run(&self, context: Arc<ProblemContext>) -> Result<RunSummary, AgentError> {
        let agents = self
            .registry
            .read()
            .map_err(|_| AgentError::RegistryPoisoned)?
            .agents
            .clone();

        let (event_tx, mut event_rx) = mpsc::unbounded_channel();
        let mut tasks = JoinSet::new();
        for agent in agents {
            let context = Arc::clone(&context);
            let event_tx = event_tx.clone();
            tasks.spawn_blocking(move || {
                let report = agent.analyze(context)?;
                let _ = event_tx.send(AgentEvent {
                    agent: report.agent.clone(),
                    finding_count: report.findings.len(),
                });
                Ok::<AgentReport, AgentError>(report)
            });
        }
        drop(event_tx);

        let mut reports = Vec::new();
        let mut failures = Vec::new();
        while let Some(result) = tasks.join_next().await {
            match result {
                Ok(Ok(report)) => reports.push(report),
                Ok(Err(error)) => failures.push(error),
                Err(error) => failures.push(AgentError::Join(error)),
            }
        }

        let mut completion_events = Vec::new();
        while let Ok(event) = event_rx.try_recv() {
            completion_events.push(event);
        }
        reports.sort_by(|left, right| left.agent.cmp(&right.agent));
        completion_events.sort_by(|left, right| left.agent.cmp(&right.agent));

        let findings: u64 = reports
            .iter()
            .map(|report| report.findings.len() as u64)
            .sum();
        let metrics = {
            let mut metrics = self
                .metrics
                .lock()
                .map_err(|_| AgentError::MetricsPoisoned)?;
            metrics.runs += 1;
            metrics.agents_completed += reports.len() as u64;
            metrics.findings += findings;
            metrics.clone()
        };

        if !failures.is_empty() {
            return Err(if failures.len() == 1 {
                failures.remove(0)
            } else {
                AgentError::Multiple(failures)
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
pub struct RelaxedAtomicAgent;

impl Agent for RelaxedAtomicAgent {
    fn name(&self) -> &'static str {
        "relaxed-atomic-check"
    }

    fn analyze(&self, context: Arc<ProblemContext>) -> Result<AgentReport, AgentError> {
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

        Ok(AgentReport {
            agent: self.name().to_owned(),
            findings,
        })
    }
}

#[derive(Default)]
pub struct TodoMarkerAgent;

impl Agent for TodoMarkerAgent {
    fn name(&self) -> &'static str {
        "todo-marker-check"
    }

    fn analyze(&self, context: Arc<ProblemContext>) -> Result<AgentReport, AgentError> {
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

        Ok(AgentReport {
            agent: self.name().to_owned(),
            findings,
        })
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use std::sync::Barrier;

    #[test]
    fn relaxed_atomic_agent_reports_only_threaded_relaxed_stores() {
        let source = "std::thread worker;\nready.store(true, std::memory_order_relaxed);";
        let report = RelaxedAtomicAgent
            .analyze(Arc::new(ProblemContext::new("sample.cpp", source)))
            .expect("analysis succeeds");

        assert_eq!(report.findings.len(), 1);
        assert_eq!(report.findings[0].line, Some(2));

        let no_thread_source = "ready.store(true, std::memory_order_relaxed);";
        let clean_report = RelaxedAtomicAgent
            .analyze(Arc::new(ProblemContext::new(
                "sample.cpp",
                no_thread_source,
            )))
            .expect("analysis succeeds");
        assert!(clean_report.findings.is_empty());
    }

    #[test]
    fn duplicate_agent_names_are_rejected() {
        let council = Council::new();
        council
            .register_agent(TodoMarkerAgent)
            .expect("first registration");

        assert!(matches!(
            council.register_agent(TodoMarkerAgent),
            Err(AgentError::DuplicateAgent("todo-marker-check"))
        ));
    }

    struct BarrierAgent {
        name: &'static str,
        barrier: Arc<Barrier>,
    }

    impl Agent for BarrierAgent {
        fn name(&self) -> &'static str {
            self.name
        }

        fn analyze(&self, _context: Arc<ProblemContext>) -> Result<AgentReport, AgentError> {
            self.barrier.wait();
            Ok(AgentReport {
                agent: self.name.to_owned(),
                findings: Vec::new(),
            })
        }
    }

    #[tokio::test(flavor = "multi_thread", worker_threads = 2)]
    async fn council_runs_agents_concurrently_and_tracks_metrics_and_events() {
        let council = Council::new();
        let barrier = Arc::new(Barrier::new(2));
        council
            .register_agent(BarrierAgent {
                name: "first",
                barrier: Arc::clone(&barrier),
            })
            .expect("register first agent");
        council
            .register_agent(BarrierAgent {
                name: "second",
                barrier,
            })
            .expect("register second agent");

        let summary = council
            .run(Arc::new(ProblemContext::new("sample.cpp", "")))
            .await
            .expect("council run succeeds");

        assert_eq!(summary.reports.len(), 2);
        assert_eq!(summary.completion_events.len(), 2);
        assert_eq!(summary.metrics.runs, 1);
        assert_eq!(summary.metrics.agents_completed, 2);
        assert_eq!(council.metrics().expect("metrics lock"), summary.metrics);
    }

    struct FailingAgent(&'static str);

    impl Agent for FailingAgent {
        fn name(&self) -> &'static str {
            self.0
        }

        fn analyze(&self, _context: Arc<ProblemContext>) -> Result<AgentReport, AgentError> {
            Err(AgentError::AgentFailure {
                agent: self.0,
                message: "test failure".to_owned(),
            })
        }
    }

    #[tokio::test(flavor = "multi_thread", worker_threads = 2)]
    async fn council_collects_errors_from_all_agents() {
        let council = Council::new();
        council
            .register_agent(FailingAgent("first"))
            .expect("register first");
        council
            .register_agent(FailingAgent("second"))
            .expect("register second");

        let result = council
            .run(Arc::new(ProblemContext::new("sample.cpp", "")))
            .await;

        assert!(matches!(result, Err(AgentError::Multiple(errors)) if errors.len() == 2));
        assert_eq!(council.metrics().expect("metrics lock").runs, 1);
    }
}
