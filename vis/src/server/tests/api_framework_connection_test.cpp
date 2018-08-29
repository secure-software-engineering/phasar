#include <iostream>
#include <string>
#include <curl/curl.h>
#include <stdlib.h>
#include <stdio.h>
#include "json.hpp"
#include <sstream>
//clang++ -o test api_framework_connection_test.cpp -lcurl -I ./ -std=c++14
using json = nlohmann::json;
using namespace std;
size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
    ((std::string *)userp)->append((char *)contents, size * nmemb);
    return size * nmemb;
}
CURL *curl;
string getIdFromWebserver()
{
    CURLcode res;
    std::string readBuffer;

    curl = curl_easy_init();
    if (curl)
    {
        curl_easy_setopt(curl, CURLOPT_URL, "http://localhost:3000/api/framework/getId");
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
        res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);

        std::cout << readBuffer << std::endl;
        auto response = json::parse(readBuffer);
        std::cout << response["my_id"] << std::endl;
        return response["my_id"];
    }
    return 0;
}

void sendToWebserver(const char *jsonString)
{
    if (curl)
    {
        printf("Json String: %s \n", jsonString);
        //setting correct headers so that the server will interpret
        //the post body as json
        struct curl_slist *headers = NULL;
        headers = curl_slist_append(headers, "Accept: application/json");
        headers = curl_slist_append(headers, "Content-Type: application/json");
        headers = curl_slist_append(headers, "charsets: utf-8");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        /* pass in a pointer to the data - libcurl will not copy */
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, jsonString);
        /* Perform the request, res will get the return code */
        CURLcode res = curl_easy_perform(curl);
        /* Check for errors */
        if (res != CURLE_OK)
        {
            fprintf(stderr, "curl_easy_perform() failed: %s\n",
                    curl_easy_strerror(res));
        }
    }
}
void sendWebserverFinish(const char *url)
{

    curl = curl_easy_init();
    if (curl)
    {
        curl_easy_setopt(curl, CURLOPT_URL, url);

        CURLcode res = curl_easy_perform(curl);
        /* Check for errors */
        if (res != CURLE_OK)
        {
            fprintf(stderr, "curl_easy_perform() failed: %s\n",
                    curl_easy_strerror(res));
        }

        curl_easy_cleanup(curl);
    }
}
int main(void)
{
    string id = getIdFromWebserver();
    std::cout << id << std::endl;
    /* get a curl handle */
    curl = curl_easy_init();
    string url = "http://localhost:3000/api/framework/addGraph/" + id;
    cout << url << endl;
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    sendToWebserver("{\"hello\" : \"world\"}");
    url = "http://localhost:3000/api/framework/graphFinish/" + id;
    sendWebserverFinish(url.c_str());
    return 0;
}