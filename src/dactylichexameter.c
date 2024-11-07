
// Copyright (c) 2023 gstaaij
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include "dactylichexameter.h"

// A dynamic array of String Views, to be able to easily split/chop them
typedef struct {
    Nob_String_View* items;
    size_t count;
    size_t capacity;
} ChoppedStringView;

// Split a string into a ChoppedStringView: a dynamic array of String Views
ChoppedStringView chopString(const char* string, char delim) {
    ChoppedStringView result = {0};
    Nob_String_View sv = nob_sv_from_cstr(string);
    Nob_String_View item;
    while (sv.count > 0) {
        item = nob_sv_chop_by_delim(&sv, delim);
        nob_da_append(&result, item);
    }
    return result;
}

// Strip out all of the empty items in the ChoppedStringView and trim it at the same time
ChoppedStringView trimChoppedString(const ChoppedStringView csv) {
    ChoppedStringView result = {0};
    for (size_t i = 0; i < csv.count; ++i) {
        Nob_String_View svTrimmed = nob_sv_trim(csv.items[i]);
        if (svTrimmed.count > 0) nob_da_append(&result, svTrimmed);
    }
    return result;
}

// Convert a string to lowercase
char* strLower(const char* string) {
    Nob_String_Builder lower = {0};
    for (size_t i = 0; string[i]; i++) {
        nob_da_append(&lower, tolower(string[i]));
    }
    nob_sb_append_null(&lower);
    return lower.items;
}

// A list of all vowels in Latin
static char vowels[] = {
    'a',
    'e',
    'i',
    'o',
    'u',
    'y',
};

// Check if a character in a string is a vowel
bool isVowel(const char* string, const size_t index) {
    char chr = string[index];
    // A 'u' after a 'q' is pronounced as a 'w', not counted as a vowel
    if (index != 0 && chr == 'u' && string[index - 1] == 'q') return false;

    for (size_t i = 0; i < NOB_ARRAY_LEN(vowels); ++i) {
        if (chr == vowels[i]) return true;
    }
    return false;
}


// A dynamic array of integers
typedef struct {
    int* items;
    size_t count;
    size_t capacity;
} DynamicArrayInt;

// Check if a DynamicArrayInt contains a certain value
bool daIntContains(DynamicArrayInt daInt, int query) {
    for (size_t i = 0; i < daInt.count; ++i) {
        if (daInt.items[i] == query) return true;
    }
    return false;
}


// A list of all diphthongs in Latin
static char* diphthongs[] = {
    "ae",
    "au",
    "ei",
    "eu",
    "oe",
};

static char* diphthongExceptionWords[] = {
    "ei",
    "eis",
    "mei",
    "meis",
};

static int diphthongExceptionWordStartIndices[] = {
    0,
    0,
    1,
    1,
};

// Check if two characters are a diphthong
bool isDiphthong(const char* string, const size_t index, const DynamicArrayInt spacePositions) {
    size_t stringLen = strlen(string);
    for (size_t j = 0; j < NOB_ARRAY_LEN(diphthongs); ++j) {
        if (string[index] == diphthongs[j][0] && string[index + 1] == diphthongs[j][1]) {
            for (size_t k = 0; k < NOB_ARRAY_LEN(diphthongExceptionWords); ++k) {
                const size_t diphthongExceptionLen = strlen(diphthongExceptionWords[k]);
                const size_t exceptionStartIndex = index - diphthongExceptionWordStartIndices[k];
                if (stringLen - exceptionStartIndex < diphthongExceptionLen) continue;
                bool isWord = true;
                for (size_t l = 0; l < diphthongExceptionLen; ++l) {
                    if (string[exceptionStartIndex + l] != diphthongExceptionWords[k][l]) {
                        isWord = false;
                        break;
                    }
                }
                if (isWord) {
                    if (!(daIntContains(spacePositions, exceptionStartIndex) && daIntContains(spacePositions, exceptionStartIndex + diphthongExceptionLen)))
                        continue;
                    return false;
                }
            }
            return true;
        }
    }
    return false;
}



char* dhStripLine(const char* string) {
    Nob_String_Builder sb = {0};
    for (size_t i = 0; i < strlen(string); ++i) {
        if (isalpha(string[i]) && !isspace(string[i])) {
            nob_da_append(&sb, string[i]);
        }
    }
    nob_sb_append_null(&sb);
    return sb.items;
}

bool dhElision(const char* line, Nob_String_Builder* sb) {
    bool result = true;
    // Clear the result string builder
    sb->count = 0;
    // Chop the line by spaces and trim it
    ChoppedStringView temp = chopString(strLower(line), ' ');
    ChoppedStringView choppedLine = trimChoppedString(temp);

    // If the line contains 0 words, fail
    if (choppedLine.count == 0) {
        nob_log(NOB_ERROR, "Empty verse");
        nob_return_defer(false);
    }

    // If there's only one word, you can just return, elision can't happen on just one word
    if (choppedLine.count < 2) {
        nob_sb_append_cstr(sb, line);
        nob_return_defer(true);
    }

    DynamicArrayInt daIntEmpty = {0};

    // Go through every word except the last one
    for (size_t i = 0; i < choppedLine.count - 1; ++i) {
        Nob_String_View word = choppedLine.items[i];
        // Check if the words ends with a vowel or an 'm'
        bool endsWithVowel = word.data[word.count - 1] == 'm' || isVowel(word.data, word.count - 1);
        if (!endsWithVowel) {
            // If not, just add the word to the string buffer and continue
            for (size_t i = 0; i < word.count; ++i) {
                if (word.data[i] != 'h')
                    nob_da_append(sb, word.data[i]);
            }
            nob_da_append(sb, ' ');
            continue;
        }

        Nob_String_View nextWord = choppedLine.items[i + 1];
        // Check if the next word begins with a vowel or an 'h'
        bool beginsWithVowel = nextWord.data[0] == 'h' || isVowel(nextWord.data, 0);
        if (beginsWithVowel) {
            // If so, perform elision

            size_t size = word.count;
            // Remove the 'm' from the word
            if (word.data[size - 1] == 'm') --size;
            // Remove an extra vowel to account for the diphthong
            if (isDiphthong(word.data, size - 2, daIntEmpty)) --size;
            // Remove the vowel
            --size;

            // Add the truncated word to the string builder
            for (size_t i = 0; i < size; ++i) {
                if (word.data[i] != 'h')
                    nob_da_append(sb, word.data[i]);
            }
            // Add extra spaces to the string builder to keep it the same length
            for (size_t _ = 1; _ < word.count - size; ++_) nob_da_append(sb, ' ');
        } else {
            // If not, just add the line to the string builder
            nob_sb_append_buf(sb, word.data, word.count);
        }
        // Add a space to seperate the words
        nob_da_append(sb, ' ');
    }
    // Add the last word to the string builder
    Nob_String_View word = choppedLine.items[choppedLine.count - 1];
    nob_sb_append_buf(sb, word.data, word.count);

defer:
    // Some cleanup that may or may not actually be neccessary
    nob_da_free(temp);
    nob_da_free(choppedLine);
    return result;
}

// Define the minimum and maximum amount of syllables/dactyli
// Lowest amount of dactyli: _ _   _ _   _ _   _ _   _ uu  _ _ (13 dactyli)
// Lowest amount of dactyli: _ uu  _ uu  _ uu  _ uu  _ uu  _ _ (17 dactyli)
#define MIN_SYLLABLES 13
#define MAX_SYLLABLES 17

// Assign numbers to the syllables, and optionally log a warning if too few were assigned
size_t numberMetra(char* syllableNumbers, char* syllableLengths, size_t amountOfSyllables, bool shouldWarn) {
    // Clear the syllableNumbers array (it should always have a length of 17, so no buffer overflows should happen)
    memset(syllableNumbers, ' ', MAX_SYLLABLES);
    size_t syllableNumberIndex = 0;
    size_t addedNumbers = 0;
    for (size_t i = 1; i <= 6; ++i) {
        // Set the number character (ASCII 0x3.)
        syllableNumbers[syllableNumberIndex] = (char) i + 0x30;
        ++addedNumbers;
        // Skip the appropriate amount of syllables
        if (syllableNumberIndex < amountOfSyllables - 1 && syllableLengths[syllableNumberIndex + 1] == 'u')
            syllableNumberIndex += 3;
        else if (syllableNumberIndex < amountOfSyllables - 1 && syllableLengths[syllableNumberIndex + 1] == '_')
            syllableNumberIndex += 2;
        else
            break;
    }

    // If the numbering didn't comlete, try from the other side too
    if (addedNumbers < 6) {
        syllableNumberIndex = amountOfSyllables - 2;
        for (size_t i = 6; i >= 1; --i) {
            // Set the number character (ASCII 0x3.)
            syllableNumbers[syllableNumberIndex] = (char) i + 0x30;
            ++addedNumbers;
            // Skip the appropriate amount of syllables
            if (syllableNumberIndex > 0 && syllableLengths[syllableNumberIndex - 1] == 'u')
                syllableNumberIndex -= 3;
            else if (syllableNumberIndex > 0 && syllableLengths[syllableNumberIndex - 1] == '_')
                syllableNumberIndex -= 2;
            else
                break;
        }
    }
    
    // Log a warning if you're told to do so
    if (shouldWarn && addedNumbers < 6) {
        nob_log(NOB_WARNING, "Couldn't completely number the metra due to some missing dactyli. You've either");
        nob_log(NOB_WARNING, "entered an invalid verse or there are rules this program doesn't account for (yet)");
    }

    // Return the amount of numbers that were filled in
    return addedNumbers;
}

void makeMetraStartLong(char* syllableNumbers, char* syllableLengths, size_t amountOfSyllables, bool shouldWarn) {
    numberMetra(syllableNumbers, syllableLengths, amountOfSyllables, shouldWarn);

    // Go through the syllables and make the ones that have a number assigned to them long, because they're at the start of a metrum
    for (size_t i = 0; i < amountOfSyllables; ++i) {
        if (syllableNumbers[i] == ' ' || syllableLengths[i] != '?') continue;
        syllableLengths[i] = '_';
    }
}

bool dhScan(const char* unstrippedLine, Nob_String_Builder* sbNumbers, Nob_String_Builder* sbScan, Nob_String_Builder* sbStrippedLine) {
    // Clear all the string builders
    sbNumbers->count = 0;
    sbScan->count = 0;
    sbStrippedLine->count = 0;

    // Strip the line and make it lowercase
    const char* line = strLower(dhStripLine(unstrippedLine));


    // Detect where spaces or special characters were in the original unstripped line
    DynamicArrayInt spacePositions = {0};
    size_t unstrippedLen = strlen(unstrippedLine);
    size_t strippedLineIndex = 0;
    for (size_t i = 0; i < unstrippedLen; ++i) {
        char chr = unstrippedLine[i];
        if (!isalpha(chr) || isspace(chr)) {
            nob_da_append(&spacePositions, strippedLineIndex);
            // Skip over all the whitespace
           while (i < unstrippedLen && (!isalpha(unstrippedLine[i]) || isspace(unstrippedLine[i]))) ++i;
           --i;
        } else {
            ++strippedLineIndex;
        }
    }


    size_t len = strlen(line);
    size_t amountOfSyllables = 0;
    // Create a list of syllable positions and initialise it at -1
    size_t syllablePositions[MAX_SYLLABLES] = {0};
    memset(syllablePositions, -1, MAX_SYLLABLES*sizeof(size_t));
    // Count the syllables (dactyli in Latin) and record their positions in the line
    for (size_t i = 0; i < len; ++i) {
        if (isVowel(line, i)) {
            syllablePositions[amountOfSyllables] = i;
            ++amountOfSyllables;

            // Check for too many syllables
            if (amountOfSyllables > MAX_SYLLABLES) {
                nob_log(NOB_ERROR, "Too many dactyli: %lld", amountOfSyllables);
                return false;
            }

            // Check for diphthongs and skip the next vowel if one is found
            if (i != len - 1 && isVowel(line, i + 1) && isDiphthong(line, i, spacePositions)) {
                i++;
            }
        }
    }

    // Check for too few syllables
    if (amountOfSyllables < MIN_SYLLABLES) {
        nob_log(NOB_ERROR, "Too few dactyli: %lld", amountOfSyllables);
        return false;
    }

    // Create a list of the characters to indicate the pronounciation of syllables. Initialise it with question marks
    char syllableLengths[MAX_SYLLABLES] = {0};
    memset(syllableLengths, '?', MAX_SYLLABLES);
    // Use (sometimes way too complicated) rules to determine lengths of syllables
    for (size_t i = 0; i < amountOfSyllables; ++i) {
        size_t lineIndex = syllablePositions[i];
        // Check for a diphthong
        if (lineIndex < len - 1 && isVowel(line, lineIndex + 1)) {
            if (isDiphthong(line, lineIndex, spacePositions)) {
                syllableLengths[i] = '_';
                continue;
            }
            syllableLengths[i] = 'u';
            continue;
        }

        // Check if there's two consonants after this vowel, and mark it as long if so; 'x' counts as 2 consonants; 'qu' counts as 1 consonant
        if (
            (lineIndex < len - 1 && line[lineIndex + 1] == 'x') ||
            (lineIndex < len - 2 && !isVowel(line, lineIndex + 1) && !isVowel(line, lineIndex + 2) && !(line[lineIndex + 1] == 'q' && line[lineIndex + 2] == 'u')) || // stupid 'qu'
            (lineIndex < len - 3 && line[lineIndex + 1] == 'q' && line[lineIndex + 2] == 'u' && !isVowel(line, lineIndex + 3))
        ) {
            syllableLengths[i] = '_';
            continue;
        }

        // The things that are always true
        // TODO: what to do in the rare occasions that the 5th metrum is _ _ instead of _ u u
        if (i == 0 || i == amountOfSyllables - 5 || i == amountOfSyllables - 2 || i == amountOfSyllables - 1) {
            syllableLengths[i] = '_';
            continue;
        }
        if (i == amountOfSyllables - 4 || i == amountOfSyllables - 3) {
            syllableLengths[i] = 'u';
            continue;
        }
    }

    // Create a list of the characters to indicate where a new metrum begins and the how-manieth it is
    char syllableNumbers[MAX_SYLLABLES] = {0};

    // Check for patterns that force a particular length to be used (thrice, just in case)
    for (size_t _n = 0; _n < 3; ++_n) {
        // "Fix" the syllables by numbering the metra and putting a '_' at the first part of every metrum
        makeMetraStartLong(syllableNumbers, syllableLengths, amountOfSyllables, false);

        // Check for patterns that force a specific length at a specific place
        for (size_t i = 1; i < amountOfSyllables - 1; ++i) {
            if (syllableLengths[i] != '?') continue;

            // Patterns that force a long syllable
            if (
                (syllableLengths[i - 1] == '_' && syllableLengths[i + 1] == '_') ||                             // _ ? _
                (i >= 2 && syllableLengths[i - 2] == 'u' && syllableLengths[i - 1] == 'u') ||                   // u u ?
                (i < amountOfSyllables - 2 && syllableLengths[i + 1] == 'u' && syllableLengths[i + 2] == 'u')   // ? u u
            ) {
                syllableLengths[i] = '_';
                continue;
            }

            // Patterns that force a short one
            if (
                (syllableLengths[i - 1] == 'u' && syllableLengths[i + 1] == '_') ||                             // u ? _
                (i >= 2 && syllableLengths[i - 2] == '_' && syllableLengths[i - 1] == 'u') ||                   // _ u ?
                (i < amountOfSyllables - 2 && syllableLengths[i + 1] == 'u' && syllableLengths[i + 2] == '_')   // ? u _
            ) {
                syllableLengths[i] = 'u';
                continue;
            }

            // A way too complicated pattern to force a short one (it's also probably redundant because of the check after these for loops)
            if (
                // n     n+1
                // _ ? ? _
                (i < amountOfSyllables - 2 && syllableLengths[i - 1] == '_' && syllableLengths[i + 2] == '_' &&
                    syllableNumbers[i - 1] + 1 == syllableNumbers[i + 2])
            ) {
                syllableLengths[i] = 'u';
                continue;
            }
        }
    }


    bool shouldAllBeShort = false;


    // Count the amount of long syllables
    size_t amountOfLongSyllables = 0;
    for (size_t i = 0; i < amountOfSyllables; ++i) {
        if (syllableLengths[i] == '_') ++amountOfLongSyllables;
    }

    size_t amountOfShortSyllablesThereShouldBe = (amountOfSyllables - 12) * 2;
    size_t amountOfLongSyllablesThereShouldBe = amountOfSyllables - amountOfShortSyllablesThereShouldBe;

    if (amountOfLongSyllables == amountOfLongSyllablesThereShouldBe) {
        shouldAllBeShort = true;
    }


    // Number the metra to prepare for the next part
    size_t amountOfNumberedMetra = numberMetra(syllableNumbers, syllableLengths, amountOfSyllables, false);

    // Count the amount of unknown lengths
    size_t amountOfUnknownLengths = 0;
    for (size_t i = 0; i < amountOfSyllables; ++i) {
        if (syllableLengths[i] == '?') ++amountOfUnknownLengths;
    }

    // If the amount of numbered metra and the amount of unknown lengths match up in a certain way, they should all be short
    // Here is an example:
    //  1                3          4     5      6
    //  _   ? ?  _   ? ? _    _     _  _  _ u u  _  _
    //
    // 5 metra were numbered, so the weird calculation (6 - 5) * 2 + 2 = 4
    // The amount of unknown lengths is also 4, so this means the unknown lengths can't be long,
    // because there would be too many metra
    if ((6 - amountOfNumberedMetra) * 2 + 2 == amountOfUnknownLengths) {
        shouldAllBeShort = true;
    }
    

    if (shouldAllBeShort) {
        for (size_t i = 0; i < amountOfSyllables; ++i) {
            if (syllableLengths[i] == '?') syllableLengths[i] = 'u';
        }
    }

    // Put the syllable numbers in the correct spots
    numberMetra(syllableNumbers, syllableLengths, amountOfSyllables, true);

    
    // Add some spaces back into the line to make it more readable, and fill the other string builders
    size_t syllableIndex = 0;
    for (size_t i = 0; i < len; ++i) {
        if (daIntContains(spacePositions, i)) {
            nob_da_append(sbNumbers, ' ');
            nob_da_append(sbScan, ' ');
            nob_da_append(sbStrippedLine, ' ');
        }
        nob_da_append(sbStrippedLine, line[i]);
        if (i == syllablePositions[syllableIndex]) {
            nob_da_append(sbNumbers, syllableNumbers[syllableIndex]);
            nob_da_append(sbScan, syllableLengths[syllableIndex]);
            ++syllableIndex;
            continue;
        }
        nob_da_append(sbNumbers, ' ');
        nob_da_append(sbScan, ' ');
    }

    // Success!
    return true;
}
