use std::{sync::OnceLock, time::SystemTime};

use colored::Colorize;
use time::{format_description, OffsetDateTime};

pub use log;
pub use log::{debug, error, info, trace, warn, Level as LogLevel, LevelFilter as LogLevelFilter};

pub struct Logger {
    format_desc: Vec<format_description::FormatItem<'static>>,
    self_crate_name: &'static str,
}

const TIME_FORMAT: &str = "[year]-[month]-[day] [hour]:[minute]:[second].[subsecond digits:3]";

impl Logger {
    #[allow(clippy::missing_panics_doc)]
    pub fn instance(self_crate_name: &'static str) -> &'static Self {
        static INSTANCE: OnceLock<Logger> = OnceLock::new();
        INSTANCE.get_or_init(|| Logger {
            format_desc: format_description::parse_borrowed::<2>(TIME_FORMAT).unwrap(),
            self_crate_name,
        })
    }
}

impl log::Log for Logger {
    fn enabled(&self, metadata: &log::Metadata) -> bool {
        if metadata.target().starts_with(self.self_crate_name) {
            true
        } else {
            metadata.level() <= LogLevel::Warn
        }
    }

    fn log(&self, record: &log::Record) {
        if !self.enabled(record.metadata()) {
            return;
        }

        let now: OffsetDateTime = SystemTime::now().into();
        let formatted_time = now.format(&self.format_desc).unwrap();

        let (level, args) = match record.level() {
            LogLevel::Error => (
                record.level().to_string().bright_red(),
                record.args().to_string().bright_red(),
            ),
            LogLevel::Warn => (
                record.level().to_string().bright_yellow(),
                record.args().to_string().bright_yellow(),
            ),
            LogLevel::Info => (record.level().to_string().cyan(), record.args().to_string().cyan()),
            LogLevel::Debug => (record.level().to_string().normal(), record.args().to_string().normal()),
            LogLevel::Trace => (record.level().to_string().black(), record.args().to_string().black()),
        };

        if record.level() == LogLevel::Error {
            eprintln!("[{formatted_time}] [{level}] - {args}");
        } else {
            println!("[{formatted_time}] [{level}] - {args}");
        }
    }

    fn flush(&self) {}
}
