#ifndef DES_H
#define DES_H

#include <string>
#include <bitset>
using namespace std;

// Initial Permutation Table
int IP[] = {
    58, 50, 42, 34, 26, 18, 10, 2,
    60, 52, 44, 36, 28, 20, 12, 4,
    62, 54, 46, 38, 30, 22, 14, 6,
    64, 56, 48, 40, 32, 24, 16, 8,
    57, 49, 41, 33, 25, 17, 9, 1,
    59, 51, 43, 35, 27, 19, 11, 3,
    61, 53, 45, 37, 29, 21, 13, 5,
    63, 55, 47, 39, 31, 23, 15, 7
};
// Final Permutation Table
int FP[] = {
    40, 8, 48, 16, 56, 24, 64, 32,
    39, 7, 47, 15, 55, 23, 63, 31,
    38, 6, 46, 14, 54, 22, 62, 30,
    37, 5, 45, 13, 53, 21, 61, 29,
    36, 4, 44, 12, 52, 20, 60, 28,
    35, 3, 43, 11, 51, 19, 59, 27,
    34, 2, 42, 10, 50, 18, 58, 26,
    33, 1, 41, 9, 49, 17, 57, 25
};

// Expansion Box
int E[] = {
    32, 1, 2, 3, 4, 5,
    4, 5, 6, 7, 8, 9,
    8, 9, 10, 11, 12, 13,
    12, 13, 14, 15, 16, 17,
    16, 17, 18, 19, 20, 21,
    20, 21, 22, 23, 24, 25,
    24, 25, 26, 27, 28, 29,
    28, 29, 30, 31, 32, 1
};
// Permutation Box
int P[] = {
    16, 7, 20, 21, 29, 12, 28, 17,
    1, 15, 23, 26, 5, 18, 31, 10,
    2, 8, 24, 14, 32, 27, 3, 9,
    19, 13, 30, 6, 22, 11, 4, 25
};

// Permuted Choice 1
int PC1[] = {
    57, 49, 41, 33, 25, 17, 9,
    1, 58, 50, 42, 34, 26, 18,
    10, 2, 59, 51, 43, 35, 27,
    19, 11, 3, 60, 52, 44, 36,
    63, 55, 47, 39, 31, 23, 15,
    7, 62, 54, 46, 38, 30, 22,
    14, 6, 61, 53, 45, 37, 29,
    21, 13, 5, 28, 20, 12, 4
};

// Permuted Choice 2
int PC2[] = {
    14, 17, 11, 24, 1, 5,
    3, 28, 15, 6, 21, 10,
    23, 19, 12, 4, 26, 8,
    16, 7, 27, 20, 13, 2,
    41, 52, 31, 37, 47, 55,
    30, 40, 51, 45, 33, 48,
    44, 49, 39, 56, 34, 53,
    46, 42, 50, 36, 29, 32
};

// Shift table
int shiftTable[] = { 1, 1, 2, 2, 2, 2, 2, 2, 1, 2, 2, 2, 2, 2, 2, 1 };

// S-Boxes
int S[8][4][16] = {
    {
        {14, 4, 13, 1, 2, 15, 11, 8, 3, 10, 6, 12, 5, 9, 0, 7},
        {0, 15, 7, 4, 14, 2, 13, 1, 10, 6, 12, 11, 9, 5, 3, 8},
        {4, 1, 14, 8, 13, 6, 2, 11, 15, 12, 9, 7, 3, 10, 5, 0},
        {15, 12, 8, 2, 4, 9, 1, 7, 5, 11, 3, 14, 10, 0, 6, 13}
    },
    {
        {15, 1, 8, 14, 6, 11, 3, 4, 9, 7, 2, 13, 12, 0, 5, 10},
        {3, 13, 4, 7, 15, 2, 8, 14, 12, 0, 1, 10, 6, 9, 11, 5},
        {0, 14, 7, 11, 10, 4, 13, 1, 5, 8, 12, 6, 9, 3, 2, 15},
        {13, 8, 10, 1, 3, 15, 4, 2, 11, 6, 7, 12, 0, 5, 14, 9}
    },
    {
        {10, 0, 9, 14, 6, 3, 15, 5, 1, 13, 12, 7, 11, 4, 2, 8},
        {13, 7, 0, 9, 3, 4, 6, 10, 2, 8, 5, 14, 12, 11, 15, 1},
        {13, 6, 4, 9, 8, 15, 3, 0, 11, 1, 2, 12, 5, 10, 14, 7},
        {1, 10, 13, 0, 6, 9, 8, 7, 4, 15, 14, 3, 11, 5, 2, 12}
    },
    {
        {7, 13, 14, 3, 0, 6, 9, 10, 1, 2, 8, 5, 11, 12, 4, 15},
        {13, 8, 11, 5, 6, 15, 0, 3, 4, 7, 2, 12, 1, 10, 14, 9},
        {10, 6, 9, 0, 12, 11, 7, 13, 15, 1, 3, 14, 5, 2, 8, 4},
        {3, 15, 0, 6, 10, 1, 13, 8, 9, 4, 5, 11, 12, 7, 2, 14}
    },
    {
        {2, 12, 4, 1, 7, 10, 11, 6, 8, 5, 3, 15, 13, 0, 14, 9},
        {14, 11, 2, 12, 4, 7, 13, 1, 5, 0, 15, 10, 3, 9, 8, 6},
        {4, 2, 1, 11, 10, 13, 7, 8, 15, 9, 12, 5, 6, 3, 0, 14},
        {11, 8, 12, 7, 1, 14, 2, 13, 6, 15, 0, 9, 10, 4, 5, 3}
    },
    {
        {12, 1, 10, 15, 9, 2, 6, 8, 0, 13, 3, 4, 14, 7, 5, 11},
        {10, 15, 4, 2, 7, 12, 9, 5, 6, 1, 13, 14, 0, 11, 3, 8},
        {9, 14, 15, 5, 2, 8, 12, 3, 7, 0, 4, 10, 1, 13, 11, 6},
        {4, 3, 2, 12, 9, 5, 15, 10, 11, 14, 1, 7, 6, 0, 8, 13}
    },
    {
        {4, 11, 2, 14, 15, 0, 8, 13, 3, 12, 9, 7, 5, 10, 6, 1},
        {13, 0, 11, 7, 4, 9, 1, 10, 14, 3, 5, 12, 2, 15, 8, 6},
        {1, 4, 11, 13, 12, 3, 7, 14, 10, 15, 6, 8, 0, 5, 9, 2},
        {6, 11, 13, 8, 1, 4, 10, 7, 9, 5, 0, 15, 14, 2, 3, 12}
    },
    {
        {13, 2, 8, 4, 6, 15, 11, 1, 10, 9, 3, 14, 5, 0, 12, 7},
        {1, 15, 13, 8, 10, 3, 7, 4, 12, 5, 6, 11, 0, 14, 9, 2},
        {7, 11, 4, 1, 9, 12, 14, 2, 0, 5, 10, 3, 13, 8, 15, 6},
        {2, 1, 14, 7, 4, 10, 8, 13, 15, 12, 9, 0, 3, 5, 6, 11}
    }
};

//utility function
string permute(string input, int* table, int n) {
    string output = "";
    for (int i = 0; i < n; i++) {
        output += input[table[i] - 1];
    }
    return output;
}

string leftShift(string input, int shifts) {
    string output = "";
    for (int i = 0; i < input.length(); i++) {
        output += input[(i + shifts) % input.length()];
    }
    return output;
}

string xorStrings(string a, string b) {
    string result = "";
    for (int i = 0; i < a.length(); i++) {
        result += (a[i] == b[i]) ? '0' : '1';
    }
    return result;
}

string decimalToBinary(int decimal) {
    return bitset<4>(decimal).to_string();
}

int binaryToDecimal(string binary) {
    int decimal = 0;
    int power = 1;
    for (int i = binary.length() - 1; i >= 0; i--) {
        if (binary[i] == '1') {
            decimal += power;
        }
        power *= 2;
    }
    return decimal;
}
//des core function
void generateRoundKeys(string key, string* roundKeys) {
    string permutedKey = permute(key, PC1, 56);
    string left = permutedKey.substr(0, 28);
    string right = permutedKey.substr(28, 28);

    for (int round = 0; round < 16; round++) {
        left = leftShift(left, shiftTable[round]);
        right = leftShift(right, shiftTable[round]);
        string combined = left + right;
        roundKeys[round] = permute(combined, PC2, 48);
    }
}

string sBoxSubstitution(string input) {
    string output = "";
    for (int i = 0; i < 8; i++) {
        string sixBits = input.substr(i * 6, 6);
        string rowBits = "";
        rowBits += sixBits[0];
        rowBits += sixBits[5];
        int row = binaryToDecimal(rowBits);
        string colBits = sixBits.substr(1, 4);
        int col = binaryToDecimal(colBits);
        int sBoxValue = S[i][row][col];
        output += decimalToBinary(sBoxValue);
    }
    return output;
}

string manglerFunction(string right, string roundKey) {
    string expanded = permute(right, E, 48);
    string xored = xorStrings(expanded, roundKey);
    string substituted = sBoxSubstitution(xored);
    string permuted = permute(substituted, P, 32);
    return permuted;
}

string desEncrypt(string plaintext, string key) {
    string roundKeys[16];
    generateRoundKeys(key, roundKeys);

    string permuted = permute(plaintext, IP, 64);
    string left = permuted.substr(0, 32);
    string right = permuted.substr(32, 32);

    for (int round = 0; round < 16; round++) {
        string tempRight = right;
        right = xorStrings(left, manglerFunction(right, roundKeys[round]));
        left = tempRight;
    }

    string combined = right + left;
    string ciphertext = permute(combined, FP, 64);
    return ciphertext;
}

string desDecrypt(string ciphertext, string key) {
    string roundKeys[16];
    generateRoundKeys(key, roundKeys);

    string permuted = permute(ciphertext, IP, 64);
    string left = permuted.substr(0, 32);
    string right = permuted.substr(32, 32);

    for (int round = 0; round < 16; round++) {
        string tempRight = right;
        right = xorStrings(left, manglerFunction(right, roundKeys[15 - round]));
        left = tempRight;
    }

    string combined = right + left;
    string plaintext = permute(combined, FP, 64);
    return plaintext;
}

//string convretion function

string stringToBinary(string input) {
    string binary = "";
    for (char c : input) {
        binary += bitset<8>(c).to_string();
    }
    return binary;
}

string binaryToString(string binary) {
    string output = "";
    for (int i = 0; i < binary.length(); i += 8) {
        string byte = binary.substr(i, 8);
        int asciiValue = binaryToDecimal(byte);
        output += char(asciiValue);
    }
    return output;
}

string padString(string input) {
    int padding = 8 - (input.length() % 8);
    if (padding != 8) {
        for (int i = 0; i < padding; i++) {
            input += (char)padding; // Store padding count
        }
    }
    return input;
}

string removePadding(string input) {
    if (input.empty()) return input;
    char lastChar = input[input.length() - 1];
    int paddingCount = (int)lastChar;
    if (paddingCount > 0 && paddingCount <= 8) {
        return input.substr(0, input.length() - paddingCount);
    }
    return input;
}

//encryption and decryption function------------

// Encrypt a string of any length
string encryptString(string plaintext, string key) {
    // Pad key to 8 bytes if needed
    while (key.length() < 8) key += "0";
    key = key.substr(0, 8); // Take first 8 characters

    // Pad plaintext to multiple of 8 bytes
    plaintext = padString(plaintext);

    string binaryKey = stringToBinary(key);
    string encrypted = "";

    // Encrypt each 8-byte block
    for (int i = 0; i < plaintext.length(); i += 8) {
        string block = plaintext.substr(i, 8);
        string binaryBlock = stringToBinary(block);
        string encryptedBlock = desEncrypt(binaryBlock, binaryKey);
        encrypted += binaryToString(encryptedBlock);
    }

    return encrypted;
}

// Decrypt a string
string decryptString(string ciphertext, string key) {
    // Pad key to 8 bytes if needed
    while (key.length() < 8) key += "0";
    key = key.substr(0, 8);

    string binaryKey = stringToBinary(key);
    string decrypted = "";

    // Decrypt each 8-byte block
    for (int i = 0; i < ciphertext.length(); i += 8) {
        string block = ciphertext.substr(i, 8);
        string binaryBlock = stringToBinary(block);
        string decryptedBlock = desDecrypt(binaryBlock, binaryKey);
        decrypted += binaryToString(decryptedBlock);
    }

    return removePadding(decrypted);
}

#endif

