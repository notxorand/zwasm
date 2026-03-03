use serde::{Deserialize, Serialize};

#[derive(Debug, Serialize, Deserialize, PartialEq)]
struct Person {
    name: String,
    age: u32,
    city: String,
    scores: Vec<i32>,
}

fn main() {
    // Serialize
    let p = Person {
        name: "Alice".to_string(),
        age: 30,
        city: "Tokyo".to_string(),
        scores: vec![95, 87, 92],
    };

    let json = serde_json::to_string(&p).unwrap();
    println!("json: {json}");

    // Deserialize
    let p2: Person = serde_json::from_str(&json).unwrap();
    println!("name={} age={} city={}", p2.name, p2.age, p2.city);
    println!("scores={:?}", p2.scores);

    // Roundtrip check
    if p == p2 {
        println!("roundtrip: OK");
    } else {
        println!("roundtrip: FAIL");
    }

    // Parse dynamic JSON
    let data: serde_json::Value = serde_json::from_str(
        r#"{"users":[{"id":1,"name":"Bob"},{"id":2,"name":"Carol"}]}"#,
    )
    .unwrap();

    let users = data["users"].as_array().unwrap();
    println!("users: {}", users.len());
    for u in users {
        println!("  id={} name={}", u["id"], u["name"]);
    }

    // Nested serialization
    let nested = serde_json::json!({
        "status": "ok",
        "count": 42,
        "items": ["a", "b", "c"]
    });
    let nested_str = serde_json::to_string(&nested).unwrap();
    let parsed: serde_json::Value = serde_json::from_str(&nested_str).unwrap();
    if parsed["count"] == 42 && parsed["items"].as_array().unwrap().len() == 3 {
        println!("nested: OK");
    } else {
        println!("nested: FAIL");
    }

    println!("result: OK");
}
