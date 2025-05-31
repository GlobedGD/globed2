use std::path::Path;

use aho_corasick::AhoCorasick;
use globed_shared::debug;

pub struct WordFilter {
    algo: AhoCorasick,
    word_count: usize,
    whole_words: Vec<String>,
}

impl WordFilter {
    pub fn new(words: &[String], whole_words: Vec<String>) -> Self {
        Self {
            word_count: words.len() + whole_words.len(),
            algo: AhoCorasick::builder()
                .ascii_case_insensitive(true)
                .build(words)
                .expect("failed to create word filter"),
            whole_words,
        }
    }

    pub fn new_from_lines(mut words: Vec<String>) -> Self {
        let mut whole_words = Vec::new();

        words.retain_mut(|w| {
            let is_whole = w.starts_with("!!") && w.ends_with("!!") && w.len() > 4;

            if is_whole {
                let mut word = std::mem::take(w);
                word.remove_matches("!!");
                whole_words.push(word);
            }

            !is_whole && !w.is_empty()
        });

        Self::new(&words, whole_words)
    }

    pub fn is_bad(&self, content: &str) -> bool {
        if self.algo.find(content).is_some() {
            return true;
        }

        // check if any of the words are contained in self.whole_words
        content
            .split(' ')
            .any(|word| self.whole_words.iter().any(|w| word.eq_ignore_ascii_case(w)))
    }

    pub fn reload_from_file(&mut self, path: &Path) -> Result<(), std::io::Error> {
        let lines = std::fs::read_to_string(path)?.lines().map(|x| x.to_string()).collect::<Vec<_>>();
        let new_filter = Self::new_from_lines(lines);
        self.algo = new_filter.algo;
        self.word_count = new_filter.word_count;
        self.whole_words = new_filter.whole_words;

        Ok(())
    }

    pub fn word_count(&self) -> usize {
        self.word_count
    }
}

impl Default for WordFilter {
    fn default() -> Self {
        Self::new(&[], Vec::new())
    }
}
