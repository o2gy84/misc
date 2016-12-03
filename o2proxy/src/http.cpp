#include <iostream>
#include <sstream>
#include <vector>

#include <unistd.h>

#include "utils.hpp"
#include "http.hpp"
#include "logger.hpp"


std::string HttpHeaders::toString() const
{
    std::stringstream ss;
    ss << _method << " " << _resource << " " << _version;

    if (_headers.size())
    {
        ss << "\r\n";
    }

    size_t counter = 0;
    for (auto h = _headers.begin(); h != _headers.end(); ++h)
    {
        ss << h->first << ": " << h->second;
        ++counter;
        if (counter < _headers.size())
        {
            ss << "\r\n";
        }
    }

    return ss.str();
}

void HttpHeaders::parseFirstRequestLine(const std::string &str)
{
    std::vector<std::string> parts = utils::split(str, " ");
    if (parts.size() < 3)
    {
        loge("broken header: ", str);
        return;
    }

    _method = parts[0];
    _version = parts[parts.size() - 1];

    for (size_t i = 1; i < parts.size() - 1; ++i)
    {
        if (i > 1)
            _resource += " ";
        _resource += parts[i];
    }
}


void HttpHeaders::fromString(const std::string &str)
{
    std::vector<std::string> hdrs = utils::split(str, "\r\n");

    parseFirstRequestLine(hdrs[0]);

    for (size_t i = 1; i < hdrs.size(); i++)
    {
        if (hdrs[i].size() == 0)
            continue;

        if (::isspace(hdrs[i][0]))
            continue;

        std::vector<std::string> key_value = utils::split(hdrs[i], ":", 2);
        if (key_value.size() < 2)
            continue;

        std::string key = utils::trimmed(key_value[0]);
        //key = utils::lowercased(key);

        _headers[key] = utils::trimmed(key_value[1]);
    }

    _content_len = 0;

    std::string content_len = header("Content-Length");
    if (content_len.empty())
    {
        return;
    }
    
    size_t c_len = std::stoi(content_len);
    if (c_len == 0)
    {
        return;
    }

    _content_len = c_len;
}

void HttpHeaders::clear()
{
    std::map<std::string, std::string>().swap(_headers);
    _method = "";
    _resource = "";
    _version = "";
    _content_len = 0;
}

HttpRequest::HttpRequest()
{
    _request_valid = false;
    _headers_ready = false;
    _chunked = false;
    _chunk_size = 0;
}


std::string HttpRequest::toString() const
{
    std::stringstream ss;
    ss << _headers.toString() << "\r\n\r\n" << _body;
    return ss.str();
}


std::string HttpRequest::dump() const
{
    std::stringbuf stringbuf;
    std::ostream os(&stringbuf);

    os << "<<<DUMP START>>>\n";
    os << _headers._method << " " << _headers._resource << " " << _headers._version;

    if (_headers._headers.size())
    {
        os << "\r\n";
    }

    size_t counter = 0;
    for (auto h = _headers._headers.begin(); h != _headers._headers.end(); ++h)
    {
        os << h->first << ": " << h->second;
        ++counter;
        if (counter < _headers._headers.size())
        {
            os << "\r\n";
        }
    }
    os << "\r\n";

    std::string hdr = _headers.toString();
    os  << "headers: " << hdr.size() << " bytes"
        << ", body: " << _body.size() << " bytes"
        << ", total: " << hdr.size() + _body.size() << " bytes (without \\r\\n\\r\\n)\n";
    os << "<<<DUMP END>>>";
    return stringbuf.str();
}

void HttpRequest::clear()
{
    _headers.clear();

    _body = "";
    _headers_string = "";

    _headers_ready = false;
    _request_valid = false;
    _chunked = false;
    _chunk_size = 0;

}

void HttpRequest::processChunked(const std::string &str, std::string::size_type start_block)
{
    // <chunk-size-in-hex><CRLF><content><CRLF>
    // 0<CRLF><CRLF>

    std::string::size_type begin_content = 0;
    std::string::size_type end_content = _chunk_size;

    if (_chunk_size == 0)
    {
        std::string::size_type start_chunk = str.find("\r\n", start_block);
        if (start_chunk == std::string::npos)
        {
            throw std::runtime_error("bad chunked format (broken hex length)");
        }

        std::string chunk_sz = str.substr(start_block, start_chunk - start_block);
        _chunk_size = std::stoi(chunk_sz, nullptr, 16);

        begin_content = start_chunk + 2;    // + "\r\n"
        end_content = begin_content + _chunk_size;
    }

    size_t total_left_len = str.size() - begin_content;

    if (_chunk_size >= total_left_len)
    {
        _body.append(str.begin() + begin_content, str.end());

        // need read more
        _chunk_size -= (str.size() - begin_content);
        return;
    }

    while (true)
    {
        std::string t1(str.begin() + begin_content, str.begin() + begin_content + 5);

        _body.append(str.begin() + begin_content, str.begin() + end_content);

        std::string test(str.begin() + end_content, str.begin() + end_content + 2);
        if (test != "\r\n")
        {
            // TODO: !!!
            loge("bad chunked format (broken crlf). test: ", test);
            sleep(2);
            throw std::runtime_error("bad chunked format (broken crlf)");
            //_chunk_size -= (end_content - begin_content);
            //return;
        }

        // start_next_block помещаем перед длиной
        size_t start_next_block = end_content + 2;

        // start_new_chunk помещаем сразу после длины
        size_t start_new_chunk = str.find("\r\n", start_next_block);

        if (start_new_chunk == std::string::npos)
        {
            // wait for new chunk_size in next packet
            _chunk_size = 0;
            return;
        }

        std::string chunk_sz = str.substr(start_next_block, start_new_chunk - start_next_block);
        _chunk_size = std::stoi(chunk_sz, nullptr, 16);

        if (_chunk_size == 0)
        {
            _request_valid = true;
            return;
        }

        start_new_chunk += 2;   // +\r\n
        total_left_len = str.size() - start_new_chunk;

        if (_chunk_size < total_left_len)
        {
            begin_content = start_new_chunk;
            end_content = start_new_chunk + _chunk_size;
            continue;
        }

        _body.append(str.begin() + start_new_chunk, str.end());

        // need read more
        _chunk_size -= total_left_len;
        break;
    }
}


void HttpRequest::append(const std::string &str)
{
    if (_headers_ready)
    {
        if (_chunked)
        {
            processChunked(str, 0);
            return;
        }

        _body.append(str);
        if (_body.size() == _headers._content_len)
        {
            _request_valid = true;
            return;
        }

        // need more bytes
        return;
    }
    else
    {
        _headers_string.append(str);
    }

    size_t pos = _headers_string.find("\r\n\r\n");
    if (pos == std::string::npos)
    {
        // need more bytes
        return;
    }

    std::string headers = _headers_string.substr(0, pos);
    _headers.fromString(headers);
    _headers_ready = true;

    std::string transfer_enc = _headers.header("Transfer-Encoding");
    if (transfer_enc == "chunked")
    {
        _chunked = true;
        processChunked(str, pos + 4);
        return;
    }

    if (_headers._content_len == 0)
    {
        _request_valid = true;
        return;
    }

    _body.append(str.begin() + pos + /*\r\n\r\n*/4, str.end());

    if (_body.size() == _headers._content_len)
    {
        _request_valid = true;
        return;
    }

    // need more bytes
}
