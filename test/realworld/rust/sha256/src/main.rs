use sha2::{Digest, Sha256};

fn hex(bytes: &[u8]) -> String {
    bytes.iter().map(|b| format!("{b:02x}")).collect()
}

fn main() {
    // Test vectors
    let tests: Vec<(&str, &str)> = vec![
        (
            "",
            "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855",
        ),
        (
            "abc",
            "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad",
        ),
        (
            "Hello, SHA-256!",
            "3d61375cc279a6fe4c887bc4eee58ec78bc32cf2b026252918b9b0d74eaffeb5",
        ),
    ];

    let mut pass = 0;
    for (input, expected) in &tests {
        let mut hasher = Sha256::new();
        hasher.update(input.as_bytes());
        let result = hex(&hasher.finalize());
        if result == *expected {
            pass += 1;
            println!("PASS: sha256(\"{input}\") = {result}");
        } else {
            println!("FAIL: sha256(\"{input}\") = {result} (expected {expected})");
        }
    }

    // Incremental hashing
    let mut hasher = Sha256::new();
    hasher.update(b"Hello, ");
    hasher.update(b"SHA-256!");
    let incremental = hex(&hasher.finalize());
    let expected = "3d61375cc279a6fe4c887bc4eee58ec78bc32cf2b026252918b9b0d74eaffeb5";
    if incremental == expected {
        pass += 1;
        println!("PASS: incremental hash matches");
    } else {
        println!("FAIL: incremental hash mismatch");
    }

    println!("sha256 tests: {pass}/4 passed");
    if pass == 4 {
        println!("result: OK");
    } else {
        println!("result: FAIL");
    }
}
