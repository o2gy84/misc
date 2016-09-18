#include <iostream>
#include <sstream>
#include <vector>

#include "utils.hpp"
#include "http.hpp"


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
        std::cerr << "broken header: " << str << "\n";
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


//HttpRequest::HttpRequest(const std::string &str)
//{
    //_request_valid = false;
    //_headers_ready = false;
    //append(str);
//}

std::string HttpRequest::toString() const
{
    std::stringstream ss;
    ss << _headers.toString() << "\r\n\r\n" << _body;
    return ss.str();
}


void HttpRequest::dump() const
{
    std::cerr << "<<<DUMP START>>>\n";
    std::string hdr = _headers.toString();
    std::cerr << hdr;

    if (hdr.back() != '\n')
    {
        std::cerr << "\n";
    }

    std::cerr << "headers: " << hdr.size() << " bytes, body: " << _body.size() 
              << " bytes, total: " << hdr.size() + _body.size() << " bytes\n";
    std::cerr << "<<<DUMP END>>>\n";
}

void HttpRequest::clear()
{
    _headers.clear();

    _content = "";
    _body = "";

    _headers_ready = false;
    _request_valid = false;
    _chunked = false;
    _chunk_size = 0;

}

void HttpRequest::append(const std::string &str)
{
    _content.append(str);

    if (_headers_ready)
    {
        _body.append(str);

        if (_body.size() == _headers._content_len)
        {
            _request_valid = true;
            return;
        }

        // need more bytes
        return;
    }

    size_t pos = _content.find("\r\n\r\n");

    if (pos == std::string::npos)
    {
        // need more bytes
        _content.append(str);
        return;
    }

    std::string headers = _content.substr(0, pos);
    _headers.fromString(headers);
    _headers_ready = true;

    std::string transfer_enc = _headers.header("Transfer-Encoding");
    if (transfer_enc == "chunked")
    {
        _chunked = true;
        size_t start_block = pos + 4;
        size_t start_chunk = _content.find("\r\n", start_block);
        std::string chunk_sz = _content.substr(start_block, start_chunk - start_block);

        _chunk_size = std::stoi(chunk_sz, nullptr, 16);
        _body.append(str.begin() + start_chunk + 2, str.end());
        return;
    }


    std::string content_len = _headers.header("Content-Length");
    _headers._content_len = 0;

    if (content_len.empty())
    {
        _request_valid = true;
        return;
    }
    
    size_t c_len = std::stoi(content_len);
    if (c_len == 0)
    {
        _request_valid = true;
        return;
    }

    _headers._content_len = c_len;

    // need c_len bytes
    _body.append(str.begin() + pos + /*\r\n\r\n*/4, str.end());

    if (_body.size() == _headers._content_len)
    {
        _request_valid = true;
        return;
    }

    // need more bytes
}
