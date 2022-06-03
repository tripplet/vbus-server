#pragma once

#include <cstring>
#include <string>
#include <iostream>
#include <sstream>
#include <unordered_map>

#include <zlib.h>
#include <brotli/encode.h>

#define COMPRESSION_BUFFER_SIZE (128 * 1024)

class HttpHandler
{
    public:
        typedef enum {
            None,
            Gzip,
            
            #if BROTLI_SUPPORT == 1
            Brotli
            #endif
        } Encoding;

    private:
        Encoding encoding = Encoding::None;
        std::unordered_map<std::string, std::string>* parameter = nullptr;
        std::string* content_type = nullptr;
        std::ostream& response;
        std::ostringstream* buffer_stream = nullptr;

        uint8_t* buffer = nullptr;

        #if BROTLI_SUPPORT == 1
        // brotli
        BrotliEncoderState* brotli;
        uint8_t* brotli_in;
        uint8_t* brotli_out;
        size_t brotli_available_in;
        const uint8_t* brotli_next_in;
        size_t brotli_available_out;
        uint8_t* brotli_next_out;
        #endif

        // gzip/deflate
        z_stream zlib;

    public:
        HttpHandler(const char* accept_encoding, const char* request_uri, std::ostream& resp);
        ~HttpHandler();

        void setContentType(const char *type) { this->content_type = new std::string(type); }
        void sendHeader();
        bool isCompressionActive() { 
                return 
                #if BROTLI_SUPPORT == 1
                this->encoding == Encoding::Brotli || 
                #endif
                this->encoding == Encoding::Gzip; }
        std::ostream* getStream();
        void process();
        void flush();

        std::string param(const std::string key);
        void setDefault(const std::string key, const std::string value);

    private:
        std::string trimAndLower(const char* input);
        void parseURL(const char* request_uri);
        void prepareEncoding(const char* accept_encoding);

        #if BROTLI_SUPPORT == 1
        void BrotliProcess();
        void BrotliFlush();
        #endif

        void GzipProcess();
        void GzipFlush();
};
