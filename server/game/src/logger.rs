use std::{sync::OnceLock, time::SystemTime};

use colored::Colorize;
use log::Level;
use time::{format_description, OffsetDateTime};

pub struct Logger {
    format_desc: Vec<format_description::FormatItem<'static>>,
}

const TIME_FORMAT: &str = "[year]-[month]-[day] [hour]:[minute]:[second].[subsecond digits:3]";

impl Logger {
    pub fn instance() -> &'static Self {
        static INSTANCE: OnceLock<Logger> = OnceLock::new();
        INSTANCE.get_or_init(|| Logger {
            format_desc: format_description::parse(TIME_FORMAT).unwrap(),
        })
    }
}

impl log::Log for Logger {
    fn enabled(&self, metadata: &log::Metadata) -> bool {
        if metadata.target().starts_with("globed_game_server") {
            true
        } else {
            metadata.level() <= Level::Warn
        }
    }

    fn log(&self, record: &log::Record) {
        if !self.enabled(record.metadata()) {
            return;
        }

        let now: OffsetDateTime = SystemTime::now().into();
        let formatted_time = now.format(&self.format_desc).unwrap();

        let (level, args) = match record.level() {
            Level::Error => (
                record.level().to_string().bright_red(),
                record.args().to_string().bright_red(),
            ),
            Level::Warn => (
                record.level().to_string().bright_yellow(),
                record.args().to_string().bright_yellow(),
            ),
            Level::Info => (record.level().to_string().cyan(), record.args().to_string().cyan()),
            Level::Debug => (record.level().to_string().normal(), record.args().to_string().normal()),
            Level::Trace => (record.level().to_string().black(), record.args().to_string().black()),
        };

        println!("[{formatted_time}] [{level}] - {args}");
    }

    fn flush(&self) {}
}
