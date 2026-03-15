use std::fs;
use std::io::Write;

fn main() {
    let path = std::env::args()
        .nth(1)
        .unwrap_or_else(|| "zwasm_test_file_io.txt".to_string());
    let content = "Hello from Rust/WASI file I/O!\nLine 2\nLine 3\n";

    // Write
    {
        let mut f = fs::File::create(&path).expect("failed to create file");
        f.write_all(content.as_bytes()).expect("failed to write");
    }

    // Read back
    let read_back = fs::read_to_string(&path).expect("failed to read file");

    if read_back == content {
        println!("file_io: write/read roundtrip OK ({} bytes)", content.len());
    } else {
        eprintln!("file_io: MISMATCH!");
        eprintln!("expected: {:?}", content);
        eprintln!("got:      {:?}", read_back);
        std::process::exit(1);
    }

    // Cleanup
    fs::remove_file(&path).expect("failed to remove file");
    println!("file_io: cleanup OK");
}
