#include <stdlib.h>
#include <defines.h>

enum http_request
{
    HTTP_REQUEST_GET,
    HTTP_REQUEST_POST,
    HTTP_REQUEST_PUT,
    HTTP_REQUEST_PATCH,
    HTTP_REQUEST_DELETE
};

struct http_header
{
    enum http_request method;
    char* url_path;
    char user_agent[512];
    char content_type[512];
};

struct pair_method_str
{
    enum http_request method;
    char* string;
};


struct pair_method_str methods[] = {

    {HTTP_REQUEST_GET, "GET"}, {HTTP_REQUEST_POST, "POST"},
    {HTTP_REQUEST_PUT, "PUT"}, {HTTP_REQUEST_PATCH, "PATCH"},
    {HTTP_REQUEST_DELETE, "DELETE"}

};

