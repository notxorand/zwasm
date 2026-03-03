package main

import (
	"fmt"
	"regexp"
)

func main() {
	tests := []struct {
		pattern  string
		text     string
		expected bool
	}{
		{`^\d{3}-\d{4}$`, "123-4567", true},
		{`^\d{3}-\d{4}$`, "12-4567", false},
		{`[a-z]+@[a-z]+\.[a-z]+`, "user@example.com", true},
		{`[a-z]+@[a-z]+\.[a-z]+`, "no-at-sign", false},
		{`^[A-Z][a-z]+$`, "Hello", true},
		{`^[A-Z][a-z]+$`, "hello", false},
		{`\b\w{5}\b`, "hello world", true},
		{`\b\w{5}\b`, "hi ok", false},
	}

	pass := 0
	for _, t := range tests {
		re := regexp.MustCompile(t.pattern)
		result := re.MatchString(t.text)
		if result == t.expected {
			pass++
		} else {
			fmt.Printf("FAIL: /%s/ ~ %q expected %v got %v\n",
				t.pattern, t.text, t.expected, result)
		}
	}

	fmt.Printf("regex tests: %d/%d passed\n", pass, len(tests))

	// Find submatch
	re := regexp.MustCompile(`(\d{4})-(\d{2})-(\d{2})`)
	m := re.FindStringSubmatch("Date: 2024-03-15 today")
	if len(m) >= 4 {
		fmt.Printf("capture: year=%s month=%s day=%s\n", m[1], m[2], m[3])
	}

	// Find all
	re2 := regexp.MustCompile(`\d+`)
	nums := re2.FindAllString("abc123def456ghi789", -1)
	fmt.Printf("find_all: %v\n", nums)

	if pass == len(tests) {
		fmt.Println("result: OK")
	} else {
		fmt.Println("result: FAIL")
	}
}
