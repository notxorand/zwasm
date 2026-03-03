// json_parse.cpp — Simple recursive descent JSON parser
#include <cstdio>
#include <cstring>
#include <cstdlib>

enum JsonType { JSON_NULL, JSON_BOOL, JSON_NUMBER, JSON_STRING, JSON_ARRAY, JSON_OBJECT };

struct JsonValue {
    JsonType type;
    union {
        bool bool_val;
        double num_val;
        char str_val[256];
    };
    // For arrays/objects: children stored externally
    int child_count;
};

static const char *json_input;
static int pos;

static void skip_ws() {
    while (json_input[pos] == ' ' || json_input[pos] == '\n' ||
           json_input[pos] == '\r' || json_input[pos] == '\t')
        pos++;
}

static JsonValue parse_value();

static JsonValue parse_string() {
    JsonValue v;
    v.type = JSON_STRING;
    v.child_count = 0;
    pos++; // skip "
    int i = 0;
    while (json_input[pos] && json_input[pos] != '"' && i < 255) {
        if (json_input[pos] == '\\') {
            pos++;
            switch (json_input[pos]) {
                case 'n': v.str_val[i++] = '\n'; break;
                case 't': v.str_val[i++] = '\t'; break;
                case '"': v.str_val[i++] = '"'; break;
                case '\\': v.str_val[i++] = '\\'; break;
                default: v.str_val[i++] = json_input[pos]; break;
            }
        } else {
            v.str_val[i++] = json_input[pos];
        }
        pos++;
    }
    v.str_val[i] = '\0';
    if (json_input[pos] == '"') pos++;
    return v;
}

static JsonValue parse_number() {
    JsonValue v;
    v.type = JSON_NUMBER;
    v.child_count = 0;
    char buf[64];
    int i = 0;
    while ((json_input[pos] >= '0' && json_input[pos] <= '9') ||
           json_input[pos] == '.' || json_input[pos] == '-' ||
           json_input[pos] == '+' || json_input[pos] == 'e' ||
           json_input[pos] == 'E') {
        buf[i++] = json_input[pos++];
    }
    buf[i] = '\0';
    v.num_val = strtod(buf, nullptr);
    return v;
}

static JsonValue parse_array() {
    JsonValue v;
    v.type = JSON_ARRAY;
    v.child_count = 0;
    pos++; // skip [
    skip_ws();
    if (json_input[pos] == ']') { pos++; return v; }

    while (1) {
        skip_ws();
        parse_value(); // consume and count
        v.child_count++;
        skip_ws();
        if (json_input[pos] == ',') { pos++; continue; }
        if (json_input[pos] == ']') { pos++; break; }
        break; // error
    }
    return v;
}

static JsonValue parse_object() {
    JsonValue v;
    v.type = JSON_OBJECT;
    v.child_count = 0;
    pos++; // skip {
    skip_ws();
    if (json_input[pos] == '}') { pos++; return v; }

    while (1) {
        skip_ws();
        parse_string(); // key
        skip_ws();
        if (json_input[pos] == ':') pos++;
        skip_ws();
        parse_value(); // value
        v.child_count++;
        skip_ws();
        if (json_input[pos] == ',') { pos++; continue; }
        if (json_input[pos] == '}') { pos++; break; }
        break; // error
    }
    return v;
}

static JsonValue parse_value() {
    skip_ws();
    char c = json_input[pos];

    if (c == '"') return parse_string();
    if (c == '{') return parse_object();
    if (c == '[') return parse_array();
    if (c == 't') {
        pos += 4; // true
        JsonValue v; v.type = JSON_BOOL; v.bool_val = true; v.child_count = 0;
        return v;
    }
    if (c == 'f') {
        pos += 5; // false
        JsonValue v; v.type = JSON_BOOL; v.bool_val = false; v.child_count = 0;
        return v;
    }
    if (c == 'n') {
        pos += 4; // null
        JsonValue v; v.type = JSON_NULL; v.child_count = 0;
        return v;
    }
    return parse_number();
}

static const char *type_name(JsonType t) {
    switch (t) {
        case JSON_NULL: return "null";
        case JSON_BOOL: return "bool";
        case JSON_NUMBER: return "number";
        case JSON_STRING: return "string";
        case JSON_ARRAY: return "array";
        case JSON_OBJECT: return "object";
    }
    return "unknown";
}

struct TestCase {
    const char *json;
    JsonType expected_type;
    int expected_children; // -1 = don't check
};

int main() {
    TestCase tests[] = {
        {"null", JSON_NULL, -1},
        {"true", JSON_BOOL, -1},
        {"false", JSON_BOOL, -1},
        {"42", JSON_NUMBER, -1},
        {"3.14", JSON_NUMBER, -1},
        {"-1.5e10", JSON_NUMBER, -1},
        {"\"hello\"", JSON_STRING, -1},
        {"\"esc\\nape\"", JSON_STRING, -1},
        {"[]", JSON_ARRAY, 0},
        {"[1, 2, 3]", JSON_ARRAY, 3},
        {"{}", JSON_OBJECT, 0},
        {"{\"a\": 1, \"b\": 2}", JSON_OBJECT, 2},
        {"[1, \"two\", true, null]", JSON_ARRAY, 4},
        {"{\"name\": \"Alice\", \"age\": 30, \"scores\": [95, 87, 92]}", JSON_OBJECT, 3},
        {"{\"nested\": {\"deep\": {\"value\": 42}}}", JSON_OBJECT, 1},
    };
    int ntests = sizeof(tests) / sizeof(tests[0]);
    int pass = 0;

    for (int i = 0; i < ntests; i++) {
        json_input = tests[i].json;
        pos = 0;
        JsonValue v = parse_value();

        bool ok = (v.type == tests[i].expected_type);
        if (ok && tests[i].expected_children >= 0) {
            ok = (v.child_count == tests[i].expected_children);
        }

        if (ok) {
            pass++;
        } else {
            printf("FAIL test %d: got type=%s children=%d\n",
                   i + 1, type_name(v.type), v.child_count);
        }
    }

    printf("json parse tests: %d/%d passed\n", pass, ntests);

    // Parse a larger document
    const char *doc =
        "{\"users\": ["
        "  {\"id\": 1, \"name\": \"Alice\", \"active\": true},"
        "  {\"id\": 2, \"name\": \"Bob\", \"active\": false},"
        "  {\"id\": 3, \"name\": \"Carol\", \"active\": true}"
        "], \"count\": 3}";
    json_input = doc;
    pos = 0;
    JsonValue root = parse_value();
    printf("large doc: type=%s children=%d\n", type_name(root.type), root.child_count);

    if (pass == ntests)
        printf("result: OK\n");
    else
        printf("result: FAIL\n");

    return 0;
}
