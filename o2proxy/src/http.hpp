#include <iostream>
#include <string>
#include <map>

struct HttpHeaders
{
    void fromString(const std::string &str);
    std::string toString() const;
    void parseFirstRequestLine(const std::string &str);
    std::string header(const std::string &hdr) const
    {
        auto it = _headers.find(hdr);
        if (it != _headers.end())
        {
            return it->second;
        }
        return "";
    }

    bool hasHeader(const std::string &hdr) const { return !header(hdr).empty(); }
    void clear();


    std::map<std::string, std::string> _headers;
    std::string _method;
    std::string _resource;
    std::string _version;

    size_t _content_len;
};

struct HttpRequest
{
    HttpRequest();
    const HttpHeaders& headers() const { return _headers; }

    void append(const std::string &str);
    bool valid() const { return _request_valid; }
    void clear();

    std::string dump() const;       // debug
    std::string toString() const;   // request as string

private:
    void processChunked(const std::string &str, std::string::size_type start_block);

private:
    HttpHeaders _headers;
    std::string _body;

    bool _request_valid;
    bool _chunked;
    bool _headers_ready;
    size_t _chunk_size;
    std::string _headers_string;
};
