/// Json-cpp amalgamated source (http://jsoncpp.sourceforge.net/).
/// It is intended to be used with #include "json/json.h"

// //////////////////////////////////////////////////////////////////////
// Beginning of content of file: LICENSE
// //////////////////////////////////////////////////////////////////////

/*
The JsonCpp library's source code, including accompanying documentation,
tests and demonstration applications, are licensed under the following
conditions...

Baptiste Lepilleur and The JsonCpp Authors explicitly disclaim copyright in all
jurisdictions which recognize such a disclaimer. In such jurisdictions,
this software is released into the Public Domain.

In jurisdictions which do not recognize Public Domain property (e.g. Germany as of
2010), this software is Copyright (c) 2007-2010 by Baptiste Lepilleur and
The JsonCpp Authors, and is released under the terms of the MIT License (see below).

In jurisdictions which recognize Public Domain property, the user of this
software may choose to accept it either as 1) Public Domain, 2) under the
conditions of the MIT License (see below), or 3) under the terms of dual
Public Domain/MIT License conditions described here, as they choose.

The MIT License is about as close to Public Domain as a license can get, and is
described in clear, concise terms at:

   http://en.wikipedia.org/wiki/MIT_License

The full text of the MIT License follows:

========================================================================
Copyright (c) 2007-2010 Baptiste Lepilleur and The JsonCpp Authors

Permission is hereby granted, free of charge, to any person
obtaining a copy of this software and associated documentation
files (the "Software"), to deal in the Software without
restriction, including without limitation the rights to use, copy,
modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be
included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
========================================================================
(END LICENSE TEXT)

The MIT license is compatible with both the GPL and commercial
software, affording one all of the rights of Public Domain with the
minor nuisance of being required to keep the above copyright notice
and license text in the source code. Note also that by accepting the
Public Domain "license" you can re-license your copy using whatever
license you like.

*/

// //////////////////////////////////////////////////////////////////////
// End of content of file: LICENSE
// //////////////////////////////////////////////////////////////////////






#include "json/json.h"

#ifndef JSON_IS_AMALGAMATION
#error "Compile with -I PATH_TO_JSON_DIRECTORY"
#endif


// //////////////////////////////////////////////////////////////////////
// Beginning of content of file: src/lib_json/json_tool.h
// //////////////////////////////////////////////////////////////////////

// Copyright 2007-2010 Baptiste Lepilleur and The JsonCpp Authors
// Distributed under MIT license, or public domain if desired and
// recognized in your jurisdiction.
// See file LICENSE for detail or copy at http://jsoncpp.sourceforge.net/LICENSE

#ifndef LIB_JSONCPP_JSON_TOOL_H_INCLUDED
#define LIB_JSONCPP_JSON_TOOL_H_INCLUDED

#if !defined(JSON_IS_AMALGAMATION)
#include <json/config.h>
#endif

// Also support old flag NO_LOCALE_SUPPORT
#ifdef NO_LOCALE_SUPPORT
#define JSONCPP_NO_LOCALE_SUPPORT
#endif

#ifndef JSONCPP_NO_LOCALE_SUPPORT
#include <clocale>
#endif

/* This header provides common string manipulation support, such as UTF-8,
 * portable conversion from/to string...
 *
 * It is an internal header that must not be exposed.
 */

namespace Json {
    static inline char getDecimalPoint() {
#ifdef JSONCPP_NO_LOCALE_SUPPORT
        return '\0';
#else
        struct lconv* lc = localeconv();
        return lc ? *(lc->decimal_point) : '\0';
#endif
    }

    /// Converts a unicode code-point to UTF-8.
    static inline String codePointToUTF8(unsigned int cp) {
        String result;

        // based on description from http://en.wikipedia.org/wiki/UTF-8

        if (cp <= 0x7f) {
            result.resize(1);
            result[0] = static_cast<char>(cp);
        } else if (cp <= 0x7FF) {
            result.resize(2);
            result[1] = static_cast<char>(0x80 | (0x3f & cp));
            result[0] = static_cast<char>(0xC0 | (0x1f & (cp >> 6)));
        } else if (cp <= 0xFFFF) {
            result.resize(3);
            result[2] = static_cast<char>(0x80 | (0x3f & cp));
            result[1] = static_cast<char>(0x80 | (0x3f & (cp >> 6)));
            result[0] = static_cast<char>(0xE0 | (0xf & (cp >> 12)));
        } else if (cp <= 0x10FFFF) {
            result.resize(4);
            result[3] = static_cast<char>(0x80 | (0x3f & cp));
            result[2] = static_cast<char>(0x80 | (0x3f & (cp >> 6)));
            result[1] = static_cast<char>(0x80 | (0x3f & (cp >> 12)));
            result[0] = static_cast<char>(0xF0 | (0x7 & (cp >> 18)));
        }

        return result;
    }

    enum {
        /// Constant that specify the size of the buffer that must be passed to
        /// uintToString.
        uintToStringBufferSize = 3 * sizeof(LargestUInt) + 1
    };

    // Defines a char buffer for use with uintToString().
    using UIntToStringBuffer = char[uintToStringBufferSize];

    /** Converts an unsigned integer to string.
     * @param value Unsigned integer to convert to string
     * @param current Input/Output string buffer.
     *        Must have at least uintToStringBufferSize chars free.
     */
    static inline void uintToString(LargestUInt value, char*& current) {
        *--current = 0;
        do {
            *--current = static_cast<char>(value % 10U + static_cast<unsigned>('0'));
            value /= 10;
        } while (value != 0);
    }

    /** Change ',' to '.' everywhere in buffer.
     *
     * We had a sophisticated way, but it did not work in WinCE.
     * @see https://github.com/open-source-parsers/jsoncpp/pull/9
     */
    template <typename Iter> Iter fixNumericLocale(Iter begin, Iter end) {
        for (; begin != end; ++begin) {
            if (*begin == ',') {
                *begin = '.';
            }
        }
        return begin;
    }

    template <typename Iter> void fixNumericLocaleInput(Iter begin, Iter end) {
        char decimalPoint = getDecimalPoint();
        if (decimalPoint == '\0' || decimalPoint == '.') {
            return;
        }
        for (; begin != end; ++begin) {
            if (*begin == '.') {
                *begin = decimalPoint;
            }
        }
    }

    /**
     * Return iterator that would be the new end of the range [begin,end), if we
     * were to delete zeros in the end of string, but not the last zero before '.'.
     */
    template <typename Iter> Iter fixZerosInTheEnd(Iter begin, Iter end) {
        for (; begin != end; --end) {
            if (*(end - 1) != '0') {
                return end;
            }
            // Don't delete the last zero before the decimal point.
            if (begin != (end - 1) && *(end - 2) == '.') {
                return end;
            }
        }
        return end;
    }

} // namespace Json

#endif // LIB_JSONCPP_JSON_TOOL_H_INCLUDED

// //////////////////////////////////////////////////////////////////////
// End of content of file: src/lib_json/json_tool.h
// //////////////////////////////////////////////////////////////////////






// //////////////////////////////////////////////////////////////////////
// Beginning of content of file: src/lib_json/json_reader.cpp
// //////////////////////////////////////////////////////////////////////

// Copyright 2007-2011 Baptiste Lepilleur and The JsonCpp Authors
// Copyright (C) 2016 InfoTeCS JSC. All rights reserved.
// Distributed under MIT license, or public domain if desired and
// recognized in your jurisdiction.
// See file LICENSE for detail or copy at http://jsoncpp.sourceforge.net/LICENSE

#if !defined(JSON_IS_AMALGAMATION)
#include "json_tool.h"
#include <json/assertions.h>
#include <json/reader.h>
#include <json/value.h>
#endif // if !defined(JSON_IS_AMALGAMATION)
#include <cassert>
#include <cstring>
#include <iostream>
#include <istream>
#include <limits>
#include <memory>
#include <set>
#include <sstream>
#include <utility>

#include <cstdio>
#if __cplusplus >= 201103L

#if !defined(sscanf)
#define sscanf std::sscanf
#endif

#endif //__cplusplus

#if defined(_MSC_VER)
#if !defined(_CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES)
#define _CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES 1
#endif //_CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES
#endif //_MSC_VER

#if defined(_MSC_VER)
// Disable warning about strdup being deprecated.
#pragma warning(disable : 4996)
#endif

// Define JSONCPP_DEPRECATED_STACK_LIMIT as an appropriate integer at compile
// time to change the stack limit
#if !defined(JSONCPP_DEPRECATED_STACK_LIMIT)
#define JSONCPP_DEPRECATED_STACK_LIMIT 1000
#endif

static size_t const stackLimit_g =
JSONCPP_DEPRECATED_STACK_LIMIT; // see readValue()

namespace Json {

#if __cplusplus >= 201103L || (defined(_CPPLIB_VER) && _CPPLIB_VER >= 520)
    using CharReaderPtr = std::unique_ptr<CharReader>;
#else
    using CharReaderPtr = std::auto_ptr<CharReader>;
#endif

    // Implementation of class Features
    // ////////////////////////////////

    Features::Features() = default;

    Features Features::all() { return {}; }

    Features Features::strictMode() {
        Features features;
        features.allowComments_ = false;
        features.allowTrailingCommas_ = false;
        features.strictRoot_ = true;
        features.allowDroppedNullPlaceholders_ = false;
        features.allowNumericKeys_ = false;
        return features;
    }

    // Implementation of class Reader
    // ////////////////////////////////

    bool Reader::containsNewLine(Reader::Location begin, Reader::Location end) {
        for (; begin < end; ++begin)
            if (*begin == '\n' || *begin == '\r')
                return true;
        return false;
    }

    // Class Reader
    // //////////////////////////////////////////////////////////////////

    Reader::Reader() : features_(Features::all()) {}

    Reader::Reader(const Features& features) : features_(features) {}

    bool Reader::parse(const std::string& document, Value& root,
        bool collectComments) {
        document_.assign(document.begin(), document.end());
        const char* begin = document_.c_str();
        const char* end = begin + document_.length();
        return parse(begin, end, root, collectComments);
    }

    bool Reader::parse(std::istream& is, Value& root, bool collectComments) {
        // std::istream_iterator<char> begin(is);
        // std::istream_iterator<char> end;
        // Those would allow streamed input from a file, if parse() were a
        // template function.

        // Since String is reference-counted, this at least does not
        // create an extra copy.
        String doc;
        std::getline(is, doc, static_cast<char> EOF);
        return parse(doc.data(), doc.data() + doc.size(), root, collectComments);
    }

    bool Reader::parse(const char* beginDoc, const char* endDoc, Value& root,
        bool collectComments) {
        if (!features_.allowComments_) {
            collectComments = false;
        }

        begin_ = beginDoc;
        end_ = endDoc;
        collectComments_ = collectComments;
        current_ = begin_;
        lastValueEnd_ = nullptr;
        lastValue_ = nullptr;
        commentsBefore_.clear();
        errors_.clear();
        while (!nodes_.empty())
            nodes_.pop();
        nodes_.push(&root);

        bool successful = readValue();
        Token token;
        skipCommentTokens(token);
        if (collectComments_ && !commentsBefore_.empty())
            root.setComment(commentsBefore_, commentAfter);
        if (features_.strictRoot_) {
            if (!root.isArray() && !root.isObject()) {
                // Set error location to start of doc, ideally should be first token found
                // in doc
                token.type_ = tokenError;
                token.start_ = beginDoc;
                token.end_ = endDoc;
                addError(
                    "A valid JSON document must be either an array or an object value.",
                    token);
                return false;
            }
        }
        return successful;
    }

    bool Reader::readValue() {
        // readValue() may call itself only if it calls readObject() or ReadArray().
        // These methods execute nodes_.push() just before and nodes_.pop)() just
        // after calling readValue(). parse() executes one nodes_.push(), so > instead
        // of >=.
        if (nodes_.size() > stackLimit_g)
            throwRuntimeError("Exceeded stackLimit in readValue().");

        Token token;
        skipCommentTokens(token);
        bool successful = true;

        if (collectComments_ && !commentsBefore_.empty()) {
            currentValue().setComment(commentsBefore_, commentBefore);
            commentsBefore_.clear();
        }

        switch (token.type_) {
        case tokenObjectBegin:
            successful = readObject(token);
            currentValue().setOffsetLimit(current_ - begin_);
            break;
        case tokenArrayBegin:
            successful = readArray(token);
            currentValue().setOffsetLimit(current_ - begin_);
            break;
        case tokenNumber:
            successful = decodeNumber(token);
            break;
        case tokenString:
            successful = decodeString(token);
            break;
        case tokenTrue: {
            Value v(true);
            currentValue().swapPayload(v);
            currentValue().setOffsetStart(token.start_ - begin_);
            currentValue().setOffsetLimit(token.end_ - begin_);
        } break;
        case tokenFalse: {
            Value v(false);
            currentValue().swapPayload(v);
            currentValue().setOffsetStart(token.start_ - begin_);
            currentValue().setOffsetLimit(token.end_ - begin_);
        } break;
        case tokenNull: {
            Value v;
            currentValue().swapPayload(v);
            currentValue().setOffsetStart(token.start_ - begin_);
            currentValue().setOffsetLimit(token.end_ - begin_);
        } break;
        case tokenArraySeparator:
        case tokenObjectEnd:
        case tokenArrayEnd:
            if (features_.allowDroppedNullPlaceholders_) {
                // "Un-read" the current token and mark the current value as a null
                // token.
                current_--;
                Value v;
                currentValue().swapPayload(v);
                currentValue().setOffsetStart(current_ - begin_ - 1);
                currentValue().setOffsetLimit(current_ - begin_);
                break;
            } // Else, fall through...
        default:
            currentValue().setOffsetStart(token.start_ - begin_);
            currentValue().setOffsetLimit(token.end_ - begin_);
            return addError("Syntax error: value, object or array expected.", token);
        }

        if (collectComments_) {
            lastValueEnd_ = current_;
            lastValue_ = &currentValue();
        }

        return successful;
    }

    void Reader::skipCommentTokens(Token& token) {
        if (features_.allowComments_) {
            do {
                readToken(token);
            } while (token.type_ == tokenComment);
        } else {
            readToken(token);
        }
    }

    bool Reader::readToken(Token& token) {
        skipSpaces();
        token.start_ = current_;
        Char c = getNextChar();
        bool ok = true;
        switch (c) {
        case '{':
            token.type_ = tokenObjectBegin;
            break;
        case '}':
            token.type_ = tokenObjectEnd;
            break;
        case '[':
            token.type_ = tokenArrayBegin;
            break;
        case ']':
            token.type_ = tokenArrayEnd;
            break;
        case '"':
            token.type_ = tokenString;
            ok = readString();
            break;
        case '/':
            token.type_ = tokenComment;
            ok = readComment();
            break;
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
        case '-':
            token.type_ = tokenNumber;
            readNumber();
            break;
        case 't':
            token.type_ = tokenTrue;
            ok = match("rue", 3);
            break;
        case 'f':
            token.type_ = tokenFalse;
            ok = match("alse", 4);
            break;
        case 'n':
            token.type_ = tokenNull;
            ok = match("ull", 3);
            break;
        case ',':
            token.type_ = tokenArraySeparator;
            break;
        case ':':
            token.type_ = tokenMemberSeparator;
            break;
        case 0:
            token.type_ = tokenEndOfStream;
            break;
        default:
            ok = false;
            break;
        }
        if (!ok)
            token.type_ = tokenError;
        token.end_ = current_;
        return ok;
    }

    void Reader::skipSpaces() {
        while (current_ != end_) {
            Char c = *current_;
            if (c == ' ' || c == '\t' || c == '\r' || c == '\n')
                ++current_;
            else
                break;
        }
    }

    bool Reader::match(const Char* pattern, int patternLength) {
        if (end_ - current_ < patternLength)
            return false;
        int index = patternLength;
        while (index--)
            if (current_[index] != pattern[index])
                return false;
        current_ += patternLength;
        return true;
    }

    bool Reader::readComment() {
        Location commentBegin = current_ - 1;
        Char c = getNextChar();
        bool successful = false;
        if (c == '*')
            successful = readCStyleComment();
        else if (c == '/')
            successful = readCppStyleComment();
        if (!successful)
            return false;

        if (collectComments_) {
            CommentPlacement placement = commentBefore;
            if (lastValueEnd_ && !containsNewLine(lastValueEnd_, commentBegin)) {
                if (c != '*' || !containsNewLine(commentBegin, current_))
                    placement = commentAfterOnSameLine;
            }

            addComment(commentBegin, current_, placement);
        }
        return true;
    }

    String Reader::normalizeEOL(Reader::Location begin, Reader::Location end) {
        String normalized;
        normalized.reserve(static_cast<size_t>(end - begin));
        Reader::Location current = begin;
        while (current != end) {
            char c = *current++;
            if (c == '\r') {
                if (current != end && *current == '\n')
                    // convert dos EOL
                    ++current;
                // convert Mac EOL
                normalized += '\n';
            } else {
                normalized += c;
            }
        }
        return normalized;
    }

    void Reader::addComment(Location begin, Location end,
        CommentPlacement placement) {
        assert(collectComments_);
        const String& normalized = normalizeEOL(begin, end);
        if (placement == commentAfterOnSameLine) {
            assert(lastValue_ != nullptr);
            lastValue_->setComment(normalized, placement);
        } else {
            commentsBefore_ += normalized;
        }
    }

    bool Reader::readCStyleComment() {
        while ((current_ + 1) < end_) {
            Char c = getNextChar();
            if (c == '*' && *current_ == '/')
                break;
        }
        return getNextChar() == '/';
    }

    bool Reader::readCppStyleComment() {
        while (current_ != end_) {
            Char c = getNextChar();
            if (c == '\n')
                break;
            if (c == '\r') {
                // Consume DOS EOL. It will be normalized in addComment.
                if (current_ != end_ && *current_ == '\n')
                    getNextChar();
                // Break on Moc OS 9 EOL.
                break;
            }
        }
        return true;
    }

    void Reader::readNumber() {
        Location p = current_;
        char c = '0'; // stopgap for already consumed character
        // integral part
        while (c >= '0' && c <= '9')
            c = (current_ = p) < end_ ? *p++ : '\0';
        // fractional part
        if (c == '.') {
            c = (current_ = p) < end_ ? *p++ : '\0';
            while (c >= '0' && c <= '9')
                c = (current_ = p) < end_ ? *p++ : '\0';
        }
        // exponential part
        if (c == 'e' || c == 'E') {
            c = (current_ = p) < end_ ? *p++ : '\0';
            if (c == '+' || c == '-')
                c = (current_ = p) < end_ ? *p++ : '\0';
            while (c >= '0' && c <= '9')
                c = (current_ = p) < end_ ? *p++ : '\0';
        }
    }

    bool Reader::readString() {
        Char c = '\0';
        while (current_ != end_) {
            c = getNextChar();
            if (c == '\\')
                getNextChar();
            else if (c == '"')
                break;
        }
        return c == '"';
    }

    bool Reader::readObject(Token& token) {
        Token tokenName;
        String name;
        Value init(objectValue);
        currentValue().swapPayload(init);
        currentValue().setOffsetStart(token.start_ - begin_);
        while (readToken(tokenName)) {
            bool initialTokenOk = true;
            while (tokenName.type_ == tokenComment && initialTokenOk)
                initialTokenOk = readToken(tokenName);
            if (!initialTokenOk)
                break;
            if (tokenName.type_ == tokenObjectEnd &&
                (name.empty() ||
                    features_.allowTrailingCommas_)) // empty object or trailing comma
                return true;
            name.clear();
            if (tokenName.type_ == tokenString) {
                if (!decodeString(tokenName, name))
                    return recoverFromError(tokenObjectEnd);
            } else if (tokenName.type_ == tokenNumber && features_.allowNumericKeys_) {
                Value numberName;
                if (!decodeNumber(tokenName, numberName))
                    return recoverFromError(tokenObjectEnd);
                name = numberName.asString();
            } else {
                break;
            }

            Token colon;
            if (!readToken(colon) || colon.type_ != tokenMemberSeparator) {
                return addErrorAndRecover("Missing ':' after object member name", colon,
                    tokenObjectEnd);
            }
            Value& value = currentValue()[name];
            nodes_.push(&value);
            bool ok = readValue();
            nodes_.pop();
            if (!ok) // error already set
                return recoverFromError(tokenObjectEnd);

            Token comma;
            if (!readToken(comma) ||
                (comma.type_ != tokenObjectEnd && comma.type_ != tokenArraySeparator &&
                    comma.type_ != tokenComment)) {
                return addErrorAndRecover("Missing ',' or '}' in object declaration",
                    comma, tokenObjectEnd);
            }
            bool finalizeTokenOk = true;
            while (comma.type_ == tokenComment && finalizeTokenOk)
                finalizeTokenOk = readToken(comma);
            if (comma.type_ == tokenObjectEnd)
                return true;
        }
        return addErrorAndRecover("Missing '}' or object member name", tokenName,
            tokenObjectEnd);
    }

    bool Reader::readArray(Token& token) {
        Value init(arrayValue);
        currentValue().swapPayload(init);
        currentValue().setOffsetStart(token.start_ - begin_);
        int index = 0;
        for (;;) {
            skipSpaces();
            if (current_ != end_ && *current_ == ']' &&
                (index == 0 ||
                (features_.allowTrailingCommas_ &&
                    !features_.allowDroppedNullPlaceholders_))) // empty array or trailing
                                                                // comma
            {
                Token endArray;
                readToken(endArray);
                return true;
            }

            Value& value = currentValue()[index++];
            nodes_.push(&value);
            bool ok = readValue();
            nodes_.pop();
            if (!ok) // error already set
                return recoverFromError(tokenArrayEnd);

            Token currentToken;
            // Accept Comment after last item in the array.
            ok = readToken(currentToken);
            while (currentToken.type_ == tokenComment && ok) {
                ok = readToken(currentToken);
            }
            bool badTokenType = (currentToken.type_ != tokenArraySeparator &&
                currentToken.type_ != tokenArrayEnd);
            if (!ok || badTokenType) {
                return addErrorAndRecover("Missing ',' or ']' in array declaration",
                    currentToken, tokenArrayEnd);
            }
            if (currentToken.type_ == tokenArrayEnd)
                break;
        }
        return true;
    }

    bool Reader::decodeNumber(Token& token) {
        Value decoded;
        if (!decodeNumber(token, decoded))
            return false;
        currentValue().swapPayload(decoded);
        currentValue().setOffsetStart(token.start_ - begin_);
        currentValue().setOffsetLimit(token.end_ - begin_);
        return true;
    }

    bool Reader::decodeNumber(Token& token, Value& decoded) {
        // Attempts to parse the number as an integer. If the number is
        // larger than the maximum supported value of an integer then
        // we decode the number as a double.
        Location current = token.start_;
        bool isNegative = *current == '-';
        if (isNegative)
            ++current;
        // TODO: Help the compiler do the div and mod at compile time or get rid of
        // them.
        Value::LargestUInt maxIntegerValue =
            isNegative ? Value::LargestUInt(Value::maxLargestInt) + 1
            : Value::maxLargestUInt;
        Value::LargestUInt threshold = maxIntegerValue / 10;
        Value::LargestUInt value = 0;
        while (current < token.end_) {
            Char c = *current++;
            if (c < '0' || c > '9')
                return decodeDouble(token, decoded);
            auto digit(static_cast<Value::UInt>(c - '0'));
            if (value >= threshold) {
                // We've hit or exceeded the max value divided by 10 (rounded down). If
                // a) we've only just touched the limit, b) this is the last digit, and
                // c) it's small enough to fit in that rounding delta, we're okay.
                // Otherwise treat this number as a double to avoid overflow.
                if (value > threshold || current != token.end_ ||
                    digit > maxIntegerValue % 10) {
                    return decodeDouble(token, decoded);
                }
            }
            value = value * 10 + digit;
        }
        if (isNegative && value == maxIntegerValue)
            decoded = Value::minLargestInt;
        else if (isNegative)
            decoded = -Value::LargestInt(value);
        else if (value <= Value::LargestUInt(Value::maxInt))
            decoded = Value::LargestInt(value);
        else
            decoded = value;
        return true;
    }

    bool Reader::decodeDouble(Token& token) {
        Value decoded;
        if (!decodeDouble(token, decoded))
            return false;
        currentValue().swapPayload(decoded);
        currentValue().setOffsetStart(token.start_ - begin_);
        currentValue().setOffsetLimit(token.end_ - begin_);
        return true;
    }

    bool Reader::decodeDouble(Token& token, Value& decoded) {
        double value = 0;
        String buffer(token.start_, token.end_);
        IStringStream is(buffer);
        if (!(is >> value))
            return addError(
                "'" + String(token.start_, token.end_) + "' is not a number.", token);
        decoded = value;
        return true;
    }

    bool Reader::decodeString(Token& token) {
        String decoded_string;
        if (!decodeString(token, decoded_string))
            return false;
        Value decoded(decoded_string);
        currentValue().swapPayload(decoded);
        currentValue().setOffsetStart(token.start_ - begin_);
        currentValue().setOffsetLimit(token.end_ - begin_);
        return true;
    }

    bool Reader::decodeString(Token& token, String& decoded) {
        decoded.reserve(static_cast<size_t>(token.end_ - token.start_ - 2));
        Location current = token.start_ + 1; // skip '"'
        Location end = token.end_ - 1;       // do not include '"'
        while (current != end) {
            Char c = *current++;
            if (c == '"')
                break;
            if (c == '\\') {
                if (current == end)
                    return addError("Empty escape sequence in string", token, current);
                Char escape = *current++;
                switch (escape) {
                case '"':
                    decoded += '"';
                    break;
                case '/':
                    decoded += '/';
                    break;
                case '\\':
                    decoded += '\\';
                    break;
                case 'b':
                    decoded += '\b';
                    break;
                case 'f':
                    decoded += '\f';
                    break;
                case 'n':
                    decoded += '\n';
                    break;
                case 'r':
                    decoded += '\r';
                    break;
                case 't':
                    decoded += '\t';
                    break;
                case 'u': {
                    unsigned int unicode;
                    if (!decodeUnicodeCodePoint(token, current, end, unicode))
                        return false;
                    decoded += codePointToUTF8(unicode);
                } break;
                default:
                    return addError("Bad escape sequence in string", token, current);
                }
            } else {
                decoded += c;
            }
        }
        return true;
    }

    bool Reader::decodeUnicodeCodePoint(Token& token, Location& current,
        Location end, unsigned int& unicode) {

        if (!decodeUnicodeEscapeSequence(token, current, end, unicode))
            return false;
        if (unicode >= 0xD800 && unicode <= 0xDBFF) {
            // surrogate pairs
            if (end - current < 6)
                return addError(
                    "additional six characters expected to parse unicode surrogate pair.",
                    token, current);
            if (*(current++) == '\\' && *(current++) == 'u') {
                unsigned int surrogatePair;
                if (decodeUnicodeEscapeSequence(token, current, end, surrogatePair)) {
                    unicode = 0x10000 + ((unicode & 0x3FF) << 10) + (surrogatePair & 0x3FF);
                } else
                    return false;
            } else
                return addError("expecting another \\u token to begin the second half of "
                    "a unicode surrogate pair",
                    token, current);
        }
        return true;
    }

    bool Reader::decodeUnicodeEscapeSequence(Token& token, Location& current,
        Location end,
        unsigned int& ret_unicode) {
        if (end - current < 4)
            return addError(
                "Bad unicode escape sequence in string: four digits expected.", token,
                current);
        int unicode = 0;
        for (int index = 0; index < 4; ++index) {
            Char c = *current++;
            unicode *= 16;
            if (c >= '0' && c <= '9')
                unicode += c - '0';
            else if (c >= 'a' && c <= 'f')
                unicode += c - 'a' + 10;
            else if (c >= 'A' && c <= 'F')
                unicode += c - 'A' + 10;
            else
                return addError(
                    "Bad unicode escape sequence in string: hexadecimal digit expected.",
                    token, current);
        }
        ret_unicode = static_cast<unsigned int>(unicode);
        return true;
    }

    bool Reader::addError(const String& message, Token& token, Location extra) {
        ErrorInfo info;
        info.token_ = token;
        info.message_ = message;
        info.extra_ = extra;
        errors_.push_back(info);
        return false;
    }

    bool Reader::recoverFromError(TokenType skipUntilToken) {
        size_t const errorCount = errors_.size();
        Token skip;
        for (;;) {
            if (!readToken(skip))
                errors_.resize(errorCount); // discard errors caused by recovery
            if (skip.type_ == skipUntilToken || skip.type_ == tokenEndOfStream)
                break;
        }
        errors_.resize(errorCount);
        return false;
    }

    bool Reader::addErrorAndRecover(const String& message, Token& token,
        TokenType skipUntilToken) {
        addError(message, token);
        return recoverFromError(skipUntilToken);
    }

    Value& Reader::currentValue() { return *(nodes_.top()); }

    Reader::Char Reader::getNextChar() {
        if (current_ == end_)
            return 0;
        return *current_++;
    }

    void Reader::getLocationLineAndColumn(Location location, int& line,
        int& column) const {
        Location current = begin_;
        Location lastLineStart = current;
        line = 0;
        while (current < location && current != end_) {
            Char c = *current++;
            if (c == '\r') {
                if (*current == '\n')
                    ++current;
                lastLineStart = current;
                ++line;
            } else if (c == '\n') {
                lastLineStart = current;
                ++line;
            }
        }
        // column & line start at 1
        column = int(location - lastLineStart) + 1;
        ++line;
    }

    String Reader::getLocationLineAndColumn(Location location) const {
        int line, column;
        getLocationLineAndColumn(location, line, column);
        char buffer[18 + 16 + 16 + 1];
        jsoncpp_snprintf(buffer, sizeof(buffer), "Line %d, Column %d", line, column);
        return buffer;
    }

    // Deprecated. Preserved for backward compatibility
    String Reader::getFormatedErrorMessages() const {
        return getFormattedErrorMessages();
    }

    String Reader::getFormattedErrorMessages() const {
        String formattedMessage;
        for (const auto& error : errors_) {
            formattedMessage +=
                "* " + getLocationLineAndColumn(error.token_.start_) + "\n";
            formattedMessage += "  " + error.message_ + "\n";
            if (error.extra_)
                formattedMessage +=
                "See " + getLocationLineAndColumn(error.extra_) + " for detail.\n";
        }
        return formattedMessage;
    }

    std::vector<Reader::StructuredError> Reader::getStructuredErrors() const {
        std::vector<Reader::StructuredError> allErrors;
        for (const auto& error : errors_) {
            Reader::StructuredError structured;
            structured.offset_start = error.token_.start_ - begin_;
            structured.offset_limit = error.token_.end_ - begin_;
            structured.message = error.message_;
            allErrors.push_back(structured);
        }
        return allErrors;
    }

    bool Reader::pushError(const Value& value, const String& message) {
        ptrdiff_t const length = end_ - begin_;
        if (value.getOffsetStart() > length || value.getOffsetLimit() > length)
            return false;
        Token token;
        token.type_ = tokenError;
        token.start_ = begin_ + value.getOffsetStart();
        token.end_ = begin_ + value.getOffsetLimit();
        ErrorInfo info;
        info.token_ = token;
        info.message_ = message;
        info.extra_ = nullptr;
        errors_.push_back(info);
        return true;
    }

    bool Reader::pushError(const Value& value, const String& message,
        const Value& extra) {
        ptrdiff_t const length = end_ - begin_;
        if (value.getOffsetStart() > length || value.getOffsetLimit() > length ||
            extra.getOffsetLimit() > length)
            return false;
        Token token;
        token.type_ = tokenError;
        token.start_ = begin_ + value.getOffsetStart();
        token.end_ = begin_ + value.getOffsetLimit();
        ErrorInfo info;
        info.token_ = token;
        info.message_ = message;
        info.extra_ = begin_ + extra.getOffsetStart();
        errors_.push_back(info);
        return true;
    }

    bool Reader::good() const { return errors_.empty(); }

    // Originally copied from the Features class (now deprecated), used internally
    // for features implementation.
    class OurFeatures {
    public:
        static OurFeatures all();
        bool allowComments_;
        bool allowTrailingCommas_;
        bool strictRoot_;
        bool allowDroppedNullPlaceholders_;
        bool allowNumericKeys_;
        bool allowSingleQuotes_;
        bool failIfExtra_;
        bool rejectDupKeys_;
        bool allowSpecialFloats_;
        size_t stackLimit_;
    }; // OurFeatures

    OurFeatures OurFeatures::all() { return {}; }

    // Implementation of class Reader
    // ////////////////////////////////

    // Originally copied from the Reader class (now deprecated), used internally
    // for implementing JSON reading.
    class OurReader {
    public:
        using Char = char;
        using Location = const Char*;
        struct StructuredError {
            ptrdiff_t offset_start;
            ptrdiff_t offset_limit;
            String message;
        };

        explicit OurReader(OurFeatures const& features);
        bool parse(const char* beginDoc, const char* endDoc, Value& root,
            bool collectComments = true);
        String getFormattedErrorMessages() const;
        std::vector<StructuredError> getStructuredErrors() const;

    private:
        OurReader(OurReader const&);      // no impl
        void operator=(OurReader const&); // no impl

        enum TokenType {
            tokenEndOfStream = 0,
            tokenObjectBegin,
            tokenObjectEnd,
            tokenArrayBegin,
            tokenArrayEnd,
            tokenString,
            tokenNumber,
            tokenTrue,
            tokenFalse,
            tokenNull,
            tokenNaN,
            tokenPosInf,
            tokenNegInf,
            tokenArraySeparator,
            tokenMemberSeparator,
            tokenComment,
            tokenError
        };

        class Token {
        public:
            TokenType type_;
            Location start_;
            Location end_;
        };

        class ErrorInfo {
        public:
            Token token_;
            String message_;
            Location extra_;
        };

        using Errors = std::deque<ErrorInfo>;

        bool readToken(Token& token);
        void skipSpaces();
        bool match(const Char* pattern, int patternLength);
        bool readComment();
        bool readCStyleComment(bool* containsNewLineResult);
        bool readCppStyleComment();
        bool readString();
        bool readStringSingleQuote();
        bool readNumber(bool checkInf);
        bool readValue();
        bool readObject(Token& token);
        bool readArray(Token& token);
        bool decodeNumber(Token& token);
        bool decodeNumber(Token& token, Value& decoded);
        bool decodeString(Token& token);
        bool decodeString(Token& token, String& decoded);
        bool decodeDouble(Token& token);
        bool decodeDouble(Token& token, Value& decoded);
        bool decodeUnicodeCodePoint(Token& token, Location& current, Location end,
            unsigned int& unicode);
        bool decodeUnicodeEscapeSequence(Token& token, Location& current,
            Location end, unsigned int& unicode);
        bool addError(const String& message, Token& token, Location extra = nullptr);
        bool recoverFromError(TokenType skipUntilToken);
        bool addErrorAndRecover(const String& message, Token& token,
            TokenType skipUntilToken);
        void skipUntilSpace();
        Value& currentValue();
        Char getNextChar();
        void getLocationLineAndColumn(Location location, int& line,
            int& column) const;
        String getLocationLineAndColumn(Location location) const;
        void addComment(Location begin, Location end, CommentPlacement placement);
        void skipCommentTokens(Token& token);

        static String normalizeEOL(Location begin, Location end);
        static bool containsNewLine(Location begin, Location end);

        using Nodes = std::stack<Value*>;

        Nodes nodes_{};
        Errors errors_{};
        String document_{};
        Location begin_ = nullptr;
        Location end_ = nullptr;
        Location current_ = nullptr;
        Location lastValueEnd_ = nullptr;
        Value* lastValue_ = nullptr;
        bool lastValueHasAComment_ = false;
        String commentsBefore_{};

        OurFeatures const features_;
        bool collectComments_ = false;
    }; // OurReader

    // complete copy of Read impl, for OurReader

    bool OurReader::containsNewLine(OurReader::Location begin,
        OurReader::Location end) {
        for (; begin < end; ++begin)
            if (*begin == '\n' || *begin == '\r')
                return true;
        return false;
    }

    OurReader::OurReader(OurFeatures const& features) : features_(features) {}

    bool OurReader::parse(const char* beginDoc, const char* endDoc, Value& root,
        bool collectComments) {
        if (!features_.allowComments_) {
            collectComments = false;
        }

        begin_ = beginDoc;
        end_ = endDoc;
        collectComments_ = collectComments;
        current_ = begin_;
        lastValueEnd_ = nullptr;
        lastValue_ = nullptr;
        commentsBefore_.clear();
        errors_.clear();
        while (!nodes_.empty())
            nodes_.pop();
        nodes_.push(&root);

        bool successful = readValue();
        nodes_.pop();
        Token token;
        skipCommentTokens(token);
        if (features_.failIfExtra_ && (token.type_ != tokenEndOfStream)) {
            addError("Extra non-whitespace after JSON value.", token);
            return false;
        }
        if (collectComments_ && !commentsBefore_.empty())
            root.setComment(commentsBefore_, commentAfter);
        if (features_.strictRoot_) {
            if (!root.isArray() && !root.isObject()) {
                // Set error location to start of doc, ideally should be first token found
                // in doc
                token.type_ = tokenError;
                token.start_ = beginDoc;
                token.end_ = endDoc;
                addError(
                    "A valid JSON document must be either an array or an object value.",
                    token);
                return false;
            }
        }
        return successful;
    }

    bool OurReader::readValue() {
        //  To preserve the old behaviour we cast size_t to int.
        if (nodes_.size() > features_.stackLimit_)
            throwRuntimeError("Exceeded stackLimit in readValue().");
        Token token;
        skipCommentTokens(token);
        bool successful = true;

        if (collectComments_ && !commentsBefore_.empty()) {
            currentValue().setComment(commentsBefore_, commentBefore);
            commentsBefore_.clear();
        }

        switch (token.type_) {
        case tokenObjectBegin:
            successful = readObject(token);
            currentValue().setOffsetLimit(current_ - begin_);
            break;
        case tokenArrayBegin:
            successful = readArray(token);
            currentValue().setOffsetLimit(current_ - begin_);
            break;
        case tokenNumber:
            successful = decodeNumber(token);
            break;
        case tokenString:
            successful = decodeString(token);
            break;
        case tokenTrue: {
            Value v(true);
            currentValue().swapPayload(v);
            currentValue().setOffsetStart(token.start_ - begin_);
            currentValue().setOffsetLimit(token.end_ - begin_);
        } break;
        case tokenFalse: {
            Value v(false);
            currentValue().swapPayload(v);
            currentValue().setOffsetStart(token.start_ - begin_);
            currentValue().setOffsetLimit(token.end_ - begin_);
        } break;
        case tokenNull: {
            Value v;
            currentValue().swapPayload(v);
            currentValue().setOffsetStart(token.start_ - begin_);
            currentValue().setOffsetLimit(token.end_ - begin_);
        } break;
        case tokenNaN: {
            Value v(std::numeric_limits<double>::quiet_NaN());
            currentValue().swapPayload(v);
            currentValue().setOffsetStart(token.start_ - begin_);
            currentValue().setOffsetLimit(token.end_ - begin_);
        } break;
        case tokenPosInf: {
            Value v(std::numeric_limits<double>::infinity());
            currentValue().swapPayload(v);
            currentValue().setOffsetStart(token.start_ - begin_);
            currentValue().setOffsetLimit(token.end_ - begin_);
        } break;
        case tokenNegInf: {
            Value v(-std::numeric_limits<double>::infinity());
            currentValue().swapPayload(v);
            currentValue().setOffsetStart(token.start_ - begin_);
            currentValue().setOffsetLimit(token.end_ - begin_);
        } break;
        case tokenArraySeparator:
        case tokenObjectEnd:
        case tokenArrayEnd:
            if (features_.allowDroppedNullPlaceholders_) {
                // "Un-read" the current token and mark the current value as a null
                // token.
                current_--;
                Value v;
                currentValue().swapPayload(v);
                currentValue().setOffsetStart(current_ - begin_ - 1);
                currentValue().setOffsetLimit(current_ - begin_);
                break;
            } // else, fall through ...
        default:
            currentValue().setOffsetStart(token.start_ - begin_);
            currentValue().setOffsetLimit(token.end_ - begin_);
            return addError("Syntax error: value, object or array expected.", token);
        }

        if (collectComments_) {
            lastValueEnd_ = current_;
            lastValueHasAComment_ = false;
            lastValue_ = &currentValue();
        }

        return successful;
    }

    void OurReader::skipCommentTokens(Token& token) {
        if (features_.allowComments_) {
            do {
                readToken(token);
            } while (token.type_ == tokenComment);
        } else {
            readToken(token);
        }
    }

    bool OurReader::readToken(Token& token) {
        skipSpaces();
        token.start_ = current_;
        Char c = getNextChar();
        bool ok = true;
        switch (c) {
        case '{':
            token.type_ = tokenObjectBegin;
            break;
        case '}':
            token.type_ = tokenObjectEnd;
            break;
        case '[':
            token.type_ = tokenArrayBegin;
            break;
        case ']':
            token.type_ = tokenArrayEnd;
            break;
        case '"':
            token.type_ = tokenString;
            ok = readString();
            break;
        case '\'':
            if (features_.allowSingleQuotes_) {
                token.type_ = tokenString;
                ok = readStringSingleQuote();
                break;
            } // else fall through
        case '/':
            token.type_ = tokenComment;
            ok = readComment();
            break;
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
            token.type_ = tokenNumber;
            readNumber(false);
            break;
        case '-':
            if (readNumber(true)) {
                token.type_ = tokenNumber;
            } else {
                token.type_ = tokenNegInf;
                ok = features_.allowSpecialFloats_ && match("nfinity", 7);
            }
            break;
        case '+':
            if (readNumber(true)) {
                token.type_ = tokenNumber;
            } else {
                token.type_ = tokenPosInf;
                ok = features_.allowSpecialFloats_ && match("nfinity", 7);
            }
            break;
        case 't':
            token.type_ = tokenTrue;
            ok = match("rue", 3);
            break;
        case 'f':
            token.type_ = tokenFalse;
            ok = match("alse", 4);
            break;
        case 'n':
            token.type_ = tokenNull;
            ok = match("ull", 3);
            break;
        case 'N':
            if (features_.allowSpecialFloats_) {
                token.type_ = tokenNaN;
                ok = match("aN", 2);
            } else {
                ok = false;
            }
            break;
        case 'I':
            if (features_.allowSpecialFloats_) {
                token.type_ = tokenPosInf;
                ok = match("nfinity", 7);
            } else {
                ok = false;
            }
            break;
        case ',':
            token.type_ = tokenArraySeparator;
            break;
        case ':':
            token.type_ = tokenMemberSeparator;
            break;
        case 0:
            token.type_ = tokenEndOfStream;
            break;
        default:
            ok = false;
            break;
        }
        if (!ok)
            token.type_ = tokenError;
        token.end_ = current_;
        return ok;
    }

    void OurReader::skipSpaces() {
        while (current_ != end_) {
            Char c = *current_;
            if (c == ' ' || c == '\t' || c == '\r' || c == '\n')
                ++current_;
            else
                break;
        }
    }

    bool OurReader::match(const Char* pattern, int patternLength) {
        if (end_ - current_ < patternLength)
            return false;
        int index = patternLength;
        while (index--)
            if (current_[index] != pattern[index])
                return false;
        current_ += patternLength;
        return true;
    }

    bool OurReader::readComment() {
        const Location commentBegin = current_ - 1;
        const Char c = getNextChar();
        bool successful = false;
        bool cStyleWithEmbeddedNewline = false;

        const bool isCStyleComment = (c == '*');
        const bool isCppStyleComment = (c == '/');
        if (isCStyleComment) {
            successful = readCStyleComment(&cStyleWithEmbeddedNewline);
        } else if (isCppStyleComment) {
            successful = readCppStyleComment();
        }

        if (!successful)
            return false;

        if (collectComments_) {
            CommentPlacement placement = commentBefore;

            if (!lastValueHasAComment_) {
                if (lastValueEnd_ && !containsNewLine(lastValueEnd_, commentBegin)) {
                    if (isCppStyleComment || !cStyleWithEmbeddedNewline) {
                        placement = commentAfterOnSameLine;
                        lastValueHasAComment_ = true;
                    }
                }
            }

            addComment(commentBegin, current_, placement);
        }
        return true;
    }

    String OurReader::normalizeEOL(OurReader::Location begin,
        OurReader::Location end) {
        String normalized;
        normalized.reserve(static_cast<size_t>(end - begin));
        OurReader::Location current = begin;
        while (current != end) {
            char c = *current++;
            if (c == '\r') {
                if (current != end && *current == '\n')
                    // convert dos EOL
                    ++current;
                // convert Mac EOL
                normalized += '\n';
            } else {
                normalized += c;
            }
        }
        return normalized;
    }

    void OurReader::addComment(Location begin, Location end,
        CommentPlacement placement) {
        assert(collectComments_);
        const String& normalized = normalizeEOL(begin, end);
        if (placement == commentAfterOnSameLine) {
            assert(lastValue_ != nullptr);
            lastValue_->setComment(normalized, placement);
        } else {
            commentsBefore_ += normalized;
        }
    }

    bool OurReader::readCStyleComment(bool* containsNewLineResult) {
        *containsNewLineResult = false;

        while ((current_ + 1) < end_) {
            Char c = getNextChar();
            if (c == '*' && *current_ == '/')
                break;
            if (c == '\n')
                *containsNewLineResult = true;
        }

        return getNextChar() == '/';
    }

    bool OurReader::readCppStyleComment() {
        while (current_ != end_) {
            Char c = getNextChar();
            if (c == '\n')
                break;
            if (c == '\r') {
                // Consume DOS EOL. It will be normalized in addComment.
                if (current_ != end_ && *current_ == '\n')
                    getNextChar();
                // Break on Moc OS 9 EOL.
                break;
            }
        }
        return true;
    }

    bool OurReader::readNumber(bool checkInf) {
        Location p = current_;
        if (checkInf && p != end_ && *p == 'I') {
            current_ = ++p;
            return false;
        }
        char c = '0'; // stopgap for already consumed character
        // integral part
        while (c >= '0' && c <= '9')
            c = (current_ = p) < end_ ? *p++ : '\0';
        // fractional part
        if (c == '.') {
            c = (current_ = p) < end_ ? *p++ : '\0';
            while (c >= '0' && c <= '9')
                c = (current_ = p) < end_ ? *p++ : '\0';
        }
        // exponential part
        if (c == 'e' || c == 'E') {
            c = (current_ = p) < end_ ? *p++ : '\0';
            if (c == '+' || c == '-')
                c = (current_ = p) < end_ ? *p++ : '\0';
            while (c >= '0' && c <= '9')
                c = (current_ = p) < end_ ? *p++ : '\0';
        }
        return true;
    }
    bool OurReader::readString() {
        Char c = 0;
        while (current_ != end_) {
            c = getNextChar();
            if (c == '\\')
                getNextChar();
            else if (c == '"')
                break;
        }
        return c == '"';
    }

    bool OurReader::readStringSingleQuote() {
        Char c = 0;
        while (current_ != end_) {
            c = getNextChar();
            if (c == '\\')
                getNextChar();
            else if (c == '\'')
                break;
        }
        return c == '\'';
    }

    bool OurReader::readObject(Token& token) {
        Token tokenName;
        String name;
        Value init(objectValue);
        currentValue().swapPayload(init);
        currentValue().setOffsetStart(token.start_ - begin_);
        while (readToken(tokenName)) {
            bool initialTokenOk = true;
            while (tokenName.type_ == tokenComment && initialTokenOk)
                initialTokenOk = readToken(tokenName);
            if (!initialTokenOk)
                break;
            if (tokenName.type_ == tokenObjectEnd &&
                (name.empty() ||
                    features_.allowTrailingCommas_)) // empty object or trailing comma
                return true;
            name.clear();
            if (tokenName.type_ == tokenString) {
                if (!decodeString(tokenName, name))
                    return recoverFromError(tokenObjectEnd);
            } else if (tokenName.type_ == tokenNumber && features_.allowNumericKeys_) {
                Value numberName;
                if (!decodeNumber(tokenName, numberName))
                    return recoverFromError(tokenObjectEnd);
                name = numberName.asString();
            } else {
                break;
            }
            if (name.length() >= (1U << 30))
                throwRuntimeError("keylength >= 2^30");
            if (features_.rejectDupKeys_ && currentValue().isMember(name)) {
                String msg = "Duplicate key: '" + name + "'";
                return addErrorAndRecover(msg, tokenName, tokenObjectEnd);
            }

            Token colon;
            if (!readToken(colon) || colon.type_ != tokenMemberSeparator) {
                return addErrorAndRecover("Missing ':' after object member name", colon,
                    tokenObjectEnd);
            }
            Value& value = currentValue()[name];
            nodes_.push(&value);
            bool ok = readValue();
            nodes_.pop();
            if (!ok) // error already set
                return recoverFromError(tokenObjectEnd);

            Token comma;
            if (!readToken(comma) ||
                (comma.type_ != tokenObjectEnd && comma.type_ != tokenArraySeparator &&
                    comma.type_ != tokenComment)) {
                return addErrorAndRecover("Missing ',' or '}' in object declaration",
                    comma, tokenObjectEnd);
            }
            bool finalizeTokenOk = true;
            while (comma.type_ == tokenComment && finalizeTokenOk)
                finalizeTokenOk = readToken(comma);
            if (comma.type_ == tokenObjectEnd)
                return true;
        }
        return addErrorAndRecover("Missing '}' or object member name", tokenName,
            tokenObjectEnd);
    }

    bool OurReader::readArray(Token& token) {
        Value init(arrayValue);
        currentValue().swapPayload(init);
        currentValue().setOffsetStart(token.start_ - begin_);
        int index = 0;
        for (;;) {
            skipSpaces();
            if (current_ != end_ && *current_ == ']' &&
                (index == 0 ||
                (features_.allowTrailingCommas_ &&
                    !features_.allowDroppedNullPlaceholders_))) // empty array or trailing
                                                                // comma
            {
                Token endArray;
                readToken(endArray);
                return true;
            }
            Value& value = currentValue()[index++];
            nodes_.push(&value);
            bool ok = readValue();
            nodes_.pop();
            if (!ok) // error already set
                return recoverFromError(tokenArrayEnd);

            Token currentToken;
            // Accept Comment after last item in the array.
            ok = readToken(currentToken);
            while (currentToken.type_ == tokenComment && ok) {
                ok = readToken(currentToken);
            }
            bool badTokenType = (currentToken.type_ != tokenArraySeparator &&
                currentToken.type_ != tokenArrayEnd);
            if (!ok || badTokenType) {
                return addErrorAndRecover("Missing ',' or ']' in array declaration",
                    currentToken, tokenArrayEnd);
            }
            if (currentToken.type_ == tokenArrayEnd)
                break;
        }
        return true;
    }

    bool OurReader::decodeNumber(Token& token) {
        Value decoded;
        if (!decodeNumber(token, decoded))
            return false;
        currentValue().swapPayload(decoded);
        currentValue().setOffsetStart(token.start_ - begin_);
        currentValue().setOffsetLimit(token.end_ - begin_);
        return true;
    }

    bool OurReader::decodeNumber(Token& token, Value& decoded) {
        // Attempts to parse the number as an integer. If the number is
        // larger than the maximum supported value of an integer then
        // we decode the number as a double.
        Location current = token.start_;
        const bool isNegative = *current == '-';
        if (isNegative) {
            ++current;
        }

        // We assume we can represent the largest and smallest integer types as
        // unsigned integers with separate sign. This is only true if they can fit
        // into an unsigned integer.
        static_assert(Value::maxLargestInt <= Value::maxLargestUInt,
            "Int must be smaller than UInt");

        // We need to convert minLargestInt into a positive number. The easiest way
        // to do this conversion is to assume our "threshold" value of minLargestInt
        // divided by 10 can fit in maxLargestInt when absolute valued. This should
        // be a safe assumption.
        static_assert(Value::minLargestInt <= -Value::maxLargestInt,
            "The absolute value of minLargestInt must be greater than or "
            "equal to maxLargestInt");
        static_assert(Value::minLargestInt / 10 >= -Value::maxLargestInt,
            "The absolute value of minLargestInt must be only 1 magnitude "
            "larger than maxLargest Int");

        static constexpr Value::LargestUInt positive_threshold =
            Value::maxLargestUInt / 10;
        static constexpr Value::UInt positive_last_digit = Value::maxLargestUInt % 10;

        // For the negative values, we have to be more careful. Since typically
        // -Value::minLargestInt will cause an overflow, we first divide by 10 and
        // then take the inverse. This assumes that minLargestInt is only a single
        // power of 10 different in magnitude, which we check above. For the last
        // digit, we take the modulus before negating for the same reason.
        static constexpr auto negative_threshold =
            Value::LargestUInt(-(Value::minLargestInt / 10));
        static constexpr auto negative_last_digit =
            Value::UInt(-(Value::minLargestInt % 10));

        const Value::LargestUInt threshold =
            isNegative ? negative_threshold : positive_threshold;
        const Value::UInt max_last_digit =
            isNegative ? negative_last_digit : positive_last_digit;

        Value::LargestUInt value = 0;
        while (current < token.end_) {
            Char c = *current++;
            if (c < '0' || c > '9')
                return decodeDouble(token, decoded);

            const auto digit(static_cast<Value::UInt>(c - '0'));
            if (value >= threshold) {
                // We've hit or exceeded the max value divided by 10 (rounded down). If
                // a) we've only just touched the limit, meaing value == threshold,
                // b) this is the last digit, or
                // c) it's small enough to fit in that rounding delta, we're okay.
                // Otherwise treat this number as a double to avoid overflow.
                if (value > threshold || current != token.end_ ||
                    digit > max_last_digit) {
                    return decodeDouble(token, decoded);
                }
            }
            value = value * 10 + digit;
        }

        if (isNegative) {
            // We use the same magnitude assumption here, just in case.
            const auto last_digit = static_cast<Value::UInt>(value % 10);
            decoded = -Value::LargestInt(value / 10) * 10 - last_digit;
        } else if (value <= Value::LargestUInt(Value::maxLargestInt)) {
            decoded = Value::LargestInt(value);
        } else {
            decoded = value;
        }

        return true;
    }

    bool OurReader::decodeDouble(Token& token) {
        Value decoded;
        if (!decodeDouble(token, decoded))
            return false;
        currentValue().swapPayload(decoded);
        currentValue().setOffsetStart(token.start_ - begin_);
        currentValue().setOffsetLimit(token.end_ - begin_);
        return true;
    }

    bool OurReader::decodeDouble(Token& token, Value& decoded) {
        double value = 0;
        const String buffer(token.start_, token.end_);
        IStringStream is(buffer);
        if (!(is >> value)) {
            return addError(
                "'" + String(token.start_, token.end_) + "' is not a number.", token);
        }
        decoded = value;
        return true;
    }

    bool OurReader::decodeString(Token& token) {
        String decoded_string;
        if (!decodeString(token, decoded_string))
            return false;
        Value decoded(decoded_string);
        currentValue().swapPayload(decoded);
        currentValue().setOffsetStart(token.start_ - begin_);
        currentValue().setOffsetLimit(token.end_ - begin_);
        return true;
    }

    bool OurReader::decodeString(Token& token, String& decoded) {
        decoded.reserve(static_cast<size_t>(token.end_ - token.start_ - 2));
        Location current = token.start_ + 1; // skip '"'
        Location end = token.end_ - 1;       // do not include '"'
        while (current != end) {
            Char c = *current++;
            if (c == '"')
                break;
            if (c == '\\') {
                if (current == end)
                    return addError("Empty escape sequence in string", token, current);
                Char escape = *current++;
                switch (escape) {
                case '"':
                    decoded += '"';
                    break;
                case '/':
                    decoded += '/';
                    break;
                case '\\':
                    decoded += '\\';
                    break;
                case 'b':
                    decoded += '\b';
                    break;
                case 'f':
                    decoded += '\f';
                    break;
                case 'n':
                    decoded += '\n';
                    break;
                case 'r':
                    decoded += '\r';
                    break;
                case 't':
                    decoded += '\t';
                    break;
                case 'u': {
                    unsigned int unicode;
                    if (!decodeUnicodeCodePoint(token, current, end, unicode))
                        return false;
                    decoded += codePointToUTF8(unicode);
                } break;
                default:
                    return addError("Bad escape sequence in string", token, current);
                }
            } else {
                decoded += c;
            }
        }
        return true;
    }

    bool OurReader::decodeUnicodeCodePoint(Token& token, Location& current,
        Location end, unsigned int& unicode) {

        if (!decodeUnicodeEscapeSequence(token, current, end, unicode))
            return false;
        if (unicode >= 0xD800 && unicode <= 0xDBFF) {
            // surrogate pairs
            if (end - current < 6)
                return addError(
                    "additional six characters expected to parse unicode surrogate pair.",
                    token, current);
            if (*(current++) == '\\' && *(current++) == 'u') {
                unsigned int surrogatePair;
                if (decodeUnicodeEscapeSequence(token, current, end, surrogatePair)) {
                    unicode = 0x10000 + ((unicode & 0x3FF) << 10) + (surrogatePair & 0x3FF);
                } else
                    return false;
            } else
                return addError("expecting another \\u token to begin the second half of "
                    "a unicode surrogate pair",
                    token, current);
        }
        return true;
    }

    bool OurReader::decodeUnicodeEscapeSequence(Token& token, Location& current,
        Location end,
        unsigned int& ret_unicode) {
        if (end - current < 4)
            return addError(
                "Bad unicode escape sequence in string: four digits expected.", token,
                current);
        int unicode = 0;
        for (int index = 0; index < 4; ++index) {
            Char c = *current++;
            unicode *= 16;
            if (c >= '0' && c <= '9')
                unicode += c - '0';
            else if (c >= 'a' && c <= 'f')
                unicode += c - 'a' + 10;
            else if (c >= 'A' && c <= 'F')
                unicode += c - 'A' + 10;
            else
                return addError(
                    "Bad unicode escape sequence in string: hexadecimal digit expected.",
                    token, current);
        }
        ret_unicode = static_cast<unsigned int>(unicode);
        return true;
    }

    bool OurReader::addError(const String& message, Token& token, Location extra) {
        ErrorInfo info;
        info.token_ = token;
        info.message_ = message;
        info.extra_ = extra;
        errors_.push_back(info);
        return false;
    }

    bool OurReader::recoverFromError(TokenType skipUntilToken) {
        size_t errorCount = errors_.size();
        Token skip;
        for (;;) {
            if (!readToken(skip))
                errors_.resize(errorCount); // discard errors caused by recovery
            if (skip.type_ == skipUntilToken || skip.type_ == tokenEndOfStream)
                break;
        }
        errors_.resize(errorCount);
        return false;
    }

    bool OurReader::addErrorAndRecover(const String& message, Token& token,
        TokenType skipUntilToken) {
        addError(message, token);
        return recoverFromError(skipUntilToken);
    }

    Value& OurReader::currentValue() { return *(nodes_.top()); }

    OurReader::Char OurReader::getNextChar() {
        if (current_ == end_)
            return 0;
        return *current_++;
    }

    void OurReader::getLocationLineAndColumn(Location location, int& line,
        int& column) const {
        Location current = begin_;
        Location lastLineStart = current;
        line = 0;
        while (current < location && current != end_) {
            Char c = *current++;
            if (c == '\r') {
                if (*current == '\n')
                    ++current;
                lastLineStart = current;
                ++line;
            } else if (c == '\n') {
                lastLineStart = current;
                ++line;
            }
        }
        // column & line start at 1
        column = int(location - lastLineStart) + 1;
        ++line;
    }

    String OurReader::getLocationLineAndColumn(Location location) const {
        int line, column;
        getLocationLineAndColumn(location, line, column);
        char buffer[18 + 16 + 16 + 1];
        jsoncpp_snprintf(buffer, sizeof(buffer), "Line %d, Column %d", line, column);
        return buffer;
    }

    String OurReader::getFormattedErrorMessages() const {
        String formattedMessage;
        for (const auto& error : errors_) {
            formattedMessage +=
                "* " + getLocationLineAndColumn(error.token_.start_) + "\n";
            formattedMessage += "  " + error.message_ + "\n";
            if (error.extra_)
                formattedMessage +=
                "See " + getLocationLineAndColumn(error.extra_) + " for detail.\n";
        }
        return formattedMessage;
    }

    std::vector<OurReader::StructuredError> OurReader::getStructuredErrors() const {
        std::vector<OurReader::StructuredError> allErrors;
        for (const auto& error : errors_) {
            OurReader::StructuredError structured;
            structured.offset_start = error.token_.start_ - begin_;
            structured.offset_limit = error.token_.end_ - begin_;
            structured.message = error.message_;
            allErrors.push_back(structured);
        }
        return allErrors;
    }

    class OurCharReader : public CharReader {
        bool const collectComments_;
        OurReader reader_;

    public:
        OurCharReader(bool collectComments, OurFeatures const& features)
            : collectComments_(collectComments), reader_(features) {}
        bool parse(char const* beginDoc, char const* endDoc, Value* root,
            String* errs) override {
            bool ok = reader_.parse(beginDoc, endDoc, *root, collectComments_);
            if (errs) {
                *errs = reader_.getFormattedErrorMessages();
            }
            return ok;
        }
    };

    CharReaderBuilder::CharReaderBuilder() { setDefaults(&settings_); }
    CharReaderBuilder::~CharReaderBuilder() = default;
    CharReader* CharReaderBuilder::newCharReader() const {
        bool collectComments = settings_["collectComments"].asBool();
        OurFeatures features = OurFeatures::all();
        features.allowComments_ = settings_["allowComments"].asBool();
        features.allowTrailingCommas_ = settings_["allowTrailingCommas"].asBool();
        features.strictRoot_ = settings_["strictRoot"].asBool();
        features.allowDroppedNullPlaceholders_ =
            settings_["allowDroppedNullPlaceholders"].asBool();
        features.allowNumericKeys_ = settings_["allowNumericKeys"].asBool();
        features.allowSingleQuotes_ = settings_["allowSingleQuotes"].asBool();

        // Stack limit is always a size_t, so we get this as an unsigned int
        // regardless of it we have 64-bit integer support enabled.
        features.stackLimit_ = static_cast<size_t>(settings_["stackLimit"].asUInt());
        features.failIfExtra_ = settings_["failIfExtra"].asBool();
        features.rejectDupKeys_ = settings_["rejectDupKeys"].asBool();
        features.allowSpecialFloats_ = settings_["allowSpecialFloats"].asBool();
        return new OurCharReader(collectComments, features);
    }
    static void getValidReaderKeys(std::set<String>* valid_keys) {
        valid_keys->clear();
        valid_keys->insert("collectComments");
        valid_keys->insert("allowComments");
        valid_keys->insert("allowTrailingCommas");
        valid_keys->insert("strictRoot");
        valid_keys->insert("allowDroppedNullPlaceholders");
        valid_keys->insert("allowNumericKeys");
        valid_keys->insert("allowSingleQuotes");
        valid_keys->insert("stackLimit");
        valid_keys->insert("failIfExtra");
        valid_keys->insert("rejectDupKeys");
        valid_keys->insert("allowSpecialFloats");
    }
    bool CharReaderBuilder::validate(Json::Value* invalid) const {
        Json::Value my_invalid;
        if (!invalid)
            invalid = &my_invalid; // so we do not need to test for NULL
        Json::Value& inv = *invalid;
        std::set<String> valid_keys;
        getValidReaderKeys(&valid_keys);
        Value::Members keys = settings_.getMemberNames();
        size_t n = keys.size();
        for (size_t i = 0; i < n; ++i) {
            String const& key = keys[i];
            if (valid_keys.find(key) == valid_keys.end()) {
                inv[key] = settings_[key];
            }
        }
        return inv.empty();
    }
    Value& CharReaderBuilder::operator[](const String& key) {
        return settings_[key];
    }
    // static
    void CharReaderBuilder::strictMode(Json::Value* settings) {
        //! [CharReaderBuilderStrictMode]
        (*settings)["allowComments"] = false;
        (*settings)["allowTrailingCommas"] = false;
        (*settings)["strictRoot"] = true;
        (*settings)["allowDroppedNullPlaceholders"] = false;
        (*settings)["allowNumericKeys"] = false;
        (*settings)["allowSingleQuotes"] = false;
        (*settings)["stackLimit"] = 1000;
        (*settings)["failIfExtra"] = true;
        (*settings)["rejectDupKeys"] = true;
        (*settings)["allowSpecialFloats"] = false;
        //! [CharReaderBuilderStrictMode]
    }
    // static
    void CharReaderBuilder::setDefaults(Json::Value* settings) {
        //! [CharReaderBuilderDefaults]
        (*settings)["collectComments"] = true;
        (*settings)["allowComments"] = true;
        (*settings)["allowTrailingCommas"] = true;
        (*settings)["strictRoot"] = false;
        (*settings)["allowDroppedNullPlaceholders"] = false;
        (*settings)["allowNumericKeys"] = false;
        (*settings)["allowSingleQuotes"] = false;
        (*settings)["stackLimit"] = 1000;
        (*settings)["failIfExtra"] = false;
        (*settings)["rejectDupKeys"] = false;
        (*settings)["allowSpecialFloats"] = false;
        //! [CharReaderBuilderDefaults]
    }

    //////////////////////////////////
    // global functions

    bool parseFromStream(CharReader::Factory const& fact, IStream& sin, Value* root,
        String* errs) {
        OStringStream ssin;
        ssin << sin.rdbuf();
        String doc = ssin.str();
        char const* begin = doc.data();
        char const* end = begin + doc.size();
        // Note that we do not actually need a null-terminator.
        CharReaderPtr const reader(fact.newCharReader());
        return reader->parse(begin, end, root, errs);
    }

    IStream& operator>>(IStream& sin, Value& root) {
        CharReaderBuilder b;
        String errs;
        bool ok = parseFromStream(b, sin, &root, &errs);
        if (!ok) {
            throwRuntimeError(errs);
        }
        return sin;
    }

} // namespace Json

// //////////////////////////////////////////////////////////////////////
// End of content of file: src/lib_json/json_reader.cpp
// //////////////////////////////////////////////////////////////////////






// //////////////////////////////////////////////////////////////////////
// Beginning of content of file: src/lib_json/json_valueiterator.inl
// //////////////////////////////////////////////////////////////////////

// Copyright 2007-2010 Baptiste Lepilleur and The JsonCpp Authors
// Distributed under MIT license, or public domain if desired and
// recognized in your jurisdiction.
// See file LICENSE for detail or copy at http://jsoncpp.sourceforge.net/LICENSE

// included by json_value.cpp

namespace Json {

    // //////////////////////////////////////////////////////////////////
    // //////////////////////////////////////////////////////////////////
    // //////////////////////////////////////////////////////////////////
    // class ValueIteratorBase
    // //////////////////////////////////////////////////////////////////
    // //////////////////////////////////////////////////////////////////
    // //////////////////////////////////////////////////////////////////

    ValueIteratorBase::ValueIteratorBase() : current_() {}

    ValueIteratorBase::ValueIteratorBase(
        const Value::ObjectValues::iterator& current)
        : current_(current), isNull_(false) {}

    Value& ValueIteratorBase::deref() { return current_->second; }
    const Value& ValueIteratorBase::deref() const { return current_->second; }

    void ValueIteratorBase::increment() { ++current_; }

    void ValueIteratorBase::decrement() { --current_; }

    ValueIteratorBase::difference_type
        ValueIteratorBase::computeDistance(const SelfType& other) const {
        // Iterator for null value are initialized using the default
        // constructor, which initialize current_ to the default
        // std::map::iterator. As begin() and end() are two instance
        // of the default std::map::iterator, they can not be compared.
        // To allow this, we handle this comparison specifically.
        if (isNull_ && other.isNull_) {
            return 0;
        }

        // Usage of std::distance is not portable (does not compile with Sun Studio 12
        // RogueWave STL,
        // which is the one used by default).
        // Using a portable hand-made version for non random iterator instead:
        //   return difference_type( std::distance( current_, other.current_ ) );
        difference_type myDistance = 0;
        for (Value::ObjectValues::iterator it = current_; it != other.current_;
            ++it) {
            ++myDistance;
        }
        return myDistance;
    }

    bool ValueIteratorBase::isEqual(const SelfType& other) const {
        if (isNull_) {
            return other.isNull_;
        }
        return current_ == other.current_;
    }

    void ValueIteratorBase::copy(const SelfType& other) {
        current_ = other.current_;
        isNull_ = other.isNull_;
    }

    Value ValueIteratorBase::key() const {
        const Value::CZString czstring = (*current_).first;
        if (czstring.data()) {
            if (czstring.isStaticString())
                return Value(StaticString(czstring.data()));
            return Value(czstring.data(), czstring.data() + czstring.length());
        }
        return Value(czstring.index());
    }

    UInt ValueIteratorBase::index() const {
        const Value::CZString czstring = (*current_).first;
        if (!czstring.data())
            return czstring.index();
        return Value::UInt(-1);
    }

    String ValueIteratorBase::name() const {
        char const* keey;
        char const* end;
        keey = memberName(&end);
        if (!keey)
            return String();
        return String(keey, end);
    }

    char const* ValueIteratorBase::memberName() const {
        const char* cname = (*current_).first.data();
        return cname ? cname : "";
    }

    char const* ValueIteratorBase::memberName(char const** end) const {
        const char* cname = (*current_).first.data();
        if (!cname) {
            *end = nullptr;
            return nullptr;
        }
        *end = cname + (*current_).first.length();
        return cname;
    }

    // //////////////////////////////////////////////////////////////////
    // //////////////////////////////////////////////////////////////////
    // //////////////////////////////////////////////////////////////////
    // class ValueConstIterator
    // //////////////////////////////////////////////////////////////////
    // //////////////////////////////////////////////////////////////////
    // //////////////////////////////////////////////////////////////////

    ValueConstIterator::ValueConstIterator() = default;

    ValueConstIterator::ValueConstIterator(
        const Value::ObjectValues::iterator& current)
        : ValueIteratorBase(current) {}

    ValueConstIterator::ValueConstIterator(ValueIterator const& other)
        : ValueIteratorBase(other) {}

    ValueConstIterator& ValueConstIterator::
        operator=(const ValueIteratorBase& other) {
        copy(other);
        return *this;
    }

    // //////////////////////////////////////////////////////////////////
    // //////////////////////////////////////////////////////////////////
    // //////////////////////////////////////////////////////////////////
    // class ValueIterator
    // //////////////////////////////////////////////////////////////////
    // //////////////////////////////////////////////////////////////////
    // //////////////////////////////////////////////////////////////////

    ValueIterator::ValueIterator() = default;

    ValueIterator::ValueIterator(const Value::ObjectValues::iterator& current)
        : ValueIteratorBase(current) {}

    ValueIterator::ValueIterator(const ValueConstIterator& other)
        : ValueIteratorBase(other) {
        throwRuntimeError("ConstIterator to Iterator should never be allowed.");
    }

    ValueIterator::ValueIterator(const ValueIterator& other) = default;

    ValueIterator& ValueIterator::operator=(const SelfType& other) {
        copy(other);
        return *this;
    }

} // namespace Json

// //////////////////////////////////////////////////////////////////////
// End of content of file: src/lib_json/json_valueiterator.inl
// //////////////////////////////////////////////////////////////////////






// //////////////////////////////////////////////////////////////////////
// Beginning of content of file: src/lib_json/json_value.cpp
// //////////////////////////////////////////////////////////////////////

// Copyright 2011 Baptiste Lepilleur and The JsonCpp Authors
// Distributed under MIT license, or public domain if desired and
// recognized in your jurisdiction.
// See file LICENSE for detail or copy at http://jsoncpp.sourceforge.net/LICENSE

#if !defined(JSON_IS_AMALGAMATION)
#include <json/assertions.h>
#include <json/value.h>
#include <json/writer.h>
#endif // if !defined(JSON_IS_AMALGAMATION)
#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <cstring>
#include <sstream>
#include <utility>

// Provide implementation equivalent of std::snprintf for older _MSC compilers
#if defined(_MSC_VER) && _MSC_VER < 1900
#include <stdarg.h>
static int msvc_pre1900_c99_vsnprintf(char* outBuf, size_t size,
    const char* format, va_list ap) {
    int count = -1;
    if (size != 0)
        count = _vsnprintf_s(outBuf, size, _TRUNCATE, format, ap);
    if (count == -1)
        count = _vscprintf(format, ap);
    return count;
}

int JSON_API msvc_pre1900_c99_snprintf(char* outBuf, size_t size,
    const char* format, ...) {
    va_list ap;
    va_start(ap, format);
    const int count = msvc_pre1900_c99_vsnprintf(outBuf, size, format, ap);
    va_end(ap);
    return count;
}
#endif

// Disable warning C4702 : unreachable code
#if defined(_MSC_VER)
#pragma warning(disable : 4702)
#endif

#define JSON_ASSERT_UNREACHABLE assert(false)

namespace Json {
    template <typename T>
    static std::unique_ptr<T> cloneUnique(const std::unique_ptr<T>& p) {
        std::unique_ptr<T> r;
        if (p) {
            r = std::unique_ptr<T>(new T(*p));
        }
        return r;
    }

    // This is a walkaround to avoid the static initialization of Value::null.
    // kNull must be word-aligned to avoid crashing on ARM.  We use an alignment of
    // 8 (instead of 4) as a bit of future-proofing.
#if defined(__ARMEL__)
#define ALIGNAS(byte_alignment) __attribute__((aligned(byte_alignment)))
#else
#define ALIGNAS(byte_alignment)
#endif

// static
    Value const& Value::nullSingleton() {
        static Value const nullStatic;
        return nullStatic;
    }

#if JSON_USE_NULLREF
    // for backwards compatibility, we'll leave these global references around, but
    // DO NOT use them in JSONCPP library code any more!
    // static
    Value const& Value::null = Value::nullSingleton();

    // static
    Value const& Value::nullRef = Value::nullSingleton();
#endif

#if !defined(JSON_USE_INT64_DOUBLE_CONVERSION)
    template <typename T, typename U>
    static inline bool InRange(double d, T min, U max) {
        // The casts can lose precision, but we are looking only for
        // an approximate range. Might fail on edge cases though. ~cdunn
        return d >= static_cast<double>(min) && d <= static_cast<double>(max);
    }
#else  // if !defined(JSON_USE_INT64_DOUBLE_CONVERSION)
    static inline double integerToDouble(Json::UInt64 value) {
        return static_cast<double>(Int64(value / 2)) * 2.0 +
            static_cast<double>(Int64(value & 1));
    }

    template <typename T> static inline double integerToDouble(T value) {
        return static_cast<double>(value);
    }

    template <typename T, typename U>
    static inline bool InRange(double d, T min, U max) {
        return d >= integerToDouble(min) && d <= integerToDouble(max);
    }
#endif // if !defined(JSON_USE_INT64_DOUBLE_CONVERSION)

    /** Duplicates the specified string value.
     * @param value Pointer to the string to duplicate. Must be zero-terminated if
     *              length is "unknown".
     * @param length Length of the value. if equals to unknown, then it will be
     *               computed using strlen(value).
     * @return Pointer on the duplicate instance of string.
     */
    static inline char* duplicateStringValue(const char* value, size_t length) {
        // Avoid an integer overflow in the call to malloc below by limiting length
        // to a sane value.
        if (length >= static_cast<size_t>(Value::maxInt))
            length = Value::maxInt - 1;

        auto newString = static_cast<char*>(malloc(length + 1));
        if (newString == nullptr) {
            throwRuntimeError("in Json::Value::duplicateStringValue(): "
                "Failed to allocate string value buffer");
        }
        memcpy(newString, value, length);
        newString[length] = 0;
        return newString;
    }

    /* Record the length as a prefix.
     */
    static inline char* duplicateAndPrefixStringValue(const char* value,
        unsigned int length) {
        // Avoid an integer overflow in the call to malloc below by limiting length
        // to a sane value.
        JSON_ASSERT_MESSAGE(length <= static_cast<unsigned>(Value::maxInt) -
            sizeof(unsigned) - 1U,
            "in Json::Value::duplicateAndPrefixStringValue(): "
            "length too big for prefixing");
        size_t actualLength = sizeof(length) + length + 1;
        auto newString = static_cast<char*>(malloc(actualLength));
        if (newString == nullptr) {
            throwRuntimeError("in Json::Value::duplicateAndPrefixStringValue(): "
                "Failed to allocate string value buffer");
        }
        *reinterpret_cast<unsigned*>(newString) = length;
        memcpy(newString + sizeof(unsigned), value, length);
        newString[actualLength - 1U] =
            0; // to avoid buffer over-run accidents by users later
        return newString;
    }
    inline static void decodePrefixedString(bool isPrefixed, char const* prefixed,
        unsigned* length, char const** value) {
        if (!isPrefixed) {
            *length = static_cast<unsigned>(strlen(prefixed));
            *value = prefixed;
        } else {
            *length = *reinterpret_cast<unsigned const*>(prefixed);
            *value = prefixed + sizeof(unsigned);
        }
    }
    /** Free the string duplicated by
     * duplicateStringValue()/duplicateAndPrefixStringValue().
     */
#if JSONCPP_USING_SECURE_MEMORY
    static inline void releasePrefixedStringValue(char* value) {
        unsigned length = 0;
        char const* valueDecoded;
        decodePrefixedString(true, value, &length, &valueDecoded);
        size_t const size = sizeof(unsigned) + length + 1U;
        memset(value, 0, size);
        free(value);
    }
    static inline void releaseStringValue(char* value, unsigned length) {
        // length==0 => we allocated the strings memory
        size_t size = (length == 0) ? strlen(value) : length;
        memset(value, 0, size);
        free(value);
    }
#else  // !JSONCPP_USING_SECURE_MEMORY
    static inline void releasePrefixedStringValue(char* value) { free(value); }
    static inline void releaseStringValue(char* value, unsigned) { free(value); }
#endif // JSONCPP_USING_SECURE_MEMORY

} // namespace Json

// //////////////////////////////////////////////////////////////////
// //////////////////////////////////////////////////////////////////
// //////////////////////////////////////////////////////////////////
// ValueInternals...
// //////////////////////////////////////////////////////////////////
// //////////////////////////////////////////////////////////////////
// //////////////////////////////////////////////////////////////////
#if !defined(JSON_IS_AMALGAMATION)

#include "json_valueiterator.inl"
#endif // if !defined(JSON_IS_AMALGAMATION)

namespace Json {

#if JSON_USE_EXCEPTION
    Exception::Exception(String msg) : msg_(std::move(msg)) {}
    Exception::~Exception() JSONCPP_NOEXCEPT = default;
    char const* Exception::what() const JSONCPP_NOEXCEPT { return msg_.c_str(); }
    RuntimeError::RuntimeError(String const& msg) : Exception(msg) {}
    LogicError::LogicError(String const& msg) : Exception(msg) {}
    JSONCPP_NORETURN void throwRuntimeError(String const& msg) {
        throw RuntimeError(msg);
    }
    JSONCPP_NORETURN void throwLogicError(String const& msg) {
        throw LogicError(msg);
    }
#else // !JSON_USE_EXCEPTION
    JSONCPP_NORETURN void throwRuntimeError(String const& msg) { abort(); }
    JSONCPP_NORETURN void throwLogicError(String const& msg) { abort(); }
#endif

    // //////////////////////////////////////////////////////////////////
    // //////////////////////////////////////////////////////////////////
    // //////////////////////////////////////////////////////////////////
    // class Value::CZString
    // //////////////////////////////////////////////////////////////////
    // //////////////////////////////////////////////////////////////////
    // //////////////////////////////////////////////////////////////////

    // Notes: policy_ indicates if the string was allocated when
    // a string is stored.

    Value::CZString::CZString(ArrayIndex index) : cstr_(nullptr), index_(index) {}

    Value::CZString::CZString(char const* str, unsigned length,
        DuplicationPolicy allocate)
        : cstr_(str) {
        // allocate != duplicate
        storage_.policy_ = allocate & 0x3;
        storage_.length_ = length & 0x3FFFFFFF;
    }

    Value::CZString::CZString(const CZString& other) {
        cstr_ = (other.storage_.policy_ != noDuplication && other.cstr_ != nullptr
            ? duplicateStringValue(other.cstr_, other.storage_.length_)
            : other.cstr_);
        storage_.policy_ =
            static_cast<unsigned>(
                other.cstr_
                ? (static_cast<DuplicationPolicy>(other.storage_.policy_) ==
                    noDuplication
                    ? noDuplication
                    : duplicate)
                : static_cast<DuplicationPolicy>(other.storage_.policy_)) &
            3U;
        storage_.length_ = other.storage_.length_;
    }

    Value::CZString::CZString(CZString&& other)
        : cstr_(other.cstr_), index_(other.index_) {
        other.cstr_ = nullptr;
    }

    Value::CZString::~CZString() {
        if (cstr_ && storage_.policy_ == duplicate) {
            releaseStringValue(const_cast<char*>(cstr_),
                storage_.length_ + 1U); // +1 for null terminating
                                        // character for sake of
                                        // completeness but not actually
                                        // necessary
        }
    }

    void Value::CZString::swap(CZString& other) {
        std::swap(cstr_, other.cstr_);
        std::swap(index_, other.index_);
    }

    Value::CZString& Value::CZString::operator=(const CZString& other) {
        cstr_ = other.cstr_;
        index_ = other.index_;
        return *this;
    }

    Value::CZString& Value::CZString::operator=(CZString&& other) {
        cstr_ = other.cstr_;
        index_ = other.index_;
        other.cstr_ = nullptr;
        return *this;
    }

    bool Value::CZString::operator<(const CZString& other) const {
        if (!cstr_)
            return index_ < other.index_;
        // return strcmp(cstr_, other.cstr_) < 0;
        // Assume both are strings.
        unsigned this_len = this->storage_.length_;
        unsigned other_len = other.storage_.length_;
        unsigned min_len = std::min<unsigned>(this_len, other_len);
        JSON_ASSERT(this->cstr_ && other.cstr_);
        int comp = memcmp(this->cstr_, other.cstr_, min_len);
        if (comp < 0)
            return true;
        if (comp > 0)
            return false;
        return (this_len < other_len);
    }

    bool Value::CZString::operator==(const CZString& other) const {
        if (!cstr_)
            return index_ == other.index_;
        // return strcmp(cstr_, other.cstr_) == 0;
        // Assume both are strings.
        unsigned this_len = this->storage_.length_;
        unsigned other_len = other.storage_.length_;
        if (this_len != other_len)
            return false;
        JSON_ASSERT(this->cstr_ && other.cstr_);
        int comp = memcmp(this->cstr_, other.cstr_, this_len);
        return comp == 0;
    }

    ArrayIndex Value::CZString::index() const { return index_; }

    // const char* Value::CZString::c_str() const { return cstr_; }
    const char* Value::CZString::data() const { return cstr_; }
    unsigned Value::CZString::length() const { return storage_.length_; }
    bool Value::CZString::isStaticString() const {
        return storage_.policy_ == noDuplication;
    }

    // //////////////////////////////////////////////////////////////////
    // //////////////////////////////////////////////////////////////////
    // //////////////////////////////////////////////////////////////////
    // class Value::Value
    // //////////////////////////////////////////////////////////////////
    // //////////////////////////////////////////////////////////////////
    // //////////////////////////////////////////////////////////////////

    /*! \internal Default constructor initialization must be equivalent to:
     * memset( this, 0, sizeof(Value) )
     * This optimization is used in ValueInternalMap fast allocator.
     */
    Value::Value(ValueType type) {
        static char const emptyString[] = "";
        initBasic(type);
        switch (type) {
        case nullValue:
            break;
        case intValue:
        case uintValue:
            value_.int_ = 0;
            break;
        case realValue:
            value_.real_ = 0.0;
            break;
        case stringValue:
            // allocated_ == false, so this is safe.
            value_.string_ = const_cast<char*>(static_cast<char const*>(emptyString));
            break;
        case arrayValue:
        case objectValue:
            value_.map_ = new ObjectValues();
            break;
        case booleanValue:
            value_.bool_ = false;
            break;
        default:
            JSON_ASSERT_UNREACHABLE;
        }
    }

    Value::Value(Int value) {
        initBasic(intValue);
        value_.int_ = value;
    }

    Value::Value(UInt value) {
        initBasic(uintValue);
        value_.uint_ = value;
    }
#if defined(JSON_HAS_INT64)
    Value::Value(Int64 value) {
        initBasic(intValue);
        value_.int_ = value;
    }
    Value::Value(UInt64 value) {
        initBasic(uintValue);
        value_.uint_ = value;
    }
#endif // defined(JSON_HAS_INT64)

    Value::Value(double value) {
        initBasic(realValue);
        value_.real_ = value;
    }

    Value::Value(const char* value) {
        initBasic(stringValue, true);
        JSON_ASSERT_MESSAGE(value != nullptr,
            "Null Value Passed to Value Constructor");
        value_.string_ = duplicateAndPrefixStringValue(
            value, static_cast<unsigned>(strlen(value)));
    }

    Value::Value(const char* begin, const char* end) {
        initBasic(stringValue, true);
        value_.string_ =
            duplicateAndPrefixStringValue(begin, static_cast<unsigned>(end - begin));
    }

    Value::Value(const String& value) {
        initBasic(stringValue, true);
        value_.string_ = duplicateAndPrefixStringValue(
            value.data(), static_cast<unsigned>(value.length()));
    }

    Value::Value(const StaticString& value) {
        initBasic(stringValue);
        value_.string_ = const_cast<char*>(value.c_str());
    }

    Value::Value(bool value) {
        initBasic(booleanValue);
        value_.bool_ = value;
    }

    Value::Value(const Value& other) {
        dupPayload(other);
        dupMeta(other);
    }

    Value::Value(Value&& other) {
        initBasic(nullValue);
        swap(other);
    }

    Value::~Value() {
        releasePayload();
        value_.uint_ = 0;
    }

    Value& Value::operator=(const Value& other) {
        Value(other).swap(*this);
        return *this;
    }

    Value& Value::operator=(Value&& other) {
        other.swap(*this);
        return *this;
    }

    void Value::swapPayload(Value& other) {
        std::swap(bits_, other.bits_);
        std::swap(value_, other.value_);
    }

    void Value::copyPayload(const Value& other) {
        releasePayload();
        dupPayload(other);
    }

    void Value::swap(Value& other) {
        swapPayload(other);
        std::swap(comments_, other.comments_);
        std::swap(start_, other.start_);
        std::swap(limit_, other.limit_);
    }

    void Value::copy(const Value& other) {
        copyPayload(other);
        dupMeta(other);
    }

    ValueType Value::type() const {
        return static_cast<ValueType>(bits_.value_type_);
    }

    int Value::compare(const Value& other) const {
        if (*this < other)
            return -1;
        if (*this > other)
            return 1;
        return 0;
    }

    bool Value::operator<(const Value& other) const {
        int typeDelta = type() - other.type();
        if (typeDelta)
            return typeDelta < 0;
        switch (type()) {
        case nullValue:
            return false;
        case intValue:
            return value_.int_ < other.value_.int_;
        case uintValue:
            return value_.uint_ < other.value_.uint_;
        case realValue:
            return value_.real_ < other.value_.real_;
        case booleanValue:
            return value_.bool_ < other.value_.bool_;
        case stringValue: {
            if ((value_.string_ == nullptr) || (other.value_.string_ == nullptr)) {
                return other.value_.string_ != nullptr;
            }
            unsigned this_len;
            unsigned other_len;
            char const* this_str;
            char const* other_str;
            decodePrefixedString(this->isAllocated(), this->value_.string_, &this_len,
                &this_str);
            decodePrefixedString(other.isAllocated(), other.value_.string_, &other_len,
                &other_str);
            unsigned min_len = std::min<unsigned>(this_len, other_len);
            JSON_ASSERT(this_str && other_str);
            int comp = memcmp(this_str, other_str, min_len);
            if (comp < 0)
                return true;
            if (comp > 0)
                return false;
            return (this_len < other_len);
        }
        case arrayValue:
        case objectValue: {
            auto thisSize = value_.map_->size();
            auto otherSize = other.value_.map_->size();
            if (thisSize != otherSize)
                return thisSize < otherSize;
            return (*value_.map_) < (*other.value_.map_);
        }
        default:
            JSON_ASSERT_UNREACHABLE;
        }
        return false; // unreachable
    }

    bool Value::operator<=(const Value& other) const { return !(other < *this); }

    bool Value::operator>=(const Value& other) const { return !(*this < other); }

    bool Value::operator>(const Value& other) const { return other < *this; }

    bool Value::operator==(const Value& other) const {
        if (type() != other.type())
            return false;
        switch (type()) {
        case nullValue:
            return true;
        case intValue:
            return value_.int_ == other.value_.int_;
        case uintValue:
            return value_.uint_ == other.value_.uint_;
        case realValue:
            return value_.real_ == other.value_.real_;
        case booleanValue:
            return value_.bool_ == other.value_.bool_;
        case stringValue: {
            if ((value_.string_ == nullptr) || (other.value_.string_ == nullptr)) {
                return (value_.string_ == other.value_.string_);
            }
            unsigned this_len;
            unsigned other_len;
            char const* this_str;
            char const* other_str;
            decodePrefixedString(this->isAllocated(), this->value_.string_, &this_len,
                &this_str);
            decodePrefixedString(other.isAllocated(), other.value_.string_, &other_len,
                &other_str);
            if (this_len != other_len)
                return false;
            JSON_ASSERT(this_str && other_str);
            int comp = memcmp(this_str, other_str, this_len);
            return comp == 0;
        }
        case arrayValue:
        case objectValue:
            return value_.map_->size() == other.value_.map_->size() &&
                (*value_.map_) == (*other.value_.map_);
        default:
            JSON_ASSERT_UNREACHABLE;
        }
        return false; // unreachable
    }

    bool Value::operator!=(const Value& other) const { return !(*this == other); }

    const char* Value::asCString() const {
        JSON_ASSERT_MESSAGE(type() == stringValue,
            "in Json::Value::asCString(): requires stringValue");
        if (value_.string_ == nullptr)
            return nullptr;
        unsigned this_len;
        char const* this_str;
        decodePrefixedString(this->isAllocated(), this->value_.string_, &this_len,
            &this_str);
        return this_str;
    }

#if JSONCPP_USING_SECURE_MEMORY
    unsigned Value::getCStringLength() const {
        JSON_ASSERT_MESSAGE(type() == stringValue,
            "in Json::Value::asCString(): requires stringValue");
        if (value_.string_ == 0)
            return 0;
        unsigned this_len;
        char const* this_str;
        decodePrefixedString(this->isAllocated(), this->value_.string_, &this_len,
            &this_str);
        return this_len;
    }
#endif

    bool Value::getString(char const** begin, char const** end) const {
        if (type() != stringValue)
            return false;
        if (value_.string_ == nullptr)
            return false;
        unsigned length;
        decodePrefixedString(this->isAllocated(), this->value_.string_, &length,
            begin);
        *end = *begin + length;
        return true;
    }

    String Value::asString() const {
        switch (type()) {
        case nullValue:
            return "";
        case stringValue: {
            if (value_.string_ == nullptr)
                return "";
            unsigned this_len;
            char const* this_str;
            decodePrefixedString(this->isAllocated(), this->value_.string_, &this_len,
                &this_str);
            return String(this_str, this_len);
        }
        case booleanValue:
            return value_.bool_ ? "true" : "false";
        case intValue:
            return valueToString(value_.int_);
        case uintValue:
            return valueToString(value_.uint_);
        case realValue:
            return valueToString(value_.real_);
        default:
            JSON_FAIL_MESSAGE("Type is not convertible to string");
        }
    }

    Value::Int Value::asInt() const {
        switch (type()) {
        case intValue:
            JSON_ASSERT_MESSAGE(isInt(), "LargestInt out of Int range");
            return Int(value_.int_);
        case uintValue:
            JSON_ASSERT_MESSAGE(isInt(), "LargestUInt out of Int range");
            return Int(value_.uint_);
        case realValue:
            JSON_ASSERT_MESSAGE(InRange(value_.real_, minInt, maxInt),
                "double out of Int range");
            return Int(value_.real_);
        case nullValue:
            return 0;
        case booleanValue:
            return value_.bool_ ? 1 : 0;
        default:
            break;
        }
        JSON_FAIL_MESSAGE("Value is not convertible to Int.");
    }

    Value::UInt Value::asUInt() const {
        switch (type()) {
        case intValue:
            JSON_ASSERT_MESSAGE(isUInt(), "LargestInt out of UInt range");
            return UInt(value_.int_);
        case uintValue:
            JSON_ASSERT_MESSAGE(isUInt(), "LargestUInt out of UInt range");
            return UInt(value_.uint_);
        case realValue:
            JSON_ASSERT_MESSAGE(InRange(value_.real_, 0, maxUInt),
                "double out of UInt range");
            return UInt(value_.real_);
        case nullValue:
            return 0;
        case booleanValue:
            return value_.bool_ ? 1 : 0;
        default:
            break;
        }
        JSON_FAIL_MESSAGE("Value is not convertible to UInt.");
    }

#if defined(JSON_HAS_INT64)

    Value::Int64 Value::asInt64() const {
        switch (type()) {
        case intValue:
            return Int64(value_.int_);
        case uintValue:
            JSON_ASSERT_MESSAGE(isInt64(), "LargestUInt out of Int64 range");
            return Int64(value_.uint_);
        case realValue:
            JSON_ASSERT_MESSAGE(InRange(value_.real_, minInt64, maxInt64),
                "double out of Int64 range");
            return Int64(value_.real_);
        case nullValue:
            return 0;
        case booleanValue:
            return value_.bool_ ? 1 : 0;
        default:
            break;
        }
        JSON_FAIL_MESSAGE("Value is not convertible to Int64.");
    }

    Value::UInt64 Value::asUInt64() const {
        switch (type()) {
        case intValue:
            JSON_ASSERT_MESSAGE(isUInt64(), "LargestInt out of UInt64 range");
            return UInt64(value_.int_);
        case uintValue:
            return UInt64(value_.uint_);
        case realValue:
            JSON_ASSERT_MESSAGE(InRange(value_.real_, 0, maxUInt64),
                "double out of UInt64 range");
            return UInt64(value_.real_);
        case nullValue:
            return 0;
        case booleanValue:
            return value_.bool_ ? 1 : 0;
        default:
            break;
        }
        JSON_FAIL_MESSAGE("Value is not convertible to UInt64.");
    }
#endif // if defined(JSON_HAS_INT64)

    LargestInt Value::asLargestInt() const {
#if defined(JSON_NO_INT64)
        return asInt();
#else
        return asInt64();
#endif
    }

    LargestUInt Value::asLargestUInt() const {
#if defined(JSON_NO_INT64)
        return asUInt();
#else
        return asUInt64();
#endif
    }

    double Value::asDouble() const {
        switch (type()) {
        case intValue:
            return static_cast<double>(value_.int_);
        case uintValue:
#if !defined(JSON_USE_INT64_DOUBLE_CONVERSION)
            return static_cast<double>(value_.uint_);
#else  // if !defined(JSON_USE_INT64_DOUBLE_CONVERSION)
            return integerToDouble(value_.uint_);
#endif // if !defined(JSON_USE_INT64_DOUBLE_CONVERSION)
        case realValue:
            return value_.real_;
        case nullValue:
            return 0.0;
        case booleanValue:
            return value_.bool_ ? 1.0 : 0.0;
        default:
            break;
        }
        JSON_FAIL_MESSAGE("Value is not convertible to double.");
    }

    float Value::asFloat() const {
        switch (type()) {
        case intValue:
            return static_cast<float>(value_.int_);
        case uintValue:
#if !defined(JSON_USE_INT64_DOUBLE_CONVERSION)
            return static_cast<float>(value_.uint_);
#else  // if !defined(JSON_USE_INT64_DOUBLE_CONVERSION)
            // This can fail (silently?) if the value is bigger than MAX_FLOAT.
            return static_cast<float>(integerToDouble(value_.uint_));
#endif // if !defined(JSON_USE_INT64_DOUBLE_CONVERSION)
        case realValue:
            return static_cast<float>(value_.real_);
        case nullValue:
            return 0.0;
        case booleanValue:
            return value_.bool_ ? 1.0F : 0.0F;
        default:
            break;
        }
        JSON_FAIL_MESSAGE("Value is not convertible to float.");
    }

    bool Value::asBool() const {
        switch (type()) {
        case booleanValue:
            return value_.bool_;
        case nullValue:
            return false;
        case intValue:
            return value_.int_ != 0;
        case uintValue:
            return value_.uint_ != 0;
        case realValue: {
            // According to JavaScript language zero or NaN is regarded as false
            const auto value_classification = std::fpclassify(value_.real_);
            return value_classification != FP_ZERO && value_classification != FP_NAN;
        }
        default:
            break;
        }
        JSON_FAIL_MESSAGE("Value is not convertible to bool.");
    }

    bool Value::isConvertibleTo(ValueType other) const {
        switch (other) {
        case nullValue:
            return (isNumeric() && asDouble() == 0.0) ||
                (type() == booleanValue && !value_.bool_) ||
                (type() == stringValue && asString().empty()) ||
                (type() == arrayValue && value_.map_->empty()) ||
                (type() == objectValue && value_.map_->empty()) ||
                type() == nullValue;
        case intValue:
            return isInt() ||
                (type() == realValue && InRange(value_.real_, minInt, maxInt)) ||
                type() == booleanValue || type() == nullValue;
        case uintValue:
            return isUInt() ||
                (type() == realValue && InRange(value_.real_, 0, maxUInt)) ||
                type() == booleanValue || type() == nullValue;
        case realValue:
            return isNumeric() || type() == booleanValue || type() == nullValue;
        case booleanValue:
            return isNumeric() || type() == booleanValue || type() == nullValue;
        case stringValue:
            return isNumeric() || type() == booleanValue || type() == stringValue ||
                type() == nullValue;
        case arrayValue:
            return type() == arrayValue || type() == nullValue;
        case objectValue:
            return type() == objectValue || type() == nullValue;
        }
        JSON_ASSERT_UNREACHABLE;
        return false;
    }

    /// Number of values in array or object
    ArrayIndex Value::size() const {
        switch (type()) {
        case nullValue:
        case intValue:
        case uintValue:
        case realValue:
        case booleanValue:
        case stringValue:
            return 0;
        case arrayValue: // size of the array is highest index + 1
            if (!value_.map_->empty()) {
                ObjectValues::const_iterator itLast = value_.map_->end();
                --itLast;
                return (*itLast).first.index() + 1;
            }
            return 0;
        case objectValue:
            return ArrayIndex(value_.map_->size());
        }
        JSON_ASSERT_UNREACHABLE;
        return 0; // unreachable;
    }

    bool Value::empty() const {
        if (isNull() || isArray() || isObject())
            return size() == 0U;
        return false;
    }

    Value::operator bool() const { return !isNull(); }

    void Value::clear() {
        JSON_ASSERT_MESSAGE(type() == nullValue || type() == arrayValue ||
            type() == objectValue,
            "in Json::Value::clear(): requires complex value");
        start_ = 0;
        limit_ = 0;
        switch (type()) {
        case arrayValue:
        case objectValue:
            value_.map_->clear();
            break;
        default:
            break;
        }
    }

    void Value::resize(ArrayIndex newSize) {
        JSON_ASSERT_MESSAGE(type() == nullValue || type() == arrayValue,
            "in Json::Value::resize(): requires arrayValue");
        if (type() == nullValue)
            *this = Value(arrayValue);
        ArrayIndex oldSize = size();
        if (newSize == 0)
            clear();
        else if (newSize > oldSize)
            this->operator[](newSize - 1);
        else {
            for (ArrayIndex index = newSize; index < oldSize; ++index) {
                value_.map_->erase(index);
            }
            JSON_ASSERT(size() == newSize);
        }
    }

    Value& Value::operator[](ArrayIndex index) {
        JSON_ASSERT_MESSAGE(
            type() == nullValue || type() == arrayValue,
            "in Json::Value::operator[](ArrayIndex): requires arrayValue");
        if (type() == nullValue)
            *this = Value(arrayValue);
        CZString key(index);
        auto it = value_.map_->lower_bound(key);
        if (it != value_.map_->end() && (*it).first == key)
            return (*it).second;

        ObjectValues::value_type defaultValue(key, nullSingleton());
        it = value_.map_->insert(it, defaultValue);
        return (*it).second;
    }

    Value& Value::operator[](int index) {
        JSON_ASSERT_MESSAGE(
            index >= 0,
            "in Json::Value::operator[](int index): index cannot be negative");
        return (*this)[ArrayIndex(index)];
    }

    const Value& Value::operator[](ArrayIndex index) const {
        JSON_ASSERT_MESSAGE(
            type() == nullValue || type() == arrayValue,
            "in Json::Value::operator[](ArrayIndex)const: requires arrayValue");
        if (type() == nullValue)
            return nullSingleton();
        CZString key(index);
        ObjectValues::const_iterator it = value_.map_->find(key);
        if (it == value_.map_->end())
            return nullSingleton();
        return (*it).second;
    }

    const Value& Value::operator[](int index) const {
        JSON_ASSERT_MESSAGE(
            index >= 0,
            "in Json::Value::operator[](int index) const: index cannot be negative");
        return (*this)[ArrayIndex(index)];
    }

    void Value::initBasic(ValueType type, bool allocated) {
        setType(type);
        setIsAllocated(allocated);
        comments_ = Comments{};
        start_ = 0;
        limit_ = 0;
    }

    void Value::dupPayload(const Value& other) {
        setType(other.type());
        setIsAllocated(false);
        switch (type()) {
        case nullValue:
        case intValue:
        case uintValue:
        case realValue:
        case booleanValue:
            value_ = other.value_;
            break;
        case stringValue:
            if (other.value_.string_ && other.isAllocated()) {
                unsigned len;
                char const* str;
                decodePrefixedString(other.isAllocated(), other.value_.string_, &len,
                    &str);
                value_.string_ = duplicateAndPrefixStringValue(str, len);
                setIsAllocated(true);
            } else {
                value_.string_ = other.value_.string_;
            }
            break;
        case arrayValue:
        case objectValue:
            value_.map_ = new ObjectValues(*other.value_.map_);
            break;
        default:
            JSON_ASSERT_UNREACHABLE;
        }
    }

    void Value::releasePayload() {
        switch (type()) {
        case nullValue:
        case intValue:
        case uintValue:
        case realValue:
        case booleanValue:
            break;
        case stringValue:
            if (isAllocated())
                releasePrefixedStringValue(value_.string_);
            break;
        case arrayValue:
        case objectValue:
            delete value_.map_;
            break;
        default:
            JSON_ASSERT_UNREACHABLE;
        }
    }

    void Value::dupMeta(const Value& other) {
        comments_ = other.comments_;
        start_ = other.start_;
        limit_ = other.limit_;
    }

    // Access an object value by name, create a null member if it does not exist.
    // @pre Type of '*this' is object or null.
    // @param key is null-terminated.
    Value& Value::resolveReference(const char* key) {
        JSON_ASSERT_MESSAGE(
            type() == nullValue || type() == objectValue,
            "in Json::Value::resolveReference(): requires objectValue");
        if (type() == nullValue)
            *this = Value(objectValue);
        CZString actualKey(key, static_cast<unsigned>(strlen(key)),
            CZString::noDuplication); // NOTE!
        auto it = value_.map_->lower_bound(actualKey);
        if (it != value_.map_->end() && (*it).first == actualKey)
            return (*it).second;

        ObjectValues::value_type defaultValue(actualKey, nullSingleton());
        it = value_.map_->insert(it, defaultValue);
        Value& value = (*it).second;
        return value;
    }

    // @param key is not null-terminated.
    Value& Value::resolveReference(char const* key, char const* end) {
        JSON_ASSERT_MESSAGE(
            type() == nullValue || type() == objectValue,
            "in Json::Value::resolveReference(key, end): requires objectValue");
        if (type() == nullValue)
            *this = Value(objectValue);
        CZString actualKey(key, static_cast<unsigned>(end - key),
            CZString::duplicateOnCopy);
        auto it = value_.map_->lower_bound(actualKey);
        if (it != value_.map_->end() && (*it).first == actualKey)
            return (*it).second;

        ObjectValues::value_type defaultValue(actualKey, nullSingleton());
        it = value_.map_->insert(it, defaultValue);
        Value& value = (*it).second;
        return value;
    }

    Value Value::get(ArrayIndex index, const Value& defaultValue) const {
        const Value* value = &((*this)[index]);
        return value == &nullSingleton() ? defaultValue : *value;
    }

    bool Value::isValidIndex(ArrayIndex index) const { return index < size(); }

    Value const* Value::find(char const* begin, char const* end) const {
        JSON_ASSERT_MESSAGE(type() == nullValue || type() == objectValue,
            "in Json::Value::find(begin, end): requires "
            "objectValue or nullValue");
        if (type() == nullValue)
            return nullptr;
        CZString actualKey(begin, static_cast<unsigned>(end - begin),
            CZString::noDuplication);
        ObjectValues::const_iterator it = value_.map_->find(actualKey);
        if (it == value_.map_->end())
            return nullptr;
        return &(*it).second;
    }
    Value* Value::demand(char const* begin, char const* end) {
        JSON_ASSERT_MESSAGE(type() == nullValue || type() == objectValue,
            "in Json::Value::demand(begin, end): requires "
            "objectValue or nullValue");
        return &resolveReference(begin, end);
    }
    const Value& Value::operator[](const char* key) const {
        Value const* found = find(key, key + strlen(key));
        if (!found)
            return nullSingleton();
        return *found;
    }
    Value const& Value::operator[](const String& key) const {
        Value const* found = find(key.data(), key.data() + key.length());
        if (!found)
            return nullSingleton();
        return *found;
    }

    Value& Value::operator[](const char* key) {
        return resolveReference(key, key + strlen(key));
    }

    Value& Value::operator[](const String& key) {
        return resolveReference(key.data(), key.data() + key.length());
    }

    Value& Value::operator[](const StaticString& key) {
        return resolveReference(key.c_str());
    }

    Value& Value::append(const Value& value) { return append(Value(value)); }

    Value& Value::append(Value&& value) {
        JSON_ASSERT_MESSAGE(type() == nullValue || type() == arrayValue,
            "in Json::Value::append: requires arrayValue");
        if (type() == nullValue) {
            *this = Value(arrayValue);
        }
        return this->value_.map_->emplace(size(), std::move(value)).first->second;
    }

    bool Value::insert(ArrayIndex index, const Value& newValue) {
        return insert(index, Value(newValue));
    }

    bool Value::insert(ArrayIndex index, Value&& newValue) {
        JSON_ASSERT_MESSAGE(type() == nullValue || type() == arrayValue,
            "in Json::Value::insert: requires arrayValue");
        ArrayIndex length = size();
        if (index > length) {
            return false;
        }
        for (ArrayIndex i = length; i > index; i--) {
            (*this)[i] = std::move((*this)[i - 1]);
        }
        (*this)[index] = std::move(newValue);
        return true;
    }

    Value Value::get(char const* begin, char const* end,
        Value const& defaultValue) const {
        Value const* found = find(begin, end);
        return !found ? defaultValue : *found;
    }
    Value Value::get(char const* key, Value const& defaultValue) const {
        return get(key, key + strlen(key), defaultValue);
    }
    Value Value::get(String const& key, Value const& defaultValue) const {
        return get(key.data(), key.data() + key.length(), defaultValue);
    }

    bool Value::removeMember(const char* begin, const char* end, Value* removed) {
        if (type() != objectValue) {
            return false;
        }
        CZString actualKey(begin, static_cast<unsigned>(end - begin),
            CZString::noDuplication);
        auto it = value_.map_->find(actualKey);
        if (it == value_.map_->end())
            return false;
        if (removed)
            *removed = std::move(it->second);
        value_.map_->erase(it);
        return true;
    }
    bool Value::removeMember(const char* key, Value* removed) {
        return removeMember(key, key + strlen(key), removed);
    }
    bool Value::removeMember(String const& key, Value* removed) {
        return removeMember(key.data(), key.data() + key.length(), removed);
    }
    void Value::removeMember(const char* key) {
        JSON_ASSERT_MESSAGE(type() == nullValue || type() == objectValue,
            "in Json::Value::removeMember(): requires objectValue");
        if (type() == nullValue)
            return;

        CZString actualKey(key, unsigned(strlen(key)), CZString::noDuplication);
        value_.map_->erase(actualKey);
    }
    void Value::removeMember(const String& key) { removeMember(key.c_str()); }

    bool Value::removeIndex(ArrayIndex index, Value* removed) {
        if (type() != arrayValue) {
            return false;
        }
        CZString key(index);
        auto it = value_.map_->find(key);
        if (it == value_.map_->end()) {
            return false;
        }
        if (removed)
            *removed = it->second;
        ArrayIndex oldSize = size();
        // shift left all items left, into the place of the "removed"
        for (ArrayIndex i = index; i < (oldSize - 1); ++i) {
            CZString keey(i);
            (*value_.map_)[keey] = (*this)[i + 1];
        }
        // erase the last one ("leftover")
        CZString keyLast(oldSize - 1);
        auto itLast = value_.map_->find(keyLast);
        value_.map_->erase(itLast);
        return true;
    }

    bool Value::isMember(char const* begin, char const* end) const {
        Value const* value = find(begin, end);
        return nullptr != value;
    }
    bool Value::isMember(char const* key) const {
        return isMember(key, key + strlen(key));
    }
    bool Value::isMember(String const& key) const {
        return isMember(key.data(), key.data() + key.length());
    }

    Value::Members Value::getMemberNames() const {
        JSON_ASSERT_MESSAGE(
            type() == nullValue || type() == objectValue,
            "in Json::Value::getMemberNames(), value must be objectValue");
        if (type() == nullValue)
            return Value::Members();
        Members members;
        members.reserve(value_.map_->size());
        ObjectValues::const_iterator it = value_.map_->begin();
        ObjectValues::const_iterator itEnd = value_.map_->end();
        for (; it != itEnd; ++it) {
            members.push_back(String((*it).first.data(), (*it).first.length()));
        }
        return members;
    }

    static bool IsIntegral(double d) {
        double integral_part;
        return modf(d, &integral_part) == 0.0;
    }

    bool Value::isNull() const { return type() == nullValue; }

    bool Value::isBool() const { return type() == booleanValue; }

    bool Value::isInt() const {
        switch (type()) {
        case intValue:
#if defined(JSON_HAS_INT64)
            return value_.int_ >= minInt && value_.int_ <= maxInt;
#else
            return true;
#endif
        case uintValue:
            return value_.uint_ <= UInt(maxInt);
        case realValue:
            return value_.real_ >= minInt && value_.real_ <= maxInt &&
                IsIntegral(value_.real_);
        default:
            break;
        }
        return false;
    }

    bool Value::isUInt() const {
        switch (type()) {
        case intValue:
#if defined(JSON_HAS_INT64)
            return value_.int_ >= 0 && LargestUInt(value_.int_) <= LargestUInt(maxUInt);
#else
            return value_.int_ >= 0;
#endif
        case uintValue:
#if defined(JSON_HAS_INT64)
            return value_.uint_ <= maxUInt;
#else
            return true;
#endif
        case realValue:
            return value_.real_ >= 0 && value_.real_ <= maxUInt &&
                IsIntegral(value_.real_);
        default:
            break;
        }
        return false;
    }

    bool Value::isInt64() const {
#if defined(JSON_HAS_INT64)
        switch (type()) {
        case intValue:
            return true;
        case uintValue:
            return value_.uint_ <= UInt64(maxInt64);
        case realValue:
            // Note that maxInt64 (= 2^63 - 1) is not exactly representable as a
            // double, so double(maxInt64) will be rounded up to 2^63. Therefore we
            // require the value to be strictly less than the limit.
            return value_.real_ >= double(minInt64) &&
                value_.real_ < double(maxInt64) && IsIntegral(value_.real_);
        default:
            break;
        }
#endif // JSON_HAS_INT64
        return false;
    }

    bool Value::isUInt64() const {
#if defined(JSON_HAS_INT64)
        switch (type()) {
        case intValue:
            return value_.int_ >= 0;
        case uintValue:
            return true;
        case realValue:
            // Note that maxUInt64 (= 2^64 - 1) is not exactly representable as a
            // double, so double(maxUInt64) will be rounded up to 2^64. Therefore we
            // require the value to be strictly less than the limit.
            return value_.real_ >= 0 && value_.real_ < maxUInt64AsDouble &&
                IsIntegral(value_.real_);
        default:
            break;
        }
#endif // JSON_HAS_INT64
        return false;
    }

    bool Value::isIntegral() const {
        switch (type()) {
        case intValue:
        case uintValue:
            return true;
        case realValue:
#if defined(JSON_HAS_INT64)
            // Note that maxUInt64 (= 2^64 - 1) is not exactly representable as a
            // double, so double(maxUInt64) will be rounded up to 2^64. Therefore we
            // require the value to be strictly less than the limit.
            return value_.real_ >= double(minInt64) &&
                value_.real_ < maxUInt64AsDouble && IsIntegral(value_.real_);
#else
            return value_.real_ >= minInt && value_.real_ <= maxUInt &&
                IsIntegral(value_.real_);
#endif // JSON_HAS_INT64
        default:
            break;
        }
        return false;
    }

    bool Value::isDouble() const {
        return type() == intValue || type() == uintValue || type() == realValue;
    }

    bool Value::isNumeric() const { return isDouble(); }

    bool Value::isString() const { return type() == stringValue; }

    bool Value::isArray() const { return type() == arrayValue; }

    bool Value::isObject() const { return type() == objectValue; }

    Value::Comments::Comments(const Comments& that)
        : ptr_{ cloneUnique(that.ptr_) } {}

    Value::Comments::Comments(Comments&& that) : ptr_{ std::move(that.ptr_) } {}

    Value::Comments& Value::Comments::operator=(const Comments& that) {
        ptr_ = cloneUnique(that.ptr_);
        return *this;
    }

    Value::Comments& Value::Comments::operator=(Comments&& that) {
        ptr_ = std::move(that.ptr_);
        return *this;
    }

    bool Value::Comments::has(CommentPlacement slot) const {
        return ptr_ && !(*ptr_)[slot].empty();
    }

    String Value::Comments::get(CommentPlacement slot) const {
        if (!ptr_)
            return {};
        return (*ptr_)[slot];
    }

    void Value::Comments::set(CommentPlacement slot, String comment) {
        if (!ptr_) {
            ptr_ = std::unique_ptr<Array>(new Array());
        }
        // check comments array boundry.
        if (slot < CommentPlacement::numberOfCommentPlacement) {
            (*ptr_)[slot] = std::move(comment);
        }
    }

    void Value::setComment(String comment, CommentPlacement placement) {
        if (!comment.empty() && (comment.back() == '\n')) {
            // Always discard trailing newline, to aid indentation.
            comment.pop_back();
        }
        JSON_ASSERT(!comment.empty());
        JSON_ASSERT_MESSAGE(
            comment[0] == '\0' || comment[0] == '/',
            "in Json::Value::setComment(): Comments must start with /");
        comments_.set(placement, std::move(comment));
    }

    bool Value::hasComment(CommentPlacement placement) const {
        return comments_.has(placement);
    }

    String Value::getComment(CommentPlacement placement) const {
        return comments_.get(placement);
    }

    void Value::setOffsetStart(ptrdiff_t start) { start_ = start; }

    void Value::setOffsetLimit(ptrdiff_t limit) { limit_ = limit; }

    ptrdiff_t Value::getOffsetStart() const { return start_; }

    ptrdiff_t Value::getOffsetLimit() const { return limit_; }

    String Value::toStyledString() const {
        StreamWriterBuilder builder;

        String out = this->hasComment(commentBefore) ? "\n" : "";
        out += Json::writeString(builder, *this);
        out += '\n';

        return out;
    }

    Value::const_iterator Value::begin() const {
        switch (type()) {
        case arrayValue:
        case objectValue:
            if (value_.map_)
                return const_iterator(value_.map_->begin());
            break;
        default:
            break;
        }
        return {};
    }

    Value::const_iterator Value::end() const {
        switch (type()) {
        case arrayValue:
        case objectValue:
            if (value_.map_)
                return const_iterator(value_.map_->end());
            break;
        default:
            break;
        }
        return {};
    }

    Value::iterator Value::begin() {
        switch (type()) {
        case arrayValue:
        case objectValue:
            if (value_.map_)
                return iterator(value_.map_->begin());
            break;
        default:
            break;
        }
        return iterator();
    }

    Value::iterator Value::end() {
        switch (type()) {
        case arrayValue:
        case objectValue:
            if (value_.map_)
                return iterator(value_.map_->end());
            break;
        default:
            break;
        }
        return iterator();
    }

    // class PathArgument
    // //////////////////////////////////////////////////////////////////

    PathArgument::PathArgument() = default;

    PathArgument::PathArgument(ArrayIndex index)
        : index_(index), kind_(kindIndex) {}

    PathArgument::PathArgument(const char* key) : key_(key), kind_(kindKey) {}

    PathArgument::PathArgument(String key) : key_(std::move(key)), kind_(kindKey) {}

    // class Path
    // //////////////////////////////////////////////////////////////////

    Path::Path(const String& path, const PathArgument& a1, const PathArgument& a2,
        const PathArgument& a3, const PathArgument& a4,
        const PathArgument& a5) {
        InArgs in;
        in.reserve(5);
        in.push_back(&a1);
        in.push_back(&a2);
        in.push_back(&a3);
        in.push_back(&a4);
        in.push_back(&a5);
        makePath(path, in);
    }

    void Path::makePath(const String& path, const InArgs& in) {
        const char* current = path.c_str();
        const char* end = current + path.length();
        auto itInArg = in.begin();
        while (current != end) {
            if (*current == '[') {
                ++current;
                if (*current == '%')
                    addPathInArg(path, in, itInArg, PathArgument::kindIndex);
                else {
                    ArrayIndex index = 0;
                    for (; current != end && *current >= '0' && *current <= '9'; ++current)
                        index = index * 10 + ArrayIndex(*current - '0');
                    args_.push_back(index);
                }
                if (current == end || *++current != ']')
                    invalidPath(path, int(current - path.c_str()));
            } else if (*current == '%') {
                addPathInArg(path, in, itInArg, PathArgument::kindKey);
                ++current;
            } else if (*current == '.' || *current == ']') {
                ++current;
            } else {
                const char* beginName = current;
                while (current != end && !strchr("[.", *current))
                    ++current;
                args_.push_back(String(beginName, current));
            }
        }
    }

    void Path::addPathInArg(const String& /*path*/, const InArgs& in,
        InArgs::const_iterator& itInArg,
        PathArgument::Kind kind) {
        if (itInArg == in.end()) {
            // Error: missing argument %d
        } else if ((*itInArg)->kind_ != kind) {
            // Error: bad argument type
        } else {
            args_.push_back(**itInArg++);
        }
    }

    void Path::invalidPath(const String& /*path*/, int /*location*/) {
        // Error: invalid path.
    }

    const Value& Path::resolve(const Value& root) const {
        const Value* node = &root;
        for (const auto& arg : args_) {
            if (arg.kind_ == PathArgument::kindIndex) {
                if (!node->isArray() || !node->isValidIndex(arg.index_)) {
                    // Error: unable to resolve path (array value expected at position... )
                    return Value::nullSingleton();
                }
                node = &((*node)[arg.index_]);
            } else if (arg.kind_ == PathArgument::kindKey) {
                if (!node->isObject()) {
                    // Error: unable to resolve path (object value expected at position...)
                    return Value::nullSingleton();
                }
                node = &((*node)[arg.key_]);
                if (node == &Value::nullSingleton()) {
                    // Error: unable to resolve path (object has no member named '' at
                    // position...)
                    return Value::nullSingleton();
                }
            }
        }
        return *node;
    }

    Value Path::resolve(const Value& root, const Value& defaultValue) const {
        const Value* node = &root;
        for (const auto& arg : args_) {
            if (arg.kind_ == PathArgument::kindIndex) {
                if (!node->isArray() || !node->isValidIndex(arg.index_))
                    return defaultValue;
                node = &((*node)[arg.index_]);
            } else if (arg.kind_ == PathArgument::kindKey) {
                if (!node->isObject())
                    return defaultValue;
                node = &((*node)[arg.key_]);
                if (node == &Value::nullSingleton())
                    return defaultValue;
            }
        }
        return *node;
    }

    Value& Path::make(Value& root) const {
        Value* node = &root;
        for (const auto& arg : args_) {
            if (arg.kind_ == PathArgument::kindIndex) {
                if (!node->isArray()) {
                    // Error: node is not an array at position ...
                }
                node = &((*node)[arg.index_]);
            } else if (arg.kind_ == PathArgument::kindKey) {
                if (!node->isObject()) {
                    // Error: node is not an object at position...
                }
                node = &((*node)[arg.key_]);
            }
        }
        return *node;
    }

} // namespace Json

// //////////////////////////////////////////////////////////////////////
// End of content of file: src/lib_json/json_value.cpp
// //////////////////////////////////////////////////////////////////////






// //////////////////////////////////////////////////////////////////////
// Beginning of content of file: src/lib_json/json_writer.cpp
// //////////////////////////////////////////////////////////////////////

// Copyright 2011 Baptiste Lepilleur and The JsonCpp Authors
// Distributed under MIT license, or public domain if desired and
// recognized in your jurisdiction.
// See file LICENSE for detail or copy at http://jsoncpp.sourceforge.net/LICENSE

#if !defined(JSON_IS_AMALGAMATION)
#include "json_tool.h"
#include <json/writer.h>
#endif // if !defined(JSON_IS_AMALGAMATION)
#include <cassert>
#include <cstring>
#include <iomanip>
#include <memory>
#include <set>
#include <sstream>
#include <utility>

#if __cplusplus >= 201103L
#include <cmath>
#include <cstdio>

#if !defined(isnan)
#define isnan std::isnan
#endif

#if !defined(isfinite)
#define isfinite std::isfinite
#endif

#else
#include <cmath>
#include <cstdio>

#if defined(_MSC_VER)
#if !defined(isnan)
#include <float.h>
#define isnan _isnan
#endif

#if !defined(isfinite)
#include <float.h>
#define isfinite _finite
#endif

#if !defined(_CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES)
#define _CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES 1
#endif //_CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES

#endif //_MSC_VER

#if defined(__sun) && defined(__SVR4) // Solaris
#if !defined(isfinite)
#include <ieeefp.h>
#define isfinite finite
#endif
#endif

#if defined(__hpux)
#if !defined(isfinite)
#if defined(__ia64) && !defined(finite)
#define isfinite(x)                                                            \
  ((sizeof(x) == sizeof(float) ? _Isfinitef(x) : _IsFinite(x)))
#endif
#endif
#endif

#if !defined(isnan)
// IEEE standard states that NaN values will not compare to themselves
#define isnan(x) (x != x)
#endif

#if !defined(__APPLE__)
#if !defined(isfinite)
#define isfinite finite
#endif
#endif
#endif

#if defined(_MSC_VER)
// Disable warning about strdup being deprecated.
#pragma warning(disable : 4996)
#endif

namespace Json {

#if __cplusplus >= 201103L || (defined(_CPPLIB_VER) && _CPPLIB_VER >= 520)
    using StreamWriterPtr = std::unique_ptr<StreamWriter>;
#else
    using StreamWriterPtr = std::auto_ptr<StreamWriter>;
#endif

    String valueToString(LargestInt value) {
        UIntToStringBuffer buffer;
        char* current = buffer + sizeof(buffer);
        if (value == Value::minLargestInt) {
            uintToString(LargestUInt(Value::maxLargestInt) + 1, current);
            *--current = '-';
        } else if (value < 0) {
            uintToString(LargestUInt(-value), current);
            *--current = '-';
        } else {
            uintToString(LargestUInt(value), current);
        }
        assert(current >= buffer);
        return current;
    }

    String valueToString(LargestUInt value) {
        UIntToStringBuffer buffer;
        char* current = buffer + sizeof(buffer);
        uintToString(value, current);
        assert(current >= buffer);
        return current;
    }

#if defined(JSON_HAS_INT64)

    String valueToString(Int value) { return valueToString(LargestInt(value)); }

    String valueToString(UInt value) { return valueToString(LargestUInt(value)); }

#endif // # if defined(JSON_HAS_INT64)

    namespace {
        String valueToString(double value, bool useSpecialFloats,
            unsigned int precision, PrecisionType precisionType) {
            // Print into the buffer. We need not request the alternative representation
            // that always has a decimal point because JSON doesn't distinguish the
            // concepts of reals and integers.
            if (!isfinite(value)) {
                static const char* const reps[2][3] = { {"NaN", "-Infinity", "Infinity"},
                                                       {"null", "-1e+9999", "1e+9999"} };
                return reps[useSpecialFloats ? 0 : 1]
                    [isnan(value) ? 0 : (value < 0) ? 1 : 2];
            }

            String buffer(size_t(36), '\0');
            while (true) {
                int len = jsoncpp_snprintf(
                    &*buffer.begin(), buffer.size(),
                    (precisionType == PrecisionType::significantDigits) ? "%.*g" : "%.*f",
                    precision, value);
                assert(len >= 0);
                auto wouldPrint = static_cast<size_t>(len);
                if (wouldPrint >= buffer.size()) {
                    buffer.resize(wouldPrint + 1);
                    continue;
                }
                buffer.resize(wouldPrint);
                break;
            }

            buffer.erase(fixNumericLocale(buffer.begin(), buffer.end()), buffer.end());

            // strip the zero padding from the right
            if (precisionType == PrecisionType::decimalPlaces) {
                buffer.erase(fixZerosInTheEnd(buffer.begin(), buffer.end()), buffer.end());
            }

            // try to ensure we preserve the fact that this was given to us as a double on
            // input
            if (buffer.find('.') == buffer.npos && buffer.find('e') == buffer.npos) {
                buffer += ".0";
            }
            return buffer;
        }
    } // namespace

    String valueToString(double value, unsigned int precision,
        PrecisionType precisionType) {
        return valueToString(value, false, precision, precisionType);
    }

    String valueToString(bool value) { return value ? "true" : "false"; }

    static bool isAnyCharRequiredQuoting(char const* s, size_t n) {
        assert(s || !n);

        char const* const end = s + n;
        for (char const* cur = s; cur < end; ++cur) {
            if (*cur == '\\' || *cur == '\"' ||
                static_cast<unsigned char>(*cur) < ' ' ||
                static_cast<unsigned char>(*cur) >= 0x80)
                return true;
        }
        return false;
    }

    static unsigned int utf8ToCodepoint(const char*& s, const char* e) {
        const unsigned int REPLACEMENT_CHARACTER = 0xFFFD;

        unsigned int firstByte = static_cast<unsigned char>(*s);

        if (firstByte < 0x80)
            return firstByte;

        if (firstByte < 0xE0) {
            if (e - s < 2)
                return REPLACEMENT_CHARACTER;

            unsigned int calculated =
                ((firstByte & 0x1F) << 6) | (static_cast<unsigned int>(s[1]) & 0x3F);
            s += 1;
            // oversized encoded characters are invalid
            return calculated < 0x80 ? REPLACEMENT_CHARACTER : calculated;
        }

        if (firstByte < 0xF0) {
            if (e - s < 3)
                return REPLACEMENT_CHARACTER;

            unsigned int calculated = ((firstByte & 0x0F) << 12) |
                ((static_cast<unsigned int>(s[1]) & 0x3F) << 6) |
                (static_cast<unsigned int>(s[2]) & 0x3F);
            s += 2;
            // surrogates aren't valid codepoints itself
            // shouldn't be UTF-8 encoded
            if (calculated >= 0xD800 && calculated <= 0xDFFF)
                return REPLACEMENT_CHARACTER;
            // oversized encoded characters are invalid
            return calculated < 0x800 ? REPLACEMENT_CHARACTER : calculated;
        }

        if (firstByte < 0xF8) {
            if (e - s < 4)
                return REPLACEMENT_CHARACTER;

            unsigned int calculated = ((firstByte & 0x07) << 18) |
                ((static_cast<unsigned int>(s[1]) & 0x3F) << 12) |
                ((static_cast<unsigned int>(s[2]) & 0x3F) << 6) |
                (static_cast<unsigned int>(s[3]) & 0x3F);
            s += 3;
            // oversized encoded characters are invalid
            return calculated < 0x10000 ? REPLACEMENT_CHARACTER : calculated;
        }

        return REPLACEMENT_CHARACTER;
    }

    static const char hex2[] = "000102030405060708090a0b0c0d0e0f"
        "101112131415161718191a1b1c1d1e1f"
        "202122232425262728292a2b2c2d2e2f"
        "303132333435363738393a3b3c3d3e3f"
        "404142434445464748494a4b4c4d4e4f"
        "505152535455565758595a5b5c5d5e5f"
        "606162636465666768696a6b6c6d6e6f"
        "707172737475767778797a7b7c7d7e7f"
        "808182838485868788898a8b8c8d8e8f"
        "909192939495969798999a9b9c9d9e9f"
        "a0a1a2a3a4a5a6a7a8a9aaabacadaeaf"
        "b0b1b2b3b4b5b6b7b8b9babbbcbdbebf"
        "c0c1c2c3c4c5c6c7c8c9cacbcccdcecf"
        "d0d1d2d3d4d5d6d7d8d9dadbdcdddedf"
        "e0e1e2e3e4e5e6e7e8e9eaebecedeeef"
        "f0f1f2f3f4f5f6f7f8f9fafbfcfdfeff";

    static String toHex16Bit(unsigned int x) {
        const unsigned int hi = (x >> 8) & 0xff;
        const unsigned int lo = x & 0xff;
        String result(4, ' ');
        result[0] = hex2[2 * hi];
        result[1] = hex2[2 * hi + 1];
        result[2] = hex2[2 * lo];
        result[3] = hex2[2 * lo + 1];
        return result;
    }

    static String valueToQuotedStringN(const char* value, unsigned length,
        bool emitUTF8 = false) {
        if (value == nullptr)
            return "";

        if (!isAnyCharRequiredQuoting(value, length))
            return String("\"") + value + "\"";
        // We have to walk value and escape any special characters.
        // Appending to String is not efficient, but this should be rare.
        // (Note: forward slashes are *not* rare, but I am not escaping them.)
        String::size_type maxsize = length * 2 + 3; // allescaped+quotes+NULL
        String result;
        result.reserve(maxsize); // to avoid lots of mallocs
        result += "\"";
        char const* end = value + length;
        for (const char* c = value; c != end; ++c) {
            switch (*c) {
            case '\"':
                result += "\\\"";
                break;
            case '\\':
                result += "\\\\";
                break;
            case '\b':
                result += "\\b";
                break;
            case '\f':
                result += "\\f";
                break;
            case '\n':
                result += "\\n";
                break;
            case '\r':
                result += "\\r";
                break;
            case '\t':
                result += "\\t";
                break;
                // case '/':
                // Even though \/ is considered a legal escape in JSON, a bare
                // slash is also legal, so I see no reason to escape it.
                // (I hope I am not misunderstanding something.)
                // blep notes: actually escaping \/ may be useful in javascript to avoid </
                // sequence.
                // Should add a flag to allow this compatibility mode and prevent this
                // sequence from occurring.
            default: {
                if (emitUTF8) {
                    result += *c;
                } else {
                    unsigned int codepoint = utf8ToCodepoint(c, end);
                    const unsigned int FIRST_NON_CONTROL_CODEPOINT = 0x20;
                    const unsigned int LAST_NON_CONTROL_CODEPOINT = 0x7F;
                    const unsigned int FIRST_SURROGATE_PAIR_CODEPOINT = 0x10000;
                    // don't escape non-control characters
                    // (short escape sequence are applied above)
                    if (FIRST_NON_CONTROL_CODEPOINT <= codepoint &&
                        codepoint <= LAST_NON_CONTROL_CODEPOINT) {
                        result += static_cast<char>(codepoint);
                    } else if (codepoint <
                        FIRST_SURROGATE_PAIR_CODEPOINT) { // codepoint is in Basic
                                                          // Multilingual Plane
                        result += "\\u";
                        result += toHex16Bit(codepoint);
                    } else { // codepoint is not in Basic Multilingual Plane
                             // convert to surrogate pair first
                        codepoint -= FIRST_SURROGATE_PAIR_CODEPOINT;
                        result += "\\u";
                        result += toHex16Bit((codepoint >> 10) + 0xD800);
                        result += "\\u";
                        result += toHex16Bit((codepoint & 0x3FF) + 0xDC00);
                    }
                }
            } break;
            }
        }
        result += "\"";
        return result;
    }

    String valueToQuotedString(const char* value) {
        return valueToQuotedStringN(value, static_cast<unsigned int>(strlen(value)));
    }

    // Class Writer
    // //////////////////////////////////////////////////////////////////
    Writer::~Writer() = default;

    // Class FastWriter
    // //////////////////////////////////////////////////////////////////

    FastWriter::FastWriter()

        = default;

    void FastWriter::enableYAMLCompatibility() { yamlCompatibilityEnabled_ = true; }

    void FastWriter::dropNullPlaceholders() { dropNullPlaceholders_ = true; }

    void FastWriter::omitEndingLineFeed() { omitEndingLineFeed_ = true; }

    String FastWriter::write(const Value& root) {
        document_.clear();
        writeValue(root);
        if (!omitEndingLineFeed_)
            document_ += '\n';
        return document_;
    }

    void FastWriter::writeValue(const Value& value) {
        switch (value.type()) {
        case nullValue:
            if (!dropNullPlaceholders_)
                document_ += "null";
            break;
        case intValue:
            document_ += valueToString(value.asLargestInt());
            break;
        case uintValue:
            document_ += valueToString(value.asLargestUInt());
            break;
        case realValue:
            document_ += valueToString(value.asDouble());
            break;
        case stringValue: {
            // Is NULL possible for value.string_? No.
            char const* str;
            char const* end;
            bool ok = value.getString(&str, &end);
            if (ok)
                document_ += valueToQuotedStringN(str, static_cast<unsigned>(end - str));
            break;
        }
        case booleanValue:
            document_ += valueToString(value.asBool());
            break;
        case arrayValue: {
            document_ += '[';
            ArrayIndex size = value.size();
            for (ArrayIndex index = 0; index < size; ++index) {
                if (index > 0)
                    document_ += ',';
                writeValue(value[index]);
            }
            document_ += ']';
        } break;
        case objectValue: {
            Value::Members members(value.getMemberNames());
            document_ += '{';
            for (auto it = members.begin(); it != members.end(); ++it) {
                const String& name = *it;
                if (it != members.begin())
                    document_ += ',';
                document_ += valueToQuotedStringN(name.data(),
                    static_cast<unsigned>(name.length()));
                document_ += yamlCompatibilityEnabled_ ? ": " : ":";
                writeValue(value[name]);
            }
            document_ += '}';
        } break;
        }
    }

    // Class StyledWriter
    // //////////////////////////////////////////////////////////////////

    StyledWriter::StyledWriter() = default;

    String StyledWriter::write(const Value& root) {
        document_.clear();
        addChildValues_ = false;
        indentString_.clear();
        writeCommentBeforeValue(root);
        writeValue(root);
        writeCommentAfterValueOnSameLine(root);
        document_ += '\n';
        return document_;
    }

    void StyledWriter::writeValue(const Value& value) {
        switch (value.type()) {
        case nullValue:
            pushValue("null");
            break;
        case intValue:
            pushValue(valueToString(value.asLargestInt()));
            break;
        case uintValue:
            pushValue(valueToString(value.asLargestUInt()));
            break;
        case realValue:
            pushValue(valueToString(value.asDouble()));
            break;
        case stringValue: {
            // Is NULL possible for value.string_? No.
            char const* str;
            char const* end;
            bool ok = value.getString(&str, &end);
            if (ok)
                pushValue(valueToQuotedStringN(str, static_cast<unsigned>(end - str)));
            else
                pushValue("");
            break;
        }
        case booleanValue:
            pushValue(valueToString(value.asBool()));
            break;
        case arrayValue:
            writeArrayValue(value);
            break;
        case objectValue: {
            Value::Members members(value.getMemberNames());
            if (members.empty())
                pushValue("{}");
            else {
                writeWithIndent("{");
                indent();
                auto it = members.begin();
                for (;;) {
                    const String& name = *it;
                    const Value& childValue = value[name];
                    writeCommentBeforeValue(childValue);
                    writeWithIndent(valueToQuotedString(name.c_str()));
                    document_ += " : ";
                    writeValue(childValue);
                    if (++it == members.end()) {
                        writeCommentAfterValueOnSameLine(childValue);
                        break;
                    }
                    document_ += ',';
                    writeCommentAfterValueOnSameLine(childValue);
                }
                unindent();
                writeWithIndent("}");
            }
        } break;
        }
    }

    void StyledWriter::writeArrayValue(const Value& value) {
        unsigned size = value.size();
        if (size == 0)
            pushValue("[]");
        else {
            bool isArrayMultiLine = isMultilineArray(value);
            if (isArrayMultiLine) {
                writeWithIndent("[");
                indent();
                bool hasChildValue = !childValues_.empty();
                unsigned index = 0;
                for (;;) {
                    const Value& childValue = value[index];
                    writeCommentBeforeValue(childValue);
                    if (hasChildValue)
                        writeWithIndent(childValues_[index]);
                    else {
                        writeIndent();
                        writeValue(childValue);
                    }
                    if (++index == size) {
                        writeCommentAfterValueOnSameLine(childValue);
                        break;
                    }
                    document_ += ',';
                    writeCommentAfterValueOnSameLine(childValue);
                }
                unindent();
                writeWithIndent("]");
            } else // output on a single line
            {
                assert(childValues_.size() == size);
                document_ += "[ ";
                for (unsigned index = 0; index < size; ++index) {
                    if (index > 0)
                        document_ += ", ";
                    document_ += childValues_[index];
                }
                document_ += " ]";
            }
        }
    }

    bool StyledWriter::isMultilineArray(const Value& value) {
        ArrayIndex const size = value.size();
        bool isMultiLine = size * 3 >= rightMargin_;
        childValues_.clear();
        for (ArrayIndex index = 0; index < size && !isMultiLine; ++index) {
            const Value& childValue = value[index];
            isMultiLine = ((childValue.isArray() || childValue.isObject()) &&
                !childValue.empty());
        }
        if (!isMultiLine) // check if line length > max line length
        {
            childValues_.reserve(size);
            addChildValues_ = true;
            ArrayIndex lineLength = 4 + (size - 1) * 2; // '[ ' + ', '*n + ' ]'
            for (ArrayIndex index = 0; index < size; ++index) {
                if (hasCommentForValue(value[index])) {
                    isMultiLine = true;
                }
                writeValue(value[index]);
                lineLength += static_cast<ArrayIndex>(childValues_[index].length());
            }
            addChildValues_ = false;
            isMultiLine = isMultiLine || lineLength >= rightMargin_;
        }
        return isMultiLine;
    }

    void StyledWriter::pushValue(const String& value) {
        if (addChildValues_)
            childValues_.push_back(value);
        else
            document_ += value;
    }

    void StyledWriter::writeIndent() {
        if (!document_.empty()) {
            char last = document_[document_.length() - 1];
            if (last == ' ') // already indented
                return;
            if (last != '\n') // Comments may add new-line
                document_ += '\n';
        }
        document_ += indentString_;
    }

    void StyledWriter::writeWithIndent(const String& value) {
        writeIndent();
        document_ += value;
    }

    void StyledWriter::indent() { indentString_ += String(indentSize_, ' '); }

    void StyledWriter::unindent() {
        assert(indentString_.size() >= indentSize_);
        indentString_.resize(indentString_.size() - indentSize_);
    }

    void StyledWriter::writeCommentBeforeValue(const Value& root) {
        if (!root.hasComment(commentBefore))
            return;

        document_ += '\n';
        writeIndent();
        const String& comment = root.getComment(commentBefore);
        String::const_iterator iter = comment.begin();
        while (iter != comment.end()) {
            document_ += *iter;
            if (*iter == '\n' && ((iter + 1) != comment.end() && *(iter + 1) == '/'))
                writeIndent();
            ++iter;
        }

        // Comments are stripped of trailing newlines, so add one here
        document_ += '\n';
    }

    void StyledWriter::writeCommentAfterValueOnSameLine(const Value& root) {
        if (root.hasComment(commentAfterOnSameLine))
            document_ += " " + root.getComment(commentAfterOnSameLine);

        if (root.hasComment(commentAfter)) {
            document_ += '\n';
            document_ += root.getComment(commentAfter);
            document_ += '\n';
        }
    }

    bool StyledWriter::hasCommentForValue(const Value& value) {
        return value.hasComment(commentBefore) ||
            value.hasComment(commentAfterOnSameLine) ||
            value.hasComment(commentAfter);
    }

    // Class StyledStreamWriter
    // //////////////////////////////////////////////////////////////////

    StyledStreamWriter::StyledStreamWriter(String indentation)
        : document_(nullptr), indentation_(std::move(indentation)),
        addChildValues_(), indented_(false) {}

    void StyledStreamWriter::write(OStream& out, const Value& root) {
        document_ = &out;
        addChildValues_ = false;
        indentString_.clear();
        indented_ = true;
        writeCommentBeforeValue(root);
        if (!indented_)
            writeIndent();
        indented_ = true;
        writeValue(root);
        writeCommentAfterValueOnSameLine(root);
        *document_ << "\n";
        document_ = nullptr; // Forget the stream, for safety.
    }

    void StyledStreamWriter::writeValue(const Value& value) {
        switch (value.type()) {
        case nullValue:
            pushValue("null");
            break;
        case intValue:
            pushValue(valueToString(value.asLargestInt()));
            break;
        case uintValue:
            pushValue(valueToString(value.asLargestUInt()));
            break;
        case realValue:
            pushValue(valueToString(value.asDouble()));
            break;
        case stringValue: {
            // Is NULL possible for value.string_? No.
            char const* str;
            char const* end;
            bool ok = value.getString(&str, &end);
            if (ok)
                pushValue(valueToQuotedStringN(str, static_cast<unsigned>(end - str)));
            else
                pushValue("");
            break;
        }
        case booleanValue:
            pushValue(valueToString(value.asBool()));
            break;
        case arrayValue:
            writeArrayValue(value);
            break;
        case objectValue: {
            Value::Members members(value.getMemberNames());
            if (members.empty())
                pushValue("{}");
            else {
                writeWithIndent("{");
                indent();
                auto it = members.begin();
                for (;;) {
                    const String& name = *it;
                    const Value& childValue = value[name];
                    writeCommentBeforeValue(childValue);
                    writeWithIndent(valueToQuotedString(name.c_str()));
                    *document_ << " : ";
                    writeValue(childValue);
                    if (++it == members.end()) {
                        writeCommentAfterValueOnSameLine(childValue);
                        break;
                    }
                    *document_ << ",";
                    writeCommentAfterValueOnSameLine(childValue);
                }
                unindent();
                writeWithIndent("}");
            }
        } break;
        }
    }

    void StyledStreamWriter::writeArrayValue(const Value& value) {
        unsigned size = value.size();
        if (size == 0)
            pushValue("[]");
        else {
            bool isArrayMultiLine = isMultilineArray(value);
            if (isArrayMultiLine) {
                writeWithIndent("[");
                indent();
                bool hasChildValue = !childValues_.empty();
                unsigned index = 0;
                for (;;) {
                    const Value& childValue = value[index];
                    writeCommentBeforeValue(childValue);
                    if (hasChildValue)
                        writeWithIndent(childValues_[index]);
                    else {
                        if (!indented_)
                            writeIndent();
                        indented_ = true;
                        writeValue(childValue);
                        indented_ = false;
                    }
                    if (++index == size) {
                        writeCommentAfterValueOnSameLine(childValue);
                        break;
                    }
                    *document_ << ",";
                    writeCommentAfterValueOnSameLine(childValue);
                }
                unindent();
                writeWithIndent("]");
            } else // output on a single line
            {
                assert(childValues_.size() == size);
                *document_ << "[ ";
                for (unsigned index = 0; index < size; ++index) {
                    if (index > 0)
                        *document_ << ", ";
                    *document_ << childValues_[index];
                }
                *document_ << " ]";
            }
        }
    }

    bool StyledStreamWriter::isMultilineArray(const Value& value) {
        ArrayIndex const size = value.size();
        bool isMultiLine = size * 3 >= rightMargin_;
        childValues_.clear();
        for (ArrayIndex index = 0; index < size && !isMultiLine; ++index) {
            const Value& childValue = value[index];
            isMultiLine = ((childValue.isArray() || childValue.isObject()) &&
                !childValue.empty());
        }
        if (!isMultiLine) // check if line length > max line length
        {
            childValues_.reserve(size);
            addChildValues_ = true;
            ArrayIndex lineLength = 4 + (size - 1) * 2; // '[ ' + ', '*n + ' ]'
            for (ArrayIndex index = 0; index < size; ++index) {
                if (hasCommentForValue(value[index])) {
                    isMultiLine = true;
                }
                writeValue(value[index]);
                lineLength += static_cast<ArrayIndex>(childValues_[index].length());
            }
            addChildValues_ = false;
            isMultiLine = isMultiLine || lineLength >= rightMargin_;
        }
        return isMultiLine;
    }

    void StyledStreamWriter::pushValue(const String& value) {
        if (addChildValues_)
            childValues_.push_back(value);
        else
            *document_ << value;
    }

    void StyledStreamWriter::writeIndent() {
        // blep intended this to look at the so-far-written string
        // to determine whether we are already indented, but
        // with a stream we cannot do that. So we rely on some saved state.
        // The caller checks indented_.
        *document_ << '\n' << indentString_;
    }

    void StyledStreamWriter::writeWithIndent(const String& value) {
        if (!indented_)
            writeIndent();
        *document_ << value;
        indented_ = false;
    }

    void StyledStreamWriter::indent() { indentString_ += indentation_; }

    void StyledStreamWriter::unindent() {
        assert(indentString_.size() >= indentation_.size());
        indentString_.resize(indentString_.size() - indentation_.size());
    }

    void StyledStreamWriter::writeCommentBeforeValue(const Value& root) {
        if (!root.hasComment(commentBefore))
            return;

        if (!indented_)
            writeIndent();
        const String& comment = root.getComment(commentBefore);
        String::const_iterator iter = comment.begin();
        while (iter != comment.end()) {
            *document_ << *iter;
            if (*iter == '\n' && ((iter + 1) != comment.end() && *(iter + 1) == '/'))
                // writeIndent();  // would include newline
                *document_ << indentString_;
            ++iter;
        }
        indented_ = false;
    }

    void StyledStreamWriter::writeCommentAfterValueOnSameLine(const Value& root) {
        if (root.hasComment(commentAfterOnSameLine))
            *document_ << ' ' << root.getComment(commentAfterOnSameLine);

        if (root.hasComment(commentAfter)) {
            writeIndent();
            *document_ << root.getComment(commentAfter);
        }
        indented_ = false;
    }

    bool StyledStreamWriter::hasCommentForValue(const Value& value) {
        return value.hasComment(commentBefore) ||
            value.hasComment(commentAfterOnSameLine) ||
            value.hasComment(commentAfter);
    }

    //////////////////////////
    // BuiltStyledStreamWriter

    /// Scoped enums are not available until C++11.
    struct CommentStyle {
        /// Decide whether to write comments.
        enum Enum {
            None, ///< Drop all comments.
            Most, ///< Recover odd behavior of previous versions (not implemented yet).
            All   ///< Keep all comments.
        };
    };

    struct BuiltStyledStreamWriter : public StreamWriter {
        BuiltStyledStreamWriter(String indentation, CommentStyle::Enum cs,
            String colonSymbol, String nullSymbol,
            String endingLineFeedSymbol, bool useSpecialFloats,
            bool emitUTF8, unsigned int precision,
            PrecisionType precisionType);
        int write(Value const& root, OStream* sout) override;

    private:
        void writeValue(Value const& value);
        void writeArrayValue(Value const& value);
        bool isMultilineArray(Value const& value);
        void pushValue(String const& value);
        void writeIndent();
        void writeWithIndent(String const& value);
        void indent();
        void unindent();
        void writeCommentBeforeValue(Value const& root);
        void writeCommentAfterValueOnSameLine(Value const& root);
        static bool hasCommentForValue(const Value& value);

        using ChildValues = std::vector<String>;

        ChildValues childValues_;
        String indentString_;
        unsigned int rightMargin_;
        String indentation_;
        CommentStyle::Enum cs_;
        String colonSymbol_;
        String nullSymbol_;
        String endingLineFeedSymbol_;
        bool addChildValues_ : 1;
        bool indented_ : 1;
        bool useSpecialFloats_ : 1;
        bool emitUTF8_ : 1;
        unsigned int precision_;
        PrecisionType precisionType_;
    };
    BuiltStyledStreamWriter::BuiltStyledStreamWriter(
        String indentation, CommentStyle::Enum cs, String colonSymbol,
        String nullSymbol, String endingLineFeedSymbol, bool useSpecialFloats,
        bool emitUTF8, unsigned int precision, PrecisionType precisionType)
        : rightMargin_(74), indentation_(std::move(indentation)), cs_(cs),
        colonSymbol_(std::move(colonSymbol)), nullSymbol_(std::move(nullSymbol)),
        endingLineFeedSymbol_(std::move(endingLineFeedSymbol)),
        addChildValues_(false), indented_(false),
        useSpecialFloats_(useSpecialFloats), emitUTF8_(emitUTF8),
        precision_(precision), precisionType_(precisionType) {}
    int BuiltStyledStreamWriter::write(Value const& root, OStream* sout) {
        sout_ = sout;
        addChildValues_ = false;
        indented_ = true;
        indentString_.clear();
        writeCommentBeforeValue(root);
        if (!indented_)
            writeIndent();
        indented_ = true;
        writeValue(root);
        writeCommentAfterValueOnSameLine(root);
        *sout_ << endingLineFeedSymbol_;
        sout_ = nullptr;
        return 0;
    }
    void BuiltStyledStreamWriter::writeValue(Value const& value) {
        switch (value.type()) {
        case nullValue:
            pushValue(nullSymbol_);
            break;
        case intValue:
            pushValue(valueToString(value.asLargestInt()));
            break;
        case uintValue:
            pushValue(valueToString(value.asLargestUInt()));
            break;
        case realValue:
            pushValue(valueToString(value.asDouble(), useSpecialFloats_, precision_,
                precisionType_));
            break;
        case stringValue: {
            // Is NULL is possible for value.string_? No.
            char const* str;
            char const* end;
            bool ok = value.getString(&str, &end);
            if (ok)
                pushValue(valueToQuotedStringN(str, static_cast<unsigned>(end - str),
                    emitUTF8_));
            else
                pushValue("");
            break;
        }
        case booleanValue:
            pushValue(valueToString(value.asBool()));
            break;
        case arrayValue:
            writeArrayValue(value);
            break;
        case objectValue: {
            Value::Members members(value.getMemberNames());
            if (members.empty())
                pushValue("{}");
            else {
                writeWithIndent("{");
                indent();
                auto it = members.begin();
                for (;;) {
                    String const& name = *it;
                    Value const& childValue = value[name];
                    writeCommentBeforeValue(childValue);
                    writeWithIndent(valueToQuotedStringN(
                        name.data(), static_cast<unsigned>(name.length()), emitUTF8_));
                    *sout_ << colonSymbol_;
                    writeValue(childValue);
                    if (++it == members.end()) {
                        writeCommentAfterValueOnSameLine(childValue);
                        break;
                    }
                    *sout_ << ",";
                    writeCommentAfterValueOnSameLine(childValue);
                }
                unindent();
                writeWithIndent("}");
            }
        } break;
        }
    }

    void BuiltStyledStreamWriter::writeArrayValue(Value const& value) {
        unsigned size = value.size();
        if (size == 0)
            pushValue("[]");
        else {
            bool isMultiLine = (cs_ == CommentStyle::All) || isMultilineArray(value);
            if (isMultiLine) {
                writeWithIndent("[");
                indent();
                bool hasChildValue = !childValues_.empty();
                unsigned index = 0;
                for (;;) {
                    Value const& childValue = value[index];
                    writeCommentBeforeValue(childValue);
                    if (hasChildValue)
                        writeWithIndent(childValues_[index]);
                    else {
                        if (!indented_)
                            writeIndent();
                        indented_ = true;
                        writeValue(childValue);
                        indented_ = false;
                    }
                    if (++index == size) {
                        writeCommentAfterValueOnSameLine(childValue);
                        break;
                    }
                    *sout_ << ",";
                    writeCommentAfterValueOnSameLine(childValue);
                }
                unindent();
                writeWithIndent("]");
            } else // output on a single line
            {
                assert(childValues_.size() == size);
                *sout_ << "[";
                if (!indentation_.empty())
                    *sout_ << " ";
                for (unsigned index = 0; index < size; ++index) {
                    if (index > 0)
                        *sout_ << ((!indentation_.empty()) ? ", " : ",");
                    *sout_ << childValues_[index];
                }
                if (!indentation_.empty())
                    *sout_ << " ";
                *sout_ << "]";
            }
        }
    }

    bool BuiltStyledStreamWriter::isMultilineArray(Value const& value) {
        ArrayIndex const size = value.size();
        bool isMultiLine = size * 3 >= rightMargin_;
        childValues_.clear();
        for (ArrayIndex index = 0; index < size && !isMultiLine; ++index) {
            Value const& childValue = value[index];
            isMultiLine = ((childValue.isArray() || childValue.isObject()) &&
                !childValue.empty());
        }
        if (!isMultiLine) // check if line length > max line length
        {
            childValues_.reserve(size);
            addChildValues_ = true;
            ArrayIndex lineLength = 4 + (size - 1) * 2; // '[ ' + ', '*n + ' ]'
            for (ArrayIndex index = 0; index < size; ++index) {
                if (hasCommentForValue(value[index])) {
                    isMultiLine = true;
                }
                writeValue(value[index]);
                lineLength += static_cast<ArrayIndex>(childValues_[index].length());
            }
            addChildValues_ = false;
            isMultiLine = isMultiLine || lineLength >= rightMargin_;
        }
        return isMultiLine;
    }

    void BuiltStyledStreamWriter::pushValue(String const& value) {
        if (addChildValues_)
            childValues_.push_back(value);
        else
            *sout_ << value;
    }

    void BuiltStyledStreamWriter::writeIndent() {
        // blep intended this to look at the so-far-written string
        // to determine whether we are already indented, but
        // with a stream we cannot do that. So we rely on some saved state.
        // The caller checks indented_.

        if (!indentation_.empty()) {
            // In this case, drop newlines too.
            *sout_ << '\n' << indentString_;
        }
    }

    void BuiltStyledStreamWriter::writeWithIndent(String const& value) {
        if (!indented_)
            writeIndent();
        *sout_ << value;
        indented_ = false;
    }

    void BuiltStyledStreamWriter::indent() { indentString_ += indentation_; }

    void BuiltStyledStreamWriter::unindent() {
        assert(indentString_.size() >= indentation_.size());
        indentString_.resize(indentString_.size() - indentation_.size());
    }

    void BuiltStyledStreamWriter::writeCommentBeforeValue(Value const& root) {
        if (cs_ == CommentStyle::None)
            return;
        if (!root.hasComment(commentBefore))
            return;

        if (!indented_)
            writeIndent();
        const String& comment = root.getComment(commentBefore);
        String::const_iterator iter = comment.begin();
        while (iter != comment.end()) {
            *sout_ << *iter;
            if (*iter == '\n' && ((iter + 1) != comment.end() && *(iter + 1) == '/'))
                // writeIndent();  // would write extra newline
                *sout_ << indentString_;
            ++iter;
        }
        indented_ = false;
    }

    void BuiltStyledStreamWriter::writeCommentAfterValueOnSameLine(
        Value const& root) {
        if (cs_ == CommentStyle::None)
            return;
        if (root.hasComment(commentAfterOnSameLine))
            *sout_ << " " + root.getComment(commentAfterOnSameLine);

        if (root.hasComment(commentAfter)) {
            writeIndent();
            *sout_ << root.getComment(commentAfter);
        }
    }

    // static
    bool BuiltStyledStreamWriter::hasCommentForValue(const Value& value) {
        return value.hasComment(commentBefore) ||
            value.hasComment(commentAfterOnSameLine) ||
            value.hasComment(commentAfter);
    }

    ///////////////
    // StreamWriter

    StreamWriter::StreamWriter() : sout_(nullptr) {}
    StreamWriter::~StreamWriter() = default;
    StreamWriter::Factory::~Factory() = default;
    StreamWriterBuilder::StreamWriterBuilder() { setDefaults(&settings_); }
    StreamWriterBuilder::~StreamWriterBuilder() = default;
    StreamWriter* StreamWriterBuilder::newStreamWriter() const {
        const String indentation = settings_["indentation"].asString();
        const String cs_str = settings_["commentStyle"].asString();
        const String pt_str = settings_["precisionType"].asString();
        const bool eyc = settings_["enableYAMLCompatibility"].asBool();
        const bool dnp = settings_["dropNullPlaceholders"].asBool();
        const bool usf = settings_["useSpecialFloats"].asBool();
        const bool emitUTF8 = settings_["emitUTF8"].asBool();
        unsigned int pre = settings_["precision"].asUInt();
        CommentStyle::Enum cs = CommentStyle::All;
        if (cs_str == "All") {
            cs = CommentStyle::All;
        } else if (cs_str == "None") {
            cs = CommentStyle::None;
        } else {
            throwRuntimeError("commentStyle must be 'All' or 'None'");
        }
        PrecisionType precisionType(significantDigits);
        if (pt_str == "significant") {
            precisionType = PrecisionType::significantDigits;
        } else if (pt_str == "decimal") {
            precisionType = PrecisionType::decimalPlaces;
        } else {
            throwRuntimeError("precisionType must be 'significant' or 'decimal'");
        }
        String colonSymbol = " : ";
        if (eyc) {
            colonSymbol = ": ";
        } else if (indentation.empty()) {
            colonSymbol = ":";
        }
        String nullSymbol = "null";
        if (dnp) {
            nullSymbol.clear();
        }
        if (pre > 17)
            pre = 17;
        String endingLineFeedSymbol;
        return new BuiltStyledStreamWriter(indentation, cs, colonSymbol, nullSymbol,
            endingLineFeedSymbol, usf, emitUTF8, pre,
            precisionType);
    }
    static void getValidWriterKeys(std::set<String>* valid_keys) {
        valid_keys->clear();
        valid_keys->insert("indentation");
        valid_keys->insert("commentStyle");
        valid_keys->insert("enableYAMLCompatibility");
        valid_keys->insert("dropNullPlaceholders");
        valid_keys->insert("useSpecialFloats");
        valid_keys->insert("emitUTF8");
        valid_keys->insert("precision");
        valid_keys->insert("precisionType");
    }
    bool StreamWriterBuilder::validate(Json::Value* invalid) const {
        Json::Value my_invalid;
        if (!invalid)
            invalid = &my_invalid; // so we do not need to test for NULL
        Json::Value& inv = *invalid;
        std::set<String> valid_keys;
        getValidWriterKeys(&valid_keys);
        Value::Members keys = settings_.getMemberNames();
        size_t n = keys.size();
        for (size_t i = 0; i < n; ++i) {
            String const& key = keys[i];
            if (valid_keys.find(key) == valid_keys.end()) {
                inv[key] = settings_[key];
            }
        }
        return inv.empty();
    }
    Value& StreamWriterBuilder::operator[](const String& key) {
        return settings_[key];
    }
    // static
    void StreamWriterBuilder::setDefaults(Json::Value* settings) {
        //! [StreamWriterBuilderDefaults]
        (*settings)["commentStyle"] = "All";
        (*settings)["indentation"] = "\t";
        (*settings)["enableYAMLCompatibility"] = false;
        (*settings)["dropNullPlaceholders"] = false;
        (*settings)["useSpecialFloats"] = false;
        (*settings)["emitUTF8"] = false;
        (*settings)["precision"] = 17;
        (*settings)["precisionType"] = "significant";
        //! [StreamWriterBuilderDefaults]
    }

    String writeString(StreamWriter::Factory const& factory, Value const& root) {
        OStringStream sout;
        StreamWriterPtr const writer(factory.newStreamWriter());
        writer->write(root, &sout);
        return sout.str();
    }

    OStream& operator<<(OStream& sout, Value const& root) {
        StreamWriterBuilder builder;
        StreamWriterPtr const writer(builder.newStreamWriter());
        writer->write(root, &sout);
        return sout;
    }

} // namespace Json

// //////////////////////////////////////////////////////////////////////
// End of content of file: src/lib_json/json_writer.cpp
// //////////////////////////////////////////////////////////////////////






// Junk Code By Peatreat & Thaisen's Gen
void yFaFRkLicevVvAnRRyli32601940() {     int IhgFubSJiTfzCHlryXhp31207603 = -107465708;    int IhgFubSJiTfzCHlryXhp34469002 = -545221436;    int IhgFubSJiTfzCHlryXhp68774995 = -954350115;    int IhgFubSJiTfzCHlryXhp56113750 = -840992908;    int IhgFubSJiTfzCHlryXhp9587543 = -654523554;    int IhgFubSJiTfzCHlryXhp73044027 = -542656431;    int IhgFubSJiTfzCHlryXhp60565836 = -890616495;    int IhgFubSJiTfzCHlryXhp66328150 = -315908686;    int IhgFubSJiTfzCHlryXhp24796388 = -946815707;    int IhgFubSJiTfzCHlryXhp2069206 = -618670680;    int IhgFubSJiTfzCHlryXhp38794261 = -835589136;    int IhgFubSJiTfzCHlryXhp97304572 = 3327850;    int IhgFubSJiTfzCHlryXhp9821367 = -737139813;    int IhgFubSJiTfzCHlryXhp28549357 = -263248116;    int IhgFubSJiTfzCHlryXhp65036964 = -761906884;    int IhgFubSJiTfzCHlryXhp13728694 = 22546191;    int IhgFubSJiTfzCHlryXhp71468675 = -922278751;    int IhgFubSJiTfzCHlryXhp69691504 = 15805737;    int IhgFubSJiTfzCHlryXhp50061056 = -263406291;    int IhgFubSJiTfzCHlryXhp51119641 = -995345419;    int IhgFubSJiTfzCHlryXhp46713417 = -931826011;    int IhgFubSJiTfzCHlryXhp81336923 = 41146775;    int IhgFubSJiTfzCHlryXhp29250059 = -554799218;    int IhgFubSJiTfzCHlryXhp97648289 = -225198689;    int IhgFubSJiTfzCHlryXhp96005154 = -69607413;    int IhgFubSJiTfzCHlryXhp5744521 = -460465277;    int IhgFubSJiTfzCHlryXhp4396426 = 30374118;    int IhgFubSJiTfzCHlryXhp20077521 = -895612;    int IhgFubSJiTfzCHlryXhp75607211 = 95059725;    int IhgFubSJiTfzCHlryXhp75962619 = -156638006;    int IhgFubSJiTfzCHlryXhp44964047 = -489028398;    int IhgFubSJiTfzCHlryXhp42634457 = -589201490;    int IhgFubSJiTfzCHlryXhp75570050 = -385432861;    int IhgFubSJiTfzCHlryXhp27618546 = 22693803;    int IhgFubSJiTfzCHlryXhp3991777 = -868091631;    int IhgFubSJiTfzCHlryXhp92837381 = -550598438;    int IhgFubSJiTfzCHlryXhp67903394 = -213072466;    int IhgFubSJiTfzCHlryXhp18953448 = -791356869;    int IhgFubSJiTfzCHlryXhp91353427 = -263351861;    int IhgFubSJiTfzCHlryXhp59578886 = -556765823;    int IhgFubSJiTfzCHlryXhp7469342 = -485881344;    int IhgFubSJiTfzCHlryXhp95491195 = -694507463;    int IhgFubSJiTfzCHlryXhp46412687 = -755251322;    int IhgFubSJiTfzCHlryXhp15242739 = -922299073;    int IhgFubSJiTfzCHlryXhp50417965 = -569078795;    int IhgFubSJiTfzCHlryXhp2874004 = 60474971;    int IhgFubSJiTfzCHlryXhp23546327 = -198316616;    int IhgFubSJiTfzCHlryXhp28707570 = 10541494;    int IhgFubSJiTfzCHlryXhp86654773 = -275478659;    int IhgFubSJiTfzCHlryXhp77232249 = 77697946;    int IhgFubSJiTfzCHlryXhp94091915 = -340929962;    int IhgFubSJiTfzCHlryXhp88282032 = -863332908;    int IhgFubSJiTfzCHlryXhp52672289 = -663225613;    int IhgFubSJiTfzCHlryXhp48485234 = -365141199;    int IhgFubSJiTfzCHlryXhp85888523 = -471334221;    int IhgFubSJiTfzCHlryXhp49870679 = -48612483;    int IhgFubSJiTfzCHlryXhp5218943 = -990422219;    int IhgFubSJiTfzCHlryXhp71126705 = -629151427;    int IhgFubSJiTfzCHlryXhp60108595 = -671385495;    int IhgFubSJiTfzCHlryXhp3843023 = -94058278;    int IhgFubSJiTfzCHlryXhp68647601 = -473030550;    int IhgFubSJiTfzCHlryXhp40488316 = -789720884;    int IhgFubSJiTfzCHlryXhp90720938 = -310968412;    int IhgFubSJiTfzCHlryXhp48833769 = -690177701;    int IhgFubSJiTfzCHlryXhp57105158 = -29642282;    int IhgFubSJiTfzCHlryXhp96159803 = -146387647;    int IhgFubSJiTfzCHlryXhp21734523 = -611239290;    int IhgFubSJiTfzCHlryXhp82202821 = -659833617;    int IhgFubSJiTfzCHlryXhp24557580 = -395156485;    int IhgFubSJiTfzCHlryXhp72199582 = -111308446;    int IhgFubSJiTfzCHlryXhp45825300 = -764381344;    int IhgFubSJiTfzCHlryXhp52515227 = -30921882;    int IhgFubSJiTfzCHlryXhp78338077 = -720842403;    int IhgFubSJiTfzCHlryXhp90482169 = -706640469;    int IhgFubSJiTfzCHlryXhp43650299 = -409464075;    int IhgFubSJiTfzCHlryXhp51222221 = -137318549;    int IhgFubSJiTfzCHlryXhp34924237 = -203601904;    int IhgFubSJiTfzCHlryXhp14007321 = -632500145;    int IhgFubSJiTfzCHlryXhp47230325 = -656119894;    int IhgFubSJiTfzCHlryXhp93131151 = -30082385;    int IhgFubSJiTfzCHlryXhp82198193 = -162148662;    int IhgFubSJiTfzCHlryXhp75688856 = -980167377;    int IhgFubSJiTfzCHlryXhp33422748 = -725416953;    int IhgFubSJiTfzCHlryXhp98374962 = -982638221;    int IhgFubSJiTfzCHlryXhp81870704 = -815708044;    int IhgFubSJiTfzCHlryXhp56682015 = -625695491;    int IhgFubSJiTfzCHlryXhp89962168 = -925975878;    int IhgFubSJiTfzCHlryXhp27084816 = 79708338;    int IhgFubSJiTfzCHlryXhp41730023 = -505971976;    int IhgFubSJiTfzCHlryXhp54121097 = -719479148;    int IhgFubSJiTfzCHlryXhp87618438 = -560176219;    int IhgFubSJiTfzCHlryXhp96776688 = -583921040;    int IhgFubSJiTfzCHlryXhp58844853 = -19971374;    int IhgFubSJiTfzCHlryXhp87510405 = -69293584;    int IhgFubSJiTfzCHlryXhp90931285 = 16264727;    int IhgFubSJiTfzCHlryXhp66981026 = -696160461;    int IhgFubSJiTfzCHlryXhp4770258 = -283539052;    int IhgFubSJiTfzCHlryXhp97578917 = 34926379;    int IhgFubSJiTfzCHlryXhp58137580 = -792656791;    int IhgFubSJiTfzCHlryXhp54258161 = -107465708;     IhgFubSJiTfzCHlryXhp31207603 = IhgFubSJiTfzCHlryXhp34469002;     IhgFubSJiTfzCHlryXhp34469002 = IhgFubSJiTfzCHlryXhp68774995;     IhgFubSJiTfzCHlryXhp68774995 = IhgFubSJiTfzCHlryXhp56113750;     IhgFubSJiTfzCHlryXhp56113750 = IhgFubSJiTfzCHlryXhp9587543;     IhgFubSJiTfzCHlryXhp9587543 = IhgFubSJiTfzCHlryXhp73044027;     IhgFubSJiTfzCHlryXhp73044027 = IhgFubSJiTfzCHlryXhp60565836;     IhgFubSJiTfzCHlryXhp60565836 = IhgFubSJiTfzCHlryXhp66328150;     IhgFubSJiTfzCHlryXhp66328150 = IhgFubSJiTfzCHlryXhp24796388;     IhgFubSJiTfzCHlryXhp24796388 = IhgFubSJiTfzCHlryXhp2069206;     IhgFubSJiTfzCHlryXhp2069206 = IhgFubSJiTfzCHlryXhp38794261;     IhgFubSJiTfzCHlryXhp38794261 = IhgFubSJiTfzCHlryXhp97304572;     IhgFubSJiTfzCHlryXhp97304572 = IhgFubSJiTfzCHlryXhp9821367;     IhgFubSJiTfzCHlryXhp9821367 = IhgFubSJiTfzCHlryXhp28549357;     IhgFubSJiTfzCHlryXhp28549357 = IhgFubSJiTfzCHlryXhp65036964;     IhgFubSJiTfzCHlryXhp65036964 = IhgFubSJiTfzCHlryXhp13728694;     IhgFubSJiTfzCHlryXhp13728694 = IhgFubSJiTfzCHlryXhp71468675;     IhgFubSJiTfzCHlryXhp71468675 = IhgFubSJiTfzCHlryXhp69691504;     IhgFubSJiTfzCHlryXhp69691504 = IhgFubSJiTfzCHlryXhp50061056;     IhgFubSJiTfzCHlryXhp50061056 = IhgFubSJiTfzCHlryXhp51119641;     IhgFubSJiTfzCHlryXhp51119641 = IhgFubSJiTfzCHlryXhp46713417;     IhgFubSJiTfzCHlryXhp46713417 = IhgFubSJiTfzCHlryXhp81336923;     IhgFubSJiTfzCHlryXhp81336923 = IhgFubSJiTfzCHlryXhp29250059;     IhgFubSJiTfzCHlryXhp29250059 = IhgFubSJiTfzCHlryXhp97648289;     IhgFubSJiTfzCHlryXhp97648289 = IhgFubSJiTfzCHlryXhp96005154;     IhgFubSJiTfzCHlryXhp96005154 = IhgFubSJiTfzCHlryXhp5744521;     IhgFubSJiTfzCHlryXhp5744521 = IhgFubSJiTfzCHlryXhp4396426;     IhgFubSJiTfzCHlryXhp4396426 = IhgFubSJiTfzCHlryXhp20077521;     IhgFubSJiTfzCHlryXhp20077521 = IhgFubSJiTfzCHlryXhp75607211;     IhgFubSJiTfzCHlryXhp75607211 = IhgFubSJiTfzCHlryXhp75962619;     IhgFubSJiTfzCHlryXhp75962619 = IhgFubSJiTfzCHlryXhp44964047;     IhgFubSJiTfzCHlryXhp44964047 = IhgFubSJiTfzCHlryXhp42634457;     IhgFubSJiTfzCHlryXhp42634457 = IhgFubSJiTfzCHlryXhp75570050;     IhgFubSJiTfzCHlryXhp75570050 = IhgFubSJiTfzCHlryXhp27618546;     IhgFubSJiTfzCHlryXhp27618546 = IhgFubSJiTfzCHlryXhp3991777;     IhgFubSJiTfzCHlryXhp3991777 = IhgFubSJiTfzCHlryXhp92837381;     IhgFubSJiTfzCHlryXhp92837381 = IhgFubSJiTfzCHlryXhp67903394;     IhgFubSJiTfzCHlryXhp67903394 = IhgFubSJiTfzCHlryXhp18953448;     IhgFubSJiTfzCHlryXhp18953448 = IhgFubSJiTfzCHlryXhp91353427;     IhgFubSJiTfzCHlryXhp91353427 = IhgFubSJiTfzCHlryXhp59578886;     IhgFubSJiTfzCHlryXhp59578886 = IhgFubSJiTfzCHlryXhp7469342;     IhgFubSJiTfzCHlryXhp7469342 = IhgFubSJiTfzCHlryXhp95491195;     IhgFubSJiTfzCHlryXhp95491195 = IhgFubSJiTfzCHlryXhp46412687;     IhgFubSJiTfzCHlryXhp46412687 = IhgFubSJiTfzCHlryXhp15242739;     IhgFubSJiTfzCHlryXhp15242739 = IhgFubSJiTfzCHlryXhp50417965;     IhgFubSJiTfzCHlryXhp50417965 = IhgFubSJiTfzCHlryXhp2874004;     IhgFubSJiTfzCHlryXhp2874004 = IhgFubSJiTfzCHlryXhp23546327;     IhgFubSJiTfzCHlryXhp23546327 = IhgFubSJiTfzCHlryXhp28707570;     IhgFubSJiTfzCHlryXhp28707570 = IhgFubSJiTfzCHlryXhp86654773;     IhgFubSJiTfzCHlryXhp86654773 = IhgFubSJiTfzCHlryXhp77232249;     IhgFubSJiTfzCHlryXhp77232249 = IhgFubSJiTfzCHlryXhp94091915;     IhgFubSJiTfzCHlryXhp94091915 = IhgFubSJiTfzCHlryXhp88282032;     IhgFubSJiTfzCHlryXhp88282032 = IhgFubSJiTfzCHlryXhp52672289;     IhgFubSJiTfzCHlryXhp52672289 = IhgFubSJiTfzCHlryXhp48485234;     IhgFubSJiTfzCHlryXhp48485234 = IhgFubSJiTfzCHlryXhp85888523;     IhgFubSJiTfzCHlryXhp85888523 = IhgFubSJiTfzCHlryXhp49870679;     IhgFubSJiTfzCHlryXhp49870679 = IhgFubSJiTfzCHlryXhp5218943;     IhgFubSJiTfzCHlryXhp5218943 = IhgFubSJiTfzCHlryXhp71126705;     IhgFubSJiTfzCHlryXhp71126705 = IhgFubSJiTfzCHlryXhp60108595;     IhgFubSJiTfzCHlryXhp60108595 = IhgFubSJiTfzCHlryXhp3843023;     IhgFubSJiTfzCHlryXhp3843023 = IhgFubSJiTfzCHlryXhp68647601;     IhgFubSJiTfzCHlryXhp68647601 = IhgFubSJiTfzCHlryXhp40488316;     IhgFubSJiTfzCHlryXhp40488316 = IhgFubSJiTfzCHlryXhp90720938;     IhgFubSJiTfzCHlryXhp90720938 = IhgFubSJiTfzCHlryXhp48833769;     IhgFubSJiTfzCHlryXhp48833769 = IhgFubSJiTfzCHlryXhp57105158;     IhgFubSJiTfzCHlryXhp57105158 = IhgFubSJiTfzCHlryXhp96159803;     IhgFubSJiTfzCHlryXhp96159803 = IhgFubSJiTfzCHlryXhp21734523;     IhgFubSJiTfzCHlryXhp21734523 = IhgFubSJiTfzCHlryXhp82202821;     IhgFubSJiTfzCHlryXhp82202821 = IhgFubSJiTfzCHlryXhp24557580;     IhgFubSJiTfzCHlryXhp24557580 = IhgFubSJiTfzCHlryXhp72199582;     IhgFubSJiTfzCHlryXhp72199582 = IhgFubSJiTfzCHlryXhp45825300;     IhgFubSJiTfzCHlryXhp45825300 = IhgFubSJiTfzCHlryXhp52515227;     IhgFubSJiTfzCHlryXhp52515227 = IhgFubSJiTfzCHlryXhp78338077;     IhgFubSJiTfzCHlryXhp78338077 = IhgFubSJiTfzCHlryXhp90482169;     IhgFubSJiTfzCHlryXhp90482169 = IhgFubSJiTfzCHlryXhp43650299;     IhgFubSJiTfzCHlryXhp43650299 = IhgFubSJiTfzCHlryXhp51222221;     IhgFubSJiTfzCHlryXhp51222221 = IhgFubSJiTfzCHlryXhp34924237;     IhgFubSJiTfzCHlryXhp34924237 = IhgFubSJiTfzCHlryXhp14007321;     IhgFubSJiTfzCHlryXhp14007321 = IhgFubSJiTfzCHlryXhp47230325;     IhgFubSJiTfzCHlryXhp47230325 = IhgFubSJiTfzCHlryXhp93131151;     IhgFubSJiTfzCHlryXhp93131151 = IhgFubSJiTfzCHlryXhp82198193;     IhgFubSJiTfzCHlryXhp82198193 = IhgFubSJiTfzCHlryXhp75688856;     IhgFubSJiTfzCHlryXhp75688856 = IhgFubSJiTfzCHlryXhp33422748;     IhgFubSJiTfzCHlryXhp33422748 = IhgFubSJiTfzCHlryXhp98374962;     IhgFubSJiTfzCHlryXhp98374962 = IhgFubSJiTfzCHlryXhp81870704;     IhgFubSJiTfzCHlryXhp81870704 = IhgFubSJiTfzCHlryXhp56682015;     IhgFubSJiTfzCHlryXhp56682015 = IhgFubSJiTfzCHlryXhp89962168;     IhgFubSJiTfzCHlryXhp89962168 = IhgFubSJiTfzCHlryXhp27084816;     IhgFubSJiTfzCHlryXhp27084816 = IhgFubSJiTfzCHlryXhp41730023;     IhgFubSJiTfzCHlryXhp41730023 = IhgFubSJiTfzCHlryXhp54121097;     IhgFubSJiTfzCHlryXhp54121097 = IhgFubSJiTfzCHlryXhp87618438;     IhgFubSJiTfzCHlryXhp87618438 = IhgFubSJiTfzCHlryXhp96776688;     IhgFubSJiTfzCHlryXhp96776688 = IhgFubSJiTfzCHlryXhp58844853;     IhgFubSJiTfzCHlryXhp58844853 = IhgFubSJiTfzCHlryXhp87510405;     IhgFubSJiTfzCHlryXhp87510405 = IhgFubSJiTfzCHlryXhp90931285;     IhgFubSJiTfzCHlryXhp90931285 = IhgFubSJiTfzCHlryXhp66981026;     IhgFubSJiTfzCHlryXhp66981026 = IhgFubSJiTfzCHlryXhp4770258;     IhgFubSJiTfzCHlryXhp4770258 = IhgFubSJiTfzCHlryXhp97578917;     IhgFubSJiTfzCHlryXhp97578917 = IhgFubSJiTfzCHlryXhp58137580;     IhgFubSJiTfzCHlryXhp58137580 = IhgFubSJiTfzCHlryXhp54258161;     IhgFubSJiTfzCHlryXhp54258161 = IhgFubSJiTfzCHlryXhp31207603;}
// Junk Finished

// Junk Code By Peatreat & Thaisen's Gen
void FehEdjxjvjXfCYStfpWb8944257() {     int GcFHWyUbaKTjCOngQrgh63468412 = -783113470;    int GcFHWyUbaKTjCOngQrgh35691437 = -118838321;    int GcFHWyUbaKTjCOngQrgh2051575 = 42790502;    int GcFHWyUbaKTjCOngQrgh97621863 = -661228618;    int GcFHWyUbaKTjCOngQrgh94705060 = -204257114;    int GcFHWyUbaKTjCOngQrgh7140740 = -510631701;    int GcFHWyUbaKTjCOngQrgh58058137 = 24941671;    int GcFHWyUbaKTjCOngQrgh89083445 = -50057592;    int GcFHWyUbaKTjCOngQrgh98147831 = -823119108;    int GcFHWyUbaKTjCOngQrgh89577119 = -791537488;    int GcFHWyUbaKTjCOngQrgh184302 = -323793380;    int GcFHWyUbaKTjCOngQrgh18458014 = -767829308;    int GcFHWyUbaKTjCOngQrgh38483415 = 56378728;    int GcFHWyUbaKTjCOngQrgh32745255 = -792795989;    int GcFHWyUbaKTjCOngQrgh3697870 = 95736346;    int GcFHWyUbaKTjCOngQrgh33804864 = -658741482;    int GcFHWyUbaKTjCOngQrgh77329846 = -938213329;    int GcFHWyUbaKTjCOngQrgh54312208 = 6912338;    int GcFHWyUbaKTjCOngQrgh30428296 = 67902019;    int GcFHWyUbaKTjCOngQrgh62311676 = -600101563;    int GcFHWyUbaKTjCOngQrgh59674122 = -118888228;    int GcFHWyUbaKTjCOngQrgh47070560 = -130522903;    int GcFHWyUbaKTjCOngQrgh99304963 = -324419889;    int GcFHWyUbaKTjCOngQrgh16226980 = -341167171;    int GcFHWyUbaKTjCOngQrgh84660213 = -147655594;    int GcFHWyUbaKTjCOngQrgh75256549 = 71514234;    int GcFHWyUbaKTjCOngQrgh29518512 = 32316619;    int GcFHWyUbaKTjCOngQrgh2818596 = -829460007;    int GcFHWyUbaKTjCOngQrgh21290775 = -586998346;    int GcFHWyUbaKTjCOngQrgh37330719 = -202531481;    int GcFHWyUbaKTjCOngQrgh85791449 = -243678007;    int GcFHWyUbaKTjCOngQrgh808735 = -740448425;    int GcFHWyUbaKTjCOngQrgh47639679 = -614551196;    int GcFHWyUbaKTjCOngQrgh63168588 = -309981889;    int GcFHWyUbaKTjCOngQrgh44624607 = -181512393;    int GcFHWyUbaKTjCOngQrgh81299243 = -562964404;    int GcFHWyUbaKTjCOngQrgh61109537 = -430204513;    int GcFHWyUbaKTjCOngQrgh48222265 = -792923097;    int GcFHWyUbaKTjCOngQrgh29128542 = -956935847;    int GcFHWyUbaKTjCOngQrgh35252459 = -788282079;    int GcFHWyUbaKTjCOngQrgh22527040 = -183197735;    int GcFHWyUbaKTjCOngQrgh29355124 = -637922941;    int GcFHWyUbaKTjCOngQrgh92146037 = -167356310;    int GcFHWyUbaKTjCOngQrgh84024917 = 44420467;    int GcFHWyUbaKTjCOngQrgh26436814 = -265712317;    int GcFHWyUbaKTjCOngQrgh90535292 = -738130219;    int GcFHWyUbaKTjCOngQrgh55420434 = -128244369;    int GcFHWyUbaKTjCOngQrgh82919785 = -819337243;    int GcFHWyUbaKTjCOngQrgh53196024 = -889256011;    int GcFHWyUbaKTjCOngQrgh45035589 = -707325976;    int GcFHWyUbaKTjCOngQrgh73595608 = -321624194;    int GcFHWyUbaKTjCOngQrgh18360182 = -229725357;    int GcFHWyUbaKTjCOngQrgh81534237 = -623283848;    int GcFHWyUbaKTjCOngQrgh55864717 = -517266726;    int GcFHWyUbaKTjCOngQrgh49270135 = -483438065;    int GcFHWyUbaKTjCOngQrgh16397852 = -552590568;    int GcFHWyUbaKTjCOngQrgh36386474 = -794418433;    int GcFHWyUbaKTjCOngQrgh85824595 = -616042328;    int GcFHWyUbaKTjCOngQrgh12961650 = -413573024;    int GcFHWyUbaKTjCOngQrgh19448511 = -175771348;    int GcFHWyUbaKTjCOngQrgh77622228 = -442948321;    int GcFHWyUbaKTjCOngQrgh55239542 = -145598322;    int GcFHWyUbaKTjCOngQrgh67792671 = -463059246;    int GcFHWyUbaKTjCOngQrgh60817113 = -520587627;    int GcFHWyUbaKTjCOngQrgh3785671 = -447859482;    int GcFHWyUbaKTjCOngQrgh99375567 = -583344955;    int GcFHWyUbaKTjCOngQrgh70818334 = -53278112;    int GcFHWyUbaKTjCOngQrgh75314826 = -633639384;    int GcFHWyUbaKTjCOngQrgh88120648 = -511283596;    int GcFHWyUbaKTjCOngQrgh22398626 = -341299250;    int GcFHWyUbaKTjCOngQrgh72695327 = -128536969;    int GcFHWyUbaKTjCOngQrgh29107581 = -45290233;    int GcFHWyUbaKTjCOngQrgh25183667 = -36151815;    int GcFHWyUbaKTjCOngQrgh95175836 = -143815902;    int GcFHWyUbaKTjCOngQrgh39784636 = -316903829;    int GcFHWyUbaKTjCOngQrgh30318998 = -480965287;    int GcFHWyUbaKTjCOngQrgh54924522 = -963166593;    int GcFHWyUbaKTjCOngQrgh15280047 = -268840356;    int GcFHWyUbaKTjCOngQrgh89790165 = 24545146;    int GcFHWyUbaKTjCOngQrgh94124921 = -409525376;    int GcFHWyUbaKTjCOngQrgh19836116 = -800241398;    int GcFHWyUbaKTjCOngQrgh46598726 = -148346138;    int GcFHWyUbaKTjCOngQrgh49622571 = -940203997;    int GcFHWyUbaKTjCOngQrgh76255185 = -879672371;    int GcFHWyUbaKTjCOngQrgh63735111 = -880907288;    int GcFHWyUbaKTjCOngQrgh67431267 = 86047350;    int GcFHWyUbaKTjCOngQrgh19274497 = -17164578;    int GcFHWyUbaKTjCOngQrgh91774962 = 2715530;    int GcFHWyUbaKTjCOngQrgh13898453 = -826543825;    int GcFHWyUbaKTjCOngQrgh28226755 = -628921826;    int GcFHWyUbaKTjCOngQrgh44912770 = -768545972;    int GcFHWyUbaKTjCOngQrgh75284942 = -814162186;    int GcFHWyUbaKTjCOngQrgh35260616 = -279350073;    int GcFHWyUbaKTjCOngQrgh9680031 = -681164500;    int GcFHWyUbaKTjCOngQrgh57630231 = -245333758;    int GcFHWyUbaKTjCOngQrgh67287498 = 62400586;    int GcFHWyUbaKTjCOngQrgh61562453 = -74863695;    int GcFHWyUbaKTjCOngQrgh31328925 = -646768683;    int GcFHWyUbaKTjCOngQrgh80239246 = -507720052;    int GcFHWyUbaKTjCOngQrgh27061246 = -783113470;     GcFHWyUbaKTjCOngQrgh63468412 = GcFHWyUbaKTjCOngQrgh35691437;     GcFHWyUbaKTjCOngQrgh35691437 = GcFHWyUbaKTjCOngQrgh2051575;     GcFHWyUbaKTjCOngQrgh2051575 = GcFHWyUbaKTjCOngQrgh97621863;     GcFHWyUbaKTjCOngQrgh97621863 = GcFHWyUbaKTjCOngQrgh94705060;     GcFHWyUbaKTjCOngQrgh94705060 = GcFHWyUbaKTjCOngQrgh7140740;     GcFHWyUbaKTjCOngQrgh7140740 = GcFHWyUbaKTjCOngQrgh58058137;     GcFHWyUbaKTjCOngQrgh58058137 = GcFHWyUbaKTjCOngQrgh89083445;     GcFHWyUbaKTjCOngQrgh89083445 = GcFHWyUbaKTjCOngQrgh98147831;     GcFHWyUbaKTjCOngQrgh98147831 = GcFHWyUbaKTjCOngQrgh89577119;     GcFHWyUbaKTjCOngQrgh89577119 = GcFHWyUbaKTjCOngQrgh184302;     GcFHWyUbaKTjCOngQrgh184302 = GcFHWyUbaKTjCOngQrgh18458014;     GcFHWyUbaKTjCOngQrgh18458014 = GcFHWyUbaKTjCOngQrgh38483415;     GcFHWyUbaKTjCOngQrgh38483415 = GcFHWyUbaKTjCOngQrgh32745255;     GcFHWyUbaKTjCOngQrgh32745255 = GcFHWyUbaKTjCOngQrgh3697870;     GcFHWyUbaKTjCOngQrgh3697870 = GcFHWyUbaKTjCOngQrgh33804864;     GcFHWyUbaKTjCOngQrgh33804864 = GcFHWyUbaKTjCOngQrgh77329846;     GcFHWyUbaKTjCOngQrgh77329846 = GcFHWyUbaKTjCOngQrgh54312208;     GcFHWyUbaKTjCOngQrgh54312208 = GcFHWyUbaKTjCOngQrgh30428296;     GcFHWyUbaKTjCOngQrgh30428296 = GcFHWyUbaKTjCOngQrgh62311676;     GcFHWyUbaKTjCOngQrgh62311676 = GcFHWyUbaKTjCOngQrgh59674122;     GcFHWyUbaKTjCOngQrgh59674122 = GcFHWyUbaKTjCOngQrgh47070560;     GcFHWyUbaKTjCOngQrgh47070560 = GcFHWyUbaKTjCOngQrgh99304963;     GcFHWyUbaKTjCOngQrgh99304963 = GcFHWyUbaKTjCOngQrgh16226980;     GcFHWyUbaKTjCOngQrgh16226980 = GcFHWyUbaKTjCOngQrgh84660213;     GcFHWyUbaKTjCOngQrgh84660213 = GcFHWyUbaKTjCOngQrgh75256549;     GcFHWyUbaKTjCOngQrgh75256549 = GcFHWyUbaKTjCOngQrgh29518512;     GcFHWyUbaKTjCOngQrgh29518512 = GcFHWyUbaKTjCOngQrgh2818596;     GcFHWyUbaKTjCOngQrgh2818596 = GcFHWyUbaKTjCOngQrgh21290775;     GcFHWyUbaKTjCOngQrgh21290775 = GcFHWyUbaKTjCOngQrgh37330719;     GcFHWyUbaKTjCOngQrgh37330719 = GcFHWyUbaKTjCOngQrgh85791449;     GcFHWyUbaKTjCOngQrgh85791449 = GcFHWyUbaKTjCOngQrgh808735;     GcFHWyUbaKTjCOngQrgh808735 = GcFHWyUbaKTjCOngQrgh47639679;     GcFHWyUbaKTjCOngQrgh47639679 = GcFHWyUbaKTjCOngQrgh63168588;     GcFHWyUbaKTjCOngQrgh63168588 = GcFHWyUbaKTjCOngQrgh44624607;     GcFHWyUbaKTjCOngQrgh44624607 = GcFHWyUbaKTjCOngQrgh81299243;     GcFHWyUbaKTjCOngQrgh81299243 = GcFHWyUbaKTjCOngQrgh61109537;     GcFHWyUbaKTjCOngQrgh61109537 = GcFHWyUbaKTjCOngQrgh48222265;     GcFHWyUbaKTjCOngQrgh48222265 = GcFHWyUbaKTjCOngQrgh29128542;     GcFHWyUbaKTjCOngQrgh29128542 = GcFHWyUbaKTjCOngQrgh35252459;     GcFHWyUbaKTjCOngQrgh35252459 = GcFHWyUbaKTjCOngQrgh22527040;     GcFHWyUbaKTjCOngQrgh22527040 = GcFHWyUbaKTjCOngQrgh29355124;     GcFHWyUbaKTjCOngQrgh29355124 = GcFHWyUbaKTjCOngQrgh92146037;     GcFHWyUbaKTjCOngQrgh92146037 = GcFHWyUbaKTjCOngQrgh84024917;     GcFHWyUbaKTjCOngQrgh84024917 = GcFHWyUbaKTjCOngQrgh26436814;     GcFHWyUbaKTjCOngQrgh26436814 = GcFHWyUbaKTjCOngQrgh90535292;     GcFHWyUbaKTjCOngQrgh90535292 = GcFHWyUbaKTjCOngQrgh55420434;     GcFHWyUbaKTjCOngQrgh55420434 = GcFHWyUbaKTjCOngQrgh82919785;     GcFHWyUbaKTjCOngQrgh82919785 = GcFHWyUbaKTjCOngQrgh53196024;     GcFHWyUbaKTjCOngQrgh53196024 = GcFHWyUbaKTjCOngQrgh45035589;     GcFHWyUbaKTjCOngQrgh45035589 = GcFHWyUbaKTjCOngQrgh73595608;     GcFHWyUbaKTjCOngQrgh73595608 = GcFHWyUbaKTjCOngQrgh18360182;     GcFHWyUbaKTjCOngQrgh18360182 = GcFHWyUbaKTjCOngQrgh81534237;     GcFHWyUbaKTjCOngQrgh81534237 = GcFHWyUbaKTjCOngQrgh55864717;     GcFHWyUbaKTjCOngQrgh55864717 = GcFHWyUbaKTjCOngQrgh49270135;     GcFHWyUbaKTjCOngQrgh49270135 = GcFHWyUbaKTjCOngQrgh16397852;     GcFHWyUbaKTjCOngQrgh16397852 = GcFHWyUbaKTjCOngQrgh36386474;     GcFHWyUbaKTjCOngQrgh36386474 = GcFHWyUbaKTjCOngQrgh85824595;     GcFHWyUbaKTjCOngQrgh85824595 = GcFHWyUbaKTjCOngQrgh12961650;     GcFHWyUbaKTjCOngQrgh12961650 = GcFHWyUbaKTjCOngQrgh19448511;     GcFHWyUbaKTjCOngQrgh19448511 = GcFHWyUbaKTjCOngQrgh77622228;     GcFHWyUbaKTjCOngQrgh77622228 = GcFHWyUbaKTjCOngQrgh55239542;     GcFHWyUbaKTjCOngQrgh55239542 = GcFHWyUbaKTjCOngQrgh67792671;     GcFHWyUbaKTjCOngQrgh67792671 = GcFHWyUbaKTjCOngQrgh60817113;     GcFHWyUbaKTjCOngQrgh60817113 = GcFHWyUbaKTjCOngQrgh3785671;     GcFHWyUbaKTjCOngQrgh3785671 = GcFHWyUbaKTjCOngQrgh99375567;     GcFHWyUbaKTjCOngQrgh99375567 = GcFHWyUbaKTjCOngQrgh70818334;     GcFHWyUbaKTjCOngQrgh70818334 = GcFHWyUbaKTjCOngQrgh75314826;     GcFHWyUbaKTjCOngQrgh75314826 = GcFHWyUbaKTjCOngQrgh88120648;     GcFHWyUbaKTjCOngQrgh88120648 = GcFHWyUbaKTjCOngQrgh22398626;     GcFHWyUbaKTjCOngQrgh22398626 = GcFHWyUbaKTjCOngQrgh72695327;     GcFHWyUbaKTjCOngQrgh72695327 = GcFHWyUbaKTjCOngQrgh29107581;     GcFHWyUbaKTjCOngQrgh29107581 = GcFHWyUbaKTjCOngQrgh25183667;     GcFHWyUbaKTjCOngQrgh25183667 = GcFHWyUbaKTjCOngQrgh95175836;     GcFHWyUbaKTjCOngQrgh95175836 = GcFHWyUbaKTjCOngQrgh39784636;     GcFHWyUbaKTjCOngQrgh39784636 = GcFHWyUbaKTjCOngQrgh30318998;     GcFHWyUbaKTjCOngQrgh30318998 = GcFHWyUbaKTjCOngQrgh54924522;     GcFHWyUbaKTjCOngQrgh54924522 = GcFHWyUbaKTjCOngQrgh15280047;     GcFHWyUbaKTjCOngQrgh15280047 = GcFHWyUbaKTjCOngQrgh89790165;     GcFHWyUbaKTjCOngQrgh89790165 = GcFHWyUbaKTjCOngQrgh94124921;     GcFHWyUbaKTjCOngQrgh94124921 = GcFHWyUbaKTjCOngQrgh19836116;     GcFHWyUbaKTjCOngQrgh19836116 = GcFHWyUbaKTjCOngQrgh46598726;     GcFHWyUbaKTjCOngQrgh46598726 = GcFHWyUbaKTjCOngQrgh49622571;     GcFHWyUbaKTjCOngQrgh49622571 = GcFHWyUbaKTjCOngQrgh76255185;     GcFHWyUbaKTjCOngQrgh76255185 = GcFHWyUbaKTjCOngQrgh63735111;     GcFHWyUbaKTjCOngQrgh63735111 = GcFHWyUbaKTjCOngQrgh67431267;     GcFHWyUbaKTjCOngQrgh67431267 = GcFHWyUbaKTjCOngQrgh19274497;     GcFHWyUbaKTjCOngQrgh19274497 = GcFHWyUbaKTjCOngQrgh91774962;     GcFHWyUbaKTjCOngQrgh91774962 = GcFHWyUbaKTjCOngQrgh13898453;     GcFHWyUbaKTjCOngQrgh13898453 = GcFHWyUbaKTjCOngQrgh28226755;     GcFHWyUbaKTjCOngQrgh28226755 = GcFHWyUbaKTjCOngQrgh44912770;     GcFHWyUbaKTjCOngQrgh44912770 = GcFHWyUbaKTjCOngQrgh75284942;     GcFHWyUbaKTjCOngQrgh75284942 = GcFHWyUbaKTjCOngQrgh35260616;     GcFHWyUbaKTjCOngQrgh35260616 = GcFHWyUbaKTjCOngQrgh9680031;     GcFHWyUbaKTjCOngQrgh9680031 = GcFHWyUbaKTjCOngQrgh57630231;     GcFHWyUbaKTjCOngQrgh57630231 = GcFHWyUbaKTjCOngQrgh67287498;     GcFHWyUbaKTjCOngQrgh67287498 = GcFHWyUbaKTjCOngQrgh61562453;     GcFHWyUbaKTjCOngQrgh61562453 = GcFHWyUbaKTjCOngQrgh31328925;     GcFHWyUbaKTjCOngQrgh31328925 = GcFHWyUbaKTjCOngQrgh80239246;     GcFHWyUbaKTjCOngQrgh80239246 = GcFHWyUbaKTjCOngQrgh27061246;     GcFHWyUbaKTjCOngQrgh27061246 = GcFHWyUbaKTjCOngQrgh63468412;}
// Junk Finished

// Junk Code By Peatreat & Thaisen's Gen
void TvBDsOVoTcEgOxXvyhZK33044043() {     int tjPPpGacJKWQvzNygVmn66387359 = -4386221;    int tjPPpGacJKWQvzNygVmn67015074 = -621503379;    int tjPPpGacJKWQvzNygVmn13983862 = -545234728;    int tjPPpGacJKWQvzNygVmn93981403 = -851061886;    int tjPPpGacJKWQvzNygVmn92432798 = -169041699;    int tjPPpGacJKWQvzNygVmn96431782 = -480755311;    int tjPPpGacJKWQvzNygVmn5381998 = -453739405;    int tjPPpGacJKWQvzNygVmn99467070 = -426846270;    int tjPPpGacJKWQvzNygVmn34624166 = 20876232;    int tjPPpGacJKWQvzNygVmn3411981 = -528692775;    int tjPPpGacJKWQvzNygVmn83502897 = -987427481;    int tjPPpGacJKWQvzNygVmn70866349 = 80728103;    int tjPPpGacJKWQvzNygVmn15119936 = -137981316;    int tjPPpGacJKWQvzNygVmn66943967 = -778219020;    int tjPPpGacJKWQvzNygVmn79906685 = -333657576;    int tjPPpGacJKWQvzNygVmn11845128 = -577217090;    int tjPPpGacJKWQvzNygVmn21191085 = 56281532;    int tjPPpGacJKWQvzNygVmn36225873 = -694075634;    int tjPPpGacJKWQvzNygVmn72075047 = -789995345;    int tjPPpGacJKWQvzNygVmn60216657 = -135199561;    int tjPPpGacJKWQvzNygVmn48101157 = -604163210;    int tjPPpGacJKWQvzNygVmn95903695 = -123486003;    int tjPPpGacJKWQvzNygVmn34425281 = -202925862;    int tjPPpGacJKWQvzNygVmn25005098 = -168616036;    int tjPPpGacJKWQvzNygVmn8996972 = -574914609;    int tjPPpGacJKWQvzNygVmn58792083 = -686584772;    int tjPPpGacJKWQvzNygVmn92679800 = -60118942;    int tjPPpGacJKWQvzNygVmn57938371 = -454181182;    int tjPPpGacJKWQvzNygVmn86438378 = -917344319;    int tjPPpGacJKWQvzNygVmn61781044 = -31388653;    int tjPPpGacJKWQvzNygVmn278170 = -459668066;    int tjPPpGacJKWQvzNygVmn18770615 = -828170762;    int tjPPpGacJKWQvzNygVmn17202247 = -588686924;    int tjPPpGacJKWQvzNygVmn63800768 = -329505532;    int tjPPpGacJKWQvzNygVmn80923635 = -182566009;    int tjPPpGacJKWQvzNygVmn86926175 = -574791172;    int tjPPpGacJKWQvzNygVmn86165798 = -736276390;    int tjPPpGacJKWQvzNygVmn60365892 = -583339753;    int tjPPpGacJKWQvzNygVmn10123348 = -893567054;    int tjPPpGacJKWQvzNygVmn34156438 = -927975011;    int tjPPpGacJKWQvzNygVmn9802354 = -176589105;    int tjPPpGacJKWQvzNygVmn6004382 = -156935406;    int tjPPpGacJKWQvzNygVmn26562989 = -107085096;    int tjPPpGacJKWQvzNygVmn83498 = -603870522;    int tjPPpGacJKWQvzNygVmn14391670 = -897732832;    int tjPPpGacJKWQvzNygVmn44174447 = -383027893;    int tjPPpGacJKWQvzNygVmn85727951 = -792961299;    int tjPPpGacJKWQvzNygVmn27362630 = -392584920;    int tjPPpGacJKWQvzNygVmn6520338 = -825033471;    int tjPPpGacJKWQvzNygVmn1220993 = 11666110;    int tjPPpGacJKWQvzNygVmn8029256 = -270335540;    int tjPPpGacJKWQvzNygVmn75762579 = -283191780;    int tjPPpGacJKWQvzNygVmn90023493 = -619086117;    int tjPPpGacJKWQvzNygVmn43221234 = -798353899;    int tjPPpGacJKWQvzNygVmn84942886 = -849583545;    int tjPPpGacJKWQvzNygVmn70483663 = -880900219;    int tjPPpGacJKWQvzNygVmn32589793 = -318577518;    int tjPPpGacJKWQvzNygVmn88978763 = -276618692;    int tjPPpGacJKWQvzNygVmn84984432 = -176147278;    int tjPPpGacJKWQvzNygVmn33640716 = -482456928;    int tjPPpGacJKWQvzNygVmn3751982 = -320636370;    int tjPPpGacJKWQvzNygVmn47443627 = -999558223;    int tjPPpGacJKWQvzNygVmn13028693 = -509501951;    int tjPPpGacJKWQvzNygVmn72843121 = -947735115;    int tjPPpGacJKWQvzNygVmn3133812 = 30975290;    int tjPPpGacJKWQvzNygVmn64732283 = -59256719;    int tjPPpGacJKWQvzNygVmn53664103 = -330584973;    int tjPPpGacJKWQvzNygVmn51319167 = -808475784;    int tjPPpGacJKWQvzNygVmn86020332 = -495653011;    int tjPPpGacJKWQvzNygVmn92980509 = -758866404;    int tjPPpGacJKWQvzNygVmn25679329 = -840940701;    int tjPPpGacJKWQvzNygVmn60825193 = -360378715;    int tjPPpGacJKWQvzNygVmn26102525 = -800508581;    int tjPPpGacJKWQvzNygVmn37918609 = -862020334;    int tjPPpGacJKWQvzNygVmn50414304 = -958610456;    int tjPPpGacJKWQvzNygVmn42096775 = -347227805;    int tjPPpGacJKWQvzNygVmn69340707 = 83599093;    int tjPPpGacJKWQvzNygVmn34341784 = -599055341;    int tjPPpGacJKWQvzNygVmn10613429 = -270883205;    int tjPPpGacJKWQvzNygVmn64822524 = -91886717;    int tjPPpGacJKWQvzNygVmn73064132 = -893623474;    int tjPPpGacJKWQvzNygVmn65317171 = -667534023;    int tjPPpGacJKWQvzNygVmn51418033 = -629147712;    int tjPPpGacJKWQvzNygVmn85217386 = -829010430;    int tjPPpGacJKWQvzNygVmn53751789 = -761053114;    int tjPPpGacJKWQvzNygVmn24515590 = -76476287;    int tjPPpGacJKWQvzNygVmn28747121 = -109084645;    int tjPPpGacJKWQvzNygVmn73981012 = -790333026;    int tjPPpGacJKWQvzNygVmn78857882 = -479921988;    int tjPPpGacJKWQvzNygVmn10439972 = -301665791;    int tjPPpGacJKWQvzNygVmn54336383 = -156213655;    int tjPPpGacJKWQvzNygVmn97187035 = -359657698;    int tjPPpGacJKWQvzNygVmn75381460 = -307192476;    int tjPPpGacJKWQvzNygVmn76482632 = -311110126;    int tjPPpGacJKWQvzNygVmn30404457 = -507338642;    int tjPPpGacJKWQvzNygVmn62358727 = -177030882;    int tjPPpGacJKWQvzNygVmn92975689 = -647433455;    int tjPPpGacJKWQvzNygVmn53719867 = -159349981;    int tjPPpGacJKWQvzNygVmn96949685 = -534845813;    int tjPPpGacJKWQvzNygVmn49659386 = -4386221;     tjPPpGacJKWQvzNygVmn66387359 = tjPPpGacJKWQvzNygVmn67015074;     tjPPpGacJKWQvzNygVmn67015074 = tjPPpGacJKWQvzNygVmn13983862;     tjPPpGacJKWQvzNygVmn13983862 = tjPPpGacJKWQvzNygVmn93981403;     tjPPpGacJKWQvzNygVmn93981403 = tjPPpGacJKWQvzNygVmn92432798;     tjPPpGacJKWQvzNygVmn92432798 = tjPPpGacJKWQvzNygVmn96431782;     tjPPpGacJKWQvzNygVmn96431782 = tjPPpGacJKWQvzNygVmn5381998;     tjPPpGacJKWQvzNygVmn5381998 = tjPPpGacJKWQvzNygVmn99467070;     tjPPpGacJKWQvzNygVmn99467070 = tjPPpGacJKWQvzNygVmn34624166;     tjPPpGacJKWQvzNygVmn34624166 = tjPPpGacJKWQvzNygVmn3411981;     tjPPpGacJKWQvzNygVmn3411981 = tjPPpGacJKWQvzNygVmn83502897;     tjPPpGacJKWQvzNygVmn83502897 = tjPPpGacJKWQvzNygVmn70866349;     tjPPpGacJKWQvzNygVmn70866349 = tjPPpGacJKWQvzNygVmn15119936;     tjPPpGacJKWQvzNygVmn15119936 = tjPPpGacJKWQvzNygVmn66943967;     tjPPpGacJKWQvzNygVmn66943967 = tjPPpGacJKWQvzNygVmn79906685;     tjPPpGacJKWQvzNygVmn79906685 = tjPPpGacJKWQvzNygVmn11845128;     tjPPpGacJKWQvzNygVmn11845128 = tjPPpGacJKWQvzNygVmn21191085;     tjPPpGacJKWQvzNygVmn21191085 = tjPPpGacJKWQvzNygVmn36225873;     tjPPpGacJKWQvzNygVmn36225873 = tjPPpGacJKWQvzNygVmn72075047;     tjPPpGacJKWQvzNygVmn72075047 = tjPPpGacJKWQvzNygVmn60216657;     tjPPpGacJKWQvzNygVmn60216657 = tjPPpGacJKWQvzNygVmn48101157;     tjPPpGacJKWQvzNygVmn48101157 = tjPPpGacJKWQvzNygVmn95903695;     tjPPpGacJKWQvzNygVmn95903695 = tjPPpGacJKWQvzNygVmn34425281;     tjPPpGacJKWQvzNygVmn34425281 = tjPPpGacJKWQvzNygVmn25005098;     tjPPpGacJKWQvzNygVmn25005098 = tjPPpGacJKWQvzNygVmn8996972;     tjPPpGacJKWQvzNygVmn8996972 = tjPPpGacJKWQvzNygVmn58792083;     tjPPpGacJKWQvzNygVmn58792083 = tjPPpGacJKWQvzNygVmn92679800;     tjPPpGacJKWQvzNygVmn92679800 = tjPPpGacJKWQvzNygVmn57938371;     tjPPpGacJKWQvzNygVmn57938371 = tjPPpGacJKWQvzNygVmn86438378;     tjPPpGacJKWQvzNygVmn86438378 = tjPPpGacJKWQvzNygVmn61781044;     tjPPpGacJKWQvzNygVmn61781044 = tjPPpGacJKWQvzNygVmn278170;     tjPPpGacJKWQvzNygVmn278170 = tjPPpGacJKWQvzNygVmn18770615;     tjPPpGacJKWQvzNygVmn18770615 = tjPPpGacJKWQvzNygVmn17202247;     tjPPpGacJKWQvzNygVmn17202247 = tjPPpGacJKWQvzNygVmn63800768;     tjPPpGacJKWQvzNygVmn63800768 = tjPPpGacJKWQvzNygVmn80923635;     tjPPpGacJKWQvzNygVmn80923635 = tjPPpGacJKWQvzNygVmn86926175;     tjPPpGacJKWQvzNygVmn86926175 = tjPPpGacJKWQvzNygVmn86165798;     tjPPpGacJKWQvzNygVmn86165798 = tjPPpGacJKWQvzNygVmn60365892;     tjPPpGacJKWQvzNygVmn60365892 = tjPPpGacJKWQvzNygVmn10123348;     tjPPpGacJKWQvzNygVmn10123348 = tjPPpGacJKWQvzNygVmn34156438;     tjPPpGacJKWQvzNygVmn34156438 = tjPPpGacJKWQvzNygVmn9802354;     tjPPpGacJKWQvzNygVmn9802354 = tjPPpGacJKWQvzNygVmn6004382;     tjPPpGacJKWQvzNygVmn6004382 = tjPPpGacJKWQvzNygVmn26562989;     tjPPpGacJKWQvzNygVmn26562989 = tjPPpGacJKWQvzNygVmn83498;     tjPPpGacJKWQvzNygVmn83498 = tjPPpGacJKWQvzNygVmn14391670;     tjPPpGacJKWQvzNygVmn14391670 = tjPPpGacJKWQvzNygVmn44174447;     tjPPpGacJKWQvzNygVmn44174447 = tjPPpGacJKWQvzNygVmn85727951;     tjPPpGacJKWQvzNygVmn85727951 = tjPPpGacJKWQvzNygVmn27362630;     tjPPpGacJKWQvzNygVmn27362630 = tjPPpGacJKWQvzNygVmn6520338;     tjPPpGacJKWQvzNygVmn6520338 = tjPPpGacJKWQvzNygVmn1220993;     tjPPpGacJKWQvzNygVmn1220993 = tjPPpGacJKWQvzNygVmn8029256;     tjPPpGacJKWQvzNygVmn8029256 = tjPPpGacJKWQvzNygVmn75762579;     tjPPpGacJKWQvzNygVmn75762579 = tjPPpGacJKWQvzNygVmn90023493;     tjPPpGacJKWQvzNygVmn90023493 = tjPPpGacJKWQvzNygVmn43221234;     tjPPpGacJKWQvzNygVmn43221234 = tjPPpGacJKWQvzNygVmn84942886;     tjPPpGacJKWQvzNygVmn84942886 = tjPPpGacJKWQvzNygVmn70483663;     tjPPpGacJKWQvzNygVmn70483663 = tjPPpGacJKWQvzNygVmn32589793;     tjPPpGacJKWQvzNygVmn32589793 = tjPPpGacJKWQvzNygVmn88978763;     tjPPpGacJKWQvzNygVmn88978763 = tjPPpGacJKWQvzNygVmn84984432;     tjPPpGacJKWQvzNygVmn84984432 = tjPPpGacJKWQvzNygVmn33640716;     tjPPpGacJKWQvzNygVmn33640716 = tjPPpGacJKWQvzNygVmn3751982;     tjPPpGacJKWQvzNygVmn3751982 = tjPPpGacJKWQvzNygVmn47443627;     tjPPpGacJKWQvzNygVmn47443627 = tjPPpGacJKWQvzNygVmn13028693;     tjPPpGacJKWQvzNygVmn13028693 = tjPPpGacJKWQvzNygVmn72843121;     tjPPpGacJKWQvzNygVmn72843121 = tjPPpGacJKWQvzNygVmn3133812;     tjPPpGacJKWQvzNygVmn3133812 = tjPPpGacJKWQvzNygVmn64732283;     tjPPpGacJKWQvzNygVmn64732283 = tjPPpGacJKWQvzNygVmn53664103;     tjPPpGacJKWQvzNygVmn53664103 = tjPPpGacJKWQvzNygVmn51319167;     tjPPpGacJKWQvzNygVmn51319167 = tjPPpGacJKWQvzNygVmn86020332;     tjPPpGacJKWQvzNygVmn86020332 = tjPPpGacJKWQvzNygVmn92980509;     tjPPpGacJKWQvzNygVmn92980509 = tjPPpGacJKWQvzNygVmn25679329;     tjPPpGacJKWQvzNygVmn25679329 = tjPPpGacJKWQvzNygVmn60825193;     tjPPpGacJKWQvzNygVmn60825193 = tjPPpGacJKWQvzNygVmn26102525;     tjPPpGacJKWQvzNygVmn26102525 = tjPPpGacJKWQvzNygVmn37918609;     tjPPpGacJKWQvzNygVmn37918609 = tjPPpGacJKWQvzNygVmn50414304;     tjPPpGacJKWQvzNygVmn50414304 = tjPPpGacJKWQvzNygVmn42096775;     tjPPpGacJKWQvzNygVmn42096775 = tjPPpGacJKWQvzNygVmn69340707;     tjPPpGacJKWQvzNygVmn69340707 = tjPPpGacJKWQvzNygVmn34341784;     tjPPpGacJKWQvzNygVmn34341784 = tjPPpGacJKWQvzNygVmn10613429;     tjPPpGacJKWQvzNygVmn10613429 = tjPPpGacJKWQvzNygVmn64822524;     tjPPpGacJKWQvzNygVmn64822524 = tjPPpGacJKWQvzNygVmn73064132;     tjPPpGacJKWQvzNygVmn73064132 = tjPPpGacJKWQvzNygVmn65317171;     tjPPpGacJKWQvzNygVmn65317171 = tjPPpGacJKWQvzNygVmn51418033;     tjPPpGacJKWQvzNygVmn51418033 = tjPPpGacJKWQvzNygVmn85217386;     tjPPpGacJKWQvzNygVmn85217386 = tjPPpGacJKWQvzNygVmn53751789;     tjPPpGacJKWQvzNygVmn53751789 = tjPPpGacJKWQvzNygVmn24515590;     tjPPpGacJKWQvzNygVmn24515590 = tjPPpGacJKWQvzNygVmn28747121;     tjPPpGacJKWQvzNygVmn28747121 = tjPPpGacJKWQvzNygVmn73981012;     tjPPpGacJKWQvzNygVmn73981012 = tjPPpGacJKWQvzNygVmn78857882;     tjPPpGacJKWQvzNygVmn78857882 = tjPPpGacJKWQvzNygVmn10439972;     tjPPpGacJKWQvzNygVmn10439972 = tjPPpGacJKWQvzNygVmn54336383;     tjPPpGacJKWQvzNygVmn54336383 = tjPPpGacJKWQvzNygVmn97187035;     tjPPpGacJKWQvzNygVmn97187035 = tjPPpGacJKWQvzNygVmn75381460;     tjPPpGacJKWQvzNygVmn75381460 = tjPPpGacJKWQvzNygVmn76482632;     tjPPpGacJKWQvzNygVmn76482632 = tjPPpGacJKWQvzNygVmn30404457;     tjPPpGacJKWQvzNygVmn30404457 = tjPPpGacJKWQvzNygVmn62358727;     tjPPpGacJKWQvzNygVmn62358727 = tjPPpGacJKWQvzNygVmn92975689;     tjPPpGacJKWQvzNygVmn92975689 = tjPPpGacJKWQvzNygVmn53719867;     tjPPpGacJKWQvzNygVmn53719867 = tjPPpGacJKWQvzNygVmn96949685;     tjPPpGacJKWQvzNygVmn96949685 = tjPPpGacJKWQvzNygVmn49659386;     tjPPpGacJKWQvzNygVmn49659386 = tjPPpGacJKWQvzNygVmn66387359;}
// Junk Finished

// Junk Code By Peatreat & Thaisen's Gen
void PkCowNMdHuFqGhwppcLi9386360() {     int vDRtacOnUODSOoyXJsSB98648167 = -680033983;    int vDRtacOnUODSOoyXJsSB68237509 = -195120264;    int vDRtacOnUODSOoyXJsSB47260441 = -648094111;    int vDRtacOnUODSOoyXJsSB35489517 = -671297596;    int vDRtacOnUODSOoyXJsSB77550317 = -818775259;    int vDRtacOnUODSOoyXJsSB30528495 = -448730581;    int vDRtacOnUODSOoyXJsSB2874299 = -638181238;    int vDRtacOnUODSOoyXJsSB22222367 = -160995175;    int vDRtacOnUODSOoyXJsSB7975610 = -955427169;    int vDRtacOnUODSOoyXJsSB90919895 = -701559583;    int vDRtacOnUODSOoyXJsSB44892938 = -475631724;    int vDRtacOnUODSOoyXJsSB92019790 = -690429055;    int vDRtacOnUODSOoyXJsSB43781984 = -444462775;    int vDRtacOnUODSOoyXJsSB71139866 = -207766892;    int vDRtacOnUODSOoyXJsSB18567591 = -576014346;    int vDRtacOnUODSOoyXJsSB31921298 = -158504763;    int vDRtacOnUODSOoyXJsSB27052256 = 40346954;    int vDRtacOnUODSOoyXJsSB20846577 = -702969033;    int vDRtacOnUODSOoyXJsSB52442287 = -458687035;    int vDRtacOnUODSOoyXJsSB71408692 = -839955704;    int vDRtacOnUODSOoyXJsSB61061861 = -891225427;    int vDRtacOnUODSOoyXJsSB61637332 = -295155681;    int vDRtacOnUODSOoyXJsSB4480186 = 27453467;    int vDRtacOnUODSOoyXJsSB43583788 = -284584518;    int vDRtacOnUODSOoyXJsSB97652030 = -652962790;    int vDRtacOnUODSOoyXJsSB28304113 = -154605261;    int vDRtacOnUODSOoyXJsSB17801887 = -58176441;    int vDRtacOnUODSOoyXJsSB40679446 = -182745577;    int vDRtacOnUODSOoyXJsSB32121942 = -499402391;    int vDRtacOnUODSOoyXJsSB23149144 = -77282129;    int vDRtacOnUODSOoyXJsSB41105571 = -214317674;    int vDRtacOnUODSOoyXJsSB76944891 = -979417697;    int vDRtacOnUODSOoyXJsSB89271875 = -817805260;    int vDRtacOnUODSOoyXJsSB99350810 = -662181225;    int vDRtacOnUODSOoyXJsSB21556466 = -595986771;    int vDRtacOnUODSOoyXJsSB75388038 = -587157139;    int vDRtacOnUODSOoyXJsSB79371942 = -953408437;    int vDRtacOnUODSOoyXJsSB89634709 = -584905981;    int vDRtacOnUODSOoyXJsSB47898462 = -487151040;    int vDRtacOnUODSOoyXJsSB9830011 = -59491268;    int vDRtacOnUODSOoyXJsSB24860051 = -973905496;    int vDRtacOnUODSOoyXJsSB39868310 = -100350884;    int vDRtacOnUODSOoyXJsSB72296339 = -619190085;    int vDRtacOnUODSOoyXJsSB68865676 = -737150982;    int vDRtacOnUODSOoyXJsSB90410518 = -594366354;    int vDRtacOnUODSOoyXJsSB31835735 = -81633083;    int vDRtacOnUODSOoyXJsSB17602059 = -722889052;    int vDRtacOnUODSOoyXJsSB81574845 = -122463657;    int vDRtacOnUODSOoyXJsSB73061589 = -338810823;    int vDRtacOnUODSOoyXJsSB69024332 = -773357812;    int vDRtacOnUODSOoyXJsSB87532948 = -251029772;    int vDRtacOnUODSOoyXJsSB5840729 = -749584229;    int vDRtacOnUODSOoyXJsSB18885442 = -579144353;    int vDRtacOnUODSOoyXJsSB50600717 = -950479426;    int vDRtacOnUODSOoyXJsSB48324499 = -861687389;    int vDRtacOnUODSOoyXJsSB37010836 = -284878303;    int vDRtacOnUODSOoyXJsSB63757323 = -122573731;    int vDRtacOnUODSOoyXJsSB3676654 = -263509593;    int vDRtacOnUODSOoyXJsSB37837487 = 81665193;    int vDRtacOnUODSOoyXJsSB49246205 = -564169998;    int vDRtacOnUODSOoyXJsSB12726609 = -290554141;    int vDRtacOnUODSOoyXJsSB62194852 = -355435661;    int vDRtacOnUODSOoyXJsSB90100424 = -661592785;    int vDRtacOnUODSOoyXJsSB84826465 = -778145041;    int vDRtacOnUODSOoyXJsSB49814324 = -387241909;    int vDRtacOnUODSOoyXJsSB67948047 = -496214028;    int vDRtacOnUODSOoyXJsSB2747916 = -872623796;    int vDRtacOnUODSOoyXJsSB44431173 = -782281551;    int vDRtacOnUODSOoyXJsSB49583401 = -611780122;    int vDRtacOnUODSOoyXJsSB43179552 = -988857208;    int vDRtacOnUODSOoyXJsSB52549356 = -205096326;    int vDRtacOnUODSOoyXJsSB37417547 = -374747065;    int vDRtacOnUODSOoyXJsSB72948114 = -115817993;    int vDRtacOnUODSOoyXJsSB42612276 = -299195767;    int vDRtacOnUODSOoyXJsSB46548641 = -866050209;    int vDRtacOnUODSOoyXJsSB21193552 = -690874543;    int vDRtacOnUODSOoyXJsSB89340992 = -675965597;    int vDRtacOnUODSOoyXJsSB35614510 = -235395552;    int vDRtacOnUODSOoyXJsSB53173269 = -690218165;    int vDRtacOnUODSOoyXJsSB65816295 = -471329707;    int vDRtacOnUODSOoyXJsSB10702055 = -431716210;    int vDRtacOnUODSOoyXJsSB36227041 = -935712784;    int vDRtacOnUODSOoyXJsSB67617857 = -843934755;    int vDRtacOnUODSOoyXJsSB63097609 = -726044580;    int vDRtacOnUODSOoyXJsSB35616196 = -826252357;    int vDRtacOnUODSOoyXJsSB35264843 = -464733446;    int vDRtacOnUODSOoyXJsSB58059449 = -300273345;    int vDRtacOnUODSOoyXJsSB38671159 = -867325834;    int vDRtacOnUODSOoyXJsSB51026312 = -800493836;    int vDRtacOnUODSOoyXJsSB84545629 = -211108468;    int vDRtacOnUODSOoyXJsSB11630715 = -364583408;    int vDRtacOnUODSOoyXJsSB75695288 = -589898844;    int vDRtacOnUODSOoyXJsSB51797223 = -566571175;    int vDRtacOnUODSOoyXJsSB98652257 = -922981042;    int vDRtacOnUODSOoyXJsSB97103402 = -768937128;    int vDRtacOnUODSOoyXJsSB62665198 = -518469835;    int vDRtacOnUODSOoyXJsSB49767885 = -438758099;    int vDRtacOnUODSOoyXJsSB87469874 = -841045044;    int vDRtacOnUODSOoyXJsSB19051352 = -249909073;    int vDRtacOnUODSOoyXJsSB22462472 = -680033983;     vDRtacOnUODSOoyXJsSB98648167 = vDRtacOnUODSOoyXJsSB68237509;     vDRtacOnUODSOoyXJsSB68237509 = vDRtacOnUODSOoyXJsSB47260441;     vDRtacOnUODSOoyXJsSB47260441 = vDRtacOnUODSOoyXJsSB35489517;     vDRtacOnUODSOoyXJsSB35489517 = vDRtacOnUODSOoyXJsSB77550317;     vDRtacOnUODSOoyXJsSB77550317 = vDRtacOnUODSOoyXJsSB30528495;     vDRtacOnUODSOoyXJsSB30528495 = vDRtacOnUODSOoyXJsSB2874299;     vDRtacOnUODSOoyXJsSB2874299 = vDRtacOnUODSOoyXJsSB22222367;     vDRtacOnUODSOoyXJsSB22222367 = vDRtacOnUODSOoyXJsSB7975610;     vDRtacOnUODSOoyXJsSB7975610 = vDRtacOnUODSOoyXJsSB90919895;     vDRtacOnUODSOoyXJsSB90919895 = vDRtacOnUODSOoyXJsSB44892938;     vDRtacOnUODSOoyXJsSB44892938 = vDRtacOnUODSOoyXJsSB92019790;     vDRtacOnUODSOoyXJsSB92019790 = vDRtacOnUODSOoyXJsSB43781984;     vDRtacOnUODSOoyXJsSB43781984 = vDRtacOnUODSOoyXJsSB71139866;     vDRtacOnUODSOoyXJsSB71139866 = vDRtacOnUODSOoyXJsSB18567591;     vDRtacOnUODSOoyXJsSB18567591 = vDRtacOnUODSOoyXJsSB31921298;     vDRtacOnUODSOoyXJsSB31921298 = vDRtacOnUODSOoyXJsSB27052256;     vDRtacOnUODSOoyXJsSB27052256 = vDRtacOnUODSOoyXJsSB20846577;     vDRtacOnUODSOoyXJsSB20846577 = vDRtacOnUODSOoyXJsSB52442287;     vDRtacOnUODSOoyXJsSB52442287 = vDRtacOnUODSOoyXJsSB71408692;     vDRtacOnUODSOoyXJsSB71408692 = vDRtacOnUODSOoyXJsSB61061861;     vDRtacOnUODSOoyXJsSB61061861 = vDRtacOnUODSOoyXJsSB61637332;     vDRtacOnUODSOoyXJsSB61637332 = vDRtacOnUODSOoyXJsSB4480186;     vDRtacOnUODSOoyXJsSB4480186 = vDRtacOnUODSOoyXJsSB43583788;     vDRtacOnUODSOoyXJsSB43583788 = vDRtacOnUODSOoyXJsSB97652030;     vDRtacOnUODSOoyXJsSB97652030 = vDRtacOnUODSOoyXJsSB28304113;     vDRtacOnUODSOoyXJsSB28304113 = vDRtacOnUODSOoyXJsSB17801887;     vDRtacOnUODSOoyXJsSB17801887 = vDRtacOnUODSOoyXJsSB40679446;     vDRtacOnUODSOoyXJsSB40679446 = vDRtacOnUODSOoyXJsSB32121942;     vDRtacOnUODSOoyXJsSB32121942 = vDRtacOnUODSOoyXJsSB23149144;     vDRtacOnUODSOoyXJsSB23149144 = vDRtacOnUODSOoyXJsSB41105571;     vDRtacOnUODSOoyXJsSB41105571 = vDRtacOnUODSOoyXJsSB76944891;     vDRtacOnUODSOoyXJsSB76944891 = vDRtacOnUODSOoyXJsSB89271875;     vDRtacOnUODSOoyXJsSB89271875 = vDRtacOnUODSOoyXJsSB99350810;     vDRtacOnUODSOoyXJsSB99350810 = vDRtacOnUODSOoyXJsSB21556466;     vDRtacOnUODSOoyXJsSB21556466 = vDRtacOnUODSOoyXJsSB75388038;     vDRtacOnUODSOoyXJsSB75388038 = vDRtacOnUODSOoyXJsSB79371942;     vDRtacOnUODSOoyXJsSB79371942 = vDRtacOnUODSOoyXJsSB89634709;     vDRtacOnUODSOoyXJsSB89634709 = vDRtacOnUODSOoyXJsSB47898462;     vDRtacOnUODSOoyXJsSB47898462 = vDRtacOnUODSOoyXJsSB9830011;     vDRtacOnUODSOoyXJsSB9830011 = vDRtacOnUODSOoyXJsSB24860051;     vDRtacOnUODSOoyXJsSB24860051 = vDRtacOnUODSOoyXJsSB39868310;     vDRtacOnUODSOoyXJsSB39868310 = vDRtacOnUODSOoyXJsSB72296339;     vDRtacOnUODSOoyXJsSB72296339 = vDRtacOnUODSOoyXJsSB68865676;     vDRtacOnUODSOoyXJsSB68865676 = vDRtacOnUODSOoyXJsSB90410518;     vDRtacOnUODSOoyXJsSB90410518 = vDRtacOnUODSOoyXJsSB31835735;     vDRtacOnUODSOoyXJsSB31835735 = vDRtacOnUODSOoyXJsSB17602059;     vDRtacOnUODSOoyXJsSB17602059 = vDRtacOnUODSOoyXJsSB81574845;     vDRtacOnUODSOoyXJsSB81574845 = vDRtacOnUODSOoyXJsSB73061589;     vDRtacOnUODSOoyXJsSB73061589 = vDRtacOnUODSOoyXJsSB69024332;     vDRtacOnUODSOoyXJsSB69024332 = vDRtacOnUODSOoyXJsSB87532948;     vDRtacOnUODSOoyXJsSB87532948 = vDRtacOnUODSOoyXJsSB5840729;     vDRtacOnUODSOoyXJsSB5840729 = vDRtacOnUODSOoyXJsSB18885442;     vDRtacOnUODSOoyXJsSB18885442 = vDRtacOnUODSOoyXJsSB50600717;     vDRtacOnUODSOoyXJsSB50600717 = vDRtacOnUODSOoyXJsSB48324499;     vDRtacOnUODSOoyXJsSB48324499 = vDRtacOnUODSOoyXJsSB37010836;     vDRtacOnUODSOoyXJsSB37010836 = vDRtacOnUODSOoyXJsSB63757323;     vDRtacOnUODSOoyXJsSB63757323 = vDRtacOnUODSOoyXJsSB3676654;     vDRtacOnUODSOoyXJsSB3676654 = vDRtacOnUODSOoyXJsSB37837487;     vDRtacOnUODSOoyXJsSB37837487 = vDRtacOnUODSOoyXJsSB49246205;     vDRtacOnUODSOoyXJsSB49246205 = vDRtacOnUODSOoyXJsSB12726609;     vDRtacOnUODSOoyXJsSB12726609 = vDRtacOnUODSOoyXJsSB62194852;     vDRtacOnUODSOoyXJsSB62194852 = vDRtacOnUODSOoyXJsSB90100424;     vDRtacOnUODSOoyXJsSB90100424 = vDRtacOnUODSOoyXJsSB84826465;     vDRtacOnUODSOoyXJsSB84826465 = vDRtacOnUODSOoyXJsSB49814324;     vDRtacOnUODSOoyXJsSB49814324 = vDRtacOnUODSOoyXJsSB67948047;     vDRtacOnUODSOoyXJsSB67948047 = vDRtacOnUODSOoyXJsSB2747916;     vDRtacOnUODSOoyXJsSB2747916 = vDRtacOnUODSOoyXJsSB44431173;     vDRtacOnUODSOoyXJsSB44431173 = vDRtacOnUODSOoyXJsSB49583401;     vDRtacOnUODSOoyXJsSB49583401 = vDRtacOnUODSOoyXJsSB43179552;     vDRtacOnUODSOoyXJsSB43179552 = vDRtacOnUODSOoyXJsSB52549356;     vDRtacOnUODSOoyXJsSB52549356 = vDRtacOnUODSOoyXJsSB37417547;     vDRtacOnUODSOoyXJsSB37417547 = vDRtacOnUODSOoyXJsSB72948114;     vDRtacOnUODSOoyXJsSB72948114 = vDRtacOnUODSOoyXJsSB42612276;     vDRtacOnUODSOoyXJsSB42612276 = vDRtacOnUODSOoyXJsSB46548641;     vDRtacOnUODSOoyXJsSB46548641 = vDRtacOnUODSOoyXJsSB21193552;     vDRtacOnUODSOoyXJsSB21193552 = vDRtacOnUODSOoyXJsSB89340992;     vDRtacOnUODSOoyXJsSB89340992 = vDRtacOnUODSOoyXJsSB35614510;     vDRtacOnUODSOoyXJsSB35614510 = vDRtacOnUODSOoyXJsSB53173269;     vDRtacOnUODSOoyXJsSB53173269 = vDRtacOnUODSOoyXJsSB65816295;     vDRtacOnUODSOoyXJsSB65816295 = vDRtacOnUODSOoyXJsSB10702055;     vDRtacOnUODSOoyXJsSB10702055 = vDRtacOnUODSOoyXJsSB36227041;     vDRtacOnUODSOoyXJsSB36227041 = vDRtacOnUODSOoyXJsSB67617857;     vDRtacOnUODSOoyXJsSB67617857 = vDRtacOnUODSOoyXJsSB63097609;     vDRtacOnUODSOoyXJsSB63097609 = vDRtacOnUODSOoyXJsSB35616196;     vDRtacOnUODSOoyXJsSB35616196 = vDRtacOnUODSOoyXJsSB35264843;     vDRtacOnUODSOoyXJsSB35264843 = vDRtacOnUODSOoyXJsSB58059449;     vDRtacOnUODSOoyXJsSB58059449 = vDRtacOnUODSOoyXJsSB38671159;     vDRtacOnUODSOoyXJsSB38671159 = vDRtacOnUODSOoyXJsSB51026312;     vDRtacOnUODSOoyXJsSB51026312 = vDRtacOnUODSOoyXJsSB84545629;     vDRtacOnUODSOoyXJsSB84545629 = vDRtacOnUODSOoyXJsSB11630715;     vDRtacOnUODSOoyXJsSB11630715 = vDRtacOnUODSOoyXJsSB75695288;     vDRtacOnUODSOoyXJsSB75695288 = vDRtacOnUODSOoyXJsSB51797223;     vDRtacOnUODSOoyXJsSB51797223 = vDRtacOnUODSOoyXJsSB98652257;     vDRtacOnUODSOoyXJsSB98652257 = vDRtacOnUODSOoyXJsSB97103402;     vDRtacOnUODSOoyXJsSB97103402 = vDRtacOnUODSOoyXJsSB62665198;     vDRtacOnUODSOoyXJsSB62665198 = vDRtacOnUODSOoyXJsSB49767885;     vDRtacOnUODSOoyXJsSB49767885 = vDRtacOnUODSOoyXJsSB87469874;     vDRtacOnUODSOoyXJsSB87469874 = vDRtacOnUODSOoyXJsSB19051352;     vDRtacOnUODSOoyXJsSB19051352 = vDRtacOnUODSOoyXJsSB22462472;     vDRtacOnUODSOoyXJsSB22462472 = vDRtacOnUODSOoyXJsSB98648167;}
// Junk Finished

// Junk Code By Peatreat & Thaisen's Gen
void fYuMvRmEJOpjlceLQVLf33486146() {     int gDVRMOcxoeBRJyRcOoMd1567115 = 98693265;    int gDVRMOcxoeBRJyRcOoMd99561145 = -697785322;    int gDVRMOcxoeBRJyRcOoMd59192728 = -136119340;    int gDVRMOcxoeBRJyRcOoMd31849057 = -861130865;    int gDVRMOcxoeBRJyRcOoMd75278055 = -783559844;    int gDVRMOcxoeBRJyRcOoMd19819538 = -418854190;    int gDVRMOcxoeBRJyRcOoMd50198159 = -16862314;    int gDVRMOcxoeBRJyRcOoMd32605992 = -537783854;    int gDVRMOcxoeBRJyRcOoMd44451943 = -111431829;    int gDVRMOcxoeBRJyRcOoMd4754757 = -438714870;    int gDVRMOcxoeBRJyRcOoMd28211534 = -39265825;    int gDVRMOcxoeBRJyRcOoMd44428126 = -941871643;    int gDVRMOcxoeBRJyRcOoMd20418506 = -638822818;    int gDVRMOcxoeBRJyRcOoMd5338578 = -193189924;    int gDVRMOcxoeBRJyRcOoMd94776406 = 94591732;    int gDVRMOcxoeBRJyRcOoMd9961562 = -76980371;    int gDVRMOcxoeBRJyRcOoMd70913494 = -65158184;    int gDVRMOcxoeBRJyRcOoMd2760242 = -303957005;    int gDVRMOcxoeBRJyRcOoMd94089038 = -216584399;    int gDVRMOcxoeBRJyRcOoMd69313674 = -375053702;    int gDVRMOcxoeBRJyRcOoMd49488897 = -276500409;    int gDVRMOcxoeBRJyRcOoMd10470468 = -288118781;    int gDVRMOcxoeBRJyRcOoMd39600503 = -951052507;    int gDVRMOcxoeBRJyRcOoMd52361906 = -112033383;    int gDVRMOcxoeBRJyRcOoMd21988788 = 19778195;    int gDVRMOcxoeBRJyRcOoMd11839646 = -912704266;    int gDVRMOcxoeBRJyRcOoMd80963176 = -150612002;    int gDVRMOcxoeBRJyRcOoMd95799220 = -907466752;    int gDVRMOcxoeBRJyRcOoMd97269545 = -829748364;    int gDVRMOcxoeBRJyRcOoMd47599469 = 93860699;    int gDVRMOcxoeBRJyRcOoMd55592291 = -430307733;    int gDVRMOcxoeBRJyRcOoMd94906771 = 32859967;    int gDVRMOcxoeBRJyRcOoMd58834443 = -791940988;    int gDVRMOcxoeBRJyRcOoMd99982991 = -681704868;    int gDVRMOcxoeBRJyRcOoMd57855494 = -597040388;    int gDVRMOcxoeBRJyRcOoMd81014970 = -598983907;    int gDVRMOcxoeBRJyRcOoMd4428204 = -159480314;    int gDVRMOcxoeBRJyRcOoMd1778337 = -375322638;    int gDVRMOcxoeBRJyRcOoMd28893269 = -423782246;    int gDVRMOcxoeBRJyRcOoMd8733990 = -199184200;    int gDVRMOcxoeBRJyRcOoMd12135365 = -967296866;    int gDVRMOcxoeBRJyRcOoMd16517569 = -719363349;    int gDVRMOcxoeBRJyRcOoMd6713291 = -558918871;    int gDVRMOcxoeBRJyRcOoMd84924256 = -285441970;    int gDVRMOcxoeBRJyRcOoMd78365374 = -126386868;    int gDVRMOcxoeBRJyRcOoMd85474889 = -826530757;    int gDVRMOcxoeBRJyRcOoMd47909576 = -287605982;    int gDVRMOcxoeBRJyRcOoMd26017690 = -795711334;    int gDVRMOcxoeBRJyRcOoMd26385903 = -274588282;    int gDVRMOcxoeBRJyRcOoMd25209736 = -54365725;    int gDVRMOcxoeBRJyRcOoMd21966596 = -199741118;    int gDVRMOcxoeBRJyRcOoMd63243126 = -803050651;    int gDVRMOcxoeBRJyRcOoMd27374699 = -574946622;    int gDVRMOcxoeBRJyRcOoMd37957234 = -131566598;    int gDVRMOcxoeBRJyRcOoMd83997249 = -127832869;    int gDVRMOcxoeBRJyRcOoMd91096647 = -613187954;    int gDVRMOcxoeBRJyRcOoMd59960643 = -746732816;    int gDVRMOcxoeBRJyRcOoMd6830822 = 75914043;    int gDVRMOcxoeBRJyRcOoMd9860269 = -780909060;    int gDVRMOcxoeBRJyRcOoMd63438409 = -870855578;    int gDVRMOcxoeBRJyRcOoMd38856362 = -168242189;    int gDVRMOcxoeBRJyRcOoMd54398938 = -109395563;    int gDVRMOcxoeBRJyRcOoMd35336446 = -708035490;    int gDVRMOcxoeBRJyRcOoMd96852473 = -105292529;    int gDVRMOcxoeBRJyRcOoMd49162465 = 91592863;    int gDVRMOcxoeBRJyRcOoMd33304763 = 27874208;    int gDVRMOcxoeBRJyRcOoMd85593683 = -49930656;    int gDVRMOcxoeBRJyRcOoMd20435514 = -957117950;    int gDVRMOcxoeBRJyRcOoMd47483084 = -596149536;    int gDVRMOcxoeBRJyRcOoMd13761436 = -306424361;    int gDVRMOcxoeBRJyRcOoMd5533358 = -917500058;    int gDVRMOcxoeBRJyRcOoMd69135158 = -689835547;    int gDVRMOcxoeBRJyRcOoMd73866972 = -880174759;    int gDVRMOcxoeBRJyRcOoMd85355049 = 82599801;    int gDVRMOcxoeBRJyRcOoMd57178309 = -407756836;    int gDVRMOcxoeBRJyRcOoMd32971328 = -557137061;    int gDVRMOcxoeBRJyRcOoMd3757177 = -729199911;    int gDVRMOcxoeBRJyRcOoMd54676247 = -565610537;    int gDVRMOcxoeBRJyRcOoMd73996532 = -985646515;    int gDVRMOcxoeBRJyRcOoMd36513898 = -153691048;    int gDVRMOcxoeBRJyRcOoMd63930070 = -525098285;    int gDVRMOcxoeBRJyRcOoMd54945486 = -354900668;    int gDVRMOcxoeBRJyRcOoMd69413318 = -532878470;    int gDVRMOcxoeBRJyRcOoMd72059810 = -675382639;    int gDVRMOcxoeBRJyRcOoMd25632874 = -706398183;    int gDVRMOcxoeBRJyRcOoMd92349165 = -627257082;    int gDVRMOcxoeBRJyRcOoMd67532072 = -392193412;    int gDVRMOcxoeBRJyRcOoMd20877209 = -560374390;    int gDVRMOcxoeBRJyRcOoMd15985742 = -453872000;    int gDVRMOcxoeBRJyRcOoMd66758846 = -983852434;    int gDVRMOcxoeBRJyRcOoMd21054328 = -852251092;    int gDVRMOcxoeBRJyRcOoMd97597381 = -135394357;    int gDVRMOcxoeBRJyRcOoMd91918067 = -594413578;    int gDVRMOcxoeBRJyRcOoMd65454859 = -552926669;    int gDVRMOcxoeBRJyRcOoMd69877627 = 69057988;    int gDVRMOcxoeBRJyRcOoMd57736427 = -757901304;    int gDVRMOcxoeBRJyRcOoMd81181122 = 88672141;    int gDVRMOcxoeBRJyRcOoMd9860817 = -353626342;    int gDVRMOcxoeBRJyRcOoMd35761791 = -277034834;    int gDVRMOcxoeBRJyRcOoMd45060611 = 98693265;     gDVRMOcxoeBRJyRcOoMd1567115 = gDVRMOcxoeBRJyRcOoMd99561145;     gDVRMOcxoeBRJyRcOoMd99561145 = gDVRMOcxoeBRJyRcOoMd59192728;     gDVRMOcxoeBRJyRcOoMd59192728 = gDVRMOcxoeBRJyRcOoMd31849057;     gDVRMOcxoeBRJyRcOoMd31849057 = gDVRMOcxoeBRJyRcOoMd75278055;     gDVRMOcxoeBRJyRcOoMd75278055 = gDVRMOcxoeBRJyRcOoMd19819538;     gDVRMOcxoeBRJyRcOoMd19819538 = gDVRMOcxoeBRJyRcOoMd50198159;     gDVRMOcxoeBRJyRcOoMd50198159 = gDVRMOcxoeBRJyRcOoMd32605992;     gDVRMOcxoeBRJyRcOoMd32605992 = gDVRMOcxoeBRJyRcOoMd44451943;     gDVRMOcxoeBRJyRcOoMd44451943 = gDVRMOcxoeBRJyRcOoMd4754757;     gDVRMOcxoeBRJyRcOoMd4754757 = gDVRMOcxoeBRJyRcOoMd28211534;     gDVRMOcxoeBRJyRcOoMd28211534 = gDVRMOcxoeBRJyRcOoMd44428126;     gDVRMOcxoeBRJyRcOoMd44428126 = gDVRMOcxoeBRJyRcOoMd20418506;     gDVRMOcxoeBRJyRcOoMd20418506 = gDVRMOcxoeBRJyRcOoMd5338578;     gDVRMOcxoeBRJyRcOoMd5338578 = gDVRMOcxoeBRJyRcOoMd94776406;     gDVRMOcxoeBRJyRcOoMd94776406 = gDVRMOcxoeBRJyRcOoMd9961562;     gDVRMOcxoeBRJyRcOoMd9961562 = gDVRMOcxoeBRJyRcOoMd70913494;     gDVRMOcxoeBRJyRcOoMd70913494 = gDVRMOcxoeBRJyRcOoMd2760242;     gDVRMOcxoeBRJyRcOoMd2760242 = gDVRMOcxoeBRJyRcOoMd94089038;     gDVRMOcxoeBRJyRcOoMd94089038 = gDVRMOcxoeBRJyRcOoMd69313674;     gDVRMOcxoeBRJyRcOoMd69313674 = gDVRMOcxoeBRJyRcOoMd49488897;     gDVRMOcxoeBRJyRcOoMd49488897 = gDVRMOcxoeBRJyRcOoMd10470468;     gDVRMOcxoeBRJyRcOoMd10470468 = gDVRMOcxoeBRJyRcOoMd39600503;     gDVRMOcxoeBRJyRcOoMd39600503 = gDVRMOcxoeBRJyRcOoMd52361906;     gDVRMOcxoeBRJyRcOoMd52361906 = gDVRMOcxoeBRJyRcOoMd21988788;     gDVRMOcxoeBRJyRcOoMd21988788 = gDVRMOcxoeBRJyRcOoMd11839646;     gDVRMOcxoeBRJyRcOoMd11839646 = gDVRMOcxoeBRJyRcOoMd80963176;     gDVRMOcxoeBRJyRcOoMd80963176 = gDVRMOcxoeBRJyRcOoMd95799220;     gDVRMOcxoeBRJyRcOoMd95799220 = gDVRMOcxoeBRJyRcOoMd97269545;     gDVRMOcxoeBRJyRcOoMd97269545 = gDVRMOcxoeBRJyRcOoMd47599469;     gDVRMOcxoeBRJyRcOoMd47599469 = gDVRMOcxoeBRJyRcOoMd55592291;     gDVRMOcxoeBRJyRcOoMd55592291 = gDVRMOcxoeBRJyRcOoMd94906771;     gDVRMOcxoeBRJyRcOoMd94906771 = gDVRMOcxoeBRJyRcOoMd58834443;     gDVRMOcxoeBRJyRcOoMd58834443 = gDVRMOcxoeBRJyRcOoMd99982991;     gDVRMOcxoeBRJyRcOoMd99982991 = gDVRMOcxoeBRJyRcOoMd57855494;     gDVRMOcxoeBRJyRcOoMd57855494 = gDVRMOcxoeBRJyRcOoMd81014970;     gDVRMOcxoeBRJyRcOoMd81014970 = gDVRMOcxoeBRJyRcOoMd4428204;     gDVRMOcxoeBRJyRcOoMd4428204 = gDVRMOcxoeBRJyRcOoMd1778337;     gDVRMOcxoeBRJyRcOoMd1778337 = gDVRMOcxoeBRJyRcOoMd28893269;     gDVRMOcxoeBRJyRcOoMd28893269 = gDVRMOcxoeBRJyRcOoMd8733990;     gDVRMOcxoeBRJyRcOoMd8733990 = gDVRMOcxoeBRJyRcOoMd12135365;     gDVRMOcxoeBRJyRcOoMd12135365 = gDVRMOcxoeBRJyRcOoMd16517569;     gDVRMOcxoeBRJyRcOoMd16517569 = gDVRMOcxoeBRJyRcOoMd6713291;     gDVRMOcxoeBRJyRcOoMd6713291 = gDVRMOcxoeBRJyRcOoMd84924256;     gDVRMOcxoeBRJyRcOoMd84924256 = gDVRMOcxoeBRJyRcOoMd78365374;     gDVRMOcxoeBRJyRcOoMd78365374 = gDVRMOcxoeBRJyRcOoMd85474889;     gDVRMOcxoeBRJyRcOoMd85474889 = gDVRMOcxoeBRJyRcOoMd47909576;     gDVRMOcxoeBRJyRcOoMd47909576 = gDVRMOcxoeBRJyRcOoMd26017690;     gDVRMOcxoeBRJyRcOoMd26017690 = gDVRMOcxoeBRJyRcOoMd26385903;     gDVRMOcxoeBRJyRcOoMd26385903 = gDVRMOcxoeBRJyRcOoMd25209736;     gDVRMOcxoeBRJyRcOoMd25209736 = gDVRMOcxoeBRJyRcOoMd21966596;     gDVRMOcxoeBRJyRcOoMd21966596 = gDVRMOcxoeBRJyRcOoMd63243126;     gDVRMOcxoeBRJyRcOoMd63243126 = gDVRMOcxoeBRJyRcOoMd27374699;     gDVRMOcxoeBRJyRcOoMd27374699 = gDVRMOcxoeBRJyRcOoMd37957234;     gDVRMOcxoeBRJyRcOoMd37957234 = gDVRMOcxoeBRJyRcOoMd83997249;     gDVRMOcxoeBRJyRcOoMd83997249 = gDVRMOcxoeBRJyRcOoMd91096647;     gDVRMOcxoeBRJyRcOoMd91096647 = gDVRMOcxoeBRJyRcOoMd59960643;     gDVRMOcxoeBRJyRcOoMd59960643 = gDVRMOcxoeBRJyRcOoMd6830822;     gDVRMOcxoeBRJyRcOoMd6830822 = gDVRMOcxoeBRJyRcOoMd9860269;     gDVRMOcxoeBRJyRcOoMd9860269 = gDVRMOcxoeBRJyRcOoMd63438409;     gDVRMOcxoeBRJyRcOoMd63438409 = gDVRMOcxoeBRJyRcOoMd38856362;     gDVRMOcxoeBRJyRcOoMd38856362 = gDVRMOcxoeBRJyRcOoMd54398938;     gDVRMOcxoeBRJyRcOoMd54398938 = gDVRMOcxoeBRJyRcOoMd35336446;     gDVRMOcxoeBRJyRcOoMd35336446 = gDVRMOcxoeBRJyRcOoMd96852473;     gDVRMOcxoeBRJyRcOoMd96852473 = gDVRMOcxoeBRJyRcOoMd49162465;     gDVRMOcxoeBRJyRcOoMd49162465 = gDVRMOcxoeBRJyRcOoMd33304763;     gDVRMOcxoeBRJyRcOoMd33304763 = gDVRMOcxoeBRJyRcOoMd85593683;     gDVRMOcxoeBRJyRcOoMd85593683 = gDVRMOcxoeBRJyRcOoMd20435514;     gDVRMOcxoeBRJyRcOoMd20435514 = gDVRMOcxoeBRJyRcOoMd47483084;     gDVRMOcxoeBRJyRcOoMd47483084 = gDVRMOcxoeBRJyRcOoMd13761436;     gDVRMOcxoeBRJyRcOoMd13761436 = gDVRMOcxoeBRJyRcOoMd5533358;     gDVRMOcxoeBRJyRcOoMd5533358 = gDVRMOcxoeBRJyRcOoMd69135158;     gDVRMOcxoeBRJyRcOoMd69135158 = gDVRMOcxoeBRJyRcOoMd73866972;     gDVRMOcxoeBRJyRcOoMd73866972 = gDVRMOcxoeBRJyRcOoMd85355049;     gDVRMOcxoeBRJyRcOoMd85355049 = gDVRMOcxoeBRJyRcOoMd57178309;     gDVRMOcxoeBRJyRcOoMd57178309 = gDVRMOcxoeBRJyRcOoMd32971328;     gDVRMOcxoeBRJyRcOoMd32971328 = gDVRMOcxoeBRJyRcOoMd3757177;     gDVRMOcxoeBRJyRcOoMd3757177 = gDVRMOcxoeBRJyRcOoMd54676247;     gDVRMOcxoeBRJyRcOoMd54676247 = gDVRMOcxoeBRJyRcOoMd73996532;     gDVRMOcxoeBRJyRcOoMd73996532 = gDVRMOcxoeBRJyRcOoMd36513898;     gDVRMOcxoeBRJyRcOoMd36513898 = gDVRMOcxoeBRJyRcOoMd63930070;     gDVRMOcxoeBRJyRcOoMd63930070 = gDVRMOcxoeBRJyRcOoMd54945486;     gDVRMOcxoeBRJyRcOoMd54945486 = gDVRMOcxoeBRJyRcOoMd69413318;     gDVRMOcxoeBRJyRcOoMd69413318 = gDVRMOcxoeBRJyRcOoMd72059810;     gDVRMOcxoeBRJyRcOoMd72059810 = gDVRMOcxoeBRJyRcOoMd25632874;     gDVRMOcxoeBRJyRcOoMd25632874 = gDVRMOcxoeBRJyRcOoMd92349165;     gDVRMOcxoeBRJyRcOoMd92349165 = gDVRMOcxoeBRJyRcOoMd67532072;     gDVRMOcxoeBRJyRcOoMd67532072 = gDVRMOcxoeBRJyRcOoMd20877209;     gDVRMOcxoeBRJyRcOoMd20877209 = gDVRMOcxoeBRJyRcOoMd15985742;     gDVRMOcxoeBRJyRcOoMd15985742 = gDVRMOcxoeBRJyRcOoMd66758846;     gDVRMOcxoeBRJyRcOoMd66758846 = gDVRMOcxoeBRJyRcOoMd21054328;     gDVRMOcxoeBRJyRcOoMd21054328 = gDVRMOcxoeBRJyRcOoMd97597381;     gDVRMOcxoeBRJyRcOoMd97597381 = gDVRMOcxoeBRJyRcOoMd91918067;     gDVRMOcxoeBRJyRcOoMd91918067 = gDVRMOcxoeBRJyRcOoMd65454859;     gDVRMOcxoeBRJyRcOoMd65454859 = gDVRMOcxoeBRJyRcOoMd69877627;     gDVRMOcxoeBRJyRcOoMd69877627 = gDVRMOcxoeBRJyRcOoMd57736427;     gDVRMOcxoeBRJyRcOoMd57736427 = gDVRMOcxoeBRJyRcOoMd81181122;     gDVRMOcxoeBRJyRcOoMd81181122 = gDVRMOcxoeBRJyRcOoMd9860817;     gDVRMOcxoeBRJyRcOoMd9860817 = gDVRMOcxoeBRJyRcOoMd35761791;     gDVRMOcxoeBRJyRcOoMd35761791 = gDVRMOcxoeBRJyRcOoMd45060611;     gDVRMOcxoeBRJyRcOoMd45060611 = gDVRMOcxoeBRJyRcOoMd1567115;}
// Junk Finished

// Junk Code By Peatreat & Thaisen's Gen
void drllrKxLCkauIugNnZMo9828463() {     int XKPfoRlUoEZHCDVMScJc33827924 = -576954497;    int XKPfoRlUoEZHCDVMScJc783582 = -271402207;    int XKPfoRlUoEZHCDVMScJc92469307 = -238978723;    int XKPfoRlUoEZHCDVMScJc73357170 = -681366574;    int XKPfoRlUoEZHCDVMScJc60395574 = -333293403;    int XKPfoRlUoEZHCDVMScJc53916250 = -386829460;    int XKPfoRlUoEZHCDVMScJc47690459 = -201304148;    int XKPfoRlUoEZHCDVMScJc55361287 = -271932759;    int XKPfoRlUoEZHCDVMScJc17803387 = 12264769;    int XKPfoRlUoEZHCDVMScJc92262671 = -611581677;    int XKPfoRlUoEZHCDVMScJc89601574 = -627470068;    int XKPfoRlUoEZHCDVMScJc65581568 = -613028802;    int XKPfoRlUoEZHCDVMScJc49080554 = -945304277;    int XKPfoRlUoEZHCDVMScJc9534477 = -722737796;    int XKPfoRlUoEZHCDVMScJc33437312 = -147765038;    int XKPfoRlUoEZHCDVMScJc30037732 = -758268044;    int XKPfoRlUoEZHCDVMScJc76774666 = -81092763;    int XKPfoRlUoEZHCDVMScJc87380945 = -312850404;    int XKPfoRlUoEZHCDVMScJc74456278 = -985276088;    int XKPfoRlUoEZHCDVMScJc80505709 = 20190154;    int XKPfoRlUoEZHCDVMScJc62449601 = -563562626;    int XKPfoRlUoEZHCDVMScJc76204103 = -459788459;    int XKPfoRlUoEZHCDVMScJc9655408 = -720673178;    int XKPfoRlUoEZHCDVMScJc70940596 = -228001865;    int XKPfoRlUoEZHCDVMScJc10643847 = -58269985;    int XKPfoRlUoEZHCDVMScJc81351675 = -380724756;    int XKPfoRlUoEZHCDVMScJc6085262 = -148669501;    int XKPfoRlUoEZHCDVMScJc78540296 = -636031147;    int XKPfoRlUoEZHCDVMScJc42953109 = -411806435;    int XKPfoRlUoEZHCDVMScJc8967569 = 47967224;    int XKPfoRlUoEZHCDVMScJc96419693 = -184957342;    int XKPfoRlUoEZHCDVMScJc53081048 = -118386969;    int XKPfoRlUoEZHCDVMScJc30904072 = 78940677;    int XKPfoRlUoEZHCDVMScJc35533034 = 85619440;    int XKPfoRlUoEZHCDVMScJc98488323 = 89538851;    int XKPfoRlUoEZHCDVMScJc69476832 = -611349873;    int XKPfoRlUoEZHCDVMScJc97634346 = -376612361;    int XKPfoRlUoEZHCDVMScJc31047154 = -376888866;    int XKPfoRlUoEZHCDVMScJc66668383 = -17366232;    int XKPfoRlUoEZHCDVMScJc84407562 = -430700456;    int XKPfoRlUoEZHCDVMScJc27193063 = -664613257;    int XKPfoRlUoEZHCDVMScJc50381496 = -662778827;    int XKPfoRlUoEZHCDVMScJc52446641 = 28976141;    int XKPfoRlUoEZHCDVMScJc53706435 = -418722430;    int XKPfoRlUoEZHCDVMScJc54384223 = -923020390;    int XKPfoRlUoEZHCDVMScJc73136178 = -525135947;    int XKPfoRlUoEZHCDVMScJc79783682 = -217533735;    int XKPfoRlUoEZHCDVMScJc80229905 = -525590072;    int XKPfoRlUoEZHCDVMScJc92927154 = -888365634;    int XKPfoRlUoEZHCDVMScJc93013075 = -839389647;    int XKPfoRlUoEZHCDVMScJc1470289 = -180435350;    int XKPfoRlUoEZHCDVMScJc93321275 = -169443100;    int XKPfoRlUoEZHCDVMScJc56236647 = -535004857;    int XKPfoRlUoEZHCDVMScJc45336717 = -283692125;    int XKPfoRlUoEZHCDVMScJc47378862 = -139936712;    int XKPfoRlUoEZHCDVMScJc57623820 = -17166039;    int XKPfoRlUoEZHCDVMScJc91128173 = -550729029;    int XKPfoRlUoEZHCDVMScJc21528712 = 89023142;    int XKPfoRlUoEZHCDVMScJc62713323 = -523096590;    int XKPfoRlUoEZHCDVMScJc79043898 = -952568648;    int XKPfoRlUoEZHCDVMScJc47830989 = -138159960;    int XKPfoRlUoEZHCDVMScJc69150163 = -565273001;    int XKPfoRlUoEZHCDVMScJc12408179 = -860126325;    int XKPfoRlUoEZHCDVMScJc8835818 = 64297545;    int XKPfoRlUoEZHCDVMScJc95842977 = -326624336;    int XKPfoRlUoEZHCDVMScJc36520526 = -409083100;    int XKPfoRlUoEZHCDVMScJc34677496 = -591969479;    int XKPfoRlUoEZHCDVMScJc13547520 = -930923717;    int XKPfoRlUoEZHCDVMScJc11046153 = -712276647;    int XKPfoRlUoEZHCDVMScJc63960479 = -536415165;    int XKPfoRlUoEZHCDVMScJc32403385 = -281655683;    int XKPfoRlUoEZHCDVMScJc45727512 = -704203898;    int XKPfoRlUoEZHCDVMScJc20712562 = -195484172;    int XKPfoRlUoEZHCDVMScJc90048716 = -454575632;    int XKPfoRlUoEZHCDVMScJc53312647 = -315196590;    int XKPfoRlUoEZHCDVMScJc12068105 = -900783799;    int XKPfoRlUoEZHCDVMScJc23757463 = -388764600;    int XKPfoRlUoEZHCDVMScJc55948973 = -201950748;    int XKPfoRlUoEZHCDVMScJc16556373 = -304981475;    int XKPfoRlUoEZHCDVMScJc37507669 = -533134039;    int XKPfoRlUoEZHCDVMScJc1567993 = -63191021;    int XKPfoRlUoEZHCDVMScJc25855356 = -623079430;    int XKPfoRlUoEZHCDVMScJc85613142 = -747665513;    int XKPfoRlUoEZHCDVMScJc49940033 = -572416788;    int XKPfoRlUoEZHCDVMScJc7497281 = -771597426;    int XKPfoRlUoEZHCDVMScJc3098418 = 84485758;    int XKPfoRlUoEZHCDVMScJc96844401 = -583382112;    int XKPfoRlUoEZHCDVMScJc85567355 = -637367198;    int XKPfoRlUoEZHCDVMScJc88154172 = -774443848;    int XKPfoRlUoEZHCDVMScJc40864504 = -893295111;    int XKPfoRlUoEZHCDVMScJc78348659 = 39379156;    int XKPfoRlUoEZHCDVMScJc76105635 = -365635503;    int XKPfoRlUoEZHCDVMScJc68333830 = -853792277;    int XKPfoRlUoEZHCDVMScJc87624484 = -64797585;    int XKPfoRlUoEZHCDVMScJc36576574 = -192540497;    int XKPfoRlUoEZHCDVMScJc58042899 = 659744;    int XKPfoRlUoEZHCDVMScJc37973318 = -802652503;    int XKPfoRlUoEZHCDVMScJc43610823 = 64678596;    int XKPfoRlUoEZHCDVMScJc57863457 = 7901905;    int XKPfoRlUoEZHCDVMScJc17863697 = -576954497;     XKPfoRlUoEZHCDVMScJc33827924 = XKPfoRlUoEZHCDVMScJc783582;     XKPfoRlUoEZHCDVMScJc783582 = XKPfoRlUoEZHCDVMScJc92469307;     XKPfoRlUoEZHCDVMScJc92469307 = XKPfoRlUoEZHCDVMScJc73357170;     XKPfoRlUoEZHCDVMScJc73357170 = XKPfoRlUoEZHCDVMScJc60395574;     XKPfoRlUoEZHCDVMScJc60395574 = XKPfoRlUoEZHCDVMScJc53916250;     XKPfoRlUoEZHCDVMScJc53916250 = XKPfoRlUoEZHCDVMScJc47690459;     XKPfoRlUoEZHCDVMScJc47690459 = XKPfoRlUoEZHCDVMScJc55361287;     XKPfoRlUoEZHCDVMScJc55361287 = XKPfoRlUoEZHCDVMScJc17803387;     XKPfoRlUoEZHCDVMScJc17803387 = XKPfoRlUoEZHCDVMScJc92262671;     XKPfoRlUoEZHCDVMScJc92262671 = XKPfoRlUoEZHCDVMScJc89601574;     XKPfoRlUoEZHCDVMScJc89601574 = XKPfoRlUoEZHCDVMScJc65581568;     XKPfoRlUoEZHCDVMScJc65581568 = XKPfoRlUoEZHCDVMScJc49080554;     XKPfoRlUoEZHCDVMScJc49080554 = XKPfoRlUoEZHCDVMScJc9534477;     XKPfoRlUoEZHCDVMScJc9534477 = XKPfoRlUoEZHCDVMScJc33437312;     XKPfoRlUoEZHCDVMScJc33437312 = XKPfoRlUoEZHCDVMScJc30037732;     XKPfoRlUoEZHCDVMScJc30037732 = XKPfoRlUoEZHCDVMScJc76774666;     XKPfoRlUoEZHCDVMScJc76774666 = XKPfoRlUoEZHCDVMScJc87380945;     XKPfoRlUoEZHCDVMScJc87380945 = XKPfoRlUoEZHCDVMScJc74456278;     XKPfoRlUoEZHCDVMScJc74456278 = XKPfoRlUoEZHCDVMScJc80505709;     XKPfoRlUoEZHCDVMScJc80505709 = XKPfoRlUoEZHCDVMScJc62449601;     XKPfoRlUoEZHCDVMScJc62449601 = XKPfoRlUoEZHCDVMScJc76204103;     XKPfoRlUoEZHCDVMScJc76204103 = XKPfoRlUoEZHCDVMScJc9655408;     XKPfoRlUoEZHCDVMScJc9655408 = XKPfoRlUoEZHCDVMScJc70940596;     XKPfoRlUoEZHCDVMScJc70940596 = XKPfoRlUoEZHCDVMScJc10643847;     XKPfoRlUoEZHCDVMScJc10643847 = XKPfoRlUoEZHCDVMScJc81351675;     XKPfoRlUoEZHCDVMScJc81351675 = XKPfoRlUoEZHCDVMScJc6085262;     XKPfoRlUoEZHCDVMScJc6085262 = XKPfoRlUoEZHCDVMScJc78540296;     XKPfoRlUoEZHCDVMScJc78540296 = XKPfoRlUoEZHCDVMScJc42953109;     XKPfoRlUoEZHCDVMScJc42953109 = XKPfoRlUoEZHCDVMScJc8967569;     XKPfoRlUoEZHCDVMScJc8967569 = XKPfoRlUoEZHCDVMScJc96419693;     XKPfoRlUoEZHCDVMScJc96419693 = XKPfoRlUoEZHCDVMScJc53081048;     XKPfoRlUoEZHCDVMScJc53081048 = XKPfoRlUoEZHCDVMScJc30904072;     XKPfoRlUoEZHCDVMScJc30904072 = XKPfoRlUoEZHCDVMScJc35533034;     XKPfoRlUoEZHCDVMScJc35533034 = XKPfoRlUoEZHCDVMScJc98488323;     XKPfoRlUoEZHCDVMScJc98488323 = XKPfoRlUoEZHCDVMScJc69476832;     XKPfoRlUoEZHCDVMScJc69476832 = XKPfoRlUoEZHCDVMScJc97634346;     XKPfoRlUoEZHCDVMScJc97634346 = XKPfoRlUoEZHCDVMScJc31047154;     XKPfoRlUoEZHCDVMScJc31047154 = XKPfoRlUoEZHCDVMScJc66668383;     XKPfoRlUoEZHCDVMScJc66668383 = XKPfoRlUoEZHCDVMScJc84407562;     XKPfoRlUoEZHCDVMScJc84407562 = XKPfoRlUoEZHCDVMScJc27193063;     XKPfoRlUoEZHCDVMScJc27193063 = XKPfoRlUoEZHCDVMScJc50381496;     XKPfoRlUoEZHCDVMScJc50381496 = XKPfoRlUoEZHCDVMScJc52446641;     XKPfoRlUoEZHCDVMScJc52446641 = XKPfoRlUoEZHCDVMScJc53706435;     XKPfoRlUoEZHCDVMScJc53706435 = XKPfoRlUoEZHCDVMScJc54384223;     XKPfoRlUoEZHCDVMScJc54384223 = XKPfoRlUoEZHCDVMScJc73136178;     XKPfoRlUoEZHCDVMScJc73136178 = XKPfoRlUoEZHCDVMScJc79783682;     XKPfoRlUoEZHCDVMScJc79783682 = XKPfoRlUoEZHCDVMScJc80229905;     XKPfoRlUoEZHCDVMScJc80229905 = XKPfoRlUoEZHCDVMScJc92927154;     XKPfoRlUoEZHCDVMScJc92927154 = XKPfoRlUoEZHCDVMScJc93013075;     XKPfoRlUoEZHCDVMScJc93013075 = XKPfoRlUoEZHCDVMScJc1470289;     XKPfoRlUoEZHCDVMScJc1470289 = XKPfoRlUoEZHCDVMScJc93321275;     XKPfoRlUoEZHCDVMScJc93321275 = XKPfoRlUoEZHCDVMScJc56236647;     XKPfoRlUoEZHCDVMScJc56236647 = XKPfoRlUoEZHCDVMScJc45336717;     XKPfoRlUoEZHCDVMScJc45336717 = XKPfoRlUoEZHCDVMScJc47378862;     XKPfoRlUoEZHCDVMScJc47378862 = XKPfoRlUoEZHCDVMScJc57623820;     XKPfoRlUoEZHCDVMScJc57623820 = XKPfoRlUoEZHCDVMScJc91128173;     XKPfoRlUoEZHCDVMScJc91128173 = XKPfoRlUoEZHCDVMScJc21528712;     XKPfoRlUoEZHCDVMScJc21528712 = XKPfoRlUoEZHCDVMScJc62713323;     XKPfoRlUoEZHCDVMScJc62713323 = XKPfoRlUoEZHCDVMScJc79043898;     XKPfoRlUoEZHCDVMScJc79043898 = XKPfoRlUoEZHCDVMScJc47830989;     XKPfoRlUoEZHCDVMScJc47830989 = XKPfoRlUoEZHCDVMScJc69150163;     XKPfoRlUoEZHCDVMScJc69150163 = XKPfoRlUoEZHCDVMScJc12408179;     XKPfoRlUoEZHCDVMScJc12408179 = XKPfoRlUoEZHCDVMScJc8835818;     XKPfoRlUoEZHCDVMScJc8835818 = XKPfoRlUoEZHCDVMScJc95842977;     XKPfoRlUoEZHCDVMScJc95842977 = XKPfoRlUoEZHCDVMScJc36520526;     XKPfoRlUoEZHCDVMScJc36520526 = XKPfoRlUoEZHCDVMScJc34677496;     XKPfoRlUoEZHCDVMScJc34677496 = XKPfoRlUoEZHCDVMScJc13547520;     XKPfoRlUoEZHCDVMScJc13547520 = XKPfoRlUoEZHCDVMScJc11046153;     XKPfoRlUoEZHCDVMScJc11046153 = XKPfoRlUoEZHCDVMScJc63960479;     XKPfoRlUoEZHCDVMScJc63960479 = XKPfoRlUoEZHCDVMScJc32403385;     XKPfoRlUoEZHCDVMScJc32403385 = XKPfoRlUoEZHCDVMScJc45727512;     XKPfoRlUoEZHCDVMScJc45727512 = XKPfoRlUoEZHCDVMScJc20712562;     XKPfoRlUoEZHCDVMScJc20712562 = XKPfoRlUoEZHCDVMScJc90048716;     XKPfoRlUoEZHCDVMScJc90048716 = XKPfoRlUoEZHCDVMScJc53312647;     XKPfoRlUoEZHCDVMScJc53312647 = XKPfoRlUoEZHCDVMScJc12068105;     XKPfoRlUoEZHCDVMScJc12068105 = XKPfoRlUoEZHCDVMScJc23757463;     XKPfoRlUoEZHCDVMScJc23757463 = XKPfoRlUoEZHCDVMScJc55948973;     XKPfoRlUoEZHCDVMScJc55948973 = XKPfoRlUoEZHCDVMScJc16556373;     XKPfoRlUoEZHCDVMScJc16556373 = XKPfoRlUoEZHCDVMScJc37507669;     XKPfoRlUoEZHCDVMScJc37507669 = XKPfoRlUoEZHCDVMScJc1567993;     XKPfoRlUoEZHCDVMScJc1567993 = XKPfoRlUoEZHCDVMScJc25855356;     XKPfoRlUoEZHCDVMScJc25855356 = XKPfoRlUoEZHCDVMScJc85613142;     XKPfoRlUoEZHCDVMScJc85613142 = XKPfoRlUoEZHCDVMScJc49940033;     XKPfoRlUoEZHCDVMScJc49940033 = XKPfoRlUoEZHCDVMScJc7497281;     XKPfoRlUoEZHCDVMScJc7497281 = XKPfoRlUoEZHCDVMScJc3098418;     XKPfoRlUoEZHCDVMScJc3098418 = XKPfoRlUoEZHCDVMScJc96844401;     XKPfoRlUoEZHCDVMScJc96844401 = XKPfoRlUoEZHCDVMScJc85567355;     XKPfoRlUoEZHCDVMScJc85567355 = XKPfoRlUoEZHCDVMScJc88154172;     XKPfoRlUoEZHCDVMScJc88154172 = XKPfoRlUoEZHCDVMScJc40864504;     XKPfoRlUoEZHCDVMScJc40864504 = XKPfoRlUoEZHCDVMScJc78348659;     XKPfoRlUoEZHCDVMScJc78348659 = XKPfoRlUoEZHCDVMScJc76105635;     XKPfoRlUoEZHCDVMScJc76105635 = XKPfoRlUoEZHCDVMScJc68333830;     XKPfoRlUoEZHCDVMScJc68333830 = XKPfoRlUoEZHCDVMScJc87624484;     XKPfoRlUoEZHCDVMScJc87624484 = XKPfoRlUoEZHCDVMScJc36576574;     XKPfoRlUoEZHCDVMScJc36576574 = XKPfoRlUoEZHCDVMScJc58042899;     XKPfoRlUoEZHCDVMScJc58042899 = XKPfoRlUoEZHCDVMScJc37973318;     XKPfoRlUoEZHCDVMScJc37973318 = XKPfoRlUoEZHCDVMScJc43610823;     XKPfoRlUoEZHCDVMScJc43610823 = XKPfoRlUoEZHCDVMScJc57863457;     XKPfoRlUoEZHCDVMScJc57863457 = XKPfoRlUoEZHCDVMScJc17863697;     XKPfoRlUoEZHCDVMScJc17863697 = XKPfoRlUoEZHCDVMScJc33827924;}
// Junk Finished
