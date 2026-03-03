use flate2::read::{DeflateDecoder, DeflateEncoder};
use flate2::Compression;
use std::io::Read;

fn compress(data: &[u8]) -> Vec<u8> {
    let mut encoder = DeflateEncoder::new(data, Compression::default());
    let mut compressed = Vec::new();
    encoder.read_to_end(&mut compressed).unwrap();
    compressed
}

fn decompress(data: &[u8]) -> Vec<u8> {
    let mut decoder = DeflateDecoder::new(data);
    let mut decompressed = Vec::new();
    decoder.read_to_end(&mut decompressed).unwrap();
    decompressed
}

fn main() {
    let tests: Vec<&[u8]> = vec![
        b"Hello, compression!",
        b"AAAAAABBBBBBCCCCCCDDDDDDEEEEEEFFFFFFGGGGGG",
        b"The quick brown fox jumps over the lazy dog. \
          The quick brown fox jumps over the lazy dog. \
          The quick brown fox jumps over the lazy dog.",
    ];

    let mut pass = 0;
    for (i, original) in tests.iter().enumerate() {
        let compressed = compress(original);
        let decompressed = decompress(&compressed);

        let ratio = 100.0 * compressed.len() as f64 / original.len() as f64;
        let ok = decompressed == *original;

        println!(
            "test {}: {} -> {} bytes ({:.1}%), roundtrip={}",
            i + 1,
            original.len(),
            compressed.len(),
            ratio,
            if ok { "OK" } else { "FAIL" }
        );

        if ok {
            pass += 1;
        }
    }

    // Large data test
    let large: Vec<u8> = (0..10000u32).map(|i| (i % 256) as u8).collect();
    let compressed = compress(&large);
    let decompressed = decompress(&compressed);
    let ok = decompressed == large;
    println!(
        "test 4: {} -> {} bytes ({:.1}%), roundtrip={}",
        large.len(),
        compressed.len(),
        100.0 * compressed.len() as f64 / large.len() as f64,
        if ok { "OK" } else { "FAIL" }
    );
    if ok {
        pass += 1;
    }

    println!("compression tests: {pass}/4 passed");
    if pass == 4 {
        println!("result: OK");
    } else {
        println!("result: FAIL");
    }
}
