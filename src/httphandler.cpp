#include <cctype>
#include <algorithm>
#include <locale>

#include <uriparser/Uri.h>
//#include <miniz.h>

#include "httphandler.hpp"

HttpHandler::HttpHandler(const char* accept_encoding, const char* request_uri, std::ostream& resp) : response(resp)
{
    this->prepareEncoding(accept_encoding);
    this->parseURL(request_uri);
}

HttpHandler::~HttpHandler()
{
    delete this->parameter;
    delete this->content_type;
    delete this->buffer_stream;
    delete[] this->buffer;
}

void HttpHandler::sendHeader()
{
    this->response << "Content-Type: " << *this->content_type << "\r\n"
                   << "Cache-Control: no-cache, no-store, must-revalidate\r\n"
                   "Pragma: no-cache\r\n"
                   "Expires: 0\r\n"
                   "Access-Control-Allow-Origin: *\r\n";

    if (this->encoding == Encoding::Brotli)
    {
        this->response << "Content-Encoding: br\r\n";
    }
    else if (this->encoding == Encoding::Gzip)
    {
        this->response << "Content-Encoding: gzip\r\n";
    }

    this->response << "\r\n";
}

inline std::string HttpHandler::trimAndLower(const char* input)
{
    if (input == nullptr)
    {
        return std::string();
    }

    std::string input_str(input);

    auto wsfront = std::find_if_not(input_str.begin(), input_str.end(), [](int c) {
        return std::isspace(c);
    });
    auto wsback = std::find_if_not(input_str.rbegin(), input_str.rend(), [](int c) {
        return std::isspace(c);
    }).base();

    return (wsback <= wsfront ? std::string() : std::string(wsfront, wsback));
}

void HttpHandler::process()
{
    if (!isCompressionActive())
    {
        return;
    }

    if (this->encoding == Encoding::Brotli)
    {
        this->BrotliProcess();
    }
}


void HttpHandler::BrotliProcess()
{
    this->brotli_available_in = this->buffer_stream->str().length();
    this->buffer_stream->str().copy(reinterpret_cast<char*>(this->brotli_in), this->brotli_available_in, 0);
    this->brotli_next_in = this->brotli_in;

    if (!BrotliEncoderCompressStream(this->brotli, BROTLI_OPERATION_PROCESS, &this->brotli_available_in, &this->brotli_next_in, &this->brotli_available_out, &this->brotli_next_out, NULL)) {
        std::cerr << "failed to compress data" << std::endl;
        return;
    }

    if (this->brotli_available_out != COMPRESSION_BUFFER_SIZE) {
        this->response.write(reinterpret_cast<const char*>(this->brotli_out), COMPRESSION_BUFFER_SIZE - this->brotli_available_out);
        this->brotli_available_out = COMPRESSION_BUFFER_SIZE;
        this->brotli_next_out = this->brotli_out;
    }

    this->buffer_stream->str("");
}

void HttpHandler::flush()
{
    if (!isCompressionActive())
    {
        return;
    }

    if (this->encoding == Encoding::Brotli)
    {
        BrotliFlush();
    }
}

void HttpHandler::BrotliFlush()
{
    do
    {
        this->brotli_available_in = 0;
        if (!BrotliEncoderCompressStream(this->brotli, BROTLI_OPERATION_FINISH, &this->brotli_available_in, &this->brotli_next_in, &this->brotli_available_out, &this->brotli_next_out, NULL)) {
            std::cerr << "failed to compress data" << std::endl;
            return;
        }

        this->response.write(reinterpret_cast<const char*>(this->brotli_out), COMPRESSION_BUFFER_SIZE - this->brotli_available_out);
        this->brotli_available_out = COMPRESSION_BUFFER_SIZE;
        this->brotli_next_out = this->brotli_out;
    } while(BrotliEncoderHasMoreOutput(this->brotli));
}

std::ostream* HttpHandler::getStream()
{
    if (isCompressionActive())
    {
        return reinterpret_cast<std::ostream*>(this->buffer_stream);
    }
    else
    {
        return &this->response;
    }
}

void HttpHandler::prepareEncoding(const char* accept_encoding)
{
    if (accept_encoding == nullptr) {
        this->encoding = Encoding::Normal;
        return;
    }

    auto enc = trimAndLower(std::strtok(const_cast<char*>(accept_encoding), ","));
    while (!enc.empty())
    {
        if (enc == "gzip" && this->encoding != Encoding::Brotli) {
            //this->encoding = Encoding::Gzip;
        }
        else if (enc == "br") {
            // Prefer brotli over gzip
            this->encoding = Encoding::Brotli;
            break;
        }

        enc = trimAndLower(std::strtok(NULL, " "));
    }

    if (isCompressionActive())
    {
        this->buffer_stream = new std::ostringstream();
    }

    if (this->encoding == Encoding::Brotli)
    {
        // Init brotli
        this->buffer = new uint8_t[2 * COMPRESSION_BUFFER_SIZE];
        this->brotli_in = buffer;
        this->brotli_out = buffer + COMPRESSION_BUFFER_SIZE;
        this->brotli_available_in = 0;
        this->brotli_next_in = NULL;
        this->brotli_available_out = COMPRESSION_BUFFER_SIZE;
        this->brotli_next_out = this->brotli_out;

        this->brotli = BrotliEncoderCreateInstance(nullptr, nullptr, 0);
        BrotliEncoderSetParameter(this->brotli, BROTLI_PARAM_MODE, BROTLI_MODE_TEXT);
        BrotliEncoderSetParameter(this->brotli, BROTLI_PARAM_QUALITY, 2);
    }
}

void HttpHandler::setDefault(const std::string key, const std::string value)
{
    if (this->parameter->count(key) == 0)
    {
        (*this->parameter)[key] = value;
    }
}

std::string HttpHandler::param(const std::string key)
{
    return (*this->parameter)[key];
}

void HttpHandler::parseURL(const char* request_uri)
{
    this->parameter = new std::unordered_map<std::string, std::string>();
    UriParserStateA state;
    UriUriA uri;
    state.uri = &uri;

    if (request_uri == nullptr)
    {
        return;
    }

    if (uriParseUriA(&state, request_uri) != URI_SUCCESS)
    {
        uriFreeUriMembersA(&uri);
        return;
    }

    UriQueryListA* queryList;
    int itemCount;

    if (uriDissectQueryMallocA(&queryList, &itemCount, uri.query.first, uri.query.afterLast) == URI_SUCCESS)
    {
        auto& current = queryList;

        do
        {
            if (queryList->value != nullptr) {
                auto buffer = new char[std::strlen(queryList->value) + 1];
                std::strcpy(buffer, (char*) queryList->value);

                uriUnescapeInPlaceExA(buffer, false, URI_BR_DONT_TOUCH);
                (*this->parameter)[queryList->key] = buffer;
                delete[] buffer;
            }
            else {
                (*this->parameter)[queryList->key] = std::string("");
            }

            current = queryList->next;
        } while (current != nullptr);

        uriFreeQueryListA(queryList);
        uriFreeUriMembersA(&uri);
        return;
    }
}
