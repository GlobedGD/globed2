use std::path::Path;

use aho_corasick::AhoCorasick;

pub struct WordFilter {
    algo: AhoCorasick,
    word_count: usize,
}

impl WordFilter {
    pub fn new(words: &[String]) -> Self {
        Self {
            word_count: words.len(),
            algo: AhoCorasick::builder()
                .ascii_case_insensitive(true)
                .build(words)
                .expect("failed to create word filter"),
        }
    }

    pub fn is_bad(&self, content: &str) -> bool {
        self.algo.find(content).is_some()
    }

    pub fn reload_from_file(&mut self, path: &Path) -> Result<(), std::io::Error> {
        let words = std::fs::read_to_string(path)?.lines().map(|x| x.to_string()).collect::<Vec<_>>();

        let new_filter = Self::new(&words);
        self.algo = new_filter.algo;
        self.word_count = new_filter.word_count;

        Ok(())
    }

    pub fn word_count(&self) -> usize {
        self.word_count
    }
}

impl Default for WordFilter {
    fn default() -> Self {
        Self::new(&[])
    }
}
