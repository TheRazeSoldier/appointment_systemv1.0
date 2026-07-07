#include "auth.h"
#include <iostream>
#include <string>

bool testSHA256(const std::string& input, const std::string& expected) {
    std::string result = Auth::sha256(input);
    bool passed = (result == expected);
    std::cout << "Test: '" << (input.empty() ? "(empty)" : input) << "'\n";
    std::cout << "Expected: " << expected << "\n";
    std::cout << "Got:      " << result << "\n";
    std::cout << (passed ? "PASS" : "FAIL") << "\n\n";
    return passed;
}

int main() {
    std::cout << "SHA256 Verification Tests\n";
    std::cout << "=========================\n\n";
    
    bool allPassed = true;
    
    allPassed &= testSHA256("", "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");
    allPassed &= testSHA256("abc", "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");
    allPassed &= testSHA256("abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq", 
                           "248d6a61d20638b8e5c026930c3e6039a33ce45964ff2167f6ecedd419db06c1");
    allPassed &= testSHA256("The quick brown fox jumps over the lazy dog",
                           "d7a8fbb307d7809469ca9abcb0082e4f8d5651e46d3cdb762d02d0bf37c9e592");
    
    std::cout << "=========================\n";
    std::cout << (allPassed ? "ALL TESTS PASSED" : "SOME TESTS FAILED") << "\n";
    
    return allPassed ? 0 : 1;
}