/// Simple pattern matching in Rust — no heap allocation
/// Uses fixed arrays to avoid any JIT-triggering allocator patterns

const MAX_STATES: usize = 128;

#[derive(Clone, Copy)]
enum StateKind {
    Literal,
    Any,
    Split,
    Accept,
}

#[derive(Clone, Copy)]
struct State {
    kind: StateKind,
    ch: u8,
    out1: usize,
    out2: usize,
}

struct Nfa {
    states: [State; MAX_STATES],
    nstates: usize,
    start: usize,
}

impl Nfa {
    fn new() -> Self {
        Nfa {
            states: [State { kind: StateKind::Accept, ch: 0, out1: 0, out2: 0 }; MAX_STATES],
            nstates: 0,
            start: 0,
        }
    }

    fn add_state(&mut self, kind: StateKind, ch: u8, out1: usize, out2: usize) -> usize {
        let id = self.nstates;
        self.states[id] = State { kind, ch, out1, out2 };
        self.nstates += 1;
        id
    }

    fn compile(&mut self, pattern: &[u8]) {
        self.nstates = 0;
        let accept = self.add_state(StateKind::Accept, 0, 0, 0);
        let mut next = accept;

        // Parse tokens from right to left
        let mut i = pattern.len();
        while i > 0 {
            i -= 1;

            // Check for quantifier
            let mut quant: u8 = 0;
            if pattern[i] == b'*' || pattern[i] == b'+' || pattern[i] == b'?' {
                quant = pattern[i];
                if i == 0 { break; }
                i -= 1;
            }

            // Match state
            let (kind, ch) = if pattern[i] == b'.' {
                (StateKind::Any, 0u8)
            } else {
                (StateKind::Literal, pattern[i])
            };

            if quant == b'*' {
                let match_id = self.add_state(kind, ch, next, 0); // temp
                let split_id = self.add_state(StateKind::Split, 0, match_id, next);
                self.states[match_id].out1 = split_id; // loop back
                next = split_id;
            } else if quant == b'+' {
                let match_id = self.add_state(kind, ch, next, 0);
                let split_id = self.add_state(StateKind::Split, 0, match_id, next);
                self.states[match_id].out1 = split_id;
                next = match_id;
            } else if quant == b'?' {
                let match_id = self.add_state(kind, ch, next, 0);
                let split_id = self.add_state(StateKind::Split, 0, match_id, next);
                next = split_id;
            } else {
                let match_id = self.add_state(kind, ch, next, 0);
                next = match_id;
            }
        }

        self.start = next;
    }

    fn run(&self, text: &[u8]) -> bool {
        let mut cur = [false; MAX_STATES];
        let mut nxt = [false; MAX_STATES];

        // Add start state with epsilon closure
        self.epsilon_close(self.start, &mut cur);

        for &byte in text {
            nxt = [false; MAX_STATES];
            for s in 0..self.nstates {
                if !cur[s] { continue; }
                match self.states[s].kind {
                    StateKind::Literal => {
                        if byte == self.states[s].ch {
                            self.epsilon_close(self.states[s].out1, &mut nxt);
                        }
                    }
                    StateKind::Any => {
                        self.epsilon_close(self.states[s].out1, &mut nxt);
                    }
                    _ => {}
                }
            }
            cur = nxt;
        }

        for s in 0..self.nstates {
            if cur[s] {
                if let StateKind::Accept = self.states[s].kind { return true; }
            }
        }
        false
    }

    fn epsilon_close(&self, state: usize, set: &mut [bool; MAX_STATES]) {
        if state >= self.nstates || set[state] { return; }
        set[state] = true;
        if let StateKind::Split = self.states[state].kind {
            self.epsilon_close(self.states[state].out1, set);
            self.epsilon_close(self.states[state].out2, set);
        }
    }
}

fn main() {
    let tests: [(&str, &str, bool); 20] = [
        ("abc",       "abc",      true),
        ("abc",       "abx",      false),
        ("a.c",       "abc",      true),
        ("a.c",       "aXc",      true),
        ("a.c",       "ac",       false),
        ("ab*c",      "ac",       true),
        ("ab*c",      "abc",      true),
        ("ab*c",      "abbbbc",   true),
        ("ab+c",      "ac",       false),
        ("ab+c",      "abc",      true),
        ("ab+c",      "abbbbc",   true),
        ("colou?r",   "color",    true),
        ("colou?r",   "colour",   true),
        ("a.*b",      "aXYZb",    true),
        ("a.*b",      "ab",       true),
        ("hello",     "hello",    true),
        ("hello",     "world",    false),
        ("h.l+o",     "hello",    true),
        ("h.l+o",     "hallo",    true),
        ("h.l+o",     "ho",       false),
    ];

    let mut pass = 0u32;
    let total = tests.len();

    for (pattern, text, expected) in &tests {
        let mut nfa = Nfa::new();
        nfa.compile(pattern.as_bytes());
        let result = nfa.run(text.as_bytes());
        if result == *expected {
            pass += 1;
        } else {
            println!("FAIL: /{pattern}/ ~ \"{text}\" expected {expected} got {result}");
        }
    }

    println!("regex tests: {pass}/{total} passed");

    if pass as usize == total {
        println!("result: OK");
    } else {
        println!("result: FAIL");
    }
}
