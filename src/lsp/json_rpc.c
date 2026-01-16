
#include "json_rpc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Basic JSON parsing helpers
char *get_json_string(const char *json, const char *key)
{
    char search[256];
    sprintf(search, "\"%s\":\"", key);
    char *p = strstr(json, search);
    if (!p)
    {
        return NULL;
    }
    p += strlen(search);
    char *end = strchr(p, '"');
    if (!end)
    {
        return NULL;
    }
    int len = end - p;
    char *res = malloc(len + 1);
    strncpy(res, p, len);
    res[len] = 0;
    return res;
}

// Extract nested "text" from params/contentChanges/0/text or
// params/textDocument/text This is very hacky for MVP. proper JSON library
// needed.

char *get_text_content(const char *json)
{
    char *p = strstr(json, "\"text\":\"");
    if (!p)
    {
        return NULL;
    }
    p += 8;

    size_t cap = strlen(p);
    char *res = malloc(cap + 1);
    char *dst = res;

    while (*p)
    {
        if (*p == '\\')
        {
            p++;
            if (*p == 'n')
            {
                *dst++ = '\n';
            }
            else if (*p == 'r')
            {
                *dst++ = '\r';
            }
            else if (*p == 't')
            {
                *dst++ = '\t';
            }
            else if (*p == '"')
            {
                *dst++ = '"';
            }
            else if (*p == '\\')
            {
                *dst++ = '\\';
            }
            else
            {
                *dst++ = *p; // preserve others
            }
            p++;
        }
        else if (*p == '"')
        {
            break; // End of string.
        }
        else
        {
            *dst++ = *p++;
        }
    }
    *dst = 0;
    return res;
}

// Helper to get line/char
void get_json_position(const char *json, int *line, int *col)
{
    char *pos = strstr(json, "\"position\":");
    if (!pos)
    {
        return;
    }
    char *l = strstr(pos, "\"line\":");
    if (l)
    {
        *line = atoi(l + 7);
    }
    char *c = strstr(pos, "\"character\":");
    if (c)
    {
        *col = atoi(c + 12);
    }
}

void lsp_check_file(const char *uri, const char *src);
void lsp_goto_definition(const char *uri, int line, int col);
void lsp_hover(const char *uri, int line, int col);
void lsp_completion(const char *uri, int line, int col);

void handle_request(const char *json_str)
{
    if (strstr(json_str, "\"method\":\"initialize\""))
    {
        const char *response = "{\"jsonrpc\":\"2.0\",\"id\":1,\"result\":{"
                               "\"capabilities\":{\"textDocumentSync\":1,"
                               "\"definitionProvider\":true,\"hoverProvider\":true,"
                               "\"completionProvider\":{"
                               "\"triggerCharacters\":[\".\"]}}}}";
        fprintf(stdout, "Content-Length: %ld\r\n\r\n%s", strlen(response), response);
        fflush(stdout);
        return;
    }

    if (strstr(json_str, "\"method\":\"textDocument/didOpen\"") ||
        strstr(json_str, "\"method\":\"textDocument/didChange\""))
    {

        char *uri = get_json_string(json_str, "uri");
        char *text = get_text_content(json_str);

        if (uri && text)
        {
            fprintf(stderr, "zls: Checking %s\n", uri);
            lsp_check_file(uri, text);
        }

        if (uri)
        {
            free(uri);
        }
        if (text)
        {
            free(text);
        }
    }

    if (strstr(json_str, "\"method\":\"textDocument/definition\""))
    {
        char *uri = get_json_string(json_str, "uri");
        int line = 0, col = 0;
        get_json_position(json_str, &line, &col);

        if (uri)
        {
            fprintf(stderr, "zls: Definition request at %d:%d\n", line, col);
            lsp_goto_definition(uri, line, col);
            free(uri);
        }
    }

    if (strstr(json_str, "\"method\":\"textDocument/hover\""))
    {
        char *uri = get_json_string(json_str, "uri");
        int line = 0, col = 0;
        get_json_position(json_str, &line, &col);

        if (uri)
        {
            fprintf(stderr, "zls: Hover request at %d:%d\n", line, col);
            lsp_hover(uri, line, col);
            free(uri);
        }
    }

    if (strstr(json_str, "\"method\":\"textDocument/completion\""))
    {
        char *uri = get_json_string(json_str, "uri");
        int line = 0, col = 0;
        get_json_position(json_str, &line, &col);

        if (uri)
        {
            fprintf(stderr, "zls: Completion request at %d:%d\n", line, col);
            lsp_completion(uri, line, col);
            free(uri);
        }
    }
}
