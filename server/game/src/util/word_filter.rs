use aho_corasick::AhoCorasick;

pub struct WordFilter {
    algo: AhoCorasick,
}

impl WordFilter {
    pub fn new(words: &[String]) -> Self {
        Self {
            algo: AhoCorasick::builder()
                .ascii_case_insensitive(true)
                .build(words)
                .expect("failed to create word filter"),
        }
    }

    pub fn is_bad(&self, content: &str) -> bool {
        self.algo.find(content).is_some()
    }
}

impl Default for WordFilter {
    fn default() -> Self {
        Self::new(&[])
    }
}
