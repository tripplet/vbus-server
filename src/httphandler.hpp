#pragma once

#include <cstring>
#include <string>
#include <iostream>
#include <sstream>
#include <unordered_map>

#include <brotli/encode.h>

#define COMPRESSION_BUFFER_SIZE 65536

class HttpHandler
{
    public:
        typedef enum {
            Normal,
            Gzip,
            Brotli
        } Encoding;

    private:
        Encoding encoding = Encoding::Normal;
        std::unordered_map<std::string, std::string>* parameter = nullptr;
        std::string* content_type = nullptr;
        std::ostream& response;
        std::ostringstream* buffer_stream = nullptr;

        uint8_t* buffer = nullptr;

        // brotli
        BrotliEncoderState* brotli;
        uint8_t* brotli_in;
        uint8_t* brotli_out;
        size_t brotli_available_in;
        const uint8_t* brotli_next_in;
        size_t brotli_available_out;
        uint8_t* brotli_next_out;

        // gzip/deflate


    public:
        HttpHandler(const char* accept_encoding, const char* request_uri, std::ostream& resp);
        ~HttpHandler();

        void setContentType(const char *type) { this->content_type = new std::string(type); }
        void sendHeader();
        bool isCompressionActive() { return this->encoding == Encoding::Gzip || this->encoding == Encoding::Brotli; }
        std::ostream* getStream();
        void process();
        void flush();

        std::string param(const std::string key);
        void setDefault(const std::string key, const std::string value);

    private:
        std::string trimAndLower(const char* input);
        void parseURL(const char* request_uri);
        void BrotliProcess();
        void BrotliFlush();
        void prepareEncoding(const char* accept_encoding);
};